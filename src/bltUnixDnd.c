/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */


/*
 *  bltUnixDnd.c --
 *
 * This module implements a drag-and-drop manager for the BLT Toolkit.
 * Allows widgets to be registered as drag&drop sources and targets
 * for handling "drag-and-drop" operations between Tcl/Tk
 * applications.
 *
 *	Copyright 1995-2004 George A Howlett.
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
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_DRAGDROP

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include <X11/Xatom.h>
#include <X11/Xproto.h>

#include "bltAlloc.h"
#include "bltMath.h"
#include <bltHash.h>
#include <bltChain.h>
#include "bltOp.h"
#include "bltInitCmd.h"

#define DND_THREAD_KEY	"BLT Dnd Data"

#define DND_SELECTED	(1<<0)
#define DND_INITIATED	(1<<1)
#define DND_ACTIVE	(DND_SELECTED | DND_INITIATED)
#define DND_IN_PACKAGE  (1<<2)	/* Indicates if a token package command is
				 * currently active. The user may invoke
				 * "update" or "tkwait" commands from within
				 * the package command script. This allows the
				 * "drag" operation to preempt itself. */
#define DND_VOIDED	(1<<3)
#define DND_DELETED	(1<<4)

#define PACK(lo,hi)	(((hi) << 16) | ((lo) & 0x0000FFFF))
#define UNPACK(x,lo,hi) ((lo) = (x & 0x0000FFFF), (hi) = (x >> 16))

#define WATCH_ENTER	(1<<0)
#define WATCH_LEAVE	(1<<1)
#define WATCH_MOTION	(1<<2)
#define WATCH_MASK	(WATCH_ENTER | WATCH_LEAVE | WATCH_MOTION)

/* Source-to-Target Message Types */

#define ST_DRAG_ENTER	0x1001
#define ST_DRAG_LEAVE	0x1002
#define ST_DRAG_MOTION	0x1003
#define ST_DROP		0x1004

/* Target-to-Source Message Types */

#define TS_DRAG_STATUS	0x1005
#define TS_START_DROP	0x1006
#define TS_DROP_RESULT	0x1007

/* Indices of information fields in ClientMessage array. */

#define MESG_TYPE	0	/* Message type. */
#define MESG_WINDOW	1	/* Window id of remote. */
#define MESG_TIMESTAMP	2	/* Transaction timestamp. */
#define MESG_POINT	3	/* Root X-Y coordinate. */
#define MESG_STATE	4	/* Button and key state. */
#define MESG_RESPONSE	3	/* Response to drag/drop message. */
#define MESG_FORMAT	3	/* Format atom. */
#define MESG_PROPERTY	4	/* Index of button #/key state. */

/* Drop Status Values (actions included) */

#define DROP_CONTINUE	-2
#define DROP_FAIL	-1
#define DROP_CANCEL	0
#define DROP_OK		1
#define DROP_COPY	1
#define DROP_LINK	2
#define DROP_MOVE	3

#define PROP_WATCH_FLAGS	0
#define PROP_DATA_FORMATS	1
#define PROP_MAX_SIZE		1000	/* Maximum size of property. */

#define PROTO_BLT	0
#define PROTO_XDND	1

#define TOKEN_OFFSET	0
#define TOKEN_REDRAW	(1<<0)

#define TOKEN_NORMAL	0
#define TOKEN_REJECT	-1
#define TOKEN_ACTIVE	1
#define TOKEN_TIMEOUT	5000	/* 5 second timeout for drop requests. */

/*
 *   Each widget representing a drag & drop target is tagged with 
 *   a "BltDndTarget" property in XA_STRING format.  This property 
 *   identifies the window as a target.  It's formated as a TCL list 
 *   and contains the following information:
 *
 *	"flags DATA_TYPE DATA_TYPE ..."
 *
 *	"INTERP_NAME TARGET_NAME WINDOW_ID DATA_TYPE DATA_TYPE ..."
 *
 *	INTERP_NAME	Name of the target application's interpreter.
 *	TARGET_NAME	Path name of widget registered as the drop target.  
 *	WINDOW_ID	Window Id of the target's communication window. 
 *			Used to forward Enter/Leave/Motion event information 
 *			to the target.
 *	DATA_TYPE	One or more "types" handled by the target.
 *
 *   When the user invokes the "drag" operation, the window hierarchy
 *   is progressively examined.  Window information is cached during
 *   the operation, to minimize X server traffic. Windows carrying a
 *   "BltDndTarget" property are identified.  When the token is dropped 
 *   over a valid site, the drop information is sent to the application 
 *   via the usual "send" command.  If communication fails, the drag&drop 
 *   facility automatically posts a rejection symbol on the token window.  
 */

/* 
 * Drop Protocol:
 *
 *		Source				Target
 *              ------                          ------
 *   ButtonRelease-? event.
 *   Invokes blt::dnd drop
 *		   +
 *   Send "drop" message to target (via 
 *   ClientMessage). Contains X-Y, key/ --> Gets "drop" message. 
 *   button state, source window XID.       Invokes LeaveCmd proc.
 *					    Gets property from source of 
 *					    ordered matching formats.  
 *					            +
 *					    Invokes DropCmd proc. Arguments
 *					    are X-Y coordinate, key/button 
 *					    state, transaction timestamp, 
 *					    list of matching formats.  
 *						    +
 *					    Target selects format and invokes
 *					    blt::dnd pull to transfer the data
 *					    in the selected format.
 *						    +
 *					    Sends "drop start" message to 
 *					    source.  Contains selected format
 *   Gets "drop start" message.		<-- (as atom), ?action?, target window
 *   Invokes data handler for the     	    ID, transaction timestamp. 
 *   selected format.				    +
 *                +			    Waits for property to change on
 *   Places first packet of data in         its window.  Time out set for
 *   property on target window.         --> no response.
 *                +                                 +
 *   Waits for response property            After each packet, sets zero-length
 *   change. Time out set for no resp.  <-- property on source window.
 *   If non-zero length packet, error               +
 *   occurred, packet is error message.     Sends "drop finished" message.
 *					    Contains transaction timestamp, 
 *   Gets "drop finished" message.      <-- status, ?action?.
 *   Invokes FinishCmd proc. 
 */

/* Configuration Parameters */

#define DEF_DND_BUTTON_BACKGROUND		RGB_YELLOW
#define DEF_DND_BUTTON_BG_MONO		STD_NORMAL_BG_MONO
#define DEF_DND_BUTTON_NUMBER		"3"
#define DEF_DND_ENTER_COMMAND		(char *)NULL
#define DEF_DND_LEAVE_COMMAND		(char *)NULL
#define DEF_DND_MOTION_COMMAND		(char *)NULL
#define DEF_DND_DROP_COMMAND		(char *)NULL
#define DEF_DND_RESULT_COMMAND		(char *)NULL
#define DEF_DND_PACKAGE_COMMAND		(char *)NULL
#define DEF_DND_SELF_TARGET		"no"
#define DEF_DND_SEND			(char *)NULL
#define DEF_DND_IS_TARGET		"no"
#define DEF_DND_IS_SOURCE		"no"
#define DEF_DND_SITE_COMMAND		(char *)NULL

#define DEF_DND_DRAG_THRESHOLD		"0"
#define DEF_TOKEN_ACTIVE_BACKGROUND	STD_ACTIVE_BACKGROUND
#define DEF_TOKEN_ACTIVE_BG_MONO	STD_ACTIVE_BG_MONO
#define DEF_TOKEN_ACTIVE_BORDERWIDTH	"3"
#define DEF_TOKEN_ACTIVE_RELIEF		"sunken"
#define DEF_TOKEN_ANCHOR		"se"
#define DEF_TOKEN_BACKGROUND		STD_NORMAL_BACKGROUND
#define DEF_TOKEN_BG_MONO		STD_NORMAL_BG_MONO
#define DEF_TOKEN_BORDERWIDTH		"3"
#define DEF_TOKEN_CURSOR		"top_left_arrow"
#define DEF_TOKEN_REJECT_BACKGROUND	STD_NORMAL_BACKGROUND
#define DEF_TOKEN_REJECT_BG_MONO	RGB_WHITE
#define DEF_TOKEN_REJECT_FOREGROUND	RGB_RED
#define DEF_TOKEN_REJECT_FG_MONO	RGB_BLACK
#define DEF_TOKEN_REJECT_STIPPLE_COLOR	(char *)NULL
#define DEF_TOKEN_REJECT_STIPPLE_MONO	RGB_GREY50
#define DEF_TOKEN_RELIEF		"raised"

static Blt_OptionFreeProc FreeCursors;
static Blt_OptionParseProc ObjToCursors;
static Blt_OptionPrintProc CursorsToObj;
static Blt_CustomOption cursorsOption =
{
    ObjToCursors, CursorsToObj, FreeCursors, (ClientData)0
};

typedef struct {
    Blt_HashTable dndTable;	/* Hash table of dnd structures keyed by 
				 * the address of the reference Tk window */
    Tk_Window tkMain;
    Display *display;
    Atom mesgAtom;		/* Atom signifying a drag-and-drop message. */
    Atom formatsAtom;		/* Source formats property atom.  */
    Atom targetAtom;		/* Target property atom. */
    Atom commAtom;		/* Communication property atom. */

#ifdef HAVE_XDND
    Blt_HashTable handlerTable; /* Table of toplevel windows with XdndAware 
				 * properties attached to them. */
    Atom XdndActionListAtom;
    Atom XdndAwareAtom;
    Atom XdndEnterAtom;
    Atom XdndFinishedAtom;
    Atom XdndLeaveAtom;
    Atom XdndPositionAtom;
    Atom XdndSelectionAtom;
    Atom XdndStatusAtom;
    Atom XdndTypeListAtom;

    Atom XdndActionCopyAtom;
    Atom XdndActionMoveAtom;
    Atom XdndActionLinkAtom;
    Atom XdndActionAskAtom;
    Atom XdndActionPrivateAtom;
    Atom XdndActionDescriptionAtom;
#endif
} DndInterpData;


typedef struct {
    Tcl_DString ds;
    Window window;		/* Source/Target window */
    Display *display;
    Atom commAtom;		/* Data communication property atom. */
    int packetSize;
    Tcl_TimerToken timerToken;
    int status;			/* Status of transaction:  CONTINUE, OK, FAIL,
				 * or TIMEOUT. */
    int timestamp;		/* Timestamp of the transaction. */
    int offset;
    int protocol;		/* Drag-and-drop protocol used by the source:
				 * either PROTO_BLT or PROTO_XDND. */
} DropPending;

/* 
 * SubstDescriptors --
 *
 *	Structure to hold letter-value pairs for percent substitutions.
 */
typedef struct {
    char letter;		/* character like 'x' in "%x" */
    const char *value;		/* value to be substituted in place of "%x" */
} SubstDescriptors;

/*
 *  Drag&Drop Registration Data
 */
typedef struct {
    Tk_Window tkwin;		/* Window that embodies the token.  NULL
				 * means that the window has been destroyed
				 * but the data structures haven't yet been
				 * cleaned up. */

    Display *display;		/* Display containing widget.  Used, among
				 * other things, so that resources can be
				 * freed even after tkwin has gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with widget.  Used
				 * to delete widget command. */
    Tk_3DBorder border;		/* Structure used to draw 3-D border and
				 * background.  NULL means no background
				 * or border. */
    int borderWidth;		/* Width of 3-D border (if any). */
    int relief;			/* 3-d effect: TK_RELIEF_RAISED etc. */

    int flags;			/* Various flags;  see below for
				 * definitions. */

    /* Token specific fields */
    int x, y;			/* Last position of token window */
    int startX, startY;

    int status;			/* Indicates the current status of the token:
				 * 0 is normal, 1 is active. */
    int lastStatus;		/* Indicates the last status of the token. */
    Tcl_TimerToken timerToken;	/* Token for routine to hide tokenwin */
    GC fillGC;			/* GC used to draw rejection fg: (\) */
    GC outlineGC;		/* GC used to draw rejection bg: (\) */
    int width, height;

    /* User-configurable fields */

    Tk_Anchor anchor;		/* Position of token win relative to mouse */
    Tk_3DBorder normalBorder;	/* Border/background for token window */
    Tk_3DBorder activeBorder;	/* Border/background for token window */
    int activeRelief;
    int activeBW;	/* Border width in pixels */
    XColor *fillColor;		/* Color used to draw rejection fg: (\) */
    XColor *outlineColor;	/* Color used to draw rejection bg: (\) */
    Pixmap rejectStipple;	/* Stipple used to draw rejection: (\) */
    int reqWidth, reqHeight;

    int numSteps;

} Token;

/*
 *  Winfo --
 *
 *	This structure represents a window hierarchy examined during a single
 *	"drag" operation.  It's used to cache information to reduce the round
 *	trip calls to the server needed to query window geometry information
 *	and grab the target property.  
 */
typedef struct _Winfo {
    Window window;		/* Window in hierarchy. */
    
    int initialized;		/* If zero, the rest of this structure's
				 * information hasn't been set. */
    
    int x1, y1, x2, y2;		/* Extents of the window (upper-left and
				 * lower-right corners). */
    
    struct _Winfo *parentPtr;	/* Parent node. NULL if root. Used to
				 * compute offset for X11 windows. */
    
    Blt_Chain chain;		/* List of this window's children. If NULL,
				 * there are no children. */
    
    int isTarget;		/* Indicates if this window is a drag&drop
				 * target. */
    int lookedForProperty;	/* Indicates if this window  */
    
    int eventFlags;		/* Retrieved from the target's drag&drop 
				 * property, indicates what kinds of pointer
				 * events should be relayed to the target via
				 * ClientMessages. Possible values are OR-ed 
				 * combinations of the following bits: 
				 *	001 Enter events.  
				 *	010 Motion events.
				 *	100 Leave events.  
				 */
    const char *matches;
    
} Winfo;

/*
 *  Dnd --
 *
 *	This structure represents the drag&drop manager.  It is associated
 *	with a widget as a drag&drop source, target, or both.  It contains
 *	both the source and target components, since a widget can be both 
 *	a drag source and a drop target.  
 */
typedef struct {
    Tcl_Interp *interp;		/* Interpreter associated with the drag&drop
				 * manager. */
    
    Tk_Window tkwin;		/* Tk window representing the drag&drop 
				 * manager (can be source and/or target). */

    Display *display;		/* Display for drag&drop widget. Saved to free
				 * resources after window has been destroyed. */

    int isSource;		/* Indicates if this drag&drop manager can act
				 * as a drag source. */
    int isTarget;		/* Indicates if this drag&drop manager can act
				 * as a drop target. */

    int targetPropertyExists;	/* Indicates is the drop target property has 
				 * been set. */

    unsigned int flags;		/* Various flags;  see below for
				 * definitions. */
    int timestamp;		/* Id of the current drag&drop transaction. */

    int x, y;			/* Last known location of the mouse pointer. */

    Blt_HashEntry *hashPtr;

    DndInterpData *dataPtr;	

    /* Source component. */
    
    Blt_HashTable getDataTable;	/* Table of data handlers (converters)
				 * registered for this source. */

    int reqButton;		/* Button used to invoke drag operation. */

    int button;			/* Last button press detected. */
    int keyState;		/* Last key state.  */

    Tk_Cursor cursor;		/* Cursor restored after dragging */

    int selfTarget;		/* Indicated if the source should drop onto 
				 * itself. */

    const char **reqFormats;	/* List of requested data formats. The
				 * list should be ordered with the more 
				 * desireable formats first. You can also
				 * temporarily turn off a source by setting 
				 * the value to the empty string. */

    Winfo *rootPtr;		/* Cached window information: Gathered
				 * and used during the "drag" operation 
				 * to see if the mouse pointer is over a 
				 * valid target. */

    Winfo *windowPtr;		/* Points to information about the last 
				 * target the pointer was over. If NULL, 
				 * the pointer was not over a valid target. */

    const char **packageCmd;	/* TCL command executed at start of the drag
				 * operation to initialize token. */

    const char **resultCmd;	/* TCL command executed at the end of the
				 * "drop" operation to indicate its status. */

    const char **siteCmd;	/* TCL command executed to update token 
				 * window. */

    Token *tokenPtr;		/* Token used to provide special cursor. */
    

    Tcl_TimerToken timerToken;

    Tk_Cursor *cursors;		/* Array of drag-and-drop cursors. */
    int cursorPos;

    int dragStart;		/* Minimum number of pixels movement
				 * before B1-Motion is considered to
				 * start dragging. */

    /* Target component. */

    Blt_HashTable setDataTable;	/* Table of data handlers (converters)
				 * registered for this target. */
    const char **enterCmd;	/* TCL proc called when the mouse enters the
				 * target. */
    const char **leaveCmd;	/* TCL proc called when the mouse leaves the
				 * target. */
    const char **motionCmd;	/* TCL proc called when the mouse is moved
				 * over the target. */
    const char **dropCmd;	/* TCL proc called when the mouse button
				 * is released over the target. */

    const char *matchingFormats;  /*  */
    int lastId;			/* The last transaction id used. This is used
				 * to cache the above formats string. */

    DropPending *pendingPtr;	/* Points to structure containing information
				 * about a current drop in progress. If NULL,
				 * no drop is in progress. */

    short int dropX, dropY;	/* Location of the current drop. */
    short int dragX, dragY;	/* Starting position of token window */
} Dnd;


typedef struct {
    Tk_Window tkwin;		/* Toplevel window of the drop target. */
    int refCount;		/* # of targets referencing this structure. */
    Dnd *dndPtr;		/* Last drop target selected.  Used the 
				 * implement Enter/Leave events for targets. 
				 * If NULL, indicates that no drop target was 
				 * previously selected. */
    int lastRepsonse;		/* Indicates what the last response was. */
    Window window;		/* Window id of the top-level window (ie.
				 * the wrapper). */
    const char **formatArr;	/* List of formats available from source. 
				 * Must be pruned down to matching list. */
    DndInterpData *dataPtr;
    int x, y;
    
} XDndHandler;

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_LIST, "-allowformats", "allowFormats", "AllowFormats", 
	DEF_DND_SEND, Blt_Offset(Dnd, reqFormats), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_INT, "-button", "buttonNumber", "ButtonNumber",
	DEF_DND_BUTTON_NUMBER, Blt_Offset(Dnd, reqButton), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-dragthreshold", "dragThreshold", "DragThreshold",
	DEF_DND_DRAG_THRESHOLD, Blt_Offset(Dnd, dragStart), 0},
    {BLT_CONFIG_CUSTOM, "-cursors", "cursors", "cursors", DEF_TOKEN_CURSOR, 
	Blt_Offset(Dnd, cursors), BLT_CONFIG_NULL_OK, &cursorsOption },
    {BLT_CONFIG_LIST, "-onenter", "onEnter", "OnEnter", DEF_DND_ENTER_COMMAND, 
	Blt_Offset(Dnd, enterCmd), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_LIST, "-onmotion", "onMotion", "OnMotion", 
	DEF_DND_MOTION_COMMAND, Blt_Offset(Dnd, motionCmd), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_LIST, "-onleave", "onLeave", "OnLeave", DEF_DND_LEAVE_COMMAND, 
	Blt_Offset(Dnd, leaveCmd), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_LIST, "-ondrop", "onDrop", "OnDrop", DEF_DND_DROP_COMMAND, 
	Blt_Offset(Dnd, dropCmd), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_LIST, "-package", "packageCommand", "PackageCommand",
	DEF_DND_PACKAGE_COMMAND, Blt_Offset(Dnd, packageCmd), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_LIST, "-result", "result", "Result", DEF_DND_RESULT_COMMAND, 
	Blt_Offset(Dnd, resultCmd), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-selftarget", "selfTarget", "SelfTarget",
	DEF_DND_SELF_TARGET, Blt_Offset(Dnd, selfTarget), 0},
    {BLT_CONFIG_LIST, "-site", "siteCommand", "Command", DEF_DND_SITE_COMMAND, 
	Blt_Offset(Dnd, siteCmd), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-source", "source", "Source", DEF_DND_IS_SOURCE, 
	Blt_Offset(Dnd, isSource), 0},
    {BLT_CONFIG_BOOLEAN, "-target", "target", "Target", DEF_DND_IS_TARGET, 
	Blt_Offset(Dnd, isTarget), 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0},
};

static Blt_ConfigSpec tokenConfigSpecs[] =
{
    {BLT_CONFIG_BORDER, "-activebackground", "activeBackground",
	"ActiveBackground", DEF_TOKEN_ACTIVE_BACKGROUND,
	Blt_Offset(Token, activeBorder), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_BORDER, "-activebackground", "activeBackground",
	"ActiveBackground", DEF_TOKEN_ACTIVE_BG_MONO, 
	Blt_Offset(Token, activeBorder), BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "activeRelief",
	DEF_TOKEN_ACTIVE_RELIEF, Blt_Offset(Token, activeRelief), 0},
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor",
	DEF_TOKEN_ANCHOR, Blt_Offset(Token, anchor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-activeborderwidth", "activeBorderWidth",
	"ActiveBorderWidth", DEF_TOKEN_ACTIVE_BORDERWIDTH, 
	Blt_Offset(Token, activeBW), 0},
    {BLT_CONFIG_BORDER, "-background", "background", "Background",
	DEF_TOKEN_BACKGROUND, Blt_Offset(Token, normalBorder),
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_BORDER, "-background", "background", "Background",
	DEF_TOKEN_BG_MONO, Blt_Offset(Token, normalBorder),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_TOKEN_BORDERWIDTH, Blt_Offset(Token, borderWidth), 0},
    {BLT_CONFIG_COLOR, "-outline", "outline", "Outline", 
	DEF_TOKEN_REJECT_BACKGROUND, Blt_Offset(Token, outlineColor), 
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-outline", "outline", "Outline", 
	DEF_TOKEN_REJECT_BG_MONO, Blt_Offset(Token, outlineColor), 
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-fill", "fill", "Fill", DEF_TOKEN_REJECT_FOREGROUND, 
	Blt_Offset(Token, fillColor), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-fill", "fill", "Fill", DEF_TOKEN_REJECT_BACKGROUND, 
	Blt_Offset(Token, fillColor), BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_BITMAP, "-rejectstipple", "rejectStipple", "Stipple",
	DEF_TOKEN_REJECT_STIPPLE_COLOR, Blt_Offset(Token, rejectStipple),
	BLT_CONFIG_COLOR_ONLY | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BITMAP, "-rejectstipple", "rejectStipple", "Stipple",
	DEF_TOKEN_REJECT_STIPPLE_MONO, Blt_Offset(Token, rejectStipple),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_TOKEN_RELIEF, 
	Blt_Offset(Token, relief), 0},
    {BLT_CONFIG_INT, "-width", "width", "Width", (char *)NULL, 
	Blt_Offset(Token, reqWidth), 0},
    {BLT_CONFIG_INT, "-height", "height", "Height", (char *)NULL, 
	Blt_Offset(Token, reqHeight), 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0},
};

/*
 *  Forward Declarations
 */

static Tcl_ObjCmdProc DndCmd;
static Tcl_FreeProc DestroyDnd;
static Tk_GenericProc DndEventProc;
static Tk_EventProc TokenEventProc;
static Tcl_IdleProc DisplayToken;

static Winfo *InitRoot (Dnd *dndPtr);
static Winfo *OverTarget (Dnd *dndPtr);
static int ConfigureToken (Tcl_Interp *interp, Dnd *dndPtr, int objc,	
	Tcl_Obj *const *objv, int flags);
static int GetDndFromObj (ClientData clientData, Tcl_Interp *interp,
	Tcl_Obj *objPtr, Dnd **dndPtrPtr);
static void AddTargetProperty (Dnd *dndPtr);
static void CancelDrag (Dnd *dndPtr);
static void DrawRejectSymbol (Dnd *dndPtr);
static void FreeWinfo (Winfo *wr);
static void GetWinfo (Display *display, Winfo * windowPtr);
static void HideToken (Dnd *dndPtr);
static void MoveToken (Dnd *dndPtr);
static Dnd *CreateDnd (Tcl_Interp *interp, Tk_Window tkwin);

/*ARGSUSED*/
static void
FreeCursors(
    ClientData clientData,	/* Not used. */
    Display *display,		
    char *widgRec,
    int offset)
{
    Tk_Cursor **cursorsPtr = (Tk_Cursor **)(widgRec + offset);

    if (*cursorsPtr != NULL) {
	Tk_Cursor *cp;

	for (cp = *cursorsPtr; *cp != None; cp++) {
	    Tk_FreeCursor(display, *cp);
	}
	Blt_Free(*cursorsPtr);
	*cursorsPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToCursors --
 *
 *	Converts the resize mode into its numeric representation.  Valid
 *	mode strings are "none", "expand", "shrink", or "both".
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToCursors(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* String representing cursors. */
    char *widgRec,		/* Structure record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    int objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc > 0) {
	Tk_Cursor **cursorsPtr = (Tk_Cursor **)(widgRec + offset);
	Tk_Cursor *cursors;
	int i;

	cursors = Blt_AssertCalloc(objc + 1, sizeof(Tk_Cursor));
	for (i = 0; i < objc; i++) {
	    cursors[i] = Tk_AllocCursorFromObj(interp, tkwin, objv[i]);
	    if (cursors[i] == None) {
		Tk_Cursor *cp;

		for (cp = cursors; *cp != None; cp++) {
		    Tk_FreeCursor(Tk_Display(tkwin), *cp);
		}
		return TCL_ERROR;
	    }
	}    
	*cursorsPtr = cursors;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CursorsToObj --
 *
 *	Returns resize mode string based upon the resize flags.
 *
 * Results:
 *	The resize mode string is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
CursorsToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Cursor record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    Tk_Cursor *cursors = *(Tk_Cursor **)(widgRec + offset);
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (cursors != NULL) {
	Tcl_Obj *objPtr;
	Tk_Cursor *cp;

	for (cp = cursors; *cp != NULL; cp++) {
	    objPtr = Tcl_NewStringObj(Tk_NameOfCursor(Tk_Display(tkwin), *cp),
		-1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    }
    return listObjPtr;
}


static Tcl_Obj *
PrintList(Tcl_Interp *interp, const char **list)
{
    Tcl_Obj *listObjPtr;
    const char **p;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for(p = list; *p != NULL; p++) {
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj(*p, -1));
    }
    return listObjPtr;
}


/* ARGSUSED */
static int
XSendEventErrorProc(
    ClientData clientData, 
    XErrorEvent *errEventPtr)
{
    int *errorPtr = clientData;

    *errorPtr = TCL_ERROR;
    return 0;
}

static void
SendClientMsg(
    Display *display,
    Window window,
    Atom mesgAtom,
    int data0, int data1, int data2, int data3, int data4)
{
    XEvent event;
    Tk_ErrorHandler handler;
    int result;
    int any = -1;

    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.display = display;
    event.xclient.window = window;
    event.xclient.message_type = mesgAtom;
    event.xclient.format = 32;
    event.xclient.data.l[0] = data0;
    event.xclient.data.l[1] = data1;
    event.xclient.data.l[2] = data2;
    event.xclient.data.l[3] = data3;
    event.xclient.data.l[4] = data4;

    result = TCL_OK;
    handler = Tk_CreateErrorHandler(display, any, X_SendEvent, any,
	XSendEventErrorProc, &result);
    if (!XSendEvent(display, window, False, ClientMessage, &event)) {
	result = TCL_ERROR;
    }
    Tk_DeleteErrorHandler(handler);
    XSync(display, False);
    if (result != TCL_OK) {
	fprintf(stderr, "XSendEvent response to drop: Protocol failed\n");
    }
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

static int
GetMaxPropertySize(Display *display)
{
    int size;

    size = Blt_MaxRequestSize(display, sizeof(char));
    size -= 32;
    return size;
}

/*
 *---------------------------------------------------------------------------
 *
 *  GetProperty --
 *
 *	Returns the data associated with the named property on the
 *	given window.  All data is assumed to be 8-bit string data.
 *
 * ------------------------------------------------------------------------ 
 */
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
	display,		/* Display of window. */
	window,			/* Window holding the property. */
	atom,			/* Name of property. */
	0,			/* Offset of data (for multiple reads). */
	GetMaxPropertySize(display), /* Maximum number of items to read. */
	False,			/* If true, delete the property. */
	XA_STRING,		/* Desired type of property. */
	&typeAtom,		/* (out) Actual type of the property. */
	&format,		/* (out) Actual format of the property. */
	&numItems,		/* (out) # of items in specified format. */
	&bytesAfter,		/* (out) # of bytes remaining to be read. */
	&data);
    if ((result != Success) || (format != 8) || (typeAtom != XA_STRING)) {
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
 *  SetProperty --
 *
 *	Associates the given data with the a property on a given window.
 *	All data is assumed to be 8-bit string data.
 *
 * ------------------------------------------------------------------------ 
 */
static void
SetProperty(Tk_Window tkwin, Atom atom, const char *data)
{
    XChangeProperty(Tk_Display(tkwin), Tk_WindowId(tkwin), atom, XA_STRING,
	8, PropModeReplace, (unsigned char *)data, strlen(data) + 1);
}

/*
 *---------------------------------------------------------------------------
 *
 *  GetWindowArea --
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
GetWindowArea(Display *display, Winfo *windowPtr)
{
    XWindowAttributes winAttrs;

    if (XGetWindowAttributes(display, windowPtr->window, &winAttrs)) {
	windowPtr->x1 = winAttrs.x;
	windowPtr->y1 = winAttrs.y;
	windowPtr->x2 = winAttrs.x + winAttrs.width - 1;
	windowPtr->y2 = winAttrs.y + winAttrs.height - 1;
    }
    return (winAttrs.map_state == IsViewable);
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
static Winfo *
FindTopWindow(Dnd *dndPtr, int x, int y)
{
    Winfo *rootPtr;
    Blt_ChainLink link;
    Winfo *windowPtr;

    rootPtr = dndPtr->rootPtr;
    if (!rootPtr->initialized) {
	GetWinfo(dndPtr->display, rootPtr);
    }
    if ((x < rootPtr->x1) || (x > rootPtr->x2) ||
	(y < rootPtr->y1) || (y > rootPtr->y2)) {
	return NULL;		/* Point is not over window  */
    }
    windowPtr = rootPtr;

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
	    GetWinfo(dndPtr->display, rootPtr);
	}
	if (rootPtr->window == Blt_GetWindowId(dndPtr->tokenPtr->tkwin)) {
	    continue;		/* Don't examine the token window. */
	}
	if ((x >= rootPtr->x1) && (x <= rootPtr->x2) &&
	    (y >= rootPtr->y1) && (y <= rootPtr->y2)) {
	    /*   
	     * Remember the last window containing the pointer and descend
	     * into its window hierarchy. We'll look for a child that also
	     * contains the pointer.  
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
 *  GetWidgetCursor --
 *
 *	Queries a widget for its current cursor.   The given window
 *	may or may not be a Tk widget that has a -cursor option. 
 *
 *  Results:
 *	Returns the current cursor of the widget.
 *
 * ------------------------------------------------------------------------ 
 */
static Tk_Cursor
GetWidgetCursor(
    Tcl_Interp *interp,		/* Interpreter to evaluate widget command. */
    Tk_Window tkwin)		/* Window of drag&drop source. */
{
    Tk_Cursor cursor;
    Tcl_DString ds, savedResult;

    cursor = None;
    Tcl_DStringInit(&ds);
    Blt_DStringAppendElements(&ds, Tk_PathName(tkwin), "cget", "-cursor",
	      (char *)NULL);
    Tcl_DStringInit(&savedResult);
    Tcl_DStringGetResult(interp, &savedResult);
    if (Tcl_GlobalEval(interp, Tcl_DStringValue(&ds)) == TCL_OK) {
	const char *name;

	name = Tcl_GetStringResult(interp);
	if ((name != NULL) && (name[0] != '\0')) {
	    cursor = Tk_GetCursor(interp, tkwin, Tk_GetUid(name));
	}
    }
    Tcl_DStringResult(interp, &savedResult);
    Tcl_DStringFree(&ds);
    return cursor;
}

/*
 *---------------------------------------------------------------------------
 *
 *  NameOfStatus --
 *
 *	Converts a numeric drop result into its string representation.
 *
 *  Results:
 *	Returns a static string representing the drop result.
 *
 *---------------------------------------------------------------------------
 */
static const char *
NameOfStatus(int status)
{
    switch (status) {
    case DROP_OK:
	return "active";
    case DROP_CONTINUE:
	return "normal";
    case DROP_FAIL:
	return "reject";
    case DROP_CANCEL:
	return "cancel";
    default:
	return "unknown status value";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  NameOfAction --
 *
 *	Converts a numeric drop result into its string representation.
 *
 *  Results:
 *	Returns a static string representing the drop result.
 *
 *---------------------------------------------------------------------------
 */
static const char *
NameOfAction(int action)
{
    switch (action) {
    case DROP_COPY:
	return "copy";
    case DROP_CANCEL:
	return "cancel";
    case DROP_MOVE:
	return "move";
	break;
    case DROP_LINK:
	return "link";
    case DROP_FAIL:
	return "fail";
    default:
	return "unknown action";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  GetAction --
 *
 *	Converts a string to its numeric drop result value.
 *
 *  Results:
 *	Returns the drop result.
 *
 *---------------------------------------------------------------------------
 */
static int
GetAction(const char *string)
{
    char c;

    c = string[0];
    if ((c == 'c') && (strcmp(string, "cancel") == 0)) {
	return DROP_CANCEL;
    } else if ((c == 'f') && (strcmp(string, "fail") == 0)) {
	return DROP_FAIL;
    } else if ((c == 'm') && (strcmp(string, "move") == 0)) {
	return DROP_MOVE;
    } else if ((c == 'l') && (strcmp(string, "link") == 0)) {
	return DROP_LINK;
    } else if ((c == 'c') && (strcmp(string, "copy") == 0)) {
	return DROP_COPY;
    } else {
	return DROP_COPY;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  GetDragResult --
 *
 *	Converts a string to its numeric drag result value.
 *
 *  Results:
 *	Returns the drag result.
 *
 *---------------------------------------------------------------------------
 */
static int
GetDragResult(Tcl_Interp *interp, const char *string)
{
    char c;
    int bool;

    c = string[0];
    if ((c == 'c') && (strcmp(string, "cancel") == 0)) {
	return DROP_CANCEL;
    } else if (Tcl_GetBoolean(interp, string, &bool) != TCL_OK) {
	Tcl_BackgroundError(interp);
	return DROP_CANCEL;
    }
    return bool;
}

static void
AnimateActiveCursor(ClientData clientData)
{
    Dnd *dndPtr = clientData;    
    Tk_Cursor cursor;

    dndPtr->cursorPos++;
    cursor = dndPtr->cursors[dndPtr->cursorPos];
    if (cursor == None) {
	cursor = dndPtr->cursors[1];
	dndPtr->cursorPos = 1;
    }
    Tk_DefineCursor(dndPtr->tkwin, cursor);
    dndPtr->timerToken = Tcl_CreateTimerHandler(100, AnimateActiveCursor,
	    dndPtr);
}

static void
StartActiveCursor(Dnd *dndPtr)
{
    if (dndPtr->timerToken != NULL) {
	Tcl_DeleteTimerHandler(dndPtr->timerToken);
    }
    if (dndPtr->cursors != NULL) {
	Tk_Cursor cursor;

	dndPtr->cursorPos = 1;
	cursor = dndPtr->cursors[1];
	if (cursor != None) {
	    Tk_DefineCursor(dndPtr->tkwin, cursor);
	    dndPtr->timerToken = Tcl_CreateTimerHandler(125, 
		AnimateActiveCursor, dndPtr);
	}
    }
}

static void
StopActiveCursor(Dnd *dndPtr)
{
    if (dndPtr->cursorPos > 0) {
	dndPtr->cursorPos = 0;
    }
    if (dndPtr->cursors != NULL) {
	Tk_DefineCursor(dndPtr->tkwin, dndPtr->cursors[0]);
    }
    if (dndPtr->timerToken != NULL) {
	Tcl_DeleteTimerHandler(dndPtr->timerToken);
	dndPtr->timerToken = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedrawToken --
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
EventuallyRedrawToken(Dnd *dndPtr)
{
    Token *tokenPtr;

    if (dndPtr->tokenPtr == NULL) {
	return;
    }
    tokenPtr = dndPtr->tokenPtr;
    if ((tokenPtr->tkwin != NULL) && (tokenPtr->tkwin != NULL) && 
	!(tokenPtr->flags & TOKEN_REDRAW)) {
	tokenPtr->flags |= TOKEN_REDRAW;
	Tcl_DoWhenIdle(DisplayToken, dndPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  RaiseToken --
 *
 *---------------------------------------------------------------------------
 */
static void
RaiseToken(Dnd *dndPtr)
{
    Token *tokenPtr = dndPtr->tokenPtr;

    if (dndPtr->flags & DND_INITIATED) {
	if ((Tk_Width(tokenPtr->tkwin) != Tk_ReqWidth(tokenPtr->tkwin)) ||
	    (Tk_Height(tokenPtr->tkwin) != Tk_ReqHeight(tokenPtr->tkwin))) {
	    Blt_ResizeToplevelWindow(tokenPtr->tkwin, 
		Tk_ReqWidth(tokenPtr->tkwin),
		Tk_ReqHeight(tokenPtr->tkwin));
	}
	Blt_MapToplevelWindow(tokenPtr->tkwin);
	Blt_RaiseToplevelWindow(tokenPtr->tkwin);
    }
}



/*
 *---------------------------------------------------------------------------
 *
 *  DisplayToken --
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayToken(ClientData clientData)
{
    Dnd *dndPtr = clientData;
    Token *tokenPtr = dndPtr->tokenPtr;
    int relief;
    Tk_3DBorder border;
    int borderWidth;

    tokenPtr->flags &= ~TOKEN_REDRAW;
    if (tokenPtr->status == DROP_OK) {
	relief = tokenPtr->activeRelief;
	border = tokenPtr->activeBorder;
	borderWidth = tokenPtr->activeBW;
	if ((dndPtr->cursors != NULL) && (dndPtr->cursorPos == 0)) {
	    StartActiveCursor(dndPtr);
	}
    } else {
	relief = tokenPtr->relief;
	border = tokenPtr->normalBorder;
	borderWidth = tokenPtr->borderWidth;
	StopActiveCursor(dndPtr);
    } 
    Blt_Fill3DRectangle(tokenPtr->tkwin, Tk_WindowId(tokenPtr->tkwin), border, 
	0, 0, Tk_Width(tokenPtr->tkwin), Tk_Height(tokenPtr->tkwin), 
	borderWidth, relief);
    tokenPtr->lastStatus = tokenPtr->status;
    if (tokenPtr->status == DROP_FAIL) {
	DrawRejectSymbol(dndPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  FadeToken --
 *
 *	Fades the token into the target.
 *
 *---------------------------------------------------------------------------
 */
static void
FadeToken(Dnd *dndPtr)		/* Drag-and-drop manager (source). */
{ 
    Token *tokenPtr = dndPtr->tokenPtr;
    int w, h;
    int dx, dy;
    Window window;

    if (tokenPtr->status == DROP_FAIL) {
	tokenPtr->numSteps = 1;
	return;
    }
    if (tokenPtr->numSteps == 1) {
	HideToken(dndPtr);
	dndPtr->flags &= ~(DND_ACTIVE | DND_VOIDED);
	return;
    }
    if (tokenPtr->timerToken != NULL) {
	Tcl_DeleteTimerHandler(tokenPtr->timerToken);
    }
    tokenPtr->timerToken = Tcl_CreateTimerHandler(10, 
		  (Tcl_TimerProc *)FadeToken, dndPtr);
    tokenPtr->numSteps--;

    w = Tk_ReqWidth(tokenPtr->tkwin) * tokenPtr->numSteps / 10;
    h = Tk_ReqHeight(tokenPtr->tkwin) * tokenPtr->numSteps / 10;
    if (w < 1) {
	w = 1;
    } 
    if (h < 1) {
	h = 1;
    }
    dx = (Tk_ReqWidth(tokenPtr->tkwin) - w) / 2;
    dy = (Tk_ReqHeight(tokenPtr->tkwin) - h) / 2;
    window = Blt_GetWindowId(tokenPtr->tkwin);
    XMoveResizeWindow(dndPtr->display, window, tokenPtr->x + dx, 
	     tokenPtr->y + dy, (unsigned int)w, (unsigned int)h);
    tokenPtr->width = w, tokenPtr->height = h;
}

/*
 *---------------------------------------------------------------------------
 *
 *  SnapToken --
 *
 *	Snaps the token back to the source.
 *
 *---------------------------------------------------------------------------
 */
static void
SnapToken(Dnd *dndPtr)
{ 
    Token *tokenPtr = dndPtr->tokenPtr;

    if (tokenPtr->numSteps == 1) {
	HideToken(dndPtr);
	return;
    }
    if (tokenPtr->timerToken != NULL) {
	Tcl_DeleteTimerHandler(tokenPtr->timerToken);
    }
    tokenPtr->timerToken = Tcl_CreateTimerHandler(10, 
	(Tcl_TimerProc *)SnapToken, dndPtr);
    tokenPtr->numSteps--;
    tokenPtr->x -= (tokenPtr->x - tokenPtr->startX) / tokenPtr->numSteps;
    tokenPtr->y -= (tokenPtr->y - tokenPtr->startY) / tokenPtr->numSteps;
    if ((tokenPtr->x != Tk_X(tokenPtr->tkwin)) || 
	(tokenPtr->y != Tk_Y(tokenPtr->tkwin))) {
	Tk_MoveToplevelWindow(tokenPtr->tkwin, tokenPtr->x, tokenPtr->y);
    }
    RaiseToken(dndPtr);
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
HideToken(Dnd *dndPtr)
{
    Token *tokenPtr = dndPtr->tokenPtr;

    if (tokenPtr->timerToken != NULL) {
	Tcl_DeleteTimerHandler(tokenPtr->timerToken);
	tokenPtr->timerToken = NULL;
    }
    if (dndPtr->flags & DND_INITIATED) {
	/* Reset the cursor back to its normal state.  */
	StopActiveCursor(dndPtr);
	if (dndPtr->cursor == None) {
	    Tk_UndefineCursor(dndPtr->tkwin);
	} else {
	    Tk_DefineCursor(dndPtr->tkwin, dndPtr->cursor);
	}
	if (tokenPtr->tkwin != NULL) {
	    Tk_UnmapWindow(tokenPtr->tkwin); 
	    Blt_ResizeToplevelWindow(tokenPtr->tkwin, 
			Tk_ReqWidth(tokenPtr->tkwin), 
			Tk_ReqHeight(tokenPtr->tkwin));
	}
    }
    if (dndPtr->rootPtr != NULL) {
	FreeWinfo(dndPtr->rootPtr);
	dndPtr->rootPtr = NULL;
    }
    dndPtr->flags &= ~(DND_ACTIVE | DND_VOIDED);
    tokenPtr->status = DROP_CONTINUE;
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 *  MorphToken --
 *
 *	Fades the token into the target.
 *
 *---------------------------------------------------------------------------
 */
static void
MorphToken(Dnd *dndPtr)		/* Drag-and-drop manager (source). */
{ 
    Token *tokenPtr = dndPtr->tokenPtr;

    if (tokenPtr->status == DROP_FAIL) {
	tokenPtr->numSteps = 1;
	return;
    }
    if (tokenPtr->numSteps == 1) {
	HideToken(dndPtr);
	dndPtr->flags &= ~(DND_ACTIVE | DND_VOIDED);
	return;
    }
    if (tokenPtr->timerToken != NULL) {
	Tcl_DeleteTimerHandler(tokenPtr->timerToken);
    }
    tokenPtr->timerToken = Tcl_CreateTimerHandler(10, 
	(Tcl_TimerProc *)MorphToken, dndPtr);
    tokenPtr->numSteps--;

    if (dndPtr->flags & DROP_CANCEL) {
	tokenPtr->numSteps--;
	tokenPtr->x -= (tokenPtr->x - tokenPtr->startX) / tokenPtr->numSteps;
	tokenPtr->y -= (tokenPtr->y - tokenPtr->startY) / tokenPtr->numSteps;
	if ((tokenPtr->x != Tk_X(tokenPtr->tkwin)) || 
	    (tokenPtr->y != Tk_Y(tokenPtr->tkwin))) {
	    Tk_MoveToplevelWindow(tokenPtr->tkwin, tokenPtr->x, tokenPtr->y);
	}
    } else {
	int w, h;
	int dx, dy;
	Window window;

	w = Tk_ReqWidth(tokenPtr->tkwin) * tokenPtr->numSteps / 10;
	h = Tk_ReqHeight(tokenPtr->tkwin) * tokenPtr->numSteps / 10;
	if (w < 1) {
	    w = 1;
	} 
	if (h < 1) {
	    h = 1;
	}
	dx = (Tk_ReqWidth(tokenPtr->tkwin) - w) / 2;
	dy = (Tk_ReqHeight(tokenPtr->tkwin) - h) / 2;
	window = Blt_GetWindowId(tokenPtr->tkwin);
	XMoveResizeWindow(dndPtr->display, window, tokenPtr->x + dx, 
			  tokenPtr->y + dy, (unsigned int)w, (unsigned int)h);
	tokenPtr->width = w, tokenPtr->height = h;
    }
    RaiseToken(dndPtr);
}
#endif

static void
GetTokenPosition(Dnd *dndPtr, int x, int y)
{ 
    Token *tokenPtr = dndPtr->tokenPtr;
    int maxX, maxY;
    int vx, vy, dummy;
    int screenWidth, screenHeight;

    /* Adjust current location for virtual root windows.  */
    Tk_GetVRootGeometry(dndPtr->tkwin, &vx, &vy, &dummy, &dummy);
    x += vx - TOKEN_OFFSET;
    y += vy - TOKEN_OFFSET;

    Blt_SizeOfScreen(tokenPtr->tkwin, &screenWidth, &screenHeight);
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
    tokenPtr->x = x, tokenPtr->y  = y;
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
MoveToken(Dnd *dndPtr)		/* drag&drop source window data */
{ 
    Token *tokenPtr = dndPtr->tokenPtr;

    GetTokenPosition(dndPtr, dndPtr->x, dndPtr->y);
    if ((tokenPtr->x != Tk_X(tokenPtr->tkwin)) || 
	(tokenPtr->y != Tk_Y(tokenPtr->tkwin))) {
	Tk_MoveToplevelWindow(tokenPtr->tkwin, tokenPtr->x, tokenPtr->y);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 *  ChangeToken --
 *
 *	Invoked when the event loop is idle to determine whether or not
 *	the current drag&drop token position is over another drag&drop
 *	target.
 *
 *---------------------------------------------------------------------------
 */
static void
ChangeToken(Dnd *dndPtr, int status)
{
    Token *tokenPtr = dndPtr->tokenPtr;

    tokenPtr->status = status;
    EventuallyRedrawToken(dndPtr);

    /*
     *  If the source has a site command, then invoke it to
     *  modify the appearance of the token window.  Pass any
     *  errors onto the drag&drop error handler.
     */
    if (dndPtr->siteCmd) {
	Tcl_Interp *interp = dndPtr->interp;
	Tcl_DString ds, savedResult;
	const char **p;
	
	Tcl_DStringInit(&ds);
	for (p = dndPtr->siteCmd; *p != NULL; p++) {
	    Tcl_DStringAppendElement(&ds, *p);
	}
	Tcl_DStringAppendElement(&ds, Tk_PathName(dndPtr->tkwin));
	Tcl_DStringAppendElement(&ds, "timestamp");
	Tcl_DStringAppendElement(&ds, Blt_Utoa(dndPtr->timestamp));
	Tcl_DStringAppendElement(&ds, "status");
	Tcl_DStringAppendElement(&ds, NameOfStatus(status));
	Tcl_DStringInit(&savedResult);
	Tcl_DStringGetResult(interp, &savedResult);
	if (Tcl_GlobalEval(interp, Tcl_DStringValue(&ds)) != TCL_OK) {
	    Tcl_BackgroundError(interp);
	}
	Tcl_DStringFree(&ds);
	Tcl_DStringResult(interp, &savedResult);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  DrawRejectSymbol --
 *
 *	Draws a rejection mark on the current drag&drop token, and arranges
 *	for the token to be unmapped after a small delay.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawRejectSymbol(Dnd *dndPtr)
{
    Token *tokenPtr = dndPtr->tokenPtr;
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
    XSetLineAttributes(Tk_Display(tokenPtr->tkwin), tokenPtr->outlineGC,
	lineWidth + 2, LineSolid, CapButt, JoinBevel);

    XDrawArc(Tk_Display(tokenPtr->tkwin), Tk_WindowId(tokenPtr->tkwin),
	tokenPtr->outlineGC, x, y, w, h, 0, 23040);

    XDrawLine(Tk_Display(tokenPtr->tkwin), Tk_WindowId(tokenPtr->tkwin),
	tokenPtr->outlineGC, x + lineWidth, y + lineWidth, x + w - lineWidth,
	y + h - lineWidth);

    /*
     *  Draw the rejection symbol foreground (\) on the token window...
     */
    XSetLineAttributes(Tk_Display(tokenPtr->tkwin), tokenPtr->fillGC,
	lineWidth, LineSolid, CapButt, JoinBevel);

    XDrawArc(Tk_Display(tokenPtr->tkwin), Tk_WindowId(tokenPtr->tkwin),
	tokenPtr->fillGC, x, y, w, h, 0, 23040);

    XDrawLine(Tk_Display(tokenPtr->tkwin), Tk_WindowId(tokenPtr->tkwin),
	tokenPtr->fillGC, x + lineWidth, y + lineWidth, x + w - lineWidth,
	y + h - lineWidth);

    tokenPtr->status = DROP_FAIL;
    /*
     *  Arrange for token window to disappear eventually.
     */
    if (tokenPtr->timerToken != NULL) {
	Tcl_DeleteTimerHandler(tokenPtr->timerToken);
    }
    tokenPtr->timerToken = Tcl_CreateTimerHandler(1000, 
	(Tcl_TimerProc *)HideToken, dndPtr);
    RaiseToken(dndPtr);
    dndPtr->flags &= ~(DND_ACTIVE | DND_VOIDED);
}

/*
 *---------------------------------------------------------------------------
 *
 *  CreateToken --
 *
 *	Looks for a Source record in the hash table for drag&drop source
 *	widgets.  Creates a new record if the widget name is not already
 *	registered.  Returns a pointer to the desired record.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyToken(DestroyData data)
{
    Dnd *dndPtr = (Dnd *)data;
    Token *tokenPtr = dndPtr->tokenPtr;

    dndPtr->tokenPtr = NULL;
    if (tokenPtr == NULL) {
	return;
    }
    if (tokenPtr->flags & TOKEN_REDRAW) {
	Tcl_CancelIdleCall(DisplayToken, dndPtr);
    }
    Blt_FreeOptions(tokenConfigSpecs, (char *)tokenPtr, dndPtr->display, 0);
    if (tokenPtr->timerToken) {
	Tcl_DeleteTimerHandler(tokenPtr->timerToken);
    }
    if (tokenPtr->fillGC != NULL) {
	Tk_FreeGC(dndPtr->display, tokenPtr->fillGC);
    }
    if (tokenPtr->outlineGC != NULL) {
	Tk_FreeGC(dndPtr->display, tokenPtr->outlineGC);
    }
    if (tokenPtr->tkwin != NULL) {
	Tk_DeleteEventHandler(tokenPtr->tkwin, 
	      ExposureMask | StructureNotifyMask, TokenEventProc, dndPtr);
	Tk_DestroyWindow(tokenPtr->tkwin);
    }
    Blt_Free(tokenPtr);
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
TokenEventProc(ClientData clientData, XEvent *eventPtr)
{
    Dnd *dndPtr = clientData;
    Token *tokenPtr = dndPtr->tokenPtr;

    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
	if (tokenPtr->tkwin != NULL) {
	    EventuallyRedrawToken(dndPtr);
	}
    } else if (eventPtr->type == DestroyNotify) {
	tokenPtr->tkwin = NULL;
	if (tokenPtr->flags & TOKEN_REDRAW) {
	    tokenPtr->flags &= ~TOKEN_REDRAW;
	    Tcl_CancelIdleCall(DisplayToken, dndPtr);
	}
	Tcl_EventuallyFree(dndPtr, DestroyToken);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  CreateToken --
 *
 *	Looks for a Source record in the hash table for drag&drop source
 *	widgets.  Creates a new record if the widget name is not already
 *	registered.  Returns a pointer to the desired record.
 *
 *---------------------------------------------------------------------------
 */
static int
CreateToken(
    Tcl_Interp *interp,
    Dnd *dndPtr)
{
    XSetWindowAttributes attrs;
    Tk_Window tkwin;
    unsigned int mask;
    Token *tokenPtr;

    tokenPtr = Blt_AssertCalloc(1, sizeof(Token));
    tokenPtr->anchor = TK_ANCHOR_SE;
    tokenPtr->relief = TK_RELIEF_RAISED;
    tokenPtr->activeRelief = TK_RELIEF_SUNKEN;
    tokenPtr->borderWidth = tokenPtr->activeBW = 3;

    /* Create toplevel on parent's screen. */
    tkwin = Tk_CreateWindow(interp, dndPtr->tkwin, "dndtoken", "");
    if (tkwin == NULL) {
	Blt_Free(tokenPtr);
	return TCL_ERROR;
    }
    tokenPtr->tkwin = tkwin;
    Tk_SetClass(tkwin, "BltDndToken"); 
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask,
	TokenEventProc, dndPtr);
    attrs.override_redirect = True;
    attrs.backing_store = WhenMapped;
    attrs.save_under = True;
    mask = CWOverrideRedirect | CWSaveUnder | CWBackingStore;
    Tk_ChangeWindowAttributes(tkwin, mask, &attrs);
    Tk_SetInternalBorder(tkwin, tokenPtr->borderWidth + 2);
    Tk_MakeWindowExist(tkwin);
    dndPtr->tokenPtr = tokenPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  ConfigureToken --
 *
 *	Called to process an (objc,objv) list to configure (or
 *	reconfigure) a drag&drop source widget.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureToken(
    Tcl_Interp *interp,		/* current interpreter */
    Dnd *dndPtr,		/* Drag&drop source widget record */
    int objc,			/* number of arguments */
    Tcl_Obj *const *objv,		/* argument strings */
    int flags)			/* flags controlling interpretation */
{
    GC newGC;
    Token *tokenPtr = dndPtr->tokenPtr;
    XGCValues gcValues;
    unsigned long gcMask;

    Tk_MakeWindowExist(tokenPtr->tkwin);
    if (Blt_ConfigureWidgetFromObj(interp, tokenPtr->tkwin, tokenConfigSpecs, 
		objc, objv, (char *)tokenPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    /*
     *  Set up the rejection outline GC for the token window...
     */
    gcValues.foreground = tokenPtr->outlineColor->pixel;
    gcValues.subwindow_mode = IncludeInferiors;
    gcValues.graphics_exposures = False;
    gcValues.line_style = LineSolid;
    gcValues.cap_style = CapButt;
    gcValues.join_style = JoinBevel;

    gcMask = GCForeground | GCSubwindowMode | GCLineStyle |
	GCCapStyle | GCJoinStyle | GCGraphicsExposures;
    newGC = Tk_GetGC(dndPtr->tkwin, gcMask, &gcValues);

    if (tokenPtr->outlineGC != NULL) {
	Tk_FreeGC(dndPtr->display, tokenPtr->outlineGC);
    }
    tokenPtr->outlineGC = newGC;

    /*
     *  Set up the rejection fill GC for the token window...
     */
    gcValues.foreground = tokenPtr->fillColor->pixel;
    if (tokenPtr->rejectStipple != None) {
	gcValues.stipple = tokenPtr->rejectStipple;
	gcValues.fill_style = FillStippled;
	gcMask |= GCStipple | GCFillStyle;
    }
    newGC = Tk_GetGC(dndPtr->tkwin, gcMask, &gcValues);

    if (tokenPtr->fillGC != NULL) {
	Tk_FreeGC(dndPtr->display, tokenPtr->fillGC);
    }
    tokenPtr->fillGC = newGC;

    if ((tokenPtr->reqWidth > 0) && (tokenPtr->reqHeight > 0)) {
	Tk_GeometryRequest(tokenPtr->tkwin, tokenPtr->reqWidth, 
		tokenPtr->reqHeight);
    }
    /*
     *  Reset the border width in case it has changed...
     */
    Tk_SetInternalBorder(tokenPtr->tkwin, tokenPtr->borderWidth + 2);
    return TCL_OK;
}

static int
GetFormattedData(
    Dnd *dndPtr, 
    char *format, 
    int timestamp, 
    Tcl_DString *resultPtr)
{
    Tcl_Interp *interp = dndPtr->interp;
    Blt_HashEntry *hPtr;
    char **formatCmd;
    Tcl_DString savedResult;
    Tcl_DString ds;
    char **p;
    int x, y;

    /* Find the data converter for the prescribed format. */
    hPtr = Blt_FindHashEntry(&dndPtr->getDataTable, format);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "can't find format \"", format, 
	 "\" in source \"", Tk_PathName(dndPtr->tkwin), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    formatCmd = Blt_GetHashValue(hPtr);
    Tcl_DStringInit(&ds);
    for (p = formatCmd; *p != NULL; p++) {
	Tcl_DStringAppendElement(&ds, *p);
    }
    x = dndPtr->dragX - Blt_RootX(dndPtr->tkwin);
    y = dndPtr->dragY - Blt_RootY(dndPtr->tkwin);
    Tcl_DStringAppendElement(&ds, Tk_PathName(dndPtr->tkwin));
    Tcl_DStringAppendElement(&ds, "x");
    Tcl_DStringAppendElement(&ds, Blt_Itoa(x));
    Tcl_DStringAppendElement(&ds, "y");
    Tcl_DStringAppendElement(&ds, Blt_Itoa(y));
    Tcl_DStringAppendElement(&ds, "timestamp");
    Tcl_DStringAppendElement(&ds, Blt_Utoa(timestamp));
    Tcl_DStringAppendElement(&ds, "format");
    Tcl_DStringAppendElement(&ds, format);
    Tcl_DStringInit(&savedResult);
    Tcl_DStringGetResult(interp, &savedResult);
    if (Tcl_GlobalEval(interp, Tcl_DStringValue(&ds)) != TCL_OK) {
	Tcl_BackgroundError(interp);
    }
    Tcl_DStringFree(&ds);
    Tcl_DStringInit(resultPtr);
    Tcl_DStringGetResult(interp, resultPtr);

    /* Restore the interpreter result. */
    Tcl_DStringResult(interp, &savedResult);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 *  DestroyDnd --
 *
 *	Free resources allocated for the drag&drop window.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyDnd(DestroyData data)
{
    Dnd *dndPtr = (Dnd *)data;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    char **cmd;

    Blt_FreeOptions(configSpecs, (char *)dndPtr, dndPtr->display, 0);
    Tk_DeleteGenericHandler(DndEventProc, dndPtr);
    for (hPtr = Blt_FirstHashEntry(&dndPtr->getDataTable, &cursor);
	hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	cmd = Blt_GetHashValue(hPtr);
	if (cmd != NULL) {
	    Blt_Free(cmd);
	}
    }
    Blt_DeleteHashTable(&dndPtr->getDataTable);

    for (hPtr = Blt_FirstHashEntry(&dndPtr->setDataTable, &cursor);
	hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	cmd = Blt_GetHashValue(hPtr);
	if (cmd != NULL) {
	    Blt_Free(cmd);
	}
    }
    Blt_DeleteHashTable(&dndPtr->setDataTable);
    if (dndPtr->rootPtr != NULL) {
	FreeWinfo(dndPtr->rootPtr);
    }
    if (dndPtr->cursor != None) {
	Tk_FreeCursor(dndPtr->display, dndPtr->cursor);
    }
    if (dndPtr->reqFormats != NULL) {
	Blt_Free(dndPtr->reqFormats);
    }
    if (dndPtr->matchingFormats != NULL) {
	Blt_Free(dndPtr->matchingFormats);
    }

    /* Now that the various commands are custom list options, we need
     * to explicitly free them. */
    if (dndPtr->motionCmd != NULL) {
	Blt_Free(dndPtr->motionCmd);
    }
    if (dndPtr->leaveCmd != NULL) {
	Blt_Free(dndPtr->leaveCmd);
    }
    if (dndPtr->enterCmd != NULL) {
	Blt_Free(dndPtr->enterCmd);
    }
    if (dndPtr->dropCmd != NULL) {
	Blt_Free(dndPtr->dropCmd);
    }
    if (dndPtr->resultCmd != NULL) {
	Blt_Free(dndPtr->resultCmd);
    }
    if (dndPtr->packageCmd != NULL) {
	Blt_Free(dndPtr->packageCmd);
    }
    if (dndPtr->siteCmd != NULL) {
	Blt_Free(dndPtr->siteCmd);
    }

    if (dndPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&dndPtr->dataPtr->dndTable, dndPtr->hashPtr);
    }
    if (dndPtr->tokenPtr != NULL) {
	DestroyToken((DestroyData)dndPtr);
    }
    if (dndPtr->tkwin != NULL) {
	XDeleteProperty(dndPtr->display, Tk_WindowId(dndPtr->tkwin),
			dndPtr->dataPtr->targetAtom);
	XDeleteProperty(dndPtr->display, Tk_WindowId(dndPtr->tkwin),
			dndPtr->dataPtr->commAtom);
    }
    Blt_Free(dndPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 *  GetDnd --
 *
 *	Looks for a Source record in the hash table for drag&drop source
 *	widgets.  Returns a pointer to the desired record.
 *
 *---------------------------------------------------------------------------
 */
static int
GetDndFromObj(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,		/* widget pathname for desired record */
    Dnd **dndPtrPtr)
{
    DndInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Tk_Window tkwin;
    char *pathName;

    pathName = Tcl_GetString(objPtr);
    assert(interp != NULL);
    tkwin = Tk_NameToWindow(interp, pathName, dataPtr->tkMain);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    hPtr = Blt_FindHashEntry(&dataPtr->dndTable, (char *)tkwin);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "window \"", pathName,
	     "\" is not a drag&drop source/target", (char *)NULL);
	return TCL_ERROR;
    }
    *dndPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  CreateDnd --
 *
 *	Looks for a Source record in the hash table for drag&drop source
 *	widgets.  Creates a new record if the widget name is not already
 *	registered.  Returns a pointer to the desired record.
 *
 *---------------------------------------------------------------------------
 */
static Dnd *
CreateDnd(
    Tcl_Interp *interp,
    Tk_Window tkwin)		/* Widget for desired record */
{
    Dnd *dndPtr;

    dndPtr = Blt_AssertCalloc(1, sizeof(Dnd));
    dndPtr->interp = interp;
    dndPtr->display = Tk_Display(tkwin);
    dndPtr->tkwin = tkwin;
    Tk_MakeWindowExist(tkwin);
    Blt_InitHashTable(&dndPtr->setDataTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&dndPtr->getDataTable, BLT_STRING_KEYS);
    Tk_CreateGenericHandler(DndEventProc, dndPtr);
    return dndPtr;
}

static int
ConfigureDnd(Tcl_Interp *interp, Dnd *dndPtr)
{
    Tcl_DString ds;
    int button, result;

    if (!Blt_CommandExists(interp, "::blt::DndInit")) {
	static char cmd[] = "source [file join $blt_library dnd.tcl]";
	/* 
	 * If the "DndInit" routine hasn't been sourced, do it now.
	 */
	if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
	    Tcl_AddErrorInfo(interp,
		     "\n    (while loading bindings for blt::drag&drop)");
	    return TCL_ERROR;
	}
    }
    /*  
     * Reset the target property if it's changed state or
     * added/subtracted one of its callback procedures.  
     */
    if (Blt_ConfigModified(configSpecs, "-target", "-onenter",  "-onmotion",
	   "-onleave", (char *)NULL)) {
	if (dndPtr->targetPropertyExists) {
	    XDeleteProperty(dndPtr->display, Tk_WindowId(dndPtr->tkwin),
			    dndPtr->dataPtr->targetAtom);
	    dndPtr->targetPropertyExists = FALSE;
	}
	if (dndPtr->isTarget) {
	    AddTargetProperty(dndPtr);
	    dndPtr->targetPropertyExists = TRUE;
	}
    }
    if (dndPtr->isSource) {
	/* Check the button binding for valid range (0 or 1-5) */
	if ((dndPtr->reqButton < 0) || (dndPtr->reqButton > 5)) {
	    Tcl_AppendResult(interp, 
			     "button must be 1-5, or 0 for no bindings",
			     (char *)NULL);
	    return TCL_ERROR;
	}
	button = dndPtr->reqButton;
    } else {
	button = 0;
    }
    Tcl_DStringInit(&ds);
    Blt_DStringAppendElements(&ds, "::blt::DndInit", 
	Tk_PathName(dndPtr->tkwin), Blt_Itoa(button), (char *)NULL);
    result = Tcl_GlobalEval(interp, Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
    if (result != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * SendRestrictProc --
 *
 *	This procedure filters incoming events when a "send" command
 *	is outstanding.  It defers all events except those containing
 *	send commands and results.
 *
 * Results:
 *	False is returned except for property-change events on a
 *	commWindow.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

/* ARGSUSED */
static Tk_RestrictAction
SendRestrictProc(
    ClientData clientData,	/* Drag-and-drop manager. */
    XEvent *eventPtr)		/* Event that just arrived. */
{
    Dnd *dndPtr = clientData;

    if (eventPtr->xproperty.window != Tk_WindowId(dndPtr->tkwin)) {
	return TK_PROCESS_EVENT; /* Event not in our window. */
    }
    if ((eventPtr->type == PropertyNotify) &&
	(eventPtr->xproperty.state == PropertyNewValue)) {
	return TK_PROCESS_EVENT; /* This is the one we want to process. */
    }
    if (eventPtr->type == Expose) {
	return TK_PROCESS_EVENT; /* Let expose events also get
				  * handled. */
    }
    return TK_DEFER_EVENT;	/* Defer everything else. */
}

/*
 *---------------------------------------------------------------------------
 *
 * SendTimerProc --
 *
 *	Procedure called when the timer event elapses.  Used to wait
 *	between attempts checking for the designated window.
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
SendTimerProc(ClientData clientData)
{
    int *statusPtr = clientData;

    /* 
     * An unusually long amount of time has elapsed since the drag
     * start message was sent.  Assume that the other party has died
     * and abort the operation.  
     */
    *statusPtr = DROP_FAIL;
}

#define WAIT_INTERVAL	2000	/* Twenty seconds. */

/*
 *---------------------------------------------------------------------------
 *
 *  TargetPropertyEventProc --
 *
 *	Invoked by the Tk dispatcher to handle widget events.  Manages redraws
 *	for the drag&drop token window.
 *
 *---------------------------------------------------------------------------
 */
static void
TargetPropertyEventProc(
    ClientData clientData,		/* Data associated with transaction. */
    XEvent *eventPtr)			/* information about event */
{
    DropPending *pendingPtr = clientData;
    unsigned char *data;
    int result, format;
    Atom typeAtom;
    unsigned long numItems, bytesAfter;

#ifdef notdef
    fprintf(stderr, "TargetPropertyEventProc\n");
#endif
    if ((eventPtr->type != PropertyNotify) ||
	(eventPtr->xproperty.atom != pendingPtr->commAtom) || 
	(eventPtr->xproperty.state != PropertyNewValue)) {
	return;
    }
    Tcl_DeleteTimerHandler(pendingPtr->timerToken);
    data = NULL;
    result = XGetWindowProperty(
	eventPtr->xproperty.display,	/* Display of window. */
	eventPtr->xproperty.window,     /* Window holding the property. */
	eventPtr->xproperty.atom,	/* Name of property. */
	0,				/* Offset of data (for multiple
					 * reads). */
	pendingPtr->packetSize,		/* Maximum number of items to read. */
	False,				/* If true, delete the property. */
	XA_STRING,			/* Desired type of property. */
	&typeAtom,			/* (out) Actual type of the property. */
	&format,			/* (out) Actual format of the
					 * property. */
	&numItems,			/* (out) # of items in specified
					 * format. */
	&bytesAfter,			/* (out) # of bytes remaining to be
					 * read. */
	&data);
#ifdef notdef
    fprintf(stderr, 
	"TargetPropertyEventProc: result=%d, typeAtom=%d, format=%d, numItems=%d\n",
	    result, typeAtom, format, numItems);
#endif
    pendingPtr->status = DROP_FAIL;
    if ((result == Success) && (typeAtom == XA_STRING) && (format == 8)) {
	pendingPtr->status = DROP_OK;
#ifdef notdef
	fprintf(stderr, "data found is (%s)\n", data);
#endif
	Tcl_DStringAppend(&pendingPtr->ds, (char *)data, -1);
	XFree(data);
	if (numItems == pendingPtr->packetSize) {
	    /* Normally, we'll receive the data in one chunk. But if more are
	     * required, reset the timer and go back into the wait loop
	     * again. */
	    pendingPtr->timerToken = Tcl_CreateTimerHandler(WAIT_INTERVAL, 
		SendTimerProc, &pendingPtr->status);
	    pendingPtr->status = DROP_CONTINUE;
	}
    } 
    /* Set an empty, zero-length value on the source's property. This acts as a
     * handshake, indicating that the target received the latest chunk. */
#ifdef notdef
    fprintf(stderr, "TargetPropertyEventProc: set response property\n");
#endif
    XChangeProperty(pendingPtr->display, pendingPtr->window, 
	pendingPtr->commAtom, XA_STRING, 8, PropModeReplace, 
	(unsigned char *)"", 0);
}

#ifdef HAVE_XDND

static int 
XDndSelectionProc(clientData, interp, portion)
    ClientData clientData;
    Tcl_Interp *interp;
    char *portion;
{
    DropPending *pendingPtr = clientData;

    Tcl_DStringAppend(&pendingPtr->ds, portion, -1);
#ifdef notdef
    fprintf(stderr, "-> XDndGetSelectionProc\n");
#endif
    return TCL_OK;
}

#endif /* HAVE_XDND */

static void
CompleteDataTransaction(Dnd *dndPtr, char *format, DropPending *pendingPtr)
{
    DndInterpData *dataPtr = dndPtr->dataPtr;
    Tk_Window tkwin;
    Atom formatAtom;

#ifdef notdef
    fprintf(stderr, "-> CompleteDataTransaction\n");
#endif
    /* Check if the source is local to the application. */
    tkwin = Tk_IdToWindow(dndPtr->display, pendingPtr->window);
    if (tkwin != NULL) {
	Blt_HashEntry *hPtr;

	hPtr = Blt_FindHashEntry(&dndPtr->dataPtr->dndTable, (char *)tkwin);
	if (hPtr != NULL) {
	    Dnd *srcPtr;

	    srcPtr = Blt_GetHashValue(hPtr);
	    GetFormattedData(srcPtr, format, pendingPtr->timestamp, 
		&pendingPtr->ds);
	}
	return;
    }

    formatAtom = XInternAtom(pendingPtr->display, format, False);

    if (pendingPtr->protocol == PROTO_XDND) { 
#ifdef HAVE_XDND
	if (Tk_GetSelection(dndPtr->interp, dndPtr->tkwin, 
		dataPtr->XdndSelectionAtom, formatAtom, XDndSelectionProc, 
		pendingPtr) != TCL_OK) {
	    pendingPtr->status = DROP_FAIL;
	}
#endif
	pendingPtr->status = DROP_OK;
    } else {
	Tk_RestrictProc *proc;
	ClientData arg;

	SendClientMsg(pendingPtr->display, pendingPtr->window, 
		dataPtr->mesgAtom, 
		TS_START_DROP, 
		(int)Tk_WindowId(dndPtr->tkwin),
		pendingPtr->timestamp, 
		(int)formatAtom, 
		(int)pendingPtr->commAtom);

	pendingPtr->commAtom = dndPtr->dataPtr->commAtom;
	pendingPtr->status = DROP_CONTINUE;
	pendingPtr->display = dndPtr->display;
	proc = Tk_RestrictEvents(SendRestrictProc, dndPtr, &arg);
	Tk_CreateEventHandler(dndPtr->tkwin, PropertyChangeMask, 
		TargetPropertyEventProc, pendingPtr);
	pendingPtr->timerToken = Tcl_CreateTimerHandler(WAIT_INTERVAL, 
		SendTimerProc, &pendingPtr->status);
	/*  
	 * Enter a loop processing X events until the all the data is received
	 * or the source is declared to be dead (i.e. we timeout).  While
	 * waiting for a result, restrict handling to just property-related
	 * events so that the transfer is synchronous with respect to other
	 * events in the widget.
	 */
	while (pendingPtr->status == DROP_CONTINUE) { 
	    Tcl_DoOneEvent(TCL_ALL_EVENTS);	/* Wait for property event. */
	}
	Tk_RestrictEvents(proc, arg, &arg);
	Tcl_DeleteTimerHandler(pendingPtr->timerToken);
	Tk_DeleteEventHandler(dndPtr->tkwin, PropertyChangeMask, 
	      TargetPropertyEventProc, pendingPtr);
    }
#ifdef notdef
    fprintf(stderr, "<- CompleteDataTransaction\n");
#endif
}

/*
 *---------------------------------------------------------------------------
 *
 *  SourcePropertyEventProc --
 *
 *	Invoked by the Tk dispatcher when a PropertyNotify event occurs on the
 *	source window.  The event acts as a handshake between the target and the
 *	source.  The source acknowledges the target has received the last packet
 *	of data and sends the next packet.
 *
 *	Note a special case.  If the data is divisible by the packetsize, then
 *	an extra zero-length packet is sent to mark the end of the data.  A
 *	packetsize length packet indicates more is to follow.
 *
 *	Normally the property is empty (zero-length).  But if an errored
 *	occurred on the target, it will contain the error message.
 *
 * ------------------------------------------------------------------------ 
 */
static void
SourcePropertyEventProc(
    ClientData clientData,		/* data associated with widget */
    XEvent *eventPtr)			/* information about event */
{
    DropPending *pendingPtr = clientData;
    unsigned char *data;
    int result, format;
    Atom typeAtom;
    unsigned long numItems, bytesAfter;
    int size, bytesLeft;
    unsigned char *p;

#ifdef notdef
    fprintf(stderr, "-> SourcePropertyEventProc\n");
#endif
    if ((eventPtr->xproperty.atom != pendingPtr->commAtom)
	|| (eventPtr->xproperty.state != PropertyNewValue)) {
	return;
    }
    Tcl_DeleteTimerHandler(pendingPtr->timerToken);
    data = NULL;
    result = XGetWindowProperty(
	eventPtr->xproperty.display,	/* Display of window. */
	eventPtr->xproperty.window,     /* Window holding the property. */
	eventPtr->xproperty.atom,	/* Name of property. */
	0,				/* Offset of data (for multiple
					 * reads). */
	pendingPtr->packetSize,		/* Maximum number of items to read. */
	True,				/* If true, delete the property. */
	XA_STRING,			/* Desired type of property. */
	&typeAtom,			/* (out) Actual type of the property. */
	&format,			/* (out) Actual format of the
					 * property. */
	&numItems,			/* (out) # of items in specified
					 * format. */
	&bytesAfter,			/* (out) # of bytes remaining to be
					 * read. */
	&data);

    if ((result != Success) || (typeAtom != XA_STRING) || (format != 8)) {
	pendingPtr->status = DROP_FAIL;
#ifdef notdef
    fprintf(stderr, "<- SourcePropertyEventProc: wrong format\n");
#endif
	return;				/* Wrong data format. */
    }
    if (numItems > 0) {
	pendingPtr->status = DROP_FAIL;
	Tcl_DStringFree(&pendingPtr->ds);
	Tcl_DStringAppend(&pendingPtr->ds, (char *)data, -1);
	XFree(data);
#ifdef notdef
    fprintf(stderr, "<- SourcePropertyEventProc: error\n");
#endif
	return;				/* Error occurred on target. */
    }    
    bytesLeft = Tcl_DStringLength(&pendingPtr->ds) - pendingPtr->offset;
    if (bytesLeft <= 0) {
#ifdef notdef
	fprintf(stderr, "<- SourcePropertyEventProc: done\n");
#endif
	pendingPtr->status = DROP_OK;
	size = 0;
    } else {
	size = MIN(bytesLeft, pendingPtr->packetSize);
	pendingPtr->status = DROP_CONTINUE;
    }
    p = (unsigned char *)Tcl_DStringValue(&pendingPtr->ds) + 
	pendingPtr->offset;
    XChangeProperty(pendingPtr->display, pendingPtr->window, 
	pendingPtr->commAtom, XA_STRING, 8, PropModeReplace, p, size);
    pendingPtr->offset += size;
    pendingPtr->timerToken = Tcl_CreateTimerHandler(WAIT_INTERVAL, 
	   SendTimerProc, &pendingPtr->status);
#ifdef notdef
    fprintf(stderr, "<- SourcePropertyEventProc\n");
#endif
}


static void
SendDataToTarget(Dnd *dndPtr, DropPending *pendingPtr)
{
    int size;
    Tk_RestrictProc *proc;
    ClientData arg;

#ifdef notdef
    fprintf(stderr, "-> SendDataToTarget\n");
#endif
    Tk_CreateEventHandler(dndPtr->tkwin, PropertyChangeMask, 
	SourcePropertyEventProc, pendingPtr);
    pendingPtr->timerToken = Tcl_CreateTimerHandler(WAIT_INTERVAL, 
	SendTimerProc, &pendingPtr->status);
    size = MIN(Tcl_DStringLength(&pendingPtr->ds), pendingPtr->packetSize);

    proc = Tk_RestrictEvents(SendRestrictProc, dndPtr, &arg);

    /* 
     * Setting the property starts the process.  The target will see the
     * PropertyChange event and respond accordingly.
     */
    XChangeProperty(dndPtr->display, pendingPtr->window, 
	pendingPtr->commAtom, XA_STRING, 8, PropModeReplace, 
	(unsigned char *)Tcl_DStringValue(&pendingPtr->ds), size);
    pendingPtr->offset += size;

    /*
     * Enter a loop processing X events until the result comes in or the target
     * is declared to be dead.  While waiting for a result, look only at
     * property-related events so that the handshake is synchronous with respect
     * to other events in the application.
     */
    pendingPtr->status = DROP_CONTINUE;
    while (pendingPtr->status == DROP_CONTINUE) {
	/* Wait for the property change event. */
	Tcl_DoOneEvent(TCL_ALL_EVENTS);
    }
    Tk_RestrictEvents(proc, arg, &arg);
    Tcl_DeleteTimerHandler(pendingPtr->timerToken);
    Tk_DeleteEventHandler(dndPtr->tkwin, PropertyChangeMask, 
	  SourcePropertyEventProc, pendingPtr);
#ifdef notdef
    fprintf(stderr, "<- SendDataToTarget\n");
#endif
}

static void
DoDrop(Dnd *dndPtr, XEvent *eventPtr)
{
    DndInterpData *dataPtr = dndPtr->dataPtr;
    Token *tokenPtr = dndPtr->tokenPtr;
    Tcl_Interp *interp = dndPtr->interp;
    struct DropRequest {
	int mesg;			/* TS_DROP_RESULT message. */
	Window window;			/* Target window. */
	int timestamp;			/* Transaction timestamp. */
	Atom formatAtom;		/* Format requested. */
    } *dropPtr;
    char *format;
    DropPending pending;

    if (tokenPtr->timerToken != NULL) {
	Tcl_DeleteTimerHandler(tokenPtr->timerToken);
    }
    dropPtr = (struct DropRequest *)eventPtr->xclient.data.l;
    format = XGetAtomName(dndPtr->display, dropPtr->formatAtom);
#ifdef notdef
    fprintf(stderr, "DoDrop %s 0x%x\n", Tk_PathName(dndPtr->tkwin), 
	    dropPtr->window);
#endif
    if (GetFormattedData(dndPtr, format, dropPtr->timestamp, &pending.ds)
	!= TCL_OK) {
	Tcl_BackgroundError(interp);
	/* Send empty string to break target's wait loop. */
	XChangeProperty(dndPtr->display, dropPtr->window, dataPtr->commAtom, 
		XA_STRING, 8, PropModeReplace, (unsigned char *)"", 0);
	return;
    } 
    pending.window = dropPtr->window;
    pending.display = dndPtr->display;
    pending.commAtom = dataPtr->commAtom;
    pending.offset = 0;
    pending.packetSize = GetMaxPropertySize(dndPtr->display);
    SendDataToTarget(dndPtr, &pending);
    Tcl_DStringFree(&pending.ds);
#ifdef notdef
    fprintf(stderr, "<- DoDrop\n");
#endif
}

static void
DropFinished(Dnd *dndPtr, XEvent *eventPtr)
{
    Tcl_Interp *interp = dndPtr->interp;
    Tcl_DString ds, savedResult;
    int result;
    const char **p;
    struct DropResult {
	int mesg;			/* TS_DROP_RESULT message. */
	Window window;			/* Target window. */
	int timestamp;			/* Transaction timestamp. */
	int result;			/* Result of transaction. */
    } *dropPtr;

#ifdef notdef
    fprintf(stderr, "DropFinished\n");
#endif
    dropPtr = (struct DropResult *)eventPtr->xclient.data.l;

    Tcl_DStringInit(&ds);
    for (p = dndPtr->resultCmd; *p != NULL; p++) {
	Tcl_DStringAppendElement(&ds, *p);
    }
    Tcl_DStringAppendElement(&ds, Tk_PathName(dndPtr->tkwin));
    Tcl_DStringAppendElement(&ds, "action");
    Tcl_DStringAppendElement(&ds, NameOfAction(dropPtr->result));
    Tcl_DStringAppendElement(&ds, "timestamp");
    Tcl_DStringAppendElement(&ds, Blt_Utoa(dropPtr->timestamp));

    Tcl_DStringInit(&savedResult);
    Tcl_DStringGetResult(interp, &savedResult);
    result = Tcl_GlobalEval(interp, Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
    if (result != TCL_OK) {
	Tcl_BackgroundError(interp);
    }
    Tcl_DStringResult(interp, &savedResult);
}


static void
FreeFormats(Dnd *dndPtr)
{
    if (dndPtr->matchingFormats != NULL) {
	Blt_Free(dndPtr->matchingFormats);
	dndPtr->matchingFormats = NULL;
    }
    dndPtr->lastId = None;
}

static const char *
GetSourceFormats(Dnd *dndPtr, Window window, int timestamp)
{
    if (dndPtr->lastId != timestamp) {
	unsigned char *data;
	
	FreeFormats(dndPtr);
	data = GetProperty(dndPtr->display, window, 
		   dndPtr->dataPtr->formatsAtom);
	if (data != NULL) {
	    dndPtr->matchingFormats = Blt_AssertStrdup((char *)data);
	    XFree(data);
	}
	dndPtr->lastId = timestamp;
    }
    if (dndPtr->matchingFormats == NULL) {
	return "";
    }
    return dndPtr->matchingFormats;
}
 

static int
InvokeCallback(Dnd *dndPtr, const char **cmd, int x, int y, const char *formats,
	       int button, int keyState, int timestamp)
{
    Tcl_DString ds, savedResult;
    Tcl_Interp *interp = dndPtr->interp;
    int result;
    const char **p;

    Tcl_DStringInit(&ds);
    for (p = cmd; *p != NULL; p++) {
	Tcl_DStringAppendElement(&ds, *p);
    }
    Tcl_DStringAppendElement(&ds, Tk_PathName(dndPtr->tkwin));
    /* Send coordinates relative to target. */
    x -= Blt_RootX(dndPtr->tkwin);    
    y -= Blt_RootY(dndPtr->tkwin);
    Tcl_DStringAppendElement(&ds, "x");
    Tcl_DStringAppendElement(&ds, Blt_Itoa(x));
    Tcl_DStringAppendElement(&ds, "y");
    Tcl_DStringAppendElement(&ds, Blt_Itoa(y));
    Tcl_DStringAppendElement(&ds, "formats");
    if (formats == NULL) {
	formats = "";
    }
    Tcl_DStringAppendElement(&ds, formats);
    Tcl_DStringAppendElement(&ds, "button");
    Tcl_DStringAppendElement(&ds, Blt_Itoa(button));
    Tcl_DStringAppendElement(&ds, "state");
    Tcl_DStringAppendElement(&ds, Blt_Itoa(keyState));
    Tcl_DStringAppendElement(&ds, "timestamp");
    Tcl_DStringAppendElement(&ds, Blt_Utoa(timestamp));
    Tcl_Preserve(interp);
    Tcl_DStringInit(&savedResult);
    Tcl_DStringGetResult(interp, &savedResult);
    result = Tcl_GlobalEval(interp, Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
    if (result == TCL_OK) {
	result = GetDragResult(interp, Tcl_GetStringResult(interp));
    } else {
	result = DROP_CANCEL;
	Tcl_BackgroundError(interp);
    }
    Tcl_DStringResult(interp, &savedResult);
    Tcl_Release(interp);
    return result;
}

/* 
 *---------------------------------------------------------------------------
 *
 *  AcceptDrop --
 *	
 *	Invokes a TCL procedure to handle the target's side of the drop.  A Tcl
 *	procedure is invoked, either one designated for this target by the user
 *	(-ondrop) or a default TCL procedure.  It is passed the following
 *	arguments:
 *
 *		widget		The path name of the target. 
 *		x		X-coordinate of the mouse relative to the 
 *				widget.
 *		y		Y-coordinate of the mouse relative to the 
 *				widget.
 *		formats		A list of data formats acceptable to both
 *				the source and target.
 *		button		Button pressed.
 *		state		Key state.
 *		timestamp	Timestamp of transaction.
 *		action		Requested action from source.
 *
 *	If the TCL procedure returns "cancel", this indicates that the drop was
 *	not accepted by the target and the reject symbol should be displayed.
 *	Otherwise one of the following strings may be recognized:
 *
 *		"cancel"	Drop was canceled.
 *		"copy"		Source data has been successfully copied.
 *		"link"		Target has made a link to the data. It's 
 *				Ok for the source to remove it's association
 *				with the data, but not to delete the data
 *				itself.
 *		"move"		Source data has been successfully copied,
 *				it's Ok for the source to delete its
 *				association with the data and the data itself.
 *
 *	The result is relayed back to the source via another client message.
 *	The source may or may not be waiting for the result.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A TCL procedure is invoked in the target to handle the drop event.
 *	The result of the drop is sent (via another ClientMessage) to the
 *	source.
 *
 *---------------------------------------------------------------------------
 */
static int
AcceptDrop(
    Dnd *dndPtr,		/* Target where the drop event occurred. */
    int x, int y,
    const char *formats,
    int button, 
    int keyState, 
    int timestamp)
{
    Tcl_Interp *interp = dndPtr->interp;
    const char **cmd;
    Tcl_DString ds, savedResult;
    int result;
    
    if (dndPtr->motionCmd != NULL) {
	result = InvokeCallback(dndPtr, dndPtr->motionCmd, x, y, formats, 
		button, keyState, timestamp);
	if (result != DROP_OK) {
	    return result;
	}
    }
    if (dndPtr->leaveCmd != NULL) {
	InvokeCallback(dndPtr, dndPtr->leaveCmd, x, y, formats, button, 
	       keyState, timestamp);
    }
    Tcl_DStringInit(&ds);
    cmd = dndPtr->dropCmd;
    if (cmd != NULL) {
	const char **p;

	for (p = cmd; *p != NULL; p++) {
	    Tcl_DStringAppendElement(&ds, *p);
	}
    } else {
	Tcl_DStringAppendElement(&ds, "::blt::DndStdDrop");
    }
    Tcl_DStringAppendElement(&ds, Tk_PathName(dndPtr->tkwin));
    dndPtr->dropX = x - Blt_RootX(dndPtr->tkwin);
    dndPtr->dropY = y - Blt_RootY(dndPtr->tkwin);
    Tcl_DStringAppendElement(&ds, "x");
    Tcl_DStringAppendElement(&ds, Blt_Itoa(dndPtr->dropX));
    Tcl_DStringAppendElement(&ds, "y");
    Tcl_DStringAppendElement(&ds, Blt_Itoa(dndPtr->dropY));
    Tcl_DStringAppendElement(&ds, "formats");
    Tcl_DStringAppendElement(&ds, formats);
    Tcl_DStringAppendElement(&ds, "button");
    Tcl_DStringAppendElement(&ds, Blt_Itoa(button));
    Tcl_DStringAppendElement(&ds, "state");
    Tcl_DStringAppendElement(&ds, Blt_Itoa(keyState));
    Tcl_DStringAppendElement(&ds, "timestamp");
    Tcl_DStringAppendElement(&ds, Blt_Utoa(timestamp));
    Tcl_Preserve(interp);
    Tcl_DStringInit(&savedResult);
    Tcl_DStringGetResult(interp, &savedResult);
    result = Tcl_GlobalEval(interp, Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
    if (result == TCL_OK) {
	result = GetAction(Tcl_GetStringResult(interp));
    } else {
	result = DROP_CANCEL;
	Tcl_BackgroundError(interp);
    }
    Tcl_DStringResult(interp, &savedResult);
    Tcl_Release(interp);
    return result;
}

/* 
 *---------------------------------------------------------------------------
 *
 *  HandleDropEvent --
 *	
 *	Invokes a TCL procedure to handle the target's side of the drop.  This
 *	routine is triggered via a client message from the drag source
 *	indicating that the token was dropped over this target. The fields of
 *	the incoming message are:
 *
 *		data.l[0]	Message type.
 *		data.l[1]	Window Id of the source.
 *		data.l[2]	Screen X-coordinate of the pointer.
 *		data.l[3]	Screen Y-coordinate of the pointer.
 *		data.l[4]	Id of the drag&drop transaction.
 *
 *	A TCL procedure is invoked, either one designated for this target by the
 *	user (-ondrop) or a default TCL procedure. It is passed the following
 *	arguments:
 *
 *		widget		The path name of the target. 
 *		x		X-coordinate of the mouse relative to the 
 *				widget.
 *		y		Y-coordinate of the mouse relative to the 
 *				widget.
 *		formats		A list of data formats acceptable to both
 *				the source and target.
 *
 *	If the TCL procedure returns "cancel", this indicates that the drop was
 *	not accepted by the target and the reject symbol should be displayed.
 *	Otherwise one of the following strings may be recognized:
 *
 *		"cancel"	Drop was canceled.
 *		"copy"		Source data has been successfully copied.
 *		"link"		Target has made a link to the data. It's 
 *				Ok for the source to remove it's association
 *				with the data, but not to delete the data
 *				itself.
 *		"move"		Source data has been successfully copied,
 *				it's Ok for the source to delete its
 *				association with the data and the data itself.
 *
 *	The result is relayed back to the source via another client message.
 *	The source may or may not be waiting for the result.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A TCL procedure is invoked in the target to handle the drop event.
 *	The result of the drop is sent (via another ClientMessage) to the
 *	source.
 *
 *---------------------------------------------------------------------------
 */
static void
HandleDropEvent(
    Dnd *dndPtr,			/* Target where the drop event
					 * occurred. */
    XEvent *eventPtr)			/* Message sent from the drag source. */
{
    int button, keyState;
    int x, y;
    const char *formats;
    int result;
    struct DropInfo {
	int mesg;			/* TS_DROP message. */
	Window window;			/* Source window. */
	int timestamp;			/* Transaction timestamp. */
	int point;			/* Root X-Y coordinate of pointer. */
	int flags;			/* Button/keystate information. */
    } *dropPtr;
    DropPending pending;

    dropPtr = (struct DropInfo *)eventPtr->xclient.data.l;
    UNPACK(dropPtr->point, x, y);
    UNPACK(dropPtr->flags, button, keyState);

    /* Set up temporary bookkeeping for the drop transaction */
    memset (&pending, 0, sizeof(pending));
    pending.window = dropPtr->window;
    pending.display = eventPtr->xclient.display;
    pending.timestamp = dropPtr->timestamp;
    pending.protocol = PROTO_BLT;
    pending.packetSize = GetMaxPropertySize(pending.display);
    Tcl_DStringInit(&pending.ds);

    formats = GetSourceFormats(dndPtr, dropPtr->window, dropPtr->timestamp);

    dndPtr->pendingPtr = &pending;
    result = AcceptDrop(dndPtr, x, y, formats, button, keyState, 
	dropPtr->timestamp);
    dndPtr->pendingPtr = NULL;

    /* Target-to-Source: Drop result message. */
    SendClientMsg(dndPtr->display, dropPtr->window, dndPtr->dataPtr->mesgAtom,
	TS_DROP_RESULT, (int)Tk_WindowId(dndPtr->tkwin), dropPtr->timestamp, 
	result, 0);
    FreeFormats(dndPtr);
}

/* 
 *---------------------------------------------------------------------------
 *
 *  HandleDragEvent --
 *	
 *	Invokes one of 3 TCL procedures to handle the target's side of the drag
 *	operation.  This routine is triggered via a ClientMessage from the drag
 *	source indicating that the token as either entered, moved, or left this
 *	target.  The source sends messages only if TCL procedures on the target
 *	have been defined to watch the events. The message_type field can be
 *	either
 *
 *	  ST_DRAG_ENTER		The mouse has entered the target.
 *	  ST_DRAG_MOTION	The mouse has moved within the target.
 *	  ST_DRAG_LEAVE		The mouse has left the target.
 *
 *	The data fields are as follows:
 *	  data.l[0]		Message type.
 *	  data.l[1]		Window Id of the source.
 *	  data.l[2]		Timestamp of the drag&drop transaction.
 *	  data.l[3]		Root X-Y coordinate of the pointer.
 *	  data.l[4]		Button and key state information.
 *
 *	For any of the 3 TCL procedures, the following arguments are passed:
 *
 *	  widget		The path name of the target. 
 *	  x			X-coordinate of the mouse in the widget.
 *	  y			Y-coordinate of the mouse in the widget.
 *	  formats		A list of data formats acceptable to both
 *				the source and target.
 *
 *	If the TCL procedure returns "cancel", this indicates that the drag
 *	operation has been canceled and the reject symbol should be displayed.
 *	Otherwise it should return a boolean:
 *
 *	  true			Target will accept drop.
 *	  false			Target will not accept the drop.
 *
 *	The purpose of the Enter and Leave procedure is to allow the target to
 *	provide visual feedback that the drop can occur or not.  The Motion
 *	procedure is for cases where the drop area is a smaller area within the
 *	target, such as a canvas item on a canvas. The procedure can determine
 *	(based upon the X-Y coordinates) whether the pointer is over the canvas
 *	item and return a value accordingly.
 *
 *	The result of the TCL procedure is then relayed back to the source by a
 *	ClientMessage.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A TCL procedure is invoked in the target to handle the drag event.
 *	The result of the drag is sent (via another ClientMessage) to the
 *	source.
 *
 *---------------------------------------------------------------------------
 */
static void
HandleDragEvent(
    Dnd *dndPtr,			/* Target where the drag event
					 * occurred. */
    XEvent *eventPtr)			/* Message sent from the drag source. */
{
    const char **cmd;
    int resp;
    int x, y;
    int button, keyState;
    const char *formats;
    struct DragInfo {
	int mesg;			/* Drag-and-drop message type. */
	Window window;			/* Source window. */
	int timestamp;			/* Transaction timestamp. */
	int point;			/* Root X-Y coordinate of pointer. */
	int flags;			/* Button/keystate information. */
    } *dragPtr;

    dragPtr = (struct DragInfo *)eventPtr->xclient.data.l;

    cmd = NULL;
    switch (dragPtr->mesg) {
    case ST_DRAG_ENTER:
	cmd = dndPtr->enterCmd;
	break;
    case ST_DRAG_MOTION:
	cmd = dndPtr->motionCmd;
	break;
    case ST_DRAG_LEAVE:
	cmd = dndPtr->leaveCmd;
	break;
    } 
    if (cmd == NULL) {
	return;				/* Nothing to do. */
    }
    UNPACK(dragPtr->point, x, y);
    UNPACK(dragPtr->flags, button, keyState);
    formats = GetSourceFormats(dndPtr, dragPtr->window, dragPtr->timestamp);
    resp = InvokeCallback(dndPtr, cmd, x, y, formats, button, keyState,
		  dragPtr->timestamp);

    /* Target-to-Source: Drag result message. */
    SendClientMsg(dndPtr->display, dragPtr->window, dndPtr->dataPtr->mesgAtom,
	TS_DRAG_STATUS, (int)Tk_WindowId(dndPtr->tkwin), dragPtr->timestamp, 
	resp, 0);
}

/*
 *---------------------------------------------------------------------------
 *
 *  DndEventProc --
 *
 *	Invoked by Tk_HandleEvent whenever a DestroyNotify event is received
 *	on a registered drag&drop source widget.
 *
 *---------------------------------------------------------------------------
 */
static int
DndEventProc(
    ClientData clientData,		/* Drag&drop record. */
    XEvent *eventPtr)			/* Event description. */
{
    Dnd *dndPtr = clientData;

    if (eventPtr->xany.window != Tk_WindowId(dndPtr->tkwin)) {
	return 0;
    }
    if (eventPtr->type == DestroyNotify) {
	dndPtr->tkwin = NULL;
	dndPtr->flags |= DND_DELETED;
	Tcl_EventuallyFree(dndPtr, DestroyDnd);
	return 0;		/* Other handlers have to see this event too.*/
    } else if (eventPtr->type == ButtonPress) {
	dndPtr->keyState = eventPtr->xbutton.state;
	dndPtr->button =  eventPtr->xbutton.button;
	return 0;
    } else if (eventPtr->type == ButtonRelease) {
	dndPtr->keyState = eventPtr->xbutton.state;
	dndPtr->button =  eventPtr->xbutton.button;
	return 0;
    } else if (eventPtr->type == MotionNotify) {
	dndPtr->keyState = eventPtr->xmotion.state;
	return 0;
    } else if ((eventPtr->type == ClientMessage) &&
	(eventPtr->xclient.message_type == dndPtr->dataPtr->mesgAtom)) {
	int result;

	switch((unsigned int)eventPtr->xclient.data.l[0]) {
	case TS_START_DROP:
	    DoDrop(dndPtr, eventPtr);
	    return 1;
	    
	case TS_DROP_RESULT:
	    result = eventPtr->xclient.data.l[MESG_RESPONSE];
	    dndPtr->tokenPtr->status = result;
	    if (result == DROP_CANCEL) {
		CancelDrag(dndPtr);
	    } else if (result == DROP_FAIL) {
		EventuallyRedrawToken(dndPtr);
	    } else {
		dndPtr->tokenPtr->numSteps = 10;
		FadeToken(dndPtr);
	    }
	    if (dndPtr->resultCmd != NULL) {
		DropFinished(dndPtr, eventPtr);
	    }
	    return 1;

	case TS_DRAG_STATUS:
	    result = eventPtr->xclient.data.l[MESG_RESPONSE];
	    ChangeToken(dndPtr, result);
	    return 1;

	case ST_DROP:
	    HandleDropEvent(dndPtr, eventPtr);
	    return 1;

	case ST_DRAG_ENTER:
	case ST_DRAG_MOTION:
	case ST_DRAG_LEAVE:
	    HandleDragEvent(dndPtr, eventPtr);
	    return 1;
	}
    }
    return 0;
}

static void
SendPointerMessage(
    Dnd *dndPtr,		/* Source drag&drop manager. */
    int eventType,		/* Type of event to relay. */
    Winfo *windowPtr,		/* Generic window information. */
    int x, int y)		/* Root coordinates of mouse. */
{
    /* Source-to-Target: Pointer event messages. */
    SendClientMsg(
	dndPtr->display,	/* Display of recipient window. */
	windowPtr->window,	/* Recipient window. */
	dndPtr->dataPtr->mesgAtom, /* Message type. */
	eventType,		/* Data 1 */
	(int)Tk_WindowId(dndPtr->tkwin), /* Data 2 */
	dndPtr->timestamp,	/* Data 3  */
	PACK(x, y),		/* Data 4 */
	PACK(dndPtr->button, dndPtr->keyState)); /* Data 5 */
    /* Don't wait the response. */
}

static void
RelayEnterEvent(Dnd *dndPtr, Winfo *windowPtr, int x, int y)
{
    if ((windowPtr != NULL) && (windowPtr->eventFlags & WATCH_ENTER)) {
	SendPointerMessage(dndPtr, ST_DRAG_ENTER, windowPtr, x, y);
    }
}

static void
RelayLeaveEvent(Dnd *dndPtr, Winfo *windowPtr, int x, int y)
{ 
    if ((windowPtr != NULL) && (windowPtr->eventFlags & WATCH_LEAVE)) {
	SendPointerMessage(dndPtr, ST_DRAG_LEAVE, windowPtr, x, y);
    }
}

static void
RelayMotionEvent(Dnd *dndPtr, Winfo *windowPtr, int x, int y)
{
    if ((windowPtr != NULL) && (windowPtr->eventFlags & WATCH_MOTION)) {
	SendPointerMessage(dndPtr, ST_DRAG_MOTION, windowPtr, x, y);
    }
}

static void
RelayDropEvent(Dnd *dndPtr, Winfo *windowPtr, int x, int y)
{
    SendPointerMessage(dndPtr, ST_DROP, windowPtr, x, y);
}

/*
 *---------------------------------------------------------------------------
 *
 *  FreeWinfo --
 *
 *---------------------------------------------------------------------------
 */
static void
FreeWinfo(Winfo *windowPtr)	/* Window rep to be freed */
{
    Winfo *childPtr;
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(windowPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	childPtr = Blt_Chain_GetValue(link);
	FreeWinfo(childPtr);	/* Recursively free children. */
    }
    if (windowPtr->matches != NULL) {
	Blt_Free(windowPtr->matches);
    }
    Blt_Chain_Destroy(windowPtr->chain);
    Blt_Free(windowPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 *  GetWinfo --
 *
 *	Invoked during "drag" operations. Digs into the root window
 *	hierarchy and caches the window-related information.
 *	If the current point lies over an uninitialized window (i.e.
 *	one that already has an allocated Winfo structure, but has
 *	not been filled in yet), this routine is called to query 
 *	window coordinates.  If the window has any children, more 
 *	uninitialized Winfo structures are allocated.  Further queries 
 *	will cause these structures to be initialized in turn.
 *
 *---------------------------------------------------------------------------
 */
static void
GetWinfo(Display *display, Winfo *windowPtr) /* window rep to be initialized */
{
    int visible;

    if (windowPtr->initialized) {
	return;
    }
    /* Query for the window coordinates.  */
    visible = GetWindowArea(display, windowPtr);
    if (visible) {
	Blt_ChainLink link;
	Blt_Chain chain;
	Winfo *childPtr;

	/* Add offset from parent's origin to coordinates */
	if (windowPtr->parentPtr != NULL) {
	    windowPtr->x1 += windowPtr->parentPtr->x1;
	    windowPtr->y1 += windowPtr->parentPtr->y1;
	    windowPtr->x2 += windowPtr->parentPtr->x1;
	    windowPtr->y2 += windowPtr->parentPtr->y1;
	}
	/*
	 * Collect a list of child windows, sorted in z-order.  The
	 * topmost window will be first in the list.
	 */
	chain = GetWindowZOrder(display, windowPtr->window);

	/* Add and initialize extra slots if needed. */
	for (link = Blt_Chain_FirstLink(chain); link != NULL;
	    link = Blt_Chain_NextLink(link)) {
	    childPtr = Blt_AssertCalloc(1, sizeof(Winfo));
	    childPtr->initialized = FALSE;
	    childPtr->window = (Window)Blt_Chain_GetValue(link);
	    childPtr->parentPtr = windowPtr;
	    Blt_Chain_SetValue(link, childPtr);
	}
	windowPtr->chain = chain;
    } else {
	/* If it's not viewable don't bother doing anything else. */
	windowPtr->x1 = windowPtr->y1 = windowPtr->x2 = windowPtr->y2 = -1;
	windowPtr->chain = NULL;
    }
    windowPtr->initialized = TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 *  InitRoot --
 *
 *	Invoked at the start of a "drag" operation to capture the
 *	positions of all windows on the current root.  Queries the
 *	entire window hierarchy and determines the placement of each
 *	window.  Queries the "BltDndTarget" property info where
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
static Winfo *
InitRoot(Dnd *dndPtr)
{
    Winfo *rootPtr;

    rootPtr = Blt_AssertCalloc(1, sizeof(Winfo));
    rootPtr->window = DefaultRootWindow(dndPtr->display);
    dndPtr->windowPtr = NULL;
    GetWinfo(dndPtr->display, rootPtr);
    return rootPtr;
}


static int
ParseProperty(Tcl_Interp *interp, Dnd *dndPtr, Winfo *windowPtr, char *data)
{
    int argc;
    const char **argv;
    int eventFlags;
    Tcl_DString ds;
    int count;
    int i;
    
    if (Tcl_SplitList(interp, data, &argc, &argv) != TCL_OK) {
	return TCL_ERROR;	/* Malformed property list. */
    }
    if (argc < 1) {
	Tcl_AppendResult(interp, "Malformed property \"", data, "\"", 
			 (char *)NULL);
	goto error;
    }
    if (Tcl_GetInt(interp, argv[PROP_WATCH_FLAGS], &eventFlags) != TCL_OK) {
	goto error;
    }

    /* target flags, type1, type2, ... */
    /*
     * The target property contains a list of possible formats.
     * Compare this with what formats the source is willing to
     * convert and compress the list down to just the matching
     * formats.  It's up to the target to request the specific 
     * type (or types) that it wants.
     */
    count = 0;
    Tcl_DStringInit(&ds);
    if (dndPtr->reqFormats == NULL) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch cursor;
	char *fmt;

	for (i = 1; i < argc; i++) {
	    for(hPtr = Blt_FirstHashEntry(&dndPtr->getDataTable, &cursor);
		hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
		fmt = Blt_GetHashKey(&dndPtr->getDataTable, hPtr);
		if ((*fmt == argv[i][0]) && (strcmp(fmt, argv[i]) == 0)) {
		    Tcl_DStringAppendElement(&ds, argv[i]);
		    count++;
		    break;
		}
	    }
	}
    } else {
	const char **s;

	for (i = 1; i < argc; i++) {
	    for (s = dndPtr->reqFormats; *s != NULL; s++) {
		if ((**s == argv[i][0]) && (strcmp(*s, argv[i]) == 0)) {
		    Tcl_DStringAppendElement(&ds, argv[i]);
		    count++;
		}
	    }
	}
    }
    if (count == 0) {
#ifdef notdef
	fprintf(stderr, "source/target mismatch: No matching types\n");
#endif
	return TCL_BREAK;
    } 
    if (eventFlags != 0) {
	SetProperty(dndPtr->tkwin, dndPtr->dataPtr->formatsAtom, 
		    Tcl_DStringValue(&ds));
	windowPtr->matches = NULL;
    } else {
	windowPtr->matches = Blt_AssertStrdup(Tcl_DStringValue(&ds));
    }
    Tcl_DStringFree(&ds);	
    windowPtr->eventFlags = eventFlags;
    return TCL_OK;
 error:
    Blt_Free(argv);
    return TCL_ERROR;
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
static Winfo *
OverTarget(Dnd *dndPtr)		/* drag&drop source window */
{
    Tcl_Interp *interp = dndPtr->interp;
    int x, y;
    int vx, vy;
    int dummy;
    Winfo *windowPtr;

    /* 
     * If no window info has been been gathered yet for this target,
     * then abort this call.  This probably means that the token is
     * moved before it has been properly built.  
     */
    if (dndPtr->rootPtr == NULL) {
	fprintf(stderr, "rootPtr not initialized\n");
	return NULL;
    }
    /* Adjust current location for virtual root windows.  */
    Tk_GetVRootGeometry(dndPtr->tkwin, &vx, &vy, &dummy, &dummy);
    x = dndPtr->x + vx;
    y = dndPtr->y + vy;

    windowPtr = FindTopWindow(dndPtr, x, y);
    if (windowPtr == NULL) {
	return NULL;		/* Not over a window. */
    }
    if ((!dndPtr->selfTarget) && 
	(Tk_WindowId(dndPtr->tkwin) == windowPtr->window)) {
	return NULL;		/* If the self-target flag isn't set,
				 *  don't allow the source window to
				 *  drop onto itself.  */
    }
    if (!windowPtr->lookedForProperty) {
	unsigned char *data;
	int result;

	windowPtr->lookedForProperty = TRUE;
	/* See if this window has a "BltDndTarget" property. */
	data = GetProperty(dndPtr->display, windowPtr->window, 
		dndPtr->dataPtr->targetAtom);
	if (data == NULL) {
#ifdef notdef
	    fprintf(stderr, "No property on 0x%x\n", windowPtr->window);
#endif
	    return NULL;		/* No such property on window. */
	}
	result = ParseProperty(interp, dndPtr, windowPtr, (char *)data);
	XFree(data);
	if (result == TCL_BREAK) {
#ifdef notdef
	    fprintf(stderr, "No matching formats\n");
#endif
	    return NULL;
	}
	if (result != TCL_OK) {
	    Tcl_BackgroundError(interp);
	    return NULL;		/* Malformed property list. */
	} 
	windowPtr->isTarget = TRUE;
    }
    if (!windowPtr->isTarget) {
	return NULL;
    }
    return windowPtr;

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
AddTargetProperty(Dnd *dndPtr)	/* drag&drop target window data */
{
    Tcl_DString ds;
    Blt_HashEntry *hPtr;
    unsigned int eventFlags;
    Blt_HashSearch cursor;
    char *fmt;
    char string[200];

    Tcl_DStringInit(&ds);
    /*
     * Each target window's dnd property contains
     *
     *	1. Mouse event flags.
     *  2. List of all the data types that can be handled. If none
     *     are listed, then all can be handled.
     */
    eventFlags = 0;
    if (dndPtr->enterCmd != NULL) {
	eventFlags |= WATCH_ENTER;
    }
    if (dndPtr->leaveCmd != NULL) {
	eventFlags |= WATCH_LEAVE;
    }
    if (dndPtr->motionCmd != NULL) {
	eventFlags |= WATCH_MOTION;
    }
    Blt_FormatString(string, 200, "0x%x", eventFlags);
    Tcl_DStringAppendElement(&ds, string);
    for (hPtr = Blt_FirstHashEntry(&dndPtr->setDataTable, &cursor);
	hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	fmt = Blt_GetHashKey(&dndPtr->setDataTable, hPtr);
	Tcl_DStringAppendElement(&ds, fmt);
    }
    SetProperty(dndPtr->tkwin, dndPtr->dataPtr->targetAtom, 
		Tcl_DStringValue(&ds));
    dndPtr->targetPropertyExists = TRUE;
    Tcl_DStringFree(&ds);
}

static void
CancelDrag(Dnd *dndPtr)
{
    if (dndPtr->flags & DND_INITIATED) {
	dndPtr->tokenPtr->numSteps = 10;
	SnapToken(dndPtr);
	StopActiveCursor(dndPtr);
	if (dndPtr->cursor == None) {
	    Tk_UndefineCursor(dndPtr->tkwin);
	} else {
	    Tk_DefineCursor(dndPtr->tkwin, dndPtr->cursor);
	}
    }
    if (dndPtr->rootPtr != NULL) {
	FreeWinfo(dndPtr->rootPtr);
	dndPtr->rootPtr = NULL;
    }
}

static int
DragInit(Dnd *dndPtr, int x, int y)
{
    Token *tokenPtr = dndPtr->tokenPtr;
    int result;
    Winfo *newPtr;

    assert((dndPtr->flags & DND_ACTIVE) == DND_SELECTED);
    
    if (dndPtr->rootPtr != NULL) {
	FreeWinfo(dndPtr->rootPtr);
    }
    dndPtr->rootPtr = InitRoot(dndPtr); /* Reset information cache. */ 
    dndPtr->flags &= ~DND_VOIDED;

    dndPtr->x = x;	/* Save current location. */
    dndPtr->y = y;
    result = TRUE;
    Tcl_Preserve(dndPtr);
    if (dndPtr->packageCmd != NULL) {
	Tcl_DString ds, savedResult;
	Tcl_Interp *interp = dndPtr->interp;
	int status;
	const char **p;
	int rx, ry;

	Tcl_DStringInit(&ds);
	for (p = dndPtr->packageCmd; *p != NULL; p++) {
	    Tcl_DStringAppendElement(&ds, *p);
	}
	Tcl_DStringAppendElement(&ds, Tk_PathName(dndPtr->tkwin));
	rx = dndPtr->dragX - Blt_RootX(dndPtr->tkwin);
	ry = dndPtr->dragY - Blt_RootY(dndPtr->tkwin);
	Tcl_DStringAppendElement(&ds, "x");
	Tcl_DStringAppendElement(&ds, Blt_Itoa(rx));
	Tcl_DStringAppendElement(&ds, "y");
	Tcl_DStringAppendElement(&ds, Blt_Itoa(ry));
	Tcl_DStringAppendElement(&ds, "button");
	Tcl_DStringAppendElement(&ds, Blt_Itoa(dndPtr->button));
	Tcl_DStringAppendElement(&ds, "state");
	Tcl_DStringAppendElement(&ds, Blt_Itoa(dndPtr->keyState));
	Tcl_DStringAppendElement(&ds, "timestamp");
	Tcl_DStringAppendElement(&ds, Blt_Utoa(dndPtr->timestamp));
	Tcl_DStringAppendElement(&ds, "token");
	Tcl_DStringAppendElement(&ds, Tk_PathName(tokenPtr->tkwin));

	Tcl_DStringInit(&savedResult);
	Tcl_DStringGetResult(interp, &savedResult);
	dndPtr->flags |= DND_IN_PACKAGE;
	status = Tcl_GlobalEval(interp, Tcl_DStringValue(&ds));
	dndPtr->flags &= ~DND_IN_PACKAGE;
	if (status == TCL_OK) {
	    result = GetDragResult(interp, Tcl_GetStringResult(interp));
	} else {
	    Tcl_BackgroundError(interp);
	}
	Tcl_DStringFree(&ds);
	Tcl_DStringResult(interp, &savedResult);
	Tcl_DStringFree(&ds);
	if (status != TCL_OK) {
	    HideToken(dndPtr);
	    Tcl_Release(dndPtr);
	    return TCL_ERROR;
	}
    }
    if (dndPtr->flags & DND_VOIDED) {
	HideToken(dndPtr);
	Tcl_Release(dndPtr);
	return TCL_RETURN;
    }
    if ((!result) || (dndPtr->flags & DND_DELETED)) {
	HideToken(dndPtr);
	Tcl_Release(dndPtr);
	return TCL_RETURN;
    }
    Tcl_Release(dndPtr);

    if (dndPtr->cursor != None) {
	Tk_Cursor cursor;
	
	/* Save the old cursor */
	cursor = GetWidgetCursor(dndPtr->interp, dndPtr->tkwin);
	if (dndPtr->cursor != None) {
	    Tk_FreeCursor(dndPtr->display, dndPtr->cursor);
	}
	dndPtr->cursor = cursor;
	if (dndPtr->cursors != NULL) {
	    /* Temporarily install the drag-and-drop cursor. */
	    Tk_DefineCursor(dndPtr->tkwin, dndPtr->cursors[0]);
	}
    }
    if (Tk_WindowId(tokenPtr->tkwin) == None) {
	Tk_MakeWindowExist(tokenPtr->tkwin);
    }
    if (!Tk_IsMapped(tokenPtr->tkwin)) {
	Tk_MapWindow(tokenPtr->tkwin);
    }
    dndPtr->flags |= DND_INITIATED;
    newPtr = OverTarget(dndPtr);
    RelayEnterEvent(dndPtr, newPtr, x, y);
    dndPtr->windowPtr = newPtr;
    tokenPtr->status = (newPtr != NULL) ? DROP_OK : DROP_CONTINUE;
    if (tokenPtr->lastStatus != tokenPtr->status) {
	EventuallyRedrawToken(dndPtr);
    }
    MoveToken(dndPtr);		/* Move token to current drag point. */ 
    RaiseToken(dndPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  CancelOp --
 *
 *	Cancels the current drag&drop operation for the source.  Calling
 *	this operation does not affect the transfer of data from the 
 *	source to the target, once the drop has been made.  From the 
 *	source's point of view, the drag&drop operation is already over. 
 *
 *	Example: dnd cancel .widget
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Hides the token and sets a flag indicating that further "drag"
 *	and "drop" operations should be ignored.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CancelOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Dnd *dndPtr;

    if (GetDndFromObj(clientData, interp, objv[2], &dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!dndPtr->isSource) {
	Tcl_AppendResult(interp, "widget \"", Tk_PathName(dndPtr->tkwin), 
	 "\" is not a registered drag&drop source.", (char *)NULL);
	return TCL_ERROR;
    }
    /* Send the target a Leave message so it can change back. */
    RelayLeaveEvent(dndPtr, dndPtr->windowPtr, 0, 0);
    CancelDrag(dndPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  CgetOp --
 *
 *	Called to process an (objc,objv) list to configure (or
 *	reconfigure) a drag&drop widget.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED*/
static int
CgetOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Dnd *dndPtr;

    if (GetDndFromObj(clientData, interp, objv[2], &dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return Blt_ConfigureValueFromObj(interp, dndPtr->tkwin, configSpecs, 
	(char *)dndPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 *  ConfigureOp --
 *
 *	Called to process an (objc,objv) list to configure (or
 *	reconfigure) a drag&drop widget.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,		/* current interpreter */
    int objc,			/* number of arguments */
    Tcl_Obj *const *objv)		/* argument strings */
{
    Dnd *dndPtr;
    int flags;

    if (GetDndFromObj(clientData, interp, objv[2], &dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    flags = BLT_CONFIG_OBJV_ONLY;
    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, dndPtr->tkwin, configSpecs, 
		(char *)dndPtr, (Tcl_Obj *)NULL, flags);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, dndPtr->tkwin, configSpecs,
	    (char *)dndPtr, objv[3], flags);
    } 
    if (Blt_ConfigureWidgetFromObj(interp, dndPtr->tkwin, configSpecs, objc - 3,
		   objv + 3, (char *)dndPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    if (ConfigureDnd(interp, dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  DeleteOp --
 *
 *	Deletes the drag&drop manager from the widget.  If a "-source"
 *	or "-target" switch is present, only that component of the
 *	drag&drop manager is shutdown.  The manager is not deleted
 *	unless both the target and source components are shutdown.
 *
 *	Example: dnd delete .widget
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Deletes the drag&drop manager.  Also the source and target window 
 *	properties are removed from the widget.
 *
 * ------------------------------------------------------------------------ 
 */
static int
DeleteOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    int i;

    for(i = 3; i < objc; i++) {
	Dnd *dndPtr;

	if (GetDndFromObj(clientData, interp, objv[i], &dndPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	dndPtr->flags |= DND_DELETED;
	Tcl_EventuallyFree(dndPtr, DestroyDnd);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  SelectOp --
 *
 *	Initializes a drag&drop transaction.  Typically this operation
 *	is called from a ButtonPress event on a source widget.  The
 *	window information cache is initialized, and the token is 
 *	initialized and displayed.  
 *
 *	Example: dnd pickup .widget x y 
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	The token is initialized and displayed.  This may require invoking
 *	a user-defined package command.  The window information cache is
 *	also initialized.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Dnd *dndPtr;
    int x, y, timestamp;
    Token *tokenPtr;

    if (GetDndFromObj(clientData, interp, objv[2], &dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!dndPtr->isSource) {
	Tcl_AppendResult(interp, "widget \"", Tk_PathName(dndPtr->tkwin), 
	 "\" is not a registered drag&drop source.", (char *)NULL);
	return TCL_ERROR;
    }
    tokenPtr = dndPtr->tokenPtr;
    if (tokenPtr == NULL) {
	Tcl_AppendResult(interp, "no drag&drop token created for \"", 
		 objv[2], "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[5], &timestamp) != TCL_OK) {
	return TCL_ERROR;
    }
    if (dndPtr->flags & (DND_IN_PACKAGE | DND_ACTIVE | DND_VOIDED)) {
	return TCL_OK;
    }

    if (tokenPtr->timerToken != NULL) {
	HideToken(dndPtr);	/* If the user selected again before the
				 * token snap/melt has completed, first 
				 * disable the token timer callback. */
    }
    /* At this point, simply save the starting pointer location. */
    dndPtr->dragX = x, dndPtr->dragY = y;
    GetTokenPosition(dndPtr, x, y);
    tokenPtr->startX = tokenPtr->x;
    tokenPtr->startY = tokenPtr->y;
    dndPtr->timestamp = timestamp;
    dndPtr->flags |= DND_SELECTED;
    
    if (dndPtr->dragStart == 0) {
	if (DragInit(dndPtr, x, y) == TCL_ERROR) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  DragOp --
 *
 *	Continues the drag&drop transaction.  Typically this operation
 *	is called from a button Motion event on a source widget.  Pointer
 *	event messages are possibly sent to the target, indicating Enter,
 *	Leave, and Motion events.
 *
 *	Example: dnd drag .widget x y 
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Pointer events are relayed to the target (if the mouse is over
 *	one).
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DragOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Winfo *newPtr, *oldPtr;
    Dnd *dndPtr;
    int x, y;

    if (GetDndFromObj(clientData, interp, objv[2], &dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!dndPtr->isSource) {
	Tcl_AppendResult(interp, "widget \"", Tk_PathName(dndPtr->tkwin), 
	 "\" is not a registered drag&drop source.", (char *)NULL);
	return TCL_ERROR;
    }
    if (dndPtr->tokenPtr == NULL) {
	Tcl_AppendResult(interp, "no drag&drop token created for \"", 
		 objv[2], "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)) {
	return TCL_ERROR;
    }

    if ((dndPtr->flags & DND_SELECTED) == 0) {
	return TCL_OK;		/* Re-entered this routine. */
    }	
    /* 
     * The following code gets tricky because the package command may 
     * call "update" or "tkwait".  A motion event may then trigger 
     * this routine, before the token has been initialized. Until the 
     * package command finishes, no target messages are sent and drops 
     * are silently ignored.  Note that we do still track mouse 
     * movements, so that when the package command completes, it will 
     * have the latest pointer position.  
     */
    dndPtr->x = x;	/* Save current location. */
    dndPtr->y = y;

    if (dndPtr->flags & DND_IN_PACKAGE) {
	return TCL_OK;		/* Re-entered this routine. */
    }
    if ((dndPtr->flags & DND_INITIATED) == 0) {
	int dx, dy;
	int result;

	dx = dndPtr->dragX - x;
	dy = dndPtr->dragY - y;
	if ((ABS(dx) < dndPtr->dragStart) && (ABS(dy) < dndPtr->dragStart)) {
	    return TCL_OK;
	}
	result = DragInit(dndPtr, x, y);
	if (result == TCL_ERROR) {
	    return TCL_ERROR;
	}
	if (result == TCL_RETURN) {
	    return TCL_OK;
	}
    }
    if (dndPtr->flags & DND_VOIDED) {
	return TCL_OK;
    }
    oldPtr = dndPtr->windowPtr;
    newPtr = OverTarget(dndPtr);
    if (newPtr == oldPtr) {
	RelayMotionEvent(dndPtr, oldPtr, x, y); 
	dndPtr->windowPtr = oldPtr;
    } else {
	RelayLeaveEvent(dndPtr, oldPtr, x, y);
	RelayEnterEvent(dndPtr, newPtr, x, y);
	dndPtr->windowPtr = newPtr;
    } 
    dndPtr->tokenPtr->status = (newPtr != NULL) ? DROP_OK : DROP_CONTINUE;
    if (dndPtr->tokenPtr->lastStatus != dndPtr->tokenPtr->status) {
	EventuallyRedrawToken(dndPtr);
    }
    MoveToken(dndPtr);		/* Move token to current drag point. */ 
    RaiseToken(dndPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 *  DropOp --
 *
 *	Finishes the drag&drop transaction by dropping the data on the
 *	selected target.  Typically this operation is called from a 
 *	ButtonRelease event on a source widget.  Note that a Leave message
 *	is always sent to the target so that is can un-highlight itself.
 *	The token is hidden and a drop message is sent to the target.
 *
 *	Example: dnd drop .widget x y 
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	The token is hidden and a drop message is sent to the target.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DropOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Winfo *windowPtr;
    Dnd *dndPtr;
    int x, y;

    if (GetDndFromObj(clientData, interp, objv[2], &dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!dndPtr->isSource) {
	Tcl_AppendResult(interp, "widget \"", Tk_PathName(dndPtr->tkwin), 
	 "\" is not a registered drag&drop source.", (char *)NULL);
	return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    dndPtr->x = x;	/* Save drag&drop location */
    dndPtr->y = y;
    if ((dndPtr->flags & DND_INITIATED) == 0) {
	return TCL_OK;		/* Never initiated any drag operation. */
    }
    if (dndPtr->flags & DND_VOIDED) {
	HideToken(dndPtr);
	return TCL_OK;
    }
    windowPtr = OverTarget(dndPtr);
    if (windowPtr != NULL) {
	if (windowPtr->matches != NULL) {
	    SetProperty(dndPtr->tkwin, dndPtr->dataPtr->formatsAtom, 
			windowPtr->matches);
	}
	MoveToken(dndPtr);	/* Move token to current drag point. */ 
	RaiseToken(dndPtr);
	RelayDropEvent(dndPtr, windowPtr, x, y);
#ifdef notdef
	tokenPtr->numSteps = 10;
	FadeToken(dndPtr);
#endif
    } else {
	CancelDrag(dndPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  GetdataOp --
 *
 *	Registers one or more data formats with a drag&drop source.  
 *	Each format has a TCL command associated with it.  This command
 *	is automatically invoked whenever data is pulled from the source
 *	to a target requesting the data in that particular format.  The 
 *	purpose of the TCL command is to get the data from in the 
 *	application. When the TCL command is invoked, special percent 
 *	substitutions are made:
 *
 *		%#		Drag&drop transaction timestamp.
 *		%W		Source widget.
 *
 *	If a converter (command) already exists for a format, it 
 *	overwrites the existing command.
 *
 *	Example: dnd getdata .widget "color" { %W cget -bg }
 *
 * Results:
 *	A standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
static int
GetdataOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Dnd *dndPtr;
    int i;

    if (GetDndFromObj(clientData, interp, objv[2], &dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch cursor;

	/* Return list of source data formats */
	for (hPtr = Blt_FirstHashEntry(&dndPtr->getDataTable, &cursor);
	    hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    Tcl_AppendElement(interp,
		Blt_GetHashKey(&dndPtr->getDataTable, hPtr));
	}
	return TCL_OK;
    }

    if (objc == 4) {
	Blt_HashEntry *hPtr;
	const char *string;
	const char **argv;

	string = Tcl_GetString(objv[3]);
	hPtr = Blt_FindHashEntry(&dndPtr->getDataTable, string);
	if (hPtr == NULL) {
	    Tcl_AppendResult(interp, "can't find handler for format \"", 
		string, "\" for source \"", Tk_PathName(dndPtr->tkwin), "\"",
	     (char *)NULL);
	    return TCL_ERROR;
	}
	argv = Blt_GetHashValue(hPtr);
	if (argv == NULL) {
	    Tcl_SetStringObj(Tcl_GetObjResult(interp), "", -1);
	} else {
	    Tcl_SetObjResult(interp, PrintList(interp, argv));
	}
	return TCL_OK;
    }

    for (i = 3; i < objc; i += 2) {
	Blt_HashEntry *hPtr;
	const char **argv;
	int argc;
	int isNew;

	hPtr = Blt_CreateHashEntry(&dndPtr->getDataTable, 
		Tcl_GetString(objv[i]), &isNew);
	if (!isNew) {
	    argv = Blt_GetHashValue(hPtr);
	    Blt_Free(argv);
	}
	if (Tcl_SplitList(interp, Tcl_GetString(objv[i + 1]), &argc, &argv) 
	    != TCL_OK) {
	    Blt_DeleteHashEntry(&dndPtr->getDataTable, hPtr);
	    return TCL_ERROR;
	}
	Blt_SetHashValue(hPtr, argv);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  NamesOp --
 *
 *	Returns the names of all the drag&drop managers.  If either
 *	a "-source" or "-target" switch is present, only the names of
 *	managers acting as sources or targets respectively are returned.
 *	A pattern argument may also be given.  Only those managers
 *	matching the pattern are returned.
 *
 *	Examples: dnd names
 *		  dnd names -source
 *		  dnd names -target
 *		  dnd names .*label
 * Results:
 *	A standard TCL result.  A TCL list of drag&drop manager
 *	names is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
NamesOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    DndInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    Dnd *dndPtr;
    int findSources, findTargets;
    char *string;
    Tcl_Obj *listObjPtr;

    findSources = findTargets = TRUE;
    if (objc > 2) {
	string = Tcl_GetString(objv[2]);
	if ((string[0] == '-') && (strcmp(string, "-source") == 0)) {
	    findTargets = FALSE;
	    objc--, objv++;
	} else if ((string[0] == '-') && (strcmp(string, "-target") == 0)) {
	    findSources = FALSE;
	    objc--, objv++;
	}
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hPtr = Blt_FirstHashEntry(&dataPtr->dndTable, &cursor); 
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	dndPtr = Blt_GetHashValue(hPtr);
	if (objc > 3) {
	    string = Tcl_GetString(objv[3]);
	    if (!Tcl_StringMatch(Tk_PathName(dndPtr->tkwin), string)) {
		continue;
	    }
	}
	if (((findSources) && (dndPtr->isSource)) ||
	    ((findTargets) && (dndPtr->isTarget))) {
	    Tcl_ListObjAppendElement(interp, listObjPtr,
			Tcl_NewStringObj(Tk_PathName(dndPtr->tkwin), -1));
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}


#ifdef notdef
RunCommand(Dnd *dndPtr, char **formatCmd)
{
    Tcl_DString ds, savedResult;
    Tcl_Obj **objv;
    char **p;
    
    objc = 0;
    for (objc = 0, p = formatCmd; *p != NULL; p++) {
	objc++;
    }
    objc += 12;
    objv = Blt_AssertMalloc(sizeof(Tcl_Obj *) * (objc + 1));
    for (i = 0; p = formatCmd; *p != NULL; p++, i++) {
	objv[i] = Tcl_NewStringObj(*p, -1);
    }
    objv[i++] = Tcl_NewStringObj(Tk_PathName(dndPtr->tkwin), -1);
    objv[i++] = Tcl_NewStringObj("x", 1);
    objv[i++] = Tcl_NewStringObj("x", 1);
    objv[i++] = Tcl_NewIntObj(dndPtr->dropX);
    objv[i++] = Tcl_NewStringObj("y", 1);
    objv[i++] = Tcl_NewIntObj(dndPtr->dropY);
    objv[i++] = Tcl_NewStringObj("timestamp", 9);
    objv[i++] = Tcl_NewStringObj(Blt_Utoa(dndPtr->pendingPtr->timestamp), -1);
    objv[i++] = Tcl_NewStringObj("format", 6);
    objv[i++] = Tcl_NewStringObj(fmt, -1);
    objv[i++] = Tcl_NewStringObj("value", 5);
    objv[i++] = Tcl_NewStringObj(
	Tcl_DStringValue(&dndPtr->pendingPtr->ds),
	Tcl_DStringLength(&dndPtr->pendingPtr->ds));
    for (i = 0; i < objc; i++) {
	Tcl_IncrRefCount(objv[i]);
    }
    Tcl_DStringInit(&savedResult);
    Tcl_DStringGetResult(interp, &savedResult);
    if (Tcl_EvalObjv(interp, objc, objv, 0) != TCL_OK) {
	Tcl_BackgroundError(interp);
    }
    Tcl_DStringResult(interp, &savedResult);
    for (i = 0; i < objc; i++) {
	Tcl_DecrRefCount(objv[i]);
    }
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 *  PullOp --
 *
 *	Pulls the current data from the source in the given format.
 *	application.
 *
 *	dnd pull .widget format data
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Invokes the target's data converter to store the data. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PullOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Dnd *dndPtr;		/* drag&drop source record */
    int result;
    char **formatCmd;
    char *fmt;
    Blt_HashEntry *hPtr;

    if (GetDndFromObj(clientData, interp, objv[2], &dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!dndPtr->isTarget) {
	Tcl_AppendResult(interp, "widget \"", Tk_PathName(dndPtr->tkwin), 
	 "\" is not a registered drag&drop target.", (char *)NULL);
	return TCL_ERROR;
    }
    fmt = Tcl_GetString(objv[3]);
    hPtr = Blt_FindHashEntry(&dndPtr->setDataTable, fmt);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "can't find format \"", fmt, 
	 "\" in target \"", Tk_PathName(dndPtr->tkwin), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    formatCmd = Blt_GetHashValue(hPtr);
    if (dndPtr->pendingPtr == NULL) {
	Tcl_AppendResult(interp, "no drop in progress", (char *)NULL);
	return TCL_ERROR;
    }

    CompleteDataTransaction(dndPtr, fmt, dndPtr->pendingPtr);
    result = TCL_OK;
    if (Tcl_DStringLength(&dndPtr->pendingPtr->ds) > 0) {
	Tcl_DString ds, savedResult;
	char **p;

	Tcl_DStringInit(&ds);
	for (p = formatCmd; *p != NULL; p++) {
	    Tcl_DStringAppendElement(&ds, *p);
	}
	Tcl_DStringAppendElement(&ds, Tk_PathName(dndPtr->tkwin));
	Tcl_DStringAppendElement(&ds, "x");
	Tcl_DStringAppendElement(&ds, Blt_Itoa(dndPtr->dropX));
	Tcl_DStringAppendElement(&ds, "y");
	Tcl_DStringAppendElement(&ds, Blt_Itoa(dndPtr->dropY));
	Tcl_DStringAppendElement(&ds, "timestamp");
	Tcl_DStringAppendElement(&ds, 
		Blt_Utoa(dndPtr->pendingPtr->timestamp));
	Tcl_DStringAppendElement(&ds, "format");
	Tcl_DStringAppendElement(&ds, Tcl_GetString(objv[3]));
	Tcl_DStringAppendElement(&ds, "value");
	Tcl_DStringAppendElement(&ds, 
		Tcl_DStringValue(&dndPtr->pendingPtr->ds));
	Tcl_DStringInit(&savedResult);
	Tcl_DStringGetResult(interp, &savedResult);
	if (Tcl_GlobalEval(interp, Tcl_DStringValue(&ds)) != TCL_OK) {
	    Tcl_BackgroundError(interp);
	}
	Tcl_DStringResult(interp, &savedResult);
	Tcl_DStringFree(&ds);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 *  SetdataOp --
 *
 *	Registers one or more data formats with a drag&drop target.  
 *	Each format has a TCL command associated with it.  This command
 *	is automatically invoked whenever data arrives from a source
 *	to be converted to that particular format.  The purpose of the 
 *	command is to set the data somewhere in the application (either 
 *	using a TCL command or variable).   When the TCL command is invoked, 
 *	special percent substitutions are made:
 *
 *		%#		Drag&drop transaction timestamp.
 *		%W		Target widget.
 *		%v		Data value transfered from the source to
 *				be converted into the correct format.
 *
 *	If a converter (command) already exists for a format, it 
 *	overwrites the existing command.
 *
 *	Example: dnd setdata .widget color { . configure -bg %v }
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
SetdataOp(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Dnd *dndPtr;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    int i;

    if (GetDndFromObj(clientData, interp, objv[2], &dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 3) {
	/* Show target handler data formats */
	for (hPtr = Blt_FirstHashEntry(&dndPtr->setDataTable, &cursor);
	    hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    Tcl_AppendElement(interp,
		Blt_GetHashKey(&dndPtr->setDataTable, hPtr));
	}
	return TCL_OK;
    }
    if (objc == 4) {
	const char **argv;

	hPtr = Blt_FindHashEntry(&dndPtr->setDataTable, objv[3]);
	if (hPtr == NULL) {
	    Tcl_AppendResult(interp, "can't find handler for format \"", 
	     objv[3], "\" for target \"", Tk_PathName(dndPtr->tkwin), "\"",
	     (char *)NULL);
	    return TCL_ERROR;
	}
	argv = Blt_GetHashValue(hPtr);
	if (argv == NULL) {
	    Tcl_SetStringObj(Tcl_GetObjResult(interp), "", -1);
	} else {
	    Tcl_SetObjResult(interp, PrintList(interp, argv));
	}
	return TCL_OK;
    }
    for (i = 3; i < objc; i += 2) {
	const char **argv;
	int isNew, argc;

	hPtr = Blt_CreateHashEntry(&dndPtr->setDataTable, 
		Tcl_GetString(objv[i]), &isNew);
	if (!isNew) {
	    argv = Blt_GetHashValue(hPtr);
	    Blt_Free(argv);
	}
	if (Tcl_SplitList(interp, Tcl_GetString(objv[i + 1]), &argc, &argv) 
	    != TCL_OK) {
	    Blt_DeleteHashEntry(&dndPtr->setDataTable, hPtr);
	    return TCL_ERROR;
	}
	Blt_SetHashValue(hPtr, argv);
    }
    AddTargetProperty(dndPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  RegisterOp --
 *
 *  dnd register .window 
 *---------------------------------------------------------------------------
 */
static int
RegisterOp(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    DndInterpData *dataPtr = clientData;
    Tk_Window tkwin;
    Blt_HashEntry *hPtr;
    Dnd *dndPtr;
    int isNew;

    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[2]), dataPtr->tkMain);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    hPtr = Blt_CreateHashEntry(&dataPtr->dndTable, (char *)tkwin, &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "\"", Tk_PathName(tkwin), 
	     "\" is already registered as a drag&drop manager", (char *)NULL);
	return TCL_ERROR;
    }
    dndPtr = CreateDnd(interp, tkwin);
    dndPtr->hashPtr = hPtr;
    dndPtr->dataPtr = dataPtr;
    Blt_SetHashValue(hPtr, dndPtr);
    if (Blt_ConfigureWidgetFromObj(interp, dndPtr->tkwin, configSpecs, objc - 3,
	   objv + 3, (char *)dndPtr, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    if (ConfigureDnd(interp, dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  TokenWindowOp --
 *
 *---------------------------------------------------------------------------
 */
static int
TokenWindowOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Dnd *dndPtr;
    int flags;

    if (GetDndFromObj(clientData, interp, objv[3], &dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    flags = 0;
    if (dndPtr->tokenPtr == NULL) {
	if (CreateToken(interp, dndPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	flags = BLT_CONFIG_OBJV_ONLY;
    }
    if (ConfigureToken(interp, dndPtr, objc - 4, objv + 4, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), 
		     Tk_PathName(dndPtr->tokenPtr->tkwin), -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  TokenCgetOp --
 *
 *	Called to process an (objc,objv) list to configure (or
 *	reconfigure) a drag&drop widget.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED*/
static int
TokenCgetOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Dnd *dndPtr;

    if (GetDndFromObj(clientData, interp, objv[3], &dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (dndPtr->tokenPtr == NULL) {
	Tcl_AppendResult(interp, "no token created for \"", objv[3], "\"",
		 (char *)NULL);
	return TCL_ERROR;
    }
    return Blt_ConfigureValueFromObj(interp, dndPtr->tokenPtr->tkwin, 
	tokenConfigSpecs, (char *)dndPtr->tokenPtr, objv[4], 
	BLT_CONFIG_OBJV_ONLY);
}

/*
 *---------------------------------------------------------------------------
 *
 *  TokenConfigureOp --
 *
 *---------------------------------------------------------------------------
 */
static int
TokenConfigureOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Token *tokenPtr;
    Dnd *dndPtr;
    int flags;

    if (GetDndFromObj(clientData, interp, objv[3], &dndPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    flags = BLT_CONFIG_OBJV_ONLY;
    if (dndPtr->tokenPtr == NULL) {
	Tcl_AppendResult(interp, "no token created for \"", objv[3], "\"",
		 (char *)NULL);
	return TCL_ERROR;
    }
    tokenPtr = dndPtr->tokenPtr;
    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, tokenPtr->tkwin, 
		tokenConfigSpecs, (char *)tokenPtr, (Tcl_Obj *)NULL, flags);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, tokenPtr->tkwin, 
		tokenConfigSpecs, (char *)tokenPtr, objv[3], flags);
    } 
    return ConfigureToken(interp, dndPtr, objc - 4, objv + 4, flags);
}

static Blt_OpSpec tokenOps[] =
{
    {"cget",      2, TokenCgetOp,      5, 5, "widget option",},
    {"configure", 2, TokenConfigureOp, 4, 0, "widget ?option value?...",},
    {"window",    1, TokenWindowOp,    4, 0, "widget ?option value?...",},
};

static int numTokenOps = sizeof(tokenOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 *  TokenOp --
 *
 *---------------------------------------------------------------------------
 */
static int
TokenOp(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numTokenOps, tokenOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}

static Blt_OpSpec dndOps[] =
{
    {"cancel",    2, CancelOp, 3, 3, "widget",},
    {"cget",      2, CgetOp, 4, 4, "widget option",},
    {"configure", 4, ConfigureOp, 3, 0, "widget ?option value?...",},
#ifdef notdef
    {"convert",   4, ConvertOp, 5, 5, "widget data format",},
#endif
    {"delete",    2, DeleteOp, 3, 0,"?-source|-target? widget...",},
    {"drag",      3, DragOp, 3, 0, "widget x y ?token?",},
    {"drop",      3, DropOp, 3, 0, "widget x y ?token?",},
    {"getdata",   1, GetdataOp, 3, 0, "widget ?format command?",},
    {"names",     1, NamesOp, 2, 4, "?-source|-target? ?pattern?",},
    {"pull",      1, PullOp, 4, 4, "widget format",},
    {"register",  1, RegisterOp, 3, 0, "widget ?option value?...",},
    {"select",    3, SelectOp, 6, 6, "widget x y timestamp",},
    {"setdata",   3, SetdataOp, 3, 0, "widget ?format command?",},
    {"token",     1, TokenOp, 3, 0, "args...",},
};

static int numDndOps = sizeof(dndOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 *  DndCmd --
 *
 *	Invoked by TCL whenever the user issues a drag&drop command.
 *
 *---------------------------------------------------------------------------
 */
static int
DndCmd(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp,		/* current interpreter */
    int objc,			/* number of arguments */
    Tcl_Obj *const *objv)	/* Argument strings */
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numDndOps, dndOps, BLT_OP_ARG1, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * DndInterpDeleteProc --
 *
 *	This is called when the interpreter hosting the "dnd" command is 
 *	destroyed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Destroys the hash table containing the drag&drop managers.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
DndInterpDeleteProc(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp)
{
    DndInterpData *dataPtr = clientData;
    Dnd *dndPtr;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    for (hPtr = Blt_FirstHashEntry(&dataPtr->dndTable, &cursor);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	dndPtr = Blt_GetHashValue(hPtr);
	dndPtr->hashPtr = NULL;
	DestroyDnd((DestroyData)dndPtr);
    }
    Blt_DeleteHashTable(&dataPtr->dndTable);
    Tcl_DeleteAssocData(interp, DND_THREAD_KEY);
    Blt_Free(dataPtr);
}

static DndInterpData *
GetDndInterpData(Tcl_Interp *interp)
{
    DndInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (DndInterpData *)Tcl_GetAssocData(interp, DND_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
	Display *display;
	Tk_Window tkwin;

	dataPtr = Blt_AssertMalloc(sizeof(DndInterpData));
	tkwin = Tk_MainWindow(interp);
	display = Tk_Display(tkwin);
	dataPtr->tkMain = tkwin;
	dataPtr->display = display;
	Tcl_SetAssocData(interp, DND_THREAD_KEY, DndInterpDeleteProc,
		dataPtr);
	Blt_InitHashTable(&dataPtr->dndTable, BLT_ONE_WORD_KEYS);
	dataPtr->mesgAtom = XInternAtom(display, "BLT Dnd Message", False);
	dataPtr->targetAtom = XInternAtom(display, "BLT Dnd Target", False);
	dataPtr->formatsAtom = XInternAtom(display, "BLT Dnd Formats",False);
	dataPtr->commAtom = XInternAtom(display, "BLT Dnd CommData", False);

#ifdef HAVE_XDND
	dataPtr->XdndActionListAtom = XInternAtom(display, "XdndActionList", 
		False);
	dataPtr->XdndAwareAtom = XInternAtom(display, "XdndAware", False);
	dataPtr->XdndEnterAtom = XInternAtom(display, "XdndEnter", False);
	dataPtr->XdndFinishedAtom = XInternAtom(display, "XdndFinished", False);
	dataPtr->XdndLeaveAtom = XInternAtom(display, "XdndLeave", False);
	dataPtr->XdndPositionAtom = XInternAtom(display, "XdndPosition", False);
	dataPtr->XdndSelectionAtom = XInternAtom(display, "XdndSelection", 
						 False);
	dataPtr->XdndStatusAtom = XInternAtom(display, "XdndStatus", False);
	dataPtr->XdndTypeListAtom = XInternAtom(display, "XdndTypeList", False);
#endif /* HAVE_XDND */
    }
    return dataPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 *  Blt_DndCmdInitProc --
 *
 *	Adds the drag&drop command to the given interpreter.  Should
 *	be invoked to properly install the command whenever a new
 *	interpreter is created.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_DndCmdInitProc(Tcl_Interp *interp) /* Interpreter to be updated */
{
    static Blt_CmdSpec cmdSpec = { "dnd", DndCmd };

    cmdSpec.clientData = GetDndInterpData(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

#ifdef notdef
/*
 * Registers bitmap outline of dragged data, used to indicate
 * what is being dragged by source.  Bitmap is XOR-ed as cursor/token
 * is moved around the screen.
 */
static void
Blt_DndSetOutlineBitmap(Tk_Window tkwin, Pixmap bitmap, int x, int y)
{
    
}
#endif

#ifdef HAVE_XDND

static void
XDndFreeFormats(XDndHandler *handlerPtr)
{
    if (handlerPtr->formatArr != NULL) {
	char **p;

	for (p = handlerPtr->formatArr; *p != NULL; p++) {
	    XFree(*p);
	}
	Blt_Free(handlerPtr->formatArr);
	handlerPtr->formatArr = NULL;
    }
}

static char **
XDndGetFormats(XDndHandler *handlerPtr, XEvent *eventPtr)
{
    int flags;
    Window window;
    unsigned char *data;
    Atom typeAtom;
    Atom format;
    int numItems, bytesAfter;
    Atom *atomArr;
    char *nameArr[XDND_MAX_TYPES + 1];
    Display *display;

    XDndFreeFormats(handlerPtr);
    display = eventPtr->xclient.display;
    window = eventPtr->xclient.data.l[0];
    flags = eventPtr->xclient.data.l[1];
    data = NULL;
    if (flags & 0x01) {
	result = XGetWindowProperty(
	    display,		/* Display of window. */
	    window,		/* Window holding the property. */
	    handlerPtr->dataPtr->XdndTypeListAtom, /* Name of property. */
	    0,			/* Offset of data (for multiple reads). */
	    XDND_MAX_TYPES,	/* Maximum number of items to read. */
	    False,		/* If true, delete the property. */
	    XA_ATOM,		/* Desired type of property. */
	    &typeAtom,		/* (out) Actual type of the property. */
	    &format,		/* (out) Actual format of the property. */
	    &numItems,		/* (out) # of items in specified format. */
	    &bytesAfter,	/* (out) # of bytes remaining to be read. */
	    (unsigned char **)&data);
	if ((result != Success) || (format != 32) || (typeAtom != XA_ATOM)) {
	    if (data != NULL) {
		XFree((char *)data);
		return (char **)NULL;
	    }
	}
	atomArr = (Atom *)data;
	nAtoms = numItems;
    } else {
	atomArr = &(eventPtr->xclient.data.l[2]);
	nAtoms = 3;
    }
    formatArr = Blt_AssertCalloc(nAtoms + 1, sizeof(char *));
    for (i = 0; (i < numAtoms) && (atomArr[i] != None); i++) {
	formatArr[i] = XGetAtomName(display, atomArr[i]);
    }
    if (data != NULL) {
	XFree((char *)data);
    }
    handlerPtr->formatArr = formatArr;
}

static char *
GetMatchingFormats(Dnd *dndPtr, char **formatArr)
{
    int numMatches;

    numMatches = 0;
    Tcl_DStringInit(&ds);
    for (p = formatArr; *p != NULL; p++) {
	for(hPtr = Blt_FirstHashEntry(&dndPtr->setDataTable, &cursor);
	    hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    fmt = Blt_GetHashKey(&dndPtr->setDataTable, hPtr);
	    if ((*fmt == **p) && (strcmp(fmt, *p) == 0)) {
		Tcl_DStringAppendElement(&ds, *p);
		nMatches++;
		break;
	    }
	}
    }
    if (nMatches > 0) {
	char *string;

	string = Blt_AssertStrdup(Tcl_DStringValue(&ds));
	Tcl_DStringFree(&ds);
	return string;
    }
    return NULL;
}

static void
XDndPointerEvent(XDndHandler *handlerPtr, XEvent *eventPtr)
{
    Tk_Window tkwin;
    int flags;
    Atom action;
    Window window;
    
    flags = 0;
    action = None;
    window = eventPtr->xclient.data.l[MESG_XDND_WINDOW];

    /* 
     * If the XDND source has no formats specified, don't process any
     * further.  Simply send a "no accept" action with the message.
     */
    if (handlerPtr->formatArr != NULL) { 
	Dnd *newPtr, *oldPtr;
	int point;
	int button, keyState;
	int x, y;
	char *formats;

	point = (int)eventPtr->xclient.data.l[MESG_XDND_POINT];
	UNPACK(point, x, y);

	/*  
	 * See if the mouse pointer currently over a drop target. We first
	 * determine what Tk window is under the mouse, and then check if 
	 * that window is registered as a drop target.  
	 */
	newPtr = NULL;
	tkwin = Tk_CoordsToWindow(x, y, handlerPtr->tkwin);
	if (tkwin != NULL) {
	    Blt_HashEntry *hPtr;
	    
	    hPtr = Blt_FindHashEntry(&handlerPtr->dataPtr->dndTable, 
				     (char *)tkwin);
	    if (hPtr != NULL) {
		newPtr = Blt_GetHashValue(hPtr);
		if (!newPtr->isTarget) {
		    newPtr = NULL; /* Not a DND target. */
		}
		formats = GetMatchingFormats(newPtr, handlerPtr->formatArr);
		if (formats == NULL) {
		    newPtr = NULL; /* Source has no matching formats. */
		} 
	    }
	}
	button = keyState = 0;
	oldPtr = handlerPtr->dndPtr;
	resp = DROP_CANCEL;
	if (newPtr == oldPtr) {
	    if ((oldPtr != NULL) && (oldPtr->motionCmd != NULL)) {
		resp = InvokeCallback(oldPtr, oldPtr->motionCmd, x, y, formats,
			 button, keyState, dndPtr->timestamp);
	    }
	} else {
	    if ((oldPtr != NULL) && (oldPtr->leaveCmd != NULL)) {
		InvokeCallback(oldPtr, oldPtr->leaveCmd, x, y, formats, button,
			 keyState, dndPtr->timestamp);
	    }
	    if ((newPtr != NULL) && (newPtr->enterCmd != NULL)) {
		resp = InvokeCallback(newPtr, newPtr->enterCmd, x, y, formats, 
			button, keyState, dndPtr->timestamp);
	    }
	    handlerPtr->dndPtr = newPtr;
	    /* 
	     * Save the current mouse position, since we get them from the
	     * drop message. 
	     */
	    newPtr->x = x;	
	    newPtr->y = y;
	} 
	if (formats != NULL) {
	    Blt_Free(formats);
	}
	flags = XDND_FLAGS_WANT_POSITION_MSGS;
	if (resp) {
	    flags |= XDND_FLAGS_ACCEPT_DROP;
	    action = handlerPtr->dataPtr->XdndActionCopyAtom;
	}
    }
    /* Target-to-Source: Drag result message. */
    SendClientMsg(handlerPtr->display, window, 
	handlerPtr->dataPtr->XdndStatusAtom, handlerPtr->window, 
	flags, 0, 0, action);
}

static void
XDndDropEvent(XDndHandler *handlerPtr, XEvent *eventPtr)
{
    Tk_Window tkwin;
    int flags;
    Atom action;
    Window window;
    int timestamp;

    flags = 0;
    action = None;
    window = eventPtr->xclient.data.l[MESG_XDND_WINDOW];
    timestamp = eventPtr->xclient.data.l[MESG_XDND_TIMESTAMP];

    /* 
     * If no formats were specified for the XDND source or if the last 
     * motion event did not place the mouse over a valid drop target, 
     * don't process any further. Simply send a "no accept" action with 
     * the message.
     */
    if ((handlerPtr->formatArr != NULL) && (handlerPtr->dndPtr != NULL)) { 
	int button, keyState;
	Dnd *dndPtr = handlerPtr->dndPtr;
	DropPending pending;
	int resp;

	button = keyState = 0;		/* Protocol doesn't supply this
					 * information. */

	/* Set up temporary bookkeeping for the drop transaction */
	memset (&pending, 0, sizeof(pending));
	pending.window = window;
	pending.display = eventPtr->xclient.display;
	pending.timestamp = timestamp;
	pending.protocol = PROTO_XDND;
	pending.packetSize = GetMaxPropertySize(pending.display);
	Tcl_DStringInit(&pending.ds);
	
	formats = GetMatchingFormats(handlerPtr->dndPtr, handlerPtr->formatArr);
	if (formats == NULL) {
	}
	dndPtr->pendingPtr = &pending;
	resp = AcceptDrop(dndPtr, dndPtr->x, dndPtr->y, formats, button,
	     keyState, action, timestamp);
	dndPtr->pendingPtr = NULL;
	if (resp) {
	    flags |= XDND_FLAGS_ACCEPT_DROP;
	    action = handlerPtr->dataPtr->XdndActionCopyAtom;
	}
    }
    /* Target-to-Source: Drag result message. */
    SendClientMsg(handlerPtr->display, window, 
	handlerPtr->dataPtr->XdndStatusAtom, handlerPtr->window, 
	flags, 0, 0, action);
}

/*
 *---------------------------------------------------------------------------
 *
 *  XDndProtoEventProc --
 *
 *	Invoked by Tk_HandleEvent whenever a DestroyNotify event is received
 *	on a registered drag&drop source widget.
 *
 *---------------------------------------------------------------------------
 */
static int
XDndProtoEventProc(
    ClientData clientData,		/* Drag&drop record. */
    XEvent *eventPtr)			/* Event description. */
{
    DndInterpData *dataPtr = clientData;
    Tk_Window tkwin;
    Blt_HashEntry *hPtr;
    XDndHandler *handlerPtr;
    int point;
    int x, y;
    Atom mesg;

    if (eventPtr->type != ClientMessage) {
	return 0;			/* Not a ClientMessage event. */
    }
    /* Was the recipient a registered toplevel window? */
    hPtr = Blt_FindHashEntry(&dataPtr->handlerTable, 
	     (char *)eventPtr->xclient.window);
    if (hPtr == NULL) {
	return 0;			/* No handler registered with window. */
    }
    handlerPtr = Blt_GetHashValue(hPtr);
    mesg = eventPtr->xclient.message_type;
    if (mesg == dataPtr->XdndEnterAtom) {
	XDndGetFormats(handlerPtr, eventPtr);
	handlerPtr->dndPtr = NULL;
    } else if (mesg == dataPtr->XdndPositionAtom) {
	XDndPointerEvent(handlerPtr, eventPtr);
    } else if (mesg == dataPtr->XdndLeaveAtom) {
	XDndFreeFormats(handlerPtr);	/* Free up any collected formats. */
	if (handlerPtr->dndPtr != NULL) {
	    InvokeCallback(handlerPtr->dndPtr, handlerPtr->dndPtr->leaveCmd, 
		-1, -1, NULL, 0, 0);
	    /* Send leave event to drop target. */
	}
    } else if (mesg == dataPtr->XdndDropAtom) {
	XDndDropEvent(handlerPtr, eventPtr);
    } else {
	fprintf(stderr, "Unknown client message type = 0x%x\n", mesg);
	return 0;			/* Unknown message type.  */
    }
    return 1;
}

static XDndHandler *
XDndCreateHandler(Dnd *dndPtr)
{
    Tk_Window tkwin;
    Window window;
    Blt_HashEntry *hPtr;
    int isNew;
    XDndHandler *handlerPtr;

    /* 
     * Find the containing toplevel of this window. See if an XDND handler is
     * already registered for it.
     */
    tkwin = Blt_Toplevel(dndPtr->tkwin);
    window = Blt_GetWindowId(tkwin);	/* Use the wrapper window as
					 * the real toplevel window. */
    hPtr = Blt_CreateHashEntry(&dataPtr->XDndHandlerTable, (char *)window, 
	&isNew);
    if (!isNew) {
	handlerPtr = (XDndHandler *)Blt_GetHashEntry(hPtr);
	handlerPtr->refCount++;
    } else {
	handlerPtr = Blt_AssertMalloc(sizeof(XDndHandler));
	handlerPtr->tkwin = tkwin;
	handlerPtr->dndPtr = NULL;
	handlerPtr->refCount = 1;
	handlerPtr->dataPtr = dataPtr;
	/* FIXME */
	SetProperty(window, dataPtr->XdndAwareAtom, "3");
	Blt_SetHashValue(hPtr, handlerPtr);
    }
    return handlerPtr;
}

static void
XDndDeleteHandler(Dnd *dndPtr)
{
    Tk_Window tkwin;
    Window window;
    Blt_HashEntry *hPtr;

    tkwin = Blt_Toplevel(dndPtr->tkwin);
    window = Blt_GetWindowId(tkwin);	/* Use the wrapper window as the real
					 * toplevel window. */
    hPtr = Blt_FindHashEntry(&dataPtr->XDndHandlerTable, (char *)window);
    if (hPtr != NULL) {
	XDndHandler *handlerPtr;

	handlerPtr = (XDndHandler *)Blt_GetHashEntry(hPtr);
	handlerPtr->refCount--;
	if (handlerPtr->refCount == 0) {
	    XDndFreeFormats(handlerPtr); 
	    XDeleteProperty(dndPtr->display, window, 
		dndPtr->dataPtr->XdndAwareAtom);
	    Blt_DeleteHashEntry(&dataPtr->XDndHandlerTable, hPtr);
	    Blt_Free(handlerPtr);
	}
    }
}

#endif /* HAVE_XDND */

#endif /* NO_DRAGDROP */

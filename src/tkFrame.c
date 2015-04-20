/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * tkFrame.c --
 *
 *	This module implements "frame"  and "toplevel" widgets for
 *	the Tk toolkit.  Frames are windows with a background color
 *	and possibly a 3-D effect, but not much else in the way of
 *	attributes.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 *
 *   See the file "license.terms" for information on usage and
 *   redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
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

#ifndef NO_TKFRAME

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltBg.h"
#include "bltInitCmd.h"


/*
 * Defaults for frames:
 */

#define DEF_FRAME_BACKGROUND		STD_NORMAL_BACKGROUND
#define DEF_FRAME_BG_MONO		STD_NORMAL_BG_MONO
#define DEF_FRAME_BORDERWIDTH		"0"
#define DEF_FRAME_CLASS			"Frame"
#define DEF_FRAME_COLORMAP		""
#define DEF_FRAME_CONTAINER		"0"
#define DEF_FRAME_CURSOR		""
#define DEF_FRAME_HEIGHT		"0"
#define DEF_FRAME_HIGHLIGHT_BG		STD_NORMAL_BACKGROUND
#define DEF_FRAME_HIGHLIGHT		RGB_BLACK
#define DEF_FRAME_HIGHLIGHT_WIDTH	"0"
#define DEF_FRAME_RELIEF		"flat"
#define DEF_FRAME_TAKE_FOCUS		"0"
#define DEF_FRAME_USE			""
#define DEF_FRAME_VISUAL		""
#define DEF_FRAME_WIDTH			"0"

/*
 * Defaults for toplevels (most of the defaults for frames also apply
 * to toplevels):
 */

#define DEF_TOPLEVEL_CLASS		"Toplevel"
#define DEF_TOPLEVEL_SCREEN		""
#define DEF_TOPLEVEL_MENU		""

/*
 * A data structure of the following type is kept for each
 * frame that currently exists for this process:
 */

typedef struct {
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
    char *className;		/* Class name for widget (from configuration
				 * option).  Malloc-ed. */
    int mask;			/* Either FRAME or TOPLEVEL;  used to select
				 * which configuration options are valid for
				 * widget. */
    char *screenName;		/* Screen on which widget is created.  Non-null
				 * only for top-levels.  Malloc-ed, may be
				 * NULL. */
    char *visualName;		/* Textual description of visual for window,
				 * from -visual option.  Malloc-ed, may be
				 * NULL. */
    char *colormapName;		/* Textual description of colormap for window,
				 * from -colormap option.  Malloc-ed, may be
				 * NULL. */
    char *menuName;		/* Textual description of menu to use for
				 * menubar. Malloc-ed, may be NULL. */
    Colormap colormap;		/* If not None, identifies a colormap
				 * allocated for this window, which must be
				 * freed when the window is deleted. */
    Blt_Bg normalBg;		      /* Structure used to draw 3-D border and
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
    char *takeFocus;		/* Value of -takefocus option;  not used in
				 * the C code, but used by keyboard traversal
				 * scripts.  Malloc'ed, but may be NULL. */
    int isContainer;		/* 1 means this window is a container, 0 means
				 * that it isn't. */
    char *useThis;		/* If the window is embedded, this points to
				 * the name of the window in which it is
				 * embedded (malloc'ed).  For non-embedded
				 * windows this is NULL. */
    int flags;			/* Various flags;  see below for
				 * definitions. */
} Frame;

/*
 * Flag bits for frames:
 *
 * REDRAW_PENDING:		Non-zero means a DoWhenIdle handler
 *				has already been queued to redraw
 *				this window.
 * GOT_FOCUS:			Non-zero means this widget currently
 *				has the input focus.
 */

#define REDRAW_PENDING		1
#define GOT_FOCUS		4

/*
 * The following flag bits are used so that there can be separate
 * defaults for some configuration options for frames and toplevels.
 */

#define FRAME		BLT_CONFIG_USER_BIT
#define TOPLEVEL	(BLT_CONFIG_USER_BIT << 1)
#define BOTH		(FRAME | TOPLEVEL)

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	DEF_FRAME_BACKGROUND, Blt_Offset(Frame, normalBg),
	BOTH | BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	DEF_FRAME_BG_MONO, Blt_Offset(Frame, normalBg),
	BOTH | BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL,
	(char *)NULL, 0, BOTH},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL,
	(char *)NULL, 0, BOTH},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_FRAME_BORDERWIDTH, Blt_Offset(Frame, borderWidth), BOTH},
    {BLT_CONFIG_STRING, "-class", "class", "Class",
	DEF_FRAME_CLASS, Blt_Offset(Frame, className), FRAME},
    {BLT_CONFIG_STRING, "-class", "class", "Class",
	DEF_TOPLEVEL_CLASS, Blt_Offset(Frame, className), TOPLEVEL},
    {BLT_CONFIG_STRING, "-colormap", "colormap", "Colormap",
	DEF_FRAME_COLORMAP, Blt_Offset(Frame, colormapName),
	BOTH | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-container", "container", "Container",
	DEF_FRAME_CONTAINER, Blt_Offset(Frame, isContainer), BOTH},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_FRAME_CURSOR, Blt_Offset(Frame, cursor), BOTH | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height",
	DEF_FRAME_HEIGHT, Blt_Offset(Frame, height), BOTH},
    {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_FRAME_HIGHLIGHT_BG,
	Blt_Offset(Frame, highlightBgColorPtr), BOTH},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_FRAME_HIGHLIGHT, Blt_Offset(Frame, highlightColorPtr), BOTH},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_FRAME_HIGHLIGHT_WIDTH, 
	Blt_Offset(Frame, highlightWidth), BOTH},
    {BLT_CONFIG_STRING, "-menu", "menu", "Menu",
	DEF_TOPLEVEL_MENU, Blt_Offset(Frame, menuName),
	TOPLEVEL | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief",
	DEF_FRAME_RELIEF, Blt_Offset(Frame, relief), BOTH},
    {BLT_CONFIG_STRING, "-screen", "screen", "Screen",
	DEF_TOPLEVEL_SCREEN, Blt_Offset(Frame, screenName),
	TOPLEVEL | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_FRAME_TAKE_FOCUS, Blt_Offset(Frame, takeFocus),
	BOTH | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-use", "use", "Use",
	DEF_FRAME_USE, Blt_Offset(Frame, useThis), TOPLEVEL|BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-visual", "visual", "Visual",
	DEF_FRAME_VISUAL, Blt_Offset(Frame, visualName),
	BOTH | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width",
	DEF_FRAME_WIDTH, Blt_Offset(Frame, width), BOTH},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static int ConfigureFrame (Tcl_Interp *interp, Frame * framePtr, 
	int objc, Tcl_Obj *const *objv, int flags);

static Tcl_FreeProc DestroyFrame;
static Tcl_IdleProc DisplayFrame;
static Tcl_CmdDeleteProc FrameCmdDeletedProc;
static Tk_EventProc FrameEventProc;
static Tcl_ObjCmdProc FrameWidgetCmd;
static Tcl_IdleProc MapFrame;

static Tcl_ObjCmdProc FrameCmd, ToplevelCmd;

#ifdef TK_MAINWINDOW
BLT_EXTERN 
#else
static
#endif
int TkCreateFrame (ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv, int toplevel, char *appName);

#ifndef USE_TK_STUBS
BLT_EXTERN void TkSetWindowMenuBar (Tcl_Interp *interp, Tk_Window tkwin,
	char *oldMenuName, char *menuName);

BLT_EXTERN Tk_Window TkCreateMainWindow (Tcl_Interp * interp, 
	char *screenName, char *baseName);

BLT_EXTERN void TkpSetMainMenubar (Tcl_Interp * interp, Tk_Window tkwin, 
	char *menuName);
BLT_EXTERN int TkpUseWindow (Tcl_Interp * interp, Tk_Window tkwin, char * string);
BLT_EXTERN void TkpMakeContainer (Tk_Window tkwin);
#endif	/* USE_TK_STUBS */

#if (_TK_VERSION < _VERSION(8,5,0)) 
static void 
Tk_SetClassProcs(Tk_Window tkwin, void *procs, ClientData instanceData)
{
    TkSetClassProcs(tkwin, procs, instanceData);
}
#endif 


/*
 *---------------------------------------------------------------------------
 *
 * FrameCmd, ToplevelCmd --
 *
 *	These procedures are invoked to process the "frame" and
 *	"toplevel" TCL commands.  See the user documentation for
 *	details on what they do.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.  These procedures are just wrappers;
 *	they call ButtonCreate to do all of the real work.
 *
 *---------------------------------------------------------------------------
 */
static int
FrameCmd(
    ClientData clientData,	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Argument strings. */
{
    return TkCreateFrame(clientData, interp, objc, objv, 0, (char *)NULL);
}

static int
ToplevelCmd(
    ClientData clientData,	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Argument strings. */
{
    return TkCreateFrame(clientData, interp, objc, objv, 1, (char *)NULL);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkFrameCreate --
 *
 *	This procedure is invoked to process the "frame" and "toplevel"
 *	TCL commands;  it is also invoked directly by Tk_Init to create
 *	a new main window.  See the user documentation for the "frame"
 *	and "toplevel" commands for details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

/*ARGSUSED*/
#ifndef TK_MAINWINDOW
static
#endif /* TK_MAINWINDOW */
int
TkCreateFrame(
    ClientData clientData,	/* Main window associated with interpreter.
				 * If we're called by Tk_Init to create a
				 * new application, then this is NULL. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv,		/* Argument strings. */
    int toplevel,		/* Non-zero means create a toplevel window,
				 * zero means create a frame. */
    char *appName)		/* Should only be non-NULL if clientData is
				 * NULL:  gives the base name to use for the
				 * new application. */
{
    Frame *framePtr;
    Tk_Window new;
    const char *className, *screenName, *visualName, *colormapName, *arg, 
	*useOption;
    int i, c, depth;
    unsigned int mask;
    Colormap colormap;
    Visual *visual;
    Tk_Window tkwin;

    if (objc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
	    objv[0], " pathName ?options?\"", (char *)NULL);
	return TCL_ERROR;
    }
    /*
     * Pre-process the argument list.  Scan through it to find any
     * "-class", "-screen", "-visual", and "-colormap" options.  These
     * arguments need to be processed specially, before the window
     * is configured using the usual Tk mechanisms.
     */

    className = colormapName = screenName = visualName = useOption = NULL;
    colormap = None;
    for (i = 2; i < objc; i += 2) {
	int length;

	arg = Tcl_GetStringFromObj(objv[i], &length);
	if (length < 2) {
	    continue;
	}
	c = arg[1];
	if ((c == 'c') && (strncmp(arg, "-class", length) == 0)
	    && (length >= 3)) {
	    className = Tcl_GetString(objv[i + 1]);
	} else if ((c == 'c')
	    && (strncmp(arg, "-colormap", length) == 0)) {
	    colormapName = Tcl_GetString(objv[i + 1]);
	} else if ((c == 's') && (toplevel) && 
		   (strncmp(arg, "-screen", length) == 0)) {
	    screenName = Tcl_GetString(objv[i + 1]);
	} else if ((c == 'u') && (toplevel) && 
		   (strncmp(arg, "-use", length) == 0)) {
	    useOption = Tcl_GetString(objv[i + 1]);
	} else if ((c == 'v') && (strncmp(arg, "-visual", length) == 0)) {
	    visualName = Tcl_GetString(objv[i + 1]);
	}
    }

    /*
     * Create the window, and deal with the special options -use,
     * -classname, -colormap, -screenname, and -visual.  These options
     * must be handle before calling ConfigureFrame below, and they must
     * also be processed in a particular order, for the following
     * reasons:
     * 1. Must set the window's class before calling ConfigureFrame,
     *    so that unspecified options are looked up in the option
     *    database using the correct class.
     * 2. Must set visual information before calling ConfigureFrame
     *    so that colors are allocated in a proper colormap.
     * 3. Must call TkpUseWindow before setting non-default visual
     *    information, since TkpUseWindow changes the defaults.
     */

    if (screenName == NULL) {
	screenName = (toplevel) ? "" : NULL;
    }
    tkwin = Tk_MainWindow(interp);
    if (tkwin != NULL) {
	new = Tk_CreateWindowFromPath(interp, tkwin, Tcl_GetString(objv[1]), 
		screenName);
    } else {
	/*
	 * We were called from Tk_Init;  create a new application.
	 */

	if (appName == NULL) {
	    panic("TkCreateFrame didn't get application name");
	}
	new = (Tk_Window)TkCreateMainWindow(interp, (char *)screenName,appName);
    }
    if (new == NULL) {
	goto error;
    }
    if (className == NULL) {
	className = (char *)Tk_GetOption(new, "class", "Class");
	if (className == NULL) {
	    className = (toplevel) ? "BltTkToplevel" : "BltTkFrame";
	}
    }
    Tk_SetClass(new, className);
    if (useOption == NULL) {
	useOption = Tk_GetOption(new, "use", "Use");
    }
    if (useOption != NULL) {
	if (TkpUseWindow(interp, new, (char *)useOption) != TCL_OK) {
	    goto error;
	}
    }
    if (visualName == NULL) {
	visualName = (char *)Tk_GetOption(new, "visual", "Visual");
    }
    if (colormapName == NULL) {
	colormapName = (char *)Tk_GetOption(new, "colormap", "Colormap");
    }
    if (visualName != NULL) {
	visual = Tk_GetVisual(interp, new, visualName, &depth,
	    (colormapName == NULL) ? &colormap : (Colormap *) NULL);
	if (visual == NULL) {
	    goto error;
	}
	Tk_SetWindowVisual(new, visual, depth, colormap);
    }
    if (colormapName != NULL) {
	colormap = Tk_GetColormap(interp, new, colormapName);
	if (colormap == None) {
	    goto error;
	}
	Tk_SetWindowColormap(new, colormap);
    }
    /*
     * For top-level windows, provide an initial geometry request of
     * 200x200,  just so the window looks nicer on the screen if it
     * doesn't request a size for itself.
     */

    if (toplevel) {
	Tk_GeometryRequest(new, 200, 200);
    }
    /*
     * Create the widget record, process configuration options, and
     * create event handlers.  Then fill in a few additional fields
     * in the widget record from the special options.
     */

    framePtr = Blt_AssertCalloc(1, sizeof(Frame));
    framePtr->tkwin = new;
    framePtr->display = Tk_Display(new);
    framePtr->interp = interp;
    framePtr->widgetCmd = Tcl_CreateObjCommand(interp, Tk_PathName(new), 
	FrameWidgetCmd, (ClientData)framePtr, FrameCmdDeletedProc);
    framePtr->mask = (toplevel) ? TOPLEVEL : FRAME;
    framePtr->colormap = colormap;
    framePtr->borderWidth = 0;
    framePtr->relief = TK_RELIEF_FLAT;

    /*
     * Store backreference to frame widget in window structure.
     */
    Tk_SetClassProcs(new, NULL, (ClientData)framePtr);
    mask = ExposureMask | StructureNotifyMask | FocusChangeMask;
    if (toplevel) {
	mask |= ActivateMask;
    }
    Tk_CreateEventHandler(new, mask, FrameEventProc, (ClientData)framePtr);
    if (ConfigureFrame(interp, framePtr, objc - 2, objv + 2, 0) != TCL_OK) {
	goto error;
    }
    if ((framePtr->isContainer)) {
	if (framePtr->useThis == NULL) {
	    TkpMakeContainer(framePtr->tkwin);
	} else {
	    Tcl_AppendResult(interp, "A window cannot have both the -use ",
		"and the -container option set.", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (toplevel) {
	Tcl_DoWhenIdle(MapFrame, (ClientData)framePtr);
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(new), -1);
    return TCL_OK;

  error:
    if (new != NULL) {
	Tk_DestroyWindow(new);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * FrameWidgetCmd --
 *
 *	This procedure is invoked to process the TCL command
 *	that corresponds to a frame widget.  See the user
 *	documentation for details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

static int
FrameWidgetCmd(ClientData clientData, Tcl_Interp *interp, int objc,
	       Tcl_Obj *const *objv)
{
    register Frame *framePtr = (Frame *) clientData;
    int result;
    int length;
    int c, i;
    char *string;

    if (objc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		Tcl_GetString(objv[0]), " option ?arg arg ...?\"", 
		(char *)NULL);
	return TCL_ERROR;
    }
    Tcl_Preserve((ClientData)framePtr);
    string = Tcl_GetStringFromObj(objv[1], &length);
    c = string[0];
    if ((c == 'c') && (length >= 2) && (strncmp(string, "cget", length) == 0)) {
	if (objc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		Tcl_GetString(objv[0]), " cget option\"", (char *)NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	result = Blt_ConfigureValueFromObj(interp, framePtr->tkwin, configSpecs,
	    (char *)framePtr, objv[2], framePtr->mask);
    } else if ((c == 'c') && (length >= 2) 
	       && (strncmp(string, "configure", length) == 0)) {
	if (objc == 2) {
	    result = Blt_ConfigureInfoFromObj(interp, framePtr->tkwin, 
		configSpecs, (char *)framePtr, (Tcl_Obj *)NULL, framePtr->mask);
	} else if (objc == 3) {
	    result = Blt_ConfigureInfoFromObj(interp, framePtr->tkwin, 
		configSpecs, (char *)framePtr, objv[2], framePtr->mask);
	} else {
	    /*
	     * Don't allow the options -class, -colormap, -container,
	     * -newcmap, -screen, -use, or -visual to be changed.
	     */

	    for (i = 2; i < objc; i++) {
		string = Tcl_GetStringFromObj(objv[i], &length);
		if (length < 2) {
		    continue;
		}
		c = string[1];
		if (((c == 'c') && (strncmp(string, "-class", length) == 0)) ||
		    ((c == 'c') && (framePtr->mask == TOPLEVEL) && 
		     (length >= 3) && 
		     (strncmp(string, "-colormap", length) == 0)) || 
		    ((c == 'c') && (strncmp(string, "-container", length) == 0)
			&& (length >= 3))
		    || ((c == 's') && (framePtr->mask == TOPLEVEL)
			&& (strncmp(string, "-screen", length) == 0))
		    || ((c == 'u') && (framePtr->mask == TOPLEVEL)
			&& (strncmp(string, "-use", length) == 0))
		    || ((c == 'v') && (framePtr->mask == TOPLEVEL)
			&& (strncmp(string, "-visual", length) == 0))) {
		    Tcl_AppendResult(interp, "can't modify ", string,
			" option after widget is created", (char *)NULL);
		    result = TCL_ERROR;
		    goto done;
		}
	    }
	    result = ConfigureFrame(interp, framePtr, objc - 2, objv + 2,
		BLT_CONFIG_OBJV_ONLY);
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", string,
	    "\": must be cget or configure", (char *)NULL);
	result = TCL_ERROR;
    }

  done:
    Tcl_Release((ClientData)framePtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyFrame --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of a frame at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the frame is freed up.
 *
 *---------------------------------------------------------------------------
 */

static void
DestroyFrame(DestroyData memPtr)	/* Info about frame widget. */
{
    register Frame *framePtr = (Frame *) memPtr;
    
    Blt_FreeOptions(configSpecs, (char *)framePtr, framePtr->display,
	framePtr->mask);
    if (framePtr->colormap != None) {
	Tk_FreeColormap(framePtr->display, framePtr->colormap);
    }
    Blt_Free(framePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureFrame --
 *
 *	This procedure is called to process an objv/objc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) a frame widget.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is
 *	returned, then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for framePtr;  old resources get freed, if there
 *	were any.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureFrame(
    Tcl_Interp *interp,		/* Used for error reporting. */
    register Frame *framePtr,	/* Information about widget;  may or may
				 * not already have values for some fields. */
    int objc,			/* Number of valid entries in objv. */
    Tcl_Obj *const *objv,	/* Arguments. */
    int flags)			/* Flags to pass to Blt_ConfigureWidget. */
{
    const char *oldMenuName;

    /*
     * Need the old menubar name for the menu code to delete it.
     */

    if (framePtr->menuName == NULL) {
	oldMenuName = NULL;
    } else {
	oldMenuName = Blt_AssertStrdup(framePtr->menuName);
    }
    if (Blt_ConfigureWidgetFromObj(interp, framePtr->tkwin, configSpecs,
	    objc, objv, (char *)framePtr, flags | framePtr->mask) != TCL_OK) {
	return TCL_ERROR;
    }
    if (((oldMenuName == NULL) && (framePtr->menuName != NULL))
	|| ((oldMenuName != NULL) && (framePtr->menuName == NULL))
	|| ((oldMenuName != NULL) && (framePtr->menuName != NULL)
	    && strcmp(oldMenuName, framePtr->menuName) != 0)) {
	TkSetWindowMenuBar(interp, framePtr->tkwin, (char *)oldMenuName,
	    framePtr->menuName);
    }
#ifdef notdef
    if (framePtr->normalBg != NULL) {
	Tk_SetBackgroundFromBorder(framePtr->tkwin, framePtr->normalBg);
    } else {
	Tk_SetWindowBackgroundPixmap(framePtr->tkwin, None);
    }
#endif
    Tk_SetWindowBackgroundPixmap(framePtr->tkwin, None);
    if (framePtr->highlightWidth < 0) {
	framePtr->highlightWidth = 0;
    }
    Tk_SetInternalBorder(framePtr->tkwin,
	framePtr->borderWidth + framePtr->highlightWidth);
    
    if ((framePtr->width > 0) || (framePtr->height > 0)) {
	Tk_GeometryRequest(framePtr->tkwin, framePtr->width,
	    framePtr->height);
    }
    if (oldMenuName != NULL) {
	Blt_Free(oldMenuName);
    }
    if (Tk_IsMapped(framePtr->tkwin)) {
	if (!(framePtr->flags & REDRAW_PENDING)) {
	    Tcl_DoWhenIdle(DisplayFrame, (ClientData)framePtr);
	}
	framePtr->flags |= REDRAW_PENDING;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayFrame --
 *
 *	This procedure is invoked to display a frame widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the frame in its
 *	current mode.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayFrame(ClientData clientData) /* Information about widget. */
{
    register Frame *framePtr = (Frame *) clientData;
    register Tk_Window tkwin = framePtr->tkwin;
    GC gc;

    framePtr->flags &= ~REDRAW_PENDING;
    if ((framePtr->tkwin == NULL) || !Tk_IsMapped(tkwin)
	|| framePtr->isContainer) {
	return;
    }
    Blt_Bg_FillRectangle(tkwin, Tk_WindowId(tkwin), framePtr->normalBg, 
	framePtr->highlightWidth, framePtr->highlightWidth,
	Tk_Width(tkwin) - 2 * framePtr->highlightWidth,
	Tk_Height(tkwin) - 2 * framePtr->highlightWidth,
	framePtr->borderWidth, framePtr->relief);
    if (framePtr->highlightWidth != 0) {
	if (framePtr->flags & GOT_FOCUS) {
	    gc = Tk_GCForColor(framePtr->highlightColorPtr,
		Tk_WindowId(tkwin));
	} else {
	    gc = Tk_GCForColor(framePtr->highlightBgColorPtr,
		Tk_WindowId(tkwin));
	}
	Tk_DrawFocusHighlight(tkwin, gc, framePtr->highlightWidth,
	    Tk_WindowId(tkwin));
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FrameEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher on
 *	structure changes to a frame.  For frames with 3D
 *	borders, this procedure is also invoked for exposures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *---------------------------------------------------------------------------
 */

static void
FrameEventProc(
    ClientData clientData,	/* Information about window. */
    register XEvent *eventPtr)	/* Information about event. */
{
    register Frame *framePtr = (Frame *) clientData;

    if (((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0))
	|| (eventPtr->type == ConfigureNotify)) {
	goto redraw;
    } else if (eventPtr->type == DestroyNotify) {
	if (framePtr->menuName != NULL) {
	    TkSetWindowMenuBar(framePtr->interp, framePtr->tkwin,
		framePtr->menuName, NULL);
	    Blt_Free(framePtr->menuName);
	    framePtr->menuName = NULL;
	}
	if (framePtr->tkwin != NULL) {

	    /*
	     * If this window is a container, then this event could be
	     * coming from the embedded application, in which case
	     * Tk_DestroyWindow hasn't been called yet.  When Tk_DestroyWindow
	     * is called later, then another destroy event will be generated.
	     * We need to be sure we ignore the second event, since the frame
	     * could be gone by then.  To do so, delete the event handler
	     * explicitly (normally it's done implicitly by Tk_DestroyWindow).
	     */
	    Tk_DeleteEventHandler(framePtr->tkwin,
		ExposureMask | StructureNotifyMask | FocusChangeMask,
		FrameEventProc, (ClientData)framePtr);
	    framePtr->tkwin = NULL;
	    Tcl_DeleteCommandFromToken(framePtr->interp, framePtr->widgetCmd);
	}
	if (framePtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayFrame, (ClientData)framePtr);
	}
	Tcl_CancelIdleCall(MapFrame, (ClientData)framePtr);
	Tcl_EventuallyFree((ClientData)framePtr, (Tcl_FreeProc *)DestroyFrame);
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    framePtr->flags |= GOT_FOCUS;
	    if (framePtr->highlightWidth > 0) {
		goto redraw;
	    }
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    framePtr->flags &= ~GOT_FOCUS;
	    if (framePtr->highlightWidth > 0) {
		goto redraw;
	    }
	}
    } else if (eventPtr->type == ActivateNotify) {
	TkpSetMainMenubar(framePtr->interp, framePtr->tkwin,
	    framePtr->menuName);
    }
    return;

  redraw:
    if ((framePtr->tkwin != NULL) && !(framePtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayFrame, (ClientData)framePtr);
	framePtr->flags |= REDRAW_PENDING;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FrameCmdDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If
 *	the widget isn't already in the process of being destroyed,
 *	this command destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *---------------------------------------------------------------------------
 */
static void
FrameCmdDeletedProc(ClientData clientData) /* Pointer to widget record
					      for widget. */
{
    Frame *framePtr = (Frame *) clientData;
    Tk_Window tkwin = framePtr->tkwin;

    if (framePtr->menuName != NULL) {
	TkSetWindowMenuBar(framePtr->interp, framePtr->tkwin,
	    framePtr->menuName, NULL);
	Blt_Free(framePtr->menuName);
	framePtr->menuName = NULL;
    }

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
	framePtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MapFrame --
 *
 *	This procedure is invoked as a when-idle handler to map a
 *	newly-created top-level frame.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The frame given by the clientData argument is mapped.
 *
 *---------------------------------------------------------------------------
 */
static void
MapFrame(ClientData clientData)	/* Pointer to frame structure. */
{
    Frame *framePtr = (Frame *) clientData;

    /*
     * Wait for all other background events to be processed before
     * mapping window.  This ensures that the window's correct geometry
     * will have been determined before it is first mapped, so that the
     * window manager doesn't get a false idea of its desired geometry.
     */

    Tcl_Preserve((ClientData)framePtr);
    for(;;) {
	if (Tcl_DoOneEvent(TCL_IDLE_EVENTS) == 0) {
	    break;
	}
	/*
	 * After each event, make sure that the window still exists
	 * and quit if the window has been destroyed.
	 */

	if (framePtr->tkwin == NULL) {
	    Tcl_Release((ClientData)framePtr);
	    return;
	}
    }
    Tk_MapWindow(framePtr->tkwin);
    Tcl_Release((ClientData)framePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkInstallFrameMenu --
 *
 *	This function is needed when a Windows HWND is created
 *	and a menubar has been set to the window with a system
 *	menu. It notifies the menu package so that the system
 *	menu can be rebuilt.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The system menu (if any) is created for the menubar
 *	associated with this frame.
 *
 *---------------------------------------------------------------------------
 */
#ifdef notdef	
void
TkInstallFrameMenu(Tk_Window tkwin) /* The window that was just created. */
{
#define Tk_InstanceData(tkwin)	(((Tk_FakeWin *)(tkwin))->dummy18)
#define Tk_MainPtr(tkwin)	(((Tk_FakeWin *)(tkwin))->dummy5)
    if (Tk_MainPtr(tkwin) != NULL) {
	Frame *framePtr;

	framePtr = (Frame *) Tk_InstanceData(tkwin);
	TkpMenuNotifyToplevelCreate(framePtr->interp, framePtr->menuName);
    }
}

#endif

int
Blt_FrameCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpecs[2] = {
	{"frame", FrameCmd,},
	{"toplevel", ToplevelCmd,},
    };
    return Blt_InitCmds(interp, "::blt::tk", cmdSpecs, 2);
}

#endif /* NO_TKFRAME */

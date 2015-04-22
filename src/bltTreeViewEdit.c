/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTreeViewEdit.c --
 *
 * This module implements an hierarchy widget for the BLT toolkit.
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

/*
  Remember row,column where string was acquired.
  postcombobox x y 
	icon?, text, row, column position, fg, bg button?
	based upon style.
  grab set
  SetIcon
  SetText
  SetBg
  SetFg
  SetFont
  SetButton  
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_TREEVIEW

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltTreeView.h"
#include "bltOp.h"

#define PIXMAPX(t, wx) ((wx) - (t)->xOffset)
#define PIXMAPY(t, wy) ((wy) - (t)->yOffset)

#define SCREENX(t, wx) ((wx) - (t)->xOffset + (t)->borderWidth)
#define SCREENY(t, wy) ((wy) - (t)->yOffset + (t)->borderWidth)

#define WORLDX(t, sx)  ((sx) - (t)->borderWidth + (t)->xOffset)
#define WORLDY(t, sy)  ((sy) - (t)->borderWidth + (t)->yOffset)

#define VPORTWIDTH(t)  \
    (Tk_Width((t)->tkwin) - 2 * (t)->borderWidth - (t)->yScrollbarWidth)
#define VPORTHEIGHT(t) \
    (Tk_Height((t)->tkwin) - 2 * (t)->borderWidth - (t)->xScrollbarHeight)

#define TEXTBOX_FOCUS	(1<<0)
#define MAXSCROLLBARTHICKNESS   100

#define REDRAW_PENDING          (1<<0)  /* Indicates that the widget will
					 * be redisplayed at the next idle
					 * point. */
#define LAYOUT_PENDING          (1<<1)  /* Indicates that the widget's
					 * layout is scheduled to be
					 * recomputed at the next
					 * redraw. */
#define UPDATE_PENDING          (1<<2)  /* Indicates that a component
					 * (window or scrollbar) has
					 * changed and that and update is
					 * pending.  */
#define FOCUS                   (1<<3)  /* Indicates that the combomenu
					 * currently has focus. */
#define DROPDOWN                (1<<4)  /* Indicates the combomenu is a
					 * drop down menu as opposed to a
					 * popup.  */
#define POSTED                  (1<<5)  /* Indicates the combomenu is
					 * currently posted. */

#define SCROLLX                 (1<<6)
#define SCROLLY                 (1<<7)
#define SCROLL_PENDING          (SCROLLX|SCROLLY)

#define INSTALL_XSCROLLBAR      (1<<8)  /* Indicates that the x scrollbar
					 * is scheduled to be installed at
					 * the next idle point. */
#define INSTALL_YSCROLLBAR      (1<<9)  /* Indicates that the y scrollbar
					 * is scheduled to be installed at
					 * the next idle point. */
#define RESTRICT_MIN            (1<<10) /* Indicates to constrain the width
					 * of the menu to the minimum size
					 * of the parent widget that posted
					 * the menu. */
#define RESTRICT_MAX            (1<<11) /* Indicates to constrain the width
					 * of the menu of the maximum size
					 * of the parent widget that posted
					 * the menu. */
#define RESTRICT_NONE           (0)
#define INITIALIZED             (1<<22)


#define DEF_BACKGROUND		RGB_WHITE
#define DEF_BORDERWIDTH	STD_BORDERWIDTH
#ifdef WIN32
#define DEF_BUTTON_BACKGROUND  RGB_GREY85
#else
#define DEF_BUTTON_BACKGROUND           RGB_GREY90
#endif
#define DEF_BUTTON_BORDERWIDTH          "2"
#define DEF_BUTTON_RELIEF               "raised"
#define DEF_CURSOR                      (char *)NULL
#define DEF_EXPORT_SELECTION            "no"
#define DEF_NORMAL_BACKGROUND           STD_NORMAL_BACKGROUND
#define DEF_NORMAL_FG_MONO              STD_ACTIVE_FG_MONO
#define DEF_POSTCOMMAND                 ((char *)NULL)
#define DEF_RELIEF                      "solid"
#define DEF_SCROLLBAR                   ((char *)NULL)
#define DEF_SCROLL_CMD                  ((char *)NULL)
#define DEF_SCROLL_INCR                 "2"
#define DEF_SELECT_BACKGROUND           RGB_LIGHTBLUE0
#define DEF_SELECT_BG_MONO              STD_SELECT_BG_MONO
#define DEF_SELECT_BORDERWIDTH          "1"
#define DEF_SELECT_FG_MONO              STD_SELECT_FG_MONO
#define DEF_SELECT_FOREGROUND           STD_SELECT_FOREGROUND
#define DEF_SELECT_RELIEF               "flat"
#define DEF_UNPOSTCOMMAND               ((char *)NULL)
#define DEF_WIDTH                       "0"

typedef struct {
    unsigned int flags;                 /* Various flags: see below. */
    int x1, y1, x2, y2;                 /* Coordinates of area representing
					 * the parent that posted this
					 * editor.  */
    Tk_Window tkwin;                    /* Parent window that posted this
					 * editor. */
    int editorWidth, editorHeight;
    int lastEditorWidth;
    int align;
    Tcl_Obj *cmdObjPtr;                 /* Command to be executed after
                                         * completing edits. */
    Tcl_Obj *textObjPtr;                /* Imported text string. */
} PostInfo;

/*
 * TextEditor --
 *
 *	This structure is shared by entries when their labels are edited
 *	via the keyboard.  It maintains the location of the insertion
 *	cursor and the text selection for the editted entry.  The structure
 *	is shared since we need only one.  The "focus" entry should be the
 *	only entry receiving KeyPress/KeyRelease events at any time.
 *	Information from the previously editted entry is overwritten.
 *
 *	Note that all the indices internally are in terms of bytes, not
 *	characters.  This is because UTF-8 strings may encode a single
 *	character into a multi-byte sequence.  To find the respective
 *	character position
 *
 *		n = Tcl_NumUtfChars(string, index);
 *
 *	where n is the resulting character number.
 */
typedef struct {

    /*
     * This is a SNAFU in the Tk API.  It assumes that only an official Tk
     * "toplevel" widget will ever become a toplevel window (i.e. a window
     * whose parent is the root window).  Because under Win32, Tk tries to
     * use the widget record associated with the TopLevel as a Tk frame
     * widget, to read its menu name.  What this means is that any widget
     * that's going to be a toplevel, must also look like a
     * frame. Therefore we've copied the frame widget structure fields into
     * the token.
     */

    Tk_Window tkwin;                    /* Window that embodies the frame.
					 * NULL means that the window has
					 * been destroyed but the data
					 * structures haven't yet been
					 * cleaned up. */
    Display *display;                   /* Display containing widget.
					 * Used, among other things, so
					 * that resources can be freed even
					 * after tkwin has gone away. */
    Tcl_Interp *interp;                 /* Interpreter associated with
					 * widget.  Used to delete widget
					 * command. */
    Tcl_Command widgetCmd;              /* Token for frame's widget
					 * command. */
    char *className;                    /* Class name for widget (from
					 * configuration option).
					 * Malloc-ed. */
    int mask;                           /* Either FRAME or TOPLEVEL; used
					 * to select which configuration
					 * options are valid for widget. */
    char *screenName;                   /* Screen on which widget is
					 * created.  Non-null only for
					 * top-levels.  Malloc-ed, may be
					 * NULL. */
    char *visualName;                   /* Textual description of visual
					 * for window, from -visual option.
					 * Malloc-ed, may be NULL. */
    char *colormapName;                 /* Textual description of colormap
					 * for window, from -colormap
					 * option.  Malloc-ed, may be
					 * NULL. */
    char *menuName;                     /* Textual description of menu to
					 * use for menubar. Malloc-ed, may
					 * be NULL. */
    Colormap colormap;                  /* If not None, identifies a
					 * colormap allocated for this
					 * window, which must be freed when
					 * the window is deleted. */
    Tk_3DBorder border;                 /* Structure used to draw 3-D
					 * border and background.  NULL
					 * means no background or
					 * border. */
    int borderWidth;                    /* Width of 3-D border (if any). */
    int relief;                         /* 3-d effect: TK_RELIEF_RAISED etc. */
    int highlightWidth;                 /* Width in pixels of highlight to
					 * draw around widget when it has
					 * the focus.  0 means don't draw a
					 * highlight. */
    int width;                          /* Width to request for window.  <=
					 * 0 means don't request any
					 * size. */
    int height;                         /* Height to request for window.
					 * <= 0 means don't request any
					 * size. */
    Tk_Cursor cursor;                   /* Current cursor for window, or
					 * None. */
    char *takeFocus;                    /* Value of -takefocus option; not
					 * used in the C code, but used by
					 * keyboard traversal scripts.
					 * Malloc'ed, but may be NULL. */
    int isContainer;                    /* 1 means this window is a
					 * container, 0 means that it
					 * isn't. */
    char *useThis;                      /* If the window is embedded, this
					 * points to the name of the window
					 * in which it is embedded
					 * (malloc'ed).  For non-embedded
					 * windows this is NULL. */
    int flags;                          /* Various flags; see below for
					 * definitions. */

    /* TextEditor-specific fields */
    TreeView *viewPtr;
    int x, y;                           /* Position of window. */

    int active;                         /* Indicates that the frame is
					 * active. */
    int exportSelection;

    int insertPos;                      /* Position of the cursor within
					 * the array of bytes of the
					 * entry's label. */

    int cursorX, cursorY;               /* Position of the insertion cursor
					 * in the textbox window. */
    short int cursorWidth;              /* Size of the insertion cursor
					 *    symbol. */
    short int cursorHeight;

    int selAnchor;                      /* Fixed end of selection. Used to
					 * extend the selection while
					 * maintaining the other end of the
					 * selection. */
    int selFirst;                       /* Position of the first character
					 * in the selection. */
    int selLast;                        /* Position of the last character
					 * in the selection. */

    int cursorOn;                       /* Indicates if the cursor is
					 * displayed. */
    int onTime, offTime;                /* Time in milliseconds to wait
					 * before changing the cursor from
					 * off-to-on and on-to-off. Setting
					 * offTime to 0 makes the cursor
					 * steady. */
    Tcl_TimerToken timerToken;          /* Handle for a timer event called
					 * periodically to blink the
					 * cursor. */
    /* Data-specific fields. */
    Entry *entryPtr;                    /* Selected entry */
    Column *colPtr;                     /* Column of entry to be edited */
    CellStyle *stylePtr;
    Icon icon;
    int gap;
    char *string;
    TextLayout *layoutPtr;
    Blt_Font font;
    GC gc;

    Tk_3DBorder selBorder;
    int selRelief;
    int selBW;
    XColor *selFgColor;                 /* Text color of a selected
					 * entry. */
    int buttonBW;
    Tk_3DBorder buttonBorder;
    int buttonRelief;
    PostInfo post;
    
    int xScrollUnits, yScrollUnits;

    /* Names of scrollbars to embed into the widget window. */
    Tcl_Obj *xScrollbarObjPtr, *yScrollbarObjPtr;

    /* Commands to control horizontal and vertical scrollbars. */
    Tcl_Obj *xScrollCmdObjPtr, *yScrollCmdObjPtr;

    Tk_Window xScrollbar;               /* Horizontal scrollbar to be used
					 * if necessary. If NULL, no
					 * x-scrollbar is used. */
    Tk_Window yScrollbar;               /* Vertical scrollbar to be used if
					 * necessary. If NULL, no
					 * y-scrollbar is used. */
    short int yScrollbarWidth, xScrollbarHeight;
} TextEditor;

static Blt_SwitchParseProc PostPopupSwitchProc;
static Blt_SwitchCustom postPopupSwitch = {
    PostPopupSwitchProc, NULL, NULL, 0,
};

static Blt_SwitchParseProc PostCascadeSwitchProc;
static Blt_SwitchCustom postCascadeSwitch = {
    PostCascadeSwitchProc, NULL, NULL, 0,
};
static Blt_SwitchParseProc PostBoxSwitchProc;
static Blt_SwitchCustom postBoxSwitch = {
    PostBoxSwitchProc, NULL, NULL, 0, 
};
static Blt_SwitchParseProc PostAlignSwitchProc;
static Blt_SwitchCustom postAlignSwitch = {
    PostAlignSwitchProc, NULL, NULL, 0, 
};
static Blt_SwitchParseProc PostWindowSwitchProc;
static Blt_SwitchCustom postWindowSwitch = {
    PostWindowSwitchProc, NULL, NULL, 0, 
};

typedef struct {
    unsigned int flags;                 /* Various flags: see below. */
    int x1, y1, x2, y2;                 /* Coordinates of area representing
					 * the parent that posted this
					 * menu.  */
    Tk_Window tkwin;                    /* Parent window that posted this
					 * menu. */
} PostSwitches;

#define ALIGN_LEFT      (0)             /* Menu is aligned to the center of
					 * the parent. */
#define ALIGN_CENTER    (1)             /* Menu is aligned on the left side
					 * of the parent.  */
#define ALIGN_RIGHT     (2)             /* Menu is aligned on the right
					 * side of the parent. */

#define POST_PARENT     (0)             /* Use parent geometry for location
					 * of button. */
#define POST_POPUP      (1)             /* x,y location of the menu in root
					 * coordinates. This menu is a
					 * popup.*/
#define POST_CASCADE    (2)             /* x,y location of the menu in root
					 * coordinates. This menu is a
					 * cascade.*/
#define POST_WINDOW     (3)             /* Window representing the
					 * parent. */
#define POST_REGION     (4)             /* Bounding box representing the
					 * parent area. The x1, y2, x2, y2
					 * coordinates are in root
					 * coordinates. */

static Blt_SwitchSpec postSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-align", "left|right|center", (char *)NULL,
	Blt_Offset(TextEditor, post.align), 0, 0, &postAlignSwitch},
    {BLT_SWITCH_CUSTOM, "-box", "x1 y1 x2 y2", (char *)NULL,
	0, 0, 0, &postBoxSwitch},
    {BLT_SWITCH_CUSTOM, "-cascade", "x y", (char *)NULL,
	0, 0, 0, &postCascadeSwitch},
    {BLT_SWITCH_CUSTOM, "-popup", "x y", (char *)NULL,
	0, 0, 0, &postPopupSwitch},
    {BLT_SWITCH_OBJ, "-text", "string", (char *)NULL,
	Blt_Offset(TextEditor, post.text), 0},
    {BLT_SWITCH_OBJ, "-command", "cmdPrefix", (char *)NULL,
	Blt_Offset(TextEditor, post.cmdObjPtr), 0},
    {BLT_SWITCH_CUSTOM, "-window", "path", (char *)NULL,
	Blt_Offset(TextEditor, post.tkwin), 0, 0, &postWindowSwitch},
    {BLT_SWITCH_END}
};

/* TextEditor Procedures */
static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BORDER, "-background", "background", "Background",
	DEF_BACKGROUND, Blt_Offset(TextEditor, border), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_CURSOR, Blt_Offset(TextEditor, cursor), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_BORDERWIDTH, Blt_Offset(TextEditor, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BORDER, "-buttonbackground", "buttonBackground", 
	"ButtonBackground", DEF_BUTTON_BACKGROUND,
	Blt_Offset(TextEditor, buttonBorder), 0},
    {BLT_CONFIG_RELIEF, "-buttonrelief", "buttonRelief", "ButtonRelief",
	DEF_BUTTON_RELIEF, Blt_Offset(TextEditor, buttonRelief),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-buttonborderwidth", "buttonBorderWidth", 
	"ButtonBorderWidth", DEF_BUTTON_BORDERWIDTH, 
	Blt_Offset(TextEditor, buttonBW),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BOOLEAN, "-exportselection", "exportSelection",
	"ExportSelection", DEF_EXPORT_SELECTION, 
	Blt_Offset(TextEditor, exportSelection), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_RELIEF, 
	Blt_Offset(TextEditor, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BORDER, "-selectbackground", "selectBackground", "Background",
	DEF_SELECT_BG_MONO, Blt_Offset(TextEditor, selBorder),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_BORDER, "-selectbackground", "selectBackground", "Background",
	DEF_SELECT_BACKGROUND, Blt_Offset(TextEditor, selBorder),
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_PIXELS_NNEG, "-selectborderwidth", "selectBorderWidth", 
	"BorderWidth", DEF_SELECT_BORDERWIDTH, 
	Blt_Offset(TextEditor, selBW), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Foreground",

	DEF_SELECT_FG_MONO, Blt_Offset(TextEditor, selFgColor),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Foreground",
	DEF_SELECT_FOREGROUND, Blt_Offset(TextEditor, selFgColor),
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_RELIEF, "-selectrelief", "selectRelief", "Relief",
	DEF_SELECT_RELIEF, Blt_Offset(TextEditor, selRelief),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-xscrollbar", "xScrollbar", "Scrollbar", 
	DEF_SCROLLBAR, Blt_Offset(TextEditor, xScrollbarObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	DEF_SCROLL_CMD, Blt_Offset(TextEditor, xScrollCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_POS, "-xscrollincrement", "xScrollIncrement",
	"ScrollIncrement", DEF_SCROLL_INCR, 
	 Blt_Offset(TextEditor, xScrollUnits), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-yscrollbar", "yScrollbar", "Scrollbar", 
	DEF_SCROLLBAR, Blt_Offset(TextEditor, yScrollbarObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
	DEF_SCROLL_CMD, Blt_Offset(TextEditor, yScrollCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_POS, "-yscrollincrement", "yScrollIncrement",
	"ScrollIncrement", DEF_SCROLL_INCR, 
	 Blt_Offset(TextEditor, yScrollUnits),BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-takefocus", "takeFocus", "TakeFocus",
	DEF_TAKE_FOCUS, Blt_Offset(TextEditor, takeFocusObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-unpostcommand", "unpostCommand", "UnpostCommand", 
	DEF_UNPOSTCOMMAND, Blt_Offset(TextEditor, unpostCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0}
};


static Tcl_IdleProc DisplayProc;
static Tcl_FreeProc DestroyTextEditor;
static Tcl_TimerProc BlinkCursorProc;
static Tcl_ObjCmdProc TextEditorCmd;

static Tk_LostSelProc LostSelectionProc;
static Tk_SelectionProc SelectionProc;
static Tk_EventProc EventProc;

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
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
EventuallyRedraw(TextEditor *textPtr)
{
    if ((textPtr->tkwin != NULL) && 
	((textPtr->flags & REDRAW_PENDING) == 0)) {
	textPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayProc, textPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PostWindowSwitchProc --
 *
 *      Converts a window name into Tk window.
 *
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.
 *      Otherwise, TCL_ERROR is returned and an error message is left
 *      in interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PostWindowSwitchProc(ClientData clientData, Tcl_Interp *interp,
		    const char *switchName, Tcl_Obj *objPtr, char *record,
		    int offset, int flags)
{
    TextEditor *textPtr = (TextEditor *)record;
    Tk_Window tkwin;
    const char *string;

    tkwin = NULL;
    string = Tcl_GetString(objPtr);
    if (string[0] == '\0') {
	tkwin = NULL;
    } else {
	tkwin = Tk_NameToWindow(interp, string, textPtr->tkwin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
    }
    textPtr->post.flags = POST_WINDOW;
    textPtr->post.tkwin = tkwin;
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * PostAlignSwitchProc --
 *
 *      Converts string into x and y coordinates.
 *
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.
 *      Otherwise, TCL_ERROR is returned and an error message is left
 *      in interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PostAlignSwitchProc(ClientData clientData, Tcl_Interp *interp,
		    const char *switchName, Tcl_Obj *objPtr, char *record,
		    int offset, int flags)
{
    TextEditor *textPtr = (TextEditor *)record;
    int align;
    
    if (GetAlignFromObj(interp, objPtr, &align) != TCL_OK) {
	return TCL_ERROR;
    }
    textPtr->post.align = align;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PostPopupSwitchProc --
 *
 *      Converts string into x and y coordinates.  Indicates that the menu
 *      is a popup and will be popped at the given x, y coordinate.
 *
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.
 *      Otherwise, TCL_ERROR is returned and an error message is left
 *      in interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PostPopupSwitchProc(ClientData clientData, Tcl_Interp *interp,
		    const char *switchName, Tcl_Obj *objPtr, char *record,
		    int offset, int flags)
{
    TextEditor *textPtr = (TextEditor *)record;
    int x, y;
    
    if (GetCoordsFromObj(interp, objPtr, &x, &y) != TCL_OK) {
	return TCL_ERROR;
    }
    textPtr->post.x1 = textPtr->post.x2 = x;
    textPtr->post.y1 = textPtr->post.y2 = y;
    textPtr->post.flags = POST_POPUP;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PostBoxSwitchProc --
 *
 *      Converts string into x1, y1, x2, and y2 coordinates.
 *
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.
 *      Otherwise, TCL_ERROR is returned and an error message is left
 *      in interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PostBoxSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Not used. */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    TextEditor *textPtr = (TextEditor *)record;
    Box2d box;
    
    if (GetBoxFromObj(interp, objPtr, &box) != TCL_OK) {
	return TCL_ERROR;
    }
    textPtr->post.x1 = box.x1;
    textPtr->post.y1 = box.y1;
    textPtr->post.x2 = box.x2;
    textPtr->post.y2 = box.y2;
    textPtr->post.flags = POST_REGION;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetEntryIcon --
 *
 * 	Selects the correct image for the entry's icon depending upon the
 * 	current state of the entry: active/inactive normal/selected.
 *
 *		active - normal
 *		active - selected
 *		inactive - normal
 *		inactive - selected
 *
 * Results:
 *	Returns the image for the icon.
 *
 *---------------------------------------------------------------------------
 */
static Icon
GetEntryIcon(TreeView *viewPtr, Entry *entryPtr)
{
    Icon *icons;
    Icon icon;

    icons = CHOOSE(viewPtr->icons, entryPtr->icons);
    icon = NULL;
    if (icons != NULL) {		/* Selected or normal icon? */
	icon = icons[0];
	if (((entryPtr->flags & ENTRY_CLOSED) == 0) && (icons[1] != NULL)) {
	    icon = icons[1];
	}
    }
    return icon;
}

/*
 *---------------------------------------------------------------------------
 *
 * BlinkCursorProc --
 *
 *	This procedure is called as a timer handler to blink the insertion
 *	cursor off and on.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor gets turned on or off, redisplay gets invoked, and this
 *	procedure reschedules itself.
 *
 *---------------------------------------------------------------------------
 */
static void
BlinkCursorProc(ClientData clientData)
{
    TextEditor *textPtr = clientData;
    int interval;

    if (!(textPtr->flags & TEXTBOX_FOCUS) || (textPtr->offTime == 0)) {
	return;
    }
    if (textPtr->active) {
	textPtr->cursorOn ^= 1;
	interval = (textPtr->cursorOn) ? textPtr->onTime : textPtr->offTime;
	textPtr->timerToken = 
	    Tcl_CreateTimerHandler(interval, BlinkCursorProc, textPtr);
	EventuallyRedraw(textPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EventProc --
 *
 * 	This procedure is invoked by the Tk dispatcher for various events
 * 	on treeview widgets.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get cleaned up.
 *	When it gets exposed, it is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
EventProc(ClientData clientData, XEvent *eventPtr)
{
    TextEditor *textPtr = clientData;

    if (eventPtr->type == Expose) {
	if (eventPtr->xexpose.count == 0) {
	    EventuallyRedraw(textPtr);
	}
    } else if (eventPtr->type == ConfigureNotify) {
	EventuallyRedraw(textPtr);
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
	if (eventPtr->xfocus.detail == NotifyInferior) {
	    return;
	}
	if (eventPtr->type == FocusIn) {
	    textPtr->flags |= TEXTBOX_FOCUS;
	} else {
	    textPtr->flags &= ~TEXTBOX_FOCUS;
	}
	Tcl_DeleteTimerHandler(textPtr->timerToken);
	if ((textPtr->active) && (textPtr->flags & TEXTBOX_FOCUS)) {
	    textPtr->cursorOn = TRUE;
	    if (textPtr->offTime != 0) {
		textPtr->timerToken = Tcl_CreateTimerHandler(textPtr->onTime, 
		   BlinkCursorProc, textPtr);
	    }
	} else {
	    textPtr->cursorOn = FALSE;
	    textPtr->timerToken = (Tcl_TimerToken) NULL;
	}
	EventuallyRedraw(textPtr);
    } else if (eventPtr->type == DestroyNotify) {
	if (textPtr->tkwin != NULL) {
	    textPtr->tkwin = NULL;
	}
	if (textPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayProc, textPtr);
	}
	if (textPtr->timerToken != NULL) {
	    Tcl_DeleteTimerHandler(textPtr->timerToken);
	}
	textPtr->viewPtr->comboWin = NULL;
	Tcl_EventuallyFree(textPtr, DestroyTextEditor);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * LostSelectionProc --
 *
 *	This procedure is called back by Tk when the selection is grabbed
 *	away from a Text widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The existing selection is unhighlighted, and the window is marked
 *	as not containing a selection.
 *
 *---------------------------------------------------------------------------
 */
static void
LostSelectionProc(ClientData clientData)
{
    TextEditor *textPtr = clientData;

    if ((textPtr->selFirst >= 0) && (textPtr->exportSelection)) {
	textPtr->selFirst = textPtr->selLast = -1;
	EventuallyRedraw(textPtr);
    }
}

static void
UnmanageScrollbar(TextEditor *textPtr, Tk_Window scrollbar)
{
    if (scrollbar != NULL) {
	Tk_DeleteEventHandler(scrollbar, StructureNotifyMask,
	      ScrollbarEventProc, textPtr);
	Tk_ManageGeometry(scrollbar, (Tk_GeomMgr *)NULL, textPtr);
	if (Tk_IsMapped(scrollbar)) {
	    Tk_UnmapWindow(scrollbar);
	}
    }
}

static void
ManageScrollbar(TextEditor *textPtr, Tk_Window scrollbar)
{
    if (scrollbar != NULL) {
	Tk_CreateEventHandler(scrollbar, StructureNotifyMask, 
		ScrollbarEventProc, textPtr);
	Tk_ManageGeometry(scrollbar, &comboMgrInfo, textPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * InstallScrollbar --
 *
 *      Convert the string representation of a color into a XColor pointer.
 *
 * Results:
 *      The return value is a standard TCL result.  The color pointer is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
InstallScrollbar(
    Tcl_Interp *interp,                 /* Interpreter to send results back
					 * to */
    TextEditor *textPtr,
    Tcl_Obj *objPtr,                    /* String representing scrollbar
					 * window. */
    Tk_Window *tkwinPtr)
{
    Tk_Window tkwin;

    if (objPtr == NULL) {
	*tkwinPtr = NULL;
	return;
    }
    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objPtr), textPtr->tkwin);
    if (tkwin == NULL) {
	Tcl_BackgroundError(interp);
	return;
    }
    if (Tk_Parent(tkwin) != textPtr->tkwin) {
	Tcl_AppendResult(interp, "scrollbar \"", Tk_PathName(tkwin), 
			 "\" must be a child of combomenu.", (char *)NULL);
	Tcl_BackgroundError(interp);
	return;
    }
    ManageScrollbar(textPtr, tkwin);
    *tkwinPtr = tkwin;
    return;
}

static void
InstallXScrollbar(ClientData clientData)
{
    TextEditor *textPtr = clientData;

    textPtr->flags &= ~INSTALL_XSCROLLBAR;
    InstallScrollbar(textPtr->interp, textPtr, textPtr->xScrollbarObjPtr,
		     &textPtr->xScrollbar);
}

static void
InstallYScrollbar(ClientData clientData)
{
    TextEditor *textPtr = clientData;

    textPtr->flags &= ~INSTALL_YSCROLLBAR;
    InstallScrollbar(textPtr->interp, textPtr, textPtr->yScrollbarObjPtr,
		     &textPtr->yScrollbar);
}


static int
PointerToIndex(TextEditor *textPtr, int x, int y)
{
    Blt_Font font;
    Blt_FontMetrics fontMetrics;
    TextFragment *fragPtr;
    TextLayout *layoutPtr;
    int i;
    int numBytes;
    int total;
    TreeView *viewPtr;

    viewPtr = textPtr->viewPtr;
    if ((textPtr->string == NULL) || (textPtr->string[0] == '\0')) {
	return 0;
    }
    x -= textPtr->selBW;
    y -= textPtr->selBW;

    layoutPtr = textPtr->layoutPtr;

    /* Bound the y-coordinate within the window. */
    if (y < 0) {
	y = 0;
    } else if (y >= layoutPtr->height) {
	y = layoutPtr->height - 1;
    }
    /* 
     * Compute the line that contains the y-coordinate. 
     *
     * FIXME: This assumes that segments are distributed 
     *	     line-by-line.  This may change in the future.
     */
    font = CHOOSE(viewPtr->font, textPtr->font);
    Blt_Font_GetMetrics(font, &fontMetrics);
    fragPtr = layoutPtr->fragments;
    total = 0;
    for (i = (y / fontMetrics.linespace); i > 0; i--) {
	total += fragPtr->count;
	fragPtr++;
    }
    if (x < 0) {
	numBytes = 0;
    } else if (x >= layoutPtr->width) {
	numBytes = fragPtr->count;
    } else {
	int newX;

	/* Find the character underneath the pointer. */
	numBytes = Blt_Font_Measure(font, fragPtr->text, fragPtr->count, x, 0, 
		&newX);
	if ((newX < x) && (numBytes < fragPtr->count)) {
	    double fract;
	    int length, charSize;
	    const char *next;

	    next = fragPtr->text + numBytes;
#if HAVE_UTF
	    {
		Tcl_UniChar dummy;

		length = Tcl_UtfToUniChar(next, &dummy);
	    }
#else
	    length = 1;
#endif
	    charSize = Blt_TextWidth(font, next, length);
	    fract = ((double)(x - newX) / (double)charSize);
	    if (ROUND(fract)) {
		numBytes += length;
	    }
	}
    }
    return numBytes + total;
}

static int
IndexToPointer(TextEditor *textPtr)
{
    int x, y;
    int maxLines;
    TextLayout *layoutPtr;
    TreeView *viewPtr;
    Blt_FontMetrics fontMetrics;
    int numBytes;
    int sum;
    TextFragment *fragPtr;
    Blt_Font font;
    int i;

    viewPtr = textPtr->viewPtr;
    layoutPtr = textPtr->layoutPtr;
    font = CHOOSE(viewPtr->font, textPtr->font);
    Blt_Font_GetMetrics(font, &fontMetrics);
    maxLines = (layoutPtr->height / fontMetrics.linespace) - 1;

    sum = 0;
    x = y = textPtr->borderWidth;
    if (textPtr->icon != NULL) {
	x += IconWidth(textPtr->icon) + 2 * textPtr->gap;
    }
    fragPtr = layoutPtr->fragments;
    for (i = 0; i <= maxLines; i++) {
	/* Total the number of bytes on each line.  Include newlines. */
	numBytes = fragPtr->count + 1;
	if ((sum + numBytes) > textPtr->insertPos) {
	    x += Blt_TextWidth(font, fragPtr->text, textPtr->insertPos - sum);
	    break;
	}
	y += fontMetrics.linespace;
	sum += numBytes;
	fragPtr++;
    }
    textPtr->cursorX = x;
    textPtr->cursorY = y;
    textPtr->cursorHeight = fontMetrics.linespace;
    textPtr->cursorWidth = 3;
    return TCL_OK;
}

static void
UpdateLayout(TextEditor *textPtr)
{
    TreeView *viewPtr;
    TextStyle ts;
    int width, height;
    TextLayout *layoutPtr;
    int gap, offset;
    int iw, ih;
    Blt_Font font;

    viewPtr = textPtr->viewPtr;
    offset = gap = iw = ih = 0;
    if (textPtr->icon != NULL) {
	iw = IconWidth(textPtr->icon) + 4;
	ih = IconHeight(textPtr->icon);
	gap = textPtr->gap;
    }

    /* The layout is based upon the current font. */
    Blt_Ts_InitStyle(ts);
    font = CHOOSE(viewPtr->font, textPtr->font);
    Blt_Ts_SetFont(ts, font);
    layoutPtr = Blt_Ts_CreateLayout(textPtr->string, -1, &ts);
    if (textPtr->layoutPtr != NULL) {
	Blt_Free(textPtr->layoutPtr);
    }
    textPtr->layoutPtr = layoutPtr;

    width = iw + layoutPtr->width + gap * 2;
    height = MAX(ih, layoutPtr->height);
    if ((textPtr->colPtr == &viewPtr->treeColumn) && (!viewPtr->flatView)) {
	int level;
	
	level = DEPTH(viewPtr, textPtr->entryPtr->node);
	offset = -(ICONWIDTH(level) + 2);
    }

    if (width <= (textPtr->colPtr->width + offset)) {
	width = (textPtr->colPtr->width + offset);
    } 
    if (height < textPtr->entryPtr->height) {
	height = textPtr->entryPtr->height;
    }
    textPtr->width = width + (2 * textPtr->borderWidth);
    textPtr->height = height + (2 * textPtr->borderWidth);
    IndexToPointer(textPtr);
    Tk_MoveResizeWindow(textPtr->tkwin, textPtr->x, textPtr->y, 
	      textPtr->width, textPtr->height);
    Tk_MapWindow(textPtr->tkwin);
    XRaiseWindow(textPtr->display, Tk_WindowId(textPtr->tkwin));
}

static void
InsertText(TextEditor *textPtr, char *insertText, int insertPos, int numBytes)
{
    int oldSize, newSize;
    char *oldText, *newText;

    oldText = textPtr->string;
    oldSize = (int)strlen(oldText);
    newSize = oldSize + numBytes + 1;
    newText = Blt_AssertMalloc(sizeof(char) * newSize);
    if (insertPos == oldSize) {	/* Append */
	Blt_FormatString(newText, newSize, "%s%s", oldText, insertText);
    } else if (insertPos == 0) {/* Prepend */
	Blt_FormatString(newText, newSize, "%s%s", insertText, oldText);
    } else {			/* Insert into existing. */
	Blt_FormatString(newText, newSize, "%.*s%s%s", insertPos, oldText, 
			 insertText, oldText + insertPos);
    }

    /* 
     * All indices from the start of the insertion to the end of the string
     * need to be updated.  Simply move the indices down by the number of
     * characters added.
     */
    if (textPtr->selFirst >= insertPos) {
	textPtr->selFirst += numBytes;
    }
    if (textPtr->selLast > insertPos) {
	textPtr->selLast += numBytes;
    }
    if ((textPtr->selAnchor > insertPos) || (textPtr->selFirst >= insertPos)) {
	textPtr->selAnchor += numBytes;
    }
    if (textPtr->string != NULL) {
	Blt_Free(textPtr->string);
    }
    textPtr->string = newText;
    textPtr->insertPos = insertPos + numBytes;
    UpdateLayout(textPtr);
}

static int
DeleteText(TextEditor *textPtr, int firstPos, int lastPos)
{
    char *oldText, *newText;
    int oldSize, newSize;
    int numBytes;
    char *p;

    oldText = textPtr->string;
    if (firstPos > lastPos) {
	return TCL_OK;
    }
    lastPos++;                          /* Now is the position after the
					 * last character. */

    numBytes = lastPos - firstPos;
    oldSize = strlen(oldText) + 1;
    newSize = oldSize - numBytes + 1;
    newText = Blt_AssertMalloc(sizeof(char) * newSize);
    p = newText;
    if (firstPos > 0) {
	strncpy(p, oldText, firstPos);
	p += firstPos;
    }
    *p = '\0';
    if (lastPos < oldSize) {
	strcpy(p, oldText + lastPos);
    }
    Blt_Free(oldText);

    /*
     * Since deleting characters compacts the character array, we need to
     * update the various character indices according.  It depends where
     * the index occurs in relation to range of deleted characters:
     *
     *	 before		Ignore.
     *   within		Move the index back to the start of the deletion.
     *	 after		Subtract off the deleted number of characters,
     *			since the array has been compressed by that
     *			many characters.
     *
     */
    if (textPtr->selFirst >= firstPos) {
	if (textPtr->selFirst >= lastPos) {
	    textPtr->selFirst -= numBytes;
	} else {
	    textPtr->selFirst = firstPos;
	}
    }
    if (textPtr->selLast >= firstPos) {
	if (textPtr->selLast >= lastPos) {
	    textPtr->selLast -= numBytes;
	} else {
	    textPtr->selLast = firstPos;
	}
    }
    if (textPtr->selLast <= textPtr->selFirst) {
	textPtr->selFirst = textPtr->selLast = -1; /* Cut away the entire
						* selection. */ 
    }
    if (textPtr->selAnchor >= firstPos) {
	if (textPtr->selAnchor >= lastPos) {
	    textPtr->selAnchor -= numBytes;
	} else {
	    textPtr->selAnchor = firstPos;
	}
    }
    if (textPtr->insertPos >= firstPos) {
	if (textPtr->insertPos >= lastPos) {
	    textPtr->insertPos -= numBytes;
	} else {
	    textPtr->insertPos = firstPos;
	}
    }
    textPtr->string = newText;
    UpdateLayout(textPtr);
    EventuallyRedraw(textPtr);
    return TCL_OK;
}

static int
AcquireText(TreeView *viewPtr, TextEditor *textPtr, Entry *entryPtr, Column *colPtr)
{
    CellStyle *stylePtr;
    int x, y;
    const char *string;
    Icon icon;

    if (colPtr == &viewPtr->treeColumn) {
	int level;

	level = DEPTH(viewPtr, entryPtr->node);
	x = SCREENX(viewPtr, entryPtr->worldX);
	y = SCREENY(viewPtr, entryPtr->worldY);
#ifdef notdef
	x += ICONWIDTH(level) + ICONWIDTH(level + 1) + 4;
#endif
	if (!viewPtr->flatView) {
	    x += ICONWIDTH(level);
	}
	string = GETLABEL(entryPtr);
	stylePtr = colPtr->stylePtr;
	icon = GetEntryIcon(viewPtr, entryPtr);
    } else {
	Cell *cellPtr;

	x = SCREENX(viewPtr, colPtr->worldX);
	y = SCREENY(viewPtr, entryPtr->worldY);
	stylePtr = colPtr->stylePtr;
	cellPtr = Blt_TreeView_FindCell(entryPtr, colPtr);
	string = cellPtr->text;
	if (cellPtr->stylePtr != NULL) {
	    stylePtr = cellPtr->stylePtr;
	}
	icon = stylePtr->icon;
    }
    if (textPtr->layoutPtr != NULL) {
	Blt_Free(textPtr->layoutPtr);
	textPtr->layoutPtr = NULL;
    }
    if (textPtr->string != NULL) {
	Blt_Free(textPtr->string);
    }
    if (string == NULL) {
	string = "";
    }
    textPtr->icon = icon;
    textPtr->entryPtr = entryPtr;
    textPtr->colPtr = colPtr;
    textPtr->x = x - textPtr->borderWidth;
    textPtr->y = y - textPtr->borderWidth;
    
    textPtr->gap = stylePtr->gap;
    textPtr->string = Blt_AssertStrdup(string);
    textPtr->gc = stylePtr->normalGC;
    textPtr->font = stylePtr->font;
    textPtr->selFirst = textPtr->selLast = -1;
    UpdateLayout(textPtr);
    Tk_MapWindow(textPtr->tkwin);
    EventuallyRedraw(textPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * GetIndexFromObj --
 *
 *	Parse an index into an entry and return either its value or an
 *	error.
 *
 * Results:
 *	A standard TCL result.  If all went well, then *indexPtr is filled
 *	in with the character index (into entryPtr) corresponding to
 *	string.  The index value is guaranteed to lie between 0 and the
 *	number of characters in the string, inclusive.  If an error occurs
 *	then an error message is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
GetIndexFromObj(Tcl_Interp *interp, TextEditor *textPtr, Tcl_Obj *objPtr, 
		int *indexPtr)
{
    int textPos;
    char c;
    char *string;

    string = Tcl_GetString(objPtr);
    if ((textPtr->string == NULL) || (textPtr->string[0] == '\0')) {
	*indexPtr = 0;
	return TCL_OK;
    }
    c = string[0];
    if ((c == 'a') && (strcmp(string, "anchor") == 0)) {
	textPos = textPtr->selAnchor;
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
	textPos = strlen(textPtr->string);
    } else if ((c == 'i') && (strcmp(string, "insert") == 0)) {
	textPos = textPtr->insertPos;
    } else if ((c == 'n') && (strcmp(string, "next") == 0)) {
	textPos = textPtr->insertPos;
	if (textPos < (int)strlen(textPtr->string)) {
	    textPos++;
	}
    } else if ((c == 'l') && (strcmp(string, "last") == 0)) {
	textPos = textPtr->insertPos;
	if (textPos > 0) {
	    textPos--;
	}
    } else if ((c == 's') && (strcmp(string, "sel.first") == 0)) {
	if (textPtr->selFirst < 0) {
	    textPos = -1;
	} else {
	    textPos = textPtr->selFirst;
	}
    } else if ((c == 's') && (strcmp(string, "sel.last") == 0)) {
	if (textPtr->selLast < 0) {
	    textPos = -1;
	} else {
	    textPos = textPtr->selLast;
	}
    } else if (c == '@') {
	int x, y;

	if (Blt_GetXY(interp, textPtr->tkwin, string, &x, &y) != TCL_OK) {
	    return TCL_ERROR;
	}
	textPos = PointerToIndex(textPtr, x, y);
    } else if (isdigit((int)c)) {
	int number;
	int maxChars;

	if (Tcl_GetIntFromObj(interp, objPtr, &number) != TCL_OK) {
	    return TCL_ERROR;
	}
	/* Don't allow the index to point outside the label. */
	maxChars = Tcl_NumUtfChars(textPtr->string, -1);
	if (number < 0) {
	    textPos = 0;
	} else if (number > maxChars) {
	    textPos = strlen(textPtr->string);
	} else {
	    textPos = Tcl_UtfAtIndex(textPtr->string, number) - textPtr->string;
	}
    } else {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "bad label index \"", string, "\"",
			     (char *)NULL);
	}
	return TCL_ERROR;
    }
    *indexPtr = textPos;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectText --
 *
 *	Modify the selection by moving its un-anchored end.  This could
 *	make the selection either larger or smaller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectText(TextEditor *textPtr, int textPos)
{
    int selFirst, selLast;

    /*
     * Grab the selection if we don't own it already.
     */
    if ((textPtr->exportSelection) && (textPtr->selFirst == -1)) {
	Tk_OwnSelection(textPtr->tkwin, XA_PRIMARY, LostSelectionProc, textPtr);
    }
    /*  If the anchor hasn't been set yet, assume the beginning of the text*/
    if (textPtr->selAnchor < 0) {
	textPtr->selAnchor = 0;
    }
    if (textPtr->selAnchor <= textPos) {
	selFirst = textPtr->selAnchor;
	selLast = textPos;
    } else {
	selFirst = textPos;
	selLast = textPtr->selAnchor;
    }
    if ((textPtr->selFirst != selFirst) || (textPtr->selLast != selLast)) {
	textPtr->selFirst = selFirst;
	textPtr->selLast = selLast;
	EventuallyRedraw(textPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionProc --
 *
 *	This procedure is called back by Tk when the selection is requested
 *	by someone.  It returns part or all of the selection in a buffer
 *	provided by the caller.
 *
 * Results:
 *	The return value is the number of non-NULL bytes stored at buffer.
 *	Buffer is filled (or partially filled) with a NUL-terminated string
 *	containing part or all of the selection, as given by offset and
 *	maxBytes.
 *
 * Side effects:
 *	None.
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
    TextEditor *textPtr = clientData;
    int size;

    size = (int)strlen(textPtr->string + offset);
    /*
     * Return the string currently in the textbox.
     */
    strncpy(buffer, textPtr->string + offset, maxBytes);
    buffer[maxBytes] = '\0';
    return (size > maxBytes) ? maxBytes : size;
}


static void
DestroyTextEditor(DestroyData data)
{
    TextEditor *textPtr = (TextEditor *)data;

    Blt_FreeOptions(configSpecs, (char *)textPtr, textPtr->display, 0);

    if (textPtr->string != NULL) {
	Blt_Free(textPtr->string);
    }
    if (textPtr->layoutPtr != NULL) {
	Blt_Free(textPtr->layoutPtr);
    }
    if (textPtr->timerToken != NULL) {
	Tcl_DeleteTimerHandler(textPtr->timerToken);
    }
    if (textPtr->tkwin != NULL) {
	Tk_DeleteSelHandler(textPtr->tkwin, XA_PRIMARY, XA_STRING);
    }
    Blt_Free(textPtr);
}

static void
ConfigureTextEditor(TextEditor *textPtr)
{
    int updateNeeded;
#ifdef notdef
    GC newGC;
    TextEditor *textPtr = viewPtr->textPtr;
    XGCValues gcValues;
    unsigned long gcMask;

    /*
     * GC for edit window.
     */
    gcMask = 0;
    newGC = Tk_GetGC(textPtr->tkwin, gcMask, &gcValues);
    if (textPtr->gc != NULL) {
	Tk_FreeGC(textPtr->display, textPtr->gc);
    }
    textPtr->gc = newGC;
    textPtr->width = textPtr->layoutPtr->width + 
	2 * (textPtr->borderWidth + textPtr->highlightWidth);
    textPtr->height = textPtr->layoutPtr->height + 
	2 * (textPtr->borderWidth + textPtr->highlightWidth);
    
    if (Tk_IsMapped(textPtr->tkwin)) {
	if ((textPtr->height != Tk_Height(textPtr->tkwin)) ||
	    (textPtr->width != Tk_Width(textPtr->tkwin))) {
	    Tk_ResizeWindow(textPtr->tkwin, textPtr->width, textPtr->height);
	}
    }
#endif
    updateNeeded = FALSE;
    /* Install the embedded scrollbars as needed.  We defer installing the
     * scrollbars so the scrollbar widgets don't have to exist when they are
     * specified by the -xscrollbar and -yscrollbar options respectively. The
     * down-side is that errors found in the scrollbar name will be
     * backgrounded. */
    if (Blt_ConfigModified(configSpecs, "-xscrollbar", (char *)NULL)) {
	if (textPtr->xScrollbar != NULL) {
	    UnmanageScrollbar(textPtr, textPtr->xScrollbar);
	    textPtr->xScrollbar = NULL;
	}
	if ((textPtr->flags & INSTALL_XSCROLLBAR) == 0) {
	    Tcl_DoWhenIdle(InstallXScrollbar, textPtr);
	    textPtr->flags |= INSTALL_XSCROLLBAR;
	}           
	updateNeeded = TRUE;
    }
    if (Blt_ConfigModified(configSpecs, "-yscrollbar", (char *)NULL)) {
	if (textPtr->yScrollbar != NULL) {
	    UnmanageScrollbar(textPtr, textPtr->yScrollbar);
	    textPtr->yScrollbar = NULL;
	}
	if ((textPtr->flags & INSTALL_YSCROLLBAR) == 0) {
	    Tcl_DoWhenIdle(InstallYScrollbar, textPtr);
	    textPtr->flags |= INSTALL_YSCROLLBAR;
	}           
	updateNeeded = TRUE;
    }
    if (updateNeeded) {
	if ((textPtr->flags & UPDATE_PENDING) == 0) {
	    Tcl_DoWhenIdle(ConfigureScrollbarsProc, textPtr);
	    textPtr->flags |= UPDATE_PENDING;
	}           
    }
    return TCL_OK;
}

int
Blt_TreeView_CreateTextEditor(TreeView *viewPtr, Entry *entryPtr, Column *colPtr)
{
    Tk_Window tkwin;
    TextEditor *textPtr;

    if (viewPtr->comboWin != NULL) {
	Tk_DestroyWindow(viewPtr->comboWin);
    }
    tkwin = Tk_CreateWindow(viewPtr->interp, viewPtr->tkwin, "edit", 
	(char *)NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    Tk_MakeWindowExist(tkwin);

    Tk_SetClass(tkwin, "BltTreeViewEditor"); 

    textPtr = Blt_AssertCalloc(1, sizeof(TextEditor));

    textPtr->interp = viewPtr->interp;
    textPtr->display = Tk_Display(tkwin);
    textPtr->tkwin = tkwin;
    textPtr->borderWidth = 1;
    textPtr->relief = TK_RELIEF_SOLID;
    textPtr->selRelief = TK_RELIEF_FLAT;
    textPtr->selBW = 1;
    textPtr->selAnchor = -1;
    textPtr->selFirst = textPtr->selLast = -1;
    textPtr->onTime = 600;
    textPtr->active = TRUE;
    textPtr->offTime = 300;
    textPtr->viewPtr = viewPtr;
    textPtr->buttonRelief = TK_RELIEF_SUNKEN;
    textPtr->buttonBW = 1;
    viewPtr->comboWin = tkwin;
    Blt_SetWindowInstanceData(tkwin, textPtr);
    Tk_CreateSelHandler(tkwin, XA_PRIMARY, XA_STRING, SelectionProc, textPtr,
                        XA_STRING);
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask |
	FocusChangeMask, EventProc, textPtr);
    Tcl_CreateObjCommand(viewPtr->interp, Tk_PathName(tkwin), 
	TextEditorCmd, textPtr, NULL);
    if (Blt_ConfigureWidgetFromObj(viewPtr->interp, tkwin, configSpecs, 0, 
	(Tcl_Obj **)NULL, (char *)textPtr, 0) != TCL_OK) {
	Tk_DestroyWindow(tkwin);
	return TCL_ERROR;
    }
    AcquireText(viewPtr, textPtr, entryPtr, colPtr);
    textPtr->insertPos = strlen(textPtr->string);
    
    Tk_MoveResizeWindow(tkwin, textPtr->x, textPtr->y, textPtr->width, textPtr->height);
    Tk_MapWindow(tkwin);
    Tk_MakeWindowExist(tkwin);
    XRaiseWindow(textPtr->display, Tk_WindowId(tkwin));
    EventuallyRedraw(textPtr);
    return TCL_OK;
}

static void
DisplayProc(ClientData clientData)
{
    TextEditor *textPtr = clientData;
    Pixmap drawable;
    int i;
    int x1, x2;
    int count, numChars;
    int selStart, selEnd, selLength;
    int x, y;
    TextFragment *fragPtr;
    Blt_FontMetrics fontMetrics;
    Blt_Font font;
    TreeView *viewPtr;
#ifdef notdef
    int buttonX, buttonY, buttonWidth, buttonHeight;
#endif

    viewPtr = textPtr->viewPtr;
    textPtr->flags &= ~REDRAW_PENDING;
    if (!Tk_IsMapped(textPtr->tkwin)) {
	return;
    }
    if (textPtr->colPtr == NULL) {
	return;
    }
    drawable = Blt_GetPixmap(textPtr->display, Tk_WindowId(textPtr->tkwin), 
	Tk_Width(textPtr->tkwin), Tk_Height(textPtr->tkwin), 
	Tk_Depth(textPtr->tkwin));

    Blt_Fill3DRectangle(textPtr->tkwin, drawable, textPtr->border, 0, 0,
	Tk_Width(textPtr->tkwin), Tk_Height(textPtr->tkwin), 
	textPtr->borderWidth, textPtr->relief);

    x = textPtr->borderWidth + textPtr->gap;
    y = textPtr->borderWidth;

#ifdef notdef
    buttonX = Tk_Width(textPtr->tkwin) - 
	(textPtr->borderWidth + textPtr->gap + 1);
    buttonY = textPtr->borderWidth + 1;
#endif

    if (textPtr->icon != NULL) {
	y += (textPtr->height - IconHeight(textPtr->icon)) / 2;
	Tk_RedrawImage(IconBits(textPtr->icon), 0, 0, 
		       IconWidth(textPtr->icon), 
		       IconHeight(textPtr->icon), 
		       drawable, x, y);
	x += IconWidth(textPtr->icon) + textPtr->gap;
    }
    
    font = CHOOSE(viewPtr->font, textPtr->font);
    Blt_Font_GetMetrics(font, &fontMetrics);
    fragPtr = textPtr->layoutPtr->fragments;
    count = 0;
    y = textPtr->borderWidth;
    if (textPtr->height > fontMetrics.linespace) {
	y += (textPtr->height - fontMetrics.linespace) / 2;
    }
    for (i = 0; i < textPtr->layoutPtr->numFragments; i++, fragPtr++) {
	int leftPos, rightPos;

	leftPos = count;
	count += fragPtr->count;
	rightPos = count;
	if ((rightPos < textPtr->selFirst) || (leftPos > textPtr->selLast)) {
	    /* No part of the text fragment is selected. */
	    Blt_Font_Draw(Tk_Display(textPtr->tkwin), drawable, textPtr->gc, font, 
		Tk_Depth(textPtr->tkwin), 0.0f, fragPtr->text, fragPtr->count, 
		x + fragPtr->x, y + fragPtr->y);
	    continue;
	}

	/*
	 *  A text fragment can have up to 3 regions:
	 *
	 *	1. Text before the start the selection
	 *	2. Selected text itself (drawn in a raised border)
	 *	3. Text following the selection.
	 */

	selStart = leftPos;
	selEnd = rightPos;
	/* First adjust selected region for current line. */
	if (textPtr->selFirst > leftPos) {
	    selStart = textPtr->selFirst;
	}
	if (textPtr->selLast < rightPos) {
	    selEnd = textPtr->selLast;
	}
	selLength = (selEnd - selStart);
	x1 = x;

	if (selStart > leftPos) {       /* Normal text preceding the
					 * selection */
	    numChars = (selStart - leftPos);
	    Blt_Font_Measure(font, textPtr->string + leftPos, numChars, 10000, 
		DEF_TEXT_FLAGS, &x1);
	    x1 += x;
	}
	if (selLength > 0) {            /* The selection itself */
	    int width;

	    Blt_Font_Measure(font, fragPtr->text + selStart, selLength, 10000, 
		DEF_TEXT_FLAGS, &x2);
	    x2 += x;
	    width = (x2 - x1) + 1;
	    Blt_Fill3DRectangle(textPtr->tkwin, drawable, textPtr->selBorder,
		x1, y + fragPtr->y - fontMetrics.ascent, 
		width, fontMetrics.linespace,
		textPtr->selBW, textPtr->selRelief);
	}
	Blt_Font_Draw(Tk_Display(textPtr->tkwin), drawable, textPtr->gc, font, 
		Tk_Depth(textPtr->tkwin), 0.0f, fragPtr->text, fragPtr->count, 
		fragPtr->x + x, fragPtr->y + y);
    }
    if ((textPtr->flags & TEXTBOX_FOCUS) && (textPtr->cursorOn)) {
	int left, top, right, bottom;

	IndexToPointer(textPtr);
	left = textPtr->cursorX + 1;
	right = left + 1;
	top = textPtr->cursorY;
	if (textPtr->height > fontMetrics.linespace) {
	    top += (textPtr->height - fontMetrics.linespace) / 2;
	}
	bottom = top + textPtr->cursorHeight - 1;
	XDrawLine(textPtr->display, drawable, textPtr->gc, left, top, left,
		bottom);
	XDrawLine(textPtr->display, drawable, textPtr->gc, left - 1, top, right,
		top);
	XDrawLine(textPtr->display, drawable, textPtr->gc, left - 1, bottom, 
		right, bottom);
    }
    Blt_Draw3DRectangle(textPtr->tkwin, drawable, textPtr->border, 0, 0,
	Tk_Width(textPtr->tkwin), Tk_Height(textPtr->tkwin), 
	textPtr->borderWidth, textPtr->relief);
    XCopyArea(textPtr->display, drawable, Tk_WindowId(textPtr->tkwin),
	textPtr->gc, 0, 0, Tk_Width(textPtr->tkwin), 
	Tk_Height(textPtr->tkwin), 0, 0);
    Tk_FreePixmap(textPtr->display, drawable);
}

/*ARGSUSED*/
static int
ApplyOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;
    int result;

    result = Blt_TreeView_SetEntryValue(interp, textPtr->viewPtr, textPtr->entryPtr,
	textPtr->colPtr, textPtr->string);
    Tk_DestroyWindow(textPtr->tkwin);
    return result;
}

/*ARGSUSED*/
static int
CancelOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;

    Tk_DestroyWindow(textPtr->tkwin);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, textPtr->tkwin, 
	configSpecs, (char *)textPtr, objv[2], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * 	This procedure is called to process a list of configuration options
 *	database, in order to reconfigure the one of more entries in the
 *	widget.
 *
 *	  .c configure option value
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for viewPtr; old resources get freed, if there were
 *	any.  The hypertext is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;

    if (objc == 2) {
	return Blt_ConfigureInfoFromObj(interp, textPtr->tkwin, configSpecs,
                (char *)textPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, textPtr->tkwin, configSpecs,
                (char *)textPtr, objv[3], 0);
    }
    if (Blt_ConfigureWidgetFromObj(interp, textPtr->tkwin, configSpecs,
        objc - 2, objv + 2, (char *)textPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    ConfigureTextEditor(textPtr);
    EventuallyRedraw(textPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *	Remove one or more characters from the label of an entry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets freed, the entry gets modified and (eventually)
 *	redisplayed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;
    int firstPos, lastPos;

    if (textPtr->entryPtr == NULL) {
	return TCL_OK;
    }
    if (GetIndexFromObj(interp, textPtr, objv[2], &firstPos) != TCL_OK) {
	return TCL_ERROR;
    }
    lastPos = firstPos;
    if ((objc == 4) && 
	(GetIndexFromObj(interp, textPtr, objv[3], &lastPos) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (firstPos > lastPos) {
	return TCL_OK;
    }
    return DeleteText(textPtr, firstPos, lastPos);
}


/*
 *---------------------------------------------------------------------------
 *
 * IcursorOp --
 *
 *	Returns the numeric index of the given string. Indices can be
 *	one of the following:
 *
 *	"anchor"	Selection anchor.
 *	"end"		End of the label.
 *	"insert"	Insertion cursor.
 *	"sel.first"	First character selected.
 *	"sel.last"	Last character selected.
 *	@x,y		Index at X-Y screen coordinate.
 *	number		Returns the same number.
 *
 * Results:
 *	A standard TCL result.  If the argument does not represent a
 *	valid label index, then TCL_ERROR is returned and the interpreter
 *	result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IcursorOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;
    int textPos;

    if (GetIndexFromObj(interp, textPtr, objv[2], &textPos) != TCL_OK) {
	return TCL_ERROR;
    }
    if (textPtr->colPtr != NULL) {
	textPtr->insertPos = textPos;
	IndexToPointer(textPtr);
	EventuallyRedraw(textPtr);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * IndexOp --
 *
 *	Returns the numeric index of the given string. Indices can be one
 *	of the following:
 *
 *	"anchor"	Selection anchor.
 *	"end"		End of the label.
 *	"insert"	Insertion cursor.
 *	"sel.first"	First character selected.
 *	"sel.last"	Last character selected.
 *	@x,y		Index at X-Y screen coordinate.
 *	number		Returns the same number.
 *
 * Results:
 *	A standard TCL result.  If the argument does not represent a valid
 *	label index, then TCL_ERROR is returned and the interpreter result
 *	will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IndexOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;
    int textPos;

    if (GetIndexFromObj(interp, textPtr, objv[2], &textPos) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((textPtr->colPtr != NULL) && (textPtr->string != NULL)) {
	int numChars;

	numChars = Tcl_NumUtfChars(textPtr->string, textPos);
	Tcl_SetIntObj(Tcl_GetObjResult(interp), numChars);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InsertOp --
 *
 *	Add new characters to the label of an entry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	New information gets added to textPtr; it will be redisplayed soon,
 *	but not necessarily immediately.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InsertOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;
    int extra;
    int insertPos;
    char *string;

    if (textPtr->entryPtr == NULL) {
	return TCL_ERROR;
    }
    if (GetIndexFromObj(interp, textPtr, objv[2], &insertPos) != TCL_OK) {
	return TCL_ERROR;
    }
    string = Tcl_GetStringFromObj(objv[3], &extra);
    if (extra == 0) {                   /* Nothing to insert. Move the
					 * cursor anyways. */
	textPtr->insertPos = insertPos;
    } else {
	InsertText(textPtr, string, insertPos, extra);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PostOp --
 *
 *      Posts this editor at the given root window coordinates.
 *
 *
 *      pathName post -text textString -align align 
 *      pathName post -window button -align align 
 *      pathName post -bbox "x1 y1 x2 y2" -align align
 *      pathName post -cascade "x1 y1" 
 *      pathName post (assume parent) -align bottom (default alignment is left).
 *
 *---------------------------------------------------------------------------
 */
static int
PostOp(ClientData clientData, Tcl_Interp *interp, int objc,
	Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;
    int x, y;

    memset(&textPtr->post, 0, sizeof(PostInfo));
    textPtr->post.tkwin     = Tk_Parent(textPtr->tkwin);
    textPtr->post.editorWidth = textPtr->normalWidth;
    /* Process switches  */
    if (Blt_ParseSwitches(interp, postSwitches, objc - 2, objv + 2, textPtr,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    textPtr->flags |= DROPDOWN;
    switch (textPtr->post.flags) {
    case POST_PARENT:
    case POST_WINDOW:
	{
	    Tk_Window tkwin;
	    int x, y, w, h;
	    int rootX, rootY;
	    
	    tkwin = textPtr->post.tkwin;
	    w = Tk_Width(tkwin);
	    h = Tk_Height(tkwin);
	    x = Tk_X(tkwin);
	    y = Tk_Y(tkwin);
	    Tk_GetRootCoords(Tk_Parent(tkwin), &rootX, &rootY);
	    x += rootX;
	    y += rootY;
	    textPtr->post.x1 = x;
	    textPtr->post.y1 = y;
	    textPtr->post.x2 = x + w;
	    textPtr->post.y2 = y + h;
	}
	break;
    case POST_REGION:
    case POST_CASCADE:
	break;
    case POST_POPUP:
	textPtr->flags &= ~DROPDOWN;
	break;
    }
    textPtr->post.editorWidth = textPtr->post.x2 - textPtr->post.x1;
    textPtr->post.editorHeight = textPtr->post.y2 - textPtr->post.y1;
    if ((textPtr->post.editorWidth != textPtr->post.lastEditorWidth) ||
	(textPtr->flags & LAYOUT_PENDING)) {
	ComputeEditorGeometry(textPtr);
    }
    textPtr->post.lastEditorWidth = textPtr->post.editorWidth;
    x = 0;                              /* Suppress compiler warning; */
    y = textPtr->post.y2;
    switch (textPtr->post.align) {
    case ALIGN_LEFT:
	x = textPtr->post.x1;
	break;
    case ALIGN_CENTER:
	{
	    int w;

	    w = textPtr->post.x2 - textPtr->post.x1;
	    x = textPtr->post.x1 + (w - textPtr->normalWidth) / 2; 
	}
	break;
    case ALIGN_RIGHT:
	x = textPtr->post.x2 - textPtr->normalWidth;
	break;
    }
    FixEditorCoords(textPtr, &x, &y);
    /*
     * If there is a post command for the editor, execute it.  This may
     * change the size of the editor, so be sure to recompute the editor's
     * geometry if needed.
     */
    if (textPtr->postCmdObjPtr != NULL) {
	int result;

	Tcl_IncrRefCount(textPtr->postCmdObjPtr);
	result = Tcl_EvalObjEx(interp, textPtr->postCmdObjPtr, TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(textPtr->postCmdObjPtr);
	if (result != TCL_OK) {
	    return result;
	}
	/*
	 * The post commands could have deleted the editor, which means we
	 * are dead and should go away.
	 */
	if (textPtr->tkwin == NULL) {
	    return TCL_OK;
	}
	if (textPtr->flags & LAYOUT_PENDING) {
	    ComputeEditorGeometry(textPtr);
	}
    }

    /*
     * Adjust the position of the editor if necessary to keep it visible on the
     * screen.  There are two special tricks to make this work right:
     *
     * 1. If a virtual root window manager is being used then
     *    the coordinates are in the virtual root window of
     *    textPtr's parent;  since the editor uses override-redirect
     *    mode it will be in the *real* root window for the screen,
     *    so we have to map the coordinates from the virtual root
     *    (if any) to the real root.  Can't get the virtual root
     *    from the editor itself (it will never be seen by the wm)
     *    so use its parent instead (it would be better to have an
     *    an option that names a window to use for this...).
     * 2. The editor may not have been mapped yet, so its current size
     *    might be the default 1x1.  To compute how much space it
     *    needs, use its requested size, not its actual size.
     *
     * Note that this code assumes square screen regions and all positive
     * coordinates. This does not work on a Mac with multiple monitors. But
     * then again, Tk has other problems with this.
     */
    {
	int rootX, rootY, rootWidth, rootHeight;
	int sw, sh;
	Tk_Window parent;

	parent = Tk_Parent(textPtr->tkwin);
	Blt_SizeOfScreen(textPtr->tkwin, &sw, &sh);
	Tk_GetVRootGeometry(parent, &rootX, &rootY, &rootWidth, &rootHeight);
	x += rootX;
	y += rootY;
	if (x < 0) {
	    x = 0;
	}
	if (y < 0) {
	    y = 0;
	}
	if ((x + textPtr->width) > sw) {
	    x = sw - textPtr->width;
	}
	if ((y + textPtr->height) > sh) {
	    y = sh - textPtr->height;
	}
	Tk_MoveToplevelWindow(textPtr->tkwin, x, y);
	Tk_MapWindow(textPtr->tkwin);
	Blt_MapToplevelWindow(textPtr->tkwin);
	Blt_RaiseToplevelWindow(textPtr->tkwin);
#ifdef notdef
	TkWmRestackToplevel(textPtr->tkwin, Above, NULL);
#endif
    }
    if (textPtr->activePtr == NULL) {
	ActivateItem(textPtr, textPtr->firstPtr);
    }
    textPtr->flags |= POSTED;
    return TCL_OK;
}

/*ARGSUSED*/
static int
SelectionAdjustOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;
    int textPos;
    int half1, half2;

    if (GetIndexFromObj(interp, textPtr, objv[3], &textPos) != TCL_OK) {
	return TCL_ERROR;
    }
    half1 = (textPtr->selFirst + textPtr->selLast) / 2;
    half2 = (textPtr->selFirst + textPtr->selLast + 1) / 2;
    if (textPos < half1) {
	textPtr->selAnchor = textPtr->selLast;
    } else if (textPos > half2) {
	textPtr->selAnchor = textPtr->selFirst;
    }
    return SelectText(textPtr, textPos);
}

/*ARGSUSED*/
static int
SelectionClearOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;

    if (textPtr->selFirst != -1) {
	textPtr->selFirst = textPtr->selLast = -1;
	EventuallyRedraw(textPtr);
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
SelectionFromOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;
    int textPos;

    if (GetIndexFromObj(interp, textPtr, objv[3], &textPos) != TCL_OK) {
	return TCL_ERROR;
    }
    textPtr->selAnchor = textPos;
    return TCL_OK;
}

/*ARGSUSED*/
static int
SelectionPresentOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		   Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;

    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (textPtr->selFirst != -1));
    return TCL_OK;
}

/*ARGSUSED*/
static int
SelectionRangeOp(ClientData clientData, Tcl_Interp *interp, int objc,
		 Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;
    int selFirst, selLast;

    if (GetIndexFromObj(interp, textPtr, objv[3], &selFirst) != TCL_OK) {
	return TCL_ERROR;
    }
    if (GetIndexFromObj(interp, textPtr, objv[4], &selLast) != TCL_OK) {
	return TCL_ERROR;
    }
    textPtr->selAnchor = selFirst;
    return SelectText(textPtr, selLast);
}

/*ARGSUSED*/
static int
SelectionToOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;
    int textPos;

    if (GetIndexFromObj(interp, textPtr, objv[3], &textPos) != TCL_OK) {
	return TCL_ERROR;
    }
    return SelectText(textPtr, textPos);
}


static Blt_OpSpec selectionOps[] =
{
    {"adjust", 1, SelectionAdjustOp, 4, 4, "index",},
    {"clear", 1, SelectionClearOp, 3, 3, "",},
    {"from", 1, SelectionFromOp, 4, 4, "index"},
    {"present", 1, SelectionPresentOp, 3, 3, ""},
    {"range", 1, SelectionRangeOp, 5, 5, "start end",},
    {"to", 1, SelectionToOp, 4, 4, "index"},
};

static int numSelectionOps = sizeof(selectionOps) / sizeof(Blt_OpSpec);

/*
 *	This procedure handles the individual options for text selections.
 *	The selected text is designated by start and end indices into the
 *	text pool.  The selected segment has both a anchored and unanchored
 *	ends.  The following selection operations are implemented:
 *
 *	  "adjust"	- resets either the first or last index
 *			  of the selection.
 *	  "clear"	- clears the selection. Sets first/last
 *			  indices to -1.
 *	  "from"	- sets the index of the selection anchor.
 *	  "present"	- return "1" if a selection is available,
 *			  "0" otherwise.
 *	  "range"	- sets the first and last indices.
 *	  "to"		- sets the index of the un-anchored end.
 */
static int
SelectionOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numSelectionOps, selectionOps, BLT_OP_ARG2, 
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
 * UnpostOp --
 *
 *      Unposts this text editor.
 *
 *      pathName unpost 
 *
 *---------------------------------------------------------------------------
 */
static int
UnpostOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    TextEditor *textPtr = clientData;

    if (!WithdrawMenu(textPtr)) {
	return TCL_OK;          /* This menu is already unposted. */
    }
    /*
     * If there is a unpost command for the menu, execute it.  
     */
    if (textPtr->unpostCmdObjPtr != NULL) {
	int result;

	Tcl_IncrRefCount(textPtr->unpostCmdObjPtr);
	result = Tcl_EvalObjEx(interp, textPtr->unpostCmdObjPtr, 
		TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(textPtr->unpostCmdObjPtr);
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    textPtr->flags &= ~POSTED;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextEditorCmd --
 *
 *	This procedure handles entry operations.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec comboOps[] =
{
    {"apply",     1, ApplyOp,     2, 2, "",},
    {"cancel",    2, CancelOp,    2, 2, "",},
    {"cget",      2, CgetOp,      3, 3, "value",},
    {"configure", 2, ConfigureOp, 2, 0, "?option value...?",},
    {"delete",    1, DeleteOp,    3, 4, "first ?last?"},
    {"icursor",   2, IcursorOp,   3, 3, "index"},
    {"index",     3, IndexOp,     3, 3, "index"},
    {"insert",    3, InsertOp,    4, 4, "index string"},
    {"selection", 1, SelectionOp, 2, 0, "args"},
};
static int numComboOps = sizeof(comboOps) / sizeof(Blt_OpSpec);

static int
TextEditorCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numComboOps, comboOps, BLT_OP_ARG1, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}

#endif


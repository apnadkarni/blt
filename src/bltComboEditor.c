/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltComboEditor.c --
 *
 * This module implements a popup editor for the BLT toolkit.
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
 * blt::comboeditor -
 * 
 * 1. Find operation search for text.  Returns a list of the 
 *    first and last indices if match is found.
 * 2. Add resize shangle
 * 3. Fix right/center justified deletes?
 * 4. Fix bindings with re/ to selections
 * 5. Test size restrictions.
 * 6. Add calls to CleanText.  
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_COMBOEDITOR

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
#include "bltOp.h"

#define TEXT_FLAGS      (TK_PARTIAL_OK | TK_AT_LEAST_ONE)
#define EVENT_MASK	 (ExposureMask|StructureNotifyMask|FocusChangeMask)
#define CHILD_EVENT_MASK (ExposureMask|StructureNotifyMask)

#define IPAD		4		/* Internal pad between components. */
#define XPAD		4
#define YPAD		4		/* Internal pad between components. */
#define ICWIDTH		2		/* External pad between border and
					 * arrow. */
#define CharIndexToByteOffset(s, n)	(Tcl_UtfAtIndex(s, n) - s)
#define CLAMP(x,min,max) ((((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x)))
#define FCLAMP(x)       ((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))

#define SCREENX(t, wx) ((wx) - (t)->xOffset + (t)->borderWidth + XPAD)
#define SCREENY(t, wy) ((wy) - (t)->yOffset + (t)->borderWidth + YPAD)

#define WORLDX(t, sx)  ((sx) - (t)->borderWidth - XPAD + (t)->xOffset)
#define WORLDY(t, sy)  ((sy) - (t)->borderWidth - YPAD + (t)->yOffset)

#define VPORTWIDTH(t)  \
    (Tk_Width((t)->tkwin) - 2*((t)->borderWidth+XPAD) - (t)->yScrollbarWidth)
#define VPORTHEIGHT(t) \
    (Tk_Height((t)->tkwin) - 2*((t)->borderWidth+YPAD) - (t)->xScrollbarHeight)

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
#define SELECT_PENDING          (1<<3)  /* Indicates that the selection has
					 * changed and that and an update
					 * is pending.  */
#define DROPDOWN                (1<<5)  /* Indicates the combomenu is a
					 * drop down menu as opposed to a
					 * popup.  */
#define POSTED                  (1<<6)  /* Indicates the combomenu is
					 * currently posted. */

#define SCROLLX                 (1<<7)
#define SCROLLY                 (1<<8)
#define SCROLL_PENDING          (SCROLLX|SCROLLY)

#define INSTALL_XSCROLLBAR      (1<<9)  /* Indicates that the x scrollbar
					 * is scheduled to be installed at
					 * the next idle point. */
#define INSTALL_YSCROLLBAR      (1<<10)  /* Indicates that the y scrollbar
					 * is scheduled to be installed at
					 * the next idle point. */
#define RESTRICT_MIN            (1<<11) /* Indicates to constrain the width
					 * of the menu to the minimum size
					 * of the parent widget that posted
					 * the menu. */
#define RESTRICT_MAX            (1<<12) /* Indicates to constrain the width
					 * of the menu of the maximum size
					 * of the parent widget that posted
					 * the menu. */
#define EXPORT_SELECTION        (1<<13)	/* The selection is exported to the
					 * clipboard. */
#define ICURSOR                 (1<<14)	/* Insertion cursor is active.
					 * Depending upon the timer
					 * interval, it may be drawn or not
					 * drawn. */
#define ICURSOR_ON              (1<<16)	/* The insertion cursor is currently
					 * visible on screen. */
#define FOCUS                   (1<<17)	/* The widget has focus. */
#define GEOMETRY                (1<<18)	/* The widget has focus. */
#define ACTIVE                  (1<<19)	/* The widget has focus. */
#define RESTRICT_NONE           (0)
#define INITIALIZED             (1<<22)
#define READONLY                (1<<23)	/* The widget is is read only. */


#define DEF_ACTIVE_BG                   RGB_SKYBLUE4
#define DEF_ACTIVE_FG                   RGB_WHITE
#define DEF_BACKGROUND                  RGB_WHITE
#define DEF_BORDERWIDTH                 STD_BORDERWIDTH
#define DEF_COMMAND                     ((char *)NULL)
#define DEF_CURSOR                      (char *)NULL
#define DEF_EXPORT_SELECTION            "no"
#define DEF_FONT                        STD_FONT_NORMAL
#define DEF_HEIGHT                      "0"
#define DEF_INSERT_COLOR                STD_NORMAL_FOREGROUND
#define DEF_INSERT_OFFTIME              "300"
#define DEF_INSERT_ONTIME               "600"
#define DEF_JUSTIFY                     "left"
#define DEF_NORMAL_BACKGROUND           STD_NORMAL_BACKGROUND
#define DEF_NORMAL_FG                   STD_ACTIVE_FOREGROUND
#define DEF_NORMAL_FG_MONO              STD_ACTIVE_FG_MONO
#define DEF_POST_COMMAND                ((char *)NULL)
#define DEF_READONLY                    "0"
#define DEF_RELIEF                      "solid"
#define DEF_SCROLLBAR                   ((char *)NULL)
#define DEF_SCROLL_CMD                  ((char *)NULL)
#define DEF_SCROLL_INCR                 "2"
#define DEF_SELECT_BACKGROUND           RGB_SKYBLUE4
#define DEF_SELECT_BG_MONO              STD_SELECT_BG_MONO
#define DEF_SELECT_BORDERWIDTH          "1"
#define DEF_SELECT_FG_MONO              STD_SELECT_FG_MONO
#define DEF_SELECT_FOREGROUND           STD_SELECT_FOREGROUND
#define DEF_SELECT_RELIEF               "flat"
#define DEF_SHOW                        (char *)NULL
#define DEF_TAKE_FOCUS                  "1"
#define DEF_TEXT                        (char *)NULL
#define DEF_TEXT_NORMAL_BG              RGB_WHITE
#define DEF_TEXT_NORMAL_FG              RGB_BLACK
#define DEF_UNPOST_COMMAND             ((char *)NULL)
#define DEF_WIDTH                       "0"

typedef int CharIndex;			/* Character index regardless of
					 * how many bytes (UTF) are
					 * used. */
typedef int ByteOffset;			/* Offset in bytes from the start
					 * of the text string.  This may be
					 * different between the normal
					 * text and the screen text if
					 * -show is used. */

typedef struct {
    const char *text;                   /* Pointer to start of line. */
    int numBytes;                       /* Number of bytes in line. */
    int width, height;                  /* Width of the line in pixels. */
    int worldX, worldY;
    CharIndex char1, char2;             /* Starting and ending character
                                         * indices. */
} TextLine;

typedef enum RecordTypes {
    INSERT_OP, DELETE_OP
} RecordType;

typedef struct _EditRecord {
    struct _EditRecord *nextPtr;
    RecordType type;
    CharIndex insertIndex;		/* Current index of the cursor. */
    CharIndex index;			/* Character index where text was
					   inserted. */
    int numBytes;			/* # of bytes in text string. */
    int numChars;			/* # of characters in text
                                         * string. */
    char text[1];
} EditRecord;

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
 * ComboEditor --
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
 *		n = 
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
    Tcl_Command cmdToken;               /* Token for frame's widget
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

    /* ComboEditor-specific fields */
    Blt_DBuffer dbuffer;                /* Buffer used to hold the text. */
    Blt_Bg normalBg;
    Blt_Bg activeBg;
    Blt_Bg textBg;

    XColor *normalColor;
    XColor *activeColor;
    XColor *textFg;

    GC textGC;

    /*
     * Selection Information:
     *
     * The selection is the rectangle that contains selected text.  It is
     * displayed as a solid colored entry with optionally a 3D border.
     */
    CharIndex selAnchor;		/* Character index representing the
					 * fixed end of selection. Used to
					 * extend the selection while
					 * maintaining the other end of the
					 * selection. */
    CharIndex selFirst;			/* Character index of the first
					 * character in the selection. */
    CharIndex selLast;			/* Character Index of the last
					 * character in the selection. */
    int selRelief;			/* Relief of selected
					 * items. Currently is always
					 * raised. */
    int selBorderWidth;                 /* Border width of a selected
                                         * text.*/
    XColor *selectFg;			/* Text color of a selected
                                         * text. */
    GC selectGC;
    Blt_Bg selectBg;
    Tcl_Obj *selectCmdObjPtr;

    /*
     * Scanning Information:
     */
    int scanAnchor;			/* Scan anchor in screen
					 * coordinates. */
    int scanX;				/* x-offset of the start of the
					 * scan in world coordinates.*/

    /*
     * Scrolling Information:
     */
    Tcl_Obj *scrollCmdObjPtr;		/* Command prefix for communicating
					 * with scrollbars.  If NULL,
					 * indicates no command to
					 * issue. */
    int scrollUnits;			/* # of pixels per scroll unit. */
    int scrollX;			/* x-offset of the start of visible
					 * text in the viewport. */
    int viewWidth;			/* Width of the viewport. */
    
    Blt_Font font;			/* Font of text to be display in
					 * entry. */

    short int numChars;			/* # character in text string. */
    short int numBytes;			/* bytes of in actual text string. */
    short int numScreenBytes;		/* # bytes in displayed text. */

    /*
     * Insertion cursor information:
     */
    GC insertGC;
    XColor *insertColor;		/* Color used to draw vertical bar
					 * for insertion cursor. */
    int insertOffTime;			/* Time in milliseconds cursor
					 * should spend in "off" state for
					 * each blink. */
    int insertOnTime;			/* Time in milliseconds cursor
					 * should spend in "off" state for
					 * each blink. */
    Tcl_TimerToken insertTimerToken;	/* Handle for a timer event called
					 * periodically to blink the
					 * insertion cursor. */

    int insertWidth;			/* Total width of insert cursor. */
    CharIndex insertIndex;		/* Character index of the insertion
					 * cursor.  */


    int prefTextWidth;			/* Desired width of text, measured
					 * in average characters. */

    int worldWidth, worldHeight;        /* Size of the world. */
    short int textWidth, textHeight;
    short int cursorHeight, cursorWidth;
    int normalWidth, normalHeight;
    int xOffset, yOffset;               /* Scroll offsets. */

    Tcl_Obj *cmdObjPtr;			/* If non-NULL, command to be
					 * executed when this menu is
					 * posted. */
    Tcl_Obj *menuObjPtr;	
    Tk_Window menuWin;

    Tcl_Obj *postCmdObjPtr;		/* If non-NULL, command to be
					 * executed when this menu is
					 * posted. */
    Tcl_Obj *unpostCmdObjPtr;           /* If non-NULL, command to be executed
					 * when this menu is posted. */
    int menuAnchor;
    EditRecord *undoPtr, *redoPtr;
    const char *cipher;			/* If non-NULL, this is the
					 * character to display for every
					 * character of text. */
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
    Blt_Limits reqWidth, reqHeight;     /* Requested width and height of
                                         * the widget. */
    TextLine *lines;
    int gap;
    int numLines;
    int firstLine, lastLine;
    int cursorX, cursorY;               /* Location of cursor. */
    int leader, justify;
    int x, y;
} ComboEditor;

static Blt_OptionParseProc ObjToTextProc;
static Blt_OptionPrintProc TextToObjProc;
static Blt_CustomOption textOption = {
    ObjToTextProc, TextToObjProc, NULL, (ClientData)0
};

extern Blt_CustomOption bltLimitsOption;

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_ACTIVE_BG, 
	Blt_Offset(ComboEditor, activeBg), 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
	"ActiveForeground", DEF_ACTIVE_FG, 
	Blt_Offset(ComboEditor, activeColor), 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background", 
	DEF_NORMAL_BACKGROUND, Blt_Offset(ComboEditor, normalBg), 0 },
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_BORDERWIDTH, Blt_Offset(ComboEditor, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", 
	DEF_COMMAND, Blt_Offset(ComboEditor, cmdObjPtr), 
	BLT_CONFIG_NULL_OK , },
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_CURSOR, Blt_Offset(ComboEditor, cursor), 
	BLT_CONFIG_NULL_OK , },
    {BLT_CONFIG_BITMASK, "-exportselection", "exportSelection", 
	"ExportSelection", DEF_EXPORT_SELECTION, Blt_Offset(ComboEditor, flags),
	BLT_CONFIG_DONT_SET_DEFAULT , 
	(Blt_CustomOption *)EXPORT_SELECTION},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 0, 
	0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_FONT, 
	Blt_Offset(ComboEditor, font), 0, },
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", 
	DEF_NORMAL_FG, Blt_Offset(ComboEditor, normalColor), 0, },
    {BLT_CONFIG_CUSTOM, "-height", "height", "Height", DEF_HEIGHT, 
	Blt_Offset(ComboEditor, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT,
	&bltLimitsOption},
    {BLT_CONFIG_COLOR, "-insertbackground", "insertBackground", 
	"InsertBackground", DEF_INSERT_COLOR, 
	Blt_Offset(ComboEditor, insertColor), 0},
    {BLT_CONFIG_INT, "-insertofftime", "insertOffTime", "OffTime",
	DEF_INSERT_OFFTIME, Blt_Offset(ComboEditor, insertOffTime), 
	BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_INT, "-insertontime", "insertOnTime", "OnTime",
	DEF_INSERT_ONTIME, Blt_Offset(ComboEditor, insertOnTime), 
	BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_JUSTIFY, 
	Blt_Offset(ComboEditor, justify), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-postcommand", "postCommand", "PostCommand", 
	DEF_POST_COMMAND, Blt_Offset(ComboEditor, postCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BITMASK_INVERT, "-readonly", "readOnly", "ReadOnly", 
	DEF_READONLY, Blt_Offset(ComboEditor, flags), 
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)READONLY},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_RELIEF, 
	Blt_Offset(ComboEditor, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground",
        "Background", DEF_SELECT_BACKGROUND, Blt_Offset(ComboEditor, selectBg),
        0},
    {BLT_CONFIG_PIXELS_NNEG, "-selectborderwidth", "selectBorderWidth", 
	"BorderWidth", DEF_SELECT_BORDERWIDTH, 
	Blt_Offset(ComboEditor, selBorderWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Foreground",
	DEF_SELECT_FG_MONO, Blt_Offset(ComboEditor, selectFg),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Foreground",
	DEF_SELECT_FOREGROUND, Blt_Offset(ComboEditor, selectFg),
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_RELIEF, "-selectrelief", "selectRelief", "Relief",
	DEF_SELECT_RELIEF, Blt_Offset(ComboEditor, selRelief),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-show", "show", "Show", DEF_SHOW, 
	Blt_Offset(ComboEditor, cipher), 
	BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_CUSTOM, "-text", "text", "Text", DEF_TEXT, 0, 0, &textOption},
    {BLT_CONFIG_BACKGROUND, "-textbackground", "textBackground", "Background", 
	DEF_TEXT_NORMAL_BG, Blt_Offset(ComboEditor, textBg), 0},
    {BLT_CONFIG_COLOR, "-textforeground", "textForeground", "TextForeground",
	DEF_TEXT_NORMAL_FG, Blt_Offset(ComboEditor, textFg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-textwidth", "textWidth", "TextWidth",
	DEF_WIDTH, Blt_Offset(ComboEditor, prefTextWidth), 
	BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_OBJ, "-unpostcommand", "unpostCommand", "UnpostCommand", 
	DEF_UNPOST_COMMAND, Blt_Offset(ComboEditor, unpostCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-width", "width", "Width", DEF_WIDTH, 
	Blt_Offset(ComboEditor, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT,
	&bltLimitsOption},
    {BLT_CONFIG_OBJ, "-xscrollbar", "xScrollbar", "Scrollbar", 
	DEF_SCROLLBAR, Blt_Offset(ComboEditor, xScrollbarObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	DEF_SCROLL_CMD, Blt_Offset(ComboEditor, xScrollCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_POS, "-xscrollincrement", "xScrollIncrement",
	"ScrollIncrement", DEF_SCROLL_INCR, 
	 Blt_Offset(ComboEditor, xScrollUnits), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-yscrollbar", "yScrollbar", "Scrollbar", 
	DEF_SCROLLBAR, Blt_Offset(ComboEditor, yScrollbarObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
	DEF_SCROLL_CMD, Blt_Offset(ComboEditor, yScrollCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_POS, "-yscrollincrement", "yScrollIncrement",
	"ScrollIncrement", DEF_SCROLL_INCR, 
	 Blt_Offset(ComboEditor, yScrollUnits),BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0}
};

static Blt_SwitchParseProc PostPopupSwitchProc;
static Blt_SwitchCustom postPopupSwitch = {
    PostPopupSwitchProc, NULL, NULL, 0,
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
	Blt_Offset(ComboEditor, post.align), 0, 0, &postAlignSwitch},
    {BLT_SWITCH_CUSTOM, "-box", "x1 y1 x2 y2", (char *)NULL,
	0, 0, 0, &postBoxSwitch},
    {BLT_SWITCH_OBJ, "-text", "string", (char *)NULL,
	Blt_Offset(ComboEditor, post.textObjPtr), 0},
    {BLT_SWITCH_OBJ, "-command", "cmdPrefix", (char *)NULL,
	Blt_Offset(ComboEditor, cmdObjPtr), 0},
    {BLT_SWITCH_CUSTOM, "-popup", "x y", (char *)NULL,
	0, 0, 0, &postPopupSwitch},
    {BLT_SWITCH_CUSTOM, "-window", "path", (char *)NULL,
	Blt_Offset(ComboEditor, post.tkwin), 0, 0, &postWindowSwitch},
    {BLT_SWITCH_END}
};

static Tcl_IdleProc DisplayProc;
static Tcl_IdleProc SelectCmdProc;
static Tcl_TimerProc BlinkCursorTimerProc;
static Tcl_ObjCmdProc ComboEditorCmdProc;
static Tcl_ObjCmdProc EditorInstCmdProc;

static Tk_LostSelProc LostSelectionProc;
static Tk_SelectionProc SelectionProc;
static Tk_EventProc EditorEventProc;
static Tk_EventProc ScrollbarEventProc;
static Tcl_FreeProc FreeEditorProc;

static Tk_GeomRequestProc ScrollbarGeometryProc;
static Tk_GeomLostSlaveProc ScrollbarCustodyProc;
static Tk_GeomMgr editorMgrInfo = {
    (char *)"comboeditor",              /* Name of geometry manager used by
					 * winfo. */
    ScrollbarGeometryProc,              /* Procedure to for new geometry
					 * requests. */
    ScrollbarCustodyProc,               /* Procedure when scrollbar is taken
					 * away. */
};

static void ComputeGeometry(ComboEditor *editPtr);

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
EventuallyRedraw(ComboEditor *editPtr)
{
    if ((editPtr->tkwin != NULL) && 
	((editPtr->flags & REDRAW_PENDING) == 0)) {
	editPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayProc, editPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyInvokeSelectCmd --
 *
 *      Queues a request to execute the -selectcommand code associated with
 *      the widget at the next idle point.  Invoked whenever the selection
 *      changes.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      TCL code gets executed for some application-specific task.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyInvokeSelectCmd(ComboEditor *editPtr) 
{
    if ((editPtr->flags & SELECT_PENDING) == 0) {
	editPtr->flags |= SELECT_PENDING;
	Tcl_DoWhenIdle(SelectCmdProc, editPtr);
    }
}


static void
SetTextFromObj(ComboEditor *editPtr, Tcl_Obj *objPtr) 
{
    const char *string;

    string = (const char *)Blt_DBuffer_SetFromObj(editPtr->dbuffer, objPtr);
    editPtr->flags |= (ICURSOR | SCROLL_PENDING | LAYOUT_PENDING);
    editPtr->numBytes = Blt_DBuffer_Length(editPtr->dbuffer);
    editPtr->scrollX = 0;
    editPtr->selFirst = editPtr->selLast = -1;
    editPtr->insertIndex = editPtr->numChars =
        Tcl_NumUtfChars(string, editPtr->numBytes);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetBoundedWidth --
 *
 *      Bounds a given width value to the limits described in the limit
 *      structure.  The initial starting value may be overridden by the
 *      nominal value in the limits.
 *
 * Results:
 *      Returns the constrained value.
 *
 *---------------------------------------------------------------------------
 */
static int
GetBoundedWidth(ComboEditor *editPtr, int w)     
{
    /* Check widgets for requested width values. */
    if (editPtr->reqWidth.flags & LIMITS_NOM_SET) {
	w = editPtr->reqWidth.nom;      /* Override initial value */
    }
    if (w < editPtr->reqWidth.min) {
	w = editPtr->reqWidth.min;      /* Bounded by minimum value */
    }
    if (w > editPtr->reqWidth.max) {
	w = editPtr->reqWidth.max;      /* Bounded by maximum value */
    }
    if (editPtr->flags & (RESTRICT_MIN|RESTRICT_MAX)) {
	if ((editPtr->flags & RESTRICT_MIN) &&
	    (w < editPtr->post.editorWidth)) {
	    w = editPtr->post.editorWidth;
	}
	if ((editPtr->flags & RESTRICT_MAX) &&
	    (w > editPtr->post.editorWidth)) {
	    w = editPtr->post.editorWidth;
	}
    }
    if (w > WidthOfScreen(Tk_Screen(editPtr->tkwin))) {
	w = WidthOfScreen(Tk_Screen(editPtr->tkwin));
    }
    return w;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetBoundedHeight --
 *
 *      Bounds a given value to the limits described in the limit
 *      structure.  The initial starting value may be overridden by the
 *      nominal value in the limits.
 *
 * Results:
 *      Returns the constrained value.
 *
 *---------------------------------------------------------------------------
 */
static int
GetBoundedHeight(ComboEditor *editPtr, int h)    
{
    /* Check widgets for requested height values. */
    if (editPtr->reqHeight.flags & LIMITS_NOM_SET) {
	h = editPtr->reqHeight.nom;     /* Override initial value */
    }
    if (h < editPtr->reqHeight.min) {
	h = editPtr->reqHeight.min;     /* Bounded by minimum value */
    }
    if (h > editPtr->reqHeight.max) {
	h = editPtr->reqHeight.max;     /* Bounded by maximum value */
    }
    if (h > HeightOfScreen(Tk_Screen(editPtr->tkwin))) {
	h = HeightOfScreen(Tk_Screen(editPtr->tkwin));
    }
    return h;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetAlignFromObj --
 *
 *      Converts string into alignment value.  
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
GetAlignFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *alignPtr)
{
    char c;
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'l') && (strncmp(string, "left", length) == 0)) {
	*alignPtr = ALIGN_LEFT;
    } else if ((c == 'r') && (strncmp(string, "right", length) == 0)) {
	*alignPtr = ALIGN_RIGHT;
    } else if ((c == 'c') && (strncmp(string, "center", length) == 0)) {
	*alignPtr = ALIGN_CENTER;
    } else {
	Tcl_AppendResult(interp, "bad alignment value \"", string, 
		"\": should be left, right, or center.", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetBoxFromObj --
 *
 *      Converts string into bounding box coordinates.  
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
GetBoxFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Box2d *boxPtr)
{
    int elc;
    Tcl_Obj **elv;
    int x1, y1, x2, y2;

    if (Tcl_ListObjGetElements(interp, objPtr, &elc, &elv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (elc != 4) {
	Tcl_AppendResult(interp,
		"wrong # of arguments: should be \"x1 y1 x2 y2\"",
		(char *)NULL);
	return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, elv[0], &x1) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, elv[1], &y1) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, elv[2], &x2) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, elv[3], &y2) != TCL_OK)) {
	return TCL_ERROR;
    }
    boxPtr->x1 = MIN(x1, x2);
    boxPtr->y1 = MIN(y1, y2);
    boxPtr->x2 = MAX(x2, x1);
    boxPtr->y2 = MAX(y2, y1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetCoordsFromObj --
 *
 *      Converts string into x and y coordinates.  Indicates that the
 *      menu is a popup and will be popped at the given x, y coordinate.
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
GetCoordsFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *xPtr, int *yPtr)
{
    int elc;
    Tcl_Obj **elv;
    int x, y;
    
    if (Tcl_ListObjGetElements(interp, objPtr, &elc, &elv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (elc != 2) {
	Tcl_AppendResult(interp, "wrong # of arguments: should be \"x y\"",
		(char *)NULL);
	return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, elv[0], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, elv[1], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    *xPtr = x;
    *yPtr = y;
    return TCL_OK;
}

static void
FreeUndoRecords(ComboEditor *editPtr)
{
    EditRecord *recPtr, *nextPtr;

    for (recPtr = editPtr->undoPtr; recPtr != NULL; recPtr = nextPtr) {
	nextPtr = recPtr->nextPtr;
	Blt_Free(recPtr);
    }
    editPtr->undoPtr = NULL;
}

static void
FreeRedoRecords(ComboEditor *editPtr)
{
    EditRecord *recPtr, *nextPtr;

    for (recPtr = editPtr->redoPtr; recPtr != NULL; recPtr = nextPtr) {
	nextPtr = recPtr->nextPtr;
	Blt_Free(recPtr);
    }
    editPtr->redoPtr = NULL;
}

static void
RecordEdit(ComboEditor *editPtr, RecordType type, CharIndex index,
           const char *text, int numBytes)
{
    EditRecord *recPtr;

    recPtr = Blt_AssertMalloc(sizeof(EditRecord) + numBytes);
    recPtr->type = type;
    recPtr->insertIndex = editPtr->insertIndex;
    recPtr->index = index;
    recPtr->numChars = Tcl_NumUtfChars(text, numBytes);
    recPtr->numBytes = numBytes;
    memcpy(recPtr->text, text, numBytes);
    recPtr->nextPtr = editPtr->undoPtr;
    editPtr->undoPtr = recPtr;
}

static void
CleanText(ComboEditor *editPtr)
{
    char *p, *q, *pend, *start;
    int length;
    Tcl_Obj *objPtr;
    
    /* Convert tabs and newlines to spaces, just to maintain the same
     * character index and byte offsets between the screen text and actual
     * text. */
    objPtr = Blt_DBuffer_StringObj(editPtr->dbuffer);
    p = Tcl_GetStringFromObj(objPtr, &length);
    q = start = (char *)Blt_DBuffer_Bytes(editPtr->dbuffer);
    for (pend = p + length; p < pend; p++, q++) {
        if ((*p == '\t') || (*p == ' ')) {
            while (((*p == ' ') || (*p == '\t')) && (p < pend)) {
                p++;                    /* Skip whitesapce. */
            }
            *q = ' ';
        } else {
            *q = *p;
        }
    }
    editPtr->numBytes = q - start;
    Blt_DBuffer_SetLength(editPtr->dbuffer, editPtr->numBytes);
    Tcl_DecrRefCount(objPtr);
} 

static void
BlinkCursor(ComboEditor *editPtr)
{
    int time;

    if (editPtr->flags & READONLY) {
        return;
    }
    if (editPtr->flags & ICURSOR_ON) {
	editPtr->flags &= ~ICURSOR_ON;
	time = editPtr->insertOffTime;
    } else {
	editPtr->flags |= ICURSOR_ON;
	time = editPtr->insertOnTime;
    }
    editPtr->insertTimerToken = Tcl_CreateTimerHandler(time, 
	BlinkCursorTimerProc, editPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * BlinkCursorTimerProc --
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
BlinkCursorTimerProc(ClientData clientData)
{
    ComboEditor *editPtr = clientData;

    if (editPtr->insertOffTime == 0) {
	return;
    }
    BlinkCursor(editPtr);
    EventuallyRedraw(editPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * EditorEventProc --
 *
 * 	This procedure is invoked by the Tk dispatcher for various events
 * 	on comboeditor widgets.
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
EditorEventProc(ClientData clientData, XEvent *eventPtr)
{
    ComboEditor *editPtr = clientData;

    if (eventPtr->type == Expose) {
	if (eventPtr->xexpose.count == 0) {
	    EventuallyRedraw(editPtr);
	}
    } else if (eventPtr->type == ConfigureNotify) {
	editPtr->flags |= SCROLL_PENDING;
	EventuallyRedraw(editPtr);
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
	if (eventPtr->xfocus.detail == NotifyInferior) {
	    return;
	}
	if (eventPtr->type == FocusIn) {
	    editPtr->flags |= FOCUS;
	} else {
	    editPtr->flags &= ~FOCUS;
	}
	if (editPtr->insertTimerToken != NULL) {
	    Tcl_DeleteTimerHandler(editPtr->insertTimerToken);
	    editPtr->insertTimerToken = NULL;
	}
	if ((editPtr->flags & (FOCUS|ICURSOR))==(FOCUS|ICURSOR)) {
	    if (editPtr->flags & ICURSOR_ON) {
		editPtr->flags &= ~ICURSOR_ON;
	    } else {
		editPtr->flags |= ICURSOR_ON;
	    }
	    if (editPtr->insertOffTime != 0) {
		BlinkCursor(editPtr);
	    }
	}
	EventuallyRedraw(editPtr);
    } else if (eventPtr->type == DestroyNotify) {
	if (editPtr->tkwin != NULL) {
	    editPtr->tkwin = NULL; 
	}
	if (editPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayProc, editPtr);
	}
	if (editPtr->flags & SELECT_PENDING) {
	    Tcl_CancelIdleCall(SelectCmdProc, editPtr);
	}
	if (editPtr->insertTimerToken != NULL) {
	    Tcl_DeleteTimerHandler(editPtr->insertTimerToken);
	}
	Tcl_EventuallyFree(editPtr, FreeEditorProc);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollbarEventProc --
 *
 *      This procedure is invoked by the Tk event handler when
 *      StructureNotify events occur in a scrollbar managed by the widget.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
ScrollbarEventProc(
    ClientData clientData,              /* Pointer to structure for widget
					 * referred to by eventPtr. */
    XEvent *eventPtr)                   /* Describes what just happened. */
{
    ComboEditor *editPtr = clientData;

    if (eventPtr->type == DestroyNotify) {
	if ((editPtr->yScrollbar != NULL) &&
	    (eventPtr->xany.window == Tk_WindowId(editPtr->yScrollbar))) {
	    editPtr->yScrollbar = NULL;
	} else if ((editPtr->xScrollbar != NULL) && 
		(eventPtr->xany.window == Tk_WindowId(editPtr->xScrollbar))) {
	    editPtr->xScrollbar = NULL;
	} 
    }
    editPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(editPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollbarCustodyProc --
 *
 *      This procedure is invoked when a scrollbar has been stolen by
 *      another geometry manager.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *     Arranges for the combomenu to have its layout re-arranged at the
 *      next idle point.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ScrollbarCustodyProc(
    ClientData clientData,              /* Information about the editor. */
    Tk_Window tkwin)                    /* Scrollbar stolen by another
					 * geometry manager. */
{
    ComboEditor *editPtr = (ComboEditor *)clientData;

    if (tkwin == editPtr->yScrollbar) {
	editPtr->yScrollbar = NULL;
	editPtr->yScrollbarWidth = 0;
    } else if (tkwin == editPtr->xScrollbar) {
	editPtr->xScrollbar = NULL;
	editPtr->xScrollbarHeight = 0;
    } else {
	return;         
    }
    Tk_UnmaintainGeometry(tkwin, editPtr->tkwin);
    editPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(editPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollbarGeometryProc --
 *
 *      This procedure is invoked by Tk_GeometryRequest for scrollbars
 *      managed by the combomenu.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the combomenu to have its layout re-computed and
 *      re-arranged at the next idle point.
 *
 *-------------------------------------------------------------------------- 
 */
/* ARGSUSED */
static void
ScrollbarGeometryProc(
    ClientData clientData,              /* ComboEditor widget record.  */
    Tk_Window tkwin)                    /* Scrollbar whose geometry has
					 * changed. */
{
    ComboEditor *editPtr = (ComboEditor *)clientData;

    editPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(editPtr);
}


static void
UnmanageScrollbar(ComboEditor *editPtr, Tk_Window scrollbar)
{
    Tk_DeleteEventHandler(scrollbar, StructureNotifyMask,
        ScrollbarEventProc, editPtr);
    Tk_ManageGeometry(scrollbar, (Tk_GeomMgr *)NULL, editPtr);
    if (Tk_IsMapped(scrollbar)) {
        Tk_UnmapWindow(scrollbar);
    }
}

static void
ManageScrollbar(ComboEditor *editPtr, Tk_Window scrollbar)
{
    Tk_CreateEventHandler(scrollbar, StructureNotifyMask, 
        ScrollbarEventProc, editPtr);
    Tk_ManageGeometry(scrollbar, &editorMgrInfo, editPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * InstallScrollbar --
 *
 *      Installs the scrollbar.
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
    Tcl_Interp *interp,                 /* Interpreter to report errors*/
    ComboEditor *editPtr,                /* Text editor to install
                                         * scrollbar. */
    Tcl_Obj *objPtr,                    /* Scrollbar name. */
    Tk_Window *tkwinPtr)                /* Returns tkwin of scrollbar.  */
{
    Tk_Window tkwin;
    const char *string;
    
    if (objPtr == NULL) {
	*tkwinPtr = NULL;
	return;
    }
    string = Tcl_GetString(objPtr);
    tkwin = Tk_NameToWindow(interp, string, editPtr->tkwin);
    if (tkwin == NULL) {
	Tcl_BackgroundError(interp);
	return;
    }
    if (Tk_Parent(tkwin) != editPtr->tkwin) {
	Tcl_AppendResult(interp, "scrollbar \"", Tk_PathName(tkwin), 
			 "\" must be a child of comboeditor.", (char *)NULL);
	Tcl_BackgroundError(interp);
	return;
    }
    if (tkwin != NULL) {
        ManageScrollbar(editPtr, tkwin);
    }
    *tkwinPtr = tkwin;
    return;
}

static void
InstallXScrollbar(ClientData clientData)
{
    ComboEditor *editPtr = clientData;

    editPtr->flags &= ~INSTALL_XSCROLLBAR;
    InstallScrollbar(editPtr->interp, editPtr, editPtr->xScrollbarObjPtr,
		     &editPtr->xScrollbar);
}

static void
InstallYScrollbar(ClientData clientData)
{
    ComboEditor *editPtr = clientData;

    editPtr->flags &= ~INSTALL_YSCROLLBAR;
    InstallScrollbar(editPtr->interp, editPtr, editPtr->yScrollbarObjPtr,
		     &editPtr->yScrollbar);
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
    ComboEditor *editPtr = clientData;

    if ((editPtr->selFirst >= 0) && (editPtr->flags & EXPORT_SELECTION)) {
	editPtr->selFirst = editPtr->selLast = -1;
	EventuallyRedraw(editPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectText --
 *
 *	Modify the selection by moving its un-anchored end.  This could make
 *	the selection either larger or smaller.
 *
 *	  1) If index is before the anchor point, sets the selection to the
 *	     characters from index up to but not including the anchor point.
 *	  2) If index is the same as the anchor point, does nothing.
 *	  3) If index is after the anchor point, set the selection to the
 *	     characters from the anchor point up to but not including index.  
 *	     The anchor point is determined by the most recent select from 
 *	     or select adjust command in this widget.
 *	  4) If the selection isn't in this widget then a new selection is
 *	     created using the most recent anchor point specified for the 
 *	     widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is possibly redrawn with the new selection.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectText(ComboEditor *editPtr, CharIndex index)
{
    CharIndex first, last;

    /*
     * Grab the selection if we don't own it already.
     */
    if ((editPtr->flags & EXPORT_SELECTION) && (editPtr->selFirst == -1)) {
	Tk_OwnSelection(editPtr->tkwin, XA_PRIMARY, LostSelectionProc, editPtr);
    }
    /*  If the anchor hasn't been set yet, assume the beginning of the text*/
    if (editPtr->selAnchor < 0) {
	editPtr->selAnchor = 0;
    }
    if (editPtr->selAnchor <= index) {
	first = editPtr->selAnchor;
	last = index;
    } else {
	first = index;
	last = editPtr->selAnchor;
    }
    if (((editPtr->selFirst != first) || (editPtr->selLast != last)) && 
	(first != last)) {
	editPtr->selFirst = first;
	editPtr->selLast = last;
	EventuallyRedraw(editPtr);
	if (editPtr->selectCmdObjPtr != NULL) {
	    EventuallyInvokeSelectCmd(editPtr);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectCmdProc --
 *
 *      Invoked at the next idle point whenever the current selection
 *      changes.  Executes some application-specific code in the
 *      -selectcommand option.  This provides a way for applications to
 *      handle selection changes.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      TCL code gets executed for some application-specific task.
 *
 *---------------------------------------------------------------------------
 */
static void
SelectCmdProc(ClientData clientData) 
{
    ComboEditor *editPtr = clientData;

    editPtr->flags &= ~SELECT_PENDING;
    if (editPtr->selectCmdObjPtr != NULL) {
	int result;

	Tcl_Preserve(editPtr);
	Tcl_IncrRefCount(editPtr->selectCmdObjPtr);
	result = Tcl_EvalObjEx(editPtr->interp, editPtr->selectCmdObjPtr,
                               TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(editPtr->selectCmdObjPtr);
	Tcl_Release(editPtr);
	if (result != TCL_OK) {
	    Tcl_BackgroundError(editPtr->interp);
	}
    }
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
    ClientData clientData,		/* Information about the widget. */
    int offset,				/* Offset within the selection of
					 * the first character to be
					 * returned. */
    char *buffer,			/* Location in which to place
					 * selection. */
    int maxBytes)			/* Maximum # of bytes to place in
					 * the buffer, not including
					 * terminating NULL character. */
{
    ComboEditor *editPtr = clientData;
    int size;

    size = 0;
    if (editPtr->selFirst >= 0) {
	ByteOffset first, last;
        const char *string;

        string = Blt_DBuffer_String(editPtr->dbuffer);
	first = CharIndexToByteOffset(string, editPtr->selFirst);
	last  = CharIndexToByteOffset(string, editPtr->selLast);
	size = last - first - offset;
	assert(size >= 0);
	if (size > maxBytes) {
	    size = maxBytes;
	}
	memcpy(buffer, string + first + offset, size);
	buffer[size] = '\0';
    }
    return size;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTextProc --
 *
 *	Save the text and add the item to the text hashtable.
 *
 * Results:
 *	A standard TCL result. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTextProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    ComboEditor *editPtr = (ComboEditor *)(widgRec);

    SetTextFromObj(editPtr, objPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextToObjProc --
 *
 *	Returns the current text of the entry.
 *
 * Results:
 *	The text is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TextToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              char *widgRec, int offset, int flags)	
{
    ComboEditor *editPtr = (ComboEditor *)(widgRec);

    return Blt_DBuffer_StringObj(editPtr->dbuffer);
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
    ComboEditor *editPtr = (ComboEditor *)record;
    Tk_Window tkwin;
    const char *string;

    tkwin = NULL;
    string = Tcl_GetString(objPtr);
    if (string[0] == '\0') {
	tkwin = NULL;
    } else {
	tkwin = Tk_NameToWindow(interp, string, editPtr->tkwin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
    }
    editPtr->post.flags = POST_WINDOW;
    editPtr->post.tkwin = tkwin;
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
    ComboEditor *editPtr = (ComboEditor *)record;
    int align;
    
    if (GetAlignFromObj(interp, objPtr, &align) != TCL_OK) {
	return TCL_ERROR;
    }
    editPtr->post.align = align;
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
 *      Otherwise, TCL_ERROR is returned and an error message is left in
 *      interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PostPopupSwitchProc(ClientData clientData, Tcl_Interp *interp,
		    const char *switchName, Tcl_Obj *objPtr, char *record,
		    int offset, int flags)
{
    ComboEditor *editPtr = (ComboEditor *)record;
    int x, y;
    
    if (GetCoordsFromObj(interp, objPtr, &x, &y) != TCL_OK) {
	return TCL_ERROR;
    }
    editPtr->post.x1 = editPtr->post.x2 = x;
    editPtr->post.y1 = editPtr->post.y2 = y;
    editPtr->post.flags = POST_POPUP;
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
PostBoxSwitchProc(ClientData clientData, Tcl_Interp *interp,
                  const char *switchName, Tcl_Obj *objPtr, char *record,
                  int offset, int flags)
{
    ComboEditor *editPtr = (ComboEditor *)record;
    Box2d box;
    
    if (GetBoxFromObj(interp, objPtr, &box) != TCL_OK) {
	return TCL_ERROR;
    }
    editPtr->post.x1 = box.x1;
    editPtr->post.y1 = box.y1;
    editPtr->post.x2 = box.x2;
    editPtr->post.y2 = box.y2;
    editPtr->post.flags = POST_REGION;
    return TCL_OK;
}

static TextLine *
FindLineFromIndex(ComboEditor *editPtr, CharIndex index)
{
    int low, high;

    low = 0;
    high = editPtr->numLines - 1;
    while (low <= high) {
	TextLine *linePtr;
	int median;
	
	median = (low + high) >> 1;
	linePtr = editPtr->lines + median;
	if (index < linePtr->char1) {
	    high = median - 1;
	} else if (index > linePtr->char2) {
	    low = median + 1;
	} else {
	    return linePtr;
	}
    }
    return NULL;                        /* Can't find value. */
}

static TextLine *
FindLineFromCoord(ComboEditor *editPtr, int worldY)
{
    int low, high;

    low = 0;
    high = editPtr->numLines - 1;
    while (low <= high) {
	TextLine *linePtr;
	int median;
	
	median = (low + high) >> 1;
	linePtr = editPtr->lines + median;
	if (worldY < linePtr->worldY) {
	    high = median - 1;
	} else if (worldY >= (linePtr->worldY + linePtr->height)) {
	    low = median + 1;
	} else {
	    return linePtr;
	}
    }
    return NULL;                        /* Can't find value. */
}

static CharIndex
CoordinatesToIndex(ComboEditor *editPtr, int x, int y)
{
    TextLine *linePtr;
    int worldX, worldY;
    int numBytes, numPixels;
    
    if (editPtr->flags & GEOMETRY) {
        ComputeGeometry(editPtr);
    }
    /* Convert the screen coordinates into world coordinates */
    worldX = WORLDX(editPtr, x);
    worldY = WORLDY(editPtr, y);
    linePtr = FindLineFromCoord(editPtr, worldY);
    if (linePtr == NULL) {
        return -1;                      /* Outside of range. */
    }
    if (worldX < linePtr->worldX) {
        return linePtr->char1;
    }
    if (worldX > (linePtr->worldX + linePtr->width)) {
        return linePtr->char2;
    }
    /* Find the start of the line in terms of bytes offsets. */
    numBytes = Blt_Font_Measure(editPtr->font, linePtr->text, linePtr->numBytes,
        worldX - linePtr->worldX, 0, &numPixels);
    if (numBytes >= linePtr->numBytes) {
        return linePtr->char2;
    }
    return linePtr->char1 + Tcl_NumUtfChars(linePtr->text, numBytes);
}

static int
IndexToCoordinates(ComboEditor *editPtr, CharIndex index, int *xPtr, int *yPtr)
{
    int numBytes, numChars, numPixels;
    TextLine *linePtr;
    
    if (editPtr->flags & GEOMETRY) {
        ComputeGeometry(editPtr);
    }
    linePtr = FindLineFromIndex(editPtr, index);
    if (linePtr == NULL) {
        return TCL_OK;                  /* Outside of range. */
    }
    /* Find the start of the line. */
    /* Find the start and end of the line as text offsets. */
    numChars = index - linePtr->char1;
    numBytes = CharIndexToByteOffset(linePtr->text, numChars);
    Blt_Font_Measure(editPtr->font, linePtr->text, numBytes,
        linePtr->worldX + editPtr->xOffset, TEXT_FLAGS, &numPixels);

    *xPtr = linePtr->worldX + numPixels;
    *yPtr = linePtr->worldY;
    return TCL_OK;
}

static void
InsertText(ComboEditor *editPtr, const char *text, int numBytes,
           CharIndex index)
{
    int numChars;
    int result;
    const char *string;
    ByteOffset offset;
    
    if (editPtr->flags & GEOMETRY) {
        ComputeGeometry(editPtr);
    }
    string = Blt_DBuffer_String(editPtr->dbuffer);
    offset = CharIndexToByteOffset(string, index);
    if (offset == Blt_DBuffer_Length(editPtr->dbuffer)) { /* Append */
        result = Blt_DBuffer_AppendData(editPtr->dbuffer,
                (const unsigned char *)text, numBytes);
    } else if (offset == 0) {           /* Prepend */
        result = Blt_DBuffer_InsertData(editPtr->dbuffer,
                (const unsigned char *)text, numBytes, 0);
    } else {                            /* Insert into existing. */
        result = Blt_DBuffer_InsertData(editPtr->dbuffer,
                (const unsigned char *)text, numBytes, offset);
    }
    if (!result) {
        return;
    }
    /* 
     * All indices from the start of the insertion to the end of the string
     * need to be updated.  Simply move the indices down by the number of
     * characters added.
     */
    numChars = Tcl_NumUtfChars(text, numBytes);
    if (editPtr->selFirst >= index) {
	editPtr->selFirst += numChars;
    }
    if (editPtr->selLast > index) {
	editPtr->selLast += numChars;
    }
    if ((editPtr->selAnchor > index) || (editPtr->selFirst >= index)) {
	editPtr->selAnchor += numChars;
    }
    if (editPtr->insertIndex >= index) {
	editPtr->insertIndex += numChars;
    }        
    editPtr->flags |= LAYOUT_PENDING | GEOMETRY;
    editPtr->numChars += numChars;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteText --
 *
 *	Deletes the characters designate by first and last. Last is
 *      included in the deletion.
 *
 *      Since deleting characters compacts the character array, we need to
 *      update the various character indices according.  It depends where
 *      the index occurs in relation to range of deleted characters:
 *
 *	 before		Ignore.
 *      within		Move the index back to the start of the deletion.
 *	 after		Subtract off the deleted number of characters,
 *			since the array has been compressed by that
 *			many characters.
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.  The
 *	pointer to the node is returned via itemPtrPtr.  Otherwise, TCL_ERROR
 *	is returned and an error message is left in interpreter's result
 *	field.
 *
 *---------------------------------------------------------------------------
 */
static int
DeleteText(ComboEditor *editPtr, CharIndex firstIndex, CharIndex lastIndex)
{
    int numChars, numBytes;
    ByteOffset first, last;
    const char *string;

    if (editPtr->flags & GEOMETRY) {
        ComputeGeometry(editPtr);
    }
    string = Blt_DBuffer_String(editPtr->dbuffer);
    first = CharIndexToByteOffset(string, firstIndex);
    last  = CharIndexToByteOffset(string, lastIndex);
    numBytes = last - first;
    if (!Blt_DBuffer_DeleteData(editPtr->dbuffer, first, numBytes)) {
        return FALSE;
    }
    numChars = lastIndex - firstIndex;
    if (editPtr->selFirst >= firstIndex) {
        if (editPtr->selFirst < lastIndex) {
	    editPtr->selFirst = firstIndex;/* Selection starts inside of
                                            * deleted text. */
        } else {
	    editPtr->selFirst -= numChars; /* Selection starts after
                                            * deleted text. */
	}
    }
    if (editPtr->selLast > firstIndex) {
	if (editPtr->selLast <= lastIndex) {
	    editPtr->selLast = firstIndex;
	} else {
	    editPtr->selLast -= numChars;
	}
    }
    if (editPtr->selLast <= editPtr->selFirst) {
	editPtr->selFirst = editPtr->selLast = -1; /* Cut away the entire
                                                    * selection. */ 
    }
    if (editPtr->selAnchor >= firstIndex) {
	if (editPtr->selAnchor >= lastIndex) {
	    editPtr->selAnchor -= numChars;
	} else {
	    editPtr->selAnchor = firstIndex;
	}
    }
    if (editPtr->insertIndex >= firstIndex) {
	if (editPtr->insertIndex >= lastIndex) {
	    editPtr->insertIndex -= numChars;
	} else {
	    editPtr->insertIndex = firstIndex;
	}
    }
    editPtr->numChars -= numChars;
    editPtr->flags |= LAYOUT_PENDING | GEOMETRY;
    EventuallyRedraw(editPtr);
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetWordStart --
 *
 *      Returns the index of the beginning of the word.  Index must
 *      point to a word.
 *---------------------------------------------------------------------------
 */
static CharIndex 
GetWordStart(ComboEditor *editPtr, CharIndex index)
{
    CharIndex first;
    TextLine *linePtr;
    const char *cp, *string;

    linePtr = FindLineFromIndex(editPtr, index);
    if (linePtr == NULL) {
        return -1;
    }
    string = Blt_DBuffer_String(editPtr->dbuffer);
    cp = Tcl_UtfAtIndex(string, index);
    for (first = index; first >= linePtr->char1; first--) {
        Tcl_UniChar ch;
        
        Tcl_UtfToUniChar(cp, &ch);
        if (!Tcl_UniCharIsWordChar(ch)) {
            break;
        }
        cp = Tcl_UtfPrev(cp, string);
    }
    if (first != index) {
        first++;
    }
    return first;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetWordEnd --
 *
 *      Returns the index of the end of the word.  Index must
 *      point to a word.
 *---------------------------------------------------------------------------
 */
static CharIndex 
GetWordEnd(ComboEditor *editPtr, CharIndex index)
{
    CharIndex last;
    TextLine *linePtr;
    const char *cp, *string;

    linePtr = FindLineFromIndex(editPtr, index);
    if (linePtr == NULL) {
        return -1;
    }
    string = Blt_DBuffer_String(editPtr->dbuffer);
    cp = Tcl_UtfAtIndex(string, index);
    for (last = index; last <= linePtr->char2; last++) {
        Tcl_UniChar ch;
        
        cp += Tcl_UtfToUniChar(cp, &ch);
        if (!Tcl_UniCharIsWordChar(ch)) {
            break;
        }
    }
    if (last == index) {
        last++;
    }
    return last;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetSpaceStart --
 *
 *---------------------------------------------------------------------------
 */
static CharIndex 
GetSpaceStart(ComboEditor *editPtr, CharIndex index)
{
    CharIndex first;
    TextLine *linePtr;
    const char *cp, *string;

    linePtr = FindLineFromIndex(editPtr, index);
    if (linePtr == NULL) {
        return -1;
    }
    string = Blt_DBuffer_String(editPtr->dbuffer);
    cp = Tcl_UtfAtIndex(string, index);
    for (first = index; first >= linePtr->char1; first--) {
        Tcl_UniChar ch;
        
        Tcl_UtfToUniChar(cp, &ch);
        if (!Tcl_UniCharIsSpace(ch)) {
            break;
        }
        cp = Tcl_UtfPrev(cp, string);
    }
    if (first != index) {
        first++;
    }
    return first;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetSpaceEnd --
 *
 *---------------------------------------------------------------------------
 */
static CharIndex 
GetSpaceEnd(ComboEditor *editPtr, CharIndex index)
{
    CharIndex last;
    TextLine *linePtr;
    const char *cp, *string;

    linePtr = FindLineFromIndex(editPtr, index);
    if (linePtr == NULL) {
        return -1;
    }
    string = Blt_DBuffer_String(editPtr->dbuffer);
    cp = Tcl_UtfAtIndex(string, index);
    for (last = index; last <= linePtr->char2; last++) {
        Tcl_UniChar ch;
        
        cp += Tcl_UtfToUniChar(cp, &ch);
        if (!Tcl_UniCharIsSpace(ch)) {
            break;
        }
    }
    return last;
}
            
/*
 *---------------------------------------------------------------------------
 *
 * GetLineStart --
 *
 *      Returns the index of the beginning of the line. 
 *
 *---------------------------------------------------------------------------
 */
static CharIndex 
GetLineStart(ComboEditor *editPtr, CharIndex index)
{
    TextLine *linePtr;
    
    linePtr = FindLineFromIndex(editPtr, index);
    if (linePtr == NULL) {
        return -1;
    }
    return linePtr->char1;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetLineEnd --
 *
 *      Returns the index of the end of the line. 
 *
 *---------------------------------------------------------------------------
 */
static CharIndex 
GetLineEnd(ComboEditor *editPtr, CharIndex index)
{
    TextLine *linePtr;
    
    linePtr = FindLineFromIndex(editPtr, index);
    if (linePtr == NULL) {
        return -1;
    }
    return linePtr->char2;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetIndexFromObj --
 *
 *	Converts a string representing a item index into an item pointer.
 *	The index may be in one of the following forms:
 *
 *	 number		Specifies the character as a numerical index, 
 *			where 0 corresponds to the first character in 
 *			the string.
 *	 "anchor"	Indicates the anchor point for the selection, 
 *			which is set with the select from and select 
 *			adjust widget commands.
 *	 "end"		Indicates the character just after the last one  
 *			in the entry's string.  This is equivalent to 
 *			specifying a numerical index equal to the length 
 *			of the entry's string.
 *	 "insert"       Indicates the character adjacent to and immediately 
 *			following the insertion cursor.
 *	 "sel.first"    Indicates the first character in the selection.  
 *			It is an error to use this form if the selection 
 *			isn't in the entry window.
 *       "sel.last"	Indicates the character just  after the last one 
 *			in the selection.  It is an error to use this form 
 *			if  the  selection isn't in the entry window.
 *      "whitespace"
 *      "word.start"    
 *      "word.end"
 *      "line.start"
 "      "line.end"
 *       @x,y           Coordinates in the editor's window
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.  The
 *	pointer to the node is returned via itemPtrPtr.  Otherwise, TCL_ERROR
 *	is returned and an error message is left in interpreter's result
 *	field.
 *
 *---------------------------------------------------------------------------
 */
static int
GetIndexFromObj(Tcl_Interp *interp, ComboEditor *editPtr, Tcl_Obj *objPtr,
                CharIndex *indexPtr)
{
    char *string;
    char c;
    CharIndex index;

    if (Tcl_GetIntFromObj((Tcl_Interp *)NULL, objPtr, &index) == TCL_OK) {
	/* Convert the character index into a byte offset. */
	if (Blt_DBuffer_Length(editPtr->dbuffer) == 0) {
	    *indexPtr = 0;
	    return TCL_OK;
	}
	if (index < 0) {
	    *indexPtr = 0;
	} else {
	    *indexPtr = index;
	}
	return TCL_OK;
    }
    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'a') && (strcmp(string, "anchor") == 0)) {
	if (editPtr->selAnchor < 0) {
	    Tcl_AppendResult(interp, "bad index \"", string, 
			     "\": no selection present.", (char *)NULL);
	    return TCL_ERROR;
	}
	*indexPtr = editPtr->selAnchor;
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
	*indexPtr = editPtr->numChars;
    } else if ((c == 'i') && (strcmp(string, "insert") == 0)) {
	*indexPtr = editPtr->insertIndex;
    } else if ((c == 'd') && (strcmp(string, "down") == 0)) {
        TextLine *linePtr;
        
        index = editPtr->insertIndex;
        linePtr = FindLineFromIndex(editPtr, editPtr->insertIndex);
        if ((linePtr != NULL) &&
            ((linePtr - editPtr->lines) < (editPtr->numLines - 1))) {
            int numChars;
            
            numChars = editPtr->insertIndex - linePtr->char1;
            linePtr++;
            index = linePtr->char1 + numChars;
            if (index >= linePtr->char2) {
                index = linePtr->char2;
            }
        }
        *indexPtr = index;
    } else if ((c == 'u') && (strcmp(string, "up") == 0)) {
        TextLine *linePtr;
        
        index = editPtr->insertIndex;
        linePtr = FindLineFromIndex(editPtr, editPtr->insertIndex);
        if ((linePtr != NULL) && ((linePtr - editPtr->lines) > 0)) {
            int numChars;
            
            numChars = editPtr->insertIndex - linePtr->char1;
            linePtr--;
            index = linePtr->char1 + numChars;
            if (index >= linePtr->char2) {
                index = linePtr->char2;
            }
        }
        *indexPtr = index;
    } else if ((c == 'n') && (strcmp(string, "next") == 0)) {
	index = editPtr->insertIndex;
	if (index < editPtr->numChars) {
	    index++;
	}
	*indexPtr = index;
    } else if ((c == 'p') && (strcmp(string, "previous") == 0)) {
	index = editPtr->insertIndex;
	if (index > 0) {
	    index--;
	}
	*indexPtr = index;
    } else if ((c == 'l') && (strcmp(string, "line.start") == 0)) {
        index = editPtr->insertIndex;
        if (editPtr->insertIndex != -1) {
            index = GetLineStart(editPtr, index);
        }
	*indexPtr = index;
    } else if ((c == 'l') && (strcmp(string, "line.end") == 0)) {
        index = editPtr->insertIndex;
        if (editPtr->insertIndex != -1) {
            index = GetLineEnd(editPtr, index);
        }
	*indexPtr = index;
    } else if ((c == 's') && (strcmp(string, "space.start") == 0)) {
        index = editPtr->insertIndex;
        if (editPtr->insertIndex != -1) {
            index = GetSpaceStart(editPtr, index);
        }
	*indexPtr = index;
    } else if ((c == 's') && (strcmp(string, "space.end") == 0)) {
        index = editPtr->insertIndex;
        if (editPtr->insertIndex != -1) {
            index = GetSpaceEnd(editPtr, index);
        }
	*indexPtr = index;
    } else if ((c == 'w') && (strcmp(string, "word.start") == 0)) {
        index = editPtr->insertIndex;
        if (editPtr->insertIndex != -1) {
            index = GetWordStart(editPtr, index);
        }
	*indexPtr = index;
    } else if ((c == 'w') && (strcmp(string, "word.end") == 0)) {
        index = editPtr->insertIndex;
        if (editPtr->insertIndex != -1) {
            index = GetWordEnd(editPtr, index);
        }
	*indexPtr = index;
    } else if ((c == 's') && (strcmp(string, "sel.first") == 0)) {
	*indexPtr = (int)editPtr->selFirst;
    } else if ((c == 's') && (strcmp(string, "sel.last") == 0)) {
	*indexPtr = (int)editPtr->selLast;
    } else if (c == '@') {
	int x, y;
        
	if (Blt_GetXY(interp, editPtr->tkwin, string, &x, &y) != TCL_OK) {
	    return TCL_ERROR;
	}
        *indexPtr = CoordinatesToIndex(editPtr, x, y);
    } else {
	Tcl_AppendResult(interp, "unknown index \"", string, "\"",(char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static void
ConfigureScrollbarsProc(ClientData clientData)
{
    ComboEditor *editPtr = clientData;
    Tcl_Interp *interp;

    interp = editPtr->interp;
    /* 
     * Execute the initialization procedure on this widget.
     */
    editPtr->flags &= ~UPDATE_PENDING;
    if (Tcl_VarEval(interp, "::blt::ComboEditor::ConfigureScrollbars ", 
	Tk_PathName(editPtr->tkwin), (char *)NULL) != TCL_OK) {
	Tcl_BackgroundError(interp);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeGeometry --
 *
 *	Get the extents of a possibly multiple-lined text string.
 *
 * Results:
 *	Returns via *widthPtr* and *heightPtr* the dimensions of the text
 *	string.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeGeometry(ComboEditor *editPtr)
{
    TextLine *linePtr, *endPtr;
    Blt_FontMetrics fm;
    int count;			/* Count # of characters on each line */
    int lineHeight;
    int maxHeight, maxWidth;
    int numLines;
    const char *p, *tendPtr, *start;
    const char *text;
    int lastChar;
    
#ifdef notdef
    fprintf(stderr, "Calling ComputeGeometry(%s) w=%d h=%d\n", 
	    Tk_PathName(editPtr->tkwin), Tk_Width(editPtr->tkwin),
	    Tk_Height(editPtr->tkwin));
#endif
    editPtr->flags &= ~GEOMETRY;
    text = Blt_DBuffer_String(editPtr->dbuffer);
    tendPtr = text + Blt_DBuffer_Length(editPtr->dbuffer);
    numLines = 0;
    for (p = text; p < tendPtr; p++) {
	if (*p == '\n') {
	    numLines++;
	}
    }
    if ((p != text) && (*(p - 1) != '\n')) {
	numLines++;                     /* Add a line if the last character
                                         * isn't a newline. */
    }
    if (editPtr->lines != NULL) {
        Blt_Free(editPtr->lines);
    }
    editPtr->lines = Blt_Calloc(numLines, sizeof(TextLine));
    editPtr->numLines = numLines;

    numLines = count = 0;
    maxWidth = 0;

    maxHeight = 0;
    Blt_Font_GetMetrics(editPtr->font, &fm);
    lineHeight = fm.linespace + editPtr->leader;

    linePtr = editPtr->lines;
    lastChar = 0;
    for (p = start = text; p < tendPtr; p++) {
	if (*p == '\n') {
            int numPixels;
            
	    if (count > 0) {
		numPixels = Blt_TextWidth(editPtr->font, start, count);
		if (numPixels > maxWidth) {
		    maxWidth = numPixels;
		}
	    } else {
		numPixels = 0;
	    }
	    linePtr->text = start;
	    linePtr->width = numPixels;
	    linePtr->numBytes = count;
            linePtr->char1 = lastChar;
            linePtr->char2 = linePtr->char1 + Tcl_NumUtfChars(start, count);
            linePtr->worldY = maxHeight;
            linePtr->height = lineHeight;
	    maxHeight += lineHeight;
            lastChar = linePtr->char2 + 1;
	    linePtr++;
	    numLines++;
	    start = p + 1;	/* Start the text on the next line */
	    count = 0;		/* Reset to indicate the start of a new
				 * line */
	} else {
            count++;
        }
    }
    if (numLines < editPtr->numLines) {
        int numPixels;
        
	numPixels = Blt_TextWidth(editPtr->font, start, count);
	if (numPixels > maxWidth) {
	    maxWidth = numPixels;
	}
	linePtr->text = start;
        linePtr->numBytes = count;
	linePtr->width = numPixels;
	linePtr->worldY = maxHeight;
        linePtr->char1 = lastChar;
        linePtr->char2 = linePtr->char1 + Tcl_NumUtfChars(start, count);
        linePtr->height = lineHeight;
	maxHeight += lineHeight;
	numLines++;
    }
    for (linePtr = editPtr->lines, endPtr = linePtr + editPtr->numLines;
         linePtr < endPtr; linePtr++) {
	switch (editPtr->justify) {
	default:
	case TK_JUSTIFY_LEFT:
	    /* No offset for left justified text strings */
	    linePtr->worldX = 0;
	    break;
	case TK_JUSTIFY_RIGHT:
	    linePtr->worldX = (maxWidth - linePtr->width);
	    break;
	case TK_JUSTIFY_CENTER:
	    linePtr->worldX = (maxWidth - linePtr->width) / 2;
	    break;
	}
    }
    editPtr->worldWidth = maxWidth + 4;
    editPtr->worldHeight = maxHeight - editPtr->leader;
    if (editPtr->yOffset > (editPtr->worldHeight - VPORTHEIGHT(editPtr))) {
        editPtr->yOffset = editPtr->worldHeight - VPORTHEIGHT(editPtr);
        if (editPtr->yOffset < 0) {
            editPtr->yOffset = 0;
        }
    }
    if (editPtr->xOffset > (editPtr->worldWidth - VPORTWIDTH(editPtr))) {
        editPtr->xOffset = editPtr->worldWidth - VPORTWIDTH(editPtr);
        if (editPtr->xOffset < 0) {
            editPtr->xOffset = 0;
        }
    }
}

static int
ConfigureEditor(Tcl_Interp *interp, ComboEditor *editPtr, int objc,
                Tcl_Obj *const *objv, int flags)
{
    unsigned int gcMask;
    XGCValues gcValues;
    GC newGC;
    int updateNeeded;
    Blt_FontMetrics fontMetrics;
    
    if (Blt_ConfigureWidgetFromObj(interp, editPtr->tkwin, configSpecs, objc, 
		objv, (char *)editPtr, editPtr->mask | flags) != TCL_OK) {
	return TCL_ERROR;
    }
    if (editPtr->flags & READONLY) {
	editPtr->flags &= ~ICURSOR;
    } else {
	editPtr->flags |= ICURSOR;
    }
    /* Text GC. */
    gcMask = GCForeground | GCFont;
    gcValues.foreground = editPtr->textFg->pixel;
    gcValues.font = Blt_Font_Id(editPtr->font);
    newGC = Tk_GetGC(editPtr->tkwin, gcMask, &gcValues);
    if (editPtr->textGC != NULL) {
	Tk_FreeGC(editPtr->display, editPtr->textGC);
    }
    editPtr->textGC = newGC;

    /* Selection foreground. */
    gcMask = GCForeground | GCFont;
    gcValues.foreground = editPtr->selectFg->pixel;
    gcValues.font = Blt_Font_Id(editPtr->font);
    newGC = Tk_GetGC(editPtr->tkwin, gcMask, &gcValues);
    if (editPtr->selectGC != NULL) {
	Tk_FreeGC(editPtr->display, editPtr->selectGC);
    }
    editPtr->selectGC = newGC;

    /* Insert cursor. */
    gcMask = GCForeground;
    gcValues.foreground = editPtr->insertColor->pixel;
    newGC = Tk_GetGC(editPtr->tkwin, gcMask, &gcValues);
    if (editPtr->insertGC != NULL) {
	Tk_FreeGC(editPtr->display, editPtr->insertGC);
    }
    editPtr->insertGC = newGC;
    ComputeGeometry(editPtr);

    Blt_Font_GetMetrics(editPtr->font, &fontMetrics);
    editPtr->cursorHeight = fontMetrics.linespace;
    editPtr->cursorWidth = 3;

    updateNeeded = FALSE;

    /* Install the embedded scrollbars as needed.  We defer installing the
     * scrollbars so the scrollbar widgets don't have to exist when they
     * are specified by the -xscrollbar and -yscrollbar options
     * respectively. The down-side is that errors found in the scrollbar
     * name will be backgrounded. */

    if (Blt_ConfigModified(configSpecs, "-xscrollbar", (char *)NULL)) {
	if (editPtr->xScrollbar != NULL) {
	    UnmanageScrollbar(editPtr, editPtr->xScrollbar);
	    editPtr->xScrollbar = NULL;
	}
	if ((editPtr->flags & INSTALL_XSCROLLBAR) == 0) {
	    Tcl_DoWhenIdle(InstallXScrollbar, editPtr);
	    editPtr->flags |= INSTALL_XSCROLLBAR;
	}           
	updateNeeded = TRUE;
    }
    if (Blt_ConfigModified(configSpecs, "-yscrollbar", (char *)NULL)) {
	if (editPtr->yScrollbar != NULL) {
	    UnmanageScrollbar(editPtr, editPtr->yScrollbar);
	    editPtr->yScrollbar = NULL;
	}
	if ((editPtr->flags & INSTALL_YSCROLLBAR) == 0) {
	    Tcl_DoWhenIdle(InstallYScrollbar, editPtr);
	    editPtr->flags |= INSTALL_YSCROLLBAR;
	}           
	updateNeeded = TRUE;
    }
    if (updateNeeded) {
	if ((editPtr->flags & UPDATE_PENDING) == 0) {
	    Tcl_DoWhenIdle(ConfigureScrollbarsProc, editPtr);
	    editPtr->flags |= UPDATE_PENDING;
	}           
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * FreeEditorProc --
 *
 * 	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 * 	clean up the internal structure of the widget at a safe time (when
 * 	no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Everything associated with the widget is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeEditorProc(DestroyData dataPtr)	/* Pointer to the widget record. */
{
    ComboEditor *editPtr = (ComboEditor *)dataPtr;

    Blt_FreeOptions(configSpecs, (char *)editPtr, editPtr->display, 
	editPtr->mask);
    if (editPtr->textGC != NULL) {
	Tk_FreeGC(editPtr->display, editPtr->textGC);
    }
    if (editPtr->dbuffer != NULL) {
        Blt_DBuffer_Destroy(editPtr->dbuffer);
    }
    FreeUndoRecords(editPtr);
    FreeRedoRecords(editPtr);
    if (editPtr->selectGC != NULL) {
	Tk_FreeGC(editPtr->display, editPtr->selectGC);
    }
    if (editPtr->insertGC != NULL) {
	Tk_FreeGC(editPtr->display, editPtr->insertGC);
    }
    if (editPtr->insertTimerToken != NULL) {
	Tcl_DeleteTimerHandler(editPtr->insertTimerToken);
    }
    if (editPtr->tkwin != NULL) {
	Tk_DeleteSelHandler(editPtr->tkwin, XA_PRIMARY, XA_STRING);
	Tk_DeleteEventHandler(editPtr->tkwin, EVENT_MASK, 
		EditorEventProc, editPtr);
    }
    if (editPtr->insertTimerToken != NULL) {
	Tcl_DeleteTimerHandler(editPtr->insertTimerToken);
    }
    if (editPtr->cmdToken != NULL) {
        Tcl_DeleteCommandFromToken(editPtr->interp, editPtr->cmdToken);
    }
    Blt_Free(editPtr);
}

static ComboEditor *
NewEditor(Tcl_Interp *interp, Tk_Window tkwin)
{
    ComboEditor *editPtr;

    Tk_SetClass(tkwin, "BltComboEditor"); 
    editPtr = Blt_AssertCalloc(1, sizeof(ComboEditor));
    editPtr->borderWidth = 1;
    editPtr->dbuffer = Blt_DBuffer_Create();
    editPtr->display = Tk_Display(tkwin);
    editPtr->flags |= ACTIVE | GEOMETRY | LAYOUT_PENDING;
    editPtr->insertOffTime = 300;
    editPtr->insertOnTime = 600;
    editPtr->interp = interp;
    editPtr->relief = TK_RELIEF_SOLID;
    editPtr->justify = TK_JUSTIFY_LEFT;
    editPtr->selAnchor = -1;
    editPtr->selBorderWidth = 1;
    editPtr->selFirst = editPtr->selLast = -1;
    editPtr->selRelief = TK_RELIEF_FLAT;
    editPtr->tkwin = tkwin;
    editPtr->xScrollUnits = editPtr->yScrollUnits = 6;
    Blt_ResetLimits(&editPtr->reqWidth);
    Blt_ResetLimits(&editPtr->reqHeight);
    Blt_SetWindowInstanceData(tkwin, editPtr);
    Tk_CreateSelHandler(tkwin, XA_PRIMARY, XA_STRING, SelectionProc, editPtr,
                        XA_STRING);
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask |
	FocusChangeMask, EditorEventProc, editPtr);
    Tcl_CreateObjCommand(interp, Tk_PathName(tkwin), EditorInstCmdProc,
                editPtr, NULL);
    if (ConfigureEditor(interp, editPtr, 0, (Tcl_Obj **)NULL, 0) != TCL_OK) {
	Tk_DestroyWindow(tkwin);
	return NULL;
    }
    editPtr->insertIndex = 0;
    return editPtr;
}

static int
WithdrawEditor(ComboEditor *editPtr)
{
    if (!Tk_IsMapped(editPtr->tkwin)) {
	return FALSE;                 /* This editor is already withdrawn. */
    }
    if (Tk_IsMapped(editPtr->tkwin)) {
	Tk_UnmapWindow(editPtr->tkwin);
    }
    return TRUE;
}


static inline int
GetWidth(ComboEditor *editPtr)
{
    int w;

    w = editPtr->width;
    if (w < 2) {
	w = Tk_Width(editPtr->tkwin);
    }
    if (w < 2) {
	w = Tk_ReqWidth(editPtr->tkwin);
    }
    return w;
}

static inline int
GetHeight(ComboEditor *editPtr)
{
    int h;

    h = editPtr->height;
    if (h < 2) {
	h = Tk_Height(editPtr->tkwin);
    }
    if (h < 2) {
	h = Tk_ReqHeight(editPtr->tkwin);
    }
    return h;
}

static void
FixEditorCoords(ComboEditor *editPtr, int *xPtr, int *yPtr)
{
    int x, y, w, h;
    int sw, sh;

    Blt_SizeOfScreen(editPtr->tkwin, &sw, &sh);
    x = *xPtr;
    y = *yPtr;
    w = GetWidth(editPtr);
    h = GetHeight(editPtr);

    if ((y + h) > sh) {
	y -= h;                         /* Shift the menu up by the height of
					 * the menu. */
	if (editPtr->flags & DROPDOWN) {
	    y -= editPtr->post.editorHeight;
					/* Add the height of the parent if
					 * this is a dropdown menu.  */
	}
	if (y < 0) {
	    y = 0;
	}
    }
    if ((x + w) > sw) {
	if (editPtr->flags & DROPDOWN) {
	    x = x + editPtr->post.editorWidth - w;
					/* Flip the menu anchor to the other
					 * end of the menu button/entry */
	} else {
	    x -= w;                     /* Shift the menu to the left by the
					 * width of the menu. */
	}
	if (x < 0) {
	    x = 0;
	}
    }
    *xPtr = x;
    *yPtr = y;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeLayout --
 *
 *	Get the extents of a possibly multiple-lined text string.
 *
 * Results:
 *	Returns via *widthPtr* and *heightPtr* the dimensions of the text
 *	string.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeLayout(ComboEditor *editPtr)
{
    unsigned int w, h;
    int reqWidth, reqHeight;
    
#ifdef notdef
    fprintf(stderr, "Calling ComputeLayout(%s) w=%d h=%d\n", 
	    Tk_PathName(editPtr->tkwin), Tk_Width(editPtr->tkwin),
	    Tk_Height(editPtr->tkwin));
#endif
    editPtr->flags &= ~LAYOUT_PENDING;

    /* Determine the height of the editor.  It's the maximum height of all
     * the text area. */

    editPtr->textWidth  = editPtr->textHeight  = 0;
    editPtr->normalWidth  = editPtr->normalHeight  = 0;
    editPtr->width = editPtr->height = 0;

    if (editPtr->flags & GEOMETRY) {
        ComputeGeometry(editPtr);
    }

    if (editPtr->prefTextWidth > 0) {
        w = Blt_TextWidth(editPtr->font, "0", 1);
        editPtr->width += editPtr->prefTextWidth * w;
    } else {
        editPtr->width += editPtr->worldWidth;
    }
    editPtr->height += editPtr->worldHeight;

    editPtr->width  += 2 * (editPtr->borderWidth + XPAD);
    editPtr->height += 2 * (editPtr->borderWidth + YPAD);

    /* Figure out the requested size of the widget.  This will also tell us
     * if we need scrollbars. */
 
    reqWidth  = editPtr->worldWidth  + 2 * (editPtr->borderWidth + XPAD);
    reqHeight = editPtr->worldHeight + 2 * (editPtr->borderWidth + YPAD);

    w = GetBoundedWidth(editPtr, reqWidth);
    h = GetBoundedHeight(editPtr, reqHeight);
    if ((reqWidth > w) && (editPtr->xScrollbar != NULL)) {
	editPtr->xScrollbarHeight = Tk_ReqHeight(editPtr->xScrollbar);
	h = GetBoundedHeight(editPtr, reqHeight + editPtr->xScrollbarHeight);
    } else {
        editPtr->xScrollbarHeight = 0;
    }
    if ((reqHeight > h) && (editPtr->yScrollbar != NULL)) {
	editPtr->yScrollbarWidth = Tk_ReqWidth(editPtr->yScrollbar);
	w = GetBoundedWidth(editPtr, reqWidth + editPtr->yScrollbarWidth);
    } else {
        editPtr->yScrollbarWidth = 0;
    }

    /* Save the computed width so that we only override the menu width if
     * the parent (combobutton/comboentry) width is greater than the normal
     * size of the menu.  */

    editPtr->normalWidth = w;
    editPtr->normalHeight = h;

    if (w < editPtr->post.editorWidth) {
	w = editPtr->post.editorWidth;
    }
    editPtr->width = w;
    editPtr->height = h;
    if (w != Tk_ReqWidth(editPtr->tkwin)) {
	editPtr->xOffset = 0;
    }
    if (h != Tk_ReqHeight(editPtr->tkwin)) {
	editPtr->yOffset = 0;
    }
    if ((w != Tk_ReqWidth(editPtr->tkwin)) ||
        (h != Tk_ReqHeight(editPtr->tkwin))) {
	Tk_GeometryRequest(editPtr->tkwin, w, h);
    }
    editPtr->flags |= SCROLL_PENDING;
#ifdef notdef
    fprintf(stderr, "Leaving ComputeLayout(%s) w=%d h=%d, rw=%d, rh=%d\n", 
	    Tk_PathName(editPtr->tkwin), w, h, Tk_ReqWidth(editPtr->tkwin),
	    Tk_ReqHeight(editPtr->tkwin));
#endif
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawTextLine --
 *
 * 	Draw the editable text associated with the entry.  The widget may
 * 	be scrolled so the text may be clipped.  We use a temporary pixmap
 * 	to draw the visible portion of the text.
 *
 *	We assume that text strings will be small for the most part.  The
 *	bad part of this is that we measure the text string 5 times.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawTextLine(ComboEditor *editPtr, Drawable drawable, TextLine *linePtr,
             int w, int h) 
{
    Blt_Bg bg;
    Blt_FontMetrics fm;
    GC gc;
    const char *textStart;
    int bytesLeft, pixelsLeft;
    int insertX, insertY;
    int textX, textY;
    int x, y;
    
#ifdef notdef
    fprintf(stderr, "DrawTextLine(%s, %.*s) w=%d h=%d\n", 
	    Tk_PathName(editPtr->tkwin), linePtr->numBytes,linePtr->text, w, h);
#endif
    if ((h < 2) || (w < 2)) {
	return;
    }
    Blt_Font_GetMetrics(editPtr->font, &fm);

#ifdef WIN32
    assert(drawable != None);
#endif

    bg = editPtr->textBg;
    gc = editPtr->textGC;
    x = linePtr->worldX - editPtr->xOffset;
    y = linePtr->worldY - editPtr->yOffset;
    textY = y + fm.ascent;
    textX = x;

    /* Text background. */
    { 
	int xOrigin, yOrigin;

	Blt_Bg_GetOrigin(bg, &xOrigin, &yOrigin);
	Blt_Bg_SetOrigin(editPtr->tkwin, bg, xOrigin + x, yOrigin + y);
	Blt_Bg_FillRectangle(editPtr->tkwin, drawable, bg, x, y, w,
                linePtr->height, 0, TK_RELIEF_FLAT);
	Blt_Bg_SetOrigin(editPtr->tkwin, bg, xOrigin, yOrigin);
    }	

    insertX = -1;
    textX = linePtr->worldX - editPtr->xOffset; /* Move back the starting
                                                 * position. */
    pixelsLeft = w + editPtr->xOffset;  
    bytesLeft = linePtr->numBytes;
    textStart = linePtr->text;

    if ((editPtr->flags & ICURSOR_ON) && 
        (editPtr->insertIndex >= linePtr->char1) && 
	(editPtr->insertIndex <= linePtr->char2)) {
        int numBytes, numPixels;
        
        /* The insertion cursor is on this line. Get the pixel offset to
         * the cursor by measuring the text to the location of the cursor.
         * First convert the cursor index to a byte offset. */
        numBytes = CharIndexToByteOffset(textStart,
                editPtr->insertIndex - linePtr->char1);
        numPixels = Blt_TextWidth(editPtr->font, textStart, numBytes);
        insertX = textX + numPixels;
        insertY = linePtr->worldY - editPtr->yOffset;
	if ((insertX - 4) > pixelsLeft) {
	    insertX = -1;               /* Out of the viewport. */
	}
    }

    /*
     *	Text is drawn in (up to) three segments.
     *
     *   1) Any text before the start the selection.  
     *   2) The selected text (drawn with a flat border) 
     *   3) Any text following the selection. This step will draw the 
     *      text string if there is no selection.
     */

    /* 
     * Step 1. If the selection starts on this line, draw any text
     *         preceding the selection that's still visible in the
     *         viewport. 
     */
    if ((pixelsLeft > 0) && (bytesLeft > 0) &&
        (editPtr->selFirst > linePtr->char1) &&
        (editPtr->selFirst < linePtr->char2)) {	
	int numPixels, numBytes;

        numBytes = CharIndexToByteOffset(textStart,
                editPtr->selFirst - linePtr->char1);
	numBytes = Blt_Font_Measure(editPtr->font, textStart,
                numBytes, pixelsLeft, TEXT_FLAGS, &numPixels);
        if ((textX + numPixels) > 0) {
            Blt_Font_Draw(editPtr->display, drawable, gc, editPtr->font, 
		Tk_Depth(editPtr->tkwin), 0.0f, textStart, numBytes,
                textX, textY);
        }
	textX += numPixels;
        pixelsLeft -= numPixels;
        bytesLeft  -= numBytes;
        textStart  += numBytes;
    }

    /* 
     * Step 2. Draw the selection if it's visible in the viewport.
     */
    if ((pixelsLeft > 0) && (bytesLeft > 0) &&
        (editPtr->selFirst < linePtr->char2) &&
        (editPtr->selLast > linePtr->char1)) {	
	int numBytes, numPixels;
        int selFirst, selLast;
        
        selFirst = MAX(editPtr->selFirst, linePtr->char1);
        selLast  = MIN(editPtr->selLast,  linePtr->char2);
        numBytes = CharIndexToByteOffset(textStart, selLast - selFirst);
	numBytes = Blt_Font_Measure(editPtr->font, textStart, numBytes,
                pixelsLeft, TEXT_FLAGS, &numPixels);
        if ((textX + numPixels) >  0) {
            Blt_Bg_FillRectangle(editPtr->tkwin, drawable, editPtr->selectBg,
                textX, y, numPixels, linePtr->height, editPtr->selBorderWidth,
                editPtr->selRelief);
            Blt_Font_Draw(editPtr->display, drawable, editPtr->selectGC, 
		editPtr->font, Tk_Depth(editPtr->tkwin), 0.0f, 
		textStart, numBytes, textX, textY);
        }
	textX += numPixels;
        pixelsLeft -= numPixels;
        bytesLeft  -= numBytes;
        textStart  += numBytes;
    }

    /* 
     * Step 3.  Draw any text following the selection that's visible in the
     *		viewport. In the case of no selection, this draws the
     *		entire line.
     */
    if ((pixelsLeft > 0) && (bytesLeft > 0)) {
        int numBytes, numPixels;
        
	numBytes = Blt_Font_Measure(editPtr->font, textStart, bytesLeft,
                pixelsLeft, TEXT_FLAGS, &numPixels);
	Blt_Font_Draw(editPtr->display, drawable, gc, editPtr->font,
                Tk_Depth(editPtr->tkwin), 0.0f, textStart, numBytes,
                textX, textY);
    }
    /* Draw the insertion cursor, if one is needed. */
    if (insertX >= 0) {
	int y1, y2;

	y1 = insertY + 1;
	y2 = insertY + linePtr->height - 2;
 	XDrawLine(editPtr->display, drawable, editPtr->insertGC, insertX, y1, 
		insertX, y2);
#ifndef notdef
	XDrawLine(editPtr->display, drawable, editPtr->insertGC, insertX + 1, 
		y1, insertX + 1, y2);
#endif
    }
}

static void
DrawTextArea(ComboEditor *editPtr, Drawable drawable, int x, int y, int w,
             int h)
{
    Pixmap pixmap;
    Blt_Bg bg;
    GC gc;
    int i;
    
    bg = editPtr->textBg;
    gc = editPtr->textGC;
    w = VPORTWIDTH(editPtr);
    h = VPORTHEIGHT(editPtr) + 1;

    /*
     * Create a pixmap the size of visible text area. This will be used for
     * clipping the scrolled text string.
     */
    pixmap = Blt_GetPixmap(editPtr->display, Tk_WindowId(editPtr->tkwin),
	w, h, Tk_Depth(editPtr->tkwin));

    /* Text background. */
    { 
	int xOrigin, yOrigin;

	Blt_Bg_GetOrigin(bg, &xOrigin, &yOrigin);
	Blt_Bg_SetOrigin(editPtr->tkwin, bg, xOrigin + x, yOrigin + y);
	Blt_Bg_FillRectangle(editPtr->tkwin, pixmap, bg, 0, 0, w, h, 
		0, TK_RELIEF_FLAT);
	Blt_Bg_SetOrigin(editPtr->tkwin, bg, xOrigin, yOrigin);
    }	
    for (i = editPtr->firstLine; i < editPtr->lastLine; i++) {
        DrawTextLine(editPtr, pixmap, editPtr->lines + i, w, h);
    }
    x += editPtr->borderWidth + XPAD;
    y += editPtr->borderWidth + YPAD;
    XCopyArea(editPtr->display, pixmap, drawable, gc, 0, 0, w, h, x, y);
    Tk_FreePixmap(editPtr->display, pixmap);
}

static void
ComputeVisibleLines(ComboEditor *editPtr)
{
    TextLine *linePtr; 
    
    assert((editPtr->flags & GEOMETRY) == 0);
    linePtr = FindLineFromCoord(editPtr, editPtr->yOffset);
    assert(linePtr != NULL);
    editPtr->firstLine = linePtr - editPtr->lines;
    linePtr = FindLineFromCoord(editPtr, editPtr->yOffset+VPORTHEIGHT(editPtr));
    if (linePtr != NULL) {
        editPtr->lastLine = linePtr - editPtr->lines + 1;
    } else {
        editPtr->lastLine = editPtr->numLines;
    }
}

static void
UpdateScrollbarLocations(ComboEditor *editPtr)
{
    /* Manage the geometry of the scrollbars. */
    if (editPtr->yScrollbarWidth > 0) {
	int x, y;
	int yScrollbarHeight;

	x = Tk_Width(editPtr->tkwin) - editPtr->borderWidth -
	    editPtr->yScrollbarWidth;
	y = editPtr->borderWidth;
	yScrollbarHeight = Tk_Height(editPtr->tkwin) - 
	    editPtr->xScrollbarHeight - 2 * editPtr->borderWidth;
	if ((Tk_Width(editPtr->yScrollbar) != editPtr->yScrollbarWidth) ||
	    (Tk_Height(editPtr->yScrollbar) != yScrollbarHeight) ||
	    (x != Tk_X(editPtr->yScrollbar)) || 
	    (y != Tk_Y(editPtr->yScrollbar))) {
	    Tk_MoveResizeWindow(editPtr->yScrollbar, x, y, 
		editPtr->yScrollbarWidth, yScrollbarHeight);
	}
	if (!Tk_IsMapped(editPtr->yScrollbar)) {
	    Tk_MapWindow(editPtr->yScrollbar);
	}
    } else if ((editPtr->yScrollbar != NULL) &&
	       (Tk_IsMapped(editPtr->yScrollbar))) {
	Tk_UnmapWindow(editPtr->yScrollbar);
    }
    if (editPtr->xScrollbarHeight > 0) {
	int x, y;
	int xScrollbarWidth;

	x = editPtr->borderWidth;
	y = Tk_Height(editPtr->tkwin) - editPtr->xScrollbarHeight - 
	    editPtr->borderWidth;
	xScrollbarWidth = Tk_Width(editPtr->tkwin) - 
	    editPtr->yScrollbarWidth - 2 * editPtr->borderWidth;
	if ((Tk_Width(editPtr->xScrollbar) != xScrollbarWidth) ||
	    (Tk_Height(editPtr->xScrollbar) != editPtr->xScrollbarHeight) ||
	    (x != Tk_X(editPtr->xScrollbar)) || 
	    (y != Tk_Y(editPtr->xScrollbar))) {
	    Tk_MoveResizeWindow(editPtr->xScrollbar, x, y, xScrollbarWidth,
		editPtr->xScrollbarHeight);
	}
	if (!Tk_IsMapped(editPtr->xScrollbar)) {
	    Tk_MapWindow(editPtr->xScrollbar);
	}
    } else if ((editPtr->xScrollbar != NULL) && 
	       (Tk_IsMapped(editPtr->xScrollbar))) {
	Tk_UnmapWindow(editPtr->xScrollbar);
    }
}

static void
DisplayProc(ClientData clientData)
{
    ComboEditor *editPtr = clientData;
    Pixmap drawable;
    int x, y, w, h;
    int screenWidth, screenHeight;

    editPtr->flags &= ~REDRAW_PENDING;
    if (editPtr->tkwin == NULL) {
	return;                         /* Window has been destroyed
					 * (should not get here) */
    }
#ifdef notdef
    fprintf(stderr, "Calling DisplayProc(%s) w=%d h=%d\n", 
	    Tk_PathName(editPtr->tkwin), Tk_Width(editPtr->tkwin),
	    Tk_Height(editPtr->tkwin));
#endif
    if (editPtr->flags & LAYOUT_PENDING) {
	ComputeLayout(editPtr);
    }
    w = Tk_Width(editPtr->tkwin);
    h = Tk_Height(editPtr->tkwin);
    if ((w <= 1) || (h <= 1)) {
	/* Don't bother computing the layout until the window size is
	 * something reasonable. */
	return;
    }
    if (!Tk_IsMapped(editPtr->tkwin)) {
	/* The editor's window isn't displayed, so don't bother drawing
	 * anything.  By getting this far, we've at least computed the
	 * coordinates of the editor's new layout.  */
	return;
    }
    if (editPtr->flags & SCROLL_PENDING) {
	int vw, vh;                     /* Viewport width and height. */
	/* 
	 * The view port has changed. The visible items need to be recomputed
	 * and the scrollbars updated.
	 */
	ComputeVisibleLines(editPtr);
	vw = VPORTWIDTH(editPtr);
	vh = VPORTHEIGHT(editPtr);
	if ((editPtr->xScrollCmdObjPtr) && (editPtr->flags & SCROLLX)) {
	    Blt_UpdateScrollbar(editPtr->interp, editPtr->xScrollCmdObjPtr,
		editPtr->xOffset, editPtr->xOffset+vw, editPtr->worldWidth);
	}
	if ((editPtr->yScrollCmdObjPtr) && (editPtr->flags & SCROLLY)) {
	    Blt_UpdateScrollbar(editPtr->interp, editPtr->yScrollCmdObjPtr,
		editPtr->yOffset, editPtr->yOffset+vh, editPtr->worldHeight);
	}
        UpdateScrollbarLocations(editPtr);
	editPtr->flags &= ~SCROLL_PENDING;
    }
    /*
     * Create a pixmap the size of the window for double buffering.
     */
    Blt_SizeOfScreen(editPtr->tkwin, &screenWidth, &screenHeight);
    w = CLAMP(w, 1, screenWidth);
    h = CLAMP(h, 1, screenHeight);

    /* Create pixmap the size of the widget. */
    drawable = Blt_GetPixmap(editPtr->display, Tk_WindowId(editPtr->tkwin), 
	w, h, Tk_Depth(editPtr->tkwin));
    Blt_Bg_FillRectangle(editPtr->tkwin, drawable, editPtr->textBg, 0, 0,
	w, h, editPtr->borderWidth, editPtr->relief);

    x = editPtr->borderWidth + editPtr->gap;
    y = editPtr->borderWidth;

    if ((editPtr->xScrollbarHeight > 0) && (editPtr->yScrollbarWidth > 0)) {
        Blt_Bg_FillRectangle(editPtr->tkwin, drawable, editPtr->normalBg,
                w - editPtr->yScrollbarWidth - editPtr->borderWidth,
                h - editPtr->xScrollbarHeight - editPtr->borderWidth,
                editPtr->yScrollbarWidth, editPtr->xScrollbarHeight,
                0, TK_RELIEF_FLAT);
    }        
    DrawTextArea(editPtr, drawable, x, y, w, h);
    
    XCopyArea(editPtr->display, drawable, Tk_WindowId(editPtr->tkwin),
	editPtr->textGC, 0, 0, Tk_Width(editPtr->tkwin), 
	Tk_Height(editPtr->tkwin), 0, 0);
    Tk_FreePixmap(editPtr->display, drawable);
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
    ComboEditor *editPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, editPtr->tkwin, 
	configSpecs, (char *)editPtr, objv[2], 0);
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
    ComboEditor *editPtr = clientData;

    if (objc == 2) {
	return Blt_ConfigureInfoFromObj(interp, editPtr->tkwin, configSpecs,
                (char *)editPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, editPtr->tkwin, configSpecs,
                (char *)editPtr, objv[3], 0);
    }
    if (ConfigureEditor(interp, editPtr, objc - 2, objv + 2,
                        BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    EventuallyRedraw(editPtr);
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
    ComboEditor *editPtr = clientData;
    CharIndex first, last;
    ByteOffset byte1, byte2;
    const char *string;
    
    if (GetIndexFromObj(interp, editPtr, objv[2], &first) != TCL_OK) {
	return TCL_ERROR;
    }
    last = first;
    if ((objc == 4) && 
	(GetIndexFromObj(interp, editPtr, objv[3], &last) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (first > last) {
	return TCL_OK;
    }
    if (editPtr->flags & READONLY) {
	return TCL_OK;                 /* Widget is not editable. */
    }
    string = Blt_DBuffer_String(editPtr->dbuffer);
    byte1 = CharIndexToByteOffset(string, first);
    byte2 = CharIndexToByteOffset(string, last);
    RecordEdit(editPtr, DELETE_OP, first, string + byte1, byte2 - byte1);
    if (!DeleteText(editPtr, first, last)) {
        Tcl_AppendResult(interp, "can't delete text", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *	Returns the current text string in the widget.
 *
 * Results:
 *	Standard TCL result.
 *
 *	pathName get ?firstIndex lastIndex?
 *
 *---------------------------------------------------------------------------
 */
static int
GetOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;
    Tcl_Obj *objPtr;
    
    if (objc == 4) {
        CharIndex firstChar, lastChar;
        ByteOffset firstByte, lastByte;
        const char *string;
        
        if (GetIndexFromObj(interp, editPtr, objv[2], &firstChar) != TCL_OK) {
            return TCL_ERROR;
        }
        if (GetIndexFromObj(interp, editPtr, objv[3], &lastChar) != TCL_OK) {
            return TCL_ERROR;
        }
        string = Blt_DBuffer_String(editPtr->dbuffer);
        firstByte = Tcl_NumUtfChars(string, firstChar);
        lastByte = Tcl_NumUtfChars(string, lastChar);
        objPtr = Tcl_NewStringObj(string + firstByte, lastByte - firstByte);
    } else if (objc == 2) {
        objPtr = Blt_DBuffer_StringObj(editPtr->dbuffer);
    } else {
        Tcl_AppendResult(interp, "wrong # of arguments: should be \"",
                Tcl_GetString(objv[0]), " get ?firstIndex lastIndex?",
                         (char *)NULL);
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
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
    ComboEditor *editPtr = clientData;
    CharIndex index;

    if (GetIndexFromObj(interp, editPtr, objv[2], &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if (index < 0) {
        editPtr->insertIndex = -1;
        return TCL_OK;
    }
    if (index >= editPtr->numChars) {
        index = editPtr->numChars;
    }
    editPtr->insertIndex = index;
    EventuallyRedraw(editPtr);
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
    ComboEditor *editPtr = clientData;
    CharIndex index;

    if (GetIndexFromObj(interp, editPtr, objv[2], &index) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), index);
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
 *	New information gets added to editPtr; it will be redisplayed soon,
 *	but not necessarily immediately.
 *
 *      pathName insert index string
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InsertOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;
    int length;
    CharIndex index;
    const char *string;

    if (GetIndexFromObj(interp, editPtr, objv[2], &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if (editPtr->flags & READONLY) {
	return TCL_OK;                 /* Widget is not editable. */
    }
    string = Tcl_GetStringFromObj(objv[3], &length);
    if (length == 0) {                   /* Nothing to insert. Move the
                                          * cursor anyways. */
	editPtr->insertIndex = index;
    } else {
        RecordEdit(editPtr, INSERT_OP, index, string, length);
	InsertText(editPtr, string, length, index);
    }
    EventuallyRedraw(editPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InvokeOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *  pathName invoke
 *
 *---------------------------------------------------------------------------
 */
static int
InvokeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;
    int result;

    result = TCL_OK;
    if (editPtr->cmdObjPtr != NULL) {
	Tcl_Obj *cmdObjPtr, *objPtr;

	cmdObjPtr = Tcl_DuplicateObj(editPtr->cmdObjPtr);
        objPtr = Blt_DBuffer_StringObj(editPtr->dbuffer);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	Tcl_IncrRefCount(cmdObjPtr);
        Tcl_Preserve(editPtr);
	result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_Release(editPtr);
	Tcl_DecrRefCount(cmdObjPtr);
    }
    return result;
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
    ComboEditor *editPtr = clientData;
    int x, y;

    memset(&editPtr->post, 0, sizeof(PostInfo));
    editPtr->post.tkwin     = Tk_Parent(editPtr->tkwin);
    editPtr->post.editorWidth = editPtr->normalWidth;
    /* Process switches  */
    if (Blt_ParseSwitches(interp, postSwitches, objc - 2, objv + 2, editPtr,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (editPtr->post.textObjPtr != NULL) {
        SetTextFromObj(editPtr, editPtr->post.textObjPtr);
        ComputeGeometry(editPtr);
    }
    editPtr->flags |= DROPDOWN;
    switch (editPtr->post.flags) {
    case POST_PARENT:
    case POST_WINDOW:
	{
	    Tk_Window tkwin;
	    int x, y, w, h;
	    int rootX, rootY;
	    
	    tkwin = editPtr->post.tkwin;
	    w = Tk_Width(tkwin);
	    h = Tk_Height(tkwin);
	    x = Tk_X(tkwin);
	    y = Tk_Y(tkwin);
	    Tk_GetRootCoords(Tk_Parent(tkwin), &rootX, &rootY);
	    x += rootX;
	    y += rootY;
	    editPtr->post.x1 = x;
	    editPtr->post.y1 = y;
	    editPtr->post.x2 = x + w;
	    editPtr->post.y2 = y + h;
	}
	break;
    case POST_REGION:
    case POST_CASCADE:
	break;
    case POST_POPUP:
	editPtr->flags &= ~DROPDOWN;
	break;
    }
    editPtr->post.editorWidth = editPtr->post.x2 - editPtr->post.x1;
    editPtr->post.editorHeight = editPtr->post.y2 - editPtr->post.y1;
    if ((editPtr->post.editorWidth != editPtr->post.lastEditorWidth) ||
	(editPtr->flags & LAYOUT_PENDING)) {
	ComputeLayout(editPtr);
    }
    editPtr->post.lastEditorWidth = editPtr->post.editorWidth;
    x = editPtr->post.x1;               /* Suppress compiler warning; */
    y = editPtr->post.y1;
    switch (editPtr->post.align) {
    case ALIGN_LEFT:
	x = editPtr->post.x1;
	break;
    case ALIGN_CENTER:
	{
	    int w;

	    w = editPtr->post.x2 - editPtr->post.x1;
	    x = editPtr->post.x1 + (w - editPtr->width) / 2; 
	}
	break;
    case ALIGN_RIGHT:
	x = editPtr->post.x2 - editPtr->width;
	break;
    }
    FixEditorCoords(editPtr, &x, &y);
    /*
     * If there is a post command for the editor, execute it.  This may
     * change the size of the editor, so be sure to recompute the editor's
     * geometry if needed.
     */
    if (editPtr->postCmdObjPtr != NULL) {
	int result;

	Tcl_IncrRefCount(editPtr->postCmdObjPtr);
	result = Tcl_EvalObjEx(interp, editPtr->postCmdObjPtr, TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(editPtr->postCmdObjPtr);
	if (result != TCL_OK) {
	    return result;
	}
	/*
	 * The post commands could have deleted the editor, which means we
	 * are dead and should go away.
	 */
	if (editPtr->tkwin == NULL) {
	    return TCL_OK;
	}
	if (editPtr->flags & LAYOUT_PENDING) {
	    ComputeLayout(editPtr);
	}
    }

    /*
     * Adjust the position of the editor if necessary to keep it visible on the
     * screen.  There are two special tricks to make this work right:
     *
     * 1. If a virtual root window manager is being used then
     *    the coordinates are in the virtual root window of
     *    editPtr's parent;  since the editor uses override-redirect
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

	parent = Tk_Parent(editPtr->tkwin);
	Blt_SizeOfScreen(editPtr->tkwin, &sw, &sh);
	Tk_GetVRootGeometry(parent, &rootX, &rootY, &rootWidth, &rootHeight);
	x += rootX;
	y += rootY;
	if (x < 0) {
	    x = 0;
	}
	if (y < 0) {
	    y = 0;
	}
	if ((x + editPtr->width) > sw) {
	    x = sw - editPtr->width;
	}
	if ((y + editPtr->height) > sh) {
	    y = sh - editPtr->height;
	}
	Tk_MoveToplevelWindow(editPtr->tkwin, x, y);
	Tk_MapWindow(editPtr->tkwin);
	Blt_MapToplevelWindow(editPtr->tkwin);
	Blt_RaiseToplevelWindow(editPtr->tkwin);
#ifdef notdef
	TkWmRestackToplevel(editPtr->tkwin, Above, NULL);
#endif
    }
    if (editPtr->flags & ICURSOR) {
        BlinkCursor(editPtr);
    }
    editPtr->flags |= POSTED;
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * RedoOp --
 *
 *	Inserts a new item into the comboentry at the given index.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The comboentry gets a new item.
 *
 *   .cb insert index string
 *
 *---------------------------------------------------------------------------
 */
static int
RedoOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;

    if (editPtr->flags & READONLY) {
	return TCL_OK;                 /* Widget is not editable. */
    }
    if (editPtr->redoPtr != NULL) {
	EditRecord *recPtr;

	recPtr = editPtr->redoPtr;
	if (recPtr->type == INSERT_OP) {
	    InsertText(editPtr, recPtr->text, recPtr->numBytes, recPtr->index);
	} else if (recPtr->type == DELETE_OP) {
            CharIndex last;

            last = recPtr->index + recPtr->numChars;
	    DeleteText(editPtr, recPtr->index, last);
	} else {
	    Tcl_AppendResult(interp, "unknown record type \"", 
			     Blt_Itoa(recPtr->type), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	editPtr->insertIndex = recPtr->insertIndex;
	editPtr->redoPtr = recPtr->nextPtr;
	recPtr->nextPtr = editPtr->undoPtr;
	editPtr->undoPtr = recPtr;
	EventuallyRedraw(editPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ScanOp --
 *
 *	Implements the quick scan.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ScanOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;

    int oper;
    int x;

#define SCAN_MARK	1
#define SCAN_DRAGTO	2
    {
	char *string;
	char c;
	int length;
	
	string = Tcl_GetStringFromObj(objv[2], &length);
	c = string[0];
	if ((c == 'm') && (strncmp(string, "mark", length) == 0)) {
	    oper = SCAN_MARK;
	} else if ((c == 'd') && (strncmp(string, "dragto", length) == 0)) {
	    oper = SCAN_DRAGTO;
	} else {
	    Tcl_AppendResult(interp, "bad scan operation \"", string,
		"\": should be either \"mark\" or \"dragto\"", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (objc == 3) {
	if (oper == SCAN_MARK) {
	    Tcl_SetIntObj(Tcl_GetObjResult(interp), editPtr->scanAnchor);
	}
	return TCL_OK;
    }
    if (Blt_GetPixelsFromObj(interp, editPtr->tkwin, objv[3], PIXELS_ANY, &x) 
	 != TCL_OK) {
	return TCL_ERROR;
    }
    if (oper == SCAN_MARK) {
	editPtr->scanAnchor = x;
	editPtr->scanX = editPtr->scrollX;
    } else {
	int worldX, xMax;
	int dx;

	dx = editPtr->scanAnchor - x;
	worldX = editPtr->scanX + (10 * dx);
	xMax = editPtr->viewWidth - ICWIDTH;

	if (worldX < 0) {
	    worldX = 0;
	} else if ((worldX + xMax) >= editPtr->textWidth) {
	    worldX = editPtr->textWidth; /* - (8 * xMax / 10); */
	}
	editPtr->scrollX = worldX;
	editPtr->flags |= SCROLL_PENDING;
	EventuallyRedraw(editPtr);
    }
    return TCL_OK;
}

static int
SeeOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;

    CharIndex index;
    TextLine *linePtr;
    int numBytes, numChars, numPixels;
    
    if (editPtr->flags & GEOMETRY) {
        ComputeGeometry(editPtr);
    }
    if (GetIndexFromObj(interp, editPtr, objv[2], &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if (index == -1) {
	return TCL_OK;
    }
    linePtr = FindLineFromIndex(editPtr, index);
    if (linePtr == NULL) {
        return TCL_OK;
    }
    if (linePtr->worldY < editPtr->yOffset) {
        editPtr->yOffset = linePtr->worldY;
    } else if ((linePtr->worldY + linePtr->height) >=
               (editPtr->yOffset + VPORTHEIGHT(editPtr))) {
        /* Put the line on the bottom. */
        editPtr->yOffset = linePtr->worldY -
            (VPORTHEIGHT(editPtr) - linePtr->height);
    }
    numChars = index - linePtr->char1;
    numBytes = CharIndexToByteOffset(Blt_DBuffer_String(editPtr->dbuffer),
                numChars);
    /* Measure to the first character. */
    numBytes = Blt_Font_Measure(editPtr->font, linePtr->text, numBytes,
                linePtr->worldX, 0, &numPixels);
    linePtr->text += numBytes;
    if (numPixels < editPtr->xOffset) {
        editPtr->xOffset = numPixels;
    } else if (numPixels > (editPtr->xOffset + VPORTWIDTH(editPtr))) {
        editPtr->xOffset = numPixels + VPORTWIDTH(editPtr) + 20;
    }
    editPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(editPtr);
    return TCL_OK;
}

/*ARGSUSED*/
static int
SelectionAdjustOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;
    CharIndex index;
    CharIndex half1, half2;

    if (GetIndexFromObj(interp, editPtr, objv[3], &index) != TCL_OK) {
	return TCL_ERROR;
    }
    half1 = (editPtr->selFirst + editPtr->selLast) / 2;
    half2 = (editPtr->selFirst + editPtr->selLast + 1) / 2;
    if (index < half1) {
	editPtr->selAnchor = editPtr->selLast;
    } else if (index > half2) {
	editPtr->selAnchor = editPtr->selFirst;
    }
    if (index >= 0) {
        SelectText(editPtr, index);
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
SelectionClearOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;

    if (editPtr->selFirst != -1) {
	editPtr->selFirst = editPtr->selLast = -1;
	EventuallyRedraw(editPtr);
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
SelectionFromOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;
    CharIndex index;

    if (GetIndexFromObj(interp, editPtr, objv[3], &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if (index >= 0) {
        editPtr->selAnchor = index;
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
SelectionPresentOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		   Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;
    int state;

    state = (editPtr->selFirst != -1);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*ARGSUSED*/
static int
SelectionRangeOp(ClientData clientData, Tcl_Interp *interp, int objc,
		 Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;
    int first, last;

    if (GetIndexFromObj(interp, editPtr, objv[3], &first) != TCL_OK) {
	return TCL_ERROR;
    }
    if (GetIndexFromObj(interp, editPtr, objv[4], &last) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((first >= 0) && (last >= 0)) {
        editPtr->selAnchor = first;
        SelectText(editPtr, last);
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
SelectionToOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;
    CharIndex index;

    if (GetIndexFromObj(interp, editPtr, objv[3], &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if (index >= 0) {
        SelectText(editPtr, index);
    }
    return TCL_OK;
}


static Blt_OpSpec selectionOps[] =
{
    {"adjust",  1, SelectionAdjustOp, 4, 4, "index",},
    {"clear",   1, SelectionClearOp, 3, 3, "",},
    {"from",    1, SelectionFromOp, 4, 4, "index"},
    {"present", 1, SelectionPresentOp, 3, 3, ""},
    {"range",   1, SelectionRangeOp, 5, 5, "start end",},
    {"to",      1, SelectionToOp, 4, 4, "index"},
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
 * UndoOp --
 *
 *	Inserts a new item into the comboentry at the given index.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The comboentry gets a new item.
 *
 *   .cb insert index string
 *
 *---------------------------------------------------------------------------
 */
static int
UndoOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;

    if (editPtr->flags & READONLY) {
	return TCL_OK;                 /* Widget is not editable. */
    }
    if (editPtr->undoPtr != NULL) {
	EditRecord *recPtr;

	recPtr = editPtr->undoPtr;
	if (recPtr->type == INSERT_OP) {
            CharIndex last;

            last = recPtr->index + recPtr->numChars;
	    DeleteText(editPtr, recPtr->index, last);
	} else if (recPtr->type == DELETE_OP) {
	    InsertText(editPtr, recPtr->text, recPtr->numBytes, recPtr->index);
	} else {
	    Tcl_AppendResult(interp, "unknown record type \"", 
			     Blt_Itoa(recPtr->type), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	editPtr->insertIndex = recPtr->insertIndex;
	editPtr->undoPtr = recPtr->nextPtr;
	recPtr->nextPtr = editPtr->redoPtr;
	editPtr->redoPtr = recPtr;
	EventuallyRedraw(editPtr);
    }
    return TCL_OK;
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
    ComboEditor *editPtr = clientData;

    if (!WithdrawEditor(editPtr)) {
	return TCL_OK;          /* This menu is already unposted. */
    }
    /*
     * If there is a unpost command for the menu, execute it.  
     */
    if (editPtr->unpostCmdObjPtr != NULL) {
	int result;

	Tcl_IncrRefCount(editPtr->unpostCmdObjPtr);
	result = Tcl_EvalObjEx(interp, editPtr->unpostCmdObjPtr, 
		TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(editPtr->unpostCmdObjPtr);
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    editPtr->flags &= ~POSTED;
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * WithdrawOp --
 *
 *      Hides the menu but doesn't call the unpost command. Technically
 *      the menu is still posted.
 *
 *  pathName withdraw 
 *
 *---------------------------------------------------------------------------
 */
static int
WithdrawOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;

    WithdrawEditor(editPtr);
    return TCL_OK;      
}

/*
 *---------------------------------------------------------------------------
 *
 * XViewOp --
 *
 *      Called by the scrollbar to set view horizontally in the 
 *      widget.
 *
 *  pathName xview firstFrac lastFrac
 *  pathName xview firstFrac lastFrac
 *
 *---------------------------------------------------------------------------
 */
static int
XViewOp(ClientData clientData, Tcl_Interp *interp, int objc,
	Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;
    int w;

    w = VPORTWIDTH(editPtr);
    if (objc == 2) {
	double fract;
	Tcl_Obj *listObjPtr, *objPtr;

	/* Report first and last fractions */
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	/*
	 * Note: we are bounding the fractions between 0.0 and 1.0 to support
	 * the "canvas"-style of scrolling.
	 */
	fract = (double)editPtr->xOffset / (editPtr->worldWidth);
	objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	fract = (double)(editPtr->xOffset + w) / (editPtr->worldWidth);
	objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &editPtr->xOffset,
	editPtr->worldWidth, w, editPtr->xScrollUnits, 
	BLT_SCROLL_MODE_HIERBOX) != TCL_OK) {
	return TCL_ERROR;
    }
    editPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(editPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * YViewOp --
 *
 *      Called by the scrollbar to set view vertically in the 
 *      widget.
 *
 *  pathName xview firstFrac lastFrac
 *  pathName xview firstFrac lastFrac
 *
 *---------------------------------------------------------------------------
 */
static int
YViewOp(ClientData clientData, Tcl_Interp *interp, int objc,
	Tcl_Obj *const *objv)
{
    ComboEditor *editPtr = clientData;
    int height;

    height = VPORTHEIGHT(editPtr);
    if (objc == 2) {
	double fract;
	Tcl_Obj *listObjPtr, *objPtr;

	/* Report first and last fractions */
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	/*
	 * Note: we are bounding the fractions between 0.0 and 1.0 to support
	 * the "canvas"-style of scrolling.
	 */
	fract = (double)editPtr->yOffset / (editPtr->worldHeight);
	objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	fract = (double)(editPtr->yOffset + height) /(editPtr->worldHeight);
	objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &editPtr->yOffset,
	editPtr->worldHeight, height, editPtr->yScrollUnits, 
	BLT_SCROLL_MODE_HIERBOX) != TCL_OK) {
	return TCL_ERROR;
    }
    editPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(editPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EditorInstCmdProc --
 *
 *	This procedure handles editor operations.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec editorOps[] =
{
    {"cget",      2, CgetOp,      3, 3, "value",},
    {"configure", 2, ConfigureOp, 2, 0, "?option value ...?",},
    {"delete",    1, DeleteOp,    3, 4, "firstIndex ?lastIndex?"},
    {"get",       1, GetOp,       2, 4, "?firstIndex lastIndex?"},
    {"icursor",   2, IcursorOp,   3, 3, "index"},
    {"index",     3, IndexOp,     3, 3, "index"},
    {"insert",    3, InsertOp,    4, 4, "index string"},
    {"invoke",    3, InvokeOp,    2, 2, "",},
    {"post",      4, PostOp,      2, 0, "?switches ...?",},
    {"redo",      1, RedoOp,      2, 2, "",},
    {"scan",      2, ScanOp,      3, 4, "dragto|mark x",},
    {"see",       3, SeeOp,       3, 3, "index"},
    {"selection", 3, SelectionOp, 2, 0, "args"},
    {"undo",      3, UndoOp,      2, 2, "",},
    {"unpost",    1, UnpostOp,    2, 2, "",},
    {"withdraw",  1, WithdrawOp,  2, 2, "",},
    {"xview",     2, XViewOp,     2, 5, "?moveto fract? ?scroll number what?",},
    {"yview",     2, YViewOp,     2, 5, "?moveto fract? ?scroll number what?",},
};
static int numEditorOps = sizeof(editorOps) / sizeof(Blt_OpSpec);

static int
EditorInstCmdProc(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numEditorOps, editorOps, BLT_OP_ARG1, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * EditorInstCmdDeletedProc --
 *
 *      This procedure can be called if the window was destroyed (tkwin
 *      will be NULL) and the command was deleted automatically.  In this
 *      case, we need to do nothing.
 *
 *      Otherwise this routine was called because the command was deleted.
 *      Then we need to clean-up and destroy the widget.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The widget is destroyed.
 *
 *---------------------------------------------------------------------------
 */
static void
EditorInstCmdDeletedProc(ClientData clientData)
{
    ComboEditor *editPtr = clientData;   /* Pointer to widget record. */

    if (editPtr->tkwin != NULL) {
	Tk_Window tkwin;

	tkwin = editPtr->tkwin;
	editPtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboEditorCmd --
 *
 *      This procedure is invoked to process the TCL command that
 *      corresponds to a widget managed by this module. See the user
 *      documentation for details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
ComboEditorCmdProc(
    ClientData clientData,              /* Main window associated with
					 * interpreter. */
    Tcl_Interp *interp,                 /* Current interpreter. */
    int objc,                           /* Number of arguments. */
    Tcl_Obj *const *objv)               /* Argument strings. */
{
    ComboEditor *editPtr;
    Tk_Window tkwin;
    XSetWindowAttributes attrs;
    char *path;
    unsigned int mask;

    if (objc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " pathName ?option value?...\"", 
		(char *)NULL);
	return TCL_ERROR;
    }
    /*
     * First time in this interpreter, invoke a procedure to initialize
     * various bindings on the combomenu widget.  If the procedure doesn't
     * already exist, source it from "$blt_library/bltComboEditor.tcl".  We
     * deferred sourcing the file until now so that the variable
     * $blt_library could be set within a script.
     */
    if (!Blt_CommandExists(interp, "::blt::ComboEditor::ConfigureScrollbars")) {
	if (Tcl_GlobalEval(interp, 
  	    "source [file join $blt_library bltComboEditor.tcl]") != TCL_OK) {
	    char info[200];

	    Blt_FormatString(info, 200, "\n    (while loading bindings for %.50s)", 
		    Tcl_GetString(objv[0]));
	    Tcl_AddErrorInfo(interp, info);
	    return TCL_ERROR;
	}
    }
    path = Tcl_GetString(objv[1]);
#define TOP_LEVEL_SCREEN ""
    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), path, 
	TOP_LEVEL_SCREEN);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    Tk_SetClass(tkwin, "BltComboEditor");
    editPtr = NewEditor(interp, tkwin);
    if (ConfigureEditor(interp, editPtr, objc - 2, objv + 2, 0) != TCL_OK) {
	Tk_DestroyWindow(editPtr->tkwin);
	return TCL_ERROR;
    }
    mask = (ExposureMask | StructureNotifyMask | FocusChangeMask);
    Tk_CreateEventHandler(tkwin, mask, EditorEventProc, editPtr);
    editPtr->cmdToken = Tcl_CreateObjCommand(interp, path, 
	EditorInstCmdProc, editPtr, EditorInstCmdDeletedProc);

    attrs.override_redirect = True;
    attrs.backing_store = WhenMapped;
    attrs.save_under = True;
    mask = (CWOverrideRedirect | CWSaveUnder | CWBackingStore);
#ifndef notdef
    Tk_MakeWindowExist(tkwin);
    XRaiseWindow(editPtr->display, Tk_WindowId(tkwin));
    Tk_ChangeWindowAttributes(tkwin, mask, &attrs);
#endif

    Tcl_SetObjResult(interp, objv[1]);
    return TCL_OK;
}

int
Blt_ComboEditorInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {
        "comboeditor", ComboEditorCmdProc
    };
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

#endif  /* NO_COMBOEDITOR */


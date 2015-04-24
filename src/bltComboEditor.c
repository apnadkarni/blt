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
  blt::popupeditor -

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
#include "bltOp.h"

#define TRACE_VAR_FLAGS (TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS)
#define TEXT_FLAGS (TK_PARTIAL_OK | TK_AT_LEAST_ONE)
#define EVENT_MASK	 (ExposureMask|StructureNotifyMask|FocusChangeMask)
#define CHILD_EVENT_MASK (ExposureMask|StructureNotifyMask)

#define IPAD		4		/* Internal pad between components. */
#define XPAD		1
#define YPAD		1		/* Internal pad between components. */
#define ICWIDTH		2		/* External pad between border and
					 * arrow. */
#define BUTTON_WIDTH	16
#define BUTTON_HEIGHT	16

#define CharIndexToByteOffset(s, n)	(Tcl_UtfAtIndex(s, n) - s)

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
#define SELECT_PENDING          (1<<3)  /* Indicates that the selection has
					 * changed and that and an update
					 * is pending.  */
#define INVOKE_PENDING          (1<<4)  /* Indicates that the selection has
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
#define CLRBUTTON	        (1<<15)	/* Display the clear button on the
					 * right when text has been
					 * entered. */
#define ICURSOR_ON              (1<<16)	/* The insertion cursor is currently
					 * visible on screen. */
#define FOCUS                   (1<<17)	/* The widget has focus. */
#define GEOMETRY                (1<<18)	/* The widget has focus. */
#define ACTIVE                  (1<<19)	/* The widget has focus. */
#define RESTRICT_NONE           (0)
#define INITIALIZED             (1<<22)


#define DEF_ACTIVE_BG                   RGB_SKYBLUE4
#define DEF_ACTIVE_FG                   RGB_WHITE
#define DEF_BACKGROUND                  RGB_WHITE
#define DEF_BORDERWIDTH                 STD_BORDERWIDTH
#define DEF_BUTTON_ACTIVEBACKGROUND	RGB_RED
#define DEF_BUTTON_ACTIVEFOREGROUND	RGB_WHITE
#define DEF_BUTTON_ACTIVERELIEF		"raised"
#define DEF_BUTTON_BACKGROUND		RGB_LIGHTBLUE0
#define DEF_BUTTON_BORDERWIDTH          "2"
#define DEF_BUTTON_COMMAND		(char *)NULL
#define DEF_BUTTON_FOREGROUND		RGB_LIGHTBLUE2
#define DEF_BUTTON_RELIEF		"flat"
#define DEF_CLRBUTTON                   "0"
#define DEF_CURSOR                      (char *)NULL
#define DEF_EXPORT_SELECTION            "no"
#define DEF_FONT                        STD_FONT_NORMAL
#define DEF_INSERT_COLOR                STD_NORMAL_FOREGROUND
#define DEF_INSERT_OFFTIME              "300"
#define DEF_INSERT_ONTIME               "600"
#define DEF_HEIGHT                      "0"
#define DEF_NORMAL_BACKGROUND           STD_NORMAL_BACKGROUND
#define DEF_CMD                         ((char *)NULL)
#define DEF_ICON                        ((char *)NULL)
#define DEF_NORMAL_FG                   STD_ACTIVE_FOREGROUND
#define DEF_NORMAL_FG_MONO              STD_ACTIVE_FG_MONO
#define DEF_POSTCOMMAND                 ((char *)NULL)
#define DEF_SHOW                        (char *)NULL
#define DEF_TEXT_NORMAL_BG              RGB_WHITE
#define DEF_TEXT_NORMAL_FG              RGB_BLACK
#define DEF_TEXT                        (char *)NULL
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
#define DEF_TEXT_VARIABLE               ((char *)NULL)

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
    int width;                          /* Width of the line in pixels. */
    int worldX, worldY;
    CharIndex char1, char2;             /* Starting and ending character
                                         * indices. */
    ByteOffset offset1, offset2;        /* Starting and ending byte
                                         * offsets. */
} TextLine;

static char emptyString[] = "";

typedef struct _EditRecord {
    struct _EditRecord *nextPtr;
    int type;
    CharIndex insertIndex;		/* Current index of the cursor. */
    CharIndex index;			/* Character index where text was
					   inserted. */
    int numBytes;			/* # of bytes in text string. */
    int numChars;			/* # of characters in text string. */
    char text[1];
} EditRecord;

/*
 * Icon --
 *
 *	Since instances of the same Tk image can be displayed in different
 *	windows with possibly different color palettes, Tk internally
 *	stores each instance in a linked list.  But if the instances are
 *	used in the same widget and therefore use the same color palette,
 *	this adds a lot of overhead, especially when deleting instances
 *	from the linked list.
 *
 *	For the editor widget, we never need more than a single instance of
 *	an image, regardless of how many times it's used.  Cache the image,
 *	maintaining a reference count for each image used in the widget.
 *	It's likely that the comboview widget will use many instances of
 *	the same image.
 */

typedef struct _Icon {
    Tk_Image tkImage;			/* Tk image being cached. */
    short int width, height;		/* Dimensions of the cached image. */
} *Icon;

#define IconHeight(i)	((i)->height)
#define IconWidth(i)	((i)->width)
#define IconImage(i)	((i)->tkImage)
#define IconName(i)	(Blt_Image_Name((i)->tkImage))

/*
 * Button --
 */
typedef struct {
    int borderWidth;			/* Width of 3D border around the tab's
					 * button. */
    int pad;				/* Extra padding around button. */
    int activeRelief;			/* 3D relief when the button is
					 * active. */
    int relief;				/* 3D relief of button. */
    XColor *normalFg;			/* If non-NULL, image to be displayed
					 * when button is displayed. */
    XColor *normalBg;			/* If non-NULL, image to be displayed
					 * when the button is active. */
    XColor *activeFg;			/* If non-NULL, image to be displayed
					 * when button is displayed. */
    XColor *activeBg;			/* If non-NULL, image to be displayed
					 * when the button is active. */
    Tcl_Obj *cmdObjPtr;			/* Command to be executed when the
					 * the button is invoked. */
    Blt_Painter painter;
    Blt_Picture normalPicture;		/* If non-NULL, image to be displayed
					 * when button is displayed. */
    Blt_Picture activePicture;		/* If non-NULL, image to be displayed
					 * when the button is active. */
    short int x, y;			/* Location of the button in the 
					 * entry. Used for picking. */
    short int width, height;		/* Dimension of the button. */
} Button;

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
    Tcl_Command cmdToken;              /* Token for frame's widget
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
    int selBW;				/* Border width of a selected
                                         * text.*/
    XColor *selFgColor;			/* Text color of a selected
                                         * text. */
    GC selectGC;
    Tcl_Obj *selCmdObjPtr;

    Blt_Bg selectBg;

    Button clearButton;

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
    
    /* 
     * Entry entry:
     *
     * The entry contains optionally an icon and a text string. The
     * rectangle surrounding an entry may have a 3D border.
     */
    Icon icon;				/* If non-NULL, image to be
					 * displayed in entry. Its value
					 * may be overridden by the
					 * -iconvariable option. */
    Tcl_Obj *iconVarObjPtr;		/* Name of TCL variable.  If
					 * non-NULL, this variable contains
					 * the name of an image
					 * representing the icon.  This
					 * overrides the value of the above
					 * field. */
    Icon image;				/* If non-NULL, image to be
					 * displayed instead of text in the
					 * entry. */
    char *text;				/* Text string to be displayed in
					 * the entry if an image has no
					 * been designated. Its value is
					 * overridden by the -textvariable
					 * option. */
    char *screenText;			/* Text string to be displayed on
					 * the screen.  If the -show option
					 * is used this string may consist
					 * of different characters from the
					 * above string.*/
    Tcl_Obj *textVarObjPtr;		/* Name of TCL variable.  If
					 * non-NULL, this variable contains
					 * the text string to * be
					 * displayed in the entry. This
					 * overrides the above field. */
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
    int prefIconWidth;			/* Desired width of icon, measured
					 * in pixels. */

    short int iconWidth, iconHeight;    /* Size of the icon. */
    int worldWidth, worldHeight;        /* Size of the world. */
    int textWidth, textHeight;

    ByteOffset firstOffset, lastOffset;	/* Byte offset of first and last
					 * characters visible in
					 * viewport. */
    int firstX, lastX;			/* x-coordinates of first and last
					 * characters visible in
					 * viewport. */ 
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
    int gap;
    TextLine *lines;
    int numLines;
    int xOffset, yOffset;               /* Scroll offsets. */
    int cursorX, cursorY;               /* Location of cursor. */
    int cursorHeight, cursorWidth;
    int normalWidth, normalHeight;
    int leader;
    int justify;
    int x, y;
} TextEditor;

static Blt_OptionFreeProc FreeIconProc;
static Blt_OptionParseProc ObjToIconProc;
static Blt_OptionPrintProc IconToObjProc;
static Blt_CustomOption iconOption = {
    ObjToIconProc, IconToObjProc, FreeIconProc, (ClientData)0
};

static Blt_OptionFreeProc FreeTextProc;
static Blt_OptionParseProc ObjToTextProc;
static Blt_OptionPrintProc TextToObjProc;
static Blt_CustomOption textOption = {
    ObjToTextProc, TextToObjProc, FreeTextProc, (ClientData)0
};

static Blt_OptionFreeProc FreeTextVarProc;
static Blt_OptionParseProc ObjToTextVarProc;
static Blt_OptionPrintProc TextVarToObjProc;
static Blt_CustomOption textVarOption = {
    ObjToTextVarProc, TextVarToObjProc, FreeTextVarProc, (ClientData)0
};

static Blt_ConfigSpec buttonSpecs[] =
{
    {BLT_CONFIG_COLOR, "-activebackground", "activeBackrgound", 
	"ActiveBackground", DEF_BUTTON_ACTIVEBACKGROUND, 
	Blt_Offset(Button, activeBg), 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForergound", 
	"ActiveForeground", DEF_BUTTON_ACTIVEFOREGROUND, 
	Blt_Offset(Button, activeFg), 0},
    {BLT_CONFIG_COLOR, "-background", "backrgound", "Background", 
	DEF_BUTTON_BACKGROUND, Blt_Offset(Button, normalBg), 0},
    {BLT_CONFIG_COLOR, "-foreground", "forergound", "Foreground", 
	DEF_BUTTON_FOREGROUND, Blt_Offset(Button, normalFg), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "ActiveRelief",
	DEF_BUTTON_ACTIVERELIEF, Blt_Offset(Button, activeRelief), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_BUTTON_BORDERWIDTH, Blt_Offset(Button, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", DEF_BUTTON_COMMAND, 
	Blt_Offset(Button, cmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_BUTTON_RELIEF, 
	Blt_Offset(Button, relief), 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_ACTIVE_BG, 
	Blt_Offset(TextEditor, activeBg), 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
	"ActiveForeground", DEF_ACTIVE_FG, 
	Blt_Offset(TextEditor, activeColor), 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background", 
	DEF_NORMAL_BACKGROUND, Blt_Offset(TextEditor, normalBg), 0 },
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_CURSOR, Blt_Offset(TextEditor, cursor), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_BORDERWIDTH, Blt_Offset(TextEditor, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-clearbutton", "clearButton", "ClearButton", 
	DEF_CLRBUTTON, Blt_Offset(TextEditor, flags), 
	BLT_CONFIG_DONT_SET_DEFAULT , 
	(Blt_CustomOption *)CLRBUTTON},
    {BLT_CONFIG_OBJ, "-clearcommand", "clearCommand", "ClearCommand", 
	DEF_BUTTON_COMMAND, Blt_Offset(TextEditor, clearButton.cmdObjPtr), 
	BLT_CONFIG_NULL_OK  },
    {BLT_CONFIG_OBJ, "-command", "command", "Command", 
	DEF_CMD, Blt_Offset(TextEditor, cmdObjPtr), 
	BLT_CONFIG_NULL_OK , },
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_CURSOR, Blt_Offset(TextEditor, cursor), 
	BLT_CONFIG_NULL_OK , },
    {BLT_CONFIG_BITMASK, "-exportselection", "exportSelection", 
	"ExportSelection", DEF_EXPORT_SELECTION, Blt_Offset(TextEditor, flags),
	BLT_CONFIG_DONT_SET_DEFAULT , 
	(Blt_CustomOption *)EXPORT_SELECTION},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 0, 
	0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_FONT, 
	Blt_Offset(TextEditor, font), 0, },
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", 
	DEF_NORMAL_FG, Blt_Offset(TextEditor, normalColor), 0, },
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height", DEF_HEIGHT, 
	Blt_Offset(TextEditor, reqHeight), 
	BLT_CONFIG_DONT_SET_DEFAULT , },
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", DEF_ICON, 
	Blt_Offset(TextEditor, icon), BLT_CONFIG_NULL_OK , 
	&iconOption},
    {BLT_CONFIG_COLOR, "-insertbackground", "insertBackground", 
	"InsertBackground", DEF_INSERT_COLOR, 
	Blt_Offset(TextEditor, insertColor), 0},
    {BLT_CONFIG_INT, "-insertofftime", "insertOffTime", "OffTime",
	DEF_INSERT_OFFTIME, Blt_Offset(TextEditor, insertOffTime), 
	BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_INT, "-insertontime", "insertOnTime", "OnTime",
	DEF_INSERT_ONTIME, Blt_Offset(TextEditor, insertOnTime), 
	BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_OBJ, "-postcommand", "postCommand", "PostCommand", 
	DEF_CMD, Blt_Offset(TextEditor, postCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_RELIEF, 
	Blt_Offset(TextEditor, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BORDER, "-selectbackground", "selectBackground", "Background",
	DEF_SELECT_BG_MONO, Blt_Offset(TextEditor, selectBg),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_BORDER, "-selectbackground", "selectBackground", "Background",
	DEF_SELECT_BACKGROUND, Blt_Offset(TextEditor, selectBg),
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
    {BLT_CONFIG_STRING, "-show", "show", "Show", DEF_SHOW, 
	Blt_Offset(TextEditor, cipher), 
	BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_CUSTOM, "-text", "text", "Text", DEF_TEXT, 
	Blt_Offset(TextEditor, text), 0, &textOption},
    {BLT_CONFIG_BACKGROUND, "-textbackground", "textBackground", "Background", 
	DEF_TEXT_NORMAL_BG, Blt_Offset(TextEditor, textBg), 0},
    {BLT_CONFIG_COLOR, "-textforeground", "textForeground", "TextForeground",
	DEF_TEXT_NORMAL_FG, Blt_Offset(TextEditor, textFg), 0},
    {BLT_CONFIG_CUSTOM, "-textvariable", "textVariable", "TextVariable", 
	DEF_TEXT_VARIABLE, Blt_Offset(TextEditor, textVarObjPtr), 
	BLT_CONFIG_NULL_OK , &textVarOption},
    {BLT_CONFIG_PIXELS_NNEG, "-textwidth", "textWidth", "TextWidth",
	DEF_WIDTH, Blt_Offset(TextEditor, prefTextWidth), 
	BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width",
	DEF_WIDTH, Blt_Offset(TextEditor, reqWidth), 
	BLT_CONFIG_DONT_SET_DEFAULT },
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
    {BLT_CONFIG_OBJ, "-unpostcommand", "unpostCommand", "UnpostCommand", 
	DEF_UNPOSTCOMMAND, Blt_Offset(TextEditor, unpostCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
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
	Blt_Offset(TextEditor, post.align), 0, 0, &postAlignSwitch},
    {BLT_SWITCH_CUSTOM, "-box", "x1 y1 x2 y2", (char *)NULL,
	0, 0, 0, &postBoxSwitch},
    {BLT_SWITCH_OBJ, "-text", "string", (char *)NULL,
	Blt_Offset(TextEditor, post.textObjPtr), 0},
    {BLT_SWITCH_OBJ, "-command", "cmdPrefix", (char *)NULL,
	Blt_Offset(TextEditor, post.cmdObjPtr), 0},
    {BLT_SWITCH_CUSTOM, "-window", "path", (char *)NULL,
	Blt_Offset(TextEditor, post.tkwin), 0, 0, &postWindowSwitch},
    {BLT_SWITCH_END}
};

static Tcl_IdleProc DisplayProc;
static Tcl_IdleProc SelectCmdProc;
static Tcl_FreeProc DestroyTextEditor;
static Tcl_TimerProc BlinkCursorTimerProc;
static Tcl_ObjCmdProc TextEditorCmd;

static Tk_LostSelProc LostSelectionProc;
static Tk_SelectionProc SelectionProc;
static Tk_EventProc EditorEventProc;
static Tk_EventProc ScrollbarEventProc;
static Tcl_FreeProc FreeEditorProc;
static Tk_ImageChangedProc IconChangedProc;

static Tk_GeomRequestProc ScrollbarGeometryProc;
static Tk_GeomLostSlaveProc ScrollbarCustodyProc;
static Tk_GeomMgr editorMgrInfo = {
    (char *)"comboeditor",                /* Name of geometry manager used by
					 * winfo. */
    ScrollbarGeometryProc,              /* Procedure to for new geometry
					 * requests. */
    ScrollbarCustodyProc,               /* Procedure when scrollbar is taken
					 * away. */
};

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
EventuallyRedraw(TextEditor *editPtr)
{
    if ((editPtr->tkwin != NULL) && 
	((editPtr->flags & REDRAW_PENDING) == 0)) {
	editPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayProc, editPtr);
    }
}

static int
InvokeCommand(Tcl_Interp *interp, TextEditor *editPtr) 
{
    int result;

    Tcl_Preserve(editPtr);
    Tcl_IncrRefCount(editPtr->cmdObjPtr);
    result = Tcl_EvalObjEx(interp, editPtr->cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(editPtr->cmdObjPtr);
    Tcl_Release(editPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * InvokeCmdProc --
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
InvokeCmdProc(ClientData clientData) 
{
    TextEditor *editPtr = clientData;

    editPtr->flags &= ~INVOKE_PENDING;
    if (editPtr->cmdObjPtr != NULL) {
	if (InvokeCommand(editPtr->interp, editPtr) != TCL_OK) {
	    Tcl_BackgroundError(editPtr->interp);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyInvokeCmd --
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
EventuallyInvokeCmd(TextEditor *editPtr) 
{
    if ((editPtr->flags & INVOKE_PENDING) == 0) {
	editPtr->flags |= INVOKE_PENDING;
	Tcl_DoWhenIdle(InvokeCmdProc, editPtr);
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
EventuallyInvokeSelectCmd(TextEditor *editPtr) 
{
    if ((editPtr->flags & SELECT_PENDING) == 0) {
	editPtr->flags |= SELECT_PENDING;
	Tcl_DoWhenIdle(SelectCmdProc, editPtr);
    }
}


static void
SetTextFromObj(TextEditor *editPtr, Tcl_Obj *objPtr) 
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

static char *
GetInterpResult(Tcl_Interp *interp)
{
#define MAX_ERR_MSG	1023
    static char mesg[MAX_ERR_MSG+1];

    strncpy(mesg, Tcl_GetStringResult(interp), MAX_ERR_MSG);
    mesg[MAX_ERR_MSG] = '\0';
    return mesg;
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
GetBoundedWidth(TextEditor *editPtr, int w)     
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
GetBoundedHeight(TextEditor *editPtr, int h)    
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
 * GetBoxFromObj --
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

static int
GetIconFromObj(Tcl_Interp *interp, TextEditor *editPtr, Tcl_Obj *objPtr, 
	       Icon *iconPtr)
{
    Tk_Image tkImage;
    const char *iconName;

    iconName = Tcl_GetString(objPtr);
    if (iconName[0] == '\0') {
	*iconPtr = NULL;
	return TCL_OK;
    }
    tkImage = Tk_GetImage(interp, editPtr->tkwin, iconName, IconChangedProc, 
	editPtr);
    if (tkImage != NULL) {
	struct _Icon *ip;
	int width, height;

	ip = Blt_AssertMalloc(sizeof(struct _Icon));
	Tk_SizeOfImage(tkImage, &width, &height);
	ip->tkImage = tkImage;
	ip->width = width;
	ip->height = height;
	*iconPtr = ip;
	return TCL_OK;
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 * 
 * TextVarTraceProc --
 *
 *	This procedure is invoked when someone changes the state variable
 *	associated with a text editor entry.  The editor's selected state
 *	is set to match the value of the variable.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The editor may become selected or deselected.
 *
 *---------------------------------------------------------------------------
 */
static char *
TextVarTraceProc(
    ClientData clientData,		/* Information about the item. */
    Tcl_Interp *interp,			/* Interpreter containing variable. */
    const char *name1,			/* First part of variable's name. */
    const char *name2,			/* Second part of variable's name. */
    int flags)				/* Describes what just happened. */
{
    TextEditor *editPtr = clientData;

    assert(editPtr->textVarObjPtr != NULL);
    if (flags & TCL_INTERP_DESTROYED) {
	return NULL;			/* Interpreter is going away. */
    }
    /*
     * If the variable is being unset, then re-establish the trace.
     */
    if (flags & TCL_TRACE_UNSETS) {
	if (flags & TCL_TRACE_DESTROYED) {
	    Tcl_SetVar(interp, name1, editPtr->text, TCL_GLOBAL_ONLY);
	    Tcl_TraceVar(interp, name1, TRACE_VAR_FLAGS, TextVarTraceProc, 
		clientData);
	}
	return NULL;
    }
    if (flags & TCL_TRACE_WRITES) {
	Tcl_Obj *valueObjPtr;

	/*
	 * Update the comboentry's text with the value of the variable, unless
	 * the widget already has that value (this happens when the variable
	 * changes value because we changed it because someone typed in the
	 * entry).
	 */
	valueObjPtr = Tcl_ObjGetVar2(interp, editPtr->textVarObjPtr, NULL, 
		TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
	if (valueObjPtr == NULL) {
	    return GetInterpResult(interp);
	} else {
	    SetTextFromObj(editPtr, valueObjPtr);
	    if (editPtr->cmdObjPtr != NULL) {
		EventuallyInvokeCmd(editPtr);
	    }
	}
	EventuallyRedraw(editPtr);
    }
    return NULL;
}

static int
UpdateTextVariable(Tcl_Interp *interp, TextEditor *editPtr) 
{
    Tcl_Obj *resultObjPtr, *objPtr;
    const char *varName;

    objPtr = Tcl_NewStringObj(editPtr->text, editPtr->numBytes);
    varName = Tcl_GetString(editPtr->textVarObjPtr); 
    Tcl_UntraceVar(interp, varName, TRACE_VAR_FLAGS, TextVarTraceProc,editPtr);
    Tcl_IncrRefCount(objPtr);
    resultObjPtr = Tcl_ObjSetVar2(interp, editPtr->textVarObjPtr, NULL, 
	objPtr, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    Tcl_DecrRefCount(objPtr);
    Tcl_TraceVar(interp, varName, TRACE_VAR_FLAGS, TextVarTraceProc, editPtr);
    if (resultObjPtr == NULL) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

static void
FreeUndoRecords(TextEditor *editPtr)
{
    EditRecord *recPtr, *nextPtr;

    for (recPtr = editPtr->undoPtr; recPtr != NULL; recPtr = nextPtr) {
	nextPtr = recPtr->nextPtr;
	Blt_Free(recPtr);
    }
    editPtr->undoPtr = NULL;
}

static void
FreeRedoRecords(TextEditor *editPtr)
{
    EditRecord *recPtr, *nextPtr;

    for (recPtr = editPtr->redoPtr; recPtr != NULL; recPtr = nextPtr) {
	nextPtr = recPtr->nextPtr;
	Blt_Free(recPtr);
    }
    editPtr->redoPtr = NULL;
}

static void
RecordEdit(TextEditor *editPtr, int type, CharIndex index, const char *text, 
	   int numBytes)
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
CleanText(TextEditor *editPtr)
{
    char *p, *q, *pend;

    if (editPtr->screenText != NULL) {
	Blt_Free(editPtr->screenText);
    }
    if (editPtr->cipher != NULL) {
	int i, charSize;
	Tcl_UniChar dummy;

	charSize = Tcl_UtfToUniChar(editPtr->cipher, &dummy);
	editPtr->numScreenBytes = editPtr->numChars * charSize;
	editPtr->screenText = Blt_AssertMalloc(editPtr->numScreenBytes + 1);
	for (p = editPtr->screenText, i = 0; i < editPtr->numChars; 
	     i++, p += charSize) {
	    strncpy(p, editPtr->cipher, charSize);
	}
	editPtr->screenText[editPtr->numScreenBytes] = '\0';
    } else {
        /* Convert tabs and newlines to spaces, just to maintain the same
         * character index and byte offsets between the screen text and
         * actual text. */
	editPtr->numScreenBytes = editPtr->numBytes;
	editPtr->screenText = Blt_AssertMalloc(editPtr->numScreenBytes + 1);
	for (p = editPtr->text, q = editPtr->screenText, 
		 pend = p + editPtr->numBytes; p < pend; p++, q++) {
	    if ((*p == '\n') || (*p == '\t')) {
		*q = ' ';
	    } else {
		*q = *p;
	    }
	}
    }
} 

/*
 *---------------------------------------------------------------------------
 *
 * IconChangedProc
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
IconChangedProc(
    ClientData clientData,
    int x, int y, int w, int h,		/* Not used. */
    int imageWidth, int imageHeight)	/* Not used. */
{
    TextEditor *editPtr = clientData;

    editPtr->flags |= (LAYOUT_PENDING | SCROLL_PENDING);
    EventuallyRedraw(editPtr);
}

static void
BlinkCursor(TextEditor *editPtr)
{
    int time;

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
    TextEditor *editPtr = clientData;

    if (editPtr->insertOffTime == 0) {
	return;
    }
    if (editPtr->flags & ICURSOR) {
	BlinkCursor(editPtr);
	EventuallyRedraw(editPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EditorEventProc --
 *
 * 	This procedure is invoked by the Tk dispatcher for various events on
 * 	comboentry widgets.
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
    TextEditor *editPtr = clientData;

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
	if (editPtr->flags & INVOKE_PENDING) {
	    Tcl_CancelIdleCall(InvokeCmdProc, editPtr);
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
 *      This procedure is invoked by the Tk event handler when StructureNotify
 *      events occur in a scrollbar managed by the widget.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
ScrollbarEventProc(
    ClientData clientData,              /* Pointer to Entry structure for
					 * widget referred to by eventPtr. */
    XEvent *eventPtr)                   /* Describes what just happened. */
{
    TextEditor *editPtr = clientData;

    if (eventPtr->type == ConfigureNotify) {
	editPtr->flags |= LAYOUT_PENDING;
	EventuallyRedraw(editPtr);
    } else if (eventPtr->type == DestroyNotify) {
	if ((editPtr->yScrollbar != NULL) &&
	    (eventPtr->xany.window == Tk_WindowId(editPtr->yScrollbar))) {
	    editPtr->yScrollbar = NULL;
	} else if ((editPtr->xScrollbar != NULL) && 
		(eventPtr->xany.window == Tk_WindowId(editPtr->xScrollbar))) {
	    editPtr->xScrollbar = NULL;
	} 
	editPtr->flags |= LAYOUT_PENDING;
	EventuallyRedraw(editPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollbarCustodyProc --
 *
 *      This procedure is invoked when a scrollbar has been stolen by another
 *      geometry manager.
 *
 * Results:
 *      None.
 *
 * Side effects:
  *     Arranges for the combomenu to have its layout re-arranged at the next
 *      idle point.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ScrollbarCustodyProc(
    ClientData clientData,              /* Information about the combomenu. */
    Tk_Window tkwin)                    /* Scrollbar stolen by another geometry
					 * manager. */
{
    TextEditor *editPtr = (TextEditor *)clientData;

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
 *      This procedure is invoked by Tk_GeometryRequest for scrollbars managed
 *      by the combomenu.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the combomenu to have its layout re-computed and
 *      re-arranged at the next idle point.
 *
 * -------------------------------------------------------------------------- */
/* ARGSUSED */
static void
ScrollbarGeometryProc(
    ClientData clientData,              /* TextEditor widget record.  */
    Tk_Window tkwin)                    /* Scrollbar whose geometry has
					 * changed. */
{
    TextEditor *editPtr = (TextEditor *)clientData;

    editPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(editPtr);
}


static void
UnmanageScrollbar(TextEditor *editPtr, Tk_Window scrollbar)
{
    Tk_DeleteEventHandler(scrollbar, StructureNotifyMask,
                ScrollbarEventProc, editPtr);
    Tk_ManageGeometry(scrollbar, (Tk_GeomMgr *)NULL, editPtr);
    if (Tk_IsMapped(scrollbar)) {
        Tk_UnmapWindow(scrollbar);
    }
}

static void
ManageScrollbar(TextEditor *editPtr, Tk_Window scrollbar)
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
    TextEditor *editPtr,
    Tcl_Obj *objPtr,                    /* String representing scrollbar
					 * window. */
    Tk_Window *tkwinPtr)
{
    Tk_Window tkwin;

    if (objPtr == NULL) {
	*tkwinPtr = NULL;
	return;
    }
    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objPtr), editPtr->tkwin);
    if (tkwin == NULL) {
	Tcl_BackgroundError(interp);
	return;
    }
    if (Tk_Parent(tkwin) != editPtr->tkwin) {
	Tcl_AppendResult(interp, "scrollbar \"", Tk_PathName(tkwin), 
			 "\" must be a child of combomenu.", (char *)NULL);
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
    TextEditor *editPtr = clientData;

    editPtr->flags &= ~INSTALL_XSCROLLBAR;
    InstallScrollbar(editPtr->interp, editPtr, editPtr->xScrollbarObjPtr,
		     &editPtr->xScrollbar);
}

static void
InstallYScrollbar(ClientData clientData)
{
    TextEditor *editPtr = clientData;

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
    TextEditor *editPtr = clientData;

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
SelectText(TextEditor *editPtr, CharIndex index)
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
	if (editPtr->selCmdObjPtr != NULL) {
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
    TextEditor *editPtr = clientData;

    editPtr->flags &= ~SELECT_PENDING;
    if (editPtr->selCmdObjPtr != NULL) {
	int result;

	Tcl_Preserve(editPtr);
	Tcl_IncrRefCount(editPtr->selCmdObjPtr);
	result = Tcl_EvalObjEx(editPtr->interp, editPtr->selCmdObjPtr,
                               TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(editPtr->selCmdObjPtr);
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
    int offset,				/* Offset within the selection of the
					 * first character to be returned. */
    char *buffer,			/* Location in which to place
					 * selection. */
    int maxBytes)			/* Maximum # of bytes to place in
					 * the buffer, not including
					 * terminating NULL character. */
{
    TextEditor *editPtr = clientData;
    int size;

    size = 0;
    if (editPtr->selFirst >= 0) {
	ByteOffset first, last;

	first = CharIndexToByteOffset(editPtr->screenText,editPtr->selFirst);
	last = CharIndexToByteOffset(editPtr->screenText, editPtr->selLast);
	size = last - first - offset;
	assert(size >= 0);
	if (size > maxBytes) {
	    size = maxBytes;
	}
	memcpy(buffer, Blt_DBuffer_String(editPtr->dbuffer) + first + offset,
               size);
	buffer[size] = '\0';
    }
    return size;
}


/*ARGSUSED*/
static void
FreeTextVarProc(ClientData clientData, Display *display, char *widgRec,
                int offset)
{
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);

    if (*objPtrPtr != NULL) {
	TextEditor *editPtr = (TextEditor *)(widgRec);
	const char *varName;

	varName = Tcl_GetString(*objPtrPtr);
	Tcl_UntraceVar(editPtr->interp, varName, TRACE_VAR_FLAGS, 
		TextVarTraceProc, editPtr);
	Tcl_DecrRefCount(*objPtrPtr);
	*objPtrPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTextVarProc --
 *
 *	Convert the variable to a traced variable.
 *
 * Results:
 *	The return value is a standard TCL result.  The color pointer is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTextVarProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                 Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    TextEditor *editPtr = (TextEditor *)(widgRec);
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);
    const char *varName;
    Tcl_Obj *valueObjPtr;

    /* Remove the current trace on the variable. */
    if (*objPtrPtr != NULL) {
	varName = Tcl_GetString(*objPtrPtr);
	Tcl_UntraceVar(interp, varName, TRACE_VAR_FLAGS, TextVarTraceProc, 
		editPtr);
	Tcl_DecrRefCount(*objPtrPtr);
	*objPtrPtr = NULL;
    }
    varName = Tcl_GetString(objPtr);
    if ((varName[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
	return TCL_OK;
    }

    valueObjPtr = Tcl_ObjGetVar2(interp, objPtr, NULL, TCL_GLOBAL_ONLY);
    if (valueObjPtr != NULL) {
	SetTextFromObj(editPtr, valueObjPtr);
	if (editPtr->textVarObjPtr != NULL) {
	    if (UpdateTextVariable(interp, editPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    *objPtrPtr = objPtr;
    Tcl_IncrRefCount(objPtr);
    Tcl_TraceVar(interp, varName, TRACE_VAR_FLAGS, TextVarTraceProc, editPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextVarToObjProc --
 *
 *	Return the name of the style.
 *
 * Results:
 *	The name representing the style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TextVarToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                 char *widgRec, int offset, int flags)	
{
    Tcl_Obj *objPtr = *(Tcl_Obj **)(widgRec + offset);

    if (objPtr == NULL) {
	objPtr = Tcl_NewStringObj("", -1);
    } 
    return objPtr;
}

static void
FreeIcon(TextEditor *editPtr, Icon icon)
{
    Tk_FreeImage(IconImage(icon));
    Blt_Free(icon);
}


/*ARGSUSED*/
static void
FreeIconProc(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Icon icon = *(Icon *)(widgRec + offset);

    if (icon != NULL) {
	TextEditor *editPtr = (TextEditor *)widgRec;

	FreeIcon(editPtr, icon);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToIconProc --
 *
 *	Convert a image into a hashed icon.
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.
 *	Otherwise, TCL_ERROR is returned and an error message is left in
 *	interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToIconProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    TextEditor *editPtr = (TextEditor *)widgRec;
    Icon *iconPtr = (Icon *)(widgRec + offset);
    Icon icon;

    if (GetIconFromObj(interp, editPtr, objPtr, &icon) != TCL_OK) {
	return TCL_ERROR;
    }
    if (*iconPtr != NULL) {
	FreeIcon(editPtr, *iconPtr);
    }
    *iconPtr = icon;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IconToObjProc --
 *
 *	Converts the icon into its string representation (its name).
 *
 * Results:
 *	The name of the icon is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
IconToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              char *widgRec, int offset, int flags)	
{
    Icon icon = *(Icon *)(widgRec + offset);
    Tcl_Obj *objPtr;

    if (icon == NULL) {
	objPtr = Tcl_NewStringObj("", 0);
    } else {
	objPtr =Tcl_NewStringObj(Blt_Image_Name(IconImage(icon)), -1);
    }
    return objPtr;
}

/*ARGSUSED*/
static void
FreeTextProc(ClientData clientData, Display *display, char *widgRec,
             int offset)
{
    TextEditor *editPtr = (TextEditor *)(widgRec);

    if (editPtr->text != emptyString) {
	Blt_Free(editPtr->text);
	Blt_Free(editPtr->screenText);
	editPtr->text = emptyString;
	editPtr->screenText = NULL;
	editPtr->numScreenBytes = editPtr->numBytes = 0;
    }
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
ObjToTextProc(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* String representing style. */
    char *widgRec,		/* Widget record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    TextEditor *editPtr = (TextEditor *)(widgRec);

    if (editPtr->text != emptyString) {
	Blt_Free(editPtr->text);
	Blt_Free(editPtr->screenText);
	editPtr->text = emptyString;
	editPtr->screenText = NULL;
	editPtr->numScreenBytes = editPtr->numBytes = 0;
    }
    SetTextFromObj(editPtr, objPtr);
    if (editPtr->textVarObjPtr != NULL) {
	if (UpdateTextVariable(interp, editPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
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
    TextEditor *editPtr = (TextEditor *)(widgRec);

    return Tcl_NewStringObj(editPtr->text, editPtr->numBytes);
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
    TextEditor *editPtr = (TextEditor *)record;
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
    TextEditor *editPtr = (TextEditor *)record;
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
    TextEditor *editPtr = (TextEditor *)record;
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
PostBoxSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Not used. */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    TextEditor *editPtr = (TextEditor *)record;
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

static int
SetCursorPosition(TextEditor *editPtr, CharIndex index)
{
    Blt_FontMetrics fontMetrics;
    int numBytes, numChars, numPixels;
    int x, y;
    TextLine *linePtr, *endPtr;
    
    Blt_Font_GetMetrics(editPtr->font, &fontMetrics);
    x = y = editPtr->borderWidth;
    if (editPtr->icon != NULL) {
	x += IconWidth(editPtr->icon) + 2 * editPtr->gap;
    }
    for (linePtr = editPtr->lines, endPtr = linePtr + editPtr->numLines;
         linePtr < endPtr; linePtr++) {

        if ((linePtr->char1 >= index) && (linePtr->char2 < index)) {
            break;                      /* This is line containing the 
                                         * cursor. */
        }
    }
    if (linePtr == endPtr) {
        return TCL_OK;                  /* Outside of range. */
    }
    /* Find the start of the line. */
    /* Find the start and end of the line as text offsets. */
    numChars = index - linePtr->char1;
    numBytes = CharIndexToByteOffset(linePtr->text, numChars);
    Blt_Font_Measure(editPtr->font, linePtr->text,
        numBytes, linePtr->worldX + editPtr->xOffset, TEXT_FLAGS,
        &numPixels);

    editPtr->cursorX = linePtr->worldX + numPixels;
    editPtr->cursorY = linePtr->worldY;
    editPtr->cursorHeight = fontMetrics.linespace;
    editPtr->cursorWidth = 3;
    return TCL_OK;
}

static void
InsertText(TextEditor *editPtr, const unsigned char *text, int numBytes,
           int index)
{
    int numChars;
    int result;
    const char *string;
    ByteOffset offset;
    
    string = Blt_DBuffer_String(editPtr->dbuffer);

    offset = CharIndexToByteOffset(string, index);
    if (offset == Blt_DBuffer_Length(editPtr->dbuffer)) { /* Append */
        result = Blt_DBuffer_AppendData(editPtr->dbuffer, text, numBytes);
    } else if (offset == 0) {           /* Prepend */
        result = Blt_DBuffer_InsertData(editPtr->dbuffer, text, numBytes, 0);
    } else {                            /* Insert into existing. */
        result = Blt_DBuffer_InsertData(editPtr->dbuffer, text, numBytes,
                index);
    }
    if (!result) {
        return;
    }
    /* 
     * All indices from the start of the insertion to the end of the string
     * need to be updated.  Simply move the indices down by the number of
     * characters added.
     */
    string = Blt_DBuffer_String(editPtr->dbuffer);
    numChars = Tcl_NumUtfChars(string, index);
    if (editPtr->selFirst >= index) {
	editPtr->selFirst += numChars;
    }
    if (editPtr->selLast > index) {
	editPtr->selLast += numChars;
    }
    if ((editPtr->selAnchor > index) || (editPtr->selFirst >= index)) {
	editPtr->selAnchor += numChars;
    }
    editPtr->insertIndex = index + numChars;
    editPtr->flags |= LAYOUT_PENDING;
    editPtr->numChars += numChars;
    EventuallyRedraw(editPtr);
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
DeleteText(TextEditor *editPtr, CharIndex firstIndex, CharIndex lastIndex)
{
    size_t numChars;
    
    numChars = lastIndex - firstIndex + 1;
    if (!Blt_DBuffer_DeleteData(editPtr->dbuffer, firstIndex, numChars)) {
        return TCL_ERROR;
    }
    if (editPtr->selFirst >= firstIndex) {
	if (editPtr->selFirst >= lastIndex) {
	    editPtr->selFirst -= numChars;
	} else {
	    editPtr->selFirst = firstIndex;
	}
    }
    if (editPtr->selLast >= firstIndex) {
	if (editPtr->selLast >= lastIndex) {
	    editPtr->selLast -= numChars;
	} else {
	    editPtr->selLast = firstIndex;
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
    editPtr->flags |= LAYOUT_PENDING;
    editPtr->numChars -= numChars;
    EventuallyRedraw(editPtr);
    return TCL_OK;
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
 *       @x             X-coordinate in the entry's window;  the character 
 *			spanning that x-coordinate is used.  For example, 
 *			"@0" indicates the left-most character in the window.
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
GetIndexFromObj(Tcl_Interp *interp, TextEditor *editPtr, Tcl_Obj *objPtr,
                CharIndex *indexPtr)
{
    char *string;
    char c;
    CharIndex index;

    if (Tcl_GetIntFromObj((Tcl_Interp *)NULL, objPtr, &index) == TCL_OK) {
	/* Convert the character index into a byte offset. */
	if (editPtr->screenText == NULL) {
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
    } else if ((c == 's') && (strcmp(string, "sel.first") == 0)) {
	*indexPtr = (int)editPtr->selFirst;
    } else if ((c == 's') && (strcmp(string, "sel.last") == 0)) {
	*indexPtr = (int)editPtr->selLast;
    } else if (c == '@') {
	int x, dummy, numBytes;

	if (Tcl_GetInt(interp, string+1, &x) != TCL_OK) {
	    return TCL_ERROR;
	}
	/* Convert screen position to character index */
	x -= editPtr->borderWidth + editPtr->iconWidth;
	x += editPtr->scrollX;
	numBytes = Blt_Font_Measure(editPtr->font, editPtr->screenText, 
		editPtr->numScreenBytes, x, TK_PARTIAL_OK|TK_AT_LEAST_ONE, 
		&dummy);
	*indexPtr = Tcl_NumUtfChars(editPtr->screenText, numBytes);
    } else {
	Tcl_AppendResult(interp, "unknown index \"", string, "\"",(char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyButton --
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyButton(TextEditor *editPtr, Button *butPtr)
{
    iconOption.clientData = editPtr;
    Blt_FreeOptions(buttonSpecs, (char *)butPtr, editPtr->display, 0);
    if (butPtr->activePicture != NULL) {
	Blt_FreePicture(butPtr->activePicture);
    }
    if (butPtr->normalPicture != NULL) {
	Blt_FreePicture(butPtr->normalPicture);
    }
    if (butPtr->painter != NULL) {
	Blt_FreePainter(butPtr->painter);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureButton --
 *
 * 	This procedure is called to process an objv/objc list, plus the Tk
 * 	option database, in order to configure (or reconfigure) the widget.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is
 *	returned, then interp->result contains an error message.
 *
 * Side Effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for setPtr; old resources get freed, if there were
 *	any.  The widget is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureButton(
    Tcl_Interp *interp,			/* Interpreter to report errors. */
    TextEditor *editPtr,		/* Information about widget; may or
					 * may not already have values for
					 * some fields. */
    int objc,
    Tcl_Obj *const *objv,
    int flags)
{
    Button *butPtr = &editPtr->clearButton;

    iconOption.clientData = editPtr;
    if (Blt_ConfigureWidgetFromObj(interp, editPtr->tkwin, buttonSpecs, 
	objc, objv, (char *)butPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    butPtr->width = butPtr->height = 0;
    if (editPtr->flags & CLRBUTTON) {
	butPtr->width = BUTTON_WIDTH + 2 * butPtr->borderWidth;
	butPtr->height = BUTTON_HEIGHT + 2 * butPtr->borderWidth;
    }
    EventuallyRedraw(editPtr);
    return TCL_OK;
}

static void
ConfigureScrollbarsProc(ClientData clientData)
{
    TextEditor *editPtr = clientData;
    Tcl_Interp *interp;

    interp = editPtr->interp;
    /* 
     * Execute the initialization procedure on this widget.
     */
    editPtr->flags &= ~UPDATE_PENDING;
    if (Tcl_VarEval(interp, "::blt::TextEditor::ConfigureScrollbars ", 
	Tk_PathName(editPtr->tkwin), (char *)NULL) != TCL_OK) {
	Tcl_BackgroundError(interp);
    }
}

static void
DestroyEditor(DestroyData data)
{
    TextEditor *editPtr = (TextEditor *)data;

    iconOption.clientData = editPtr;
    Blt_FreeOptions(configSpecs, (char *)editPtr, editPtr->display, 0);

    FreeUndoRecords(editPtr);
    FreeRedoRecords(editPtr);
    DestroyButton(editPtr, &editPtr->clearButton);

    if (editPtr->insertTimerToken != NULL) {
	Tcl_DeleteTimerHandler(editPtr->insertTimerToken);
    }
    if (editPtr->flags & REDRAW_PENDING) {
        Tcl_CancelIdleCall(DisplayProc, editPtr);
    }
    if (editPtr->flags & SELECT_PENDING) {
        Tcl_CancelIdleCall(SelectCmdProc, editPtr);
    }
    if (editPtr->flags & INVOKE_PENDING) {
        Tcl_CancelIdleCall(InvokeCmdProc, editPtr);
    }
    if (editPtr->tkwin != NULL) {
	Tk_DeleteSelHandler(editPtr->tkwin, XA_PRIMARY, XA_STRING);
	Tk_DeleteEventHandler(editPtr->tkwin, EVENT_MASK, EditorEventProc,
                editPtr);
    }
    if (editPtr->textGC != NULL) {
	Tk_FreeGC(editPtr->display, editPtr->textGC);
    }
    if (editPtr->selectGC != NULL) {
	Tk_FreeGC(editPtr->display, editPtr->selectGC);
    }
    if (editPtr->insertGC != NULL) {
	Tk_FreeGC(editPtr->display, editPtr->insertGC);
    }
    Tcl_DeleteCommandFromToken(editPtr->interp, editPtr->cmdToken);
    Blt_Free(editPtr);
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
ComputeGeometry(TextEditor *editPtr)
{
    TextLine *linePtr, *lines;
    Blt_FontMetrics fm;
    int count;			/* Count # of characters on each line */
    int lineHeight;
    int maxHeight, maxWidth;
    int numLines;
    const char *p, *tendPtr, *start;
    int i;
    const char *text;
    int lastByte, lastChar;
    
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

    linePtr = lines;
    lastByte = lastChar = 0;
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
            linePtr->worldY = maxHeight + fm.ascent;
            linePtr->offset1 = lastByte;
            linePtr->offset2 = linePtr->offset1 + count;
	    maxHeight += lineHeight;
            lastChar = linePtr->char2 + 1;
            lastByte = linePtr->offset2 + 1;
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
	linePtr->worldY = maxHeight + fm.ascent;
        linePtr->char1 = lastChar;
        linePtr->char2 = linePtr->char1 + Tcl_NumUtfChars(start, count);
        linePtr->offset1 = lastByte;
        linePtr->offset2 = linePtr->offset1 + count;
	maxHeight += lineHeight;
	numLines++;
    }
    for (i = 0; i < numLines; i++) {
        linePtr = lines + i;
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
    editPtr->worldWidth = maxWidth;
    editPtr->worldHeight = maxHeight - editPtr->leader;
}

static int
ConfigureEditor(Tcl_Interp *interp, TextEditor *editPtr, int objc,
                Tcl_Obj *const *objv, int flags)
{
    unsigned int gcMask;
    XGCValues gcValues;
    GC newGC;
    int updateNeeded;
    
    if (Blt_ConfigureWidgetFromObj(interp, editPtr->tkwin, configSpecs, objc, 
		objv, (char *)editPtr, editPtr->mask | flags) != TCL_OK) {
	return TCL_ERROR;
    }
    editPtr->flags |= ICURSOR;
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
    gcValues.foreground = editPtr->selFgColor->pixel;
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
    TextEditor *editPtr = (TextEditor *)dataPtr;

    iconOption.clientData = editPtr;
    Blt_FreeOptions(configSpecs, (char *)editPtr, editPtr->display, 
	editPtr->mask);
    if (editPtr->textGC != NULL) {
	Tk_FreeGC(editPtr->display, editPtr->textGC);
    }
    FreeUndoRecords(editPtr);
    FreeRedoRecords(editPtr);
    DestroyButton(editPtr, &editPtr->clearButton);
    if (editPtr->screenText != NULL) {
	Blt_Free(editPtr->screenText);
    }
    if (editPtr->selectGC != NULL) {
	Tk_FreeGC(editPtr->display, editPtr->selectGC);
    }
    if (editPtr->insertGC != NULL) {
	Tk_FreeGC(editPtr->display, editPtr->insertGC);
    }
    if (editPtr->insertTimerToken != NULL) {
	Tcl_DeleteTimerHandler(editPtr->insertTimerToken);
    }
    if (editPtr->menuWin != NULL) {
	Tk_DeleteEventHandler(editPtr->menuWin, CHILD_EVENT_MASK, 
		ChildEventProc, editPtr);
    }
    if (editPtr->tkwin != NULL) {
	Tk_DeleteSelHandler(editPtr->tkwin, XA_PRIMARY, XA_STRING);
	Tk_DeleteEventHandler(editPtr->tkwin, EVENT_MASK, 
		EditorEventProc, editPtr);
    }
    if (editPtr->insertTimerToken != NULL) {
	Tcl_DeleteTimerHandler(editPtr->insertTimerToken);
    }
    Tcl_DeleteCommandFromToken(editPtr->interp, editPtr->cmdToken);
    Blt_Free(editPtr);
}

static int
NewEditor(Tcl_Interp *interp, Tk_Window tkwin)
{
    TextEditor *editPtr;

#ifdef notdef
    Tk_MakeWindowExist(tkwin);
#endif
    Tk_SetClass(tkwin, "BltTreeViewEditor"); 
    editPtr = Blt_AssertCalloc(1, sizeof(TextEditor));

    editPtr->interp = interp;
    editPtr->display = Tk_Display(tkwin);
    editPtr->tkwin = tkwin;
    editPtr->borderWidth = 1;
    editPtr->relief = TK_RELIEF_SOLID;
    editPtr->selRelief = TK_RELIEF_FLAT;
    editPtr->selBW = 1;
    editPtr->selAnchor = -1;
    editPtr->selFirst = editPtr->selLast = -1;
    editPtr->insertOnTime = 600;
    editPtr->insertOffTime = 300;
    editPtr->flags |= ACTIVE;
    editPtr->clearButton.relief = TK_RELIEF_SUNKEN;
    editPtr->clearButton.borderWidth = 1;
    Blt_SetWindowInstanceData(tkwin, editPtr);
    Tk_CreateSelHandler(tkwin, XA_PRIMARY, XA_STRING, SelectionProc, editPtr,
                        XA_STRING);
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask |
	FocusChangeMask, EditorEventProc, editPtr);
    Tcl_CreateObjCommand(interp, Tk_PathName(tkwin), 
	TextEditorCmd, editPtr, NULL);
    if (Blt_ConfigureWidgetFromObj(interp, tkwin, configSpecs, 0, 
	(Tcl_Obj **)NULL, (char *)editPtr, 0) != TCL_OK) {
	Tk_DestroyWindow(tkwin);
	return TCL_ERROR;
    }
    editPtr->insertIndex = 0;
    Tk_MoveResizeWindow(tkwin, editPtr->x, editPtr->y, editPtr->width,
                        editPtr->height);
    Tk_MapWindow(tkwin);
    Tk_MakeWindowExist(tkwin);
    XRaiseWindow(editPtr->display, Tk_WindowId(tkwin));
    EventuallyRedraw(editPtr);
    return TCL_OK;
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
ComputeLayout(TextEditor *editPtr)
{
    Button *butPtr = &editPtr->clearButton;
    unsigned int w, h;
    int reqWidth, reqHeight;
    
    /* Determine the height of the editor.  It's the maximum height of all
     * it's components: icon, label, and clear button. */

    editPtr->iconWidth  = editPtr->iconHeight  = 0;
    editPtr->textWidth  = editPtr->textHeight  = 0;
    editPtr->normalWidth  = editPtr->normalHeight  = 0;
    butPtr->width = butPtr->height = 0;
    editPtr->width = editPtr->height = 0;

    if (editPtr->icon != NULL) {
	editPtr->iconWidth  = IconWidth(editPtr->icon) + IPAD;
	editPtr->iconHeight = IconHeight(editPtr->icon) + 2 * YPAD;
    }
    if (editPtr->prefIconWidth > 0) {
	editPtr->iconWidth = editPtr->prefIconWidth + IPAD;
    }
    editPtr->width += editPtr->iconWidth;
    if (editPtr->height < editPtr->iconHeight) {
	editPtr->height = editPtr->iconHeight;
    }

    if (editPtr->flags & GEOMETRY) {
        ComputeGeometry(editPtr);
    }

    if (editPtr->prefTextWidth > 0) {
        w = Blt_TextWidth(editPtr->font, "0", 1);
        editPtr->width += editPtr->prefTextWidth * w;
    } else {
        editPtr->width += editPtr->worldWidth;
    }
    editPtr->width += IPAD;

    if (editPtr->height < editPtr->worldHeight) {
	editPtr->height = editPtr->worldHeight;
    }
    if (editPtr->flags & CLRBUTTON) {
	Button *butPtr = &editPtr->clearButton;

	butPtr->height = BUTTON_HEIGHT;
	butPtr->width = BUTTON_WIDTH;
	butPtr->width  += 2 * (butPtr->borderWidth + butPtr->pad);
	butPtr->height += 2 * (butPtr->borderWidth + butPtr->pad);
	if (butPtr->height > editPtr->height) {
	    editPtr->height = butPtr->height;
	}
	editPtr->width += butPtr->width;
    }
    editPtr->width  += 2 * editPtr->borderWidth + IPAD;
    editPtr->height += 2 * (editPtr->borderWidth + YPAD);

    /* Figure out the requested size of the widget.  This will also tell us
     * if we need scrollbars. */
 
    reqWidth  = editPtr->worldWidth  + 2 * editPtr->borderWidth;
    reqHeight = editPtr->worldHeight + 2 * editPtr->borderWidth;

    w = GetBoundedWidth(editPtr, reqWidth);
    h = GetBoundedHeight(editPtr, reqHeight);

    if ((reqWidth > w) && (editPtr->xScrollbar != NULL)) {
	editPtr->xScrollbarHeight = Tk_ReqHeight(editPtr->xScrollbar);
	h = GetBoundedHeight(editPtr, reqHeight + editPtr->xScrollbarHeight);
    }
    if ((reqHeight > h) && (editPtr->yScrollbar != NULL)) {
	editPtr->yScrollbarWidth = Tk_ReqWidth(editPtr->yScrollbar);
	w = GetBoundedWidth(editPtr, reqWidth + editPtr->yScrollbarWidth);
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
    editPtr->flags &= ~LAYOUT_PENDING;

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
DrawTextLine(TextEditor *editPtr, Drawable drawable, TextLine *linePtr,
             int w, int h) 
{
    Blt_FontMetrics fm;
    Pixmap pixmap;
    int insertX;
    int textX, textY;
    Blt_Bg bg;
    GC gc;
    ByteOffset insertOffset, firstOffset, lastOffset;
    int numPixels;
    int x, y;
    int textOffset, xOffset, textWidth;
    
    if ((h < 2) || (w < 2)) {
	return;
    }
    if (editPtr->textHeight <= 0) {
	return;
    }
    if (h > editPtr->entryHeight) {
	h = editPtr->entryHeight;
    }
    Blt_Font_GetMetrics(editPtr->font, &fm);
    textY = fm.ascent;
    if (editPtr->entryHeight > editPtr->textHeight) {
	textY += (editPtr->entryHeight - editPtr->textHeight) / 2;
    }

#ifdef WIN32
    assert(drawable != None);
#endif
    /*
     * Create a pixmap the size of visible text area. This will be used for
     * clipping the scrolled text string.
     */
    pixmap = Blt_GetPixmap(editPtr->display, Tk_WindowId(editPtr->tkwin),
	w, h, Tk_Depth(editPtr->tkwin));

    bg = editPtr->textBg;
    gc = editPtr->textGC;
    x = linePtr->worldX - editPtr->xOffset;
    y = linePtr->worldY - editPtr->yOffset;
    
    /* Text background. */
    { 
	int xOrigin, yOrigin;

	Blt_Bg_GetOrigin(bg, &xOrigin, &yOrigin);
	Blt_Bg_SetOrigin(editPtr->tkwin, bg, xOrigin + x, yOrigin + y);
	Blt_Bg_FillRectangle(editPtr->tkwin, pixmap, bg, 0, 0, w, h, 
		0, TK_RELIEF_FLAT);
	Blt_Bg_SetOrigin(editPtr->tkwin, bg, xOrigin, yOrigin);
    }	

    /* 
     * The viewport starts somewhere over the first visible character, but
     * not necessarily at the start of the character.  Subtract the
     * viewport offset from the start of the first character.  This is zero
     * or a negative x-coordinate, indicating where start drawing the text
     * so that it's properly clipped by the temporary pixmap. */

    insertX = -1;

    /* Find the start and end of the line as text offsets. */
    textOffset = Blt_Font_Measure(editPtr->font, linePtr->text,
        linePtr->numBytes, linePtr->worldX + editPtr->xOffset, TEXT_FLAGS,
        &numPixels);
    xOffset = editPtr->xOffset - numPixels; /* Pixel offset to the first
                                             * character */
    textX      = x - xOffset;
    textWidth  = w - xOffset;
    if (((editPtr->flags & (FOCUS|ICURSOR_ON)) == (FOCUS|ICURSOR_ON)) &&
        (editPtr->selFirst == -1) &&
        (editPtr->insertIndex >= linePtr->char1) && 
	(editPtr->insertIndex < linePtr->char2)) {
        int numBytes;

	insertX = textX;
        /* The insertion cursor in on this line. Get the pixel offset to
         * the cursor by measuring the text to the location of the cursor.
         * First convert the cursor index to a byte offset. */
        numBytes = CharIndexToByteOffset(linePtr->text, 
                editPtr->insertIndex - linePtr->char1);
        insertX += Blt_TextWidth(editPtr->font, linePtr->text, numBytes);
	if (insertX > textWidth) {
	    insertX = -1;
	}
    }

    /*
     *	Text is drawn in (up to) three segments.
     *
     *	  1) Any text before the start the selection.  2) The selected text
     *	  (drawn with a flat border) 3) Any text following the selection.
     *	  This step will draw the text string if there is no selection.
     */

    selFirstByteOffset = CharIndexToByteOffset(editPtr->screenText,
        editPtr->selFirst);
    selLastByteOffset  = CharIndexToByteOffset(editPtr->screenText,
        editPtr->selLast);

    /* Step 1. Draw any text preceding the selection that's still visible
     *         in the viewport. */
    if ((textWidth > 0) && (textOffset < linePtr->offset2) &&
        (editPtr->selFirst >= linePtr->char1)) {
	int numPixels, len, numBytes;
	ByteOffset offset;

	offset = linePtr->numBytes;
	if (editPtr->selLast < linePtr->char2) {
            offset = CharIndexToByteOffset(editPtr->screenText,
                editPtr->selLast);
	}
        numBytes = offset - textOffset;
	numBytes = Blt_Font_Measure(editPtr->font, linePtr->text + textOffset,
                numBytes, textWidth, TEXT_FLAGS, &numPixels);
	Blt_Font_Draw(editPtr->display, pixmap, gc, editPtr->font, 
		Tk_Depth(editPtr->tkwin), 0.0f, 
		linePtr->text + textOffset, numBytes, textX, textY);
	textX += numPixels;
        textWidth -= numPixels;
        textOffset += numBytes;
    }
    /* Step 2. Draw the selection itself, if it's visible in the
     *         viewport. Otherwise step 1 drew as much as we need. */
    if ((textWidth > 0) && (textOffset < linePtr->offset2) &&
        (editPtr->selFirst < linePtr->char1)) {	
	Blt_Bg bg;
	int numBytes, numPixels;

	/* The background of the selection rectangle is different depending
	 * whether the widget has focus or not. */
	bg = editPtr->selectBg;
        numBytes = linePtr->offset2 - textOffset;
	numBytes = Blt_Font_Measure(editPtr->font, 
		editPtr->screenText + textOffset, numBytes, textWidth, 
                TEXT_FLAGS, &numPixels);
	Blt_Bg_FillRectangle(editPtr->tkwin, pixmap, bg, textX, 0, 
		numPixels, h, 0, TK_RELIEF_FLAT);
	Blt_Font_Draw(editPtr->display, pixmap, editPtr->selectGC, 
		editPtr->font, Tk_Depth(editPtr->tkwin), 0.0f, 
		editPtr->screenText + textOffset, numBytes, textX, textY);
	textX += numPixels;
        textWidth -= numPixels;
        textOffset += numBytes;
    }
    /* Step 3.  Draw any text following the selection that's visible
     *		in the viewport. In the case of no selection, we draw
     *		the entire text string. */
    if ((textWidth > 0) && (textOffset < linePtr->offset2)) {
	ByteOffset offset;

	offset = lastOffset;
	if (offset < editPtr->firstOffset) {
	    offset = editPtr->firstOffset;
	}
        numBytes = linePtr->endOffset - textOffset;
	numBytes = Blt_Font_Measure(editPtr->font, 
		editPtr->screenText + textOffset, numBytes, textWidth, 
                TEXT_FLAGS, &numPixels);
	Blt_Font_Draw(editPtr->display, pixmap, gc, 
		editPtr->font, Tk_Depth(editPtr->tkwin), 0.0f, 
		editPtr->screenText + textOffset, numBytes, 
		textX, textY);
    }
    /* Draw the insertion cursor, if one is needed. */
    if (insertX >= 0) {
	int y1, y2;

	y1 = 1;
	y2 = h - 2;
	XDrawLine(editPtr->display, pixmap, editPtr->insertGC, insertX, y1, 
		insertX, y2);
#ifdef notdef
	XDrawLine(editPtr->display, pixmap, editPtr->insertGC, insertX + 1, 
		y1, insertX + 1, y2);
#endif
    }
    XCopyArea(editPtr->display, pixmap, drawable, gc, 0, 0, w, h, 
	x, y);
    Tk_FreePixmap(editPtr->display, pixmap);
}

static void
DrawTextArea(TextEditor *editPtr, Drawable drawable, TextLine *linePtr,
          int x, int y, int w, int h) 
{
    Pixmap pixmap;
    Blt_Bg bg;
    GC gc;
    TextLine *linePtr, *endPtr;
    
    bg = editPtr->textBg;
    gc = editPtr->textGC;
    w = VPORTWIDTH(editPtr);
    h = VPORTHEIGHT(editPtr);
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
    /* Find the starting line */
    for (linePtr = editPtr->lines, endPtr = editPtr->line + editPtr->numLines;
         linePtr < endPtr; linePtr) {
        int sy;

        sy = SCREENY(editPtr, linePtr->worldY);
        if ((sy >= y) && (sy < (y + h))) {
            break;                      /* This is the first line is in the
                                         * viewport. */
        }
    }
    for (/*empty*/; linePtr < endPtr; linePtr++) {
        int sy;

        sy = SCREENY(editPtr, linePtr->worldY);
        if (sy >= (y + h)) {
            break;                      /* This line starts beyond the end
                                         * of the viewport. */
        }
        DrawTextLine(editPtr, pixmap, linePtr, w, h);
    }
    XCopyArea(editPtr->display, pixmap, drawable, gc, 0, 0, w, h, x, y);
    Tk_FreePixmap(editPtr->display, pixmap);
}

static void
DisplayProc(ClientData clientData)
{
    TextEditor *editPtr = clientData;
    Pixmap drawable;
    int x, y, w, h;

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
	ComputeGeometry(editPtr);
    }
    if ((Tk_Width(editPtr->tkwin) <= 1) || (Tk_Height(editPtr->tkwin) <= 1)) {
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
	editPtr->flags &= ~SCROLL_PENDING;
    }
    /*
     * Create a pixmap the size of the window for double buffering.
     */
    w = Tk_Width(editPtr->tkwin);
    h = Tk_Height(editPtr->tkwin);
    Blt_SizeOfScreen(editPtr->tkwin, &screenWidth, &screenHeight);
    w = CLAMP(w, 1, screenWidth);
    h = CLAMP(h, 1, screenHeight);

    /* Create pixmap the size of the widget. */
    drawable = Blt_GetPixmap(editPtr->display, Tk_WindowId(editPtr->tkwin), 
	w, h, Tk_Depth(editPtr->tkwin));
    Blt_Fill3DRectangle(editPtr->tkwin, drawable, editPtr->border, 0, 0,
	w, h, editPtr->borderWidth, editPtr->relief);

    x = editPtr->borderWidth + editPtr->gap;
    y = editPtr->borderWidth;

    /* Draw the icon on the left side of the text area. */
    if (editPtr->icon != NULL) {
	y += (editPtr->height - IconHeight(editPtr->icon)) / 2;
	Tk_RedrawImage(IconBits(editPtr->icon), 0, 0, 
		       IconWidth(editPtr->icon), 
		       IconHeight(editPtr->icon), 
		       drawable, x, y);
	x += IconWidth(editPtr->icon) + editPtr->gap;
    }
    DrawTextArea(editPtr, drawable, x, y, w, h);
    
    /* Draw the close button on the right side of the text area. */
    
    XCopyArea(editPtr->display, drawable, Tk_WindowId(editPtr->tkwin),
	editPtr->gc, 0, 0, Tk_Width(editPtr->tkwin), 
	Tk_Height(editPtr->tkwin), 0, 0);
    Tk_FreePixmap(editPtr->display, drawable);
}

/*ARGSUSED*/
static int
ApplyOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    TextEditor *editPtr = clientData;
    int result;

    return result;
}

/*ARGSUSED*/
static int
CancelOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    TextEditor *editPtr = clientData;

    Tk_DestroyWindow(editPtr->tkwin);
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
    TextEditor *editPtr = clientData;

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
    TextEditor *editPtr = clientData;

    if (objc == 2) {
	return Blt_ConfigureInfoFromObj(interp, editPtr->tkwin, configSpecs,
                (char *)editPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, editPtr->tkwin, configSpecs,
                (char *)editPtr, objv[3], 0);
    }
    if (Blt_ConfigureWidgetFromObj(interp, editPtr->tkwin, configSpecs,
        objc - 2, objv + 2, (char *)editPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    ConfigureTextEditor(editPtr);
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
    TextEditor *editPtr = clientData;
    CharIndex first, last;

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
    if (!DeleteText(editPtr, first, last)) {
        Tcl_AppendResult(interp, "can't delete text", (char *)NULL);
        return TCL_ERROR;
    }
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
    TextEditor *editPtr = clientData;
    CharIndex index;

    if (GetIndexFromObj(interp, editPtr, objv[2], &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((index < 0) || (index > editPtr->numChars)) {
        Tcl_AppendResult(interp, "invalid index \"", Tcl_GetString(objv[2]),
                         "\"", (char *)NULL);
        return TCL_ERROR;
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
    TextEditor *editPtr = clientData;
    int textPos;

    if (GetIndexFromObj(interp, editPtr, objv[2], &textPos) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((editPtr->colPtr != NULL) && (editPtr->string != NULL)) {
	int numChars;

	numChars = Tcl_NumUtfChars(editPtr->string, textPos);
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
 *	New information gets added to editPtr; it will be redisplayed soon,
 *	but not necessarily immediately.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InsertOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    TextEditor *editPtr = clientData;
    int length;
    CharIndex index;
    const char *string;

    if (GetIndexFromObj(interp, editPtr, objv[2], &index) != TCL_OK) {
	return TCL_ERROR;
    }
    string = Tcl_GetStringFromObj(objv[3], &length);
    if (length == 0) {                   /* Nothing to insert. Move the
					 * cursor anyways. */
	editPtr->insertIndex = index;
    } else {
	InsertText(editPtr, string, length, index);
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
    TextEditor *editPtr = clientData;
    int x, y;

    memset(&editPtr->post, 0, sizeof(PostInfo));
    editPtr->post.tkwin     = Tk_Parent(editPtr->tkwin);
    editPtr->post.editorWidth = editPtr->normalWidth;
    /* Process switches  */
    if (Blt_ParseSwitches(interp, postSwitches, objc - 2, objv + 2, editPtr,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
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
	ComputeEditorGeometry(editPtr);
    }
    editPtr->post.lastEditorWidth = editPtr->post.editorWidth;
    x = 0;                              /* Suppress compiler warning; */
    y = editPtr->post.y2;
    switch (editPtr->post.align) {
    case ALIGN_LEFT:
	x = editPtr->post.x1;
	break;
    case ALIGN_CENTER:
	{
	    int w;

	    w = editPtr->post.x2 - editPtr->post.x1;
	    x = editPtr->post.x1 + (w - editPtr->normalWidth) / 2; 
	}
	break;
    case ALIGN_RIGHT:
	x = editPtr->post.x2 - editPtr->normalWidth;
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
	    ComputeEditorGeometry(editPtr);
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
    if (editPtr->activePtr == NULL) {
	ActivateItem(editPtr, editPtr->firstPtr);
    }
    editPtr->flags |= POSTED;
    return TCL_OK;
}

/*ARGSUSED*/
static int
SelectionAdjustOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv)
{
    TextEditor *editPtr = clientData;
    int textPos;
    int half1, half2;

    if (GetIndexFromObj(interp, editPtr, objv[3], &textPos) != TCL_OK) {
	return TCL_ERROR;
    }
    half1 = (editPtr->selFirst + editPtr->selLast) / 2;
    half2 = (editPtr->selFirst + editPtr->selLast + 1) / 2;
    if (textPos < half1) {
	editPtr->selAnchor = editPtr->selLast;
    } else if (textPos > half2) {
	editPtr->selAnchor = editPtr->selFirst;
    }
    return SelectText(editPtr, textPos);
}

/*ARGSUSED*/
static int
SelectionClearOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *const *objv)
{
    TextEditor *editPtr = clientData;

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
    TextEditor *editPtr = clientData;
    int textPos;

    if (GetIndexFromObj(interp, editPtr, objv[3], &textPos) != TCL_OK) {
	return TCL_ERROR;
    }
    editPtr->selAnchor = textPos;
    return TCL_OK;
}

/*ARGSUSED*/
static int
SelectionPresentOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		   Tcl_Obj *const *objv)
{
    TextEditor *editPtr = clientData;
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
    TextEditor *editPtr = clientData;
    int selFirst, selLast;

    if (GetIndexFromObj(interp, editPtr, objv[3], &selFirst) != TCL_OK) {
	return TCL_ERROR;
    }
    if (GetIndexFromObj(interp, editPtr, objv[4], &selLast) != TCL_OK) {
	return TCL_ERROR;
    }
    editPtr->selAnchor = selFirst;
    SelectText(editPtr, selLast);
    return TCL_OK;
}

/*ARGSUSED*/
static int
SelectionToOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    TextEditor *editPtr = clientData;
    int textPos;

    if (GetIndexFromObj(interp, editPtr, objv[3], &textPos) != TCL_OK) {
	return TCL_ERROR;
    }
    return SelectText(editPtr, textPos);
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
    TextEditor *editPtr = clientData;

    if (!WithdrawMenu(editPtr)) {
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
    TextEditor *editPtr = clientData;

    WithdrawMenu(editPtr);
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
    TextEditor *editPtr = clientData;
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
	fract = (double)editPtr->xOffset / (editPtr->worldWidth+1);
	objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	fract = (double)(editPtr->xOffset + w) / (editPtr->worldWidth+1);
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
    TextEditor *editPtr = clientData;
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
	fract = (double)editPtr->yOffset / (editPtr->worldHeight+1);
	objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	fract = (double)(editPtr->yOffset + height) /(editPtr->worldHeight+1);
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
    {"apply",     1, ApplyOp,     2, 2, "",},
    {"cancel",    2, CancelOp,    2, 2, "",},
    {"cget",      2, CgetOp,      3, 3, "value",},
    {"configure", 2, ConfigureOp, 2, 0, "?option value...?",},
    {"delete",    1, DeleteOp,    3, 4, "firstIndex ?lastIndex?"},
    {"icursor",   2, IcursorOp,   3, 3, "index"},
    {"index",     3, IndexOp,     3, 3, "index"},
    {"insert",    3, InsertOp,    4, 4, "index string"},
    {"invoke",    3, InvokeOp,    3, 3, "",},
    {"post",      4, PostOp,      2, 0, "switches...",},
    {"selection", 1, SelectionOp, 2, 0, "args"},
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
    TextEditor *editPtr = clientData;   /* Pointer to widget record. */

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
ComboEditorCmd(
    ClientData clientData,              /* Main window associated with
					 * interpreter. */
    Tcl_Interp *interp,                 /* Current interpreter. */
    int objc,                           /* Number of arguments. */
    Tcl_Obj *const *objv)               /* Argument strings. */
{
    TextEditor *editPtr;
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
    if (!Blt_CommandExists(interp, "::blt::ComboEditor::PostCascade")) {
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
    editPtr->flags |= COMBOMENU;
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
    Tk_ChangeWindowAttributes(tkwin, mask, &attrs);

    Tk_MakeWindowExist(tkwin);
    Tcl_SetObjResult(interp, objv[1]);
    return TCL_OK;
}

int
Blt_ComboEditorInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {
        "comboeditor", ComboEditorCmd
    };
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

#endif


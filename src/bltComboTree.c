/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltComboTree.c --
 *
 * This module implements a combotree widget for the BLT toolkit.
 *
 *	Copyright 1998-2004 George A Howlett.
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
 * TODO:
 *
 * BUGS:
 *   1.  "open" operation should change scroll offset so that as many
 *	 new entries (up to half a screen) can be seen.
 *   2.  "open" needs to adjust the scrolloffset so that the same entry
 *	 is seen at the same place.
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_COMBOTREE

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif	/* HAVE_LIMITS_H */

#include "bltAlloc.h"
#include "bltHash.h"
#include "bltList.h"
#include "bltChain.h"
#include "bltTree.h"
#include "bltFont.h"
#include "bltText.h"
#include "bltImage.h"
#include "bltBind.h"
#include "bltBg.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define PICK_ENTRY		(ClientData)0
#define PICK_BUTTON		(ClientData)1

#if HAVE_UTF
#else
#define Tcl_NumUtfChars(s,n)	(((n) == -1) ? strlen((s)) : (n))
#define Tcl_UtfAtIndex(s,i)	((s) + (i))
#endif

#define ODD(x)			((x) | 0x01)

#define END			(-1)
#define SEPARATOR_LIST		((char *)NULL)
#define SEPARATOR_NONE		((char *)-1)

#define SEARCH_Y		1

#define ARROW_WIDTH 17
#define ARROW_HEIGHT 17

typedef const char *UID;

#define FCLAMP(x)	((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))
#define CHOOSE(default, override)	\
	(((override) == NULL) ? (default) : (override))

#define GETLABEL(e)		\
	(((e)->labelUid != NULL) ? (e)->labelUid : Blt_Tree_NodeLabel((e)->node))

/*
 * The macro below is used to modify a "char" value (e.g. by casting it to an
 * unsigned character) so that it can be used safely with macros such as
 * isspace.
 */
#define UCHAR(c)	((unsigned char) (c))

#define SCREENX(c, wx)	((wx) - (c)->xOffset + (c)->borderWidth)
#define SCREENY(c, wy)	((wy) - (c)->yOffset + (c)->borderWidth)

#define PIXMAPX(c, wx)	((wx) - (c)->xOffset)
#define PIXMAPY(c, wy)	((wy) - (c)->yOffset)

#define WORLDX(c, sx)	((sx) - (c)->borderWidth + (c)->xOffset)
#define WORLDY(c, sy)	((sy) - (c)->borderWidth + (c)->yOffset)

#define VPORTWIDTH(c)	\
    (Tk_Width((c)->tkwin) - 2 * (c)->borderWidth - (c)->yScrollbarWidth)
#define VPORTHEIGHT(c) \
    (Tk_Height((c)->tkwin) - 2 * (c)->borderWidth - (c)->xScrollbarHeight)

#define ICONWIDTH(d)	(comboPtr->levelInfo[(d)].iconWidth)
#define LEVELX(d)	(comboPtr->levelInfo[(d)].x)
#define DEPTH(h, n)	Blt_Tree_NodeDepth(n)

/*
 *---------------------------------------------------------------------------
 *
 *  Internal combotree widget flags:
 *
 *	LAYOUT_PENDING	The layout of the hierarchy needs to be recomputed.
 *
 *	REDRAW_PENDING	A redraw request is pending for the widget.
 *
 *	XSCROLL		X-scroll request is pending.
 *
 *	SCROLLY		Y-scroll request is pending.
 *
 *	SCROLL_PENDING	Both X-scroll and  Y-scroll requests are pending.
 *
 *	FOCUS		The widget is receiving keyboard events.
 *			Draw the focus highlight border around the widget.
 *
 *	DIRTY		The hierarchy has changed. It may invalidate
 *			the locations and pointers to entries.  The widget 
 *			will need to recompute its layout.
 *
 *	VIEWPORT	Indicates that the viewport has changed in some
 *			way: the size of the viewport, the location of 
 *			the viewport, or the contents of the viewport.
 *
 */

#define REDRAW_PENDING		(1<<0)	/* Indicates that the widget will be
					 * redisplayed at the next idle
					 * point. */
#define LAYOUT_PENDING		(1<<1)	/* Indicates that the widget's layout
					 * is scheduled to be recomputed at
					 * the next redraw. */
#define UPDATE_PENDING		(1<<2)	/* Indicates that a component (window
					 * or scrollbar) has changed and that
					 * and update is pending.  */
#define FOCUS			(1<<3)	/* Indicates that the combomenu
					 * currently has focus. */
#define DROPDOWN		(1<<4)	/* Indicates the combomenu is a drop
					 * down menu as opposed to a
					 * popup.  */
#define SCROLLX			(1<<5)
#define SCROLLY			(1<<6)
#define SCROLL_PENDING		(SCROLLX|SCROLLY)

#define INSTALL_XSCROLLBAR	(1<<8)	/* Indicates that the x scrollbar is
					 * scheduled to be installed at the
					 * next idle point. */
#define INSTALL_YSCROLLBAR	(1<<9)	/* Indicates that the y scrollbar is
					 * scheduled to be installed at the
					 * next idle point. */
#define RESTRICT_MIN		(1<<10)	/* Indicates to constrain the width of
					 * the menu to the minimum size of the
					 * parent widget that posted the
					 * menu. */
#define RESTRICT_MAX		(1<<11)	/* Indicates to constrain the width of
					 * the menu of the maximum size of the
					 * parent widget that posted the
					 * menu. */
#define DIRTY			(1<<12)
#define VIEWPORT		(1<<13)
#define REPOPULATE		(1<<14)

/*
 *  Miscellaneous flags:
 *
 *	HIDE_ROOT		Don't display the root entry.
 *
 *	HIDE_LEAVES		Don't display entries that are leaves.
 *
 *	NEW_TAGS		
 */
#define HIDE_ROOT		(1<<23) 
#define HIDE_LEAVES		(1<<24) 
#define NEW_TAGS		(1<<27)

/*
 *---------------------------------------------------------------------------
 *
 *  Internal entry flags:
 *
 *	ENTRY_BUTTON		Indicates that a button is needed
 *				for this entry.
 *
 *	ENTRY_CLOSED		Indicates that the entry is closed and
 *				its subentries are not displayed.
 *
 *	ENTRY_HIDE		Indicates that the entry is hidden (i.e.
 *				can not be viewed by opening or scrolling).
 *
 *	ENTRY_BTN_AUTO
 *	ENTRY_BTN_SHOW
 *	ENTRY_BTN_MASK
 *
 *---------------------------------------------------------------------------
 */
#define ENTRY_CLOSED		(1<<0)
#define ENTRY_HIDE		(1<<1)
#define ENTRY_MASK		(ENTRY_CLOSED | ENTRY_HIDE)
#define ENTRY_NOT_LEAF		(1<<2)

#define ENTRY_BUTTON		(1<<3)
#define ENTRY_ICON		(1<<4)
#define ENTRY_REDRAW		(1<<5)
#define ENTRY_LAYOUT_PENDING	(1<<6)
#define ENTRY_DATA_CHANGED	(1<<7)
#define ENTRY_DIRTY		(ENTRY_DATA_CHANGED | ENTRY_LAYOUT_PENDING)

#define ENTRY_BTN_AUTO		(1<<8)
#define ENTRY_BTN_SHOW		(1<<9)
#define ENTRY_BTN_MASK		(ENTRY_BTN_AUTO | ENTRY_BTN_SHOW)

#define ENTRY_EDITABLE		(1<<10)
#define DELETED			(1<<11)

#define COLUMN_RULE_PICKED	(1<<1)
#define COLUMN_DIRTY		(1<<2)

#define STYLE_TEXTBOX		(0)
#define STYLE_COMBOBOX		(1)
#define STYLE_CHECKBOX		(2)
#define STYLE_TYPE		0x3

#define STYLE_LAYOUT		(1<<3)
#define STYLE_DIRTY		(1<<4)
#define STYLE_HIGHLIGHT		(1<<5)
#define STYLE_USER		(1<<6)

#define STYLE_EDITABLE		(1<<10)

typedef struct _Entry Entry;
typedef struct _ComboTree ComboTree;
typedef struct _Style Style;

typedef int (CompareProc)(Tcl_Interp *interp, const char *name, 
	const char *pattern);

typedef Entry *(IterProc)(Entry *entryPtr, unsigned int mask);

/*
 * Icon --
 *
 *	Since instances of the same Tk image can be displayed in
 *	different windows with possibly different color palettes, Tk
 *	internally stores each instance in a linked list.  But if
 *	the instances are used in the same widget and therefore use
 *	the same color palette, this adds a lot of overhead,
 *	especially when deleting instances from the linked list.
 *
 *	For the combotree widget, we never need more than a single
 *	instance of an image, regardless of how many times it's used.
 *	Cache the image, maintaining a reference count for each
 *	image used in the widget.  It's likely that the combotree
 *	widget will use many instances of the same image (for example
 *	the open/close icons).
 */

typedef struct _Icon {
    Tk_Image tkImage;		/* The Tk image being cached. */

    int refCount;		/* Reference count for this image. */

    short int width, height;	/* Dimensions of the cached image. */

    Blt_HashEntry *hashPtr;	/* Hash table pointer to the image. */

} *Icon;

#define IconHeight(icon)	((icon)->height)
#define IconWidth(icon)		((icon)->width)
#define IconImage(icon)		((icon)->tkImage)
#define IconName(icon)		(Blt_Image_Name((icon)->tkImage))

struct _Style {
    const char *name;			/* Instance name. */
    Blt_HashEntry *hPtr;
    ComboTree *comboPtr;
    int refCount;			/* Indicates if the style is currently
					 * in use in the combotree. */

    unsigned int flags;			/* Bit field containing both the style
					 * type and various flags. */

    /* General style fields. */

    int borderWidth;			/* Width of 3D border. */
    int activeRelief;
    int relief;

    int gap;				/* # pixels gap between icon and
					 * text. */
    Blt_Font labelFont;
    XColor *labelNormalColor;		/* Normal foreground color of cell. */
    XColor *labelActiveColor;		/* Foreground color of cell when
					 * active. */

    Blt_Bg normalBg;		/* Normal background color. */
    Blt_Bg altBg;		/* Alternate normal background
					 * color. */
    Blt_Bg activeBg;		/* Active entry background color. */
    Blt_Bg disabledBg;		/* Disabled entry background color. */

    GC labelNormalGC;
    GC labelActiveGC;
    GC labelDisabledGC;

    Icon *icons;			/* Tk images displayed for the entry.
					 * The first image is the icon
					 * displayed to the left of the
					 * entry's label. The second is icon
					 * displayed when entry is "open". */
};

/*
 * Entry --
 *
 *	Contains data-specific information how to represent the data
 *	of a node of the hierarchy.
 *
 */
struct _Entry {
    Blt_TreeNode node;			/* Node containing entry */
    int worldX, worldY;			/* X-Y position in world coordinates
					 * where the entry is positioned. */
    Blt_HashEntry *hPtr;
    short int width, height;		/* Dimensions of the entry. This
					 * includes the size of its
					 * columns. */
    int reqHeight;			/* Requested height of the entry.
					 * Overrides computed height. */
    int vertLineLength;			/* Length of the vertical line
					 * segment. */
    short int lineHeight;		/* Height of first line of text. */
    unsigned short int flags;		/* Flags for this entry. For the
					 * definitions of the various bit
					 * fields see below. */

    Tcl_Obj *tagsObjPtr;		/* List of binding tags for this
					 * entry. */
    ComboTree *comboPtr;
    Tcl_Obj *cmdObjPtr;			/* List of binding tags for this
					 * entry. */
    Tcl_Obj *openCmdObjPtr;		/* TCL command to invoke when entries
					 * are opened. This overrides
					 * the global option. */
    Tcl_Obj *closeCmdObjPtr;		/* TCL command to invoke when entries
					 * are closed. This overrides
					 * the global option. */
    /*
     * Button information:
     */
    short int buttonX, buttonY;		/* X-Y coordinate offsets from to
					 * upper left corner of the entry to
					 * the upper-left corner of the
					 * button.  Used to pick the button
					 * quickly */

    short int iconWidth, iconHeight;	/* Maximum dimensions for icons and
					 * buttons for this entry. This is
					 * used to align the button, icon, and
					 * text. */
    /*
     * Label information:
     */
    TextLayout *textPtr;
    short int labelWidth, labelHeight;
    UID labelUid;		        /* Text displayed right of the
					 * icon. */
    int seqNum;				/* Used to navigate to next/last entry
					 * when the view is flat. */
    Style *stylePtr;			/* Default style for entry. */

};

/*
 * Button --
 *
 *	A button is the open/close indicator at the far left of the entry.  It
 *	is displayed as a plus or minus in a solid colored box with optionally
 *	an border. It has both "active" and "normal" colors.
 */
typedef struct {
    XColor *fgColor;		/* Foreground color. */
    XColor *activeFgColor;	/* Active foreground color. */

    Blt_Bg normalBg;	/* Normal button background. */
    Blt_Bg activeBg;	/* Active background color. */

    GC normalGC;
    GC activeGC;

    int reqSize;

    int borderWidth;

    int openRelief, closeRelief;

    int width, height;

    Icon *icons;

} Button;

/*
 * LevelInfo --
 *
 */
typedef struct {
    int x;
    int iconWidth;
    int labelWidth;
} LevelInfo;

/*
 * ComboTree --
 *
 *	A ComboTree is a widget that displays an hierarchical table of one or
 *	more entries.
 *
 *	Entries are positioned in "world" coordinates, referring to the
 *	virtual combotree.  Coordinate 0,0 is the upper-left corner of the root
 *	entry and the bottom is the end of the last entry.  The widget's Tk
 *	window acts as view port into this virtual space. The combotree's
 *	xOffset and yOffset fields specify the location of the view port in
 *	the virtual world.  Scrolling the viewport is therefore simply
 *	changing the xOffset and/or yOffset fields and redrawing.
 *
 *	Note that world coordinates are integers, not signed short integers
 *	like X11 screen coordinates.  It's very easy to create a hierarchy
 *	taller than 0x7FFF pixels.
 */
struct _ComboTree {

    /*
     * This works around a bug in the Tk API.  Under under Win32, Tk tries to
     * read the widget record of toplevel windows (TopLevel or Frame widget),
     * to get its menu name field.  What this means is that we must carefully
     * arrange the fields of this widget so that the menuName field is at the
     * same offset in the structure.
     */
    Tk_Window tkwin;			/* Window that embodies the frame.
					 * NULL means that the window has been
					 * destroyed but the data structures
					 * haven't yet been cleaned up. */
    Display *display;			/* Display containing widget.  Used,
					 * among other things, so that
					 * resources can be freed even after
					 * tkwin has gone away. */
    Tcl_Interp *interp;			/* Interpreter associated with widget.
					 * Used to delete widget command. */
    Tcl_Command cmdToken;		/* Token for widget's command. */
    Tcl_Obj *postCmdObjPtr;		/* If non-NULL, command to be executed
					 * when this menu is posted. */
    unsigned int flags;			/* For bitfield definitions, see
					 * below */
    Tcl_Obj *iconVarObjPtr;		/* Name of TCL variable.  If non-NULL,
					 * this variable will be set to the
					 * name of the Tk image representing
					 * the icon of the selected item.  */
    Tcl_Obj *textVarObjPtr;		/* Name of TCL variable.  If non-NULL,
					 * this variable will be set to the
					 * text string of the label of the
					 * selected item. */
    Tcl_Obj *takeFocusObjPtr;		/* Value of -takefocus option; not
					 * used in the C code, but used by
					 * keyboard traversal scripts. */
    const char *menuName;		/* Textual description of menu to use
					 * for menubar. Malloc-ed, may be
					 * NULL. */
    Tk_Cursor cursor;			/* Current cursor for window or
					 * None. */
    /*------*/
    Blt_Tree tree;			/* Handle representing the tree. */
    const char *treeName;
    Blt_HashEntry *hPtr;
    /* ComboTree_ specific fields. */ 
    Blt_HashTable entryTable;		/* Table of entry information, keyed
					 * by the node pointer. */
    int inset;				/* Total width of all borders,
					 * including traversal highlight and
					 * 3-D border.  Indicates how much
					 * interior stuff must be offset from
					 * outside edges to leave room for
					 * borders. */
    Style defStyle;
    int normalWidth, normalHeight;
    
    int borderWidth;			/* Width of 3D border. */
    int relief;				/* 3D border relief. */

    /*
     * Entries are connected by horizontal and vertical lines. They may be
     * drawn dashed or solid.
     */
    int lineWidth;		        /* Width of lines connecting
					 * entries */
    int dashes;				/* Dash on-off value. */
    XColor *lineColor;			/* Color of connecting lines. */

    /*
     * Button Information:
     *
     * The button is the open/close indicator at the far left of the entry.
     * It is usually displayed as a plus or minus in a solid colored box with
     * optionally an border. It has both "active" and "normal" colors.
     */
    Button button;
    int leader;				/* Number of pixels padding between
					 * entries. */
    int reqWidth, reqHeight;		/* Requested dimensions of the
					 * combotree widget's window. */
    GC lineGC;				/* GC for drawing dotted line between
					 * entries. */
    Entry *activePtr;			/* Last active entry. */ 
    Entry *activeBtnPtr;		/* Pointer to last active button */
    Entry *fromPtr;

    /* Names of scrollbars to embed into the widget window. */
    Tcl_Obj *xScrollbarObjPtr, *yScrollbarObjPtr;

    /* Command strings to control horizontal and vertical scrollbars. */
    Tcl_Obj *xScrollCmdObjPtr, *yScrollCmdObjPtr;

    int xScrollUnits, yScrollUnits; /* # of pixels per scroll unit. */

    /*
     * Total size of all "open" entries. This represents the range of world
     * coordinates.
     */
    int worldWidth, worldHeight;

    int xOffset, yOffset;	/* Translation between view port and world
				 * origin. */

    LevelInfo *levelInfo;
    int lastButtonWidth;

    /* Scanning information: */
    int scanAnchorX, scanAnchorY;	/* Scan anchor in screen
					 * coordinates. */
    int scanX, scanY;			/* X-Y world coordinate where the scan
					 * started. */
    Blt_HashTable iconTable;		/* Table of Tk images */
    Blt_HashTable uidTable;		/* Table of strings. */
    Blt_HashTable styleTable;		/* Table of cell styles. */
    Entry *rootPtr;			/* Root entry of tree. */
    Entry **visibleEntries;		/* Array of visible entries */
    int numVisible;			/* Number of entries in the above
					 * array */
    int numEntries;			/* Number of entries in tree. */
    int buttonFlags;			/* Global button indicator for all
					 * entries. This may be overridden by
					 * the entry's -button option. */
    Tcl_Obj *openCmdObjPtr;
    Tcl_Obj *closeCmdObjPtr;		/* TCL commands to invoke when entries
					 * are opened or closed. */
    const char *pathSep;		/* Pathname separators */
    ClientData clientData;
    Blt_BindTable bindTable;		/* Binding information for entries. */
    Blt_HashTable entryBindTagTable;
    Blt_HashTable buttonBindTagTable;

    size_t depth;
    int flatView;			/* Indicates if the view of the tree
					 * has been flattened. */
    Blt_Pool entryPool;
    Tk_Window xScrollbar;		/* Horizontal scrollbar to be used if
					 * necessary. If NULL, no x-scrollbar
					 * is used. */
    Tk_Window yScrollbar;		/* Vertical scrollbar to be used if
					 * necessary. If NULL, no y-scrollbar
					 * is * used. */

    short int yScrollbarWidth, xScrollbarHeight;

    short int maxWidth;			/* Width of the widest entry. */
    short int minHeight;		/* Minimum entry height. Used to to
					 * compute what the y-scroll unit
					 * should * be. */
    short int butWidth, butHeight;	/* Dimension of the button that posted
					 * this menu. */
    short int width, height;
    GC copyGC;
};

/*
 * EntryIterator --
 *
 *	Entries may be tagged with strings.  An entry may have many tags.  The
 *	same tag may be used for many entries.
 *	
 */

typedef enum { 
    ITER_INDEX, ITER_ALL, ITER_TAG, 
} IteratorType;

typedef struct _Iterator {
    ComboTree *comboPtr;	/* ComboTree that we're iterating over. */

    IteratorType type;		/* Type of iteration:
				 * ITER_TAG		By entry tag.
				 * ITER_ALL		By every entry.
				 * ITER_INDEX		Single entry: either 
				 *			tag or index.
				 */

    Entry *first;		/* Starting point of search, saved if iterator
				 * is reused.  Used for ITER_ALL and
				 * ITER_INDEX searches. */
    Entry *next;		/* Next entry. */

				/* For tag-based searches. */
    const char *tagName;	/* If non-NULL, is the tag that we are
				 * currently iterating over. */

    Blt_HashTable *tablePtr;	/* Pointer to tag hash table. */
    Blt_HashSearch cursor;	/* Search iterator for tag hash table. */

} EntryIterator;


#define BUTTON_IPAD		1
#define BUTTON_PAD		2
#define BUTTON_SIZE		7
#define COLUMN_PAD		2
#define FOCUS_WIDTH		1
#define ICON_HEIGHT		16
#define ICON_PADX		2
#define ICON_PADY		1
#define ICON_WIDTH		16
#define INSET_PAD		0
#define LABEL_PADX		3
#define LABEL_PADY		0

#include <X11/Xutil.h>
#include <X11/Xatom.h>


typedef ClientData (TagProc)(ComboTree *comboPtr, const char *string);
typedef int (ApplyProc) (ComboTree *comboPtr, Entry *entryPtr);


#define DEF_BTN_ACTIVE_BG		RGB_WHITE
#define DEF_BTN_ACTIVE_FG		STD_ACTIVE_FOREGROUND
#define DEF_BTN_BORDERWIDTH		"1"
#define DEF_BTN_CLOSE_RELIEF		"solid"
#define DEF_BTN_NORMAL_BG		RGB_WHITE
#define DEF_BTN_NORMAL_FG		STD_NORMAL_FOREGROUND
#define DEF_BTN_OPEN_RELIEF		"solid"
#define DEF_BTN_SIZE			"7"

#define DEF_COMBO_ACTIVE_STIPPLE	"gray25"
#define DEF_COMBO_BORDERWIDTH		"1"
#define DEF_COMBO_BUTTON		"auto"
#define DEF_COMBO_DASHES		"dot"
#define DEF_COMBO_HEIGHT		"400"
#define DEF_COMBO_HIDE_LEAVES		"no"
#define DEF_COMBO_HIDE_ROOT		"yes"
#define DEF_COMBO_ICON_VARIABLE		((char *)NULL)
#define DEF_COMBO_LINESPACING		"0"
#define DEF_COMBO_LINEWIDTH		"1"
#define DEF_COMBO_MAKE_PATH		"no"
#define DEF_COMBO_NEWTAGS		"no"
#define DEF_COMBO_RELIEF		"solid"
#define DEF_COMBO_SCROLLBAR		((char *)NULL)
#define DEF_COMBO_SCROLLINCREMENT	"20"
#define DEF_COMBO_SHOW_ROOT		"yes"
#define DEF_COMBO_TAKE_FOCUS		"1"
#define DEF_COMBO_TEXT_VARIABLE		((char *)NULL)
#define DEF_COMBO_LINECOLOR		RGB_GREY50
#define DEF_COMBO_WIDTH			"0"
#ifdef WIN32
#define DEF_COMBO_SEPARATOR		"\\"
#else
#define DEF_COMBO_SEPARATOR		"/"
#endif

#define DEF_ENTRY_STYLE			"default"
#define DEF_STYLE_ACTIVE_BG		RGB_SKYBLUE4
#define DEF_STYLE_ACTIVE_FG		RGB_WHITE
#define DEF_STYLE_ACTIVE_RELIEF		"flat"
#define DEF_STYLE_ALT_BG		((char *)NULL)
#define DEF_STYLE_BG			"white"
#define DEF_STYLE_BORDERWIDTH		STD_BORDERWIDTH
#define DEF_STYLE_FG			STD_NORMAL_FOREGROUND
#define DEF_STYLE_FONT			"Courier 12"
#define DEF_STYLE_ICONS			"::blt::ComboTree::openIcon ::blt::ComboTree::closeIcon"
#define DEF_STYLE_NORMAL_BG		STD_NORMAL_BACKGROUND
#define DEF_STYLE_RELIEF		"flat"

static Blt_TreeApplyProc CreateApplyProc;

static Blt_OptionParseProc ObjToIconsProc;
static Blt_OptionPrintProc IconsToObjProc;
static Blt_OptionFreeProc FreeIconsProc;
static Blt_CustomOption iconsOption = {
    ObjToIconsProc, IconsToObjProc, FreeIconsProc, NULL,
};

static Blt_OptionParseProc ObjToButtonProc;
static Blt_OptionPrintProc ButtonToObjProc;
static Blt_CustomOption buttonOption = {
    ObjToButtonProc, ButtonToObjProc, NULL, NULL,
};

static Blt_OptionParseProc ObjToUidProc;
static Blt_OptionPrintProc UidToObjProc;
static Blt_OptionFreeProc FreeUidProc;
static Blt_CustomOption uidOption = {
    ObjToUidProc, UidToObjProc, FreeUidProc, NULL,
};

static Blt_OptionParseProc ObjToLabelProc;
static Blt_OptionPrintProc LabelToObjProc;
static Blt_OptionFreeProc FreeLabelProc;
static Blt_CustomOption labelOption =
{
    ObjToLabelProc, LabelToObjProc, FreeLabelProc, NULL,
};

static Blt_OptionParseProc ObjToStyleProc;
static Blt_OptionPrintProc StyleToObjProc;
static Blt_OptionFreeProc FreeStyleProc;
static Blt_CustomOption styleOption = {
    ObjToStyleProc, StyleToObjProc, FreeStyleProc, NULL,
};

static Blt_ConfigSpec buttonSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"Background", DEF_BTN_ACTIVE_BG, 
	Blt_Offset(ComboTree, button.activeBg), 0},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeForeground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", "Foreground",
	DEF_BTN_ACTIVE_FG, 
	Blt_Offset(ComboTree, button.activeFgColor), 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	DEF_BTN_NORMAL_BG, Blt_Offset(ComboTree, button.normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0, 
	0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_BTN_BORDERWIDTH, Blt_Offset(ComboTree, button.borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-closerelief", "closeRelief", "Relief",
	DEF_BTN_CLOSE_RELIEF, Blt_Offset(ComboTree, button.closeRelief),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
	DEF_BTN_NORMAL_FG, Blt_Offset(ComboTree, button.fgColor), 0},
    {BLT_CONFIG_CUSTOM, "-images", "images", "Icons", (char *)NULL, 
	Blt_Offset(ComboTree, button.icons), BLT_CONFIG_NULL_OK, 
	&iconsOption},
    {BLT_CONFIG_RELIEF, "-openrelief", "openRelief", "Relief",
	DEF_BTN_OPEN_RELIEF, Blt_Offset(ComboTree, button.openRelief),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-size", "size", "Size", DEF_BTN_SIZE, 
	Blt_Offset(ComboTree, button.reqSize), 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

static Blt_ConfigSpec entrySpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-bindtags", (char *)NULL, (char *)NULL, (char *)NULL, 
	Blt_Offset(Entry, tagsObjPtr), BLT_CONFIG_NULL_OK, &uidOption},
    {BLT_CONFIG_CUSTOM, "-button", (char *)NULL, (char *)NULL, DEF_COMBO_BUTTON,
	Blt_Offset(Entry, flags), BLT_CONFIG_DONT_SET_DEFAULT, &buttonOption},
    {BLT_CONFIG_OBJ, "-closecommand", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(Entry, closeCmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-height", (char *)NULL, (char *)NULL, 
	(char *)NULL, Blt_Offset(Entry, reqHeight), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-label", (char *)NULL, (char *)NULL, (char *)NULL, 
	Blt_Offset(Entry, labelUid), 0, &labelOption},
    {BLT_CONFIG_OBJ, "-opencommand", (char *)NULL, (char *)NULL, 
	(char *)NULL, Blt_Offset(Entry, openCmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-style", (char *)NULL, (char *)NULL, DEF_ENTRY_STYLE, 
	Blt_Offset(Entry, stylePtr), 0, &styleOption},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0}
};

static Blt_ConfigSpec styleSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", (char *)NULL, (char *)NULL,
	DEF_STYLE_ACTIVE_BG, Blt_Offset(Style, activeBg), 0},
    {BLT_CONFIG_COLOR, "-activeforeground", (char *)NULL, (char *)NULL,
	DEF_STYLE_ACTIVE_FG, Blt_Offset(Style, labelActiveColor), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "Relief",
	DEF_STYLE_ACTIVE_RELIEF, Blt_Offset(Style, activeRelief), 0},
    {BLT_CONFIG_BACKGROUND, "-alternatebackground", "alternateBackground", 
	"Background", DEF_STYLE_ALT_BG, Blt_Offset(Style, altBg), 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	DEF_STYLE_BG, Blt_Offset(Style, normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_STYLE_BORDERWIDTH, Blt_Offset(Style, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_STYLE_FONT, 
	Blt_Offset(Style, labelFont), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", DEF_STYLE_FG,
	 Blt_Offset(Style, labelNormalColor), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_CUSTOM, "-icons", (char *)NULL, (char *)NULL, DEF_STYLE_ICONS,
	Blt_Offset(Style, icons), BLT_CONFIG_NULL_OK, &iconsOption},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_STYLE_RELIEF, 
	Blt_Offset(Style, relief), 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

static Blt_ConfigSpec comboSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground",
       "ActiveBackground", DEF_STYLE_ACTIVE_BG, 
	Blt_Offset(ComboTree, defStyle.activeBg), 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
	"ActiveForeground", DEF_STYLE_ACTIVE_FG, 
        Blt_Offset(ComboTree, defStyle.labelActiveColor), 0},
    {BLT_CONFIG_BACKGROUND, "-alternatebackground", "alternateBackground", 
	"Background", DEF_STYLE_ALT_BG, Blt_Offset(ComboTree, defStyle.altBg), 
	 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	DEF_STYLE_BG, Blt_Offset(ComboTree, defStyle.normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_COMBO_BORDERWIDTH, Blt_Offset(ComboTree, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-button", "button", "Button",
	DEF_COMBO_BUTTON, Blt_Offset(ComboTree, buttonFlags),
	BLT_CONFIG_DONT_SET_DEFAULT, &buttonOption},
    {BLT_CONFIG_OBJ, "-closecommand", "closeCommand", "CloseCommand",
	(char *)NULL, Blt_Offset(ComboTree, closeCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	(char *)NULL, Blt_Offset(ComboTree, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_DASHES, "-dashes", "dashes", "Dashes", 	DEF_COMBO_DASHES, 
	Blt_Offset(ComboTree, dashes), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_STYLE_FONT, 
	Blt_Offset(ComboTree, defStyle.labelFont), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
	DEF_STYLE_FG, Blt_Offset(ComboTree, defStyle.labelNormalColor), 
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_PIXELS, "-height", "height", "Height", DEF_COMBO_HEIGHT, 
	Blt_Offset(ComboTree, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-hideleaves", "hideLeaves", "HideLeaves",
	DEF_COMBO_HIDE_LEAVES, Blt_Offset(ComboTree, flags),
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)HIDE_LEAVES},
    {BLT_CONFIG_BITMASK, "-hideroot", "hideRoot", "HideRoot",
	DEF_COMBO_HIDE_ROOT, Blt_Offset(ComboTree, flags),
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)HIDE_ROOT},
    {BLT_CONFIG_OBJ, "-iconvariable", "iconVariable", "IconVariable", 
	DEF_COMBO_ICON_VARIABLE, Blt_Offset(ComboTree, iconVarObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-icons", "icons", "Icons", DEF_STYLE_ICONS, 
	Blt_Offset(ComboTree, defStyle.icons), BLT_CONFIG_NULL_OK, 
	&iconsOption},
    {BLT_CONFIG_COLOR, "-linecolor", "lineColor", "LineColor",
	DEF_COMBO_LINECOLOR, Blt_Offset(ComboTree, lineColor),
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_PIXELS_NNEG, "-linespacing", "lineSpacing", "LineSpacing",
	DEF_COMBO_LINESPACING, Blt_Offset(ComboTree, leader),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth", 
	DEF_COMBO_LINEWIDTH, Blt_Offset(ComboTree, lineWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-newtags", "newTags", "NewTags", DEF_COMBO_NEWTAGS, 
	Blt_Offset(ComboTree, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)NEW_TAGS},
    {BLT_CONFIG_OBJ, "-opencommand", "openCommand", "OpenCommand",
	(char *)NULL, Blt_Offset(ComboTree, openCmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief",
	DEF_COMBO_RELIEF, Blt_Offset(ComboTree, relief), 0},
    {BLT_CONFIG_STRING, "-separator", "separator", "Separator", 
	DEF_COMBO_SEPARATOR, Blt_Offset(ComboTree, pathSep), 
	BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_OBJ, "-takefocus", "takeFocus", "TakeFocus", 
	DEF_COMBO_TAKE_FOCUS, Blt_Offset(ComboTree, takeFocusObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-textvariable", "textVariable", "TextVariable", 
	DEF_COMBO_TEXT_VARIABLE, Blt_Offset(ComboTree, textVarObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-tree", "tree", "Tree", (char *)NULL, 
	Blt_Offset(ComboTree, treeName), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS, "-width", "width", "Width", DEF_COMBO_WIDTH, 
	Blt_Offset(ComboTree, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-xscrollbar", "xScrollbar", "Scrollbar", 
	DEF_COMBO_SCROLLBAR, Blt_Offset(ComboTree, xScrollbarObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	(char *)NULL, Blt_Offset(ComboTree, xScrollCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-xscrollincrement", "xScrollIncrement", 
	"ScrollIncrement", DEF_COMBO_SCROLLINCREMENT, 
	Blt_Offset(ComboTree, xScrollUnits), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-yscrollbar", "yScrollbar", "Scrollbar", 
	DEF_COMBO_SCROLLBAR, Blt_Offset(ComboTree, yScrollbarObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
	(char *)NULL, Blt_Offset(ComboTree, yScrollCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-yscrollincrement", "yScrollIncrement", 
	"ScrollIncrement", DEF_COMBO_SCROLLINCREMENT, 
	Blt_Offset(ComboTree, yScrollUnits), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

/* Forward Declarations */
static Tcl_IdleProc ConfigureScrollbarsProc;
static Blt_BindPickProc PickEntry;
static Blt_BindAppendTagsProc AppendTagsProc;
static Blt_TreeNotifyEventProc TreeEventProc;
static Tcl_CmdDeleteProc ComboTreeInstCmdDeleteProc;
static Tcl_FreeProc DestroyComboTree;
static Tcl_FreeProc FreeEntryProc;
static Tcl_IdleProc DisplayComboTree;
static Tcl_IdleProc DisplayEntry;
static Tcl_ObjCmdProc ComboTreeInstCmdProc;
static Tcl_ObjCmdProc ComboTreeObjCmdProc;
static Tk_EventProc ComboTreeEventProc;
static Tk_EventProc ScrollbarEventProc;
static Tk_ImageChangedProc IconChangedProc;

static Tk_GeomRequestProc ScrollbarGeometryProc;
static Tk_GeomLostSlaveProc ScrollbarCustodyProc;
static Tk_GeomMgr comboMgrInfo = {
    (char *)"combomenu",	/* Name of geometry manager used by winfo */
    ScrollbarGeometryProc,	/* Procedure to for new geometry requests */
    ScrollbarCustodyProc,	/* Procedure when scrollbar is taken away */
};

static int ComputeVisibleEntries(ComboTree *comboPtr);

typedef int (ComboTreeCmdProc)(ComboTree *comboPtr, Tcl_Interp *interp, 
	int objc, Tcl_Obj *const *objv);

static inline int
GetWidth(ComboTree *comboPtr)
{
    int w;

    w = comboPtr->width;
    if (w < 2) {
	w = Tk_Width(comboPtr->tkwin);
    }
    if (w < 2) {
	w = Tk_ReqWidth(comboPtr->tkwin);
    }
    return w;
}

static inline int
GetHeight(ComboTree *comboPtr)
{
    int h;

    h = comboPtr->height;
    if (h < 2) {
	h = Tk_Height(comboPtr->tkwin);
    }
    if (h < 2) {
	h = Tk_ReqHeight(comboPtr->tkwin);
    }
    return h;
}

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
EventuallyRedraw(ComboTree *comboPtr)
{
    if ((comboPtr->tkwin != NULL) && ((comboPtr->flags & REDRAW_PENDING)==0)) {
	comboPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayComboTree, comboPtr);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedrawEntry --
 *
 *	Tells the Tk dispatcher to call the combomenu display routine at the
 *	next idle point.  This request is made only if the window is displayed
 *	and no other redraw request is pending.
 *
 * Results: None.
 *
 * Side effects:
 *	The window is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyRedrawEntry(Entry *entryPtr) 
{
    ComboTree *comboPtr;

    comboPtr = entryPtr->comboPtr;
    if ((comboPtr->tkwin != NULL) && ((comboPtr->flags & REDRAW_PENDING)==0) &&
	((entryPtr->flags & ENTRY_REDRAW) == 0)) {
	Tcl_DoWhenIdle(DisplayEntry, entryPtr);
	entryPtr->flags |= ENTRY_REDRAW;
    }
}

static void
ConfigureScrollbarsProc(ClientData clientData)
{
    ComboTree *comboPtr = clientData;
    Tcl_Interp *interp;

    interp = comboPtr->interp;
    /* 
     * Execute the initialization procedure on this widget.
     */
    comboPtr->flags &= ~UPDATE_PENDING;
    if (Tcl_VarEval(interp, "::blt::ComboTree::ConfigureScrollbars ", 
	Tk_PathName(comboPtr->tkwin), (char *)NULL) != TCL_OK) {
	Tcl_BackgroundError(interp);
    }
}

static void
UnmanageScrollbar(ComboTree *comboPtr, Tk_Window tkwin)
{
    if (tkwin != NULL) {
	Tk_DeleteEventHandler(tkwin, StructureNotifyMask,
	      ScrollbarEventProc, comboPtr);
	Tk_ManageGeometry(tkwin, (Tk_GeomMgr *)NULL, comboPtr);
	if (Tk_IsMapped(tkwin)) {
	    Tk_UnmapWindow(tkwin);
	}
    }
}

static void
ManageScrollbar(ComboTree *comboPtr, Tk_Window tkwin)
{
    if (tkwin != NULL) {
	Tk_CreateEventHandler(tkwin, StructureNotifyMask, ScrollbarEventProc, 
		comboPtr);
	Tk_ManageGeometry(tkwin, &comboMgrInfo, comboPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * InstallScrollbar --
 *
 *	Convert the string representation of a color into a XColor pointer.
 *
 * Results:
 *	The return value is a standard TCL result.  The color pointer is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
InstallScrollbar(
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    ComboTree *comboPtr,
    Tcl_Obj *objPtr,		/* String representing scrollbar window. */
    Tk_Window *tkwinPtr)
{
    Tk_Window tkwin;

    if (objPtr == NULL) {
	*tkwinPtr = NULL;
	return;
    }
    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objPtr), comboPtr->tkwin);
    if (tkwin == NULL) {
	Tcl_BackgroundError(interp);
	return;
    }
    if (Tk_Parent(tkwin) != comboPtr->tkwin) {
	Tcl_AppendResult(interp, "scrollbar \"", Tk_PathName(tkwin), 
			 "\" must be a child of combomenu.", (char *)NULL);
	Tcl_BackgroundError(interp);
	return;
    }
    ManageScrollbar(comboPtr, tkwin);
    *tkwinPtr = tkwin;
    return;
}

static void
InstallXScrollbar(ClientData clientData)
{
    ComboTree *comboPtr = clientData;

    comboPtr->flags &= ~INSTALL_XSCROLLBAR;
    InstallScrollbar(comboPtr->interp, comboPtr, comboPtr->xScrollbarObjPtr,
		     &comboPtr->xScrollbar);
}

static void
InstallYScrollbar(ClientData clientData)
{
    ComboTree *comboPtr = clientData;

    comboPtr->flags &= ~INSTALL_YSCROLLBAR;
    InstallScrollbar(comboPtr->interp, comboPtr, comboPtr->yScrollbarObjPtr,
		     &comboPtr->yScrollbar);
}

static Entry *
NodeToEntry(ComboTree *comboPtr, Blt_TreeNode node)
{
    Blt_HashEntry *hPtr;

    if (node == NULL) {
	return NULL;
    }
    hPtr = Blt_FindHashEntry(&comboPtr->entryTable, (char *)node);
    if (hPtr == NULL) {
	Blt_Warn("NodeToEntry: can't find node %s\n", 
		 Blt_Tree_NodeLabel(node));
	abort();
	return NULL;
    }
    return Blt_GetHashValue(hPtr);
}


static Entry *
ParentEntry(Entry *entryPtr)
{
    ComboTree *comboPtr = entryPtr->comboPtr; 
    Blt_TreeNode node;

    if (entryPtr->node == Blt_Tree_RootNode(comboPtr->tree)) {
	return NULL;
    }
    node = Blt_Tree_ParentNode(entryPtr->node);
    if (node == NULL) {
	return NULL;
    }
    return NodeToEntry(comboPtr, node);
}

static int
EntryIsHidden(Entry *entryPtr)
{
    ComboTree *comboPtr = entryPtr->comboPtr; 

    if ((comboPtr->flags & HIDE_LEAVES) && (Blt_Tree_IsLeaf(entryPtr->node))) {
	return TRUE;
    }
    return (entryPtr->flags & ENTRY_HIDE) ? TRUE : FALSE;
}

static Entry *
GetEntryFromNode(ComboTree *comboPtr, Blt_TreeNode node)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&comboPtr->entryTable, (char *)node);
    if (hPtr == NULL) {
	return NULL;
    }
    return Blt_GetHashValue(hPtr);
}


static Entry *
FirstChild(Entry *entryPtr, unsigned int mask)
{
    Blt_TreeNode node;
    ComboTree *comboPtr = entryPtr->comboPtr; 

    for (node = Blt_Tree_FirstChild(entryPtr->node); node != NULL; 
	 node = Blt_Tree_NextSibling(node)) {
	entryPtr = NodeToEntry(comboPtr, node);
	if (((mask & ENTRY_HIDE) == 0) || (!EntryIsHidden(entryPtr))) {
	    return entryPtr;
	}
    }
    return NULL;
}

static Entry *
LastChild(Entry *entryPtr, unsigned int mask)
{
    Blt_TreeNode node;
    ComboTree *comboPtr = entryPtr->comboPtr; 

    for (node = Blt_Tree_LastChild(entryPtr->node); node != NULL; 
	 node = Blt_Tree_PrevSibling(node)) {
	entryPtr = NodeToEntry(comboPtr, node);
	if (((mask & ENTRY_HIDE) == 0) || (!EntryIsHidden(entryPtr))) {
	    return entryPtr;
	}
    }
    return NULL;
}

static Entry *
NextSibling(Entry *entryPtr, unsigned int mask)
{
    Blt_TreeNode node;
    ComboTree *comboPtr = entryPtr->comboPtr; 

    for (node = Blt_Tree_NextSibling(entryPtr->node); node != NULL; 
	 node = Blt_Tree_NextSibling(node)) {
	entryPtr = NodeToEntry(comboPtr, node);
	if (((mask & ENTRY_HIDE) == 0) || (!EntryIsHidden(entryPtr))) {
	    return entryPtr;
	}
    }
    return NULL;
}

static Entry *
PrevSibling(Entry *entryPtr, unsigned int mask)
{
    Blt_TreeNode node;
    ComboTree *comboPtr = entryPtr->comboPtr; 

    for (node = Blt_Tree_PrevSibling(entryPtr->node); node != NULL; 
	 node = Blt_Tree_PrevSibling(node)) {
	entryPtr = NodeToEntry(comboPtr, node);
	if (((mask & ENTRY_HIDE) == 0) || (!EntryIsHidden(entryPtr))) {
	    return entryPtr;
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * PrevEntry --
 *
 *	Returns the "previous" node in the tree.  This node (in 
 *	depth-first order) is its parent if the node has no siblings
 *	that are previous to it.  Otherwise it is the last descendant 
 *	of the last sibling.  In this case, descend the sibling's
 *	hierarchy, using the last child at any ancestor, until we
 *	we find a leaf.
 *
 *---------------------------------------------------------------------------
 */
static Entry *
PrevEntry(Entry *entryPtr, unsigned int mask)
{
    ComboTree *comboPtr = entryPtr->comboPtr; 
    Entry *prevPtr;

    if (entryPtr->node == Blt_Tree_RootNode(comboPtr->tree)) {
	return NULL;		/* The root is the first node. */
    }
    prevPtr = PrevSibling(entryPtr, mask);
    if (prevPtr == NULL) {
	/* There are no siblings previous to this one, so pick the parent. */
	prevPtr = ParentEntry(entryPtr);
    } else {
	/*
	 * Traverse down the right-most thread in order to select the
	 * last entry.  Stop if we find a "closed" entry or reach a leaf.
	 */
	entryPtr = prevPtr;
	while ((entryPtr->flags & mask) == 0) {
	    entryPtr = LastChild(entryPtr, mask);
	    if (entryPtr == NULL) {
		break;		/* Found a leaf. */
	    }
	    prevPtr = entryPtr;
	}
    }
    if (prevPtr == NULL) {
	return NULL;
    }
    return prevPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * NextEntry --
 *
 *	Returns the "next" node in relation to the given node.  
 *	The next node (in depth-first order) is either the first 
 *	child of the given node the next sibling if the node has
 *	no children (the node is a leaf).  If the given node is the 
 *	last sibling, then try it's parent next sibling.  Continue
 *	until we either find a next sibling for some ancestor or 
 *	we reach the root node.  In this case the current node is 
 *	the last node in the tree.
 *
 *---------------------------------------------------------------------------
 */
static Entry *
NextEntry(Entry *entryPtr, unsigned int mask)
{
    ComboTree *comboPtr = entryPtr->comboPtr; 
    Entry *nextPtr;
    int ignoreLeaf;

    ignoreLeaf = ((comboPtr->flags & HIDE_LEAVES) && 
	(Blt_Tree_IsLeaf(entryPtr->node)));

    if ((!ignoreLeaf) && ((entryPtr->flags & mask) == 0)) {
	nextPtr = FirstChild(entryPtr, mask); 
	if (nextPtr != NULL) {
	    return nextPtr;	/* Pick the first sub-node. */
	}
    }

    /* 
     * Back up until to a level where we can pick a "next sibling".  
     * For the last entry we'll thread our way back to the root.
     */

    while (entryPtr != comboPtr->rootPtr) {
	nextPtr = NextSibling(entryPtr, mask);
	if (nextPtr != NULL) {
	    return nextPtr;
	}
	entryPtr = ParentEntry(entryPtr);
    }
    return NULL;		/* At root, no next node. */
}

static Entry *
LastEntry(ComboTree *comboPtr, Entry *entryPtr, unsigned int mask)
{
    Blt_TreeNode next;
    Entry *nextPtr;

    next = Blt_Tree_LastChild(entryPtr->node);
    while (next != NULL) {
	nextPtr = NodeToEntry(comboPtr, next);
	if ((nextPtr->flags & mask) != mask) {
	    break;
	}
	entryPtr = nextPtr;
	next = Blt_Tree_LastChild(next);
    }
    return entryPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * NearestEntry --
 *
 *	Finds the entry closest to the given screen X-Y coordinates
 *	in the viewport.
 *
 * Results:
 *	Returns the pointer to the closest node.  If no node is
 *	visible (nodes may be hidden), NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Entry *
NearestEntry(ComboTree *comboPtr, int x, int y, int selectOne)
{
    Entry *lastPtr;
    Entry **p;

    /*
     * We implicitly can pick only visible entries.  So make sure that
     * the tree exists.
     */
    if (comboPtr->numVisible == 0) {
	return NULL;
    }
    if (y < 0) {
	return (selectOne) ? comboPtr->visibleEntries[0] : NULL;
    }
    /*
     * Since the entry positions were previously computed in world
     * coordinates, convert Y-coordinate from screen to world
     * coordinates too.
     */
    y = WORLDY(comboPtr, y);
    lastPtr = comboPtr->visibleEntries[0];
    for (p = comboPtr->visibleEntries; *p != NULL; p++) {
	Entry *entryPtr;

	entryPtr = *p;
	/*
	 * If the start of the next entry starts beyond the point,
	 * use the last entry.
	 */
	if (entryPtr->worldY > y) {
	    return (selectOne) ? entryPtr : NULL;
	}
	if (y < (entryPtr->worldY + entryPtr->height)) {
	    return entryPtr;	/* Found it. */
	}
	lastPtr = entryPtr;
    }
    return (selectOne) ? lastPtr : NULL;
}


static Entry *
FindEntryByLabel(ComboTree *comboPtr, const char *string)
{
    Entry *entryPtr;

    entryPtr = comboPtr->rootPtr;
    if (comboPtr->activePtr != NULL) {
	entryPtr = comboPtr->activePtr;
    }
    for (/*empty*/; entryPtr != NULL; 
	entryPtr = NextEntry(entryPtr, ENTRY_MASK)){
	if (strcmp(GETLABEL(entryPtr), string) == 0) {
	    return entryPtr;
	}
    }
    return NULL;
}

static void
FreeEntryProc(DestroyData data)
{
    Entry *entryPtr = (Entry *)data;
    ComboTree *comboPtr;

    comboPtr = entryPtr->comboPtr;
    Blt_Pool_FreeItem(comboPtr->entryPool, entryPtr);
}

static void
DestroyEntry(Entry *entryPtr)
{
    ComboTree *comboPtr;
    
    comboPtr = entryPtr->comboPtr;
    entryPtr->flags |= DELETED;		/* Mark the entry as deleted. */

    if (entryPtr == comboPtr->activePtr) {
	comboPtr->activePtr = ParentEntry(entryPtr);
    }
    if (entryPtr == comboPtr->activeBtnPtr) {
	comboPtr->activeBtnPtr = NULL;
    }
    Blt_DeleteBindings(comboPtr->bindTable, entryPtr);
    if (entryPtr->hPtr != NULL) {
	Blt_DeleteHashEntry(&comboPtr->entryTable, entryPtr->hPtr);
    }
    entryPtr->node = NULL;

    comboPtr = entryPtr->comboPtr;
    iconsOption.clientData = comboPtr;
    uidOption.clientData = comboPtr;
    labelOption.clientData = comboPtr;
    Blt_FreeOptions(entrySpecs, (char *)entryPtr, comboPtr->display, 0);
    if (!Blt_Tree_TagTableIsShared(comboPtr->tree)) {
	/* Don't clear tags unless this client is the only one using
	 * the tag table.*/
	Blt_Tree_ClearTags(comboPtr->tree, entryPtr->node);
    }
    if (entryPtr->textPtr != NULL) {
	Blt_Free(entryPtr->textPtr);
    }
    Tcl_EventuallyFree(entryPtr, FreeEntryProc);
}

static void
DestroyEntries(ComboTree *comboPtr) 
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    /* Release the current tree, removing any entry fields. */
    for (hPtr = Blt_FirstHashEntry(&comboPtr->entryTable, &cursor); 
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Entry *entryPtr;

	entryPtr = Blt_GetHashValue(hPtr);
	entryPtr->hPtr = NULL;
	DestroyEntry(entryPtr);
    }
    Blt_DeleteHashTable(&comboPtr->entryTable);
}

static const char *
GetFullName(
    ComboTree *comboPtr,
    Entry *entryPtr,
    Tcl_DString *resultPtr)
{
    const char **names;		/* Used the stack the component names. */
    const char *staticSpace[64+2];
    int level;
    int i;

    level = Blt_Tree_NodeDepth(entryPtr->node);
    if (GETLABEL(comboPtr->rootPtr) == NULL) {
	level--;
    }
    if (level > 64) {
	names = Blt_AssertMalloc((level + 2) * sizeof(char *));
    } else {
	names = staticSpace;
    }
    for (i = level; i >= 0; i--) {
	Blt_TreeNode node;

	/* Save the name of each ancestor in the name array. */
	names[i] = GETLABEL(entryPtr);
	node = Blt_Tree_ParentNode(entryPtr->node);
	if (node != NULL) {
	    entryPtr = NodeToEntry(comboPtr, node);
	}
    }
    if (level >= 0) {
	if (comboPtr->pathSep == NULL) {
	    for (i = 0; i <= level; i++) {
		Tcl_DStringAppendElement(resultPtr, names[i]);
	    }
	} else {
	    Tcl_DStringAppend(resultPtr, names[0], -1);
	    for (i = 1; i <= level; i++) {
		Tcl_DStringAppend(resultPtr, comboPtr->pathSep, -1);
		Tcl_DStringAppend(resultPtr, names[i], -1);
	    }
	}
    } else {
	if (comboPtr->pathSep != NULL) {
	    Tcl_DStringAppend(resultPtr, comboPtr->pathSep, -1);
	}
    }
    if (names != staticSpace) {
	Blt_Free(names);
    }
    return Tcl_DStringValue(resultPtr);
}


/*
 * Preprocess the command string for percent substitution.
 */
static void
PercentSubst(ComboTree *comboPtr, Entry *entryPtr, const char *command,
	     Tcl_DString *resultPtr)
{
    const char *last, *p;
    const char *fullName;
    Tcl_DString ds;

    /*
     * Get the full path name of the node, in case we need to substitute for
     * it.
     */
    Tcl_DStringInit(&ds);
    fullName = GetFullName(comboPtr, entryPtr, &ds);
    Tcl_DStringInit(resultPtr);
    /* Append the widget name and the node .t 0 */
    for (last = p = command; *p != '\0'; p++) {
	if (*p == '%') {
	    const char *string;
	    char buf[3];

	    if (p > last) {
		Tcl_DStringAppend(resultPtr, last, p - last);
	    }
	    switch (*(p + 1)) {
	    case '%':		/* Percent sign */
		string = "%";
		break;
	    case 'W':		/* Widget name */
		string = Tk_PathName(comboPtr->tkwin);
		break;
	    case 'P':		/* Full pathname */
		string = fullName;
		break;
	    case 'p':		/* Name of the node */
		string = GETLABEL(entryPtr);
		break;
	    case '#':		/* Node identifier */
		string = Blt_Tree_NodeIdAscii(entryPtr->node);
		break;
	    default:
		if (*(p + 1) == '\0') {
		    p--;
		}
		buf[0] = *p, buf[1] = *(p + 1), buf[2] = '\0';
		string = buf;
		break;
	    }
	    Tcl_DStringAppend(resultPtr, string, -1);
	    p++;
	    last = p + 1;
	}
    }
    if (p > last) {
	Tcl_DStringAppend(resultPtr, last, p-last);
    }
    Tcl_DStringFree(&ds);
}

static int
CloseEntry(ComboTree *comboPtr, Entry *entryPtr)
{
    Tcl_Obj *cmdObjPtr;

    if (entryPtr->flags & ENTRY_CLOSED) {
	return TCL_OK;		/* Entry is already closed. */
    }
    entryPtr->flags |= ENTRY_CLOSED;

    /*
     * Invoke the entry's "close" command, if there is one. Otherwise
     * try the treeview's global "close" command.
     */
    cmdObjPtr = CHOOSE(comboPtr->closeCmdObjPtr, entryPtr->closeCmdObjPtr);
    if (cmdObjPtr != NULL) {
	Tcl_DString ds;
	int result;

	PercentSubst(comboPtr, entryPtr, Tcl_GetString(cmdObjPtr), &ds);
	Tcl_Preserve(entryPtr);
	result = Tcl_GlobalEval(comboPtr->interp, Tcl_DStringValue(&ds));
	Tcl_Release(entryPtr);
	Tcl_DStringFree(&ds);
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    comboPtr->flags |= LAYOUT_PENDING;
    return TCL_OK;
}


static int
OpenEntry(ComboTree *comboPtr, Entry *entryPtr)
{
    Tcl_Obj *cmdObjPtr;

    if ((entryPtr->flags & ENTRY_CLOSED) == 0) {
	return TCL_OK;		/* Entry is already open. */
    }
    entryPtr->flags &= ~ENTRY_CLOSED;
    /*
     * If there's a "open" command proc specified for the entry, use that
     * instead of the more general "open" proc for the entire treeview.
     */
    cmdObjPtr = CHOOSE(comboPtr->openCmdObjPtr, entryPtr->openCmdObjPtr);
    if (cmdObjPtr != NULL) {
	Tcl_DString ds;
	int result;

	PercentSubst(comboPtr, entryPtr, Tcl_GetString(cmdObjPtr), &ds);
	Tcl_Preserve(entryPtr);
	result = Tcl_GlobalEval(comboPtr->interp, Tcl_DStringValue(&ds));
	Tcl_Release(entryPtr);
	Tcl_DStringFree(&ds);
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    comboPtr->flags |= LAYOUT_PENDING;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ActivateEntry --
 *
 *	Marks the designated entry as active.  The entry is redrawn with its
 *	active colors.  The previously active entry is deactivated.  If the
 *	new entry is NULL, then this means that no new entry is to be
 *	activated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Individual entries entries may be scheduled to be drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
ActivateEntry(ComboTree *comboPtr, Entry *entryPtr) 
{
    if ((comboPtr->activePtr == entryPtr) && (entryPtr != NULL)) {
	return;		/* Entry is already active. */
    }
    if (comboPtr->activePtr != NULL) {
	EventuallyRedrawEntry(comboPtr->activePtr);
    }
    comboPtr->activePtr = entryPtr;
    if (entryPtr != NULL) {
	EventuallyRedrawEntry(entryPtr);
    }
}

static int
ConfigureEntry(
    ComboTree *comboPtr,
    Entry *entryPtr,
    int objc,
    Tcl_Obj *const *objv,
    int flags)
{
    iconsOption.clientData = comboPtr;
    uidOption.clientData = comboPtr;
    labelOption.clientData = comboPtr;
    if (Blt_ConfigureWidgetFromObj(comboPtr->interp, comboPtr->tkwin, 
	entrySpecs, objc, objv, (char *)entryPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    /* Assume all changes require a new layout. */
    entryPtr->flags |= ENTRY_LAYOUT_PENDING;
    comboPtr->flags |= (LAYOUT_PENDING | DIRTY);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewEntry --
 *
 *	Allocates and initializes a new entry.
 *
 * Results:
 *	Returns the entry.
 *
 *---------------------------------------------------------------------------
 */
static Entry *
NewEntry(ComboTree *comboPtr, Blt_TreeNode node)
{
    Entry *entryPtr;

    entryPtr = Blt_Pool_AllocItem(comboPtr->entryPool, sizeof(Entry));
    memset(entryPtr, 0, sizeof(Entry));
    entryPtr->flags = (unsigned short)(comboPtr->buttonFlags | ENTRY_CLOSED);
    entryPtr->comboPtr = comboPtr;
    entryPtr->labelUid = NULL;
    entryPtr->node = node;
    return entryPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateEntry --
 *
 *	This procedure is called by the Tree object when a node is created and
 *	inserted into the tree.  It adds a new treeview entry field to the
 *	node.
 *
 * Results:
 *	Returns the entry.
 *
 *---------------------------------------------------------------------------
 */
static int
CreateEntry(
    ComboTree *comboPtr,
    Blt_TreeNode node,		/* Node that has just been created. */
    int objc,
    Tcl_Obj *const *objv,
    int flags)
{
    Entry *entryPtr;
    int isNew;
    Blt_HashEntry *hPtr;

    hPtr = Blt_CreateHashEntry(&comboPtr->entryTable, (char *)node, &isNew);
    if (isNew) {
	entryPtr = NewEntry(comboPtr, node);
	Blt_SetHashValue(hPtr, entryPtr);
	entryPtr->hPtr = hPtr;
    } else {
	entryPtr = Blt_GetHashValue(hPtr);
    }
    if (ConfigureEntry(comboPtr, entryPtr, objc, objv, flags) != TCL_OK) {
	DestroyEntry(entryPtr);
	return TCL_ERROR;	/* Error configuring the entry. */
    }
    comboPtr->flags |= (LAYOUT_PENDING | DIRTY);
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTagTable --
 *
 *	Returns the hash table containing row indices for a tag.
 *
 * Results:
 *	Returns a pointer to the hash table containing indices for the given
 *	tag.  If the row has no tags, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_HashTable *
GetTagTable(ComboTree *comboPtr, const char *tagName)		
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&comboPtr->entryBindTagTable, tagName);
    if (hPtr == NULL) {
	return NULL;		/* No tag by that name. */
    }
    return Blt_GetHashValue(hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetEntryIterator --
 *
 *	Converts a string into node pointer.  The string may be in one of the
 *	following forms:
 *
 *	    NNN			- inode.
 *	    "active"		- Currently active node.
 *	    "anchor"		- anchor of selected region.
 *	    "current"		- Currently picked node in bindtable.
 *	    "focus"		- The node currently with focus.
 *	    "root"		- Root node.
 *	    "end"		- Last open node in the entire hierarchy.
 *	    "next"		- Next open node from the currently active
 *				  node. Wraps around back to top.
 *	    "last"		- Previous open node from the currently active
 *				  node. Wraps around back to bottom.
 *	    "up"		- Next open node from the currently active
 *				  node. Does not wrap around.
 *	    "down"		- Previous open node from the currently active
 *				  node. Does not wrap around.
 *	    "nextsibling"	- Next sibling of the current node.
 *	    "prevsibling"	- Previous sibling of the current node.
 *	    "parent"		- Parent of the current node.
 *	    "view.top"		- Top of viewport.
 *	    "view.bottom"	- Bottom of viewport.
 *	    @x,y		- Closest node to the specified X-Y position.
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.  The
 *	pointer to the node is returned via nodePtr.  Otherwise, TCL_ERROR is
 *	returned and an error message is left in interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
static int
GetEntryIterator(
    Tcl_Interp *interp, 
    ComboTree *comboPtr, 
    Tcl_Obj *objPtr, 
    EntryIterator *iterPtr)
{
    Entry *entryPtr, *fromPtr;
    char c;
    const char *string;

    iterPtr->first = NULL;
    iterPtr->type = ITER_INDEX;
    string = Tcl_GetString(objPtr);
    iterPtr->tagName = Tcl_GetString(objPtr);
    entryPtr = NULL;
    fromPtr = comboPtr->activePtr;
    if (fromPtr == NULL) {
	fromPtr = comboPtr->rootPtr;
    }
    c = string[0];
    if (isdigit(UCHAR(string[0]))) {    
	Blt_TreeNode node;
	long inode;

	if (Blt_GetLongFromObj(interp, objPtr, &inode) != TCL_OK) {
	    return TCL_ERROR;
	}
	node = Blt_Tree_GetNode(comboPtr->tree, inode);
	if (node != NULL) {
	    iterPtr->first = NodeToEntry(comboPtr, node);
	}
	return TCL_OK;		/* Node Id. */
    }
    if (c == '@') {
	int x, y;

	if (Blt_GetXY(interp, comboPtr->tkwin, string, &x, &y) == TCL_OK) {
	    iterPtr->first = NearestEntry(comboPtr, x, y, TRUE);
	}
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
	iterPtr->first = comboPtr->rootPtr;
	iterPtr->type = ITER_ALL;
    } else if ((c == 'a') && (strcmp(string, "active") == 0)) {
	iterPtr->first = comboPtr->activePtr;
    } else if ((c == 'b') && (strcmp(string, "bottom") == 0)) {
	iterPtr->first = LastEntry(comboPtr, comboPtr->rootPtr, ENTRY_MASK);
    } else if ((c == 't') && (strcmp(string, "top") == 0)) {
	entryPtr = comboPtr->rootPtr;
	if (comboPtr->flags & HIDE_ROOT) {
	    entryPtr = NextEntry(entryPtr, ENTRY_MASK);
	}
	iterPtr->first = entryPtr;
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
	iterPtr->first = LastEntry(comboPtr, comboPtr->rootPtr, ENTRY_MASK);
    } else if ((c == 'r') && (strcmp(string, "root") == 0)) {
	iterPtr->first = comboPtr->rootPtr;
    } else if ((c == 'p') && (strcmp(string, "parent") == 0)) {
	if (fromPtr != comboPtr->rootPtr) {
	    iterPtr->first = ParentEntry(fromPtr);
	}
    } else if ((c == 'c') && (strcmp(string, "current") == 0)) {
	/* Can't trust picked entry, if entries have been added or deleted. */
	if (!(comboPtr->flags & DIRTY)) {
	    ClientData hint;

	    hint = Blt_GetCurrentHint(comboPtr->bindTable);
	    if ((hint == PICK_ENTRY) || (hint == PICK_BUTTON)) {
		entryPtr = Blt_GetCurrentItem(comboPtr->bindTable);
		if ((entryPtr != NULL) && ((entryPtr->flags & DELETED) == 0)) {
		    iterPtr->first = entryPtr;
		}
	    }
	}
    } else if ((c == 'u') && (strcmp(string, "up") == 0)) {
	entryPtr = PrevEntry(fromPtr, ENTRY_MASK);
	if (entryPtr == NULL) {
	    entryPtr = fromPtr;
	}
	if ((entryPtr == comboPtr->rootPtr) && 
	    (comboPtr->flags & HIDE_ROOT)) {
	    entryPtr = NextEntry(entryPtr, ENTRY_MASK);
	}
	iterPtr->first = entryPtr;
    } else if ((c == 'd') && (strcmp(string, "down") == 0)) {
	entryPtr = NextEntry(fromPtr, ENTRY_MASK);
	if (entryPtr == NULL) {
	    entryPtr = fromPtr;
	}
	if ((entryPtr == comboPtr->rootPtr) && 
	    (comboPtr->flags & HIDE_ROOT)) {
	    entryPtr = NextEntry(entryPtr, ENTRY_MASK);
	}
	iterPtr->first = entryPtr;
    } else if (((c == 'l') && (strcmp(string, "last") == 0)) ||
	       ((c == 'p') && (strcmp(string, "prev") == 0))) {
	entryPtr = PrevEntry(fromPtr, ENTRY_MASK);
	if (entryPtr == NULL) {
	    entryPtr = LastEntry(comboPtr, comboPtr->rootPtr, ENTRY_MASK);
	}
	if ((entryPtr == comboPtr->rootPtr) && 
	    (comboPtr->flags & HIDE_ROOT)) {
	    entryPtr = NextEntry(entryPtr, ENTRY_MASK);
	}
	iterPtr->first = entryPtr;
    } else if ((c == 'n') && (strcmp(string, "next") == 0)) {
        entryPtr = NextEntry(fromPtr, ENTRY_MASK);
	if (entryPtr == NULL) {
	    if (comboPtr->flags & HIDE_ROOT) {
		entryPtr = NextEntry(comboPtr->rootPtr,ENTRY_MASK);
	    } else {
		entryPtr = comboPtr->rootPtr;
	    }
	}
	iterPtr->first = entryPtr;
    } else if ((c == 'n') && (strcmp(string, "nextsibling") == 0)) {
	iterPtr->first = NextSibling(fromPtr, ENTRY_MASK);
    } else if ((c == 'n') && (strcmp(string, "none") == 0)) {
	iterPtr->first = NULL;
    } else if ((c == 'p') && (strcmp(string, "prevsibling") == 0)) {
	iterPtr->first = PrevSibling(fromPtr, ENTRY_MASK);
    } else if ((c == 'v') && (strcmp(string, "view.top") == 0)) {
	if (comboPtr->numVisible > 0) {
	    iterPtr->first = comboPtr->visibleEntries[0];
	}
    } else if ((c == 'v') && (strcmp(string, "view.bottom") == 0)) {
	if (comboPtr->numVisible > 0) {
	    iterPtr->first = comboPtr->visibleEntries[comboPtr->numVisible - 1];
	} 
    } else {
	entryPtr = FindEntryByLabel(comboPtr, iterPtr->tagName);
	if (entryPtr != NULL) {
	    iterPtr->first = entryPtr;
	} else {
	    iterPtr->tablePtr = GetTagTable(comboPtr, iterPtr->tagName);
	    if (iterPtr->tablePtr != NULL) {
		iterPtr->type = ITER_TAG;
		return TCL_OK;
	    }
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "can't find tag or entry \"", string,
			"\" in \"", Tk_PathName(comboPtr->tkwin), "\"", 
			(char *)NULL);
	    }
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NextTaggedEntry --
 *
 *	Returns the next entry derived from the given tag.
 *
 * Results:
 *	Returns the row location of the first entry.  If no more rows can be
 *	found, then -1 is returned.
 *
 *---------------------------------------------------------------------------
 */
static Entry *
NextTaggedEntry(EntryIterator *iterPtr)
{
    if (iterPtr->type == ITER_TAG) {
	Blt_HashEntry *hPtr;

	hPtr = Blt_NextHashEntry(&iterPtr->cursor); 
	if (hPtr != NULL) {
	    return Blt_GetHashValue(hPtr);
	}
    } else if (iterPtr->type == ITER_ALL) {
	Entry *entryPtr;
	
	entryPtr = iterPtr->next;
	if (entryPtr != NULL) {
	    iterPtr->next = NextEntry(entryPtr, ENTRY_MASK);
	}
	return entryPtr;
    }	
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * FirstTaggedEntry --
 *
 *	Returns the first entry derived from the given tag.
 *
 * Results:
 *	Returns the row location of the first entry.  If no more rows can be
 *	found, then -1 is returned.
 *
 *---------------------------------------------------------------------------
 */
static Entry *
FirstTaggedEntry(EntryIterator *iterPtr)
{
    if (iterPtr->type == ITER_TAG) {
	Blt_HashEntry *hPtr;
	
	hPtr = Blt_FirstHashEntry(iterPtr->tablePtr, &iterPtr->cursor);
	if (hPtr == NULL) {
	    return NULL;
	}
	return Blt_GetHashValue(hPtr);
    } else {
	Entry *entryPtr;
	
	entryPtr = iterPtr->first;
	iterPtr->next = NextTaggedEntry(iterPtr);
	return entryPtr;
    } 
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetEntryFromObj --
 *
 *	Gets the entry associated the given index, tag, or label.  This
 *	routine is used when you want only one entry.  It's an error if more
 *	than one entry is specified (e.g. "all" tag or range "1:4").  It's
 *	also an error if the tag is empty (no entries are currently tagged).
 *
 *---------------------------------------------------------------------------
 */
static int 
GetEntryFromObj(
    Tcl_Interp *interp, 
    ComboTree *comboPtr,
    Tcl_Obj *objPtr,
    Entry **entryPtrPtr)
{
    EntryIterator iter;
    Entry *firstPtr;

    if (GetEntryIterator(interp, comboPtr, objPtr, &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    firstPtr = FirstTaggedEntry(&iter);
    if (firstPtr != NULL) {
	Entry *nextPtr;

	nextPtr = NextTaggedEntry(&iter);
	if (nextPtr != NULL) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "multiple entries specified by \"", 
			Tcl_GetString(objPtr), "\"", (char *)NULL);
	    }
	    return TCL_ERROR;
	}
    }
    *entryPtrPtr = firstPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetEntry --
 *
 *	Returns an entry based upon its index.  This differs from
 *	GetEntryFromObj in that an non-existant entry (NULL) is treated
 *	an error.
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.  The
 *	pointer to the node is returned via nodePtr.  Otherwise, TCL_ERROR is
 *	returned and an error message is left in interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
static int
GetEntry(ComboTree *comboPtr, Tcl_Obj *objPtr, Entry **entryPtrPtr)
{
    Entry *entryPtr;

    if (GetEntryFromObj(comboPtr->interp, comboPtr, objPtr, &entryPtr) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    if (entryPtr == NULL) {
	Tcl_ResetResult(comboPtr->interp);
	Tcl_AppendResult(comboPtr->interp, "can't find entry \"", 
		Tcl_GetString(objPtr), "\" in \"", Tk_PathName(comboPtr->tkwin),
		"\"", (char *)NULL);
	return TCL_ERROR;
    }
    *entryPtrPtr = entryPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetUid --
 *
 *	Gets or creates a unique string identifier.  Strings are reference
 *	counted.  The string is placed into a hashed table local to the
 *	treeview.
 *
 * Results:
 *	Returns the pointer to the hashed string.
 *
 *---------------------------------------------------------------------------
 */
static UID
GetUid(ComboTree *comboPtr, const char *string)
{
    Blt_HashEntry *hPtr;
    int isNew;
    size_t refCount;

    hPtr = Blt_CreateHashEntry(&comboPtr->uidTable, string, &isNew);
    if (isNew) {
	refCount = 1;
    } else {
	refCount = (size_t)Blt_GetHashValue(hPtr);
	refCount++;
    }
    Blt_SetHashValue(hPtr, refCount);
    return Blt_GetHashKey(&comboPtr->uidTable, hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeUid --
 *
 *	Releases the uid.  Uids are reference counted, so only when the
 *	reference count is zero (i.e. no one else is using the string) is the
 *	entry removed from the hash table.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeUid(ComboTree *comboPtr, UID uid)
{
    Blt_HashEntry *hPtr;
    size_t refCount;

    hPtr = Blt_FindHashEntry(&comboPtr->uidTable, uid);
    assert(hPtr != NULL);
    refCount = (size_t)Blt_GetHashValue(hPtr);
    refCount--;
    if (refCount > 0) {
	Blt_SetHashValue(hPtr, refCount);
    } else {
	Blt_DeleteHashEntry(&comboPtr->uidTable, hPtr);
    }
}

static Icon
GetIcon(ComboTree *comboPtr, const char *iconName)
{
    Blt_HashEntry *hPtr;
    int isNew;
    struct _Icon *iconPtr;

    hPtr = Blt_CreateHashEntry(&comboPtr->iconTable, iconName, &isNew);
    if (isNew) {
	Tk_Image tkImage;
	int width, height;

	tkImage = Tk_GetImage(comboPtr->interp, comboPtr->tkwin, 
		(char *)iconName, IconChangedProc, comboPtr);
	if (tkImage == NULL) {
	    Blt_DeleteHashEntry(&comboPtr->iconTable, hPtr);
	    return NULL;
	}
	Tk_SizeOfImage(tkImage, &width, &height);
	iconPtr = Blt_AssertMalloc(sizeof(struct _Icon));
	iconPtr->tkImage = tkImage;
	iconPtr->hashPtr = hPtr;
	iconPtr->refCount = 1;
	iconPtr->width = width;
	iconPtr->height = height;
	Blt_SetHashValue(hPtr, iconPtr);
    } else {
	iconPtr = Blt_GetHashValue(hPtr);
	iconPtr->refCount++;
    }
    return iconPtr;
}

static void
FreeIcon(ComboTree *comboPtr, struct _Icon *iconPtr)
{
    iconPtr->refCount--;
    if (iconPtr->refCount == 0) {
	Blt_DeleteHashEntry(&comboPtr->iconTable, iconPtr->hashPtr);
	Tk_FreeImage(iconPtr->tkImage);
	Blt_Free(iconPtr);
    }
}

static void
DestroyIcons(ComboTree *comboPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    struct _Icon *iconPtr;

    for (hPtr = Blt_FirstHashEntry(&comboPtr->iconTable, &cursor);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	iconPtr = Blt_GetHashValue(hPtr);
	Tk_FreeImage(iconPtr->tkImage);
	Blt_Free(iconPtr);
    }
    Blt_DeleteHashTable(&comboPtr->iconTable);
}

static void
DestroyStyle(Style *stylePtr)
{
    ComboTree *comboPtr;

    stylePtr->refCount--;
    if (stylePtr->refCount > 0) {
	return;
    }
    comboPtr = stylePtr->comboPtr;
    iconsOption.clientData = comboPtr;
    Blt_FreeOptions(styleSpecs, (char *)stylePtr, comboPtr->display, 0);
    if (stylePtr->labelActiveGC != NULL) {
	Tk_FreeGC(comboPtr->display, stylePtr->labelActiveGC);
    }
    if (stylePtr->labelDisabledGC != NULL) {
	Tk_FreeGC(comboPtr->display, stylePtr->labelDisabledGC);
    }
    if (stylePtr->labelNormalGC != NULL) {
	Tk_FreeGC(comboPtr->display, stylePtr->labelNormalGC);
    }
    if (stylePtr->hPtr != NULL) {
	Blt_DeleteHashEntry(&stylePtr->comboPtr->styleTable, stylePtr->hPtr);
    }
    if (stylePtr != &stylePtr->comboPtr->defStyle) {
	Blt_Free(stylePtr);
    }
}

static int
AddDefaultStyle(Tcl_Interp *interp, ComboTree *comboPtr)
{
    Blt_HashEntry *hPtr;
    int isNew;
    Style *stylePtr;

    hPtr = Blt_CreateHashEntry(&comboPtr->styleTable, "default", &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "combotree style \"", "default", 
		"\" already exists.", (char *)NULL);
	return TCL_ERROR;
    }
    stylePtr = &comboPtr->defStyle;
    assert(stylePtr);
    stylePtr->refCount = 1;
    stylePtr->name = Blt_GetHashKey(&comboPtr->styleTable, hPtr);
    stylePtr->hPtr = hPtr;
    stylePtr->comboPtr = comboPtr;
    stylePtr->relief = TK_RELIEF_FLAT;
    stylePtr->activeRelief = TK_RELIEF_FLAT;
    Blt_SetHashValue(hPtr, stylePtr);
    return TCL_OK;
}

static void
DestroyStyles(ComboTree *comboPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    for (hPtr = Blt_FirstHashEntry(&comboPtr->styleTable, &cursor); 
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Style *stylePtr;

	stylePtr = Blt_GetHashValue(hPtr);
	stylePtr->hPtr = NULL;
	stylePtr->refCount = 0;
	DestroyStyle(stylePtr);
    }
    Blt_DeleteHashTable(&comboPtr->styleTable);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetStyleFromObj --
 *
 *	Gets the style associated with the given name.  
 *
 *---------------------------------------------------------------------------
 */
static int 
GetStyleFromObj(
    Tcl_Interp *interp, 
    ComboTree *comboPtr,
    Tcl_Obj *objPtr,
    Style **stylePtrPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&comboPtr->styleTable, Tcl_GetString(objPtr));
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find style \"", 
		Tcl_GetString(objPtr), "\" in combomenu \"", 
		Tk_PathName(comboPtr->tkwin), "\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    *stylePtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}



static void
MapEntry(Entry *entryPtr)
{
    ComboTree *comboPtr;
    int entryWidth, entryHeight;

    comboPtr = entryPtr->comboPtr;
    /*
     * FIXME: Use of DIRTY flag inconsistent.  When does it
     *	      mean "dirty entry"? When does it mean "dirty column"?
     *	      Does it matter? probably
     */
    if (entryPtr->flags & ENTRY_DIRTY) {
	Blt_FontMetrics fontMetrics;
	Style *stylePtr;
	const char *label;
	int textWidth, textHeight;

	stylePtr = entryPtr->stylePtr;

	entryPtr->iconWidth = entryPtr->iconHeight = 0;
	if (stylePtr->icons != NULL) {
	    int i;
	    
	    for (i = 0; i < 2; i++) {
		if (stylePtr->icons[i] == NULL) {
		    break;
		}
		if (entryPtr->iconWidth < IconWidth(stylePtr->icons[i])) {
		    entryPtr->iconWidth = IconWidth(stylePtr->icons[i]);
		}
		if (entryPtr->iconHeight < IconHeight(stylePtr->icons[i])) {
		    entryPtr->iconHeight = IconHeight(stylePtr->icons[i]);
		}
	    }
	}
	if ((stylePtr->icons == NULL) || (stylePtr->icons[0] == NULL)) {
	    entryPtr->iconWidth = ICON_WIDTH;
	    entryPtr->iconHeight = ICON_HEIGHT;
	}
	entryPtr->iconWidth += 2 * ICON_PADX;
	entryPtr->iconHeight += 2 * ICON_PADY;
	entryHeight = MAX(entryPtr->iconHeight, comboPtr->button.height);
	
	if (entryPtr->textPtr != NULL) {
	    Blt_Free(entryPtr->textPtr);
	    entryPtr->textPtr = NULL;
	}
	Blt_Font_GetMetrics(stylePtr->labelFont, &fontMetrics);
	entryPtr->lineHeight = fontMetrics.linespace;
	entryPtr->lineHeight += 2 * LABEL_PADY + comboPtr->leader;
	label = GETLABEL(entryPtr);
	if (label[0] == '\0') {
	    textWidth = textHeight = entryPtr->lineHeight;
	} else {
	    TextStyle ts;

	    Blt_Ts_InitStyle(ts);
	    Blt_Ts_SetFont(ts, stylePtr->labelFont);
	    entryPtr->textPtr = Blt_Ts_CreateLayout(label, -1, &ts);
	    textWidth = entryPtr->textPtr->width;
	    textHeight = entryPtr->textPtr->height;
	}
	textWidth += 2 * LABEL_PADX;
	textHeight += 2 * LABEL_PADY;
	textWidth = ODD(textWidth);
	if (entryPtr->reqHeight > textHeight) {
	    textHeight = entryPtr->reqHeight;
	} 
	textHeight = ODD(textHeight);
	entryWidth = textWidth;
	if (entryHeight < textHeight) {
	    entryHeight = textHeight;
	}
	entryPtr->labelWidth = textWidth;
	entryPtr->labelHeight = textHeight;
    } else {
	entryHeight = entryPtr->labelHeight;
	entryWidth = entryPtr->labelWidth;
    }
    /*  
     * Find the maximum height of the data value entries. This also has the
     * side effect of contributing the maximum width of the column.
     */
    entryPtr->width = entryWidth + COLUMN_PAD;
    entryPtr->height = entryHeight + comboPtr->leader;
    /*
     * Force the height of the entry to an even number. This is to make the
     * dots or the vertical line segments coincide with the start of the
     * horizontal lines.
     */
    if (entryPtr->height & 0x01) {
	entryPtr->height++;
    }
    entryPtr->flags &= ~ENTRY_DIRTY;
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetCoordinates --
 *
 *	Determines the maximum height of all visible entries.
 *
 *	1. Sets the worldY coordinate for all mapped/open entries.
 *	2. Determines if entry needs a button.
 *	3. Collects the minimum height of open/mapped entries. (Do for all
 *	   entries upon insert).
 *	4. Figures out horizontal extent of each entry (will be width of 
 *	   tree view column).
 *	5. Collects maximum icon size for each level.
 *	6. The height of its vertical line
 *
 * Results:
 *	Returns 1 if beyond the last visible entry, 0 otherwise.
 *
 * Side effects:
 *	The array of visible nodes is filled.
 *
 *---------------------------------------------------------------------------
 */
static void
ResetCoordinates(ComboTree *comboPtr, Entry *entryPtr, int *yPtr, int *numPtr)
{
    if ((entryPtr != comboPtr->rootPtr) && (EntryIsHidden(entryPtr))) {
	entryPtr->worldY = -1;
	entryPtr->vertLineLength = -1;
	return;			/* If the entry is hidden, then do nothing. */
    }
    /* Set the world y-coordinate of the entry. Initialize the length of the
     * dotted vertical line that runs from the entry downward with the current
     * y-offset. */
    entryPtr->worldY = *yPtr;
    entryPtr->vertLineLength = -(*yPtr);
    *yPtr += entryPtr->height;
    entryPtr->seqNum = *numPtr;
    (*numPtr)++;
    {
	int depth;
	LevelInfo *infoPtr;

	depth = Blt_Tree_NodeDepth(entryPtr->node) + 1;
	infoPtr = comboPtr->levelInfo + depth;
	if (infoPtr->labelWidth < entryPtr->labelWidth) {
	    infoPtr->labelWidth = entryPtr->labelWidth;
	}
	if (infoPtr->iconWidth < entryPtr->iconWidth) {
	    infoPtr->iconWidth = entryPtr->iconWidth;
	}
	infoPtr->iconWidth |= 0x01;
    }
    if ((entryPtr->flags & ENTRY_CLOSED) == 0) {
	Entry *bottomPtr, *nextPtr;

	bottomPtr = entryPtr;
	for (nextPtr = FirstChild(entryPtr, ENTRY_HIDE); nextPtr != NULL; 
	     nextPtr = NextSibling(nextPtr, ENTRY_HIDE)) {
	    ResetCoordinates(comboPtr, nextPtr, yPtr, numPtr);
	    bottomPtr = nextPtr;
	}
	entryPtr->vertLineLength += bottomPtr->worldY;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MapTree --
 *
 *	Compute the layout when entries are opened/closed, inserted/deleted,
 *	or when text attributes change (such as font, linespacing).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The world coordinates are set for all the opened entries.
 *
 *---------------------------------------------------------------------------
 */
static void
MapTree(ComboTree *comboPtr)
{
    int y;
    int index;

    /* 
     * Pass 1:	Reinitialize column sizes and loop through all nodes. 
     *
     *		1. Recalculate the size of each entry as needed. 
     *		2. The maximum depth of the tree. 
     *		3. Minimum height of an entry.  Dividing this by the
     *		   height of the widget gives a rough estimate of the 
     *		   maximum number of visible entries.
     *		4. Build an array to hold level information to be filled
     *		   in on pass 2.
     */
    if (comboPtr->flags & DIRTY) {
	Entry *entryPtr;

	comboPtr->minHeight = SHRT_MAX;
	comboPtr->depth = 0;
	for (entryPtr = comboPtr->rootPtr; entryPtr != NULL; 
	     entryPtr = NextEntry(entryPtr, 0)){
	    MapEntry(entryPtr);

	    /* Get the height of the shortest entry.  */
	    if (comboPtr->minHeight > entryPtr->height) {
		comboPtr->minHeight = entryPtr->height;
	    }

	    /* 
	     * Determine if the entry should display a button (indicating that
	     * it has children) and mark the entry accordingly.
	     */
	    entryPtr->flags &= ~ENTRY_BUTTON;
	    if (entryPtr->flags & ENTRY_BTN_SHOW) {
		entryPtr->flags |= ENTRY_BUTTON;
	    } else if (entryPtr->flags & ENTRY_BTN_AUTO) {
		if (comboPtr->flags & HIDE_LEAVES) {
		    /* Check that a non-leaf child exists */
		    if (FirstChild(entryPtr, ENTRY_HIDE) != NULL) {
			entryPtr->flags |= ENTRY_BUTTON;
		    }
		} else if (!Blt_Tree_IsLeaf(entryPtr->node)) {
		    entryPtr->flags |= ENTRY_BUTTON;
		}
	    }

	    /* Save the maximum depth of the tree. */
	    if (comboPtr->depth < Blt_Tree_NodeDepth(entryPtr->node)) {
		comboPtr->depth = Blt_Tree_NodeDepth(entryPtr->node);
	    }
	}

	/* Create bookkeeping for each level of the tree. */
	if (comboPtr->levelInfo != NULL) {
	    Blt_Free(comboPtr->levelInfo);
	}
	comboPtr->levelInfo = Blt_AssertCalloc(comboPtr->depth + 2, 
		sizeof(LevelInfo));
	comboPtr->flags &= ~DIRTY;
    }
    { 
	size_t i;

	/* Reset the level bookkeeping. */
	for (i = 0; i <= (comboPtr->depth + 1); i++) {
	    comboPtr->levelInfo[i].labelWidth = comboPtr->levelInfo[i].x = 
		comboPtr->levelInfo[i].iconWidth = 0;
	}
    }

    /* 
     * Pass 2:	Loop through all open/mapped nodes. 
     *
     *		1. Set world y-coordinates for entries. We must defer setting
     *		   the x-coordinates until we know the maximum icon sizes at 
     *		   each level.
     *		2. Compute the maximum depth of the tree. 
     *		3. Build an array to hold level information.
     */
    y = 0;
    if (comboPtr->flags & HIDE_ROOT) {
	/* If the root entry is to be hidden, cheat by offsetting the
	 * y-coordinates by the height of the entry. */
	y = -(comboPtr->rootPtr->height);
    } 
    index = 0;
    ResetCoordinates(comboPtr, comboPtr->rootPtr, &y, &index);
    comboPtr->worldHeight = y;	/* Set the scroll height of the hierarchy. */
    if (comboPtr->worldHeight < 1) {
	comboPtr->worldHeight = 1;
    }
    {
	int maxX;
	int sum;
	size_t i;

	sum = maxX = 0;
	for (i = 0; i <= (comboPtr->depth + 1); i++) {
	    int x;

	    sum += comboPtr->levelInfo[i].iconWidth;
	    if (i <= comboPtr->depth) {
		comboPtr->levelInfo[i + 1].x = sum;
	    }
	    x = sum + comboPtr->levelInfo[i].labelWidth;
	    if (x > maxX) {
		maxX = x;
	    }
	}
	comboPtr->worldWidth = maxX;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeComboGeometry --
 *
 *	Recompute the layout when entries are opened/closed, inserted/deleted,
 *	or when text attributes change (such as font, linespacing).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The world coordinates are set for all the opened entries.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeComboGeometry(ComboTree *comboPtr)
{
    int w, h;
    int screenWidth, screenHeight;


    MapTree(comboPtr);

    Blt_SizeOfScreen(comboPtr->tkwin, &screenWidth, &screenHeight);
    comboPtr->xScrollbarHeight = comboPtr->yScrollbarWidth = 0;
    if (comboPtr->reqWidth > 0) {
	w = comboPtr->reqWidth;
    } else if (comboPtr->reqWidth < 0) {
	w = MIN(-comboPtr->reqWidth, comboPtr->worldWidth);
    } else {
	w = Tk_Width(Tk_Parent(comboPtr->tkwin));
	if (w <= 1) {
	    w = Tk_ReqWidth(Tk_Parent(comboPtr->tkwin));
	}
    }
    if ((comboPtr->worldWidth > w) && (comboPtr->xScrollbar != NULL)) {
	comboPtr->xScrollbarHeight = Tk_ReqHeight(comboPtr->xScrollbar);
    }
    if (comboPtr->reqHeight > 0) {
	h = comboPtr->reqHeight;
    } else if (comboPtr->reqHeight < 0) {
	h = MIN(-comboPtr->reqHeight, comboPtr->worldHeight);
    } else {
	h=MIN(screenHeight, comboPtr->worldHeight);
    }
    if ((comboPtr->worldHeight > h) && (comboPtr->yScrollbar != NULL)) {
	comboPtr->yScrollbarWidth = Tk_ReqWidth(comboPtr->yScrollbar);
    }
    if ((comboPtr->flags & DROPDOWN) && (w < comboPtr->butWidth)) {
	w = comboPtr->butWidth;
    }
    if (comboPtr->reqHeight == 0) {
	h += comboPtr->xScrollbarHeight;
	h += 2 * comboPtr->borderWidth;
    }
    comboPtr->normalWidth = w;
    comboPtr->normalHeight = h;
    if ((w != Tk_ReqWidth(comboPtr->tkwin)) || 
	(h != Tk_ReqHeight(comboPtr->tkwin))) {
	Tk_GeometryRequest(comboPtr->tkwin, w, h);
    }
    comboPtr->flags |= SCROLL_PENDING | LAYOUT_PENDING;
    comboPtr->width = w;
    comboPtr->height = h;
}

/* Converters for configuration options. */

/*
 *---------------------------------------------------------------------------
 *
 * ObjToButtonProc --
 *
 *	Convert a string to one of three values.
 *		0 - false, no, off
 *		1 - true, yes, on
 *		2 - auto
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.
 *	Otherwise, TCL_ERROR is returned and an error message is left in
 *	interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToButtonProc(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* Tcl_Obj representing the new value. */
    char *widgRec,
    int offset,			/* Offset to field in structure */
    int flags)	
{
    const char *string;
    int *flagsPtr = (int *)(widgRec + offset);

    string = Tcl_GetString(objPtr);
    if ((string[0] == 'a') && (strcmp(string, "auto") == 0)) {
	*flagsPtr &= ~ENTRY_BTN_MASK;
	*flagsPtr |= ENTRY_BTN_AUTO;
    } else {
	int bool;

	if (Tcl_GetBooleanFromObj(interp, objPtr, &bool) != TCL_OK) {
	    return TCL_ERROR;
	}
	*flagsPtr &= ~ENTRY_BTN_MASK;
	if (bool) {
	    *flagsPtr |= ENTRY_BTN_SHOW;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonToObjProc --
 *
 * Results:
 *	The string representation of the button boolean is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ButtonToObjProc(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,
    int offset,			/* Offset to field in structure */
    int flags)	
{
    int bool;
    unsigned int buttonFlags = *(int *)(widgRec + offset);

    bool = (buttonFlags & ENTRY_BTN_MASK);
    if (bool == ENTRY_BTN_AUTO) {
	return Tcl_NewStringObj("auto", 4);
    } else {
	return Tcl_NewBooleanObj(bool);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLabelProc --
 *
 *	Convert the string representing the label. 
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.
 *	Otherwise, TCL_ERROR is returned and an error message is left
 *	in interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToLabelProc(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* Tcl_Obj representing the new value. */
    char *widgRec,
    int offset,			/* Offset to field in structure */
    int flags)	
{
    UID *labelPtr = (UID *)(widgRec + offset);
    const char *string;

    string = Tcl_GetString(objPtr);
    if (string[0] != '\0') {
	ComboTree *comboPtr = clientData;

	*labelPtr = GetUid(comboPtr, string);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LabelToObjProc --
 *
 * Results:
 *	The string of the entry's label is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
LabelToObjProc(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,
    int offset,			/* Offset to field in structure */
    int flags)	
{
    UID labelUid = *(UID *)(widgRec + offset);
    const char *string;

    if (labelUid == NULL) {
	Entry *entryPtr  = (Entry *)widgRec;

	string = Blt_Tree_NodeLabel(entryPtr->node);
    } else {
	string = labelUid;
    }
    return Tcl_NewStringObj(string, -1);
}

/*ARGSUSED*/
static void
FreeLabelProc(
    ClientData clientData,
    Display *display,		/* Not used. */
    char *widgRec,
    int offset)
{
    UID *labelPtr = (UID *)(widgRec + offset);

    if (*labelPtr != NULL) {
	ComboTree *comboPtr = clientData;

	FreeUid(comboPtr, *labelPtr);
	*labelPtr = NULL;
    }
}

/*ARGSUSED*/
static void
FreeStyleProc(
    ClientData clientData,
    Display *display,		/* Not used. */
    char *widgRec,
    int offset)
{
    Style *stylePtr = *(Style **)(widgRec + offset);

    if ((stylePtr != NULL) && (stylePtr != &stylePtr->comboPtr->defStyle)) {
	DestroyStyle(stylePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToStyleProc --
 *
 *	Convert the string representation of a color into a XColor pointer.
 *
 * Results:
 *	The return value is a standard TCL result.  The color pointer is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToStyleProc(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* String representing style. */
    char *widgRec,		/* Widget record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    ComboTree *comboPtr;
    Entry *entryPtr = (Entry *)widgRec;
    Style **stylePtrPtr = (Style **)(widgRec + offset);
    Style *stylePtr;
    const char *string;

    string = Tcl_GetString(objPtr);
    comboPtr = entryPtr->comboPtr;
    if ((string[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
	stylePtr = NULL;
    } else if (GetStyleFromObj(interp, comboPtr, objPtr, &stylePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Release the old style. */
    if ((*stylePtrPtr != NULL) && (*stylePtrPtr != &comboPtr->defStyle)) {
	DestroyStyle(*stylePtrPtr);
    }
    stylePtr->refCount++;
    *stylePtrPtr = stylePtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleToObjProc --
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
StyleToObjProc(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Widget information record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    Style *stylePtr = *(Style **)(widgRec + offset);
    Tcl_Obj *objPtr;

    if (stylePtr == NULL) {
	objPtr = Tcl_NewStringObj("", -1);
    } else {
	objPtr = Tcl_NewStringObj(stylePtr->name, -1);
    }
    return objPtr;
}
/*
 *---------------------------------------------------------------------------
 *
 * ObjToUidProc --
 *
 *	Converts the string to a Uid. Uid's are hashed, reference
 *	counted strings.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToUidProc(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* Tcl_Obj representing the new value. */
    char *widgRec,
    int offset,			/* Offset to field in structure */
    int flags)	
{
    ComboTree *comboPtr = clientData;
    UID *uidPtr = (UID *)(widgRec + offset);

    *uidPtr = GetUid(comboPtr, Tcl_GetString(objPtr));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * UidToObjProc --
 *
 *	Returns the uid as a string.
 *
 * Results:
 *	The fill style string is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
UidToObjProc(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,
    int offset,			/* Offset to field in structure */
    int flags)	
{
    UID uid = *(UID *)(widgRec + offset);

    if (uid == NULL) {
	return Tcl_NewStringObj("", -1);
    } else {
	return Tcl_NewStringObj(uid, -1);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeUid --
 *
 *	Free the UID from the widget record, setting it to NULL.
 *
 * Results:
 *	The UID in the widget record is set to NULL.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
FreeUidProc(
    ClientData clientData,
    Display *display,		/* Not used. */
    char *widgRec,
    int offset)
{
    UID *uidPtr = (UID *)(widgRec + offset);

    if (*uidPtr != NULL) {
	ComboTree *comboPtr = clientData;

	FreeUid(comboPtr, *uidPtr);
	*uidPtr = NULL;
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
    int x,			/* Not used. */
    int y,			/* Not used. */
    int width,			/* Not used. */
    int height,			/* Not used. */
    int imageWidth, 		/* Not used. */
    int imageHeight)		/* Not used. */
{
    ComboTree *comboPtr = clientData;

    comboPtr->flags |= (DIRTY | LAYOUT_PENDING | SCROLL_PENDING);
    EventuallyRedraw(comboPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToIconsProc --
 *
 *	Convert a list of image names into Tk images.
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
ObjToIconsProc(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* Tcl_Obj representing the new value. */
    char *widgRec,
    int offset,			/* Offset to field in structure */
    int flags)	
{
    Tcl_Obj **objv;
    ComboTree *comboPtr = clientData;
    Icon **iconPtrPtr = (Icon **)(widgRec + offset);
    Icon *icons;
    int objc;
    int result;

    result = TCL_OK;
    icons = NULL;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc > 0) {
	int i;
	
	icons = Blt_AssertMalloc(sizeof(Icon *) * (objc + 1));
	for (i = 0; i < objc; i++) {
	    icons[i] = GetIcon(comboPtr, Tcl_GetString(objv[i]));
	    if (icons[i] == NULL) {
		result = TCL_ERROR;
		break;
	    }
	}
	icons[i] = NULL;
    }
    *iconPtrPtr = icons;
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * IconsToObjProc --
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
IconsToObjProc(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,
    int offset,			/* Offset to field in structure */
    int flags)	
{
    Icon *icons = *(Icon **)(widgRec + offset);
    Tcl_Obj *listObjPtr;
    
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (icons != NULL) {
	Icon *iconPtr;

	for (iconPtr = icons; *iconPtr != NULL; iconPtr++) {
	    Tcl_Obj *objPtr;

	    objPtr = Tcl_NewStringObj(Blt_Image_Name((*iconPtr)->tkImage), -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

	}
    }
    return listObjPtr;
}

/*ARGSUSED*/
static void
FreeIconsProc(
    ClientData clientData,
    Display *display,		/* Not used. */
    char *widgRec,
    int offset)
{
    Icon **iconsPtr = (Icon **)(widgRec + offset);

    if (*iconsPtr != NULL) {
	Icon *ip;
	ComboTree *comboPtr = clientData;

	for (ip = *iconsPtr; *ip != NULL; ip++) {
	    FreeIcon(comboPtr, *ip);
	}
	Blt_Free(*iconsPtr);
	*iconsPtr = NULL;
    }
}

static int
Apply(
    ComboTree *comboPtr,
    Entry *entryPtr,		/* Root entry of subtree. */
    ApplyProc *proc,	       /* Procedure to call for each entry. */
    unsigned int flags)
{
    if ((flags & ENTRY_HIDE) && (EntryIsHidden(entryPtr))) {
	return TCL_OK;		/* Hidden node. */
    }
    if ((flags & ENTRY_HIDE) && (entryPtr->flags & ENTRY_HIDE)) {
	return TCL_OK;		/* Hidden node. */
    }
    if (((flags & ENTRY_CLOSED) == 0) || 
	((entryPtr->flags & ENTRY_CLOSED) == 0)) {
	Entry *childPtr;
	Blt_TreeNode node, next;

	for (node = Blt_Tree_FirstChild(entryPtr->node); node != NULL; 
	     node = next) {
	    next = Blt_Tree_NextSibling(node);
	    /* 
	     * Get the next child before calling Apply
	     * recursively.  This is because the apply callback may
	     * delete the node and its link.
	     */
	    childPtr = NodeToEntry(comboPtr, node);
	    if (Apply(comboPtr, childPtr, proc, flags) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    if ((*proc) (comboPtr, entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}


#ifdef notdef
int
EntryIsMapped(Entry *entryPtr)
{
    ComboTree *comboPtr = entryPtr->comboPtr; 

    /* Don't check if the entry itself is open, only that its
     * ancestors are. */
    if (EntryIsHidden(entryPtr)) {
	return FALSE;
    }
    if (entryPtr == comboPtr->rootPtr) {
	return TRUE;
    }
    entryPtr = ParentEntry(entryPtr);
    while (entryPtr != comboPtr->rootPtr) {
	if (entryPtr->flags & (ENTRY_CLOSED | ENTRY_HIDE)) {
	    return FALSE;
	}
	entryPtr = ParentEntry(entryPtr);
    }
    return TRUE;
}
#endif


static void
ConfigureButtons(ComboTree *comboPtr)
{
    Button *btnPtr = &comboPtr->button;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    gcMask = GCForeground;
    gcValues.foreground = btnPtr->fgColor->pixel;
    newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    if (btnPtr->normalGC != NULL) {
	Tk_FreeGC(comboPtr->display, btnPtr->normalGC);
    }
    btnPtr->normalGC = newGC;

    gcMask = GCForeground;
    gcValues.foreground = btnPtr->activeFgColor->pixel;
    newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    if (btnPtr->activeGC != NULL) {
	Tk_FreeGC(comboPtr->display, btnPtr->activeGC);
    }
    btnPtr->activeGC = newGC;

    btnPtr->width = btnPtr->height = ODD(btnPtr->reqSize);
    if (btnPtr->icons != NULL) {
	int i;

	for (i = 0; i < 2; i++) {
	    int width, height;

	    if (btnPtr->icons[i] == NULL) {
		break;
	    }
	    width = IconWidth(btnPtr->icons[i]);
	    height = IconWidth(btnPtr->icons[i]);
	    if (btnPtr->width < width) {
		btnPtr->width = width;
	    }
	    if (btnPtr->height < height) {
		btnPtr->height = height;
	    }
	}
    }
    btnPtr->width += 2 * btnPtr->borderWidth;
    btnPtr->height += 2 * btnPtr->borderWidth;
}

static int
ConfigureStyle(
    Tcl_Interp *interp, 
    Style *stylePtr,
    int objc,
    Tcl_Obj *const *objv,
    int flags)
{
    ComboTree *comboPtr = stylePtr->comboPtr;
    unsigned int gcMask;
    XGCValues gcValues;
    GC newGC;

    if (Blt_ConfigureWidgetFromObj(interp, comboPtr->tkwin, styleSpecs, 
	objc, objv, (char *)stylePtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    gcMask = GCForeground | GCFont | GCLineWidth;
    gcValues.font = Blt_Font_Id(stylePtr->labelFont);
    gcValues.line_width = comboPtr->lineWidth;
    if (comboPtr->dashes > 0) {
	gcMask |= (GCLineStyle | GCDashList);
	gcValues.line_style = LineOnOffDash;
	gcValues.dashes = comboPtr->dashes;
    }

    /* Normal label */
    gcValues.foreground = stylePtr->labelNormalColor->pixel;
    newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->labelNormalGC != NULL) {
	Tk_FreeGC(comboPtr->display, stylePtr->labelNormalGC);
    }
    stylePtr->labelNormalGC = newGC;
	
    /* Active label */
    gcValues.foreground = stylePtr->labelActiveColor->pixel;
    newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->labelActiveGC != NULL) {
	Tk_FreeGC(comboPtr->display, stylePtr->labelActiveGC);
    }
    stylePtr->labelActiveGC = newGC;

    return TCL_OK;
}


/*ARGSUSED*/
static int
CreateApplyProc(
    Blt_TreeNode node,		/* Node that has just been created. */
    ClientData clientData,
    int order)			/* Not used. */
{
    ComboTree *comboPtr = clientData; 
    return CreateEntry(comboPtr, node, 0, NULL, 0);
}

static int
TreeEventProc(ClientData clientData, Blt_TreeNotifyEvent *eventPtr)
{
    Blt_TreeNode node;
    ComboTree *comboPtr = clientData; 

    node = Blt_Tree_GetNode(eventPtr->tree, eventPtr->inode);
    switch (eventPtr->type) {
    case TREE_NOTIFY_CREATE:
	return CreateEntry(comboPtr, node, 0, NULL, 0);
    case TREE_NOTIFY_DELETE:
	/*  
	 * Deleting the tree node triggers a call back to free the
	 * treeview entry that is associated with it.
	 */
	if (node != NULL) {
	    Entry *entryPtr;

	    entryPtr = GetEntryFromNode(comboPtr, node);
	    if (entryPtr != NULL) {
		DestroyEntry(entryPtr);
		EventuallyRedraw(comboPtr);
		comboPtr->flags |= (LAYOUT_PENDING | DIRTY);
	    }
	}
	break;
    case TREE_NOTIFY_RELABEL:
	if (node != NULL) {
	    Entry *entryPtr;

	    entryPtr = NodeToEntry(comboPtr, node);
	    entryPtr->flags |= ENTRY_DIRTY;
	}
	/*FALLTHRU*/
    case TREE_NOTIFY_MOVE:
    case TREE_NOTIFY_SORT:
	EventuallyRedraw(comboPtr);
	comboPtr->flags |= (LAYOUT_PENDING | DIRTY);
	break;
    default:
	/* empty */
	break;
    }	
    return TCL_OK;
}


static ClientData
EntryTag(ComboTree *comboPtr, const char *string)
{
    Blt_HashEntry *hPtr;
    int isNew;			/* Not used. */

    hPtr = Blt_CreateHashEntry(&comboPtr->entryBindTagTable, string, &isNew);
    return Blt_GetHashKey(&comboPtr->entryBindTagTable, hPtr);
}

static ClientData
ButtonTag(ComboTree *comboPtr, const char *string)
{
    Blt_HashEntry *hPtr;
    int isNew;			/* Not used. */

    hPtr = Blt_CreateHashEntry(&comboPtr->buttonBindTagTable, string, &isNew);
    return Blt_GetHashKey(&comboPtr->buttonBindTagTable, hPtr);
}

static void
AddTags(ComboTree *comboPtr, Blt_Chain tags, Tcl_Obj *objPtr, TagProc *tagProc)
{
    int objc;
    Tcl_Obj **objv;
    
    if (Tcl_ListObjGetElements(comboPtr->interp, objPtr, &objc, &objv) 
	== TCL_OK) {
	int i;

	for (i = 0; i < objc; i++) {
	    ClientData clientData;

	    clientData = (*tagProc)(comboPtr, Tcl_GetString(objv[i]));
	    Blt_Chain_Append(tags, clientData);
	}
    }
}

static void
AppendTagsProc(
    Blt_BindTable table,
    ClientData object,		/* Object picked. */
    ClientData hint,		/* Context of object. */
    Blt_Chain tags)		/* (out) List of binding ids to be
				 * applied for this object. */
{
    ComboTree *comboPtr;
    Entry *entryPtr = object;
    
    if (entryPtr->flags & DELETED) {
	return;
    }
    comboPtr = Blt_GetBindingData(table);
    if (hint == (ClientData)PICK_BUTTON) {
	Blt_Chain_Append(tags, ButtonTag(comboPtr, "Button"));
	if (entryPtr->tagsObjPtr != NULL) {
	    AddTags(comboPtr, tags, entryPtr->tagsObjPtr, ButtonTag);
	} else {
	    Blt_Chain_Append(tags, ButtonTag(comboPtr, "Entry"));
	    Blt_Chain_Append(tags, ButtonTag(comboPtr, "all"));
	}
    } else {
	Blt_Chain_Append(tags, entryPtr);
	if (entryPtr->tagsObjPtr != NULL) {
	    AddTags(comboPtr, tags, entryPtr->tagsObjPtr, EntryTag);
	} else if (hint == PICK_ENTRY){
	    Blt_Chain_Append(tags, EntryTag(comboPtr, "Entry"));
	    Blt_Chain_Append(tags, EntryTag(comboPtr, "all"));
	}
    }
}

/*ARGSUSED*/
static ClientData
PickEntry(
    ClientData clientData,
    int x, int y,		/* Screen coordinates of the test point. */
    ClientData *hintPtr)	/* (out) Context of entry selected: should
				 * be PICK_ENTRY or PICK_BUTTON. */
{
    ComboTree *comboPtr = clientData;
    Entry *entryPtr;

    if (hintPtr != NULL) {
	*hintPtr = NULL;
    }
    if (comboPtr->flags & DIRTY) {
	/* Can't trust the selected entry if nodes have been added or
	 * deleted. So recompute the layout. */
	if (comboPtr->flags & LAYOUT_PENDING) {
	    ComputeComboGeometry(comboPtr);
	} 
	ComputeVisibleEntries(comboPtr);
    }
    if (comboPtr->numVisible == 0) {
	return NULL;
    }
    entryPtr = NearestEntry(comboPtr, x, y, FALSE);
    if (entryPtr == NULL) {
	return NULL;
    }
    x = WORLDX(comboPtr, x);
    y = WORLDY(comboPtr, y);
    if (hintPtr != NULL) {
	*hintPtr = PICK_ENTRY;
	if (entryPtr->flags & ENTRY_BUTTON) {
	    Button *btnPtr = &comboPtr->button;
	    int left, right, top, bottom;
	    
	    left = entryPtr->worldX + entryPtr->buttonX - BUTTON_PAD;
	    right = left + btnPtr->width + 2 * BUTTON_PAD;
	    top = entryPtr->worldY + entryPtr->buttonY - BUTTON_PAD;
	    bottom = top + btnPtr->height + 2 * BUTTON_PAD;
	    if ((x >= left) && (x < right) && (y >= top) && (y < bottom)) {
		*hintPtr = (ClientData)PICK_BUTTON;
	    }
	}
    }
    return entryPtr;
}


/*
 * TreeView Procedures
 */

/*
 *---------------------------------------------------------------------------
 *
 * NewComboTree --
 *
 *---------------------------------------------------------------------------
 */
static ComboTree *
NewComboTree(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Tk_Window tkwin;
    ComboTree *comboPtr;
    const char *name;

    name = Tcl_GetString(objPtr);
#define TOP_LEVEL_SCREEN ""
    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), name, 
	TOP_LEVEL_SCREEN);
    if (tkwin == NULL) {
	return NULL;
    }
    Tk_SetClass(tkwin, "BltComboTree");

    comboPtr = Blt_AssertCalloc(1, sizeof(ComboTree));
    comboPtr->tkwin = tkwin;
    comboPtr->display = Tk_Display(tkwin);
    comboPtr->interp = interp;
    comboPtr->flags = (HIDE_ROOT | DIRTY | LAYOUT_PENDING | REPOPULATE);
    comboPtr->leader = 0;
    comboPtr->dashes = 1;
    comboPtr->borderWidth = 1;
    comboPtr->relief = TK_RELIEF_SUNKEN;
    comboPtr->button.closeRelief = comboPtr->button.openRelief = TK_RELIEF_SOLID;
    comboPtr->reqWidth = 0;
    comboPtr->reqHeight = 0;
    comboPtr->xScrollUnits = comboPtr->yScrollUnits = 20;
    comboPtr->lineWidth = 1;
    comboPtr->button.borderWidth = 1;
    comboPtr->buttonFlags = ENTRY_BTN_AUTO;
    Blt_InitHashTableWithPool(&comboPtr->entryTable, BLT_ONE_WORD_KEYS);
    Blt_InitHashTable(&comboPtr->iconTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&comboPtr->uidTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&comboPtr->styleTable, BLT_STRING_KEYS);
    comboPtr->bindTable = Blt_CreateBindingTable(interp, tkwin, comboPtr, 
	PickEntry, AppendTagsProc);
    Blt_InitHashTable(&comboPtr->entryBindTagTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&comboPtr->buttonBindTagTable, BLT_STRING_KEYS);

    comboPtr->entryPool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    Blt_SetWindowInstanceData(tkwin, comboPtr);
    comboPtr->cmdToken = Tcl_CreateObjCommand(interp, 
	Tk_PathName(comboPtr->tkwin), 
	ComboTreeInstCmdProc, comboPtr, ComboTreeInstCmdDeleteProc);

    /*
     * By default create a tree. The name will be the same as the widget
     * pathname.
     */
    comboPtr->tree = Blt_Tree_Open(interp, Tk_PathName(comboPtr->tkwin), 
	TREE_CREATE);
    if (comboPtr->tree == NULL) {
	return NULL;
    }
    Tk_CreateEventHandler(comboPtr->tkwin, ExposureMask | StructureNotifyMask |
	FocusChangeMask, ComboTreeEventProc, comboPtr);
    if (AddDefaultStyle(interp, comboPtr) != TCL_OK) {
	return NULL;
    }
    return comboPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyComboTree --
 *
 * 	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of a TreeView at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the widget is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyComboTree(DestroyData dataPtr)	/* Pointer to the widget record. */
{
    ComboTree *comboPtr = (ComboTree *)dataPtr;
    Button *btnPtr;

    if (comboPtr->tree != NULL) {
	Blt_Tree_Close(comboPtr->tree);
    }
    iconsOption.clientData = comboPtr;
    Blt_FreeOptions(comboSpecs, (char *)comboPtr, comboPtr->display, 0);
    Tcl_DeleteCommandFromToken(comboPtr->interp, comboPtr->cmdToken);
    if (comboPtr->tkwin != NULL) {
	Tk_DeleteSelHandler(comboPtr->tkwin, XA_PRIMARY, XA_STRING);
    }
    if (comboPtr->lineGC != NULL) {
	Tk_FreeGC(comboPtr->display, comboPtr->lineGC);
    }
    if (comboPtr->copyGC != NULL) {
	Tk_FreeGC(comboPtr->display, comboPtr->copyGC);
    }
    if (comboPtr->visibleEntries != NULL) {
	Blt_Free(comboPtr->visibleEntries);
    }
    if (comboPtr->levelInfo != NULL) {
	Blt_Free(comboPtr->levelInfo);
    }
    btnPtr = &comboPtr->button;
    if (btnPtr->activeGC != NULL) {
	Tk_FreeGC(comboPtr->display, btnPtr->activeGC);
    }
    if (btnPtr->normalGC != NULL) {
	Tk_FreeGC(comboPtr->display, btnPtr->normalGC);
    }
    Blt_DestroyBindingTable(comboPtr->bindTable);
    Blt_DeleteHashTable(&comboPtr->entryBindTagTable);
    Blt_DeleteHashTable(&comboPtr->buttonBindTagTable);

    Blt_DeleteHashTable(&comboPtr->uidTable);
    Blt_DeleteHashTable(&comboPtr->entryTable);

    Blt_Pool_Destroy(comboPtr->entryPool);
    DestroyIcons(comboPtr);
    DestroyStyles(comboPtr);
    Blt_Free(comboPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * ScrollbarEventProc --
 *
 *	This procedure is invoked by the Tk event handler when StructureNotify
 *	events occur in a scrollbar managed by the widget.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
ScrollbarEventProc(
    ClientData clientData,	/* Pointer to Entry structure for widget
				 * referred to by eventPtr. */
    XEvent *eventPtr)		/* Describes what just happened. */
{
    ComboTree *comboPtr = clientData;

    if (eventPtr->type == ConfigureNotify) {
	EventuallyRedraw(comboPtr);
    } else if (eventPtr->type == DestroyNotify) {
	if (eventPtr->xany.window == Tk_WindowId(comboPtr->yScrollbar)) {
	    comboPtr->yScrollbar = NULL;
	} else if (eventPtr->xany.window == Tk_WindowId(comboPtr->xScrollbar)) {
	    comboPtr->xScrollbar = NULL;
	} 
	comboPtr->flags |= LAYOUT_PENDING;;
	EventuallyRedraw(comboPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollbarCustodyProc --
 *
 * 	This procedure is invoked when a scrollbar has been stolen by another
 * 	geometry manager.
 *
 * Results:
 *	None.
 *
 * Side effects:
  *	Arranges for the combomenu to have its layout re-arranged at the next
 *	idle point.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ScrollbarCustodyProc(
    ClientData clientData,	/* Information about the combomenu. */
    Tk_Window tkwin)		/* Scrollbar stolen by another geometry
				 * manager. */
{
    ComboTree *comboPtr = (ComboTree *)clientData;

    if (tkwin == comboPtr->yScrollbar) {
	comboPtr->yScrollbar = NULL;
	comboPtr->yScrollbarWidth = 0;
    } else if (tkwin == comboPtr->xScrollbar) {
	comboPtr->xScrollbar = NULL;
	comboPtr->xScrollbarHeight = 0;
    } else {
	return;		
    }
    Tk_UnmaintainGeometry(tkwin, comboPtr->tkwin);
    comboPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(comboPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollbarGeometryProc --
 *
 *	This procedure is invoked by Tk_GeometryRequest for scrollbars managed
 *	by the combomenu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the combomenu to have its layout re-computed and
 *	re-arranged at the next idle point.
 *
 * -------------------------------------------------------------------------- */
/* ARGSUSED */
static void
ScrollbarGeometryProc(
    ClientData clientData,	/* ComboTree widget record.  */
    Tk_Window tkwin)		/* Scrollbar whose geometry has changed. */
{
    ComboTree *comboPtr = (ComboTree *)clientData;

    comboPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(comboPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboTreeEventProc --
 *
 * 	This procedure is invoked by the Tk dispatcher for various
 * 	events on treeview widgets.
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
ComboTreeEventProc(ClientData clientData, XEvent *eventPtr)
{
    ComboTree *comboPtr = clientData;

    if (eventPtr->type == Expose) {
	if (eventPtr->xexpose.count == 0) {
	    EventuallyRedraw(comboPtr);
	    Blt_PickCurrentItem(comboPtr->bindTable);
	}
    } else if (eventPtr->type == ConfigureNotify) {
	comboPtr->flags |= (LAYOUT_PENDING | SCROLL_PENDING);
	EventuallyRedraw(comboPtr);
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    if (eventPtr->type == FocusIn) {
		comboPtr->flags |= FOCUS;
	    } else {
		comboPtr->flags &= ~FOCUS;
	    }
	    EventuallyRedraw(comboPtr);
	}
    } else if (eventPtr->type == DestroyNotify) {
	if (comboPtr->tkwin != NULL) {
	    comboPtr->tkwin = NULL;
	}
	if (comboPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayComboTree, comboPtr);
	}
	Tcl_EventuallyFree(comboPtr, DestroyComboTree);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboTreeInstCmdDeleteProc --
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
ComboTreeInstCmdDeleteProc(ClientData clientData)
{
    ComboTree *comboPtr = clientData;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this
     * procedure destroys the widget.
     */
    if (comboPtr->tkwin != NULL) {
	Tk_Window tkwin;

	tkwin = comboPtr->tkwin;
	comboPtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureComboTree --
 *
 *	Updates the GCs and other information associated with the
 *	treeview widget.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is
 * 	returned, then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for comboPtr; old resources get freed, if there
 *	were any.  The widget is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureComboTree(Tcl_Interp *interp, ComboTree *comboPtr, int objc, 
		   Tcl_Obj *const *objv, int flags)	
{
    int updateNeeded;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    if (Blt_ConfigureWidgetFromObj(interp, comboPtr->tkwin, comboSpecs, 
	objc, objv, (char *)comboPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    if (ConfigureStyle(interp, &comboPtr->defStyle, 0, NULL, 
		       BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }	
    /*
     * GC for dotted vertical line.
     */
    gcMask = (GCForeground | GCLineWidth);
    gcValues.foreground = comboPtr->lineColor->pixel;
    gcValues.line_width = comboPtr->lineWidth;
    if (comboPtr->dashes > 0) {
	gcMask |= (GCLineStyle | GCDashList);
	gcValues.line_style = LineOnOffDash;
	gcValues.dashes = comboPtr->dashes;
    }
    newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    if (comboPtr->lineGC != NULL) {
	Tk_FreeGC(comboPtr->display, comboPtr->lineGC);
    }
    comboPtr->lineGC = newGC;

    gcMask = 0;
    newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    if (comboPtr->copyGC != NULL) {
	Tk_FreeGC(comboPtr->display, comboPtr->copyGC);
    }
    comboPtr->copyGC = newGC;

    ConfigureButtons(comboPtr);
    comboPtr->inset = comboPtr->borderWidth + INSET_PAD;

    /*
     * These options change the layout of the box.  Mark the widget for update.
     */
    if (Blt_ConfigModified(comboSpecs, "-font", "-linespacing", "-*width", 
	"-height", "-hide*", "-tree", (char *)NULL)) {
	comboPtr->flags |= (LAYOUT_PENDING | SCROLL_PENDING);
    }
    /*
     * If the tree view was changed, mark all the nodes dirty (we'll be
     * switching back to either the full path name or the label) and free the
     * array representing the flattened view of the tree.
     */
    if (Blt_ConfigModified(comboSpecs, "-hideleaves", (char *)NULL)) {
	Entry *ep;
	
	comboPtr->flags |= DIRTY;
	/* Mark all entries dirty. */
	for (ep = comboPtr->rootPtr; ep != NULL; ep = NextEntry(ep, 0)) {
	    ep->flags |= ENTRY_DIRTY;
	}
    }
    if ((comboPtr->reqHeight != Tk_ReqHeight(comboPtr->tkwin)) ||
	(comboPtr->reqWidth != Tk_ReqWidth(comboPtr->tkwin))) {
	Tk_GeometryRequest(comboPtr->tkwin, comboPtr->reqWidth, 
		comboPtr->reqHeight);
    }
    if (Blt_ConfigModified(comboSpecs, "-tree", (char *)NULL)) {
	DestroyEntries(comboPtr);
	Blt_InitHashTableWithPool(&comboPtr->entryTable, BLT_ONE_WORD_KEYS);
	if (Blt_Tree_Attach(interp, comboPtr->tree, comboPtr->treeName) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
	comboPtr->flags |= REPOPULATE;
    }	
    /* If the tree object was changed, we need to setup the new one. */
    if (comboPtr->flags & REPOPULATE) {
	Blt_TreeNode root;

	Blt_Tree_CreateEventHandler(comboPtr->tree, TREE_NOTIFY_ALL, 
		TreeEventProc, comboPtr);
	root = Blt_Tree_RootNode(comboPtr->tree);
	/* Automatically add view-entry values to the new tree. */
	Blt_Tree_Apply(root, CreateApplyProc, comboPtr);
	comboPtr->rootPtr = NodeToEntry(comboPtr, root);
	/* Automatically open the root node. */
	if (OpenEntry(comboPtr, comboPtr->rootPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (comboPtr->flags & NEW_TAGS) {
	    Blt_Tree_NewTagTable(comboPtr->tree);
	}
	comboPtr->flags &= ~REPOPULATE;
    }

    updateNeeded = FALSE;
    /* Install the embedded scrollbars as needed.  We defer installing the
     * scrollbars so the scrollbar widgets don't have to exist when they are
     * specified by the -xscrollbar and -yscrollbar options respectively. The
     * down-side is that errors found in the scrollbar name will be
     * backgrounded. */
    if (Blt_ConfigModified(comboSpecs, "-xscrollbar", (char *)NULL)) {
	if (comboPtr->xScrollbar != NULL) {
	    UnmanageScrollbar(comboPtr, comboPtr->xScrollbar);
	    comboPtr->xScrollbar = NULL;
	}
	if ((comboPtr->flags & INSTALL_XSCROLLBAR) == 0) {
	    Tcl_DoWhenIdle(InstallXScrollbar, comboPtr);
	    comboPtr->flags |= INSTALL_XSCROLLBAR;
	}	    
	updateNeeded = TRUE;
    }
    if (Blt_ConfigModified(comboSpecs, "-yscrollbar", (char *)NULL)) {
	if (comboPtr->yScrollbar != NULL) {
	    UnmanageScrollbar(comboPtr, comboPtr->yScrollbar);
	    comboPtr->yScrollbar = NULL;
	}
	if ((comboPtr->flags & INSTALL_YSCROLLBAR) == 0) {
	    Tcl_DoWhenIdle(InstallYScrollbar, comboPtr);
	    comboPtr->flags |= INSTALL_YSCROLLBAR;
	}	    
	updateNeeded = TRUE;
    }
    if (updateNeeded) {
	if ((comboPtr->flags & UPDATE_PENDING) == 0) {
	    Tcl_DoWhenIdle(ConfigureScrollbarsProc, comboPtr);
	    comboPtr->flags |= UPDATE_PENDING;
	}	    
    }
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}



#ifdef notdef
static void
PrintFlags(ComboTree *comboPtr, char *string)
{    
    fprintf(stderr, "%s: flags=", string);
    if (comboPtr->flags & LAYOUT_PENDING) {
	fprintf(stderr, "layout ");
    }
    if (comboPtr->flags & REDRAW_PENDING) {
	fprintf(stderr, "redraw ");
    }
    if (comboPtr->flags & SCROLLX) {
	fprintf(stderr, "xscroll ");
    }
    if (comboPtr->flags & SCROLLY) {
	fprintf(stderr, "yscroll ");
    }
    if (comboPtr->flags & FOCUS) {
	fprintf(stderr, "focus ");
    }
    if (comboPtr->flags & DIRTY) {
	fprintf(stderr, "dirty ");
    }
    if (comboPtr->flags & REDRAW_BORDERS) {
	fprintf(stderr, "borders ");
    }
    if (comboPtr->flags & VIEWPORT) {
	fprintf(stderr, "viewport ");
    }
    fprintf(stderr, "\n");
}
#endif

static void
FixMenuCoords(ComboTree *comboPtr, int *xPtr, int *yPtr)
{
    int x, y, w, h;
    int screenWidth, screenHeight;

    Blt_SizeOfScreen(comboPtr->tkwin, &screenWidth, &screenHeight);
    x = *xPtr, y = *yPtr;

    /* Determine the size of the menu. */
    w = Tk_ReqWidth(comboPtr->tkwin);
    h = Tk_ReqHeight(comboPtr->tkwin);
    if ((y + h) > screenHeight) {
	y -= h;				/* Shift the menu up by the height of
					 * the menu. */
	if (comboPtr->flags & DROPDOWN) {
	    y -= comboPtr->butHeight;	/* Add the height of the parent if
					 * this is a dropdown menu.  */
	}
	if (y < 0) {
	    y = 0;
	}
    }
    if ((x + w) > screenWidth) {
	if (comboPtr->flags & DROPDOWN) {
	    x = x + comboPtr->butWidth - w; /* Flip the menu anchor to the other
					 * end of the menu button/entry */
	} else {
	    x -= w;			/* Shift the menu to the left by the
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
 * ComputeVisibleEntries --
 *
 *	The entries visible in the viewport (the widget's window) are
 *	inserted into the array of visible nodes.
 *
 * Results:
 *	Returns 1 if beyond the last visible entry, 0 otherwise.
 *
 * Side effects:
 *	The array of visible nodes is filled.
 *
 *---------------------------------------------------------------------------
 */
static int
ComputeVisibleEntries(ComboTree *comboPtr)
{
    int numSlots;
    int maxX, maxY;
    int xOffset, yOffset;
    Entry *entryPtr;

    xOffset = Blt_AdjustViewport(comboPtr->xOffset, comboPtr->worldWidth,
	VPORTWIDTH(comboPtr), comboPtr->xScrollUnits, BLT_SCROLL_MODE_HIERBOX);
    yOffset = Blt_AdjustViewport(comboPtr->yOffset, comboPtr->worldHeight, 
	VPORTHEIGHT(comboPtr), comboPtr->yScrollUnits, BLT_SCROLL_MODE_HIERBOX);

    if ((xOffset != comboPtr->xOffset) || (yOffset != comboPtr->yOffset)) {
	comboPtr->yOffset = yOffset;
	comboPtr->xOffset = xOffset;
	comboPtr->flags |= VIEWPORT;
    }
    /* Allocate worst case number of slots for entry array. */
    numSlots = (VPORTHEIGHT(comboPtr) / comboPtr->minHeight) + 3;
    if (numSlots != comboPtr->numVisible) {
	if (comboPtr->visibleEntries != NULL) {
	    Blt_Free(comboPtr->visibleEntries);
	}
	comboPtr->visibleEntries = Blt_AssertCalloc(numSlots, sizeof(Entry *));
    }
    comboPtr->numVisible = 0;
    comboPtr->visibleEntries[0] = NULL;

    if (comboPtr->rootPtr->flags & ENTRY_HIDE) {
	return TCL_OK;		/* Root node is hidden. */
    }

    /* Find the first node in the viewport. */
    entryPtr = comboPtr->rootPtr;
    while ((entryPtr->worldY + entryPtr->height) <= comboPtr->yOffset) {
	for (entryPtr = LastChild(entryPtr, ENTRY_HIDE); entryPtr != NULL; 
	     entryPtr = PrevSibling(entryPtr, ENTRY_HIDE)) {
	    if (entryPtr->worldY <= comboPtr->yOffset) {
		break;
	    }
	}
	/*
	 * If we can't find the starting node, then the view must be
	 * scrolled down, but some nodes were deleted.  Reset the view
	 * back to the top and try again.
	 */
	if (entryPtr == NULL) {
	    if (comboPtr->yOffset == 0) {
		return TCL_OK;	/* All entries are hidden. */
	    }
	    comboPtr->yOffset = 0;
	    continue;
	}
    }

    maxY = comboPtr->yOffset + VPORTHEIGHT(comboPtr);
    maxX = 0;
    while (entryPtr != NULL) {
	int x;
	int level;
	
	/*
	 * Compute and save the entry's X-coordinate now that we know
	 * the maximum level offset for the entire widget.
	 */
	level = Blt_Tree_NodeDepth(entryPtr->node);
	entryPtr->worldX = LEVELX(level);
	x = entryPtr->worldX + ICONWIDTH(level) + ICONWIDTH(level+1) + 
	    entryPtr->width;
	if (x > maxX) {
	    maxX = x;
	}
	if (entryPtr->worldY >= maxY) {
	    break;		/* Entry starts after viewport. */
	}
	comboPtr->visibleEntries[comboPtr->numVisible] = entryPtr;
	comboPtr->numVisible++;
	entryPtr = NextEntry(entryPtr, ENTRY_MASK);
    }
    comboPtr->visibleEntries[comboPtr->numVisible] = NULL;

    /*
     *-------------------------------------------------------------------------------
     *
     * Note:	It's assumed that the view port always starts at or over an 
     *		entry.  Check that a change in the hierarchy (e.g. closing a
     *		node) hasn't left the viewport beyond the last entry.  If so,
     *		adjust the viewport to start on the last entry.
     *
     *-------------------------------------------------------------------------------
     */
    if (comboPtr->xOffset > (comboPtr->worldWidth - comboPtr->xScrollUnits)) {
	comboPtr->xOffset = comboPtr->worldWidth - comboPtr->xScrollUnits;
    }
    if (comboPtr->yOffset > (comboPtr->worldHeight - comboPtr->yScrollUnits)) {
	comboPtr->yOffset = comboPtr->worldHeight - comboPtr->yScrollUnits;
    }
    comboPtr->xOffset = Blt_AdjustViewport(comboPtr->xOffset, 
	comboPtr->worldWidth, VPORTWIDTH(comboPtr), comboPtr->xScrollUnits, 
	BLT_SCROLL_MODE_HIERBOX);
    comboPtr->yOffset = Blt_AdjustViewport(comboPtr->yOffset,
	comboPtr->worldHeight, VPORTHEIGHT(comboPtr), comboPtr->yScrollUnits,
	BLT_SCROLL_MODE_HIERBOX);

    comboPtr->flags &= ~DIRTY;
    Blt_PickCurrentItem(comboPtr->bindTable);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetEntryIcon --
 *
 * 	Selects the correct image for the entry's icon depending upon
 *	the current state of the entry: active/inactive normal/selected.  
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
GetEntryIcon(ComboTree *comboPtr, Entry *entryPtr)
{
    Icon *icons;
    Icon icon;

    icons = entryPtr->stylePtr->icons;
    icon = NULL;
    if (icons != NULL) {	
	icon = icons[0];	/* Open icon. */
	if ((entryPtr->flags & ENTRY_CLOSED) && (icons[1] != NULL)) {
	    icon = icons[1];	/* Closed icon. */
	}
    }
    return icon;
}
/*
 *---------------------------------------------------------------------------
 *
 * DrawButton --
 *
 * 	Draws a button for the given entry. The button is drawn
 * 	centered in the region immediately to the left of the origin
 * 	of the entry (computed in the layout routines). The height
 * 	and width of the button were previously calculated from the
 * 	average row height.
 *
 *		button height = entry height - (2 * some arbitrary padding).
 *		button width = button height.
 *
 *	The button may have a border.  The symbol (either a plus or
 *	minus) is slight smaller than the width or height minus the
 *	border.
 *
 *	    x,y origin of entry
 *
 *              +---+
 *              | + | icon label
 *              +---+
 *             closed
 *
 *           |----|----| horizontal offset
 *
 *              +---+
 *              | - | icon label
 *              +---+
 *              open
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A button is drawn for the entry.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawButton(
    ComboTree *comboPtr,	/* Widget record containing the
				 * attribute information for
				 * buttons. */
    Entry *entryPtr,		/* Entry. */
    Drawable drawable,		/* Pixmap or window to draw into. */
    int x, 
    int y)
{
    Blt_Bg bg;
    Button *btnPtr = &comboPtr->button;
    Icon icon;
    int relief;
    int width, height;

    bg = (entryPtr == comboPtr->activeBtnPtr) 
	? btnPtr->activeBg : btnPtr->normalBg;
    relief = (entryPtr->flags & ENTRY_CLOSED) 
	? btnPtr->closeRelief : btnPtr->openRelief;
    if (relief == TK_RELIEF_SOLID) {
	relief = TK_RELIEF_FLAT;
    }
    Blt_Bg_FillRectangle(comboPtr->tkwin, drawable, bg, x, y,
	btnPtr->width, btnPtr->height, btnPtr->borderWidth, relief);

    x += btnPtr->borderWidth;
    y += btnPtr->borderWidth;
    width = btnPtr->width - (2 * btnPtr->borderWidth);
    height = btnPtr->height - (2 * btnPtr->borderWidth);

    icon = NULL;
    if (btnPtr->icons != NULL) {  /* Open or close button icon? */
	icon = btnPtr->icons[0];
	if (((entryPtr->flags & ENTRY_CLOSED) == 0) && 
	    (btnPtr->icons[1] != NULL)) {
	    icon = btnPtr->icons[1];
	}
    }
    if (icon != NULL) {		/* Icon or rectangle? */
	Tk_RedrawImage(IconImage(icon), 0, 0, width, height, drawable, x, y);
    } else {
	int top, bottom, left, right;
	XSegment segments[6];
	int count;
	GC gc;

	gc = (entryPtr == comboPtr->activeBtnPtr)
	    ? btnPtr->activeGC : btnPtr->normalGC;
	if (relief == TK_RELIEF_FLAT) {
	    /* Draw the box outline */

	    left = x - btnPtr->borderWidth;
	    top = y - btnPtr->borderWidth;
	    right = left + btnPtr->width - 1;
	    bottom = top + btnPtr->height - 1;

	    segments[0].x1 = left;
	    segments[0].x2 = right;
	    segments[0].y2 = segments[0].y1 = top;
	    segments[1].x2 = segments[1].x1 = right;
	    segments[1].y1 = top;
	    segments[1].y2 = bottom;
	    segments[2].x2 = segments[2].x1 = left;
	    segments[2].y1 = top;
	    segments[2].y2 = bottom;
#ifdef WIN32
	    segments[2].y2++;
#endif
	    segments[3].x1 = left;
	    segments[3].x2 = right;
	    segments[3].y2 = segments[3].y1 = bottom;
#ifdef WIN32
	    segments[3].x2++;
#endif
	}
	top = y + height / 2;
	left = x + BUTTON_IPAD;
	right = x + width - BUTTON_IPAD;

	segments[4].y1 = segments[4].y2 = top;
	segments[4].x1 = left;
	segments[4].x2 = right - 1;
#ifdef WIN32
	segments[4].x2++;
#endif

	count = 5;
	if (entryPtr->flags & ENTRY_CLOSED) { /* Draw the vertical
					       * line for the plus. */
	    top = y + BUTTON_IPAD;
	    bottom = y + height - BUTTON_IPAD;
	    segments[5].y1 = top;
	    segments[5].y2 = bottom - 1;
	    segments[5].x1 = segments[5].x2 = x + width / 2;
#ifdef WIN32
	    segments[5].y2++;
#endif
	    count = 6;
	}
	XDrawSegments(comboPtr->display, drawable, gc, segments, count);
    }
}

static int
DrawComboIcon(ComboTree *comboPtr, Entry *entryPtr, Drawable drawable, int x, 
	      int y)
{
    Icon icon;

    icon = GetEntryIcon(comboPtr, entryPtr);
    if (icon != NULL) {			/* Icon or default icon bitmap? */
	int entryHeight;
	int level;
	int maxY;
	int top, bottom;
	int topInset, botInset;
	int width, height;

	level = Blt_Tree_NodeDepth(entryPtr->node);
	entryHeight = MAX3(entryPtr->lineHeight, entryPtr->iconHeight, 
		comboPtr->button.height);
	height = IconHeight(icon);
	width = IconWidth(icon);
	if (comboPtr->flatView) {
	    x += (ICONWIDTH(0) - width) / 2;
	} else {
	    x += (ICONWIDTH(level + 1) - width) / 2;
	}	    
	y += (entryHeight - height) / 2;
	botInset = comboPtr->inset - INSET_PAD;
	topInset = comboPtr->inset;
	maxY = Tk_Height(comboPtr->tkwin) - botInset;
	top = 0;
	bottom = y + height;
	if (y < topInset) {
	    height += y - topInset;
	    top = -y + topInset;
	    y = topInset;
	} else if (bottom >= maxY) {
	    height = maxY - y;
	}
	Tk_RedrawImage(IconImage(icon), 0, top, width, height, drawable, x, y);
    } 
    return (icon != NULL);
}

static int
DrawLabel(ComboTree *comboPtr, Entry *entryPtr, Drawable drawable, int x, int y,
	  int maxLength)			
{
    const char *label;
    int entryHeight;

    entryHeight = MAX3(entryPtr->lineHeight, entryPtr->iconHeight, 
       comboPtr->button.height);

    /* Center the label, if necessary, vertically along the entry row. */
    if (entryPtr->labelHeight < entryHeight) {
	y += (entryHeight - entryPtr->labelHeight) / 2;
    }
    x += LABEL_PADX;
    y += LABEL_PADY;

    label = GETLABEL(entryPtr);
    if (label[0] != '\0') {
	Style *stylePtr;
	TextStyle ts;
	XColor *color;

	stylePtr = entryPtr->stylePtr;
	if (entryPtr == comboPtr->activePtr) {
	    color = stylePtr->labelActiveColor;
	} else {
	    color = stylePtr->labelNormalColor;
	}
	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, stylePtr->labelFont);
	Blt_Ts_SetForeground(ts, color);
	Blt_Ts_SetMaxLength(ts, maxLength);
	Blt_Ts_DrawLayout(comboPtr->tkwin, drawable, entryPtr->textPtr, &ts, 
		x, y);
	if (entryPtr == comboPtr->activePtr) {
	    Blt_Ts_UnderlineLayout(comboPtr->tkwin, drawable, entryPtr->textPtr,
		&ts, x, y);
	}
    }
    return entryHeight;
}

static void
DrawEntryBackground(
    ComboTree *comboPtr, 
    Entry *entryPtr, 
    Drawable drawable,
    int x, int y, int w, int h)
{
    Blt_Bg bg;
    Style *stylePtr;
    int relief;

    stylePtr = entryPtr->stylePtr;
    if (entryPtr == comboPtr->activePtr) {
	bg = stylePtr->activeBg;
	relief = stylePtr->activeRelief;
    } else if ((stylePtr->altBg != NULL) && (entryPtr->seqNum & 0x1)) {
	bg = stylePtr->altBg;
	relief = stylePtr->relief;
    } else {
	bg = stylePtr->normalBg;
	relief = stylePtr->relief;
    }
    /* This also fills the background where there are no entries. */
    Blt_Bg_FillRectangle(comboPtr->tkwin, drawable, bg, x, y, w, h, 
	stylePtr->borderWidth, relief);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawEntry --
 *
 * 	Draws a button for the given entry.  Note that buttons should only be
 * 	drawn if the entry has sub-entries to be opened or closed.  It's the
 * 	responsibility of the calling routine to ensure this.
 *
 *	The button is drawn centered in the region immediately to the left of
 *	the origin of the entry (computed in the layout routines). The height
 *	and width of the button were previously calculated from the average
 *	row height.
 *
 *		button height = entry height - (2 * some arbitrary padding).
 *		button width = button height.
 *
 *	The button has a border.  The symbol (either a plus or minus) is
 *	slight smaller than the width or height minus the border.
 *
 *	    x,y origin of entry
 *
 *              +---+
 *              | + | icon label
 *              +---+
 *             closed
 *
 *           |----|----| horizontal offset
 *
 *              +---+
 *              | - | icon label
 *              +---+
 *              open
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A button is drawn for the entry.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawEntry(ComboTree *comboPtr, Entry *entryPtr, Drawable drawable, 
	  int x, int y, int w, int h)
{
    Button *btnPtr = &comboPtr->button;
    int buttonY;
    int level;
    int xMax;
    int x1, y1, x2, y2;
    GC gc;

    entryPtr->flags &= ~ENTRY_REDRAW;

    if ((comboPtr->activePtr == entryPtr) && (y == 0)) {
	gc = comboPtr->defStyle.labelActiveGC;
    } else {
	gc = comboPtr->lineGC;
    }
    level = Blt_Tree_NodeDepth(entryPtr->node);
    w = ICONWIDTH(level);
    h = MAX3(entryPtr->lineHeight, entryPtr->iconHeight, btnPtr->height);

    entryPtr->buttonX = (w - btnPtr->width) / 2;
    entryPtr->buttonY = (h - btnPtr->height) / 2;

    buttonY = y + entryPtr->buttonY;

    x1 = x + (w / 2);
    y1 = y2 = buttonY + (btnPtr->height / 2);
    x2 = x1 + (ICONWIDTH(level) + ICONWIDTH(level + 1)) / 2;

    if ((Blt_Tree_ParentNode(entryPtr->node) != NULL) && 
	(comboPtr->lineWidth > 0)) {
	/*
	 * For every node except root, draw a horizontal line from the
	 * vertical bar to the middle of the icon.
	 */
	XDrawLine(comboPtr->display, drawable, gc, x1, y1, x2, y2);
    }
    if (((entryPtr->flags & ENTRY_CLOSED) == 0) && (comboPtr->lineWidth > 0) &&
	(entryPtr->vertLineLength > 0)) {
	/*
	 * Entry is open, draw vertical line.
	 */
	y2 = y1 + entryPtr->vertLineLength;
	if (y2 > Tk_Height(comboPtr->tkwin)) {
	    y2 = Tk_Height(comboPtr->tkwin); /* Clip line at window border. */
	}
	XDrawLine(comboPtr->display, drawable, gc, x2, y1, x2, y2);
    }
    if ((entryPtr->flags & ENTRY_BUTTON) && (entryPtr != comboPtr->rootPtr)) {
	/*
	 * Except for the root, draw a button for every entry that needs one.
	 * The displayed button can be either an icon (Tk image) or a line
	 * drawing (rectangle with plus or minus sign).
	 */
	DrawButton(comboPtr, entryPtr, drawable, x + entryPtr->buttonX,
		y + entryPtr->buttonY);
    }
    x += ICONWIDTH(level);

    if (!DrawComboIcon(comboPtr, entryPtr, drawable, x, y)) {
	x -= (ICON_WIDTH * 2) / 3;
    }
    x += ICONWIDTH(level + 1) + 4;

    /* Entry label. */
    xMax = comboPtr->worldWidth;
    DrawLabel(comboPtr, entryPtr, drawable, x, y, xMax - x);
}

static void
DrawEntryBackgrounds(ComboTree *comboPtr, Drawable drawable)
{
    int x;
    int width;
    Entry **entryPtrPtr;

    width = Tk_Width(comboPtr->tkwin);
    x = comboPtr->inset;

    /* This also fills the background where there are no entries. */
    Blt_Bg_FillRectangle(comboPtr->tkwin, drawable, 
	comboPtr->defStyle.normalBg, x, 0, width, Tk_Height(comboPtr->tkwin), 
	0, TK_RELIEF_FLAT);

    for (entryPtrPtr = comboPtr->visibleEntries; *entryPtrPtr != NULL; 
	 entryPtrPtr++) {
	Blt_Bg bg;
	Style *stylePtr;
	Entry *entryPtr;
	int relief;
	int y;

	entryPtr = *entryPtrPtr;
	stylePtr = entryPtr->stylePtr;
	if (entryPtr == comboPtr->activePtr) {
	    bg = stylePtr->activeBg;
	    relief = stylePtr->activeRelief;
	} else if ((stylePtr->altBg != NULL) && (entryPtr->seqNum & 0x1)) {
	    bg = stylePtr->altBg;
	    relief = stylePtr->relief;
	} else {
	    bg = stylePtr->normalBg;
	    relief = stylePtr->relief;
	}
	y = SCREENY(comboPtr, entryPtr->worldY) - 1;
	Blt_Bg_FillRectangle(comboPtr->tkwin, drawable, bg, x, y, width, 
		entryPtr->height, stylePtr->borderWidth, relief);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawVerticals --
 *
 * 	Draws vertical lines for the ancestor nodes.  While the entry
 *	of the ancestor may not be visible, its vertical line segment
 *	does extent into the viewport.  So walk back up the hierarchy
 *	drawing lines until we get to the root.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Vertical lines are drawn for the ancestor nodes.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawVerticals(ComboTree *comboPtr, Entry *entryPtr, Drawable drawable,
	      int xOffset, int yOffset)
{ 
    GC gc;

    if ((comboPtr->activePtr == entryPtr) && (yOffset > 0)) {
	gc = comboPtr->defStyle.labelActiveGC;
    } else {
	gc = comboPtr->lineGC;
    }
    while (entryPtr != comboPtr->rootPtr) {
	entryPtr = ParentEntry(entryPtr);
	if (entryPtr == NULL) {
	    break;
	}
	if (entryPtr->vertLineLength > 0) {
	    int level;
	    int ax, ay, bx, by;
	    int x, y;
	    int height;

	    /*
	     * World X-coordinates aren't computed for entries that are
	     * outside the view port.  So for each off-screen ancestor node
	     * compute it here too.
	     */
	    level = Blt_Tree_NodeDepth(entryPtr->node);
	    entryPtr->worldX = LEVELX(level);
	    x = SCREENX(comboPtr, entryPtr->worldX) - xOffset;
	    y = SCREENY(comboPtr, entryPtr->worldY) - yOffset;
	    height = MAX3(entryPtr->lineHeight, entryPtr->iconHeight, 
			  comboPtr->button.height);
	    y += (height - comboPtr->button.height) / 2;
	    ax = bx = x + ICONWIDTH(level) + ICONWIDTH(level + 1) / 2;
	    ay = y + comboPtr->button.height / 2;
	    by = ay + entryPtr->vertLineLength;
	    if ((entryPtr == comboPtr->rootPtr) && 
		(comboPtr->flags & HIDE_ROOT)) {
		ay += entryPtr->height;
	    }
	    /*
	     * Clip the line's Y-coordinates at the viewport's borders.
	     */
	    if (ay < 0) {
		ay = (ay & 0x1);	/* Make sure the dotted line starts on 
					 * the same even/odd pixel. */
	    }
	    if (by > Tk_Height(comboPtr->tkwin)) {
		by = Tk_Height(comboPtr->tkwin);
	    }
	    if ((ay < Tk_Height(comboPtr->tkwin)) && (by > 0)) {
		XDrawLine(comboPtr->display, drawable, gc, 
			  ax, ay, bx, by);
	    }
	}
    }
}

static void
DrawComboTree(ComboTree *comboPtr, Drawable drawable)
{
    Entry **entryPtrPtr;

    DrawEntryBackgrounds(comboPtr, drawable);

    if ((comboPtr->lineWidth > 0) && (comboPtr->numVisible > 0)) { 
	/* Draw all the vertical lines from topmost node. */
	DrawVerticals(comboPtr, comboPtr->visibleEntries[0], drawable, 0, 0);
    }
    for (entryPtrPtr = comboPtr->visibleEntries; *entryPtrPtr != NULL; 
	 entryPtrPtr++) {
	int x, y, w, h;
	Entry *entryPtr;

	entryPtr = *entryPtrPtr;
	x = SCREENX(comboPtr, entryPtr->worldX);
	y = SCREENY(comboPtr, entryPtr->worldY);
	w = ICONWIDTH(Blt_Tree_NodeDepth(entryPtr->node));
	h = MAX3(entryPtr->lineHeight, entryPtr->iconHeight, 
		 comboPtr->button.height);
	DrawEntry(comboPtr, entryPtr, drawable, x, y, w, h);
    }

    /* Manage the geometry of the embedded scrollbars. */

    if (comboPtr->yScrollbarWidth > 0) {
	int x, y;
	int yScrollbarHeight;

	x = Tk_Width(comboPtr->tkwin) - comboPtr->borderWidth -
	    comboPtr->yScrollbarWidth;
	y = comboPtr->borderWidth;
	yScrollbarHeight = Tk_Height(comboPtr->tkwin) - 
	    comboPtr->xScrollbarHeight - 2 * comboPtr->borderWidth;
	if ((Tk_Width(comboPtr->yScrollbar) != comboPtr->yScrollbarWidth) ||
	    (Tk_Height(comboPtr->yScrollbar) != yScrollbarHeight) ||
	    (x != Tk_X(comboPtr->yScrollbar)) || 
	    (y != Tk_Y(comboPtr->yScrollbar))) {
	    Tk_MoveResizeWindow(comboPtr->yScrollbar, x, y, 
		comboPtr->yScrollbarWidth, yScrollbarHeight);
	}
	if (!Tk_IsMapped(comboPtr->yScrollbar)) {
	    Tk_MapWindow(comboPtr->yScrollbar);
	}
    } else if ((comboPtr->yScrollbar != NULL) &&
	       (Tk_IsMapped(comboPtr->yScrollbar))) {
	Tk_UnmapWindow(comboPtr->yScrollbar);
    }
    if (comboPtr->xScrollbarHeight > 0) {
	int x, y;
	int xScrollbarWidth;

	x = comboPtr->borderWidth;
	y = Tk_Height(comboPtr->tkwin) - comboPtr->xScrollbarHeight - 
	    comboPtr->borderWidth;
	xScrollbarWidth = Tk_Width(comboPtr->tkwin) - 
	    comboPtr->yScrollbarWidth - 2 * comboPtr->borderWidth;
	if ((Tk_Width(comboPtr->xScrollbar) != xScrollbarWidth) ||
	    (Tk_Height(comboPtr->xScrollbar) != comboPtr->xScrollbarHeight) ||
	    (x != Tk_X(comboPtr->xScrollbar)) || 
	    (y != Tk_Y(comboPtr->xScrollbar))) {
	    Tk_MoveResizeWindow(comboPtr->xScrollbar, x, y, xScrollbarWidth,
		comboPtr->xScrollbarHeight);
	}
	if (!Tk_IsMapped(comboPtr->xScrollbar)) {
	    Tk_MapWindow(comboPtr->xScrollbar);
	}
    } else if ((comboPtr->xScrollbar != NULL) && 
	       (Tk_IsMapped(comboPtr->xScrollbar))) {
	Tk_UnmapWindow(comboPtr->xScrollbar);
    }
}

static void
DrawOuterBorders(ComboTree *comboPtr, Drawable drawable)
{
    /* Draw 3D border just inside of the focus highlight ring. */
    if ((comboPtr->borderWidth > 0) && (comboPtr->relief != TK_RELIEF_FLAT)) {
	Blt_Bg_DrawRectangle(comboPtr->tkwin, drawable, 
	    comboPtr->defStyle.normalBg, 0, 0, Tk_Width(comboPtr->tkwin),
	   Tk_Height(comboPtr->tkwin), comboPtr->borderWidth, comboPtr->relief);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayEntry --
 *
 *	This procedure is invoked to display an entry in the combotree widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the entry.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayEntry(ClientData clientData)
{
    Entry *entryPtr = clientData;
    ComboTree *comboPtr;
    Pixmap drawable;
    int x, y, w, h, d, sy;

#ifdef notdef
    fprintf(stderr, "DisplayEntry (%s)\n", GETLABEL(entryPtr));
#endif
    comboPtr = entryPtr->comboPtr;

    /* Create a pixmap the size of the window for double buffering. */
    comboPtr = entryPtr->comboPtr;
    w = VPORTWIDTH(comboPtr);
    h = entryPtr->height;
    drawable = Blt_GetPixmap(comboPtr->display, Tk_WindowId(comboPtr->tkwin),
	w, h, Tk_Depth(comboPtr->tkwin));
#ifdef WIN32
    assert(drawable != None);
#endif
    x = PIXMAPX(comboPtr, entryPtr->worldX);
    y = PIXMAPY(comboPtr, entryPtr->worldY) + comboPtr->borderWidth;
    DrawEntryBackground(comboPtr, entryPtr, drawable, 0, 0, w, h);
    if ((comboPtr->lineWidth > 0) && (comboPtr->numVisible > 0)) { 
	/* Draw all the vertical lines from topmost node. */
	DrawVerticals(comboPtr, entryPtr, drawable, 
		      comboPtr->borderWidth, 
		      SCREENY(comboPtr, entryPtr->worldY));
    }
    DrawEntry(comboPtr, entryPtr, drawable, x, 0, w, h);
    sy = 0;
    d = comboPtr->borderWidth - y;
    if (d > 0) {
	h -= d;
	sy = d;
	y += d;
    }
    d = (y + h) - (Tk_Height(comboPtr->tkwin) - comboPtr->borderWidth);
    if (d > 0) {
	h -= d;
    }
    XCopyArea(comboPtr->display, drawable, Tk_WindowId(comboPtr->tkwin),
	comboPtr->copyGC, 0, sy, w, h, comboPtr->borderWidth, y);
    Tk_FreePixmap(comboPtr->display, drawable);
}

/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 *	Activate the specified entry (draw with active foreground/background).
 *	Only one entry may be active at one time, so the previously active
 *	entry is deactivated.
 *
 * Results:
 *	Standard TCL result.
 *
 * Side effects:
 *	The widget is eventually redraw.
 *
 *	.cm activate entry
 *
 *---------------------------------------------------------------------------
 */
static int
ActivateOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    Entry *entryPtr;

    if (GetEntryFromObj(NULL, comboPtr, objv[2], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    } 
    if (comboPtr->activePtr == entryPtr) {
	return TCL_OK;			/* Entry is already active. */
    }
    ActivateEntry(comboPtr, NULL);	/* Deactive the current active. */
    if (entryPtr != NULL) {
	ActivateEntry(comboPtr, entryPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BindOp --
 *
 *	  .t bind entry sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BindOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ClientData object;
    Entry *entryPtr;
    const char *string;

    /*
     * Entries are selected by id only.  All other strings are interpreted as
     * a binding tag.
     */
    string = Tcl_GetString(objv[2]);
    if (isdigit(UCHAR(string[0]))) {
	Blt_TreeNode node;
	long inode;

	if (Blt_GetLongFromObj(comboPtr->interp, objv[2], &inode) != TCL_OK) {
	    return TCL_ERROR;
	}
	node = Blt_Tree_GetNode(comboPtr->tree, inode);
	object = NodeToEntry(comboPtr, node);
    } else if (GetEntryFromObj(interp, comboPtr, objv[2], &entryPtr)==TCL_OK) {
	if (entryPtr != NULL) {
	    return TCL_OK;		/* Special id doesn't currently
					 * exist. */
	}
	object = entryPtr;
    } else {
	/* Assume that this is a binding tag. */
	object = EntryTag(comboPtr, string);
    } 
    return Blt_ConfigureBindingsFromObj(interp, comboPtr->bindTable, object, 
	 objc - 3, objv + 3);
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonActivateOp --
 *
 *	Selects the button to appear active.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ButtonActivateOp(
    ComboTree *comboPtr, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    Entry *oldPtr, *entryPtr;
    const char *string;

    string = Tcl_GetString(objv[3]);
    if (string[0] == '\0') {
	entryPtr = NULL;
    } else if (GetEntryFromObj(interp, comboPtr, objv[3], &entryPtr) != TCL_OK){
	return TCL_ERROR;
    }
    if ((entryPtr != NULL) && !(entryPtr->flags & ENTRY_BUTTON)) {
	entryPtr = NULL;
    }
    oldPtr = comboPtr->activeBtnPtr;
    comboPtr->activeBtnPtr = entryPtr;
    if (!(comboPtr->flags & REDRAW_PENDING) && (entryPtr != oldPtr)) {
	if ((oldPtr != NULL) && (oldPtr != comboPtr->rootPtr)) {
#ifdef notdef
	    DrawButton(comboPtr, oldPtr);
#endif
	}
	if ((entryPtr != NULL) && (entryPtr != comboPtr->rootPtr)) {
#ifdef notdef
	    DrawButton(comboPtr, entryPtr);
#endif
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonBindOp --
 *
 *	  .ct bind tag sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ButtonBindOp(
    ComboTree *comboPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    ClientData object;

    /* Assume that this is a binding tag. */
    object = ButtonTag(comboPtr, Tcl_GetString(objv[3]));
    return Blt_ConfigureBindingsFromObj(interp, comboPtr->bindTable, object, 
	objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonCgetOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ButtonCgetOp(
    ComboTree *comboPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    return Blt_ConfigureValueFromObj(interp, comboPtr->tkwin, buttonSpecs, 
	(char *)comboPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonConfigureOp --
 *
 * 	This procedure is called to process a list of configuration options
 * 	database, in order to reconfigure the one of more entries in the
 * 	widget.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font, etc. get
 *	set for comboPtr; old resources get freed, if there were any.  
 *
 *	  .ct button configure option value
 *
 *---------------------------------------------------------------------------
 */
static int
ButtonConfigureOp(
    ComboTree *comboPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, buttonSpecs, 
		(char *)comboPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, buttonSpecs, 
		(char *)comboPtr, objv[3], 0);
    }
    iconsOption.clientData = comboPtr;
    if (Blt_ConfigureWidgetFromObj(comboPtr->interp, comboPtr->tkwin, 
	buttonSpecs, objc - 3, objv + 3, (char *)comboPtr, 
	BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    ConfigureButtons(comboPtr);
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonOp --
 *
 *	This procedure handles button operations.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec buttonOps[] =
{
    {"activate",  1, ButtonActivateOp,  4, 4, "entry",},
    {"bind",      1, ButtonBindOp,      4, 6, "tagName ?sequence command?",},
    {"cget",      2, ButtonCgetOp,      4, 4, "option",},
    {"configure", 2, ButtonConfigureOp, 3, 0, "?option value?...",},
    {"highlight", 1, ButtonActivateOp,  4, 4, "entry",},
};

static int numButtonOps = sizeof(buttonOps) / sizeof(Blt_OpSpec);

static int
ButtonOp(
    ComboTree *comboPtr, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    ComboTreeCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numButtonOps, buttonOps, BLT_OP_ARG2, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (comboPtr, interp, objc, objv);
    return result;
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
CgetOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    return Blt_ConfigureValueFromObj(interp, comboPtr->tkwin, comboSpecs,
	(char *)comboPtr, objv[2], 0);
}

/*ARGSUSED*/
static int
CloseOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int recurse, result;
    Entry *entryPtr;
    EntryIterator iter;

    recurse = FALSE;
    if (objc > 2) {
	const char *string;
	int length;

	string = Tcl_GetStringFromObj(objv[2], &length);
	if ((string[0] == '-') && (length > 1) && 
	    (strncmp(string, "-recurse", length) == 0)) {
	    objv++, objc--;
	    recurse = TRUE;
	}
    }
    if (GetEntryIterator(interp, comboPtr, objv[2], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
	 entryPtr = NextTaggedEntry(&iter)) {
	    
	/*
	 * Check if either the active entry is in this hierarchy.  Must move
	 * it or disable it before we close the node.  Otherwise it may be
	 * deleted by a TCL "close" script, and we'll be left pointing to a
	 * bogus memory location.
	 */
	if ((comboPtr->activePtr != NULL) && 
	    (Blt_Tree_IsAncestor(entryPtr->node, comboPtr->activePtr->node))) {
	    comboPtr->activePtr = entryPtr;
	}
	if (recurse) {
	    result = Apply(comboPtr, entryPtr, CloseEntry, 0);
	} else {
	    result = CloseEntry(comboPtr, entryPtr);
	}
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}	
    }
    /* Closing a node may affect the visible entries and the the world layout
     * of the entries. */
    comboPtr->flags |= (LAYOUT_PENDING | DIRTY);
    EventuallyRedraw(comboPtr);
    return TCL_OK;
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
 * Side effects:
 *	Configuration information, such as text string, colors, font, etc. get
 *	set for comboPtr; old resources get freed, if there were any.  The
 *	widget is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    if (objc == 2) {
	return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, comboSpecs,
		(char *)comboPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, 
		comboSpecs, (char *)comboPtr, objv[2], 0);
    } 
    iconsOption.clientData = comboPtr;
    if (ConfigureComboTree(interp, comboPtr, objc - 2, objv + 2, 
	BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * EntryActivateOp --
 *
 *	Selects the entry to appear active.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryActivateOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    Entry *newPtr, *oldPtr;
    const char *string;

    string = Tcl_GetString(objv[3]);
    if (string[0] == '\0') {
	newPtr = NULL;
    } else if (GetEntry(comboPtr, objv[3], &newPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    oldPtr = comboPtr->activePtr;
    comboPtr->activePtr = newPtr;
    if (((comboPtr->flags & REDRAW_PENDING) == 0) && (newPtr != oldPtr)) {
	Drawable drawable;
	int x, y;
	
	drawable = Tk_WindowId(comboPtr->tkwin);
	if (oldPtr != NULL) {
	    x = SCREENX(comboPtr, oldPtr->worldX) + 
		ICONWIDTH(Blt_Tree_NodeDepth(oldPtr->node));
	    y = SCREENY(comboPtr, oldPtr->worldY);
	    oldPtr->flags |= ENTRY_ICON;
	    DrawComboIcon(comboPtr, oldPtr, drawable, x, y);
	}
	if (newPtr != NULL) {
	    x = SCREENX(comboPtr, newPtr->worldX) +
		ICONWIDTH(Blt_Tree_NodeDepth(newPtr->node));
	    y = SCREENY(comboPtr, newPtr->worldY);
	    newPtr->flags |= ENTRY_ICON;
	    DrawComboIcon(comboPtr, newPtr, drawable, x, y);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryCgetOp --
 *
 *	  .ct entry cget entry option
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryCgetOp(
    ComboTree *comboPtr, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    Entry *entryPtr;

    if (GetEntry(comboPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return Blt_ConfigureValueFromObj(interp, comboPtr->tkwin, entrySpecs, 
	(char *)entryPtr, objv[4], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryConfigureOp --
 *
 * 	This procedure is called to process a list of configuration options
 * 	database, in order to reconfigure the one of more entries in the
 * 	widget.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font, etc. get
 *	set for comboPtr; old resources get freed, if there were any.  The
 *	hypertext is redisplayed.
 *
 *	  .ct entry configure entry option value
 *
 *---------------------------------------------------------------------------
 */
static int
EntryConfigureOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *const *objv)
{
    EntryIterator iter;
    Entry *entryPtr;
    
    iconsOption.clientData = comboPtr;
    uidOption.clientData = comboPtr;
    
    if (GetEntryIterator(interp, comboPtr, objv[3], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
	 entryPtr = NextTaggedEntry(&iter)) {
	if (objc == 4) {
	    return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, 
		entrySpecs, (char *)entryPtr, (Tcl_Obj *)NULL, 0);
	} else if (objc == 5) {
	    return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, 
		entrySpecs, (char *)entryPtr, objv[4], 0);
	} 
	if (ConfigureEntry(comboPtr, entryPtr, objc, objv, 
		BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    comboPtr->flags |= (DIRTY | LAYOUT_PENDING | SCROLL_PENDING);
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryIsHiddenOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryIsHiddenOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    Entry *entryPtr;
    int bool;

    if (GetEntry(comboPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    bool = (entryPtr->flags & ENTRY_HIDE);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryIsOpenOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryIsOpenOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    Entry *entryPtr;
    int bool;

    if (GetEntry(comboPtr, objv[3], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    bool = ((entryPtr->flags & ENTRY_CLOSED) == 0);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryOp --
 *
 *	This procedure handles entry operations.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */

static Blt_OpSpec entryOps[] =
{
    {"activate",  1, EntryActivateOp,  4, 4, "entry",},
    {"cget",      2, EntryCgetOp,      5, 5, "entry option",},
    {"configure", 2, EntryConfigureOp, 4, 0, 
	"entry ?entry...? ?option value?...",},
    {"highlight", 1, EntryActivateOp,  4, 4, "entry",},
    {"ishidden",  3, EntryIsHiddenOp,  4, 4, "entry",},
    {"isopen",    3, EntryIsOpenOp,    4, 4, "entry",},
};
static int numEntryOps = sizeof(entryOps) / sizeof(Blt_OpSpec);

static int
EntryOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ComboTreeCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numEntryOps, entryOps, BLT_OP_ARG2, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (comboPtr, interp, objc, objv);
    return result;
}

/*ARGSUSED*/
static int
ExactCompare(Tcl_Interp *interp, const char *name, const char *pattern)
{
    return (strcmp(name, pattern) == 0);
}

/*ARGSUSED*/
static int
GlobCompare(Tcl_Interp *interp, const char *name, const char *pattern)
{
    return Tcl_StringMatch(name, pattern);
}

static int
RegexpCompare(Tcl_Interp *interp, const char *name, const char *pattern)
{
    return Tcl_RegExpMatch(interp, name, pattern);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *
 *	Converts one or more node identifiers to its path component.  The path
 *	may be either the single entry name or the full path of the entry.
 *
 * Results:
 *	A standard TCL result.  The interpreter result will contain a list of
 *	the convert names.
 *
 *---------------------------------------------------------------------------
 */
static int
GetOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int useFullName;
    int i;
    Tcl_DString d1, d2;
    int count;

    useFullName = FALSE;
    if (objc > 2) {
	const char *string;

	string = Tcl_GetString(objv[2]);
	if ((string[0] == '-') && (strcmp(string, "-full") == 0)) {
	    useFullName = TRUE;
	    objv++, objc--;
	}
    }
    Tcl_DStringInit(&d1);	/* Result. */
    Tcl_DStringInit(&d2);	/* Last element. */
    count = 0;
    for (i = 2; i < objc; i++) {
	EntryIterator iter;
	Entry *entryPtr;

	if (GetEntryIterator(interp, comboPtr, objv[i], &iter) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
	     entryPtr = NextTaggedEntry(&iter)) {
	    Tcl_DStringSetLength(&d2, 0);
	    count++;
	    if (entryPtr->node != NULL) {
		if (useFullName) {
		    GetFullName(comboPtr, entryPtr, &d2);
		} else {
		    Tcl_DStringAppend(&d2, Blt_Tree_NodeLabel(entryPtr->node),-1);
		}
		Tcl_DStringAppendElement(&d1, Tcl_DStringValue(&d2));
	    }
	}
    }
    /* This handles the single element list problem. */
    if (count == 1) {
	Tcl_DStringResult(interp, &d2);
	Tcl_DStringFree(&d1);
    } else {
	Tcl_DStringResult(interp, &d1);
	Tcl_DStringFree(&d2);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ShowEntryApplyProc --
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ShowEntryApplyProc(ComboTree *comboPtr, Entry *entryPtr)
{
    entryPtr->flags &= ~ENTRY_HIDE;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HideEntryApplyProc --
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HideEntryApplyProc(ComboTree *comboPtr, Entry *entryPtr)
{
    entryPtr->flags |= ENTRY_HIDE;
    return TCL_OK;
}

static void
MapAncestors(ComboTree *comboPtr, Entry *entryPtr)
{
    while (entryPtr != comboPtr->rootPtr) {
	entryPtr = ParentEntry(entryPtr);
	if (entryPtr->flags & (ENTRY_CLOSED | ENTRY_HIDE)) {
	    comboPtr->flags |= LAYOUT_PENDING;
	    entryPtr->flags &= ~(ENTRY_CLOSED | ENTRY_HIDE);
	} 
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MapAncestorsApplyProc --
 *
 *	If a node in mapped, then all its ancestors must be mapped also.  This
 *	routine traverses upwards and maps each unmapped ancestor.  It's
 *	assumed that for any mapped ancestor, all it's ancestors will already
 *	be mapped too.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
static int
MapAncestorsApplyProc(ComboTree *comboPtr, Entry *entryPtr)
{
    /*
     * Make sure that all the ancestors of this entry are mapped too.
     */
    while (entryPtr != comboPtr->rootPtr) {
	entryPtr = ParentEntry(entryPtr);
	if ((entryPtr->flags & (ENTRY_HIDE | ENTRY_CLOSED)) == 0) {
	    break;		/* Assume ancestors are also mapped. */
	}
	entryPtr->flags &= ~(ENTRY_HIDE | ENTRY_CLOSED);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SearchAndApplyToTree --
 *
 *	Searches through the current tree and applies a procedure to matching
 *	nodes.  The search specification is taken from the following
 *	command-line arguments:
 *
 *      ?-exact? ?-glob? ?-regexp? ?-nonmatching?
 *      ?-data string?
 *      ?-name string?
 *      ?-full string?
 *      ?--?
 *      ?inode...?
 *
 * Results:
 *	A standard TCL result.  If the result is valid, and if the nonmatchPtr
 *	is specified, it returns a boolean value indicating whether or not the
 *	search was inverted.  This is needed to fix things properly for the
 *	"hide nonmatching" case.
 *
 *---------------------------------------------------------------------------
 */
static int
SearchAndApplyToTree(ComboTree *comboPtr, Tcl_Interp *interp, int objc,
		     Tcl_Obj *const *objv, ApplyProc *proc, int *nonMatchPtr)
{
    CompareProc *compareProc;
    int invertMatch;		/* normal search mode (matching entries) */
    const char *namePattern, *fullPattern;
    int i;
    int length;
    int result;
    const char *option, *pattern;
    char c;
    Blt_List options;
    Entry *entryPtr;
    Blt_ListNode node;
    const char *string;
    const char *withTag;
    Tcl_Obj *objPtr;

    options = Blt_List_Create(BLT_ONE_WORD_KEYS);
    invertMatch = FALSE;
    namePattern = fullPattern = NULL;
    compareProc = ExactCompare;
    withTag = NULL;

    entryPtr = comboPtr->rootPtr;
    for (i = 2; i < objc; i++) {
	string = Tcl_GetStringFromObj(objv[i], &length);
	if (string[0] != '-') {
	    break;
	}
	option = string + 1;
	length--;
	c = option[0];
	if ((c == 'e') && (strncmp(option, "exact", length) == 0)) {
	    compareProc = ExactCompare;
	} else if ((c == 'g') && (strncmp(option, "glob", length) == 0)) {
	    compareProc = GlobCompare;
	} else if ((c == 'r') && (strncmp(option, "regexp", length) == 0)) {
	    compareProc = RegexpCompare;
	} else if ((c == 'n') && (length > 1) &&
	    (strncmp(option, "nonmatching", length) == 0)) {
	    invertMatch = TRUE;
	} else if ((c == 'f') && (strncmp(option, "full", length) == 0)) {
	    if ((i + 1) == objc) {
		goto missingArg;
	    }
	    i++;
	    fullPattern = Tcl_GetString(objv[i]);
	} else if ((c == 'n') && (length > 1) && 
		(strncmp(option, "name", length) == 0)) {
	    if ((i + 1) == objc) {
		goto missingArg;
	    }
	    i++;
	    namePattern = Tcl_GetString(objv[i]);
	} else if ((c == 't') && (length > 1) && 
		   (strncmp(option, "tag", length) == 0)) {
	    if ((i + 1) == objc) {
		goto missingArg;
	    }
	    i++;
	    withTag = Tcl_GetString(objv[i]);
	} else if ((option[0] == '-') && (option[1] == '\0')) {
	    break;
	} else {
	    /*
	     * Verify that the switch is actually an entry configuration
	     * option.
	     */
	    if (Blt_ConfigureValueFromObj(interp, comboPtr->tkwin, entrySpecs, 
		(char *)entryPtr, objv[i], 0) != TCL_OK) {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "bad switch \"", string,
	    "\": must be -exact, -glob, -regexp, -name, -full, or -nonmatching",
		    (char *)NULL);
		return TCL_ERROR;
	    }
	    if ((i + 1) == objc) {
		goto missingArg;
	    }
	    /* Save the option in the list of configuration options */
	    node = Blt_List_GetNode(options, (char *)objv[i]);
	    if (node == NULL) {
		node = Blt_List_CreateNode(options, (char *)objv[i]);
		Blt_List_AppendNode(options, node);
	    }
	    i++;
	    Blt_List_SetValue(node, Tcl_GetString(objv[i]));
	}
    }

    if ((namePattern != NULL) || (fullPattern != NULL) ||
	(Blt_List_GetLength(options) > 0)) {
	/*
	 * Search through the tree and look for nodes that match the current
	 * spec.  Apply the input procedure to each of the matching nodes.
	 */
	for (entryPtr = comboPtr->rootPtr; entryPtr != NULL; 
	     entryPtr = NextEntry(entryPtr, 0)) {
	    if (namePattern != NULL) {
		result = (*compareProc) (interp, 
			Blt_Tree_NodeLabel(entryPtr->node), namePattern);
		if (result == invertMatch) {
		    continue;	/* Failed to match */
		}
	    }
	    if (fullPattern != NULL) {
		Tcl_DString ds;

		GetFullName(comboPtr, entryPtr, &ds);
		result = (*compareProc)(interp, Tcl_DStringValue(&ds), 
			fullPattern);
		Tcl_DStringFree(&ds);
		if (result == invertMatch) {
		    continue;	/* Failed to match */
		}
	    }
	    if (withTag != NULL) {
		result = Blt_Tree_HasTag(comboPtr->tree, entryPtr->node, withTag);
		if (result == invertMatch) {
		    continue;	/* Failed to match */
		}
	    }
	    for (node = Blt_List_FirstNode(options); node != NULL;
		node = Blt_List_NextNode(node)) {
		objPtr = (Tcl_Obj *)Blt_List_GetKey(node);
		Tcl_ResetResult(interp);
		if (Blt_ConfigureValueFromObj(interp, comboPtr->tkwin, 
			entrySpecs, (char *)entryPtr, objPtr, 0) != TCL_OK) {
		    return TCL_ERROR;	/* This shouldn't happen. */
		}
		pattern = Blt_List_GetValue(node);
		objPtr = Tcl_GetObjResult(interp);
		result = (*compareProc)(interp, Tcl_GetString(objPtr), pattern);
		if (result == invertMatch) {
		    continue;	/* Failed to match */
		}
	    }
	    /* Finally, apply the procedure to the node */
	    (*proc)(comboPtr, entryPtr);
	}
	Tcl_ResetResult(interp);
	Blt_List_Destroy(options);
    }
    /*
     * Apply the procedure to nodes that have been specified individually.
     */
    for ( /*empty*/ ; i < objc; i++) {
	EntryIterator iter;

	if (GetEntryIterator(interp, comboPtr, objv[i], &iter) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
	     entryPtr = NextTaggedEntry(&iter)) {
	    if ((*proc) (comboPtr, entryPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    if (nonMatchPtr != NULL) {
	*nonMatchPtr = invertMatch;	/* return "inverted search" status */
    }
    return TCL_OK;

  missingArg:
    Blt_List_Destroy(options);
    Tcl_AppendResult(interp, "missing pattern for search option \"", objv[i],
	"\"", (char *)NULL);
    return TCL_ERROR;

}

/*
 *---------------------------------------------------------------------------
 *
 * HideOp --
 *
 *	Hides one or more nodes.  Nodes can be specified by their inode, or by
 *	matching a name or data value pattern.  By default, the patterns are
 *	matched exactly.  They can also be matched using glob-style and
 *	regular expression rules.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
HideOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int status, nonmatching;

    status = SearchAndApplyToTree(comboPtr, interp, objc, objv, 
	HideEntryApplyProc, &nonmatching);

    if (status != TCL_OK) {
	return TCL_ERROR;
    }
    /*
     * If this was an inverted search, scan back through the tree and make
     * sure that the parents for all visible nodes are also visible.  After
     * all, if a node is supposed to be visible, its parent can't be hidden.
     */
    if (nonmatching) {
	Apply(comboPtr, comboPtr->rootPtr, MapAncestorsApplyProc, 0);
    }

    /* Hiding an entry only effects the visible nodes. */
    comboPtr->flags |= (LAYOUT_PENDING | SCROLL_PENDING);
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * IndexOp --
 *
 *	Converts one of more words representing indices of the entries in the
 *	treeview widget to their respective serial identifiers.
 *
 * Results:
 *	A standard TCL result.  Interp->result will contain the identifier of
 *	each inode found. If an inode could not be found, then the serial
 *	identifier will be the empty string.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IndexOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Entry *entryPtr;
    long nodeId;

    nodeId = -1;
    if ((GetEntryFromObj(NULL, comboPtr, objv[2], &entryPtr) == TCL_OK) && 
	(entryPtr != NULL)) {
	nodeId = Blt_Tree_NodeId(entryPtr->node);
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), nodeId);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InvokeOp --
 *
 * Results:
 *	Standard TCL result.
 *
 * Side effects:
 *	Commands may get excecuted; variables may get set.
 *
 *  .ct invoke entry 
 *
 *---------------------------------------------------------------------------
 */
static int
InvokeOp(
    ComboTree *comboPtr, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    int result;
    Entry *entryPtr;

    if (GetEntryFromObj(interp, comboPtr, objv[2], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (entryPtr == NULL) {
	return TCL_OK;		/* Entry is currently disabled. */
    }
    result = TCL_OK;
    Tcl_Preserve((ClientData)entryPtr);
    if (comboPtr->iconVarObjPtr != NULL) {
	Tcl_Obj *objPtr;
	Icon icon;

	icon = GetEntryIcon(comboPtr, entryPtr);
	objPtr = Tcl_NewStringObj(IconName(icon), -1);
	if (Tcl_ObjSetVar2(interp, comboPtr->iconVarObjPtr, NULL, objPtr, 
		TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG) == NULL) {
	    return TCL_ERROR;
	}
    }
    if (comboPtr->textVarObjPtr != NULL) {
	Tcl_Obj *objPtr;
	Tcl_DString ds;

	Tcl_DStringInit(&ds);
	GetFullName(comboPtr, entryPtr, &ds);
	objPtr = Tcl_NewStringObj(Tcl_DStringValue(&ds), -1);
	Tcl_DStringFree(&ds);
	if (Tcl_ObjSetVar2(interp, comboPtr->textVarObjPtr, NULL, objPtr, 
			   TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG) == NULL) {
	    return TCL_ERROR;
	}
    }
    if (entryPtr->cmdObjPtr != NULL) {
	Tcl_IncrRefCount(entryPtr->cmdObjPtr);
	result = Tcl_EvalObjEx(interp, entryPtr->cmdObjPtr, TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(entryPtr->cmdObjPtr);
    }
    Tcl_Release((ClientData)entryPtr);
    return result;
}

/*ARGSUSED*/
static int
NearestOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, 
	  Tcl_Obj *const *objv)
{
    Button *buttonPtr = &comboPtr->button;
    int x, y;			/* Screen coordinates of the test point. */
    Entry *entryPtr;
    int isRoot;
    const char *string;

    isRoot = FALSE;
    string = Tcl_GetString(objv[2]);
    if (strcmp("-root", string) == 0) {
	isRoot = TRUE;
	objv++, objc--;
    } 
    if (objc < 4) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " ", Tcl_GetString(objv[1]), 
		" ?-root? x y\"", (char *)NULL);
	return TCL_ERROR;
			 
    }
    if ((Tk_GetPixelsFromObj(interp, comboPtr->tkwin, objv[2], &x) != TCL_OK) ||
	(Tk_GetPixelsFromObj(interp, comboPtr->tkwin, objv[3], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (comboPtr->numVisible == 0) {
	return TCL_OK;
    }
    if (isRoot) {
	int rootX, rootY;

	Tk_GetRootCoords(comboPtr->tkwin, &rootX, &rootY);
	x -= rootX;
	y -= rootY;
    }
    entryPtr = NearestEntry(comboPtr, x, y, TRUE);
    if (entryPtr == NULL) {
	return TCL_OK;
    }
    x = WORLDX(comboPtr, x);
    y = WORLDY(comboPtr, y);
    if (objc > 4) {
	const char *where;
	int labelX, labelY, depth;
	Icon icon;

	where = "";
	if (entryPtr->flags & ENTRY_BUTTON) {
	    int buttonX, buttonY;

	    buttonX = entryPtr->worldX + entryPtr->buttonX;
	    buttonY = entryPtr->worldY + entryPtr->buttonY;
	    if ((x >= buttonX) && (x < (buttonX + buttonPtr->width)) &&
		(y >= buttonY) && (y < (buttonY + buttonPtr->height))) {
		where = "button";
		goto done;
	    }
	} 
	depth = Blt_Tree_NodeDepth(entryPtr->node);

	icon = GetEntryIcon(comboPtr, entryPtr);
	if (icon != NULL) {
	    int iconWidth, iconHeight, entryHeight;
	    int iconX, iconY;
	    
	    entryHeight = MAX(entryPtr->iconHeight, comboPtr->button.height);
	    iconHeight = IconHeight(icon);
	    iconWidth = IconWidth(icon);
	    iconX = entryPtr->worldX + ICONWIDTH(depth);
	    iconY = entryPtr->worldY;
	    iconX += (ICONWIDTH(depth + 1) - iconWidth) / 2;
	    iconY += (entryHeight - iconHeight) / 2;
	    if ((x >= iconX) && (x <= (iconX + iconWidth)) &&
		(y >= iconY) && (y < (iconY + iconHeight))) {
		where = "icon";
		goto done;
	    }
	}
	labelX = entryPtr->worldX + ICONWIDTH(depth);
	labelY = entryPtr->worldY;
	if (!comboPtr->flatView) {
	    labelX += ICONWIDTH(depth + 1) + 4;
	}	    
	if ((x >= labelX) && (x < (labelX + entryPtr->labelWidth)) &&
	    (y >= labelY) && (y < (labelY + entryPtr->labelHeight))) {
	    where = "label";
	}
    done:
	if (Tcl_SetVar(interp, Tcl_GetString(objv[4]), where, 
		TCL_LEAVE_ERR_MSG) == NULL) {
	    return TCL_ERROR;
	}
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_NodeId(entryPtr->node));
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * OpenOp --
 *
 *	Returns the node identifiers in a given range.
 *
 * .ct open ?-recurse? $entry 
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
OpenOp(
    ComboTree *comboPtr,
    Tcl_Interp *interp,		/* Not used. */
    int objc,
    Tcl_Obj *const *objv)
{
    int recurse, result;
    Entry *entryPtr;
    EntryIterator iter;

    recurse = FALSE;
    if (objc > 2) {
	int length;
	const char *string;

	string = Tcl_GetStringFromObj(objv[2], &length);
	if ((string[0] == '-') && (length > 1) && 
	    (strncmp(string, "-recurse", length) == 0)) {
	    objv++, objc--;
	    recurse = TRUE;
	}
    }
    if (GetEntryIterator(interp, comboPtr, objv[2], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
	 entryPtr = NextTaggedEntry(&iter)) {
	if (recurse) {
	    result = Apply(comboPtr, entryPtr, OpenEntry, 0);
	} else {
	    result = OpenEntry(comboPtr, entryPtr);
	}
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
	/* Make sure ancestors of this node aren't hidden. */
	MapAncestors(comboPtr, entryPtr);
    }
    /*FIXME: This is only for flattened entries.  */
    comboPtr->flags |= (LAYOUT_PENDING | DIRTY | SCROLL_PENDING);

    /* Can't trust the selected entry if nodes have been added or deleted. So
     * recompute the layout. */
    if (comboPtr->flags & LAYOUT_PENDING) {
	ComputeComboGeometry(comboPtr);
    } 
    ComputeVisibleEntries(comboPtr);
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PostOp --
 *
 *	Posts this menu at the given root window coordinates.
 *
 *  .cm post align x y ?x2 y2?
 *   0   1     2   3 4   5  6 
 *---------------------------------------------------------------------------
 */
static int
PostOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    char c;
    const char *string;
    int length;
    int x, y;

    if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) || 
	(Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)) {
	return TCL_ERROR;
    } 
    if (objc == 6) {
	Tcl_AppendResult(interp, "wrong # of arguments: should be \"",
		Tcl_GetString(objv[0]),  " post align x1 y2 ?x2 x2?\"",
		(char *)NULL);
	return TCL_ERROR;
    }
    if (objc == 7) {
	int x2, y2;

	if ((Tcl_GetIntFromObj(interp, objv[5], &x2) != TCL_OK) || 
	    (Tcl_GetIntFromObj(interp, objv[6], &y2) != TCL_OK)) {
	    return TCL_ERROR;
	} 
	comboPtr->butWidth  = (x2  > x) ? x2 - x : x - x2;
	comboPtr->butHeight = (y2  > y) ? y2 - y : y - y2;
	comboPtr->flags |= DROPDOWN;
    } else {
	Tk_Window parent;

	parent = Tk_Parent(comboPtr->tkwin);
	comboPtr->butWidth = comboPtr->normalWidth;
	comboPtr->butHeight = Tk_Height(parent);
    }
    if ((comboPtr->butWidth != comboPtr->lastButtonWidth) ||
	(comboPtr->flags & LAYOUT_PENDING)) {
	ComputeComboGeometry(comboPtr);
    }
    comboPtr->lastButtonWidth = comboPtr->butWidth;
    string = Tcl_GetStringFromObj(objv[2], &length);
    c = string[0];
    if ((c == 'l') && (strncmp(string, "left", length) == 0)) {
	/* Do nothing. */
    } else if ((c == 'r') && (strncmp(string, "right", length) == 0)) {
	x -= GetWidth(comboPtr);
    } else if ((c == 'c') && (strncmp(string, "center", length) == 0)) {
	x -= GetWidth(comboPtr) / 2;
    } else if ((c == 'p') && (strncmp(string, "popup", length) == 0)) {
	/* Do nothing. */
	comboPtr->flags &= ~DROPDOWN;
    } else {
	Tcl_AppendResult(interp, "bad alignment value \"", string, 
		"\": should be left, right, center, or popup.", (char *)NULL);
	return TCL_ERROR;
    }
    FixMenuCoords(comboPtr, &x, &y);
    /*
     * If there is a post command for the menu, execute it.  This may change
     * the size of the menu, so be sure to recompute the menu's geometry if
     * needed.
     */
    if (comboPtr->postCmdObjPtr != NULL) {
	int result;

	Tcl_IncrRefCount(comboPtr->postCmdObjPtr);
	result = Tcl_EvalObjEx(interp, comboPtr->postCmdObjPtr, 
		TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(comboPtr->postCmdObjPtr);
	if (result != TCL_OK) {
	    return result;
	}
	/*
	 * The post commands could have deleted the menu, which means we are
	 * dead and should go away.
	 */
	if (comboPtr->tkwin == NULL) {
	    return TCL_OK;
	}
	if (comboPtr->flags & LAYOUT_PENDING) {
	    ComputeComboGeometry(comboPtr);
	}
    }

    /*
     * Adjust the position of the menu if necessary to keep it visible on the
     * screen.  There are two special tricks to make this work right:
     *
     * 1. If a virtual root window manager is being used then
     *    the coordinates are in the virtual root window of
     *    menuPtr's parent;  since the menu uses override-redirect
     *    mode it will be in the *real* root window for the screen,
     *    so we have to map the coordinates from the virtual root
     *    (if any) to the real root.  Can't get the virtual root
     *    from the menu itself (it will never be seen by the wm)
     *    so use its parent instead (it would be better to have an
     *    an option that names a window to use for this...).
     * 2. The menu may not have been mapped yet, so its current size
     *    might be the default 1x1.  To compute how much space it
     *    needs, use its requested size, not its actual size.
     *
     * Note that this code assumes square screen regions and all positive
     * coordinates. This does not work on a Mac with multiple monitors. But
     * then again, Tk has other problems with this.
     */
    {
	int vx, vy, vw, vh;
	int screenWidth, screenHeight;
	Tk_Window parent;

	parent = Tk_Parent(comboPtr->tkwin);
	Blt_SizeOfScreen(comboPtr->tkwin, &screenWidth, &screenHeight);
	Tk_GetVRootGeometry(parent, &vx, &vy, &vw, &vh);
	x += vx;
	y += vy;
	if (x < 0) {
	    x = 0;
	}
	if (y < 0) {
	    y = 0;
	}
	if ((x + comboPtr->width) > screenWidth) {
	    x = screenWidth - comboPtr->width;
	}
	if ((y + comboPtr->height) > screenHeight) {
	    x = screenHeight - comboPtr->height;
	}
	Tk_MoveToplevelWindow(comboPtr->tkwin, x, y);
	Tk_MapWindow(comboPtr->tkwin);
	Blt_MapToplevelWindow(comboPtr->tkwin);
	Blt_RaiseToplevelWindow(comboPtr->tkwin);
#ifdef notdef
	TkWmRestackToplevel(comboPtr->tkwin, Above, NULL);
#endif
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
ScanOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int x, y;
    char c;
    int length;
    int oper;
    const char *string;
    Tk_Window tkwin;

#define SCAN_MARK	1
#define SCAN_DRAGTO	2
    string = Tcl_GetStringFromObj(objv[2], &length);
    c = string[0];
    tkwin = comboPtr->tkwin;
    if ((c == 'm') && (strncmp(string, "mark", length) == 0)) {
	oper = SCAN_MARK;
    } else if ((c == 'd') && (strncmp(string, "dragto", length) == 0)) {
	oper = SCAN_DRAGTO;
    } else {
	Tcl_AppendResult(interp, "bad scan operation \"", string,
	    "\": should be either \"mark\" or \"dragto\"", (char *)NULL);
	return TCL_ERROR;
    }
    if ((Blt_GetPixelsFromObj(interp, tkwin, objv[3], PIXELS_ANY, &x) 
	 != TCL_OK) ||
	(Blt_GetPixelsFromObj(interp, tkwin, objv[4], PIXELS_ANY, &y) 
	 != TCL_OK)) {
	return TCL_ERROR;
    }
    if (oper == SCAN_MARK) {
	comboPtr->scanAnchorX = x;
	comboPtr->scanAnchorY = y;
	comboPtr->scanX = comboPtr->xOffset;
	comboPtr->scanY = comboPtr->yOffset;
    } else {
	int worldX, worldY;
	int viewWidth, viewHeight;
	int dx, dy;

	dx = comboPtr->scanAnchorX - x;
	dy = comboPtr->scanAnchorY - y;
	worldX = comboPtr->scanX + (10 * dx);
	worldY = comboPtr->scanY + (10 * dy);

	viewWidth = VPORTWIDTH(comboPtr);
	if (worldX > (comboPtr->worldWidth - viewWidth)) {
	    worldX = comboPtr->worldWidth - viewWidth;
	}
	if (worldX < 0) {
	    worldX = 0;
	}
	viewHeight = VPORTHEIGHT(comboPtr);
	if (worldY > (comboPtr->worldHeight - viewHeight)) {
	    worldY = comboPtr->worldHeight - viewHeight;
	}
	if (worldY < 0) {
	    worldY = 0;
	}
	comboPtr->xOffset = worldX;
	comboPtr->yOffset = worldY;
	comboPtr->flags |= SCROLL_PENDING;
	EventuallyRedraw(comboPtr);
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
SeeOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Entry *entryPtr;
    int width, height;
    int x, y;
    Tk_Anchor anchor;
    int left, right, top, bottom;
    const char *string;

    string = Tcl_GetString(objv[2]);
    anchor = TK_ANCHOR_W;	/* Default anchor is West */
    if ((string[0] == '-') && (strcmp(string, "-anchor") == 0)) {
	if (objc == 3) {
	    Tcl_AppendResult(interp, "missing \"-anchor\" argument",
		(char *)NULL);
	    return TCL_ERROR;
	}
	if (Tk_GetAnchorFromObj(interp, objv[3], &anchor) != TCL_OK) {
	    return TCL_ERROR;
	}
	objc -= 2, objv += 2;
    }
    if (objc == 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", objv[0],
	    "see ?-anchor anchor? entry\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (GetEntryFromObj(interp, comboPtr, objv[2], &entryPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (entryPtr == NULL) {
	return TCL_OK;
    }
    if (entryPtr->flags & ENTRY_HIDE) {
	MapAncestors(comboPtr, entryPtr);
	comboPtr->flags |= SCROLL_PENDING;
	/*
	 * If the entry wasn't previously exposed, its world coordinates
	 * aren't likely to be valid.  So re-compute the layout before we try
	 * to see the viewport to the entry's location.
	 */
	ComputeComboGeometry(comboPtr);
    }
    width = VPORTWIDTH(comboPtr);
    height = VPORTHEIGHT(comboPtr);

    /*
     * XVIEW:	If the entry is left or right of the current view, adjust
     *		the offset.  If the entry is nearby, adjust the view just
     *		a bit.  Otherwise, center the entry.
     */
    left = comboPtr->xOffset;
    right = comboPtr->xOffset + width;

    switch (anchor) {
    case TK_ANCHOR_W:
    case TK_ANCHOR_NW:
    case TK_ANCHOR_SW:
	x = 0;
	break;
    case TK_ANCHOR_E:
    case TK_ANCHOR_NE:
    case TK_ANCHOR_SE:
	x = entryPtr->worldX + entryPtr->width + 
	    ICONWIDTH(Blt_Tree_NodeDepth(entryPtr->node)) - width;
	break;
    default:
	if (entryPtr->worldX < left) {
	    x = entryPtr->worldX;
	} else if ((entryPtr->worldX + entryPtr->width) > right) {
	    x = entryPtr->worldX + entryPtr->width - width;
	} else {
	    x = comboPtr->xOffset;
	}
	break;
    }
    /*
     * YVIEW:	If the entry is above or below the current view, adjust
     *		the offset.  If the entry is nearby, adjust the view just
     *		a bit.  Otherwise, center the entry.
     */
    top = comboPtr->yOffset;
    bottom = comboPtr->yOffset + height;

    switch (anchor) {
    case TK_ANCHOR_N:
	y = comboPtr->yOffset;
	break;
    case TK_ANCHOR_NE:
    case TK_ANCHOR_NW:
	y = entryPtr->worldY - (height / 2);
	break;
    case TK_ANCHOR_S:
    case TK_ANCHOR_SE:
    case TK_ANCHOR_SW:
	y = entryPtr->worldY + entryPtr->height - height;
	break;
    default:
	if (entryPtr->worldY < top) {
	    y = entryPtr->worldY;
	} else if ((entryPtr->worldY + entryPtr->height) > bottom) {
	    y = entryPtr->worldY + entryPtr->height - height;
	} else {
	    y = comboPtr->yOffset;
	}
	break;
    }
    if ((y != comboPtr->yOffset) || (x != comboPtr->xOffset)) {
	/* comboPtr->xOffset = x; */
	comboPtr->yOffset = y;
	comboPtr->flags |= SCROLL_PENDING;
    }
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ShowOp --
 *
 *	Mark one or more nodes to be exposed.  Nodes can be specified by their
 *	inode, or by matching a name or data value pattern.  By default, the
 *	patterns are matched exactly.  They can also be matched using
 *	glob-style and regular expression rules.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
ShowOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    if (SearchAndApplyToTree(comboPtr, interp, objc, objv, ShowEntryApplyProc,
	    (int *)NULL) != TCL_OK) {
	return TCL_ERROR;
    }
    comboPtr->flags |= (LAYOUT_PENDING | SCROLL_PENDING);
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/* .m style create name option value option value */
    
static int
StyleCreateOp(
    ComboTree *comboPtr, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    Style *stylePtr;
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&comboPtr->styleTable, Tcl_GetString(objv[3]),
		&isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "combomenu style \"", Tcl_GetString(objv[3]),
		"\" already exists.", (char *)NULL);
	return TCL_ERROR;
    }
    stylePtr = Blt_AssertCalloc(1, sizeof(Style));
    stylePtr->name = Blt_GetHashKey(&comboPtr->styleTable, hPtr);
    stylePtr->hPtr = hPtr;
    stylePtr->comboPtr = comboPtr;
    stylePtr->activeRelief = TK_RELIEF_RAISED;
    Blt_SetHashValue(hPtr, stylePtr);
    iconsOption.clientData = comboPtr;
    if (ConfigureStyle(interp, stylePtr, objc - 4, objv + 4, 0) != TCL_OK) {
	DestroyStyle(stylePtr);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
StyleCgetOp(
    ComboTree *comboPtr, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    Style *stylePtr;

    if (GetStyleFromObj(interp, comboPtr, objv[3], &stylePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    iconsOption.clientData = comboPtr;
    return Blt_ConfigureValueFromObj(interp, comboPtr->tkwin, styleSpecs,
	(char *)stylePtr, objv[4], 0);
}

static int
StyleConfigureOp(
    ComboTree *comboPtr, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    int result, flags;
    Style *stylePtr;

    if (GetStyleFromObj(interp, comboPtr, objv[3], &stylePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    iconsOption.clientData = comboPtr;
    flags = BLT_CONFIG_OBJV_ONLY;
    if (objc == 1) {
	return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, styleSpecs, 
		(char *)stylePtr, (Tcl_Obj *)NULL, flags);
    } else if (objc == 2) {
	return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, styleSpecs, 
		(char *)stylePtr, objv[2], flags);
    }
    Tcl_Preserve(stylePtr);
    result = ConfigureStyle(interp, stylePtr, objc - 4, objv + 4, flags);
    Tcl_Release(stylePtr);
    if (result == TCL_ERROR) {
	return TCL_ERROR;
    }
    comboPtr->flags |= (LAYOUT_PENDING | SCROLL_PENDING);
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

static int
StyleDeleteOp(
    ComboTree *comboPtr, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    Style *stylePtr;

    if (GetStyleFromObj(interp, comboPtr, objv[3], &stylePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (stylePtr->refCount > 0) {
	Tcl_AppendResult(interp, "can't destroy combotree style \"", 
			 stylePtr->name, "\": style in use.", (char *)NULL);
	return TCL_ERROR;
    }
    DestroyStyle(stylePtr);
    return TCL_OK;
}

static int
StyleNamesOp(
    ComboTree *comboPtr, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hPtr = Blt_FirstHashEntry(&comboPtr->styleTable, &cursor); 
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Style *stylePtr;
	int found;
	int i;

	found = TRUE;
	stylePtr = Blt_GetHashValue(hPtr);
	for (i = 3; i < objc; i++) {
	    const char *pattern;

	    pattern = Tcl_GetString(objv[i]);
	    found = Tcl_StringMatch(stylePtr->name, pattern);
	    if (found) {
		break;
	    }
	}
	if (found) {
	    Tcl_ListObjAppendElement(interp, listObjPtr,
		Tcl_NewStringObj(stylePtr->name, -1));
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static Blt_OpSpec styleOps[] =
{
    {"cget",        2, StyleCgetOp,        5, 5, "name option",},
    {"configure",   2, StyleConfigureOp,   4, 0, "name ?option value?...",},
    {"create",      2, StyleCreateOp,      4, 0, "name ?option value?...",},
    {"delete",      1, StyleDeleteOp,      3, 0, "?name...?",},
    {"names",       1, StyleNamesOp,       3, 0, "?pattern...?",},
};

static int numStyleOps = sizeof(styleOps) / sizeof(Blt_OpSpec);

static int
StyleOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ComboTreeCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numStyleOps, styleOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (comboPtr, interp, objc, objv);
    return result;
}

/*ARGSUSED*/
static int
ToggleOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    Entry *entryPtr;
    EntryIterator iter;

    if (GetEntryIterator(interp, comboPtr, objv[2], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
	 entryPtr = NextTaggedEntry(&iter)) {
	if (entryPtr->flags & ENTRY_CLOSED) {
	    OpenEntry(comboPtr, entryPtr);
	} else {
	    CloseEntry(comboPtr, entryPtr);
	}
    }
    comboPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * UnpostOp --
 *
 *	Unposts this menu.
 *
 *  .cm unpost 
 *
 *---------------------------------------------------------------------------
 */
static int
UnpostOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    if (!Tk_IsMapped(comboPtr->tkwin)) {
	return TCL_OK;			/* This menu is already unposted. */
    }
    if (Tk_IsMapped(comboPtr->tkwin)) {
	Tk_UnmapWindow(comboPtr->tkwin);
    }
    return TCL_OK;
}

static int
XViewOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int w, worldWidth;

    w = VPORTWIDTH(comboPtr);
    worldWidth = comboPtr->worldWidth;
    if (objc == 2) {
	double fract;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	fract = (double)comboPtr->xOffset / worldWidth;
	fract = FCLAMP(fract);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
	fract = (double)(comboPtr->xOffset + w) / worldWidth;
	fract = FCLAMP(fract);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &comboPtr->xOffset,
	    worldWidth, w, comboPtr->xScrollUnits, BLT_SCROLL_MODE_HIERBOX) 
	    != TCL_OK) {
	return TCL_ERROR;
    }
    comboPtr->flags |= SCROLLX;
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

static int
YViewOp(ComboTree *comboPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int h, worldHeight;

    h = VPORTHEIGHT(comboPtr);
    worldHeight = comboPtr->worldHeight;
    if (objc == 2) {
	double fract;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	/* Report first and last fractions */
	fract = (double)comboPtr->yOffset / worldHeight;
	fract = FCLAMP(fract);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
	fract = (double)(comboPtr->yOffset + h) / worldHeight;
	fract = FCLAMP(fract);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &comboPtr->yOffset,
	    worldHeight, h, comboPtr->yScrollUnits, BLT_SCROLL_MODE_HIERBOX)
	!= TCL_OK) {
	return TCL_ERROR;
    }
    comboPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboTreeInstCmdProc --
 *
 * 	This procedure is invoked to process commands on behalf of the
 * 	treeview widget.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec comboOps[] =
{
    {"activate",  1, ActivateOp,  3, 3, "entry",},
    {"bind",      2, BindOp,      3, 5, "tagName ?sequence command?",}, 
    {"button",    2, ButtonOp,    2, 0, "args",},
    {"cget",      2, CgetOp,      3, 3, "option",}, 
    {"close",     2, CloseOp,     2, 4, "?-recurse? entry",}, 
    {"configure", 2, ConfigureOp, 2, 0, "?option value?...",},
    {"entry",     1, EntryOp,     2, 0, "oper args",},
    {"get",       1, GetOp,       2, 0, "?-full? entry ?entry...?",},
    {"hide",      1, HideOp,      2, 0, "?-exact|-glob|-regexp? ?-nonmatching? ?-name string? ?-full string? ?-data string? ?--? ?entry...?",},
    {"index",     3, IndexOp,     3, 3, "entry",},
    {"invoke",    3, InvokeOp,    3, 3, "entry",},
    {"nearest",   1, NearestOp,   4, 5, "x y ?varName?",}, 
    {"open",      1, OpenOp,      2, 4, "?-recurse? entry",}, 
    {"post",      1, PostOp,      5, 7, "align x y ?x2 y2?",},
    {"scan",      2, ScanOp,      5, 5, "dragto|mark x y",},
    {"see",       2, SeeOp,       3, 0, "?-anchor anchor? entry",},
    {"show",      2, ShowOp,      2, 0, "?-exact? ?-glob? ?-regexp? ?-nonmatching? ?-name string? ?-full string? ?-data string? ?--? ?entry...?",},
    {"style",     2, StyleOp,	  2, 0, "args",},
    {"toggle",    1, ToggleOp,    3, 3, "entry",},
    {"unpost",    1, UnpostOp,    2, 2, "",},
    {"xview",     1, XViewOp,     2, 5, "?moveto fract? ?scroll number what?",},
    {"yview",     1, YViewOp,     2, 5, "?moveto fract? ?scroll number what?",},
};

static int numComboOps = sizeof(comboOps) / sizeof(Blt_OpSpec);

static int
ComboTreeInstCmdProc(
    ClientData clientData,	/* Information about the widget. */
    Tcl_Interp *interp,		/* Interpreter to report errors back to. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Vector of argument strings. */
{
    ComboTreeCmdProc *proc;
    ComboTree *comboPtr = clientData;
    int result;

    proc = Blt_GetOpFromObj(interp, numComboOps, comboOps, BLT_OP_ARG1, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    Tcl_Preserve(comboPtr);
    result = (*proc) (comboPtr, interp, objc, objv);
    Tcl_Release(comboPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayComboTree --
 *
 * 	This procedure is invoked to display the widget.
 *
 *      Recompute the layout of the text if necessary. This is
 *	necessary if the world coordinate system has changed.
 *	Specifically, the following may have occurred:
 *
 *	  1.  a text attribute has changed (font, linespacing, etc.).
 *	  2.  an entry's option changed, possibly resizing the entry.
 *
 *      This is deferred to the display routine since potentially
 *      many of these may occur.
 *
 *	Set the vertical and horizontal scrollbars.  This is done
 *	here since the window width and height are needed for the
 *	scrollbar calculations.
 *
 * Results:
 *	None.
 *
 * Side effects:
 * 	The widget is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayComboTree(ClientData clientData)	/* Information about widget. */
{
    ComboTree *comboPtr = clientData;
    Pixmap drawable; 

    comboPtr->flags &= ~REDRAW_PENDING;
    if (comboPtr->tkwin == NULL) {
	return;			/* Window has been destroyed. */
    }
    if (comboPtr->rootPtr == NULL) {
	Blt_Warn("no root to tree \n");
	return;
    }
    if (comboPtr->flags & LAYOUT_PENDING) {
	/*
	 * Recompute the layout when entries are opened/closed,
	 * inserted/deleted, or when text attributes change (such as
	 * font, linespacing).
	 */
	ComputeComboGeometry(comboPtr);
    }
    if (comboPtr->flags & (SCROLL_PENDING | DIRTY)) {
	/* 
	 * Scrolling means that the view port has changed and that the
	 * visible entries need to be recomputed.
	 */
	ComputeVisibleEntries(comboPtr);
	if ((comboPtr->flags & SCROLLX) && (comboPtr->xScrollCmdObjPtr!=NULL)) {
	    int w;
		
	    w = VPORTWIDTH(comboPtr);
	    Blt_UpdateScrollbar(comboPtr->interp, comboPtr->xScrollCmdObjPtr,
		comboPtr->xOffset, comboPtr->xOffset + w, comboPtr->worldWidth);
	}
	if ((comboPtr->flags & SCROLLY) && (comboPtr->yScrollCmdObjPtr!=NULL)) {
	    int h;

	    h = VPORTHEIGHT(comboPtr);
	    Blt_UpdateScrollbar(comboPtr->interp, comboPtr->yScrollCmdObjPtr,
		comboPtr->yOffset, comboPtr->yOffset + h, comboPtr->worldHeight);
	}
	comboPtr->flags &= ~SCROLL_PENDING;
    }
#ifdef notdef
    if (comboPtr->reqWidth == 0) {
	/* 
	 * The first time through this routine, set the requested
	 * width to the computed width.  All we want is to
	 * automatically set the width of the widget, not dynamically
	 * grow/shrink it as attributes change.
	 */
	comboPtr->reqWidth = comboPtr->worldWidth + 2 * comboPtr->inset;
	Tk_GeometryRequest(comboPtr->tkwin, comboPtr->reqWidth, 
		comboPtr->reqHeight);
    }
#endif
    if (!Tk_IsMapped(comboPtr->tkwin)) {
	return;
    }
    drawable = Blt_GetPixmap(comboPtr->display, Tk_WindowId(comboPtr->tkwin), 
	Tk_Width(comboPtr->tkwin), Tk_Height(comboPtr->tkwin), 
	Tk_Depth(comboPtr->tkwin));

    comboPtr->flags |= VIEWPORT;
    /* Clear the column background. */
    DrawComboTree(comboPtr, drawable);
    DrawOuterBorders(comboPtr, drawable);

    /* Now copy the new view to the window. */
    XCopyArea(comboPtr->display, drawable, Tk_WindowId(comboPtr->tkwin), 
	comboPtr->copyGC, 0, 0, Tk_Width(comboPtr->tkwin), 
	Tk_Height(comboPtr->tkwin), 0, 0);
    Tk_FreePixmap(comboPtr->display, drawable);
    comboPtr->flags &= ~VIEWPORT;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboTreeObjCmdProc --
 *
 * 	This procedure is invoked to process the TCL command that
 * 	corresponds to a widget managed by this module. See the user
 * 	documentation for details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
ComboTreeObjCmdProc(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Argument strings. */
{
    ComboTree *comboPtr;
    XSetWindowAttributes attrs;
    unsigned int mask;
    int result;

    if (objc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " pathName ?option value?...\"", 
		(char *)NULL);
	return TCL_ERROR;
    }
    comboPtr = NewComboTree(interp, objv[1]);
    if (comboPtr == NULL) {
	goto error;
    }

    /*
     * Source in the initialization script for treeview entries from
     * "$blt_library/treeview.tcl".  We deferred sourcing the file until now
     * so that the variable $blt_library could be set within a script.
     */
    if (!Blt_CommandExists(interp, "::blt::ComboTree::Initialize")) {
	static char cmd[] = { 
	    "source [file join $blt_library combotree.tcl]" 
	};
	if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
	    char info[200];

	    Blt_FormatString(info, 200, "\n    (while loading bindings for %.50s)", 
		    Tcl_GetString(objv[0]));
	    Tcl_AddErrorInfo(interp, info);
	    goto error;
	}
    }
    /* 
     * Initialize the widget's configuration options here. The options need to
     * be set first, so that entry, column, and style components can use them
     * for their own GCs.
     */
    iconsOption.clientData = comboPtr;
    if (Blt_ConfigureComponentFromObj(interp, comboPtr->tkwin, "button", 
	"Button", buttonSpecs, 0, (Tcl_Obj **)NULL, (char *)comboPtr, 0) 
	!= TCL_OK) {
	goto error;
    }
    /* 
     * Rebuild the widget's GC and other resources that are predicated by the
     * widget's configuration options.  Do the same for the default column.
     */
    if (ConfigureComboTree(interp, comboPtr, objc - 2, objv + 2, 0) != TCL_OK) {
	goto error;
    }
    /*
     * Invoke a procedure to initialize various bindings on treeview entries.
     */
    {
	Tcl_Obj *cmd[2];

	cmd[0] = Tcl_NewStringObj("::blt::ComboTree::Initialize", -1);
	cmd[1] = objv[1];
	Tcl_IncrRefCount(cmd[0]);
	Tcl_IncrRefCount(cmd[1]);
	result = Tcl_EvalObjv(interp, 2, cmd, TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(cmd[1]);
	Tcl_DecrRefCount(cmd[0]);
	if (result != TCL_OK) {
	    goto error;
	}
    }

    attrs.override_redirect = True;
    attrs.backing_store = WhenMapped;
    attrs.save_under = True;
    mask = (CWOverrideRedirect | CWSaveUnder | CWBackingStore);
    Tk_ChangeWindowAttributes(comboPtr->tkwin, mask, &attrs);
    Tk_MakeWindowExist(comboPtr->tkwin);

    Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(comboPtr->tkwin),-1);
    return TCL_OK;
  error:
    if (comboPtr != NULL) {
	Tk_DestroyWindow(comboPtr->tkwin);
    }
    return TCL_ERROR;
}

int
Blt_ComboTreeInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "combotree", ComboTreeObjCmdProc, };

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

#endif /* NO_COMBOTREE */

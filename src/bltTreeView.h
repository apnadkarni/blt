/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTreeView.h --
 *
 *	Copyright 1998-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use, copy,
 *	modify, merge, publish, distribute, sublicense, and/or sell copies
 *	of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
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

#ifndef BLT_TREEVIEW_H
#define BLT_TREEVIEW_H

#include "bltImage.h"
#include "bltHash.h"
#include "bltFont.h"
#include "bltText.h"
#include "bltChain.h"
#include "bltTree.h"
#include "bltTile.h"
#include "bltBind.h"
#include "bltBg.h"

#define ITEM_ENTRY		(ClientData)0
#define ITEM_ENTRY_BUTTON	(ClientData)1
#define ITEM_COLUMN_TITLE	(ClientData)2
#define ITEM_COLUMN_RULE	(ClientData)3
#define ITEM_CELL       	(ClientData)4
#define ITEM_STYLE		(ClientData)0x10004

#define TITLE_PADX	2
#define TITLE_PADY	1

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

#define TV_ARROW_WIDTH 17
#define TV_ARROW_HEIGHT 17

typedef const char *UID;

/*
 * The macro below is used to modify a "char" value (e.g. by casting it to
 * an unsigned character) so that it can be used safely with macros such as
 * isspace.
 */
#define UCHAR(c)	((unsigned char) (c))

#define TOGGLE(x, mask) (((x) & (mask)) ? ((x) & ~(mask)) : ((x) | (mask)))


#define SCREENX(h, wx)	((wx) - (h)->xOffset + (h)->inset)
#define SCREENY(h, wy)	((wy) - (h)->yOffset + (h)->inset + (h)->titleHeight)

#define WORLDX(h, sx)	((sx) - (h)->inset + (h)->xOffset)
#define WORLDY(h, sy)	((sy) - ((h)->inset + (h)->titleHeight) + (h)->yOffset)

#define VPORTWIDTH(h)	(Tk_Width((h)->tkwin) - 2 * (h)->inset)
#define VPORTHEIGHT(h) \
	(Tk_Height((h)->tkwin) - (h)->titleHeight - 2 * (h)->inset)

#define ICONWIDTH(d)	(viewPtr->levelInfo[(d)].iconWidth)
#define LEVELX(d)	(viewPtr->levelInfo[(d)].x)

#define DEPTH(h, n)	(((h)->flatView) ? 0 : Blt_Tree_NodeDepth(n))

/*
 *  Internal treeview widget flags:
 */
#define LAYOUT_PENDING		(1<<0)	/* The layout of the hierarchy needs
					 * to be recomputed. */
#define REDRAW_PENDING		(1<<1)	/* A redraw request is pending for
					 * the widget. */
#define SELECT_PENDING		(1<<2)	/* A "selection" command idle task is
					 * pending.  */
#define SCROLLX			(1<<3)	/* X-scroll request is pending. */
#define SCROLLY			(1<<4)	/* Y-scroll request is pending. */
/* Both X-scroll and  Y-scroll requests are pending. */
#define SCROLL_PENDING	(SCROLLX | SCROLLY)
#define FOCUS			(1<<5)	/* The widget is receiving keyboard
					 * events.  Draw the focus
					 * highlight border around the
					 * widget. */
#define DIRTY			(1<<6)	/* The hierarchy has changed. It
					 * may invalidate the locations and
					 * pointers to entries.  The widget
					 * will need to recompute its
					 * layout. */
#define UPDATE			(1<<7)
#define RESORT			(1<<8)	/* The tree has changed such that
					 * the view needs to be resorted.
					 * This can happen when an entry is
					 * open or closed, it's label
					 * changes, a column value changes,
					 * etc. */
#define SORTED			(1<<9)	/* The view is currently sorted.
					 * This is used to simply reverse
					 * the view when the sort
					 * -decreasing flag is changed. */
#define SORT_PENDING		(1<<10)		
#define TV_SORT_AUTO		(1<<11)
#define REDRAW_BORDERS		(1<<12)	/* The borders of the widget
					 * (highlight ring and 3-D border)
					 * need to be redrawn. */
#define REPOPULATE		(1<<13)	/* The tree used to populated the
					 * widget has been changed, so
					 * generate the associated data
					 * structures. */
#define VIEWPORT		(1<<14)	/* Indicates that the viewport has
					 * changed in some way: the size of
					 * the viewport, the location of
					 * the viewport, or the contents of
					 * the viewport. */
#define ALLOW_DUPLICATES	(1<<15)	/* When inserting new entries,
					 * create duplicate entries. */
#define FILL_ANCESTORS		(1<<16)	/* Automatically create ancestor
					 * entries as needed when inserting
					 * a new entry. */
#define HIDE_ROOT		(1<<17)	/* Don't display the root entry. */
#define HIDE_LEAVES		(1<<18)	/* Don't display entries that are
					 * leaves. */

#define TV_NEW_TAGS		(1<<19)
#define TV_HIGHLIGHT_CELLS	(1<<20)
#define DONT_UPDATE		(1<<21)

#define COLUMN_READONLY		(1<<23)
#define RULE_ACTIVE_COLUMN	(1<<24)
#define COLUMN_RULE_NEEDED	(1<<25)
#define SHOW_COLUMN_TITLES	(1<<26)	/* Indicates whether to draw titles
					 * over each column. */

#define TV_ITEM_COLUMN	1
#define TV_ITEM_RULE	2


#define BUTTON_AUTO		(1<<8)
#define BUTTON_SHOW		(1<<9)
#define BUTTON_MASK		(BUTTON_AUTO | BUTTON_SHOW)


#define ENTRY_EDITABLE		(1<<10)
#define ENTRY_SELECTED		(1<<11)

#define DELETED			(1<<12)

#define COLUMN_RULE_PICKED	(1<<1)

#define STYLE_TEXTBOX		(0)
#define STYLE_COMBOBOX		(1)
#define STYLE_CHECKBOX		(2)
#define STYLE_TYPE		0x3

#define STYLE_LAYOUT		(1<<3)
#define STYLE_DIRTY		(1<<4)
#define STYLE_HIGHLIGHT		(1<<5)
#define STYLE_USER		(1<<6)

#define STYLE_EDITABLE		(1<<10)
#define STYLE_POSTED		(1<<11)

typedef struct _Column Column;
typedef struct _Combobox Combobox;
typedef struct _Entry Entry;
typedef struct _TreeView TreeView;
typedef struct _ValueStyleClass ValueStyleClass;
typedef struct _ValueStyle ValueStyle;

typedef int (CompareProc)(Tcl_Interp *interp, const char *name, 
	const char *pattern);

typedef Entry *(IterProc)(Entry *entryPtr, unsigned int mask);


typedef struct {
    TreeView *viewPtr;
    unsigned int flags;
} TreeViewObj;

typedef struct {
    int tagType;
    TreeView *viewPtr;
    Blt_HashSearch cursor;
    Entry *entryPtr;
} TagIterator;

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
 *	For the treeview widget, we never need more than a single instance
 *	of an image, regardless of how many times it's used.  Cache the
 *	image, maintaining a reference count for each image used in the
 *	widget.  It's likely that the treeview widget will use many
 *	instances of the same image (for example the open/close icons).
 */

typedef struct _Icon {
    TreeView *viewPtr;			/* Widget using this icon. */
    Tk_Image tkImage;			/* The Tk image being cached. */
    Blt_HashEntry *hashPtr;		/* Hash table pointer to the
                                         * image. */
    int refCount;			/* Reference count for this
                                         * image. */
    short int width, height;		/* Dimensions of the cached
                                         * image. */
} *Icon;

#define IconHeight(icon)	((icon)->height)
#define IconWidth(icon)         ((icon)->width)
#define IconBits(icon)		((icon)->tkImage)
#define IconName(icon)		(Blt_Image_Name((icon)->tkImage))

typedef enum SortTypes { 
    SORT_DICTIONARY, SORT_ASCII, SORT_INTEGER, 
    SORT_REAL, SORT_COMMAND
} SortType;

/*
 * Column --
 *
 *	A column describes how to display a field of data in the tree.  It
 *	may display a title that you can bind to. It may display a rule for
 *	resizing the column.  Columns may be hidden, and have attributes
 *	(foreground color, background color, font, etc) that override those
 *	designated globally for the treeview widget.
 */
struct _Column {
    TreeView *viewPtr;
    unsigned int flags;			/* Flags for this entry. For the
					 * definitions of the various bit
					 * fields see below. */
    Blt_HashEntry *hashPtr;
    Blt_TreeKey key;			/* Data cell identifier for current
					 * tree. */
    int index;				/* Position of column in list.
					 * Used to indicate the first and
					 * last columns. */
    UID tagsUid;			/* List of binding tags for this
					 * entry.  UID, not a string,
					 * because in the typical case most
					 * columns will have the same
					 * bindtags. */

    /* Title-related information */
    const char *text;			/* Text displayed in column heading
					 * as its title. By default, this
					 * is the same as the data cell
					 * name. */
    short int textWidth, textHeight;	/* Dimensions of title text. */
    Blt_Font titleFont;			/* Font to draw title in. */
    XColor *titleFgColor;		/* Foreground color of text
					 * displayed in the heading */
    Blt_Bg titleBg;			/* Background color of the
                                         * heading. */
    GC titleGC;
    XColor *activeTitleFgColor;		/* Foreground color of the heading
					 * when the column is activated.*/
    Blt_Bg activeTitleBg;	

    int titleBW;
    int titleRelief;
    Tk_Justify titleJustify;		/* Indicates how the text and/or
					 * icon is justified within the
					 * column title. */  
    GC activeTitleGC;
    short int titleWidth, titleHeight;

    Icon titleIcon;			/* Icon displayed in column heading */
    Tcl_Obj *cmdObjPtr;			/* TCL script to be executed by the
					 * column's "invoke" operation. */
    Tcl_Obj *sortCmdObjPtr;		/* TCL script used to compare two
					 * columns. This overrides the
					 * global command for this
					 * column. */
    short int arrowWidth, arrowHeight;	/* Dimension of the sort direction
					 * arrow. */
    Icon sortUp, sortDown;

    /* General information. */
    int state;				/* Indicates if column title can
					 * invoked. */
    int max;				/* Maximum space allowed for
					 * column. */
    int reqMin, reqMax;			/* Requested bounds on the width of
					 * column.  Does not include any
					 * padding or the borderwidth of
					 * column.  If non-zero, overrides
					 * the computed width of the
					 * column. */
    int reqWidth;			/* User-requested width of
					 * column. Does not include any
					 * padding or the borderwidth of
					 * column.  If non-zero, overrides
					 * the computed column width. */
    int maxWidth;			/* Width of the widest entry in the
					 * column. */
    int worldX;				/* Starting world x-coordinate of
					 * the column. */
    double weight;			/* Growth factor for column.  Zero
					 * indicates that the column can
					 * not be resized. */
    int width;				/* Computed width of column. */
    ValueStyle *stylePtr;		/* Default style for column. */
    int borderWidth;			/* Border width of the column. */
    int relief;				/* Relief of the column. */
    Blt_Pad pad;			/* Horizontal padding on either
					 * side of the column. */
    Tk_Justify justify;			/* Indicates how the text or icon
					 * is justified within the
					 * column. */
    Blt_ChainLink link;
    int ruleLineWidth;
    Blt_Dashes ruleDashes;
    GC ruleGC;
    Tcl_Obj *fmtCmdPtr;
    SortType sortType;
};

#define COLUMN_DIRTY		(1<<0)
#define COLUMN_HIDDEN		(1<<1)

struct _ValueStyle {
    int refCount;			/* Usage reference count.  A
					 * reference count of zero
					 * indicates that the style may be
					 * freed. */
    unsigned int flags;			/* Bit field containing both the
					 * style type and various flags. */
    const char *name;			/* Instance name. */
    ValueStyleClass *classPtr;		/* Contains class-specific
					 * information such as
					 * configuration specifications and
					 * configure, draw, layout
					 * etc. routines. */
    Blt_HashEntry *hashPtr;		/* If non-NULL, points to the hash
					 * table entry for the style.  A
					 * style that's been deleted, but
					 * still in use (non-zero reference
					 * count) will have no hash table
					 * entry. */
    TreeView *viewPtr;			
    Blt_ChainLink link;			/* If non-NULL, pointer of the
					 * style in a list of all newly
					 * created styles. */
    /* General style fields. */
    Tk_Cursor cursor;			/* X Cursor */
    Icon icon;				/* If non-NULL, is a Tk_Image to be
					 * drawn in the cell. */
    int gap;				/* # pixels gap between icon and
					 * text. */
    Blt_Font font;
    XColor *activeFg;                   /* Foreground color of cell when
					 * active. */
    XColor *disableFg;                  /* Foreground color of cell when
					 * disabled. */
    XColor *highlightFg;		/* Foreground color of cell when
					 * highlighted. */
    XColor *normalFg;			/* Normal foreground color of
                                         * cell. */
    XColor *selectFg;			/* Foreground color of a selected
					 * cell. If non-NULL, overrides
					 * default foreground color
					 * specification. */
    Blt_Bg altBg;
    Blt_Bg activeBg;			/* Background color of cell when
					 * active. */
    Blt_Bg disableBg;                   /* Background color of cell when
                                         * disabled. */
    Blt_Bg highlightBg;			/* Background color of cell when
					 * highlighted. */
    Blt_Bg normalBg;                    /* Normal background color of
                                         * cell. */
    Blt_Bg selectBg;			/* Background color of a selected
					 * cell.  If non-NULL, overrides
					 * the default background * color
					 * specification. */
    Tcl_Obj *validateCmdObjPtr;
    GC activeGC;
    GC disableGC;
    GC highlightGC;
    GC normalGC;
    Blt_TreeKey key;			/* Actual data resides in this tree
					   value. */
    Tcl_Obj *cmdObjPtr;

};

typedef struct _Value {
    Entry *entryPtr;                    /* Entry where the value is
                                         * located. */
    Column *columnPtr;			/* Column where the value is
					 * located. */
    unsigned int width, height;		/* Dimensions of value. */
    ValueStyle *stylePtr;		/* Style information for cell
					 * displaying value. */
    const char *fmtString;		/* Raw text string. */
    TextLayout *textPtr;		/* Processes string to be
					 * displayed .*/
    struct _Value *nextPtr;
} Value;

typedef void (ValueStyleConfigureProc)(ValueStyle *stylePtr);
typedef void (ValueStyleDrawProc)(Value *valuePtr, Drawable drawable, 
        ValueStyle *stylePtr, int x, int y);
typedef int (ValueStyleEditProc)(Value *valuePtr, ValueStyle *stylePtr);
typedef void (ValueStyleFreeProc)(ValueStyle *stylePtr);
typedef void (ValueStyleGeometryProc)(ValueStyle *stylePtr, Value *valuePtr);
typedef const char * (ValueStyleIdentifyProc)(Value *valuePtr, 
        ValueStyle *stylePtr, int x, int y);
typedef int (ValueStylePostProc)(Tcl_Interp *interp, Value *valuePtr,
        ValueStyle *stylePtr);
typedef int (ValueStyleUnpostProc)(Tcl_Interp *interp, Value *valuePtr,
        ValueStyle *stylePtr);

struct _ValueStyleClass {
    const char *type;			/* Name of style class type. */
    const char *className;		/* Class name of the style. This is
					 * used as the class name of the
					 * treeview component for event
					 * bindings. */
    Blt_ConfigSpec *specsPtr;		/* Style configuration
					 * specifications */
    ValueStyleConfigureProc *configProc; /* Sets the GCs for style. */
    ValueStyleGeometryProc *geomProc;	/* Measures the area needed for the
					 * value with this style. */
    ValueStyleDrawProc *drawProc;	/* Draw the value in it's style. */
    ValueStyleIdentifyProc *identProc;	/* Routine to pick the style's button.
					 * Indicates if the mouse pointer is
					 * over the * style's button (if it
					 * has one). */
    ValueStyleEditProc *editProc;	/* Routine to edit the style's
					 * value. */
    ValueStyleFreeProc *freeProc;	/* Routine to free the style's
					 * resources. */
    ValueStylePostProc *postProc;	/* Routine to pick the style's button.
					 * Indicates if the mouse pointer is
					 * over the * style's button (if it
					 * has one). */
    ValueStyleUnpostProc *unpostProc;	/* Routine to pick the style's button.
					 * Indicates if the mouse pointer is
					 * over the * style's button (if it
					 * has one). */
};

BLT_EXTERN ValueStyle *Blt_TreeView_CreateTextBoxStyle(TreeView *viewPtr, 
	Blt_HashEntry *hPtr);
BLT_EXTERN ValueStyle *Blt_TreeView_CreateCheckBoxStyle(TreeView *viewPtr, 
	Blt_HashEntry *hPtr);
BLT_EXTERN ValueStyle *Blt_TreeView_CreateComboBoxStyle(TreeView *viewPtr, 
	Blt_HashEntry *hPtr);

/*
 * Entry --
 *
 *	Contains data-specific information how to represent the data
 *	of a node of the hierarchy.
 *
 */
struct _Entry {
    TreeView *viewPtr;
    unsigned int flags;			/* Flags for this entry. For the
					 * definitions of the various bit
					 * fields see below. */
    Blt_HashEntry *hashPtr;
    Blt_TreeNode node;			/* Node containing entry */
    int worldX, worldY;			/* X-Y position in world coordinates
					 * where the entry is positioned. */
    size_t width, height;		/* Dimensions of the entry. This
					 * includes the size of its columns. */
    int reqHeight;			/* Requested height of the entry.
					 * Overrides computed height. */
    int vertLineLength;			/* Length of the vertical line
					 * segment. */
    short int lineHeight;		/* Height of first line of text. */
    UID tagsUid;			/* List of binding tags for this
					 * entry. UID, not a string, because
					 * in the typical case most entries
					 * will have the same bindtags. */
    Tcl_Obj *openCmdObjPtr;
    Tcl_Obj *closeCmdObjPtr;		/* TCL commands to invoke when entries
					 * are opened or closed. They override
					 * those specified globally. */
    Tcl_Obj *cmdObjPtr;
    /*
     * Button information:
     */
    short int buttonX, buttonY;		/* X-Y coordinate offsets from to
					 * upper left corner of the entry to
					 * the upper-left corner of the
					 * button.  Used to pick the button
					 * quickly */
    short int iconWidth, iconHeight; 	/* Maximum dimensions for icons and
					 * buttons for this entry. This is
					 * used to align the button, icon, and
					 * text. */
    Icon *icons;			/* Tk images displayed for the entry.
					 * The first image is the icon
					 * displayed to the left of the
					 * entry's label. The second is icon
					 * displayed when entry is "open". */
    Icon *activeIcons;			/* Tk images displayed for the entry.
					 * The first image is the icon
					 * displayed to the left of the
					 * entry's label. The second * is icon
					 * displayed when entry is "open". */
    /*
     * Label information:
     */
    TextLayout *textPtr;
    short int labelWidth;
    short int labelHeight;
    UID labelUid;			/* Text displayed right of the
					 * icon. */
    Blt_Font font;			/* Font of label. Overrides global
					 * font specification. */
    const char *fullName;
    int flatIndex;			/* Used to navigate to next/last entry
					 * when the view is flat. */
    Tcl_Obj *dataObjPtr;		/* pre-fetched data for sorting */
    XColor *color;			/* Color of label. If non-NULL,
					 * overrides default text color
					 * specification. */
    GC gc;
    Value *values;			/* List of column-related information
					 * for each data value in the node.
					 * Non-NULL only if there are value
					 * entries. */
};

/*
 *---------------------------------------------------------------------------
 *
 *  Internal entry flags:
 *
 *	ENTRY_HAS_BUTTON	Indicates that a button needs to be
 *				drawn for this entry.
 *
 *	ENTRY_CLOSED		Indicates that the entry is closed and
 *				its subentries are not displayed.
 *
 *	ENTRY_HIDE		Indicates that the entry is hidden (i.e.
 *				can not be viewed by opening or scrolling).
 *
 *	BUTTON_AUTO
 *	BUTTON_SHOW
 *	BUTTON_MASK
 *
 *---------------------------------------------------------------------------
 */
#define ENTRY_CLOSED		(1<<0)
#define ENTRY_HIDE		(1<<1)
#define ENTRY_NOT_LEAF		(1<<2)
#define ENTRY_MASK		(ENTRY_CLOSED | ENTRY_HIDE)

#define ENTRY_HAS_BUTTON	(1<<3)
#define ENTRY_ICON		(1<<4)
#define ENTRY_REDRAW		(1<<5)
#define ENTRY_LAYOUT_PENDING	(1<<6)
#define ENTRY_DATA_CHANGED	(1<<7)
#define ENTRY_DIRTY		(ENTRY_DATA_CHANGED | ENTRY_LAYOUT_PENDING)

/*
 * Button --
 *
 *	A button is the open/close indicator at the far left of the entry.  It
 *	is displayed as a plus or minus in a solid colored box with optionally
 *	an border. It has both "active" and "normal" colors.
 */
typedef struct {
    XColor *normalFg;			/* Foreground color. */
    Blt_Bg normalBg;				/* Background color. */
    XColor *activeFgColor;		/* Active foreground color. */
    Blt_Bg activeBg;			/* Active background color. */
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
 * Selection Information:
 *
 * The selection is the rectangle that contains a selected entry.  There
 * may be many selected entries.  It is displayed as a solid colored box
 * with optionally a 3D border.
 */
typedef struct {
    unsigned int flags;
    int relief;				/* Relief of selected items. Currently
					 * is always raised. */
    int borderWidth;			/* Border width of a selected entry.*/
    int	mode;				/* Selection style: "single" or
					 * "multiple".  */
    XColor *fg;                         /* Text color of a selected
                                         * entry. */
    Blt_Bg bg;
    Entry *anchorPtr;			/* Fixed end of selection (i.e. entry
					 * at which selection was started.) */
    Entry *markPtr;
    GC gc;
    Tcl_Obj *cmdObjPtr;			/* TCL script that's invoked whenever
					 * the selection changes. */
    Blt_HashTable table;		/* Hash table of currently selected
					 * entries. */
    Blt_Chain list;			/* Chain of currently selected
					 * entries.  Contains the same
					 * information as the above hash
					 * table, but maintains the order
					 * in which entries are
					 * selected. */
} Selection;

#define SELECT_MODE_SINGLE	(1<<0)
#define SELECT_MODE_MULTIPLE	(1<<1)

#define SELECT_CLEAR		(1<<0)	/* Clear selection flag of entry. */
#define SELECT_SET		(1<<1)	/* Set selection flag of entry. */
/* Toggle selection flag * of entry. */
#define SELECT_TOGGLE (SELECT_SET | SELECT_CLEAR) 
/* Mask of selection set/clear/toggle flags.*/
#define SELECT_MASK   (SELECT_SET | SELECT_CLEAR) 

#define SELECT_EXPORT		(1<<2)	/* Export the selection to X11. */
#define SELECT_SORTED		(1<<4)	/* Indicates if the entries in the
					 * selection should be sorted or
					 * displayed in the order they were
					 * selected. */

typedef struct {
    Tcl_Obj *cmdObjPtr;			/* Sort command. */
    int decreasing;			/* Indicates entries should be sorted
					 * in decreasing order. */
    int viewIsDecreasing;		/* Current sorting direction */
    Column *markPtr;			/* Column to mark as sorted. */
    Blt_Chain order;			/* Order of columns in sorting. */
} SortInfo;

/*
 * TreeView --
 *
 *	A TreeView is a widget that displays an hierarchical table of one
 *	or more entries.
 *
 *	Entries are positioned in "world" coordinates, referring to the
 *	virtual treeview.  Coordinate 0,0 is the upper-left corner of the root
 *	entry and the bottom is the end of the last entry.  The widget's Tk
 *	window acts as view port into this virtual space. The treeview's
 *	xOffset and yOffset fields specify the location of the view port in
 *	the virtual world.  Scrolling the viewport is therefore simply
 *	changing the xOffset and/or yOffset fields and redrawing.
 *
 *	Note that world coordinates are integers, not signed short integers
 *	like X11 screen coordinates.  It's very easy to create a hierarchy
 *	taller than 0x7FFF pixels.
 */
struct _TreeView {
    Tcl_Interp *interp;

    Tcl_Command cmdToken;		/* Token for widget's TCL command. */

    Blt_Tree tree;			/* Token holding internal tree. */
    const char *treeName;		/* In non-NULL, is the name of the
					 * tree we are attached to */
    Blt_HashEntry *hashPtr;

    /* TreeView specific fields. */ 

    Tk_Window tkwin;			/* Window that embodies the widget.
					 * NULL means that the window has been
					 * destroyed but the data structures
					 * haven't yet been cleaned up.*/

    Display *display;			/* Display containing widget; needed,
					 * among other things, to release
					 * resources * after tkwin has already
					 * gone away. */

    Blt_HashTable entryTable;		/* Table of entry information, keyed
					 * by the node pointer. */

    Blt_HashTable columnTable;		/* Table of column information. */
    Blt_Chain columns;			/* Chain of columns. Same as the hash
					 * table above but maintains the order
					 * in which columns are displayed. */

    unsigned int flags;			/* For bitfield definitions, see
					 * below */
    int inset;				/* Total width of all borders,
					 * including traversal highlight and
					 * 3-D border.  Indicates how much
					 * interior stuff must be offset
					 * from outside edges to leave room
					 * for borders. */
    Blt_Font font;
    XColor *normalFg;
    Blt_Bg normalBg;                    /* 3D border surrounding the window
					 * (viewport). */
    Blt_Bg altBg;
    int borderWidth;			/* Width of 3D border. */
    int relief;				/* 3D border relief. */
    int highlightWidth;			/* Width in pixels of highlight to
					 * draw around widget when it has the
					 * focus.  <= 0 means don't draw a
					 * highlight. */
    XColor *highlightBgColor;		/* Color for drawing traversal
					 * highlight area when highlight is
					 * off. */
    XColor *highlightColor;		/* Color for drawing traversal
					 * highlight. */
    const char *pathSep;		/* Pathname separators */
    const char *trimLeft;		/* Leading characters to trim from
					 * pathnames */
    /*
     * Entries are connected by horizontal and vertical lines. They may be
     * drawn dashed or solid.
     */
    int lineWidth;			/* Width of lines connecting
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

    /*
     * Selection Information:
     *
     * The selection is the rectangle that contains a selected entry.  There
     * may be many selected entries.  It is displayed as a solid colored box
     * with optionally a 3D border.
     */
    Selection selection;

    int leader;				/* Number of pixels padding between
					 * entries. */

    Tk_Cursor cursor;			/* X Cursor */

    Tk_Cursor resizeCursor;		/* Resize Cursor */

    int reqWidth, reqHeight;	       /* Requested dimensions of the treeview
					* widget's window. */

    GC lineGC;				/* GC for drawing dotted line between
					 * entries. */

    XColor *focusColor;

    Blt_Dashes focusDashes;		/* Dash on-off value. */

    GC focusGC;				/* Graphics context for the active
					 * label. */

    Tk_Window comboWin;		

    Entry *activePtr;			/* Last active entry. */ 

    Entry *focusPtr;			/* Entry that currently has focus. */

    Entry *activeBtnPtr;		/* Pointer to last active button */

    Entry *fromPtr;

    Value *activeValuePtr;		/* Last active value. */ 

    Value *postPtr;                     /* Points to posted value. */

    int xScrollUnits, yScrollUnits;	/* # of pixels per scroll unit. */

    /* Command strings to control horizontal and vertical scrollbars. */
    Tcl_Obj *xScrollCmdObjPtr, *yScrollCmdObjPtr;

    int scrollMode;			/* Selects mode of scrolling: either
					 * BLT_SCROLL_MODE_HIERBOX, 
					 * BLT_SCROLL_MODE_LISTBOX, or 
					 * BLT_SCROLL_MODE_CANVAS. */
    /*
     * Total size of all "open" entries. This represents the range of world
     * coordinates.
     */
    int worldWidth, worldHeight;

    int xOffset, yOffset;		/* Translation between view port and
					 * world origin. */
    short int minHeight;		/* Minimum entry height. Used to to
					 * compute what the y-scroll unit
					 * should * be. */
    short int titleHeight;		/* Height of column titles. */

    LevelInfo *levelInfo;

    /* Scanning information: */
    int scanAnchorX, scanAnchorY;	/* Scan anchor in screen
					 * coordinates. */
    int scanX, scanY;			/* X-Y world coordinate where the scan
					 * started. */

    Blt_HashTable iconTable;		/* Table of Tk images */
    Blt_HashTable uidTable;		/* Table of strings. */
    Blt_HashTable styleTable;		/* Table of cell styles. */
    Blt_Chain userStyles;		/* List of user-created styles. */
    Entry *rootPtr;			/* Root entry of tree. */
    Entry **visibleArr;			/* Array of visible entries. */
    int numVisible;			/* # of entries in the visible array. */
    int numEntries;			/* # of entries in tree. */
    int treeWidth;			/* Computed width of the tree. */

    int buttonFlags;			/* Global button indicator for all
					 * entries.  This may be overridden by
					 * the entry's -button option. */
    Tcl_Obj *openCmdObjPtr;
    Tcl_Obj *closeCmdObjPtr;		/* TCL commands to invoke when entries
					 * are opened or closed. */
    Tcl_Obj *entryCmdObjPtr;		/* TCL script to be executed by the
					 * an entry "invoke" operation. */
    Icon *icons;			/* Tk images displayed for the entry.
					 * The first image is the icon
					 * displayed to the left of the
					 * entry's label. The second is icon
					 * displayed when entry is "open". */
    Icon *activeIcons;			/* Tk images displayed for the entry.
					 * The first image is the icon
					 * displayed to the left of the
					 * entry's label. The second is icon
					 * displayed when entry is "open". */
    const char *takeFocus;

    ClientData clientData;

    Blt_BindTable bindTable;		/* Binding information for entries. */

    Blt_HashTable entryTagTable;
    Blt_HashTable buttonTagTable;
    Blt_HashTable columnTagTable;
    Blt_HashTable styleTagTable;
    ValueStyle *stylePtr;		/* Default style for text cells */
    Column treeColumn;
    Column *colActivePtr; 
    Column *colActiveTitlePtr;		/* Column title currently active. */
    Column *colResizePtr;		/* Column that is being resized. */
    size_t depth;
    int flatView;			/* Indicates if the view of the tree
					 * has been flattened. */
    Entry **flatArr;			/* Flattened array of entries. */
    SortInfo sortInfo;			/* Information about sorting the tree.*/

    Tcl_Obj *iconVarObjPtr;		/* Name of TCL variable.  If non-NULL,
					 * this variable will be set to the
					 * name of the Tk image representing
					 * the icon of the selected item.  */
    Tcl_Obj *textVarObjPtr;		/* Name of TCL variable.  If non-NULL,
					 * this variable will be set to the
					 * text string of the label of the
					 * selected item. */
    Tcl_Obj *colCmdObjPtr;		/* TCL script to be executed when the
					 * column is invoked. */
#ifdef notdef
    Pixmap drawable;			/* Pixmap used to cache the entries
					 * displayed.  The pixmap is saved so
					 * that only selected elements can be
					 * drawn quicky. */
    short int drawWidth, drawHeight;
#endif
    short int ruleAnchor, ruleMark;

    Blt_Pool entryPool;
    Blt_Pool valuePool;
};

BLT_EXTERN void Blt_TreeView_FreeIcon(TreeView *viewPtr, Icon icon);
BLT_EXTERN Icon Blt_TreeView_GetIcon(TreeView *viewPtr, const char *iconName);
BLT_EXTERN Value *Blt_TreeView_FindValue(Entry *entryPtr, Column *colPtr);
BLT_EXTERN int Blt_TreeView_TextOp(TreeView *viewPtr, Tcl_Interp *interp, 
	int objc, Tcl_Obj *const *objv);
BLT_EXTERN int Blt_TreeView_CreateCombobox(TreeView *viewPtr, Entry *entryPtr, 
	Column *colPtr);

BLT_EXTERN void Blt_TreeView_DestroySort(TreeView *viewPtr);

BLT_EXTERN Icon Blt_TreeView_GetEntryIcon(TreeView *viewPtr, Entry *entryPtr);
BLT_EXTERN int Blt_TreeView_GetStyle(Tcl_Interp *interp, TreeView *viewPtr, 
	const char *styleName, ValueStyle **stylePtrPtr);
BLT_EXTERN void Blt_TreeView_FreeStyle(TreeView *viewPtr, 
	ValueStyle *stylePtr);
BLT_EXTERN ValueStyle *Blt_TreeView_CreateStyle(Tcl_Interp *interp, 
	TreeView *viewPtr, int type, const char *styleName, int objc, 
	Tcl_Obj *const *objv);
BLT_EXTERN int Blt_TreeView_CreateTextbox(TreeView *viewPtr, Entry *entryPtr, 
	Column *colPtr);
BLT_EXTERN int Blt_TreeView_SetEntryValue(Tcl_Interp *interp, TreeView *viewPtr,
	Entry *entryPtr, Column *columnPtr, const char *string);

#define CHOOSE(default, override)	\
	(((override) == NULL) ? (default) : (override))

#define GETLABEL(e)		\
	(((e)->labelUid != NULL)? (e)->labelUid : Blt_Tree_NodeLabel((e)->node))

#endif /* BLT_TREEVIEW_H */

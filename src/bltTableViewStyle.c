/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltTableViewStyle.c --
 *
 * This module implements styles for cells of the tableview widget.
 *
 *	Copyright 1998-2008 George A Howlett.
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

#ifndef NO_TABLEVIEW

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif	/* HAVE_CTYPE_H */

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltHash.h"
#include "bltChain.h"
#include "bltList.h"
#include "bltPool.h"
#include "bltConfig.h"
#include "bltSwitch.h"
#include "bltNsUtil.h"
#include "bltVar.h"
#include "bltBg.h"
#include "bltPicture.h"
#include "bltPainter.h"
#include "bltFont.h"
#include "bltDataTable.h"
#include "bltBind.h"
#include "bltTableView.h"
#include "bltInitCmd.h"
#include "bltImage.h"
#include "bltText.h"
#include "bltOp.h"

#define STYLE_GAP		3
#define ARROW_WIDTH		13
#define FOCUS_PAD		3	/* 1 pixel either side of a 1 pixel
					 * line */
#define CELL_PADX		2
#define CELL_PADY		1
#define SHOW_TEXT		(1<<0)
#define NORMAL			(1<<1)
#define ACTIVE			(1<<3)
#define DISABLED		(1<<4)
#define HIGHLIGHT		(1<<5)

#define STATE_MASK	(POSTED|NORMAL)

#define TEXT_VAR_TRACED	(1<<16)
#define ICON_VAR_TRACED	(1<<17)
#define TRACE_VAR_FLAGS		(TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|\
				 TCL_TRACE_UNSETS)



#ifdef WIN32
#define DEF_ACTIVE_BG			RGB_GREY85
#define DEF_CURSOR			"arrow"
#else
#define DEF_ACTIVE_BG			RGB_GREY95
#define DEF_CURSOR			"hand2"
#endif

#define DEF_ACTIVE_FG			STD_ACTIVE_FOREGROUND
#define DEF_ALT_BG			RGB_GREY97
#define DEF_CHECKBOX_ACTIVE_RELIEF	"raised"
#define DEF_CHECKBOX_BORDERWIDTH	"1"
#define DEF_CHECKBOX_BOX_COLOR		RGB_GREY50
#define DEF_CHECKBOX_CHECK_COLOR	STD_INDICATOR_COLOR
#define DEF_CHECKBOX_COMMAND		(char *)NULL
#define DEF_CHECKBOX_CURSOR		(char *)NULL
#define DEF_CHECKBOX_EDIT		"1"
#define DEF_CHECKBOX_FILL_COLOR		RGB_WHITE
#define DEF_CHECKBOX_FONT		STD_FONT_SMALL
#define DEF_CHECKBOX_GAP		"4"
#define DEF_CHECKBOX_LINEWIDTH		"2"
#define DEF_CHECKBOX_OFFVALUE		"0"
#define DEF_CHECKBOX_ONVALUE		"1"
#define DEF_CHECKBOX_RELIEF		"flat"
#define DEF_CHECKBOX_SHOWVALUE		"yes"
#define DEF_CHECKBOX_SIZE		"11"
#define DEF_COMBOBOX_ACTIVE_RELIEF	"raised"
#define DEF_COMBOBOX_ARROW_BORDERWIDTH	"2"
#define DEF_COMBOBOX_ARROW_RELIEF	"raised"
#define DEF_COMBOBOX_BORDERWIDTH	"1"
#define DEF_COMBOBOX_CURSOR		(char *)NULL
#define DEF_COMBOBOX_EDIT		"1"
#define DEF_COMBOBOX_FONT		STD_FONT_NORMAL
#define DEF_COMBOBOX_ICON_VARIABLE	(char *)NULL
#define DEF_COMBOBOX_MENU		(char *)NULL
#define DEF_COMBOBOX_POSTED_RELIEF	"sunken"
#define DEF_COMBOBOX_POST_CMD		(char *)NULL
#define DEF_COMBOBOX_RELIEF		"flat"
#define DEF_COMBOBOX_STATE		"normal"
#define DEF_COMBOBOX_TEXT		(char *)NULL
#define DEF_COMBOBOX_TEXT_VARIABLE	(char *)NULL
#define DEF_COMMAND			(char *)NULL
#define DEF_DISABLE_BG			RGB_GREY97
#define DEF_DISABLE_FG			RGB_GREY85
#define DEF_FOCUS_COLOR			"black"
#define DEF_FOCUS_DASHES		"dot"
#define DEF_GAP				"3"
#define DEF_ICON			(char *)NULL
#define DEF_IMAGEBOX_ACTIVE_RELIEF	"flat"
#define DEF_IMAGEBOX_BORDERWIDTH	"1"
#define DEF_IMAGEBOX_COMMAND		(char *)NULL
#define DEF_IMAGEBOX_CURSOR		(char *)NULL
#define DEF_IMAGEBOX_EDIT		"0"
#define DEF_IMAGEBOX_FONT		STD_FONT_SMALL
#define DEF_IMAGEBOX_RELIEF		"flat"
#define DEF_IMAGEBOX_SHOW_TEXT		"1"
#define DEF_IMAGEBOX_SIDE		"left"
#define DEF_JUSTIFY			"center"
#define DEF_NORMAL_BG			RGB_WHITE
#define DEF_NORMAL_FG			STD_NORMAL_FOREGROUND
#define DEF_HIGHLIGHT_BG		(char *)NULL
#define DEF_HIGHLIGHT_FG		RGB_RED3
#define DEF_SELECT_BG			STD_SELECT_BACKGROUND
#define DEF_SELECT_FG			STD_SELECT_FOREGROUND
#define DEF_TEXTBOX_ACTIVE_RELIEF	"flat"
#define DEF_TEXTBOX_BORDERWIDTH		"1"
#define DEF_TEXTBOX_COMMAND		(char *)NULL
#define DEF_TEXTBOX_CURSOR		(char *)NULL
#define DEF_TEXTBOX_EDIT		"0"
#define DEF_TEXTBOX_FONT		STD_FONT_NORMAL
#define DEF_TEXTBOX_RELIEF		"flat"
#define DEF_TEXTBOX_SIDE		"left"

static Blt_OptionParseProc ObjToIconProc;
static Blt_OptionPrintProc IconToObjProc;
static Blt_OptionFreeProc FreeIconProc;
static Blt_CustomOption iconOption =
{
    ObjToIconProc, IconToObjProc, FreeIconProc, 
    (ClientData)0,			/* Need to point this to the tableview
					 * widget before calling these
					 * procedures. */
};

static Blt_OptionFreeProc FreeIconVarProc;
static Blt_OptionParseProc ObjToIconVarProc;
static Blt_OptionPrintProc IconVarToObjProc;
static Blt_CustomOption iconVarOption = {
    ObjToIconVarProc, IconVarToObjProc, FreeIconVarProc, (ClientData)0
};
static Blt_OptionParseProc ObjToStateProc;
static Blt_OptionPrintProc StateToObjProc;
static Blt_CustomOption stateOption = {
    ObjToStateProc, StateToObjProc, NULL, (ClientData)0
};
static Blt_OptionFreeProc FreeTextVarProc;
static Blt_OptionParseProc ObjToTextVarProc;
static Blt_OptionPrintProc TextVarToObjProc;
static Blt_CustomOption textVarOption = {
    ObjToTextVarProc, TextVarToObjProc, FreeTextVarProc, (ClientData)0
};
static Blt_OptionFreeProc FreeTextProc;
static Blt_OptionParseProc ObjToTextProc;
static Blt_OptionPrintProc TextToObjProc;
static Blt_CustomOption textOption = {
    ObjToTextProc, TextToObjProc, FreeTextProc, (ClientData)0
};


/* 
 * Styles are the settings and attributes for cells.  They are separate 
 * from cells themselves for efficency sake.  Typically there will be 
 * many more cells than different ways to display them.  
 */
 
/* 
 * TextBoxStyle --
 *
 *	Treats the cell as a plain text box that can be edited (via a popup
 *	text editor widget).  The text box consists of an option icon, and a
 *	text string.  The icon may be to the left or right of the text.
 */
typedef struct {
    int refCount;			/* Usage reference count.  A reference
					 * count of zero indicates that the
					 * style is no longer used and may be
					 * freed. */
    unsigned int flags;			/* Bit fields containing various
					 * flags. */
    const char *name;			/* Instance name. */
    CellStyleClass *classPtr;		/* Contains class-specific information
					 * such as configuration
					 * specifications and routines how to
					 * configure, draw, layout, etc the
					 * cell according to the style. */
    Blt_HashEntry *hashPtr;		/* If non-NULL, points to the hash
					 * table entry for the style.  A style
					 * that's been deleted, but still in
					 * use (non-zero reference count) will
					 * have no hash table entry. */
    Blt_HashTable table;		/* Table of cells that have this
					 * style. We use this to mark the
					 * cells dirty when the style
					 * changes. */
    TableView *viewPtr;			/* Widget using this style. */
    /* General style fields. */
    Tk_Cursor cursor;			/* X Cursor */
    Icon icon;				/* If non-NULL, is a Tk_Image to be
					 * drawn in the cell. */
    int gap;				/* # pixels gap between icon and
					 * text. */
    Blt_Font font;
    XColor *normalFg;			/* Normal color of the text. */
    XColor *activeFg;			/* Color of the text when the cell is
					 * active. */
    XColor *disableFg;			/* Color of the text when the cell is
					 * disabled. */
    XColor *highlightFg;		/* Color of the text when the cell is
					 * highlighted. */
    XColor *selectFg;			/* Color of the text when the cell is
					 * selected. */
    Blt_Bg normalBg;			/* Normal background color of cell. */
    Blt_Bg activeBg;			/* Background color when the cell is
					 * active. Textboxes are usually never
					 * active. */
    Blt_Bg altBg;			/* Alternative normal background. */
    Blt_Bg disableBg;			/* Background color when the cell is
					 * disabled. */
    Blt_Bg highlightBg;			/* Background color when the cell is 
					 * highlighted. */
    Blt_Bg selectBg;			/* Background color when the cell is 
					 * selected. */
    GC normalGC;			/* Graphics context of normal text. */
    GC activeGC;			/* Graphics context of active text. */
    GC disableGC;			/* Graphics context of disabled text. */
    GC highlightGC;			/* Graphics context of highlighted
					 * text. */
    GC selectGC;			/* Graphics context of selected
					 * text. */
    Tk_Justify justify;			/* Indicates how the text or icon is
					 * justified within the column. */
    int borderWidth;			/* Width of outer border surrounding
					 * the entire box. */
    int relief, activeRelief;		/* Relief of outer border. */

    Tcl_Obj *cmdObjPtr;			/* If non-NULL, TCL procedure called
					 * to format the style is invoked.*/
    /* TextBox-specific fields */
    Tcl_Obj *editCmdObjPtr;		/* If non-NULL, TCL procedure called
					 * to allow the user to edit the text
					 * string. */
    int side;
} TextBoxStyle;

/* 
 * CheckBoxStyle --
 *
 *	Treats the cell as a check box that can possibly be edited (via a
 *	builtin check button).  The check box consists of the check indicator
 *	(a box with or without a check), an option icon, and an optional text
 *	string or image.  The icon may be to the left or right of the text.
 *	The check is always on the left.
 *
 *	When the check button is pressed, the table value associated with the
 *	cell is toggled.  The on/off values may be specified, but default to
 *	1/0.
 */
typedef struct {
    int refCount;			/* Usage reference count.  A reference
					 * count of zero indicates that the
					 * style may be freed. */
    unsigned int flags;			/* Bit field containing both the style
					 * type and various flags. */
    const char *name;			/* Instance name. */
    CellStyleClass *classPtr;		/* Contains class-specific information
					 * such as configuration
					 * specifications and configure, draw,
					 * layout, etc. routines. */
    Blt_HashEntry *hashPtr;		/* If non-NULL, points to the hash
					 * table entry for the style.  A style
					 * that's been deleted, but still in
					 * use (non-zero reference count) will
					 * have no hash table entry. */
    Blt_HashTable table;		/* Table of cells that have this
					 * style. We use this to mark the
					 * cells dirty when the style
					 * changes. */
    TableView *viewPtr;			/* Widget using this style. */
    /* General style fields. */
    Tk_Cursor cursor;			/* X Cursor */
    Icon icon;				/* If non-NULL, is a Tk_Image to be
					 * drawn in the cell. */
    int gap;				/* # pixels gap between icon and
					 * text. */
    Blt_Font font;
    XColor *normalFg;			/* Normal color of the text. */
    XColor *activeFg;			/* Color of the text when the cell is
					 * active. */
    XColor *disableFg;			/* Color of the text when the cell is
					 * disabled. */
    XColor *highlightFg;		/* Color of the text when the cell is
					 * highlighted. */
    XColor *selectFg;			/* Color of the text when the cell is
					 * selected. */
    Blt_Bg normalBg;			/* Normal background color of cell. */
    Blt_Bg activeBg;			/* Background color when the cell is
					 * active. Textboxes are usually never
					 * active. */
    Blt_Bg altBg;			/* Alternative normal background. */
    Blt_Bg disableBg;			/* Background color when the cell is
					 * disabled. */
    Blt_Bg highlightBg;			/* Background color when the cell is 
					 * highlighted. */
    Blt_Bg selectBg;			/* Background color when the cell is 
					 * selected. */
    GC normalGC;			/* Graphics context of normal text. */
    GC activeGC;			/* Graphics context of active text. */
    GC disableGC;			/* Graphics context of disabled text. */
    GC highlightGC;			/* Graphics context of highlighted
					 * text. */
    GC selectGC;			/* Graphics context of selected
					 * text. */
    Tk_Justify justify;			/* Indicates how the text or icon is
					 * justified within the column. */
    int borderWidth;			/* Width of outer border surrounding
					 * the entire box. */
    int relief, activeRelief;		/* Relief of outer border. */

    Tcl_Obj *cmdObjPtr;			/* If non-NULL, TCL procedure called
					 * to format the style is invoked.*/
    /* Checkbox specific fields. */
    int size;				/* Size of the checkbox. */
    Tcl_Obj *onValueObjPtr;
    Tcl_Obj *offValueObjPtr;
    int lineWidth;			/* Linewidth of the check box. */
    XColor *boxColor;			/* Rectangle (box) color (grey). */
    XColor *fillColor;			/* Fill color (white) */
    XColor *checkColor;			/* Check color (red). */

    TextLayout *onPtr, *offPtr;
    
    Blt_Painter painter;
    Blt_Picture selectedBox;
    Blt_Picture normalBox;
    Blt_Picture disableBox;
} CheckBoxStyle;


/* 
 * ComboBoxStyle --
 *
 *	Treats the cell as a combo box button that can possibly be edited (via
 *	an external combo menu).  The combo box consists of an option icon, a
 *	text string, and an arrow button.  The icon is always to the left of
 *	the text.  The arrow is always to the right.
 *
 *	When the combo button is pressed, a combo menu is posed, and the table
 *	value associated with the cell is set to the selected menu value.  The
 *	user can not edit values, only menu current items can be used.
 */
typedef struct {
    int refCount;			/* Usage reference count.  A reference
					 * count of zero indicates that the
					 * style may be freed. */
    unsigned int flags;			/* Bit field containing both the style
					 * type and various flags. */
    const char *name;			/* Instance name. */
    CellStyleClass *classPtr;		/* Contains class-specific information
					 * such as configuration
					 * specifications and configure, draw,
					 * layout, etc. routines. */
    Blt_HashEntry *hashPtr;		/* If non-NULL, points to the hash
					 * table entry for the style.  A style
					 * that's been deleted, but still in
					 * use (non-zero reference count) will
					 * have no hash table entry. */
    Blt_HashTable table;		/* Table of cells that have this
					 * style. We use this to mark the
					 * cells dirty when the style
					 * changes. */
    TableView *viewPtr;			/* Widget using this style. */
    /* General style fields. */
    Tk_Cursor cursor;			/* X Cursor */
    Icon icon;				/* If non-NULL, is a Tk_Image to be
					 * drawn in the cell. */
    int gap;				/* # pixels gap between icon and
					 * text. */
    Blt_Font font;
    XColor *normalFg;			/* Normal color of the text. */
    XColor *activeFg;			/* Color of the text when the cell is
					 * active. */
    XColor *disableFg;			/* Color of the text when the cell is
					 * disabled. */
    XColor *highlightFg;		/* Color of the text when the cell is
					 * highlighted. */
    XColor *selectFg;			/* Color of the text when the cell is
					 * selected. */
    Blt_Bg normalBg;			/* Normal background color of cell. */
    Blt_Bg activeBg;			/* Background color when the cell is
					 * active. Textboxes are usually never
					 * active. */
    Blt_Bg altBg;			/* Alternative normal background. */
    Blt_Bg disableBg;			/* Background color when the cell is
					 * disabled. */
    Blt_Bg highlightBg;			/* Background color when the cell is 
					 * highlighted. */
    Blt_Bg selectBg;			/* Background color when the cell is 
					 * selected. */
    GC normalGC;			/* Graphics context of normal text. */
    GC activeGC;			/* Graphics context of active text. */
    GC disableGC;			/* Graphics context of disabled text. */
    GC highlightGC;			/* Graphics context of highlighted
					 * text. */
    GC selectGC;			/* Graphics context of selected
					 * text. */
    Tk_Justify justify;			/* Indicates how the text or icon is
					 * justified within the column. */
    int borderWidth;			/* Width of outer border surrounding
					 * the entire box. */
    int relief, activeRelief;		/* Relief of outer border. */
    Tcl_Obj *cmdObjPtr;			/* If non-NULL, TCL procedure called
					 * to format the style is invoked.*/
    /* ComboBox-specific fields */
    int scrollWidth;
    /*  */
    int postedRelief;

    int textLen;
    /*
     * The combobox contains an optional icon and text string. 
     */
    Tcl_Obj *iconVarObjPtr;		/* Name of TCL variable.  If non-NULL,
					 * this variable contains the name of
					 * an image representing the icon.
					 * This overrides the value of the
					 * above field. */
    const char *text;			/* Text string to be displayed in the
					 * button if an image has no been
					 * designated. Its value is overridden
					 * by the -textvariable option. */
    Tcl_Obj *textVarObjPtr;		/* Name of TCL variable.  If non-NULL,
					 * this variable contains the text
					 * string to be displayed in the
					 * button. This overrides the above
					 * field. */
    /*  
     * Arrow (button) Information:
     *
     * The arrow is a button with an optional 3D border.
     */
    int arrowBW;
    int arrowPad;
    int arrowRelief;
    int reqArrowWidth;

    int prefWidth;			/* Desired width of window, measured
					 * in average characters. */
    int inset;
    short int arrowWidth, arrowHeight;
    short int iconWidth, iconHeight;
    short int width, height;
    Tcl_Obj *menuObjPtr;		/* Name of the menu to be posted by
					 * this style. */
    Tcl_Obj *postCmdObjPtr;		/* If non-NULL, command to be executed
					 * when this menu is posted. */
    int menuAnchor;
} ComboBoxStyle;

/* 
 * ImageBoxStyle --
 *
 *	Treats the cell as a image box that can't be edited.  The image box
 *	consists of an option icon, and an optional text string, and the
 *	image.  The icon is always centered to the left of the image.  The
 *	text may be above of below the image.
 */
typedef struct {
    int refCount;			/* Usage reference count.  A reference
					 * count of zero indicates that the
					 * style may be freed. */
    unsigned int flags;			/* Bit field containing both the style
					 * type and various flags. */
    const char *name;			/* Instance name. */
    CellStyleClass *classPtr;		/* Contains class-specific information
					 * such as configuration
					 * specifications and configure, draw,
					 * layout, etc. routines. */
    Blt_HashEntry *hashPtr;		/* If non-NULL, points to the hash
					 * table entry for the style.  A style
					 * that's been deleted, but still in
					 * use (non-zero reference count) will
					 * have no hash table entry. */
    Blt_HashTable table;		/* Table of cells that have this
					 * style. We use this to mark the
					 * cells dirty when the style
					 * changes. */
    TableView *viewPtr;			/* Widget using this style. */
    /* General style fields. */
    Tk_Cursor cursor;			/* X Cursor */
    Icon icon;				/* If non-NULL, is a Tk_Image to be
					 * drawn in the cell. */
    int gap;				/* # pixels gap between icon and
					 * text. */
    Blt_Font font;
    XColor *normalFg;			/* Normal color of the text. */
    XColor *activeFg;			/* Color of the text when the cell is
					 * active. */
    XColor *disableFg;			/* Color of the text when the cell is
					 * disabled. */
    XColor *highlightFg;		/* Color of the text when the cell is
					 * highlighted. */
    XColor *selectFg;			/* Color of the text when the cell is
					 * selected. */
    Blt_Bg normalBg;			/* Normal background color of cell. */
    Blt_Bg activeBg;			/* Background color when the cell is
					 * active. Textboxes are usually never
					 * active. */
    Blt_Bg altBg;			/* Alternative normal background. */
    Blt_Bg disableBg;			/* Background color when the cell is
					 * disabled. */
    Blt_Bg highlightBg;			/* Background color when the cell is 
					 * highlighted. */
    Blt_Bg selectBg;			/* Background color when the cell is 
					 * selected. */
    GC normalGC;			/* Graphics context of normal text. */
    GC activeGC;			/* Graphics context of active text. */
    GC disableGC;			/* Graphics context of disabled text. */
    GC highlightGC;			/* Graphics context of highlighted
					 * text. */
    GC selectGC;			/* Graphics context of selected
					 * text. */
    Tk_Justify justify;			/* Indicates how the text or icon is
					 * justified within the column. */
    int borderWidth;			/* Width of outer border surrounding
					 * the entire box. */
    int relief, activeRelief;		/* Relief of outer border. */
    Tcl_Obj *cmdObjPtr;			/* If non-NULL, TCL procedure called
					 * to format the style is invoked.*/
    /* ImageBox-specific fields */
    int side;				/* Position the text (top or bottom)
					 * in relation to the image.  */
} ImageBoxStyle;
 
static Blt_ConfigSpec textBoxStyleSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_ACTIVE_BG, 
	Blt_Offset(TextBoxStyle, activeBg), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "ActiveRelief", 
	DEF_TEXTBOX_ACTIVE_RELIEF, Blt_Offset(TextBoxStyle, activeRelief), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeFackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
	"ActiveForeground", DEF_ACTIVE_FG, 
	Blt_Offset(TextBoxStyle, activeFg), 0},
    {BLT_CONFIG_SYNONYM, "-altbg", "alternateBackground", (char *)NULL,
	(char *)NULL, 0, 0},
    {BLT_CONFIG_BACKGROUND, "-alternatebackground", "alternateBackground", 
	"Background", DEF_ALT_BG, Blt_Offset(TextBoxStyle, altBg), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_NORMAL_BG, Blt_Offset(TextBoxStyle, normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_TEXTBOX_BORDERWIDTH, Blt_Offset(TextBoxStyle, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CURSOR, "-cursor", "cursor", "Cursor", DEF_TEXTBOX_CURSOR, 
	Blt_Offset(TextBoxStyle, cursor), 0},
    {BLT_CONFIG_BACKGROUND, "-disabledbackground", "disabledBackground",
	"DisabledBackground", DEF_DISABLE_BG, 
        Blt_Offset(TextBoxStyle, disableBg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground", 
       "DisabledForeground", DEF_DISABLE_FG, 
	Blt_Offset(TextBoxStyle, disableFg), 0},
    {BLT_CONFIG_SYNONYM, "-disabledbg", "disabledBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-disabledfg", "disabledForeground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_BITMASK, "-edit", "edit", "Edit", DEF_TEXTBOX_EDIT, 
	Blt_Offset(TextBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT,
	(Blt_CustomOption *)EDIT},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_TEXTBOX_FONT,
	Blt_Offset(TextBoxStyle, font), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", 
	DEF_NORMAL_FG, Blt_Offset(TextBoxStyle, normalFg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-gap", "gap", "Gap", DEF_GAP, 
	Blt_Offset(TextBoxStyle, gap), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground", 
	"HighlightBackground", DEF_HIGHLIGHT_BG, 
	Blt_Offset(TextBoxStyle, highlightBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-highlightbg", "highlightBackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-highlightfg", "highlistFackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", 
	"HighlightForeground", DEF_HIGHLIGHT_FG, 
	Blt_Offset(TextBoxStyle, highlightFg), 0},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", DEF_ICON, 
	Blt_Offset(TextBoxStyle, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_JUSTIFY, 
	Blt_Offset(TextBoxStyle, justify), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
	"Foreground", DEF_SELECT_BG, Blt_Offset(TextBoxStyle, selectBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
	DEF_SELECT_FG, Blt_Offset(TextBoxStyle, selectFg), 0},
    {BLT_CONFIG_SIDE, "-side", "side", "side", DEF_TEXTBOX_SIDE, 
	Blt_Offset(TextBoxStyle, side), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0}
};

static Blt_ConfigSpec checkBoxStyleSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_ACTIVE_BG, 
	Blt_Offset(CheckBoxStyle, activeBg), 0},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeFackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
	"ActiveForeground", DEF_ACTIVE_FG, 
	Blt_Offset(CheckBoxStyle, activeFg), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "ActiveRelief", 
	DEF_CHECKBOX_ACTIVE_RELIEF, Blt_Offset(CheckBoxStyle, activeRelief), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-altbg", "alternateBackground", (char *)NULL,
	(char *)NULL, 0, 0},
    {BLT_CONFIG_BACKGROUND, "-alternatebackground", "alternateBackground", 
	"Background", DEF_ALT_BG, Blt_Offset(CheckBoxStyle, altBg), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_NORMAL_BG, Blt_Offset(CheckBoxStyle, normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_CHECKBOX_BORDERWIDTH, Blt_Offset(CheckBoxStyle, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_POS, "-boxsize", "boxSize", "BoxSize", DEF_CHECKBOX_SIZE,
	Blt_Offset(CheckBoxStyle, size), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", DEF_CHECKBOX_COMMAND, 
        Blt_Offset(CheckBoxStyle, cmdObjPtr), 0},
    {BLT_CONFIG_CURSOR, "-cursor", "cursor", "Cursor", DEF_CHECKBOX_CURSOR, 
	Blt_Offset(CheckBoxStyle, cursor), 0},
    {BLT_CONFIG_BACKGROUND, "-disabledbackground", "disabledBackground",
	"DisabledBackground", DEF_DISABLE_BG, 
        Blt_Offset(TextBoxStyle, disableBg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground", 
       "DisabledForeground", DEF_DISABLE_FG, 
	Blt_Offset(TextBoxStyle, disableFg), 0},
    {BLT_CONFIG_SYNONYM, "-disabledbg", "disabledBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-disabledfg", "disabledForeground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_BITMASK, "-edit", "edit", "Edit", DEF_CHECKBOX_EDIT, 
	Blt_Offset(CheckBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT,
	(Blt_CustomOption *)EDIT},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_CHECKBOX_FONT,
	Blt_Offset(TextBoxStyle, font), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", 
	DEF_NORMAL_FG, Blt_Offset(CheckBoxStyle, normalFg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-gap", "gap", "Gap", DEF_CHECKBOX_GAP, 
	Blt_Offset(CheckBoxStyle, gap), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground", 
	"HighlightBackground", DEF_HIGHLIGHT_BG, 
	Blt_Offset(CheckBoxStyle, highlightBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-highlightbg", "highlightBackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-highlightfg", "highlistFackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", 
	"HighlightForeground", DEF_HIGHLIGHT_FG, 
	Blt_Offset(CheckBoxStyle, highlightFg), 0},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", DEF_ICON, 
	Blt_Offset(CheckBoxStyle, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_JUSTIFY, 
	Blt_Offset(CheckBoxStyle, justify), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth",
	DEF_CHECKBOX_LINEWIDTH, Blt_Offset(CheckBoxStyle, lineWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_COLOR, "-checkcolor", "checkColor", "CheckColor", 
	DEF_CHECKBOX_CHECK_COLOR, Blt_Offset(CheckBoxStyle, checkColor), 0},
    {BLT_CONFIG_COLOR, "-boxcolor", "boxColor", "BoxColor", 
	DEF_CHECKBOX_BOX_COLOR, Blt_Offset(CheckBoxStyle, boxColor), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-fillcolor", "fillColor", "FillColor", 
	DEF_CHECKBOX_FILL_COLOR, Blt_Offset(CheckBoxStyle, fillColor), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-offvalue", "offValue", "OffValue",
	DEF_CHECKBOX_OFFVALUE, Blt_Offset(CheckBoxStyle, offValueObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-onvalue", "onValue", "OnValue",
	DEF_CHECKBOX_ONVALUE, Blt_Offset(CheckBoxStyle, onValueObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
	"Foreground", DEF_SELECT_BG, Blt_Offset(CheckBoxStyle, selectBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
	DEF_SELECT_FG, Blt_Offset(CheckBoxStyle, selectFg), 0},
    {BLT_CONFIG_BITMASK, "-showvalue", "showValue", "ShowValue",
	DEF_CHECKBOX_SHOWVALUE, Blt_Offset(CheckBoxStyle, flags), 
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SHOW_VALUES},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

static Blt_ConfigSpec comboBoxStyleSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_ACTIVE_BG, 
	Blt_Offset(ComboBoxStyle, activeBg), 0},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeFackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
	"ActiveForeground", DEF_ACTIVE_FG, 
	Blt_Offset(ComboBoxStyle, activeFg), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "ActiveRelief", 
	DEF_COMBOBOX_ACTIVE_RELIEF, Blt_Offset(ComboBoxStyle, activeRelief), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-altbg", "alternateBackground", (char *)NULL,
	(char *)NULL, 0, 0},
    {BLT_CONFIG_BACKGROUND, "-alternatebackground", "alternateBackground", 
	"Background", DEF_ALT_BG, Blt_Offset(ComboBoxStyle, altBg), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_NORMAL_BG, Blt_Offset(ComboBoxStyle, normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0, 
	0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_COMBOBOX_BORDERWIDTH, Blt_Offset(ComboBoxStyle, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-arrowrelief", "arrowRelief", "ArrowRelief",
	DEF_COMBOBOX_ARROW_RELIEF, Blt_Offset(ComboBoxStyle, arrowRelief),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-arrowborderwidth", "arrowBorderWidth", 
	"ArrowBorderWidth", DEF_COMBOBOX_ARROW_BORDERWIDTH, 
	Blt_Offset(ComboBoxStyle, arrowBW), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CURSOR, "-cursor", "cursor", "Cursor", DEF_COMBOBOX_CURSOR, 
	Blt_Offset(ComboBoxStyle, cursor), 0},
    {BLT_CONFIG_BACKGROUND, "-disabledbackground", "disabledBackground",
	"DisabledBackground", DEF_DISABLE_BG, 
        Blt_Offset(ComboBoxStyle, disableBg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground", 
       "DisabledForeground", DEF_DISABLE_FG, 
	Blt_Offset(ComboBoxStyle, disableFg), 0},
    {BLT_CONFIG_SYNONYM, "-disabledbg", "disabledBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-disabledfg", "disabledForeground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_BITMASK, "-edit", "edit", "Edit", DEF_COMBOBOX_EDIT, 
	Blt_Offset(ComboBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT,
	(Blt_CustomOption *)EDIT},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_COMBOBOX_FONT,
	Blt_Offset(ComboBoxStyle, font), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
	DEF_NORMAL_FG, Blt_Offset(ComboBoxStyle, normalFg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-gap", "gap", "Gap", DEF_GAP, 
	Blt_Offset(ComboBoxStyle, gap), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground", 
	"HighlightBackground", DEF_HIGHLIGHT_BG, 
	Blt_Offset(ComboBoxStyle, highlightBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-highlightbg", "highlightBackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-highlightfg", "highlistFackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", 
	"HighlightForeground", DEF_HIGHLIGHT_FG, 
	Blt_Offset(ComboBoxStyle, highlightFg), 0},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", DEF_ICON, 
	Blt_Offset(ComboBoxStyle, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_CUSTOM, "-iconvariable", "iconVariable", "IconVariable", 
	DEF_COMBOBOX_ICON_VARIABLE, Blt_Offset(ComboBoxStyle, iconVarObjPtr), 
        BLT_CONFIG_NULL_OK, &iconVarOption},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_JUSTIFY, 
	Blt_Offset(ComboBoxStyle, justify), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-menu", "menu", "Menu", DEF_COMBOBOX_MENU, 
	Blt_Offset(ComboBoxStyle, menuObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-postcommand", "postCommand", "PostCommand", 
	DEF_COMBOBOX_POST_CMD, Blt_Offset(ComboBoxStyle, postCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_RELIEF, "-postedrelief", "postedRelief", "PostedRelief", 
	DEF_COMBOBOX_POSTED_RELIEF, Blt_Offset(ComboBoxStyle, postedRelief), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_COMBOBOX_RELIEF, 
	Blt_Offset(ComboBoxStyle, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
	"Foreground", DEF_SELECT_BG, Blt_Offset(ComboBoxStyle, selectBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
	DEF_SELECT_FG, Blt_Offset(ComboBoxStyle, selectFg), 0},
    {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_COMBOBOX_STATE, 
	Blt_Offset(ComboBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	&stateOption},
    {BLT_CONFIG_CUSTOM, "-text", "text", "Text", DEF_COMBOBOX_TEXT, 
	Blt_Offset(ComboBoxStyle, text), 0, &textOption},
    {BLT_CONFIG_CUSTOM, "-textvariable", "textVariable", "TextVariable", 
	DEF_COMBOBOX_TEXT_VARIABLE, Blt_Offset(ComboBoxStyle, textVarObjPtr), 
        BLT_CONFIG_NULL_OK, &textVarOption},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};


static Blt_ConfigSpec imageBoxStyleSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_ACTIVE_BG, 
	Blt_Offset(ImageBoxStyle, activeBg), 0},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeFackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
	"ActiveForeground", DEF_ACTIVE_FG, 
	Blt_Offset(ImageBoxStyle, activeFg), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "ActiveRelief", 
	DEF_IMAGEBOX_ACTIVE_RELIEF, Blt_Offset(ImageBoxStyle, activeRelief), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-altbg", "alternateBackground", (char *)NULL,
	(char *)NULL, 0, 0},
    {BLT_CONFIG_BACKGROUND, "-alternatebackground", "alternateBackground", 
	"Background", DEF_ALT_BG, Blt_Offset(ImageBoxStyle, altBg), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	DEF_NORMAL_BG, Blt_Offset(ImageBoxStyle, normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_IMAGEBOX_BORDERWIDTH, Blt_Offset(ImageBoxStyle, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", DEF_IMAGEBOX_COMMAND, 
	Blt_Offset(ImageBoxStyle, cmdObjPtr), 0},
    {BLT_CONFIG_CURSOR, "-cursor", "cursor", "Cursor", DEF_IMAGEBOX_CURSOR, 
	Blt_Offset(ImageBoxStyle, cursor), 0},
    {BLT_CONFIG_BACKGROUND, "-disabledbackground", "disabledBackground",
	"DisabledBackground", DEF_DISABLE_BG, 
        Blt_Offset(ImageBoxStyle, disableBg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground", 
       "DisabledForeground", DEF_DISABLE_FG, 
	Blt_Offset(ImageBoxStyle, disableFg), 0},
    {BLT_CONFIG_SYNONYM, "-disabledbg", "disabledBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-disabledfg", "disabledForeground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_BITMASK, "-edit", "edit", "Edit", DEF_IMAGEBOX_EDIT, 
	Blt_Offset(ImageBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT,
	(Blt_CustomOption *)EDIT},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_IMAGEBOX_FONT,
	Blt_Offset(ImageBoxStyle, font), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", 
	DEF_NORMAL_FG, Blt_Offset(ImageBoxStyle, normalFg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-gap", "gap", "Gap", DEF_GAP, 
	Blt_Offset(ImageBoxStyle, gap), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground", 
	"HighlightBackground", DEF_HIGHLIGHT_BG, 
	Blt_Offset(ImageBoxStyle, highlightBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-highlightbg", "highlightBackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-highlightfg", "highlistFackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", 
	"HighlightForeground", DEF_HIGHLIGHT_FG, 
	Blt_Offset(ImageBoxStyle, highlightFg), 0},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", DEF_ICON, 
	Blt_Offset(ImageBoxStyle, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_JUSTIFY, 
	Blt_Offset(ImageBoxStyle, justify), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
	"Foreground", DEF_SELECT_BG, Blt_Offset(ImageBoxStyle, selectBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
	DEF_SELECT_FG, Blt_Offset(ImageBoxStyle, selectFg), 0},
    {BLT_CONFIG_BITMASK, "-showtext", "showText", "ShowText", 
	DEF_IMAGEBOX_SHOW_TEXT, Blt_Offset(ImageBoxStyle, flags), 
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SHOW_TEXT},
    {BLT_CONFIG_SIDE, "-side", "side", "side", DEF_IMAGEBOX_SIDE, 
	Blt_Offset(ImageBoxStyle, side), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0}
};

static CellStyleConfigureProc CheckBoxStyleConfigureProc;
static CellStyleConfigureProc ComboBoxStyleConfigureProc;
static CellStyleConfigureProc ImageBoxStyleConfigureProc;
static CellStyleConfigureProc TextBoxStyleConfigureProc;
static CellStyleDrawProc CheckBoxStyleDrawProc;
static CellStyleDrawProc ComboBoxStyleDrawProc;
static CellStyleDrawProc ImageBoxStyleDrawProc;
static CellStyleDrawProc TextBoxStyleDrawProc;
static CellStyleFreeProc CheckBoxStyleFreeProc;
static CellStyleFreeProc ComboBoxStyleFreeProc;
static CellStyleFreeProc ImageBoxStyleFreeProc;
static CellStyleFreeProc TextBoxStyleFreeProc;
static CellStyleGeometryProc CheckBoxStyleGeometryProc;
static CellStyleGeometryProc ComboBoxStyleGeometryProc;
static CellStyleGeometryProc ImageBoxStyleGeometryProc;
static CellStyleGeometryProc TextBoxStyleGeometryProc;
#ifdef notdef
static CellStyleIdentifyProc CheckBoxStyleIdentifyProc;
#endif
static CellStyleIdentifyProc ComboBoxStyleIdentifyProc;
static CellStylePostProc ComboBoxStylePostProc;
static CellStyleUnpostProc ComboBoxStyleUnpostProc;

/* TextBoxStyle */
static CellStyleClass textBoxStyleClass = {
    "textbox",
    "TextBoxStyle",
    textBoxStyleSpecs,
    TextBoxStyleConfigureProc,
    TextBoxStyleGeometryProc,
    TextBoxStyleDrawProc,
    NULL,				/* identProc */
    TextBoxStyleFreeProc,
    NULL,				/* postProc */
    NULL,				/* unpostProc */
};

/* CheckBoxStyle */
static CellStyleClass checkBoxStyleClass = {
    "checkbox",
    "CheckBoxStyle",
    checkBoxStyleSpecs,
    CheckBoxStyleConfigureProc,
    CheckBoxStyleGeometryProc,
    CheckBoxStyleDrawProc,
    NULL,				/* identProc */
    CheckBoxStyleFreeProc,
    NULL,				/* postProc */
    NULL,				/* unpostProc */
};

/* ComboBoxStyle */
static CellStyleClass comboBoxStyleClass = {
    "combobox", 
    "ComboBoxStyle",
    comboBoxStyleSpecs,
    ComboBoxStyleConfigureProc,
    ComboBoxStyleGeometryProc,
    ComboBoxStyleDrawProc,
    ComboBoxStyleIdentifyProc,
    ComboBoxStyleFreeProc,
    ComboBoxStylePostProc,
    ComboBoxStyleUnpostProc,
};

/* ImageBoxStyle */
static CellStyleClass imageBoxStyleClass = {
    "imagebox", 
    "ImageBoxStyle",
    imageBoxStyleSpecs,
    ImageBoxStyleConfigureProc,
    ImageBoxStyleGeometryProc,
    ImageBoxStyleDrawProc,
    NULL,				/* identProc */
    ImageBoxStyleFreeProc,
    NULL,				/* postProc */
    NULL,				/* unpostProc */
};

static INLINE Blt_Bg 
GetHighlightBg(CellStyle *stylePtr, Row *rowPtr) 
{
    if (stylePtr->highlightBg != NULL) {
	return stylePtr->highlightBg;
    }
    if ((stylePtr->altBg != NULL) && (rowPtr->visibleIndex & 0x1)) {
	return stylePtr->altBg;
    }
    return stylePtr->normalBg;
}


/*
 *---------------------------------------------------------------------------
 *
 * PropagateDirtyFlags --
 *
 *	Sets the dirty flag on all the row, columns, and cells using this
 *	style.  This is because the style changed such that the geometry
 *	of the individual cells may not be valid anymore.  Mark them
 *	as dirty and recompute the geometry at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static void
PropagateDirtyFlags(TableView *viewPtr, CellStyle *stylePtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    long i;

    /* Step 1: Check for specially applied cells.  */
    for (hPtr = Blt_FirstHashEntry(&stylePtr->table, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Cell *cellPtr;

	cellPtr = Blt_GetHashValue(hPtr);
	cellPtr->flags |= GEOMETRY;
    }
    /* Step 2: Check for rows with the same style.  */
    for (i = 0; i < viewPtr->numRows; i++) {
	Row *rowPtr;

	rowPtr = viewPtr->rows[i];
	if (rowPtr->stylePtr == stylePtr) {
	    rowPtr->flags |= GEOMETRY;
	}
    }
    /* Step 3: Check for columns with the same style.  */
    for (i = 0; i < viewPtr->numColumns; i++) {
	Column *colPtr;

	colPtr = viewPtr->columns[i];
	if (colPtr->stylePtr == stylePtr) {
	    colPtr->flags |= GEOMETRY;
	}
    }
    Blt_TableView_EventuallyRedraw(viewPtr);
}

static int
UpdateTextVariable(Tcl_Interp *interp, ComboBoxStyle *stylePtr) 
{
    Tcl_Obj *resultObjPtr, *objPtr;
	
    objPtr = Tcl_NewStringObj(stylePtr->text, stylePtr->textLen);
    Tcl_IncrRefCount(objPtr);
    resultObjPtr = Tcl_ObjSetVar2(interp, stylePtr->textVarObjPtr, NULL, 
	objPtr, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    Tcl_DecrRefCount(objPtr);
    if (resultObjPtr == NULL) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
UpdateIconVariable(Tcl_Interp *interp, ComboBoxStyle *stylePtr) 
{
    Tcl_Obj *resultObjPtr, *objPtr;
    
    if (stylePtr->icon != NULL) {
	objPtr = Tcl_NewStringObj(IconName(stylePtr->icon), -1);
    } else {
	objPtr = Tcl_NewStringObj("", -1);
    }
    Tcl_IncrRefCount(objPtr);
    resultObjPtr = Tcl_ObjSetVar2(interp, stylePtr->iconVarObjPtr, NULL, 
	objPtr, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    Tcl_DecrRefCount(objPtr);
    if (resultObjPtr == NULL) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

static void
SetTextFromObj(ComboBoxStyle *stylePtr, Tcl_Obj *objPtr) 
{
    int numBytes;
    const char *string;
	    
    if (stylePtr->text != NULL) {
	Blt_Free(stylePtr->text);
    }
    string = Tcl_GetStringFromObj(objPtr, &numBytes);
    stylePtr->text = Blt_AssertMalloc(numBytes + 1);
    strncpy((char *)stylePtr->text, string, numBytes);
    stylePtr->textLen = numBytes;
    stylePtr->flags |= LAYOUT_PENDING;
}

/*
 *---------------------------------------------------------------------------
 *
 * IconChangedProc
 *
 *	Called when the image changes for the icon used in a cell style.
 *	Everything using style (cells, rows, and columns) are marked dirty.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
IconChangedProc(ClientData clientData, int x, int y, int width, int height,
		int imageWidth, int imageHeight)	
{
    CellStyle *stylePtr = clientData;
    TableView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    viewPtr->flags |= GEOMETRY;
    PropagateDirtyFlags(viewPtr, stylePtr);
}

static Icon
GetIcon(CellStyle *stylePtr, const char *iconName)
{
    Blt_HashEntry *hPtr;
    int isNew;
    struct _Icon *iconPtr;
    TableView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    hPtr = Blt_CreateHashEntry(&viewPtr->iconTable, iconName, &isNew);
    if (isNew) {
	Tk_Image tkImage;
	int w, h;

	tkImage = Tk_GetImage(viewPtr->interp, viewPtr->tkwin,(char *)iconName, 
		IconChangedProc, stylePtr);
	if (tkImage == NULL) {
	    Blt_DeleteHashEntry(&viewPtr->iconTable, hPtr);
	    return NULL;
	}
	Tk_SizeOfImage(tkImage, &w, &h);
	iconPtr = Blt_AssertMalloc(sizeof(struct _Icon));
	iconPtr->viewPtr  = viewPtr;
	iconPtr->tkImage  = tkImage;
	iconPtr->hashPtr  = hPtr;
	iconPtr->refCount = 1;
	iconPtr->width    = w;
	iconPtr->height   = h;
	Blt_SetHashValue(hPtr, iconPtr);
    } else {
	iconPtr = Blt_GetHashValue(hPtr);
	iconPtr->refCount++;
    }
    return iconPtr;
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

static void
FreeIcon(Icon icon)
{
    struct _Icon *iconPtr = icon;

    iconPtr->refCount--;
    if (iconPtr->refCount == 0) {
	TableView *viewPtr;

	viewPtr = iconPtr->viewPtr;
	Blt_DeleteHashEntry(&viewPtr->iconTable, iconPtr->hashPtr);
	Tk_FreeImage(iconPtr->tkImage);
	Blt_Free(iconPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CellImageChangedProc
 *
 * Results:
 *	None.
 *
 * FIXME:  The tableview widget needs to know that the cell geometry 
 *	   image needs to be recomputed.  All we have at this point is
 *	   the cell.  We need the viewPtr;
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
CellImageChangedProc(ClientData clientData, int x, int y, int width, int height,
		     int imageWidth, int imageHeight)	
{
    Cell *cellPtr = clientData;

    /* FIXME: need to signal redraw to tableview widget.  */
    cellPtr->flags |= GEOMETRY;
}


/*ARGSUSED*/
static void
FreeIconProc(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Icon *iconPtr = (Icon *)(widgRec + offset);

    if (*iconPtr != NULL) {
	FreeIcon(*iconPtr);
	*iconPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToIconProc --
 *
 *	Convert the name of an icon into a Tk image.
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
    Icon *iconPtr = (Icon *)(widgRec + offset);
    CellStyle *stylePtr = (CellStyle *)widgRec;
    Icon icon;
    int length;
    const char *string;

    string = Tcl_GetStringFromObj(objPtr, &length);
    icon = NULL;
    if (length > 0) {
	icon = GetIcon(stylePtr, string);
	if (icon == NULL) {
	    return TCL_ERROR;
	}
    }
    if (*iconPtr != NULL) {
	FreeIcon(*iconPtr);
    }
    *iconPtr = icon;
    if (strcmp(stylePtr->classPtr->className, "ComboBoxStyle") == 0) {
	ComboBoxStyle *comboPtr = (ComboBoxStyle *)stylePtr;

	if (comboPtr->iconVarObjPtr != NULL) {
	    if (UpdateIconVariable(interp, comboPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
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

    if (icon == NULL) {
	return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(Blt_Image_Name((icon)->tkImage), -1);
}

/*
 *---------------------------------------------------------------------------
 * 
 * TextVarTraceProc --
 *
 *	This procedure is invoked when someone changes the state variable
 *	associated with a combobutton.  
 *
 * Results:
 *	NULL is always returned.
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
    ComboBoxStyle *stylePtr = clientData;

    assert(stylePtr->textVarObjPtr != NULL);
    if (flags & TCL_INTERP_DESTROYED) {
    	return NULL;			/* Interpreter is going away. */

    }
    /*
     * If the variable is being unset, then re-establish the trace.
     */
    if (flags & TCL_TRACE_UNSETS) {
	if (flags & TCL_TRACE_DESTROYED) {
	    Tcl_SetVar(interp, name1, stylePtr->text, TCL_GLOBAL_ONLY);
	    Tcl_TraceVar(interp, name1, TRACE_VAR_FLAGS, TextVarTraceProc, 
		clientData);
	    stylePtr->flags |= TEXT_VAR_TRACED;
	}
	return NULL;
    }
    if (flags & TCL_TRACE_WRITES) {
	Tcl_Obj *valueObjPtr;

	/*
	 * Update the combobutton's text with the value of the variable,
	 * unless the widget already has that value (this happens when the
	 * variable changes value because we changed it because someone typed
	 * in the entry).
	 */
	valueObjPtr = Tcl_ObjGetVar2(interp, stylePtr->textVarObjPtr, NULL, 
		TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
	if (valueObjPtr == NULL) {
	    return GetInterpResult(interp);
	} else {
	    SetTextFromObj(stylePtr, valueObjPtr);
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 * 
 * IconVarTraceProc --
 *
 *	This procedure is invoked when someone changes the state
 *	variable associated with combobutton. 
 *
 * Results:
 *	NULL is always returned.
 *
 *---------------------------------------------------------------------------
 */
static char *
IconVarTraceProc(
    ClientData clientData,		/* Information about the item. */
    Tcl_Interp *interp,			/* Interpreter containing variable. */
    const char *name1,			/* First part of variable's name. */
    const char *name2,			/* Second part of variable's name. */
    int flags)				/* Describes what just happened. */
{
    ComboBoxStyle *stylePtr = clientData;

    assert(stylePtr->iconVarObjPtr != NULL);
    if (flags & TCL_INTERP_DESTROYED) {
    	return NULL;			/* Interpreter is going away. */

    }
    /*
     * If the variable is being unset, then re-establish the trace.
     */
    if (flags & TCL_TRACE_UNSETS) {
	if (flags & TCL_TRACE_DESTROYED) {
	    Tcl_SetVar(interp, name1, IconName(stylePtr->icon),TCL_GLOBAL_ONLY);
	    Tcl_TraceVar(interp, name1, TRACE_VAR_FLAGS, IconVarTraceProc, 
		clientData);
	    stylePtr->flags |= ICON_VAR_TRACED;
	}
	return NULL;
    }
    if (flags & TCL_TRACE_WRITES) {
	Icon icon;
	Tcl_Obj *valueObjPtr;

	/*
	 * Update the combobutton's icon with the image whose name is
	 * stored in the variable.
	 */
	valueObjPtr = Tcl_ObjGetVar2(interp, stylePtr->iconVarObjPtr, NULL, 
		TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
	if (valueObjPtr == NULL) {
	    return GetInterpResult(interp);
	}
	icon = GetIcon((CellStyle *)stylePtr, Tcl_GetString(valueObjPtr));
	if (icon == NULL) {
	    return GetInterpResult(interp);
	}
	if (stylePtr->icon != NULL) {
	    FreeIcon(stylePtr->icon);
	}
	stylePtr->icon = icon;
	stylePtr->viewPtr->flags |= GEOMETRY;
	Blt_TableView_EventuallyRedraw(stylePtr->viewPtr);
    }
    return NULL;
}

/*ARGSUSED*/
static void
FreeIconVarProc(
    ClientData clientData,
    Display *display,			/* Not used. */
    char *widgRec,
    int offset)
{
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);

    if (*objPtrPtr != NULL) {
	ComboBoxStyle *stylePtr = (ComboBoxStyle *)widgRec;

	Tcl_UntraceVar(stylePtr->viewPtr->interp, Tcl_GetString(*objPtrPtr), 
		TRACE_VAR_FLAGS, IconVarTraceProc, stylePtr);
	Tcl_DecrRefCount(*objPtrPtr);
	*objPtrPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToIconVarProc --
 *
 *	Convert the variable to a traced variable.
 *
 * Results:
 *	The return value is a standard TCL result.  The icon variable is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToIconVarProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to report results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing style. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)(widgRec);
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);
    char *varName;
    Tcl_Obj *valueObjPtr;

    /* Remove the current trace on the variable. */
    if (*objPtrPtr != NULL) {
	Tcl_UntraceVar(interp, Tcl_GetString(*objPtrPtr), TRACE_VAR_FLAGS, 
		       IconVarTraceProc, stylePtr);
	Tcl_DecrRefCount(*objPtrPtr);
	*objPtrPtr = NULL;
    }
    varName = Tcl_GetString(objPtr);
    if ((varName[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
	return TCL_OK;
    }

    valueObjPtr = Tcl_ObjGetVar2(interp, objPtr, NULL, TCL_GLOBAL_ONLY);
    if (valueObjPtr != NULL) {
	Icon icon;

	icon = GetIcon((CellStyle *)stylePtr, Tcl_GetString(valueObjPtr));
	if (icon == NULL) {
	    return TCL_ERROR;
	}
	if (stylePtr->icon != NULL) {
	    FreeIcon(stylePtr->icon);
	}
	stylePtr->icon = icon;
    }
    *objPtrPtr = objPtr;
    Tcl_IncrRefCount(objPtr);
    Tcl_TraceVar(interp, varName, TRACE_VAR_FLAGS, IconVarTraceProc, stylePtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IconVarToObjProc --
 *
 *	Return the name of the style.
 *
 * Results:
 *	The name representing the icon variable is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
IconVarToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget information record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Tcl_Obj *objPtr = *(Tcl_Obj **)(widgRec + offset);

    if (objPtr == NULL) {
	objPtr = Tcl_NewStringObj("", -1);
    } 
    return objPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToStateProc --
 *
 *	Converts the string representing a state into a bitflag.
 *
 * Results:
 *	The return value is a standard TCL result.  The state flags are
 *	updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToStateProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to report results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing state. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    const char *string;
    char c;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'n') && (strncmp(string, "normal", length) == 0)) {
	*flagsPtr &= ~POSTED;
    } else if ((c == 'p') && (strncmp(string, "posted", length) == 0)) {
	*flagsPtr |= POSTED;
    } else {
	Tcl_AppendResult(interp, "unknown state \"", string, 
	    "\": should be posted or normal.", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StateToObjProc --
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
StateToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget information record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    unsigned int state = *(unsigned int *)(widgRec + offset);
    const char *string;

    if (state & POSTED) {
	string = "posted";
    } else {
	string = "normal";
    }
    return Tcl_NewStringObj(string, -1);
}

/*ARGSUSED*/
static void
FreeTextVarProc(
    ClientData clientData,
    Display *display,			/* Not used. */
    char *widgRec,
    int offset)
{
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);

    if (*objPtrPtr != NULL) {
	ComboBoxStyle *stylePtr = (ComboBoxStyle *)(widgRec);
	char *varName;

	varName = Tcl_GetString(*objPtrPtr);
	Tcl_UntraceVar(stylePtr->viewPtr->interp, varName, TRACE_VAR_FLAGS, 
		TextVarTraceProc, stylePtr);
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
 *	The return value is a standard TCL result.  The text variable is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTextVarProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to report results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing style. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)(widgRec);
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);
    char *varName;
    Tcl_Obj *valueObjPtr;

    /* Remove the current trace on the variable. */
    if (*objPtrPtr != NULL) {
	varName = Tcl_GetString(*objPtrPtr);
	Tcl_UntraceVar(interp, varName, TRACE_VAR_FLAGS, TextVarTraceProc, 
		stylePtr);
	Tcl_DecrRefCount(*objPtrPtr);
	*objPtrPtr = NULL;
    }
    varName = Tcl_GetString(objPtr);
    if ((varName[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
	return TCL_OK;
    }

    valueObjPtr = Tcl_ObjGetVar2(interp, objPtr, NULL, TCL_GLOBAL_ONLY);
    if (valueObjPtr != NULL) {
	SetTextFromObj(stylePtr, valueObjPtr);
	if (stylePtr->textVarObjPtr != NULL) {
	    if (UpdateTextVariable(interp, stylePtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    *objPtrPtr = objPtr;
    Tcl_IncrRefCount(objPtr);
    Tcl_TraceVar(interp, varName, TRACE_VAR_FLAGS, TextVarTraceProc, stylePtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextVarToObjProc --
 *
 *	Return the name of the text variable.
 *
 * Results:
 *	The name representing the text variable is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TextVarToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget information record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Tcl_Obj *objPtr = *(Tcl_Obj **)(widgRec + offset);

    if (objPtr == NULL) {
	objPtr = Tcl_NewStringObj("", -1);
    } 
    return objPtr;
}


/*ARGSUSED*/
static void
FreeTextProc(ClientData clientData, Display *display, char *widgRec, int offset)
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)(widgRec);

    if (stylePtr->text != NULL) {
	Blt_Free(stylePtr->text);
	stylePtr->text = NULL;
	stylePtr->textLen = 0;
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
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to report results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing style. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)(widgRec);

    if (stylePtr->text != NULL) {
	Blt_Free(stylePtr->text);
	stylePtr->text = NULL;
	stylePtr->textLen = 0;
    }
    SetTextFromObj(stylePtr, objPtr);
    if (stylePtr->textVarObjPtr != NULL) {
	if (UpdateTextVariable(interp, stylePtr) != TCL_OK) {
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
 *	Return the text of the item.
 *
 * Results:
 *	The text is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TextToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget information record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)(widgRec);

    if (stylePtr->text == NULL) {
	return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(stylePtr->text, stylePtr->textLen);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewTextBoxStyle --
 *
 *	Creates a "textbox" style.
 *
 * Results:
 *	A pointer to the new style structure.
 *
 *---------------------------------------------------------------------------
 */
static CellStyle *
NewTextBoxStyle(TableView *viewPtr, Blt_HashEntry *hPtr)
{
    TextBoxStyle *stylePtr;

    stylePtr = Blt_AssertCalloc(1, sizeof(TextBoxStyle));
    stylePtr->classPtr = &textBoxStyleClass;
    stylePtr->viewPtr = viewPtr;
    stylePtr->side = SIDE_LEFT;
    stylePtr->gap = STYLE_GAP;
    stylePtr->relief = stylePtr->activeRelief = TK_RELIEF_FLAT;
    stylePtr->name = Blt_GetHashKey(&viewPtr->styleTable, hPtr);
    stylePtr->hashPtr = hPtr;
    stylePtr->flags = 0;		/*  */
    stylePtr->refCount = 1;
    stylePtr->borderWidth = 1;
    Blt_SetHashValue(hPtr, stylePtr);
    Blt_InitHashTable(&stylePtr->table, sizeof(CellKey)/sizeof(int));
    return (CellStyle *)stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextBoxStyleConfigureProc --
 *
 *	Configures a "textbox" style.  This routine generates the GCs required
 *	for a textbox style.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs are created for the style.
 *
 *---------------------------------------------------------------------------
 */
static void
TextBoxStyleConfigureProc(TableView *viewPtr, CellStyle *cellStylePtr)
{
    TextBoxStyle *stylePtr = (TextBoxStyle *)cellStylePtr;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    gcMask = GCForeground | GCFont | GCDashList | GCLineWidth | GCLineStyle;
    gcValues.dashes = 1;
    gcValues.font = Blt_Font_Id(stylePtr->font);
    gcValues.line_width = 0;
    gcValues.line_style = LineOnOffDash;

    /* Normal text. */
    gcValues.foreground = stylePtr->normalFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    stylePtr->normalGC = newGC;

    /* Disabled text. */
    gcValues.foreground = stylePtr->disableFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->disableGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->disableGC);
    }
    stylePtr->disableGC = newGC;

    /* Selected text. */
    gcValues.foreground = stylePtr->selectFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->selectGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->selectGC);
    }
    stylePtr->selectGC = newGC;

    /* Active text. */
    gcValues.foreground = stylePtr->activeFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    stylePtr->activeGC = newGC;

    /* Highlight text. */
    gcValues.foreground = stylePtr->highlightFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    if (Blt_ConfigModified(stylePtr->classPtr->specs, "-font", (char *)NULL)) {
	PropagateDirtyFlags(viewPtr, (CellStyle *)stylePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TextBoxStyleGeometryProc --
 *
 *	Determines the space requirements for the "textbox" given the value to
 *	be displayed.  Depending upon whether an icon or text is displayed and
 *	their relative placements, this routine computes the space needed for
 *	the text entry.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The width and height fields of *cellPtr* are set with the computed
 *	dimensions. The dirty flag is cleared.
 *
 *---------------------------------------------------------------------------
 */
static void
TextBoxStyleGeometryProc(Cell *cellPtr, CellStyle *cellStylePtr)
{
    CellKey *keyPtr;
    Column *colPtr;
    Row *rowPtr;
    TextBoxStyle *stylePtr = (TextBoxStyle *)cellStylePtr;
    int gap;
    unsigned int iw, ih, tw, th;
    TableView *viewPtr;

    viewPtr = cellPtr->viewPtr;
    cellPtr->flags &= ~GEOMETRY;	/* Remove the geometry flag from the
					 * cell. */
    keyPtr = GetKey(cellPtr);
    rowPtr = keyPtr->rowPtr;
    colPtr = keyPtr->colPtr;

    cellPtr->width = cellPtr->height = 2 * (stylePtr->borderWidth + FOCUS_PAD);
    cellPtr->width += colPtr->ruleWidth;
    cellPtr->height += rowPtr->ruleHeight;
    /* Free the old text and image (in case we switched styles). */
    if ((cellPtr->text != NULL) && (cellPtr->flags & TEXTALLOC)) {
	Blt_Free(cellPtr->text);
    }
    cellPtr->flags &= ~TEXTALLOC;
    if (cellPtr->tkImage != NULL) {
	Tk_FreeImage(cellPtr->tkImage);
    }
    cellPtr->text = NULL;
    cellPtr->tkImage = NULL;

    if (!blt_table_value_exists(viewPtr->table, rowPtr->row, colPtr->column)){
	return;				/* No data. */
    }
    /* Get the actual text to be displayed. If the style has a -formatcommand
     * defined, call it to get the formatted text string. */
    if (colPtr->fmtCmdObjPtr != NULL) {
	Tcl_Obj *cmdObjPtr, *objPtr;
	int result;
	Tcl_Interp *interp;
	const char *text;

	interp = viewPtr->interp;
	/* command rowIndex columnIndex */
	cmdObjPtr = Tcl_DuplicateObj(colPtr->fmtCmdObjPtr);
	objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
	Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
	Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	Tcl_IncrRefCount(cmdObjPtr);
	result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(cmdObjPtr);
	if (result != TCL_OK) {
	    Tcl_BackgroundError(interp);
	    return;
	}
	text = Tcl_GetString(Tcl_GetObjResult(interp));
	cellPtr->text = Blt_Strdup(text);
	cellPtr->flags |= TEXTALLOC;
    } else {
	cellPtr->text = blt_table_get_string(viewPtr->table, rowPtr->row, 
		colPtr->column);
    }
    /* Now compute the geometry. */
    tw = th = iw = ih = 0;
    gap = 0;
    if (stylePtr->icon != NULL) {
	iw = IconWidth(stylePtr->icon);
	ih = IconHeight(stylePtr->icon);
    } 
    if (cellPtr->text != NULL) {	
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, stylePtr->font);
	Blt_Ts_GetExtents(&ts, cellPtr->text, &tw, &th);
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
	cellPtr->textWidth = tw;
	cellPtr->textHeight = th;
    } 
    if (stylePtr->side & (SIDE_TOP | SIDE_BOTTOM)) {
	cellPtr->width  += MAX(tw, iw) + 2 * CELL_PADX;
	cellPtr->height += ih + gap + th + 2 * CELL_PADY;
    } else {
	cellPtr->width  += iw + gap + tw + 2 * CELL_PADX;
	cellPtr->height += MAX(th, ih) + 2 * CELL_PADY;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TextBoxStyleDrawProc --
 *
 *	Draws the textbox given the screen coordinates and the value to be
 *	displayed.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The textbox value is drawn.
 *
 *      +-------------+	
 *	||Icon| |text||	
 *      +-------------+
 *  
 *---------------------------------------------------------------------------
 */
static void
TextBoxStyleDrawProc(Cell *cellPtr, Drawable drawable, CellStyle *cellStylePtr,
		     int x, int y)
{
    TextBoxStyle *stylePtr = (TextBoxStyle *)cellStylePtr;
    int ix, iy, iw, ih;
    int tx, ty, tw, th;
    int gap, colWidth, rowHeight, cellWidth, cellHeight;
    Blt_Bg bg;
    GC gc;
    CellKey *keyPtr;
    Row *rowPtr;
    Column *colPtr;
    int relief;
    TableView *viewPtr;

    viewPtr = cellPtr->viewPtr;
    keyPtr = GetKey(cellPtr);
    rowPtr = keyPtr->rowPtr;
    colPtr = keyPtr->colPtr;
    relief = stylePtr->relief;
    if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & SELECTED) { 
	/* Selected */
	bg = stylePtr->selectBg;
	gc = stylePtr->selectGC;
    } else if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & DISABLED) {
	/* Disabled */
	bg = stylePtr->disableBg;
	gc = stylePtr->disableGC;
    } else if ((stylePtr->flags & EDIT) && (viewPtr->activePtr == cellPtr)) {
	/* Active */
	bg = stylePtr->activeBg;
	gc = stylePtr->activeGC;
	relief = stylePtr->activeRelief;
    } else if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & HIGHLIGHT) { 
	/* Highlighted */
	bg = GetHighlightBg((CellStyle *)stylePtr, rowPtr);
	gc = stylePtr->highlightGC;
    } else {		
	/* Normal */
	bg = stylePtr->normalBg;
	if ((stylePtr->altBg != NULL) && (rowPtr->visibleIndex & 0x1)) {
	    bg = stylePtr->altBg;
	}
	gc = stylePtr->normalGC;
    }
    rowHeight = rowPtr->height;
    colWidth  = colPtr->width;

    /* Draw background. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, colWidth,
	rowHeight, stylePtr->borderWidth, relief);

    /* Draw Rule */
    if (rowPtr->ruleHeight > 0) {
	rowHeight -= rowPtr->ruleHeight;
	XFillRectangle(viewPtr->display, drawable, gc, x, y + rowHeight, 
		colWidth, rowPtr->ruleHeight);
    }
    if (colPtr->ruleWidth > 0) {
	colWidth -= colPtr->ruleWidth;
	XFillRectangle(viewPtr->display, drawable, gc, x + colWidth, y, 
		colPtr->ruleWidth, rowHeight);
    }
    rowHeight -= 2 * stylePtr->borderWidth + 3;
    colWidth  -= 2 * stylePtr->borderWidth + 3;
    x += stylePtr->borderWidth + 1;
    y += stylePtr->borderWidth + 1;

    /* Draw the focus ring if this cell has focus. */
    if ((viewPtr->flags & FOCUS) && (viewPtr->focusPtr == cellPtr)) {
	XDrawRectangle(viewPtr->display, drawable, gc, x, y, colWidth, 
		       rowHeight);
    }
    x += CELL_PADX;
    y += CELL_PADY;
    cellHeight = cellPtr->height - 2 * (stylePtr->borderWidth + CELL_PADY) - 3;
    cellWidth = cellPtr->width - 2 * (stylePtr->borderWidth + CELL_PADX) - 3;
    /* Justify (x) and center (y) the contents of the cell. */
    if (rowHeight > cellHeight) {
	y += (rowHeight - cellHeight) / 2;
    }
    if (colWidth > cellWidth) {
	switch (stylePtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    x += (colWidth - cellWidth);
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (colWidth - cellWidth) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }
    tw = th = iw = ih = 0;		/* Suppress compiler warning. */
    gap = 0;
    if (stylePtr->icon != NULL) {
	iw = IconWidth(stylePtr->icon);
	ih = IconHeight(stylePtr->icon);
    }
    if (cellPtr->text != NULL) {
	tw = cellPtr->textWidth;
	th = cellPtr->textHeight;
	tw = cellWidth - iw;
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    ix = tx = x, iy = ty = y;
    switch (stylePtr->side) {
    case SIDE_RIGHT:
	tx = x;
	if (cellHeight > th) {
	    ty = y + (cellHeight - th) / 2;
	}
	ix = tx + tw + gap;
	if (cellHeight > ih) {
	    iy = y + (cellHeight - ih) / 2;
	}
	break;
    case SIDE_LEFT:
	ix = x;
	if (cellHeight > ih) {
	    iy = y + (cellHeight - ih) / 2;
	}
	tx = ix + iw + gap;
	if (cellHeight > th) {
	    ty = y + (cellHeight - th) / 2;
	}
	break;
    case SIDE_TOP:
	iy = y;
	if (cellWidth > iw) {
	    ix = x + (cellWidth - iw) / 2;
	}
	ty = iy + ih + gap;
	if (cellWidth > tw) {
	    tx = x + (cellWidth - tw) / 2;
	}
	break;
    case SIDE_BOTTOM:
	ty = y;
	if (cellWidth > tw) {
	    tx = x + (cellWidth - tw) / 2;
	}
	iy = ty + th + gap;
	if (cellWidth > iw) {
	    ix = x + (cellWidth - iw) / 2;
	}
	break;
    }
    if (stylePtr->icon != NULL) {
	Tk_RedrawImage(IconBits(stylePtr->icon), 0, 0, iw, ih,drawable, ix, iy);
    }
    if (cellPtr->text != NULL) {
	TextStyle ts;
	int xMax;
	TextLayout *textPtr;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, stylePtr->font);
	Blt_Ts_SetGC(ts, gc);
	xMax = colWidth - iw - gap;
	Blt_Ts_SetMaxLength(ts, xMax);
	textPtr = Blt_Ts_CreateLayout(cellPtr->text, -1, &ts);
	Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, textPtr, &ts, tx, ty);
	if (viewPtr->activePtr == cellPtr) {
	    Blt_Ts_UnderlineLayout(viewPtr->tkwin, drawable, textPtr,&ts,tx,ty);
	}
	Blt_Free(textPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TextBoxStyleFreeProc --
 *
 *	Releases resources allocated for the textbox. 
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs allocated for the textbox are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
TextBoxStyleFreeProc(CellStyle *cellStylePtr)
{
    TextBoxStyle *stylePtr = (TextBoxStyle *)cellStylePtr;
    TableView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    iconOption.clientData = viewPtr;
    Blt_FreeOptions(stylePtr->classPtr->specs, (char *)stylePtr, 
	viewPtr->display, 0);
    if (stylePtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&viewPtr->styleTable, stylePtr->hashPtr);
    } 
    Blt_DeleteHashTable(&stylePtr->table);
    if (stylePtr->selectGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->selectGC);
    }
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->disableGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->disableGC);
    }
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    Blt_Free(stylePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewCheckBoxStyle --
 *
 *	Creates a "checkbox" style.
 *
 * Results:
 *	A pointer to the new style structure.
 *
 *---------------------------------------------------------------------------
 */
static CellStyle *
NewCheckBoxStyle(TableView *viewPtr, Blt_HashEntry *hPtr)
{
    CheckBoxStyle *stylePtr;

    stylePtr = Blt_AssertCalloc(1, sizeof(CheckBoxStyle));
    stylePtr->classPtr = &checkBoxStyleClass;
    stylePtr->viewPtr = viewPtr;
    stylePtr->gap = 4;
    stylePtr->size = 15;
    stylePtr->lineWidth = 2;
    stylePtr->name = Blt_GetHashKey(&viewPtr->styleTable, hPtr);
    stylePtr->hashPtr = hPtr;
    stylePtr->flags = EDIT | SHOW_VALUES;
    stylePtr->relief = TK_RELIEF_FLAT;
    stylePtr->activeRelief = TK_RELIEF_RAISED;
    stylePtr->borderWidth = 1;
    stylePtr->refCount = 1;
    Blt_SetHashValue(hPtr, stylePtr);
    Blt_InitHashTable(&stylePtr->table, sizeof(CellKey)/sizeof(int));
    return (CellStyle *)stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckBoxStyleConfigureProc --
 *
 *	Configures a "checkbox" style.  This routine performs generates the
 *	GCs required for a checkbox style.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs are created for the style.
 *
 *---------------------------------------------------------------------------
 */
static void
CheckBoxStyleConfigureProc(TableView *viewPtr, CellStyle *cellStylePtr)
{
    GC newGC;
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    XGCValues gcValues;
    unsigned long gcMask;

    gcMask = GCForeground | GCFont | GCDashList | GCLineWidth | GCLineStyle;
    gcValues.dashes = 1;
    gcValues.font = Blt_Font_Id(stylePtr->font);
    gcValues.line_width = 0;
    gcValues.line_style = LineOnOffDash;

    /* Normal text. */
    gcValues.foreground = stylePtr->normalFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    stylePtr->normalGC = newGC;

    /* Active text. */
    gcValues.foreground = stylePtr->activeFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    stylePtr->activeGC = newGC;

    /* Disabled text. */
    gcValues.foreground = stylePtr->disableFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->disableGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->disableGC);
    }
    stylePtr->disableGC = newGC;

    /* Selected text. */
    gcValues.foreground = stylePtr->selectFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->selectGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->selectGC);
    }
    stylePtr->selectGC = newGC;

    /* Highlight text. */
    gcValues.foreground = stylePtr->highlightFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    if (Blt_ConfigModified(stylePtr->classPtr->specs, "-boxsize", 
		(char *)NULL)) {
	if (stylePtr->selectedBox != NULL) {
	    Blt_FreePicture(stylePtr->selectedBox);
	    stylePtr->selectedBox = NULL;
	}
	if (stylePtr->normalBox != NULL) {
	    Blt_FreePicture(stylePtr->normalBox);
	    stylePtr->normalBox = NULL;
	}
    }
    if (stylePtr->selectedBox == NULL) {
	unsigned int bw, bh;

	bw = bh = stylePtr->size | 0x1;
	stylePtr->selectedBox = Blt_PaintCheckbox(bw, bh, stylePtr->fillColor, 
		stylePtr->boxColor, stylePtr->checkColor, TRUE);
    } 
    if (stylePtr->normalBox == NULL) {
	unsigned int bw, bh;

	bw = bh = stylePtr->size | 0x1;
	stylePtr->normalBox = Blt_PaintCheckbox(bw, bh, stylePtr->fillColor,
		stylePtr->boxColor, stylePtr->checkColor, FALSE);
    } 
    if ((stylePtr->flags & SHOW_VALUES) && 
	(Blt_ConfigModified(stylePtr->classPtr->specs, "-font", (char *)NULL))){
	PropagateDirtyFlags(viewPtr, (CellStyle *)stylePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckBoxStyleGeometryProc --
 *
 *	Determines the space requirements for the "checkbox" given the value
 *	to be displayed.  Depending upon whether an icon or text is displayed
 *	and their relative placements, this routine computes the space needed
 *	for the text entry.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The width and height fields of *cellPtr* are set with the
 *	computed dimensions.
 *
 *---------------------------------------------------------------------------
 */
static void
CheckBoxStyleGeometryProc(Cell *cellPtr, CellStyle *cellStylePtr)
{
    CellKey *keyPtr;
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    Column *colPtr;
    Row *rowPtr;
    int bw, bh;
    int gap;
    int iw, ih, tw, th;
    TableView *viewPtr;

    viewPtr = cellPtr->viewPtr;
    keyPtr = GetKey(cellPtr);
    rowPtr = keyPtr->rowPtr;
    colPtr = keyPtr->colPtr;

    cellPtr->flags &= ~GEOMETRY;	/* Remove the dirty flag from the
					 * cell. */
    bw = bh = stylePtr->size | 0x1;
    tw = th = iw = ih = 0;
    cellPtr->width = cellPtr->height = 2 * (stylePtr->borderWidth + FOCUS_PAD);
    cellPtr->width += 2 * CELL_PADX;
    cellPtr->height += 2 * CELL_PADY;
    cellPtr->width += colPtr->ruleWidth;
    cellPtr->height += rowPtr->ruleHeight;

    if (stylePtr->icon != NULL) {
	iw = IconWidth(stylePtr->icon);
	ih = IconHeight(stylePtr->icon);
    } 
    if (stylePtr->onPtr != NULL) {
	Blt_Free(stylePtr->onPtr);
	stylePtr->onPtr = NULL;
    }
    if (stylePtr->offPtr != NULL) {
	Blt_Free(stylePtr->offPtr);
	stylePtr->offPtr = NULL;
    }
    gap = 0;
    if ((cellPtr->text != NULL) && (cellPtr->flags & TEXTALLOC)) {
	Blt_Free(cellPtr->text);
    }
    cellPtr->flags &= ~TEXTALLOC;
    if (cellPtr->tkImage != NULL) {
	Tk_FreeImage(cellPtr->tkImage);
    }
    cellPtr->text = NULL;
    cellPtr->tkImage = NULL;

    if (blt_table_value_exists(viewPtr->table, rowPtr->row, colPtr->column)) {
	if (colPtr->fmtCmdObjPtr != NULL) {
	    Tcl_Obj *cmdObjPtr, *objPtr;
	    int result;
	    Tcl_Interp *interp;
	    const char *text;
	    
	    interp = viewPtr->interp;
	    /* command rowIndex columnIndex */
	    cmdObjPtr = Tcl_DuplicateObj(colPtr->fmtCmdObjPtr);
	    objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
	    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	    objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
	    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	    Tcl_IncrRefCount(cmdObjPtr);
	    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
	    Tcl_DecrRefCount(cmdObjPtr);
	    if (result != TCL_OK) {
		Tcl_BackgroundError(interp);
		return;
	    }
	    text = Tcl_GetString(Tcl_GetObjResult(interp));
	    cellPtr->text = Blt_Strdup(text);
	    cellPtr->flags |= TEXTALLOC;
	} else {
	    cellPtr->text = blt_table_get_string(viewPtr->table, rowPtr->row, 
		colPtr->column);
	}
    }
    if (stylePtr->flags & SHOW_VALUES) {
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, stylePtr->font);
	if ((Blt_ConfigModified(stylePtr->classPtr->specs, "-onvalue", 
		(char *)NULL)) || (stylePtr->onPtr == NULL)) {
	    const char *string;
	    int length;

	    if (stylePtr->onPtr != NULL) {
		Blt_Free(stylePtr->onPtr);
	    }
	    string = Tcl_GetStringFromObj(stylePtr->onValueObjPtr, &length);
	    stylePtr->onPtr = Blt_Ts_CreateLayout(string, length, &ts);
	}
	if ((Blt_ConfigModified(stylePtr->classPtr->specs, "-offvalue", 
		(char *)NULL)) || (stylePtr->offPtr == NULL)) {
	    const char *string;
	    int length;

	    if (stylePtr->offPtr != NULL) {
		Blt_Free(stylePtr->offPtr);
	    }
	    string = Tcl_GetStringFromObj(stylePtr->offValueObjPtr, &length);
	    stylePtr->offPtr = Blt_Ts_CreateLayout(string, length, &ts);
	}
	tw = MAX(stylePtr->offPtr->width,  stylePtr->onPtr->width);
	th = MAX(stylePtr->offPtr->height, stylePtr->onPtr->height);
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    cellPtr->width += stylePtr->gap * 2 + bw + iw + gap + tw;
    cellPtr->height += MAX3(bh, th, ih) | 0x1;
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckBoxStyleDrawProc --
 *
 *	Draws the "checkbox" given the screen coordinates and the
 *	value to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *      +-----------------+	
 *	||x| |Icon| |text||	
 *      +-----------------+
 *  
 *---------------------------------------------------------------------------
 */
static void
CheckBoxStyleDrawProc(Cell *cellPtr, Drawable drawable, CellStyle *cellStylePtr,
		      int x, int y)
{
    Blt_Bg bg;
    CellKey *keyPtr;
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    Column *colPtr;
    GC gc;
    Row *rowPtr;
    TextLayout *textPtr;
    int bool;
    int bx, by, ix, iy, tx, ty;
    unsigned int gap, colWidth, rowHeight, cellWidth, cellHeight;
    int relief;
    unsigned int bw, bh, iw, ih, th;
    TableView *viewPtr;

    viewPtr = cellPtr->viewPtr;
    keyPtr = GetKey(cellPtr);
    rowPtr = keyPtr->rowPtr;
    colPtr = keyPtr->colPtr;

    relief = stylePtr->relief;
    if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & SELECTED) { 
	/* Selected */
	bg = stylePtr->selectBg;
	gc = stylePtr->selectGC;
    } else if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & DISABLED) {
	/* Disabled */
	bg = stylePtr->disableBg;
	gc = stylePtr->disableGC;
    } else if ((stylePtr->flags & EDIT) && (viewPtr->activePtr == cellPtr)) {
	/* Active */
	bg = stylePtr->activeBg;
	gc = stylePtr->activeGC;
	relief = stylePtr->activeRelief;
    } else if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & HIGHLIGHT) { 
	/* Highlighted */
	bg = GetHighlightBg((CellStyle *)stylePtr, rowPtr);
	gc = stylePtr->highlightGC;
    } else {		
	/* Normal */
	bg = stylePtr->normalBg;
	if ((stylePtr->altBg != NULL) && (rowPtr->visibleIndex & 0x1)) {
	    bg = stylePtr->altBg;
	}
	gc = stylePtr->normalGC;
    }

    rowHeight = rowPtr->height;
    colWidth  = colPtr->width;

    /* Draw background. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, colWidth,
	rowHeight, stylePtr->borderWidth, relief);

    rowHeight -= 2 * stylePtr->borderWidth + 3;
    colWidth  -= 2 * stylePtr->borderWidth + 3;
    x += stylePtr->borderWidth + 1;
    y += stylePtr->borderWidth + 1;
    /* Draw the focus ring if this cell has focus. */
    if ((viewPtr->flags & FOCUS) && (viewPtr->focusPtr == cellPtr)) {
	XDrawRectangle(viewPtr->display, drawable, gc, x, y, colWidth, 
		       rowHeight);
    }
    x += CELL_PADX;
    y += CELL_PADY;
    cellHeight = cellPtr->height - 2 * (stylePtr->borderWidth + CELL_PADY) - 3;
    cellWidth = cellPtr->width - 2 * (stylePtr->borderWidth + CELL_PADX) - 3;

    /* Justify (x) and center (y) the contents of the cell. */
    if (rowHeight > cellHeight) {
	y += (rowHeight - cellHeight) / 2;
    }
    if (colWidth > cellWidth) {
	switch(stylePtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    x += (colWidth - cellWidth);
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (colWidth - cellWidth) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }

    if (cellPtr->text == NULL) {
	bool = 0;
    } else {
	const char *string;

	string = Tcl_GetString(stylePtr->onValueObjPtr);
	bool = (strcmp(cellPtr->text, string) == 0);
    }
    textPtr = (bool) ? stylePtr->onPtr : stylePtr->offPtr;

    /*
     * Draw the box and check. 
     *
     *		+-----------+
     *		|           |
     *		|         * |
     *          |        *  |
     *          | *     *   |
     *          |  *   *    |
     *          |   * *     |
     *		|    *      |
     *		+-----------+
     */
    bw = bh = stylePtr->size | 0x1;
    bx = x + stylePtr->gap;
    by = y + ((int)cellHeight - (int)bh) / 2;
    {
	Blt_Picture picture;
	
	picture = (bool) ? stylePtr->selectedBox : stylePtr->normalBox;
	if (stylePtr->painter == NULL) {
	    stylePtr->painter = Blt_GetPainter(viewPtr->tkwin, 1.0);
	}
	Blt_PaintPicture(stylePtr->painter, drawable, picture, 0, 0, 
		bw, bh, bx, by, 0);
    }
    iw = ih = 0;
    if (stylePtr->icon != NULL) {
	iw = IconWidth(stylePtr->icon);
	ih = IconHeight(stylePtr->icon);
    }
    th = 0;
    gap = 0;
    if (stylePtr->flags & SHOW_VALUES) {
	th = textPtr->height;
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    x = bx + bw + stylePtr->gap;

    /* The icon sits to the left of the text. */
    ix = x;
    iy = y + (cellHeight - ih) / 2;
    tx = ix + iw + gap;
    ty = y + (cellHeight - th) / 2;
    if (stylePtr->icon != NULL) {
	Tk_RedrawImage(IconBits(stylePtr->icon), 0, 0, iw, ih, drawable,ix, iy);
    }
    if (stylePtr->flags & SHOW_VALUES) {
	TextStyle ts;
	int xMax;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, stylePtr->font);
	Blt_Ts_SetGC(ts, gc);
	xMax = colWidth - iw - bw - gap - stylePtr->gap;
	Blt_Ts_SetMaxLength(ts, xMax);
	Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, textPtr, &ts, tx, ty);
    }
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * CheckBoxStyleIdentifyProc --
 *
 *	Draws the "checkbox" given the screen coordinates and the value to be
 *	displayed.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *---------------------------------------------------------------------------
 */
static int
CheckBoxStyleIdentifyProc(TableView *viewPtr, Cell *cellPtr, 
			  CellStyle *cellStylePtr, int x, int y)
{
    Column *colPtr;
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    int columnWidth;
    int x1, y1, x2, y2;
    int rowHeight, colWidth;

    rowHeight = rowPtr->height;
    colWidth  = colPtr->width;
    if (columnWidth > cellPtr->width) {
	switch(stylePtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    x1 += (colWidth - cellPtr->width);
	    break;
	case TK_JUSTIFY_CENTER:
	    x1 += (colWidth - cellPtr->width) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }
    x2 = x1 + cellPtr->width;
    y2 = y2 + cellPtr->height;
    width = height = (stylePtr->size | 0x1) + 2 * stylePtr->lineWidth;
    x = colPtr->worldX + colPtr->pad.side1 + stylePtr->gap - stylePtr->lineWidth;
    y = rowPtr->worldY + (rowPtr->height - height) / 2;
    if ((x >= x1) && (x < x2) && (y >= y1) && (y2 < y2)) {
	return TRUE;
    }
    return FALSE;
}
#endif


/*
 *---------------------------------------------------------------------------
 *
 * CheckBoxStyleFreeProc --
 *
 *	Releases resources allocated for the checkbox. 
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs allocated for the checkbox are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
CheckBoxStyleFreeProc(CellStyle *cellStylePtr)
{
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    TableView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    iconOption.clientData = viewPtr;
    Blt_FreeOptions(stylePtr->classPtr->specs, (char *)stylePtr, 
	viewPtr->display, 0);
    if (stylePtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&viewPtr->styleTable, stylePtr->hashPtr);
    } 
    Blt_DeleteHashTable(&stylePtr->table);
    if (stylePtr->selectGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->selectGC);
    }
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->disableGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->disableGC);
    }
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    if (stylePtr->offPtr != NULL) {
	Blt_Free(stylePtr->offPtr);
    }
    if (stylePtr->onPtr != NULL) {
	Blt_Free(stylePtr->onPtr);
    }
    Blt_Free(stylePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NeweComboBoxStyle --
 *
 *	Creates a "combobox" style.
 *
 * Results:
 *	A pointer to the new style structure.
 *
 *---------------------------------------------------------------------------
 */
static CellStyle *
NewComboBoxStyle(TableView *viewPtr, Blt_HashEntry *hPtr)
{
    ComboBoxStyle *stylePtr;

    stylePtr = Blt_AssertCalloc(1, sizeof(ComboBoxStyle));
    stylePtr->classPtr = &comboBoxStyleClass;
    stylePtr->viewPtr = viewPtr;
    stylePtr->gap = STYLE_GAP;
    stylePtr->arrowRelief = stylePtr->activeRelief = TK_RELIEF_RAISED;
    stylePtr->arrowBW = 2;
    stylePtr->arrowWidth = ARROW_WIDTH;
    stylePtr->borderWidth = 1;
    stylePtr->relief = TK_RELIEF_FLAT;
    stylePtr->name = Blt_GetHashKey(&viewPtr->styleTable, hPtr);
    stylePtr->hashPtr = hPtr;
    stylePtr->flags = EDIT;
    stylePtr->refCount = 1;
    stylePtr->postedRelief = TK_RELIEF_SUNKEN;
    Blt_SetHashValue(hPtr, stylePtr);
    Blt_InitHashTable(&stylePtr->table, sizeof(CellKey)/sizeof(int));
    return (CellStyle *)stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleConfigureProc --
 *
 *	Configures a "combobox" style.  This routine performs generates the
 *	GCs required for a combobox style.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs are created for the style.
 *
 *---------------------------------------------------------------------------
 */
static void
ComboBoxStyleConfigureProc(TableView *viewPtr, CellStyle *cellStylePtr)
{
    GC newGC;
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    XGCValues gcValues;
    unsigned long gcMask;

    gcMask = GCForeground | GCFont | GCDashList | GCLineWidth | GCLineStyle;
    gcValues.dashes = 1;
    gcValues.font = Blt_Font_Id(stylePtr->font);
    gcValues.line_width = 0;
    gcValues.line_style = LineOnOffDash;

    /* Normal text. */
    gcValues.foreground = stylePtr->normalFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    stylePtr->normalGC = newGC;

    /* Active text. */
    gcValues.foreground = stylePtr->activeFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    stylePtr->activeGC = newGC;

    /* Disabled text. */
    gcValues.foreground = stylePtr->disableFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->disableGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->disableGC);
    }
    stylePtr->disableGC = newGC;

    /* Selected text. */
    gcValues.foreground = stylePtr->selectFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->selectGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->selectGC);
    }
    stylePtr->selectGC = newGC;

    /* Highlight text. */
    gcValues.foreground = stylePtr->highlightFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    if (Blt_ConfigModified(stylePtr->classPtr->specs, "-font", (char *)NULL)) {
	PropagateDirtyFlags(viewPtr, (CellStyle *)stylePtr);
    }
}


static int
GetComboMenuGeometry(Tcl_Interp *interp, TableView *viewPtr, 
		     ComboBoxStyle *stylePtr, unsigned int *widthPtr, 
		     unsigned int *heightPtr)
{
    Tcl_Obj *cmdObjPtr, *objPtr;
    unsigned int maxWidth, maxHeight;
    Tcl_Obj **objv;
    int objc, i, result;

    cmdObjPtr = Tcl_DuplicateObj(stylePtr->menuObjPtr);
    objPtr = Tcl_NewStringObj("names", 5);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    Tcl_IncrRefCount(cmdObjPtr);
    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(cmdObjPtr);
    if (result != TCL_OK) {
	return TCL_ERROR;
    }
    objPtr = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(objPtr);
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    maxWidth = maxHeight = 0;
    for (i = 0; i < objc ; i++) {
	const char *text;
	TextStyle ts;
	unsigned int tw, th;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, stylePtr->font);
	text = Tcl_GetString(objv[i]);
	Blt_Ts_GetExtents(&ts, text, &tw, &th);
	if (maxWidth < tw) {
	    maxWidth = tw;
	}
	if (maxHeight < th) {
	    maxHeight = th;
	}
    }
    Tcl_DecrRefCount(objPtr);
    *widthPtr = maxWidth;
    *heightPtr = maxHeight;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleGeometryProc --
 *
 *	Determines the space requirements for the "combobox" given the value
 *	to be displayed.  Depending upon whether an icon or text is displayed
 *	and their relative placements, this routine computes the space needed
 *	for the text entry.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The width and height fields of *cellPtr* are set with the computed
 *	dimensions.
 *
 *---------------------------------------------------------------------------
 */
static void
ComboBoxStyleGeometryProc(Cell *cellPtr, CellStyle *cellStylePtr)
{
    CellKey *keyPtr;
    Column *colPtr;
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    Row *rowPtr;
    int gap;
    unsigned int iw, ih, tw, th, ah;
    TableView *viewPtr;

    viewPtr = cellPtr->viewPtr;
    keyPtr = GetKey(cellPtr);
    rowPtr = keyPtr->rowPtr;
    colPtr = keyPtr->colPtr;

    cellPtr->flags &= ~GEOMETRY;	/* Remove the dirty flag from the
					 * cell. */
    tw = th = iw = ih = 0;
    cellPtr->width = cellPtr->height = 2 * (stylePtr->borderWidth + FOCUS_PAD);
    cellPtr->width += 2 * CELL_PADX;
    cellPtr->height += 2 * CELL_PADY;
    cellPtr->width += colPtr->ruleWidth;
    cellPtr->height += rowPtr->ruleHeight;

    /* We don't know if the menu changed.  Do this once for the style. */
    if (stylePtr->menuObjPtr != NULL) {
	GetComboMenuGeometry(viewPtr->interp, viewPtr, stylePtr, &tw, &th);
	cellPtr->textWidth = tw;
	cellPtr->textHeight = th;
    }
    if (stylePtr->icon != NULL) {
	iw = IconWidth(stylePtr->icon);
	ih = IconHeight(stylePtr->icon);
    } 
    if ((cellPtr->text != NULL) && (cellPtr->flags & TEXTALLOC)) {
	Blt_Free(cellPtr->text);
    }
    cellPtr->flags &= ~TEXTALLOC;
    if (cellPtr->tkImage != NULL) {
	Tk_FreeImage(cellPtr->tkImage);
    }
    cellPtr->tkImage = NULL;

    gap = 0;
    cellPtr->text = blt_table_get_string(viewPtr->table, rowPtr->row, 
			colPtr->column);
    if (cellPtr->text != NULL) {	/* New string defined. */
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    } 
    ah = stylePtr->arrowWidth = Blt_TextWidth(stylePtr->font, "0", 1) + 
	2 * (stylePtr->arrowBW + 1);
    cellPtr->width  += iw + 4 * gap + stylePtr->arrowWidth + tw;
    cellPtr->height += MAX3(th, ih, ah);
}


/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleDrawProc --
 *
 *	Draws the "combobox" given the screen coordinates and the
 *	value to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The combobox value is drawn.
 *
 *      +-----------------+	
 *	||Icon| |text| |v||	
 *      +--------------+--+
 *  
 *---------------------------------------------------------------------------
 */
static void
ComboBoxStyleDrawProc(Cell *cellPtr, Drawable drawable, CellStyle *cellStylePtr,
		      int x, int y)
{
    Blt_Bg bg;
    CellKey *keyPtr;
    Column *colPtr;
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    GC gc;
    Row *rowPtr;
    int ix, iy, tx, ty;
    unsigned int gap, colWidth, rowHeight, cellWidth, cellHeight;
    unsigned int iw, ih, th;
    int relief;
    XColor *fg;
    TableView *viewPtr;

    viewPtr = cellPtr->viewPtr;
    keyPtr = GetKey(cellPtr);
    rowPtr = keyPtr->rowPtr;
    colPtr = keyPtr->colPtr;

    relief = stylePtr->relief;
    if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & SELECTED) { 
	bg = stylePtr->selectBg;
	gc = stylePtr->selectGC;
	fg = stylePtr->selectFg;
    } else if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & DISABLED) {
	/* Disabled */
	bg = stylePtr->disableBg;
	gc = stylePtr->disableGC;
	fg = stylePtr->disableFg;
    } else if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & HIGHLIGHT) { 
	/* Highlighted */
	bg = GetHighlightBg((CellStyle *)stylePtr, rowPtr);
	gc = stylePtr->highlightGC;
	fg = stylePtr->highlightFg;
    } else {				/* Normal */
	bg = stylePtr->normalBg;
	if ((stylePtr->altBg != NULL) && (rowPtr->visibleIndex & 0x1)) {
	    bg = stylePtr->altBg;
	}
	gc = stylePtr->normalGC;
	fg = stylePtr->normalFg;
    }
    rowHeight = rowPtr->height;
    colWidth  = colPtr->width;

    /* Draw background. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, colWidth,
	rowHeight, stylePtr->borderWidth, stylePtr->relief);

    rowHeight -= 2 * stylePtr->borderWidth + 3;
    colWidth  -= 2 * stylePtr->borderWidth + 3;
    x += stylePtr->borderWidth + 1;
    y += stylePtr->borderWidth + 1;
    /* Draw the focus ring if this cell has focus. */
    if ((viewPtr->flags & FOCUS) && (viewPtr->focusPtr == cellPtr)) {
	XDrawRectangle(viewPtr->display, drawable, gc, x, y, colWidth, 
		       rowHeight);
    }
    x += CELL_PADX;
    y += CELL_PADY;
    cellHeight = cellPtr->height - 2 * (stylePtr->borderWidth + CELL_PADY) - 3;
    cellWidth = cellPtr->width - 2 * (stylePtr->borderWidth + CELL_PADX) - 3;

    /* Justify (x) and center (y) the contents of the cell. */
    if (rowHeight > cellHeight) {
	y += (rowHeight - cellHeight) / 2;
    }
    if (colWidth > cellWidth) {
	switch(stylePtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    x += (colWidth - cellWidth);
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (colWidth - cellWidth) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }

#ifdef notdef
    tx = ty = ix = iy = 0;	/* Suppress compiler warning. */
#endif
    iw = ih = 0;
    if (stylePtr->icon != NULL) {
	iw = IconWidth(stylePtr->icon);
	ih = IconHeight(stylePtr->icon);
    }
    th = 0;
    if (cellPtr->text != NULL) {
	/* FIXME: */
	th = cellHeight;
    }
    gap = 0;
    if ((stylePtr->icon != NULL) && (cellPtr->text != NULL)) {
	gap = stylePtr->gap;
    }

    ix = x + gap;
    iy = y + (cellHeight - ih) / 2;
    tx = ix + iw + gap;
    ty = y + (cellHeight - th) / 2;

    if (stylePtr->icon != NULL) {
	Tk_RedrawImage(IconBits(stylePtr->icon), 0, 0, iw, ih, drawable, ix,iy);
    }
    if (cellPtr->text != NULL) {
	TextStyle ts;
	int xMax;
	TextLayout *textPtr;
	
	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, stylePtr->font);
	Blt_Ts_SetGC(ts, gc);
	xMax = SCREENX(viewPtr, colPtr->worldX) + colWidth - 
	    stylePtr->arrowWidth;
	Blt_Ts_SetMaxLength(ts, xMax - tx);
	textPtr = Blt_Ts_CreateLayout(cellPtr->text, -1, &ts);
	Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, textPtr, &ts, tx, ty);
	if (viewPtr->activePtr == cellPtr) {
	    Blt_Ts_UnderlineLayout(viewPtr->tkwin, drawable, textPtr,&ts,tx,ty);
	}
	Blt_Free(textPtr);
    }
    if ((stylePtr->flags & EDIT) && (viewPtr->activePtr == cellPtr)) {
	int ax, ay;
	unsigned int aw, ah;

	aw = stylePtr->arrowWidth + (2 * stylePtr->arrowBW);
	ah = cellHeight /* - (2 * stylePtr->gap)*/;
	ax = x + colWidth - aw - stylePtr->gap;
	ay = y;

	bg = stylePtr->activeBg;
	fg = stylePtr->activeFg;
	relief = (stylePtr->flags & POSTED) ? 
	    stylePtr->postedRelief : stylePtr->activeRelief;
	Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, ax, ay, aw, ah, 
		stylePtr->arrowBW, relief);
	aw -= 2 * stylePtr->borderWidth;
	ax += stylePtr->borderWidth;
	Blt_DrawArrow(viewPtr->display, drawable, fg, ax, ay, aw, ah, 
		stylePtr->arrowBW, ARROW_DOWN);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleIdentifyProc --
 *
 *	Draws the "combobox" given the screen coordinates and the value to be
 *	displayed.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *---------------------------------------------------------------------------
 */
static const char *
ComboBoxStyleIdentifyProc(Cell *cellPtr, CellStyle *cellStylePtr, int x, int y)
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    int ax;
    unsigned int aw;
    CellKey *keyPtr;
    Column *colPtr;

    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    aw = stylePtr->arrowWidth + (2 * stylePtr->arrowBW);
    ax = colPtr->width - stylePtr->borderWidth - aw - stylePtr->gap;
    if ((x >= 0) && (x < ax)) {
	return "text";
    }
    return "button";
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleFreeProc --
 *
 *	Releases resources allocated for the combobox.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs allocated for the combobox are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
ComboBoxStyleFreeProc(CellStyle *cellStylePtr)
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    TableView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    iconOption.clientData = viewPtr;
    Blt_FreeOptions(stylePtr->classPtr->specs, (char *)stylePtr, 
	viewPtr->display, 0);
    if (stylePtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&viewPtr->styleTable, stylePtr->hashPtr);
    } 
    Blt_DeleteHashTable(&stylePtr->table);
    if (stylePtr->selectGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->selectGC);
    }
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->disableGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->disableGC);
    }
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    Blt_Free(stylePtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStylePostProc --
 *
 *	Posts the menu associated with the designated cell.
 *
 * Results:
 *	Standard TCL result.
 *
 * Side effects:
 *	Commands may get excecuted; variables may get set; sub-menus may
 *	get posted.
 *
 *	.view filter post col
 *
 *---------------------------------------------------------------------------
 */
static int
ComboBoxStylePostProc(Tcl_Interp *interp, Cell *cellPtr,CellStyle *cellStylePtr)
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    Row *rowPtr;
    Column *colPtr;
    CellKey *keyPtr;
    const char *menuName;
    Tk_Window tkwin;
    TableView *viewPtr;

    viewPtr = cellPtr->viewPtr;
    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    rowPtr = keyPtr->rowPtr;
    if (viewPtr->postPtr != NULL) {
	return TCL_OK;		       /* Another filter's menu is currently
					* posted. */
    }
    if ((rowPtr->flags|colPtr->flags) & (DISABLED|HIDDEN)) {
	return TCL_OK;		       /* Menu is in a row or column that is
					* hidden or disabled. */
    }
    if (stylePtr->menuObjPtr == NULL) {
	return TCL_OK;			/* No menu associated with filter. */
    }
    menuName = Tcl_GetString(stylePtr->menuObjPtr);
    tkwin = Tk_NameToWindow(interp, menuName, viewPtr->tkwin);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    if (Tk_Parent(tkwin) != viewPtr->tkwin) {
	Tcl_AppendResult(interp, "can't post \"", Tk_PathName(tkwin), 
		"\": it isn't a descendant of ", Tk_PathName(viewPtr->tkwin),
		(char *)NULL);
	return TCL_ERROR;
    }
    if (stylePtr->postCmdObjPtr != NULL) {
	int result;

	Tcl_Preserve(viewPtr);
	result = Tcl_EvalObjEx(interp, stylePtr->postCmdObjPtr, 
		TCL_EVAL_GLOBAL);
	Tcl_Release(viewPtr);
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    {
	Tcl_Obj *cmdObjPtr;
	int result;
	int x1, y1, x2, y2;
	int rootX, rootY;

	Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
	x1 = SCREENX(viewPtr, colPtr->worldX) + rootX;
	x2 = x1 + colPtr->width;
	y1 = SCREENY(viewPtr, rowPtr->worldY) + rootY;
	y2 = y1 + rowPtr->height;
	cmdObjPtr = Tcl_DuplicateObj(stylePtr->menuObjPtr);
	Tcl_ListObjAppendElement(interp, cmdObjPtr, Tcl_NewStringObj("post",4));
	Tcl_ListObjAppendElement(interp, cmdObjPtr,
		Tcl_NewStringObj("right", 5));
	Tcl_ListObjAppendElement(interp, cmdObjPtr, Tcl_NewIntObj(x2));
	Tcl_ListObjAppendElement(interp, cmdObjPtr, Tcl_NewIntObj(y2));
	Tcl_ListObjAppendElement(interp, cmdObjPtr, Tcl_NewIntObj(x1));
	Tcl_ListObjAppendElement(interp, cmdObjPtr, Tcl_NewIntObj(y1));
	Tcl_IncrRefCount(cmdObjPtr);
	Tcl_Preserve(viewPtr);
	result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
	Tcl_Release(viewPtr);
	Tcl_DecrRefCount(cmdObjPtr);
	if (result == TCL_OK) {
	    viewPtr->postPtr = cellPtr;
	    Blt_SetCurrentItem(viewPtr->bindTable, viewPtr->postPtr, ITEM_CELL);
	}
	return result;
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleUnpostProc --
 *
 * Results:
 *	Standard TCL result.
 *
 * Side effects:
 *	Commands may get excecuted; variables may get set; sub-menus may
 *	get posted.
 *
 *  .view filter unpost
 *
 *---------------------------------------------------------------------------
 */
static int
ComboBoxStyleUnpostProc(Tcl_Interp *interp, Cell *cellPtr, 
			CellStyle *cellStylePtr)
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    const char *menuName;
    Tk_Window tkwin;
    Column *colPtr;
    Row *rowPtr;
    CellKey *keyPtr;
    TableView *viewPtr;

    viewPtr = cellPtr->viewPtr;
    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    rowPtr = keyPtr->rowPtr;
    if (stylePtr->menuObjPtr == NULL) {
	return TCL_OK;
    }
    viewPtr->postPtr = NULL;
    assert(((colPtr->flags|rowPtr->flags) & (HIDDEN|DISABLED)) == 0);

    menuName = Tcl_GetString(stylePtr->menuObjPtr);
    tkwin = Tk_NameToWindow(interp, menuName, viewPtr->tkwin);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    if (Tk_Parent(tkwin) != viewPtr->tkwin) {
	Tcl_AppendResult(interp, "can't unpost \"", Tk_PathName(tkwin), 
		"\": it isn't a descendant of ", 
		Tk_PathName(viewPtr->tkwin), (char *)NULL);
	return TCL_ERROR;
    }
    Blt_UnmapToplevelWindow(tkwin);
    if (Tk_IsMapped(tkwin)) {
	Tk_UnmapWindow(tkwin);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewImageBoxStyle --
 *
 *	Creates a "combobox" style.
 *
 * Results:
 *	A pointer to the new style structure.
 *
 *---------------------------------------------------------------------------
 */
static CellStyle *
NewImageBoxStyle(TableView *viewPtr, Blt_HashEntry *hPtr)
{
    ImageBoxStyle *stylePtr;

    stylePtr = Blt_AssertCalloc(1, sizeof(ImageBoxStyle));
    stylePtr->classPtr = &imageBoxStyleClass;
    stylePtr->viewPtr = viewPtr;
    stylePtr->gap = STYLE_GAP;
    stylePtr->borderWidth = 1;
    stylePtr->relief = stylePtr->activeRelief = TK_RELIEF_FLAT;
    stylePtr->name = Blt_GetHashKey(&viewPtr->styleTable, hPtr);
    stylePtr->hashPtr = hPtr;
    stylePtr->flags = SHOW_TEXT;
    stylePtr->refCount = 1;
    Blt_SetHashValue(hPtr, stylePtr);
    Blt_InitHashTable(&stylePtr->table, sizeof(CellKey)/sizeof(int));
    return (CellStyle *)stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageBoxStyleConfigureProc --
 *
 *	Configures a "imagebox" style.  This routine performs generates the
 *	GCs required for a combobox style.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs are created for the style.
 *
 *---------------------------------------------------------------------------
 */
static void
ImageBoxStyleConfigureProc(TableView *viewPtr, CellStyle *cellStylePtr)
{
    ImageBoxStyle *stylePtr = (ImageBoxStyle *)cellStylePtr;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    gcMask = GCForeground | GCFont | GCDashList | GCLineWidth | GCLineStyle;
    gcValues.dashes = 1;
    gcValues.font = Blt_Font_Id(stylePtr->font);
    gcValues.line_width = 0;
    gcValues.line_style = LineOnOffDash;

    /* Normal text. */
    gcValues.foreground = stylePtr->normalFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    stylePtr->normalGC = newGC;

    /* Disabled text. */
    gcValues.foreground = stylePtr->disableFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->disableGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->disableGC);
    }
    stylePtr->disableGC = newGC;

    /* Selected text. */
    gcValues.foreground = stylePtr->selectFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->selectGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->selectGC);
    }
    stylePtr->selectGC = newGC;

    /* Active text. */
    gcValues.foreground = stylePtr->activeFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    stylePtr->activeGC = newGC;

    /* Highlight text. */
    gcValues.foreground = stylePtr->highlightFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    if (Blt_ConfigModified(stylePtr->classPtr->specs, "-font", (char *)NULL)) {
	PropagateDirtyFlags(viewPtr, (CellStyle *)stylePtr);
    }
}

static int
ParseImageFormat(Tcl_Interp *interp, TableView *viewPtr, Cell *cellPtr, 
		 Tcl_Obj *objPtr)
{
    Tcl_Obj **objv;
    int objc;
    const char *imageName;
    Tk_Image tkImage;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((objc < 1) || (objc > 2)) {
	Tcl_AppendResult(interp, "wrong # of arguments in image result",
			 (char *)NULL);
	return TCL_ERROR;
    }
    imageName = Tcl_GetString(objv[0]);
    tkImage = Tk_GetImage(interp, viewPtr->tkwin, imageName, 
			  CellImageChangedProc, (ClientData)cellPtr);
    if (tkImage == NULL) {
	return TCL_ERROR;
    }
    if (cellPtr->tkImage != NULL) {
	Tk_FreeImage(cellPtr->tkImage);
    }
    cellPtr->tkImage = tkImage;
    if ((cellPtr->text != NULL) && (cellPtr->flags & TEXTALLOC)) {
	Blt_Free(cellPtr->text);
    }
    cellPtr->text = NULL;
    if (objc == 2) {
	cellPtr->text = Blt_Strdup(Tcl_GetString(objv[1]));
	cellPtr->flags |= TEXTALLOC;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageBoxStyleGeometryProc --
 *
 *	Determines the space requirements for the imagebox given the image,
 *	text, and icon to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The width and height fields of *cellPtr* are set with the computed
 *	dimensions.
 *
 *      +--------------+	    +--------------+
 *	||Icon| |Image||	    ||Icon| |text| |
 *	|       |text| |	    |       |Image||
 *      +--------------+	    +--------------+
 *  
 *---------------------------------------------------------------------------
 */
static void
ImageBoxStyleGeometryProc(Cell *cellPtr, CellStyle *cellStylePtr)
{
    CellKey *keyPtr;
    ImageBoxStyle *stylePtr = (ImageBoxStyle *)cellStylePtr;
    Row *rowPtr;
    Column *colPtr;
    unsigned int iw, ih, pw, ph, tw, th;
    TableView *viewPtr;
    Tcl_Interp *interp;
    Tcl_Obj *objPtr;

    viewPtr = cellPtr->viewPtr;
    keyPtr = GetKey(cellPtr);
    rowPtr = keyPtr->rowPtr;
    colPtr = keyPtr->colPtr;

    cellPtr->flags &= ~GEOMETRY;	/* Remove the dirty flag from the
					 * cell. */
    pw = ph = iw = ih = tw = th = 0;
    cellPtr->width = cellPtr->height = 2 * (stylePtr->borderWidth + FOCUS_PAD);
    cellPtr->width += 2 * CELL_PADX;
    cellPtr->height += 2 * CELL_PADY;
    cellPtr->width += colPtr->ruleWidth;
    cellPtr->height += rowPtr->ruleHeight;

    if ((cellPtr->text != NULL) && (cellPtr->flags & TEXTALLOC)) {
	Blt_Free(cellPtr->text);
    }
    cellPtr->flags &= ~TEXTALLOC;
    if (cellPtr->tkImage != NULL) {
	Tk_FreeImage(cellPtr->tkImage);
    }
    cellPtr->text = NULL;
    cellPtr->tkImage = NULL;

    interp = viewPtr->interp;
    if (colPtr->fmtCmdObjPtr != NULL) {
	int result;
	Tcl_Obj *cmdObjPtr;

	/* Invoke the format command to return the image and title text
	 * based upon the value in the cell.  */
	/* command rowIndex columnIndex */
	cmdObjPtr = Tcl_DuplicateObj(colPtr->fmtCmdObjPtr);  
	objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
	Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
	Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	Tcl_IncrRefCount(cmdObjPtr);
	result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(cmdObjPtr);
	if (result != TCL_OK) {
	    Tcl_BackgroundError(interp);
	    return;
	}
	objPtr = Tcl_GetObjResult(interp);
    } else {
	objPtr = blt_table_get_obj(viewPtr->table, rowPtr->row, colPtr->column);
    }
    if (objPtr != NULL) {
	if (ParseImageFormat(interp, viewPtr, cellPtr, objPtr) != TCL_OK) {
	    Tcl_BackgroundError(interp);
	    return;
	}
    }
    iw = ih = pw = ph = tw = th = 0;
    if (cellPtr->tkImage != NULL) {
	Tk_SizeOfImage(cellPtr->tkImage, (int *)&pw, (int *)&ph);
    }
    cellPtr->height += MAX(ph, ih);
    if (stylePtr->icon != NULL) {
	iw = IconWidth(stylePtr->icon);
	ih = IconHeight(stylePtr->icon);
    } 
    if ((stylePtr->flags & SHOW_TEXT) && (cellPtr->text != NULL)) {
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, stylePtr->font);
	Blt_Ts_GetExtents(&ts, cellPtr->text, &tw, &th);
	cellPtr->height += th;
	if (cellPtr->tkImage != NULL) {
	    cellPtr->height += stylePtr->gap;
	}
    }
    cellPtr->width += iw + MAX(pw, tw);
    if (stylePtr->icon != NULL) {
	cellPtr->width += stylePtr->gap;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageBoxStyleDrawProc --
 *
 *	Draws the "combobox" given the screen coordinates and the
 *	value to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The imagebox value is drawn.
 *
 *      +--------------+	    +--------------+
 *	||Icon| |Image||	    ||Icon| |text| |
 *	|       |text| |	    |       |Image||
 *      +--------------+	    +--------------+
 *  
 *---------------------------------------------------------------------------
 */
static void
ImageBoxStyleDrawProc(Cell *cellPtr, Drawable drawable, CellStyle *cellStylePtr,
		      int x, int y)
{
    Blt_Bg bg;
    Row *rowPtr;
    Column *colPtr;
    GC gc;
    ImageBoxStyle *stylePtr = (ImageBoxStyle *)cellStylePtr;
    int gap, colWidth, rowHeight, cellWidth, cellHeight;
    int ix, iy, px, py, tx, ty;
    unsigned int pw, ph, iw, ih, tw, th;
    int relief;
    CellKey *keyPtr;
    TableView *viewPtr;

    viewPtr = cellPtr->viewPtr;
    keyPtr = GetKey(cellPtr);
    rowPtr = keyPtr->rowPtr;
    colPtr = keyPtr->colPtr;

    relief = stylePtr->relief;
    if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & SELECTED) { 
	/* Selected */
	bg = stylePtr->selectBg;
	gc = stylePtr->selectGC;
    } else if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & DISABLED) {
	/* Disabled */
	bg = stylePtr->disableBg;
	gc = stylePtr->disableGC;
    } else if ((stylePtr->flags & EDIT) && (viewPtr->activePtr == cellPtr)) {
	/* Active */
	bg = stylePtr->activeBg;
	gc = stylePtr->activeGC;
	relief = stylePtr->activeRelief;
    } else if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & HIGHLIGHT) { 
	/* Highlighted */
	bg = GetHighlightBg((CellStyle *)stylePtr, rowPtr);
	gc = stylePtr->highlightGC;
    } else {		
	/* Normal */
	bg = stylePtr->normalBg;
	if ((stylePtr->altBg != NULL) && (rowPtr->visibleIndex & 0x1)) {
	    bg = stylePtr->altBg;
	}
	gc = stylePtr->normalGC;
    }

    rowHeight = rowPtr->height;
    colWidth  = colPtr->width;

    /* Draw background. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, colWidth,
	rowHeight, stylePtr->borderWidth, relief);

    rowHeight -= 2 * stylePtr->borderWidth + 3;
    colWidth  -= 2 * stylePtr->borderWidth + 3;
    x += stylePtr->borderWidth + 1;
    y += stylePtr->borderWidth + 1;
    /* Draw the focus ring if this cell has focus. */
    if ((viewPtr->flags & FOCUS) && (viewPtr->focusPtr == cellPtr)) {
	XDrawRectangle(viewPtr->display, drawable, gc, x, y, colWidth, 
		       rowHeight);
    }
    x += CELL_PADX;
    y += CELL_PADY;
    cellHeight = cellPtr->height - 2 * (stylePtr->borderWidth + CELL_PADY) - 3;
    cellWidth = cellPtr->width - 2 * (stylePtr->borderWidth + CELL_PADX) - 3;

    /* Justify (x) and center (y) the contents of the cell. */
    if (rowHeight > cellHeight) {
	y += (rowHeight - cellHeight) / 2;
    }
    if (colWidth > cellWidth) {
	switch(stylePtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    x += (colWidth - cellWidth);
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (colWidth - cellWidth) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }
    ix = iy = px = py = tx = ty = 0;
    pw = ph = tw = th = iw = ih = 0;
    if (stylePtr->icon != NULL) {
	iw = IconWidth(stylePtr->icon);
	ih = IconHeight(stylePtr->icon);
    }
    gap = 0;
    if (cellPtr->tkImage != NULL) {
	Tk_SizeOfImage(cellPtr->tkImage, (int *)&pw, (int *)&ph);
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    if ((stylePtr->icon != NULL) && (cellPtr->text != NULL)) {
	gap = stylePtr->gap;
    }
    ix = x + gap;
    iy = y + (cellHeight - ih) / 2;
    px = ix + iw + gap;
    py = y;
    if (stylePtr->icon != NULL) {
	Tk_RedrawImage(IconBits(stylePtr->icon), 0, 0, iw, ih,drawable, ix, iy);
	x += iw + gap;
    }
    if (cellPtr->tkImage != NULL) {
	Tk_RedrawImage(cellPtr->tkImage, 0, 0, pw, ph, drawable, px, py);
    }
    if ((stylePtr->flags & SHOW_TEXT) && (cellPtr->text != NULL)) {
	TextStyle ts;
	int xMax;
	TextLayout *textPtr;

	ty = y + ph + gap;
	tx = x;
	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, stylePtr->font);
	Blt_Ts_SetGC(ts, gc);
	xMax = colWidth - iw - gap;
	Blt_Ts_SetMaxLength(ts, xMax);
	textPtr = Blt_Ts_CreateLayout(cellPtr->text, -1, &ts);
	Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, textPtr, &ts, tx, ty);
	if (viewPtr->activePtr == cellPtr) {
	    Blt_Ts_UnderlineLayout(viewPtr->tkwin, drawable, textPtr,&ts,tx,ty);
	}
	Blt_Free(textPtr);
    }
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * ImageBoxStyleIdentifyProc --
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *---------------------------------------------------------------------------
 */
static int
ImageBoxStyleIdentifyProc(TableView *viewPtr, Cell *cellPtr, 
			  CellStyle *cellStylePtr, int x, int y)
{
    ImageBoxStyle *stylePtr = (ImageBoxStyle *)cellStylePtr;

    /* Pick the image, image title, and icon. */
    if ((x >= 0) && (x < width)) && (y >= 0) && (y < height)) {
	return TRUE;
    }
    return FALSE;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * ImageBoxStyleFreeProc --
 *
 *	Releases resources allocated for the imagebox. 
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs allocated for the combobox are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
ImageBoxStyleFreeProc(CellStyle *cellStylePtr)
{
    ImageBoxStyle *stylePtr = (ImageBoxStyle *)cellStylePtr;
    TableView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    iconOption.clientData = viewPtr;
    Blt_FreeOptions(stylePtr->classPtr->specs, (char *)stylePtr, 
	viewPtr->display, 0);
    if (stylePtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&viewPtr->styleTable, stylePtr->hashPtr);
    } 
    Blt_DeleteHashTable(&stylePtr->table);
    if (stylePtr->selectGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->selectGC);
    }
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->disableGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->disableGC);
    }
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    Blt_Free(stylePtr);
}

#ifdef notdef
static int
GetStyle(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, 
	 CellStyle **stylePtrPtr)
{
    Blt_HashEntry *hPtr;
    const char *styleName;

    styleName = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&viewPtr->styleTable, styleName);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find cell style \"", styleName, 
		"\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    *stylePtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}
#endif

CellStyle *
Blt_TableView_CreateCellStyle(
     Tcl_Interp *interp,
     TableView *viewPtr,		/* Blt_TableView_ widget. */
     int type,				/* Type of style: either
					 * STYLE_TEXTBOX,
					 * STYLE_COMBOBOX, or
					 * STYLE_CHECKBOX */
     const char *styleName)		/* Name of the new style. */
{    
    CellStyle *stylePtr;
    Blt_HashEntry *hPtr;
    int isNew;
    
    hPtr = Blt_CreateHashEntry(&viewPtr->styleTable, styleName, &isNew);
    if (!isNew) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "cell style \"", styleName, 
		"\" already exists", (char *)NULL);
	}
	return NULL;
    }
    /* Create the new marker based upon the given type */
    switch (type) {
    case STYLE_TEXTBOX:
	stylePtr = NewTextBoxStyle(viewPtr, hPtr);	break;
    case STYLE_COMBOBOX:
	stylePtr = NewComboBoxStyle(viewPtr, hPtr);	break;
    case STYLE_CHECKBOX:
	stylePtr = NewCheckBoxStyle(viewPtr, hPtr);	break;
    case STYLE_IMAGEBOX:
	stylePtr = NewImageBoxStyle(viewPtr, hPtr);	break;
    default:
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "unknown style type", (char *)NULL);
	}
	return NULL;
    }
    iconOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(interp, viewPtr->tkwin, styleName, 
	stylePtr->classPtr->className, stylePtr->classPtr->specs, 0, 
		(Tcl_Obj **)NULL, (char *)stylePtr, 0) != TCL_OK) {
	(*stylePtr->classPtr->freeProc)(stylePtr);
	return NULL;
    }
    return stylePtr;
}

#endif /* NO_TABLEVIEW */

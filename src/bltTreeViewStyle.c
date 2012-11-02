
/*
 * bltTreeViewStyle.c --
 *
 * This module implements styles for treeview widget cells.
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

#ifndef NO_TREEVIEW

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "bltAlloc.h"
#include "bltList.h"
#include "bltPicture.h"
#include "bltBg.h"
#include "bltPainter.h"
#include "bltTreeView.h"
#include "bltOp.h"

#define STYLE_GAP		2
#define ARROW_WIDTH		13

static Blt_OptionParseProc ObjToIconProc;
static Blt_OptionPrintProc IconToObjProc;
static Blt_OptionFreeProc FreeIconProc;
static Blt_CustomOption iconOption = {
    ObjToIconProc, IconToObjProc, FreeIconProc, NULL,
};

#define DEF_STYLE_HIGHLIGHT_BACKGROUND	STD_NORMAL_BACKGROUND
#define DEF_STYLE_HIGHLIGHT_FOREGROUND	STD_NORMAL_FOREGROUND
#ifdef WIN32
#define DEF_STYLE_ACTIVE_BACKGROUND	RGB_GREY85
#else
#define DEF_STYLE_ACTIVE_BACKGROUND	RGB_GREY95
#endif
#define DEF_STYLE_ACTIVE_FOREGROUND 	STD_ACTIVE_FOREGROUND
#define DEF_STYLE_GAP			"2"

typedef struct {
    int refCount;			/* Usage reference count.  A reference
					 * count of zero indicates that the
					 * style may be freed. */
    unsigned int flags;			/* Bit field containing both the style
					 * type and various flags. */
    TreeView *viewPtr;			
    const char *name;			/* Instance name. */
    ColumnStyleClass *classPtr;		/* Contains class-specific information
					 * such as configuration
					 * specifications and * configure,
					 * draw, etc. routines. */
    Blt_HashEntry *hashPtr;		/* If non-NULL, points to the hash
					 * table entry for the style.  A style
					 * that's been deleted, but still in
					 * use (non-zero reference count)
					 * will have no hash table entry. */
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
    XColor *fgColor;			/* Normal foreground color of cell. */
    XColor *highlightFgColor;		/* Foreground color of cell when
					 * highlighted. */
    XColor *activeFgColor;		/* Foreground color of cell when
					 * active. */
    XColor *selFgColor;			/* Foreground color of a selected
					 * cell. If non-NULL, overrides default
					 * foreground color specification. */
    Blt_Bg bg;				/* Normal background color of cell. */
    Blt_Bg highlightBg;			/* Background color of cell when
					 * highlighted. */
    Blt_Bg activeBg;			/* Background color of cell when
					 * active. */
    Blt_Bg selBg;			/* Background color of a selected
					 * cell.  If non-NULL, overrides the
					 * default background color
					 * specification. */
    Tcl_Obj *validateCmdObjPtr;

    GC gc;
    GC highlightGC;
    GC activeGC;
    Blt_TreeKey key;			/* Actual data resides in this tree
					   value. */
    Tcl_Obj *cmdObjPtr;

    /* TextBox-specific fields */
    int side;				/* Position of the text in relation to
					 * the icon.  */
} TextBoxStyle;

#ifdef WIN32
#define DEF_TEXTBOX_CURSOR		"arrow"
#else
#define DEF_TEXTBOX_CURSOR		"hand2"
#endif /*WIN32*/
#define DEF_TEXTBOX_SIDE		"left"
#define DEF_TEXTBOX_VALIDATE_COMMAND	(char *)NULL
#define DEF_TEXTBOX_COMMAND		(char *)NULL

static Blt_ConfigSpec textBoxSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_STYLE_ACTIVE_BACKGROUND, 
	Blt_Offset(TextBoxStyle, activeBg), 0},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeFackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
	"ActiveForeground", DEF_STYLE_ACTIVE_FOREGROUND, 
	Blt_Offset(TextBoxStyle, activeFgColor), 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	(char *)NULL, Blt_Offset(TextBoxStyle, bg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", DEF_TEXTBOX_COMMAND, 
	Blt_Offset(TextBoxStyle, cmdObjPtr), 0},
    {BLT_CONFIG_CURSOR, "-cursor", "cursor", "Cursor", DEF_TEXTBOX_CURSOR, 
	Blt_Offset(TextBoxStyle, cursor), 0},
    {BLT_CONFIG_BITMASK, "-edit", "edit", "Edit", (char *)NULL, 
	Blt_Offset(TextBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT,
	(Blt_CustomOption *)STYLE_EDITABLE},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", (char *)NULL, 
	Blt_Offset(TextBoxStyle, font), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", (char *)NULL,
	Blt_Offset(TextBoxStyle, fgColor),BLT_CONFIG_NULL_OK },
    {BLT_CONFIG_PIXELS_NNEG, "-gap", "gap", "Gap", DEF_STYLE_GAP, 
	Blt_Offset(TextBoxStyle, gap), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_STYLE_HIGHLIGHT_BACKGROUND, 
        Blt_Offset(TextBoxStyle, highlightBg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", 
	"HighlightForeground", DEF_STYLE_HIGHLIGHT_FOREGROUND, 
	Blt_Offset(TextBoxStyle, highlightFgColor), 0},
    {BLT_CONFIG_SYNONYM, "-highlightbg", "highlightBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-highlightfg", "highlightForeground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", (char *)NULL, 
	Blt_Offset(TextBoxStyle, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_STRING, "-key", "key", "key", 	(char *)NULL, 
	Blt_Offset(TextBoxStyle, key), BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
	"Foreground", (char *)NULL, Blt_Offset(TextBoxStyle, selBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
	(char *)NULL, Blt_Offset(TextBoxStyle, selFgColor), 0},
    {BLT_CONFIG_SIDE, "-side", "side", "side", DEF_TEXTBOX_SIDE, 
	Blt_Offset(TextBoxStyle, side), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-validatecommand", "validateCommand", 
	"ValidateCommand", DEF_TEXTBOX_VALIDATE_COMMAND, 
	Blt_Offset(TextBoxStyle, validateCmdObjPtr), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0}
};


typedef struct {
    int refCount;			/* Usage reference count.  A reference
					 * count of zero indicates that the
					 * style may be freed. */
    unsigned int flags;			/* Bit field containing both the style
					 * type and various flags. */
    TreeView *viewPtr;			
    const char *name;			/* Instance name. */
    ColumnStyleClass *classPtr;		/* Contains class-specific information
					 * such as configuration
					 * specifications and * configure,
					 * draw, etc. routines. */
    Blt_HashEntry *hashPtr;		/* If non-NULL, points to the hash
					 * table entry for the style.  A style
					 * that's been deleted, but still in
					 * use (non-zero reference count)
					 * will have no hash table entry. */
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
    XColor *fgColor;			/* Normal foreground color of cell. */
    XColor *highlightFgColor;		/* Foreground color of cell when
					 * highlighted. */
    XColor *activeFgColor;		/* Foreground color of cell when
					 * active. */
    XColor *selFgColor;			/* Foreground color of a selected
					 * cell. If non-NULL, overrides
					 * default foreground color
					 * specification. */

    Blt_Bg bg;				/* Normal background color of cell. */
    Blt_Bg highlightBg;			/* Background color of cell when
					 * highlighted. */
    Blt_Bg activeBg;			/* Background color of cell when
					 * active. */
    Blt_Bg selBg;			/* Background color of a selected
					 * cell.  If non-NULL, overrides the
					 * default background color
					 * specification. */
    Tcl_Obj *validateCmdObjPtr;
    GC gc;
    GC highlightGC;
    GC activeGC;
    Blt_TreeKey key;			/* Actual data resides in this tree
					   value. */

    Tcl_Obj *cmdObjPtr;			/* If non-NULL, command to be invoked
					 * when check is clicked. */
    /* Checkbox specific fields. */
    int size;				/* Size of the checkbox. */
    int showValue;			/* If non-zero, display the on/off
					 * value.  */
    const char *onValue;
    const char *offValue;
    int lineWidth;			/* Linewidth of the surrounding
					 * box. */
    XColor *boxColor;			/* Rectangle (box) color (grey). */
    XColor *fillColor;			/* Fill color (white) */
    XColor *checkColor;			/* Check color (red). */

    TextLayout *onPtr, *offPtr;
    
    Blt_Painter painter;
    Blt_Picture selectedPicture;
    Blt_Picture normalPicture;
    Blt_Picture disabledPicture;
} CheckBoxStyle;

#define DEF_CHECKBOX_BOX_COLOR		(char *)NULL
#define DEF_CHECKBOX_CHECK_COLOR	"red"
#define DEF_CHECKBOX_COMMAND		(char *)NULL
#define DEF_CHECKBOX_FILL_COLOR		(char *)NULL
#define DEF_CHECKBOX_OFFVALUE		"0"
#define DEF_CHECKBOX_ONVALUE		"1"
#define DEF_CHECKBOX_SHOWVALUE		"yes"
#define DEF_CHECKBOX_SIZE		"11"
#define DEF_CHECKBOX_LINEWIDTH		"2"
#define DEF_CHECKBOX_GAP		"4"
#ifdef WIN32
#define DEF_CHECKBOX_CURSOR		"arrow"
#else
#define DEF_CHECKBOX_CURSOR		"hand2"
#endif /*WIN32*/

static Blt_ConfigSpec checkBoxSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_STYLE_ACTIVE_BACKGROUND, 
	Blt_Offset(CheckBoxStyle, activeBg), 0},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeFackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
	"ActiveForeground", DEF_STYLE_ACTIVE_FOREGROUND, 
	Blt_Offset(CheckBoxStyle, activeFgColor), 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	(char *)NULL, Blt_Offset(CheckBoxStyle, bg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_PIXELS_POS, "-boxsize", "boxSize", "BoxSize", DEF_CHECKBOX_SIZE,
	Blt_Offset(CheckBoxStyle, size), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", DEF_CHECKBOX_COMMAND, 
        Blt_Offset(CheckBoxStyle, cmdObjPtr), 0},
    {BLT_CONFIG_CURSOR, "-cursor", "cursor", "Cursor", DEF_CHECKBOX_CURSOR, 
	Blt_Offset(CheckBoxStyle, cursor), 0},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", (char *)NULL, 
	Blt_Offset(CheckBoxStyle, font), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", (char *)NULL,
	Blt_Offset(CheckBoxStyle, fgColor), BLT_CONFIG_NULL_OK },
    {BLT_CONFIG_PIXELS_NNEG, "-gap", "gap", "Gap", DEF_CHECKBOX_GAP, 
	Blt_Offset(CheckBoxStyle, gap), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_STYLE_HIGHLIGHT_BACKGROUND, 
        Blt_Offset(CheckBoxStyle, highlightBg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", 
	"HighlightForeground", DEF_STYLE_HIGHLIGHT_FOREGROUND, 
	 Blt_Offset(CheckBoxStyle, highlightFgColor), 0},
    {BLT_CONFIG_SYNONYM, "-highlightbg", "highlightBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-highlightfg", "highlightForeground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", (char *)NULL, 
	Blt_Offset(CheckBoxStyle, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_STRING, "-key", "key", "key", (char *)NULL, 
	Blt_Offset(CheckBoxStyle, key), BLT_CONFIG_NULL_OK, 0},
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
    {BLT_CONFIG_STRING, "-offvalue", "offValue", "OffValue", 
	DEF_CHECKBOX_OFFVALUE, Blt_Offset(CheckBoxStyle, offValue), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-onvalue", "onValue", "OnValue", DEF_CHECKBOX_ONVALUE,
	Blt_Offset(CheckBoxStyle, onValue), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-key", "key", "key", (char *)NULL, 
	Blt_Offset(CheckBoxStyle, key), BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
	"Foreground", (char *)NULL, Blt_Offset(CheckBoxStyle, selBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
	(char *)NULL, Blt_Offset(CheckBoxStyle, selFgColor), 0},
    {BLT_CONFIG_BOOLEAN, "-showvalue", "showValue", "ShowValue",
	DEF_CHECKBOX_SHOWVALUE, Blt_Offset(CheckBoxStyle, showValue), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

typedef struct {
    int refCount;			/* Usage reference count.  A reference
					 * count of zero indicates that the
					 * style may be freed. */
    unsigned int flags;			/* Bit field containing both the style
					 * type and various flags. */
    TreeView *viewPtr;			
    const char *name;			/* Instance name. */
    ColumnStyleClass *classPtr;		/* Contains class-specific information
					 * such as configuration
					 * specifications and * configure,
					 * draw, etc. routines. */
    Blt_HashEntry *hashPtr;		/* If non-NULL, points to the hash
					 * table entry for the style.  A style
					 * that's been deleted, but still in
					 * use (non-zero reference count)
					 * will have no hash table entry. */
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
    XColor *fgColor;			/* Normal foreground color of cell. */
    XColor *highlightFgColor;		/* Foreground color of cell when
					 * highlighted. */
    XColor *activeFgColor;		/* Foreground color of cell when
					 * active. */
    XColor *selFgColor;			/* Foreground color of a selected
					 * cell. If non-NULL, overrides
					 * default foreground color
					 * specification. */
    Blt_Bg bg;				/* Normal background color of cell. */
    Blt_Bg highlightBg;			/* Background color of cell when
					 * highlighted. */
    Blt_Bg activeBg;			/* Background color of cell when
					 * active. */

    Blt_Bg selBg;			/* Background color of a selected
					 * cell.  If non-NULL, overrides the
					 * default background color
					 * specification. */
    Tcl_Obj *validateCmdObjPtr;
    GC gc;
    GC highlightGC;
    GC activeGC;
    Blt_TreeKey key;			/* Actual data resides in this tree
					   value. */

    Tcl_Obj *cmdObjPtr;

    /* ComboBox-specific fields */

    int borderWidth;			/* Width of outer border surrounding
					 * the entire box. */
    int relief;				/* Relief of outer border. */


    const char *choices;		/* List of available choices. */
    const char *choiceIcons;		/* List of icons associated with
					 * choices. */
    int scrollWidth;
    int arrow;
    int arrowWidth;
    int arrowBW;			/* Border width of arrow. */
    int arrowRelief;			/* Normal relief of arrow. */
} ComboBoxStyle;

#define DEF_COMBOBOX_BORDERWIDTH	"1"
#define DEF_COMBOBOX_ARROW_BORDERWIDTH	"1"
#define DEF_COMBOBOX_ARROW_RELIEF	"raised"
#define DEF_COMBOBOX_RELIEF		"flat"
#ifdef WIN32
#define DEF_COMBOBOX_CURSOR		"arrow"
#else
#define DEF_COMBOBOX_CURSOR		"hand2"
#endif /*WIN32*/

static Blt_ConfigSpec comboBoxSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_STYLE_ACTIVE_BACKGROUND, 
	Blt_Offset(ComboBoxStyle, activeBg), 0},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeFackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
	"ActiveForeground", DEF_STYLE_ACTIVE_FOREGROUND, 
	Blt_Offset(ComboBoxStyle, activeFgColor), 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	(char *)NULL, Blt_Offset(ComboBoxStyle, bg), BLT_CONFIG_NULL_OK},
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
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", (char *)NULL, 
	Blt_Offset(ComboBoxStyle, font), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
	(char *)NULL, Blt_Offset(ComboBoxStyle, fgColor), BLT_CONFIG_NULL_OK },
    {BLT_CONFIG_PIXELS_NNEG, "-gap", "gap", "Gap", DEF_STYLE_GAP, 
	Blt_Offset(ComboBoxStyle, gap), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_STYLE_HIGHLIGHT_BACKGROUND, 
        Blt_Offset(ComboBoxStyle, highlightBg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", 
	"HighlightForeground", DEF_STYLE_HIGHLIGHT_FOREGROUND, 
	 Blt_Offset(ComboBoxStyle, highlightFgColor), 0},
    {BLT_CONFIG_SYNONYM, "-highlightbg", "highlightBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-highlightfg", "highlightForeground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", (char *)NULL, 
	Blt_Offset(ComboBoxStyle, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_STRING, "-key", "key", "key", (char *)NULL, 
	Blt_Offset(ComboBoxStyle, key), BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_COMBOBOX_RELIEF, 
	Blt_Offset(ComboBoxStyle, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
	"Foreground", (char *)NULL, Blt_Offset(ComboBoxStyle, selBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
	(char *)NULL, Blt_Offset(ComboBoxStyle, selFgColor), 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

static ColumnStyleConfigureProc CheckBoxStyleConfigureProc;
static ColumnStyleConfigureProc ComboBoxStyleConfigureProc;
static ColumnStyleConfigureProc TextBoxStyleConfigureProc;
static ColumnStyleDrawProc CheckBoxStyleDrawProc;
static ColumnStyleDrawProc ComboBoxStyleDrawProc;
static ColumnStyleDrawProc TextBoxStyleDrawProc;
static ColumnStyleEditProc CheckBoxStyleEditProc;
static ColumnStyleEditProc ComboBoxStyleEditProc;
static ColumnStyleEditProc TextBoxStyleEditProc;
static ColumnStyleFreeProc CheckBoxStyleFreeProc;
static ColumnStyleFreeProc ComboBoxStyleFreeProc;
static ColumnStyleFreeProc TextBoxStyleFreeProc;
static ColumnStyleGeometryProc CheckBoxStyleGeometryProc;
static ColumnStyleGeometryProc ComboBoxStyleGeometryProc;
static ColumnStyleGeometryProc TextBoxStyleGeometryProc;

static int
EntryIsSelected(TreeView *viewPtr, Entry *entryPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&viewPtr->selection.table, (char *)entryPtr);
    return (hPtr != NULL);
}

static INLINE XColor *
GetStyleForeground(Column *colPtr)
{
    ColumnStyle *stylePtr;

    stylePtr = colPtr->stylePtr;
    if ((stylePtr != NULL) && (stylePtr->fgColor != NULL)) {
	return stylePtr->fgColor;
    }
    return colPtr->viewPtr->fgColor;
}

/*ARGSUSED*/
static void
FreeIconProc(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Icon *iconPtr = (Icon *)(widgRec + offset);
    TreeView *viewPtr = clientData;

    if (*iconPtr != NULL) {
	Blt_TreeView_FreeIcon(viewPtr, *iconPtr);
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
ObjToIconProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to report results */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* Tcl_Obj representing the value. */
    char *widgRec,
    int offset,				/* Offset to field in structure */
    int flags)	
{
    ColumnStyle *stylePtr = (char *)widgRec;
    TreeView *viewPtr;
    Icon *iconPtr = (Icon *)(widgRec + offset);
    Icon icon;
    int length;
    const char *string;

    viewPtr = stylePtr->viewPtr;
    string = Tcl_GetStringFromObj(objPtr, &length);
    icon = NULL;
    if (length > 0) {
	icon = Blt_TreeView_GetIcon(viewPtr, string);
	if (icon == NULL) {
	    return TCL_ERROR;
	}
    }
    if (*iconPtr != NULL) {
	Blt_TreeView_FreeIcon(viewPtr, *iconPtr);
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
IconToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Icon icon = *(Icon *)(widgRec + offset);

    if (icon == NULL) {
	return Tcl_NewStringObj("", -1);
    } else {
	return Tcl_NewStringObj(Blt_Image_Name((icon)->tkImage), -1);
    }
}


static ColumnStyleClass textBoxClass = {
    "textbox",
    "TextBoxStyle",
    textBoxSpecs,
    TextBoxStyleConfigureProc,
    TextBoxStyleGeometryProc,
    TextBoxStyleDrawProc,
    NULL,				/* identProc */
    TextBoxStyleEditProc,
    TextBoxStyleFreeProc,
    NULL,				/* postProc */
    NULL				/* unpostProc */
};

static ColumnStyleClass checkBoxClass = {
    "checkbox",
    "CheckBoxStyle",
    checkBoxSpecs,
    CheckBoxStyleConfigureProc,
    CheckBoxStyleGeometryProc,
    CheckBoxStyleDrawProc,
    NULL,				/* identProc */
    CheckBoxStyleEditProc,
    CheckBoxStyleFreeProc,
    NULL,				/* postProc */
    NULL				/* unpostProc */
};

static ColumnStyleClass comboBoxClass = {
    "combobox", 
    "ComboBoxStyle", 
    comboBoxSpecs,
    ComboBoxStyleConfigureProc,
    ComboBoxStyleGeometryProc,
    ComboBoxStyleDrawProc,
    NULL,				/* identProc */
    ComboBoxStyleEditProc,
    ComboBoxStyleFreeProc,
    NULL,				/* postProc */
    NULL				/* unpostProc */
};

/*
 *---------------------------------------------------------------------------
 *
 * Blt_TreeView_CreateTextBoxStyle --
 *
 *	Creates a "textbox" style.
 *
 * Results:
 *	A pointer to the new style structure.
 *
 *---------------------------------------------------------------------------
 */
ColumnStyle *
Blt_TreeView_CreateTextBoxStyle(TreeView *viewPtr, Blt_HashEntry *hPtr)
{
    TextBoxStyle *stylePtr;

    stylePtr = Blt_AssertCalloc(1, sizeof(TextBoxStyle));
    stylePtr->classPtr = &textBoxClass;
    stylePtr->side = SIDE_LEFT;
    stylePtr->gap = STYLE_GAP;
    stylePtr->name = Blt_GetHashKey(&viewPtr->styleTable, hPtr);
    stylePtr->hashPtr = hPtr;
    stylePtr->link = NULL;
    stylePtr->flags = STYLE_TEXTBOX;
    stylePtr->refCount = 1;
    stylePtr->viewPtr = viewPtr;
    Blt_SetHashValue(hPtr, stylePtr);
    return (ColumnStyle *)stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextBoxStyleConfigureProc --
 *
 *	Configures a "textbox" style.  This routine performs generates the GCs
 *	required for a textbox style.
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
TextBoxStyleConfigureProc(ColumnStyle *colStylePtr)
{
    GC newGC;
    TextBoxStyle *stylePtr = (TextBoxStyle *)colStylePtr;
    TreeView *viewPtr;
    XGCValues gcValues;
    unsigned long gcMask;

    viewPtr = stylePtr->viewPtr;
    gcMask = GCForeground | GCFont;
    gcValues.font = Blt_Font_Id(CHOOSE(viewPtr->font, stylePtr->font));

    /* Normal GC */
    gcValues.foreground = CHOOSE(viewPtr->fgColor, stylePtr->fgColor)->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->gc != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->gc);
    }
    stylePtr->gc = newGC;

    /* Highlight GC  */
    gcValues.foreground = stylePtr->highlightFgColor->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    /* Active GC  */
    gcValues.foreground = stylePtr->activeFgColor->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    stylePtr->activeGC = newGC;
    stylePtr->flags |= STYLE_DIRTY;
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
 *	The width and height fields of *valuePtr* are set with the computed
 *	dimensions.
 *
 *---------------------------------------------------------------------------
 */
static void
TextBoxStyleGeometryProc(ColumnStyle *colStylePtr, Value *valuePtr)
{
    TextBoxStyle *stylePtr = (TextBoxStyle *)colStylePtr;
    TreeView *viewPtr;
    int gap;
    int iconWidth, iconHeight;
    int textWidth, textHeight;

    viewPtr = stylePtr->viewPtr;
    textWidth = textHeight = 0;
    iconWidth = iconHeight = 0;
    valuePtr->width = valuePtr->height = 0;

    if (stylePtr->icon != NULL) {
	iconWidth = TreeView_IconWidth(stylePtr->icon);
	iconHeight = TreeView_IconHeight(stylePtr->icon);
    } 
    if (valuePtr->textPtr != NULL) {
	Blt_Free(valuePtr->textPtr);
	valuePtr->textPtr = NULL;
    }
    if (valuePtr->fmtString != NULL) {	/* New string defined. */
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
	valuePtr->textPtr = Blt_Ts_CreateLayout(valuePtr->fmtString, -1, &ts);
    } 
    gap = 0;
    if (valuePtr->textPtr != NULL) {
	textWidth = valuePtr->textPtr->width;
	textHeight = valuePtr->textPtr->height;
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    if (stylePtr->side & (SIDE_TOP | SIDE_BOTTOM)) {
	valuePtr->width = MAX(textWidth, iconWidth);
	valuePtr->height = iconHeight + gap + textHeight;
    } else {
	valuePtr->width = iconWidth + gap + textWidth;
	valuePtr->height = MAX(textHeight, iconHeight);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TextBoxStyleDrawProc --
 *
 *	Draws the "textbox" given the screen coordinates and the value to be
 *	displayed.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The textbox value is drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
TextBoxStyleDrawProc(Entry *entryPtr, Value *valuePtr, Drawable drawable, 
		     ColumnStyle *colStylePtr, int x, int y)
{
    Blt_Bg bg;
    Column *colPtr;
    TextBoxStyle *stylePtr = (TextBoxStyle *)colStylePtr;
    TreeView *viewPtr;
    XColor *fgColor;
    int gap, columnWidth;
    int iconX, iconY, iconWidth, iconHeight;
    int textX, textY, textWidth, textHeight;

    viewPtr = stylePtr->viewPtr;
    colPtr = valuePtr->columnPtr;

    if (stylePtr->flags & STYLE_HIGHLIGHT) {
	bg = stylePtr->highlightBg;
	fgColor = stylePtr->highlightFgColor;
    } else {
	if (stylePtr->bg != NULL) {
	    bg = stylePtr->bg;
	} else if ((viewPtr->altBg != NULL) && (entryPtr->flatIndex & 0x1)) {
	    bg = viewPtr->altBg;
	} else {
	    bg = viewPtr->bg;
	}
	fgColor = CHOOSE(viewPtr->fgColor, stylePtr->fgColor);
    }
    if (EntryIsSelected(viewPtr, entryPtr)) {
	bg = (stylePtr->selBg != NULL) ? 
	    stylePtr->selBg : viewPtr->selection.bg;
    } 
    /*
     * Draw the active or normal background color over the entire label area.
     * This includes both the tab's text and image.  The rectangle should be 2
     * pixels wider/taller than this area. So if the label consists of just an
     * image, we get an halo around the image when the tab is active.
     */
    if (bg != NULL) {
	Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, 
		colPtr->width, entryPtr->height, 0, TK_RELIEF_FLAT);
    }

    columnWidth = colPtr->width - 
	(2 * colPtr->borderWidth + PADDING(colPtr->pad));
    if (columnWidth > valuePtr->width) {
	switch(colPtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    x += (columnWidth - valuePtr->width);
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (columnWidth - valuePtr->width) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }

    textX = textY = iconX = iconY = 0;	/* Suppress compiler warning. */
    
    iconWidth = iconHeight = 0;
    if (stylePtr->icon != NULL) {
	iconWidth = TreeView_IconWidth(stylePtr->icon);
	iconHeight = TreeView_IconHeight(stylePtr->icon);
    }
    textWidth = textHeight = 0;
    if (valuePtr->textPtr != NULL) {
	textWidth = valuePtr->textPtr->width;
	textHeight = valuePtr->textPtr->height;
    }
    gap = 0;
    if ((stylePtr->icon != NULL) && (valuePtr->textPtr != NULL)) {
	gap = stylePtr->gap;
    }
    switch (stylePtr->side) {
    case SIDE_RIGHT:
	textX = x;
	textY = y + (entryPtr->height - textHeight) / 2;
	iconX = textX + textWidth + gap;
	iconY = y + (entryPtr->height - iconHeight) / 2;
	break;
    case SIDE_LEFT:
	iconX = x;
	iconY = y + (entryPtr->height - iconHeight) / 2;
	textX = iconX + iconWidth + gap;
	textY = y + (entryPtr->height - textHeight) / 2;
	break;
    case SIDE_TOP:
	iconY = y;
	iconX = x + (columnWidth - iconWidth) / 2;
	textY = iconY + iconHeight + gap;
	textX = x + (columnWidth - textWidth) / 2;
	break;
    case SIDE_BOTTOM:
	textY = y;
	textX = x + (columnWidth - textWidth) / 2;
	iconY = textY + textHeight + gap;
	iconX = x + (columnWidth - iconWidth) / 2;
	break;
    }
    if (stylePtr->icon != NULL) {
	Tk_RedrawImage(TreeView_IconBits(stylePtr->icon), 0, 0, iconWidth, 
		       iconHeight, drawable, iconX, iconY);
    }
    if (valuePtr->textPtr != NULL) {
	TextStyle ts;
	XColor *color;
	Blt_Font font;
	int xMax;

	font = CHOOSE(viewPtr->font, stylePtr->font);
	if (EntryIsSelected(viewPtr, entryPtr)) {
	    if (stylePtr->selFgColor != NULL) {
		color = stylePtr->selFgColor;
	    } else {
		color = viewPtr->selection.fgColor;
	    }
	} else if (entryPtr->color != NULL) {
	    color = entryPtr->color;
	} else {
	    color = fgColor;
	}
	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, font);
	Blt_Ts_SetForeground(ts, color);
	xMax = colPtr->width - colPtr->titleBW - colPtr->pad.side2;
	Blt_Ts_SetMaxLength(ts, xMax);
	Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, valuePtr->textPtr, &ts, 
		textX, textY);
    }
    stylePtr->flags &= ~STYLE_DIRTY;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextBoxStyleEditProc --
 *
 *	Edits the "textbox".
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TextBoxStyleEditProc(Entry *entryPtr, Column *colPtr, ColumnStyle *colStylePtr)
{
    TreeView *viewPtr;

    viewPtr = colStylePtr->viewPtr;
    return Blt_TreeView_CreateTextbox(viewPtr, entryPtr, colPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * TextBoxStyleFreeProc --
 *
 *	Releases resources allocated for the textbox. The resources freed by
 *	this routine are specific only to the "textbox".  Other resources
 *	(common to all styles) are freed in the Blt_TreeView_FreeStyle
 *	routine.
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
TextBoxStyleFreeProc(ColumnStyle *colStylePtr)
{
    TextBoxStyle *stylePtr = (TextBoxStyle *)colStylePtr;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->gc != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->gc);
    }
    if (stylePtr->icon != NULL) {
	Blt_TreeView_FreeIcon(viewPtr, stylePtr->icon);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_TreeView_CreateCheckBoxStyle --
 *
 *	Creates a "checkbox" style.
 *
 * Results:
 *	A pointer to the new style structure.
 *
 *---------------------------------------------------------------------------
 */
ColumnStyle *
Blt_TreeView_CreateCheckBoxStyle(TreeView *viewPtr, Blt_HashEntry *hPtr)
{
    CheckBoxStyle *stylePtr;

    stylePtr = Blt_AssertCalloc(1, sizeof(CheckBoxStyle));
    stylePtr->classPtr = &checkBoxClass;
    stylePtr->gap = 4;
    stylePtr->size = 15;
    stylePtr->lineWidth = 2;
    stylePtr->showValue = TRUE;
    stylePtr->name = Blt_GetHashKey(&viewPtr->styleTable, hPtr);
    stylePtr->hashPtr = hPtr;
    stylePtr->link = NULL;
    stylePtr->flags = STYLE_CHECKBOX;
    stylePtr->refCount = 1;
    stylePtr->viewPtr = viewPtr;
    Blt_SetHashValue(hPtr, stylePtr);
    return (ColumnStyle *)stylePtr;
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
CheckBoxStyleConfigureProc(ColumnStyle *colStylePtr)
{
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)colStylePtr;
    GC newGC;
    TreeView *viewPtr;
    XColor *bgColor;
    XGCValues gcValues;
    unsigned long gcMask;

    viewPtr = stylePtr->viewPtr;
    gcMask = GCForeground | GCBackground | GCFont;
    gcValues.font = Blt_Font_Id(CHOOSE(viewPtr->font, stylePtr->font));
    bgColor = Blt_Bg_BorderColor(CHOOSE(viewPtr->bg, stylePtr->bg));

    gcValues.background = bgColor->pixel;
    gcValues.foreground = CHOOSE(viewPtr->fgColor, stylePtr->fgColor)->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->gc != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->gc);
    }
    stylePtr->gc = newGC;
    gcValues.background = Blt_Bg_BorderColor(stylePtr->highlightBg)->pixel;
    gcValues.foreground = stylePtr->highlightFgColor->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    gcValues.background = Blt_Bg_BorderColor(stylePtr->activeBg)->pixel;
    gcValues.foreground = stylePtr->activeFgColor->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    stylePtr->activeGC = newGC;

    stylePtr->flags |= STYLE_DIRTY;
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
 *	The width and height fields of *valuePtr* are set with the
 *	computed dimensions.
 *
 *---------------------------------------------------------------------------
 */
static void
CheckBoxStyleGeometryProc(ColumnStyle *colStylePtr, Value *valuePtr)
{
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)colStylePtr;
    TreeView *viewPtr;
    unsigned int bw, bh, iw, ih, tw, th, gap;

    viewPtr = stylePtr->viewPtr;
    bw = bh = ODD(stylePtr->size);
    valuePtr->width = valuePtr->height = 0;
    iw = ih = 0;
    if (stylePtr->icon != NULL) {
	iw = TreeView_IconWidth(stylePtr->icon);
	ih = TreeView_IconHeight(stylePtr->icon);
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
    tw = th = 0;
    if (stylePtr->showValue) {
	TextStyle ts;
	const char *string;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
	string = (stylePtr->onValue != NULL) ? stylePtr->onValue : 
	    valuePtr->fmtString;
	stylePtr->onPtr = Blt_Ts_CreateLayout(string, -1, &ts);
	string = (stylePtr->offValue != NULL) ? stylePtr->offValue : 
	    valuePtr->fmtString;
	stylePtr->offPtr = Blt_Ts_CreateLayout(string, -1, &ts);
	tw = MAX(stylePtr->offPtr->width, stylePtr->onPtr->width);
	th = MAX(stylePtr->offPtr->height, stylePtr->onPtr->height);
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    valuePtr->width = stylePtr->gap * 2 + bw + iw + gap + tw;
    valuePtr->height = MAX3(bh, th, ih) | 0x1;
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
 *---------------------------------------------------------------------------
 */
static void
CheckBoxStyleDrawProc(Entry *entryPtr, Value *valuePtr, Drawable drawable, 
		      ColumnStyle *colStylePtr, int x, int y)
{
    Blt_Bg bg;
    Blt_Font font;
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)colStylePtr;
    Column *colPtr;
    TextLayout *textPtr;
    TreeView *viewPtr;
    XColor *fgColor;
    int bool;
    int borderWidth, relief;
    int gap, columnWidth;
    int iconX, iconY, iconWidth, iconHeight;
    int textX, textY, textHeight;
    int xBox, yBox, boxWidth, boxHeight;

    viewPtr = stylePtr->viewPtr;
    font = CHOOSE(viewPtr->font, stylePtr->font);
    colPtr = valuePtr->columnPtr;
    borderWidth = 0;
    relief = TK_RELIEF_FLAT;
    if (valuePtr == viewPtr->activeValuePtr) {
	bg = stylePtr->activeBg;
	fgColor = stylePtr->activeFgColor;
	borderWidth = 1;
	relief = TK_RELIEF_RAISED;
    } else if (stylePtr->flags & STYLE_HIGHLIGHT) {
	bg = stylePtr->highlightBg;
	fgColor = stylePtr->highlightFgColor;
    } else {
	/* If a background was specified, override the current background.
	 * Otherwise, use the standard background taking into consideration if
	 * its the odd or even color. */
	if (stylePtr->bg != NULL) {
	    bg = stylePtr->bg;
	} else {
	    bg = ((viewPtr->altBg != NULL) && (entryPtr->flatIndex & 0x1)) 
		? viewPtr->altBg : viewPtr->bg;
	}
	fgColor = CHOOSE(viewPtr->fgColor, stylePtr->fgColor);
    }
    columnWidth = colPtr->width - PADDING(colPtr->pad);

    /*
     * Draw the active or normal background color over the entire label area.
     * This includes both the tab's text and image.  The rectangle should be 2
     * pixels wider/taller than this area. So if the label consists of just an
     * image, we get an halo around the image when the tab is active.
     */
    if (EntryIsSelected(viewPtr, entryPtr)) {
	bg = CHOOSE(viewPtr->selection.bg, stylePtr->selBg);
    }
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y+1, 
	columnWidth, entryPtr->height - 2, borderWidth, relief);

    if (columnWidth > valuePtr->width) {
	switch(colPtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    x += (columnWidth - valuePtr->width);
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (columnWidth - valuePtr->width) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }

    bool = (strcmp(valuePtr->fmtString, stylePtr->onValue) == 0);
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
    boxWidth = boxHeight = ODD(stylePtr->size);
    xBox = x + stylePtr->gap;
    yBox = y + (entryPtr->height - boxHeight) / 2;
    {
	Blt_Picture picture;
	
	if (bool) {
	    if (stylePtr->selectedPicture == NULL) {
		stylePtr->selectedPicture = Blt_PaintCheckbox(boxWidth, boxHeight, 
			stylePtr->fillColor, stylePtr->boxColor, stylePtr->checkColor, 
			TRUE);
	    } 
	    picture = stylePtr->selectedPicture;
	} else {
	    if (stylePtr->normalPicture == NULL) {
		stylePtr->normalPicture = Blt_PaintCheckbox(boxWidth, boxHeight, 
			stylePtr->fillColor, stylePtr->boxColor, stylePtr->checkColor, 
			FALSE);
	    } 
	    picture = stylePtr->normalPicture;
	}
	if (stylePtr->painter == NULL) {
	    stylePtr->painter = Blt_GetPainter(viewPtr->tkwin, 1.0);
	}
	Blt_PaintPicture(stylePtr->painter, drawable, picture, 0, 0, 
			 boxWidth, boxHeight, xBox, yBox, 0);
    }
    iconWidth = iconHeight = 0;
    if (stylePtr->icon != NULL) {
	iconWidth = TreeView_IconWidth(stylePtr->icon);
	iconHeight = TreeView_IconHeight(stylePtr->icon);
    }
    textHeight = 0;
    gap = 0;
    if (stylePtr->showValue) {
	textHeight = textPtr->height;
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    x = xBox + boxWidth + stylePtr->gap;

    /* The icon sits to the left of the text. */
    iconX = x;
    iconY = y + (entryPtr->height - iconHeight) / 2;
    textX = iconX + iconWidth + gap;
    textY = y + (entryPtr->height - textHeight) / 2;
    if (stylePtr->icon != NULL) {
	Tk_RedrawImage(TreeView_IconBits(stylePtr->icon), 0, 0, iconWidth, 
		       iconHeight, drawable, iconX, iconY);
    }
    if ((stylePtr->showValue) && (textPtr != NULL)) {
	TextStyle ts;
	XColor *color;
	int xMax;

	if (EntryIsSelected(viewPtr, entryPtr)) {
	    if (stylePtr->selFgColor != NULL) {
		color = stylePtr->selFgColor;
	    } else {
		color = viewPtr->selection.fgColor;
	    }
	} else if (entryPtr->color != NULL) {
	    color = entryPtr->color;
	} else {
	    color = fgColor;
	}
	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, font);
	Blt_Ts_SetForeground(ts, color);
	xMax = SCREENX(viewPtr, colPtr->worldX) + colPtr->width - 
	    colPtr->titleBW - colPtr->pad.side2;
	Blt_Ts_SetMaxLength(ts, xMax - textX);
	Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, textPtr, &ts, textX, textY);
    }
    stylePtr->flags &= ~STYLE_DIRTY;
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckBoxStyleEditProc --
 *
 *	Edits the "checkbox".
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
CheckBoxStyleEditProc(Entry *entryPtr, Column *colPtr, ColumnStyle *colStylePtr)
{
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)colStylePtr;
    Tcl_Obj *objPtr;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    if (Blt_Tree_GetValueByKey(viewPtr->interp, viewPtr->tree, entryPtr->node, 
	colPtr->key, &objPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (strcmp(Tcl_GetString(objPtr), stylePtr->onValue) == 0) {
	objPtr = Tcl_NewStringObj(stylePtr->offValue, -1);
    } else {
	objPtr = Tcl_NewStringObj(stylePtr->onValue, -1);
    }
    entryPtr->flags |= ENTRY_DIRTY;
    viewPtr->flags |= (DIRTY | LAYOUT_PENDING | SCROLL_PENDING);
    if (Blt_Tree_SetValueByKey(viewPtr->interp, viewPtr->tree, entryPtr->node, 
	colPtr->key, objPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (stylePtr->cmdObjPtr != NULL) {
	Tcl_Obj *cmdObjPtr;
	int result;

	cmdObjPtr = Tcl_DuplicateObj(stylePtr->cmdObjPtr);
	Tcl_ListObjAppendElement(viewPtr->interp, cmdObjPtr, 
		Tcl_NewLongObj(Blt_Tree_NodeId(entryPtr->node)));
	Tcl_IncrRefCount(cmdObjPtr);
	result = Tcl_EvalObjEx(viewPtr->interp, cmdObjPtr, TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(cmdObjPtr);
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckBoxStyleFreeProc --
 *
 *	Releases resources allocated for the checkbox. The resources freed by
 *	this routine are specific only to the "checkbox".  Other resources
 *	(common to all styles) are freed in the Blt_TreeView_FreeStyle
 *	routine.
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
CheckBoxStyleFreeProc(ColumnStyle *colStylePtr)
{
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)colStylePtr;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->gc != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->gc);
    }
    if (stylePtr->icon != NULL) {
	Blt_TreeView_FreeIcon(viewPtr, stylePtr->icon);
    }
    if (stylePtr->offPtr != NULL) {
	Blt_Free(stylePtr->offPtr);
    }
    if (stylePtr->onPtr != NULL) {
	Blt_Free(stylePtr->onPtr);
    }
    if (stylePtr->selectedPicture != NULL) {
	Blt_FreePicture(stylePtr->selectedPicture);
    }
    if (stylePtr->normalPicture != NULL) {
	Blt_FreePicture(stylePtr->normalPicture);
    }
    if (stylePtr->disabledPicture != NULL) {
	Blt_FreePicture(stylePtr->disabledPicture);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_TreeView_CreateComboBoxStyle --
 *
 *	Creates a "combobox" style.
 *
 * Results:
 *	A pointer to the new style structure.
 *
 *---------------------------------------------------------------------------
 */
ColumnStyle *
Blt_TreeView_CreateComboBoxStyle(TreeView *viewPtr, Blt_HashEntry *hPtr)
{
    ComboBoxStyle *stylePtr;

    stylePtr = Blt_AssertCalloc(1, sizeof(ComboBoxStyle));
    stylePtr->classPtr = &comboBoxClass;
    stylePtr->gap = STYLE_GAP;
    stylePtr->arrowRelief = TK_RELIEF_RAISED;
    stylePtr->arrowBW = 1;
    stylePtr->borderWidth = 1;
    stylePtr->relief = TK_RELIEF_FLAT;
    stylePtr->name = Blt_GetHashKey(&viewPtr->styleTable, hPtr);
    stylePtr->hashPtr = hPtr;
    stylePtr->link = NULL;
    stylePtr->flags = STYLE_COMBOBOX;
    stylePtr->refCount = 1;
    stylePtr->viewPtr = viewPtr;
    Blt_SetHashValue(hPtr, stylePtr);
    return (ColumnStyle *)stylePtr;
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
ComboBoxStyleConfigureProc(ColumnStyle *colStylePtr)
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)colStylePtr;
    GC newGC;
    TreeView *viewPtr;
    XColor *bgColor;
    XGCValues gcValues;
    unsigned long gcMask;

    viewPtr = stylePtr->viewPtr;
    gcValues.font = Blt_Font_Id(CHOOSE(viewPtr->font, stylePtr->font));
    bgColor = Blt_Bg_BorderColor(CHOOSE(viewPtr->bg, stylePtr->bg));
    gcMask = GCForeground | GCBackground | GCFont;

    /* Normal foreground */
    gcValues.background = bgColor->pixel;
    gcValues.foreground = CHOOSE(viewPtr->fgColor, stylePtr->fgColor)->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->gc != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->gc);
    }
    stylePtr->gc = newGC;

    /* Highlight foreground */
    gcValues.background = Blt_Bg_BorderColor(stylePtr->highlightBg)->pixel;
    gcValues.foreground = stylePtr->highlightFgColor->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    /* Active foreground */
    gcValues.background = Blt_Bg_BorderColor(stylePtr->activeBg)->pixel;
    gcValues.foreground = stylePtr->activeFgColor->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    stylePtr->activeGC = newGC;
    stylePtr->flags |= STYLE_DIRTY;
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
 *	The width and height fields of *valuePtr* are set with the computed
 *	dimensions.
 *
 *---------------------------------------------------------------------------
 */
static void
ComboBoxStyleGeometryProc(ColumnStyle *colStylePtr, Value *valuePtr)
{
    Blt_Font font;
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)colStylePtr;
    TreeView *viewPtr;
    int gap;
    int iconWidth, iconHeight;
    int textWidth, textHeight;

    viewPtr = stylePtr->viewPtr;
    textWidth = textHeight = 0;
    iconWidth = iconHeight = 0;
    valuePtr->width = valuePtr->height = 0;

    if (stylePtr->icon != NULL) {
	iconWidth = TreeView_IconWidth(stylePtr->icon);
	iconHeight = TreeView_IconHeight(stylePtr->icon);
    } 
    if (valuePtr->textPtr != NULL) {
	Blt_Free(valuePtr->textPtr);
	valuePtr->textPtr = NULL;
    }
    font = CHOOSE(viewPtr->font, stylePtr->font);
    if (valuePtr->fmtString != NULL) {	/* New string defined. */
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, font);
	valuePtr->textPtr = Blt_Ts_CreateLayout(valuePtr->fmtString, -1, &ts);
    } 
    gap = 0;
    if (valuePtr->textPtr != NULL) {
	textWidth = valuePtr->textPtr->width;
	textHeight = valuePtr->textPtr->height;
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    stylePtr->arrowWidth = Blt_TextWidth(font, "0", 1);
    stylePtr->arrowWidth += 2 * stylePtr->arrowBW;
    valuePtr->width = 2 * stylePtr->borderWidth + iconWidth + 4 * gap + 
	stylePtr->arrowWidth + textWidth;
    valuePtr->height = MAX(textHeight, iconHeight) + 2 * stylePtr->borderWidth;
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
 *---------------------------------------------------------------------------
 */
static void
ComboBoxStyleDrawProc(Entry *entryPtr, Value *valuePtr, Drawable drawable, 
		      ColumnStyle *colStylePtr, int x, int y)
{
    Blt_Bg bg;
    Column *colPtr;
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)colStylePtr;
    TreeView *viewPtr;
    XColor *fgColor;
    int arrowX, arrowY;
    int borderWidth;
    int gap, columnWidth;
    int iconX, iconY, iconWidth, iconHeight;
    int relief;
    int textX, textY, textHeight;

    viewPtr = stylePtr->viewPtr;
    borderWidth = 0;
    relief = TK_RELIEF_FLAT;
    colPtr = valuePtr->columnPtr;
    if (valuePtr == viewPtr->activeValuePtr) {
	bg = stylePtr->activeBg;
	fgColor = stylePtr->activeFgColor;
	borderWidth = 1;
	relief = TK_RELIEF_RAISED;
    } else if (stylePtr->flags & STYLE_HIGHLIGHT) {
	bg = stylePtr->highlightBg;
	fgColor = stylePtr->highlightFgColor;
    } else {
	/* If a background was specified, override the current background.
	 * Otherwise, use the standard background taking into consideration if
	 * its the odd or even color. */
	if (stylePtr->bg != NULL) {
	    bg = stylePtr->bg;
	} else {
	    bg = ((viewPtr->altBg != NULL) && (entryPtr->flatIndex & 0x1)) 
		? viewPtr->altBg : viewPtr->bg;
	}
	fgColor = GetStyleForeground(colPtr);
    }

    columnWidth = colPtr->width - PADDING(colPtr->pad);
    /* if (valuePtr == viewPtr->activeValuePtr) */ {
	/*
	 * Draw the active or normal background color over the entire label
	 * area.  This includes both the tab's text and image.  The rectangle
	 * should be 2 pixels wider/taller than this area. So if the label
	 * consists of just an image, we get an halo around the image when the
	 * tab is active.
	 */
	if (EntryIsSelected(viewPtr, entryPtr)) {
	    if (stylePtr->selBg != NULL) {
		bg = stylePtr->selBg;
	    } else {
		bg = viewPtr->selection.bg;
	    }
	    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y+1, 
		columnWidth, entryPtr->height - 2, borderWidth, relief);
	} else {
	    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y+1, 
		columnWidth, entryPtr->height - 2, borderWidth, relief);
	}
    }   
    if (EntryIsSelected(viewPtr, entryPtr)) {
	fgColor = viewPtr->selection.fgColor;
    }
    arrowX = x + colPtr->width;
    arrowX -= colPtr->pad.side2 + stylePtr->borderWidth  + stylePtr->arrowWidth + 
	stylePtr->gap;
    arrowY = y;

    if (columnWidth > valuePtr->width) {
	switch(colPtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    x += (columnWidth - valuePtr->width);
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (columnWidth - valuePtr->width) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }

#ifdef notdef
    textX = textY = iconX = iconY = 0;	/* Suppress compiler warning. */
#endif
    
    iconWidth = iconHeight = 0;
    if (stylePtr->icon != NULL) {
	iconWidth = TreeView_IconWidth(stylePtr->icon);
	iconHeight = TreeView_IconHeight(stylePtr->icon);
    }
    textHeight = 0;
    if (valuePtr->textPtr != NULL) {
	textHeight = valuePtr->textPtr->height;
    }
    gap = 0;
    if ((stylePtr->icon != NULL) && (valuePtr->textPtr != NULL)) {
	gap = stylePtr->gap;
    }

    iconX = x + gap;
    iconY = y + (entryPtr->height - iconHeight) / 2;
    textX = iconX + iconWidth + gap;
    textY = y + (entryPtr->height - textHeight) / 2;

    if (stylePtr->icon != NULL) {
	Tk_RedrawImage(TreeView_IconBits(stylePtr->icon), 0, 0, iconWidth, 
	       iconHeight, drawable, iconX, iconY);
    }
    if (valuePtr->textPtr != NULL) {
	TextStyle ts;
	XColor *color;
	Blt_Font font;
	int xMax;
	
	font = CHOOSE(viewPtr->font, stylePtr->font);
	if (EntryIsSelected(viewPtr, entryPtr)) {
	    color = CHOOSE(viewPtr->selection.fgColor, stylePtr->selFgColor);
	} else if (entryPtr->color != NULL) {
	    color = entryPtr->color;
	} else {
	    color = fgColor;
	}
	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, font);
	Blt_Ts_SetForeground(ts, color);
	xMax = SCREENX(viewPtr, colPtr->worldX) + colPtr->width - 
	    colPtr->titleBW - colPtr->pad.side2 - stylePtr->arrowWidth;
	Blt_Ts_SetMaxLength(ts, xMax - textX);
	Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, valuePtr->textPtr, &ts, 
		textX, textY);
    }
    if (valuePtr == viewPtr->activeValuePtr) {
	bg = stylePtr->activeBg;
    } else {
	bg = colPtr->titleBg;
#ifdef notdef
	bg = CHOOSE(viewPtr->bg, stylePtr->bg);
#endif
    }
#ifdef notdef
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, arrowX, 
	arrowY + stylePtr->borderWidth, stylePtr->arrowWidth, 
	entryPtr->height - 2 * stylePtr->borderWidth, stylePtr->arrowBW, 
	stylePtr->arrowRelief); 
#endif
    Blt_DrawArrow(viewPtr->display, drawable, fgColor, arrowX, arrowY - 1, 
	stylePtr->arrowWidth, entryPtr->height, stylePtr->arrowBW, ARROW_DOWN);
    stylePtr->flags &= ~STYLE_DIRTY;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleEditProc --
 *
 *	Edits the "combobox".
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ComboBoxStyleEditProc(Entry *entryPtr, Column *colPtr, ColumnStyle *colStylePtr)
{
    TreeView *viewPtr;

    viewPtr = colStylePtr->viewPtr;
    return Blt_TreeView_CreateTextbox(viewPtr, entryPtr, colPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleFreeProc --
 *
 *	Releases resources allocated for the combobox. The resources freed by
 *	this routine are specific only to the "combobox".  Other resources
 *	(common to all styles) are freed in the Blt_TreeView_FreeStyle
 *	routine.
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
ComboBoxStyleFreeProc(ColumnStyle *colStylePtr)
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)colStylePtr;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->gc != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->gc);
    }
    if (stylePtr->icon != NULL) {
	Blt_TreeView_FreeIcon(viewPtr, stylePtr->icon);
    }
}


#endif /*TREEVIEW*/

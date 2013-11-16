/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTreeViewStyle.c --
 *
 * This module implements styles for treeview widget cells.
 *
 *	Copyright 1998-2008 George A Howlett.
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

#define GEOMETRY                0

#define STYLE_GAP		2
#define ARROW_WIDTH		13

#define TEXT_VAR_TRACED         (1<<16)
#define ICON_VAR_TRACED         (1<<17)
#define TRACE_VAR_FLAGS		(TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|\
				 TCL_TRACE_UNSETS)

#define DEF_ACTIVE_FG			STD_ACTIVE_FOREGROUND
#define DEF_ALT_BG			RGB_GREY97
#define DEF_DISABLE_BG			RGB_GREY97
#define DEF_DISABLE_FG			RGB_GREY85
#define DEF_FOCUS_COLOR			"black"
#define DEF_FOCUS_DASHES		"dot"
#define DEF_GAP				"3"
#define DEF_HIGHLIGHT_BG                STD_NORMAL_BACKGROUND
#define DEF_HIGHLIGHT_FG                STD_NORMAL_FOREGROUND
#define DEF_ICON			(char *)NULL
#define DEF_JUSTIFY			"center"
#ifdef WIN32
#define DEF_ACTIVE_BG                   RGB_GREY85
#else
#define DEF_ACTIVE_BG                   RGB_GREY95
#endif

#ifdef WIN32
#define DEF_TEXTBOX_CURSOR		"arrow"
#else
#define DEF_TEXTBOX_CURSOR		"hand2"
#endif /*WIN32*/
#define DEF_TEXTBOX_SIDE		"left"
#define DEF_TEXTBOX_VALIDATE_COMMAND	(char *)NULL
#define DEF_TEXTBOX_COMMAND		(char *)NULL

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

static Blt_OptionParseProc ObjToIconProc;
static Blt_OptionPrintProc IconToObjProc;
static Blt_OptionFreeProc FreeIconProc;
static Blt_CustomOption iconOption = {
    ObjToIconProc, IconToObjProc, FreeIconProc, NULL,
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

typedef struct {
    int refCount;			/* Usage reference count.  A
					 * reference count of zero
					 * indicates that the style may be
					 * freed. */
    unsigned int flags;			/* Bit field containing both the
					 * style type and various flags. */
    const char *name;			/* Instance name. */
    CellStyleClass *classPtr;		/* Contains class-specific
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
					   cell. */
    Tcl_Obj *cmdObjPtr;

    /* TextBox-specific fields */
    int side;				/* Position of the text in relation to
					 * the icon.  */
    int justify;
} TextBoxStyle;


static Blt_ConfigSpec textBoxSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_ACTIVE_BG, 
	Blt_Offset(TextBoxStyle, activeBg), 0},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeFackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
	"ActiveForeground", DEF_ACTIVE_FG, 
	Blt_Offset(TextBoxStyle, activeFg), 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	(char *)NULL, Blt_Offset(TextBoxStyle, normalBg), BLT_CONFIG_NULL_OK},
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
	Blt_Offset(TextBoxStyle, normalFg),BLT_CONFIG_NULL_OK },
    {BLT_CONFIG_PIXELS_NNEG, "-gap", "gap", "Gap", DEF_GAP, 
	Blt_Offset(TextBoxStyle, gap), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_HIGHLIGHT_BG, 
        Blt_Offset(TextBoxStyle, highlightBg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", 
	"HighlightForeground", DEF_HIGHLIGHT_FG, 
	Blt_Offset(TextBoxStyle, highlightFg), 0},
    {BLT_CONFIG_SYNONYM, "-highlightbg", "highlightBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-highlightfg", "highlightForeground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", (char *)NULL, 
	Blt_Offset(TextBoxStyle, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_STRING, "-key", "key", "key", 	(char *)NULL, 
	Blt_Offset(TextBoxStyle, key), BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_JUSTIFY, 
	Blt_Offset(TextBoxStyle, justify), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
	"Foreground", (char *)NULL, Blt_Offset(TextBoxStyle, selectBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
	(char *)NULL, Blt_Offset(TextBoxStyle, selectFg), 0},
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
    int refCount;			/* Usage reference count.  A
					 * reference count of zero
					 * indicates that the style may be
					 * freed. */
    unsigned int flags;			/* Bit field containing both the
					 * style type and various flags. */
    const char *name;			/* Instance name. */
    CellStyleClass *classPtr;		/* Contains class-specific
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
					   cell. */
    Tcl_Obj *cmdObjPtr;

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
    int justify;
} CheckBoxStyle;


static Blt_ConfigSpec checkBoxSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_ACTIVE_BG, 
	Blt_Offset(CheckBoxStyle, activeBg), 0},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeFackground", 
	(char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
	"ActiveForeground", DEF_ACTIVE_FG, 
	Blt_Offset(CheckBoxStyle, activeFg), 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	(char *)NULL, Blt_Offset(CheckBoxStyle, normalBg), BLT_CONFIG_NULL_OK},
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
	Blt_Offset(CheckBoxStyle, normalFg), BLT_CONFIG_NULL_OK },
    {BLT_CONFIG_PIXELS_NNEG, "-gap", "gap", "Gap", DEF_CHECKBOX_GAP, 
	Blt_Offset(CheckBoxStyle, gap), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_HIGHLIGHT_BG, 
        Blt_Offset(CheckBoxStyle, highlightBg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", 
	"HighlightForeground", DEF_HIGHLIGHT_FG, 
	 Blt_Offset(CheckBoxStyle, highlightFg), 0},
    {BLT_CONFIG_SYNONYM, "-highlightbg", "highlightBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-highlightfg", "highlightForeground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", (char *)NULL, 
	Blt_Offset(CheckBoxStyle, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_STRING, "-key", "key", "key", (char *)NULL, 
	Blt_Offset(CheckBoxStyle, key), BLT_CONFIG_NULL_OK, 0},
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
    {BLT_CONFIG_STRING, "-offvalue", "offValue", "OffValue", 
	DEF_CHECKBOX_OFFVALUE, Blt_Offset(CheckBoxStyle, offValue), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-onvalue", "onValue", "OnValue", DEF_CHECKBOX_ONVALUE,
	Blt_Offset(CheckBoxStyle, onValue), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-key", "key", "key", (char *)NULL, 
	Blt_Offset(CheckBoxStyle, key), BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
	"Foreground", (char *)NULL, Blt_Offset(CheckBoxStyle, selectBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
	(char *)NULL, Blt_Offset(CheckBoxStyle, selectFg), 0},
    {BLT_CONFIG_BOOLEAN, "-showvalue", "showValue", "ShowValue",
	DEF_CHECKBOX_SHOWVALUE, Blt_Offset(CheckBoxStyle, showValue), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

typedef struct {
    int refCount;			/* Usage reference count.  A
					 * reference count of zero
					 * indicates that the style may be
					 * freed. */
    unsigned int flags;			/* Bit field containing both the
					 * style type and various flags. */
    const char *name;			/* Instance name. */
    CellStyleClass *classPtr;		/* Contains class-specific
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
					   cell. */
    Tcl_Obj *cmdObjPtr;

    /* ComboBox-specific fields */

    int justify;
    int borderWidth;			/* Width of outer border
					 * surrounding the entire box. */
    int relief;				/* Relief of outer border. */

    int scrollWidth;
    /*  */
    int postedRelief, activeRelief;

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

    int arrow;
} ComboBoxStyle;

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
    {BLT_CONFIG_RELIEF, "-arrowrelief", "arrowRelief", "ArrowRelief",
	DEF_COMBOBOX_ARROW_RELIEF, Blt_Offset(ComboBoxStyle, arrowRelief),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-arrowborderwidth", "arrowBorderWidth", 
	"ArrowBorderWidth", DEF_COMBOBOX_ARROW_BORDERWIDTH, 
	Blt_Offset(ComboBoxStyle, arrowBW), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	(char *)NULL, Blt_Offset(ComboBoxStyle, normalBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0, 
	0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_COMBOBOX_BORDERWIDTH, Blt_Offset(ComboBoxStyle, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
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
	(Blt_CustomOption *)STYLE_EDITABLE},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", (char *)NULL, 
	Blt_Offset(ComboBoxStyle, font), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
	(char *)NULL, Blt_Offset(ComboBoxStyle, normalFg), BLT_CONFIG_NULL_OK },
    {BLT_CONFIG_PIXELS_NNEG, "-gap", "gap", "Gap", DEF_GAP, 
	Blt_Offset(ComboBoxStyle, gap), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_HIGHLIGHT_BG, 
        Blt_Offset(ComboBoxStyle, highlightBg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", 
	"HighlightForeground", DEF_HIGHLIGHT_FG, 
	 Blt_Offset(ComboBoxStyle, highlightFg), 0},
    {BLT_CONFIG_SYNONYM, "-highlightbg", "highlightBackground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-highlightfg", "highlightForeground", (char *)NULL, 
	(char *)NULL, 0, 0},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", (char *)NULL, 
	Blt_Offset(ComboBoxStyle, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_CUSTOM, "-iconvariable", "iconVariable", "IconVariable", 
	DEF_COMBOBOX_ICON_VARIABLE, Blt_Offset(ComboBoxStyle, iconVarObjPtr), 
        BLT_CONFIG_NULL_OK, &iconVarOption},
    {BLT_CONFIG_STRING, "-key", "key", "key", (char *)NULL, 
	Blt_Offset(ComboBoxStyle, key), BLT_CONFIG_NULL_OK, 0},
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
	"Foreground", (char *)NULL, Blt_Offset(ComboBoxStyle, selectBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
	(char *)NULL, Blt_Offset(ComboBoxStyle, selectFg), 0},
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

static CellStyleConfigureProc CheckBoxStyleConfigureProc;
static CellStyleConfigureProc ComboBoxStyleConfigureProc;
static CellStyleConfigureProc TextBoxStyleConfigureProc;
static CellStyleDrawProc CheckBoxStyleDrawProc;
static CellStyleDrawProc ComboBoxStyleDrawProc;
static CellStyleDrawProc TextBoxStyleDrawProc;
static CellStyleEditProc CheckBoxStyleEditProc;
static CellStyleEditProc ComboBoxStyleEditProc;
static CellStyleEditProc TextBoxStyleEditProc;
static CellStyleFreeProc CheckBoxStyleFreeProc;
static CellStyleFreeProc ComboBoxStyleFreeProc;
static CellStyleFreeProc TextBoxStyleFreeProc;
static CellStyleGeometryProc CheckBoxStyleGeometryProc;
static CellStyleGeometryProc ComboBoxStyleGeometryProc;
static CellStyleGeometryProc TextBoxStyleGeometryProc;
static CellStyleIdentifyProc ComboBoxStyleIdentifyProc;
static CellStylePostProc ComboBoxStylePostProc;
static CellStyleUnpostProc ComboBoxStyleUnpostProc;


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
    CellStyle *stylePtr;

    stylePtr = colPtr->stylePtr;
    if ((stylePtr != NULL) && (stylePtr->normalFg != NULL)) {
	return stylePtr->normalFg;
    }
    return colPtr->viewPtr->normalFg;
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
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    viewPtr->flags |= GEOMETRY;
}

static Icon
GetIcon(CellStyle *stylePtr, const char *iconName)
{
    Blt_HashEntry *hPtr;
    int isNew;
    struct _Icon *iconPtr;
    TreeView *viewPtr;

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
	TreeView *viewPtr;

	viewPtr = iconPtr->viewPtr;
	Blt_DeleteHashEntry(&viewPtr->iconTable, iconPtr->hashPtr);
	Tk_FreeImage(iconPtr->tkImage);
	Blt_Free(iconPtr);
    }
}

#ifdef notdef
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

    /* FIXME: need to signal redraw to treeview widget.  */
    cellPtr->flags |= GEOMETRY;
}
#endif

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
	 * unless the widget already has that value.  This happens when the
	 * variable changes the value because we changed it because someone
	 * typed in the entry.
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
	Blt_TreeView_EventuallyRedraw(stylePtr->viewPtr);
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
	*flagsPtr &= ~STYLE_POSTED;
    } else if ((c == 'p') && (strncmp(string, "posted", length) == 0)) {
	*flagsPtr |= STYLE_POSTED;
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

    if (state & STYLE_POSTED) {
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

static CellStyleClass textBoxClass = {
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

static CellStyleClass checkBoxClass = {
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

static CellStyleClass comboBoxClass = {
    "combobox", 
    "ComboBoxStyle", 
    comboBoxStyleSpecs,
    ComboBoxStyleConfigureProc,
    ComboBoxStyleGeometryProc,
    ComboBoxStyleDrawProc,
    ComboBoxStyleIdentifyProc,          
    ComboBoxStyleEditProc,
    ComboBoxStyleFreeProc,
    ComboBoxStylePostProc,     
    ComboBoxStyleUnpostProc    
};

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
NewTextBoxStyle(TreeView *viewPtr, Blt_HashEntry *hPtr)
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
    return (CellStyle *)stylePtr;
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
TextBoxStyleConfigureProc(CellStyle *cellStylePtr)
{
    GC newGC;
    TextBoxStyle *stylePtr = (TextBoxStyle *)cellStylePtr;
    TreeView *viewPtr;
    XGCValues gcValues;
    unsigned long gcMask;

    viewPtr = stylePtr->viewPtr;
    gcMask = GCForeground | GCFont;
    gcValues.font = Blt_Font_Id(CHOOSE(viewPtr->font, stylePtr->font));

    /* Normal GC */
    gcValues.foreground = CHOOSE(viewPtr->normalFg, stylePtr->normalFg)->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    stylePtr->normalGC = newGC;

    /* Highlight GC  */
    gcValues.foreground = stylePtr->highlightFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    /* Active GC  */
    gcValues.foreground = stylePtr->activeFg->pixel;
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
 *	Determines the space requirements for the "textbox" given the cell to
 *	be displayed.  Depending upon whether an icon or text is displayed and
 *	their relative placements, this routine computes the space needed for
 *	the text entry.
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
TextBoxStyleGeometryProc(CellStyle *cellStylePtr, Cell *cellPtr)
{
    TextBoxStyle *stylePtr = (TextBoxStyle *)cellStylePtr;
    TreeView *viewPtr;
    int gap;
    int iconWidth, iconHeight;
    int textWidth, textHeight;

    viewPtr = stylePtr->viewPtr;
    textWidth = textHeight = 0;
    iconWidth = iconHeight = 0;
    cellPtr->width = cellPtr->height = 0;

    if (stylePtr->icon != NULL) {
	iconWidth = IconWidth(stylePtr->icon);
	iconHeight = IconHeight(stylePtr->icon);
    } 
    if (cellPtr->textPtr != NULL) {
	Blt_Free(cellPtr->textPtr);
	cellPtr->textPtr = NULL;
    }
    if (cellPtr->fmtString != NULL) {	/* New string defined. */
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
	cellPtr->textPtr = Blt_Ts_CreateLayout(cellPtr->fmtString, -1, &ts);
    } 
    gap = 0;
    if (cellPtr->textPtr != NULL) {
	textWidth = cellPtr->textPtr->width;
	textHeight = cellPtr->textPtr->height;
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    if (stylePtr->side & (SIDE_TOP | SIDE_BOTTOM)) {
	cellPtr->width = MAX(textWidth, iconWidth);
	cellPtr->height = iconHeight + gap + textHeight;
    } else {
	cellPtr->width = iconWidth + gap + textWidth;
	cellPtr->height = MAX(textHeight, iconHeight);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TextBoxStyleDrawProc --
 *
 *	Draws the "textbox" given the screen coordinates and the cell to be
 *	displayed.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The textbox cell is drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
TextBoxStyleDrawProc(Cell *cellPtr, Drawable drawable, 
                     CellStyle *cellStylePtr, int x, int y)
{
    Blt_Bg bg;
    Column *colPtr;
    TextBoxStyle *stylePtr = (TextBoxStyle *)cellStylePtr;
    TreeView *viewPtr;
    XColor *fg;
    int gap, columnWidth;
    int iconX, iconY, iconWidth, iconHeight;
    int textX, textY, textWidth, textHeight;
    Entry *entryPtr;

    viewPtr = stylePtr->viewPtr;
    colPtr = cellPtr->colPtr;
    entryPtr = cellPtr->entryPtr;
    if (stylePtr->flags & STYLE_HIGHLIGHT) {
	bg = stylePtr->highlightBg;
	fg = stylePtr->highlightFg;
    } else {
	if (stylePtr->normalBg != NULL) {
	    bg = stylePtr->normalBg;
	} else if ((viewPtr->altBg != NULL) && (entryPtr->flatIndex & 0x1)) {
	    bg = viewPtr->altBg;
	} else {
	    bg = viewPtr->normalBg;
	}
	fg = CHOOSE(viewPtr->normalFg, stylePtr->normalFg);
    }
    if (EntryIsSelected(viewPtr, entryPtr)) {
	bg = (stylePtr->selectBg != NULL) ? 
	    stylePtr->selectBg : viewPtr->selection.bg;
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
    if (columnWidth > cellPtr->width) {
	switch(colPtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    x += (columnWidth - cellPtr->width);
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (columnWidth - cellPtr->width) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }

    textX = textY = iconX = iconY = 0;	/* Suppress compiler warning. */
    
    iconWidth = iconHeight = 0;
    if (stylePtr->icon != NULL) {
	iconWidth = IconWidth(stylePtr->icon);
	iconHeight = IconHeight(stylePtr->icon);
    }
    textWidth = textHeight = 0;
    if (cellPtr->textPtr != NULL) {
	textWidth = cellPtr->textPtr->width;
	textHeight = cellPtr->textPtr->height;
    }
    gap = 0;
    if ((stylePtr->icon != NULL) && (cellPtr->textPtr != NULL)) {
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
	Tk_RedrawImage(IconBits(stylePtr->icon), 0, 0, iconWidth, 
		       iconHeight, drawable, iconX, iconY);
    }
    if (cellPtr->textPtr != NULL) {
	TextStyle ts;
	XColor *color;
	Blt_Font font;
	int xMax;

	font = CHOOSE(viewPtr->font, stylePtr->font);
	if (EntryIsSelected(viewPtr, entryPtr)) {
	    if (stylePtr->selectFg != NULL) {
		color = stylePtr->selectFg;
	    } else {
		color = viewPtr->selection.fg;
	    }
	} else if (entryPtr->color != NULL) {
	    color = entryPtr->color;
	} else {
	    color = fg;
	}
	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, font);
	Blt_Ts_SetForeground(ts, color);
	xMax = colPtr->width - colPtr->titleBW - colPtr->pad.side2;
	Blt_Ts_SetMaxLength(ts, xMax);
	Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, cellPtr->textPtr, &ts, 
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
 *	The checkbox cell is drawn.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TextBoxStyleEditProc(Cell *cellPtr, CellStyle *stylePtr)
{
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    return Blt_TreeView_CreateTextbox(viewPtr, cellPtr->entryPtr, 
                                      cellPtr->colPtr);
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
TextBoxStyleFreeProc(CellStyle *cellStylePtr)
{
    TextBoxStyle *stylePtr = (TextBoxStyle *)cellStylePtr;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    if (stylePtr->icon != NULL) {
	FreeIcon(stylePtr->icon);
    }
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
NewCheckBoxStyle(TreeView *viewPtr, Blt_HashEntry *hPtr)
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
CheckBoxStyleConfigureProc(CellStyle *cellStylePtr)
{
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    GC newGC;
    TreeView *viewPtr;
    XColor *bgColor;
    XGCValues gcValues;
    unsigned long gcMask;

    viewPtr = stylePtr->viewPtr;
    gcMask = GCForeground | GCBackground | GCFont;
    gcValues.font = Blt_Font_Id(CHOOSE(viewPtr->font, stylePtr->font));
    bgColor = Blt_Bg_BorderColor(CHOOSE(viewPtr->normalBg, stylePtr->normalBg));

    gcValues.background = bgColor->pixel;
    gcValues.foreground = CHOOSE(viewPtr->normalFg, stylePtr->normalFg)->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    stylePtr->normalGC = newGC;
    gcValues.background = Blt_Bg_BorderColor(stylePtr->highlightBg)->pixel;
    gcValues.foreground = stylePtr->highlightFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    gcValues.background = Blt_Bg_BorderColor(stylePtr->activeBg)->pixel;
    gcValues.foreground = stylePtr->activeFg->pixel;
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
CheckBoxStyleGeometryProc(CellStyle *cellStylePtr, Cell *cellPtr)
{
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    TreeView *viewPtr;
    unsigned int bw, bh, iw, ih, tw, th, gap;

    viewPtr = stylePtr->viewPtr;
    bw = bh = ODD(stylePtr->size);
    cellPtr->width = cellPtr->height = 0;
    iw = ih = 0;
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
    tw = th = 0;
    if (stylePtr->showValue) {
	TextStyle ts;
	const char *string;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
	string = (stylePtr->onValue != NULL) ? stylePtr->onValue : 
	    cellPtr->fmtString;
	stylePtr->onPtr = Blt_Ts_CreateLayout(string, -1, &ts);
	string = (stylePtr->offValue != NULL) ? stylePtr->offValue : 
	    cellPtr->fmtString;
	stylePtr->offPtr = Blt_Ts_CreateLayout(string, -1, &ts);
	tw = MAX(stylePtr->offPtr->width, stylePtr->onPtr->width);
	th = MAX(stylePtr->offPtr->height, stylePtr->onPtr->height);
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    cellPtr->width = stylePtr->gap * 2 + bw + iw + gap + tw;
    cellPtr->height = MAX3(bh, th, ih) | 0x1;
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckBoxStyleDrawProc --
 *
 *	Draws the "checkbox" given the screen coordinates and the
 *	cell to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox cell is drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
CheckBoxStyleDrawProc(Cell *cellPtr, Drawable drawable, 
		      CellStyle *cellStylePtr, int x, int y)
{
    Blt_Bg bg;
    Blt_Font font;
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    Column *colPtr;
    TextLayout *textPtr;
    TreeView *viewPtr;
    XColor *fg;
    int bool;
    int borderWidth, relief;
    int gap, columnWidth;
    int iconX, iconY, iconWidth, iconHeight;
    int textX, textY, textHeight;
    int xBox, yBox, boxWidth, boxHeight;
    Entry *entryPtr;

    viewPtr = stylePtr->viewPtr;
    font = CHOOSE(viewPtr->font, stylePtr->font);
    colPtr = cellPtr->colPtr;
    entryPtr = cellPtr->entryPtr;
    borderWidth = 0;
    relief = TK_RELIEF_FLAT;
    if (cellPtr == viewPtr->activeCellPtr) {
	bg = stylePtr->activeBg;
	fg = stylePtr->activeFg;
	borderWidth = 1;
	relief = TK_RELIEF_RAISED;
    } else if (stylePtr->flags & STYLE_HIGHLIGHT) {
	bg = stylePtr->highlightBg;
	fg = stylePtr->highlightFg;
    } else {
	/* If a background was specified, override the current background.
	 * Otherwise, use the standard background taking into consideration if
	 * its the odd or even color. */
	if (stylePtr->normalBg != NULL) {
	    bg = stylePtr->normalBg;
	} else {
	    bg = ((viewPtr->altBg != NULL) && (entryPtr->flatIndex & 0x1)) 
		? viewPtr->altBg : viewPtr->normalBg;
	}
	fg = CHOOSE(viewPtr->normalFg, stylePtr->normalFg);
    }
    columnWidth = colPtr->width - PADDING(colPtr->pad);

    /*
     * Draw the active or normal background color over the entire label area.
     * This includes both the tab's text and image.  The rectangle should be 2
     * pixels wider/taller than this area. So if the label consists of just an
     * image, we get an halo around the image when the tab is active.
     */
    if (EntryIsSelected(viewPtr, entryPtr)) {
	bg = CHOOSE(viewPtr->selection.bg, stylePtr->selectBg);
    }
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y+1, 
	columnWidth, entryPtr->height - 2, borderWidth, relief);

    if (columnWidth > cellPtr->width) {
	switch(colPtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    x += (columnWidth - cellPtr->width);
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (columnWidth - cellPtr->width) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }

    bool = (strcmp(cellPtr->fmtString, stylePtr->onValue) == 0);
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
	iconWidth = IconWidth(stylePtr->icon);
	iconHeight = IconHeight(stylePtr->icon);
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
	Tk_RedrawImage(IconBits(stylePtr->icon), 0, 0, iconWidth, 
		       iconHeight, drawable, iconX, iconY);
    }
    if ((stylePtr->showValue) && (textPtr != NULL)) {
	TextStyle ts;
	XColor *color;
	int xMax;

	if (EntryIsSelected(viewPtr, entryPtr)) {
	    if (stylePtr->selectFg != NULL) {
		color = stylePtr->selectFg;
	    } else {
		color = viewPtr->selection.fg;
	    }
	} else if (entryPtr->color != NULL) {
	    color = entryPtr->color;
	} else {
	    color = fg;
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
 *	The checkbox cell is drawn.
 *
 *---------------------------------------------------------------------------
 */
static int
CheckBoxStyleEditProc(Cell *cellPtr, CellStyle *cellStylePtr)
{
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    Tcl_Obj *objPtr;
    TreeView *viewPtr;
    Entry *entryPtr;
    Column *colPtr;

    entryPtr = cellPtr->entryPtr;
    colPtr = cellPtr->colPtr;
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
CheckBoxStyleFreeProc(CellStyle *cellStylePtr)
{
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    if (stylePtr->icon != NULL) {
	FreeIcon(stylePtr->icon);
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
 * NewComboBoxStyle --
 *
 *	Creates a "combobox" style.
 *
 * Results:
 *	A pointer to the new style structure.
 *
 *---------------------------------------------------------------------------
 */
static CellStyle *
NewComboBoxStyle(TreeView *viewPtr, Blt_HashEntry *hPtr)
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
ComboBoxStyleConfigureProc(CellStyle *cellStylePtr)
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    GC newGC;
    TreeView *viewPtr;
    XColor *bgColor;
    XGCValues gcValues;
    unsigned long gcMask;

    viewPtr = stylePtr->viewPtr;
    gcValues.font = Blt_Font_Id(CHOOSE(viewPtr->font, stylePtr->font));
    bgColor = Blt_Bg_BorderColor(CHOOSE(viewPtr->normalBg, stylePtr->normalBg));
    gcMask = GCForeground | GCBackground | GCFont;

    /* Normal foreground */
    gcValues.background = bgColor->pixel;
    gcValues.foreground = CHOOSE(viewPtr->normalFg, stylePtr->normalFg)->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    stylePtr->normalGC = newGC;

    /* Highlight foreground */
    gcValues.background = Blt_Bg_BorderColor(stylePtr->highlightBg)->pixel;
    gcValues.foreground = stylePtr->highlightFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    /* Active foreground */
    gcValues.background = Blt_Bg_BorderColor(stylePtr->activeBg)->pixel;
    gcValues.foreground = stylePtr->activeFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    stylePtr->activeGC = newGC;
    stylePtr->flags |= STYLE_DIRTY;
}

static int
GetComboMenuGeometry(Tcl_Interp *interp, TreeView *viewPtr, 
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
 *	The width and height fields of *valuePtr* are set with the computed
 *	dimensions.
 *
 *---------------------------------------------------------------------------
 */
static void
ComboBoxStyleGeometryProc(CellStyle *cellStylePtr, Cell *cellPtr)
{
    Blt_Font font;
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    TreeView *viewPtr;
    int gap;
    int iconWidth, iconHeight;
    int textWidth, textHeight;

    viewPtr = stylePtr->viewPtr;
    textWidth = textHeight = 0;
    iconWidth = iconHeight = 0;
    cellPtr->width = cellPtr->height = 0;

    if (stylePtr->icon != NULL) {
	iconWidth = IconWidth(stylePtr->icon);
	iconHeight = IconHeight(stylePtr->icon);
    } 
    if (cellPtr->textPtr != NULL) {
	Blt_Free(cellPtr->textPtr);
	cellPtr->textPtr = NULL;
    }
    font = CHOOSE(viewPtr->font, stylePtr->font);
    if (cellPtr->fmtString != NULL) {	/* New string defined. */
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, font);
	cellPtr->textPtr = Blt_Ts_CreateLayout(cellPtr->fmtString, -1, &ts);
    } 
    gap = 0;
    if (cellPtr->textPtr != NULL) {
	textWidth = cellPtr->textPtr->width;
	textHeight = cellPtr->textPtr->height;
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    stylePtr->arrowWidth = Blt_TextWidth(font, "0", 1);
    stylePtr->arrowWidth += 2 * stylePtr->arrowBW;
    cellPtr->width = 2 * stylePtr->borderWidth + iconWidth + 4 * gap + 
	stylePtr->arrowWidth + textWidth;
    cellPtr->height = MAX(textHeight, iconHeight) + 2 * stylePtr->borderWidth;
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleDrawProc --
 *
 *	Draws the "combobox" given the screen coordinates and the
 *	cell to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The combobox cell is drawn.
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
    if ((entryPtr->flags|colPtr->flags|cellPtr->flags) & SELECTED) { 
	bg = stylePtr->selectBg;
	gc = stylePtr->selectGC;
	fg = stylePtr->selectFg;
    } else if ((entryPtr->flags|colPtr->flags|cellPtr->flags) & DISABLED) {
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
    cellWidth =  cellPtr->width  - 2 * (stylePtr->borderWidth + CELL_PADX) - 3;

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
#endif

/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleDrawProc --
 *
 *	Draws the "combobox" given the screen coordinates and the
 *	cell to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The combobox cell is drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
ComboBoxStyleDrawProc(Cell *cellPtr, Drawable drawable, 
		      CellStyle *cellStylePtr, int x, int y)
{
    Blt_Bg bg;
    Column *colPtr;
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    TreeView *viewPtr;
    XColor *fg;
    int arrowX, arrowY;
    int borderWidth;
    int gap, columnWidth;
    int iconX, iconY, iconWidth, iconHeight;
    int relief;
    int textX, textY, textHeight;
    Entry *entryPtr;

    viewPtr = stylePtr->viewPtr;
    borderWidth = 0;
    relief = TK_RELIEF_FLAT;
    colPtr = cellPtr->colPtr;
    entryPtr = cellPtr->entryPtr;
    if (cellPtr == viewPtr->activeCellPtr) {
	bg = stylePtr->activeBg;
	fg = stylePtr->activeFg;
	borderWidth = 1;
	relief = TK_RELIEF_RAISED;
    } else if (stylePtr->flags & STYLE_HIGHLIGHT) {
	bg = stylePtr->highlightBg;
	fg = stylePtr->highlightFg;
    } else {
	/* If a background was specified, override the current background.
	 * Otherwise, use the standard background taking into consideration if
	 * its the odd or even color. */
	if (stylePtr->normalBg != NULL) {
	    bg = stylePtr->normalBg;
	} else {
	    bg = ((viewPtr->altBg != NULL) && (entryPtr->flatIndex & 0x1)) 
		? viewPtr->altBg : viewPtr->normalBg;
	}
	fg = GetStyleForeground(colPtr);
    }

    columnWidth = colPtr->width - PADDING(colPtr->pad);
    /*
     * Draw the active or normal background color over the entire label
     * area.  This includes both the tab's text and image.  The
     * rectangle should be 2 pixels wider/taller than this area. So if
     * the label consists of just an image, we get an halo around the
     * image when the tab is active.
     */
    if (EntryIsSelected(viewPtr, entryPtr)) {
        if (stylePtr->selectBg != NULL) {
            bg = stylePtr->selectBg;
        } else {
            bg = viewPtr->selection.bg;
        }
        Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y+1, 
                columnWidth, entryPtr->height - 2, borderWidth, relief);
    } else {
        Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y+1, 
                columnWidth, entryPtr->height - 2, borderWidth, relief);
    }
    if (EntryIsSelected(viewPtr, entryPtr)) {
	fg = viewPtr->selection.fg;
    }
    arrowX = x + colPtr->width;
    arrowX -= colPtr->pad.side2 + stylePtr->borderWidth  + 
        stylePtr->arrowWidth + stylePtr->gap;
    arrowY = y;

    if (columnWidth > cellPtr->width) {
	switch(colPtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    x += (columnWidth - cellPtr->width);
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (columnWidth - cellPtr->width) / 2;
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
	iconWidth = IconWidth(stylePtr->icon);
	iconHeight = IconHeight(stylePtr->icon);
    }
    textHeight = 0;
    if (cellPtr->textPtr != NULL) {
	textHeight = cellPtr->textPtr->height;
    }
    gap = 0;
    if ((stylePtr->icon != NULL) && (cellPtr->textPtr != NULL)) {
	gap = stylePtr->gap;
    }

    iconX = x + gap;
    iconY = y + (entryPtr->height - iconHeight) / 2;
    textX = iconX + iconWidth + gap;
    textY = y + (entryPtr->height - textHeight) / 2;

    if (stylePtr->icon != NULL) {
	Tk_RedrawImage(IconBits(stylePtr->icon), 0, 0, iconWidth, 
	       iconHeight, drawable, iconX, iconY);
    }
    if (cellPtr->textPtr != NULL) {
	TextStyle ts;
	XColor *color;
	Blt_Font font;
	int xMax;
	
	font = CHOOSE(viewPtr->font, stylePtr->font);
	if (EntryIsSelected(viewPtr, entryPtr)) {
	    color = CHOOSE(viewPtr->selection.fg, stylePtr->selectFg);
	} else if (entryPtr->color != NULL) {
	    color = entryPtr->color;
	} else {
	    color = fg;
	}
	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, font);
	Blt_Ts_SetForeground(ts, color);
	xMax = SCREENX(viewPtr, colPtr->worldX) + colPtr->width - 
	    colPtr->titleBW - colPtr->pad.side2 - stylePtr->arrowWidth;
	Blt_Ts_SetMaxLength(ts, xMax - textX);
	Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, cellPtr->textPtr, &ts, 
		textX, textY);
    }
    if (cellPtr == viewPtr->activeCellPtr) {
	bg = stylePtr->activeBg;
    } else {
	bg = colPtr->titleBg;
#ifdef notdef
	bg = CHOOSE(viewPtr->bg, stylePtr->normalBg);
#endif
    }
#ifdef notdef
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, arrowX, 
	arrowY + stylePtr->borderWidth, stylePtr->arrowWidth, 
	entryPtr->height - 2 * stylePtr->borderWidth, stylePtr->arrowBW, 
	stylePtr->arrowRelief); 
#endif
    Blt_DrawArrow(viewPtr->display, drawable, fg, arrowX, arrowY - 1, 
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
 *	The checkbox cell is drawn.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ComboBoxStyleEditProc(Cell *cellPtr, CellStyle *cellStylePtr)
{
    TreeView *viewPtr;

    viewPtr = cellStylePtr->viewPtr;
    return Blt_TreeView_CreateTextbox(viewPtr, cellPtr->entryPtr, 
                                      cellPtr->colPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleIdentifyProc --
 *
 *	Draws the "combobox" given the screen coordinates and the cell to
 *	be displayed.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox cell is drawn.
 *
 *---------------------------------------------------------------------------
 */
static const char *
ComboBoxStyleIdentifyProc(Cell *cellPtr, CellStyle *cellStylePtr, 
                          int x, int y)
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    int ax;
    unsigned int aw;
    Column *colPtr;

    colPtr = cellPtr->colPtr;
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
ComboBoxStylePostProc(Tcl_Interp *interp, Cell *cellPtr, 
                      CellStyle *cellStylePtr)
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    const char *menuName;
    Tk_Window tkwin;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    if (viewPtr->postPtr != NULL) {
	return TCL_OK;		       /* Another filter's menu is
					* currently posted. */
    }
    if (stylePtr->menuObjPtr == NULL) {
	return TCL_OK;			/* No menu associated with
                                         * filter. */
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
	    return TCL_ERROR;           /* Error invoking -postcommand */
	}
    }
    {
	Tcl_Obj *cmdObjPtr;
	int result;
	int x1, y1, x2, y2;
	int rootX, rootY;
        Column *colPtr;
        Entry *entryPtr;

        colPtr = cellPtr->colPtr;
        entryPtr = cellPtr->entryPtr;
	Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
	x1 = SCREENX(viewPtr, colPtr->worldX) + rootX;
	x2 = x1 + colPtr->width;
	y1 = SCREENY(viewPtr, entryPtr->worldY) + rootY;
	y2 = y1 + entryPtr->height;
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
    TreeView *viewPtr;

    if (stylePtr->menuObjPtr == NULL) {
	return TCL_OK;
    }
    viewPtr = stylePtr->viewPtr;
    viewPtr->postPtr = NULL;
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
 * ComboBoxStyleFreeProc --
 *
 *	Releases resources allocated for the combobox. The resources freed
 *	by this routine are specific only to the "combobox".  Other
 *	resources (common to all styles) are freed in the
 *	Blt_TreeView_FreeStyle routine.
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
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
    if (stylePtr->icon != NULL) {
	FreeIcon(stylePtr->icon);
    }
}

CellStyle *
Blt_TreeView_CreateStyle(Tcl_Interp *interp,
     TreeView *viewPtr,			/* Blt_TreeView_ widget. */
     int type,				/* Type of style: either
					 * STYLE_TEXTBOX,
					 * STYLE_COMBOBOX, or
					 * STYLE_CHECKBOX */
    const char *styleName,		/* Name of the new style. */
    int objc,
    Tcl_Obj *const *objv)
{    
    Blt_HashEntry *hPtr;
    int isNew;
    CellStyle *stylePtr;
    
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
	stylePtr = NewTextBoxStyle(viewPtr, hPtr);      break;
    case STYLE_COMBOBOX:
	stylePtr = NewComboBoxStyle(viewPtr, hPtr);     break;
    case STYLE_CHECKBOX:
	stylePtr = NewCheckBoxStyle(viewPtr, hPtr);     break;
    default:
	return NULL;
    }
    iconOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(interp, viewPtr->tkwin, styleName, 
	stylePtr->classPtr->className, stylePtr->classPtr->specsPtr, 
	objc, objv, (char *)stylePtr, 0) != TCL_OK) {
	(*stylePtr->classPtr->freeProc)(stylePtr);
	return NULL;
    }
    return stylePtr;
}

#endif /*TREEVIEW*/

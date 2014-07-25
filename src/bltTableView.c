/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTableView.c --
 *
 * This module implements an table viewer widget for the BLT toolkit.
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
 *	o -autocreate rows|columns|both|none for new table
 *	x Focus ring on cells.
 *	x Rules for row/columns (-rulewidth > 0)
 *      o Dashes for row/column rules.
 *	o -span for rows/columns titles 
 *	o Printing PS/PDF 
 *	x Underline text for active cells.
 *	x Imagebox cells
 *	x Combobox cells
 *	x Combobox column filters
 *	o Text editor widget for cells
 *	o XCopyArea for scrolling non-damaged areas.
 *	o Manage scrollbars geometry.
 *	o -padx -pady for styles 
 *	o -titlefont, -titlecolor, -activetitlecolor, -disabledtitlecolor 
 *	   for columns?
 *	o -justify period (1.23), comma (1,23), space (1.23 ev)
 *	o color icons for resize cursors
 */
/*
 * Known Bugs:
 *	o Row and column titles sometimes get drawn in the wrong location.
 *	o Don't handle hidden rows and columns in ComputeVisibility.
 *	o Too slow loading.  
 *	o Should be told what rows and columns are being added instead
 *	  of checking. Make that work with -whenidle.
 *	o Overallocate row and column arrays so that you don't have to
 *	  keep allocating a new array everytime.
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

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif	/* HAVE_LIMITS_H */

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "tclIntDecls.h"
#include "bltConfig.h"
#include "bltSwitch.h"
#include "bltNsUtil.h"
#include "bltVar.h"
#include "bltBg.h"
#include "bltFont.h"
#include "bltDataTable.h"
#include "bltBind.h"
#include "bltTableView.h"
#include "bltInitCmd.h"
#include "bltImage.h"
#include "bltText.h"
#include "bltOp.h"

#define TABLEVIEW_FIND_KEY "BLT TableView Find Command Interface"

#define TITLE_PADX		2
#define TITLE_PADY		1
#define FILTER_GAP		3

#define COLUMN_PAD		2
#define FOCUS_WIDTH		1
#define ICON_PADX		2
#define ICON_PADY		1
#define INSET_PAD		0
#define LABEL_PADX		0
#define LABEL_PADY		0

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define RESIZE_AREA		(8)
#define FCLAMP(x)	((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))

typedef ClientData (TagProc)(TableView *viewPtr, const char *string);

#define DEF_CELL_STATE			"normal"
#define DEF_CELL_STYLE			(char *)NULL

#define DEF_ACTIVE_TITLE_BG		STD_ACTIVE_BACKGROUND
#define DEF_ACTIVE_TITLE_FG		STD_ACTIVE_FOREGROUND
#define DEF_AUTO_CREATE			"0"
#define DEF_AUTO_FILTERS		"0"
#define DEF_BACKGROUND			STD_NORMAL_BACKGROUND
#define DEF_BIND_TAGS			"all"
#define DEF_BORDERWIDTH			STD_BORDERWIDTH
#define DEF_COLUMN_ACTIVE_TITLE_RELIEF	"raised"
#define DEF_COLUMN_COMMAND		(char *)NULL
#define DEF_COLUMN_EDIT			"yes"
#define DEF_COLUMN_FORMAT_COMMAND	(char *)NULL
#define DEF_COLUMN_HIDE			"no"
#define DEF_COLUMN_ICON			(char *)NULL
#define DEF_COLUMN_MAX			"0"		
#define DEF_COLUMN_MIN			"0"
#define DEF_COLUMN_NORMAL_TITLE_BG	STD_NORMAL_BACKGROUND
#define DEF_COLUMN_NORMAL_TITLE_FG	STD_NORMAL_FOREGROUND
#define DEF_COLUMN_RESIZE_CURSOR	"arrow"
#define DEF_COLUMN_PAD			"2"
#define DEF_COLUMN_SHOW			"yes"
#define DEF_COLUMN_STATE		"normal"
#define DEF_COLUMN_STYLE		(char *)NULL
#define DEF_COLUMN_TITLE_BORDERWIDTH	STD_BORDERWIDTH
#define DEF_COLUMN_TITLE_FONT		STD_FONT_NORMAL
#define DEF_COLUMN_TITLE_JUSTIFY	"center"
#define DEF_COLUMN_TITLE_RELIEF		"raised"
#define DEF_COLUMN_WEIGHT		"1.0"
#define DEF_COLUMN_WIDTH		"0"
#define DEF_DISABLED_TITLE_BG		STD_DISABLED_BACKGROUND
#define DEF_DISABLED_TITLE_FG		STD_DISABLED_FOREGROUND
#define DEF_EXPORT_SELECTION		"no"
#define DEF_FILTER_ACTIVE_BG		STD_ACTIVE_BACKGROUND
#define DEF_FILTER_ACTIVE_FG		RGB_BLACK
#define DEF_FILTER_ACTIVE_RELIEF	"raised"
#define DEF_FILTER_BORDERWIDTH		"1"
#define DEF_FILTER_HIGHLIGHT		"0"
#define DEF_FILTER_DISABLED_BG		STD_DISABLED_BACKGROUND
#define DEF_FILTER_DISABLED_FG		STD_DISABLED_FOREGROUND
#define DEF_FILTER_HIGHLIGHT_BG		RGB_GREY97 /*"#FFFFDD"*/
#define DEF_FILTER_HIGHLIGHT_FG		STD_NORMAL_FOREGROUND
#define DEF_FILTER_FONT			STD_FONT_NORMAL
#define DEF_FILTER_ICON			(char *)NULL
#define DEF_FILTER_MENU			(char *)NULL
#define DEF_FILTER_NORMAL_BG		RGB_WHITE
#define DEF_FILTER_NORMAL_FG		RGB_BLACK
#define DEF_FILTER_SELECT_BG		STD_SELECT_BACKGROUND
#define DEF_FILTER_SELECT_FG		STD_SELECT_FOREGROUND
#define DEF_FILTER_SELECT_RELIEF	"sunken"
#define DEF_FILTER_RELIEF		"solid"
#define DEF_FILTER_SHOW			"yes"
#define DEF_FILTER_STATE		"normal"
#define DEF_FILTER_TEXT			""
#define DEF_FOCUS_HIGHLIGHT_BG		STD_NORMAL_BACKGROUND
#define DEF_FOCUS_HIGHLIGHT_COLOR	RGB_BLACK
#define DEF_FOCUS_HIGHLIGHT_WIDTH	"2"
#define DEF_HEIGHT			"400"
#define DEF_RELIEF			"sunken"
#define DEF_ROW_ACTIVE_TITLE_RELIEF	"raised"
#define DEF_ROW_COMMAND			(char *)NULL
#define DEF_ROW_EDIT			"yes"
#define DEF_ROW_HEIGHT			"0"
#define DEF_ROW_HIDE			"no"
#define DEF_ROW_ICON			(char *)NULL
#define DEF_ROW_MAX			"0"		
#define DEF_ROW_MIN			"0"
#define DEF_ROW_NORMAL_TITLE_BG		STD_NORMAL_BACKGROUND
#define DEF_ROW_NORMAL_TITLE_FG		STD_NORMAL_FOREGROUND
#define DEF_ROW_RESIZE_CURSOR		"arrow"
#define DEF_ROW_SHOW			"yes"
#define DEF_ROW_STATE			"normal"
#define DEF_ROW_STYLE			(char *)NULL
#define DEF_ROW_TITLE_BORDERWIDTH	STD_BORDERWIDTH
#define DEF_ROW_TITLE_FONT		STD_FONT_SMALL
#define DEF_ROW_TITLE_JUSTIFY		"center"
#define DEF_ROW_TITLE_RELIEF		"raised"
#define DEF_ROW_WEIGHT			"1.0"
#define DEF_RULE_HEIGHT			"0"
#define DEF_RULE_WIDTH			"0"
#define DEF_SCROLL_INCREMENT		"20"
#define DEF_SCROLL_MODE			"hierbox"
#define DEF_SELECT_MODE			"single"
#define DEF_SORT_COLUMN			(char *)NULL
#define DEF_SORT_COLUMNS		(char *)NULL
#define DEF_SORT_COMMAND		(char *)NULL
#define DEF_SORT_DECREASING		"no"
#define DEF_SORT_DOWN_ICON		(char *)NULL
#define DEF_SORT_SELECTION		"no"
#define DEF_SORT_TYPE			"auto"
#define DEF_SORT_UP_ICON		(char *)NULL
#define DEF_STYLE			"default"
#define DEF_TABLE			(char *)NULL
#define DEF_TAKE_FOCUS			"1"
#define DEF_TITLES			"column"
#define DEF_WIDTH			"200"

static const char *sortTypeStrings[] = {
    "dictionary", "ascii", "integer", "real", "command", "none", "auto", NULL
};

enum SortTypeValues { 
    SORT_DICTIONARY, SORT_ASCII, SORT_INTEGER, 
    SORT_REAL, SORT_COMMAND, SORT_NONE, SORT_AUTO
};


static Blt_OptionParseProc ObjToAutoCreateProc;
static Blt_OptionPrintProc AutoCreateToObjProc;
static Blt_CustomOption autoCreateOption = {
    ObjToAutoCreateProc, AutoCreateToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToColumnTitleProc;
static Blt_OptionPrintProc ColumnTitleToObjProc;
static Blt_OptionFreeProc FreeColumnTitleProc;
static Blt_CustomOption columnTitleOption = {
    ObjToColumnTitleProc, ColumnTitleToObjProc, FreeColumnTitleProc, 
    (ClientData)0
};

static Blt_OptionParseProc ObjToSortColumnProc;
static Blt_OptionPrintProc SortColumnToObjProc;
static Blt_CustomOption sortColumnOption = {
    ObjToSortColumnProc, SortColumnToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToSortOrderProc;
static Blt_OptionPrintProc SortOrderToObjProc;
static Blt_OptionFreeProc FreeSortOrderProc;
static Blt_CustomOption sortOrderOption = {
    ObjToSortOrderProc, SortOrderToObjProc, FreeSortOrderProc, (ClientData)0
};
static Blt_OptionParseProc ObjToEnumProc;
static Blt_OptionPrintProc EnumToObjProc;
static Blt_CustomOption typeOption = {
    ObjToEnumProc, EnumToObjProc, NULL, (ClientData)sortTypeStrings
};
static Blt_OptionParseProc ObjToIconProc;
static Blt_OptionPrintProc IconToObjProc;
static Blt_OptionFreeProc FreeIconProc;
static Blt_CustomOption iconOption = {
    ObjToIconProc, IconToObjProc, FreeIconProc, 
    (ClientData)0,			/* Needs to point to the tableview
					 * widget before calling routines. */
};
static Blt_OptionParseProc ObjToRowTitleProc;
static Blt_OptionPrintProc RowTitleToObjProc;
static Blt_OptionFreeProc FreeRowTitleProc;
static Blt_CustomOption rowTitleOption = {
    ObjToRowTitleProc, RowTitleToObjProc, FreeRowTitleProc, (ClientData)0
};
static Blt_OptionParseProc ObjToScrollModeProc;
static Blt_OptionPrintProc ScrollModeToObjProc;
static Blt_CustomOption scrollModeOption = {
    ObjToScrollModeProc, ScrollModeToObjProc, NULL, NULL,
};

static Blt_OptionParseProc ObjToSelectModeProc;
static Blt_OptionPrintProc SelectModeToObjProc;
static Blt_CustomOption selectModeOption = {
    ObjToSelectModeProc, SelectModeToObjProc, NULL, NULL,
};

static Blt_OptionParseProc ObjToStateProc;
static Blt_OptionPrintProc StateToObjProc;
static Blt_CustomOption stateOption = {
    ObjToStateProc, StateToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToCellStateProc;
static Blt_OptionPrintProc CellStateToObjProc;
static Blt_CustomOption cellStateOption = {
    ObjToCellStateProc, CellStateToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToStyleProc;
static Blt_OptionPrintProc StyleToObjProc;
static Blt_OptionFreeProc FreeStyleProc;
static Blt_CustomOption styleOption = {
    ObjToStyleProc, StyleToObjProc, FreeStyleProc, 
    (ClientData)0,			/* Needs to point to the tableview
					 * widget before calling routines. */
};

static Blt_OptionParseProc ObjToTableProc;
static Blt_OptionPrintProc TableToObjProc;
static Blt_OptionFreeProc FreeTableProc;
static Blt_CustomOption tableOption = {
    ObjToTableProc, TableToObjProc, FreeTableProc, NULL,
};

static Blt_OptionParseProc ObjToTitlesProc;
static Blt_OptionPrintProc TitlesToObjProc;
static Blt_CustomOption titlesOption = {
    ObjToTitlesProc, TitlesToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToUidProc;
static Blt_OptionPrintProc UidToObjProc;
static Blt_OptionFreeProc FreeUidProc;
static Blt_CustomOption uidOption = {
    ObjToUidProc, UidToObjProc, FreeUidProc, NULL,
};

static Blt_ConfigSpec tableSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activecolumntitlebackground", 
	"activeColumnTitleBackground", "ActiveTitleBackground", 
	DEF_ACTIVE_TITLE_BG, Blt_Offset(TableView, colActiveTitleBg), 0},
    {BLT_CONFIG_COLOR, "-activecolumntitleforeground", 
	"activeColumnTitleForeground", "ActiveTitleForeground", 
	DEF_ACTIVE_TITLE_FG, Blt_Offset(TableView, colActiveTitleFg), 0},
    {BLT_CONFIG_BACKGROUND, "-activerowtitlebackground", 
	"activeRowTitleBackground", "ActiveTitleBackground", 
	DEF_ACTIVE_TITLE_BG, Blt_Offset(TableView, rowActiveTitleBg), 0},
    {BLT_CONFIG_COLOR, "-activerowtitleforeground",
	"activeRowTitleForeground", "ActiveTitleForeground", 
	DEF_ACTIVE_TITLE_FG, Blt_Offset(TableView, rowActiveTitleFg), 0},
    {BLT_CONFIG_CUSTOM, "-autocreate", "autoCreate", "AutoCreate",
	DEF_AUTO_CREATE, Blt_Offset(TableView, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, &autoCreateOption},
    {BLT_CONFIG_BITMASK, "-autofilters", "autoFilters", "AutoFilters",
	DEF_AUTO_FILTERS, Blt_Offset(TableView, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)AUTOFILTERS},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background", 
	DEF_BACKGROUND, Blt_Offset(TableView, bg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_BORDERWIDTH, Blt_Offset(TableView, borderWidth), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-columncommand", "columnCommand", "ColumnCommand", 
	DEF_COLUMN_COMMAND, Blt_Offset(TableView, colCmdObjPtr),
	BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_CURSOR, "-columnresizecursor", "columnResizeCursor", 
        "ResizeCursor", DEF_COLUMN_RESIZE_CURSOR, 
	Blt_Offset(TableView, colResizeCursor), 0},
    {BLT_CONFIG_BACKGROUND, "-columntitlebackground", "columnTitleBackground",
	"TitleBackground", DEF_COLUMN_NORMAL_TITLE_BG, 
	Blt_Offset(TableView, colNormalTitleBg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-columntitleborderwidth", 
	"columnTitleBorderWidth", "TitleBorderWidth", 
	DEF_COLUMN_TITLE_BORDERWIDTH, 
	Blt_Offset(TableView,colTitleBorderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_FONT, "-columntitlefont", "columnTitleFont", "TitleFont", 
	DEF_COLUMN_TITLE_FONT, Blt_Offset(TableView, colTitleFont), 0},
    {BLT_CONFIG_COLOR, "-columntitleforeground", "columnTitleForeground", 
	"TitleForeground", DEF_COLUMN_NORMAL_TITLE_FG, 
	Blt_Offset(TableView, colNormalTitleFg), 0},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor", (char *)NULL, 
	Blt_Offset(TableView, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-decreasingicon", "decreasingIcon","DecreasingIcon", 
	DEF_SORT_DOWN_ICON, Blt_Offset(TableView, sort.down), 
	BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &iconOption},
    {BLT_CONFIG_BACKGROUND, "-disabledcolumntitlebackground", 
	"diabledColumnTitleBackground", "DisabledTitleBackground", 
	DEF_DISABLED_TITLE_BG, Blt_Offset(TableView, colDisabledTitleBg), 0},
    {BLT_CONFIG_COLOR, "-disabledcolumntitleforeground", 
	"disabledColumnTitleForeground", "DisabledTitleForeground", 
	DEF_DISABLED_TITLE_FG, Blt_Offset(TableView, colDisabledTitleFg), 0},
    {BLT_CONFIG_BACKGROUND, "-disabledrowtitlebackground", 
	"diabledRowTitleBackground", "DisabledTitleBackground", 
	DEF_DISABLED_TITLE_BG, Blt_Offset(TableView, rowDisabledTitleBg), 0},
    {BLT_CONFIG_COLOR, "-disabledrowtitleforeground", 
	"disabledRowTitleForeground", "DisabledTitleForeground", 
	DEF_DISABLED_TITLE_FG, Blt_Offset(TableView, rowDisabledTitleFg), 0},
    {BLT_CONFIG_BITMASK, "-exportselection", "exportSelection", 
	"ExportSelection", DEF_EXPORT_SELECTION, Blt_Offset(TableView, flags),
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SELECT_EXPORT},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height", DEF_HEIGHT, 
	Blt_Offset(TableView, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_FOCUS_HIGHLIGHT_BG, 
        Blt_Offset(TableView, highlightBgColor), 0},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_FOCUS_HIGHLIGHT_COLOR, Blt_Offset(TableView, highlightColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_FOCUS_HIGHLIGHT_WIDTH, 
	Blt_Offset(TableView, highlightWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-rowcommand", "rowCommand", "RowCommand", 
	DEF_ROW_COMMAND, Blt_Offset(TableView, rowCmdObjPtr),
	BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_CURSOR, "-rowresizecursor", "rowResizeCursor","ResizeCursor", 
	DEF_ROW_RESIZE_CURSOR, Blt_Offset(TableView, rowResizeCursor), 0},
    {BLT_CONFIG_BACKGROUND, "-rowtitlebackground", "rowTitleBackground", 
	"TitleBackground", DEF_ROW_NORMAL_TITLE_BG, 
	Blt_Offset(TableView, rowNormalTitleBg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-rowtitleborderwidth", "rowTitleBorderWidth", 
	"TitleBorderWidth", DEF_ROW_TITLE_BORDERWIDTH, 
	Blt_Offset(TableView, rowTitleBorderWidth), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_FONT, "-rowtitlefont", "rowTitleFont", "TitleFont", 
	DEF_ROW_TITLE_FONT, Blt_Offset(TableView, rowTitleFont), 0},
    {BLT_CONFIG_COLOR, "-rowtitleforeground", "rowTitleForeground", 
	"RowTitleForeground", DEF_ROW_NORMAL_TITLE_FG, 
	Blt_Offset(TableView, rowNormalTitleFg), 0},
    {BLT_CONFIG_CUSTOM, "-scrollmode", "scrollMode", "ScrollMode",
	DEF_SCROLL_MODE, Blt_Offset(TableView, scrollMode),
	BLT_CONFIG_DONT_SET_DEFAULT, &scrollModeOption},
    {BLT_CONFIG_OBJ, "-selectcommand", "selectCommand", "selectCommand",
	(char *)NULL, Blt_Offset(TableView, selectCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-selectmode", "selectMode", "SelectMode",
	DEF_SELECT_MODE, Blt_Offset(TableView, selectMode), 
	BLT_CONFIG_DONT_SET_DEFAULT, &selectModeOption},
    {BLT_CONFIG_CUSTOM, "-increasingicon", "increaingIcon", "IncreasingIcon", 
	DEF_SORT_UP_ICON, Blt_Offset(TableView, sort.up), 
	BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &iconOption},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_RELIEF, 
	Blt_Offset(TableView, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-sortselection", "sortSelection", "SortSelection",
	DEF_SORT_SELECTION, Blt_Offset(TableView, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SELECT_SORTED},
    {BLT_CONFIG_CUSTOM, "-style", "style", "Style", DEF_STYLE, 
	Blt_Offset(TableView, stylePtr), BLT_CONFIG_DONT_SET_DEFAULT, 
	&styleOption},
    {BLT_CONFIG_CUSTOM, "-table", "table", "Table", DEF_TABLE,
	Blt_Offset(TableView, table), BLT_CONFIG_NULL_OK, &tableOption},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_TAKE_FOCUS, Blt_Offset(TableView, takeFocus), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-titles", "Titles", "Titles", DEF_TITLES, 
	Blt_Offset(TableView, flags), 0, (Blt_CustomOption *)&titlesOption},
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width", DEF_WIDTH, 
	Blt_Offset(TableView, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	(char *)NULL, Blt_Offset(TableView, xScrollCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-xscrollincrement", "xScrollIncrement", 
	"ScrollIncrement", DEF_SCROLL_INCREMENT, 
	Blt_Offset(TableView, xScrollUnits), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
	(char *)NULL, Blt_Offset(TableView, yScrollCmdObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-yscrollincrement", "yScrollIncrement", 
	"ScrollIncrement", DEF_SCROLL_INCREMENT, 
	Blt_Offset(TableView, yScrollUnits), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0}
};

static Blt_ConfigSpec columnSpecs[] =
{
    {BLT_CONFIG_RELIEF, "-activetitlerelief", "activeTitleRelief", 
	"TitleRelief", DEF_COLUMN_ACTIVE_TITLE_RELIEF, 
	Blt_Offset(Column, activeTitleRelief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags", DEF_BIND_TAGS, 
	Blt_Offset(Column, bindTags), BLT_CONFIG_NULL_OK, &uidOption},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", DEF_COLUMN_COMMAND, 
	Blt_Offset(Column, cmdObjPtr),
	BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_BITMASK_INVERT, "-edit", "edit", "Edit", DEF_COLUMN_EDIT, 
	Blt_Offset(Column, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)EDIT},
    {BLT_CONFIG_OBJ, "-filterdata", "filterData", "FilterData", (char *)NULL,
	Blt_Offset(Column, filterDataObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-filtericon", "filterIcon", "FilterIcon",
	DEF_FILTER_ICON, Blt_Offset(Column, filterIcon), 
	BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &iconOption},
    {BLT_CONFIG_OBJ, "-filtermenu", "filterMenu", "FilterMenu", 
	DEF_FILTER_MENU, Blt_Offset(Column, filterMenuObjPtr), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-filtertext", "filterText", "FilterText",
	DEF_FILTER_TEXT, Blt_Offset(Column, filterText), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-formatcommand", "formatCommand", "FormatCommand", 
	DEF_COLUMN_FORMAT_COMMAND, Blt_Offset(Column, fmtCmdObjPtr), 0},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_COLUMN_HIDE, 
	Blt_Offset(Column, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_BITMASK, "-filterhighlight", "filterhighliht", 
	"FilterHighlight", DEF_FILTER_HIGHLIGHT, 
	Blt_Offset(Column, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)FILTERHIGHLIGHT},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "icon", DEF_COLUMN_ICON, 
	Blt_Offset(Column, icon), 
	BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &iconOption},
    {BLT_CONFIG_PIXELS_NNEG, "-max", "max", "Max", DEF_COLUMN_MAX, 
	Blt_Offset(Column, reqMax), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-min", "min", "Min", DEF_COLUMN_MIN, 
	Blt_Offset(Column, reqMin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK_INVERT, "-show", "show", "Show", DEF_COLUMN_SHOW, 
	Blt_Offset(Column, flags), BLT_CONFIG_DONT_SET_DEFAULT,
	(Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_PIXELS_NNEG, "-rulewidth", "ruleWidth", "RuleWidth",
        DEF_RULE_WIDTH, Blt_Offset(Column, ruleWidth), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-sortcommand", "sortCommand", "SortCommand",
	DEF_SORT_COMMAND, Blt_Offset(Column, sortCmdObjPtr),
	BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-sortmode", "sortMode", "SortMode", DEF_SORT_TYPE, 
	Blt_Offset(Column, sortType), 0, &typeOption},
    {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_COLUMN_STATE, 
	Blt_Offset(Column, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	&stateOption},
    {BLT_CONFIG_CUSTOM, "-style", "style", "Style", DEF_COLUMN_STYLE, 
	Blt_Offset(Column, stylePtr), BLT_CONFIG_NULL_OK, &styleOption},
    {BLT_CONFIG_CUSTOM, "-title", "title", "Title", (char *)NULL, 
	Blt_Offset(Column, title), 
	BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &columnTitleOption},
    {BLT_CONFIG_JUSTIFY, "-titlejustify", "titleJustify", "TitleJustify", 
        DEF_COLUMN_TITLE_JUSTIFY, Blt_Offset(Column, titleJustify), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-titlerelief", "titleRelief", "TitleRelief", 
	DEF_COLUMN_TITLE_RELIEF, Blt_Offset(Column, titleRelief), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DOUBLE, "-weight", "weight", "Weight", DEF_COLUMN_WEIGHT, 
	Blt_Offset(Column, weight), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width", DEF_COLUMN_WIDTH, 
	Blt_Offset(Column, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

static Blt_ConfigSpec cellSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_CELL_STATE, 
	Blt_Offset(Cell, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	&cellStateOption},
    {BLT_CONFIG_CUSTOM, "-style", "style", "Style", DEF_CELL_STYLE, 
	Blt_Offset(Cell, stylePtr), BLT_CONFIG_NULL_OK, &styleOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec rowSpecs[] =
{
    {BLT_CONFIG_RELIEF, "-activetitlerelief", "activeTitleRelief", 
	"TitleRelief", DEF_ROW_ACTIVE_TITLE_RELIEF, 
	Blt_Offset(Row, activeTitleRelief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags", DEF_BIND_TAGS, 
	Blt_Offset(Row, bindTags), BLT_CONFIG_NULL_OK, &uidOption},
    {BLT_CONFIG_STRING, "-command", "command", "Command", DEF_ROW_COMMAND, 
        Blt_Offset(Row, cmdObjPtr), 
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_BITMASK_INVERT, "-edit", "edit", "Edit", DEF_ROW_EDIT, 
	Blt_Offset(Row, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)EDIT},
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height", DEF_ROW_HEIGHT, 
	Blt_Offset(Row, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_ROW_HIDE, 
	Blt_Offset(Row, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "icon", DEF_ROW_ICON, 
	Blt_Offset(Row, icon), 
	BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &iconOption},
    {BLT_CONFIG_PIXELS_NNEG, "-max", "max", "Max", DEF_ROW_MAX, 
	Blt_Offset(Row, reqMax), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-min", "min", "Min", DEF_ROW_MIN, 
	Blt_Offset(Row, reqMin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-ruleheight", "ruleHeight", "RuleHeight",
        DEF_RULE_HEIGHT, Blt_Offset(Row, ruleHeight), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK_INVERT, "-show", "show", "Show", DEF_ROW_SHOW, 
	Blt_Offset(Row, flags), BLT_CONFIG_DONT_SET_DEFAULT,
	(Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_ROW_STATE, 
	Blt_Offset(Row, flags), BLT_CONFIG_DONT_SET_DEFAULT, &stateOption},
    {BLT_CONFIG_CUSTOM, "-style", "style", "Style", DEF_ROW_STYLE, 
	Blt_Offset(Row, stylePtr), BLT_CONFIG_NULL_OK, &styleOption},
    {BLT_CONFIG_CUSTOM, "-title", "title", "Title", (char *)NULL, 
	Blt_Offset(Row, title), 
	BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &rowTitleOption},
    {BLT_CONFIG_JUSTIFY, "-titlejustify", "titleJustify", "TitleJustify", 
        DEF_ROW_TITLE_JUSTIFY, Blt_Offset(Row, titleJustify), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-titlerelief", "titleRelief", "TitleRelief",
	DEF_ROW_TITLE_RELIEF, Blt_Offset(Row, titleRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DOUBLE, "-weight", "weight", "Weight", DEF_ROW_WEIGHT, 
	Blt_Offset(Row, weight), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

static Blt_ConfigSpec sortSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-columns", "columns", "columns",
	DEF_SORT_COLUMNS, Blt_Offset(TableView, sort.order),
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &sortOrderOption},
    {BLT_CONFIG_BOOLEAN, "-decreasing", "decreasing", "Decreasing",
	DEF_SORT_DECREASING, Blt_Offset(TableView, sort.decreasing),
        BLT_CONFIG_DONT_SET_DEFAULT}, 
    {BLT_CONFIG_CUSTOM, "-mark", "mark", "mark",
	DEF_SORT_COLUMN, Blt_Offset(TableView, sort.firstPtr),
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &sortColumnOption},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

static Blt_ConfigSpec filterSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_FILTER_ACTIVE_BG, 
	Blt_Offset(TableView, filter.activeBg), 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
	"ActiveForeground", DEF_FILTER_ACTIVE_FG, 
	Blt_Offset(TableView, filter.activeFg), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "ActiveRelief", 
	DEF_FILTER_ACTIVE_RELIEF, Blt_Offset(TableView, filter.activeRelief), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background", 
	DEF_FILTER_NORMAL_BG, Blt_Offset(TableView, filter.normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth", 
	DEF_FILTER_BORDERWIDTH, Blt_Offset(TableView, filter.borderWidth), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_BORDERWIDTH, Blt_Offset(TableView, filter.borderWidth), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-disabledbackground", "diabledBackground", 
	"DisabledBackground", DEF_FILTER_DISABLED_BG, 
	Blt_Offset(TableView, filter.disabledBg), 0},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground", 
	"DisabledForeground", DEF_FILTER_DISABLED_FG, 
	Blt_Offset(TableView, filter.disabledFg), 0},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_FILTER_FONT, 
	Blt_Offset(TableView, filter.font), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", 
	DEF_FILTER_NORMAL_FG, Blt_Offset(TableView, filter.normalFg), 0},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide",
	DEF_AUTO_FILTERS, Blt_Offset(TableView, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)AUTOFILTERS},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground", 
	"HighlightBackground", DEF_FILTER_HIGHLIGHT_BG, 
	Blt_Offset(TableView, filter.highlightBg), 0},
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", 
	"HighlightForeground", DEF_FILTER_HIGHLIGHT_FG, 
	Blt_Offset(TableView, filter.highlightFg), 0},
    {BLT_CONFIG_OBJ, "-menu", "menu", "Menu", DEF_FILTER_MENU,
	Blt_Offset(TableView, filter.menuObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_RELIEF, "-postedrelief", "postedRelief", "PostedRelief", 
	DEF_FILTER_SELECT_RELIEF, Blt_Offset(TableView, filter.selectRelief), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_FILTER_RELIEF, 
	Blt_Offset(TableView, filter.relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK_INVERT, "-show", "show", "Show", DEF_FILTER_SHOW, 
	Blt_Offset(TableView, flags), BLT_CONFIG_DONT_SET_DEFAULT,
	(Blt_CustomOption *)AUTOFILTERS},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
	"SelectBackground", DEF_FILTER_SELECT_BG, 
	Blt_Offset(TableView, filter.selectBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", 
	"SelectForeground", DEF_FILTER_SELECT_FG, 
	Blt_Offset(TableView, filter.selectFg), 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0}
};

static Blt_HashTable findTable;
static int initialized = FALSE;

/* Forward Declarations */
static BLT_TABLE_NOTIFY_EVENT_PROC TableEventProc;
static Blt_BindPickProc TableViewPickProc;
static Blt_BindAppendTagsProc AppendTagsProc;
static Tcl_CmdDeleteProc TableViewInstCmdDeleteProc;
static Tcl_FreeProc TableViewFreeProc;
static Tcl_FreeProc RowFreeProc;
static Tcl_FreeProc ColumnFreeProc;
static Tcl_FreeProc CellFreeProc;
static Tcl_IdleProc DisplayProc;
static Tcl_ObjCmdProc TableViewCmdProc;
static Tcl_ObjCmdProc TableViewInstObjCmdProc;
static Tk_EventProc TableViewEventProc;
static Tk_ImageChangedProc IconChangedProc;
static Tk_SelectionProc SelectionProc;

static void ComputeGeometry(TableView *viewPtr);
static void ComputeLayout(TableView *viewPtr);
static void ComputeVisibleEntries(TableView *viewPtr);
static int GetColumn(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, 
	Column **colPtrPtr);
static int GetColumns(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, 
	Blt_Chain *columnsPtr);
static int GetRow(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, 
	Row **rowPtrPtr);
static int GetRows(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, 
	Blt_Chain *rowsPtr);
static int AttachTable(Tcl_Interp *interp, TableView *viewPtr);
static void RebuildTableView(TableView *viewPtr);
static void AddColumns(TableView *viewPtr, BLT_TABLE_NOTIFY_EVENT *eventPtr);
static void AddRows(TableView *viewPtr, BLT_TABLE_NOTIFY_EVENT *eventPtr);
static void DeleteColumns(TableView *viewPtr, BLT_TABLE_NOTIFY_EVENT *eventPtr);
static void DeleteRows(TableView *viewPtr, BLT_TABLE_NOTIFY_EVENT *eventPtr);

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *	Queues a request to redraw the widget at the next idle point.  A
 *	new idle event procedure is queued only if the there's isn't one
 *	already queued and updates are turned on.
 *
 *	The DONT_UPDATE flag lets the user to turn off redrawing the
 *	tableview while changes are happening to the table itself.
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
EventuallyRedraw(TableView *viewPtr)
{
    viewPtr->flags |= REDRAW;
    if ((viewPtr->tkwin != NULL) && 
	((viewPtr->flags & (DONT_UPDATE|REDRAW_PENDING)) == 0)) {
	viewPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayProc, viewPtr);
    }
}

void
Blt_TableView_EventuallyRedraw(TableView *viewPtr)
{
    EventuallyRedraw(viewPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * PossiblyRedraw --
 *
 *	Queues a request to redraw the widget at the next idle point.  A
 *	new idle event procedure is queued only if the there's isn't one
 *	already queued and updates are turned on.
 *
 *	The DONT_UPDATE flag lets the user to turn off redrawing the
 *	tableview while changes are happening to the table itself.
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
PossiblyRedraw(TableView *viewPtr)
{
    if ((viewPtr->tkwin != NULL) && 
	((viewPtr->flags & (DONT_UPDATE|REDRAW_PENDING)) == 0)) {
	viewPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayProc, viewPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectCommandProc --
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
SelectCommandProc(ClientData clientData) 
{
    TableView *viewPtr = clientData;
    Tcl_Obj *cmdObjPtr;

    viewPtr->flags &= ~SELECT_PENDING;
    cmdObjPtr = viewPtr->selectCmdObjPtr;
    if (cmdObjPtr != NULL) {
	Tcl_Preserve(viewPtr);
	if (Tcl_EvalObjEx(viewPtr->interp, cmdObjPtr, TCL_EVAL_GLOBAL) 
	    != TCL_OK) {
	    Tcl_BackgroundError(viewPtr->interp);
	}
	Tcl_Release(viewPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyInvokeSelectCommand --
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
EventuallyInvokeSelectCommand(TableView *viewPtr)
{
    if ((viewPtr->flags & SELECT_PENDING) == 0) {
	viewPtr->flags |= SELECT_PENDING;
	Tcl_DoWhenIdle(SelectCommandProc, viewPtr);
    }
}

static void
ClearSelections(TableView *viewPtr)
{
    if (viewPtr->selectMode == SELECT_CELLS) {
	viewPtr->selectCells.anchorPtr = viewPtr->selectCells.markPtr = NULL;
    } else {
	int i;

	for (i = 0; i < viewPtr->numRows; i++) {
	    Row *rowPtr;

	    rowPtr = viewPtr->rows[i];
	    rowPtr->flags &= ~SELECTED;
	    rowPtr->link = NULL;
	}
	Blt_Chain_Reset(viewPtr->selectRows.list);
    }
    EventuallyRedraw(viewPtr);
    if (viewPtr->selectCmdObjPtr != NULL) {
	EventuallyInvokeSelectCommand(viewPtr);
    }
}

static TableView *tableViewInstance;

static int
CompareValues(Column *colPtr, const Row *r1Ptr, const Row *r2Ptr)
{
    TableView *viewPtr;
    const char *s1, *s2;
    long l1, l2;
    double d1, d2;
    int result;
    int sortType;
    BLT_TABLE_COLUMN col;
    BLT_TABLE_ROW r1, r2;

    viewPtr = colPtr->viewPtr;
    col = colPtr->column;
    r1 = r1Ptr->row;
    r2 = r2Ptr->row;

    /* Check for empty values. */
    if (!blt_table_value_exists(viewPtr->table, r1, col)) {
	if (!blt_table_value_exists(viewPtr->table, r2, col)) {
	    return 0;
	}
	return 1;
    } else if (!blt_table_value_exists(viewPtr->table, r2, col)) {
	return -1;
    }

    result = 0;
    sortType = colPtr->sortType;
    if (sortType == SORT_AUTO) {
	switch (blt_table_column_type(col)) {
	case TABLE_COLUMN_TYPE_STRING:
	    sortType = SORT_DICTIONARY;	break;
	case TABLE_COLUMN_TYPE_INT:
	    sortType = SORT_INTEGER;	break;
	case TABLE_COLUMN_TYPE_DOUBLE:
	    sortType = SORT_REAL;	break;
	default:
	    sortType = SORT_DICTIONARY;	break;
	}
    }
    switch (sortType) {
    case SORT_ASCII:
	s1 = blt_table_get_string(viewPtr->table, r1, col);
	s2 = blt_table_get_string(viewPtr->table, r2, col);
	result = strcmp(s1, s2);
	break;
    case SORT_DICTIONARY:
	s1 = blt_table_get_string(viewPtr->table, r1, col);
	s2 = blt_table_get_string(viewPtr->table, r2, col);
	result = Blt_DictionaryCompare(s1, s2);
	break;
    case SORT_INTEGER:
	l1 = blt_table_get_long(viewPtr->table, r1, col, 0);
	l2 = blt_table_get_long(viewPtr->table, r2, col, 0);
	result = l1 - l2;
	break;
    case SORT_REAL:
	d1 = blt_table_get_double(viewPtr->table, r1, col);
	d2 = blt_table_get_double(viewPtr->table, r2, col);
	result = (d1 > d2) ? 1 : (d1 < d2) ? -1 : 0;
	break;
    }
    return result;
}


/*
 *---------------------------------------------------------------------------
 *
 * CompareRows --
 *
 *	Comparison routine (used by qsort) to sort a chain of subnodes.
 *
 * Results:
 *	1 is the first is greater, -1 is the second is greater, 0
 *	if equal.
 *
 *---------------------------------------------------------------------------
 */
static int
CompareRows(const void *a, const void *b)
{
    const Row *r1Ptr = *(Row **)a;
    const Row *r2Ptr = *(Row **)b;
    TableView *viewPtr;
    int result;
    SortInfo *sortPtr;
    Blt_ChainLink link;

    viewPtr = tableViewInstance;
    sortPtr = &viewPtr->sort;
    result = 0;				/* Suppress compiler warning. */
    for (link = Blt_Chain_FirstLink(sortPtr->order); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
	Column *colPtr;

	colPtr = Blt_Chain_GetValue(link);
	/* Fetch the data for sorting. */
	if ((colPtr->sortType == SORT_COMMAND) && 
	    (colPtr->sortCmdObjPtr != NULL)) {
	    Tcl_Interp *interp;
	    Tcl_Obj *objPtr, *cmdObjPtr, *resultObjPtr;

	    interp = viewPtr->interp;
	    cmdObjPtr = Tcl_DuplicateObj(colPtr->sortCmdObjPtr);
	    /* Table name */
	    objPtr = Tcl_NewStringObj(blt_table_name(viewPtr->table), -1);
	    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	    /* Row */
	    objPtr = Tcl_NewLongObj(blt_table_row_index(r1Ptr->row));
	    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	    /* Column */
	    objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
	    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	    /* Row */
	    objPtr = Tcl_NewLongObj(blt_table_row_index(r2Ptr->row));
	    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	    /* Column */
	    objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
	    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);

	    Tcl_IncrRefCount(cmdObjPtr);
	    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
	    Tcl_DecrRefCount(cmdObjPtr);
	    if (result != TCL_OK) {
		Tcl_BackgroundError(interp);
	    }
	    resultObjPtr = Tcl_GetObjResult(interp);
	    if (Tcl_GetIntFromObj(interp, resultObjPtr, &result) != TCL_OK) {
		Tcl_BackgroundError(interp);
	    }
	} else {
	    result = CompareValues(colPtr, r1Ptr, r2Ptr);
	}
	if (result != 0) {
	    break;
	}
    }
    if (sortPtr->decreasing) {
	return -result;
    } 
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortTableView --
 *
 *	Sorts the flatten array of entries.
 *
 *---------------------------------------------------------------------------
 */
static void
SortTableView(TableView *viewPtr)
{
    SortInfo *sortPtr = &viewPtr->sort;

    tableViewInstance = viewPtr;
    viewPtr->sort.flags &= ~SORT_PENDING;
    if (viewPtr->numRows < 2) {
	return;
    }
    if (sortPtr->flags & SORTED) {
	long first, last;

	if (sortPtr->decreasing == sortPtr->viewIsDecreasing) {
	    return;
	}
	/* 
	 * The view is already sorted but in the wrong direction.  Reverse
	 * the entries in the array.
	 */
 	for (first = 0, last = viewPtr->numRows - 1; last > first; 
	     first++, last--) {
	    Row *hold;

	    hold = viewPtr->rows[first];
	    viewPtr->rows[first] = viewPtr->rows[last];
	    viewPtr->rows[last] = hold;
	}
	sortPtr->viewIsDecreasing = sortPtr->decreasing;
	sortPtr->flags |= SORTED;
	viewPtr->flags |= LAYOUT_PENDING;
	return;
    }
    qsort((char *)viewPtr->rows, viewPtr->numRows, sizeof(Row *),
	(QSortCompareProc *)CompareRows);

    sortPtr->viewIsDecreasing = sortPtr->decreasing;
    sortPtr->flags |= SORTED;
    viewPtr->flags |= LAYOUT_PENDING;
}

static void
DestroyStyles(TableView *viewPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&viewPtr->styleTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	CellStyle *stylePtr;

	stylePtr = Blt_GetHashValue(hPtr);
	stylePtr->hashPtr = NULL;
	(*stylePtr->classPtr->freeProc)(stylePtr);
    }
    Blt_DeleteHashTable(&viewPtr->styleTable);
}

static int
GetStyle(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, 
	 CellStyle **stylePtrPtr)
{
    const char *string;
    Blt_HashEntry *hPtr;

    string = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&viewPtr->styleTable, string);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find style \"", 
		Tcl_GetString(objPtr),
		"\" in \"", Tk_PathName(viewPtr->tkwin), "\"", 
		(char *)NULL);
	}
	return TCL_ERROR;
    }
    *stylePtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetUid --
 *
 *	Gets or creates a unique string identifier.  Strings are reference
 *	counted.  The string is placed into a hashed table local to the
 *	tableview.
 *
 * Results:
 *	Returns the pointer to the hashed string.
 *
 *---------------------------------------------------------------------------
 */
static UID
GetUid(TableView *viewPtr, const char *string)
{
    Blt_HashEntry *hPtr;
    int isNew;
    size_t refCount;

    hPtr = Blt_CreateHashEntry(&viewPtr->uidTable, string, &isNew);
    if (isNew) {
	refCount = 1;
    } else {
	refCount = (size_t)Blt_GetHashValue(hPtr);
	refCount++;
    }
    Blt_SetHashValue(hPtr, refCount);
    return Blt_GetHashKey(&viewPtr->uidTable, hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeUid --
 *
 *	Releases the uid.  Uids are reference counted, so only when the
 *	reference count is zero (i.e. no one else is using the string) is
 *	the entry removed from the hash table.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeUid(TableView *viewPtr, UID uid)
{
    Blt_HashEntry *hPtr;
    size_t refCount;

    hPtr = Blt_FindHashEntry(&viewPtr->uidTable, uid);
    assert(hPtr != NULL);
    refCount = (size_t)Blt_GetHashValue(hPtr);
    refCount--;
    if (refCount > 0) {
	Blt_SetHashValue(hPtr, refCount);
    } else {
	Blt_DeleteHashEntry(&viewPtr->uidTable, hPtr);
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
IconChangedProc(ClientData clientData, int x, int y, int width, int height,
		int imageWidth, int imageHeight)	
{
    TableView *viewPtr = clientData;

    viewPtr->flags |= (GEOMETRY | LAYOUT_PENDING);
    EventuallyRedraw(viewPtr);
}

static Icon
GetIcon(TableView *viewPtr, const char *iconName)
{
    Blt_HashEntry *hPtr;
    int isNew;
    struct _Icon *iconPtr;

    hPtr = Blt_CreateHashEntry(&viewPtr->iconTable, iconName, &isNew);
    if (isNew) {
	Tk_Image tkImage;
	int width, height;

	tkImage = Tk_GetImage(viewPtr->interp, viewPtr->tkwin,(char *)iconName, 
		IconChangedProc, viewPtr);
	if (tkImage == NULL) {
	    Blt_DeleteHashEntry(&viewPtr->iconTable, hPtr);
	    return NULL;
	}
	Tk_SizeOfImage(tkImage, &width, &height);
	iconPtr = Blt_AssertMalloc(sizeof(struct _Icon));
	iconPtr->viewPtr = viewPtr;
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

static void
DestroyIcons(TableView *viewPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    struct _Icon *iconPtr;

    for (hPtr = Blt_FirstHashEntry(&viewPtr->iconTable, &iter);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	iconPtr = Blt_GetHashValue(hPtr);
	Tk_FreeImage(iconPtr->tkImage);
	Blt_Free(iconPtr);
    }
    Blt_DeleteHashTable(&viewPtr->iconTable);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToAutoCreateProc --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToAutoCreateProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		    Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    char c;
    const char *string;
    int *flagsPtr = (int*)(widgRec + offset);
    int length, mask;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'b') && (strncmp(string, "both", length) == 0)) {
	mask = AUTO_ROWS | AUTO_COLUMNS;
    } else if ((c == 'c') && (strncmp(string, "columns", length) == 0)) {
	mask = AUTO_COLUMNS;
    } else if ((c == 'r') && (strncmp(string, "rows", length) == 0)) {
	mask = AUTO_ROWS;
    } else if ((c == 'n') && (strncmp(string, "none", length) == 0)) {
	mask = 0;
    } else {
	Tcl_AppendResult(interp, "unknown autocreate value \"", string, 
		"\": should be both, columns, rows, or none", (char *)NULL);
	return TCL_ERROR;
    }
    *flagsPtr &= AUTOCREATE;
    *flagsPtr |= mask;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AutoCreateToObjProc --
 *
 *	Returns the current -autocreate value as a string.
 *
 * Results:
 *	The TCL string object is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
AutoCreateToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		     char *widgRec, int offset, int flags)	
{
    int mask = *(int *)(widgRec + offset);
    const char *string;

    mask &= AUTOCREATE;
    if (mask == AUTOCREATE) {
	string = "both";
    } else if (mask == AUTO_ROWS) {
	string = "rows";
    } else if (mask == AUTO_COLUMNS) {
	string = "columns";
    } else if (mask == 0) {
	string = "none";
    } else {
	string = "???";
    }
    return Tcl_NewStringObj(string, -1);
}

/*ARGSUSED*/
static void
FreeColumnTitleProc(ClientData clientData, Display *display, char *widgRec, 
		    int offset)
{
    Column *colPtr = (Column *)widgRec;
    const char **stringPtr = (const char **)(widgRec + offset);

    if (*stringPtr != NULL) {
	if (colPtr->flags & TEXTALLOC) {
	    Blt_Free(*stringPtr);
	}
	*stringPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToColumnTitleProc --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColumnTitleProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		     Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    Column *colPtr = (Column *)widgRec;
    const char **stringPtr = (const char **)(widgRec + offset);
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    if (colPtr->flags & TEXTALLOC) {
	Blt_Free(*stringPtr);
    }
    if (length == 0) {			/* Revert back to the row title */
	*stringPtr = blt_table_column_label(colPtr->column);
	colPtr->flags &= ~TEXTALLOC;
	return TCL_OK;
    } else {
	*stringPtr = Blt_AssertStrdup(string);
	colPtr->flags |= TEXTALLOC;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnTitleToObjProc --
 *
 *	Returns the current column title as a string.
 *
 * Results:
 *	The title is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ColumnTitleToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		     char *widgRec, int offset, int flags)	
{
    const char *string = *(char **)(widgRec + offset);

    return Tcl_NewStringObj(string, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSortColumnProc --
 *
 *	Convert the string reprsenting a column, to its numeric form.
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
ObjToSortColumnProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		    Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    TableView *viewPtr = (TableView *)widgRec;
    Column **colPtrPtr = (Column **)(widgRec + offset);
    Column *colPtr;

    if (GetColumn(interp, viewPtr, objPtr, &colPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    *colPtrPtr = colPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortColumnToObjProc --
 *
 * Results:
 *	The string representation of the column is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SortColumnToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		    char *widgRec, int offset, int flags)	
{
    Column *colPtr = *(Column **)(widgRec + offset);

    if (colPtr == NULL) {
	return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewLongObj(blt_table_column_index(colPtr->column));
}


/*ARGSUSED*/
static void
FreeSortOrderProc(ClientData clientData, Display *display, char *widgRec, 
		  int offset)
{
    Blt_Chain *chainPtr = (Blt_Chain *)(widgRec + offset);

    if (*chainPtr != NULL) {
	Blt_Chain_Destroy(*chainPtr);
	*chainPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSortOrderProc --
 *
 *	Convert the string reprsenting a column, to its numeric form.
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
ObjToSortOrderProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		   Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    TableView *viewPtr = (TableView *)widgRec;
    Blt_Chain *chainPtr = (Blt_Chain *)(widgRec + offset);
    Blt_Chain chain;
    int i, objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    chain = Blt_Chain_Create();
    for (i = 0; i < objc; i++) {
	Column *colPtr;

	if (GetColumn(interp, viewPtr, objv[i], &colPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (colPtr == NULL) {
	    fprintf(stderr, "ObjToColumns: Column %s is NULL\n", 
		    Tcl_GetString(objv[i])); 
	    continue;
	}
	Blt_Chain_Append(chain, colPtr);
    }
    if (*chainPtr != NULL) {
	Blt_Chain_Destroy(*chainPtr);
    }
    *chainPtr = chain;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortOrderToObjProc --
 *
 * Results:
 *	The string representation of the column is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SortOrderToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		   char *widgRec, int offset, int flags)	
{
    Blt_Chain chain = *(Blt_Chain *)(widgRec + offset);
    Blt_ChainLink link;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	Column *colPtr;
	Tcl_Obj *objPtr;

	colPtr = Blt_Chain_GetValue(link);
	objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToEnumProc --
 *
 *	Converts the string into its enumerated type.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToEnumProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
	      Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    int *enumPtr = (int *)(widgRec + offset);
    char c;
    char **p;
    int i;
    int count;
    char *string;

    string = Tcl_GetString(objPtr);
    c = string[0];
    count = 0;
    for (p = (char **)clientData; *p != NULL; p++) {
	if ((c == p[0][0]) && (strcmp(string, *p) == 0)) {
	    *enumPtr = count;
	    return TCL_OK;
	}
	count++;
    }
    *enumPtr = -1;

    Tcl_AppendResult(interp, "bad value \"", string, "\": should be ", 
	(char *)NULL);
    p = (char **)clientData; 
    if (count > 0) {
	Tcl_AppendResult(interp, p[0], (char *)NULL);
    }
    for (i = 1; i < (count - 1); i++) {
	Tcl_AppendResult(interp, " ", p[i], ", ", (char *)NULL);
    }
    if (count > 1) {
	Tcl_AppendResult(interp, " or ", p[count - 1], ".", (char *)NULL);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * EnumToObjProc --
 *
 *	Returns the string associated with the enumerated type.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
EnumToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
	      char *widgRec, int offset, int flags)	
{
    int value = *(int *)(widgRec + offset);
    char **strings = (char **)clientData;
    char **p;
    int count;

    count = 0;
    for (p = strings; *p != NULL; p++) {
	if (value == count) {
	    return Tcl_NewStringObj(*p, -1);
	}
	count++;
    }
    return Tcl_NewStringObj("unknown value", -1);
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
    TableView *viewPtr = clientData;
    Icon *iconPtr = (Icon *)(widgRec + offset);
    Icon icon;
    int length;
    const char *string;

    string = Tcl_GetStringFromObj(objPtr, &length);
    icon = NULL;
    if (length > 0) {
	icon = GetIcon(viewPtr, string);
	if (icon == NULL) {
	    return TCL_ERROR;
	}
    }
    if (*iconPtr != NULL) {
	FreeIcon(*iconPtr);
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

    if (icon == NULL) {
	return Tcl_NewStringObj("", -1);
    } 
    return Tcl_NewStringObj(Blt_Image_Name((icon)->tkImage), -1);
}
/*ARGSUSED*/
static void
FreeRowTitleProc(ClientData clientData, Display *display, char *widgRec, 
		 int offset)
{
    Row *rowPtr = (Row *)widgRec;
    const char **stringPtr = (const char **)(widgRec + offset);

    if (*stringPtr != NULL) {
	if (rowPtr->flags & TEXTALLOC) {
	    Blt_Free(*stringPtr);
	    rowPtr->flags &= ~TEXTALLOC;
	}
	*stringPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToRowTitleProc --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToRowTitleProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		  Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    Row *rowPtr = (Row *)widgRec;
    const char **stringPtr = (const char **)(widgRec + offset);
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    if (rowPtr->flags & TEXTALLOC) {
	Blt_Free(*stringPtr);
	rowPtr->flags &= ~TEXTALLOC;
    }
    if (length == 0) {			/* Revert back to the row title */
	*stringPtr = blt_table_row_label(rowPtr->row);
	rowPtr->flags &= ~TEXTALLOC;
	return TCL_OK;
    } else {
	*stringPtr = Blt_AssertStrdup(string);
	rowPtr->flags |= TEXTALLOC;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowTitleToObjProc --
 *
 *	Returns the current row title as a string.
 *
 * Results:
 *	The title is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
RowTitleToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		char *widgRec, int offset, int flags)	
{
    const char *string = *(char **)(widgRec + offset);

    return Tcl_NewStringObj(string, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToScrollModeProc --
 *
 *	Convert the string reprsenting a scroll mode, to its numeric form.
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
ObjToScrollModeProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		    Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    char c;
    const char *string;
    int *modePtr = (int *)(widgRec + offset);
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'l') && (strncmp(string, "listbox", length) == 0)) {
	*modePtr = BLT_SCROLL_MODE_LISTBOX;
    } else if ((c == 't') && (strncmp(string, "hierbox", length) == 0)) {
	*modePtr = BLT_SCROLL_MODE_HIERBOX;
    } else if ((c == 'c') && (strncmp(string, "canvas", length) == 0)) {
	*modePtr = BLT_SCROLL_MODE_CANVAS;
    } else {
	Tcl_AppendResult(interp, "bad scroll mode \"", string,
		"\": should be tableview, listbox, or canvas.", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollModeToObjProc --
 *
 * Results:
 *	The string representation of the scroll mode is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ScrollModeToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin, 
		    char *widgRec, int offset, int flags)	
{
    int mode = *(int *)(widgRec + offset);

    switch (mode) {
    case BLT_SCROLL_MODE_LISTBOX: 
	return Tcl_NewStringObj("listbox", 7);
    case BLT_SCROLL_MODE_HIERBOX: 
	return Tcl_NewStringObj("hierbox", 9);
    case BLT_SCROLL_MODE_CANVAS:  
	return Tcl_NewStringObj("canvas", 6);
    default:
	return Tcl_NewStringObj("???", 3);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSelectModeProc --
 *
 *	Convert the string reprsenting a scroll mode, to its numeric form.
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
ObjToSelectModeProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		    Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    const char *string;
    char c;
    int *modePtr = (int *)(widgRec + offset);
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 's') && (strncmp(string, "single", length) == 0)) {
	*modePtr = SELECT_SINGLE_ROW;
    } else if ((c == 'm') && (strncmp(string, "multiple", length) == 0)) {
	*modePtr = SELECT_MULTIPLE_ROWS;
    } else if ((c == 'c') && (strncmp(string, "cells", length) == 0)) {
	*modePtr = SELECT_CELLS;
    } else {
	Tcl_AppendResult(interp, "bad select mode \"", string,
	    "\": should be single, multiple, or cells.",(char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectModeToObjProc --
 *
 * Results:
 *	The string representation of the select mode is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SelectModeToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		    char *widgRec, int offset, int flags)	
{
    int mode = *(int *)(widgRec + offset);

    switch (mode) {
    case SELECT_SINGLE_ROW:
	return Tcl_NewStringObj("single", 6);
    case SELECT_MULTIPLE_ROWS:
	return Tcl_NewStringObj("multiple", 8);
    case SELECT_CELLS:
	return Tcl_NewStringObj("cells", 5);
    default:
	return Tcl_NewStringObj("???", 3);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToStateProc --
 *
 *	Convert the name of a state into an integer.
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
ObjToStateProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
	       Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    int *flagsPtr = (int *)(widgRec + offset);
    const char *string;
    char c;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'n') && (strncmp(string, "normal", length) == 0)) {
	*flagsPtr &= ~(DISABLED|HIGHLIGHT);
    } else if ((c == 'd') && (strncmp(string, "disabled", length) == 0)) {
	*flagsPtr &= ~(DISABLED|HIGHLIGHT);
	*flagsPtr |= DISABLED;
    } else if ((c == 'h') && (strncmp(string, "highlighted", length) == 0)) {
	*flagsPtr &= ~(DISABLED|HIGHLIGHT);
	*flagsPtr |= HIGHLIGHT;
    } else {
	Tcl_AppendResult(interp, "invalid state \"", string, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StateToObjProc --
 *
 *	Converts the state into its string representation.
 *
 * Results:
 *	The name of the state is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
StateToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
	       char *widgRec, int offset, int flags)	
{
    int state = *(int *)(widgRec + offset);

    if (state & DISABLED) {
	return Tcl_NewStringObj("disabled", 8);
    } 
    if (state & HIGHLIGHT) {
	return Tcl_NewStringObj("highlighted", 11);
    } 
    return Tcl_NewStringObj("normal", 6);
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToCellStateProc --
 *
 *	Converts the string representing a cell state into a bitflag.
 *
 * Results:
 *	The return value is a standard TCL result.  The state flags are
 *	updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToCellStateProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to report
                                         * results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing state. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Cell *cellPtr = (Cell *)widgRec;
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    const char *string;
    char c;
    int length, mask;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'n') && (strncmp(string, "normal", length) == 0)) {
	mask = 0;
        if (cellPtr == cellPtr->viewPtr->postPtr) {
            cellPtr->viewPtr->postPtr = NULL;
        }
    } else if ((c == 'p') && (strncmp(string, "disabled", length) == 0)) {
	mask = DISABLED;
    } else if ((c == 'h') && (strncmp(string, "highlighted", length) == 0)) {
	mask = HIGHLIGHT;
    } else if ((c == 'p') && (strncmp(string, "posted", length) == 0)) {
	mask = POSTED;
    } else {
	Tcl_AppendResult(interp, "unknown state \"", string, 
	    "\": should be disabled, posted, or normal.", (char *)NULL);
	return TCL_ERROR;
    }
    if (cellPtr == cellPtr->viewPtr->postPtr) {
        cellPtr->viewPtr->postPtr = NULL;
    }
    if (mask & POSTED) {
            cellPtr->viewPtr->postPtr = cellPtr;
    }        
    *flagsPtr &= ~CELL_FLAGS_MASK;
    *flagsPtr |= mask;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellStateToObjProc --
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
CellStateToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget information record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    unsigned int state = *(unsigned int *)(widgRec + offset);
    const char *string;

    if (state & DISABLED) {
	string = "disabled";
    } else if (state & POSTED) {
	string = "posted";
    } else if (state & HIGHLIGHT) {
	string = "highlighted";
    } else {
	string = "normal";
    }
    return Tcl_NewStringObj(string, -1);
}

/*ARGSUSED*/
static void
FreeStyleProc(ClientData clientData, Display *display, char *widgRec, 
	      int offset)
{
    CellStyle **stylePtrPtr = (CellStyle **)(widgRec + offset);
    CellStyle *stylePtr;

    stylePtr = *stylePtrPtr;
    if (stylePtr != NULL) {
	stylePtr->refCount--;
	if (stylePtr->refCount <= 0) {
	    (*stylePtr->classPtr->freeProc)(stylePtr);
	}
	*stylePtrPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToStyleProc --
 *
 *	Convert the name of an icon into a tableview style.
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
ObjToStyleProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
	       Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    CellStyle **stylePtrPtr = (CellStyle **)(widgRec + offset);
    CellStyle *stylePtr;
    const char *string;
    TableView *viewPtr;

    viewPtr = clientData;
    stylePtr = NULL;
    string = Tcl_GetString(objPtr);
    if (string[0] != '\0') {
	if (GetStyle(interp, viewPtr, objPtr, &stylePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	stylePtr->refCount++;		/* Increment the reference count to
					 * indicate that we are using this
					 * style. */
    }
    viewPtr->flags |= (LAYOUT_PENDING | GEOMETRY);
    if (*stylePtrPtr != NULL) {
	(*stylePtrPtr)->refCount--;
	if ((*stylePtrPtr)->refCount <= 0) {
	    (*stylePtr->classPtr->freeProc)(*stylePtrPtr);
	}
    }
    *stylePtrPtr = stylePtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleToObjProc --
 *
 *	Converts the style into its string representation (its name).
 *
 * Results:
 *	The name of the style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
StyleToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
	       char *widgRec, int offset, int flags)	
{
    CellStyle *stylePtr = *(CellStyle **)(widgRec + offset);

    if (stylePtr != NULL) {
	if (stylePtr->name != NULL) {
	    return Tcl_NewStringObj(stylePtr->name, -1);
	}
    } 
    return Tcl_NewStringObj("", -1);
}

/*ARGSUSED*/
static void
FreeTableProc(ClientData clientData, Display *display, char *widgRec, 
	      int offset)
{
    BLT_TABLE *tablePtr = (BLT_TABLE *)(widgRec + offset);

    if (*tablePtr != NULL) {
	TableView *viewPtr = clientData;

	/* 
	 * Release the current table, removing any rows/columns and entry
	 * fields.
	 */
	ClearSelections(viewPtr);
	blt_table_close(*tablePtr);
	*tablePtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTableProc --
 *
 *	Convert the string representing the name of a table object into a
 *	table token.
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
ObjToTableProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
	       Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    TableView *viewPtr = (TableView *)widgRec;
    BLT_TABLE *tablePtr = (BLT_TABLE *)(widgRec + offset);
    BLT_TABLE table;

    if (blt_table_open(interp, Tcl_GetString(objPtr), &table) != TCL_OK) {
	return TCL_ERROR;
    }
    if (*tablePtr != NULL) {
        FreeTableProc(clientData, viewPtr->display, widgRec, offset);
	viewPtr->rowNotifier = NULL;
	viewPtr->colNotifier = NULL;
    }
    *tablePtr = table;
    viewPtr->flags |= (GEOMETRY | LAYOUT_PENDING);
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}
/*
 *---------------------------------------------------------------------------
 *
 * TableToObjProc --
 *
 * Results:
 *	The string representation of the table is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TableToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin, 
	       char *widgRec, int offset, int flags)	
{
    BLT_TABLE table = *(BLT_TABLE *)(widgRec + offset);
    const char *name;

    if (table == NULL) {
	name = "";
    } else {
	name = blt_table_name(table);
    }
    return Tcl_NewStringObj(name, -1);
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToTitlesProc --
 *
 *	Converts the string to a titles flag: ROW_TITLES or COLUMN_TITLES. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTitlesProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    int *flagsPtr = (int *)(widgRec + offset);
    const char *string;
    int length;
    char c;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'r') && (strncmp(string, "rows", length) == 0)) {
	*flagsPtr &= ~TITLES_MASK;
	*flagsPtr |= ROW_TITLES;
    } else if ((c == 'c') && (strncmp(string, "columns", length) == 0)) {
	*flagsPtr &= ~TITLES_MASK;
	*flagsPtr |= COLUMN_TITLES;
    } else if ((c == 'b') && (strncmp(string, "both", length) == 0)) {
	*flagsPtr &= ~TITLES_MASK;
	*flagsPtr |= COLUMN_TITLES | ROW_TITLES;
    } else if ((c == 'n') && (strncmp(string, "none", length) == 0)) {
	*flagsPtr &= ~TITLES_MASK;
    } else {
	Tcl_AppendResult(interp, "unknown titles option \"", string, "\": ",
		"should be columns, rows, none, or both", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TitlesToObjProc --
 *
 *	Returns the titles flags as a string.
 *
 * Results:
 *	The fill style string is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TitlesToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
		char *widgRec, int offset, int flags)	
{
    int titles = *(int *)(widgRec + offset);
    const char *string;

    string = NULL;			/* Suppress compiler warning. */
    switch (titles & TITLES_MASK) {
    case ROW_TITLES:
	string = "rows";	break;
    case COLUMN_TITLES:
	string = "columns";	break;
    case 0:
	string = "none";	break;
    case (ROW_TITLES|COLUMN_TITLES):
	string = "both";	break;
    }
    return Tcl_NewStringObj(string, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToUidProc --
 *
 *	Converts the string to a Uid. Uid's are hashed, reference counted
 *	strings.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToUidProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
	     Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    TableView *viewPtr = clientData;
    UID *uidPtr = (UID *)(widgRec + offset);

    *uidPtr = GetUid(viewPtr, Tcl_GetString(objPtr));
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
UidToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
	     char *widgRec, int offset, int flags)	
{
    UID uid = *(UID *)(widgRec + offset);

    if (uid == NULL) {
	return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(uid, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeUidProc --
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
FreeUidProc(ClientData clientData, Display *display, char *widgRec, int offset)
{
    UID *uidPtr = (UID *)(widgRec + offset);

    if (*uidPtr != NULL) {
	TableView *viewPtr = clientData;

	FreeUid(viewPtr, *uidPtr);
	*uidPtr = NULL;
    }
}

static Row *
GetRowContainer(TableView *viewPtr, BLT_TABLE_ROW row)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&viewPtr->rowTable, (char *)row);
    if (hPtr == NULL) {
	return NULL;
    }
    return Blt_GetHashValue(hPtr);
}


static Column *
GetColumnContainer(TableView *viewPtr, BLT_TABLE_COLUMN col)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&viewPtr->columnTable, (char *)col);
    if (hPtr == NULL) {
	return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

static INLINE long
GetLastVisibleColumnIndex(TableView *viewPtr)
{
    long index;
    index = -1;
    if (viewPtr->numVisibleColumns > 0) {
        Column *colPtr;

        colPtr = viewPtr->visibleColumns[viewPtr->numVisibleColumns - 1];
        index = colPtr->index;
    }
    return index;
}

static INLINE long
GetLastVisibleRowIndex(TableView *viewPtr)
{
    long index;

    index = -1;
    if (viewPtr->numVisibleRows > 0) {
        Row *rowPtr;

        rowPtr = viewPtr->visibleRows[viewPtr->numVisibleRows - 1];
        index = rowPtr->index;
    }
    return index;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowTraceProc --
 *
 * Results:
 *	Returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowTraceProc(ClientData clientData, BLT_TABLE_TRACE_EVENT *eventPtr)
{
    TableView *viewPtr = clientData; 

    if (eventPtr->mask & (TABLE_TRACE_WRITES | TABLE_TRACE_UNSETS)) {
	Row *rowPtr;

	rowPtr = GetRowContainer(viewPtr, eventPtr->row);
	if (rowPtr != NULL) {
	    rowPtr->flags |= GEOMETRY | REDRAW;
	}
	viewPtr->flags |= GEOMETRY | LAYOUT_PENDING;
        /* Check if the event's row or column occur outside of the range of
         * visible cells. */
        if ((blt_table_row_index(eventPtr->row) > 
             GetLastVisibleRowIndex(viewPtr)) ||
            (blt_table_column_index(eventPtr->column) > 
             GetLastVisibleColumnIndex(viewPtr))) {
            return TCL_OK;
        }
	PossiblyRedraw(viewPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnTraceProc --
 *
 * Results:
 *	Returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnTraceProc(ClientData clientData, BLT_TABLE_TRACE_EVENT *eventPtr)
{
    TableView *viewPtr = clientData; 

    if (eventPtr->mask & (TABLE_TRACE_WRITES | TABLE_TRACE_UNSETS)) {
	Column *colPtr;

	colPtr = GetColumnContainer(viewPtr, eventPtr->column);
	if (colPtr != NULL) {
	    colPtr->flags |= GEOMETRY | REDRAW;
	}
	viewPtr->flags |= GEOMETRY | LAYOUT_PENDING;
        /* Check if the event's row or column occur outside of the range of
         * visible cells. */
        if ((blt_table_row_index(eventPtr->row) > 
             GetLastVisibleRowIndex(viewPtr)) ||
            (blt_table_column_index(eventPtr->column) > 
             GetLastVisibleColumnIndex(viewPtr))) {
            return TCL_OK;
        }
	PossiblyRedraw(viewPtr);
    }
    return TCL_OK;
}

static void
CellFreeProc(DestroyData data)
{
    Cell *cellPtr = (Cell *)data;
    TableView *viewPtr;
    
    viewPtr = cellPtr->viewPtr;
    Blt_Pool_FreeItem(viewPtr->cellPool, cellPtr);
}

static void
DestroyCell(Cell *cellPtr) 
{
    CellKey *keyPtr;
    TableView *viewPtr;
    
    viewPtr = cellPtr->viewPtr;
    if (cellPtr == viewPtr->activePtr) {
	viewPtr->activePtr = NULL;
    }
    if (cellPtr == viewPtr->focusPtr) {
	viewPtr->focusPtr = NULL;
	Blt_SetFocusItem(viewPtr->bindTable, viewPtr->focusPtr, 
			 (ClientData)ITEM_CELL);
    }
    if (cellPtr->stylePtr != NULL) {
	cellPtr->stylePtr->refCount--;
	if (cellPtr->stylePtr->refCount <= 0) {
	    (*cellPtr->stylePtr->classPtr->freeProc)(cellPtr->stylePtr);
	}
    }
    keyPtr = GetKey(cellPtr);
    if ((keyPtr->rowPtr == viewPtr->selectRows.anchorPtr) || 
	(keyPtr->rowPtr == viewPtr->selectRows.markPtr)) {
	viewPtr->selectRows.markPtr = viewPtr->selectRows.anchorPtr = NULL;
    }
    if (cellPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&viewPtr->cellTable, cellPtr->hashPtr);
    }
    if ((cellPtr->text != NULL) && (cellPtr->flags & TEXTALLOC)) {
	Blt_Free(cellPtr->text);
    }
    if (cellPtr->tkImage != NULL) {
	Tk_FreeImage(cellPtr->tkImage);
    }
    cellPtr->flags |= DELETED;
    Tcl_EventuallyFree(cellPtr, CellFreeProc);
}

static void
RemoveRowCells(TableView *viewPtr, Row *rowPtr)
{
    CellKey key;
    long i;

    /* For each column remove the row, column combination in the table. */
    key.rowPtr = rowPtr;
    for (i = 0; i < viewPtr->numColumns; i++) {
	Blt_HashEntry *hPtr;
	Column *colPtr;

	colPtr = viewPtr->columns[i];
	key.colPtr = colPtr;
	hPtr = Blt_FindHashEntry(&viewPtr->cellTable, &key);
	if (hPtr != NULL) {
	    Cell *cellPtr;

	    cellPtr = Blt_GetHashValue(hPtr);
	    DestroyCell(cellPtr);
	}
    }
}

static void
RemoveColumnCells(TableView *viewPtr, Column *colPtr)
{
    CellKey key;
    long i;

    /* For each row remove the row,column combination in the table. */
    key.colPtr = colPtr;
    for (i = 0; i < viewPtr->numRows; i++) {
	Blt_HashEntry *hPtr;
	Row *rowPtr;

	rowPtr = viewPtr->rows[i];
	key.rowPtr = rowPtr;
	hPtr = Blt_FindHashEntry(&viewPtr->cellTable, &key);
	if (hPtr != NULL) {
	    Cell *cellPtr;

	    cellPtr = Blt_GetHashValue(hPtr);
	    DestroyCell(cellPtr);
	}
    }
}

static void
RowFreeProc(DestroyData data)
{
    Row *rowPtr = (Row *)data;
    TableView *viewPtr;

    viewPtr = rowPtr->viewPtr;
    Blt_Pool_FreeItem(viewPtr->rowPool, rowPtr);
}

static void
DestroyRow(Row *rowPtr)
{
    TableView *viewPtr;

    viewPtr = rowPtr->viewPtr;
    uidOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    iconOption.clientData = viewPtr;
    Blt_FreeOptions(rowSpecs, (char *)rowPtr, viewPtr->display, 0);
    if (rowPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&viewPtr->rowTable, rowPtr->hashPtr);
    }
    blt_table_clear_row_traces(viewPtr->table, rowPtr->row);
    if ((rowPtr->flags & DELETED) == 0) {
	RemoveRowCells(viewPtr, rowPtr);
    }
    rowPtr->flags |= DELETED;
    Tcl_EventuallyFree(rowPtr, RowFreeProc);
}

static Row *
NewRow(TableView *viewPtr, BLT_TABLE_ROW row, Blt_HashEntry *hPtr)
{
    Row *rowPtr;

    rowPtr = Blt_Pool_AllocItem(viewPtr->rowPool, sizeof(Row));
    memset(rowPtr, 0, sizeof(Row));
    rowPtr->row = row;
    rowPtr->viewPtr = viewPtr;
    rowPtr->index = -1;
    rowPtr->title = blt_table_row_label(row);
    rowPtr->flags = GEOMETRY | REDRAW;
    rowPtr->weight = 1.0;
    rowPtr->max = SHRT_MAX;
    rowPtr->titleJustify = TK_JUSTIFY_RIGHT;
    rowPtr->titleRelief = rowPtr->activeTitleRelief = TK_RELIEF_RAISED;
    rowPtr->hashPtr = hPtr;
    Blt_SetHashValue(hPtr, rowPtr);
    return rowPtr;
}

static Row *
CreateRow(TableView *viewPtr, BLT_TABLE_ROW row, Blt_HashEntry *hPtr)
{
    Row *rowPtr;

    rowPtr = NewRow(viewPtr, row, hPtr);
    iconOption.clientData = viewPtr;
    uidOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(viewPtr->interp, viewPtr->tkwin, 
	rowPtr->title, "Row", rowSpecs, 0, (Tcl_Obj **)NULL, (char *)rowPtr, 
	0) != TCL_OK) {
	DestroyRow(rowPtr);
	return NULL;
    }
    return rowPtr;
}

static void
ColumnFreeProc(DestroyData data)
{
    Column *colPtr = (Column *)data;
    TableView *viewPtr;

    viewPtr = colPtr->viewPtr;
    Blt_Pool_FreeItem(viewPtr->columnPool, colPtr);
}

static void
DestroyColumn(Column *colPtr)
{
    TableView *viewPtr;

    viewPtr = colPtr->viewPtr;
    uidOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    iconOption.clientData = viewPtr;
    Blt_FreeOptions(columnSpecs, (char *)colPtr, viewPtr->display, 0);
    if (colPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&viewPtr->columnTable, colPtr->hashPtr);
    }
    blt_table_clear_column_traces(viewPtr->table, colPtr->column);
    if ((colPtr->flags & DELETED) == 0) {
	RemoveColumnCells(viewPtr, colPtr);
    }
    colPtr->flags |= DELETED;
    Tcl_EventuallyFree(colPtr, ColumnFreeProc);
}

static Column *
NewColumn(TableView *viewPtr, BLT_TABLE_COLUMN col, Blt_HashEntry *hPtr)
{
    Column *colPtr;

    colPtr = Blt_Pool_AllocItem(viewPtr->columnPool, sizeof(Column));
    memset(colPtr, 0, sizeof(Column));
    colPtr->column = col;
    colPtr->viewPtr = viewPtr;
    colPtr->index = -1;
    colPtr->title = blt_table_column_label(col);
    colPtr->flags = GEOMETRY | REDRAW;
    colPtr->weight = 1.0;
    colPtr->ruleWidth = 1;
    colPtr->pad.side1 = colPtr->pad.side2 = 0;
    colPtr->max = SHRT_MAX;
    colPtr->titleJustify = TK_JUSTIFY_CENTER;
    colPtr->titleRelief = colPtr->activeTitleRelief = TK_RELIEF_RAISED;
    colPtr->hashPtr = hPtr;
    Blt_SetHashValue(hPtr, colPtr);
    return colPtr;
}

static void
GetColumnTitleGeometry(TableView *viewPtr, Column *colPtr)
{
    unsigned int aw, ah, iw, ih, tw, th;

    colPtr->titleWidth = 2 * (viewPtr->colTitleBorderWidth + TITLE_PADX);
    colPtr->titleHeight = 2 * (viewPtr->colTitleBorderWidth + TITLE_PADY);

    aw = ah = tw = th = iw = ih = 0;
    if (colPtr->icon != NULL) {
	iw = IconWidth(colPtr->icon);
	ih = IconHeight(colPtr->icon);
	colPtr->titleWidth += iw;
    }
    if ((viewPtr->sort.up != NULL) && (viewPtr->sort.down != NULL)) {
	aw = MAX(IconWidth(viewPtr->sort.up), IconWidth(viewPtr->sort.down));
	ah = MAX(IconHeight(viewPtr->sort.up), IconHeight(viewPtr->sort.down));
    } else {
	aw = ah = 17;
    }
    colPtr->titleWidth += aw + TITLE_PADX;
    if ((colPtr->flags & TEXTALLOC) == 0) {
	colPtr->title = blt_table_column_label(colPtr->column);
    }
    if (colPtr->title != NULL) {
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, viewPtr->colTitleFont);
	Blt_Ts_GetExtents(&ts, colPtr->title,  &tw, &th);
	colPtr->textWidth = tw;
	colPtr->textHeight = th;
	colPtr->titleWidth += tw;
	if (colPtr->icon != NULL) {
	    colPtr->titleWidth += TITLE_PADX;
	}
    }
    colPtr->titleHeight += MAX3(ih, th, ah);
}


/*
 * GetColumnFiltersGeometry -- 
 *
 *      +---------------------------+	
 *	|b|x|icon|x|text|x|arrow|x|b|	
 *      +---------------------------+
 *
 * b = filter borderwidth
 * x = padx 
 */
static void
GetColumnFiltersGeometry(TableView *viewPtr)
{
    unsigned int ah;
    int i;
    FilterInfo *filterPtr;

    filterPtr = &viewPtr->filter;
    viewPtr->colFilterHeight = 0;
    viewPtr->arrowWidth = ah = Blt_TextWidth(filterPtr->font, "0", 1) + 
	2 * (filterPtr->borderWidth + 1);
    for (i = 0; i < viewPtr->numColumns; i++) {
	Column *colPtr;
	unsigned int tw, th, ih, iw;

	colPtr = viewPtr->columns[i];
	tw = th = ih = iw = 0;
	if (colPtr->filterIcon != NULL) {
	    ih = IconHeight(colPtr->filterIcon);
	    iw = IconWidth(colPtr->filterIcon);
	}
	if (colPtr->filterText != NULL) {
	    TextStyle ts;
	    
	    Blt_Ts_InitStyle(ts);
	    Blt_Ts_SetFont(ts, filterPtr->font);
	    Blt_Ts_GetExtents(&ts, colPtr->filterText, &tw, &th);
	    colPtr->filterTextWidth = tw;
	    colPtr->filterTextHeight = th;
	} else {
	    Blt_FontMetrics fm;

	    Blt_Font_GetMetrics(filterPtr->font, &fm);
	    th = fm.linespace;
            colPtr->filterTextWidth = 0, colPtr->filterTextHeight = th;
	}
	
	colPtr->filterHeight = MAX3(ah, th, ih);
	if (viewPtr->colFilterHeight < colPtr->filterHeight) {
	    viewPtr->colFilterHeight = colPtr->filterHeight;
	}
    }
    viewPtr->colFilterHeight += 2 * (filterPtr->borderWidth + TITLE_PADY + 1);
}

static int 
ConfigureColumn(TableView *viewPtr, Column *colPtr)
{
    if (Blt_ConfigModified(columnSpecs, "-font", "-title", "-hide", "-icon", 
	"-arrowwidth", "-borderwidth", (char *)NULL)) {
	if (viewPtr->flags & COLUMN_TITLES) {
	    GetColumnTitleGeometry(viewPtr, colPtr);
	} 
    }
    if (Blt_ConfigModified(columnSpecs, "-filtertext", (char *)NULL)) {
	GetColumnFiltersGeometry(viewPtr);
    }
    if (Blt_ConfigModified(columnSpecs, "-style", (char *)NULL)) {
	/* If the style changed, recompute the geometry of the cells. */
	colPtr->flags |= GEOMETRY;
	viewPtr->flags |= GEOMETRY | REDRAW;
    }
    return TCL_OK;
}

static Column *
CreateColumn(TableView *viewPtr, BLT_TABLE_COLUMN col, Blt_HashEntry *hPtr)
{
    Column *colPtr;

    colPtr = NewColumn(viewPtr, col, hPtr);
    iconOption.clientData = viewPtr;
    uidOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(viewPtr->interp, viewPtr->tkwin, 
	colPtr->title, "Column", columnSpecs, 0, (Tcl_Obj **)NULL, 
	(char *)colPtr, 0) != TCL_OK) {
	DestroyColumn(colPtr);
	return NULL;
    }
    ConfigureColumn(viewPtr, colPtr);
    return colPtr;
}

static Column *
GetFirstColumn(TableView *viewPtr)
{
    long i;

    for (i = 0; i < viewPtr->numColumns; i++) {
	Column *colPtr;

	colPtr = viewPtr->columns[i];
	if ((colPtr->flags & (HIDDEN|DISABLED)) == 0) {
	    return colPtr;
	}
    }
    return NULL;
}

static Column *
GetNextColumn(Column *colPtr)
{
    TableView *viewPtr = colPtr->viewPtr;
    long i;

    for (i = colPtr->index + 1; i < viewPtr->numColumns; i++) {
	colPtr = viewPtr->columns[i];
	if ((colPtr->flags & (HIDDEN|DISABLED)) == 0) {
	    return colPtr;
	}
    }
    return NULL;
}

static Column *
GetPrevColumn(Column *colPtr)
{
    long i;
    TableView *viewPtr = colPtr->viewPtr;

    for (i = colPtr->index - 1; i >= 0; i--) {
	colPtr = viewPtr->columns[i];
	if ((colPtr->flags & (HIDDEN|DISABLED)) == 0) {
	    return colPtr;
	}
    }
    return NULL;
}

static Column *
GetLastColumn(TableView *viewPtr)
{
    long i;

    for (i = viewPtr->numColumns - 1; i >= 0; i--) {
	Column *colPtr;

	colPtr = viewPtr->columns[i];
	if ((colPtr->flags & (HIDDEN|DISABLED)) == 0) {
	    return colPtr;
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * NearestColumn --
 *
 *	Finds the row closest to the given screen Y-coordinate in the
 *	viewport.
 *
 * Results:
 *	Returns the pointer to the closest row.  If no row is visible (rows
 *	may be hidden), NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Column *
NearestColumn(TableView *viewPtr, int x, int selectOne)
{
    Column *lastPtr;
    long i;

    /*
     * We implicitly can pick only visible rows.  So make sure that the
     * row exists.
     */
    if (viewPtr->numVisibleRows == 0) {
	return NULL;
    }
    if (x < viewPtr->rowTitleWidth) {
	return (selectOne) ? viewPtr->visibleColumns[0] : NULL;
    }
    /*
     * Since the entry positions were previously computed in world
     * coordinates, convert x-coordinate from screen to world coordinates
     * too.
     */
    x = WORLDX(viewPtr, x);
    lastPtr = NULL;			/* Suppress compiler warning. */
    /* FIXME: This can be a binary search. */
    for (i = 0; i < viewPtr->numVisibleColumns; i++) {
	Column *colPtr;

	colPtr = viewPtr->visibleColumns[i];
	if (colPtr->worldX > x) {
	    return (selectOne) ? colPtr : NULL;
	}
	if (x < (colPtr->worldX + colPtr->width)) {
	    return colPtr;		/* Found it. */
	}
	lastPtr = colPtr;
    }
    return (selectOne) ? lastPtr : NULL;
}

static int
GetColumnByIndex(TableView *viewPtr, const char *string, Column **colPtrPtr)
{
    Column *focusPtr, *colPtr;
    char c;

    focusPtr = colPtr = NULL;
    if (viewPtr->focusPtr != NULL) {
	CellKey *keyPtr;
	
	keyPtr = Blt_GetHashKey(&viewPtr->cellTable,viewPtr->focusPtr->hashPtr);
	focusPtr = keyPtr->colPtr;
    }
    c = string[0];
    if (c == '@') {
	int x;

	if (Tcl_GetInt(NULL, string + 1, &x) == TCL_OK) {
	    colPtr = NearestColumn(viewPtr, x, FALSE);
	}
    } else if ((c == 'l') && (strcmp(string, "last") == 0)) {
	colPtr = GetLastColumn(viewPtr);
    } else if ((c == 'f') && (strcmp(string, "first") == 0)) {
	colPtr = GetFirstColumn(viewPtr);
    } else if ((c == 'f') && (strcmp(string, "focus") == 0)) {
	if (focusPtr != NULL) {
	    colPtr = focusPtr;
	}
    } else if ((c == 'a') && (strcmp(string, "active") == 0)) {
	colPtr = viewPtr->colActiveTitlePtr;
    } else if ((c == 'c') && (strcmp(string, "current") == 0)) {
	TableObj *objPtr;

	objPtr = Blt_GetCurrentItem(viewPtr->bindTable);
	if ((objPtr != NULL) && ((objPtr->flags & DELETED) == 0)) {
	    unsigned long flags;

	    flags = (long)Blt_GetCurrentHint(viewPtr->bindTable);
	    if (flags & ITEM_COLUMN_MASK) {
		colPtr = (Column *)objPtr;
	    } else if (flags & ITEM_CELL) {
		Cell *cellPtr;
		CellKey *keyPtr;

		cellPtr = (Cell *)objPtr;
		keyPtr = GetKey(cellPtr);
		colPtr = keyPtr->colPtr;
	    }
	}
    } else if ((c == 'p') && (strcmp(string, "previous") == 0)) {
	if (focusPtr != NULL) {
	    colPtr = GetPrevColumn(focusPtr);
	}
    } else if ((c == 'n') && (strcmp(string, "next") == 0)) {
	if (focusPtr != NULL) {
	    colPtr = GetNextColumn(focusPtr);
	}
    } else if ((c == 'n') && (strcmp(string, "none") == 0)) {
	colPtr = NULL;
    } else if ((c == 'v') && (strcmp(string, "view.left") == 0)) {
	if (viewPtr->numVisibleColumns > 0) {
	    colPtr = viewPtr->visibleColumns[0];
	}
    } else if ((c == 'v') && (strcmp(string, "view.right") == 0)) {
	if (viewPtr->numVisibleColumns > 0) {
	    colPtr = viewPtr->visibleColumns[viewPtr->numVisibleColumns-1];
	} 
    } else {
	return TCL_ERROR;
    }
    *colPtrPtr = colPtr;
    return TCL_OK;
}

static int
GetColumn(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, 
	  Column **colPtrPtr)
{
    BLT_TABLE_COLUMN col;
    Blt_HashEntry *hPtr;
    const char *string;

    string = Tcl_GetString(objPtr);

    /* First check if it's a special column index.  */
    if (GetColumnByIndex(viewPtr, string, colPtrPtr) == TCL_OK) {
	return TCL_OK;
    }
    if (viewPtr->table == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "no datatable configured.", (char *)NULL);
	}
	return TCL_ERROR;
    }
    /* Next see if it's a column in the table. */
    col = blt_table_get_column(interp, viewPtr->table, objPtr);
    if (col == NULL) {
	return TCL_ERROR;
    }
    hPtr = Blt_FindHashEntry(&viewPtr->columnTable, (char *)col);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find column \"", string, 
		"\" in \"", Tk_PathName(viewPtr->tkwin), "\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    *colPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

static Row *
GetFirstRow(TableView *viewPtr)
{
    long i;

    for (i = 0; i < viewPtr->numRows; i++) {
	Row *rowPtr;

	rowPtr = viewPtr->rows[i];
	if ((rowPtr->flags & (HIDDEN|DISABLED)) == 0) {
	    return rowPtr;
	}
    }
    return NULL;
}

static Row *
GetNextRow(Row *rowPtr)
{
    long i;
    TableView *viewPtr = rowPtr->viewPtr;

    for (i = rowPtr->index + 1; i < viewPtr->numRows; i++) {
	rowPtr = viewPtr->rows[i];
	if ((rowPtr->flags & (HIDDEN|DISABLED)) == 0) {
	    return rowPtr;
	}
    }
    return NULL;
}

static Row *
GetPrevRow(Row *rowPtr)
{
    long i;
    TableView *viewPtr = rowPtr->viewPtr;

    for (i = rowPtr->index - 1; i >= 0; i--) {
	rowPtr = viewPtr->rows[i];
	if ((rowPtr->flags & (HIDDEN|DISABLED)) == 0) {
	    return rowPtr;
	}
    }
    return NULL;
}

static Row *
GetLastRow(TableView *viewPtr)
{
    long i;

    for (i = viewPtr->numRows -1; i >= 0; i--) {
	Row *rowPtr;

	rowPtr = viewPtr->rows[i];
	if ((rowPtr->flags & (HIDDEN|DISABLED)) == 0) {
	    return rowPtr;
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * NearestRow --
 *
 *	Finds the row closest to the given screen Y-coordinate in the
 *	viewport.
 *
 * Results:
 *	Returns the pointer to the closest row.  If no row is visible (rows
 *	may be hidden), NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Row *
NearestRow(TableView *viewPtr, int y, int selectOne)
{
    Row *lastPtr;
    long i;

    /*
     * We implicitly can pick only visible rows.  So make sure that the
     * row exists.
     */
    if (viewPtr->numVisibleRows == 0) {
	return NULL;
    }
    if (y < (viewPtr->colTitleHeight + viewPtr->colFilterHeight)) {
	return (selectOne) ? viewPtr->visibleRows[0] : NULL;
    }
    lastPtr = NULL;			/* Suppress compiler warning. */
    /*
     * Since the entry positions were previously computed in world
     * coordinates, convert Y-coordinate from screen to world coordinates
     * too.
     */
    y = WORLDY(viewPtr, y);
    /* FIXME: This can be a binary search. */
    for (i = 0; i < viewPtr->numVisibleRows; i++) {
	Row *rowPtr;

	rowPtr = viewPtr->visibleRows[i];
	if (rowPtr->worldY > y) {
	    return (selectOne) ? rowPtr : NULL;
	}
	if (y < (rowPtr->worldY + rowPtr->height)) {
	    return rowPtr;		/* Found it. */
	}
	lastPtr = rowPtr;
    }
    return (selectOne) ? lastPtr : NULL;
}

static int
GetRowByIndex(TableView *viewPtr, Tcl_Obj *objPtr, Row **rowPtrPtr)
{
    Row *focusPtr, *rowPtr;
    char c;
    const char *string;
    int length;

    focusPtr = rowPtr = NULL;
    if (viewPtr->focusPtr != NULL) {
	CellKey *keyPtr;
	
	keyPtr = Blt_GetHashKey(&viewPtr->cellTable,viewPtr->focusPtr->hashPtr);
	focusPtr = keyPtr->rowPtr;
    }
    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if (c == '@') {
	int y;

	if (Tcl_GetInt(NULL, string + 1, &y) == TCL_OK) {
	    rowPtr = NearestRow(viewPtr, y, FALSE);
	}
    } else if ((c == 'a') && (length > 1) && 
	       (strncmp(string, "active", length) == 0)) {
	rowPtr = viewPtr->rowActiveTitlePtr;
    } else if ((c == 'c') && (strncmp(string, "current", length) == 0)) {
	TableObj *objPtr;

	objPtr = Blt_GetCurrentItem(viewPtr->bindTable);
	if ((objPtr != NULL) && ((objPtr->flags & DELETED) == 0)) {
	    unsigned long flags;

	    flags = (long)Blt_GetCurrentHint(viewPtr->bindTable);
	    if (flags & ITEM_ROW_MASK) {
		rowPtr = (Row *)objPtr;
	    } else if (flags & ITEM_CELL) {
		Cell *cellPtr;
		CellKey *keyPtr;

		cellPtr = (Cell *)objPtr;
		keyPtr = GetKey(cellPtr);
		rowPtr = keyPtr->rowPtr;
	    }
	}
    } else if ((c == 'l') && (strncmp(string, "last", length) == 0)) {
	rowPtr = GetLastRow(viewPtr);
    } else if ((c == 'f') && (strncmp(string, "first", length) == 0)) {
	rowPtr = GetFirstRow(viewPtr);
    } else if ((c == 'f') && (strncmp(string, "focus", length) == 0)) {
	if (focusPtr != NULL) {
	    rowPtr = focusPtr;
	}
    } else if ((c == 'p') && (strncmp(string, "previous", length) == 0)) {
	if (focusPtr != NULL) {
	    rowPtr = GetPrevRow(focusPtr);
	}
    } else if ((c == 'n') && (strncmp(string, "next", length) == 0)) {
	if (focusPtr != NULL) {
	    rowPtr = GetNextRow(focusPtr);
	}
    } else if ((c == 'n') && (strncmp(string, "none", length) == 0)) {
	rowPtr = NULL;
    } else if ((c == 'm') && (strncmp(string, "mark", length) == 0)) {
	rowPtr = viewPtr->selectRows.markPtr;
    } else if ((c == 'a') && (length > 1) && 
	       (strncmp(string, "anchor", length) == 0)) {
	rowPtr = viewPtr->selectRows.anchorPtr;
    } else if ((c == 'v') && (length > 5) &&
	       (strncmp(string, "view.top", length) == 0)) {
	if (viewPtr->numVisibleRows > 0) {
	    rowPtr = viewPtr->visibleRows[0];
	}
    } else if ((c == 'v') && (length > 5) &&
	       (strncmp(string, "view.bottom", length) == 0)) {
	if (viewPtr->numVisibleRows > 0) {
	    rowPtr = viewPtr->visibleRows[viewPtr->numVisibleRows-1];
	} 
    } else {
	return TCL_ERROR;
    }
    *rowPtrPtr = rowPtr;
    return TCL_OK;
}

static int
GetRow(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, Row **rowPtrPtr)
{
    BLT_TABLE_ROW row;
    Blt_HashEntry *hPtr;

    /* First check if it's a special column index.  */
    if (GetRowByIndex(viewPtr, objPtr, rowPtrPtr) == TCL_OK) {
	return TCL_OK;
    }
    /* Next see if it's a row in the table. */
    if (viewPtr->table == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "no datatable configured.", (char *)NULL);
	}
	return TCL_ERROR;
    }
    row = blt_table_get_row(interp, viewPtr->table, objPtr);
    if (row == NULL) {
	return TCL_ERROR;
    }
    hPtr = Blt_FindHashEntry(&viewPtr->rowTable, (char *)row);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find row \"", Tcl_GetString(objPtr),
		"\" in \"", Tk_PathName(viewPtr->tkwin), "\"", 
		(char *)NULL);
	}
	return TCL_ERROR;
    }
    *rowPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}


static int
GetRows(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, 
	Blt_Chain *rowsPtr)
{
    Blt_Chain chain;
    Row *rowPtr;

    chain = Blt_Chain_Create();
    /* First check if it's a row that we know about (to handle special
     * tableview-specific indices like "focus" or "current").  */
    if (GetRow(NULL, viewPtr, objPtr, &rowPtr) == TCL_OK) {
	if (rowPtr != NULL) {
	    Blt_Chain_Append(chain, rowPtr);
	}
    } else {
	BLT_TABLE_ITERATOR iter;
	BLT_TABLE_ROW row;

	/* Process the name as a row tag. */
	if (blt_table_iterate_row(interp, viewPtr->table, objPtr, &iter) 
	    != TCL_OK){
	    Blt_Chain_Destroy(chain);
	    return TCL_ERROR;
	}
	/* Append the new rows onto the chain. */
	for (row = blt_table_first_tagged_row(&iter); row != NULL; 
	     row = blt_table_next_tagged_row(&iter)) {
	    Blt_Chain_Append(chain, GetRowContainer(viewPtr, row));
	}
    }
    *rowsPtr = chain;
    return TCL_OK;
}

static int
GetColumns(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, 
	   Blt_Chain *columnsPtr)
{
    Blt_Chain chain;
    Column *colPtr;

    chain = Blt_Chain_Create();
    /* First check if it's a column that we know about (to handle special
     * tableview-specific indices like "focus" or "current").  */
    if (GetColumn(NULL, viewPtr, objPtr, &colPtr) == TCL_OK) {
	if (colPtr != NULL) {
	    Blt_Chain_Append(chain, colPtr);
	}
    } else {
	BLT_TABLE_ITERATOR iter;
	BLT_TABLE_COLUMN col;

	/* Process the name as a column tag. */
	if (blt_table_iterate_column(interp, viewPtr->table, objPtr, &iter) 
	    != TCL_OK){
	    Blt_Chain_Destroy(chain);
	    return TCL_ERROR;
	}
	/* Append the new columns onto the chain. */
	for (col = blt_table_first_tagged_column(&iter); col != NULL; 
	     col = blt_table_next_tagged_column(&iter)) {
	    Blt_Chain_Append(chain, GetColumnContainer(viewPtr, col));
	}
    }
    *columnsPtr = chain;
    return TCL_OK;
}

static Blt_Chain 
IterateRowsObjv(Tcl_Interp *interp, TableView *viewPtr, int objc, 
		Tcl_Obj *const *objv)
{
    Blt_HashTable rowTable;
    Blt_Chain chain;
    int i;

    chain = Blt_Chain_Create();
    Blt_InitHashTableWithPool(&rowTable, BLT_ONE_WORD_KEYS);
    for (i = 0; i < objc; i++) {
	BLT_TABLE_ITERATOR iter;
	BLT_TABLE_ROW row;
	int isNew;
	Row *rowPtr;

	if (GetRow(NULL, viewPtr, objv[i], &rowPtr) == TCL_OK) {
	    if (rowPtr != NULL) {
		
		Blt_CreateHashEntry(&rowTable, (char *)rowPtr->row, &isNew);
		if (isNew) {
		    Blt_Chain_Append(chain, rowPtr);
		}
	    }
	    continue;
	}
	if (blt_table_iterate_row(interp, viewPtr->table, objv[i], &iter) 
	    != TCL_OK){
	    Blt_DeleteHashTable(&rowTable);
	    Blt_Chain_Destroy(chain);
	    return NULL;
	}
	/* Append the new rows onto the chain. */
	for (row = blt_table_first_tagged_row(&iter); row != NULL; 
	     row = blt_table_next_tagged_row(&iter)) {
	    int isNew;

	    Blt_CreateHashEntry(&rowTable, (char *)row, &isNew);
	    if (isNew) {
		Blt_Chain_Append(chain, GetRowContainer(viewPtr, row));
	    }
	}
    }
    Blt_DeleteHashTable(&rowTable);
    return chain;
}

static Blt_Chain 
IterateColumnsObjv(Tcl_Interp *interp, TableView *viewPtr, int objc, 
		Tcl_Obj *const *objv)
{
    Blt_HashTable colTable;
    Blt_Chain chain;
    int i;

    chain = Blt_Chain_Create();
    Blt_InitHashTableWithPool(&colTable, BLT_ONE_WORD_KEYS);
    for (i = 0; i < objc; i++) {
	BLT_TABLE_ITERATOR iter;
	BLT_TABLE_COLUMN col;
	int isNew;
	Column *colPtr;

	if (GetColumn(NULL, viewPtr, objv[i], &colPtr) == TCL_OK) {
	    if (colPtr != NULL) {
		
		Blt_CreateHashEntry(&colTable, (char *)colPtr->column, &isNew);
		if (isNew) {
		    Blt_Chain_Append(chain, colPtr);
		}
	    }
	    continue;
	}
	if (blt_table_iterate_column(interp, viewPtr->table, objv[i], &iter) 
	    != TCL_OK){
	    Blt_DeleteHashTable(&colTable);
	    Blt_Chain_Destroy(chain);
	    return NULL;
	}
	/* Append the new columns onto the chain. */
	for (col = blt_table_first_tagged_column(&iter); col != NULL; 
	     col = blt_table_next_tagged_column(&iter)) {
	    int isNew;

	    Blt_CreateHashEntry(&colTable, (char *)col, &isNew);
	    if (isNew) {
		Blt_Chain_Append(chain, GetColumnContainer(viewPtr, col));
	    }
	}
    }
    Blt_DeleteHashTable(&colTable);
    return chain;
}

static Cell *
GetCell(TableView *viewPtr, Row *rowPtr, Column *colPtr)
{
    CellKey key;
    Blt_HashEntry *hPtr;

    key.rowPtr = rowPtr;
    key.colPtr = colPtr;
    hPtr = Blt_FindHashEntry(&viewPtr->cellTable, (char *)&key);
    if (hPtr == NULL) {
	return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

static int
GetCellByIndex(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, 
	       Cell **cellPtrPtr)
{
    char c;
    const char *string;
    int length;

    *cellPtrPtr = NULL;
    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if (c == '@') {
	int x, y;

	if (Blt_GetXY(NULL, viewPtr->tkwin, string, &x, &y) == TCL_OK) {
	    Column *colPtr;
	    Row *rowPtr;

	    colPtr = NearestColumn(viewPtr, x, FALSE);
	    rowPtr = NearestRow(viewPtr, y, FALSE);
	    if ((rowPtr != NULL) && (colPtr != NULL)) {
		*cellPtrPtr = GetCell(viewPtr, rowPtr, colPtr);
	    }
	}
	return TCL_OK;
    } else if ((c == 'a') && (length > 1) && 
	       (strncmp(string, "active", length) == 0)) {
	*cellPtrPtr = viewPtr->activePtr;
	return TCL_OK;
    } else if ((c == 'f') && (strncmp(string, "focus", length) == 0)) {
	*cellPtrPtr = viewPtr->focusPtr;
	return TCL_OK;
    } else if ((c == 'n') && (strncmp(string, "none", length) == 0)) {
	*cellPtrPtr = NULL;
	return TCL_OK;
    } else if ((c == 'c') && (strncmp(string, "current", length) == 0)) {
	TableObj *objPtr;

	objPtr = Blt_GetCurrentItem(viewPtr->bindTable);
	if ((objPtr != NULL) && ((objPtr->flags & DELETED) == 0)) {
	    unsigned long flags;

	    flags = (long)Blt_GetCurrentHint(viewPtr->bindTable);
	    if (flags & ITEM_CELL) {
		*cellPtrPtr = (Cell *)objPtr;
	    }
	}
	return TCL_OK;
    } else if ((c == 'l') && (strncmp(string, "left", length) == 0)) {
	if (viewPtr->focusPtr != NULL) {
	    Column *colPtr;
	    CellKey *keyPtr;

	    keyPtr = GetKey(viewPtr->focusPtr);
	    colPtr = GetPrevColumn(keyPtr->colPtr);
	    if (colPtr != NULL) {
		*cellPtrPtr = GetCell(viewPtr, keyPtr->rowPtr, colPtr);

	    }
	}
	return TCL_OK;
    } else if ((c == 'r') && (strncmp(string, "right", length) == 0)) {
	if (viewPtr->focusPtr != NULL) {
	    Column *colPtr;
	    CellKey *keyPtr;
	    
	    keyPtr = GetKey(viewPtr->focusPtr);
	    colPtr = GetNextColumn(keyPtr->colPtr);
	    if (colPtr != NULL) {
		*cellPtrPtr = GetCell(viewPtr, keyPtr->rowPtr, colPtr);
	    }
	}
	return TCL_OK;
    } else if ((c == 'u') && (strncmp(string, "up", length) == 0)) {
	if (viewPtr->focusPtr != NULL) {
	    Row *rowPtr;
	    CellKey *keyPtr;
	    
	    keyPtr = GetKey(viewPtr->focusPtr);
	    rowPtr = GetPrevRow(keyPtr->rowPtr);
	    if (rowPtr != NULL) {
		*cellPtrPtr = GetCell(viewPtr, rowPtr, keyPtr->colPtr);
	    }
	}
	return TCL_OK;
    } else if ((c == 'd') && (strncmp(string, "down", length) == 0)) {
	if (viewPtr->focusPtr != NULL) {
	    Row *rowPtr;
	    CellKey *keyPtr;
	    
	    keyPtr = GetKey(viewPtr->focusPtr);
	    rowPtr = GetNextRow(keyPtr->rowPtr);
	    if (rowPtr != NULL) {
		*cellPtrPtr = GetCell(viewPtr, rowPtr, keyPtr->colPtr);
	    }
	}
	return TCL_OK;
    } else if ((c == 'm') && (strncmp(string, "mark", length) == 0)) {
	CellSelection *selectPtr = &viewPtr->selectCells;

	if (selectPtr->markPtr != NULL) {
	    *cellPtrPtr = GetCell(viewPtr, selectPtr->markPtr->rowPtr,
		selectPtr->markPtr->colPtr);
	}
	return TCL_OK;
    } else if ((c == 'a') && (length > 1) && 
	       (strncmp(string, "anchor", length) == 0)) {
	CellSelection *selectPtr = &viewPtr->selectCells;

	if (selectPtr->markPtr != NULL) {
	    *cellPtrPtr = GetCell(viewPtr, selectPtr->anchorPtr->rowPtr,
		selectPtr->anchorPtr->colPtr);
	}
	return TCL_OK;
    } 
    return TCL_CONTINUE;
}


static int
GetCellFromObj(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, 
	       Cell **cellPtrPtr)
{
    int objc;
    Tcl_Obj **objv;
    Row *rowPtr;
    Column *colPtr;

    *cellPtrPtr = NULL;
    if (GetCellByIndex(interp, viewPtr, objPtr, cellPtrPtr) == TCL_OK) {
	return TCL_OK;
    }
    /*
     * Pick apart the cell descriptor to get the row and columns.
     */
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc != 2) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "wrong # elements in cell index \"",
			     Tcl_GetString(objPtr), "\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    if ((GetRow(interp, viewPtr, objv[0], &rowPtr) != TCL_OK) ||
	(GetColumn(interp, viewPtr, objv[1], &colPtr) != TCL_OK)) {
	return TCL_ERROR;
    }
    if ((colPtr != NULL) && (rowPtr != NULL)) {
	*cellPtrPtr = GetCell(viewPtr, rowPtr, colPtr);
    }
    return TCL_OK;
}

static int
GetCellsFromObj(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, 
		Blt_Chain *cellsPtr)
{
    Cell *cellPtr;
    int objc;
    Tcl_Obj **objv;
    Blt_Chain rows, columns, cells;
    Blt_ChainLink link;

    cellPtr = NULL;
    if (GetCellByIndex(interp, viewPtr, objPtr, &cellPtr) == TCL_OK) {
	cells = Blt_Chain_Create();
	Blt_Chain_Append(cells, cellPtr);
	*cellsPtr = cells;
	return TCL_OK;
    }
    /*
     * Pick apart the cell descriptor to get the row and columns.
     */
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc != 2) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "wrong # elements in cell index \"",
			     Tcl_GetString(objPtr), "\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    if (GetRows(interp, viewPtr, objv[0], &rows) != TCL_OK) {
	return TCL_ERROR;
    }
    if (GetColumns(interp, viewPtr, objv[1], &columns) != TCL_OK) {
	Blt_Chain_Destroy(rows);
	return TCL_ERROR;
    }
    cells = Blt_Chain_Create();
    for (link = Blt_Chain_FirstLink(rows); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	Blt_ChainLink link2;
	Row *rowPtr;

	rowPtr = Blt_Chain_GetValue(link);
	if (rowPtr == NULL) {
	    continue;
	}
	for (link2 = Blt_Chain_FirstLink(columns); link2 != NULL; 
	     link2 = Blt_Chain_NextLink(link2)) {
	    Column *colPtr;
	    
	    colPtr = Blt_Chain_GetValue(link2);
	    if (colPtr == NULL) {
		continue;
	    }
	    cellPtr = GetCell(viewPtr, rowPtr, colPtr);
	    Blt_Chain_Append(cells, cellPtr);
	}
    }
    Blt_Chain_Destroy(rows);
    Blt_Chain_Destroy(columns);
    *cellsPtr = cells;
    return TCL_OK;
}

static Blt_Chain 
IterateCellsObjv(Tcl_Interp *interp, TableView *viewPtr, int objc, 
		Tcl_Obj *const *objv)
{
    Blt_HashTable cellTable;
    Blt_Chain chain;
    int i;

    chain = Blt_Chain_Create();
    Blt_InitHashTableWithPool(&cellTable, BLT_ONE_WORD_KEYS);
    for (i = 0; i < objc; i++) {
	Blt_Chain cells;
	Blt_ChainLink link;

	if (GetCellsFromObj(interp, viewPtr, objv[i], &cells) != TCL_OK) {
	    Blt_Chain_Destroy(chain);
	    return NULL;
	}
	for (link = Blt_Chain_FirstLink(cells); link != NULL;
	     link = Blt_Chain_NextLink(link)) {
	    Cell *cellPtr;

	    cellPtr = Blt_Chain_GetValue(link);
	    if (cellPtr != NULL) {
		int isNew;

		Blt_CreateHashEntry(&cellTable, (char *)cellPtr, &isNew);
		if (isNew) {
		    Blt_Chain_Append(chain, cellPtr);
		}
	    }
	}
	Blt_Chain_Destroy(cells);
    }
    Blt_DeleteHashTable(&cellTable);
    return chain;
}

static CellStyle *
GetCurrentStyle(TableView *viewPtr, Row *rowPtr, Column *colPtr, Cell *cellPtr)
{
    if ((cellPtr != NULL) && (cellPtr->stylePtr != NULL)) {
	return cellPtr->stylePtr;
    }
    if ((rowPtr != NULL) && (rowPtr->stylePtr != NULL)) {
	return rowPtr->stylePtr;
    }
    if ((colPtr != NULL) && (colPtr->stylePtr != NULL)) {
	return colPtr->stylePtr;
    }
    return viewPtr->stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * LostSelection --
 *
 *	This procedure is called back by Tk when the selection is grabbed
 *	away.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The existing selection is unhighlighted, and the window is
 *	marked as not containing a selection.
 *
 *---------------------------------------------------------------------------
 */
static void
LostSelection(ClientData clientData)
{
    TableView *viewPtr = clientData;

    if (viewPtr->flags & SELECT_EXPORT) {
	ClearSelections(viewPtr);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * SelectRow --
 *
 *	Adds the given row from the set of selected rows.  It's OK
 *	if the row has already been selected.
 *
 *---------------------------------------------------------------------------
 */
static void
SelectRow(TableView *viewPtr, Row *rowPtr)
{
    if ((rowPtr->flags & SELECTED) == 0) {
	RowSelection *selectPtr = &viewPtr->selectRows;

	rowPtr->flags |= SELECTED;
	rowPtr->link = Blt_Chain_Append(selectPtr->list, rowPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DeselectRow --
 *
 *	Removes the given row from the set of selected rows. It's OK
 *	if the row isn't already selected.
 *
 *---------------------------------------------------------------------------
 */
static void
DeselectRow(TableView *viewPtr, Row *rowPtr)
{
    RowSelection *selectPtr = &viewPtr->selectRows;

    rowPtr->flags &= ~SELECTED;
    Blt_Chain_DeleteLink(selectPtr->list, rowPtr->link);
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectRows --
 *
 *	Sets the selection flag for a range of nodes.  The range is determined
 *	by two pointers which designate the first/last nodes of the range.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectRows(TableView *viewPtr, Row *fromPtr, Row *toPtr)
{
    long from, to, i;

    from = fromPtr->index;
    to = toPtr->index;
    if (from > to) {
	for (i = from; i >= to; i--) {
	    Row *rowPtr;

	    rowPtr = viewPtr->rows[i];
	    switch (viewPtr->selectRows.flags & SELECT_MASK) {
	    case SELECT_CLEAR:
		DeselectRow(viewPtr, rowPtr); break;
	    case SELECT_SET:
		SelectRow(viewPtr, rowPtr);   break;
	    case SELECT_TOGGLE:
		if (rowPtr->flags & SELECTED) {
		    DeselectRow(viewPtr, rowPtr);
		} else {
		    SelectRow(viewPtr, rowPtr);
		}
		break;
	    }
	}
    } else {
	for (i = from; i <= to; i++) {
	    Row *rowPtr;

	    rowPtr = viewPtr->rows[i];
	    switch (viewPtr->selectRows.flags & SELECT_MASK) {
	    case SELECT_CLEAR:
		DeselectRow(viewPtr, rowPtr); break;
	    case SELECT_SET:
		SelectRow(viewPtr, rowPtr);	break;
	    case SELECT_TOGGLE:
		if (rowPtr->flags & SELECTED) {
		    DeselectRow(viewPtr, rowPtr);
		} else {
		    SelectRow(viewPtr, rowPtr);
		}
		break;
	    }
	}
    }
    return TCL_OK;
}

static void
GetRowTitleGeometry(TableView *viewPtr, Row *rowPtr)
{
    unsigned int iw, ih, tw, th;
    unsigned int gap;

    rowPtr->titleWidth = 2 * (viewPtr->rowTitleBorderWidth + TITLE_PADX);
    rowPtr->titleHeight = 2 * (viewPtr->rowTitleBorderWidth + TITLE_PADY);
	
    gap = tw = th = iw = ih = 0;
    if (rowPtr->icon != NULL) {
	iw = IconWidth(rowPtr->icon);
	ih = IconHeight(rowPtr->icon);
	rowPtr->titleWidth += iw;
    }
    if ((rowPtr->flags & TEXTALLOC) == 0) {
	rowPtr->title = blt_table_row_label(rowPtr->row);
    }
    if (rowPtr->title != NULL) {
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, viewPtr->rowTitleFont);
	Blt_Ts_GetExtents(&ts, rowPtr->title, &tw, &th);
    }
    gap = ((iw > 0) && (tw > 0)) ? 2 : 0;
    rowPtr->titleHeight += MAX(ih, th);
    rowPtr->titleWidth += tw + gap + iw;
}


static int 
ConfigureRow(TableView *viewPtr, Row *rowPtr) 
{
    if (Blt_ConfigModified(rowSpecs, "-titlefont", "-title", "-hide", "-icon", 
	"-show", "-borderwidth", (char *)NULL)) {
	if (viewPtr->flags & ROW_TITLES) {
	    GetRowTitleGeometry(viewPtr, rowPtr);
	} 
    }
    if (Blt_ConfigModified(rowSpecs, "-style", (char *)NULL)) {
	/* If the style changed, recompute the geometry of the cells. */
	rowPtr->flags |= GEOMETRY;
	viewPtr->flags |= GEOMETRY | REDRAW;
    }
    return TCL_OK;
}


#ifdef notdef
static void
PrintEventFlags(int type)
{
    fprintf(stderr, "event flags are: ");
    if (type & TABLE_NOTIFY_COLUMN_CHANGED) {
	fprintf(stderr, "-column ");
    } 
    if (type & TABLE_NOTIFY_ROW_CHANGED) {
	fprintf(stderr, "-row ");
    } 
    if (type & TABLE_NOTIFY_CREATE) {
	fprintf(stderr, "-create ");
    } 
    if (type & TABLE_NOTIFY_DELETE) {
	fprintf(stderr, "-delete ");
    }
    if (type & TABLE_NOTIFY_MOVE) {
	fprintf(stderr, "-move ");
    }
    if (type & TABLE_NOTIFY_RELABEL) {
	fprintf(stderr, "-relabel ");
    }
    fprintf(stderr, "\n");
}
#endif

static int
TableEventProc(ClientData clientData, BLT_TABLE_NOTIFY_EVENT *eventPtr)
{
    TableView *viewPtr = clientData; 

   if (eventPtr->type & (TABLE_NOTIFY_DELETE|TABLE_NOTIFY_CREATE)) {
       if (eventPtr->type == TABLE_NOTIFY_ROWS_CREATED) {
	   if (viewPtr->flags & AUTO_ROWS) {
	       AddRows(viewPtr, eventPtr);
	   }
       } else if (eventPtr->type == TABLE_NOTIFY_COLUMNS_CREATED) {
	   if (viewPtr->flags & AUTO_COLUMNS) {
	       AddColumns(viewPtr, eventPtr);
	   }
       } else if (eventPtr->type & TABLE_NOTIFY_ROWS_DELETED) {
	   DeleteRows(viewPtr, eventPtr);
       } else if (eventPtr->type & TABLE_NOTIFY_COLUMNS_DELETED) {
	   DeleteColumns(viewPtr, eventPtr);
       }
       return TCL_OK;
    } 
    if (eventPtr->type & TABLE_NOTIFY_COLUMN_CHANGED) {
	if (eventPtr->type & TABLE_NOTIFY_RELABEL) {
	    Column *colPtr;
	    
	    colPtr = GetColumnContainer(viewPtr, eventPtr->column);
	    if (colPtr != NULL) {
		GetColumnTitleGeometry(viewPtr, colPtr);
	    }
	} else if (eventPtr->type & TABLE_NOTIFY_MOVE) {
	    RebuildTableView(viewPtr);
	    /* FIXME: handle column moves */
	}	
    }
    if (eventPtr->type & TABLE_NOTIFY_ROW_CHANGED) {
	if (eventPtr->type & TABLE_NOTIFY_RELABEL) {
	    Row *rowPtr;
	    
	    rowPtr = GetRowContainer(viewPtr, eventPtr->row);
	    if (rowPtr != NULL) {
		GetRowTitleGeometry(viewPtr, rowPtr);
	    }
	} else if (eventPtr->type & TABLE_NOTIFY_MOVE) {
	    RebuildTableView(viewPtr);
	    /* FIXME: handle row moves */
	}	
    }
    return TCL_OK;
}

static ClientData
RowTag(TableView *viewPtr, const char *key)
{
    Blt_HashEntry *hPtr;
    int isNew;				/* Not used. */

    hPtr = Blt_CreateHashEntry(&viewPtr->rowTagTable, key, &isNew);
    return Blt_GetHashKey(&viewPtr->rowTagTable, hPtr);
}

static ClientData
ColumnTag(TableView *viewPtr, const char *key)
{
    Blt_HashEntry *hPtr;
    int isNew;				/* Not used. */

    hPtr = Blt_CreateHashEntry(&viewPtr->colTagTable, key, &isNew);
    return Blt_GetHashKey(&viewPtr->colTagTable, hPtr);
}

static ClientData
CellTag(TableView *viewPtr, const char *key)
{
    Blt_HashEntry *hPtr;
    int isNew;				/* Not used. */

    hPtr = Blt_CreateHashEntry(&viewPtr->cellTagTable, key, &isNew);
    return Blt_GetHashKey(&viewPtr->cellTagTable, hPtr);
}

static void
AddBindTags(TableView *viewPtr, Blt_Chain tags, const char *string, 
	TagProc *tagProc)
{
    int argc;
    const char **argv;
    
    if (Tcl_SplitList((Tcl_Interp *)NULL, string, &argc, &argv) == TCL_OK) {
	int i;

	for (i = 0; i < argc; i++) {
	    Blt_Chain_Append(tags, (*tagProc)(viewPtr, argv[i]));
	}
	Blt_Free(argv);
    }
}

static void
AppendTagsProc(Blt_BindTable table, ClientData object, ClientData hint, 
	       Blt_Chain tags)
{
    TableView *viewPtr;
    long flags = (long)hint;
    TableObj *objPtr;

    objPtr = object;
    if (objPtr->flags & DELETED) {
	return;
    }
    viewPtr = Blt_GetBindingData(table);
    if (flags & ITEM_COLUMN_FILTER) {
	Blt_Chain_Append(tags, ColumnTag(viewPtr, "ColumnFilter"));
    } else if (flags & ITEM_COLUMN_RESIZE) {
	Blt_Chain_Append(tags, ColumnTag(viewPtr, "Resize"));
    } else if (flags & ITEM_COLUMN_TITLE) {
	Column *colPtr = object;

	Blt_Chain_Append(tags, colPtr);
	if (colPtr->bindTags != NULL) {
	    AddBindTags(viewPtr, tags, colPtr->bindTags, ColumnTag);
	}
    } else if(flags & ITEM_ROW_RESIZE) {
	Blt_Chain_Append(tags, RowTag(viewPtr, "Resize"));
    } else if(flags & ITEM_ROW_TITLE) {
	Row *rowPtr = object;

	Blt_Chain_Append(tags, rowPtr);
	if (rowPtr->bindTags != NULL) {
	    AddBindTags(viewPtr, tags, rowPtr->bindTags, RowTag);
	}
    } else if(flags & ITEM_CELL) {
	Cell *cellPtr = object;
	CellStyle *stylePtr;
	CellKey *keyPtr;
	    
	keyPtr = GetKey(cellPtr);
	stylePtr = GetCurrentStyle(viewPtr, keyPtr->rowPtr, keyPtr->colPtr,
				   cellPtr);
	if (stylePtr->name != NULL) {
	    Blt_Chain_Append(tags, CellTag(viewPtr, stylePtr->name));
	}
	Blt_Chain_Append(tags, CellTag(viewPtr, stylePtr->classPtr->className));
	Blt_Chain_Append(tags, CellTag(viewPtr, "all"));
    } else {
	fprintf(stderr, "unknown object %lx\n", (unsigned long)object);
    }
}

/*ARGSUSED*/
static ClientData
TableViewPickProc(
    ClientData clientData,
    int x, int y,			/* Screen coordinates of the test
					 * point. */
    ClientData *hintPtr)		/* (out) Context of item selected:
					 * should be ITEM_CELL,
					 * ITEM_ROW_TITLE, ITEM_ROW_RESIZE,
					 * ITEM_COLUMN_TITLE, or
					 * ITEM_COLUMN_RESIZE. */
{
    TableView *viewPtr = clientData;
    Row *rowPtr;
    Column *colPtr;
    int worldX, worldY;
    Cell *cellPtr;

    /* This simulates a cell grab. All events are assigned to the current
     * "grab" cell. */
    if (viewPtr->postPtr != NULL) {
	if (hintPtr != NULL) {
	    unsigned long flags;
	    
	    flags = ITEM_CELL;
	    *hintPtr = (ClientData)flags;
	}
	return viewPtr->postPtr;
    }
    if (viewPtr->filter.postPtr != NULL) {
	if (hintPtr != NULL) {
	    unsigned long flags;
	    
	    flags = ITEM_COLUMN_FILTER;
	    *hintPtr = (ClientData)flags;
	}
	return viewPtr->filter.postPtr;
    }
    if (hintPtr != NULL) {
	*hintPtr = NULL;
    }
    if (viewPtr->flags & GEOMETRY) {
	ComputeGeometry(viewPtr);
    }	
    if (viewPtr->flags & LAYOUT_PENDING) {
	/* Can't trust the selected items if rows/columns have been added
	 * or deleted. So recompute the layout. */
	ComputeLayout(viewPtr);
    }
    if (viewPtr->flags & SCROLL_PENDING) {
	viewPtr->flags &= ~SCROLL_PENDING;
	ComputeVisibleEntries(viewPtr);
    }
    if ((viewPtr->numVisibleRows == 0) || (viewPtr->numVisibleColumns == 0)) {
	return NULL;			/* Nothing to pick. */
    }
    viewPtr->colActivePtr = colPtr = NearestColumn(viewPtr, x, FALSE);
    viewPtr->rowActivePtr = rowPtr = NearestRow(viewPtr, y, FALSE);
    worldX = WORLDX(viewPtr, x);
    worldY = WORLDY(viewPtr, y);
    /* Determine if we're picking a column heading as opposed a cell.  */
    if ((colPtr != NULL) && ((colPtr->flags & (DISABLED|HIDDEN)) == 0) &&
	(viewPtr->flags & COLUMN_TITLES)) {

	if (y < (viewPtr->inset + viewPtr->colTitleHeight)) {
	    if (hintPtr != NULL) {
		unsigned long flags;
		
		flags = ITEM_COLUMN_TITLE;
		if (worldX >= (colPtr->worldX + colPtr->width - RESIZE_AREA)) {
		    flags |= ITEM_COLUMN_RESIZE;
		}
		*hintPtr = (ClientData)flags;
	    }
	    return colPtr;		/* We're picking the filter. */
	}
	if (y < (viewPtr->inset + viewPtr->colTitleHeight + 
		 viewPtr->colFilterHeight)) {
	    if (hintPtr != NULL) {
		unsigned long flags;
		
		flags = ITEM_COLUMN_FILTER;
		*hintPtr = (ClientData)flags;
	    }
	    return colPtr;		/* We're picking the title/resize. */
	}
    }
    /* Determine if we're picking a row heading as opposed a cell.  */
    if ((rowPtr != NULL) && ((rowPtr->flags & (DISABLED|HIDDEN)) == 0) &&
	(viewPtr->flags & ROW_TITLES) && 
	(x < (viewPtr->inset + viewPtr->rowTitleWidth))) {
	if (hintPtr != NULL) {
	    unsigned long flags;

	    flags = ITEM_ROW_TITLE;
	    if (worldY >= (rowPtr->worldY + rowPtr->height - RESIZE_AREA)) {
		flags |= ITEM_ROW_RESIZE;
	    }
	    *hintPtr = (ClientData)flags;
	}
	return rowPtr;			/* We're picking the title/resize. */
    }
    if ((colPtr == NULL) || (colPtr->flags & (DISABLED|HIDDEN))) {
	return NULL;			/* Ignore disabled columns. */
    }
    if ((rowPtr == NULL) || (rowPtr->flags & (DISABLED|HIDDEN))) {
	return NULL;			/* Ignore disabled rows. */
    }
    cellPtr = GetCell(viewPtr, rowPtr, colPtr);

    if (hintPtr != NULL) {
	unsigned long flags;

	flags = ITEM_CELL;
	*hintPtr = (ClientData)flags;
    }
    return cellPtr;
}

/*
 * TableView Procedures
 */
static void
ResetTableView(TableView *viewPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    /* Free old row, columns, and cells. */
    for (hPtr = Blt_FirstHashEntry(&viewPtr->columnTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Column *colPtr;

	colPtr = Blt_GetHashValue(hPtr);
	colPtr->hashPtr = NULL;
	colPtr->flags |= DELETED;	/* Mark the column as deleted. This
					 * prevents DestroyColumn from pruning
					 * out cells that reside in this
					 * column. We'll delete the entire
					 * cell table. */
	DestroyColumn(colPtr);
    }
    for (hPtr = Blt_FirstHashEntry(&viewPtr->rowTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Row *rowPtr;

	rowPtr = Blt_GetHashValue(hPtr);
	rowPtr->hashPtr = NULL;
	rowPtr->flags |= DELETED;	/* Mark the row as deleted. This
					 * prevents DestroyRow from pruning
					 * out cells that reside in this
					 * row. We'll delete the entire
					 * cell table. */
	DestroyRow(rowPtr);
    }
    for (hPtr = Blt_FirstHashEntry(&viewPtr->cellTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Cell *cellPtr;

	cellPtr = Blt_GetHashValue(hPtr);
	DestroyCell(cellPtr);
    }
    Blt_SetCurrentItem(viewPtr->bindTable, NULL, NULL);
    Blt_DeleteHashTable(&viewPtr->rowTable);
    Blt_DeleteHashTable(&viewPtr->columnTable);
    Blt_DeleteHashTable(&viewPtr->cellTable);
    Blt_InitHashTable(&viewPtr->cellTable, sizeof(CellKey)/sizeof(int));
    Blt_InitHashTable(&viewPtr->rowTable, BLT_ONE_WORD_KEYS);
    Blt_InitHashTable(&viewPtr->columnTable, BLT_ONE_WORD_KEYS);
    if (viewPtr->rows != NULL) {
	Blt_Free(viewPtr->rows);
	viewPtr->rows = NULL;
    }
    if (viewPtr->columns != NULL) {
	Blt_Free(viewPtr->columns);
	viewPtr->columns = NULL;
    }
    if (viewPtr->visibleRows != NULL) {
	Blt_Free(viewPtr->visibleRows);
	viewPtr->visibleRows = NULL;
    }
    if (viewPtr->visibleColumns != NULL) {
	Blt_Free(viewPtr->visibleColumns);
	viewPtr->visibleColumns = NULL;
    }
    viewPtr->numRows = viewPtr->numColumns = 0;
    viewPtr->numVisibleRows = viewPtr->numVisibleColumns = 0;
    ClearSelections(viewPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TableViewFreeProc --
 *
 * 	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 * 	clean up the internal structure of a TableView at a safe time (when
 * 	no-one is using it anymore).
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
TableViewFreeProc(DestroyData dataPtr) /* Pointer to the widget record. */
{
    TableView *viewPtr = (TableView *)dataPtr;

    ResetTableView(viewPtr);
    if (viewPtr->table != NULL) {
	blt_table_close(viewPtr->table);
	viewPtr->rowNotifier = NULL;
	viewPtr->colNotifier = NULL;
	viewPtr->table = NULL;
    }
    iconOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    tableOption.clientData = viewPtr;
    Blt_FreeOptions(tableSpecs, (char *)viewPtr, viewPtr->display, 0);
    Blt_FreeOptions(filterSpecs, (char *)viewPtr, viewPtr->display, 0);
    if (viewPtr->tkwin != NULL) {
	Tk_DeleteSelHandler(viewPtr->tkwin, XA_PRIMARY, XA_STRING);
    }
    Blt_DestroyBindingTable(viewPtr->bindTable);
    /* Destroy remaing styles after freeing table option but before dumping
     * icons.  The styles may be using icons. */
    DestroyStyles(viewPtr);
    DestroyIcons(viewPtr);
    Blt_Chain_Destroy(viewPtr->selectRows.list);
    Blt_DeleteHashTable(&viewPtr->cellTable);
    Blt_DeleteHashTable(&viewPtr->rowTable);
    Blt_DeleteHashTable(&viewPtr->columnTable);
    Blt_DeleteHashTable(&viewPtr->rowTagTable);
    Blt_DeleteHashTable(&viewPtr->colTagTable);
    Blt_DeleteHashTable(&viewPtr->cellTagTable);
    Blt_DeleteHashTable(&viewPtr->uidTable);
    Blt_Pool_Destroy(viewPtr->rowPool);
    Blt_Pool_Destroy(viewPtr->columnPool);
    Blt_Pool_Destroy(viewPtr->cellPool);
    Blt_Free(viewPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TableViewEventProc --
 *
 * 	This procedure is invoked by the Tk dispatcher for various events on
 * 	tableview widgets.
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
TableViewEventProc(ClientData clientData, XEvent *eventPtr)
{
    TableView *viewPtr = clientData;

    if (eventPtr->type == Expose) {
	if (eventPtr->xexpose.count == 0) {
	    viewPtr->flags |= SCROLL_PENDING;
	    EventuallyRedraw(viewPtr);
	    Blt_PickCurrentItem(viewPtr->bindTable);
	}
    } else if (eventPtr->type == ConfigureNotify) {
	/* Size of the viewport has changed. Recompute visibilty. */
	viewPtr->flags |= LAYOUT_PENDING | SCROLL_PENDING;
	EventuallyRedraw(viewPtr);
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    if (eventPtr->type == FocusIn) {
		viewPtr->flags |= FOCUS;
	    } else {
		viewPtr->flags &= ~FOCUS;
	    }
	    EventuallyRedraw(viewPtr);
	}
    } else if (eventPtr->type == DestroyNotify) {
	if (viewPtr->tkwin != NULL) {
	    viewPtr->tkwin = NULL;
	    Tcl_DeleteCommandFromToken(viewPtr->interp, viewPtr->cmdToken);
	}
	if (viewPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayProc, viewPtr);
	}
	if (viewPtr->flags & SELECT_PENDING) {
	    Tcl_CancelIdleCall(SelectCommandProc, viewPtr);
	}
	Tcl_EventuallyFree(viewPtr, TableViewFreeProc);
    }
}

typedef struct {
    int count;
    int length;
    Tcl_DString *dsPtr;
} CsvWriter;

/* Selection Procedures */
static void
CsvStartRecord(CsvWriter *writerPtr)
{
    writerPtr->count = 0;
}

static void
CsvEndRecord(CsvWriter *writerPtr)
{
    Tcl_DStringAppend(writerPtr->dsPtr, "\n", 1);
    writerPtr->length++;
}

static void
CsvAppendRecord(CsvWriter *writerPtr, const char *field, int length, 
		BLT_TABLE_COLUMN_TYPE type)
{
    const char *fp;
    char *p;
    int extra, doQuote;

    doQuote = (type == TABLE_COLUMN_TYPE_STRING);
    extra = 0;
    if (field == NULL) {
	length = 0;
    } else {
	for (fp = field; *fp != '\0'; fp++) {
	    if ((*fp == '\n') || (*fp == ',') || (*fp == ' ') || 
		(*fp == '\t')) {
		doQuote = TRUE;
	    } else if (*fp == '"') {
		doQuote = TRUE;
		extra++;
	    }
	}
	if (doQuote) {
	    extra += 2;
	}
	if (length < 0) {
	    length = fp - field;
	}
    }
    if (writerPtr->count > 0) {
	Tcl_DStringAppend(writerPtr->dsPtr, ",", 1);
	writerPtr->length++;
    }
    length = length + extra + writerPtr->length;
    Tcl_DStringSetLength(writerPtr->dsPtr, length);
    p = Tcl_DStringValue(writerPtr->dsPtr) + writerPtr->length;
    writerPtr->length = length;
    if (field != NULL) {
	if (doQuote) {
	    *p++ = '"';
	}
	for (fp = field; *fp != '\0'; fp++) {
	    if (*fp == '"') {
		*p++ = '"';
	    }
	    *p++ = *fp;
	}
	if (doQuote) {
	    *p++ = '"';
	}
    }
    writerPtr->count++;
}

static void
CsvAppendValue(CsvWriter *writerPtr, TableView *viewPtr, Row *rowPtr, 
	    Column *colPtr)
{
    const char *string;
    int length;
    BLT_TABLE_COLUMN_TYPE type;
    
    string = blt_table_get_string(viewPtr->table, rowPtr->row, colPtr->column);
    length = strlen(string);
    type = blt_table_column_type(colPtr->column);
    CsvAppendRecord(writerPtr, string, length, type);
}

static void
CsvAppendRow(CsvWriter *writerPtr, TableView *viewPtr, Row *rowPtr)
{
    long i;

    CsvStartRecord(writerPtr);
    for (i = 0; i < viewPtr->numColumns; i++) {
	Column *colPtr;

	colPtr = viewPtr->columns[i];
	if ((colPtr->flags & HIDDEN) == 0) {
	    CsvAppendValue(writerPtr, viewPtr, rowPtr, colPtr);
	}
    }
    CsvEndRecord(writerPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * SelectionProc --
 *
 *	This procedure is called back by Tk when the selection is requested by
 *	someone.  It returns part or all of the selection in a buffer provided
 *	by the caller.
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
    int offset,				/* Offset within selection of first
					 * character to be returned. */
    char *buffer,			/* Location in which to place
					 * selection. */
    int maxBytes)			/* Maximum number of bytes to place at
					 * buffer, not including terminating
					 * NULL character. */
{
    Tcl_DString ds;
    TableView *viewPtr = clientData;
    int size;
    CsvWriter writer;

    if ((viewPtr->flags & SELECT_EXPORT) == 0) {
	return -1;
    }
    /*
     * Retrieve the names of the selected entries.
     */
    Tcl_DStringInit(&ds);
    writer.dsPtr = &ds;
    writer.length = 0;
    writer.count = 0;
    if (viewPtr->selectMode == SELECT_CELLS) {
	long i;

	for (i = viewPtr->selectCells.anchorPtr->rowPtr->index; 
	     i <= viewPtr->selectCells.markPtr->rowPtr->index; i++) {
	    long j;
	    Row *rowPtr;

	    rowPtr = viewPtr->rows[i];
	    CsvStartRecord(&writer);
	    for (j = viewPtr->selectCells.anchorPtr->colPtr->index; 
		 j <= viewPtr->selectCells.markPtr->colPtr->index; j++) {
		Column *colPtr;

		colPtr = viewPtr->columns[i];
		CsvAppendValue(&writer, viewPtr, rowPtr, colPtr);
	    }
	    CsvEndRecord(&writer);
	}
    } else {
	if (viewPtr->flags & SELECT_SORTED) {
	    Blt_ChainLink link;

	    
	    for (link = Blt_Chain_FirstLink(viewPtr->selectRows.list); 
		 link != NULL; link = Blt_Chain_NextLink(link)) {
		Row *rowPtr;

		rowPtr = Blt_Chain_GetValue(link);
		CsvAppendRow(&writer, viewPtr, rowPtr);
	    }
	} else {
	    long i;

	    for (i = 0; i < viewPtr->numRows; i++) {
		Row *rowPtr;

		rowPtr = viewPtr->rows[i];
		if (rowPtr->flags & SELECTED) {
		    CsvAppendRow(&writer, viewPtr, rowPtr);
		}
	    }
	}
    }
    size = Tcl_DStringLength(&ds) - offset;
    strncpy(buffer, Tcl_DStringValue(&ds) + offset, maxBytes);
    Tcl_DStringFree(&ds);
    buffer[maxBytes] = '\0';
    return (size > maxBytes) ? maxBytes : size;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureTableView --
 *
 *	Updates the GCs and other information associated with the tableview
 *	widget.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is returned,
 *	then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font, etc. get
 *	set for viewPtr; old resources get freed, if there were any.  The widget
 *	is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureTableView(Tcl_Interp *interp, TableView *viewPtr)	
{
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    /* GC for normal row title. */
    gcMask = GCForeground | GCFont;
    gcValues.foreground = viewPtr->rowNormalTitleFg->pixel;
    gcValues.font = Blt_Font_Id(viewPtr->rowTitleFont);
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (viewPtr->rowNormalTitleGC != NULL) {
	Tk_FreeGC(viewPtr->display, viewPtr->rowNormalTitleGC);
    }
    viewPtr->rowNormalTitleGC = newGC;

    /* GC for active row title. */
    gcValues.foreground = viewPtr->rowActiveTitleFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (viewPtr->rowActiveTitleGC != NULL) {
	Tk_FreeGC(viewPtr->display, viewPtr->rowActiveTitleGC);
    }
    viewPtr->rowActiveTitleGC = newGC;

    /* GC for disabled row title. */
    gcValues.foreground = viewPtr->rowDisabledTitleFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (viewPtr->rowDisabledTitleGC != NULL) {
	Tk_FreeGC(viewPtr->display, viewPtr->rowDisabledTitleGC);
    }
    viewPtr->rowDisabledTitleGC = newGC;

    /* GC for normal column title. */
    gcMask = GCForeground | GCFont;
    gcValues.foreground = viewPtr->colNormalTitleFg->pixel;
    gcValues.font = Blt_Font_Id(viewPtr->colTitleFont);
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (viewPtr->colNormalTitleGC != NULL) {
	Tk_FreeGC(viewPtr->display, viewPtr->colNormalTitleGC);
    }
    viewPtr->colNormalTitleGC = newGC;

    /* GC for active column title. */
    gcValues.foreground = viewPtr->colActiveTitleFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (viewPtr->colActiveTitleGC != NULL) {
	Tk_FreeGC(viewPtr->display, viewPtr->colActiveTitleGC);
    }
    viewPtr->colActiveTitleGC = newGC;

    /* GC for disabled row title. */
    gcValues.foreground = viewPtr->colDisabledTitleFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (viewPtr->colDisabledTitleGC != NULL) {
	Tk_FreeGC(viewPtr->display, viewPtr->colDisabledTitleGC);
    }
    viewPtr->colDisabledTitleGC = newGC;

    viewPtr->inset = viewPtr->highlightWidth + viewPtr->borderWidth + INSET_PAD;

    /*
     * If the table object was changed, we need to setup the new one.
     */
    if (Blt_ConfigModified(tableSpecs, "-table", (char *)NULL)) {
	if (AttachTable(interp, viewPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    /*
     * These options change the layout of the box.  Mark the widget for update.
     */
    if (Blt_ConfigModified(tableSpecs, "-width", "-height", "-hide", 
			   (char *)NULL)) {
	viewPtr->flags |= SCROLL_PENDING;
    }
    if (Blt_ConfigModified(tableSpecs, "-font", "-linespacing", (char *)NULL)) {
	viewPtr->flags |= GEOMETRY;
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureFilters --
 *
 *	Updates the GCs and other information associated with the tableview
 *	widget.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is returned,
 *	then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font, etc. get
 *	set for viewPtr; old resources get freed, if there were any.  The widget
 *	is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureFilters(Tcl_Interp *interp, TableView *viewPtr)	
{
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;
    FilterInfo *filterPtr;

    filterPtr = &viewPtr->filter;

    /* GC for normal filter. */
    gcMask = GCForeground | GCFont;
    gcValues.foreground = filterPtr->normalFg->pixel;
    gcValues.font = Blt_Font_Id(filterPtr->font);
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (filterPtr->normalGC != NULL) {
	Tk_FreeGC(viewPtr->display, filterPtr->normalGC);
    }
    filterPtr->normalGC = newGC;

    /* GC for active filter. */
    gcValues.foreground = filterPtr->activeFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (filterPtr->activeGC != NULL) {
	Tk_FreeGC(viewPtr->display, filterPtr->activeGC);
    }
    filterPtr->activeGC = newGC;

    /* GC for disabled filter. */
    gcValues.foreground = filterPtr->disabledFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (filterPtr->disabledGC != NULL) {
	Tk_FreeGC(viewPtr->display, filterPtr->disabledGC);
    }
    filterPtr->disabledGC = newGC;

    /* GC for selected (posted) filter. */
    gcValues.foreground = filterPtr->selectFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (filterPtr->selectGC != NULL) {
	Tk_FreeGC(viewPtr->display, filterPtr->selectGC);
    }
    filterPtr->selectGC = newGC;

    /* GC for highlighted filter. */
    gcValues.foreground = filterPtr->highlightFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (filterPtr->highlightGC != NULL) {
	Tk_FreeGC(viewPtr->display, filterPtr->highlightGC);
    }
    filterPtr->highlightGC = newGC;

    /*
     * These options change the layout of the box.  Mark the widget for update.
     */
    if (Blt_ConfigModified(filterSpecs, "-show", "-hide", (char *)NULL)) {
	viewPtr->flags |= SCROLL_PENDING;
    }
    if (Blt_ConfigModified(tableSpecs, "-font", (char *)NULL)) {
	viewPtr->flags |= LAYOUT_PENDING;
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawColumnFilter --
 *
 *	Draws the combo filter button given the screen coordinates and the
 *	value to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The combo filter button value is drawn.
 *
 *      +---------------------------+	
 *	|b|x|icon|x|text|x|arrow|x|b|	
 *      +---------------------------+
 *
 * b = filter borderwidth
 * x = padx 
 *---------------------------------------------------------------------------
 */
static void
DrawColumnFilter(TableView *viewPtr, Column *colPtr, Drawable drawable, 
		 int x, int y)
{
    Blt_Bg bg, filterBg;
    GC gc;
    unsigned int colWidth, rowHeight, filterWidth, filterHeight;
    int relief;
    XColor *fg;
    FilterInfo *filterPtr;

    filterPtr = &viewPtr->filter;
    if (viewPtr->colTitleHeight < 1) {
	return;
    }
    relief = filterPtr->relief;
    if (colPtr->flags & DISABLED) {	/* Disabled  */
	filterBg = bg = filterPtr->disabledBg;
	gc = filterPtr->disabledGC;
	fg = filterPtr->disabledFg;
    } else if (colPtr == filterPtr->postPtr) {	 /* Selected */
	filterBg = bg = filterPtr->selectBg;
	gc = filterPtr->selectGC;
	relief = filterPtr->selectRelief;
	fg = filterPtr->selectFg;
    } else if (colPtr == filterPtr->activePtr) {  /* Active */
        filterBg = filterPtr->normalBg;
	bg = filterPtr->activeBg;
	gc = filterPtr->activeGC;
	relief = filterPtr->activeRelief;
	fg = filterPtr->activeFg;
    } else if (colPtr->flags & FILTERHIGHLIGHT) { /* Highlighted */
	filterBg = bg = filterPtr->highlightBg;
	gc = filterPtr->highlightGC;
	fg = filterPtr->highlightFg;
	relief = TK_RELIEF_FLAT;
    } else {				/* Normal */
	filterBg = bg = filterPtr->normalBg;
	gc = filterPtr->normalGC;
	fg = filterPtr->normalFg;
	relief = TK_RELIEF_FLAT;
    }
    rowHeight = viewPtr->colFilterHeight;
    colWidth  = colPtr->width;

    /* Draw background. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, colWidth,
	rowHeight, 0, TK_RELIEF_FLAT);
    x += filterPtr->outerBorderWidth;
    y += filterPtr->outerBorderWidth;
    colWidth -= 2 * filterPtr->outerBorderWidth;
    rowHeight -= 2 * filterPtr->outerBorderWidth;
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, filterBg, x, y, colWidth,
	rowHeight, filterPtr->borderWidth, filterPtr->relief);
    rowHeight -= 2 * (filterPtr->borderWidth + TITLE_PADY);
    colWidth  -= 2 * (filterPtr->borderWidth + TITLE_PADX);
    x += filterPtr->borderWidth + TITLE_PADX;
    y += filterPtr->borderWidth + TITLE_PADY;
    filterHeight = rowHeight;
    filterWidth = colPtr->width - 
	2 * (filterPtr->borderWidth + TITLE_PADX);
    /* Justify (x) and center (y) the contents of the cell. */
    if (rowHeight > filterHeight) {
	y += (rowHeight - filterHeight) / 2;
    }
    if (colPtr->filterIcon != NULL) {
	int ix, iy, gap;

	ix = x;
	iy = y;
	if (filterHeight > IconHeight(colPtr->filterIcon)) {
	    iy += (filterHeight - IconHeight(colPtr->filterIcon)) / 2;
	}
	Tk_RedrawImage(IconBits(colPtr->filterIcon), 0, 0, 
		IconWidth(colPtr->filterIcon), IconHeight(colPtr->filterIcon), 
		drawable, ix, iy);
	gap = 0;
	if (colPtr->filterText != NULL) {
	    gap = TITLE_PADX;
	}
	x += IconWidth(colPtr->filterIcon) + gap;
	filterWidth -= IconWidth(colPtr->filterIcon) + gap;
    }
    if (colPtr->filterText != NULL) {
	int tx, ty;
	int maxLength;

	tx = x;
	ty = y;
	if (filterHeight > colPtr->filterTextHeight) {
	    ty += (filterHeight - colPtr->filterTextHeight) / 2;
	}
	maxLength = filterWidth;
	if ((colPtr == filterPtr->activePtr)||(colPtr == filterPtr->postPtr)) {
	    maxLength -= viewPtr->arrowWidth + TITLE_PADX;
	}
	if (maxLength > 0) {
	    TextStyle ts;
	    TextLayout *textPtr;
	    
	    Blt_Ts_InitStyle(ts);
	    Blt_Ts_SetFont(ts, filterPtr->font);
	    Blt_Ts_SetGC(ts, gc);
	    Blt_Ts_SetMaxLength(ts, maxLength);
	    textPtr = Blt_Ts_CreateLayout(colPtr->filterText, -1, &ts);
	    Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, textPtr, &ts, tx, ty);
	    Blt_Free(textPtr);
	}
    }
    if ((colPtr == filterPtr->activePtr) || (colPtr == filterPtr->postPtr)) {
	int ax, ay, aw, ah;

	aw = viewPtr->arrowWidth + (2 * filterPtr->borderWidth);
	ah = filterHeight;
	ax = x + filterWidth - aw - 1;
	ay = y;
	Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, ax - 1, ay, aw, ah, 
		filterPtr->borderWidth, relief);
	aw -= 2 * filterPtr->borderWidth;
	ax += filterPtr->borderWidth;
	Blt_DrawArrow(viewPtr->display, drawable, fg, ax, ay, aw, ah, 
		      filterPtr->borderWidth, ARROW_DOWN);
    }
}

static void
DisplayCell(Cell *cellPtr, Drawable drawable, int buffer)
{
    CellKey *keyPtr;
    Row *rowPtr;
    Column *colPtr;
    int x, y;
    int x1, x2, y1, y2;
    int clipped;
    CellStyle *stylePtr;
    TableView *viewPtr;

    viewPtr = cellPtr->viewPtr;
    keyPtr = GetKey(cellPtr);
    rowPtr = keyPtr->rowPtr;
    colPtr = keyPtr->colPtr;
    stylePtr = GetCurrentStyle(viewPtr, rowPtr, colPtr, cellPtr);
    x1 = x = SCREENX(viewPtr, colPtr->worldX);
    y1 = y = SCREENY(viewPtr, rowPtr->worldY);
    x2 = x1 + colPtr->width;
    y2 = y1 + rowPtr->height;
    clipped = FALSE;
    if ((x1 >= (Tk_Width(viewPtr->tkwin) - viewPtr->inset)) ||
	(y1 >= (Tk_Height(viewPtr->tkwin) - viewPtr->inset)) ||
	(x2 <= (viewPtr->inset + viewPtr->rowTitleWidth)) ||
	(y2 <= (viewPtr->inset + + viewPtr->colFilterHeight +
		viewPtr->colTitleHeight))) {
	return;				/* Cell isn't in viewport.  This can
					 * happen when the active cell is
					 * scrolled off screen. */
    }
    if (x1 < (viewPtr->inset + viewPtr->rowTitleWidth)) {
	x1 = viewPtr->inset + viewPtr->rowTitleWidth;
	clipped = TRUE;
    }
    if (x2 >= (Tk_Width(viewPtr->tkwin) - viewPtr->inset)) {
	x2 = Tk_Width(viewPtr->tkwin) - viewPtr->inset;
	clipped = TRUE;
    }
    if (y1 < (viewPtr->inset + viewPtr->colFilterHeight + 
	      viewPtr->colTitleHeight)) {
	y1 = viewPtr->inset + viewPtr->colFilterHeight + 
	    viewPtr->colTitleHeight;
	clipped = TRUE;
    }
    if (y2 >= (Tk_Height(viewPtr->tkwin) - viewPtr->inset)) {
	y2 = Tk_Height(viewPtr->tkwin) - viewPtr->inset;
	clipped = TRUE;
    }
    if ((buffer) || (clipped)) {
	long w, h, dx, dy;
	Pixmap pixmap;

	w = x2 - x1;
	h = y2 - y1;
	dx = x1 - x;
	dy = y1 - y;

	if ((h < 1) || (w < 1)) {
	    return;
	}
	/* Draw into a pixmap and then copy it into the drawable.  */
	pixmap = Blt_GetPixmap(viewPtr->display, Tk_WindowId(viewPtr->tkwin), 
		w, h, Tk_Depth(viewPtr->tkwin));
	(*stylePtr->classPtr->drawProc)(cellPtr, pixmap, stylePtr, -dx, -dy);
	XCopyArea(viewPtr->display, pixmap, drawable, viewPtr->rowNormalTitleGC,
		  0, 0, w, h, x + dx, y + dy);
	Tk_FreePixmap(viewPtr->display, pixmap);
    } else {
	(*stylePtr->classPtr->drawProc)(cellPtr, drawable, stylePtr, x, y);
    }
}

static void
DrawColumnTitle(TableView *viewPtr, Column *colPtr, Drawable drawable, int x, 
		int y)
{
    Blt_Bg bg;
    XColor *fg;
    GC gc;
    unsigned int wanted, igap, agap;
    int relief;
    unsigned int aw, ah, iw, ih, tw, th;
    unsigned int colWidth, colHeight;
    SortInfo *sortPtr;

    sortPtr = &viewPtr->sort;
    if (viewPtr->colTitleHeight < 1) {
	return;
    }
    relief = colPtr->titleRelief;
    if (colPtr->flags & DISABLED) {	/* Disabled  */
	bg = viewPtr->colDisabledTitleBg;
	gc = viewPtr->colDisabledTitleGC;
	fg = viewPtr->colDisabledTitleFg;
    } else if (colPtr == viewPtr->colActiveTitlePtr) {  /* Active */
	bg = viewPtr->colActiveTitleBg;
	gc = viewPtr->colActiveTitleGC;
	relief = colPtr->activeTitleRelief;
	fg = viewPtr->colActiveTitleFg;
    } else {				/* Normal */
	bg = viewPtr->colNormalTitleBg;
	gc = viewPtr->colNormalTitleGC;
	fg = viewPtr->colNormalTitleFg;
    }

    colWidth = colPtr->width;
    colHeight = viewPtr->colTitleHeight;
    /* Clear the title area by drawing the background. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, colWidth, 
	viewPtr->colTitleHeight, viewPtr->colTitleBorderWidth, relief);

    colWidth -= 2 * (viewPtr->colTitleBorderWidth + TITLE_PADX);
    colHeight -= 2 * (viewPtr->colTitleBorderWidth + TITLE_PADY);

    x += viewPtr->colTitleBorderWidth + TITLE_PADX;
    y += viewPtr->colTitleBorderWidth + TITLE_PADY;

    tw = th = iw = ih = aw = ah = 0;
    agap = igap = 0;
    if (colPtr == sortPtr->firstPtr) {
	if ((sortPtr->up != NULL) && (sortPtr->down != NULL)) {
	    aw = MAX(IconWidth(sortPtr->up), IconWidth(sortPtr->down));
	    ah = MAX(IconHeight(sortPtr->up), IconHeight(sortPtr->down));
	} else {
	    aw = ah = 17;
	}
    }
    if (colPtr->icon != NULL) {
	iw = IconWidth(colPtr->icon);
	ih = IconHeight(colPtr->icon);
    }
    if ((iw > 0) && (colPtr->textWidth > 0)) {
	igap = TITLE_PADX;
    }
    if ((colPtr == viewPtr->sort.firstPtr) && (aw > 0)) {
	agap = TITLE_PADX;
    }
    tw = colPtr->textWidth;
    th = colPtr->textHeight;
    wanted = tw + aw + iw + agap + igap;
    if (colWidth > wanted) {
	switch (colPtr->titleJustify) {
	case TK_JUSTIFY_RIGHT:
	    x += colWidth - wanted;
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (colWidth - wanted) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    } else if (colWidth < wanted) {
	tw = colWidth - aw + iw + agap + igap;
    }
    if (colPtr->icon != NULL) {
	int iy;

	/* Center the icon vertically.  We already know the column title is at
	 * least as tall as the icon. */
	iy = y;
	if (colHeight > ih) {
	    iy += (colHeight - ih) / 2;
	}
	Tk_RedrawImage(IconBits(colPtr->icon), 0, 0, iw, ih, drawable, x, iy);
	x += iw + igap;
    }
    if (colPtr->title != NULL) {
	TextStyle ts;
	int ty;

	ty = y;
	if (colHeight > colPtr->textHeight) {
	    ty += (colHeight - colPtr->textHeight) / 2;
	}
	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, viewPtr->colTitleFont);
	Blt_Ts_SetGC(ts, gc);
	Blt_Ts_SetMaxLength(ts, tw);
	Blt_Ts_DrawText(viewPtr->tkwin, drawable, colPtr->title,-1, &ts, x, ty);
	x += tw + agap;
    }
    if (colPtr == viewPtr->sort.firstPtr) {
	int ay;

	ay = y + (colHeight - ah) / 2;
	if ((viewPtr->sort.decreasing) && (viewPtr->sort.up != NULL)) {
	    Tk_RedrawImage(IconBits(viewPtr->sort.up), 0, 0, aw, ah, drawable, 
		x, ay);
	} else if (viewPtr->sort.down != NULL) {
	    Tk_RedrawImage(IconBits(viewPtr->sort.down), 0, 0, aw, ah, drawable,
		x, ay);
	} else {
	    Blt_DrawArrow(viewPtr->display, drawable, fg, x, ay, aw, ah, 
		viewPtr->colTitleBorderWidth, 
		(viewPtr->sort.decreasing) ? ARROW_UP : ARROW_DOWN);
	}
    }
}

static void
DrawRowTitle(TableView *viewPtr, Row *rowPtr, Drawable drawable, int x, int y)
{
    Blt_Bg bg;
    GC gc;
    int h, dy;
    int avail, need;
    int relief;

    if (viewPtr->rowTitleWidth < 1) {
	return;
    }
    relief = rowPtr->titleRelief;
    if (rowPtr->flags & DISABLED) {	/* Disabled  */
	bg = viewPtr->rowDisabledTitleBg;
	gc = viewPtr->rowDisabledTitleGC;
    } else if (rowPtr == viewPtr->rowActiveTitlePtr) {  /* Active */
	bg = viewPtr->rowActiveTitleBg;
	gc = viewPtr->rowActiveTitleGC;
	relief = rowPtr->activeTitleRelief;
    } else {				/* Normal */
	bg = viewPtr->rowNormalTitleBg;
	gc = viewPtr->rowNormalTitleGC;
    }
    dy = y;
    h = rowPtr->height;
    if (rowPtr->index == (viewPtr->numRows - 1)) {
	/* If there's any room left over, let the last row take it. */
	h = Tk_Height(viewPtr->tkwin) - y;
    }
    /* Clear the title area by drawing the background. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, dy, 
	viewPtr->rowTitleWidth, h, viewPtr->rowTitleBorderWidth, relief);

    avail = viewPtr->rowTitleWidth - 
	2 * (viewPtr->rowTitleBorderWidth + TITLE_PADX); 
    need  = rowPtr->titleWidth     - 
	2 * (viewPtr->rowTitleBorderWidth + TITLE_PADX);
    x += viewPtr->rowTitleBorderWidth + TITLE_PADX;
    y += viewPtr->rowTitleBorderWidth + TITLE_PADY;
    if (avail > need) {
	switch (rowPtr->titleJustify) {
	case TK_JUSTIFY_RIGHT:
	    x += avail - need;
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (avail - need) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }
    if (rowPtr->icon != NULL) {
	int ix, iy, iw, ih;

	ih = IconHeight(rowPtr->icon);
	iw = IconWidth(rowPtr->icon);
	ix = x;
	/* Center the icon vertically.  We already know the column title is at
	 * least as tall as the icon. */
	iy = y + (rowPtr->titleHeight - ih) / 2;
	Tk_RedrawImage(IconBits(rowPtr->icon), 0, 0, iw, ih, drawable, ix, iy);
	x += iw + 2;
	avail -= iw + 2;
    }
    if (rowPtr->title != NULL) {
	TextStyle ts;
	int ty;

	ty = y;
	if (rowPtr->height > rowPtr->titleHeight) {
	    ty += (rowPtr->height - rowPtr->titleHeight) / 2;
	}
	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, viewPtr->rowTitleFont);
	Blt_Ts_SetGC(ts, gc);
	Blt_Ts_SetMaxLength(ts, avail);
	Blt_Ts_DrawText(viewPtr->tkwin, drawable, rowPtr->title,-1, &ts, x, ty);
    }
}

static void
DisplayRowTitle(TableView *viewPtr, Row *rowPtr, Drawable drawable)
{
    int x, y, y1, y2;
    int clipped;

    x = viewPtr->inset;
    y1 = y = SCREENY(viewPtr, rowPtr->worldY);
    y2 = y1 + rowPtr->height;
    if ((y1 >= (Tk_Height(viewPtr->tkwin) - viewPtr->inset)) ||
	(y2 <= (viewPtr->inset + viewPtr->colFilterHeight +
		viewPtr->colTitleHeight))) {
	return;				/* Row starts after the window or ends
					 * before the window. */
    }
    clipped = FALSE;
    if (y1 < (viewPtr->inset + viewPtr->colFilterHeight + 
	      viewPtr->colTitleHeight)) {
	y1 = viewPtr->inset + viewPtr->colFilterHeight +viewPtr->colTitleHeight;
	clipped = TRUE;
    }
    if (y2 >= (Tk_Height(viewPtr->tkwin) - viewPtr->inset)) {
	y2 = Tk_Height(viewPtr->tkwin) - viewPtr->inset;
	clipped = TRUE;
    }
    if (clipped) {
	long h, dy;
	Pixmap pixmap;

	h = y2 - y1;
	dy = y1 - y;
	/* Draw into a pixmap and then copy it into the drawable.  */
	pixmap = Blt_GetPixmap(viewPtr->display, Tk_WindowId(viewPtr->tkwin), 
		viewPtr->rowTitleWidth, h, Tk_Depth(viewPtr->tkwin));
	DrawRowTitle(viewPtr, rowPtr, pixmap, 0, -dy);
	XCopyArea(viewPtr->display, pixmap, drawable, viewPtr->rowNormalTitleGC,
		  0, 0, viewPtr->rowTitleWidth, h, x, y + dy);
	Tk_FreePixmap(viewPtr->display, pixmap);
    } else {
	DrawRowTitle(viewPtr, rowPtr, drawable, x, y);
    }
}

static void
DisplayColumnTitle(TableView *viewPtr, Column *colPtr, Drawable drawable)
{
    int x, y, x1, x2;
    int clipped;

    y = viewPtr->inset;
    x1 = x = SCREENX(viewPtr, colPtr->worldX);
    x2 = x1 + colPtr->width;
    if ((x1 >= (Tk_Width(viewPtr->tkwin) - viewPtr->inset)) ||
	(x2 <= (viewPtr->inset + viewPtr->rowTitleWidth))) {
	return;				/* Column starts after the window or
					 * ends before the the window. */
    }
    clipped = FALSE;
    if (x1 < (viewPtr->inset + viewPtr->rowTitleWidth)) {
	x1 = viewPtr->inset + viewPtr->rowTitleWidth;
	clipped = TRUE;
    }
    if (x2 > (Tk_Width(viewPtr->tkwin) - viewPtr->inset)) {
	x2 = Tk_Width(viewPtr->tkwin) - viewPtr->inset;
	clipped = TRUE;
    }
    if (clipped) {
	long w, dx;
	Pixmap pixmap;

	w = x2 - x1;
	dx = x1 - x;
	/* Draw into a pixmap and then copy it into the drawable.  */
	pixmap = Blt_GetPixmap(viewPtr->display, Tk_WindowId(viewPtr->tkwin), 
		w, viewPtr->colTitleHeight, Tk_Depth(viewPtr->tkwin));
	DrawColumnTitle(viewPtr, colPtr, pixmap, -dx, 0);
	XCopyArea(viewPtr->display, pixmap, drawable, viewPtr->colNormalTitleGC,
		  0, 0, w, viewPtr->colTitleHeight, x + dx, y);
	Tk_FreePixmap(viewPtr->display, pixmap);
    } else {
	DrawColumnTitle(viewPtr, colPtr, drawable, x, y);
    }
}

static void
DisplayColumnFilter(TableView *viewPtr, Column *colPtr, Drawable drawable)
{
    int x, y, x1, x2;
    int clipped;

    y = viewPtr->inset + viewPtr->colTitleHeight;
    x1 = x = SCREENX(viewPtr, colPtr->worldX);
    x2 = x1 + colPtr->width;
    if ((x1 >= (Tk_Width(viewPtr->tkwin) - viewPtr->inset)) ||
	(x2 <= (viewPtr->inset + viewPtr->rowTitleWidth))) {
	return;				/* Column starts after the window or
					 * ends before the the window. */
    }
    clipped = FALSE;
    if (x1 < (viewPtr->inset + viewPtr->rowTitleWidth)) {
	x1 = viewPtr->inset + viewPtr->rowTitleWidth;
	clipped = TRUE;
    }
    if (x2 > (Tk_Width(viewPtr->tkwin) - viewPtr->inset)) {
	x2 = Tk_Width(viewPtr->tkwin) - viewPtr->inset;
	clipped = TRUE;
    }
    if (clipped) {
	long w, dx;
	Pixmap pixmap;

	w = x2 - x1;
	dx = x1 - x;
	/* Draw into a pixmap and then copy it into the drawable.  */
	pixmap = Blt_GetPixmap(viewPtr->display, Tk_WindowId(viewPtr->tkwin), 
		w, viewPtr->colFilterHeight, Tk_Depth(viewPtr->tkwin));
	DrawColumnFilter(viewPtr, colPtr, pixmap, -dx, 0);
	XCopyArea(viewPtr->display, pixmap, drawable, viewPtr->colNormalTitleGC,
		0, 0, w, viewPtr->colFilterHeight, x + dx, y);
	Tk_FreePixmap(viewPtr->display, pixmap);
    } else {
	DrawColumnFilter(viewPtr, colPtr, drawable, x, y);
    }
}

static void
DisplayColumnTitles(TableView *viewPtr, Drawable drawable)
{
    long i;

    for (i = 0; i < viewPtr->numVisibleColumns; i++) {
	Column *colPtr;

	colPtr = viewPtr->visibleColumns[i];
	if ((colPtr->flags & HIDDEN) == 0) {
	    DisplayColumnTitle(viewPtr, colPtr, drawable);
	    if (viewPtr->flags & AUTOFILTERS) {
		DisplayColumnFilter(viewPtr, colPtr, drawable);
	    }
	}
    }
}

static void
DisplayRowTitles(TableView *viewPtr, Drawable drawable)
{
    long i;

    for (i = 0; i < viewPtr->numVisibleRows; i++) {
	Row *rowPtr;

	rowPtr = viewPtr->visibleRows[i];
	if ((rowPtr->flags & HIDDEN) == 0) {
	    DisplayRowTitle(viewPtr, rowPtr, drawable);
	}
    }
}


static void
DrawOuterBorders(TableView *viewPtr, Drawable drawable)
{
    /* Draw 3D border just inside of the focus highlight ring. */
    if (viewPtr->borderWidth > 0) {
	Blt_Bg_DrawRectangle(viewPtr->tkwin, drawable, 
		viewPtr->rowNormalTitleBg, viewPtr->highlightWidth, 
		viewPtr->highlightWidth, 
		Tk_Width(viewPtr->tkwin) - 2 * viewPtr->highlightWidth,
		Tk_Height(viewPtr->tkwin) - 2 * viewPtr->highlightWidth,
		viewPtr->borderWidth, viewPtr->relief);
    }
    /* Draw focus highlight ring. */
    if (viewPtr->highlightWidth > 0) {
	XColor *color;
	GC gc;

	color = (viewPtr->flags & FOCUS)
	    ? viewPtr->highlightColor : viewPtr->highlightBgColor;
	gc = Tk_GCForColor(color, drawable);
	Tk_DrawFocusHighlight(viewPtr->tkwin, gc, viewPtr->highlightWidth,
	    drawable);
    }
}

static void
AdjustColumns(TableView *viewPtr)
{
    Column *lastPtr;
    long i, x;
    double weight;
    int growth;
    long numOpen;

    growth = VPORTWIDTH(viewPtr) - viewPtr->worldWidth;
    assert(growth > 0);
    lastPtr = NULL;

    numOpen = 0;
    weight = 0.0;
    /* Find out how many columns still have space available */
    for (i = 0; i < viewPtr->numColumns; i++) { 
	Column *colPtr;

	colPtr = viewPtr->columns[i];
	if (colPtr->flags & HIDDEN) {
	    continue;
	}
	lastPtr = colPtr;
	if ((colPtr->weight == 0.0) || (colPtr->width >= colPtr->max) || 
	    (colPtr->reqWidth > 0)) {
	    continue;
	}
	numOpen++;
	weight += colPtr->weight;
    }
    while ((numOpen > 0) && (weight > 0.0) && (growth > 0)) {
	int ration;

	ration = (int)(growth / weight);
	if (ration == 0) {
	    ration = 1;
	}
	for (i = 0; i < viewPtr->numColumns; i++) {
	    Column *colPtr;
	    int size, avail;

	    if ((numOpen <= 0) || (growth <= 0) || (weight <= 0.0)) {
		break;
	    }
	    colPtr = viewPtr->columns[i];
	    if (colPtr->flags & HIDDEN) {
		continue;
	    }
	    lastPtr = colPtr;
	    if ((colPtr->weight == 0.0) || (colPtr->width >= colPtr->max) || 
		(colPtr->reqWidth > 0)) {
		continue;
	    }
	    size = (int)(ration * colPtr->weight);
	    if (size > growth) {
		size = growth; 
	    }
	    avail = colPtr->max - colPtr->width;
	    if (size > avail) {
		size = avail;
		numOpen--;
		weight -= colPtr->weight;
	    }
	    colPtr->width += size;
	    growth -= size;
	}
    }
    if ((growth > 0) && (lastPtr != NULL)) {
	lastPtr->width += growth;
    }
    x = 0;
    for (i = 0; i < viewPtr->numColumns; i++) {
	Column *colPtr;

	colPtr = viewPtr->columns[i];
#ifdef notdef
	colPtr->width |= 0x1;		/* Make the width of the column
					 * odd. This means that the dotted
					 * focus rectangle will have dots on
					 * the corners.  */
#endif
	if (colPtr->flags & HIDDEN) {
	    continue;			/* Ignore hidden columns. */
	}
	colPtr->worldX = x;
	x += colPtr->width;
#ifdef notdef
	fprintf(stderr, "Adjust col %s w=%d\n", colPtr->title, colPtr->width);
#endif
    }
}

#ifdef notdef
static void
AdjustRows(TableView *viewPtr)
{
    Row *lastPtr;
    double weight;
    int growth;
    long numOpen;
    long i, y;

    growth = VPORTHEIGHT(viewPtr) - viewPtr->worldHeight;
    assert(growth > 0);
    lastPtr = NULL;
    numOpen = 0;
    weight = 0.0;
    /* Find out how many columns still have space available */
    for (i = 0; i < viewPtr->numRows; i++) {
	Row *rowPtr;

	rowPtr = viewPtr->rows[i];
	if (rowPtr->flags & HIDDEN) {
	    continue;
	}
	lastPtr = rowPtr;
	if ((rowPtr->weight == 0.0) || (rowPtr->height >= rowPtr->max) || 
	    (rowPtr->reqHeight > 0)) {
	    continue;
	}
	numOpen++;
	weight += rowPtr->weight;
    }

    while ((numOpen > 0) && (weight > 0.0) && (growth > 0)) {
	int ration;
	long i;

	ration = (int)(growth / weight);
	if (ration == 0) {
	    ration = 1;
	}
	for (i = 0; i < viewPtr->numRows; i++) { 
	    Row *rowPtr;
	    int size, avail;

	    rowPtr = viewPtr->rows[i];
	    if (rowPtr->flags & HIDDEN) {
		continue;
	    }
	    lastPtr = rowPtr;
	    if ((rowPtr->weight == 0.0) || (rowPtr->height >= rowPtr->max) || 
		(rowPtr->reqHeight > 0)) {
		continue;
	    }
	    size = (int)(ration * rowPtr->weight);
	    if (size > growth) {
		size = growth; 
	    }
	    avail = rowPtr->max - rowPtr->height;
	    if (size > avail) {
		size = avail;
		numOpen--;
		weight -= rowPtr->height;
	    }
	    rowPtr->height += size;
	    growth -= size;
	    if ((numOpen <= 0) || (growth <= 0) || (weight <= 0.0)) {
		break;
	    }
	}
    }
    if ((growth > 0) && (lastPtr != NULL)) {
	lastPtr->height += growth;
    }
    y = 0;
    for (i = 0; i < viewPtr->numRows; i++) {
	Row *rowPtr;

	rowPtr = viewPtr->rows[i];
#ifdef notdef
	rowPtr->height |= 0x1;		/* Make the width of the column
					 * odd. This means that the dotted
					 * focus rectangle will have dots on
					 * the corners.  */
#endif
	if (rowPtr->flags & HIDDEN) {
	    continue;			/* Ignore hidden columns. */
	}
	rowPtr->worldY = y;
	y += rowPtr->height;
    }
}
#endif




static Cell *
NewCell(TableView *viewPtr, Blt_HashEntry *hashPtr)
{
    Cell *cellPtr;

    cellPtr = Blt_Pool_AllocItem(viewPtr->cellPool, sizeof(Cell));
    cellPtr->hashPtr = hashPtr;
    cellPtr->viewPtr = viewPtr;
    cellPtr->flags = GEOMETRY;
    cellPtr->text = NULL;
    cellPtr->tkImage = NULL;
    cellPtr->stylePtr = NULL;
    cellPtr->width = cellPtr->height = 0;
    return cellPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * TableView operations
 *
 *---------------------------------------------------------------------------
 */

/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 *	Makes the cell appear active.
 *
 *	.t activate cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Cell *cellPtr, *activePtr;

    if (viewPtr->table == NULL) {
	Tcl_AppendResult(interp, "no data table to view.", (char *)NULL);
	return TCL_ERROR;
    }
    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    activePtr = viewPtr->activePtr;
    viewPtr->activePtr = cellPtr;
    /* If we aren't already queued to redraw the widget, try to directly draw
     * into window. */
    if ((viewPtr->flags & REDRAW_PENDING) == 0) {
	Drawable drawable;

	drawable = Tk_WindowId(viewPtr->tkwin);
	if (activePtr != NULL) {
	    DisplayCell(activePtr, drawable, TRUE);
	}
	DisplayCell(cellPtr, drawable, TRUE);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BboxOp --
 *
 *	.t bbox cell ?cell?...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BboxOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int i;
    int x1, y1, x2, y2;
    Tcl_Obj *listObjPtr;

    if (viewPtr->table == NULL) {
	Tcl_AppendResult(interp, "no data table to view.", (char *)NULL);
	return TCL_ERROR;
    }
    if (viewPtr->flags & (LAYOUT_PENDING|GEOMETRY)) {
	/*
	 * The layout is dirty.  Recompute it now, before we use the world
	 * dimensions.  But remember that the "bbox" operation isn't valid for
	 * hidden entries (since they're not visible, they don't have world
	 * coordinates).
	 */
	ComputeGeometry(viewPtr);
    }

    x1 = viewPtr->worldWidth;
    y1 = viewPtr->worldHeight;
    x2 = y2 = 0;
    for (i = 2; i < objc; i++) {
	Cell *cellPtr;
	CellKey *keyPtr;
	Column *colPtr;
	Row *rowPtr;

	if (GetCellFromObj(interp, viewPtr, objv[i], &cellPtr)  != TCL_OK) {
	    return TCL_ERROR;
	}
	if (cellPtr == NULL) {
	    continue;
	}
	keyPtr = GetKey(cellPtr);
	rowPtr = keyPtr->rowPtr;
	colPtr = keyPtr->colPtr;
	if (x1 > colPtr->worldX) {
	    x1 = colPtr->worldX;
	}
	if ((colPtr->worldX + colPtr->width) > x2) {
	    x2 = colPtr->worldX + colPtr->width;
	}
	if (y1 > rowPtr->worldY) {
	    y1 = rowPtr->worldY;
	}
	if ((rowPtr->worldY + rowPtr->height) > y2) {
	    y2 = rowPtr->worldY + rowPtr->height;
	}
    }
    {
	int w, h;

	w = VPORTWIDTH(viewPtr);
	h = VPORTHEIGHT(viewPtr);
	/*
	 * Do a min-max text for the intersection of the viewport and the
	 * computed bounding box.  If there is no intersection, return the
	 * empty string.
	 */
	if ((x2 < viewPtr->xOffset) || (y2 < viewPtr->yOffset) ||
	    (x1 >= (viewPtr->xOffset + w)) || (y1 >= (viewPtr->yOffset + h))) {
	    return TCL_OK;
	}
	x1 = SCREENX(viewPtr, x1);
	y1 = SCREENY(viewPtr, y1);
	x2 = SCREENX(viewPtr, x2);
	y2 = SCREENY(viewPtr, y2);
    }

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x1));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y1));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x2));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y2));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BindOp --
 *
 *	.t bind tag sequence command
 *	.t bindtags idOrTag idOrTag...
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BindOp(TableView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ClientData object;
    Cell *cellPtr;

    /*
     * Cells are selected by id only.  All other strings are interpreted as
     * a binding tag.
     */
    if ((GetCellFromObj(NULL, viewPtr, objv[2], &cellPtr) != TCL_OK) ||
	(cellPtr == NULL)) {
	/* Assume that this is a binding tag. */
	object = CellTag(viewPtr, Tcl_GetString(objv[2]));
    } else {
	object = cellPtr;
    } 
    return Blt_ConfigureBindingsFromObj(interp, viewPtr->bindTable, object, 
	 objc - 3, objv + 3);
}


/*
 *---------------------------------------------------------------------------
 *
 * CellActivateOp --
 *
 * 	Turns on highlighting for a particular cell.  Only one cell
 *      can be active at a time.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 *      .view cell activate ?cell?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Cell *cellPtr, *activePtr;

    if (viewPtr->table == NULL) {
	Tcl_AppendResult(interp, "no data table to view.", (char *)NULL);
	return TCL_ERROR;
    }
    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    activePtr = viewPtr->activePtr;
    viewPtr->activePtr = cellPtr;
    /* If we aren't already queued to redraw the widget, try to directly draw
     * into window. */
    if ((viewPtr->flags & REDRAW_PENDING) == 0) {
	Drawable drawable;

	drawable = Tk_WindowId(viewPtr->tkwin);
	if (activePtr != NULL) {
	    DisplayCell(activePtr, drawable, TRUE);
	}
	DisplayCell(cellPtr, drawable, TRUE);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * CellBboxOp --
 *
 *      .view cell bbox $cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellBboxOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int i;
    int x1, y1, x2, y2;
    Tcl_Obj *listObjPtr;

    if (viewPtr->table == NULL) {
	Tcl_AppendResult(interp, "no data table to view.", (char *)NULL);
	return TCL_ERROR;
    }
    if (viewPtr->flags & (LAYOUT_PENDING|GEOMETRY)) {
	/*
	 * The layout is dirty.  Recompute it now, before we use the world
	 * dimensions.  But remember that the "bbox" operation isn't valid for
	 * hidden entries (since they're not visible, they don't have world
	 * coordinates).
	 */
	ComputeGeometry(viewPtr);
    }

    x1 = viewPtr->worldWidth;
    y1 = viewPtr->worldHeight;
    x2 = y2 = 0;
    for (i = 3; i < objc; i++) {
	Cell *cellPtr;
	CellKey *keyPtr;
	Column *colPtr;
	Row *rowPtr;

	if (GetCellFromObj(interp, viewPtr, objv[i], &cellPtr)  != TCL_OK) {
	    return TCL_ERROR;
	}
	if (cellPtr == NULL) {
	    continue;
	}
	keyPtr = GetKey(cellPtr);
	rowPtr = keyPtr->rowPtr;
	colPtr = keyPtr->colPtr;
	if (x1 > colPtr->worldX) {
	    x1 = colPtr->worldX;
	}
	if ((colPtr->worldX + colPtr->width) > x2) {
	    x2 = colPtr->worldX + colPtr->width;
	}
	if (y1 > rowPtr->worldY) {
	    y1 = rowPtr->worldY;
	}
	if ((rowPtr->worldY + rowPtr->height) > y2) {
	    y2 = rowPtr->worldY + rowPtr->height;
	}
    }
    {
	int w, h;

	w = VPORTWIDTH(viewPtr);
	h = VPORTHEIGHT(viewPtr);
	/*
	 * Do a min-max text for the intersection of the viewport and the
	 * computed bounding box.  If there is no intersection, return the
	 * empty string.
	 */
	if ((x2 < viewPtr->xOffset) || (y2 < viewPtr->yOffset) ||
	    (x1 >= (viewPtr->xOffset + w)) || (y1 >= (viewPtr->yOffset + h))) {
	    return TCL_OK;
	}
	x1 = SCREENX(viewPtr, x1);
	y1 = SCREENY(viewPtr, y1);
	x2 = SCREENX(viewPtr, x2);
	y2 = SCREENY(viewPtr, y2);
    }

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x1));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y1));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x2));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y2));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellCgetOp --
 *
 *      .view cell cget $cell option 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Cell *cellPtr;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, cellSpecs,
	(char *)cellPtr, objv[4], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * CellConfigureOp --
 *
 * 	This procedure is called to process an objv/objc list, plus the Tk
 * 	option database, in order to configure (or reconfigure) the widget.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for viewPtr; old resources get freed, if there were
 *	any.  The widget is redisplayed.
 *
 *      .view cell configure $cell ?option value?
 *---------------------------------------------------------------------------
 */
static int
CellConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Cell *cellPtr;
    CellStyle *oldStylePtr;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, 
		cellSpecs, (char *)cellPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 5) {
	return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, 
		cellSpecs, (char *)cellPtr, objv[4], 0);
    } 
    iconOption.clientData = viewPtr;
    tableOption.clientData = viewPtr;
    oldStylePtr = cellPtr->stylePtr;
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, cellSpecs, 
	objc - 4, objv + 4, (char *)cellPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((Blt_ConfigModified(cellSpecs, "-style", (char *)NULL)) &&
        (oldStylePtr != cellPtr->stylePtr)) {
        CellKey *keyPtr;

	keyPtr = GetKey(cellPtr);
        if (cellPtr->stylePtr != NULL) {
            int isNew;

            cellPtr->stylePtr->refCount++;	
	    Blt_CreateHashEntry(&cellPtr->stylePtr->table, (char *)keyPtr, 
                &isNew);
        }
        if (oldStylePtr != NULL) {
            Blt_HashEntry *hPtr;

            /* Remove the cell from old style's table of cells. */
            oldStylePtr->refCount--;
            hPtr = Blt_FindHashEntry(&oldStylePtr->table, (char *)keyPtr);
            if (hPtr != NULL) {
                Blt_DeleteHashEntry(&oldStylePtr->table, hPtr);
            }
            if (oldStylePtr->refCount <= 0) {
                (*oldStylePtr->classPtr->freeProc)(oldStylePtr);
            }
        }
        cellPtr->flags |= GEOMETRY;	/* Assume that the new style
					 * changes the geometry of the
					 * cell. */
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellDeactivateOp --
 *
 * 	Deactivates all cells.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *	  .view deactivate 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Cell *activePtr;

    activePtr = viewPtr->activePtr;
    viewPtr->activePtr = NULL;
    /* If we aren't already queued to redraw the widget, try to directly
     * draw into window. */
    if ((viewPtr->flags & REDRAW_PENDING) == 0) {
	if (activePtr != NULL) {
	    Drawable drawable;

	    drawable = Tk_WindowId(viewPtr->tkwin);
	    DisplayCell(activePtr, drawable, TRUE);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellFocusOp --
 *
 * 	Gets or sets focus on a cell.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *	  .view cell focus ?cell?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellFocusOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Cell *cellPtr;

    if (objc == 3) {
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	if (viewPtr->focusPtr != NULL) {
	    CellKey *keyPtr;
	    Column *colPtr;
	    Row *rowPtr;
	    Tcl_Obj *objPtr;

	    keyPtr = GetKey(viewPtr->focusPtr);
	    rowPtr = keyPtr->rowPtr;
	    colPtr = keyPtr->colPtr;
	    objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr != NULL) {
	CellKey *keyPtr;
	Row *rowPtr;
	Column *colPtr;

	keyPtr = GetKey(cellPtr);
	rowPtr = keyPtr->rowPtr;
	colPtr = keyPtr->colPtr;
	if ((rowPtr->flags|colPtr->flags) & (HIDDEN|DISABLED)) {
	    return TCL_OK;		/* Can't set focus to hidden or
					 * disabled cell */
	}
	if (cellPtr != viewPtr->focusPtr) {
	    viewPtr->focusPtr = cellPtr;
	    EventuallyRedraw(viewPtr);
	}
	Blt_SetFocusItem(viewPtr->bindTable, viewPtr->focusPtr, ITEM_CELL);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellIdentifyOp --
 *
 *	.t cell identify cell x y 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellIdentifyOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellKey *keyPtr;
    CellStyle *stylePtr;
    Column *colPtr;
    Row *rowPtr;
    TableView *viewPtr = clientData;
    const char *string;
    int x, y, rootX, rootY;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    rowPtr = keyPtr->rowPtr;
    /* Convert from root coordinates to window-local coordinates to cell-local
     * coordinates */
    Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
    x -= rootX + SCREENX(viewPtr, colPtr->worldX);
    y -= rootY + SCREENY(viewPtr, rowPtr->worldY);
    string = NULL;
    stylePtr = GetCurrentStyle(viewPtr, rowPtr, colPtr, cellPtr);
    if (stylePtr->classPtr->identProc != NULL) {
	string = (*stylePtr->classPtr->identProc)(cellPtr, stylePtr, x, y);
    }
    if (string != NULL) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), string, -1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellIndexOp --
 *
 *	Converts the string representing a cell index into their respective
 *      "node field" identifiers.
 *
 * Results: 
 *      A standard TCL result.  Interp->result will contain the identifier
 *	of each inode found. If an inode could not be found, then the
 *	serial identifier will be the empty string.
 *
 *      .view cell index $cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellIndexOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellKey *keyPtr;
    Column *colPtr;
    Row *rowPtr;
    TableView *viewPtr = clientData;
    Tcl_Obj *listObjPtr, *objPtr;

    if ((GetCellFromObj(NULL, viewPtr, objv[3], &cellPtr) != TCL_OK) ||
	(cellPtr == NULL)) {
	return TCL_OK;
    }
    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    rowPtr = keyPtr->rowPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellInvokeOp --
 *
 *      .view cell invoke $cell
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellInvokeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Row *rowPtr;
    Column *colPtr;
    Cell *cellPtr;
    CellStyle *stylePtr;
    CellKey *keyPtr;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    rowPtr = keyPtr->rowPtr;
    stylePtr = GetCurrentStyle(viewPtr, rowPtr, colPtr, cellPtr);
    if (stylePtr->cmdObjPtr != NULL) {
	int result;
	Tcl_Obj *cmdObjPtr, *objPtr;

	cmdObjPtr = Tcl_DuplicateObj(stylePtr->cmdObjPtr);
	objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
	Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
	Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	Tcl_IncrRefCount(cmdObjPtr);
	Tcl_Preserve(cellPtr);
	result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
	Tcl_Release(cellPtr);
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
 * CellSeeOp --
 *
 *      .view cell see $cell
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellSeeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellKey *keyPtr;
    Column *colPtr;
    Row *rowPtr;
    TableView *viewPtr = clientData;
    long x, y;
    int viewWidth, viewHeight;

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    rowPtr = keyPtr->rowPtr;
    viewWidth = VPORTWIDTH(viewPtr);
    viewHeight = VPORTHEIGHT(viewPtr);
    y = viewPtr->yOffset;
    x = viewPtr->xOffset;
    if (rowPtr->worldY < y) {
	y = rowPtr->worldY;
    } else if ((rowPtr->worldY + rowPtr->height) > (y + viewHeight)) {
	y = rowPtr->worldY + rowPtr->height - viewHeight;
    }
    if (colPtr->worldX < x) {
	x = colPtr->worldX;
    } else if ((colPtr->worldX + colPtr->width) > (x + viewWidth)) {
	x = colPtr->worldX + colPtr->width - viewWidth;
    }
    if (x < 0) {
	x = 0;
    }
    if (y < 0) {
	y = 0;
    }
    if (x != viewPtr->xOffset) {
	viewPtr->xOffset = x;
	viewPtr->flags |= SCROLLX;
    }
    if (y != viewPtr->yOffset) {
	viewPtr->yOffset = y;
	viewPtr->flags |= SCROLLY;
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellStyleOp --
 *
 *      .view cell style $cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellStyleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    CellStyle *stylePtr;
    Cell *cellPtr;
    CellKey *keyPtr;

    cellPtr = NULL;			/* Suppress compiler warning. */
    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    keyPtr = GetKey(cellPtr);
    stylePtr = GetCurrentStyle(viewPtr, keyPtr->rowPtr, keyPtr->colPtr, 
                cellPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), stylePtr->name, -1);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * CellWritableOp --
 *
 *	  .view cell writable $cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellWritableOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    TableView *viewPtr = clientData;
    int state;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    state = FALSE;
    if (cellPtr != NULL) {
	CellKey *keyPtr;
	CellStyle *stylePtr;
	Column *colPtr;
	Row *rowPtr;

	keyPtr = GetKey(cellPtr);
	colPtr = keyPtr->colPtr;
	rowPtr = keyPtr->rowPtr;
	stylePtr = GetCurrentStyle(viewPtr, rowPtr, colPtr, cellPtr);
	
	state = (stylePtr->flags & EDIT);
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellOp --
 *
 *	This procedure handles cell operations.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec cellOps[] =
{
    {"activate",   1, CellActivateOp,    3, 4, "?cell?",},
    {"bbox",       1, CellBboxOp,        4, 4, "cell",},
    {"cget",       2, CellCgetOp,        5, 5, "cell option",},
    {"configure",  2, CellConfigureOp,   4, 0, "cell ?option value?...",},
    {"deactivate", 1, CellDeactivateOp,  3, 3, "",},
    {"focus",      2, CellFocusOp,       4, 0, "?cell?",},
    {"identify",   2, CellIdentifyOp,    6, 6, "cell x y",},
    {"index",      3, CellIndexOp,       4, 4, "cell",},
    {"invoke",     3, CellInvokeOp,      4, 4, "cell",},
    {"see",        3, CellSeeOp,         4, 4, "cell",},
    {"style",      3, CellStyleOp,       4, 4, "cell",},
    {"writable",   3, CellWritableOp,    4, 4, "cell",},
};
static int numCellOps = sizeof(cellOps) / sizeof(Blt_OpSpec);

static int
CellOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numCellOps, cellOps, BLT_OP_ARG2, objc, 
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
 * CgetOp --
 *
 *	.t cget option
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    iconOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    tableOption.clientData = viewPtr;
    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, tableSpecs,
	(char *)viewPtr, objv[2], 0);
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
 *	set for viewPtr; old resources get freed, if there were any.  The widget
 *	is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    iconOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    tableOption.clientData = viewPtr;
    if (objc == 2) {
	return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, tableSpecs, 
		(char *)viewPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, tableSpecs,
		(char *)viewPtr, objv[2], 0);
    } 
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, tableSpecs, 
	objc - 2, objv + 2, (char *)viewPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    if (ConfigureTableView(interp, viewPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CurselectionOp --
 *
 *	.t curselection
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CurselectionOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (viewPtr->selectMode == SELECT_CELLS) {
	Tcl_Obj *objPtr;
	BLT_TABLE_ROW row;
	BLT_TABLE_COLUMN col;

	row = viewPtr->selectCells.anchorPtr->rowPtr->row;
	objPtr = Tcl_NewLongObj(blt_table_row_index(row));
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	col = viewPtr->selectCells.anchorPtr->colPtr->column;
	objPtr = Tcl_NewLongObj(blt_table_column_index(col));
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	row = viewPtr->selectCells.markPtr->rowPtr->row;
	objPtr = Tcl_NewLongObj(blt_table_row_index(row));
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	col = viewPtr->selectCells.markPtr->colPtr->column;
	objPtr = Tcl_NewLongObj(blt_table_column_index(col));
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    } else {
	if (viewPtr->flags & SELECT_SORTED) {
	    Blt_ChainLink link;
	    
	    for (link = Blt_Chain_FirstLink(viewPtr->selectRows.list); 
		 link != NULL; link = Blt_Chain_NextLink(link)) {
 		Row *rowPtr;
		Tcl_Obj *objPtr;
		
		rowPtr = Blt_Chain_GetValue(link);
		objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	} else {
	    long i;
	    
	    for (i = 0; i < viewPtr->numRows; i++) {
		Row *rowPtr;
		
		rowPtr = viewPtr->rows[i];
		if (rowPtr->flags & SELECTED) {
		    Tcl_Obj *objPtr;
		    
		    objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
		    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
		}
	    }
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnActivateOp --
 *
 *	Selects the button to appear active.
 *
 *	.t column activate ?col?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Column *colPtr, *activePtr;
    
    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (colPtr == NULL) {
fprintf(stderr, "ColumnActivate: Column %s is NULL\n", Tcl_GetString(objv[3])); 
	return TCL_OK;
    }
    if (((viewPtr->flags & COLUMN_TITLES) == 0) || 
	(colPtr->flags & (HIDDEN | DISABLED))) {
	return TCL_OK;			/* Disabled or hidden row. */
    }
    activePtr = viewPtr->colActiveTitlePtr;
    viewPtr->colActiveTitlePtr = colPtr;

    /* If we aren't already queued to redraw the widget, try to directly draw
     * into window. */
    if ((viewPtr->flags & REDRAW_PENDING) == 0) {
	Drawable drawable;

	drawable = Tk_WindowId(viewPtr->tkwin);
	if (activePtr != NULL) {
	    DisplayColumnTitle(viewPtr, activePtr, drawable);
	}
	DisplayColumnTitle(viewPtr, colPtr, drawable);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnBindOp --
 *
 *	Bind a callback to an event on a column title.
 *
 *	  .t column bind tag sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnBindOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    ClientData object;
    Column *colPtr;

    if (GetColumn(NULL, viewPtr, objv[3], &colPtr) == TCL_OK) {
	object = colPtr;
    } else {
	object = ColumnTag(viewPtr, Tcl_GetString(objv[3]));
    }
    return Blt_ConfigureBindingsFromObj(interp, viewPtr->bindTable, object,
	objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnCgetOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnCgetOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    Column *colPtr;

    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK){
	return TCL_ERROR;
    }
    if (colPtr == NULL) {
	fprintf(stderr, "ColumnCget: Column %s is NULL\n", Tcl_GetString(objv[3])); 
	return TCL_OK;
    }
    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, columnSpecs, 
	(char *)colPtr, objv[4], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnConfigureOp --
 *
 * 	This procedure is called to process a list of configuration
 *	options database, in order to reconfigure the one of more
 *	entries in the widget.
 *
 *	  .tv column configure $col option value
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for viewPtr; old resources get freed, if there
 *	were any.  
 *
 *	.tv column configure col ?option value?
 *---------------------------------------------------------------------------
 */
static int
ColumnConfigureOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv)
{
    Blt_Chain columns;
    Blt_ChainLink link;

    uidOption.clientData = viewPtr;
    iconOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;

    if ((objc == 4) || (objc == 5)) {
	Column *colPtr;

	/* Must refer to a single if reporting the configuration options. */
	if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (colPtr == NULL) {
	    fprintf(stderr, "ColumnConfigure: Column %s is NULL\n", 
		    Tcl_GetString(objv[3])); 
	    return TCL_OK;
	}
	if (objc == 4) {
	    return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, columnSpecs,
		(char *)colPtr, (Tcl_Obj *)NULL, 0);
	} else if (objc == 5) {
	    return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, columnSpecs,
		(char *)colPtr, objv[4], 0);
	}
    }
    if (GetColumns(interp, viewPtr, objv[3], &columns) != TCL_OK) {
	return TCL_ERROR;
    }
    for (link = Blt_Chain_FirstLink(columns); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	Column *colPtr;

	colPtr = Blt_Chain_GetValue(link);
	if (colPtr == NULL) {
	    fprintf(stderr, "ColumnConfigure: Column %s is NULL\n", 
		    Tcl_GetString(objv[3])); 
	    Blt_Chain_Destroy(columns);
	    return TCL_OK;
	}
	if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, columnSpecs, 
		objc - 4, objv + 4, (char *)colPtr, BLT_CONFIG_OBJV_ONLY) 
		!= TCL_OK) {
	    Blt_Chain_Destroy(columns);
	    return TCL_ERROR;
	}
	/*FIXME: Makes every change redo everything. */
	if (Blt_ConfigModified(columnSpecs, "-formatcommand", "-style", "-icon",
			       (char *)NULL)) {
	    colPtr->flags |= GEOMETRY;
	    viewPtr->flags |= GEOMETRY | REDRAW;
	}
	ConfigureColumn(viewPtr, colPtr);
    }
    Blt_Chain_Destroy(columns);
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnDeactivateOp --
 *
 *	Selects the column to appear normally.
 *
 *	.t column deactivate
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		   Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Column *activePtr;
    
    if ((viewPtr->flags & COLUMN_TITLES) == 0) {
	return TCL_OK;			/* Disabled or hidden row. */
    }
    activePtr = viewPtr->colActiveTitlePtr;
    viewPtr->colActiveTitlePtr = NULL;
    /* If we aren't already queued to redraw the widget, try to directly draw
     * into window. */
    if ((viewPtr->flags & REDRAW_PENDING) == 0) {
	if (activePtr != NULL) {
	    Drawable drawable;

	    drawable = Tk_WindowId(viewPtr->tkwin);
	    DisplayColumnTitle(viewPtr, activePtr, drawable);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnDeleteOp --
 *
 *	.t column delete col col col 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
	       Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    long i, count;
    Blt_Chain columns;
    Blt_ChainLink link;

    /* Mark all the named columns as deleted. */
    columns = IterateColumnsObjv(interp, viewPtr, objc - 3, objv + 3);
    if (columns == NULL) {
	return TCL_ERROR;
    }
    for (link = Blt_Chain_FirstLink(columns); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
	Column *colPtr;

	colPtr = Blt_Chain_GetValue(link);
	colPtr->flags |= DELETED;
    }
    Blt_Chain_Destroy(columns);

    /* Now compress the columns array while freeing the marked columns. */
    count = 0;
    for (i = 0; i < viewPtr->numColumns; i++) {
	Column *colPtr;
	
	colPtr = viewPtr->columns[i];
	if (colPtr->flags & DELETED) {
	    DestroyColumn(colPtr);
	    continue;
	}
	viewPtr->columns[count] = colPtr;
	count++;
    }
    /* Requires a new layout. Sort order and individual geometries stay the
     * same. */
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnExistsOp --
 *
 *	.tv column exists col
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
	       Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int exists;
    Column *colPtr;

    exists = FALSE;
    if (GetColumn(NULL, viewPtr, objv[3], &colPtr) == TCL_OK) {
	exists = (colPtr != NULL);
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), exists);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnExposeOp --
 *
 *	.tv column expose ?col...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnExposeOp(ClientData clientData, Tcl_Interp *interp, int objc,
	       Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    if (objc == 3) {
	long i;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	for (i = 0; i < viewPtr->numColumns; i++) {
	    Column *colPtr;

	    colPtr = viewPtr->columns[i];
	    if ((colPtr->flags & HIDDEN) == 0) {
		Tcl_Obj *objPtr;

		objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	}
	Tcl_SetObjResult(interp, listObjPtr);
    } else {
	int redraw;
	Blt_Chain columns;
	Blt_ChainLink link;
	
	columns = IterateColumnsObjv(interp, viewPtr, objc - 3, objv + 3);
	if (columns == NULL) {
	    return TCL_ERROR;
	}
	redraw = FALSE;
	for (link = Blt_Chain_FirstLink(columns); link != NULL; 
	     link = Blt_Chain_NextLink(link)) {
	    Column *colPtr;
	    
	    colPtr = Blt_Chain_GetValue(link);
	    if (colPtr->flags & HIDDEN) {
		colPtr->flags |= HIDDEN;
		redraw = TRUE;
	    }
	}
	Blt_Chain_Destroy(columns);
	if (redraw) {
	    viewPtr->flags |= SCROLL_PENDING;
	    EventuallyRedraw(viewPtr);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnHideOp --
 *
 *	.tv column hide ?row...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnHideOp(ClientData clientData, Tcl_Interp *interp, int objc,
	     Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    if (objc == 3) {
	long i;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	for (i = 0; i < viewPtr->numColumns; i++) {
	    Column *colPtr;

	    colPtr = viewPtr->columns[i];
	    if (colPtr->flags & HIDDEN) {
		Tcl_Obj *objPtr;

		objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	}
	Tcl_SetObjResult(interp, listObjPtr);
    } else {
	int redraw;
	Blt_Chain columns;
	Blt_ChainLink link;
	
	columns = IterateColumnsObjv(interp, viewPtr, objc - 3, objv + 3);
	if (columns == NULL) {
	    return TCL_ERROR;
	}
	redraw = FALSE;
	for (link = Blt_Chain_FirstLink(columns); link != NULL; 
	     link = Blt_Chain_NextLink(link)) {
	    Column *colPtr;
	    
	    colPtr = Blt_Chain_GetValue(link);
	    if ((colPtr->flags & HIDDEN) == 0) {
		colPtr->flags |= HIDDEN;
		redraw = TRUE;
	    }
	}
	Blt_Chain_Destroy(columns);
	if (redraw) {
	    viewPtr->flags |= SCROLL_PENDING;
	    EventuallyRedraw(viewPtr);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnIndexOp --
 *
 *	.t colun index col
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnIndexOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Column *colPtr;
    long index;

    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    index = (colPtr != NULL) ? blt_table_column_index(colPtr->column) : -1;
    Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ColumnInsertOp --
 *
 *	Add new columns to the table.
 *
 *	.tv column insert position name ?option values?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnInsertOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    BLT_TABLE_COLUMN col;
    Blt_HashEntry *hPtr;
    CellKey key;
    Column **columns;
    Column *colPtr;
    TableView *viewPtr = clientData;
    int isNew;
    long i, insertPos;

    col = blt_table_get_column(interp, viewPtr->table, objv[3]);
    if (col == NULL) {
	return TCL_ERROR;
    }
    /* 
     * Column doesn't have to exist.  We'll add it when the table adds columns.
     * What to put in the table as a place holder.  
     */
    hPtr = Blt_CreateHashEntry(&viewPtr->columnTable, (char *)col, &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "a column \"", Tcl_GetString(objv[3]),
		"\" already exists in \"", Tk_PathName(viewPtr->tkwin),
		"\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (Blt_GetPositionFromObj(viewPtr->interp, objv[4], &insertPos) != TCL_OK){
	return TCL_ERROR;
    }
    if ((insertPos == -1) || (insertPos >= viewPtr->numRows)) {
	insertPos = viewPtr->numColumns; /* Insert at end of list. */
    }
    colPtr = NewColumn(viewPtr, col, hPtr);
    colPtr->flags |= STICKY;		/* Don't allow column to be reset. */
    iconOption.clientData = viewPtr;
    uidOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(viewPtr->interp, viewPtr->tkwin, 
	colPtr->title, "Column", columnSpecs, objc - 4, objv + 4, 
	(char *)colPtr, 0) != TCL_OK) {	
	DestroyColumn(colPtr);
	return TCL_ERROR;
    }
    columns = Blt_AssertCalloc(viewPtr->numColumns + 1, sizeof(Column *));
    if (insertPos > 0) {
	memcpy(columns, viewPtr->columns, insertPos * sizeof(Column *));
    }
    columns[insertPos] = colPtr;
    if (insertPos < viewPtr->numColumns) {
	memcpy(columns + insertPos + 1, viewPtr->columns + insertPos, 
	       (viewPtr->numColumns - insertPos) * sizeof(Column *));
    }	
    viewPtr->numColumns++;
    if (viewPtr->columns != NULL) {
        Blt_Free(viewPtr->columns);
    }
    viewPtr->columns = columns;
    key.colPtr = colPtr;
    for (i = 0; i < viewPtr->numRows; i++) {
	Blt_HashEntry *hPtr;
	int isNew;

	key.rowPtr = viewPtr->rows[i];
	hPtr = Blt_CreateHashEntry(&viewPtr->cellTable, (char *)&key, &isNew);
	if (isNew) {
	    Cell *cellPtr;

	    cellPtr = NewCell(viewPtr, hPtr);
	    Blt_SetHashValue(hPtr, cellPtr);
	}
    }
    viewPtr->flags |= GEOMETRY;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnInvokeOp --
 *
 * 	This procedure is called to invoke a column command.
 *
 *	  .h column invoke columnName
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnInvokeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Column *colPtr;
    TableView *viewPtr = clientData;
    Tcl_Obj *objPtr, *cmdObjPtr;
    int result;
    
    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    cmdObjPtr = (colPtr->cmdObjPtr == NULL) 
	? viewPtr->colCmdObjPtr : colPtr->cmdObjPtr;
    if (((viewPtr->flags & COLUMN_TITLES) == 0) || (colPtr == NULL)  ||
	(colPtr->flags & (DISABLED|HIDDEN)) || (cmdObjPtr == NULL)) {
	return TCL_OK;
    }
    Tcl_Preserve(viewPtr);
    /* command pathName colIndex  */
    cmdObjPtr = Tcl_DuplicateObj(cmdObjPtr);  
    objPtr = Tcl_NewStringObj(Tk_PathName(viewPtr->tkwin), -1);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    Tcl_IncrRefCount(cmdObjPtr);
    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(cmdObjPtr);
    Tcl_Release(viewPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnMoveOp --
 *
 *	Move a column.
 *
 * .h column move field1 position
 *---------------------------------------------------------------------------
 */

/*
 *---------------------------------------------------------------------------
 *
 * ColumnNamesOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Tcl_Obj *listObjPtr;
    long i;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (i = 0; i < viewPtr->numColumns; i++) {
	Column *colPtr;
	Tcl_Obj *objPtr;

	colPtr = viewPtr->columns[i];
	objPtr = Tcl_NewStringObj(blt_table_column_label(colPtr->column), -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
ColumnNearestOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int x;			   /* Screen coordinates of the test point. */
    Column *colPtr;
    long index;

#ifdef notdef
    int isRoot;

    isRoot = FALSE;
    string = Tcl_GetString(objv[3]);

    if (strcmp("-root", string) == 0) {
	isRoot = TRUE;
	objv++, objc--;
    }
    if (objc != 5) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " ", Tcl_GetString(objv[1]), 
		Tcl_GetString(objv[2]), " ?-root? x y\"", (char *)NULL);
	return TCL_ERROR;
			 
    }
#endif
    if (Tk_GetPixelsFromObj(interp, viewPtr->tkwin, objv[3], &x) != TCL_OK) {
	return TCL_ERROR;
    } 
    colPtr = NearestColumn(viewPtr, x, TRUE);
    index = (colPtr != NULL) ? blt_table_column_index(colPtr->column) : -1;
    Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}

static void
UpdateColumnMark(TableView *viewPtr, int newMark)
{
    Column *colPtr;
    int dx;
    int width;

    colPtr = viewPtr->colResizePtr;
    if (colPtr == NULL) {
	return;
    }
    dx = newMark - viewPtr->colResizeAnchor; 
    width = colPtr->width;
    if ((colPtr->reqMin > 0) && ((width + dx) < colPtr->reqMin)) {
	dx = colPtr->reqMin - width;
    }
    if ((colPtr->reqMax > 0) && ((width + dx) > colPtr->reqMax)) {
	dx = colPtr->reqMax - width;
    }
    if ((width + dx) < 4) {
	dx = 4 - width;
    }
    viewPtr->colResizeMark = viewPtr->colResizeAnchor + dx;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnResizeActivateOp --
 *
 *	Turns on/off the resize cursor.
 *
 *	.t column resize activate col 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		       Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Column *colPtr;

    if (GetColumn(interp, viewPtr, objv[4], &colPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((colPtr == NULL) || (colPtr->flags & (HIDDEN|DISABLED))){
	fprintf(stderr, "ColumnResizeActivate: Column %s is NULL\n", Tcl_GetString(objv[3])); 
	return TCL_OK;
    }
    if (viewPtr->colResizeCursor != None) {
	Tk_DefineCursor(viewPtr->tkwin, viewPtr->colResizeCursor);
    } 
    viewPtr->colResizePtr = colPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnResizeAnchorOp --
 *
 *	Set the anchor for the resize.
 *
 *	.t column resize anchor x
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeAnchorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int y;

    if (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
	return TCL_ERROR;
    } 
    viewPtr->colResizeAnchor = y;
    viewPtr->flags |= COLUMN_RESIZE | REDRAW;
    UpdateColumnMark(viewPtr, y);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnResizeDeactiveOp --
 *
 *	Turns off the resize cursor.
 *
 *	.t column resize deactivate 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
			 Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    Tk_UndefineCursor(viewPtr->tkwin);
    viewPtr->colResizePtr = NULL;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnResizeMarkOp --
 *
 *	Sets the resize mark.  The distance between the mark and the anchor
 *	is the delta to change the width of the active column.
 *
 *	.t column resize mark x
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeMarkOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		   Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int y;

    if (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
	return TCL_ERROR;
    } 
    viewPtr->flags |= COLUMN_RESIZE | REDRAW;
    UpdateColumnMark(viewPtr, y);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnResizeCurrentOp --
 *
 *	Returns the new width of the column including the resize delta.
 *
 *	.t column resize current
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeCurrentOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		      Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    viewPtr->flags &= ~COLUMN_RESIZE;
    UpdateColumnMark(viewPtr, viewPtr->colResizeMark);
    if (viewPtr->colResizePtr != NULL) {
	int width, delta;

	delta = (viewPtr->colResizeMark - viewPtr->colResizeAnchor);
	width = viewPtr->colResizePtr->width + delta;
	Tcl_SetIntObj(Tcl_GetObjResult(interp), width);
    }
    return TCL_OK;
}

static Blt_OpSpec columnResizeOps[] =
{ 
    {"activate",   2, ColumnResizeActivateOp,   5, 5, "column"},
    {"anchor",     2, ColumnResizeAnchorOp,     5, 5, "x"},
    {"current",    1, ColumnResizeCurrentOp,    4, 4, "",},
    {"deactivate", 1, ColumnResizeDeactivateOp, 4, 4, ""},
    {"mark",       1, ColumnResizeMarkOp,       5, 5, "x"},
};

static int numColumnResizeOps = sizeof(columnResizeOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * ColumnResizeOp --
 *
 *---------------------------------------------------------------------------
 */
static int
ColumnResizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numColumnResizeOps, columnResizeOps, 
	BLT_OP_ARG3, objc, objv,0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}


/*
 *---------------------------------------------------------------------------
 *
 * ColumnSeeOp --
 *
 *	.t column see col
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnSeeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    Column *colPtr;
    TableView *viewPtr = clientData;
    long x;

    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (colPtr == NULL) {
	fprintf(stderr, "ColumnSee: Column %s is NULL\n", 
		Tcl_GetString(objv[3])); 
	return TCL_OK;
    }
    x = viewPtr->xOffset;
    if (((colPtr->worldX + colPtr->width) < viewPtr->xOffset) ||
	(colPtr->worldX >= viewPtr->xOffset)) {
	x = colPtr->worldX - VPORTWIDTH(viewPtr) / 2;
    }
    if (x < 0) {
	x = 0;
    }
    if (x != viewPtr->xOffset) {
	viewPtr->xOffset = x;
	viewPtr->flags |= SCROLLX;
	EventuallyRedraw(viewPtr);
    }
    return TCL_OK;
}


static Blt_OpSpec columnOps[] = {
    {"activate",   1, ColumnActivateOp,   4, 4, "col",}, 
    {"bind",       1, ColumnBindOp,       4, 6, "tagName ?sequence command?",},
    {"cget",       2, ColumnCgetOp,       5, 5, "col option",},	
    {"configure",  2, ColumnConfigureOp,  4, 0, "coltag ?option value?...",}, 
    {"deactivate", 2, ColumnDeactivateOp, 3, 3, "",},
    {"delete",     2, ColumnDeleteOp,     3, 0, "coltag...",}, 
    {"exists",     3, ColumnExistsOp,     4, 4, "coltag",}, 
    {"expose",     3, ColumnExposeOp,     3, 0, "?coltag...?",},
    {"hide",       1, ColumnHideOp,       3, 0, "?coltag...?",},
    {"index",      3, ColumnIndexOp,      4, 4, "col",}, 
    {"insert",     3, ColumnInsertOp,     5, 0, "col pos ?option value?...",},  
    {"invoke",     3, ColumnInvokeOp,     4, 4, "col",},  
    {"names",      2, ColumnNamesOp,      3, 3, "",},
    {"nearest",    2, ColumnNearestOp,    4, 4, "y",},
    {"resize",     1, ColumnResizeOp,     3, 0, "args",},
    {"see",        2, ColumnSeeOp,        4, 4, "col",}, 
    {"show",       2, ColumnExposeOp,     3, 0, "?coltag...?",},
};
static int numColumnOps = sizeof(columnOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * ColumnOp --
 *
 *---------------------------------------------------------------------------
 */
static int
ColumnOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numColumnOps, columnOps, BLT_OP_ARG2, 
	objc, objv,0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * DeactivateOp --
 *
 *	Makes the formerly active cell appear normal.
 *
 *	.t deactivate 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Cell *activePtr;

    activePtr = viewPtr->activePtr;
    viewPtr->activePtr = NULL;
    /* If we aren't already queued to redraw the widget, try to directly draw
     * into window. */
    if ((viewPtr->flags & REDRAW_PENDING) == 0) {
	if (activePtr != NULL) {
	    Drawable drawable;

	    drawable = Tk_WindowId(viewPtr->tkwin);
	    DisplayCell(activePtr, drawable, TRUE);
	}
    }
    return TCL_OK;
}

typedef struct {
    BLT_TABLE table;			/* Table to be evaluated */
    BLT_TABLE_ROW row;			/* Current row. */
    Blt_HashTable varTable;		/* Variable cache. */
    TableView *viewPtr;

    /* Public values */
    Tcl_Obj *emptyValueObjPtr;
    const char *tag;
    unsigned int flags;
} FindSwitches;

#define FIND_INVERT	(1<<0)

static Blt_SwitchSpec findSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-emptyvalue", "string",	(char *)NULL,
	Blt_Offset(FindSwitches, emptyValueObjPtr), 0},
    {BLT_SWITCH_STRING, "-addtag", "tagName", (char *)NULL,
	Blt_Offset(FindSwitches, tag), 0},
    {BLT_SWITCH_BITMASK, "-invert", "", (char *)NULL,
	Blt_Offset(FindSwitches, flags), 0, FIND_INVERT},
    {BLT_SWITCH_END}
};

static int
ColumnVarResolverProc(
    Tcl_Interp *interp,			/* Current interpreter. */
    const char *name,			/* Variable name being resolved. */
    Tcl_Namespace *nsPtr,		/* Current namespace context. */
    int flags,				/* TCL_LEAVE_ERR_MSG => leave error
					 * message. */
    Tcl_Var *varPtr)			/* (out) Resolved variable. */ 
{
    Blt_HashEntry *hPtr;
    BLT_TABLE_COLUMN col;
    FindSwitches *switchesPtr;
    Tcl_Obj *valueObjPtr;
    long index;
    
    /* 
     * Global variables:  table, viewPtr, varTable, rowPtr.
     */
    hPtr = Blt_FindHashEntry(&findTable, nsPtr);
    if (hPtr == NULL) {
	/* This should never happen.  We can't find data associated with the
	 * current namespace.  But this routine should never be called unless
	 * we're in a namespace that with linked with this variable
	 * resolver. */
	return TCL_CONTINUE;	
    }
    switchesPtr = Blt_GetHashValue(hPtr);

    /* Look up the column from the variable name given. */
    if (Blt_GetLong((Tcl_Interp *)NULL, (char *)name, &index) == TCL_OK) {
 	col = blt_table_get_column_by_index(switchesPtr->table, index);
    } else {
	col = blt_table_get_column_by_label(switchesPtr->table, name);
    }
    if (col == NULL) {
	/* Variable name doesn't refer to any column. Pass it back to the Tcl
	 * interpreter and let it resolve it normally. */
	return TCL_CONTINUE;
    }
    valueObjPtr = blt_table_get_obj(switchesPtr->table, switchesPtr->row, 
	col);
    if (valueObjPtr == NULL) {
	valueObjPtr = switchesPtr->emptyValueObjPtr;
	if (valueObjPtr == NULL) {
	    return TCL_CONTINUE;
	}
    }
    *varPtr = Blt_GetCachedVar(&switchesPtr->varTable, name, valueObjPtr);
    return TCL_OK;
}

static int
EvaluateExpr(Tcl_Interp *interp, Tcl_Obj *exprObjPtr, int *boolPtr)
{
    Tcl_Obj *resultObjPtr;
    int bool;

    if (Tcl_ExprObj(interp, exprObjPtr, &resultObjPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetBooleanFromObj(interp, resultObjPtr, &bool) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_DecrRefCount(resultObjPtr);
    *boolPtr = bool;
    return TCL_OK;
}

static int
FindRows(Tcl_Interp *interp, TableView *viewPtr, Tcl_Obj *objPtr, 
	 FindSwitches *switchesPtr)
{
    Blt_HashEntry *hPtr;
     Tcl_Namespace *nsPtr;
    Tcl_Obj *listObjPtr;
     int isNew;
    int result = TCL_OK;
    long i;

    Tcl_AddInterpResolvers(interp, TABLEVIEW_FIND_KEY, 
	(Tcl_ResolveCmdProc*)NULL, ColumnVarResolverProc, 
	(Tcl_ResolveCompiledVarProc*)NULL);

    Blt_InitHashTable(&switchesPtr->varTable, BLT_ONE_WORD_KEYS);

    if (!initialized) {
	Blt_InitHashTable(&findTable, BLT_ONE_WORD_KEYS);
    }
    nsPtr = Tcl_GetCurrentNamespace(interp);
    hPtr = Blt_CreateHashEntry(&findTable, (char *)nsPtr, &isNew);
    assert(isNew);
    Blt_SetHashValue(hPtr, switchesPtr);

    /* Now process each row, evaluating the expression. */
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (i = 0; i < viewPtr->numRows; i++) {
	int bool;
	Row *rowPtr;
	
	rowPtr = viewPtr->rows[i];
	if (rowPtr->flags & HIDDEN) {
	    continue;			/* Ignore hidden rows. */
	}
	switchesPtr->row = rowPtr->row;
	result = EvaluateExpr(interp, objPtr, &bool);
	if (result != TCL_OK) {
	    break;
	}
	if (switchesPtr->flags & FIND_INVERT) {
	    bool = !bool;
	}
	if (bool) {
	    Tcl_Obj *objPtr;

	    if (switchesPtr->tag != NULL) {
		result = blt_table_set_row_tag(interp, viewPtr->table, 
			rowPtr->row, switchesPtr->tag);
		if (result != TCL_OK) {
		    break;
		}
	    }
	    objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    }
    if (result != TCL_OK) {
	Tcl_DecrRefCount(listObjPtr);
    } else {
	Tcl_SetObjResult(interp, listObjPtr);
    }
    /* Clean up. */
    Blt_DeleteHashEntry(&findTable, hPtr);
    Blt_FreeCachedVars(&switchesPtr->varTable);
    if (!Tcl_RemoveInterpResolvers(interp, TABLEVIEW_FIND_KEY)) {
	Tcl_AppendResult(interp, "can't delete resolver scheme", 
		(char *)NULL);
	return TCL_ERROR;
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterActivateOp --
 *
 *	Selects the filter to appear active.
 *
 *	.t filter activate ?col?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FilterActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Column *colPtr, *activePtr;
    FilterInfo *filterPtr;


    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (((viewPtr->flags & COLUMN_TITLES) == 0) || (colPtr == NULL) ||
	(colPtr->flags & (HIDDEN | DISABLED))) {
fprintf(stderr, "FilterActivate: Column %s is NULL\n", Tcl_GetString(objv[3])); 
	return TCL_OK;			/* Disabled or hidden row. */
    }
    filterPtr = &viewPtr->filter;
    activePtr = filterPtr->activePtr;
    filterPtr->activePtr = colPtr;

    /* If we aren't already queued to redraw the widget, try to directly draw
     * into window. */
    if ((viewPtr->flags & REDRAW_PENDING) == 0) {
	Drawable drawable;

	drawable = Tk_WindowId(viewPtr->tkwin);
	if (activePtr != NULL) {
	    DisplayColumnFilter(viewPtr, activePtr, drawable);
	}
	DisplayColumnFilter(viewPtr, colPtr, drawable);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterCgetOp --
 *
 *	.t filter cget -option
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FilterCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, filterSpecs, 
	(char *)viewPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterConfigureOp --
 *
 * 	This procedure is called to process a list of configuration
 *	options database, in order to reconfigure the one of more
 *	entries in the widget.
 *
 *	  .t filter configure option value
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *	.t filter configure 
 *---------------------------------------------------------------------------
 */
static int
FilterConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, filterSpecs, 
		(char *)viewPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, filterSpecs, 
		(char *)viewPtr, objv[3], 0);
    }
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, filterSpecs, 
	objc - 3, objv + 3, (char *)viewPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterDeactivateOp --
 *
 *	Selects the filter to appear normally.
 *
 *	.t filter deactivate
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FilterDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		   Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Column *activePtr;
    FilterInfo *filterPtr;
    
    if ((viewPtr->flags & COLUMN_TITLES) == 0) {
	return TCL_OK;			/* Disabled or hidden row. */
    }
    filterPtr = &viewPtr->filter;
    activePtr = filterPtr->activePtr;
    filterPtr->activePtr = NULL;
    /* If we aren't already queued to redraw the widget, try to directly draw
     * into window. */
    if ((viewPtr->flags & REDRAW_PENDING) == 0) {
	if (activePtr != NULL) {
	    Drawable drawable;

	    drawable = Tk_WindowId(viewPtr->tkwin);
	    DisplayColumnFilter(viewPtr, activePtr, drawable);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterInsideOp --
 *
 *	.t filter inside cell x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FilterInsideOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Column *colPtr;
    int state;
    int x, y, rootX, rootY;

    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (colPtr == NULL) {
	fprintf(stderr, "FilterInside: Column %s is NULL\n", Tcl_GetString(objv[3])); 
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    /* Convert from root coordinates to window-local coordinates */
    Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
    x -= rootX, y -= rootY;
    state = FALSE;
    if (colPtr != NULL) {
	x = WORLDX(viewPtr, x);
	if ((x >= colPtr->worldX) && (x < (colPtr->worldX + colPtr->width)) &&
	    (y >= viewPtr->inset + viewPtr->colTitleHeight) && 
	    (y < (viewPtr->inset + viewPtr->colTitleHeight + 
		  viewPtr->colFilterHeight))) {
	    state = TRUE;
	}
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterPostOp --
 *
 *	Posts the filter menu associated with this widget.
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
FilterPostOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    const char *menuName;
    Tk_Window tkwin;
    Column *colPtr;
    FilterInfo *filterPtr;
    int x1, y1, x2, y2;
    int rootX, rootY;

    filterPtr = &viewPtr->filter;
    if (objc == 3) {
	long index;

	/* Report the column that has the filter menu posted. */
	index = -1;
	if (filterPtr->postPtr != NULL) {
	    index = blt_table_column_index(filterPtr->postPtr->column);
	}
	Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
	return TCL_OK;
    }
    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (colPtr == NULL) {
	return TCL_OK;
    }
    if (filterPtr->postPtr != NULL) {
	return TCL_OK;		       /* Another filter's menu is currently
					* posted. */
    }
    if (colPtr->flags & (DISABLED|HIDDEN)) {
	return TCL_OK;		       /* Filter's menu is in a column that is
					* hidden or disabled. */
    }
    if (filterPtr->menuObjPtr == NULL) {
	return TCL_OK;			/* No menu associated with filter. */
    }
    menuName = Tcl_GetString(filterPtr->menuObjPtr);
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

    Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
    x1 = SCREENX(viewPtr, colPtr->worldX) + rootX;
    x2 = x1 + colPtr->width;
    y1 = viewPtr->inset + viewPtr->colTitleHeight + rootY;
    y2 = y1 + viewPtr->colFilterHeight;
    
    if (filterPtr->postCmdObjPtr != NULL) {
	Tcl_Obj *cmdObjPtr;
	int result;

	/* Call the designated post command for the filter menu. Pass it the
	 * bounding box of the filter button so it can arrange itself */
	cmdObjPtr = Tcl_DuplicateObj(filterPtr->postCmdObjPtr);
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
	    filterPtr->postPtr = colPtr;
	}
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (strcmp(Tk_Class(tkwin), "BltComboMenu") == 0) {
	Tcl_Obj *cmdObjPtr;
	int result;

	cmdObjPtr = Tcl_DuplicateObj(filterPtr->menuObjPtr);
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
	    filterPtr->postPtr = colPtr;
	}
	return result;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterUnpostOp --
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
FilterUnpostOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    const char *menuName;
    Tk_Window tkwin;
    Column *colPtr;
    FilterInfo *filterPtr;

    filterPtr = &viewPtr->filter;
    if ((filterPtr->menuObjPtr == NULL) || (filterPtr->postPtr == NULL)) {
	return TCL_OK;
    }
    colPtr = filterPtr->postPtr;
    assert((colPtr->flags & (HIDDEN|DISABLED)) == 0);

    menuName = Tcl_GetString(filterPtr->menuObjPtr);
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
    filterPtr->postPtr = NULL;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterOp --
 *
 *	Comparison routine (used by qsort) to sort a chain of subnodes.
 *	A simple string comparison is performed on each node name.
 *
 *	.h filter configure col
 *	.h filter cget col -recurse root
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec filterOps[] =
{
    {"activate",   1, FilterActivateOp,    4, 4, "col",},
    {"cget",       2, FilterCgetOp,        4, 4, "option",},
    {"configure",  2, FilterConfigureOp,   4, 0, "col ?option value?...",},
    {"deactivate", 1, FilterDeactivateOp,  3, 3, "",},
    {"inside",     1, FilterInsideOp,      6, 6, "col x y",},
    {"post",       1, FilterPostOp,        3, 4, "?col?",},
    {"unpost",     1, FilterUnpostOp,      3, 3, "",},
};
static int numFilterOps = sizeof(filterOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
FilterOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numFilterOps, filterOps, BLT_OP_ARG2, objc, 
	    objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * FindOp --
 *
 *	Find rows based upon the expression provided.
 *
 * Results:
 *	A standard TCL result.  The interpreter result will contain a list of
 *	the node serial identifiers.
 *
 *	.t find expr 
 *	
 *---------------------------------------------------------------------------
 */
static int
FindOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    FindSwitches switches;
    int result;
    TableView *viewPtr = clientData;

    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, findSwitches, objc - 3, objv + 3, 
	&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    switches.table = viewPtr->table;
    switches.viewPtr = viewPtr;
    result = FindRows(interp, viewPtr, objv[2], &switches);
    Blt_FreeSwitches(findSwitches, &switches, 0);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * FocusOp --
 *
 *	.t focus ?cell?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FocusOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Cell *cellPtr;

    if (objc == 2) {
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	if (viewPtr->focusPtr != NULL) {
	    CellKey *keyPtr;
	    Column *colPtr;
	    Row *rowPtr;
	    Tcl_Obj *objPtr;

	    keyPtr = GetKey(viewPtr->focusPtr);
	    rowPtr = keyPtr->rowPtr;
	    colPtr = keyPtr->colPtr;
	    objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr != NULL) {
	CellKey *keyPtr;
	Row *rowPtr;
	Column *colPtr;

	keyPtr = GetKey(cellPtr);
	rowPtr = keyPtr->rowPtr;
	colPtr = keyPtr->colPtr;
	if ((rowPtr->flags|colPtr->flags) & (HIDDEN|DISABLED)) {
	    return TCL_OK;		/* Can't set focus to hidden or
					 * disabled cell */
	}
	if (cellPtr != viewPtr->focusPtr) {
	    viewPtr->focusPtr = cellPtr;
	    EventuallyRedraw(viewPtr);
	}
	Blt_SetFocusItem(viewPtr->bindTable, viewPtr->focusPtr, ITEM_CELL);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GrabOp --
 *
 *	.t grab ?cell?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GrabOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    TableView *viewPtr = clientData;

    if (objc == 2) {
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	if (viewPtr->postPtr != NULL) {
	    Tcl_Obj *objPtr;
	    CellKey *keyPtr;
	    Column *colPtr;
	    Row *rowPtr;

	    keyPtr = GetKey(viewPtr->postPtr);
	    colPtr = keyPtr->colPtr;
	    rowPtr = keyPtr->rowPtr;
	    objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_SetCurrentItem(viewPtr->bindTable, viewPtr->postPtr, ITEM_CELL);
    viewPtr->postPtr = cellPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HighlightOp --
 *
 *	Makes the cell appear highlighted.  The cell is redrawn in its
 *	highlighted foreground and background colors.
 *
 *	.t highlight cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HighlightOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Cell *cellPtr;
    const char *string;

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    /* If we aren't already queued to redraw the widget, try to directly draw
     * into window. */
    string = Tcl_GetString(objv[1]);
    if (string[0] == 'h') {
	cellPtr->flags |= HIGHLIGHT;
    } else {
	cellPtr->flags &= ~HIGHLIGHT;
    }
    if ((viewPtr->flags & REDRAW_PENDING) == 0) {
	Drawable drawable;

	drawable = Tk_WindowId(viewPtr->tkwin);
	DisplayCell(cellPtr, drawable, TRUE);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IdentifyOp --
 *
 *	.t identify cell x y 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IdentifyOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellKey *keyPtr;
    CellStyle *stylePtr;
    Column *colPtr;
    Row *rowPtr;
    TableView *viewPtr = clientData;
    const char *string;
    int x, y, rootX, rootY;

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    rowPtr = keyPtr->rowPtr;
    /* Convert from root coordinates to window-local coordinates to cell-local
     * coordinates */
    Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
    x -= rootX + SCREENX(viewPtr, colPtr->worldX);
    y -= rootY + SCREENY(viewPtr, rowPtr->worldY);
    string = NULL;
    stylePtr = GetCurrentStyle(viewPtr, rowPtr, colPtr, cellPtr);
    if (stylePtr->classPtr->identProc != NULL) {
	string = (*stylePtr->classPtr->identProc)(cellPtr, stylePtr, x, y);
    }
    if (string != NULL) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), string, -1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IndexOp --
 *
 *	.t index cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IndexOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellKey *keyPtr;
    Column *colPtr;
    Row *rowPtr;
    TableView *viewPtr = clientData;
    Tcl_Obj *listObjPtr, *objPtr;

    if ((GetCellFromObj(NULL, viewPtr, objv[2], &cellPtr) != TCL_OK) ||
	(cellPtr == NULL)) {
	return TCL_OK;
    }
    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    rowPtr = keyPtr->rowPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InsideOp --
 *
 *	.t inside cell x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InsideOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Cell *cellPtr;
    int state;
    int x, y, rootX, rootY;

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    /* Convert from root coordinates to window-local coordinates */
    Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
    x -= rootX, y -= rootY;
    state = FALSE;
    if (cellPtr != NULL) {
	Row *rowPtr;
	Column *colPtr;
	CellKey *keyPtr;

	keyPtr = GetKey(cellPtr);
	colPtr = keyPtr->colPtr;
	rowPtr = keyPtr->rowPtr;
	x = WORLDX(viewPtr, x);
	y = WORLDY(viewPtr, y);
	
	if ((x >= colPtr->worldX) && (x < (colPtr->worldX + colPtr->width)) &&
	    (y >= rowPtr->worldY) && (y < (rowPtr->worldY + rowPtr->height))) {
	    state = TRUE;
	}
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InvokeOp --
 *
 *	.t invoke cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InvokeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Row *rowPtr;
    Column *colPtr;
    Cell *cellPtr;
    CellStyle *stylePtr;
    CellKey *keyPtr;

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    rowPtr = keyPtr->rowPtr;
    stylePtr = GetCurrentStyle(viewPtr, rowPtr, colPtr, cellPtr);
    if (stylePtr->cmdObjPtr != NULL) {
	int result;
	Tcl_Obj *cmdObjPtr, *objPtr;

	cmdObjPtr = Tcl_DuplicateObj(stylePtr->cmdObjPtr);
	objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
	Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	objPtr = Tcl_NewLongObj(blt_table_column_index(colPtr->column));
	Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	Tcl_IncrRefCount(cmdObjPtr);
	Tcl_Preserve(cellPtr);
	result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
	Tcl_Release(cellPtr);
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
 * IsHiddenOp --
 *
 *	.t ishidden cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsHiddenOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    TableView *viewPtr = clientData;
    int state;

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    state = FALSE;
    if (cellPtr != NULL) {
	CellKey *keyPtr;
	Column *colPtr;
	Row *rowPtr;

	keyPtr = GetKey(cellPtr);
	colPtr = keyPtr->colPtr;
	rowPtr = keyPtr->rowPtr;
	state = ((rowPtr->flags|colPtr->flags) & HIDDEN);
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowActivateOp --
 *
 *	Selects the button to appear active.
 *
 *	.t row activate row
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Drawable drawable;
    Row *rowPtr, *activePtr;
    
    if (GetRow(interp, viewPtr, objv[3], &rowPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (rowPtr == NULL) {
	return TCL_OK;
    }
    if (((viewPtr->flags & ROW_TITLES) == 0) || 
	(rowPtr->flags & (HIDDEN | DISABLED))) {
	return TCL_OK;			/* Disabled or hidden row. */
    }
    activePtr = viewPtr->rowActiveTitlePtr;
    viewPtr->rowActiveTitlePtr = rowPtr;
    drawable = Tk_WindowId(viewPtr->tkwin);
    /* If we aren't already queued to redraw the widget, try to directly draw
     * into window. */
    if ((viewPtr->flags & REDRAW_PENDING) == 0) {
	if (activePtr != NULL) {
	    DisplayRowTitle(viewPtr, activePtr, drawable);
	}
	DisplayRowTitle(viewPtr, rowPtr, drawable);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowBindOp --
 *
 *	  .t row bind tag sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowBindOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	  Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    ClientData object;
    Row *rowPtr;

    if (GetRow(NULL, viewPtr, objv[3], &rowPtr) == TCL_OK) {
	object = rowPtr;
    } else {
	object = RowTag(viewPtr, Tcl_GetString(objv[3]));
    }
    return Blt_ConfigureBindingsFromObj(interp, viewPtr->bindTable, object,
	objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * RowCgetOp --
 *
 *---------------------------------------------------------------------------
 */
static int
RowCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	  Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Row *rowPtr;

    if (GetRow(interp, viewPtr, objv[3], &rowPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (rowPtr == NULL) {
	return TCL_OK;
    }
    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, rowSpecs, 
	(char *)rowPtr, objv[4], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * RowConfigureOp --
 *
 * 	This procedure is called to process a list of configuration
 *	options database, in order to reconfigure the one of more
 *	entries in the widget.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for viewPtr; old resources get freed, if there
 *	were any.  The hypertext is redisplayed.
 *
 *	.tv row configure row ?option value?
 *
 *---------------------------------------------------------------------------
 */
static int
RowConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Row *rowPtr;

    uidOption.clientData = viewPtr;
    iconOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    if (GetRow(interp, viewPtr, objv[3], &rowPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (rowPtr == NULL) {
	return TCL_OK;
    }
    if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, rowSpecs, 
		(char *)rowPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 5) {
	return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, rowSpecs, 
		(char *)rowPtr, objv[4], 0);
    }
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, rowSpecs, 
	objc - 4, objv + 4, (char *)rowPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    ConfigureRow(viewPtr, rowPtr);

    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * RowDeactivateOp --
 *
 *	Turn off active highlighting for all row titles.
 *
 *	.t row deactivate
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Drawable drawable;
    Row *activePtr;
    
    if ((viewPtr->flags & TITLES_MASK) == 0) {
	return TCL_OK;			/* Not displaying row titles. */
    } /*  */
    activePtr = viewPtr->rowActiveTitlePtr;
    viewPtr->rowActiveTitlePtr = NULL;
    drawable = Tk_WindowId(viewPtr->tkwin);

    /* If we aren't already queued to redraw the widget, try to directly draw
     * into window. */
    if ((viewPtr->flags & REDRAW_PENDING) == 0) {
	if (activePtr != NULL) {
	    DisplayRowTitle(viewPtr, activePtr, drawable);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowDeleteOp --
 *
 *	.t row delete row row row 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
	    Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    long i, count;
    Blt_Chain chain;
    Blt_ChainLink link;

    /* Mark all the named columns as deleted. */
    chain = IterateRowsObjv(interp, viewPtr, objc - 3, objv + 3);
    if (chain == NULL) {
	return TCL_ERROR;
    }
    for (link = Blt_Chain_FirstLink(chain); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
	Row *rowPtr;

	rowPtr = Blt_Chain_GetValue(link);
	rowPtr->flags |= DELETED;
    }
    Blt_Chain_Destroy(chain);

    /* Now compress the rows array while freeing the marked rows. */
    count = 0;
    for (i = 0; i < viewPtr->numRows; i++) {
	Row *rowPtr;
	
	rowPtr = viewPtr->rows[i];
	if (rowPtr->flags & DELETED) {
	    DestroyRow(rowPtr);
	    continue;
	}
	viewPtr->rows[count] = rowPtr;
	count++;
    }
    /* Requires a new layout. Sort order and individual geometies stay the
     * same. */
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowExistsOp --
 *
 *	.tv row exists row
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
	    Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int exists;
    Row *rowPtr;

    exists = FALSE;
    if (GetRow(NULL, viewPtr, objv[3], &rowPtr) == TCL_OK) {
	exists = (rowPtr != NULL);
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), exists);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowExposeOp --
 *
 *	.tv row expose ?row...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowExposeOp(ClientData clientData, Tcl_Interp *interp, int objc,
	    Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    if (objc == 3) {
	long i;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	for (i = 0; i < viewPtr->numRows; i++) {
	    Row *rowPtr;

	    rowPtr = viewPtr->rows[i];
	    if ((rowPtr->flags & HIDDEN) == 0) {
		Tcl_Obj *objPtr;

		objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	}
	Tcl_SetObjResult(interp, listObjPtr);
    } else {
	int redraw;
	Blt_Chain chain;
	Blt_ChainLink link;
	
	chain = IterateRowsObjv(interp, viewPtr, objc - 3, objv + 3);
	if (chain == NULL) {
	    return TCL_ERROR;
	}
	redraw = FALSE;
	for (link = Blt_Chain_FirstLink(chain); link != NULL; 
	     link = Blt_Chain_NextLink(link)) {
	    Row *rowPtr;
	    
	    rowPtr = Blt_Chain_GetValue(link);
	    if (rowPtr->flags & HIDDEN) {
		rowPtr->flags &= ~HIDDEN;
		redraw = TRUE;
	    }
	}
	Blt_Chain_Destroy(chain);
	if (redraw) {
	    viewPtr->flags |= SCROLL_PENDING;
	    EventuallyRedraw(viewPtr);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowHideOp --
 *
 *	.tv row hide ?row...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowHideOp(ClientData clientData, Tcl_Interp *interp, int objc,
	  Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    if (objc == 3) {
	long i;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	for (i = 0; i < viewPtr->numRows; i++) {
	    Row *rowPtr;

	    rowPtr = viewPtr->rows[i];
	    if (rowPtr->flags & HIDDEN) {
		Tcl_Obj *objPtr;

		objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	}
	Tcl_SetObjResult(interp, listObjPtr);
    } else {
	int redraw;
	Blt_Chain chain;
	Blt_ChainLink link;
	
	chain = IterateRowsObjv(interp, viewPtr, objc - 3, objv + 3);
	if (chain == NULL) {
	    return TCL_ERROR;
	}
	redraw = FALSE;
	for (link = Blt_Chain_FirstLink(chain); link != NULL; 
	     link = Blt_Chain_NextLink(link)) {
	    Row *rowPtr;
	    
	    rowPtr = Blt_Chain_GetValue(link);
	    if ((rowPtr->flags & HIDDEN) == 0) {
		rowPtr->flags |= HIDDEN;
		redraw = TRUE;
	    }
	}
	Blt_Chain_Destroy(chain);
	if (redraw) {
	    viewPtr->flags |= SCROLL_PENDING;
	    EventuallyRedraw(viewPtr);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowIndexOp --
 *
 *	.t row index row
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowIndexOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    Row *rowPtr;
    TableView *viewPtr = clientData;
    long index;

    if (GetRow(interp, viewPtr, objv[3], &rowPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    index = (rowPtr != NULL) ? blt_table_row_index(rowPtr->row) : -1;
    Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowInsertOp --
 *
 *	Add new rows to the displayed in the tableview widget.  
 *
 *	.t row insert row position ?option values?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowInsertOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    BLT_TABLE_ROW row;
    Blt_HashEntry *hPtr;
    CellKey key;
    Row **rows;
    Row *rowPtr;
    TableView *viewPtr = clientData;
    int isNew;
    long i, insertPos;

    row = blt_table_get_row(interp, viewPtr->table, objv[3]);
    if (row == NULL) {
	return TCL_ERROR;
    }
    hPtr = Blt_CreateHashEntry(&viewPtr->rowTable, (char *)row, &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "a row \"", Tcl_GetString(objv[3]),
		"\" already exists in \"", Tk_PathName(viewPtr->tkwin),
		"\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (Blt_GetPositionFromObj(viewPtr->interp, objv[4], &insertPos) != TCL_OK){
	return TCL_ERROR;
    }
    if ((insertPos == -1) || (insertPos >= viewPtr->numRows)) {
	insertPos = viewPtr->numRows;		/* Insert at end of list. */
    }
    rowPtr = NewRow(viewPtr, row, hPtr);
    iconOption.clientData = viewPtr;
    uidOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(viewPtr->interp, viewPtr->tkwin, 
	rowPtr->title, "Row", rowSpecs, objc - 4, objv + 4, (char *)rowPtr, 0) 
	!= TCL_OK) {
	DestroyRow(rowPtr);
	return TCL_ERROR;
    }
    rows = Blt_AssertCalloc(viewPtr->numRows + 1, sizeof(Row *));
    if (insertPos > 0) {
	memcpy(rows, viewPtr->rows, insertPos * sizeof(Row *));
    }
    rows[insertPos] = rowPtr;
    if (insertPos < viewPtr->numRows) {
	memcpy(rows + insertPos + 1, viewPtr->rows + insertPos, 
	       (viewPtr->numRows - insertPos) * sizeof(Row *));
    }	
    EventuallyRedraw(viewPtr);
    if (viewPtr->rows != NULL) {
        Blt_Free(viewPtr->rows);
    }
    viewPtr->rows = rows;
    key.rowPtr = rowPtr;
    for (i = 0; i < viewPtr->numColumns; i++) {
	Blt_HashEntry *hPtr;
	int isNew;

	key.colPtr = viewPtr->columns[i];
	hPtr = Blt_CreateHashEntry(&viewPtr->cellTable, (char *)&key, &isNew);
	if (isNew) {
	    Cell *cellPtr;

	    cellPtr = NewCell(viewPtr, hPtr);
	    Blt_SetHashValue(hPtr, cellPtr);
	}
    }
    viewPtr->flags |= GEOMETRY;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowInvokeOp --
 *
 * 	This procedure is called to invoke a command associated with the
 *	row title.  The title must be not disabled or hidden for the 
 *	command to be executed.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 *	.t row invoke rowName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowInvokeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    Row *rowPtr;
    TableView *viewPtr = clientData;
    Tcl_Obj *objPtr, *cmdObjPtr;
    int result;

    if (GetRow(interp, viewPtr, objv[3], &rowPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    cmdObjPtr = (rowPtr->cmdObjPtr == NULL) 
	? viewPtr->rowCmdObjPtr : rowPtr->cmdObjPtr;
    if (((viewPtr->flags & ROW_TITLES) == 0) || (rowPtr == NULL) ||
	(rowPtr->flags & (DISABLED|HIDDEN)) || (cmdObjPtr == NULL)) {
	return TCL_OK;
    }
    Tcl_Preserve(viewPtr);
    cmdObjPtr = Tcl_DuplicateObj(cmdObjPtr);  
    objPtr = Tcl_NewLongObj(blt_table_row_index(rowPtr->row));
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    Tcl_IncrRefCount(cmdObjPtr);
    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(cmdObjPtr);
    Tcl_Release(viewPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowMoveOp --
 *
 *	Move one or more rows.
 *
 *	.t row move first last newPos
 *
 *---------------------------------------------------------------------------
 */

/*
 *---------------------------------------------------------------------------
 *
 * RowNamesOp --
 *
 *	.t row names ?pattern...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Tcl_Obj *listObjPtr;
    long i;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (i = 0; i < viewPtr->numRows; i++) {
	Row *rowPtr;
	Tcl_Obj *objPtr;

	rowPtr = viewPtr->rows[i];
	objPtr = Tcl_NewStringObj(blt_table_row_label(rowPtr->row), -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowNearestOp --
 *
 *	.t row nearest y ?-root?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowNearestOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int y;			   /* Screen coordinates of the test point. */
    Row *rowPtr;
    long index;

#ifdef notdef
    int isRoot;

    isRoot = FALSE;
    string = Tcl_GetString(objv[3]);

    if (strcmp("-root", string) == 0) {
	isRoot = TRUE;
	objv++, objc--;
    }
    if (objc != 5) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " ", Tcl_GetString(objv[1]), 
		Tcl_GetString(objv[2]), " ?-root? x y\"", (char *)NULL);
	return TCL_ERROR;
			 
    }
#endif
    if (Tk_GetPixelsFromObj(interp, viewPtr->tkwin, objv[3], &y) != TCL_OK) {
	return TCL_ERROR;
    } 
    rowPtr = NearestRow(viewPtr, y, TRUE);
    index = (rowPtr != NULL) ? blt_table_row_index(rowPtr->row) : -1;
    Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * UpdateRowMark --
 *
 *---------------------------------------------------------------------------
 */
static void
UpdateRowMark(TableView *viewPtr, int newMark)
{
    Row *rowPtr;
    int dy;
    int height;

    if (viewPtr->rowResizePtr == NULL) {
	return;				/* No row being resized. */
    }
    rowPtr = viewPtr->rowResizePtr;
    dy = newMark - viewPtr->rowResizeAnchor; 
    height = rowPtr->height;
    if ((rowPtr->reqMin > 0) && ((height + dy) < rowPtr->reqMin)) {
	dy = rowPtr->reqMin - height;
    }
    if ((rowPtr->reqMax > 0) && ((height + dy) > rowPtr->reqMax)) {
	dy = rowPtr->reqMax - height;
    }
    if ((height + dy) < 4) {
	dy = 4 - height;
    }
    viewPtr->rowResizeMark = viewPtr->rowResizeAnchor + dy;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowResizeActivateOp --
 *
 *	Turns on/off the resize cursor.
 *
 *	.t row resize activate row 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowResizeActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		    Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Row *rowPtr;

    if (GetRow(interp, viewPtr, objv[4], &rowPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((rowPtr == NULL) || (rowPtr->flags & (HIDDEN|DISABLED))){
	return TCL_OK;
    }
    if (viewPtr->rowResizeCursor != None) {
	Tk_DefineCursor(viewPtr->tkwin, viewPtr->rowResizeCursor);
    } 
    viewPtr->rowResizePtr = rowPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowResizeAnchorOp --
 *
 *	Set the anchor for the resize.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowResizeAnchorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int y;

    if (Tcl_GetIntFromObj(NULL, objv[4], &y) != TCL_OK) {
	return TCL_ERROR;
    } 
    viewPtr->rowResizeAnchor = y;
    viewPtr->flags |= ROW_RESIZE;
    UpdateRowMark(viewPtr, y);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowResizeDeactiveOp --
 *
 *	Turns off the resize cursor.
 *
 *	.t row resize deactivate 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowResizeDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		      Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    Tk_UndefineCursor(viewPtr->tkwin);
    viewPtr->rowResizePtr = NULL;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowResizeMarkOp --
 *
 *	Sets the resize mark.  The distance between the mark and the anchor
 *	is the delta to change the width of the active row.
 *
 *	.t row resize mark x 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowResizeMarkOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int y;

    if (Tcl_GetIntFromObj(NULL, objv[4], &y) != TCL_OK) {
	return TCL_ERROR;
    } 
    viewPtr->flags |= ROW_RESIZE;
    UpdateRowMark(viewPtr, y);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowResizeCurrentOp --
 *
 *	Returns the new width of the row including the resize delta.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowResizeCurrentOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		   Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    viewPtr->flags &= ~ROW_RESIZE;
    UpdateRowMark(viewPtr, viewPtr->rowResizeMark);
    if (viewPtr->rowResizePtr != NULL) {
	int height, delta;

	delta = (viewPtr->rowResizeMark - viewPtr->rowResizeAnchor);
	height = viewPtr->rowResizePtr->height + delta;
	Tcl_SetIntObj(Tcl_GetObjResult(interp), height);
    }
    return TCL_OK;
}

static Blt_OpSpec rowResizeOps[] =
{ 
    {"activate",   2, RowResizeActivateOp,   5, 5, "row"},
    {"anchor",     2, RowResizeAnchorOp,     5, 5, "x"},
    {"current",    1, RowResizeCurrentOp,    4, 4, "",},
    {"deactivate", 1, RowResizeDeactivateOp, 4, 4, ""},
    {"mark",       1, RowResizeMarkOp,       5, 5, "x"},
};

static int numRowResizeOps = sizeof(rowResizeOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * RowResizeOp --
 *
 *---------------------------------------------------------------------------
 */
static int
RowResizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numRowResizeOps, rowResizeOps, BLT_OP_ARG3, 
	objc, objv,0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * RowSeeOp --
 *
 *	Implements the quick scan.
 *
 *	.t row see row
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowSeeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    Row *rowPtr;
    TableView *viewPtr = clientData;
    long y, viewHeight;

    if (GetRow(interp, viewPtr, objv[3], &rowPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (rowPtr == NULL) {
	return TCL_OK;
    }
    viewHeight = VPORTHEIGHT(viewPtr);
    y = viewPtr->yOffset;
    if (rowPtr->worldY < y) {
	y = rowPtr->worldY;
    } else if ((rowPtr->worldY + rowPtr->height) > (y + viewHeight)) {
	y = rowPtr->worldY + rowPtr->height - viewHeight;
    }
    if (y < 0) {
	y = 0;
    }
    if (y != viewPtr->yOffset) {
	viewPtr->yOffset = y;
	viewPtr->flags |= SCROLLY;
	EventuallyRedraw(viewPtr);
    }
    return TCL_OK;
}

static Blt_OpSpec rowOps[] =
{
    {"activate",   1, RowActivateOp,   4, 4, "row",},
    {"bind",       1, RowBindOp,       4, 6, "tagName ?sequence command?",},
    {"cget",       2, RowCgetOp,       5, 5, "row option",},
    {"configure",  2, RowConfigureOp,  4, 0, "row ?option value?...",},
    {"deactivate", 3, RowDeactivateOp, 3, 3, "",},
    {"delete",     3, RowDeleteOp,     4, 0, "row...",},
    {"exists",     3, RowExistsOp,     4, 4, "row",},
    {"expose",     3, RowExposeOp,     3, 0, "?row...?",},
    {"hide",       1, RowHideOp,       3, 0, "?row...?",},
    {"index",      3, RowIndexOp,      4, 4, "row",},
    {"insert",     3, RowInsertOp,     5, 0, "row position ?option value?...",},
    {"invoke",     3, RowInvokeOp,     4, 4, "row",},
    {"names",      2, RowNamesOp,      3, 3, "",},
    {"nearest",    2, RowNearestOp,    4, 4, "y",},
    {"resize",     1, RowResizeOp,     3, 0, "args",},
    {"see",        2, RowSeeOp,        4, 4, "row",},
    {"show",       2, RowExposeOp,     3, 0, "?row...?",},
};
static int numRowOps = sizeof(rowOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * RowOp --
 *
 *---------------------------------------------------------------------------
 */
static int
RowOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numRowOps, rowOps, BLT_OP_ARG2, 
	objc, objv,0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
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
ScanOp(TableView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int x, y;
    char c;
    int length;
    int oper;
    char *string;
    Tk_Window tkwin;

#define SCAN_MARK	1
#define SCAN_DRAGTO	2
    string = Tcl_GetStringFromObj(objv[2], &length);
    c = string[0];
    tkwin = viewPtr->tkwin;
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
	viewPtr->scanAnchorX = x;
	viewPtr->scanAnchorY = y;
	viewPtr->scanX = viewPtr->xOffset;
	viewPtr->scanY = viewPtr->yOffset;
    } else {
	int worldX, worldY;
	int dx, dy;

	dx = viewPtr->scanAnchorX - x;
	dy = viewPtr->scanAnchorY - y;
	worldX = viewPtr->scanX + (10 * dx);
	worldY = viewPtr->scanY + (10 * dy);

	if (worldX < 0) {
	    worldX = 0;
	} else if (worldX >= viewPtr->worldWidth) {
	    worldX = viewPtr->worldWidth - viewPtr->xScrollUnits;
	}
	if (worldY < 0) {
	    worldY = 0;
	} else if (worldY >= viewPtr->worldHeight) {
	    worldY = viewPtr->worldHeight - viewPtr->yScrollUnits;
	}
	viewPtr->xOffset = worldX;
	viewPtr->yOffset = worldY;
	viewPtr->flags |= SCROLL_PENDING;
	EventuallyRedraw(viewPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SeeOp --
 *
 *	Changes to view to encompass the specified cell.
 *
 *	.t see cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SeeOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellKey *keyPtr;
    Column *colPtr;
    Row *rowPtr;
    TableView *viewPtr = clientData;
    long x, y;
    int viewWidth, viewHeight;

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    rowPtr = keyPtr->rowPtr;
    viewWidth = VPORTWIDTH(viewPtr);
    viewHeight = VPORTHEIGHT(viewPtr);
    y = viewPtr->yOffset;
    x = viewPtr->xOffset;
    if (rowPtr->worldY < y) {
	y = rowPtr->worldY;
    } else if ((rowPtr->worldY + rowPtr->height) > (y + viewHeight)) {
	y = rowPtr->worldY + rowPtr->height - viewHeight;
    }
    if (colPtr->worldX < x) {
	x = colPtr->worldX;
    } else if ((colPtr->worldX + colPtr->width) > (x + viewWidth)) {
	x = colPtr->worldX + colPtr->width - viewWidth;
    }
    if (x < 0) {
	x = 0;
    }
    if (y < 0) {
	y = 0;
    }
    if (x != viewPtr->xOffset) {
	viewPtr->xOffset = x;
	viewPtr->flags |= SCROLLX;
    }
    if (y != viewPtr->yOffset) {
	viewPtr->yOffset = y;
	viewPtr->flags |= SCROLLY;
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionAnchorOp --
 *
 *	Sets the selection anchor to the element given by a index.  The
 *	selection anchor is the end of the selection that is fixed while
 *	dragging out a selection with the mouse.  The index "anchor" may be
 *	used to refer to the anchor element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *	.t selection anchor cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionAnchorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellKey *keyPtr;
    TableView *viewPtr = clientData;

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    keyPtr = GetKey(cellPtr);
    if (viewPtr->selectMode == SELECT_CELLS) {
	CellSelection *selectPtr;

	selectPtr = &viewPtr->selectCells;
	/* Set both the selection anchor and the mark. This indicates that a
	 * single cell is selected. */
	selectPtr->markPtr = selectPtr->anchorPtr = keyPtr;
    } else {
	RowSelection *selectPtr;

	selectPtr = &viewPtr->selectRows;
	/* Set both the anchor and the mark. Indicates that a single row is
	 * selected. */
	selectPtr->anchorPtr = selectPtr->markPtr = keyPtr->rowPtr;
	SelectRow(viewPtr, keyPtr->rowPtr);
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionClearallOp
 *
 *	Clears the entire selection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *	.t selection clearall
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionClearallOp(ClientData clientData, Tcl_Interp *interp, int objc,
		    Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    ClearSelections(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionIncludesOp
 *
 *	Returns 1 if the element indicated by index is currently
 *	selected, 0 if it isn't.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *	.t selection includes cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionIncludesOp(ClientData clientData, Tcl_Interp *interp, int objc,
		    Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    TableView *viewPtr = clientData;
    int state;
    CellKey *keyPtr;
    Column *colPtr;
    Row *rowPtr;

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	Tcl_SetBooleanObj(Tcl_GetObjResult(interp), FALSE);
	return TCL_OK;
    }
    state = FALSE;
    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    rowPtr = keyPtr->rowPtr;
    if (viewPtr->selectMode == SELECT_CELLS) {
	if (((rowPtr->flags|colPtr->flags) & (HIDDEN | DISABLED)) == 0) {
	    CellSelection *selectPtr = &viewPtr->selectCells;
	    
	    if ((selectPtr->anchorPtr->rowPtr->index <= rowPtr->index) &&
		(selectPtr->anchorPtr->colPtr->index <= rowPtr->index) &&
		(selectPtr->markPtr->rowPtr->index >= rowPtr->index) &&
		(selectPtr->markPtr->colPtr->index >= rowPtr->index)) {
		state = TRUE;
	    }
	}
    } else {
	if ((rowPtr->flags & (HIDDEN | DISABLED)) == 0) {
	    state = rowPtr->flags & SELECTED;
	}
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionMarkOp --
 *
 *	Sets the selection mark to the element given by a index.  The
 *	selection anchor is the end of the selection that is movable while
 *	dragging out a selection with the mouse.  The index "mark" may be used
 *	to refer to the anchor element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *	.t selection mark cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionMarkOp(ClientData clientData, Tcl_Interp *interp, int objc,
		Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Cell *cellPtr;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    if (viewPtr->selectMode == SELECT_CELLS) {
	CellSelection *selectPtr;
	
	selectPtr = &viewPtr->selectCells;
	if (selectPtr->anchorPtr == NULL) {
	    fprintf(stderr, "cell selection anchor must be set first\n");
	    return TCL_OK;
	}
	selectPtr->markPtr = GetKey(cellPtr);
    } else {
	RowSelection *selectPtr;
	Row *rowPtr;
	CellKey *keyPtr;
	
	selectPtr = &viewPtr->selectRows;
	if (selectPtr->anchorPtr == NULL) {
	    Tcl_AppendResult(interp, "row selection anchor must be set first", 
			     (char *)NULL);
	    return TCL_ERROR;
	}
	keyPtr = GetKey(cellPtr);
	rowPtr = keyPtr->rowPtr;
	if (selectPtr->markPtr != rowPtr) {
	    Blt_ChainLink link, next;

	    /* Deselect rows from the list all the way back to the anchor. */
	    for (link = Blt_Chain_LastLink(selectPtr->list); link != NULL; 
		link = next) {
		Row *selRowPtr;

		next = Blt_Chain_PrevLink(link);
		selRowPtr = Blt_Chain_GetValue(link);
		if (selRowPtr == selectPtr->anchorPtr) {
		    break;
		}
		DeselectRow(viewPtr, selRowPtr);
	    }
	    selectPtr->flags &= ~SELECT_MASK;
	    selectPtr->flags |= SELECT_SET;
	    SelectRows(viewPtr, selectPtr->anchorPtr, rowPtr);
	    selectPtr->markPtr = rowPtr;
	}
    }
    EventuallyRedraw(viewPtr);
    if (viewPtr->selectCmdObjPtr != NULL) {
	EventuallyInvokeSelectCommand(viewPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionPresentOp
 *
 *	Returns 1 if there is a selection and 0 if it isn't.
 *
 * Results:
 *	A standard TCL result.  interp->result will contain a boolean string
 *	indicating if there is a selection.
 *
 *	.t selection present 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionPresentOp(ClientData clientData, Tcl_Interp *interp, int objc,
		   Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int state;

    if (viewPtr->selectMode == SELECT_CELLS) {
	state = (viewPtr->selectCells.anchorPtr != NULL);
    } else {
	state = (Blt_Chain_GetLength(viewPtr->selectRows.list) > 0);
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionSetOp
 *
 *	Selects, deselects, or toggles all of the elements in the range
 *	between first and last, inclusive, without affecting the selection
 *	state of elements outside that range.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *	.t selection set cell ?cell?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Row *rowAnchorPtr, *rowMarkPtr;
    Column *colAnchorPtr, *colMarkPtr;
    Cell *cellPtr;
    CellKey *anchorPtr, *markPtr;

    if (viewPtr->flags & (GEOMETRY | LAYOUT_PENDING)) {
	/*
	 * The layout is dirty.  Recompute it now so that we can use
	 * view.top and view.bottom for nodes.
	 */
	ComputeGeometry(viewPtr);
    }
    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    markPtr = anchorPtr = GetKey(cellPtr);
    colMarkPtr = colAnchorPtr = anchorPtr->colPtr;
    rowMarkPtr = rowAnchorPtr = anchorPtr->rowPtr;
    if ((rowAnchorPtr->flags|colAnchorPtr->flags) & HIDDEN) {
	Tcl_AppendResult(interp, "can't select hidden anchor", (char *)NULL);
	return TCL_ERROR;
    }
    if (objc == 5) {
	if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (cellPtr == NULL) {
	    return TCL_OK;
	}
	markPtr = GetKey(cellPtr);
	colMarkPtr = markPtr->colPtr;
	rowMarkPtr = markPtr->rowPtr;
	if ((rowMarkPtr->flags|colMarkPtr->flags) & HIDDEN) {
	    Tcl_AppendResult(interp, "can't select hidden mark", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (viewPtr->selectMode == SELECT_CELLS) {
	CellSelection *selectPtr = &viewPtr->selectCells;
	const char *string;

	string = Tcl_GetString(objv[2]);
	if (strcmp(string, "set") != 0) {
	    anchorPtr = markPtr = NULL;
	}
	selectPtr->anchorPtr = anchorPtr;
	selectPtr->markPtr = markPtr;
    } else {
	RowSelection *selectPtr = &viewPtr->selectRows;
	const char *string;

	selectPtr->flags &= ~SELECT_MASK;
	string = Tcl_GetString(objv[2]);
	switch (string[0]) {
	case 's':
	    selectPtr->flags |= SELECT_SET;	break;
	case 'c':
	    selectPtr->flags |= SELECT_CLEAR;	break;
	case 't':
	    selectPtr->flags |= SELECT_TOGGLE;	 break;
	}
	SelectRows(viewPtr, rowAnchorPtr, rowMarkPtr);
	selectPtr->flags &= ~SELECT_MASK;
	selectPtr->anchorPtr = rowAnchorPtr;
	selectPtr->markPtr = rowMarkPtr;
    }
    if (viewPtr->flags & SELECT_EXPORT) {
	Tk_OwnSelection(viewPtr->tkwin, XA_PRIMARY, LostSelection, viewPtr);
    }
    EventuallyRedraw(viewPtr);
    if (viewPtr->selectCmdObjPtr != NULL) {
	EventuallyInvokeSelectCommand(viewPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionOp --
 *
 *	This procedure handles the individual options for text selections.
 *	The selected text is designated by start and end indices into the text
 *	pool.  The selected segment has both a anchored and unanchored ends.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *	.t selection op args
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec selectionOps[] =
{
    {"anchor",   1, SelectionAnchorOp,   4, 4, "cell",},
    {"clear",    5, SelectionSetOp,      4, 5, "cell ?cell?",},
    {"clearall", 6, SelectionClearallOp, 3, 3, "",},
    {"includes", 1, SelectionIncludesOp, 4, 4, "cell",},
    {"mark",     1, SelectionMarkOp,     4, 4, "cell",},
    {"present",  1, SelectionPresentOp,  3, 3, "",},
    {"set",      1, SelectionSetOp,      4, 5, "cell ?cell?",},
    {"toggle",   1, SelectionSetOp,      4, 5, "cell ?cell?",},
};
static int numSelectionOps = sizeof(selectionOps) / sizeof(Blt_OpSpec);

static int
SelectionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numSelectionOps, selectionOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (viewPtr, interp, objc, objv);
}

static int
SortAutoOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    SortInfo *sortPtr;

    sortPtr = &viewPtr->sort;
    if (objc == 4) {
	int state;
	int isAuto;

	isAuto = ((sortPtr->flags & SORT_ALWAYS) != 0);
	if (Tcl_GetBooleanFromObj(interp, objv[3], &state) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (isAuto != state) {
	    viewPtr->flags |= LAYOUT_PENDING;
	    EventuallyRedraw(viewPtr);
	}
	if (state) {
	    sortPtr->flags |= SORT_ALWAYS;
	} else {
	    sortPtr->flags &= ~SORT_ALWAYS;
	}
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), sortPtr->flags & SORT_ALWAYS);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortCgetOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SortCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, sortSpecs, 
	(char *)viewPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * SortConfigureOp --
 *
 * 	This procedure is called to process a list of configuration
 *	options database, in order to reconfigure the one of more
 *	entries in the widget.
 *
 *	  .t sort configure option value
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *---------------------------------------------------------------------------
 */
static int
SortConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    SortInfo *sortPtr;

    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, sortSpecs, 
		(char *)viewPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, sortSpecs, 
		(char *)viewPtr, objv[3], 0);
    }
    sortPtr = &viewPtr->sort;
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, sortSpecs, 
	objc - 3, objv + 3, (char *)viewPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    sortPtr->flags &= ~SORTED;
    viewPtr->flags |= LAYOUT_PENDING;
    if (sortPtr->flags & SORT_ALWAYS) {
	sortPtr->flags |= SORT_PENDING;
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*ARGSUSED*/
static int
SortOnceOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    viewPtr->flags |= LAYOUT_PENDING;
    viewPtr->sort.flags &= ~SORTED;
    viewPtr->sort.flags |= SORT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortOp --
 *
 *	Comparison routine (used by qsort) to sort a chain of subnodes.
 *	A simple string comparison is performed on each node name.
 *
 *	.h sort auto
 *	.h sort once -recurse root
 *
 * Results:
 *	1 is the first is greater, -1 is the second is greater, 0
 *	if equal.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec sortOps[] =
{
    {"auto",      1, SortAutoOp,      3, 4, "?boolean?",},
    {"cget",      2, SortCgetOp,      4, 4, "option",},
    {"configure", 2, SortConfigureOp, 3, 0, "?option value?...",},
    {"once",      1, SortOnceOp,      3, 0, "node...",},
};
static int numSortOps = sizeof(sortOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
SortOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numSortOps, sortOps, BLT_OP_ARG2, objc, 
	    objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}


/*
 *---------------------------------------------------------------------------
 *
 * StyleApplyOp --
 *
 *	  .t style apply styleName cell...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleApplyOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    CellStyle *stylePtr;
    Blt_Chain cells;
    Blt_ChainLink link;

    if (GetStyle(interp, viewPtr, objv[3], &stylePtr)  != TCL_OK) {
	return TCL_ERROR;
    }
    cells = IterateCellsObjv(interp, viewPtr, objc - 4, objv + 4);
    for (link = Blt_Chain_FirstLink(cells); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
	Cell *cellPtr;

	cellPtr = Blt_Chain_GetValue(link);
	if (cellPtr->stylePtr != stylePtr) {
	    Blt_HashEntry *hPtr;
	    CellKey *keyPtr;
	    int isNew;

	    keyPtr = GetKey(cellPtr);
	    if (cellPtr->stylePtr != NULL) {
		/* Remove the cell from old style's table of cells. */
		hPtr = Blt_FindHashEntry(&stylePtr->table, (char *)keyPtr);
		if (hPtr != NULL) {
		    Blt_DeleteHashEntry(&stylePtr->table, hPtr);
		}
		cellPtr->stylePtr->refCount--;
		if (cellPtr->stylePtr->refCount <= 0) {
		    (*cellPtr->stylePtr->classPtr->freeProc)(cellPtr->stylePtr);
		}
	    }
	    stylePtr->refCount++;	
	    cellPtr->stylePtr = stylePtr;
	    Blt_CreateHashEntry(&stylePtr->table, (char *)keyPtr, &isNew);
	    cellPtr->flags |= GEOMETRY;	/* Assume that the new style
					 * changes the geometry of the
					 * cell. */
	}
    }
    Blt_Chain_Destroy(cells);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * StyleCgetOp --
 *
 *	  .t style cget "styleName" -background
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleCgetOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    CellStyle *stylePtr;

    if (GetStyle(interp, viewPtr, objv[3], &stylePtr)  != TCL_OK) {
	return TCL_ERROR;
    }
    iconOption.clientData = viewPtr;
    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, 
	stylePtr->classPtr->specs, (char *)stylePtr, objv[4], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleConfigureOp --
 *
 * 	This procedure is called to process a list of configuration options
 * 	database, in order to reconfigure a style.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font, etc. get
 *	set for stylePtr; old resources get freed, if there were any.
 *
 *	.t style configure styleName ?option value?..
 *
 *---------------------------------------------------------------------------
 */
static int
StyleConfigureOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *const *objv)
{
    CellStyle *stylePtr;

    if (GetStyle(interp, viewPtr, objv[3], &stylePtr)  != TCL_OK) {
	return TCL_ERROR;
    }
    iconOption.clientData = viewPtr;
    if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, 
	    stylePtr->classPtr->specs, (char *)stylePtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 5) {
	return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, 
		stylePtr->classPtr->specs, (char *)stylePtr, objv[5], 0);
    }
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, 
	stylePtr->classPtr->specs, objc - 4, objv + 4, (char *)stylePtr, 
	BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    (*stylePtr->classPtr->configProc)(viewPtr, stylePtr);
    viewPtr->flags |= (LAYOUT_PENDING | GEOMETRY);
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * StyleCreateOp --
 *
 *	  .t style create combobox "styleName" -background blue
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleCreateOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    CellStyle *stylePtr;
    char c;
    const char *string;
    int type, length;

    string = Tcl_GetStringFromObj(objv[3], &length);
    c = string[0];
    if ((c == 't') && (strncmp(string, "textbox", length) == 0)) {
	type = STYLE_TEXTBOX;
    } else if ((c == 'c') && (length > 2) && 
	       (strncmp(string, "checkbox", length) == 0)) {
	type = STYLE_CHECKBOX;
    } else if ((c == 'c') && (length > 2) && 
	       (strncmp(string, "combobox", length) == 0)) {
	type = STYLE_COMBOBOX;
    } else if ((c == 'i') && (strncmp(string, "imagebox", length) == 0)) {
	type = STYLE_IMAGEBOX;
    } else {
	Tcl_AppendResult(interp, "unknown style type \"", string, 
		"\": should be textbox, checkbox, combobox, or imagebox.", 
		(char *)NULL);
	return TCL_ERROR;
    }
    string = Tcl_GetString(objv[4]);
    stylePtr = Blt_TableView_CreateCellStyle(interp, viewPtr, type, string);
    if (stylePtr == NULL) {
	return TCL_ERROR;
    }
    iconOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(interp, viewPtr->tkwin, stylePtr->name, 
	stylePtr->classPtr->className, stylePtr->classPtr->specs, objc - 5, 
	objv + 5, (char *)stylePtr, 0) != TCL_OK) {
	(*stylePtr->classPtr->freeProc)(stylePtr);
	return TCL_ERROR;
    }
    (*stylePtr->classPtr->configProc)(viewPtr, stylePtr);
    Tcl_SetObjResult(interp, objv[4]);
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleDeleteOp --
 *
 * 	Eliminates one or more style names.  A style still may be in use after
 * 	its name has been officially removed.  Only its hash table entry is
 * 	removed.  The style itself remains until its reference count returns
 * 	to zero (i.e. no one else is using it).
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 *	.t style forget styleName...
 *
 *---------------------------------------------------------------------------
 */
static int
StyleDeleteOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    int i;

    for (i = 3; i < objc; i++) {
	CellStyle *stylePtr;

	if (GetStyle(interp, viewPtr, objv[i], &stylePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	/* 
	 * Removing the style from the hash tables frees up the style name
	 * again.  The style itself may not be removed until it's been
	 * released by everything using it.
	 */
	if (stylePtr->hashPtr != NULL) {
	    Blt_DeleteHashEntry(&viewPtr->styleTable, stylePtr->hashPtr);
	    stylePtr->hashPtr = NULL;
	    stylePtr->name = NULL;	/* Name points to hash key. */
	} 
	stylePtr->refCount--;
	if (stylePtr->refCount <= 0) {
	    (*stylePtr->classPtr->freeProc)(stylePtr);
	}
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleExistsOp --
 *
 *	.tv style exists $name
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleExistsOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    int exists;
    const char *name;

    name = Tcl_GetString(objv[3]);
    hPtr = Blt_FindHashEntry(&viewPtr->styleTable, name);
    exists = (hPtr != NULL);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), exists);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleGetOp --
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 *	.t style get cell
 *
 *---------------------------------------------------------------------------
 */
static int
StyleGetOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellKey *keyPtr;
    CellStyle *stylePtr;
    Column *colPtr;
    Row *rowPtr;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    rowPtr = keyPtr->rowPtr;
    stylePtr = GetCurrentStyle(viewPtr, rowPtr, colPtr, cellPtr);
    if (stylePtr->name != NULL) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), stylePtr->name, -1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleNamesOp --
 *
 * 	Lists the names of all the current styles in the tableview widget.
 *
 *	  .t style names
 *
 * Results:
 *	Always TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleNamesOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    Tcl_Obj *listObjPtr, *objPtr;
    CellStyle *stylePtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hPtr = Blt_FirstHashEntry(&viewPtr->styleTable, &cursor); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&cursor)) {
	stylePtr = Blt_GetHashValue(hPtr);
	if (stylePtr->name == NULL) {
	    continue;			/* Style has been deleted, but is
					 * still in use. */
	}
	objPtr = Tcl_NewStringObj(stylePtr->name, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleTypeOp --
 *
 *	  .t style type styleName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleTypeOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    CellStyle *stylePtr;

    if (GetStyle(interp, viewPtr, objv[3], &stylePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), stylePtr->classPtr->type, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleOp --
 *
 *	.t style apply 
 *	.t style cget styleName -foreground
 *	.t style configure styleName -fg blue -bg green
 *	.t style create type styleName ?options?
 *	.t style delete styleName
 *	.t style get cell
 *	.t style names ?pattern?
 *	.t style type styleName 
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec styleOps[] = {
    {"apply",     1, StyleApplyOp,     4, 0, "styleName cell...",},
    {"cget",      2, StyleCgetOp,      5, 5, "styleName option",},
    {"configure", 2, StyleConfigureOp, 4, 0, "styleName options...",},
    {"create",    2, StyleCreateOp,    5, 0, "type styleName options...",},
    {"delete",    1, StyleDeleteOp,    3, 0, "styleName...",},
    {"exists",    1, StyleExistsOp,    4, 4, "styleName",},
    {"get",       1, StyleGetOp,       4, 4, "cell",},
    {"names",     1, StyleNamesOp,     3, 3, "",}, 
    {"type",      1, StyleTypeOp,      4, 4, "styleName",},
};

static int numStyleOps = sizeof(styleOps) / sizeof(Blt_OpSpec);

static int
StyleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numStyleOps, styleOps, BLT_OP_ARG2, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc)(clientData, interp, objc, objv);
    return result;
}


/*
 *---------------------------------------------------------------------------
 *
 * TypeOp --
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 *	.t type cell
 *
 *---------------------------------------------------------------------------
 */
static int
TypeOp(TableView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellKey *keyPtr;
    CellStyle *stylePtr;
    Column *colPtr;
    Row *rowPtr;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (cellPtr == NULL) {
	return TCL_OK;
    }
    keyPtr = GetKey(cellPtr);
    colPtr = keyPtr->colPtr;
    rowPtr = keyPtr->rowPtr;
    stylePtr = GetCurrentStyle(viewPtr, rowPtr, colPtr, cellPtr);
    if (stylePtr->name != NULL) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), stylePtr->classPtr->type,-1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * UpdatesOp --
 *
 *	.tv updates false
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
UpdatesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	  Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int state;

    if (objc == 3) {
	if (Tcl_GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (state) {
	    viewPtr->flags &= ~DONT_UPDATE;
	    viewPtr->flags |= LAYOUT_PENDING | GEOMETRY;
	    EventuallyRedraw(viewPtr);
	} else {
	    viewPtr->flags |= DONT_UPDATE;
	}
    } else {
	state = (viewPtr->flags & DONT_UPDATE) ? FALSE : TRUE;
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * WritableOp --
 *
 *	  .t writable cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
WritableOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    TableView *viewPtr = clientData;
    int state;

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    state = FALSE;
    if (cellPtr != NULL) {
	CellKey *keyPtr;
	CellStyle *stylePtr;
	Column *colPtr;
	Row *rowPtr;

	keyPtr = GetKey(cellPtr);
	colPtr = keyPtr->colPtr;
	rowPtr = keyPtr->rowPtr;
	stylePtr = GetCurrentStyle(viewPtr, rowPtr, colPtr, cellPtr);
	
	state = (stylePtr->flags & EDIT);
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

static int
XViewOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int width, worldWidth;

    width = VPORTWIDTH(viewPtr);
    worldWidth = viewPtr->worldWidth;
    if (objc == 2) {
	double fract;
	Tcl_Obj *listObjPtr;

	/*
	 * Note that we are bounding the fractions between 0.0 and 1.0
	 * to support the "canvas"-style of scrolling.
	 */
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	fract = (double)viewPtr->xOffset / worldWidth;
	fract = FCLAMP(fract);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
	fract = (double)(viewPtr->xOffset + width) / worldWidth;
	fract = FCLAMP(fract);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &viewPtr->xOffset,
	    worldWidth, width, viewPtr->xScrollUnits, viewPtr->scrollMode) 
	    != TCL_OK) {
	return TCL_ERROR;
    }
    viewPtr->flags |= SCROLLX;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

static int
YViewOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int height, worldHeight;

    height = VPORTHEIGHT(viewPtr);
    worldHeight = viewPtr->worldHeight;
    if (objc == 2) {
	double fract;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	/* Report first and last fractions */
	fract = (double)viewPtr->yOffset / worldHeight;
	fract = FCLAMP(fract);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
	fract = (double)(viewPtr->yOffset + height) / worldHeight;
	fract = FCLAMP(fract);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &viewPtr->yOffset,
	    worldHeight, height, viewPtr->yScrollUnits, viewPtr->scrollMode)
	!= TCL_OK) {
	return TCL_ERROR;
    }
    viewPtr->flags |= SCROLLY;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/* 
 * .t op 
 */
static Blt_OpSpec viewOps[] =
{
    {"activate",     1, ActivateOp,      3, 3, "cell"},
    {"bbox",         2, BboxOp,          3, 0, "cell ?cell...?",}, 
    {"bind",         2, BindOp,          3, 5, "cell ?sequence command?",}, 
    {"cell",         2, CellOp,          2, 0, "args",}, 
    {"cget",         2, CgetOp,          3, 3, "option",}, 
    {"column",       3, ColumnOp,	 2, 0, "oper args",}, 
    {"configure",    3, ConfigureOp,     2, 0, "?option value?...",},
    {"curselection", 2, CurselectionOp,  2, 2, "",},
    {"deactivate",   1, DeactivateOp,    2, 2, ""},
    {"filter",       3, FilterOp,	 2, 0, "args",},
    {"find",         3, FindOp,          2, 0, "expr",}, 
    {"focus",        2, FocusOp,         2, 3, "?cell?",}, 
    {"grab",         1, GrabOp,          2, 3, "?cell?",}, 
    {"highlight",    1, HighlightOp,     3, 3, "cell",}, 
    {"identify",     2, IdentifyOp,      5, 5, "cell x y",}, 
    {"index",        3, IndexOp,         3, 3, "cell",}, 
    {"inside",       3, InsideOp,        5, 5, "cell x y",}, 
    {"invoke",       3, InvokeOp,        3, 3, "cell",}, 
    {"ishidden",     2, IsHiddenOp,      3, 3, "cell",},
    {"row",          1, RowOp,		 2, 0, "oper args",}, 
    {"scan",         2, ScanOp,          5, 5, "dragto|mark x y",},
    {"see",          3, SeeOp,           3, 3, "cell",},
    {"selection",    3, SelectionOp,     2, 0, "oper args",},
    {"sort",         2, SortOp,		 2, 0, "args",},
    {"style",        2, StyleOp,         2, 0, "args",},
    {"type",         1, TypeOp,          3, 3, "cell",},
    {"unhighlight",  3, HighlightOp,     3, 3, "cell",}, 
    {"updates",      2, UpdatesOp,       2, 3, "?bool?",},
    {"writable",     1, WritableOp,      3, 3, "cell",},
    {"xview",        1, XViewOp,         2, 5, "?moveto fract? ?scroll number what?",},
    {"yview",        1, YViewOp,         2, 5, "?moveto fract? ?scroll number what?",},
};

static int numViewOps = sizeof(viewOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * TableViewInstObjCmdProc --
 *
 * 	This procedure is invoked to process commands on behalf of the
 * 	tableview widget.
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
TableViewInstObjCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
			Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numViewOps, viewOps, BLT_OP_ARG1, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    Tcl_Preserve(viewPtr);
    result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(viewPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TableViewInstCmdDeleteProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If the
 *	widget isn't already in the process of being destroyed, this command
 *	destroys it.
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
TableViewInstCmdDeleteProc(ClientData clientData)
{
    TableView *viewPtr = clientData;

    /*
     * This procedure could be called either because the tableview window was
     * destroyed and the command was then deleted (in which case tkwin is
     * NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */
    if (viewPtr->tkwin != NULL) {
	Tk_Window tkwin;

	tkwin = viewPtr->tkwin;
	viewPtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

static void
GetCellGeometry(Cell *cellPtr)
{
    CellStyle *stylePtr;
    CellKey *keyPtr;
    TableView *viewPtr;

    viewPtr = cellPtr->viewPtr;
    keyPtr = GetKey(cellPtr);
    stylePtr = GetCurrentStyle(viewPtr, keyPtr->rowPtr, keyPtr->colPtr,cellPtr);
    (*stylePtr->classPtr->geomProc)(cellPtr, stylePtr);
}
    
static void
ComputeGeometry(TableView *viewPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    long i;
 
    viewPtr->flags &= ~GEOMETRY;        
    viewPtr->rowTitleWidth = viewPtr->colTitleHeight = 0;

    /* Step 1. Set the initial size of the row or column by computing its
     *         title size. Get the geometry of hidden rows and columns so
     *         that it doesn't cost to show/hide them. */
    for (i = 0; i < viewPtr->numColumns; i++) {
	Column *colPtr;

	colPtr = viewPtr->columns[i];
	if (colPtr->flags & GEOMETRY) {
	    if (viewPtr->flags & COLUMN_TITLES) {
		GetColumnTitleGeometry(viewPtr, colPtr);
	    } else {
		colPtr->titleWidth = colPtr->titleHeight = 0;
	    }
	}
	colPtr->nomWidth = colPtr->titleWidth;
	if ((colPtr->flags & HIDDEN) == 0) {
	    if (colPtr->titleHeight > viewPtr->colTitleHeight) {
		viewPtr->colTitleHeight = colPtr->titleHeight;
	    }
	}
    }
    for (i = 0; i < viewPtr->numRows; i++) {
	Row *rowPtr;

	rowPtr = viewPtr->rows[i];
	if (rowPtr->flags & GEOMETRY) {
	    if (viewPtr->flags & ROW_TITLES) {
		GetRowTitleGeometry(viewPtr, rowPtr);
	    } else {
		rowPtr->titleHeight = rowPtr->titleWidth = 0;
	    }
	}
	rowPtr->nomHeight = rowPtr->titleHeight;
	if ((rowPtr->flags & HIDDEN) == 0) {
	    if (rowPtr->titleWidth > viewPtr->rowTitleWidth) {
		viewPtr->rowTitleWidth = rowPtr->titleWidth;
	    }
	}
    }
    /* Step 2: Get the dimensions each cell (recomputing the geometry as
     *         needed) and compute the tallest/widest cell in each
     *         row/column. */
    for (hPtr = Blt_FirstHashEntry(&viewPtr->cellTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	CellKey *keyPtr;
	Row *rowPtr;
	Column *colPtr;
	Cell *cellPtr;
	
	keyPtr = (CellKey *)Blt_GetHashKey(&viewPtr->cellTable, hPtr);
	rowPtr = keyPtr->rowPtr;
	colPtr = keyPtr->colPtr;
	cellPtr = Blt_GetHashValue(hPtr);
	if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & GEOMETRY) {
	    GetCellGeometry(cellPtr);
	}
	if (cellPtr->width > colPtr->nomWidth) {
	    colPtr->nomWidth = cellPtr->width;
	}
	if (cellPtr->height > rowPtr->nomHeight) {
	    rowPtr->nomHeight = cellPtr->height;
	}
    }
    if (viewPtr->flags & AUTOFILTERS) {
	GetColumnFiltersGeometry(viewPtr);
    }
    viewPtr->flags |= LAYOUT_PENDING;
}

static void
ComputeLayout(TableView *viewPtr)
{
    unsigned long x, y;
    long i;

    viewPtr->flags &= ~LAYOUT_PENDING;
    x = y = 0;
    for (i = 0; i < viewPtr->numRows; i++) {
	Row *rowPtr;

	rowPtr = viewPtr->rows[i];
	rowPtr->flags &= ~GEOMETRY;	/* Always remove the geometry
                                         * flag. */
	rowPtr->index = i;		/* Reset the index. */
	rowPtr->height = rowPtr->nomHeight;
	if (rowPtr->reqHeight > 0) {
	    rowPtr->height = rowPtr->reqHeight;
	}
	rowPtr->worldY = y;
	if ((rowPtr->flags & HIDDEN) == 0) {
	    y += rowPtr->height;
	}
    }
    viewPtr->worldHeight = y;
#ifdef notdef
    if (viewPtr->worldHeight < VPORTHEIGHT(viewPtr)) {
	AdjustRows(viewPtr);
    }
#endif

    for (i = 0; i < viewPtr->numColumns; i++) {
	Column *colPtr;

	colPtr = viewPtr->columns[i];
	colPtr->flags &= ~GEOMETRY; 	/* Always remove the geometry
                                         * flag. */
	colPtr->index = i;		/* Reset the index. */
	colPtr->width = 0;
	colPtr->worldX = x;
	if (colPtr->reqWidth > 0) {
	    colPtr->width = colPtr->reqWidth;
	} else {
	    colPtr->width = colPtr->nomWidth;
	    if ((colPtr->reqMin > 0) && (colPtr->reqMin > colPtr->width)) {
		colPtr->width = colPtr->reqMin;
	    }
	    if ((colPtr->reqMax > 0) && (colPtr->reqMax < colPtr->width)) {
		colPtr->width = colPtr->reqMax;
	    }
	}
	if ((colPtr->flags & HIDDEN) == 0) {
	    x += colPtr->width;
	}
    }
    viewPtr->worldWidth = x;
    if (viewPtr->worldWidth < VPORTWIDTH(viewPtr)) {
	AdjustColumns(viewPtr);
    }
    viewPtr->height = viewPtr->worldHeight = y;
    viewPtr->width = viewPtr->worldWidth =  x;
    viewPtr->width += 2 * viewPtr->inset;
    viewPtr->height += 2 * viewPtr->inset;
    if (viewPtr->flags & COLUMN_TITLES) {
	viewPtr->height += viewPtr->colTitleHeight;
    }
    if (viewPtr->flags & AUTOFILTERS) {
	viewPtr->height += viewPtr->colFilterHeight;
    }
    if (viewPtr->flags & ROW_TITLES) {
	viewPtr->width += viewPtr->rowTitleWidth;
    }
    viewPtr->flags |= SCROLL_PENDING;	/* Flag to recompute visible rows
					 * and columns. */
}

static void
ComputeVisibleEntries(TableView *viewPtr)
{
    unsigned int  viewWidth, viewHeight;
    unsigned long xOffset, yOffset;
    long numVisibleRows, numVisibleColumns, i, j;
    long low, high;
    long first, last;

    xOffset = Blt_AdjustViewport(viewPtr->xOffset, viewPtr->worldWidth,
	VPORTWIDTH(viewPtr), viewPtr->xScrollUnits, viewPtr->scrollMode);
    yOffset = Blt_AdjustViewport(viewPtr->yOffset, 
	viewPtr->worldHeight, VPORTHEIGHT(viewPtr), viewPtr->yScrollUnits, 
	viewPtr->scrollMode);
    if ((viewPtr->numRows == 0) || (viewPtr->numColumns == 0)) {
	/*return;*/
    }
    if ((xOffset != viewPtr->xOffset) || (yOffset != viewPtr->yOffset)) {
	viewPtr->yOffset = yOffset;
	viewPtr->xOffset = xOffset;
    }
    viewWidth = VPORTWIDTH(viewPtr);
    viewHeight = VPORTHEIGHT(viewPtr);

    first = 0, last = -1;
    /* FIXME: Handle hidden rows. */
    /* Find the row that contains the start of the viewport.  */
    low = 0; high = viewPtr->numRows - 1;
    while (low <= high) {
	long mid;
	Row *rowPtr;
	
	mid = (low + high) >> 1;
	rowPtr = viewPtr->rows[mid];
	if (viewPtr->yOffset >
	    (rowPtr->worldY + rowPtr->height)) {
	    low = mid + 1;
	} else if (viewPtr->yOffset < rowPtr->worldY) {
	    high = mid - 1;
	} else {
	    first = mid;
	    break;
	}
    }
    numVisibleRows  = 0;
    /* Now look for the last row in the viewport. */
    for (i = first; i < viewPtr->numRows; i++) {
	Row *rowPtr;
	    
	rowPtr = viewPtr->rows[i];
	if (rowPtr->flags & HIDDEN) {
	    continue;
	}
	if (rowPtr->worldY >= (viewPtr->yOffset + viewHeight)) {
	    break;			/* Row starts after the end of the
					 * viewport. */
	}
	last = i + 1;
	numVisibleRows++;
    }
    if (numVisibleRows != viewPtr->numVisibleRows) {
	if (viewPtr->visibleRows != NULL) {
	    Blt_Free(viewPtr->visibleRows);
	}
	viewPtr->visibleRows = 
	    Blt_AssertCalloc(numVisibleRows + 1, sizeof(Row*));
	viewPtr->numVisibleRows = numVisibleRows;
    }
    if (viewPtr->numVisibleRows > 0) {
	for (j = 0, i = first; i < last; i++) {
	    Row *rowPtr;
	    
	    rowPtr = viewPtr->rows[i];
	    if ((rowPtr->flags & HIDDEN) == 0) {
		viewPtr->visibleRows[j] = rowPtr;
		j++;
		if (rowPtr->flags & REDRAW) {
		    rowPtr->flags &= ~REDRAW;
		    viewPtr->flags |= REDRAW | SCROLL_PENDING;
		}
	    }
	}
    }

    first = 0, last = -1;
    numVisibleColumns = 0;
    /* FIXME: Handle hidden columns. */
    /* Find the column that contains the start of the viewport.  */
    low = 0; high = viewPtr->numColumns - 1;
    while (low <= high) {
	long mid;
	Column *colPtr;
	
	mid = (low + high) >> 1;
	colPtr = viewPtr->columns[mid];
	if (viewPtr->xOffset > 
	    (colPtr->worldX + colPtr->width + colPtr->ruleWidth)) {
	    low = mid + 1;
	} else if (viewPtr->xOffset < colPtr->worldX) {
	    high = mid - 1;
	} else {
	    first = mid;
	    break;
	}
    }
    /* Now look for the last column in the viewport. */
    for (i = first; i < viewPtr->numColumns; i++) {
	Column *colPtr;
	
	colPtr = viewPtr->columns[i];
	if (colPtr->flags & HIDDEN) {
	    continue;
	}
	if ((colPtr->worldX) >= (viewPtr->xOffset + viewWidth)) {
	    break;			/* Column starts after the end of the
					 * viewport. */
	}
	last = i + 1;
	numVisibleColumns++;
    }
    if (numVisibleColumns != viewPtr->numVisibleColumns) {
	if (viewPtr->visibleColumns != NULL) {
	    Blt_Free(viewPtr->visibleColumns);
	}
	viewPtr->visibleColumns = 
	    Blt_AssertCalloc(numVisibleColumns + 1, sizeof(Row*));
	viewPtr->numVisibleColumns = numVisibleColumns;
    }
    if (viewPtr->numVisibleColumns > 0) {
	for (j = 0, i = first; i < last; i++) {
	    Column *colPtr;
	    
	    colPtr = viewPtr->columns[i];
	    if ((colPtr->flags & HIDDEN) == 0) {
		viewPtr->visibleColumns[j] = colPtr;
		j++;
		if (colPtr->flags & REDRAW) {
		    colPtr->flags &= ~REDRAW;
		    viewPtr->flags |= REDRAW | SCROLL_PENDING;
		}
	    }
	}
    }
    assert(viewPtr->numVisibleColumns <= viewPtr->numColumns);
}

static void
RebuildTableView(TableView *viewPtr)
{
    BLT_TABLE_COLUMN col;
    BLT_TABLE_ROW row;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    long i;
    Column **columns;
    Row **rows;
    unsigned long count, numRows, numColumns;
    Blt_Chain deleted;
    Blt_ChainLink link;

    /* 
     * Step 1:  Unmark rows and columns are in the table.
     */
    for (hPtr = Blt_FirstHashEntry(&viewPtr->columnTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Column *colPtr;

	colPtr = Blt_GetHashValue(hPtr);
	colPtr->flags |= DELETED;
    }
    for (hPtr = Blt_FirstHashEntry(&viewPtr->rowTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Row *rowPtr;

	rowPtr = Blt_GetHashValue(hPtr);
	rowPtr->flags |= DELETED;
    }

    /* 
     * Step 2:  Add and unmark rows and columns are in the table.
     */
    count = 0;
    numRows = blt_table_num_rows(viewPtr->table);
    rows = Blt_AssertMalloc(sizeof(Row *) * numRows);
    for (row = blt_table_first_row(viewPtr->table); row != NULL;  
	 row = blt_table_next_row(viewPtr->table, row)) {
	Blt_HashEntry *hPtr;
	int isNew;
	Row *rowPtr;
	    
	hPtr = Blt_CreateHashEntry(&viewPtr->rowTable, (char *)row, &isNew);
	if (isNew) {
	    rowPtr = CreateRow(viewPtr, row, hPtr);
	} else if (viewPtr->flags & AUTO_ROWS) {
	    rowPtr = Blt_GetHashValue(hPtr);
	} else {
	    continue;
	}
	rowPtr->flags &= ~DELETED;
	rows[count] = rowPtr;
	count++;
    }
    count = 0;
    numColumns = blt_table_num_columns(viewPtr->table);
    columns = Blt_AssertMalloc(sizeof(Column *) * numColumns);
    for (col = blt_table_first_column(viewPtr->table); col != NULL;  
	 col = blt_table_next_column(viewPtr->table, col)) {
	Blt_HashEntry *hPtr;
	int isNew;
	Column *colPtr;

	hPtr = Blt_CreateHashEntry(&viewPtr->columnTable, (char *)col, &isNew);
	if (isNew) {
	    colPtr = CreateColumn(viewPtr, col, hPtr);
	} else if (viewPtr->flags & AUTO_COLUMNS) {
	    colPtr = Blt_GetHashValue(hPtr);
	} else {
	    continue;
	}
	colPtr->flags &= ~DELETED;
	columns[count] = colPtr;
	count++;
    }
    /* 
     * Step 3:  Remove cells of rows and columns that were deleted.
     */
    deleted = Blt_Chain_Create();
    for (hPtr = Blt_FirstHashEntry(&viewPtr->cellTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Cell *cellPtr;
	CellKey *keyPtr;
	Row *rowPtr;
	Column *colPtr;

	/* Remove any cells that whose row and columns are no longer valid. */
	keyPtr = Blt_GetHashKey(&viewPtr->cellTable, hPtr);
	colPtr = keyPtr->colPtr;
	rowPtr = keyPtr->rowPtr;
	cellPtr = Blt_GetHashValue(hPtr);
	if ((rowPtr->flags | colPtr->flags) & DELETED) {
	    Blt_Chain_Append(deleted, cellPtr);
	}
    }
    for (link = Blt_Chain_FirstLink(deleted); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	Cell *cellPtr;
	
	cellPtr = Blt_Chain_GetValue(link);
	if (cellPtr->hashPtr != NULL) {
	    Blt_DeleteHashEntry(&viewPtr->cellTable, cellPtr->hashPtr);
	}
	DestroyCell(cellPtr);
    }
    Blt_Chain_Destroy(deleted);
    /* 
     * Step 4:  Remove rows and columns that were deleted.
     */
    for (i = 0; i < viewPtr->numRows; i++) {
	Row *rowPtr;

	rowPtr = viewPtr->rows[i];
	if (rowPtr->flags & DELETED) {
	    DestroyRow(rowPtr);
	}
    }
    for (i = 0; i < viewPtr->numColumns; i++) {
	Column *colPtr;

	colPtr = viewPtr->columns[i];
	if (colPtr->flags & DELETED) {
	    DestroyColumn(colPtr);
	}
    }
    if (viewPtr->columns != NULL) {
	Blt_Free(viewPtr->columns);
    }
    viewPtr->columns = columns;
    viewPtr->numColumns = numColumns;
    if (viewPtr->rows != NULL) {
	Blt_Free(viewPtr->rows);
    }
    viewPtr->numRows = numRows;
    viewPtr->rows = rows;
    /* Step 5. Create cells */
    for (i = 0; i < viewPtr->numRows; i++) {
	CellKey key;
	long j;

	key.rowPtr = viewPtr->rows[i];
	for (j = 0; j < viewPtr->numColumns; j++) {
	    Cell *cellPtr;
	    Blt_HashEntry *hPtr;
	    int isNew;

	    key.colPtr = viewPtr->columns[j];
	    hPtr = Blt_CreateHashEntry(&viewPtr->cellTable, (char *)&key, 
		&isNew);
	    if (isNew) {
		cellPtr = NewCell(viewPtr, hPtr);
		Blt_SetHashValue(hPtr, cellPtr);
	    }
	}
    }
    viewPtr->flags |= LAYOUT_PENDING | SCROLL_PENDING;
    EventuallyRedraw(viewPtr);
}


static void
AddCellGeometry(TableView *viewPtr, Cell *cellPtr)
{
    CellKey *keyPtr;
    Row *rowPtr;
    Column *colPtr;

    keyPtr = GetKey(cellPtr);
    rowPtr = keyPtr->rowPtr;
    colPtr = keyPtr->colPtr;
    GetCellGeometry(cellPtr);
    if (cellPtr->width > colPtr->nomWidth) {
	colPtr->nomWidth = cellPtr->width;
    }
    if (cellPtr->height > rowPtr->nomHeight) {
	rowPtr->nomHeight = cellPtr->height;
    }
}

static void
AddColumnGeometry(TableView *viewPtr, Column *colPtr)
{
    if (colPtr->flags & GEOMETRY) {
	if (viewPtr->flags & COLUMN_TITLES) {
	    GetColumnTitleGeometry(viewPtr, colPtr);
	} else {
	    colPtr->titleWidth = colPtr->titleHeight = 0;
	}
    }
    colPtr->nomWidth = colPtr->titleWidth;
    if ((colPtr->flags & HIDDEN) == 0) {
	if (colPtr->titleHeight > viewPtr->colTitleHeight) {
	    viewPtr->colTitleHeight = colPtr->titleHeight;
	}
    }
    if (viewPtr->flags & AUTOFILTERS) {
	GetColumnFiltersGeometry(viewPtr);
    }
}

static void
AddRowGeometry(TableView *viewPtr, Row *rowPtr)
{
    if (rowPtr->flags & GEOMETRY) {
	if (viewPtr->flags & ROW_TITLES) {
	    GetRowTitleGeometry(viewPtr, rowPtr);
	} else {
	    rowPtr->titleHeight = rowPtr->titleWidth = 0;
	}
    }
    rowPtr->nomHeight = rowPtr->titleHeight;
    if ((rowPtr->flags & HIDDEN) == 0) {
	if (rowPtr->titleWidth > viewPtr->rowTitleWidth) {
	    viewPtr->rowTitleWidth = rowPtr->titleWidth;
	}
    }
    if (viewPtr->flags & AUTOFILTERS) {
	GetColumnFiltersGeometry(viewPtr);
    }
}

static void
DeleteColumns(TableView *viewPtr, BLT_TABLE_NOTIFY_EVENT *eventPtr)
{
    long i, j, numColumns;
    BLT_TABLE_COLUMN col;
    Column **columns;

    /* Step 1: Mark all columns as to be deleted. */
    for (i = 0; i < viewPtr->numColumns; i++) {
	Column *colPtr;

	colPtr = viewPtr->columns[i];
	colPtr->flags |= DELETED;
    }
    /* Step 2: Unmark all the columns that still exist in the table. */
    numColumns = 0;
    for (col = blt_table_first_column(viewPtr->table); col != NULL;
	 col = blt_table_next_column(viewPtr->table, col)) {
	Blt_HashEntry *hPtr;
	Column *colPtr;

	hPtr = Blt_FindHashEntry(&viewPtr->columnTable, col);
	assert(hPtr != NULL);
	colPtr = Blt_GetHashValue(hPtr);
	colPtr->flags &= ~DELETED;
	numColumns++;
    }
    /* Step 3: Delete the marked columns, first removing all the associated
     * cells. */
    columns = Blt_AssertMalloc(sizeof(Column *) * numColumns);
    for (i = j = 0; i < viewPtr->numColumns; i++) {
	Column *colPtr;

	colPtr = viewPtr->columns[i];
	if (colPtr->flags & DELETED) {
	    colPtr->flags &= ~DELETED;
	    RemoveColumnCells(viewPtr, colPtr);
	    if (viewPtr->flags & AUTO_COLUMNS) {
		DestroyColumn(colPtr);
	    }
	} else {
	    columns[j++] = colPtr;
	}
    }
    if (viewPtr->columns != NULL) {
        Blt_Free(viewPtr->columns);
    }
    viewPtr->columns = columns;
    viewPtr->numColumns = numColumns;
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
}

static void
AddColumns(TableView *viewPtr, BLT_TABLE_NOTIFY_EVENT *eventPtr)
{
    Column **columns;
    long i;
    unsigned long count, oldNumColumns, newNumColumns;

    oldNumColumns = viewPtr->numColumns;
    newNumColumns = blt_table_num_columns(viewPtr->table);
    assert(newNumColumns > oldNumColumns);
    columns = Blt_AssertMalloc(sizeof(Column *) * newNumColumns);
    count = 0;
    for (i = 0; i < newNumColumns; i++) {
	Blt_HashEntry *hPtr;
	int isNew;
	Column *colPtr;
	BLT_TABLE_COLUMN col;

	col = blt_table_column(viewPtr->table, i);
	hPtr = Blt_CreateHashEntry(&viewPtr->columnTable, (char *)col, &isNew);
	if (isNew) {
	    CellKey key;
	    long j;
	    
	    colPtr = CreateColumn(viewPtr, col, hPtr);
	    AddColumnGeometry(viewPtr, colPtr);
	    key.colPtr = colPtr;
	    for (j = 0; j < viewPtr->numRows; j++) {
		Cell *cellPtr;
		Blt_HashEntry *h2Ptr;
		int isNew;

		key.rowPtr = viewPtr->rows[j];
		h2Ptr = Blt_CreateHashEntry(&viewPtr->cellTable, (char *)&key, 
					    &isNew);
		assert(isNew);
		cellPtr = NewCell(viewPtr, h2Ptr);
		AddCellGeometry(viewPtr, cellPtr);
		Blt_SetHashValue(h2Ptr, cellPtr);
	    }
	} else {
	    colPtr = Blt_GetHashValue(hPtr);
	}
	columns[count] = colPtr;
	count++;
    }
    if (viewPtr->columns != NULL) {
        Blt_Free(viewPtr->columns);
    }
    viewPtr->columns = columns;
    viewPtr->numColumns = newNumColumns;
    viewPtr->flags |= LAYOUT_PENDING;
    PossiblyRedraw(viewPtr);
}

static void
DeleteRows(TableView *viewPtr, BLT_TABLE_NOTIFY_EVENT *eventPtr)
{
    long i, j, numRows;
    BLT_TABLE_ROW row;
    Row **rows;

    /* Step 1: Mark all rows as to be deleted. */
    for (i = 0; i < viewPtr->numRows; i++) {
	Row *rowPtr;

	rowPtr = viewPtr->rows[i];
	rowPtr->flags |= DELETED;
    }
    /* Step 2: Unmark all the rows that still exists in the table. */
    numRows = 0;
    for (row = blt_table_first_row(viewPtr->table); row != NULL;
	 row = blt_table_next_row(viewPtr->table, row)) {
	Blt_HashEntry *hPtr;
	Row *rowPtr;

	hPtr = Blt_FindHashEntry(&viewPtr->rowTable, row);
	assert(hPtr != NULL);
	rowPtr = Blt_GetHashValue(hPtr);
	rowPtr->flags &= ~DELETED;
	numRows++;
    }
    /* Step 3: Delete the marked rows, first removing all the associated
     * cells. */
    rows = Blt_AssertMalloc(sizeof(Row *) * numRows);
    for (i = j = 0; i < viewPtr->numRows; i++) {
	Row *rowPtr;

	rowPtr = viewPtr->rows[i];
	if (rowPtr->flags & DELETED) {
	    rowPtr->flags &= ~DELETED;
	    RemoveRowCells(viewPtr, rowPtr);
	    if (viewPtr->flags & AUTO_ROWS) {
		DestroyRow(rowPtr);
	    }
	} else {
	    rows[j++] = rowPtr;
	}
    }
    if (viewPtr->rows != NULL) {
        Blt_Free(viewPtr->rows);
    }
    viewPtr->rows = rows;
    viewPtr->numRows = numRows;
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
}

static void
AddRows(TableView *viewPtr, BLT_TABLE_NOTIFY_EVENT *eventPtr)
{
    long i;
    Row **rows;
    unsigned long count, newNumRows, oldNumRows;

    oldNumRows = viewPtr->numRows;
    newNumRows = blt_table_num_rows(viewPtr->table);
    assert(newNumRows > oldNumRows);
    rows = Blt_AssertMalloc(sizeof(Row *) * newNumRows);
    count = 0;
    for (i = 0; i < newNumRows; i++) {
	Blt_HashEntry *hPtr;
	int isNew;
	Row *rowPtr;
	BLT_TABLE_ROW row;

	row = blt_table_row(viewPtr->table, i);
	hPtr = Blt_CreateHashEntry(&viewPtr->rowTable, (char *)row, &isNew);
	if (isNew) {
	    CellKey key;
	    long j;
	    
	    rowPtr = CreateRow(viewPtr, row, hPtr);
	    AddRowGeometry(viewPtr, rowPtr);
	    key.rowPtr = rowPtr;
	    for (j = 0; j < viewPtr->numColumns; j++) {
		Cell *cellPtr;
		Blt_HashEntry *h2Ptr;
		int isNew;

		key.colPtr = viewPtr->columns[j];
		h2Ptr = Blt_CreateHashEntry(&viewPtr->cellTable, (char *)&key, 
			&isNew);
		assert(isNew);
		cellPtr = NewCell(viewPtr, h2Ptr);
		AddCellGeometry(viewPtr, cellPtr);
		Blt_SetHashValue(h2Ptr, cellPtr);
	    }
	} else {
	    rowPtr = Blt_GetHashValue(hPtr);
	}
	rows[count] = rowPtr;
	count++;
    }
    if (viewPtr->rows != NULL) {
        Blt_Free(viewPtr->rows);
    }
    viewPtr->rows = rows;
    viewPtr->numRows = newNumRows;
    viewPtr->flags |= LAYOUT_PENDING;
    PossiblyRedraw(viewPtr);
}


static int
AttachTable(Tcl_Interp *interp, TableView *viewPtr)
{
    long i;
    unsigned int flags;

    /* Try to match the current rows and columns in the view with the new
     * table names. */

    ResetTableView(viewPtr);
    viewPtr->colNotifier = blt_table_create_column_notifier(interp, 
	viewPtr->table, NULL, TABLE_NOTIFY_ALL_EVENTS | TABLE_NOTIFY_WHENIDLE, 
	TableEventProc, NULL, viewPtr);
    viewPtr->rowNotifier = blt_table_create_row_notifier(interp, 
	viewPtr->table, NULL, TABLE_NOTIFY_ALL_EVENTS | TABLE_NOTIFY_WHENIDLE, 
	TableEventProc, NULL, viewPtr);
    viewPtr->numRows = viewPtr->numColumns = 0;
    /* Rows. */
    if (viewPtr->flags & AUTO_ROWS) {
	BLT_TABLE_ROW row;
	long i;

	viewPtr->numRows = blt_table_num_rows(viewPtr->table);
	viewPtr->rows = Blt_Malloc(viewPtr->numRows * sizeof(Row *));
	if (viewPtr->rows == NULL) {
	    return TCL_ERROR;
	}
	for (i = 0, row = blt_table_first_row(viewPtr->table); row != NULL;  
	     row = blt_table_next_row(viewPtr->table, row), i++) {
	    Blt_HashEntry *hPtr;
	    int isNew;
	    Row *rowPtr;
	    
	    hPtr = Blt_CreateHashEntry(&viewPtr->rowTable, (char *)row, &isNew);
	    assert(isNew);
	    rowPtr = CreateRow(viewPtr, row, hPtr);
	    viewPtr->rows[i] = rowPtr;
	}
	assert(i == viewPtr->numRows);
    }
    /* Columns. */
    if (viewPtr->flags & AUTO_COLUMNS) {
	BLT_TABLE_COLUMN col;
	long i;

	viewPtr->numColumns = blt_table_num_columns(viewPtr->table);
	viewPtr->columns = Blt_Malloc(viewPtr->numColumns *sizeof(Column *));
	if (viewPtr->columns == NULL) {
	    if (viewPtr->rows != NULL) {
		Blt_Free(viewPtr->rows);
		viewPtr->rows = NULL;
	    }
	    return TCL_ERROR;
	}
	for (i = 0, col = blt_table_first_column(viewPtr->table); col != NULL;  
	     col = blt_table_next_column(viewPtr->table, col), i++) {
	    Blt_HashEntry *hPtr;
	    int isNew;
	    Column *colPtr;
	    
	    hPtr = Blt_CreateHashEntry(&viewPtr->columnTable, (char *)col,
		&isNew);
	    assert(isNew);
	    colPtr = CreateColumn(viewPtr, col, hPtr);
	    viewPtr->columns[i] = colPtr;
	}
	assert(i == viewPtr->numColumns);
    }
    /* Create cells */
    for (i = 0; i < viewPtr->numRows; i++) {
	CellKey key;
	long j;
	
	key.rowPtr = viewPtr->rows[i];
	for (j = 0; j < viewPtr->numColumns; j++) {
	    Cell *cellPtr;
	    Blt_HashEntry *hPtr;
	    int isNew;
	    
	    key.colPtr = viewPtr->columns[j];
	    hPtr = Blt_CreateHashEntry(&viewPtr->cellTable, (char *)&key, 
		&isNew);
	    cellPtr = NewCell(viewPtr, hPtr);
	    Blt_SetHashValue(hPtr, cellPtr);
	}
    }
    flags = TABLE_TRACE_FOREIGN_ONLY | TABLE_TRACE_WRITES | TABLE_TRACE_UNSETS;
    blt_table_trace_row(viewPtr->table, TABLE_TRACE_ALL_ROWS, flags, 
	RowTraceProc, NULL, viewPtr);
    blt_table_trace_column(viewPtr->table, TABLE_TRACE_ALL_COLUMNS, flags, 
	ColumnTraceProc, NULL, viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayProc --
 *
 * 	This procedure is invoked to display the widget.
 *
 *      Recompute the layout of the text if necessary. This is necessary if
 *      the world coordinate system has changed.  Specifically, the
 *      following may have occurred:
 *
 *	  1.  a text attribute has changed (font, linespacing, etc.).
 *	  2.  an entry's option changed, possibly resizing the entry.
 *
 *      This is deferred to the display routine since potentially many of
 *      these may occur.
 *
 *	Set the vertical and horizontal scrollbars.  This is done here
 *	since the window width and height are needed for the scrollbar
 *	calculations.
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
DisplayProc(ClientData clientData)
{
    Pixmap drawable; 
    TableView *viewPtr = clientData;
    int reqWidth, reqHeight;
    long i;

    viewPtr->flags &= ~REDRAW_PENDING;
    if (viewPtr->tkwin == NULL) {
	return;				/* Window has been destroyed. */
    }
#ifdef notdef
    fprintf(stderr, "DisplayProc %s\n", Tk_PathName(viewPtr->tkwin));
#endif
    if (viewPtr->sort.flags & SORT_PENDING) {
	/* If the table needs resorting do it now before recalculating the
	 * geometry. */
	SortTableView(viewPtr);	
    }
    if (viewPtr->flags & GEOMETRY) {
	ComputeGeometry(viewPtr);
    }	
    if (viewPtr->flags & LAYOUT_PENDING) {
	ComputeLayout(viewPtr);
    }
    if (viewPtr->flags & SCROLL_PENDING) {
	int width, height;

	/* Scrolling means that the view port has changed or that the
	 * visible entries need to be recomputed. */
	width = VPORTWIDTH(viewPtr);
	height = VPORTHEIGHT(viewPtr);
	if ((viewPtr->flags & SCROLLX) && (viewPtr->xScrollCmdObjPtr != NULL)) {
	    /* Tell the x-scrollbar the new sizes. */
	    Blt_UpdateScrollbar(viewPtr->interp, viewPtr->xScrollCmdObjPtr, 
		viewPtr->xOffset, viewPtr->xOffset + width, 
		viewPtr->worldWidth);
	}
	if ((viewPtr->flags & SCROLLY) && (viewPtr->yScrollCmdObjPtr != NULL)) {
	    /* Tell the y-scrollbar the new sizes. */
	    Blt_UpdateScrollbar(viewPtr->interp, viewPtr->yScrollCmdObjPtr,
		viewPtr->yOffset, viewPtr->yOffset + height,
		viewPtr->worldHeight);
	}
	viewPtr->flags &= ~SCROLL_PENDING;
	/* Determine the visible rows and columns. The can happen when the
	 * -hide flags changes on a row or column. */
	ComputeVisibleEntries(viewPtr);
    }
    reqHeight = (viewPtr->reqHeight > 0) ? viewPtr->reqHeight : 
	viewPtr->worldHeight + viewPtr->colTitleHeight + 
	viewPtr->colFilterHeight + 2 * viewPtr->inset + 1;
    reqWidth = (viewPtr->reqWidth > 0) ? viewPtr->reqWidth : 
	viewPtr->worldWidth + viewPtr->rowTitleWidth + 2 * viewPtr->inset;

    if ((reqWidth != Tk_ReqWidth(viewPtr->tkwin)) || 
	(reqHeight != Tk_ReqHeight(viewPtr->tkwin))) {
	Tk_GeometryRequest(viewPtr->tkwin, reqWidth, reqHeight);
    }
    if (!Tk_IsMapped(viewPtr->tkwin)) {
	return;
    }
    if ((viewPtr->flags & REDRAW) == 0) {
	return;
    }
    viewPtr->flags &= ~REDRAW;
    Blt_PickCurrentItem(viewPtr->bindTable);
    if ((viewPtr->numVisibleRows == 0) || (viewPtr->numVisibleColumns == 0)){
	/* Empty table, draw blank area. */
	Blt_Bg_FillRectangle(viewPtr->tkwin, Tk_WindowId(viewPtr->tkwin), 
		viewPtr->bg, 0, 0, Tk_Width(viewPtr->tkwin), 
		Tk_Height(viewPtr->tkwin), viewPtr->borderWidth, 
		viewPtr->relief);
	DrawOuterBorders(viewPtr, Tk_WindowId(viewPtr->tkwin));
    }

    drawable = Blt_GetPixmap(viewPtr->display, Tk_WindowId(viewPtr->tkwin), 
	Tk_Width(viewPtr->tkwin), Tk_Height(viewPtr->tkwin), 
	Tk_Depth(viewPtr->tkwin));
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, 
	viewPtr->bg, 0, 0, Tk_Width(viewPtr->tkwin), 
	Tk_Height(viewPtr->tkwin), viewPtr->borderWidth, viewPtr->relief);

    if ((viewPtr->focusPtr == NULL) && (viewPtr->numVisibleRows > 0) &&
	(viewPtr->numVisibleColumns > 0)) {
	/* Re-establish the focus entry at the top entry. */
	Row *rowPtr;
	Column *colPtr;

	colPtr = GetFirstColumn(viewPtr);
	rowPtr = GetFirstRow(viewPtr);
	viewPtr->focusPtr = GetCell(viewPtr, rowPtr, colPtr);
    }
    /* Draw the cells. */
    for (i = 0; i < viewPtr->numVisibleRows; i++) {
	long j;
	Row *rowPtr;

	rowPtr = viewPtr->visibleRows[i];
	rowPtr->visibleIndex = i;
	/* Draw each cell in the row. */
	for (j = 0; j < viewPtr->numVisibleColumns; j++) {
	    Column *colPtr;
	    Cell *cellPtr;

	    colPtr = viewPtr->visibleColumns[j];
	    cellPtr = GetCell(viewPtr, rowPtr, colPtr);
	    DisplayCell(cellPtr, drawable, FALSE);
	}
    }
    if (viewPtr->flags & ROW_TITLES) {
	DisplayRowTitles(viewPtr, drawable);
    }
    if (viewPtr->flags & COLUMN_TITLES) {
	DisplayColumnTitles(viewPtr, drawable);
    }
    if ((viewPtr->flags & TITLES_MASK) == TITLES_MASK) {
	Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, 
		viewPtr->colNormalTitleBg, viewPtr->inset, viewPtr->inset, 
		viewPtr->rowTitleWidth, viewPtr->colTitleHeight, 
		viewPtr->colTitleBorderWidth, TK_RELIEF_RAISED);
	Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, 
		viewPtr->colNormalTitleBg, viewPtr->inset, 
		viewPtr->inset + viewPtr->colTitleHeight, 
		viewPtr->rowTitleWidth, viewPtr->colFilterHeight, 
		viewPtr->colTitleBorderWidth, TK_RELIEF_RAISED);
    }
    DrawOuterBorders(viewPtr, drawable);
    /* Now copy the new view to the window. */
    XCopyArea(viewPtr->display, drawable, Tk_WindowId(viewPtr->tkwin), 
	viewPtr->rowNormalTitleGC, 0, 0, Tk_Width(viewPtr->tkwin), 
	Tk_Height(viewPtr->tkwin), 0, 0);
    Tk_FreePixmap(viewPtr->display, drawable);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewTableView --
 *
 *---------------------------------------------------------------------------
 */
static TableView *
NewTableView(Tcl_Interp *interp, Tk_Window tkwin)
{
    TableView *viewPtr;

    Tk_SetClass(tkwin, "BltTableView");
    viewPtr = Blt_AssertCalloc(1, sizeof(TableView));
    viewPtr->tkwin = tkwin;
    viewPtr->display = Tk_Display(tkwin);
    viewPtr->interp = interp;
    viewPtr->flags = GEOMETRY | SCROLL_PENDING | LAYOUT_PENDING | AUTOCREATE;
    viewPtr->highlightWidth = 2;
    viewPtr->borderWidth = 2;
    viewPtr->relief = TK_RELIEF_SUNKEN;
    viewPtr->scrollMode = BLT_SCROLL_MODE_HIERBOX;
    viewPtr->xScrollUnits = viewPtr->yScrollUnits = 20;
    viewPtr->selectMode = SELECT_SINGLE_ROW;
    viewPtr->selectRows.list = Blt_Chain_Create();
    viewPtr->reqWidth = viewPtr->reqHeight = 400;
    viewPtr->colTitleBorderWidth = viewPtr->rowTitleBorderWidth = 2;
    viewPtr->filter.borderWidth = 1;
    viewPtr->filter.outerBorderWidth = 1;
    viewPtr->filter.relief = TK_RELIEF_SOLID;
    viewPtr->filter.selectRelief = TK_RELIEF_SUNKEN;
    viewPtr->filter.activeRelief = TK_RELIEF_RAISED;
    viewPtr->bindTable = Blt_CreateBindingTable(interp, tkwin, viewPtr, 
	TableViewPickProc, AppendTagsProc);
    Blt_InitHashTableWithPool(&viewPtr->cellTable, sizeof(CellKey)/sizeof(int));
    Blt_InitHashTableWithPool(&viewPtr->rowTable, BLT_ONE_WORD_KEYS);
    Blt_InitHashTableWithPool(&viewPtr->columnTable, BLT_ONE_WORD_KEYS);
    Blt_InitHashTable(&viewPtr->iconTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&viewPtr->styleTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&viewPtr->rowTagTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&viewPtr->colTagTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&viewPtr->cellTagTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&viewPtr->uidTable, BLT_STRING_KEYS);

    viewPtr->rowPool    = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    viewPtr->columnPool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    viewPtr->cellPool   = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    viewPtr->cmdToken = Tcl_CreateObjCommand(interp, Tk_PathName(tkwin), 
	TableViewInstObjCmdProc, viewPtr, TableViewInstCmdDeleteProc);

    Blt_SetWindowInstanceData(tkwin, viewPtr);
    Tk_CreateSelHandler(tkwin, XA_PRIMARY, XA_STRING, SelectionProc,
	viewPtr, XA_STRING);
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask |
	FocusChangeMask, TableViewEventProc, viewPtr);

    return viewPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TableViewCmdProc --
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
TableViewCmdProc(
    ClientData clientData,		/* Main window associated with
					 * interpreter. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument strings. */
{
    TableView *viewPtr;
    Tcl_Obj *cmdObjPtr, *objPtr;
    const char *string;
    int result;
    Tk_Window tkwin, mainWin;
    CellStyle *stylePtr;

    tkwin = NULL;
    if (objc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " pathName ?option value?...\"", 
		(char *)NULL);
	return TCL_ERROR;
    }
    /*
     * Invoke a procedure to initialize various bindings on tableview
     * entries.  If the procedure doesn't already exist, source it from
     * "$blt_library/tableview.tcl".  We deferred sourcing the file until
     * now so that the variable $blt_library could be set within a script.
     */
    if (!Blt_CommandExists(interp, "::blt::TableView::Initialize")) {
	if (Tcl_GlobalEval(interp, 
		"source [file join $blt_library tableview.tcl]") != TCL_OK) {
	    char info[200];

	    Blt_FormatString(info, 200, 
                             "\n    (while loading bindings for %.50s)", 
		    Tcl_GetString(objv[0]));
	    Tcl_AddErrorInfo(interp, info);
	    return TCL_ERROR;
	}
    }
    mainWin = Tk_MainWindow(interp);
    string = Tcl_GetString(objv[1]);
    tkwin = Tk_CreateWindowFromPath(interp, mainWin, string, (char *)NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    viewPtr = NewTableView(interp, tkwin);
    /* 
     * Initialize the widget's configuration options here. The options need
     * to be set first, so that entry, column, and style components can use
     * them for their own GCs.
     */
    iconOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    tableOption.clientData = viewPtr;
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, tableSpecs, 
	objc - 2, objv + 2, (char *)viewPtr, 0) != TCL_OK) {
	goto error;
    }
    /* 
     * Rebuild the widget's GC and other resources that are predicated by
     * the widget's configuration options.  Do the same for the default
     * column.
     */
    if (ConfigureTableView(interp, viewPtr) != TCL_OK) {
	goto error;
    }

    stylePtr = Blt_TableView_CreateCellStyle(interp, viewPtr, STYLE_TEXTBOX, 
	"default");
    if (stylePtr == NULL) {
	goto error;
    }
    viewPtr->stylePtr = stylePtr;
    iconOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(interp, tkwin, stylePtr->name,
	 stylePtr->classPtr->className, stylePtr->classPtr->specs, 0, NULL, 
	(char *)stylePtr, 0) != TCL_OK) {
	(*stylePtr->classPtr->freeProc)(stylePtr);
	goto error;
    }
    (*stylePtr->classPtr->configProc)(viewPtr, stylePtr);

    if (Blt_ConfigureComponentFromObj(interp, tkwin, "filter", "Filter", 
	filterSpecs, 0, NULL, (char *)viewPtr, 0) != TCL_OK) {
	goto error;
    }
    ConfigureFilters(interp, viewPtr);

    cmdObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    objPtr = Tcl_NewStringObj("::blt::TableView::Initialize", -1);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objv[1]);
    Tcl_IncrRefCount(cmdObjPtr);
    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(cmdObjPtr);
    if (result != TCL_OK) {
	goto error;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(viewPtr->tkwin), -1);
    return TCL_OK;

  error:
    Tk_DestroyWindow(tkwin);
    return TCL_ERROR;
}

int
Blt_TableViewCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {"tableview", TableViewCmdProc};

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}
#endif /* NO_TABLEVIEW */

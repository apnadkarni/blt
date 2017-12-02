/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTableView.c --
 *
 * This module implements an table viewer widget for the BLT toolkit.
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
 * TODO:
 *      o -autocreate rows|columns|both|none for new table
 *      x Focus ring on cells.
 *      x Rules for row/columns (-rulewidth > 0)
 *      o Dashes for row/column rules.
 *      o -span for rows/columns titles 
 *      o Printing PS/PDF 
 *      x Underline text for active cells.
 *      x Imagebox cells
 *      x Combobox cells
 *      x Combobox column filters
 *      x Text editor widget for cells
 *      o XCopyArea for scrolling non-damaged areas.
 *      o Manage scrollbars geometry.
 *      o -padx -pady for styles 
 *      o -titlefont, -titlecolor, -activetitlecolor, -disabledtitlecolor 
 *         for columns?
 *      o -justify period (1.23), comma (1,23), space (1.23 ev)
 *      o color icons for resize cursors
 */
/*
 * Known Bugs:
 *      o Row and column titles sometimes get drawn in the wrong location.
 *      o Don't handle hidden rows and columns in ComputeVisibility.
 *      o Too slow loading.  
 *      o Should be told what rows and columns are being added instead
 *        of checking. Make that work with -whenidle.
 *      o Overallocate row and column arrays so that you don't have to
 *        keep allocating a new array everytime.
 */
#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_TABLEVIEW

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif  /* HAVE_CTYPE_H */

#ifdef HAVE_STDLIB_H
  #include <stdlib.h> 
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_LIMITS_H
  #include <limits.h>
#endif  /* HAVE_LIMITS_H */

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

#define TITLE_PADX              2
#define TITLE_PADY              1
#define FILTER_GAP              3

#define COLUMN_PAD              2
#define FOCUS_WIDTH             1
#define ICON_PADX               2
#define ICON_PADY               1
#define INSET_PAD               0
#define LABEL_PADX              0
#define LABEL_PADY              0

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define RESIZE_AREA             (8)
#define FCLAMP(x)       ((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))
#define CHOOSE(default, override)       \
        (((override) == NULL) ? (default) : (override))

typedef ClientData (TagProc)(TableView *viewPtr, const char *string);

#define DEF_CELL_STATE                  "normal"
#define DEF_CELL_STYLE                  (char *)NULL

#define DEF_ACTIVE_TITLE_BG             STD_ACTIVE_BACKGROUND
#define DEF_ACTIVE_TITLE_FG             STD_ACTIVE_FOREGROUND
#define DEF_AUTO_CREATE                 "0"
#define DEF_COLUMN_FILTERS              "0"
#define DEF_BACKGROUND                  STD_NORMAL_BACKGROUND
#define DEF_BIND_TAGS                   "all"
#define DEF_BORDERWIDTH                 STD_BORDERWIDTH
#define DEF_COLUMN_ACTIVE_TITLE_RELIEF  "raised"
#define DEF_COLUMN_BIND_TAGS            "all"
#define DEF_COLUMN_COMMAND              (char *)NULL
#define DEF_COLUMN_EDIT                 "yes"
#define DEF_COLUMN_FORMAT_COMMAND       (char *)NULL
#define DEF_COLUMN_HIDE                 "no"
#define DEF_COLUMN_ICON                 (char *)NULL
#define DEF_COLUMN_MAX                  "0"             
#define DEF_COLUMN_MIN                  "0"
#define DEF_COLUMN_NORMAL_TITLE_BG      STD_NORMAL_BACKGROUND
#define DEF_COLUMN_NORMAL_TITLE_FG      STD_NORMAL_FOREGROUND
#define DEF_COLUMN_RESIZE_CURSOR        "arrow"
#define DEF_COLUMN_PAD                  "2"
#define DEF_COLUMN_SHOW                 "yes"
#define DEF_COLUMN_STATE                "normal"
#define DEF_COLUMN_STYLE                (char *)NULL
#define DEF_COLUMN_TITLE_BORDERWIDTH    STD_BORDERWIDTH
#define DEF_COLUMN_TITLE_FONT           STD_FONT_NORMAL
#define DEF_COLUMN_TITLE_JUSTIFY        "center"
#define DEF_COLUMN_TITLE_RELIEF         "raised"
#define DEF_COLUMN_WEIGHT               "1.0"
#define DEF_COLUMN_WIDTH                "0"
#define DEF_DISABLED_TITLE_BG           STD_DISABLED_BACKGROUND
#define DEF_DISABLED_TITLE_FG           STD_DISABLED_FOREGROUND
#define DEF_EXPORT_SELECTION            "no"
#define DEF_FILTER_ACTIVE_BG            STD_ACTIVE_BACKGROUND
#define DEF_FILTER_ACTIVE_FG            RGB_BLACK
#define DEF_FILTER_ACTIVE_RELIEF        "raised"
#define DEF_FILTER_BORDERWIDTH          "1"
#define DEF_FILTER_HIGHLIGHT            "0"
#define DEF_FILTER_DISABLED_BG          STD_DISABLED_BACKGROUND
#define DEF_FILTER_DISABLED_FG          STD_DISABLED_FOREGROUND
#define DEF_FILTER_HIGHLIGHT_BG         RGB_GREY97 /*"#FFFFDD"*/
#define DEF_FILTER_HIGHLIGHT_FG         STD_NORMAL_FOREGROUND
#define DEF_FILTER_FONT                 STD_FONT_NORMAL
#define DEF_FILTER_ICON                 (char *)NULL
#define DEF_FILTER_MENU                 (char *)NULL
#define DEF_FILTER_NORMAL_BG            RGB_WHITE
#define DEF_FILTER_NORMAL_FG            RGB_BLACK
#define DEF_FILTER_SELECT_BG            STD_SELECT_BACKGROUND
#define DEF_FILTER_SELECT_FG            STD_SELECT_FOREGROUND
#define DEF_FILTER_SELECT_RELIEF        "sunken"
#define DEF_FILTER_RELIEF               "solid"
#define DEF_FILTER_SHOW                 "yes"
#define DEF_FILTER_STATE                "normal"
#define DEF_FILTER_TEXT                 ""
#define DEF_FOCUS_HIGHLIGHT_BG          STD_NORMAL_BACKGROUND
#define DEF_FOCUS_HIGHLIGHT_COLOR       RGB_BLACK
#define DEF_FOCUS_HIGHLIGHT_WIDTH       "2"
#define DEF_HEIGHT                      "400"
#define DEF_RELIEF                      "sunken"
#define DEF_ROW_ACTIVE_TITLE_RELIEF     "raised"
#define DEF_ROW_BIND_TAGS               "all"
#define DEF_ROW_COMMAND                 (char *)NULL
#define DEF_ROW_EDIT                    "yes"
#define DEF_ROW_HEIGHT                  "0"
#define DEF_ROW_HIDE                    "no"
#define DEF_ROW_ICON                    (char *)NULL
#define DEF_ROW_MAX                     "0"             
#define DEF_ROW_MIN                     "0"
#define DEF_ROW_NORMAL_TITLE_BG         STD_NORMAL_BACKGROUND
#define DEF_ROW_NORMAL_TITLE_FG         STD_NORMAL_FOREGROUND
#define DEF_ROW_RESIZE_CURSOR           "arrow"
#define DEF_ROW_SHOW                    "yes"
#define DEF_ROW_STATE                   "normal"
#define DEF_ROW_STYLE                   (char *)NULL
#define DEF_ROW_TITLE_BORDERWIDTH       STD_BORDERWIDTH
#define DEF_ROW_TITLE_FONT              STD_FONT_NORMAL
#define DEF_ROW_TITLE_JUSTIFY           "center"
#define DEF_ROW_TITLE_RELIEF            "raised"
#define DEF_ROW_WEIGHT                  "1.0"
#define DEF_RULE_HEIGHT                 "0"
#define DEF_RULE_WIDTH                  "0"
#define DEF_SCROLL_INCREMENT            "20"
#define DEF_SCROLL_MODE                 "hierbox"
#define DEF_SELECT_MODE                 "singlerow"
#define DEF_SORT_COLUMN                 (char *)NULL
#define DEF_SORT_COLUMNS                (char *)NULL
#define DEF_SORT_COMMAND                (char *)NULL
#define DEF_SORT_DECREASING             "no"
#define DEF_SORT_DOWN_ICON              (char *)NULL
#define DEF_SORT_SELECTION              "no"
#define DEF_SORT_TYPE                   "auto"
#define DEF_SORT_UP_ICON                (char *)NULL
#define DEF_STYLE                       "default"
#define DEF_TABLE                       (char *)NULL
#define DEF_TAKE_FOCUS                  "1"
#define DEF_TITLES                      "column"
#define DEF_WIDTH                       "200"
#define DEF_MAX_COLUMN_WIDTH            "0"
#define DEF_MAX_ROW_HEIGHT              "0"

static const char *sortTypeStrings[] = {
    "dictionary", "ascii", "integer", "real", "command", "none", "auto", NULL
};

enum SortTypeValues { 
    SORT_DICTIONARY, SORT_ASCII, SORT_INTEGER, 
    SORT_REAL, SORT_COMMAND, SORT_NONE, SORT_AUTO
};

static Blt_OptionParseProc ObjToAutoCreate;
static Blt_OptionPrintProc AutoCreateToObj;
static Blt_CustomOption autoCreateOption = {
    ObjToAutoCreate, AutoCreateToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToColumnTitle;
static Blt_OptionPrintProc ColumnTitleToObj;
static Blt_OptionFreeProc FreeColumnTitleProc;
static Blt_CustomOption columnTitleOption = {
    ObjToColumnTitle, ColumnTitleToObj, FreeColumnTitleProc, 
    (ClientData)0
};

static Blt_OptionParseProc ObjToSortColumn;
static Blt_OptionPrintProc SortColumnToObj;
static Blt_CustomOption sortColumnOption = {
    ObjToSortColumn, SortColumnToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToSortOrder;
static Blt_OptionPrintProc SortOrderToObj;
static Blt_OptionFreeProc FreeSortOrderProc;
static Blt_CustomOption sortOrderOption = {
    ObjToSortOrder, SortOrderToObj, FreeSortOrderProc, (ClientData)0
};
static Blt_OptionParseProc ObjToEnum;
static Blt_OptionPrintProc EnumToObj;
static Blt_CustomOption typeOption = {
    ObjToEnum, EnumToObj, NULL, (ClientData)sortTypeStrings
};
static Blt_OptionParseProc ObjToIcon;
static Blt_OptionPrintProc IconToObj;
static Blt_OptionFreeProc FreeIconProc;
static Blt_CustomOption iconOption = {
    ObjToIcon, IconToObj, FreeIconProc, 
    (ClientData)0,                      /* Needs to point to the tableview
                                         * widget before calling
                                         * routines. */
};
static Blt_OptionParseProc ObjToRowTitle;
static Blt_OptionPrintProc RowTitleToObj;
static Blt_OptionFreeProc FreeRowTitleProc;
static Blt_CustomOption rowTitleOption = {
    ObjToRowTitle, RowTitleToObj, FreeRowTitleProc, (ClientData)0
};
static Blt_OptionParseProc ObjToScrollMode;
static Blt_OptionPrintProc ScrollModeToObj;
static Blt_CustomOption scrollModeOption = {
    ObjToScrollMode, ScrollModeToObj, NULL, NULL,
};

static Blt_OptionParseProc ObjToSelectMode;
static Blt_OptionPrintProc SelectModeToObj;
static Blt_CustomOption selectModeOption = {
    ObjToSelectMode, SelectModeToObj, NULL, NULL,
};

static Blt_OptionParseProc ObjToState;
static Blt_OptionPrintProc StateToObj;
static Blt_CustomOption stateOption = {
    ObjToState, StateToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToCellState;
static Blt_OptionPrintProc CellStateToObj;
static Blt_CustomOption cellStateOption = {
    ObjToCellState, CellStateToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToLimits;
static Blt_OptionPrintProc LimitsToObj;
static Blt_CustomOption limitsOption =
{
    ObjToLimits, LimitsToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToStyle;
static Blt_OptionPrintProc StyleToObj;
static Blt_OptionFreeProc FreeStyleProc;
static Blt_CustomOption styleOption = {
    ObjToStyle, StyleToObj, FreeStyleProc, 
    (ClientData)0,                      /* Needs to point to the tableview
                                         * widget before calling
                                         * routines. */
};

static Blt_OptionParseProc ObjToTable;
static Blt_OptionPrintProc TableToObj;
static Blt_OptionFreeProc FreeTableProc;
static Blt_CustomOption tableOption = {
    ObjToTable, TableToObj, FreeTableProc, NULL,
};

static Blt_OptionParseProc ObjToTitles;
static Blt_OptionPrintProc TitlesToObj;
static Blt_CustomOption titlesOption = {
    ObjToTitles, TitlesToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToCachedObj;
static Blt_OptionPrintProc CachedObjToObj;
static Blt_OptionFreeProc FreeCachedObjProc;
static Blt_CustomOption cachedObjOption = {
    ObjToCachedObj, CachedObjToObj, FreeCachedObjProc, NULL,
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
    {BLT_CONFIG_BITMASK, "-columnfilters", "columnFilters", "ColumnFilters",
        DEF_COLUMN_FILTERS, Blt_Offset(TableView, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)COLUMN_FILTERS},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background", 
        DEF_BACKGROUND, Blt_Offset(TableView, bg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth"},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
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
        "disabledColumnTitleBackground", "DisabledTitleBackground", 
        DEF_DISABLED_TITLE_BG, Blt_Offset(TableView, colDisabledTitleBg), 0},
    {BLT_CONFIG_COLOR, "-disabledcolumntitleforeground", 
        "disabledColumnTitleForeground", "DisabledTitleForeground", 
        DEF_DISABLED_TITLE_FG, Blt_Offset(TableView, colDisabledTitleFg), 0},
    {BLT_CONFIG_BACKGROUND, "-disabledrowtitlebackground", 
        "disabledRowTitleBackground", "DisabledTitleBackground", 
        DEF_DISABLED_TITLE_BG, Blt_Offset(TableView, rowDisabledTitleBg), 0},
    {BLT_CONFIG_COLOR, "-disabledrowtitleforeground", 
        "disabledRowTitleForeground", "DisabledTitleForeground", 
        DEF_DISABLED_TITLE_FG, Blt_Offset(TableView, rowDisabledTitleFg), 0},
    {BLT_CONFIG_BITMASK, "-exportselection", "exportSelection", 
        "ExportSelection", DEF_EXPORT_SELECTION, Blt_Offset(TableView, flags),
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SELECT_EXPORT},
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
    {BLT_CONFIG_CUSTOM, "-increasingicon", "increaingIcon", "IncreasingIcon", 
        DEF_SORT_UP_ICON, Blt_Offset(TableView, sort.up), 
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &iconOption},
    {BLT_CONFIG_PIXELS_NNEG, "-maxcolumnwidth", "maxColumnWidth", 
        "MaxColumnWidth", DEF_MAX_COLUMN_WIDTH, 
        Blt_Offset(TableView, maxColWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-maxrowheight", "maxRowHeight", "MaxRowHeight", 
        DEF_MAX_ROW_HEIGHT, Blt_Offset(TableView, maxRowHeight), 
        BLT_CONFIG_DONT_SET_DEFAULT},
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
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth"},
    {BLT_CONFIG_OBJ, "-bindtags", "bindTags", "BindTags", DEF_COLUMN_BIND_TAGS, 
        Blt_Offset(Column, bindTagsObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", DEF_COLUMN_COMMAND, 
        Blt_Offset(Column, cmdObjPtr),
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_BITMASK_INVERT, "-edit", "edit", "Edit", DEF_COLUMN_EDIT, 
        Blt_Offset(Column, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)EDIT},
    {BLT_CONFIG_OBJ, "-filterdata", "filterData", "FilterData", (char *)NULL,
        Blt_Offset(Column, filterDataObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FONT, "-filterfont", "filterFont", "FilterFont", 
        DEF_FILTER_FONT, Blt_Offset(Column, filterFont), 0},
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
    {BLT_CONFIG_CUSTOM, "-width", "width", "Width", DEF_COLUMN_WIDTH, 
        Blt_Offset(Column, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT, 
        &limitsOption},
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
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth"},
    {BLT_CONFIG_OBJ, "-bindtags", "bindTags", "BindTags", DEF_ROW_BIND_TAGS, 
        Blt_Offset(Row, bindTagsObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-command", "command", "Command", DEF_ROW_COMMAND, 
        Blt_Offset(Row, cmdObjPtr), 
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_BITMASK_INVERT, "-edit", "edit", "Edit", DEF_ROW_EDIT, 
        Blt_Offset(Row, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)EDIT},
    {BLT_CONFIG_CUSTOM, "-height", "height", "Height", DEF_ROW_HEIGHT, 
        Blt_Offset(Row, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT, 
        &limitsOption},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_ROW_HIDE, 
        Blt_Offset(Row, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "icon", DEF_ROW_ICON, 
        Blt_Offset(Row, icon), 
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &iconOption},
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

/* 
 * Some of the configuration options here are holders for TCL code to use.
 */
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
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth"},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth", 
        DEF_FILTER_BORDERWIDTH, Blt_Offset(TableView, filter.borderWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_BORDERWIDTH, Blt_Offset(TableView, filter.borderWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-disabledbackground", "disabledBackground", 
        "DisabledBackground", DEF_FILTER_DISABLED_BG, 
        Blt_Offset(TableView, filter.disabledBg), 0},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground", 
        "DisabledForeground", DEF_FILTER_DISABLED_FG, 
        Blt_Offset(TableView, filter.disabledFg), 0},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground"},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_FILTER_FONT, 
        Blt_Offset(TableView, filter.font), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", 
        DEF_FILTER_NORMAL_FG, Blt_Offset(TableView, filter.normalFg), 0},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide",
        DEF_COLUMN_FILTERS, Blt_Offset(TableView, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)COLUMN_FILTERS},
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
        (Blt_CustomOption *)COLUMN_FILTERS},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
        "SelectBackground", DEF_FILTER_SELECT_BG, 
        Blt_Offset(TableView, filter.selectBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", 
        "SelectForeground", DEF_FILTER_SELECT_FG, 
        Blt_Offset(TableView, filter.selectFg), 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
        0, 0}
};

typedef struct {
    unsigned int flags;
} BBoxSwitches;

#define BBOX_ROOT     (1<<0)

static Blt_SwitchSpec bboxSwitches[] = 
{
    {BLT_SWITCH_BITS_NOARG, "-root", "", (char *)NULL,
        Blt_Offset(BBoxSwitches, flags), 0, BBOX_ROOT},
    {BLT_SWITCH_END}
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
static void ReorderColumns(TableView *viewPtr);
static void ReorderRows(TableView *viewPtr);
static void AddRow(TableView *viewPtr, BLT_TABLE_ROW row);
static void AddColumn(TableView *viewPtr, BLT_TABLE_COLUMN col);
static void DeleteRow(TableView *viewPtr, BLT_TABLE_ROW row);
static void DeleteColumn(TableView *viewPtr, BLT_TABLE_COLUMN col);
static int InitColumnFilters(Tcl_Interp *interp, TableView *viewPtr);
static int ConfigureFilters(Tcl_Interp *interp, TableView *viewPtr);

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *      Queues a request to redraw the widget at the next idle point.  A
 *      new idle event procedure is queued only if the there's isn't one
 *      already queued and updates are turned on.
 *
 *      The DONT_UPDATE flag lets the user to turn off redrawing the
 *      tableview while changes are happening to the table itself.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Information gets redisplayed.  Right now we don't do selective
 *      redisplays:  the whole window will be redrawn.
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
 *      Queues a request to redraw the widget at the next idle point.  A
 *      new idle event procedure is queued only if the there's isn't one
 *      already queued and updates are turned on.
 *
 *      The DONT_UPDATE flag lets the user to turn off redrawing the
 *      tableview while changes are happening to the table itself.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Information gets redisplayed.  Right now we don't do selective
 *      redisplays:  the whole window will be redrawn.
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

static Tcl_Obj *
GetRowIndexObj(TableView *viewPtr, Row *rowPtr) 
{
    long index;

    index = blt_table_row_index(viewPtr->table, rowPtr->row);
    return Tcl_NewLongObj(index);
}

static Tcl_Obj *
GetColumnIndexObj(TableView *viewPtr, Column *colPtr) 
{
    long index;

    index = blt_table_column_index(viewPtr->table, colPtr->column);
    return Tcl_NewLongObj(index);
}

/*
 *---------------------------------------------------------------------------
 *
 * RethreadRows --
 *
 *      Rethreads the list of rows according to the current row map.  This
 *      is done after the rows are sorted and the list needs to reflect the
 *      reordering in the map.
 *
 *---------------------------------------------------------------------------
 */
static void
RethreadRows(TableView *viewPtr) 
{
    Row *prevPtr, *rowPtr;
    size_t i;
    
    /* Relink the first N-1 rows. */
    prevPtr = NULL;
    for (i = 0; i < (viewPtr->numRows - 1); i++) {
        Row *rowPtr;
        
        rowPtr = viewPtr->rowMap[i];
        rowPtr->index = i;
        rowPtr->prevPtr = prevPtr;
        rowPtr->nextPtr = viewPtr->rowMap[i+1];
        prevPtr = rowPtr;
    }
    /* Relink the last row. */
    rowPtr = viewPtr->rowMap[i];
    rowPtr->index = i;
    rowPtr->prevPtr = prevPtr;
    rowPtr->nextPtr = NULL;
    /* Reset the head and tail. */
    viewPtr->rowTailPtr = rowPtr;
    viewPtr->rowHeadPtr = viewPtr->rowMap[0];
}

/*
 *---------------------------------------------------------------------------
 *
 * RethreadColumns --
 *
 *      Rethreads the list of columns according to the current row map.  This
 *      is done after the columns are sorted and the list needs to reflect the
 *      reordering in the map.
 *
 *---------------------------------------------------------------------------
 */
static void
RethreadColumns(TableView *viewPtr) 
{
    Column *prevPtr, *colPtr;
    size_t i;
    
    /* Relink the first N-1 columns. */
    prevPtr = NULL;
    for (i = 0; i < (viewPtr->numColumns - 1); i++) {
        Column *colPtr;
        
        colPtr = viewPtr->columnMap[i];
        colPtr->index = i;
        colPtr->prevPtr = prevPtr;
        colPtr->nextPtr = viewPtr->columnMap[i+1];
        prevPtr = colPtr;
    }
    /* Relink the last column. */
    colPtr = viewPtr->columnMap[i];
    colPtr->index = i;
    colPtr->prevPtr = prevPtr;
    colPtr->nextPtr = NULL;
    /* Reset the head and tail. */
    viewPtr->colTailPtr = colPtr;
    viewPtr->colHeadPtr = viewPtr->columnMap[0];
}


/*
 *---------------------------------------------------------------------------
 *
 * RenumberRows --
 *
 *      Reindexes the rows according to their current location in the row
 *      list.  This is reflected in both the row map and in the index in
 *      the individual row structure.
 *
 *---------------------------------------------------------------------------
 */
static void
RenumberRows(TableView *viewPtr) 
{
    size_t i;
    Row *rowPtr;

    /* If the sizes are different reallocate the row map. */
    if (viewPtr->numMappedRows != viewPtr->numRows) {
        Row **map;

        map = Blt_AssertMalloc(viewPtr->numRows * sizeof(Row *));
        if (viewPtr->rowMap != NULL) {
            Blt_Free(viewPtr->rowMap);
        }
        viewPtr->rowMap = map;
        viewPtr->numMappedRows = viewPtr->numRows;
    } 
    /* Reset the row map and reindex the rows. */
    for (i = 0, rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL;
         rowPtr = rowPtr->nextPtr, i++) {
        rowPtr->index = i;
        viewPtr->rowMap[i] = rowPtr;
    }
    assert(i == viewPtr->numRows);
    viewPtr->flags &= ~REINDEX_ROWS;
}

/*
 *---------------------------------------------------------------------------
 *
 * RenumberColumns --
 *
 *      Reindexes the columns according to their current location in the
 *      column list.  This is reflected in both the column map and in the
 *      index in the individual column structure.
 *
 *---------------------------------------------------------------------------
 */
static void
RenumberColumns(TableView *viewPtr) 
{
    size_t i;
    Column *colPtr;

    /* If the sizes are different reallocate the column map. */
    if (viewPtr->numMappedColumns != viewPtr->numColumns) {
        Column **map;

        map = Blt_AssertMalloc(viewPtr->numColumns * sizeof(Column *));
        if (viewPtr->columnMap != NULL) {
            Blt_Free(viewPtr->columnMap);
        }
        viewPtr->columnMap = map;
        viewPtr->numMappedColumns = viewPtr->numColumns;
    } 
    /* Reset the column map and reindex the columns. */
    for (i = 0, colPtr = viewPtr->colHeadPtr; colPtr != NULL;
         colPtr = colPtr->nextPtr, i++) {
        colPtr->index = i;
        viewPtr->columnMap[i] = colPtr;
    }
    assert(i == viewPtr->numColumns);
    viewPtr->flags &= ~REINDEX_COLUMNS;
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

/*
 *---------------------------------------------------------------------------
 *
 * MoveRows --
 *
 *      Moves one or more row.
 *
 *---------------------------------------------------------------------------
 */
static void
MoveRows(TableView *viewPtr, Row *destPtr, Row *firstPtr, Row *lastPtr, 
         int after) 
{
    assert (firstPtr->index <= lastPtr->index);
    /* Unlink the sub-list from the list of rows. */
    if (viewPtr->rowHeadPtr == firstPtr) {
        viewPtr->rowHeadPtr = lastPtr->nextPtr;
        lastPtr->nextPtr->prevPtr = NULL;
    } else {
        firstPtr->prevPtr->nextPtr = lastPtr->nextPtr;
    }
    if (viewPtr->rowTailPtr == lastPtr) {
        viewPtr->rowTailPtr = firstPtr->prevPtr;
        firstPtr->prevPtr->nextPtr = NULL;
    } else {
        lastPtr->nextPtr->prevPtr = firstPtr->prevPtr;
    }
    firstPtr->prevPtr = lastPtr->nextPtr = NULL;

    /* Now attach the detached list to the destination. */
    if (after) { 
        /* [a]->[dest]->[b] */
        /*            [first]->[last] */
        if (destPtr->nextPtr == NULL) {
            assert(destPtr == viewPtr->rowTailPtr);
            viewPtr->rowTailPtr = lastPtr; /* Append to the end. */
        } else {
            destPtr->nextPtr->prevPtr = lastPtr;
        }
        lastPtr->nextPtr = destPtr->nextPtr;
        destPtr->nextPtr = firstPtr;
        firstPtr->prevPtr = destPtr;
    } else {
        /*           [a]->[dest]->[b] */
        /* [first]->[last] */
        if (destPtr->prevPtr == NULL) {
            viewPtr->rowHeadPtr = firstPtr;
        } else {
            destPtr->prevPtr->nextPtr = firstPtr;
        }
        firstPtr->prevPtr = destPtr->prevPtr;
        destPtr->prevPtr = lastPtr;
        lastPtr->nextPtr = destPtr;
    }
    /* FIXME: You don't have to reset the entire map. */
    RenumberRows(viewPtr);
    /* FIXME: Layout changes with move but not geometry. */
    viewPtr->flags |= GEOMETRY;
    EventuallyRedraw(viewPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * MoveColumns --
 *
 *      Moves one or more columns.
 *
 *---------------------------------------------------------------------------
 */
static void
MoveColumns(TableView *viewPtr, Column *destPtr, Column *firstPtr, 
            Column *lastPtr, int after) 
{
    assert (firstPtr->index <= lastPtr->index);
    /* Unlink the sub-list from the list of columns. */
    if (viewPtr->colHeadPtr == firstPtr) {
        viewPtr->colHeadPtr = lastPtr->nextPtr;
        lastPtr->nextPtr->prevPtr = NULL;
    } else {
        firstPtr->prevPtr->nextPtr = lastPtr->nextPtr;
    }
    if (viewPtr->colTailPtr == lastPtr) {
        viewPtr->colTailPtr = firstPtr->prevPtr;
        firstPtr->prevPtr->nextPtr = NULL;
    } else {
        lastPtr->nextPtr->prevPtr = firstPtr->prevPtr;
    }
    firstPtr->prevPtr = lastPtr->nextPtr = NULL;

    /* Now attach the detached list to the destination. */
    if (after) { 
        /* [a]->[dest]->[b] */
        /*            [first]->[last] */
        if (destPtr->nextPtr == NULL) {
            assert(destPtr == viewPtr->colTailPtr);
            viewPtr->colTailPtr = lastPtr; /* Append to the end. */
        } else {
            destPtr->nextPtr->prevPtr = lastPtr;
        }
        lastPtr->nextPtr = destPtr->nextPtr;
        destPtr->nextPtr = firstPtr;
        firstPtr->prevPtr = destPtr;
    } else {
        /*           [a]->[dest]->[b] */
        /* [first]->[last] */
        if (destPtr->prevPtr == NULL) {
            viewPtr->colHeadPtr = firstPtr;
        } else {
            destPtr->prevPtr->nextPtr = firstPtr;
        }
        firstPtr->prevPtr = destPtr->prevPtr;
        destPtr->prevPtr = lastPtr;
        lastPtr->nextPtr = destPtr;
    }
    /* FIXME: You don't have to reset the entire map. */
    RenumberColumns(viewPtr);
    /* FIXME: Layout changes with move but not geometry. */
    viewPtr->flags |= GEOMETRY;
    EventuallyRedraw(viewPtr);
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
    switch (viewPtr->selectMode) {
    case SELECT_CELLS:
        if (viewPtr->selectCells.cellTable.numEntries > 0) {
            Blt_DeleteHashTable(&viewPtr->selectCells.cellTable);
            Blt_InitHashTable(&viewPtr->selectCells.cellTable,
                              sizeof(CellKey)/sizeof(int));
        }
        break;
    case SELECT_SINGLE_ROW:
    case SELECT_MULTIPLE_ROWS:
        {
            Row *rowPtr;
            
            for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL; 
                 rowPtr = rowPtr->nextPtr) {
                rowPtr->flags &= ~SELECTED;
                rowPtr->link = NULL;
            }
            Blt_Chain_Reset(viewPtr->selectRows.list);
        }
        break;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetColumnXOffset --
 *
 *      Computes the closest X offset the the put the column in view.
 *      If the column is already in view the old X offset is returned.
 *
 * Results:
 *      Returns the X offset to view the column.
 *
 *---------------------------------------------------------------------------
 */
static long
GetColumnXOffset(TableView *viewPtr, Column *colPtr)
{
    long xOffset;

    xOffset = viewPtr->xOffset;
    if (colPtr->worldX < viewPtr->xOffset) {
        xOffset = colPtr->worldX;
    }
    if ((colPtr->worldX + colPtr->width) >= 
        (viewPtr->xOffset + VPORTWIDTH(viewPtr))) {
        xOffset = (colPtr->worldX + colPtr->width) - VPORTWIDTH(viewPtr);
    } 
    if (xOffset < 0) {
        xOffset = 0;
    }
    return xOffset;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetRowYOffset --
 *
 *      Computes the closest Y offset the the put the row in view.
 *      If the row is already in view the old Y offset is returned.
 *
 * Results:
 *      Returns the Y offset to view the row.
 *
 *---------------------------------------------------------------------------
 */
static long
GetRowYOffset(TableView *viewPtr, Row *rowPtr)
{
    long yOffset;

    yOffset = viewPtr->yOffset;
    if (rowPtr->worldY < viewPtr->yOffset) {
        yOffset = rowPtr->worldY;
    }
    if ((rowPtr->worldY + rowPtr->height) >= 
        (viewPtr->yOffset + VPORTHEIGHT(viewPtr))) {
        yOffset = (rowPtr->worldY + rowPtr->height) - VPORTHEIGHT(viewPtr);
    } 
    if (yOffset < 0) {
        yOffset = 0;
    }
    return yOffset;
}

static TableView *tableViewInstance;

static int
CompareRowValues(Column *colPtr, const Row *r1Ptr, const Row *r2Ptr)
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
            sortType = SORT_DICTIONARY; break;
        case TABLE_COLUMN_TYPE_LONG:
            sortType = SORT_INTEGER;    break;
        case TABLE_COLUMN_TYPE_TIME:
        case TABLE_COLUMN_TYPE_DOUBLE:
            sortType = SORT_REAL;       break;
        default:
            sortType = SORT_DICTIONARY; break;
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
        l1 = blt_table_get_long(NULL, viewPtr->table, r1, col, 0);
        l2 = blt_table_get_long(NULL, viewPtr->table, r2, col, 0);
        result = l1 - l2;
        break;
    case SORT_REAL:
        d1 = blt_table_get_double(NULL, viewPtr->table, r1, col);
        d2 = blt_table_get_double(NULL, viewPtr->table, r2, col);
        result = (d1 > d2) ? 1 : (d1 < d2) ? -1 : 0;
        break;
    }
    return result;
}


static int
CompareRowsWithCommand(TableView *viewPtr, Column *colPtr, Row *r1Ptr, 
                       Row *r2Ptr)
{
    Tcl_Interp *interp;
    Tcl_Obj *objPtr, *cmdObjPtr, *resultObjPtr;
    int result;

    interp = viewPtr->interp;
    cmdObjPtr = Tcl_DuplicateObj(colPtr->sortCmdObjPtr);
    /* Table name */
    objPtr = Tcl_NewStringObj(blt_table_name(viewPtr->table), -1);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    /* Row */
    objPtr = GetRowIndexObj(viewPtr, r1Ptr);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    /* Column */
    objPtr = GetColumnIndexObj(viewPtr, colPtr);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    /* Row */
    objPtr = GetRowIndexObj(viewPtr, r2Ptr);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    /* Column */
    objPtr = GetColumnIndexObj(viewPtr, colPtr);
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
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * CompareRows --
 *
 *      Comparison routine (used by qsort) to sort a chain of subnodes.
 *
 * Results:
 *      1 is the first is greater, -1 is the second is greater, 0
 *      if equal.
 *
 *---------------------------------------------------------------------------
 */
static int
CompareRows(const void *a, const void *b)
{
    Row *r1Ptr = *(Row **)a;
    Row *r2Ptr = *(Row **)b;
    TableView *viewPtr;
    int result;
    SortInfo *sortPtr;
    Blt_ChainLink link;

    viewPtr = tableViewInstance;
    sortPtr = &viewPtr->sort;
    result = 0;                         /* Suppress compiler warning. */
    for (link = Blt_Chain_FirstLink(sortPtr->order); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Column *colPtr;

        colPtr = Blt_Chain_GetValue(link);
        /* Fetch the data for sorting. */
        if ((colPtr->sortType == SORT_COMMAND) && 
            (colPtr->sortCmdObjPtr != NULL)) {
            result = CompareRowsWithCommand(viewPtr, colPtr, r1Ptr, r2Ptr);
        } else {
            result = CompareRowValues(colPtr, r1Ptr, r2Ptr);
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
 *      Sorts the flatten array of entries.
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
        Row *rowPtr;
        size_t i;

        if (sortPtr->decreasing == sortPtr->viewIsDecreasing) {
            return;
        }
        /* 
         * The view is already sorted but in the wrong direction.  Reverse
         * the entries in the array.
         */
        for (i = 0, rowPtr = viewPtr->rowTailPtr; rowPtr != NULL; 
             rowPtr = rowPtr->prevPtr, i++) {
            viewPtr->rowMap[i] = rowPtr;
        }
        sortPtr->viewIsDecreasing = sortPtr->decreasing;
    } else {
        qsort((char *)viewPtr->rowMap, viewPtr->numRows, sizeof(Row *),
              (QSortCompareProc *)CompareRows);
    }
    RethreadRows(viewPtr);
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
 * GetCachedObj --
 *
 *      Gets or creates a unique string identifier.  Strings are reference
 *      counted.  The string is placed into a hashed table local to the
 *      tableview.
 *
 * Results:
 *      Returns the pointer to the hashed string.
 *
 *---------------------------------------------------------------------------
 */
static Tcl_Obj *
GetCachedObj(TableView *viewPtr, Tcl_Obj *objPtr)
{
    Blt_HashEntry *hPtr;
    int isNew;
    const char *string;

    string = Tcl_GetString(objPtr);
    hPtr = Blt_CreateHashEntry(&viewPtr->cachedObjTable, string, &isNew);
    if (isNew) {
        Blt_SetHashValue(hPtr, objPtr);
    } else {
        objPtr = Blt_GetHashValue(hPtr);
    }
    Tcl_IncrRefCount(objPtr);
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeCachedObj --
 *
 *      Releases the cached Tcl_Obj.  Cached Tcl_Objs are reference
 *      counted, so only when the reference count is zero (i.e. no one else
 *      is using the string) is the entry removed from the hash table.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeCachedObj(TableView *viewPtr, Tcl_Obj *objPtr)
{
    Blt_HashEntry *hPtr;
    const char *string;

    string = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&viewPtr->cachedObjTable, string);
    assert(hPtr != NULL);
    if (objPtr->refCount <= 1) {
        Blt_DeleteHashEntry(&viewPtr->cachedObjTable, hPtr);
    }
    Tcl_DecrRefCount(objPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * IconChangedProc
 *
 * Results:
 *      None.
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
 * ObjToAutoCreate --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToAutoCreate(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
    *flagsPtr &= ~AUTOCREATE;
    *flagsPtr |= mask;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AutoCreateToObj --
 *
 *      Returns the current -autocreate value as a string.
 *
 * Results:
 *      The TCL string object is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
AutoCreateToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                     char *widgRec, int offset, int flags)      
{
    int mask = *(int *)(widgRec + offset);
    const char *string;

    mask &= AUTOCREATE;                 /* Only care about AUTO flags. */
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
 * ObjToColumnTitle --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColumnTitle(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
    if (length == 0) {                  /* Revert back to the row title */
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
 * ColumnTitleToObj --
 *
 *      Returns the current column title as a string.
 *
 * Results:
 *      The title is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ColumnTitleToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                     char *widgRec, int offset, int flags)      
{
    const char *string = *(char **)(widgRec + offset);

    return Tcl_NewStringObj(string, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSortColumn --
 *
 *      Converts the string, reprsenting a column, to its numeric form.
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
ObjToSortColumn(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
 * SortColumnToObj --
 *
 * Results:
 *      The string representation of the column is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SortColumnToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                    char *widgRec, int offset, int flags)       
{
    Column *colPtr = *(Column **)(widgRec + offset);

    if (colPtr == NULL) {
        return Tcl_NewStringObj("", -1);
    }
    return GetColumnIndexObj(colPtr->viewPtr, colPtr);
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
 * ObjToSortOrder --
 *
 *      Converts the string reprsenting a column, to its numeric form.
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
ObjToSortOrder(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
 * SortOrderToObj --
 *
 * Results:
 *      The string representation of the column is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SortOrderToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
        objPtr = GetColumnIndexObj(colPtr->viewPtr, colPtr);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToEnum --
 *
 *      Converts the string into its enumerated type.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToEnum(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
 * EnumToObj --
 *
 *      Returns the string associated with the enumerated type.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
EnumToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
 * ObjToIcon --
 *
 *      Convert the names of an icon into a Tk image.
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
ObjToIcon(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
 * IconToObj --
 *
 *      Converts the icon into its string representation (its name).
 *
 * Results:
 *      The name of the icon is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
IconToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
 * ObjToRowTitle --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToRowTitle(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
    if (length == 0) {                  /* Revert back to the row title */
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
 * RowTitleToObj --
 *
 *      Returns the current row title as a string.
 *
 * Results:
 *      The title is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
RowTitleToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                char *widgRec, int offset, int flags)   
{
    const char *string = *(char **)(widgRec + offset);

    return Tcl_NewStringObj(string, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToScrollMode --
 *
 *      Convert the string reprsenting a scroll mode, to its numeric form.
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
ObjToScrollMode(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
 * ScrollModeToObj --
 *
 * Results:
 *      The string representation of the scroll mode is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ScrollModeToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin, 
                    char *widgRec, int offset, int flags)       
{
    int mode = *(int *)(widgRec + offset);

    switch (mode) {
    case BLT_SCROLL_MODE_LISTBOX: 
        return Tcl_NewStringObj("listbox", 7);
    case BLT_SCROLL_MODE_HIERBOX: 
        return Tcl_NewStringObj("hierbox", 7);
    case BLT_SCROLL_MODE_CANVAS:  
        return Tcl_NewStringObj("canvas", 6);
    default:
        return Tcl_NewStringObj("???", 3);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSelectMode --
 *
 *      Convert the string reprsenting a scroll mode, to its numeric form.
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
ObjToSelectMode(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                Tcl_Obj *objPtr, char *widgRec, int offset, int flags)      
{
    const char *string;
    char c;
    int *modePtr = (int *)(widgRec + offset);
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 's') && (strncmp(string, "singlerow", length) == 0)) {
        *modePtr = SELECT_SINGLE_ROW;
    } else if ((c == 'm') && (strncmp(string, "multiplerows", length) == 0)) {
        *modePtr = SELECT_MULTIPLE_ROWS;
    } else if ((c == 'c') && (strncmp(string, "cells", length) == 0)) {
        *modePtr = SELECT_CELLS;
    } else {
        Tcl_AppendResult(interp, "bad select mode \"", string,
            "\": should be singlerow, multiplerows, or cells.",(char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectModeToObj --
 *
 * Results:
 *      The string representation of the select mode is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SelectModeToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                    char *widgRec, int offset, int flags)       
{
    int mode = *(int *)(widgRec + offset);

    switch (mode) {
    case SELECT_SINGLE_ROW:
        return Tcl_NewStringObj("singlerow", 9);
    case SELECT_MULTIPLE_ROWS:
        return Tcl_NewStringObj("multiplerows", 12);
    case SELECT_CELLS:
        return Tcl_NewStringObj("cells", 5);
    default:
        return Tcl_NewStringObj("???", 3);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToState --
 *
 *      Convert the name of a state into an integer.
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
ObjToState(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
 * StateToObj --
 *
 *      Converts the state into its string representation.
 *
 * Results:
 *      The name of the state is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
StateToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
 * ObjToCellState --
 *
 *      Converts the string representing a cell state into a bitflag.
 *
 * Results:
 *      The return value is a standard TCL result.  The state flags are
 *      updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToCellState(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
               Tcl_Obj *objPtr, char *widgRec, int offset, int flags)   
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
 * CellStateToObj --
 *
 *      Return the name of the style.
 *
 * Results:
 *      The name representing the style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
CellStateToObj(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Widget information record */
    int offset,                         /* Offset to field in structure */
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
 * ObjToStyle --
 *
 *      Convert the name of an icon into a tableview style.
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
ObjToStyle(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
        stylePtr->refCount++;           /* Increment the reference count to
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
 * StyleToObj --
 *
 *      Converts the style into its string representation (its name).
 *
 * Results:
 *      The name of the style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
StyleToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
 * ObjToTable --
 *
 *      Convert the string representing the name of a table object into a
 *      table token.
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
ObjToTable(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
 * TableToObj --
 *
 * Results:
 *      The string representation of the table is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TableToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin, 
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
 * ObjToTitles --
 *
 *      Converts the string to a titles flag: ROW_TITLES or COLUMN_TITLES. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTitles(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
 * TitlesToObj --
 *
 *      Returns the titles flags as a string.
 *
 * Results:
 *      The fill style string is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TitlesToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                char *widgRec, int offset, int flags)   
{
    int titles = *(int *)(widgRec + offset);
    const char *string;

    string = NULL;                      /* Suppress compiler warning. */
    switch (titles & TITLES_MASK) {
    case ROW_TITLES:
        string = "rows";        break;
    case COLUMN_TITLES:
        string = "columns";     break;
    case 0:
        string = "none";        break;
    case (ROW_TITLES|COLUMN_TITLES):
        string = "both";        break;
    }
    return Tcl_NewStringObj(string, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToCachedObj --
 *
 *      Converts the string to a cached Tcl_Obj. Cacheded Tcl_Obj's are
 *      hashed, reference counted strings.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToCachedObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
               Tcl_Obj *objPtr, char *widgRec, int offset, int flags)     
{
    TableView *viewPtr = clientData;
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);

    *objPtrPtr = GetCachedObj(viewPtr, objPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CachedObjToObj --
 *
 *      Returns the cached Tcl_Obj.
 *
 * Results:
 *      The fill style string is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
CachedObjToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
               char *widgRec, int offset, int flags)      
{
    Tcl_Obj *objPtr = *(Tcl_Obj **)(widgRec + offset);

    if (objPtr == NULL) {
        return Tcl_NewStringObj("", -1);
    }
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeCachedObjProc --
 *
 *      Free the cached obj from the widget record, setting it to NULL.
 *
 * Results:
 *      The CACHEDOBJ in the widget record is set to NULL.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
FreeCachedObjProc(ClientData clientData, Display *display, char *widgRec,
                  int offset)
{
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);

    if (*objPtrPtr != NULL) {
        TableView *viewPtr = clientData;

        FreeCachedObj(viewPtr, *objPtrPtr);
        *objPtrPtr = NULL;
    }
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
GetBoundedWidth(int w, Limits *limitsPtr)               
{
    if (limitsPtr->flags & LIMITS_SET_NOM) {
        w = limitsPtr->nom;         /* Override initial value */
    }
    if (w < limitsPtr->min) {
        w = limitsPtr->min;         /* Bounded by minimum value */
    } else if (w > limitsPtr->max) {
        w = limitsPtr->max;         /* Bounded by maximum value */
    }
    return w;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetBoundedHeight --
 *
 *      Bounds a given value to the limits described in the limit structure.
 *      The initial starting value may be overridden by the nominal value in
 *      the limits.
 *
 * Results:
 *      Returns the constrained value.
 *
 *---------------------------------------------------------------------------
 */
static int
GetBoundedHeight(int h, Limits *limitsPtr)
{
    if (limitsPtr->flags & LIMITS_SET_NOM) {
        h = limitsPtr->nom;             /* Override initial value */
    }
    if (h < limitsPtr->min) {
        h = limitsPtr->min;             /* Bounded by minimum value */
    } else if (h > limitsPtr->max) {
        h = limitsPtr->max;             /* Bounded by maximum value */
    }
    return h;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLimits --
 *
 *      Converts the list of elements into zero or more pixel values which
 *      determine the range of pixel values possible.  An element can be in
 *      any form accepted by Tk_GetPixels. The list has a different meaning
 *      based upon the number of elements.
 *
 *          # of elements:
 *
 *          0 - the limits are reset to the defaults.
 *          1 - the minimum and maximum values are set to this
 *              value, freezing the range at a single value.
 *          2 - first element is the minimum, the second is the
 *              maximum.
 *          3 - first element is the minimum, the second is the
 *              maximum, and the third is the nominal value.
 *
 *      Any element may be the empty string which indicates the default.
 *
 * Results:
 *      The return value is a standard TCL result.  The min and max fields
 *      of the range are set.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToLimits(
    ClientData clientData,      /* Not used. */
    Tcl_Interp *interp,         /* Interpreter to send results back to */
    Tk_Window tkwin,            /* Widget of table */
    Tcl_Obj *objPtr,            /* New width list */
    char *widgRec,              /* Widget record */
    int offset,                 /* Offset to field in structure */
    int flags)  
{
    Limits *limitsPtr = (Limits *)(widgRec + offset);
    Tcl_Obj **elv;
    int elc;
    int limits[3];
    int limitsFlags;

    elv = NULL;
    elc = 0;

    /* Initialize limits to default values */
    limits[2] = LIMITS_NOM;
    limits[1] = LIMITS_MAX;
    limits[0] = LIMITS_MIN;
    limitsFlags = 0;

    if (objPtr != NULL) {
        int i;

        if (Tcl_ListObjGetElements(interp, objPtr, &elc, &elv) != TCL_OK) {
            return TCL_ERROR;
        }
        if (elc > 3) {
            Tcl_AppendResult(interp, "wrong # limits \"", 
                        Tcl_GetString(objPtr), "\"", (char *)NULL);
            return TCL_ERROR;
        }
        for (i = 0; i < elc; i++) {
            int length, size;

            Tcl_GetStringFromObj(elv[i], &length);
            if (length == 0) {
                continue;             /* Empty string: use default value */
            }
            limitsFlags |= (LIMITS_SET_BIT << i);
            if (Tk_GetPixelsFromObj(interp, tkwin, elv[i], &size) != TCL_OK) {
                return TCL_ERROR;
            }
            if ((size < LIMITS_MIN) || (size > LIMITS_MAX)) {
                Tcl_AppendResult(interp, "bad limits \"", 
                        Tcl_GetString(objPtr), "\"", (char *)NULL);
                return TCL_ERROR;
            }
            limits[i] = size;
        }
    }
    /*
    * Check the limits specified.  We can't check the requested
    * size of widgets.
    */
    switch (elc) {
    case 1:
        limitsFlags |= (LIMITS_SET_MIN | LIMITS_SET_MAX);
        limits[1] = limits[0];       /* Set minimum and maximum to value */
        break;

    case 2:
        if (limits[1] < limits[0]) {
            Tcl_AppendResult(interp, "bad range \"", Tcl_GetString(objPtr),
                "\": min > max", (char *)NULL);
            return TCL_ERROR;         /* Minimum is greater than maximum */
        }
        break;
    case 3:
        if (limits[1] < limits[0]) {
            Tcl_AppendResult(interp, "bad range \"", Tcl_GetString(objPtr),
                             "\": min > max", (char *)NULL);
            return TCL_ERROR;         /* Minimum is greater than maximum */
        }
        if ((limits[2] < limits[0]) || (limits[2] > limits[1])) {
            Tcl_AppendResult(interp, "nominal value \"", 
                             Tcl_GetString(objPtr),
                             "\" out of range", (char *)NULL);
            return TCL_ERROR;        /* Nominal is outside of range defined
                                      * by minimum and maximum */
        }
        break;
    }
    limitsPtr->min = limits[0];
    limitsPtr->max = limits[1];
    limitsPtr->nom = limits[2];
    limitsPtr->flags = limitsFlags;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetLimits --
 *
 *      Resets the limits to their default values.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
INLINE static void
ResetLimits(Limits *limitsPtr)  /* Limits to be imposed on the value */
{
    limitsPtr->flags = 0;
    limitsPtr->min = LIMITS_MIN;
    limitsPtr->max = LIMITS_MAX;
    limitsPtr->nom = LIMITS_NOM;
}

/*
 *---------------------------------------------------------------------------
 *
 * NameOfLimits --
 *
 *      Convert the values into a list representing the limits.
 *
 * Results:
 *      The static string representation of the limits is returned.
 *
 *---------------------------------------------------------------------------
 */
static Tcl_Obj *
NameOfLimits(Tcl_Interp *interp, Limits *limitsPtr)
{
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    objPtr = (limitsPtr->flags & LIMITS_SET_MIN) ? 
        Tcl_NewIntObj(limitsPtr->min) : Tcl_NewStringObj("", 0);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = (limitsPtr->flags & LIMITS_SET_MAX) ? 
        Tcl_NewIntObj(limitsPtr->max) : Tcl_NewStringObj("", 0);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = (limitsPtr->flags & LIMITS_SET_NOM) ? 
        Tcl_NewIntObj(limitsPtr->nom) : Tcl_NewStringObj("", 0);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * LimitsToObj --
 *
 *      Convert the limits of the pixel values allowed into a list.
 *
 * Results:
 *      The string representation of the limits is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
LimitsToObj(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Not used. */
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Row/column structure record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Limits *limitsPtr = (Limits *)(widgRec + offset);

    return NameOfLimits(interp, limitsPtr);
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
 *      Returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowTraceProc(ClientData clientData, BLT_TABLE_TRACE_EVENT *eventPtr)
{
    TableView *viewPtr = clientData; 

    if (eventPtr->mask & (TABLE_TRACE_WRITES | TABLE_TRACE_UNSETS)) {
        Column *colPtr;
        Row *rowPtr;
        long row, col;

        colPtr = GetColumnContainer(viewPtr, eventPtr->column);
        rowPtr = GetRowContainer(viewPtr, eventPtr->row);
        row = col = -1;
        if (colPtr != NULL) {
            col = colPtr->index;
        }
        if (rowPtr != NULL) {
            rowPtr->flags |= GEOMETRY | REDRAW;
            row = rowPtr->index;
        }
        viewPtr->flags |= GEOMETRY | LAYOUT_PENDING;
        /* Check if the event's row or column occur outside of the range of
         * visible cells. */
        if ((row > GetLastVisibleRowIndex(viewPtr)) ||
            (col > GetLastVisibleColumnIndex(viewPtr))) {
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
 *      Returns TCL_OK.
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
        Row *rowPtr;
        long row, col;

        colPtr = GetColumnContainer(viewPtr, eventPtr->column);
        rowPtr = GetRowContainer(viewPtr, eventPtr->row);
        row = col = -1;
        if (colPtr != NULL) {
            colPtr->flags |= GEOMETRY | REDRAW;
            col = colPtr->index;
        }
        if (rowPtr != NULL) {
            row = rowPtr->index;
        }
        viewPtr->flags |= GEOMETRY | LAYOUT_PENDING;
        /* Check if the event's row or column occur outside of the range of
         * visible cells. */
        if ((row > GetLastVisibleRowIndex(viewPtr)) ||
            (col > GetLastVisibleColumnIndex(viewPtr))) {
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
    Blt_DeleteBindings(viewPtr->bindTable, cellPtr);
    if (cellPtr == viewPtr->focusPtr) {
        viewPtr->focusPtr = NULL;
        Blt_SetFocusItem(viewPtr->bindTable, viewPtr->focusPtr, 
                         (ClientData)ITEM_CELL);
    }
    keyPtr = GetKey(cellPtr);
    if (cellPtr->stylePtr != NULL) {
        Blt_HashEntry *hPtr;

        cellPtr->stylePtr->refCount--;
        /* Remove the cell from the style's cell table. */
        hPtr = Blt_FindHashEntry(&cellPtr->stylePtr->table, (char *)keyPtr);
        if (hPtr != NULL) {
            Blt_DeleteHashEntry(&cellPtr->stylePtr->table, hPtr);
        }
        if (cellPtr->stylePtr->refCount <= 0) {
            (*cellPtr->stylePtr->classPtr->freeProc)(cellPtr->stylePtr);
        }
    }
    ClearSelections(viewPtr);
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
    Column *colPtr;

    /* For each column remove the row, column combination in the table. */
    key.rowPtr = rowPtr;
    for (colPtr = viewPtr->colHeadPtr; colPtr != NULL; 
         colPtr = colPtr->nextPtr) {
        Blt_HashEntry *hPtr;

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
    Row *rowPtr;

    /* For each row remove the row,column combination in the table. */
    key.colPtr = colPtr;
    for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL; 
         rowPtr = rowPtr->nextPtr) {
        Blt_HashEntry *hPtr;

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
    cachedObjOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    iconOption.clientData = viewPtr;
    Blt_DeleteBindings(viewPtr->bindTable, rowPtr);
    Blt_FreeOptions(rowSpecs, (char *)rowPtr, viewPtr->display, 0);
    if (rowPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&viewPtr->rowTable, rowPtr->hashPtr);
    }
    blt_table_clear_row_traces(viewPtr->table, rowPtr->row);
    if ((rowPtr->flags & DELETED) == 0) {
        RemoveRowCells(viewPtr, rowPtr);
    }
    if (viewPtr->rowHeadPtr == rowPtr) {
        viewPtr->rowHeadPtr = rowPtr->nextPtr;
    }
    if (viewPtr->rowTailPtr == rowPtr) {
        viewPtr->rowTailPtr = rowPtr->prevPtr;
    }
    if (rowPtr->nextPtr != NULL) {
        rowPtr->nextPtr->prevPtr = rowPtr->prevPtr;
    }
    if (rowPtr->prevPtr != NULL) {
        rowPtr->prevPtr->nextPtr = rowPtr->nextPtr;
    }
    rowPtr->prevPtr = rowPtr->nextPtr = NULL;
    viewPtr->numRows--;
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
    rowPtr->index = viewPtr->numRows;
    ResetLimits(&rowPtr->reqHeight);
    Blt_SetHashValue(hPtr, rowPtr);
    if (viewPtr->rowHeadPtr == NULL) {
        viewPtr->rowTailPtr = viewPtr->rowHeadPtr = rowPtr;
    } else {
        rowPtr->prevPtr = viewPtr->rowTailPtr;
        if (viewPtr->rowTailPtr != NULL) {
            viewPtr->rowTailPtr->nextPtr = rowPtr;
        }
        viewPtr->rowTailPtr = rowPtr;
    }
    viewPtr->numRows++;
    return rowPtr;
}

static Row *
CreateRow(TableView *viewPtr, BLT_TABLE_ROW row, Blt_HashEntry *hPtr)
{
    Row *rowPtr;

    rowPtr = NewRow(viewPtr, row, hPtr);
    iconOption.clientData = viewPtr;
    cachedObjOption.clientData = viewPtr;
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
    cachedObjOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    iconOption.clientData = viewPtr;
    Blt_DeleteBindings(viewPtr->bindTable, colPtr);
    Blt_FreeOptions(columnSpecs, (char *)colPtr, viewPtr->display, 0);
    if (colPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&viewPtr->columnTable, colPtr->hashPtr);
    }
    if (colPtr->column != NULL) {
        blt_table_clear_column_traces(viewPtr->table, colPtr->column);
    }
    if ((colPtr->flags & DELETED) == 0) {
        RemoveColumnCells(viewPtr, colPtr);
    }
    if (viewPtr->colHeadPtr == colPtr) {
        viewPtr->colHeadPtr = colPtr->nextPtr;
    }
    if (viewPtr->colTailPtr == colPtr) {
        viewPtr->colTailPtr = colPtr->prevPtr;
    }
    if (colPtr->nextPtr != NULL) {
        colPtr->nextPtr->prevPtr = colPtr->prevPtr;
    }
    if (colPtr->prevPtr != NULL) {
        colPtr->prevPtr->nextPtr = colPtr->nextPtr;
    }
    colPtr->prevPtr = colPtr->nextPtr = NULL;
    viewPtr->numColumns--;
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
    colPtr->index = viewPtr->numColumns;
    Blt_SetHashValue(hPtr, colPtr);
    ResetLimits(&colPtr->reqWidth);
    if (viewPtr->colHeadPtr == NULL) {
        viewPtr->colTailPtr = viewPtr->colHeadPtr = colPtr;
    } else {
        colPtr->prevPtr = viewPtr->colTailPtr;
        if (viewPtr->colTailPtr != NULL) {
            viewPtr->colTailPtr->nextPtr = colPtr;
        }
        viewPtr->colTailPtr = colPtr;
    }
    viewPtr->numColumns++;
    return colPtr;
}

static void
ComputeColumnTitleGeometry(TableView *viewPtr, Column *colPtr)
{
    unsigned int aw, ah, iw, ih, tw, th;

    colPtr->titleWidth  = 2 * (viewPtr->colTitleBorderWidth + TITLE_PADX);
    colPtr->titleHeight = 2 * (viewPtr->colTitleBorderWidth + TITLE_PADY);
    colPtr->textHeight = colPtr->textWidth = 0;
    aw = ah = tw = th = iw = ih = 0;
    if (colPtr->icon != NULL) {
        iw = IconWidth(colPtr->icon);
        ih = IconHeight(colPtr->icon);
        colPtr->titleWidth += iw;
    }
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
    if ((viewPtr->sort.up != NULL) && (viewPtr->sort.down != NULL)) {
        aw = MAX(IconWidth(viewPtr->sort.up), IconWidth(viewPtr->sort.down));
        ah = MAX(IconHeight(viewPtr->sort.up), IconHeight(viewPtr->sort.down));
    } else {
        Blt_FontMetrics fm;

        Blt_Font_GetMetrics(viewPtr->colTitleFont, &fm);
        ah = fm.linespace;
        aw = colPtr->textHeight * 60 / 100;
    }
    colPtr->titleWidth  += aw + TITLE_PADX;
    colPtr->titleHeight += MAX3(ih, th, ah);
}


/*
 * InitColumnFilters -- 
 *
 *      Called by ConfigureTableView routine to initialize the column
 *      filters menu used by all columns.  This calls TCL code to 
 *      create the column filter menu and scrollbars.
 */
static int
InitColumnFilters(Tcl_Interp *interp, TableView *viewPtr)
{
    int result;
    Tcl_Obj *cmdObjPtr, *objPtr;

    if ((viewPtr->flags & COLUMN_FILTERS) == 0) {
        return TCL_OK;
    }
    if (!Blt_CommandExists(interp,"::blt::TableView::InitColumnFilters")) {
        return TCL_OK;
    }
    cmdObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    objPtr = Tcl_NewStringObj("::blt::TableView::InitColumnFilters", -1);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    objPtr = Tcl_NewStringObj(Tk_PathName(viewPtr->tkwin), -1);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    Tcl_IncrRefCount(cmdObjPtr);
    Tcl_Preserve(viewPtr);
    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_Release(viewPtr);
    Tcl_DecrRefCount(cmdObjPtr);
    return result;
}

/*
 * ComputeColumnFiltersGeometry -- 
 *
 *      +---------------------------+   
 *      |b|x|icon|x|text|x|arrow|x|b|   
 *      +---------------------------+
 *
 * b = filter borderwidth
 * x = padx 
 */
static void
ComputeColumnFiltersGeometry(TableView *viewPtr)
{
    unsigned int ah;
    FilterInfo *filterPtr;
    Column *colPtr;

    filterPtr = &viewPtr->filter;
    viewPtr->colFilterHeight = 0;
    viewPtr->arrowWidth = ah = Blt_TextWidth(filterPtr->font, "0", 1) + 
        2 * (filterPtr->borderWidth + 1);
    for (colPtr = viewPtr->colHeadPtr; colPtr != NULL; 
         colPtr = colPtr->nextPtr) {
        unsigned int tw, th, ih, iw;

        tw = th = ih = iw = 0;
        if (colPtr->filterIcon != NULL) {
            ih = IconHeight(colPtr->filterIcon);
            iw = IconWidth(colPtr->filterIcon);
        }
        if (colPtr->filterText != NULL) {
            TextStyle ts;
            Blt_Font font;

            Blt_Ts_InitStyle(ts);
            font = CHOOSE(filterPtr->font, colPtr->filterFont);
            Blt_Ts_SetFont(ts, font);
            Blt_Ts_GetExtents(&ts, colPtr->filterText, &tw, &th);
            colPtr->filterTextWidth = tw;
            colPtr->filterTextHeight = th;
        } else {
            Blt_FontMetrics fm;
            Blt_Font font;

            font = CHOOSE(filterPtr->font, colPtr->filterFont);
            Blt_Font_GetMetrics(font, &fm);
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
            ComputeColumnTitleGeometry(viewPtr, colPtr);
        } 
    }
    if (Blt_ConfigModified(columnSpecs, "-filtertext", (char *)NULL)) {
        ComputeColumnFiltersGeometry(viewPtr);
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
    cachedObjOption.clientData = viewPtr;
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
    Column *colPtr;

    for (colPtr = viewPtr->colHeadPtr; colPtr != NULL; 
         colPtr = colPtr->nextPtr) {
        if ((colPtr->flags & (HIDDEN|DISABLED|DELETED)) == 0) {
            return colPtr;
        }
    }
    return NULL;
}

static Column *
GetNextColumn(Column *colPtr)
{
    for (colPtr = colPtr->nextPtr; colPtr != NULL; colPtr = colPtr->nextPtr) {
        if ((colPtr->flags & (HIDDEN|DISABLED|DELETED)) == 0) {
            return colPtr;
        }
    }
    return NULL;
}

static Column *
GetPrevColumn(Column *colPtr)
{
    for (colPtr = colPtr->prevPtr; colPtr != NULL; colPtr = colPtr->prevPtr) {
        if ((colPtr->flags & (HIDDEN|DISABLED|DELETED)) == 0) {
            return colPtr;
        }
    }
    return NULL;
}

static Column *
GetLastColumn(TableView *viewPtr)
{
    Column *colPtr;

    for (colPtr = viewPtr->colTailPtr; colPtr != NULL; 
         colPtr = colPtr->prevPtr) {
        if ((colPtr->flags & (HIDDEN|DISABLED|DELETED)) == 0) {
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
 *      Finds the row closest to the given screen X-coordinate in the
 *      viewport.
 *
 * Results:
 *      Returns the pointer to the closest row.  If no row is visible (rows
 *      may be hidden), NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Column *
NearestColumn(TableView *viewPtr, int x, int selectOne)
{
    Column *lastPtr;
    long i;

    if (x < viewPtr->rowTitleWidth) {
        return (selectOne) ? viewPtr->visibleColumns[0] : NULL;
    }
    /*
     * Since the entry positions were previously computed in world
     * coordinates, convert x-coordinate from screen to world coordinates
     * too.
     */
    x = WORLDX(viewPtr, x);
    lastPtr = NULL;                     /* Suppress compiler warning. */
    /* FIXME: This can be a binary search. */
    for (i = 0; i < viewPtr->numVisibleColumns; i++) {
        Column *colPtr;

        colPtr = viewPtr->visibleColumns[i];
        if (colPtr->worldX > x) {
            return (selectOne) ? colPtr : NULL;
        }
        if (x < (colPtr->worldX + colPtr->width)) {
            return colPtr;              /* Found it. */
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
        
        keyPtr = Blt_GetHashKey(&viewPtr->cellTable,
                                viewPtr->focusPtr->hashPtr);
        focusPtr = keyPtr->colPtr;
    }
    c = string[0];
    if (c == '@') {
        int x;

        if (Tcl_GetInt(NULL, string + 1, &x) == TCL_OK) {
            colPtr = NearestColumn(viewPtr, x, FALSE);
        }
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
        colPtr = GetLastColumn(viewPtr);
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
            ItemType type;

            type = (ItemType)Blt_GetCurrentHint(viewPtr->bindTable);
            switch (type) {
            case ITEM_COLUMN_TITLE:
            case ITEM_COLUMN_FILTER:
            case ITEM_COLUMN_RESIZE:
                colPtr = (Column *)objPtr;
                break;
            case ITEM_CELL:
                {
                    Cell *cellPtr;
                    CellKey *keyPtr;
                    
                    cellPtr = (Cell *)objPtr;
                    keyPtr = GetKey(cellPtr);
                    colPtr = keyPtr->colPtr;
                }
                break;
            default:
                break;
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
    Row *rowPtr;

    for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL; 
         rowPtr = rowPtr->nextPtr) {
        if ((rowPtr->flags & (HIDDEN|DISABLED|DELETED)) == 0) {
            return rowPtr;
        }
    }
    return NULL;
}

static Row *
GetNextRow(Row *rowPtr)
{
    for (rowPtr = rowPtr->nextPtr; rowPtr != NULL; rowPtr = rowPtr->nextPtr) {
        if ((rowPtr->flags & (HIDDEN|DISABLED|DELETED)) == 0) {
            return rowPtr;
        }
    }
    return NULL;
}

static Row *
GetPrevRow(Row *rowPtr)
{
    for (rowPtr = rowPtr->prevPtr; rowPtr != NULL; rowPtr = rowPtr->prevPtr) {
        if ((rowPtr->flags & (HIDDEN|DISABLED|DELETED)) == 0) {
            return rowPtr;
        }
    }
    return NULL;
}

static Row *
GetLastRow(TableView *viewPtr)
{
    Row *rowPtr;

    for (rowPtr = viewPtr->rowTailPtr; rowPtr != NULL; 
         rowPtr = rowPtr->prevPtr) {
        if ((rowPtr->flags & (HIDDEN|DISABLED|DELETED)) == 0) {
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
 *      Finds the row closest to the given screen Y-coordinate in the
 *      viewport.
 *
 * Results:
 *      Returns the pointer to the closest row.  If no row is visible (rows
 *      may be hidden), NULL is returned.
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
    lastPtr = NULL;                     /* Suppress compiler warning. */
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
            return rowPtr;              /* Found it. */
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
        
        keyPtr = Blt_GetHashKey(&viewPtr->cellTable,
                                viewPtr->focusPtr->hashPtr);
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
    } else if ((c == 'e') && (length > 1) && 
               (strncmp(string, "end", length) == 0)) {
        rowPtr = GetLastRow(viewPtr);
    } else if ((c == 'c') && (strncmp(string, "current", length) == 0)) {
        TableObj *objPtr;

        objPtr = Blt_GetCurrentItem(viewPtr->bindTable);
        if ((objPtr != NULL) && ((objPtr->flags & DELETED) == 0)) {
            ItemType type;

            type = (ItemType)Blt_GetCurrentHint(viewPtr->bindTable);
            switch (type) {
            case ITEM_ROW_TITLE:
            case ITEM_ROW_FILTER:
            case ITEM_ROW_RESIZE:
                rowPtr = (Row *)objPtr;
                break;
            case ITEM_CELL:
                {
                    Cell *cellPtr;
                    CellKey *keyPtr;
                    
                    cellPtr = (Cell *)objPtr;
                    keyPtr = GetKey(cellPtr);
                    rowPtr = keyPtr->rowPtr;
                }
                break;
            default:
                break;
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

        if (blt_table_iterate_rows(interp, viewPtr->table, objPtr, &iter)
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

    if (viewPtr->table == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "no datatable configured.", (char *)NULL);
        }
        return TCL_ERROR;
    }
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

        if (blt_table_iterate_columns(interp, viewPtr->table, objPtr, &iter)
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
                continue;
            }
        }
        if (blt_table_iterate_rows(interp, viewPtr->table, objv[i], &iter)
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
                continue;
            }
        }
        if (blt_table_iterate_columns(interp, viewPtr->table, objv[i], &iter)
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
            ItemType type;
            
            type = (ItemType)Blt_GetCurrentHint(viewPtr->bindTable);
            if (type == ITEM_CELL) {
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
        CellSelection *selPtr = &viewPtr->selectCells;

        if (selPtr->markPtr != NULL) {
            *cellPtrPtr = GetCell(viewPtr, selPtr->markPtr->rowPtr,
                                  selPtr->markPtr->colPtr);
        }
        return TCL_OK;
    } else if ((c == 'a') && (length > 1) && 
               (strncmp(string, "anchor", length) == 0)) {
        CellSelection *selPtr = &viewPtr->selectCells;

        if (selPtr->anchorPtr != NULL) {
            *cellPtrPtr = GetCell(viewPtr, selPtr->anchorPtr->rowPtr,
                selPtr->anchorPtr->colPtr);
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
    /* FIXME: Try to get cell by tag. */
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
    /* FIXME: Try to get cells by tag. */
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
    Blt_InitHashTableWithPool(&cellTable, sizeof(CellKey)/sizeof(int));
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
                CellKey *keyPtr;

                keyPtr = GetKey(cellPtr);
                Blt_CreateHashEntry(&cellTable, (char *)keyPtr, &isNew);
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
 *      This procedure is called back by Tk when the selection is grabbed
 *      away.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The existing selection is unhighlighted, and the window is marked
 *      as not containing a selection.
 *
 *---------------------------------------------------------------------------
 */
static void
LostSelection(ClientData clientData)
{
    TableView *viewPtr = clientData;

    if (viewPtr->flags & SELECT_EXPORT) {
        ClearSelections(viewPtr);
        EventuallyRedraw(viewPtr);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * SelectRow --
 *
 *      Adds the given row from the set of selected rows.  It's OK if the
 *      row has already been selected.
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
 *      Removes the given row from the set of selected rows. It's OK
 *      if the row isn't already selected.
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
 *      Sets the selection flag for a range of nodes.  The range is
 *      determined by two pointers which designate the first/last nodes of
 *      the range.
 *
 * Results:
 *      Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectRows(TableView *viewPtr, Row *fromPtr, Row *toPtr)
{
    RenumberRows(viewPtr);
    if (fromPtr->index > toPtr->index) {
        Row *rowPtr;

        for (rowPtr = fromPtr; rowPtr != NULL; rowPtr = rowPtr->prevPtr) {
            if ((rowPtr->flags & HIDDEN) == 0) {
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
            if (rowPtr == toPtr) {
                break;
            }
        }
    } else {
        Row *rowPtr;

        for (rowPtr = fromPtr; rowPtr != NULL; rowPtr = rowPtr->nextPtr) {
            if ((rowPtr->flags & HIDDEN) == 0) {
                switch (viewPtr->selectRows.flags & SELECT_MASK) {
                case SELECT_CLEAR:
                    DeselectRow(viewPtr, rowPtr);   break;
                case SELECT_SET:
                    SelectRow(viewPtr, rowPtr);     break;
                case SELECT_TOGGLE:
                    if (rowPtr->flags & SELECTED) {
                        DeselectRow(viewPtr, rowPtr);
                    } else {
                        SelectRow(viewPtr, rowPtr);
                    }
                    break;
                }
            }
            if (rowPtr == toPtr) {
                break;
            }
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectRange --
 *
 *      Sets the selection flag for a range of nodes.  The range is
 *      determined by two pointers which designate the first/last nodes of
 *      the range.
 *
 * Results:
 *      Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
static void
AddSelectionRange(TableView *viewPtr)
{
    CellSelection *selPtr;
    Row *rowPtr, *firstRowPtr, *lastRowPtr;
    Column *firstColPtr, *lastColPtr;
    CellKey key;

    selPtr = &viewPtr->selectCells;
    if (selPtr->anchorPtr == NULL) {
        return;
    }
    if (selPtr->anchorPtr->rowPtr->index > selPtr->markPtr->rowPtr->index) {
        lastRowPtr = selPtr->anchorPtr->rowPtr;
        firstRowPtr = selPtr->markPtr->rowPtr;
    } else {
        firstRowPtr = selPtr->anchorPtr->rowPtr;
        lastRowPtr = selPtr->markPtr->rowPtr;
    }        
    if (selPtr->anchorPtr->colPtr->index > selPtr->markPtr->colPtr->index) {
        lastColPtr = selPtr->anchorPtr->colPtr;
        firstColPtr = selPtr->markPtr->colPtr;
    } else {
        firstColPtr = selPtr->anchorPtr->colPtr;
        lastColPtr = selPtr->markPtr->colPtr;
    }        
    for (rowPtr = firstRowPtr; rowPtr != NULL; rowPtr = rowPtr->nextPtr) {
        Column *colPtr;

        key.rowPtr = rowPtr;
        for (colPtr = firstColPtr; colPtr != NULL; colPtr = colPtr->nextPtr) {
            int isNew;

            key.colPtr = colPtr;
            Blt_CreateHashEntry(&selPtr->cellTable, &key, &isNew);
            if (colPtr == lastColPtr) {
                break;
            }
        }
        if (rowPtr == lastRowPtr) {
            break;
        }
    }
    selPtr->markPtr = selPtr->anchorPtr = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetSelectedCells --
 *
 *      Sets the selection flag for a range of nodes.  The range is
 *      determined by two pointers which designate the first/last nodes of
 *      the range.
 *
 * Results:
 *      Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
static void
GetSelectedCells(TableView *viewPtr, CellKey *anchorPtr, CellKey *markPtr)
{
    Row *minRowPtr, *maxRowPtr;
    Column *minColPtr, *maxColPtr;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    CellSelection *selPtr;

    selPtr = &viewPtr->selectCells;
    minRowPtr = maxRowPtr = NULL;
    minColPtr = maxColPtr = NULL;
    for (hPtr = Blt_FirstHashEntry(&selPtr->cellTable, &iter); 
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        CellKey *keyPtr;

        keyPtr = (CellKey *)Blt_GetHashKey(&selPtr->cellTable, hPtr);
        if ((minRowPtr == NULL) || (minRowPtr->index > keyPtr->rowPtr->index)){
            minRowPtr = keyPtr->rowPtr;
        } 
        if ((maxRowPtr == NULL) || (maxRowPtr->index < keyPtr->rowPtr->index)){
            maxRowPtr = keyPtr->rowPtr;
        } 
        if ((minColPtr == NULL) || (minColPtr->index > keyPtr->colPtr->index)){
            minColPtr = keyPtr->colPtr;
        } 
        if ((maxColPtr == NULL) || (maxColPtr->index < keyPtr->colPtr->index)){
            maxColPtr = keyPtr->colPtr;
        } 
    }        
    anchorPtr->rowPtr = minRowPtr;
    anchorPtr->colPtr = minColPtr;
    markPtr->rowPtr = maxRowPtr;
    markPtr->colPtr = maxColPtr;
}

static void
GetSelectedRows(TableView *viewPtr, CellKey *anchorPtr, CellKey *markPtr)
{
    Row *rowPtr;
    Column *colPtr;
    CellSelection *selPtr;

    selPtr = &viewPtr->selectCells;
    for (rowPtr = anchorPtr->rowPtr; rowPtr != NULL; rowPtr = rowPtr->nextPtr) {
        int selected;

        selected = FALSE;
        rowPtr->flags &= ~HAS_SELECTION;
        for (colPtr = anchorPtr->colPtr; colPtr != NULL; 
             colPtr = colPtr->nextPtr) {
            CellKey key;

            key.colPtr = colPtr;
            key.rowPtr = rowPtr;
            if (Blt_FindHashEntry(&selPtr->cellTable, &key) != NULL) {
                selected = TRUE;
                break;
            }
            if (colPtr == markPtr->colPtr) {
                break;
            }
        }
        if (selected) {
            rowPtr->flags |= HAS_SELECTION;
        }
        if (rowPtr == markPtr->rowPtr) {
            break;
        }
    }
}

static void
GetSelectedColumns(TableView *viewPtr, CellKey *anchorPtr, CellKey *markPtr)
{
    Row *rowPtr;
    Column *colPtr;
    CellSelection *selPtr;

    selPtr = &viewPtr->selectCells;
    for (colPtr = anchorPtr->colPtr; colPtr != NULL; colPtr = colPtr->nextPtr) {
        int selected;

        selected = FALSE;
        colPtr->flags &= ~HAS_SELECTION;
        for (rowPtr = anchorPtr->rowPtr; rowPtr != NULL; rowPtr = rowPtr->nextPtr) {
            CellKey key;

            key.colPtr = colPtr;
            key.rowPtr = rowPtr;
            if (Blt_FindHashEntry(&selPtr->cellTable, &key) != NULL) {
                selected = TRUE;
                break;
            }
            if (rowPtr == markPtr->rowPtr) {
                break;
            }
        }
        if (selected) {
            colPtr->flags |= HAS_SELECTION;
        }
        if (colPtr == markPtr->colPtr) {
            break;
        }
    }
}

static void
ComputeRowTitleGeometry(TableView *viewPtr, Row *rowPtr)
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
            ComputeRowTitleGeometry(viewPtr, rowPtr);
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
    Tcl_DString ds;

    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds "event flags are: ", -1);
    if (type & TABLE_NOTIFY_COLUMN_CHANGED) {
        Tcl_DStringAppend(&ds "-column ", -1);
    } 
    if (type & TABLE_NOTIFY_ROW_CHANGED) {
        Tcl_DStringAppend(&ds "-row ", -1);
    } 
    if (type & TABLE_NOTIFY_CREATE) {
        Tcl_DStringAppend(&ds "-create ", -1);
    } 
    if (type & TABLE_NOTIFY_DELETE) {
        Tcl_DStringAppend(&ds "-delete ", -1);
    }
    if (type & TABLE_NOTIFY_MOVE) {
        Tcl_DStringAppend(&ds "-move ", -1);
    }
    if (type & TABLE_NOTIFY_RELABEL) {
        Tcl_DStringAppend(&ds "-relabel ", -1);
    }
    fprintf(stderr, "%s\n", Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
}
#endif

static int
TableEventProc(ClientData clientData, BLT_TABLE_NOTIFY_EVENT *eventPtr)
{
    TableView *viewPtr = clientData; 

   if (eventPtr->type & (TABLE_NOTIFY_DELETE|TABLE_NOTIFY_CREATE)) {
       if (eventPtr->type == TABLE_NOTIFY_ROWS_CREATED) {
           if (viewPtr->flags & AUTO_ROWS) {
               /* Add the row and eventually reindex */
               AddRow(viewPtr, eventPtr->row);
           }
       } else if (eventPtr->type == TABLE_NOTIFY_COLUMNS_CREATED) {
           if (viewPtr->flags & AUTO_COLUMNS) {
               /* Add the column and eventually reindex */
               AddColumn(viewPtr, eventPtr->column);
           }
       } else if (eventPtr->type == TABLE_NOTIFY_ROWS_DELETED) {
           if (viewPtr->flags & AUTO_ROWS) {
               /* Delete the row and eventually reindex */
               DeleteRow(viewPtr, eventPtr->row);
           }
       } else if (eventPtr->type == TABLE_NOTIFY_COLUMNS_DELETED) {
           if (viewPtr->flags & AUTO_COLUMNS) {
               /* Delete the column and eventually reindex */
               DeleteColumn(viewPtr, eventPtr->column);
           }
       }
       return TCL_OK;
    } 
    if (eventPtr->type & TABLE_NOTIFY_COLUMN_CHANGED) {
        if (eventPtr->type & TABLE_NOTIFY_RELABEL) {
            Column *colPtr;
            
            colPtr = GetColumnContainer(viewPtr, eventPtr->column);
            if (colPtr != NULL) {
                ComputeColumnTitleGeometry(viewPtr, colPtr);
            }
        } else if (eventPtr->type & TABLE_NOTIFY_MOVE) {
            ReorderColumns(viewPtr);
        }       
    }
    if (eventPtr->type & TABLE_NOTIFY_ROW_CHANGED) {
        if (eventPtr->type & TABLE_NOTIFY_RELABEL) {
            Row *rowPtr;
            
            rowPtr = GetRowContainer(viewPtr, eventPtr->row);
            if (rowPtr != NULL) {
                ComputeRowTitleGeometry(viewPtr, rowPtr);
            }
        } else if (eventPtr->type & TABLE_NOTIFY_MOVE) {
            ReorderRows(viewPtr);
        }       
    }
    return TCL_OK;
}

static BindTag 
MakeBindTag(TableView *viewPtr, ClientData clientData, int type)
{
    Blt_HashEntry *hPtr;
    int isNew;                          /* Not used. */
    struct _BindTag tag;

    memset(&tag, 0, sizeof(tag));
    tag.type = type;
    tag.clientData = clientData;
    hPtr = Blt_CreateHashEntry(&viewPtr->bindTagTable, &tag, &isNew);
    return Blt_GetHashKey(&viewPtr->bindTagTable, hPtr);
}

static BindTag
MakeStringBindTag(TableView *viewPtr, const char *string, int type)
{
    Blt_HashEntry *hPtr;
    int isNew;                          /* Not used. */

    hPtr = Blt_CreateHashEntry(&viewPtr->uidTable, string, &isNew);
    return MakeBindTag(viewPtr, Blt_GetHashKey(&viewPtr->uidTable, hPtr), type);
}


static void
AddBindTags(TableView *viewPtr, Blt_Chain tags, Tcl_Obj *objPtr, int type)
{
    int objc;
    Tcl_Obj **objv;
    
    if (Tcl_ListObjGetElements(NULL, objPtr, &objc, &objv) == TCL_OK) {
        int i;

        for (i = 0; i < objc; i++) {
            const char *string;

            string = Tcl_GetString(objv[i]);
            Blt_Chain_Append(tags, MakeStringBindTag(viewPtr, string, type));
        }
    }
}

static void
AppendTagsProc(Blt_BindTable table, ClientData item, ClientData hint, 
               Blt_Chain tags)
{
    TableView *viewPtr;
    ItemType type = (ItemType)hint;
    TableObj *objPtr;
    
    objPtr = item;
    if (objPtr->flags & DELETED) {
        return;
    }
    viewPtr = Blt_GetBindingData(table);
    switch(type) {
    case ITEM_COLUMN_FILTER:
    case ITEM_COLUMN_TITLE:
    case ITEM_COLUMN_RESIZE:
        {
            Column *colPtr = item;
            
            Blt_Chain_Append(tags, MakeBindTag(viewPtr, item, type));
            if (colPtr->bindTagsObjPtr != NULL) {
                AddBindTags(viewPtr, tags, colPtr->bindTagsObjPtr, type);
            }
        }
        break;

    case ITEM_ROW_RESIZE:
    case ITEM_ROW_TITLE:
        {
            Row *rowPtr = item;
            
            Blt_Chain_Append(tags, MakeBindTag(viewPtr, item, type));
            if (rowPtr->bindTagsObjPtr != NULL) {
                AddBindTags(viewPtr, tags, rowPtr->bindTagsObjPtr, type);
            }
        }
        break;
        
    case ITEM_CELL:
        {
            Cell *cellPtr = item;
            CellStyle *stylePtr;
            CellKey *keyPtr;
            BindTag tag;
            
            keyPtr = GetKey(cellPtr);
            Blt_Chain_Append(tags, MakeBindTag(viewPtr, cellPtr, type));
            stylePtr = GetCurrentStyle(viewPtr, keyPtr->rowPtr, keyPtr->colPtr,
                                       cellPtr);
            Blt_Chain_Append(tags, MakeBindTag(viewPtr, keyPtr->rowPtr, type));
            Blt_Chain_Append(tags, MakeBindTag(viewPtr, keyPtr->colPtr, type));
            if (stylePtr->name != NULL) {
                /* Append cell style name. */
                tag = MakeStringBindTag(viewPtr, stylePtr->name, type);
                Blt_Chain_Append(tags, tag);
            }
            /* Append cell style class. */
            tag = MakeStringBindTag(viewPtr, stylePtr->classPtr->className,
                             type);
            Blt_Chain_Append(tags, tag);
            Blt_Chain_Append(tags, MakeStringBindTag(viewPtr, "all", type));
        }
        break;

    default:
        fprintf(stderr, "unknown item type (%d) %p\n", type, item);
        break;
    }
}

/*ARGSUSED*/
static ClientData
TableViewPickProc(
    ClientData clientData,
    int x, int y,                       /* Screen coordinates of the test
                                         * point. */
    ClientData *hintPtr)                /* (out) Context of item selected:
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
            *hintPtr = (ClientData)ITEM_CELL;
        }
        return viewPtr->postPtr;
    }
    if (viewPtr->filter.postPtr != NULL) {
        if (hintPtr != NULL) {
            *hintPtr = (ClientData)ITEM_COLUMN_FILTER;
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
        ComputeVisibleEntries(viewPtr);
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
                ItemType type;
                
                if (worldX >= (colPtr->worldX + colPtr->width - RESIZE_AREA)) {
                    type = ITEM_COLUMN_RESIZE;
                } else {
                    type = ITEM_COLUMN_TITLE;
                }
                *hintPtr = (ClientData)type;
            }
            return colPtr;              /* We're picking the filter. */
        }
        if (y < (viewPtr->inset + viewPtr->colTitleHeight + 
                 viewPtr->colFilterHeight)) {
            if (hintPtr != NULL) {
                *hintPtr = (ClientData)ITEM_COLUMN_FILTER;
            }
            return colPtr;              /* We're picking the title/resize. */
        }
    }
    /* Determine if we're picking a row heading as opposed a cell.  */
    if ((rowPtr != NULL) && ((rowPtr->flags & (DISABLED|HIDDEN)) == 0) &&
        (viewPtr->flags & ROW_TITLES) && 
        (x < (viewPtr->inset + viewPtr->rowTitleWidth))) {
        if (hintPtr != NULL) {
            ItemType type;

            if (worldY >= (rowPtr->worldY + rowPtr->height - RESIZE_AREA)) {
                type = ITEM_ROW_RESIZE;
            } else {
                type = ITEM_ROW_TITLE;
            }
            *hintPtr = (ClientData)type;
        }
        return rowPtr;                  /* We're picking the title/resize. */
    }
    if ((colPtr == NULL) || (colPtr->flags & (DISABLED|HIDDEN))) {
        return NULL;                    /* Ignore disabled columns. */
    }
    if ((rowPtr == NULL) || (rowPtr->flags & (DISABLED|HIDDEN))) {
        return NULL;                    /* Ignore disabled rows. */
    }
    cellPtr = GetCell(viewPtr, rowPtr, colPtr);
    if (hintPtr != NULL) {
        *hintPtr = (ClientData)ITEM_CELL;
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
        colPtr->flags |= DELETED;       /* Mark the column as deleted. This
                                         * prevents DestroyColumn from
                                         * pruning out cells that reside in
                                         * this column. We'll delete the
                                         * entire cell table. */
        DestroyColumn(colPtr);
    }
    for (hPtr = Blt_FirstHashEntry(&viewPtr->rowTable, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        Row *rowPtr;

        rowPtr = Blt_GetHashValue(hPtr);
        rowPtr->hashPtr = NULL;
        rowPtr->flags |= DELETED;       /* Mark the row as deleted. This
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
        cellPtr->hashPtr = NULL;
        DestroyCell(cellPtr);
    }
    Blt_SetCurrentItem(viewPtr->bindTable, NULL, NULL);
    Blt_DeleteHashTable(&viewPtr->rowTable);
    Blt_DeleteHashTable(&viewPtr->columnTable);
    Blt_DeleteHashTable(&viewPtr->cellTable);
    Blt_InitHashTable(&viewPtr->cellTable, sizeof(CellKey)/sizeof(int));
    Blt_InitHashTable(&viewPtr->rowTable, BLT_ONE_WORD_KEYS);
    Blt_InitHashTable(&viewPtr->columnTable, BLT_ONE_WORD_KEYS);
    if (viewPtr->rowMap != NULL) {
        Blt_Free(viewPtr->rowMap);
        viewPtr->rowMap = NULL;
    }
    if (viewPtr->columnMap != NULL) {
        Blt_Free(viewPtr->columnMap);
        viewPtr->columnMap = NULL;
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
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the internal structure of a TableView at a safe time (when
 *      no-one is using it anymore).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the widget is freed up.
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
    if (viewPtr->sort.upArrow != NULL) {
        Blt_FreePicture(viewPtr->sort.upArrow);
    }
    if (viewPtr->sort.downArrow != NULL) {
        Blt_FreePicture(viewPtr->sort.downArrow);
    }
    if (viewPtr->painter != NULL) {
        Blt_FreePainter(viewPtr->painter);
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
    Blt_DeleteHashTable(&viewPtr->selectCells.cellTable);
    Blt_Chain_Destroy(viewPtr->selectRows.list);
    Blt_DeleteHashTable(&viewPtr->cellTable);
    Blt_DeleteHashTable(&viewPtr->rowTable);
    Blt_DeleteHashTable(&viewPtr->columnTable);
    Blt_DeleteHashTable(&viewPtr->bindTagTable);
    Blt_DeleteHashTable(&viewPtr->uidTable);
    Blt_DeleteHashTable(&viewPtr->cachedObjTable);
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
 *      This procedure is invoked by the Tk dispatcher for various events
 *      on tableview widgets.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      When the window gets deleted, internal structures get cleaned up.
 *      When it gets exposed, it is redisplayed.
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
    length = (string == NULL) ? 0 : strlen(string);
    type = blt_table_column_type(colPtr->column);
    CsvAppendRecord(writerPtr, string, length, type);
}

static void
CsvAppendRow(CsvWriter *writerPtr, TableView *viewPtr, Row *rowPtr)
{
    Column *colPtr;

    CsvStartRecord(writerPtr);
    for (colPtr = viewPtr->colHeadPtr; colPtr != NULL; 
         colPtr = colPtr->nextPtr) {
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
 *      This procedure is called back by Tk when the selection is requested
 *      by someone.  It returns part or all of the selection in a buffer
 *      provided by the caller.
 *
 * Results:
 *      The return value is the number of non-NULL bytes stored at buffer.
 *      Buffer is filled (or partially filled) with a NUL-terminated string
 *      containing part or all of the selection, as given by offset and
 *      maxBytes.
 *
 * Side effects:
 *      None.
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
    memset(&writer, 0, sizeof(CsvWriter));
    writer.dsPtr = &ds;
    writer.length = 0;
    writer.count = 0;
    switch (viewPtr->selectMode) {
    case SELECT_CELLS:
        {
            Row *rowPtr;
            CellKey anchor, mark;

            GetSelectedCells(viewPtr, &anchor, &mark);
            GetSelectedRows(viewPtr, &anchor, &mark);
            GetSelectedColumns(viewPtr, &anchor, &mark);
            for (rowPtr = anchor.rowPtr; 
                 rowPtr != NULL && rowPtr->index <= mark.rowPtr->index; 
                 rowPtr = rowPtr->nextPtr) {
                Column *colPtr;

                if ((rowPtr->flags & HAS_SELECTION) == 0) {
                    continue;
                }
                CsvStartRecord(&writer);
                for (colPtr = anchor.colPtr; colPtr != NULL; 
                     colPtr = colPtr->nextPtr) {

                    if ((colPtr->flags & HAS_SELECTION) == 0) {
                        continue;
                    }
                    CsvAppendValue(&writer, viewPtr, rowPtr, colPtr);
                    if (colPtr == mark.colPtr) {
                        break;
                    }
                }
                CsvEndRecord(&writer);
                if (rowPtr == mark.rowPtr) {
                    break;
                }
            }
        }
        break;

    case SELECT_SINGLE_ROW:
    case SELECT_MULTIPLE_ROWS:
        if (viewPtr->flags & SELECT_SORTED) {
            Blt_ChainLink link;
            
            for (link = Blt_Chain_FirstLink(viewPtr->selectRows.list); 
                 link != NULL; link = Blt_Chain_NextLink(link)) {
                Row *rowPtr;
                
                rowPtr = Blt_Chain_GetValue(link);
                CsvAppendRow(&writer, viewPtr, rowPtr);
            }
        } else {
            Row *rowPtr;
            
            for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL; 
                 rowPtr = rowPtr->nextPtr) {
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
 *      Updates the GCs and other information associated with the tableview
 *      widget.
 *
 * Results:
 *      The return value is a standard TCL result.  If TCL_ERROR is
 *      returned, then interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for viewPtr; old resources get freed, if there were
 *      any.  The widget is redisplayed.
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
 *      Updates the GCs and other information associated with the tableview
 *      widget.
 *
 * Results:
 *      The return value is a standard TCL result.  If TCL_ERROR is
 *      returned, then interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for viewPtr; old resources get freed, if there were
 *      any.  The widget is redisplayed.
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

static Blt_Picture
GetFilterArrowPicture(FilterInfo *filterPtr, int w, int h, XColor *colorPtr)
{
    if ((filterPtr->downArrow == NULL) ||
        (Blt_Picture_Width(filterPtr->downArrow) != w) ||
        (Blt_Picture_Height(filterPtr->downArrow) != h)) {
        Blt_Picture picture;
        int ix, iy, iw, ih;
        
        if (filterPtr->downArrow != NULL) {
            Blt_FreePicture(filterPtr->downArrow);
        }
        iw = w * 75 / 100;
        ih = h * 40 / 100;
        
        picture = Blt_CreatePicture(w, h);
        Blt_BlankPicture(picture, 0x0);
        iy = (h - ih) / 2;
        ix = (w - iw) / 2;
        Blt_PaintArrowHead(picture, ix, iy, iw, ih,
                           Blt_XColorToPixel(colorPtr), ARROW_DOWN);
        filterPtr->downArrow = picture;
    }
    return filterPtr->downArrow;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawColumnFilter --
 *
 *      Draws the combo filter button given the screen coordinates and the
 *      value to be displayed.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The combo filter button value is drawn.
 *
 *      +---------------------------+   
 *      |b|x|icon|x|text|x|arrow|x|b|   
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
    rowHeight = viewPtr->colFilterHeight;
    colWidth  = colPtr->width;
    if ((rowHeight == 0) || (colWidth == 0)) {
        return;
    }
    relief = filterPtr->relief;
    if (colPtr->flags & DISABLED) {     /* Disabled  */
        fg = filterPtr->disabledFg;
        filterBg = bg = filterPtr->disabledBg;
        gc = filterPtr->disabledGC;
    } else if (colPtr == filterPtr->postPtr) {   /* Posted */
        bg = filterPtr->activeBg;
        fg = filterPtr->activeFg;
        filterBg = filterPtr->normalBg;
        gc = filterPtr->activeGC;
        relief = filterPtr->selectRelief;
    } else if (colPtr == filterPtr->activePtr) {  /* Active */
        bg = filterPtr->activeBg;
        fg = filterPtr->activeFg;
        filterBg = filterPtr->normalBg;
        gc = filterPtr->activeGC;
        relief = filterPtr->activeRelief;
    } else if (colPtr->flags & FILTERHIGHLIGHT) { /* Highlighted */
        fg = filterPtr->highlightFg;
        filterBg = bg = filterPtr->highlightBg;
        gc = filterPtr->highlightGC;
        relief = TK_RELIEF_FLAT;
    } else {                            /* Normal */
        fg = filterPtr->normalFg;
        filterBg = bg = filterPtr->normalBg;
        gc = filterPtr->normalGC;
        relief = TK_RELIEF_FLAT;
    }

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
            Blt_Font font;

            font = CHOOSE(filterPtr->font, colPtr->filterFont);
            Blt_Ts_InitStyle(ts);
            Blt_Ts_SetFont(ts, font);
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
        ax = x + filterWidth - aw;
        ay = y;
        Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, ax, ay,
                             aw, ah, filterPtr->borderWidth, relief);
        aw -= 2 * filterPtr->borderWidth;
        ax += filterPtr->borderWidth;
        if ((ax <= Tk_Width(viewPtr->tkwin)) &&
             (ay <= Tk_Height(viewPtr->tkwin)) && (aw > 0) && (ah > 0)) {
            Blt_Picture picture;

            picture = GetFilterArrowPicture(filterPtr, aw, ah, fg);
            if (viewPtr->painter == NULL) {
                viewPtr->painter = Blt_GetPainter(viewPtr->tkwin, 1.0);
            }
            Blt_PaintPicture(viewPtr->painter, drawable,
                             picture, 0, 0, aw, ah, ax, ay, 0);
        }
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
        return;                         /* Cell isn't in viewport.  This
                                         * can happen when the active cell
                                         * is scrolled off screen. */
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

static Blt_Picture
GetSortArrowPicture(TableView *viewPtr, int w, int h)
{
    if (viewPtr->sort.decreasing) {
        if (viewPtr->sort.upArrow == NULL) {
            Blt_Picture picture;
            int ix, iy, iw, ih;
            
            iw = w * 50 / 100;
            ih = h * 80 / 100;
            iy = (h - ih) / 2;
            ix = (w - iw) / 2;
            picture = Blt_CreatePicture(w, h);
            Blt_BlankPicture(picture, 0x0);
            Blt_PaintArrow(picture, ix, iy, iw, ih, 0xFFFF0000, ARROW_UP);
            viewPtr->sort.upArrow = picture;
        }
        return viewPtr->sort.upArrow;
    } else {
        if (viewPtr->sort.downArrow == NULL) {
            Blt_Picture picture;
            int ix, iy, iw, ih;

            iw = w * 50 / 100;
            ih = h * 80 / 100;
            iy = (h - ih) / 2;
            ix = (w - iw) / 2;
            picture = Blt_CreatePicture(w, h);
            Blt_BlankPicture(picture, 0x0);
            Blt_PaintArrow(picture, ix, iy, iw, ih, 0xFF0000FF, ARROW_DOWN);
            viewPtr->sort.downArrow = picture;
        }            
        return viewPtr->sort.downArrow;
    }
}

static void
DrawColumnTitle(TableView *viewPtr, Column *colPtr, Drawable drawable, int x, 
                int y)
{
    Blt_Bg bg;
    GC gc;
    SortInfo *sortPtr;
    int relief;
    int wanted, colWidth, colHeight;
    unsigned int aw, ah, iw, ih, tw, th;
    unsigned int igap, agap;

    sortPtr = &viewPtr->sort;
    if (viewPtr->colTitleHeight < 1) {
        return;
    }
    colWidth = colPtr->width;
    colHeight = viewPtr->colTitleHeight;
    if ((colWidth == 0) || (colHeight == 0)) {
        return;
    }
    relief = colPtr->titleRelief;
    if (colPtr->flags & DISABLED) {     /* Disabled  */
        bg = viewPtr->colDisabledTitleBg;
        gc = viewPtr->colDisabledTitleGC;
    } else if (colPtr == viewPtr->colActiveTitlePtr) {  /* Active */
        bg = viewPtr->colActiveTitleBg;
        gc = viewPtr->colActiveTitleGC;
        relief = colPtr->activeTitleRelief;
    } else {                            /* Normal */
        bg = viewPtr->colNormalTitleBg;
        gc = viewPtr->colNormalTitleGC;
    }

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
            Blt_FontMetrics fm;

            Blt_Font_GetMetrics(viewPtr->colTitleFont, &fm);
            ah = fm.linespace;
            aw = ah * 60 / 100;
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

        /* Center the icon vertically.  We already know the column title is
         * at least as tall as the icon. */
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
        int ax, ay;
        ax = x;
        ay = y + (colHeight - ah) / 2;
        ay--;
        if ((viewPtr->sort.decreasing) && (viewPtr->sort.up != NULL)) {
            Tk_RedrawImage(IconBits(viewPtr->sort.up), 0, 0, aw, ah, drawable, 
                ax, ay);
        } else if (viewPtr->sort.down != NULL) {
            Tk_RedrawImage(IconBits(viewPtr->sort.down), 0, 0, aw, ah, drawable,
                ax, ay);
        } else if ((aw > 0) && (ah > 0)) {
            Blt_Picture picture;

            picture = GetSortArrowPicture(viewPtr, aw, ah);
            if (viewPtr->painter == NULL) {
                viewPtr->painter = Blt_GetPainter(viewPtr->tkwin, 1.0);
            }
            Blt_PaintPicture(viewPtr->painter, drawable,
                             picture, 0, 0, aw, ah, ax, ay, 0);
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
    if (rowPtr->flags & DISABLED) {     /* Disabled  */
        bg = viewPtr->rowDisabledTitleBg;
        gc = viewPtr->rowDisabledTitleGC;
    } else if (rowPtr == viewPtr->rowActiveTitlePtr) {  /* Active */
        bg = viewPtr->rowActiveTitleBg;
        gc = viewPtr->rowActiveTitleGC;
        relief = rowPtr->activeTitleRelief;
    } else {                            /* Normal */
        bg = viewPtr->rowNormalTitleBg;
        gc = viewPtr->rowNormalTitleGC;
    }
    dy = y;
    h = rowPtr->height;
    if (rowPtr->index == (viewPtr->numRows - 1)) {
        /* If there's any room left over, let the last row take it. */
        h = Tk_Height(viewPtr->tkwin) - y;
    }
    if ((viewPtr->rowTitleWidth == 0) || (h == 0)) {
        return;
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
        /* Center the icon vertically.  We already know the column title is
         * at least as tall as the icon. */
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
        return;                         /* Row starts after the window or
                                         * ends before the window. */
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
        return;                         /* Column starts after the window
                                         * or ends before the the
                                         * window. */
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
        return;                         /* Column starts after the window
                                         * or ends before the the
                                         * window. */
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
            if (viewPtr->flags & COLUMN_FILTERS) {
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
        int w, h;

        w = Tk_Width(viewPtr->tkwin)  - 2 * viewPtr->highlightWidth;
        h = Tk_Height(viewPtr->tkwin) - 2 * viewPtr->highlightWidth;
        if ((w > 0) && (h > 0)) {
            Blt_Bg_DrawRectangle(viewPtr->tkwin, drawable, 
                viewPtr->rowNormalTitleBg, viewPtr->highlightWidth, 
                viewPtr->highlightWidth, w, h, 
                viewPtr->borderWidth, viewPtr->relief);
        }
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
    long x;
    Column *colPtr;
    double weight;
    int growth;
    long numOpen;

    growth = VPORTWIDTH(viewPtr) - viewPtr->worldWidth;
    assert(growth > 0);
    lastPtr = NULL;

    numOpen = 0;
    weight = 0.0;

    /* Find out how many columns still have space available */
    for (colPtr = viewPtr->colHeadPtr; colPtr != NULL; 
         colPtr = colPtr->nextPtr) {
        if (colPtr->flags & HIDDEN) {
            continue;
        }
        lastPtr = colPtr;
        if ((colPtr->weight == 0.0) || (colPtr->width >= colPtr->max)) {
            continue;
        }
        numOpen++;
        weight += colPtr->weight;
    }
    while ((numOpen > 0) && (weight > 0.0) && (growth > 0)) {
        int ration;
        Column *colPtr;

        ration = (int)(growth / weight);
        if (ration == 0) {
            ration = 1;
        }
        for (colPtr = viewPtr->colHeadPtr; colPtr != NULL; 
             colPtr = colPtr->nextPtr) {
            int size, avail;
            
            if ((numOpen <= 0) || (growth <= 0) || (weight <= 0.0)) {
                break;
            }
            if (colPtr->flags & HIDDEN) {
                continue;
            }
            lastPtr = colPtr;
            if ((colPtr->weight == 0.0) || (colPtr->width >= colPtr->max)) {
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
    for (colPtr = viewPtr->colHeadPtr; colPtr != NULL; 
         colPtr = colPtr->nextPtr) {
        if (colPtr->flags & HIDDEN) {
            continue;                   /* Ignore hidden columns. */
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
    Row *rowPtr;

    growth = VPORTHEIGHT(viewPtr) - viewPtr->worldHeight;
    assert(growth > 0);
    lastPtr = NULL;
    numOpen = 0;
    weight = 0.0;
    /* Find out how many columns still have space available */
    for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL; 
         rowPtr = rowPtr-nextPtr) {
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
        Row *rowPtr;

        ration = (int)(growth / weight);
        if (ration == 0) {
            ration = 1;
        }
        for (rowPtr = rowPtr->rowHeadPtr; rowPtr != NULL;
             rowPtr = rowPtr->nextPtr) { 
            int size, avail;

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
    for (rowPtr = rowPtr->rowHeadPtr; rowPtr != NULL; 
         rowPtr = rowPtr->nextPtr) {
        if (rowPtr->flags & HIDDEN) {
            continue;                   /* Ignore hidden columns. */
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
 *      Makes the cell appear active.
 *
 *      pathName activate cell
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
    /* If we aren't already queued to redraw the widget, try to directly
     * draw into window. */
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
 *      pathName bbox cellName ?switches ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BboxOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellKey *keyPtr;
    Column *colPtr;
    Row *rowPtr;
    TableView *viewPtr = clientData;
    Tcl_Obj *listObjPtr;
    int w, h;
    int x1, y1, x2, y2;
    BBoxSwitches switches;
    
    if (viewPtr->table == NULL) {
        Tcl_AppendResult(interp, "no data table to view.", (char *)NULL);
        return TCL_ERROR;
    }
    if (viewPtr->flags & (LAYOUT_PENDING|GEOMETRY)) {
        /*
         * The layout is dirty.  Recompute it now, before we use the world
         * dimensions.  But remember that the "bbox" operation isn't valid
         * for hidden entries (since they're not visible, they don't have
         * world coordinates).
         */
        ComputeGeometry(viewPtr);
    }

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr)  != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, bboxSwitches, objc - 3, objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    keyPtr = GetKey(cellPtr);
    rowPtr = keyPtr->rowPtr;
    colPtr = keyPtr->colPtr;
    x1 = colPtr->worldX;
    x2 = colPtr->worldX + colPtr->width;
    y1 = rowPtr->worldY;
    y2 = rowPtr->worldY + rowPtr->height;

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
    if (switches.flags & BBOX_ROOT) {
        int rootX, rootY;
        
        Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
        x1 += rootX, y1 += rootY;
        x2 += rootX, y2 += rootY;
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
 *      pathName bind tag sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BindOp(TableView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    BindTag tag;
    Cell *cellPtr;

    /*
     * Cells are selected by id only.  All other strings are interpreted as
     * a binding tag.
     */
    if ((GetCellFromObj(NULL, viewPtr, objv[2], &cellPtr) == TCL_OK) &&
        (cellPtr != NULL)) {
        tag = MakeBindTag(viewPtr, cellPtr, ITEM_CELL);
    } else {
        /* Assume that this is a binding tag. */
        tag = MakeStringBindTag(viewPtr, Tcl_GetString(objv[2]), ITEM_CELL);
    } 
    return Blt_ConfigureBindingsFromObj(interp, viewPtr->bindTable, tag, 
         objc - 3, objv + 3);
}

/*
 *---------------------------------------------------------------------------
 *
 * CellActivateOp --
 *
 *      Turns on highlighting for a particular cell.  Only one cell can be
 *      active at a time.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 *      pathName cell activate ?cell?
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
    /* If we aren't already queued to redraw the widget, try to directly
     * draw into window. */
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
 *      pathName cell bbox cellName ?switches...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellBboxOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellKey *keyPtr;
    Column *colPtr;
    Row *rowPtr;
    TableView *viewPtr = clientData;
    Tcl_Obj *listObjPtr;
    int w, h;
    int x1, y1, x2, y2;
    BBoxSwitches switches;
    
    if (viewPtr->table == NULL) {
        Tcl_AppendResult(interp, "no data table to view.", (char *)NULL);
        return TCL_ERROR;
    }
    if (viewPtr->flags & (LAYOUT_PENDING|GEOMETRY)) {
        /*
         * The layout is dirty.  Recompute it now, before we use the world
         * dimensions.  But remember that the "bbox" operation isn't valid
         * for hidden entries (since they're not visible, they don't have
         * world coordinates).
         */
        ComputeGeometry(viewPtr);
    }

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr)  != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, bboxSwitches, objc - 3, objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    keyPtr = GetKey(cellPtr);
    rowPtr = keyPtr->rowPtr;
    colPtr = keyPtr->colPtr;
    x1 = colPtr->worldX;
    x2 = colPtr->worldX + colPtr->width;
    y1 = rowPtr->worldY;
    y2 = rowPtr->worldY + rowPtr->height;

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
    if (switches.flags & BBOX_ROOT) {
        int rootX, rootY;
        
        Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
        x1 += rootX, y1 += rootY;
        x2 += rootX, y2 += rootY;
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
 *      pathName cell cget cellName option 
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
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) the widget.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for viewPtr; old resources get freed, if there were
 *      any.  The widget is redisplayed.
 *
 *      pathName cell configure cellName ?option value...?
 *
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
            Blt_HashEntry *hPtr;

            cellPtr->stylePtr->refCount++;      
            hPtr = Blt_CreateHashEntry(&cellPtr->stylePtr->table, 
                                       (char *)keyPtr, &isNew);
            assert(isNew);
            Blt_SetHashValue(hPtr, cellPtr);
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
        cellPtr->flags |= GEOMETRY;     /* Assume that the new style
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
 *      Deactivates all cells.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 *        pathName deactivate 
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
 *      Gets or sets focus on a cell.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 *        pathName cell focus ?cell?
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
            objPtr = GetRowIndexObj(viewPtr, rowPtr);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            objPtr = GetColumnIndexObj(viewPtr, colPtr);
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
            return TCL_OK;              /* Can't set focus to hidden or
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
 *      pathName cell identify cellName x y 
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
 *      Converts the string representing a cell index into their respective
 *      "node field" identifiers.
 *
 * Results: 
 *      A standard TCL result.  Interp->result will contain the identifier
 *      of each inode found. If an inode could not be found, then the
 *      serial identifier will be the empty string.
 *
 *      pathName cell index cellName
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
    objPtr = GetRowIndexObj(viewPtr, rowPtr);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = GetColumnIndexObj(viewPtr, colPtr);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellInvokeOp --
 *
 *      pathName cell invoke cellName
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
        objPtr = GetRowIndexObj(viewPtr, rowPtr);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        objPtr = GetColumnIndexObj(viewPtr, colPtr);
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
 *      pathName cell see cellName
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellSeeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellKey *keyPtr;
    TableView *viewPtr = clientData;
    long xOffset, yOffset;

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    keyPtr = GetKey(cellPtr);
    yOffset = GetRowYOffset(viewPtr, keyPtr->rowPtr);
    xOffset = GetColumnXOffset(viewPtr, keyPtr->colPtr);
    if (xOffset != viewPtr->xOffset) {
        viewPtr->xOffset = xOffset;
        viewPtr->flags |= SCROLLX;
    }
    if (yOffset != viewPtr->yOffset) {
        viewPtr->yOffset = yOffset;
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
 *      pathName cell style cellName
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

    cellPtr = NULL;                     /* Suppress compiler warning. */
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
 *        pathName cell writable cellName
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
 *      This procedure handles cell operations.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec cellOps[] =
{
    {"activate",   1, CellActivateOp,    3, 4, "?cellName?",},
    {"bbox",       1, CellBboxOp,        4, 0, "cellName ?switches ...?",},
    {"cget",       2, CellCgetOp,        5, 5, "cellName option",},
    {"configure",  2, CellConfigureOp,   4, 0, "cellName ?option value ...?",},
    {"deactivate", 1, CellDeactivateOp,  3, 3, "",},
    {"focus",      2, CellFocusOp,       4, 0, "?cellName?",},
    {"identify",   2, CellIdentifyOp,    6, 6, "cellName x y",},
    {"index",      3, CellIndexOp,       4, 4, "cellName",},
    {"invoke",     3, CellInvokeOp,      4, 4, "cellName",},
    {"see",        3, CellSeeOp,         4, 4, "cellName",},
    {"style",      3, CellStyleOp,       4, 4, "cellName",},
    {"writable",   3, CellWritableOp,    4, 4, "cellName",},
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
 *      pathName cget option
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
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) the widget.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for viewPtr; old resources get freed, if there were
 *      any.  The widget is redisplayed. 
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
 *      pathName curselection
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CurselectionOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    switch (viewPtr->selectMode) {
    case SELECT_CELLS:
        {
            Blt_HashEntry *hPtr;
            Blt_HashSearch iter;
            CellSelection *selPtr = &viewPtr->selectCells;

            for (hPtr = Blt_FirstHashEntry(&selPtr->cellTable, &iter); 
                 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
                CellKey *keyPtr;
                Tcl_Obj *objPtr, *subListObjPtr;
                
                keyPtr = Blt_GetHashValue(hPtr);
                subListObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
                objPtr = GetRowIndexObj(viewPtr, keyPtr->rowPtr);
                Tcl_ListObjAppendElement(interp, subListObjPtr, objPtr);
                objPtr = GetColumnIndexObj(viewPtr, keyPtr->colPtr);
                Tcl_ListObjAppendElement(interp, subListObjPtr, objPtr);
                Tcl_ListObjAppendElement(interp, listObjPtr, subListObjPtr);
            }
        }
        break;
    case SELECT_SINGLE_ROW:
    case SELECT_MULTIPLE_ROWS:
        if (viewPtr->flags & SELECT_SORTED) {
            Blt_ChainLink link;
            
            for (link = Blt_Chain_FirstLink(viewPtr->selectRows.list); 
                 link != NULL; link = Blt_Chain_NextLink(link)) {
                Row *rowPtr;
                Tcl_Obj *objPtr;
                
                rowPtr = Blt_Chain_GetValue(link);
                objPtr = GetRowIndexObj(viewPtr, rowPtr);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
        } else {
            Row *rowPtr;
            
            for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL;
                 rowPtr = rowPtr->nextPtr) {
                if (rowPtr->flags & SELECTED) {
                    Tcl_Obj *objPtr;
                    
                    objPtr = GetRowIndexObj(viewPtr, rowPtr);
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
 *      Sets the button to appear active.
 *
 *      pathName column activate ?col?
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
        return TCL_OK;                  /* Disabled or hidden row. */
    }
    activePtr = viewPtr->colActiveTitlePtr;
    viewPtr->colActiveTitlePtr = colPtr;

    /* If we aren't already queued to redraw the widget, try to directly
     * draw into window. */
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
 *      Bind a callback to an event on a column title.
 *
 *        pathName column bind tag type sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnBindOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    BindTag tag;
    Column *colPtr;
    const char *string;
    int length;
    char c;
    ItemType type;

    string = Tcl_GetStringFromObj(objv[4], &length);
    c = string[0];
    if ((c == 'c') && (strncmp(string, "cell", length) == 0)) {
        type = ITEM_CELL;
    } else if ((c == 't') && (strncmp(string, "title", length) == 0)) {
        type = ITEM_COLUMN_TITLE;
    } else if ((c == 'r') && (strncmp(string, "resize", length) == 0)) {
        type = ITEM_COLUMN_RESIZE;
    } else if ((c == 'f') && (strncmp(string, "filter", length) == 0)) {
        type = ITEM_COLUMN_FILTER;
    } else {
        Tcl_AppendResult(interp, "Bad column bind tag type \"", string, "\"",
                         (char *)NULL);
        return TCL_ERROR;
    }
    if (GetColumn(NULL, viewPtr, objv[3], &colPtr) == TCL_OK) {
        tag = MakeBindTag(viewPtr, colPtr, type);
    } else {
        tag = MakeStringBindTag(viewPtr, Tcl_GetString(objv[3]), type);
    }
    return Blt_ConfigureBindingsFromObj(interp, viewPtr->bindTable, tag,
        objc - 5, objv + 5);
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
 *      This procedure is called to process a list of configuration options
 *      database, in order to reconfigure the one of more entries in the
 *      widget.
 *
 *        pathName column configure $col option value
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for viewPtr; old resources get freed, if there were
 *      any.
 *
 *      pathName column configure col ?option value?
 *
 *---------------------------------------------------------------------------
 */
static int
ColumnConfigureOp(TableView *viewPtr, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    Blt_Chain columns;
    Blt_ChainLink link;

    cachedObjOption.clientData = viewPtr;
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
 *      Sets the column to appear normally.
 *
 *      pathName column deactivate
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
        return TCL_OK;                  /* Disabled or hidden row. */
    }
    activePtr = viewPtr->colActiveTitlePtr;
    viewPtr->colActiveTitlePtr = NULL;
    /* If we aren't already queued to redraw the widget, try to directly
     * draw into window. */
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
 *      pathName column delete col col col 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
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
        DestroyColumn(colPtr);
    }
    Blt_Chain_Destroy(columns);

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
 *      pathName column exists col
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
 *      pathName column expose ?colName...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnExposeOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    if (objc == 3) {
        Column *colPtr;
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        for (colPtr = viewPtr->colHeadPtr; colPtr != NULL; 
             colPtr = colPtr->nextPtr) {
            if ((colPtr->flags & HIDDEN) == 0) {
                Tcl_Obj *objPtr;

                objPtr = GetColumnIndexObj(viewPtr, colPtr);
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
 * ColumnFindOp --
 *
 *      pathName column find x1 y1 x2 y2 ?switches ...?
 *              -enclosing -overlapping -root 
 *---------------------------------------------------------------------------
 */
static int
ColumnFindOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int x1, y1, x2, y2;
    long i;
    int rootX, rootY;

    if ((Tk_GetPixelsFromObj(interp, viewPtr->tkwin, objv[3], &x1) != TCL_OK) ||
        (Tk_GetPixelsFromObj(interp, viewPtr->tkwin, objv[4], &y1) != TCL_OK) ||
        (Tk_GetPixelsFromObj(interp, viewPtr->tkwin, objv[5], &x2) != TCL_OK) ||
        (Tk_GetPixelsFromObj(interp, viewPtr->tkwin, objv[6], &y2) != TCL_OK)) {
        return TCL_ERROR;
    } 
    rootX = rootY = 0;
#ifdef notdef
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, colFindSwitches, objc - 7, objv + 7, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (switches.flags & FIND_ROOT) {
        Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
    }
#endif
    if (x1 > x2) {
        int tmp;

        tmp = x2; x2 = x1; x1 = tmp;
    }
    if (y1 > y2) {
        int tmp;

        tmp = y2; y2 = y1; y1 = tmp;
    }
    y1 = WORLDX(viewPtr, y1 - rootY);
    y2 = WORLDX(viewPtr, y2 - rootY);
    if ((y2 < viewPtr->inset) || 
        (y1 >= (viewPtr->inset + viewPtr->colTitleHeight))) {
        Tcl_SetWideIntObj(Tcl_GetObjResult(interp), -1);
        return TCL_OK;
    }
    /*
     * Since the entry positions were previously computed in world
     * coordinates, convert x-coordinates from screen to world coordinates
     * too.
     */
    x1 = WORLDX(viewPtr, x1 - rootX);
    x2 = WORLDX(viewPtr, x2 - rootX);
    for (i = 0; i < viewPtr->numVisibleColumns; i++) {
        Column *colPtr;

        colPtr = viewPtr->visibleColumns[i];
        if ((x1 < (colPtr->worldX + colPtr->width)) && 
            (x2 > colPtr->worldX)) {
            size_t index;

            index = blt_table_column_index(viewPtr->table, colPtr->column);
            Tcl_SetWideIntObj(Tcl_GetObjResult(interp), index);
            return TCL_OK;
        }
    }
    Tcl_SetWideIntObj(Tcl_GetObjResult(interp), -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnHideOp --
 *
 *      pathName column hide ?colName...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnHideOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    if (objc == 3) {
        Column *colPtr;
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        for (colPtr = viewPtr->colHeadPtr; colPtr != NULL; 
             colPtr = colPtr->nextPtr) {
            if (colPtr->flags & HIDDEN) {
                Tcl_Obj *objPtr;

                objPtr = GetColumnIndexObj(viewPtr, colPtr);
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
 *      pathName column index col
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
    ssize_t index;

    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    index = (colPtr != NULL) ? 
        blt_table_column_index(viewPtr->table, colPtr->column) : -1;
    Tcl_SetWideIntObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ColumnInsertOp --
 *
 *      Add new columns to the table.
 *
 *      pathName column insert colName position ?option values?
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
    Column *colPtr;
    Row *rowPtr;
    TableView *viewPtr = clientData;
    int isNew;
    long insertPos;

    if (viewPtr->table == NULL) {
        Tcl_AppendResult(interp, "no data table to view.", (char *)NULL);
        return TCL_ERROR;
    }
    col = blt_table_get_column(interp, viewPtr->table, objv[3]);
    if (col == NULL) {
        return TCL_ERROR;
    }
    /* Test for position before creating the column.  */
    if (Blt_GetPositionFromObj(viewPtr->interp, objv[4], &insertPos) != TCL_OK){
        return TCL_ERROR;
    }
    /* 
     * Column doesn't have to exist.  We'll add it when the table adds
     * columns.
     */
    hPtr = Blt_CreateHashEntry(&viewPtr->columnTable, (char *)col, &isNew);
    if (!isNew) {
        Tcl_AppendResult(interp, "a column \"", Tcl_GetString(objv[3]),
                "\" already exists in \"", Tk_PathName(viewPtr->tkwin),
                "\"", (char *)NULL);
        return TCL_ERROR;
    }
    colPtr = NewColumn(viewPtr, col, hPtr);
    Blt_SetHashValue(hPtr, colPtr);
    colPtr->flags |= STICKY;            /* Don't allow column to be
                                         * reset. */
    iconOption.clientData = viewPtr;
    cachedObjOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(viewPtr->interp, viewPtr->tkwin, 
        colPtr->title, "Column", columnSpecs, objc - 5, objv + 5, 
        (char *)colPtr, 0) != TCL_OK) { 
        DestroyColumn(colPtr);
        return TCL_ERROR;
    }
    if ((insertPos != -1) && (insertPos < (viewPtr->numColumns - 1))) {
        Column *destPtr;

        destPtr = viewPtr->columnMap[insertPos];
        MoveColumns(viewPtr, destPtr, colPtr, colPtr, FALSE);
    }
    key.colPtr = colPtr;
    /* Automatically populate cells for each row in the new column. */
    for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL; 
         rowPtr = rowPtr->nextPtr) {
        Blt_HashEntry *hPtr;
        int isNew;

        key.rowPtr = rowPtr;
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
 *      This procedure is called to invoke a column command.
 *
 *        pathName column invoke columnName
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
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
    objPtr = GetColumnIndexObj(viewPtr, colPtr);
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
 *      Move one or more columns.
 *
 *      tableName column move destCol firstCol lastCol ?switches?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnMoveOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Column *destPtr, *firstPtr, *lastPtr;
    TableView *viewPtr = clientData;
    int after = TRUE;

    if (GetColumn(interp, viewPtr, objv[3], &destPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetColumn(interp, viewPtr, objv[4], &firstPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetColumn(interp, viewPtr, objv[5], &lastPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (viewPtr->flags & REINDEX_COLUMNS) {
        RenumberColumns(viewPtr);
    }

    /* Check if range is valid. */
    if (firstPtr->index > lastPtr->index) {
        return TCL_OK;                  /* No range. */
    }

    /* Check that destination is outside the range of columns to be moved. */
    if ((destPtr->index >= firstPtr->index) &&
        (destPtr->index <= lastPtr->index)) {
        Tcl_AppendResult(interp, "destination column \"", 
                Tcl_GetString(objv[3]),
                 "\" can't be in the range of columns to be moved.", 
                (char *)NULL);
        return TCL_ERROR;
    }
    MoveColumns(viewPtr, destPtr, firstPtr, lastPtr, after);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ColumnNamesOp --
 *
 *      tableName column names ?pattern?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Tcl_Obj *listObjPtr;
    Column *colPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (colPtr = viewPtr->colHeadPtr; colPtr != NULL; 
         colPtr = colPtr->nextPtr) {
        Tcl_Obj *objPtr;

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
    int x;                         /* Screen coordinates of the test point. */
    Column *colPtr;
    ssize_t index;

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
    index = (colPtr != NULL) ? 
        blt_table_column_index(viewPtr->table, colPtr->column) : -1;
    Tcl_SetWideIntObj(Tcl_GetObjResult(interp), index);
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
    if ((colPtr->reqWidth.min > 0) && ((width + dx) < colPtr->reqWidth.min)) {
        fprintf(stderr, "column %s: bounding min to %d\n", colPtr->title,
                colPtr->reqWidth.min);
        dx = colPtr->reqWidth.min - width;
    }
    if ((colPtr->reqWidth.max > 0) && ((width + dx) > colPtr->reqWidth.max)) {
        fprintf(stderr, "column %s: bounding max to %d\n", colPtr->title,
                colPtr->reqWidth.max);
        dx = colPtr->reqWidth.max - width;
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
 *      Turns on/off the resize cursor.
 *
 *      pathName column resize activate col 
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
 *      Set the anchor for the resize.
 *
 *      pathName column resize anchor ?x?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeAnchorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    if (objc == 5) { 
        int y;

        if (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
            return TCL_ERROR;
        } 
        viewPtr->colResizeAnchor = y;
        UpdateColumnMark(viewPtr, y);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), viewPtr->colResizeAnchor);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnResizeDeactiveOp --
 *
 *      Turns off the resize cursor.
 *
 *      pathName column resize deactivate 
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
 *      Sets the resize mark.  The distance between the mark and the anchor
 *      is the delta to change the width of the active column.
 *
 *      pathName column resize mark ?x?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeMarkOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    if (objc == 5) { 
        int y;

        if (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
            return TCL_ERROR;
        } 
        UpdateColumnMark(viewPtr, y);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), viewPtr->colResizeMark);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnResizeGetOp --
 *
 *      Returns the new width of the column including the resize delta.
 *
 *      pathName column resize get 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeGetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    UpdateColumnMark(viewPtr, viewPtr->colResizeMark);
    if (viewPtr->colResizePtr != NULL) {
        int width, delta;

        delta = (viewPtr->colResizeMark - viewPtr->colResizeAnchor);
        width = viewPtr->colResizePtr->width + delta;
        Tcl_SetIntObj(Tcl_GetObjResult(interp), width);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnResizeSetOp --
 *
 *      Sets the nominal width of the column currently being resized.
 *
 *      pathName column resize set
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Column *colPtr;
    
    UpdateColumnMark(viewPtr, viewPtr->colResizeMark);
    colPtr = viewPtr->colResizePtr;
    if (colPtr != NULL) {
        int dx;

        dx = (viewPtr->colResizeMark - viewPtr->colResizeAnchor);
        colPtr->reqWidth.nom = colPtr->width + dx;
        colPtr->reqWidth.flags |= LIMITS_SET_NOM;
        viewPtr->colResizeAnchor = viewPtr->colResizeMark;
        viewPtr->flags |= LAYOUT_PENDING;
        EventuallyRedraw(viewPtr);
    }
    return TCL_OK;
}

static Blt_OpSpec columnResizeOps[] =
{ 
    {"activate",   2, ColumnResizeActivateOp,   5, 5, "column"},
    {"anchor",     2, ColumnResizeAnchorOp,     4, 5, "?x?"},
    {"deactivate", 1, ColumnResizeDeactivateOp, 4, 4, ""},
    {"get",        1, ColumnResizeGetOp,        4, 4, "",},
    {"mark",       1, ColumnResizeMarkOp,       4, 5, "?x?"},
    {"set",        1, ColumnResizeSetOp,        4, 4, ""},
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
 *      pathName column see col
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
    long xOffset;

    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (colPtr == NULL) {
        fprintf(stderr, "ColumnSee: Column %s is NULL\n", 
                Tcl_GetString(objv[3])); 
        return TCL_OK;
    }
    xOffset = GetColumnXOffset(viewPtr, colPtr);
    if (xOffset != viewPtr->xOffset) {
        viewPtr->xOffset = xOffset;
        viewPtr->flags |= SCROLLX;
        EventuallyRedraw(viewPtr);
    }
    return TCL_OK;
}

static Blt_OpSpec columnOps[] = {
    {"activate",   1, ColumnActivateOp,   4, 4, "colName",}, 
    {"bind",       1, ColumnBindOp,       5, 7, "tagName type ?sequence command?",},
    {"cget",       2, ColumnCgetOp,       5, 5, "colName option",}, 
    {"configure",  2, ColumnConfigureOp,  4, 0, "colName ?option value ...?",}, 
    {"deactivate", 2, ColumnDeactivateOp, 3, 3, "",},
    {"delete",     2, ColumnDeleteOp,     3, 0, "colName...",}, 
    {"exists",     3, ColumnExistsOp,     4, 4, "colName",}, 
    {"expose",     3, ColumnExposeOp,     3, 0, "?colName ...?",},
    {"find",       1, ColumnFindOp,       7, 7, "x1 y1 x2 y2",},
    {"hide",       1, ColumnHideOp,       3, 0, "?colName ...?",},
    {"index",      3, ColumnIndexOp,      4, 4, "colName",}, 
    {"insert",     3, ColumnInsertOp,     5, 0, "colName pos ?option value ...?",},  
    {"invoke",     3, ColumnInvokeOp,     4, 4, "colName",},  
    {"move",       1, ColumnMoveOp,       6, 0, "destCol firstCol lastCol ?switches?",},  
    {"names",      2, ColumnNamesOp,      3, 3, "",},
    {"nearest",    2, ColumnNearestOp,    4, 4, "x",},
    {"resize",     1, ColumnResizeOp,     3, 0, "args",},
    {"see",        2, ColumnSeeOp,        4, 4, "colName",}, 
    {"show",       2, ColumnExposeOp,     3, 0, "?colName ...?",},
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
 *      Makes the formerly active cell appear normal.
 *
 *      pathName deactivate 
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
    BLT_TABLE table;                    /* Table to be evaluated */
    BLT_TABLE_ROW row;                  /* Current row. */
    Blt_HashTable varTable;             /* Variable cache. */
    TableView *viewPtr;

    /* Public values */
    Tcl_Obj *emptyValueObjPtr;
    const char *tag;
    unsigned int flags;
} FindSwitches;

#define FIND_INVERT     (1<<0)

static Blt_SwitchSpec findSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-emptyvalue", "string",   (char *)NULL,
        Blt_Offset(FindSwitches, emptyValueObjPtr), 0},
    {BLT_SWITCH_STRING, "-addtag", "tagName", (char *)NULL,
        Blt_Offset(FindSwitches, tag), 0},
    {BLT_SWITCH_BITS_NOARG, "-invert", "", (char *)NULL,
        Blt_Offset(FindSwitches, flags), 0, FIND_INVERT},
    {BLT_SWITCH_END}
};

static int
ColumnVarResolverProc(
    Tcl_Interp *interp,                 /* Current interpreter. */
    const char *name,                   /* Variable name being resolved. */
    Tcl_Namespace *nsPtr,               /* Current namespace context. */
    int flags,                          /* TCL_LEAVE_ERR_MSG => leave error
                                         * message. */
    Tcl_Var *varPtr)                    /* (out) Resolved variable. */ 
{
    Blt_HashEntry *hPtr;
    BLT_TABLE_COLUMN col;
    FindSwitches *switchesPtr;
    Tcl_Obj *valueObjPtr;
    int64_t index;
    
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
    if ((isdigit(name[0])) &&
        (Blt_GetInt64((Tcl_Interp *)NULL, (char *)name, &index) == TCL_OK)) {
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
    Row *rowPtr;

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
    for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL; 
         rowPtr = rowPtr->nextPtr) {
        int bool;

        if (rowPtr->flags & HIDDEN) {
            continue;                   /* Ignore hidden rows. */
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
            objPtr = GetRowIndexObj(viewPtr, rowPtr);
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
 *      Sets the filter to appear active.
 *
 *      pathName filter activate ?col?
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
        return TCL_OK;                  /* Disabled or hidden row. */
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
 *      pathName filter cget -option
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
 *      This procedure is called to process a list of configuration
 *      options database, in order to reconfigure the one of more
 *      entries in the widget.
 *
 *        pathName filter configure option value
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 *      pathName filter configure 
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
 *      Sets the filter to appear normally.
 *
 *      pathName filter deactivate
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
        return TCL_OK;                  /* Disabled or hidden row. */
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
 *      pathName filter inside cell x y
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
 *      Posts the filter menu associated with this widget.
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      pathName filter post col
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
    int result;

    filterPtr = &viewPtr->filter;
    if (objc == 3) {
        ssize_t index;

        /* Report the column that has the filter menu posted. */
        index = -1;
        if (filterPtr->postPtr != NULL) {
            index = blt_table_column_index(viewPtr->table, 
                                           filterPtr->postPtr->column);
        }
        Tcl_SetWideIntObj(Tcl_GetObjResult(interp), index);
        return TCL_OK;
    }
    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (colPtr == NULL) {
        return TCL_OK;
    }
    if (filterPtr->postPtr != NULL) {
        return TCL_OK;                 /* Another filter's menu is currently
                                        * posted. */
    }
    if (colPtr->flags & (DISABLED|HIDDEN)) {
        return TCL_OK;                 /* Filter's menu is in a column that is
                                        * hidden or disabled. */
    }
    if (filterPtr->menuObjPtr == NULL) {
        return TCL_OK;                  /* No menu associated with filter. */
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
    
    result = TCL_ERROR;
    if (filterPtr->postCmdObjPtr != NULL) {
        Tcl_Obj *cmdObjPtr;

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
        Tcl_Obj *cmdObjPtr, *objPtr, *listObjPtr;

        cmdObjPtr = Tcl_DuplicateObj(filterPtr->menuObjPtr);
        /* menu post -align right -box {x1 y1 x2 y2}  */
        objPtr = Tcl_NewStringObj("post", 4);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        objPtr = Tcl_NewStringObj("-align", 6);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        objPtr = Tcl_NewStringObj("right", 5);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        objPtr = Tcl_NewStringObj("-box", 4);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x2));
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y2));
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x1));
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y1));
        Tcl_ListObjAppendElement(interp, cmdObjPtr, listObjPtr);
        Tcl_IncrRefCount(cmdObjPtr);
        Tcl_Preserve(viewPtr);
        result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_Release(viewPtr);
        Tcl_DecrRefCount(cmdObjPtr);
        if (result == TCL_OK) {
            filterPtr->postPtr = colPtr;
        }
    }
    if ((viewPtr->flags & REDRAW_PENDING) == 0) {
        if (filterPtr->postPtr != NULL) {
            Drawable drawable;
            
            drawable = Tk_WindowId(viewPtr->tkwin);
            DisplayColumnFilter(viewPtr, filterPtr->postPtr, drawable);
        }
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterUnpostOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *  pathName filter unpost
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
 *      Comparison routine (used by qsort) to sort a chain of subnodes.
 *      A simple string comparison is performed on each node name.
 *
 *      pathName filter configure col
 *      pathName filter cget col -recurse root
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec filterOps[] =
{
    {"activate",   1, FilterActivateOp,    4, 4, "col",},
    {"cget",       2, FilterCgetOp,        4, 4, "option",},
    {"configure",  2, FilterConfigureOp,   3, 0, "?option value ...?",},
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
 *      Find rows based upon the expression provided.
 *
 * Results:
 *      A standard TCL result.  The interpreter result will contain a list of
 *      the node serial identifiers.
 *
 *      pathName find expr 
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

    if (viewPtr->table == NULL) {
        Tcl_AppendResult(interp, "no data table to view.", (char *)NULL);
        return TCL_ERROR;
    }
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
 *      pathName focus ?cell?
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
            objPtr = GetRowIndexObj(viewPtr, rowPtr);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            objPtr = GetColumnIndexObj(viewPtr, colPtr);
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
            return TCL_OK;              /* Can't set focus to hidden or
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
 *      pathName grab ?cell?
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
            objPtr = GetRowIndexObj(viewPtr, rowPtr);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            objPtr = GetColumnIndexObj(viewPtr, colPtr);
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
 *      Makes the cell appear highlighted.  The cell is redrawn in its
 *      highlighted foreground and background colors.
 *
 *      pathName highlight cell
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
 *      pathName identify cell x y 
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
 *      pathName index cell
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
    objPtr = GetRowIndexObj(viewPtr, rowPtr);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = GetColumnIndexObj(viewPtr, colPtr);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InsideOp --
 *
 *      pathName inside cell x y
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
 *      pathName invoke cell
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
        objPtr = GetRowIndexObj(viewPtr, rowPtr);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        objPtr = GetColumnIndexObj(viewPtr, colPtr);
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
 *      pathName ishidden cell
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
 *      Sets the button to appear active.
 *
 *      pathName row activate row
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
        return TCL_OK;                  /* Disabled or hidden row. */
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
 *        pathName row bind tag type sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowBindOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    BindTag tag;
    Row *rowPtr;
    const char *string;
    int length;
    char c;
    ItemType type;

    string = Tcl_GetStringFromObj(objv[4], &length);
    c = string[0];
    if ((c == 'c') && (strncmp(string, "cell", length) == 0)) {
        type = ITEM_CELL;
    } else if ((c == 't') && (strncmp(string, "title", length) == 0)) {
        type = ITEM_ROW_TITLE;
    } else if ((c == 'r') && (strncmp(string, "resize", length) == 0)) {
        type = ITEM_ROW_RESIZE;
    } else {
        return TCL_ERROR;
    }
    if (GetRow(NULL, viewPtr, objv[3], &rowPtr) == TCL_OK) {
        tag = MakeBindTag(viewPtr, rowPtr, type);
    } else {
        tag = MakeStringBindTag(viewPtr, Tcl_GetString(objv[3]), type);
    }
    return Blt_ConfigureBindingsFromObj(interp, viewPtr->bindTable, tag,
        objc - 5, objv + 5);
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
 *      This procedure is called to process a list of configuration
 *      options database, in order to reconfigure the one of more
 *      entries in the widget.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for viewPtr; old resources get freed, if there
 *      were any.  The hypertext is redisplayed.
 *
 *      pathName row configure row ?option value?
 *
 *---------------------------------------------------------------------------
 */
static int
RowConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Row *rowPtr;

    cachedObjOption.clientData = viewPtr;
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
 *      Turn off active highlighting for all row titles.
 *
 *      pathName row deactivate
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
        return TCL_OK;                  /* Not displaying row titles. */
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
 *      pathName row delete row row row 
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
RowDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
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
        DestroyRow(rowPtr);
    }
    Blt_Chain_Destroy(chain);

    /* Requires a new layout. Sort order and individual geometies stay the
     * same. */
    viewPtr->flags |= LAYOUT_PENDING | REINDEX_ROWS;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowExistsOp --
 *
 *      pathName row exists row
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
 *      pathName row expose ?rowName ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowExposeOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    if (objc == 3) {
        Row *rowPtr;
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL; 
             rowPtr = rowPtr->nextPtr) {
            if ((rowPtr->flags & HIDDEN) == 0) {
                Tcl_Obj *objPtr;

                objPtr = GetRowIndexObj(viewPtr, rowPtr);
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
 *      pathName row hide ?rowName ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowHideOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    if (objc == 3) {
        Row *rowPtr;
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL; 
             rowPtr = rowPtr->nextPtr) {
            if (rowPtr->flags & HIDDEN) {
                Tcl_Obj *objPtr;

                objPtr = GetRowIndexObj(viewPtr, rowPtr);
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
 *      pathName row index row
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
    ssize_t index;

    if (GetRow(interp, viewPtr, objv[3], &rowPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    index = (rowPtr != NULL) ? 
        blt_table_row_index(viewPtr->table, rowPtr->row) : -1;
    Tcl_SetWideIntObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowInsertOp --
 *
 *      Add new rows to the displayed in the tableview widget.  
 *
 *      pathName row insert row position ?option values?
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
    Row *rowPtr;
    TableView *viewPtr = clientData;
    int isNew;
    Column *colPtr;
    long insertPos;

    if (viewPtr->table == NULL) {
        Tcl_AppendResult(interp, "no data table to view.", (char *)NULL);
        return TCL_ERROR;
    }
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
    rowPtr = NewRow(viewPtr, row, hPtr);
    iconOption.clientData = viewPtr;
    cachedObjOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(viewPtr->interp, viewPtr->tkwin, 
        rowPtr->title, "Row", rowSpecs, objc - 4, objv + 4, (char *)rowPtr, 0) 
        != TCL_OK) {
        DestroyRow(rowPtr);
        return TCL_ERROR;
    }
    if ((insertPos != -1) && (insertPos < (viewPtr->numRows - 1))) {
        Row *destPtr;

        destPtr = viewPtr->rowMap[insertPos];
        MoveRows(viewPtr, destPtr, rowPtr, rowPtr, FALSE);
    }
    /* Generate cells for the new row. */
    key.rowPtr = rowPtr;
    for (colPtr = viewPtr->colHeadPtr; colPtr != NULL;
         colPtr = colPtr->nextPtr) {
        Blt_HashEntry *hPtr;
        int isNew;

        key.colPtr = colPtr;
        hPtr = Blt_CreateHashEntry(&viewPtr->cellTable, (char *)&key, &isNew);
        if (isNew) {
            Cell *cellPtr;

            cellPtr = NewCell(viewPtr, hPtr);
            Blt_SetHashValue(hPtr, cellPtr);
        }
    }
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowInvokeOp --
 *
 *      This procedure is called to invoke a command associated with the
 *      row title.  The title must be not disabled or hidden for the 
 *      command to be executed.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 *      pathName row invoke rowName
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
    objPtr = GetRowIndexObj(viewPtr, rowPtr);
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
 *      Move one or more rows.
 *
 *      pathName row move dest first last ?switches?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowMoveOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Row *destPtr, *firstPtr, *lastPtr;
    TableView *viewPtr = clientData;
    int after = TRUE;

    if (GetRow(interp, viewPtr, objv[3], &destPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetRow(interp, viewPtr, objv[4], &firstPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetRow(interp, viewPtr, objv[5], &lastPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (viewPtr->flags & REINDEX_ROWS) {
        RenumberRows(viewPtr);
    }

    /* Check if range is valid. */
    if (firstPtr->index > lastPtr->index) {
        return TCL_OK;                  /* No range. */
    }

    /* Check that destination is outside the range of columns to be moved. */
    if ((destPtr->index >= firstPtr->index) &&
        (destPtr->index <= lastPtr->index)) {
        Tcl_AppendResult(interp, "destination row \"", 
                Tcl_GetString(objv[3]),
                 "\" can't be in the range of rows to be moved.", 
                (char *)NULL);
        return TCL_ERROR;
    }
    MoveRows(viewPtr, destPtr, firstPtr, lastPtr, after);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowNamesOp --
 *
 *      pathName row names ?pattern ...?
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
    Row *rowPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL; 
         rowPtr = rowPtr->nextPtr) {
        Tcl_Obj *objPtr;

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
 *      pathName row nearest y ?-root?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowNearestOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int y;                         /* Screen coordinates of the test point. */
    Row *rowPtr;

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
    if (rowPtr != NULL) {
        Tcl_SetWideIntObj(Tcl_GetObjResult(interp),
                          blt_table_row_index(viewPtr->table, rowPtr->row));
    } else {
        Tcl_SetWideIntObj(Tcl_GetObjResult(interp), -1);
    }
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
        return;                         /* No row being resized. */
    }
    rowPtr = viewPtr->rowResizePtr;
    dy = newMark - viewPtr->rowResizeAnchor; 
    height = rowPtr->height;
    if ((rowPtr->reqHeight.min > 0) && ((height + dy) < rowPtr->reqHeight.min)){
        dy = rowPtr->reqHeight.min - height;
    }
    if ((rowPtr->reqHeight.max > 0) && ((height + dy) > rowPtr->reqHeight.max)){
        dy = rowPtr->reqHeight.max - height;
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
 *      Turns on/off the resize cursor.
 *
 *      pathName row resize activate row 
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
 *      Set the anchor for the resize.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowResizeAnchorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;

    if (objc == 5) { 
        int y;

        if (Tcl_GetIntFromObj(NULL, objv[4], &y) != TCL_OK) {
            return TCL_ERROR;
        } 
        viewPtr->rowResizeAnchor = y;
        UpdateRowMark(viewPtr, y);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), viewPtr->rowResizeAnchor);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowResizeDeactiveOp --
 *
 *      Turns off the resize cursor.
 *
 *      pathName row resize deactivate 
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
 *      Sets the resize mark.  The distance between the mark and the anchor
 *      is the delta to change the width of the active row.
 *
 *      pathName row resize mark x 
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

    if (objc == 5) {
        if (Tcl_GetIntFromObj(NULL, objv[4], &y) != TCL_OK) {
            return TCL_ERROR;
        } 
        UpdateRowMark(viewPtr, y);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), viewPtr->rowResizeMark);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowResizeSetOp --
 *
 *      Sets the nominal height of the column currently being resized.
 *
 *      pathName row resize set
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowResizeSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    Row *rowPtr;
    
    UpdateRowMark(viewPtr, viewPtr->rowResizeMark);
    rowPtr = viewPtr->rowResizePtr;
    if (rowPtr != NULL) {
        int dy;

        dy = (viewPtr->rowResizeMark - viewPtr->rowResizeAnchor);
        rowPtr->reqHeight.nom = rowPtr->height + dy;
        rowPtr->reqHeight.flags |= LIMITS_SET_NOM;
        viewPtr->rowResizeAnchor = viewPtr->rowResizeMark;
        viewPtr->flags |= LAYOUT_PENDING;
        EventuallyRedraw(viewPtr);
    }
    return TCL_OK;
}

static Blt_OpSpec rowResizeOps[] =
{ 
    {"activate",   2, RowResizeActivateOp,   5, 5, "row"},
    {"anchor",     2, RowResizeAnchorOp,     4, 5, "?y?"},
    {"deactivate", 1, RowResizeDeactivateOp, 4, 4, ""},
    {"mark",       1, RowResizeMarkOp,       4, 5, "?y?"},
    {"set",        1, RowResizeSetOp,        4, 4, "",},
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
 *      Implements the quick scan.
 *
 *      pathName row see row
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
    long yOffset;

    if (GetRow(interp, viewPtr, objv[3], &rowPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (rowPtr == NULL) {
        return TCL_OK;
    }
    yOffset = GetRowYOffset(viewPtr, rowPtr);
    if (yOffset != viewPtr->yOffset) {
        viewPtr->yOffset = yOffset;
        viewPtr->flags |= SCROLLY;
        EventuallyRedraw(viewPtr);
    }
    return TCL_OK;
}

static Blt_OpSpec rowOps[] =
{
    {"activate",   1, RowActivateOp,   4, 4, "rowName",},
    {"bind",       1, RowBindOp,       5, 7, "tagName type ?sequence command?",},
    {"cget",       2, RowCgetOp,       5, 5, "rowName option",},
    {"configure",  2, RowConfigureOp,  4, 0, "rowName ?option value ...?",},
    {"deactivate", 3, RowDeactivateOp, 3, 3, "",},
    {"delete",     3, RowDeleteOp,     4, 0, "rowName...",},
    {"exists",     3, RowExistsOp,     4, 4, "rowName",},
    {"expose",     3, RowExposeOp,     3, 0, "?rowName ...?",},
    {"hide",       1, RowHideOp,       3, 0, "?rowName ...?",},
    {"index",      3, RowIndexOp,      4, 4, "rowName",},
    {"insert",     3, RowInsertOp,     5, 0, "rowName position ?option value ...?",},
    {"invoke",     3, RowInvokeOp,     4, 4, "rowName",},
    {"move",       1, RowMoveOp,       6, 0, "destCol firstCol lastCol ?switches?",},  
    {"names",      2, RowNamesOp,      3, 3, "",},
    {"nearest",    2, RowNearestOp,    4, 4, "y",},
    {"resize",     1, RowResizeOp,     3, 0, "args",},
    {"see",        2, RowSeeOp,        4, 4, "rowName",},
    {"show",       2, RowExposeOp,     3, 0, "?rowName ...?",},
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
 *      Implements the quick scan.
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

#define SCAN_MARK       1
#define SCAN_DRAGTO     2
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
 *      Changes to view to encompass the specified cell.
 *
 *      pathName see cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SeeOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellKey *keyPtr;
    TableView *viewPtr = clientData;
    long xOffset, yOffset;

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    keyPtr = GetKey(cellPtr);
    yOffset = GetRowYOffset(viewPtr, keyPtr->rowPtr);
    xOffset = GetColumnXOffset(viewPtr, keyPtr->colPtr);
    if (xOffset != viewPtr->xOffset) {
        viewPtr->xOffset = xOffset;
        viewPtr->flags |= SCROLLX;
    }
    if (yOffset != viewPtr->yOffset) {
        viewPtr->yOffset = yOffset;
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
 *      Sets the selection anchor to the element given by a index.  The
 *      selection anchor is the end of the selection that is fixed while
 *      dragging out a selection with the mouse.  The index "anchor" may be
 *      used to refer to the anchor element.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName selection anchor cell
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

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    keyPtr = GetKey(cellPtr);
    if (viewPtr->selectMode == SELECT_CELLS) {
        CellSelection *selPtr;

        selPtr = &viewPtr->selectCells;
        /* Set both the selection anchor and the mark. This indicates that a
         * single cell is selected. */
        selPtr->markPtr = selPtr->anchorPtr = keyPtr;
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
 *      Clears the entire selection.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName selection clearall
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
    EventuallyRedraw(viewPtr);
    if (viewPtr->selectCmdObjPtr != NULL) {
        EventuallyInvokeSelectCommand(viewPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionExportOp
 *
 *      Exports the current selection.  It is not an error if not selection
 *      is present.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection is exported.
 *
 *      pathName selection export
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionExportOp(ClientData clientData, Tcl_Interp *interp, int objc,
                  Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    int state;

    if (viewPtr->selectMode == SELECT_CELLS) {
        state = (viewPtr->selectCells.cellTable.numEntries > 0);
    } else {
        state = (Blt_Chain_GetLength(viewPtr->selectRows.list) > 0);
    }
    if (state) {
        Tk_OwnSelection(viewPtr->tkwin, XA_PRIMARY, LostSelection, viewPtr);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * SelectionIncludesOp
 *
 *      Returns 1 if the element indicated by index is currently
 *      selected, 0 if it isn't.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName selection includes cell
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

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
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
            Blt_HashEntry *hPtr;

            hPtr = Blt_FindHashEntry(&viewPtr->selectCells.cellTable, keyPtr);
            if (hPtr != NULL) {
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
 *      Sets the selection mark to the element given by a index.  The
 *      selection anchor is the end of the selection that is movable while
 *      dragging out a selection with the mouse.  The index "mark" may be used
 *      to refer to the anchor element.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName selection mark cell
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
        CellSelection *selPtr = &viewPtr->selectCells;
        
        if (selPtr->anchorPtr == NULL) {
            fprintf(stderr, "Attempting to set mark before anchor. Cell selection anchor must be set first\n");
            return TCL_OK;
        }
        selPtr->markPtr = GetKey(cellPtr);
        selPtr->flags &= ~SELECT_MASK;
        selPtr->flags |= SELECT_SET;
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
 *      Returns 1 if there is a selection and 0 if it isn't.
 *
 * Results:
 *      A standard TCL result.  interp->result will contain a boolean string
 *      indicating if there is a selection.
 *
 *      pathName selection present 
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
        state = (viewPtr->selectCells.cellTable.numEntries > 0);
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
 *      Selects, deselects, or toggles all of the elements in the range
 *      between first and last, inclusive, without affecting the selection
 *      state of elements outside that range.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName selection set cell cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    TableView *viewPtr = clientData;
    CellKey *anchorPtr, *markPtr;
    Cell *cellPtr;

    if (viewPtr->flags & (GEOMETRY | LAYOUT_PENDING)) {
        /*
         * The layout is dirty.  Recompute it now so that we can use
         * view.top and view.bottom for nodes.
         */
        ComputeGeometry(viewPtr);
    }
    if (GetCellFromObj(NULL, viewPtr, objv[3], &cellPtr) != TCL_OK) {
        /* Silently ignore invalid cell selections. This is to prevent
         * errors when the table is empty. */
        return TCL_OK;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    anchorPtr = GetKey(cellPtr);
    if ((anchorPtr->rowPtr->flags|anchorPtr->colPtr->flags) & HIDDEN) {
        Tcl_AppendResult(interp, "can't select hidden anchor",
                         (char *)NULL);
        return TCL_ERROR;
    }
    if (GetCellFromObj(NULL, viewPtr, objv[4], &cellPtr) != TCL_OK) {
        /* Silently ignore invalid cell selections. This is to prevent
         * errors when the table is empty. */
        return TCL_OK;
    }
    markPtr = GetKey(cellPtr);
    if ((markPtr->rowPtr->flags|markPtr->colPtr->flags) & HIDDEN) {
        Tcl_AppendResult(interp, "can't select hidden mark", (char *)NULL);
        return TCL_ERROR;
    }
    if (markPtr == NULL) {
        markPtr = anchorPtr;
    }
    if (viewPtr->selectMode == SELECT_CELLS) {
        CellSelection *selPtr = &viewPtr->selectCells;
        const char *string;

        selPtr->anchorPtr = anchorPtr;
        selPtr->markPtr = markPtr;
        selPtr->flags &= ~SELECT_MASK;
        string = Tcl_GetString(objv[2]);
        switch (string[0]) {
        case 's':
            selPtr->flags |= SELECT_SET;     break;
        case 'c':
            selPtr->flags |= SELECT_CLEAR;   break;
        case 't':
            selPtr->flags |= SELECT_TOGGLE;   break;
        }
        if ((anchorPtr != NULL) && (markPtr != NULL)) {
            AddSelectionRange(viewPtr);
        }
    } else {
        RowSelection *selectPtr = &viewPtr->selectRows;
        const char *string;

        selectPtr->flags &= ~SELECT_MASK;
        string = Tcl_GetString(objv[2]);
        switch (string[0]) {
        case 's':
            selectPtr->flags |= SELECT_SET;     break;
        case 'c':
            selectPtr->flags |= SELECT_CLEAR;   break;
        case 't':
            selectPtr->flags |= SELECT_TOGGLE;   break;
        }
        SelectRows(viewPtr, anchorPtr->rowPtr, markPtr->rowPtr);
        selectPtr->flags &= ~SELECT_MASK;
        selectPtr->anchorPtr = anchorPtr->rowPtr;
        selectPtr->markPtr = markPtr->rowPtr;
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
 *      This procedure handles the individual options for text selections.
 *      The selected text is designated by start and end indices into the text
 *      pool.  The selected segment has both a anchored and unanchored ends.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName selection op args
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec selectionOps[] =
{
    {"anchor",   1, SelectionAnchorOp,   4, 4, "cellName",},
    {"clear",    5, SelectionSetOp,      5, 5, "anchorCell markCell",},
    {"clearall", 6, SelectionClearallOp, 3, 3, "",},
    {"export",   1, SelectionExportOp,   3, 3, "",},
    {"includes", 1, SelectionIncludesOp, 4, 4, "cellName",},
    {"mark",     1, SelectionMarkOp,     4, 4, "cellName",},
    {"present",  1, SelectionPresentOp,  3, 3, "",},
    {"set",      1, SelectionSetOp,      5, 5, "anchorCell markCell",},
    {"toggle",   1, SelectionSetOp,      5, 5, "anchorCell markCell",},
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
 *      This procedure is called to process a list of configuration
 *      options database, in order to reconfigure the one of more
 *      entries in the widget.
 *
 *        pathName sort configure option value
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
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
 *      Comparison routine (used by qsort) to sort a chain of subnodes.
 *      A simple string comparison is performed on each node name.
 *
 *      pathName sort auto
 *      pathName sort once -recurse root
 *
 * Results:
 *      1 is the first is greater, -1 is the second is greater, 0
 *      if equal.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec sortOps[] =
{
    {"auto",      1, SortAutoOp,      3, 4, "?boolean?",},
    {"cget",      2, SortCgetOp,      4, 4, "option",},
    {"configure", 2, SortConfigureOp, 3, 0, "?option value ...?",},
    {"once",      1, SortOnceOp,      3, 3, "",},
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
 *        pathName style apply styleName cellName ...
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
            cellPtr->flags |= GEOMETRY; /* Assume that the new style
                                         * changes the geometry of the
                                         * cell. */
            viewPtr->flags |= (LAYOUT_PENDING | GEOMETRY);
            EventuallyRedraw(viewPtr);
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
 *        pathName style cget "styleName" -background
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
 *      This procedure is called to process a list of configuration options
 *      database, in order to reconfigure a style.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font, etc. get
 *      set for stylePtr; old resources get freed, if there were any.
 *
 *      pathName style configure styleName ?option value?..
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
 *        pathName style create combobox "styleName" -background blue
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
    } else if ((c == 'p') && (strncmp(string, "pushbutton", length) == 0)) {
        type = STYLE_PUSHBUTTON;
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
 *      Eliminates one or more style names.  A style still may be in use after
 *      its name has been officially removed.  Only its hash table entry is
 *      removed.  The style itself remains until its reference count returns
 *      to zero (i.e. no one else is using it).
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 *      pathName style forget styleName ...
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
        if (stylePtr == viewPtr->stylePtr) {
            continue;                   /* Can't delete fallback style. */
        }
        /* 
         * Removing the style from the hash tables frees up the style name
         * again.  The style itself may not be removed until it's been
         * released by everything using it.
         */
        if (stylePtr->hashPtr != NULL) {
            Blt_DeleteHashEntry(&viewPtr->styleTable, stylePtr->hashPtr);
            stylePtr->hashPtr = NULL;
            stylePtr->name = NULL;      /* Name points to hash key. */
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
 *      pathName style exists $name
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
 *      A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 *      pathName style get cell
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
 *      Lists the names of all the current styles in the tableview widget.
 *
 *        pathName style names
 *
 * Results:
 *      Always TCL_OK.
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
            continue;                   /* Style has been deleted, but is
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
 *        pathName style type styleName
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
 *      pathName style apply 
 *      pathName style cget styleName -foreground
 *      pathName style configure styleName -fg blue -bg green
 *      pathName style create type styleName ?options?
 *      pathName style delete styleName
 *      pathName style get cellName
 *      pathName style names ?pattern?
 *      pathName style type styleName 
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec styleOps[] = {
    {"apply",     1, StyleApplyOp,     4, 0, "styleName cellName...",},
    {"cget",      2, StyleCgetOp,      5, 5, "styleName option",},
    {"configure", 2, StyleConfigureOp, 4, 0, "styleName options...",},
    {"create",    2, StyleCreateOp,    5, 0, "type styleName options...",},
    {"delete",    1, StyleDeleteOp,    3, 0, "styleName...",},
    {"exists",    1, StyleExistsOp,    4, 4, "styleName",},
    {"get",       1, StyleGetOp,       4, 4, "cellName",},
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
 *      A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 *      pathName type cell
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
 *      pathName updates false
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
 *        pathName writable cell
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
 * pathName op 
 */
static Blt_OpSpec viewOps[] =
{
    {"activate",     1, ActivateOp,      3, 3, "cellName"},
    {"bbox",         2, BboxOp,          3, 0, "cellName ?switches ...?",}, 
    {"bind",         2, BindOp,          3, 5, "cellName ?sequence command?",}, 
    {"cell",         2, CellOp,          2, 0, "args",}, 
    {"cget",         2, CgetOp,          3, 3, "option",}, 
    {"column",       3, ColumnOp,        2, 0, "oper args",}, 
    {"configure",    3, ConfigureOp,     2, 0, "?option value ...?",},
    {"curselection", 2, CurselectionOp,  2, 2, "",},
    {"deactivate",   1, DeactivateOp,    2, 2, ""},
    {"filter",       3, FilterOp,        2, 0, "args",},
    {"find",         3, FindOp,          2, 0, "expr",}, 
    {"focus",        2, FocusOp,         2, 3, "?cellName?",}, 
    {"grab",         1, GrabOp,          2, 3, "?cellName?",}, 
    {"highlight",    1, HighlightOp,     3, 3, "cellName",}, 
    {"identify",     2, IdentifyOp,      5, 5, "cellName x y",}, 
    {"index",        3, IndexOp,         3, 3, "cellName",}, 
    {"inside",       3, InsideOp,        5, 5, "cellName x y",}, 
    {"invoke",       3, InvokeOp,        3, 3, "cellName",}, 
    {"ishidden",     2, IsHiddenOp,      3, 3, "cellName",},
    {"row",          1, RowOp,           2, 0, "oper args",}, 
    {"scan",         2, ScanOp,          5, 5, "dragto|mark x y",},
    {"see",          3, SeeOp,           3, 3, "cellName",},
    {"selection",    3, SelectionOp,     2, 0, "oper args",},
    {"sort",         2, SortOp,          2, 0, "args",},
    {"style",        2, StyleOp,         2, 0, "args",},
    {"type",         1, TypeOp,          3, 3, "cellName",},
    {"unhighlight",  3, HighlightOp,     3, 3, "cellName",}, 
    {"updates",      2, UpdatesOp,       2, 3, "?bool?",},
    {"writable",     1, WritableOp,      3, 3, "cellName",},
    {"xview",        1, XViewOp,         2, 5, "?moveto fract? ?scroll number what?",},
    {"yview",        1, YViewOp,         2, 5, "?moveto fract? ?scroll number what?",},
};

static int numViewOps = sizeof(viewOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * TableViewInstObjCmdProc --
 *
 *      This procedure is invoked to process commands on behalf of the
 *      tableview widget.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
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
 *      This procedure is invoked when a widget command is deleted.  If the
 *      widget isn't already in the process of being destroyed, this
 *      command destroys it.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The widget is destroyed.
 *
 *---------------------------------------------------------------------------
 */
static void
TableViewInstCmdDeleteProc(ClientData clientData)
{
    TableView *viewPtr = clientData;

    /*
     * This procedure could be called either because the tableview window
     * was destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
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
ComputeCellGeometry(Cell *cellPtr)
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
    Column *colPtr;
    Row *rowPtr;
    long i;

    viewPtr->flags &= ~GEOMETRY;        
    viewPtr->rowTitleWidth = viewPtr->colTitleHeight = 0;

    /* Step 1. Set the initial size of the row or column by computing its
     *         title size. Get the geometry of hidden rows and columns so
     *         that it doesn't cost to show/hide them. */
    for (i = 0, colPtr = viewPtr->colHeadPtr; colPtr != NULL;
         colPtr = colPtr->nextPtr, i++) {
        if (colPtr->flags & GEOMETRY) {
            if (viewPtr->flags & COLUMN_TITLES) {
                ComputeColumnTitleGeometry(viewPtr, colPtr);
            } else {
                colPtr->titleWidth = colPtr->titleHeight = 0;
            }
        }
        colPtr->index = i;
        colPtr->nom = colPtr->titleWidth;
        if ((colPtr->flags & HIDDEN) == 0) {
            if (colPtr->titleHeight > viewPtr->colTitleHeight) {
                viewPtr->colTitleHeight = colPtr->titleHeight;
            }
        }
    }
    for (i = 0, rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL;
         rowPtr = rowPtr->nextPtr, i++) {
        if (rowPtr->flags & GEOMETRY) {
            if (viewPtr->flags & ROW_TITLES) {
                ComputeRowTitleGeometry(viewPtr, rowPtr);
            } else {
                rowPtr->titleHeight = rowPtr->titleWidth = 0;
            }
        }
        rowPtr->index = i;
        rowPtr->nom = rowPtr->titleHeight;
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
            ComputeCellGeometry(cellPtr);
        }
        /* Override the initial width of the cell if it exceeds the
         * designated maximum.  */
        if ((viewPtr->maxColWidth > 0) && 
            (cellPtr->width > viewPtr->maxColWidth)) {
            cellPtr->width = viewPtr->maxColWidth;
        }
        if (cellPtr->width > colPtr->nom) {
            colPtr->nom = cellPtr->width;
        }
        /* Override the initial height of the cell if it exceeds the
         * designated maximum.  */
        if ((viewPtr->maxRowHeight > 0) && 
            (cellPtr->height > viewPtr->maxRowHeight)) {
            cellPtr->height = viewPtr->maxRowHeight;
        }
        if (cellPtr->height > rowPtr->nom) {
            rowPtr->nom = cellPtr->height;
        }
    }
    if (viewPtr->flags & COLUMN_FILTERS) {
        ComputeColumnFiltersGeometry(viewPtr);
    }
    viewPtr->flags |= LAYOUT_PENDING;
}

static void
ComputeLayout(TableView *viewPtr)
{
    unsigned long x, y;
    long i;
    Column *colPtr;
    Row *rowPtr;

    viewPtr->flags &= ~LAYOUT_PENDING;
    x = y = 0;
    for (i = 0, rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL; 
         rowPtr = rowPtr->nextPtr, i++) {
        rowPtr->flags &= ~GEOMETRY;     /* Always remove the geometry
                                         * flag. */
        rowPtr->index = i;              /* Reset the index. */
        rowPtr->height = GetBoundedHeight(rowPtr->nom, &rowPtr->reqHeight);
        if (rowPtr->reqHeight.flags & LIMITS_SET_NOM) {
            /*
             * This could be done more cleanly.  We want to ensure that the
             * requested nominal size is not overridden when determining
             * the normal sizes.  So temporarily fix min and max to the
             * nominal size and reset them back later.
             */
            rowPtr->min = rowPtr->max = rowPtr->height;
        } else {
            /* The range defaults to 0..MAXINT */
            rowPtr->min = rowPtr->reqHeight.min;
            rowPtr->max = rowPtr->reqHeight.max;
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

    for (i = 0, colPtr = viewPtr->colHeadPtr; colPtr != NULL; 
         colPtr = colPtr->nextPtr, i++) {

        colPtr->flags &= ~GEOMETRY;     /* Always remove the geometry
                                         * flag. */
        colPtr->index = i;              /* Reset the index. */
        colPtr->width = 0;
        colPtr->worldX = x;
        colPtr->width = GetBoundedWidth(colPtr->nom, &colPtr->reqWidth);
        if (colPtr->reqWidth.flags & LIMITS_SET_NOM) {
            /*
             * This could be done more cleanly.  We want to ensure that the
             * requested nominal size is not overridden when determining
             * the normal sizes.  So temporarily fix min and max to the
             * nominal size and reset them back later.
             */
            colPtr->min = colPtr->max = colPtr->width;
        } else {
            /* The range defaults to 0..MAXINT */
            colPtr->min = colPtr->reqWidth.min;
            colPtr->max = colPtr->reqWidth.max;
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
    viewPtr->width  = viewPtr->worldWidth  = x;
    viewPtr->width  += 2 * viewPtr->inset;
    viewPtr->height += 2 * viewPtr->inset;
    if (viewPtr->flags & COLUMN_TITLES) {
        viewPtr->height += viewPtr->colTitleHeight;
    }
    if (viewPtr->flags & COLUMN_FILTERS) {
        viewPtr->height += viewPtr->colFilterHeight;
    }
    if (viewPtr->flags & ROW_TITLES) {
        viewPtr->width += viewPtr->rowTitleWidth;
    }
    viewPtr->flags |= SCROLL_PENDING;   /* Flag to recompute visible rows
                                         * and columns. */
}

static void
ReorderVisibleIndices(TableView *viewPtr)
{
    size_t count;
    Row *rowPtr;

    /* Reorder visible indices. */
    count = 0;
    for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL;
         rowPtr = rowPtr->nextPtr) {
        if ((rowPtr->flags & HIDDEN) == 0) {
            rowPtr->visibleIndex = count;
            count++;
        }
    }
}

static void
ComputeVisibleEntries(TableView *viewPtr)
{
    unsigned int  viewWidth, viewHeight;
    unsigned long xOffset, yOffset;
    long numVisibleRows, numVisibleColumns, i, j;
    long low, high;
    long first, last;

    if (viewPtr->flags & REINDEX_ROWS) {
        RenumberRows(viewPtr);
    }
    if (viewPtr->flags & REINDEX_COLUMNS) {
        RenumberColumns(viewPtr);
    }
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
    ReorderVisibleIndices(viewPtr);
    first = 0, last = -1;
    /* FIXME: Handle hidden rows. */
    /* Find the row that contains the start of the viewport.  */
    low = 0; high = viewPtr->numRows - 1;
    while (low <= high) {
        long mid;
        Row *rowPtr;
        
        mid = (low + high) >> 1;
        rowPtr = viewPtr->rowMap[mid];
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
            
        rowPtr = viewPtr->rowMap[i];
        if (rowPtr->flags & HIDDEN) {
            continue;
        }
        if (rowPtr->worldY >= (viewPtr->yOffset + viewHeight)) {
            break;                      /* Row starts after the end of the
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
            
            rowPtr = viewPtr->rowMap[i];
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
        colPtr = viewPtr->columnMap[mid];
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
        
        colPtr = viewPtr->columnMap[i];
        if (colPtr->flags & HIDDEN) {
            continue;
        }
        if ((colPtr->worldX) >= (viewPtr->xOffset + viewWidth)) {
            break;                      /* Column starts after the end of
                                         * the viewport. */
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
            
            colPtr = viewPtr->columnMap[i];
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
ReorderRows(TableView *viewPtr)
{
    size_t i;
    Row *lastPtr;
    BLT_TABLE_ROW row;

    lastPtr = NULL;
    for (i = 0, row = blt_table_first_row(viewPtr->table); row != NULL; 
         row = blt_table_next_row(row), i++) {
        Row *rowPtr;

        rowPtr = GetRowContainer(viewPtr, row);
        assert(rowPtr != NULL);
        viewPtr->rowMap[i] = rowPtr;
        if (lastPtr != NULL) {
            lastPtr->nextPtr = rowPtr;
        }
        rowPtr->prevPtr = lastPtr;
        lastPtr = rowPtr;
    }
    viewPtr->rowHeadPtr = viewPtr->rowMap[0];
    viewPtr->rowTailPtr = viewPtr->rowMap[i - 1];
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
}

static void
ReorderColumns(TableView *viewPtr)
{
    size_t i;
    Column *lastPtr;
    BLT_TABLE_COLUMN col;

    lastPtr = NULL;
    for (i = 0, col = blt_table_first_column(viewPtr->table); col != NULL; 
         col = blt_table_next_column(col), i++) {
        Column *colPtr;

        colPtr = GetColumnContainer(viewPtr, col);
        assert(colPtr != NULL);
        viewPtr->columnMap[i] = colPtr;
        if (lastPtr != NULL) {
            lastPtr->nextPtr = colPtr;
        }
        colPtr->prevPtr = lastPtr;
        lastPtr = colPtr;
    }
    viewPtr->colHeadPtr = viewPtr->columnMap[0];
    viewPtr->colTailPtr = viewPtr->columnMap[i - 1];
    viewPtr->flags |= LAYOUT_PENDING;
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
    ComputeCellGeometry(cellPtr);
    /* Override the initial width of the cell if it exceeds the designated
     * maximum.  */
    if ((viewPtr->maxColWidth > 0) && 
        (cellPtr->width > viewPtr->maxColWidth)) {
        cellPtr->width = viewPtr->maxColWidth;
    }
    if (cellPtr->width > colPtr->nom) {
        colPtr->nom = cellPtr->width;
    }
    /* Override the initial height of the cell if it exceeds the designated
     * maximum.  */
    if ((viewPtr->maxRowHeight > 0) && 
        (cellPtr->height > viewPtr->maxRowHeight)) {
        cellPtr->height = viewPtr->maxRowHeight;
    }
    if (cellPtr->height > rowPtr->nom) {
        rowPtr->nom = cellPtr->height;
    }
}

static void
AddColumnTitleGeometry(TableView *viewPtr, Column *colPtr)
{
    if (colPtr->flags & GEOMETRY) {
        if (viewPtr->flags & COLUMN_TITLES) {
            ComputeColumnTitleGeometry(viewPtr, colPtr);
        } else {
            colPtr->titleWidth = colPtr->titleHeight = 0;
        }
    }
    colPtr->nom = colPtr->titleWidth;
    if ((colPtr->flags & HIDDEN) == 0) {
        if (colPtr->titleHeight > viewPtr->colTitleHeight) {
            viewPtr->colTitleHeight = colPtr->titleHeight;
        }
    }
    if (viewPtr->flags & COLUMN_FILTERS) {
        ComputeColumnFiltersGeometry(viewPtr);
    }
}

static void
AddRowTitleGeometry(TableView *viewPtr, Row *rowPtr)
{
    if (rowPtr->flags & GEOMETRY) {
        if (viewPtr->flags & ROW_TITLES) {
            ComputeRowTitleGeometry(viewPtr, rowPtr);
        } else {
            rowPtr->titleHeight = rowPtr->titleWidth = 0;
        }
    }
    rowPtr->nom = rowPtr->titleHeight;
    if ((rowPtr->flags & HIDDEN) == 0) {
        if (rowPtr->titleWidth > viewPtr->rowTitleWidth) {
            viewPtr->rowTitleWidth = rowPtr->titleWidth;
        }
    }
    if (viewPtr->flags & COLUMN_FILTERS) {
        ComputeColumnFiltersGeometry(viewPtr);
    }
}

static void
AddRow(TableView *viewPtr, BLT_TABLE_ROW row)
{
    Blt_HashEntry *hPtr;
    Row *rowPtr;
    Column *colPtr;
    int isNew;
    CellKey key;

    hPtr = Blt_CreateHashEntry(&viewPtr->rowTable, (char *)row, &isNew);
    assert(isNew);
    rowPtr = CreateRow(viewPtr, row, hPtr);
    AddRowTitleGeometry(viewPtr, rowPtr);
    key.rowPtr = rowPtr;
    for (colPtr = viewPtr->colHeadPtr; colPtr != NULL; 
         colPtr = colPtr->nextPtr) {
        Cell *cellPtr;
        Blt_HashEntry *h2Ptr;
        int isNew;
        
        key.colPtr = colPtr;
        h2Ptr = Blt_CreateHashEntry(&viewPtr->cellTable, (char *)&key, &isNew);
        assert(isNew);
        cellPtr = NewCell(viewPtr, h2Ptr);
        AddCellGeometry(viewPtr, cellPtr);
        Blt_SetHashValue(h2Ptr, cellPtr);
    }
    viewPtr->flags |= GEOMETRY | LAYOUT_PENDING | REINDEX_ROWS;
    PossiblyRedraw(viewPtr);
}

static void
AddColumn(TableView *viewPtr, BLT_TABLE_COLUMN col)
{
    Blt_HashEntry *hPtr;
    CellKey key;
    Column *colPtr;
    Row *rowPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&viewPtr->columnTable, (char *)col, &isNew);
    assert(isNew);
    colPtr = CreateColumn(viewPtr, col, hPtr);
    AddColumnTitleGeometry(viewPtr, colPtr);
    key.colPtr = colPtr;
    for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL; 
         rowPtr = rowPtr->nextPtr) {
        Cell *cellPtr;
        Blt_HashEntry *h2Ptr;
        int isNew;
        
        key.rowPtr = rowPtr;
        h2Ptr = Blt_CreateHashEntry(&viewPtr->cellTable, (char *)&key, &isNew);
        assert(isNew);
        cellPtr = NewCell(viewPtr, h2Ptr);
        AddCellGeometry(viewPtr, cellPtr);
        Blt_SetHashValue(h2Ptr, cellPtr);
    }
    viewPtr->flags |= GEOMETRY | LAYOUT_PENDING | REINDEX_COLUMNS;
    PossiblyRedraw(viewPtr);
}

static void
DeleteRow(TableView *viewPtr, BLT_TABLE_ROW row)
{
    Row *rowPtr;
    
    rowPtr = GetRowContainer(viewPtr, row);
    assert(rowPtr);
    RemoveRowCells(viewPtr, rowPtr);
    DestroyRow(rowPtr);
    viewPtr->flags |= LAYOUT_PENDING | REINDEX_ROWS;
    EventuallyRedraw(viewPtr);
}

static void
DeleteColumn(TableView *viewPtr, BLT_TABLE_COLUMN col)
{
    Column *colPtr;
    
    colPtr = GetColumnContainer(viewPtr, col);
    assert(colPtr);
    RemoveColumnCells(viewPtr, colPtr);
    DestroyColumn(colPtr);
    viewPtr->flags |= LAYOUT_PENDING | REINDEX_COLUMNS;
    EventuallyRedraw(viewPtr);
}

static int
ReplaceTable(TableView *viewPtr, BLT_TABLE table)
{
    Column **columnMap;
    Row **rowMap;
    Column *colPtr;
    long i;
    size_t oldSize, newSize, numColumns, numRows;
    unsigned int flags;

    /* Step 1: Cancel any pending idle callbacks for this table. */
    if (viewPtr->flags & SELECT_PENDING) {
        Tcl_CancelIdleCall(SelectCommandProc, viewPtr);
    }
    /* 2. Get rid of the arrays of visible rows and columns. */
    if (viewPtr->visibleRows != NULL) {
        Blt_Free(viewPtr->visibleRows);
        viewPtr->visibleRows = NULL;
    }
    if (viewPtr->visibleColumns != NULL) {
        Blt_Free(viewPtr->visibleColumns);
        viewPtr->visibleColumns = NULL;
    }
    viewPtr->numVisibleRows = viewPtr->numVisibleColumns = 0;
    ClearSelections(viewPtr);

    /* 3. Allocate a map big enough for all columns.  Worst case is oldSize
     * + newSize. */
    oldSize = viewPtr->numColumns;
    newSize = blt_table_num_columns(table);
    numColumns = newSize;
    if (viewPtr->flags & AUTO_COLUMNS)  {
        numColumns += oldSize;
    }
    columnMap = Blt_Calloc(numColumns, sizeof(Column *));
    if (columnMap == NULL) {
        return TCL_ERROR;
    }
    /* Puts the sticky columns in the map first.  This will retain their
     * original locations. */
    for (colPtr = viewPtr->colHeadPtr; colPtr != NULL; 
         colPtr = colPtr->nextPtr) {
        if (colPtr->flags & STICKY) {
            assert(columnMap[colPtr->index] == NULL);
            columnMap[colPtr->index] = colPtr;
            viewPtr->columnMap[colPtr->index] = NULL;
        }
    }
    /* Next add columns from the new table, that already have a column. */
    if (viewPtr->flags & AUTO_COLUMNS) {
        BLT_TABLE_COLUMN col;
        long i, j;

        for (col = blt_table_first_column(table); col != NULL; 
             col = blt_table_next_column(col)) {
            BLT_TABLE_COLUMN oldCol;
            const char *label;

            label = blt_table_column_label(col);
            /* Does this column label exist in the old table? */
            oldCol = blt_table_get_column_by_label(viewPtr->table, label);
            if (oldCol != NULL) {
                Blt_HashEntry *hPtr;
                int isNew;

                /* Get the column container and replace its column
                 * reference and hash with a new entry. */
                colPtr = GetColumnContainer(viewPtr, oldCol);

                /* Replace the previous hash entry with a new one. */
                hPtr = Blt_CreateHashEntry(&viewPtr->columnTable, 
                        (char *)oldCol, &isNew);
                assert(isNew);
                if (colPtr->hashPtr != NULL) {
                    Blt_DeleteHashEntry(&viewPtr->columnTable, colPtr->hashPtr);
                }
                colPtr->hashPtr = hPtr;
                colPtr->column = col;
                viewPtr->columnMap[colPtr->index] = NULL;
                columnMap[colPtr->index] = colPtr;
            }
        }

        /* 5. New fill in the map with columns from the new table that
         * weren't in the old. */
        for (i = 0, col = blt_table_first_column(table); col != NULL;  
             col = blt_table_next_column(col)) {
            Blt_HashEntry *hPtr;
            Column *colPtr;
            int isNew;
            
            hPtr = Blt_CreateHashEntry(&viewPtr->columnTable, (char *)col, 
                                       &isNew);
            if (!isNew) {
                continue;               /* Handled in the previous
                                         * step.  */
            }
            colPtr = CreateColumn(viewPtr, col, hPtr);
            while (columnMap[i] != NULL) { /* Find the next open slot. */
                i++;                        
            }
            columnMap[i] = colPtr;
        }

        /* 6. Find any enpty slots and remove them. */
        for (i = j = 0; i < numColumns; i++) {
            if (columnMap[i] == NULL) {
                continue;
            }
            j++;
            if (i < j) {
                columnMap[j] = columnMap[i];
            }
            columnMap[j]->index = j;
        }
        numColumns = j;
        columnMap = Blt_Realloc(columnMap, numColumns * sizeof(Column *));
    }

    /* 7. Go through the old map and remove any left over columns that are
     * not in the new table. */
    for (i = 0; i < viewPtr->numColumns; i++) {
        Column *colPtr;

        colPtr = viewPtr->columnMap[i];
        if (colPtr != NULL) {
            DestroyColumn(colPtr);
        }
    }
    if (viewPtr->columnMap != NULL) {
        Blt_Free(viewPtr->columnMap);
    }

    RethreadColumns(viewPtr);

    /* 8. Allocate a new row array that can hold all the rows. */
    oldSize = viewPtr->numRows;
    newSize = blt_table_num_rows(table);
    numRows = (viewPtr->flags & AUTO_ROWS) ? MAX(oldSize, newSize) : newSize;
    rowMap = Blt_Calloc(numRows, sizeof(Row *));
    if (rowMap == NULL) {
        return TCL_ERROR;
    }

    if (viewPtr->flags & AUTO_ROWS) {
        BLT_TABLE_ROW row;
        long i, j;

        /* 9. Move rows that exist in both the old and new tables into the
         *    merge array. */
        for (i = 0; i < viewPtr->numRows; i++) {
            BLT_TABLE_ROW newRow;
            Row *rowPtr;
            const char *label;

            rowPtr = viewPtr->rowMap[i];
            label = blt_table_row_label(rowPtr->row);
            newRow = blt_table_get_row_by_label(table, label);
            if (newRow != NULL) {
                Blt_HashEntry *hPtr;
                int isNew;

                hPtr = Blt_CreateHashEntry(&viewPtr->rowTable, (char *)newRow, 
                                           &isNew);
                assert(isNew);
                if (rowPtr->hashPtr != NULL) {
                    Blt_DeleteHashEntry(&viewPtr->rowTable, rowPtr->hashPtr);
                }
                rowPtr->hashPtr = hPtr;
                rowPtr->row = newRow;
                rowMap[i] = rowPtr;
                viewPtr->rowMap[i] = NULL;
            }
        }
        /* 10. Add rows from the the new table that don't already exist. */
        for (i = 0, row = blt_table_first_row(table); row != NULL;  
             row = blt_table_next_row(row)) {
            Blt_HashEntry *hPtr;
            Row *rowPtr;
            int isNew;

            hPtr = Blt_CreateHashEntry(&viewPtr->rowTable, (char *)row, &isNew);
            if (!isNew) {
                /* This works because we're matching against the row
                 * pointer not the row label.  */
                continue;
            }
            rowPtr = CreateRow(viewPtr, row, hPtr);
            while (rowMap[i] != NULL) {    /* Get the next open slot. */
                i++;                        
            }
            rowMap[i] = rowPtr;
        }
        /* 11. Compress empty slots in row array. Re-number the row
         *     indices. */
        for (i = j = 0; i < numRows; i++) {
            if (rowMap[i] == NULL) {
                continue;
            }
            j++;
            if (i < j) {
                rowMap[j] = rowMap[i];
            }
            rowMap[j]->index = j;
        }
        numRows = j;
        rowMap = Blt_Realloc(rowMap, numRows * sizeof(Row *));
    }

    /* 12. Remove all non-NULL rows. These are rows from the old table, not
     *     used in the new table. */
    for (i = 0; i < viewPtr->numRows; i++) {
        Row *rowPtr;

        rowPtr = viewPtr->rowMap[i];
        if (rowPtr != NULL) {
            DestroyRow(rowPtr);
        }
    }
    if (viewPtr->rowMap != NULL) {
        Blt_Free(viewPtr->rowMap);
    }
    viewPtr->rowMap = rowMap;
    viewPtr->numRows = numRows;

    RethreadRows(viewPtr);

    /* 13. Create cells */
    for (i = 0; i < viewPtr->numRows; i++) {
        CellKey key;
        long j;
        
        key.rowPtr = viewPtr->rowMap[i];
        for (j = 0; j < viewPtr->numColumns; j++) {
            Blt_HashEntry *hPtr;
            Cell *cellPtr;
            int isNew;
            
            key.colPtr = viewPtr->columnMap[j];
            hPtr = Blt_CreateHashEntry(&viewPtr->cellTable, (char *)&key, 
                &isNew);
            if (isNew) {
                cellPtr = NewCell(viewPtr, hPtr);
                Blt_SetHashValue(hPtr, cellPtr);
            }
        }
    }

    viewPtr->table = table;
    flags = TABLE_TRACE_FOREIGN_ONLY | TABLE_TRACE_WRITES | TABLE_TRACE_UNSETS;
    blt_table_trace_row(table, TABLE_TRACE_ALL_ROWS, flags, 
        RowTraceProc, NULL, viewPtr);
    blt_table_trace_column(table, TABLE_TRACE_ALL_COLUMNS, flags, 
        ColumnTraceProc, NULL, viewPtr);
    return TCL_OK;
}


static int
AttachTable(Tcl_Interp *interp, TableView *viewPtr)
{
    Row *rowPtr;
    unsigned int flags;

    if (viewPtr->flags & SELECT_PENDING) {
        Tcl_CancelIdleCall(SelectCommandProc, viewPtr);
    }

    /* Try to match the current rows and columns in the view with the new
     * table names. This is so that we can keep various configuration
     * options that might have been set. */

    ResetTableView(viewPtr);
    viewPtr->colNotifier = blt_table_create_column_notifier(interp, 
        viewPtr->table, NULL, TABLE_NOTIFY_ALL_EVENTS, 
        TableEventProc, NULL, viewPtr);
    viewPtr->rowNotifier = blt_table_create_row_notifier(interp, 
        viewPtr->table, NULL, TABLE_NOTIFY_ALL_EVENTS, 
        TableEventProc, NULL, viewPtr);
    viewPtr->numRows = viewPtr->numColumns = 0;
    /* Rows. */
    if (viewPtr->flags & AUTO_ROWS) {
        BLT_TABLE_ROW row;
        size_t i, numRows;

        numRows = blt_table_num_rows(viewPtr->table);
        viewPtr->rowMap = Blt_Malloc(numRows * sizeof(Row *));
        if (viewPtr->rowMap == NULL) {
            return TCL_ERROR;
        }
        for (i = 0, row = blt_table_first_row(viewPtr->table); row != NULL;  
             row = blt_table_next_row(row), i++) {
            Blt_HashEntry *hPtr;
            int isNew;
            Row *rowPtr;
            
            hPtr = Blt_CreateHashEntry(&viewPtr->rowTable, (char *)row, &isNew);
            assert(isNew);
            rowPtr = CreateRow(viewPtr, row, hPtr);
            viewPtr->rowMap[i] = rowPtr;
        }
        assert(i == viewPtr->numRows);
    }
    /* Columns. */
    if (viewPtr->flags & AUTO_COLUMNS) {
        BLT_TABLE_COLUMN col;
        size_t i, numColumns;

        numColumns = blt_table_num_columns(viewPtr->table);
        viewPtr->columnMap = Blt_Malloc(numColumns *sizeof(Column *));
        if (viewPtr->columnMap == NULL) {
            if (viewPtr->rowMap != NULL) {
                Blt_Free(viewPtr->rowMap);
                viewPtr->rowMap = NULL;
            }
            return TCL_ERROR;
        }
        for (i = 0, col = blt_table_first_column(viewPtr->table); col != NULL;  
             col = blt_table_next_column(col), i++) {
            Blt_HashEntry *hPtr;
            int isNew;
            Column *colPtr;
            
            hPtr = Blt_CreateHashEntry(&viewPtr->columnTable, (char *)col,
                &isNew);
            assert(isNew);
            colPtr = CreateColumn(viewPtr, col, hPtr);
            Blt_SetHashValue(hPtr, colPtr);
            viewPtr->columnMap[i] = colPtr;
        }
        assert(i == viewPtr->numColumns);
    }
    /* Create cells */
    for (rowPtr = viewPtr->rowHeadPtr; rowPtr != NULL;
         rowPtr = rowPtr->nextPtr) {
        CellKey key;
        Column *colPtr;
        
        key.rowPtr = rowPtr;
        for (colPtr = viewPtr->colHeadPtr; colPtr != NULL;
             colPtr = colPtr->nextPtr) {
            Cell *cellPtr;
            Blt_HashEntry *hPtr;
            int isNew;
            
            key.colPtr = colPtr;
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
 *      This procedure is invoked to display the widget.
 *
 *      Recompute the layout of the text if necessary. This is necessary if
 *      the world coordinate system has changed.  Specifically, the
 *      following may have occurred:
 *
 *        1.  a text attribute has changed (font, linespacing, etc.).
 *        2.  an entry's option changed, possibly resizing the entry.
 *
 *      This is deferred to the display routine since potentially many of
 *      these may occur.
 *
 *      Set the vertical and horizontal scrollbars.  This is done here
 *      since the window width and height are needed for the scrollbar
 *      calculations.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The widget is redisplayed.
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
        return;                         /* Window has been destroyed. */
    }
#ifdef notdef
    fprintf(stderr, "DisplayProc %s\n", Tk_PathName(viewPtr->tkwin));
#endif
    if (viewPtr->flags & REINDEX_ROWS) {
        RenumberRows(viewPtr);
    }
    if (viewPtr->flags & REINDEX_COLUMNS) {
        RenumberColumns(viewPtr);
    }
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
        /* Draw each cell in the row. */
        for (j = 0; j < viewPtr->numVisibleColumns; j++) {
            Column *colPtr;
            Cell *cellPtr;

            colPtr = viewPtr->visibleColumns[j];
            cellPtr = GetCell(viewPtr, rowPtr, colPtr);
            assert(cellPtr != NULL);
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
        if ((viewPtr->rowTitleWidth > 0) && (viewPtr->colTitleHeight > 0)) {
            Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, 
                viewPtr->colNormalTitleBg, viewPtr->inset, viewPtr->inset, 
                viewPtr->rowTitleWidth, viewPtr->colTitleHeight, 
                viewPtr->colTitleBorderWidth, TK_RELIEF_RAISED);
        }
        if ((viewPtr->rowTitleWidth > 0) && (viewPtr->colFilterHeight > 0)) {
            Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, 
                viewPtr->colNormalTitleBg, viewPtr->inset, 
                viewPtr->inset + viewPtr->colTitleHeight, 
                viewPtr->rowTitleWidth, viewPtr->colFilterHeight, 
                viewPtr->colTitleBorderWidth, TK_RELIEF_RAISED);
        }
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
    Blt_InitHashTable(&viewPtr->bindTagTable,
                      sizeof(struct _BindTag)/sizeof(int));
    Blt_InitHashTable(&viewPtr->uidTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&viewPtr->cachedObjTable, BLT_STRING_KEYS);
    Blt_InitHashTableWithPool(&viewPtr->selectCells.cellTable, 
                              sizeof(CellKey)/sizeof(int));
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
 *      This procedure is invoked to process the TCL command that
 *      corresponds to a widget managed by this module. See the user
 *      documentation for details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
TableViewCmdProc(
    ClientData clientData,              /* Main window associated with
                                         * interpreter. */
    Tcl_Interp *interp,                 /* Current interpreter. */
    int objc,                           /* Number of arguments. */
    Tcl_Obj *const *objv)               /* Argument strings. */
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
                Tcl_GetString(objv[0]), " pathName ?option value ...?\"", 
                (char *)NULL);
        return TCL_ERROR;
    }
    /*
     * Invoke a procedure to initialize various bindings on tableview
     * entries.  If the procedure doesn't already exist, source it from
     * "$blt_library/TableView.tcl".  We deferred sourcing the file until
     * now so that the variable $blt_library could be set within a script.
     */
    if (!Blt_CommandExists(interp, "::blt::TableView::Initialize")) {
        if (Tcl_GlobalEval(interp, 
                "source [file join $blt_library bltTableView.tcl]") != TCL_OK) {
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
    if (Blt_ConfigModified(tableSpecs, "-columnfilters", (char *)NULL)) {
        ConfigureFilters(interp, viewPtr);
        if (InitColumnFilters(interp, viewPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }

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

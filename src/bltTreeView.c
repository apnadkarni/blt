/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTreeView.c --
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
 * TODO:
 *
 * BUGS:
 *   1.  "open" operation should change scroll offset so that as many new
 *       entries (up to half a screen) can be seen.
 *   2.  "open" needs to adjust the scrolloffset so that the same entry is
 *       seen at the same place.
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_TREEVIEW

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

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

#include "bltAlloc.h"
#include "bltList.h"
#include "bltSwitch.h"
#include "bltOp.h"
#include "bltInitCmd.h"
#include "bltTreeView.h"

#define BUTTON_IPAD             1
#define BUTTON_PAD              2
#define COLUMN_PAD              2
#define ENTRY_PADX              2
#define ENTRY_PADY              1
#define FOCUS_PAD               3
#define ICON_PADX               2
#define ICON_PADY               1
#define INSET_PAD               0
#define LABEL_PADX              0
#define LABEL_PADY              0
#define ENTRY_GAP               1

#define FCLAMP(x)       ((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))
#define LineWidth(w)    (((w) > 1) ? (w) : 0)

#define CHOOSESTYLE(v,c,e) \
    (((e)->stylePtr != NULL) ? (e)->stylePtr : \
     (((c)->stylePtr != NULL) ? (c)->stylePtr : (v)->stylePtr))

#define GetData(entryPtr, key, objPtrPtr) \
        Blt_Tree_GetValueByKey((Tcl_Interp *)NULL, (entryPtr)->viewPtr->tree, \
              (entryPtr)->node, key, objPtrPtr)
#define IsClosed(e)             ((e)->flags & CLOSED)
#define IsOpen(e)               (!IsClosed(e))

#define DEF_ICON_WIDTH          16
#define DEF_ICON_HEIGHT         16

#define RULE_AREA               (8)

#define TAG_UNKNOWN             (1<<0)
#define TAG_RESERVED            (1<<1)
#define TAG_USER_DEFINED        (1<<2)
#define TAG_SINGLE              (1<<3)
#define TAG_MULTIPLE            (1<<4)
#define TAG_ALL                 (1<<5)

#define NodeToObj(n)            Tcl_NewLongObj(Blt_Tree_NodeId(n))

#define DEF_BUTTON_ACTIVE_BG            RGB_WHITE
#define DEF_BUTTON_ACTIVE_BG_MONO       STD_ACTIVE_BG_MONO
#define DEF_BUTTON_ACTIVE_FOREGROUND    STD_ACTIVE_FOREGROUND
#define DEF_BUTTON_ACTIVE_FG_MONO       STD_ACTIVE_FG_MONO
#define DEF_BUTTON_BINDTAGS            "all"
#define DEF_BUTTON_BORDERWIDTH          "1"
#define DEF_BUTTON_CLOSE_RELIEF         "solid"
#define DEF_BUTTON_OPEN_RELIEF          "solid"
#define DEF_BUTTON_NORMAL_BG            RGB_WHITE
#define DEF_BUTTON_NORMAL_BG_MONO       STD_NORMAL_BG_MONO
#define DEF_BUTTON_NORMAL_FOREGROUND    STD_NORMAL_FOREGROUND
#define DEF_BUTTON_NORMAL_FG_MONO       STD_NORMAL_FG_MONO
#define DEF_BUTTON_SIZE                 "0"

#define DEF_CELL_STATE                  "normal"
#define DEF_CELL_STYLE                  (char *)NULL

#ifdef WIN32
  #define DEF_ACTIVE_BG                   RGB_GREY85
#else
  #define DEF_ACTIVE_BG                   RGB_GREY95
#endif

#define DEF_DISABLE_BG                  RGB_GREY97

#ifdef WIN32
  #define DEF_COLUMN_ACTIVE_TITLE_BG      RGB_GREY85
#else
  #define DEF_COLUMN_ACTIVE_TITLE_BG      RGB_GREY90
#endif
#define DEF_COLUMN_ACTIVE_TITLE_FG      STD_ACTIVE_FOREGROUND
#define DEF_COLUMN_ARROWWIDTH           "0"
#define DEF_COLUMN_BG           (char *)NULL
#define DEF_COLUMN_BINDTAGS            "all"
#define DEF_COLUMN_BORDERWIDTH          STD_BORDERWIDTH
#define DEF_COLUMN_COLOR                RGB_BLACK
#define DEF_COLUMN_EDIT                 "yes"
#define DEF_COLUMN_FONT                 STD_FONT
#define DEF_COLUMN_COMMAND              (char *)NULL
#define DEF_COLUMN_FORMATCOMMAND        (char *)NULL
#define DEF_COLUMN_HIDE                 "no"
#define DEF_COLUMN_SHOW                 "yes"
#define DEF_COLUMN_JUSTIFY              "center"
#define DEF_COLUMN_MAX                  "0"
#define DEF_COLUMN_MIN                  "0"
#define DEF_COLUMN_PAD                  "2"
#define DEF_COLUMN_RELIEF               "flat"
#define DEF_COLUMN_STATE                "normal"
#define DEF_COLUMN_STYLE                "default"
#define DEF_COLUMN_TITLE_BG             STD_NORMAL_BACKGROUND
#define DEF_COLUMN_TITLE_BORDERWIDTH    STD_BORDERWIDTH
#define DEF_COLUMN_TITLE_FONT           STD_FONT_NORMAL
#define DEF_COLUMN_TITLE_FOREGROUND     STD_NORMAL_FOREGROUND
#define DEF_COLUMN_TITLE_RELIEF         "raised"
#define DEF_COLUMN_WEIGHT               "1.0"
#define DEF_COLUMN_WIDTH                "0"
#define DEF_COLUMN_RULE_DASHES          "dot"

#define DEF_ENTRY_BINDTAGS      "Entry all"
#define DEF_ENTRY_BUTTON        "auto"
#define DEF_ENTRY_CLOSECOMMAND  (char *)NULL
#define DEF_ENTRY_COMMAND       (char *)NULL
#define DEF_ENTRY_DATA          (char *)NULL
#define DEF_ENTRY_FOREGROUND    (char *)NULL
#define DEF_ENTRY_HEIGHT        (char *)NULL
#define DEF_ENTRY_ICONS         (char *)NULL
#define DEF_ENTRY_LABEL         (char *)NULL
#define DEF_ENTRY_OPENCOMMAND   (char *)NULL
#define DEF_ENTRY_RULE_COLOR    STD_NORMAL_BACKGROUND
#define DEF_ENTRY_RULE_HEIGHT   "0"
#define DEF_ENTRY_STYLES        (char *)NULL

#define DEF_SORT_COLUMN         (char *)NULL
#define DEF_SORT_COMMAND        (char *)NULL
#define DEF_SORT_DECREASING     "no"
#define DEF_SORT_TYPE           "dictionary"

/* RGB_LIGHTBLUE1 */

#define DEF_BG                  RGB_WHITE
#define DEF_ALT_BG              RGB_GREY97
#define DEF_BORDERWIDTH         STD_BORDERWIDTH
#define DEF_BUTTON              "auto"
#define DEF_COLUMNCOMMAND       ((char *)NULL)
#define DEF_DASHES              "dot"
#define DEF_EXPORT_SELECTION    "no"
#define DEF_FLAT                "no"
#define DEF_FOCUS_DASHES        "dot"
#define DEF_FOCUS_FOREGROUND    STD_ACTIVE_FOREGROUND
#define DEF_FOCUS_FG_MONO       STD_ACTIVE_FG_MONO
#define DEF_FONT                STD_FONT_NORMAL
#define DEF_HEIGHT              "400"
#define DEF_HIDE_LEAVES         "no"
#define DEF_HIDE_ROOT           "yes"
#define DEF_FOCUS_HIGHLIGHT_BG  STD_NORMAL_BACKGROUND
#define DEF_FOCUS_HIGHLIGHT_COLOR       RGB_BLACK
#define DEF_FOCUS_HIGHLIGHT_WIDTH       "2"
#define DEF_ICONS               ((char *)NULL)
#define DEF_LINECOLOR           RGB_GREY30
#define DEF_LINECOLOR_MONO      STD_NORMAL_FG_MONO
#define DEF_LINESPACING         "0"
#define DEF_LINEWIDTH           "1"
#define DEF_NEW_TAGS            "no"
#define DEF_RELIEF              "sunken"
#define DEF_RESIZE_CURSOR       "arrow"
#define DEF_RULE_HEIGHT         "0"
#define DEF_RULE_WIDTH          "1"
#define DEF_RULE_COLOR          STD_NORMAL_BACKGROUND
#define DEF_SCROLL_INCREMENT    "20"
#define DEF_SCROLL_MODE         "hierbox"
#define DEF_SELECT_BG           STD_SELECT_BACKGROUND 
#define DEF_SELECT_FOREGROUND   STD_SELECT_FOREGROUND
#define DEF_SELECT_MODE         "single"
#define DEF_RELIEF              "sunken"
#define DEF_SHOW_TITLES         "yes"
#define DEF_SORT_SELECTION      "no"
#define DEF_TAKE_FOCUS          "1"
#define DEF_TEXT_COLOR          STD_NORMAL_FOREGROUND
#define DEF_TEXT_MONO           STD_NORMAL_FG_MONO
#define DEF_TEXTVARIABLE        ((char *)NULL)
#define DEF_TRIMLEFT            ""
#define DEF_WIDTH               "200"

static const char *sortTypeStrings[] = {
    "dictionary", "ascii", "integer", "real", "command", NULL
};

typedef ClientData (TagProc)(TreeView *viewPtr, const char *string);
typedef int (TreeViewApplyProc)(TreeView *viewPtr, Entry *entryPtr);

static CompareProc ExactCompare, GlobCompare, RegexpCompare;
static TreeViewApplyProc ShowEntryApplyProc, HideEntryApplyProc, 
        MapAncestorsApplyProc, FixSelectionsApplyProc;
static Tk_LostSelProc LostSelection;
static TreeViewApplyProc SelectEntryApplyProc;

static Blt_OptionParseProc ObjToIcon;
static Blt_OptionPrintProc IconToObj;
static Blt_OptionFreeProc FreeIconProc;

static Blt_CustomOption iconOption = {
    ObjToIcon, IconToObj, FreeIconProc, NULL,
};

static Blt_OptionParseProc ObjToTree;
static Blt_OptionPrintProc TreeToObj;
static Blt_OptionFreeProc FreeTreeProc;
static Blt_CustomOption treeOption = {
    ObjToTree, TreeToObj, FreeTreeProc, NULL,
};

static Blt_OptionParseProc ObjToIcons;
static Blt_OptionPrintProc IconsToObj;
static Blt_OptionFreeProc FreeIconsProc;
static Blt_CustomOption iconsOption = {
    ObjToIcons, IconsToObj, FreeIconsProc, NULL,
};

static Blt_OptionParseProc ObjToButton;
static Blt_OptionPrintProc ButtonToObj;
static Blt_CustomOption buttonOption = {
    ObjToButton, ButtonToObj, NULL, NULL,
};

static Blt_OptionParseProc ObjToCachedObj;
static Blt_OptionPrintProc CachedObjToObj;
static Blt_OptionFreeProc FreeCachedObjProc;
static Blt_CustomOption cachedObjOption = {
    ObjToCachedObj, CachedObjToObj, FreeCachedObjProc, NULL,
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

static Blt_OptionParseProc ObjToSeparator;
static Blt_OptionPrintProc SeparatorToObj;
static Blt_OptionFreeProc FreeSeparator;
static Blt_CustomOption separatorOption = {
    ObjToSeparator, SeparatorToObj, FreeSeparator, NULL,
};

static Blt_OptionParseProc ObjToLabel;
static Blt_OptionPrintProc LabelToObj;
static Blt_OptionFreeProc FreeLabel;
static Blt_CustomOption labelOption = {
    ObjToLabel, LabelToObj, FreeLabel, NULL,
};

static Blt_OptionParseProc ObjToState;
static Blt_OptionPrintProc StateToObj;
static Blt_CustomOption stateOption = {
    ObjToState, StateToObj, NULL, (ClientData)0
};
static Blt_OptionParseProc ObjToStyles;
static Blt_OptionPrintProc StylesToObj;
static Blt_CustomOption stylesOption = {
    ObjToStyles, StylesToObj, NULL, NULL,
};

static Blt_OptionParseProc ObjToEnum;
static Blt_OptionPrintProc EnumToObj;
static Blt_CustomOption sortTypeOption = {
    ObjToEnum, EnumToObj, NULL, (ClientData)sortTypeStrings
};

static Blt_OptionParseProc ObjToSortMark;
static Blt_OptionPrintProc SortMarkToObj;
static Blt_CustomOption sortMarkOption = {
    ObjToSortMark, SortMarkToObj, (ClientData)0
};

static Blt_OptionParseProc ObjToSortColumns;
static Blt_OptionPrintProc SortColumnsToObj;
static Blt_OptionFreeProc FreeSortColumnsProc;
static Blt_CustomOption sortColumnsOption = {
    ObjToSortColumns, SortColumnsToObj, FreeSortColumnsProc, 
    (ClientData)0
};

static Blt_OptionParseProc ObjToData;
static Blt_OptionPrintProc DataToObj;
static Blt_CustomOption dataOption = {
    ObjToData, DataToObj, NULL, (ClientData)0,
};

static Blt_OptionParseProc ObjToStyle;
static Blt_OptionPrintProc StyleToObj;
static Blt_OptionFreeProc FreeStyleProc;
static Blt_CustomOption styleOption = {
    /* Contains a pointer to the widget that's currently being configured.
     * This is used in the custom configuration parse proc for icons.  */
    ObjToStyle, StyleToObj, FreeStyleProc, NULL,
};

static Blt_ConfigSpec buttonSpecs[] = {
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
        "Background", DEF_BUTTON_ACTIVE_BG,
        Blt_Offset(TreeView, button.activeBg), 0},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground"},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeForeground"},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", "Foreground",
        DEF_BUTTON_ACTIVE_FOREGROUND, 
        Blt_Offset(TreeView, button.activeFgColor), 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BUTTON_NORMAL_BG, Blt_Offset(TreeView, button.normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth"},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
    {BLT_CONFIG_OBJ, "-bindtags", "bindTags", "BindTags",
        DEF_BUTTON_BINDTAGS, Blt_Offset(TreeView, button.bindTagsObjPtr),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_BUTTON_BORDERWIDTH, Blt_Offset(TreeView, button.borderWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-closerelief", "closeRelief", "Relief",
        DEF_BUTTON_CLOSE_RELIEF, Blt_Offset(TreeView, button.closeRelief),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground"},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
        DEF_BUTTON_NORMAL_FOREGROUND, Blt_Offset(TreeView, button.normalFg), 0},
    {BLT_CONFIG_CUSTOM, "-images", "images", "Icons", (char *)NULL, 
        Blt_Offset(TreeView, button.icons), BLT_CONFIG_NULL_OK, &iconsOption},
    {BLT_CONFIG_RELIEF, "-openrelief", "openRelief", "Relief",
        DEF_BUTTON_OPEN_RELIEF, Blt_Offset(TreeView, button.openRelief),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-size", "size", "Size", DEF_BUTTON_SIZE, 
        Blt_Offset(TreeView, button.reqSize), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec cellSpecs[] = {
    {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_CELL_STATE, 
        Blt_Offset(Cell, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        &stateOption},
    {BLT_CONFIG_CUSTOM, "-style", "style", "Style", DEF_CELL_STYLE, 
        Blt_Offset(Cell, stylePtr), BLT_CONFIG_NULL_OK, &styleOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec entrySpecs[] = {
    {BLT_CONFIG_OBJ, "-bindtags", (char *)NULL, (char *)NULL,
        DEF_ENTRY_BINDTAGS, Blt_Offset(Entry, bindTagsObjPtr),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-button", (char *)NULL, (char *)NULL, DEF_ENTRY_BUTTON,
        Blt_Offset(Entry, flags), BLT_CONFIG_DONT_SET_DEFAULT, &buttonOption},
    {BLT_CONFIG_OBJ, "-closecommand", (char *)NULL, (char *)NULL,
        DEF_ENTRY_CLOSECOMMAND, Blt_Offset(Entry, closeCmdObjPtr),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-command", (char *)NULL, (char *)NULL,
        DEF_ENTRY_COMMAND, Blt_Offset(Entry, cmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-data", (char *)NULL, (char *)NULL, DEF_ENTRY_DATA, 0, 
        BLT_CONFIG_NULL_OK, &dataOption},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground"},
    {BLT_CONFIG_FONT, "-font", (char *)NULL, (char *)NULL, (char *)NULL, 
        Blt_Offset(Entry, font), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", (char *)NULL,
        DEF_ENTRY_FOREGROUND, Blt_Offset(Entry, color), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-height", (char *)NULL, (char *)NULL, 
        DEF_ENTRY_HEIGHT, Blt_Offset(Entry, reqHeight), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-icons", (char *)NULL, (char *)NULL, DEF_ENTRY_ICONS, 
        Blt_Offset(Entry, icons), BLT_CONFIG_NULL_OK, &iconsOption},
    {BLT_CONFIG_CUSTOM, "-label", (char *)NULL, (char *)NULL, DEF_ENTRY_LABEL, 
        Blt_Offset(Entry, labelObjPtr), 0, &labelOption},
    {BLT_CONFIG_OBJ, "-opencommand", (char *)NULL, (char *)NULL, 
        DEF_ENTRY_OPENCOMMAND, Blt_Offset(Entry, openCmdObjPtr),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-rulecolor", "ruleColor", "RuleColor",
        DEF_ENTRY_RULE_COLOR,  Blt_Offset(Entry, ruleColor),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-ruleheight", "ruleHeight", "RuleHeight",
        DEF_ENTRY_RULE_HEIGHT, Blt_Offset(Entry, ruleHeight), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-styles", (char *)NULL, (char *)NULL, DEF_ENTRY_STYLES,
        0, BLT_CONFIG_NULL_OK, &stylesOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec viewSpecs[] = {
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
        "ActiveBackground", DEF_ACTIVE_BG, Blt_Offset(TreeView, activeBg), 0},
    {BLT_CONFIG_SYNONYM, "-altbg", "alternateBackground"},
    {BLT_CONFIG_BACKGROUND, "-alternatebackground", "alternateBackground", 
        "AlternateBackground", DEF_ALT_BG, Blt_Offset(TreeView, altBg), 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BG, Blt_Offset(TreeView, normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth"},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_BORDERWIDTH, Blt_Offset(TreeView, borderWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-button", "button", "Button", DEF_BUTTON, 
        Blt_Offset(TreeView, buttonFlags), BLT_CONFIG_DONT_SET_DEFAULT, 
        &buttonOption},
    {BLT_CONFIG_OBJ, "-closecommand", "closeCommand", "CloseCommand",
        (char *)NULL, Blt_Offset(TreeView, closeCmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor", (char *)NULL, 
        Blt_Offset(TreeView, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-columncommand", "columnCommand", "ColumnCommand", 
        DEF_COLUMNCOMMAND, Blt_Offset(TreeView, colCmdObjPtr),
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_DASHES, "-dashes", "dashes", "Dashes",  DEF_DASHES, 
        Blt_Offset(TreeView, dashes), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-disabledbackground", "disabledBackground", 
        "DisabledBackground", DEF_DISABLE_BG, 
        Blt_Offset(TreeView, disabledBg), 0},
    {BLT_CONFIG_OBJ, "-entrycommand", "entryCommand", "EntryCommand",
        (char *)NULL, Blt_Offset(TreeView, entryCmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BITMASK, "-exportselection", "exportSelection",
        "ExportSelection", DEF_EXPORT_SELECTION, 
        Blt_Offset(TreeView, sel.flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)SELECT_EXPORT},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground"},
    {BLT_CONFIG_BITMASK, "-flat", "flat", "Flat", DEF_FLAT, 
        Blt_Offset(TreeView, flags), BLT_CONFIG_DONT_SET_DEFAULT,
       (Blt_CustomOption *)FLAT},
    {BLT_CONFIG_DASHES, "-focusdashes", "focusDashes", "FocusDashes",
        DEF_FOCUS_DASHES, Blt_Offset(TreeView, focusDashes), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-focusforeground", "focusForeground", "FocusForeground",
        DEF_FOCUS_FOREGROUND, Blt_Offset(TreeView, focusColor),
        BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-focusforeground", "focusForeground", "FocusForeground",
        DEF_FOCUS_FG_MONO, Blt_Offset(TreeView, focusColor), 
        BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_FONT, 
        Blt_Offset(TreeView, font), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", 
        DEF_TEXT_COLOR, Blt_Offset(TreeView, normalFg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", 
        DEF_TEXT_MONO, Blt_Offset(TreeView, normalFg), BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height", DEF_HEIGHT, 
        Blt_Offset(TreeView, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-hideleaves", "hideLeaves", "HideLeaves",
        DEF_HIDE_LEAVES, Blt_Offset(TreeView, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)HIDE_LEAVES},
    {BLT_CONFIG_BITMASK, "-hideroot", "hideRoot", "HideRoot", DEF_HIDE_ROOT,
        Blt_Offset(TreeView, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)HIDE_ROOT},
    {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
        "HighlightBackground", DEF_FOCUS_HIGHLIGHT_BG, 
        Blt_Offset(TreeView, highlightBgColor), 0},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
        DEF_FOCUS_HIGHLIGHT_COLOR, Blt_Offset(TreeView, highlightColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
        "HighlightThickness", DEF_FOCUS_HIGHLIGHT_WIDTH, 
        Blt_Offset(TreeView, highlightWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-iconvariable", "iconVariable", "IconVariable", 
        DEF_TEXTVARIABLE, Blt_Offset(TreeView, iconVarObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-icons", "icons", "Icons", DEF_ICONS, 
        Blt_Offset(TreeView, icons), BLT_CONFIG_NULL_OK, &iconsOption},
    {BLT_CONFIG_COLOR, "-linecolor", "lineColor", "LineColor",
        DEF_LINECOLOR, Blt_Offset(TreeView, lineColor), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-linecolor", "lineColor", "LineColor", 
        DEF_LINECOLOR_MONO, Blt_Offset(TreeView, lineColor), 
        BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_PIXELS_NNEG, "-linespacing", "lineSpacing", "LineSpacing",
        DEF_LINESPACING, Blt_Offset(TreeView, leader), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth", 
        DEF_LINEWIDTH, Blt_Offset(TreeView, lineWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-opencommand", "openCommand", "OpenCommand",
        (char *)NULL, Blt_Offset(TreeView, openCmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_RELIEF, 
        Blt_Offset(TreeView, relief), 0},
    {BLT_CONFIG_CURSOR, "-resizecursor", "resizeCursor", "ResizeCursor",
        DEF_RESIZE_CURSOR, Blt_Offset(TreeView, resizeCursor), 0},
    {BLT_CONFIG_CUSTOM, "-scrollmode", "scrollMode", "ScrollMode",
        DEF_SCROLL_MODE, Blt_Offset(TreeView, scrollMode),
        BLT_CONFIG_DONT_SET_DEFAULT, &scrollModeOption},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
        "Foreground", DEF_SELECT_BG, Blt_Offset(TreeView, selectedBg), 0},
    {BLT_CONFIG_OBJ, "-selectcommand", "selectCommand", "SelectCommand",
        (char *)NULL, Blt_Offset(TreeView, sel.cmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
        DEF_SELECT_FOREGROUND, Blt_Offset(TreeView, selectedFg), 0},
    {BLT_CONFIG_CUSTOM, "-selectmode", "selectMode", "SelectMode",
        DEF_SELECT_MODE, Blt_Offset(TreeView, sel.mode), 
        BLT_CONFIG_DONT_SET_DEFAULT, &selectModeOption},
    {BLT_CONFIG_CUSTOM, "-separator", "separator", "Separator", (char *)NULL, 
        Blt_Offset(TreeView, pathSep), BLT_CONFIG_NULL_OK, &separatorOption},
    {BLT_CONFIG_BITMASK, "-newtags", "newTags", "newTags", DEF_NEW_TAGS, 
        Blt_Offset(TreeView, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)TV_NEW_TAGS},
    {BLT_CONFIG_BITMASK, "-showtitles", "showTitles", "ShowTitles",
        DEF_SHOW_TITLES, Blt_Offset(TreeView, flags), 0,
        (Blt_CustomOption *)SHOW_COLUMN_TITLES},
    {BLT_CONFIG_BITMASK, "-sortselection", "sortSelection", "SortSelection",
        DEF_SORT_SELECTION, Blt_Offset(TreeView, sel.flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SELECT_SORTED},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
        DEF_TAKE_FOCUS, Blt_Offset(TreeView, takeFocus), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-textvariable", "textVariable", "TextVariable", 
        DEF_TEXTVARIABLE, Blt_Offset(TreeView, textVarObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-tree", "tree", "Tree", (char *)NULL, 
        Blt_Offset(TreeView, treeName), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-trim", "trim", "Trim", DEF_TRIMLEFT, 
        Blt_Offset(TreeView, trimLeft), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width", DEF_WIDTH, 
        Blt_Offset(TreeView, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
        (char *)NULL, Blt_Offset(TreeView, xScrollCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-xscrollincrement", "xScrollIncrement", 
        "ScrollIncrement", DEF_SCROLL_INCREMENT, 
        Blt_Offset(TreeView, xScrollUnits), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
        (char *)NULL, Blt_Offset(TreeView, yScrollCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-yscrollincrement", "yScrollIncrement", 
        "ScrollIncrement", DEF_SCROLL_INCREMENT, 
        Blt_Offset(TreeView, yScrollUnits), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};


static Blt_ConfigSpec columnSpecs[] = {
    {BLT_CONFIG_BACKGROUND, "-activetitlebackground", "activeTitleBackground", 
        "Background", DEF_COLUMN_ACTIVE_TITLE_BG, 
        Blt_Offset(Column, activeTitleBg), 0},
    {BLT_CONFIG_COLOR, "-activetitleforeground", "activeTitleForeground", 
        "Foreground", DEF_COLUMN_ACTIVE_TITLE_FG, 
        Blt_Offset(Column, activeTitleFgColor), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth"},
    {BLT_CONFIG_OBJ, "-bindtags", "bindTags", "BindTags",
        DEF_COLUMN_BINDTAGS, Blt_Offset(Column, bindTagsObjPtr),
        BLT_CONFIG_NULL_OK},
     {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_COLUMN_BORDERWIDTH, Blt_Offset(Column, borderWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-command", "command", "Command",
        DEF_COLUMN_COMMAND, Blt_Offset(Column, cmdObjPtr),
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_BITMASK_INVERT, "-edit", "edit", "Edit", DEF_COLUMN_STATE, 
        Blt_Offset(Column, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)COLUMN_READONLY},
    {BLT_CONFIG_CUSTOM, "-decreasingicon", "decreasingIcon", "DecreasingIcon", 
        (char *)NULL, Blt_Offset(Column, sortDown), 
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &iconOption},
    {BLT_CONFIG_OBJ, "-formatcommand", "formatCommand", "FormatCommand",
        (char *)NULL, Blt_Offset(Column, fmtCmdObjPtr), 
        BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_COLUMN_HIDE, 
        Blt_Offset(Column, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "icon", (char *)NULL, 
        Blt_Offset(Column, titleIcon),
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &iconOption},
    {BLT_CONFIG_CUSTOM, "-increasingicon", "increasingIcon", "IncreasingIcon", 
        (char *)NULL, Blt_Offset(Column, sortUp), 
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &iconOption},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_COLUMN_JUSTIFY, 
        Blt_Offset(Column, justify), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-max", "max", "Max", DEF_COLUMN_MAX, 
        Blt_Offset(Column, reqMax), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-min", "min", "Min", DEF_COLUMN_MIN, 
        Blt_Offset(Column, reqMin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-pad", "pad", "Pad", DEF_COLUMN_PAD, 
        Blt_Offset(Column, pad), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_COLUMN_RELIEF, 
        Blt_Offset(Column, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DASHES, "-ruledashes", "ruleDashes", "RuleDashes",
        DEF_COLUMN_RULE_DASHES, Blt_Offset(Column, ruleDashes),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-rulecolor", "ruleColor", "RuleColor", DEF_RULE_COLOR,
         Blt_Offset(Column, ruleColor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-rulewidth", "ruleWidth", "RuleWidth",
        DEF_RULE_WIDTH, Blt_Offset(Column, ruleWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK_INVERT, "-show", "show", "Show", DEF_COLUMN_SHOW, 
        Blt_Offset(Column, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_OBJ, "-sortcommand", "sortCommand", "SortCommand",
        DEF_SORT_COMMAND, Blt_Offset(Column, sortCmdObjPtr), 
        BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_CUSTOM, "-sorttype", "sortType", "SortType", DEF_SORT_TYPE, 
        Blt_Offset(Column, sortType), BLT_CONFIG_DONT_SET_DEFAULT, 
        &sortTypeOption},
    {BLT_CONFIG_STATE, "-state", "state", "State", DEF_COLUMN_STATE, 
        Blt_Offset(Column, state), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-style", "style", "Style", DEF_COLUMN_STYLE, 
        Blt_Offset(Column, stylePtr), BLT_CONFIG_NULL_OK, &styleOption},
    {BLT_CONFIG_STRING, "-text", "text", "Text",
        (char *)NULL, Blt_Offset(Column, text), 0},
    {BLT_CONFIG_STRING, "-title", "title", "Title", (char *)NULL, 
        Blt_Offset(Column, text), 0},
    {BLT_CONFIG_BACKGROUND, "-titlebackground", "titleBackground", 
        "TitleBackground", DEF_COLUMN_TITLE_BG, Blt_Offset(Column, titleBg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-titleborderwidth", "titleBorderWidth", 
        "TitleBorderWidth", DEF_COLUMN_TITLE_BORDERWIDTH, 
        Blt_Offset(Column, titleBW), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_FONT, "-titlefont", "titleFont", "Font",
        DEF_COLUMN_TITLE_FONT, Blt_Offset(Column, titleFont), 0},
    {BLT_CONFIG_COLOR, "-titleforeground", "titleForeground", "TitleForeground",
        DEF_COLUMN_TITLE_FOREGROUND, Blt_Offset(Column, titleFgColor), 0},
    {BLT_CONFIG_JUSTIFY, "-titlejustify", "titleJustify", "TitleJustify", 
        DEF_COLUMN_JUSTIFY, Blt_Offset(Column, titleJustify), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-titlerelief", "titleRelief", "TitleRelief",
        DEF_COLUMN_TITLE_RELIEF, Blt_Offset(Column, titleRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DOUBLE, "-weight", (char *)NULL, (char *)NULL,
        DEF_COLUMN_WEIGHT, Blt_Offset(Column, weight), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width",
        DEF_COLUMN_WIDTH, Blt_Offset(Column, reqWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
};

static Blt_ConfigSpec sortSpecs[] = {
    {BLT_CONFIG_CUSTOM, "-columns", "columns", "Columns",
        DEF_SORT_COLUMN, Blt_Offset(TreeView, sort.order),
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &sortColumnsOption},
    {BLT_CONFIG_OBJ, "-command", "command", "Command",
        DEF_SORT_COMMAND, Blt_Offset(TreeView, sort.cmdObjPtr),
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-decreasing", "decreasing", "Decreasing",
        DEF_SORT_DECREASING, Blt_Offset(TreeView, sort.decreasing),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-mark", "mark", "SortMark",
        DEF_SORT_COLUMN, Blt_Offset(TreeView, sort.markPtr),
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &sortMarkOption},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
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

typedef struct {
    int mask;
} ChildrenSwitches;

static Blt_SwitchSpec childrenSwitches[] = {
    {BLT_SWITCH_BITS_NOARG, "-open", "", (char *)NULL,
        Blt_Offset(ChildrenSwitches, mask), 0, CLOSED},
    {BLT_SWITCH_BITS_NOARG, "-showing", "", (char *)NULL,
        Blt_Offset(ChildrenSwitches, mask), 0, HIDDEN},
    {BLT_SWITCH_END}
};

typedef struct {
    unsigned int flags;
} CloseSwitches;

#define CLOSE_RECURSE     (1<<0)

static Blt_SwitchSpec closeSwitches[] = {
    {BLT_SWITCH_BITS_NOARG, "-recurse", "", (char *)NULL,
        Blt_Offset(CloseSwitches, flags), 0, CLOSE_RECURSE},
    {BLT_SWITCH_END}
};


typedef struct {
    unsigned int flags;
    Entry *fromPtr;
} IndexSwitches;

#define INDEX_USE_PATH  (1<<0)

static Blt_SwitchParseProc EntrySwitchProc;
static Blt_SwitchCustom entrySwitch = {
    EntrySwitchProc, NULL, NULL, (ClientData)0,
};

static Blt_SwitchSpec indexSwitches[] = {
    {BLT_SWITCH_BITS_NOARG, "-path", "", (char *)NULL,
        Blt_Offset(IndexSwitches, flags), 0, INDEX_USE_PATH},
    {BLT_SWITCH_CUSTOM, "-from", "entryName", (char *)NULL,
        Blt_Offset(IndexSwitches, fromPtr), 0, 0, &entrySwitch},
    {BLT_SWITCH_END}
};


typedef struct {
    unsigned int flags;
    int insertPos;
    Entry *rootPtr;
} InsertSwitches;

#define INSERT_PARENTS  (1<<0)
#define INSERT_NODUPS   (1<<1)

static Blt_SwitchSpec insertSwitches[] = {
    {BLT_SWITCH_INT, "-at", "position", (char *)NULL,
        Blt_Offset(InsertSwitches, insertPos), 0},
    {BLT_SWITCH_BITS_NOARG, "-parents", "", (char *)NULL,
        Blt_Offset(InsertSwitches, flags), 0, INSERT_PARENTS},
    {BLT_SWITCH_BITS_NOARG, "-nodups",  "", (char *)NULL,
        Blt_Offset(InsertSwitches, flags), 0, INSERT_NODUPS},
    {BLT_SWITCH_CUSTOM, "-root", "entryName", (char *)NULL,
        Blt_Offset(InsertSwitches, rootPtr), 0, 0, &entrySwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    int flags;
} NearestSwitches;

#define NEAREST_ROOT    (1<<0)          /* X and Y are root coordinates.  */
#define NEAREST_TITLE   (1<<1)          /* Return only in title. */

static Blt_SwitchSpec nearestColumnSwitches[] = {
    {BLT_SWITCH_BITS_NOARG, "-root", "", (char *)NULL,
        Blt_Offset(NearestSwitches, flags), 0, NEAREST_ROOT},
    {BLT_SWITCH_BITS_NOARG, "-title", "", (char *)NULL,
        Blt_Offset(NearestSwitches, flags), 0, NEAREST_TITLE},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec nearestEntrySwitches[] = {
    {BLT_SWITCH_BITS_NOARG, "-root", "", (char *)NULL,
        Blt_Offset(NearestSwitches, flags), 0, NEAREST_ROOT},
    {BLT_SWITCH_END}
};

typedef struct {
    unsigned int flags;
} OpenSwitches;

#define OPEN_RECURSE     (1<<0)

static Blt_SwitchSpec openSwitches[] = {
    {BLT_SWITCH_BITS_NOARG, "-recurse", "", (char *)NULL,
        Blt_Offset(OpenSwitches, flags), 0, OPEN_RECURSE},
    {BLT_SWITCH_END}
};


/* Forward Declarations */
static Blt_BindAppendTagsProc AppendTagsProc;
static Blt_BindPickProc PickItem;
static Blt_TreeNotifyEventProc TreeEventProc;
static Blt_TreeTraceProc TreeTraceProc;
static Tcl_CmdDeleteProc TreeViewInstCmdDeleteProc;
static Tcl_FreeProc DestroyTreeView;
static Tcl_FreeProc FreeColumn;
static Tcl_FreeProc FreeEntryProc;
static Tcl_IdleProc DisplayProc;
static Tcl_ObjCmdProc TreeViewInstCmdProc;
static Tcl_ObjCmdProc TreeViewCmdProc;
static Tk_EventProc TreeViewEventProc;
static Tk_ImageChangedProc IconChangedProc;
static Tk_SelectionProc SelectionProc;

static int ComputeVisibleEntries(TreeView *viewPtr);
static void UpdateView(TreeView *viewPtr);
static void DrawRule(TreeView *viewPtr, Column *colPtr, Drawable drawable);

static int GetEntryFromObj(Tcl_Interp *interp, TreeView *viewPtr, 
        Tcl_Obj *objPtr, Entry **entryPtrPtr);

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *      Queues a request to redraw the widget at the next idle point.
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
EventuallyRedraw(TreeView *viewPtr)
{
     if ((viewPtr->tkwin != NULL) && 
        ((viewPtr->flags & (DONT_UPDATE|REDRAW_PENDING)) == 0)) {
        viewPtr->flags |= REDRAW_PENDING;
        Tcl_DoWhenIdle(DisplayProc, viewPtr);
    }
}

void
Blt_TreeView_EventuallyRedraw(TreeView *viewPtr)
{
    EventuallyRedraw(viewPtr);
}

static int
EntryDepth(TreeView *viewPtr, Entry *entryPtr)
{
    if (viewPtr->flags & FLAT) {
        return 0;
    }
    return Blt_Tree_NodeDepth(entryPtr->node) -
        Blt_Tree_NodeDepth(viewPtr->rootPtr->node);
}

static Tcl_Obj *
CellToIndexObj(Tcl_Interp *interp, Cell *cellPtr)
{
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(cellPtr->entryPtr->node));
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewStringObj(cellPtr->colPtr->key, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    return listObjPtr;
}       

static CellStyle *
GetCurrentStyle(TreeView *viewPtr, Column *colPtr, Cell *cellPtr)
{
    if ((cellPtr != NULL) && (cellPtr->stylePtr != NULL)) {
        return cellPtr->stylePtr;
    }
    if ((colPtr != NULL) && (colPtr->stylePtr != NULL)) {
        return colPtr->stylePtr;
    }
    return viewPtr->stylePtr;
}

static Entry *
NodeToEntry(TreeView *viewPtr, Blt_TreeNode node)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&viewPtr->entryTable, (char *)node);
    if (hPtr == NULL) {
        Blt_Warn("NodeToEntry: can't find node %s\n", 
                Blt_Tree_NodeLabel(node));
        abort();
        return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

static Entry *
FindEntry(TreeView *viewPtr, Blt_TreeNode node)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&viewPtr->entryTable, (char *)node);
    if (hPtr == NULL) {
        return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

static INLINE int
EntryIsHidden(Entry *entryPtr)
{
    TreeView *viewPtr = entryPtr->viewPtr; 

    if ((viewPtr->flags & HIDE_LEAVES) && (Blt_Tree_IsLeaf(entryPtr->node))) {
        return TRUE;
    }
    return (entryPtr->flags & HIDDEN) ? TRUE : FALSE;
}

static INLINE int
EntryIsSelected(TreeView *viewPtr, Entry *entryPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&viewPtr->sel.table, (char *)entryPtr);
    return (hPtr != NULL);
}

static Entry *
FindChild(Entry *parentPtr, const char *name)
{
    Entry *childPtr;
    
    for (childPtr = parentPtr->firstChildPtr; childPtr != NULL;
         childPtr = childPtr->nextSiblingPtr) {
        if (strcmp(Blt_Tree_NodeLabel(childPtr->node), name) == 0) {
            return childPtr;
        }
    }
    return NULL;
}    

static Entry *
FirstChild(Entry *parentPtr, unsigned int hateFlags)
{
    Entry *childPtr;

    if ((hateFlags & CLOSED) && (parentPtr->flags & CLOSED)) {
        return NULL;
    }
    for (childPtr = parentPtr->firstChildPtr; childPtr != NULL;
         childPtr = childPtr->nextSiblingPtr) {
        if (((hateFlags & HIDDEN) == 0) || (!EntryIsHidden(childPtr))) {
            return childPtr;
        }
    }
    return NULL;
}

static Entry *
LastChild(Entry *parentPtr, unsigned int hateFlags)
{
    Entry *childPtr;

    if ((hateFlags & CLOSED) && (parentPtr->flags & CLOSED)) {
        return NULL;
    }
    for (childPtr = parentPtr->lastChildPtr; childPtr != NULL;
         childPtr = childPtr->prevSiblingPtr) {
        if (((hateFlags & HIDDEN) == 0) || (!EntryIsHidden(childPtr))) {
            return childPtr;
        }
    }
    return NULL;
}

static Entry *
NextSibling(Entry *entryPtr, unsigned int hateFlags)
{
    for (entryPtr = entryPtr->nextSiblingPtr; entryPtr != NULL;
         entryPtr = entryPtr->nextSiblingPtr) {
        if (((hateFlags & HIDDEN) == 0) || (!EntryIsHidden(entryPtr))) {
            return entryPtr;
        }
    }
    return NULL;
}

static Entry *
PrevSibling(Entry *entryPtr, unsigned int hateFlags)
{
    for (entryPtr = entryPtr->prevSiblingPtr; entryPtr != NULL;
         entryPtr = entryPtr->prevSiblingPtr) {
        if (((hateFlags & HIDDEN) == 0) || (!EntryIsHidden(entryPtr))) {
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
 *      Returns the "previous" node in the tree.  This node (in depth-first
 *      order) is its parent if the node has no siblings that are before
 *      to it.  Otherwise it is the last descendant of the last sibling.
 *      In this case, descend the sibling's hierarchy, using the last child
 *      at any ancestor, until we we find a leaf.
 *
 *---------------------------------------------------------------------------
 */
static Entry *
PrevEntry(Entry *entryPtr, unsigned int mask)
{
    Entry *prevPtr;

    if (entryPtr->parentPtr == NULL) {
        return NULL;                    /* The root is the first node. */
    }
    prevPtr = PrevSibling(entryPtr, mask);
    if (prevPtr == NULL) {
        /* There are no siblings previous to this one, so pick the parent. */
        prevPtr = entryPtr->parentPtr;
    } else {
        /*
         * Traverse down the right-most thread in order to select the last
         * entry.  Stop if we find a "closed" entry or reach a leaf.
         */
        entryPtr = prevPtr;
        while ((entryPtr->flags & mask) == 0) {
            entryPtr = LastChild(entryPtr, mask);
            if (entryPtr == NULL) {
                break;                  /* Found a leaf. */
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
 *      Returns the "next" node in relation to the given node.  The next
 *      node (in depth-first order) is either the first child of the given
 *      node the next sibling if the node has no children (the node is a
 *      leaf).  If the given node is the last sibling, then try it's parent
 *      next sibling.  Continue until we either find a next sibling for
 *      some ancestor or we reach the root node.  In this case the current
 *      node is the last node in the tree.
 *
 *---------------------------------------------------------------------------
 */
static Entry *
NextEntry(Entry *entryPtr, unsigned int hateFlags)
{
    TreeView *viewPtr = entryPtr->viewPtr; 
    Entry *nextPtr;
    int ignoreLeaf;

    ignoreLeaf = ((viewPtr->flags & HIDE_LEAVES) && 
                  (Blt_Tree_IsLeaf(entryPtr->node)));
    if ((!ignoreLeaf) && ((entryPtr->flags & hateFlags) == 0)) {
        nextPtr = FirstChild(entryPtr, hateFlags); 
        if (nextPtr != NULL) {
            return nextPtr;             /* Pick the first sub-node. */
        }
    }
    /* 
     * Back up to a level where we can pick a "next sibling".  For the last
     * entry we'll thread our way back to the root.
     */
    while (entryPtr != viewPtr->rootPtr) {
        nextPtr = NextSibling(entryPtr, hateFlags);
        if (nextPtr != NULL) {
            return nextPtr;
        }
        entryPtr = entryPtr->parentPtr;
    }
    return NULL;                        /* At root, no next node. */
}

/*
 *---------------------------------------------------------------------------
 *
 * IsBefore --
 *
 *      Determines if the first entry is before the second in the tree.
 *      This is used (as opposed to the Blt_Tree_IsBefore routine) because
 *      sorting may changed the order within the the view.
 *
 *---------------------------------------------------------------------------
 */
static int
IsBefore(Entry *entryPtr1, Entry *entryPtr2)
{
    long depth;
    long i;
    Entry *childPtr;
    
    if (entryPtr1 == entryPtr2) {
        return FALSE;
    }
    depth = MIN(Blt_Tree_NodeDepth(entryPtr1->node),
                Blt_Tree_NodeDepth(entryPtr2->node));
    if (depth == 0) {                   /* One of the nodes is root. */
        return (entryPtr1->parentPtr == NULL);
    }
    /* 
     * Traverse back from the deepest entry, until both entries are at the
     * same depth.  Check if this ancestor entry is the same for both
     * entries.
     */
    for (i = Blt_Tree_NodeDepth(entryPtr1->node); i > depth; i--) {
        entryPtr1 = entryPtr1->parentPtr;
    }
    if (entryPtr1 == entryPtr2) {
        return FALSE;
    }
    for (i = Blt_Tree_NodeDepth(entryPtr2->node); i > depth; i--) {
        entryPtr2 = entryPtr2->parentPtr;
    }
    if (entryPtr2 == entryPtr1) {
        return TRUE;
    }

    /* 
     * First find the mutual ancestor of both nodes.  Look at each
     * preceding ancestor level-by-level for both nodes.  Eventually we'll
     * find a node that's the parent of both ancestors.  Then find the
     * first ancestor in the parent's list of subnodes.
     */
    for (i = depth; i > 0; i--) {
        if (entryPtr1->parentPtr == entryPtr2->parentPtr) {
            break;
        }
        entryPtr1 = entryPtr1->parentPtr;
        entryPtr2 = entryPtr2->parentPtr;
    }
    for (childPtr = entryPtr1->parentPtr->firstChildPtr; childPtr != NULL; 
         childPtr = childPtr->nextSiblingPtr) {
        if (childPtr == entryPtr1) {
            return TRUE;
        } else if (childPtr == entryPtr2) {
            return FALSE;
        }
    }
    return FALSE;
}

static const char *
GetPathFromRoot(TreeView *viewPtr, Entry *entryPtr, int checkEntryLabel, 
             Tcl_DString *resultPtr)
{
    const char **names;                 /* Used the stack the component
                                         * names. */
    const char *staticSpace[64+2];
    int level;
    int i;

    level = Blt_Tree_NodeDepth(entryPtr->node);
    if (viewPtr->rootPtr->labelObjPtr == NULL) {
        level--;
    }
    if (level > 64) {
        names = Blt_AssertMalloc((level + 2) * sizeof(char *));
    } else {
        names = staticSpace;
    }
    for (i = level; i >= 0; i--) {
        /* Save the name of each ancestor in the name array. */
        if (checkEntryLabel) {
            names[i] = GETLABEL(entryPtr);
        } else {
            names[i] = Blt_Tree_NodeLabel(entryPtr->node);
        }
        entryPtr = entryPtr->parentPtr;
    }
    Tcl_DStringInit(resultPtr);
    if (level >= 0) {
        if ((viewPtr->pathSep == SEPARATOR_LIST) || 
            (viewPtr->pathSep == SEPARATOR_NONE)) {
            for (i = 0; i <= level; i++) {
                Tcl_DStringAppendElement(resultPtr, names[i]);
            }
        } else {
            Tcl_DStringAppend(resultPtr, names[0], -1);
            for (i = 1; i <= level; i++) {
                Tcl_DStringAppend(resultPtr, viewPtr->pathSep, -1);
                Tcl_DStringAppend(resultPtr, names[i], -1);
            }
        }
    } else {
        if ((viewPtr->pathSep != SEPARATOR_LIST) &&
            (viewPtr->pathSep != SEPARATOR_NONE)) {
            Tcl_DStringAppend(resultPtr, viewPtr->pathSep, -1);
        }
    }
    if (names != staticSpace) {
        Blt_Free(names);
    }
    return Tcl_DStringValue(resultPtr);
}

static void
FreePath(Entry *entryPtr)
{
    if (entryPtr->pathName != NULL) {
        Blt_Free(entryPtr->pathName);
    }
    entryPtr->pathName = NULL;
}

static const char *
PathFromRoot(TreeView *viewPtr, Entry *entryPtr)
{
    if (entryPtr->pathName == NULL) {
        Tcl_DString ds;

        Tcl_DStringInit(&ds);
        GetPathFromRoot(viewPtr, entryPtr, TRUE, &ds);
        entryPtr->pathName = Blt_AssertStrdup(Tcl_DStringValue(&ds));
        Tcl_DStringFree(&ds);
    }
    return entryPtr->pathName;
}

/*
 * Preprocess the command string for percent substitution.
 */
static Tcl_Obj *
PercentSubst(TreeView *viewPtr, Entry *entryPtr, Tcl_Obj *cmdObjPtr)
{
    const char *last, *p;
    const char *string;
    Tcl_Obj *objPtr;

    objPtr = Tcl_NewStringObj("", 0);
    /* Append the widget name and the node .t 0 */
    string = Tcl_GetString(cmdObjPtr);
    for (last = p = string; *p != '\0'; p++) {
        if (*p == '%') {
            const char *string;
            char buf[3];

            if (p > last) {
                Tcl_AppendToObj(objPtr, last, p - last);
            }
            switch (*(p + 1)) {
            case '%':                   /* Percent sign */
                string = "%";
                break;
            case 'W':                   /* Widget name */
                string = Tk_PathName(viewPtr->tkwin);
                break;
            case 'P':                   /* Path from root to entry */
                string = PathFromRoot(viewPtr, entryPtr);
                break;
            case 'p':                   /* Name of the node */
                string = GETLABEL(entryPtr);
                break;
            case '#':                   /* Node identifier */
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
            {
                int needQuotes = FALSE;
                const char *q;

                for (q = string; *q != '\0'; q++) {
                    if (*q == ' ') {
                        needQuotes = TRUE;
                        break;
                    }
                }
                if (needQuotes) {
                    Tcl_AppendToObj(objPtr, "{", 1);
                    Tcl_AppendToObj(objPtr, string, -1);
                    Tcl_AppendToObj(objPtr, "}", 1);
                } else {
                    Tcl_AppendToObj(objPtr, string, -1);
                }
            }
            p++;
            last = p + 1;
        }
    }
    if (p > last) {
        Tcl_AppendToObj(objPtr, last, p-last);
    }
     return objPtr;
}

static int
OpenEntry(TreeView *viewPtr, Entry *entryPtr)
{
    Tcl_Obj *cmdObjPtr;

    if (IsOpen(entryPtr)) {
        return TCL_OK;                  /* Entry is already open. */
    }
    entryPtr->flags &= ~CLOSED;
    viewPtr->flags |= LAYOUT_PENDING;
    /*
     * If there's a "open" command proc specified for the entry, use that
     * instead of the more general "open" proc for the entire treeview.  Be
     * careful because the "open" command may perform an update.
     */
    cmdObjPtr = CHOOSE(viewPtr->openCmdObjPtr, entryPtr->openCmdObjPtr);
    if (cmdObjPtr != NULL) {
        int result;

        cmdObjPtr = PercentSubst(viewPtr, entryPtr, cmdObjPtr);
        Tcl_IncrRefCount(cmdObjPtr);
        Tcl_Preserve(entryPtr);
        result = Tcl_EvalObjEx(viewPtr->interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_Release(entryPtr);
        Tcl_DecrRefCount(cmdObjPtr);
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
CloseEntry(TreeView *viewPtr, Entry *entryPtr)
{
    Tcl_Obj *cmdObjPtr;

    if (IsClosed(entryPtr)) {
        return TCL_OK;                  /* Entry is already closed. */
    }
    entryPtr->flags |= CLOSED;
    viewPtr->flags |= LAYOUT_PENDING;
    /*
     * Invoke the entry's "close" command, if there is one. Otherwise try
     * the treeview's global "close" command.
     */
    cmdObjPtr = CHOOSE(viewPtr->closeCmdObjPtr, entryPtr->closeCmdObjPtr);
    if (cmdObjPtr != NULL) {
        int result;

        cmdObjPtr = PercentSubst(viewPtr, entryPtr, cmdObjPtr);
        Tcl_IncrRefCount(cmdObjPtr);
        Tcl_Preserve(entryPtr);
        result = Tcl_EvalObjEx(viewPtr->interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_Release(entryPtr);
        Tcl_DecrRefCount(cmdObjPtr);
        if (result != TCL_OK) {
            viewPtr->flags |= LAYOUT_PENDING;
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * GetVerticalLineCoordinates --
 *
 *      Computes the y-coordinates of the vertical line from an entry to 
 *      its bottom-most child.  If the root node is hidden (and this is
 *      the root entry), start with the first child.
 *
 *---------------------------------------------------------------------------
 */
static void 
GetVerticalLineCoordinates(Entry *entryPtr, int *y1Ptr, int *y2Ptr)   
{
    Entry *topPtr, *botPtr;
    TreeView *viewPtr = entryPtr->viewPtr; 
    int y1, y2;

    botPtr = entryPtr->lastChildPtr;
    topPtr = entryPtr;
    if ((viewPtr->rootPtr == entryPtr) && (viewPtr->flags & HIDE_ROOT)) {
        topPtr = NextEntry(entryPtr, HIDDEN | CLOSED);
        assert(topPtr != NULL);
    }
    y1 = SCREENY(viewPtr, topPtr->worldY) + (topPtr->height / 2);
    y2 = SCREENY(viewPtr, botPtr->worldY) + (botPtr->height / 2);

    /* Make sure the vertical line starts on an odd pixel. */
    y1 |= 0x1;
    /*
     * Clip the line's Y-coordinates at the viewport's borders.
     */
    if (y1 < 0) {
        y1 = -1;
    }
    if (y2 > Tk_Height(viewPtr->tkwin)) {
        y2 = Tk_Height(viewPtr->tkwin);
    }
    /* Make sure the vertical line starts and ends on odd pixels. */
    *y1Ptr = y1;
    *y2Ptr = y2;
}


static Cell *
GetCell(Entry *entryPtr, Column *colPtr)
{
    Cell *cellPtr;

    for (cellPtr = entryPtr->cells; cellPtr != NULL; 
         cellPtr = cellPtr->nextPtr) {
        if (cellPtr->colPtr == colPtr) {
            return cellPtr;
        }
    }
    return NULL;
}

static void
AddCell(Entry *entryPtr, Column *colPtr)
{
    Cell *cellPtr;
    Tcl_Obj *objPtr;
    TreeView *viewPtr;

    viewPtr = entryPtr->viewPtr;
    objPtr = NULL;
    if (GetData(entryPtr, colPtr->key, &objPtr) != TCL_OK) {
        return;
    }
    Tcl_IncrRefCount(objPtr);
    cellPtr = GetCell(entryPtr, colPtr);
    if (cellPtr != NULL) {
        return;
    }
    /* Add a new cell only if a data entry exists. */
    cellPtr = Blt_Pool_AllocItem(entryPtr->viewPtr->cellPool, sizeof(Cell));
    memset(cellPtr, 0, sizeof(Cell));
    cellPtr->entryPtr = entryPtr;
    cellPtr->viewPtr = viewPtr;
    cellPtr->colPtr = colPtr;
    cellPtr->nextPtr = entryPtr->cells;
    entryPtr->cells = cellPtr;
    cellPtr->dataObjPtr = objPtr;
    cellPtr->flags |= GEOMETRY;
    viewPtr->flags |= LAYOUT_PENDING;   /* Says that the current view is
                                         * out-of-date. */
    if (viewPtr->flags & TV_SORT_AUTO) {
        /* If we're auto-sorting, schedule the view to be resorted. */
        viewPtr->flags |= SORT_PENDING;
    }
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
    TreeView *viewPtr = clientData;

    viewPtr->flags &= ~SELECT_PENDING;
    Tcl_Preserve(viewPtr);
    if (viewPtr->sel.cmdObjPtr != NULL) {
        if (Tcl_EvalObjEx(viewPtr->interp, viewPtr->sel.cmdObjPtr, 
                TCL_EVAL_GLOBAL) != TCL_OK) {
            Tcl_BackgroundError(viewPtr->interp);
        }
    }
    Tcl_Release(viewPtr);
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
EventuallyInvokeSelectCmd(TreeView *viewPtr)
{
    if ((viewPtr->flags & SELECT_PENDING) == 0) {
        viewPtr->flags |= SELECT_PENDING;
        Tcl_DoWhenIdle(SelectCmdProc, viewPtr);
    }
}

static void
ClearSelection(TreeView *viewPtr)
{
    Blt_DeleteHashTable(&viewPtr->sel.table);
    Blt_InitHashTable(&viewPtr->sel.table, BLT_ONE_WORD_KEYS);
    Blt_Chain_Reset(viewPtr->sel.list);
    EventuallyRedraw(viewPtr);
    if (viewPtr->sel.cmdObjPtr != NULL) {
        EventuallyInvokeSelectCmd(viewPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetEntryIcon --
 *
 *      Selects the correct image for the entry's icon depending upon the
 *      current state of the entry: active/inactive normal/selected.
 *
 *              active - normal
 *              active - selected
 *              inactive - normal
 *              inactive - selected
 *
 * Results:
 *      Returns the image for the icon.
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
    if (icons != NULL) {                /* Selected or normal icon? */
        icon = icons[0];
        if ((IsOpen(entryPtr)) && (icons[1] != NULL)) {
            icon = icons[1];
        }
    }
    return icon;
}

static void
SelectEntry(TreeView *viewPtr, Entry *entryPtr)
{
    int isNew;
    Blt_HashEntry *hPtr;
    Icon icon;
    const char *label;
    Selection *selectPtr = &viewPtr->sel;

    if ((viewPtr->flags & HIDE_ROOT) && (entryPtr == viewPtr->rootPtr)) {
        return;
    }
    hPtr = Blt_CreateHashEntry(&selectPtr->table, (char *)entryPtr, &isNew);
    if (isNew) {
        Blt_ChainLink link;

        link = Blt_Chain_Append(selectPtr->list, entryPtr);
        Blt_SetHashValue(hPtr, link);
    }
    label = GETLABEL(entryPtr);
    if ((viewPtr->textVarObjPtr != NULL) && (label != NULL)) {
        Tcl_Obj *objPtr;
        
        objPtr = Tcl_NewStringObj(label, -1);
        if (Tcl_ObjSetVar2(viewPtr->interp, viewPtr->textVarObjPtr, NULL, 
                objPtr, TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG) == NULL) {
            return;
        }
    }
    icon = GetEntryIcon(viewPtr, entryPtr);
    if ((viewPtr->iconVarObjPtr != NULL) && (icon != NULL)) {
        Tcl_Obj *objPtr;
        
        objPtr = Tcl_NewStringObj(IconName(icon), -1);
        if (Tcl_ObjSetVar2(viewPtr->interp, viewPtr->iconVarObjPtr, NULL, 
                objPtr, TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG) == NULL) {
            return;
        }
    }
}

static void
DeselectEntry(TreeView *viewPtr, Entry *entryPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&viewPtr->sel.table, (char *)entryPtr);
    if (hPtr != NULL) {
        Blt_ChainLink link;

        link = Blt_GetHashValue(hPtr);
        Blt_Chain_DeleteLink(viewPtr->sel.list, link);
        Blt_DeleteHashEntry(&viewPtr->sel.table, hPtr);
    }
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
    TreeView *viewPtr = clientData;

    if ((viewPtr->sel.flags & SELECT_EXPORT) == 0) {
        return;
    }
    ClearSelection(viewPtr);
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
static int
SelectRange(TreeView *viewPtr, Entry *fromPtr, Entry *toPtr)
{
    if (viewPtr->flags & FLAT) {
        int i;

        if (fromPtr->flatIndex > toPtr->flatIndex) {
            for (i = fromPtr->flatIndex; i >= toPtr->flatIndex; i--) {
                SelectEntryApplyProc(viewPtr, viewPtr->flatArr[i]);
            }
        } else {
            for (i = fromPtr->flatIndex; i <= toPtr->flatIndex; i++) {
                SelectEntryApplyProc(viewPtr, viewPtr->flatArr[i]);
            }
        }
    } else {
        Entry *entryPtr, *nextPtr;
        IterProc *proc;

        /* From the range determine the direction to select entries. */
        proc = (IsBefore(toPtr, fromPtr)) ? PrevEntry : NextEntry;
        /* Select entries in the range. Only select visible entries. */
        for (entryPtr = fromPtr; entryPtr != NULL; entryPtr = nextPtr) {
            nextPtr = (*proc)(entryPtr, HIDDEN | CLOSED);
            SelectEntryApplyProc(viewPtr, entryPtr);
            if (entryPtr == toPtr) {
                break;
            }
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetCurrentColumn --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Column *
GetCurrentColumn(TreeView *viewPtr)
{
    TreeViewObj *objPtr;
    ItemType type;
    
    type = (long)Blt_GetCurrentHint(viewPtr->bindTable);
    objPtr = Blt_GetCurrentItem(viewPtr->bindTable);
    if ((objPtr == NULL) || (objPtr->flags & DELETED)) {
        return NULL;
    }
    switch (type) {
    case ITEM_COLUMN_TITLE:
    case ITEM_COLUMN_RESIZE:
        return (Column *)objPtr;
        break;
        
    case ITEM_CELL:
        {
            Cell *cellPtr;
            
            cellPtr = (Cell *)objPtr;
            return cellPtr->colPtr;
        }
        break;
    default:
        break;
    }
    return NULL;
}

static Column *
NearestColumn(TreeView *viewPtr, int x, int y, ItemType *typePtr)
{
    Blt_ChainLink link;

    /*
     * Determine if the pointer is over the rightmost portion of the
     * column.  This activates the rule.
     */
    if (typePtr != NULL) {
        *typePtr = ITEM_NONE;
    }
    x = WORLDX(viewPtr, x);             /* Convert from screen to world
                                         * coordinates. */
    for(link = Blt_Chain_FirstLink(viewPtr->columns); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Column *colPtr;
        int right;
        
        colPtr = Blt_Chain_GetValue(link);
        right = colPtr->worldX + colPtr->width;
        if ((x >= colPtr->worldX) && (x <= right)) {
            ItemType type;

            type = ITEM_NONE;
            /* We're inside of a column, now considering y. */
            if (viewPtr->flags & SHOW_COLUMN_TITLES) {
                /* Check if we're inside of the column title. */
                if ((y >= viewPtr->inset) && 
                    (y < (viewPtr->titleHeight + viewPtr->inset))) {
                    type = (x >= (right - RULE_AREA)) 
                        ? ITEM_COLUMN_RESIZE : ITEM_COLUMN_TITLE;
                } 
            }
            if (typePtr != NULL) {
                *typePtr = type;
            }
            return colPtr;
        }
    }
    return NULL;                        /* Not found. */
}

static int
GetColumn(Tcl_Interp *interp, TreeView *viewPtr, Tcl_Obj *objPtr, 
          Column **colPtrPtr)
{
    const char *string;
    char c;
    int index;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 't') && (strcmp(string, "treeView") == 0)) {
        *colPtrPtr = &viewPtr->treeColumn;
    } else if ((c == 'c') && (strcmp(string, "current") == 0)){ 
        *colPtrPtr = GetCurrentColumn(viewPtr);
    } else if ((c == 'a') && (strcmp(string, "active") == 0)){ 
        *colPtrPtr = viewPtr->colActivePtr;
    } else if ((isdigit(c)) && 
               (Tcl_GetIntFromObj(NULL, objPtr, &index) == TCL_OK)) {
        Blt_ChainLink link;

        if (index >= Blt_Chain_GetLength(viewPtr->columns)) {
            Tcl_AppendResult(interp, "bad column index \"", string, "\"",
                             (char *)NULL);
            return TCL_ERROR;
        }
        link = Blt_Chain_GetNthLink(viewPtr->columns, index);
        *colPtrPtr = Blt_Chain_GetValue(link);
    } else {
        Blt_HashEntry *hPtr;
    
        hPtr = Blt_FindHashEntry(&viewPtr->columnTable, 
                Blt_Tree_GetKey(viewPtr->tree, string));
        if (hPtr == NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "can't find column \"", string, 
                        "\" in \"", Tk_PathName(viewPtr->tkwin), "\"", 
                        (char *)NULL);
            }
            return TCL_ERROR;
        } 
        *colPtrPtr = Blt_GetHashValue(hPtr);
    }
    return TCL_OK;
}


static void
TraceColumn(TreeView *viewPtr, Column *colPtr)
{
    Blt_Tree_CreateTrace(viewPtr->tree, NULL /* Node */, colPtr->key, NULL,
        TREE_TRACE_FOREIGN_ONLY | TREE_TRACE_WRITES | TREE_TRACE_UNSETS, 
        TreeTraceProc, viewPtr);
}


static void
TraceColumns(TreeView *viewPtr)
{
    Blt_ChainLink link;

    for(link = Blt_Chain_FirstLink(viewPtr->columns); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Column *colPtr;

        colPtr = Blt_Chain_GetValue(link);
        /* Keys are on a per-tree basis, re-get the key. */
        colPtr->key = Blt_Tree_GetKey(viewPtr->tree, colPtr->name);
        Blt_Tree_CreateTrace(
                viewPtr->tree, 
                NULL                        /* Node */, 
                colPtr->key                 /* Key pattern */, 
                NULL                        /* Tag */,
                TREE_TRACE_FOREIGN_ONLY|TREE_TRACE_WRITES|TREE_TRACE_UNSETS, 
                TreeTraceProc               /* Callback routine */, 
                viewPtr                     /* Client data */);
    }
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
GetCachedObj(TreeView *viewPtr, Tcl_Obj *objPtr)
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
FreeCachedObj(TreeView *viewPtr, Tcl_Obj *objPtr)
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

static void
DestroyStyle(CellStyle *stylePtr)
{
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    iconOption.clientData = viewPtr;
    Blt_FreeOptions(stylePtr->classPtr->specs, (char *)stylePtr, 
                    viewPtr->display, 0);
    (*stylePtr->classPtr->freeProc)(stylePtr); 
    /* 
     * Removing the style from the hash tables frees up the style name
     * again.  The style itself may not be removed until it's been released
     * by everything using it.
     */
    if (stylePtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&viewPtr->styleTable, stylePtr->hashPtr);
        stylePtr->hashPtr = NULL;
    } 
    if (stylePtr->link != NULL) {
        /* Only user-generated styles will be in the list. */
        Blt_Chain_DeleteLink(viewPtr->userStyles, stylePtr->link);
    }
    Blt_Free(stylePtr);
}

static void
FreeStyle(CellStyle *stylePtr)
{
    stylePtr->refCount--;
    /* If no cell is using the style, remove it.*/
    if (stylePtr->refCount <= 0) {
        DestroyStyle(stylePtr);
    }
}


static void
DestroyCell(TreeView *viewPtr, Cell *cellPtr)
{
    Blt_DeleteBindings(viewPtr->bindTable, cellPtr);
    if (viewPtr->flags & TV_SORT_AUTO) {
        viewPtr->flags |= SORT_PENDING;
    }
    if (cellPtr->stylePtr != NULL) {
        FreeStyle(cellPtr->stylePtr);
    }
    /* Fix pointers to destroyed cell. */
    if (viewPtr->activeCellPtr == cellPtr) {
        viewPtr->activeCellPtr = NULL;
    }
    if (viewPtr->focusCellPtr == cellPtr) {
        viewPtr->focusCellPtr = NULL;
    }
    if (viewPtr->postPtr == cellPtr) {
        viewPtr->postPtr = NULL;
    }
    if (cellPtr->dataObjPtr != NULL) {
        Tcl_DecrRefCount(cellPtr->dataObjPtr);
        cellPtr->dataObjPtr = NULL;
    }
}

static void
AppendEntry(Entry *parentPtr, Entry *entryPtr)
{
    if (parentPtr != NULL) {
        if (parentPtr->lastChildPtr == NULL) {
            parentPtr->firstChildPtr = parentPtr->lastChildPtr = entryPtr;
        } else {
            entryPtr->prevSiblingPtr = parentPtr->lastChildPtr;
            parentPtr->lastChildPtr->nextSiblingPtr = entryPtr;
            parentPtr->lastChildPtr = entryPtr;
        }
        entryPtr->parentPtr = parentPtr;
        parentPtr->numChildren++;
    }
}


static void
DetachEntry(Entry *entryPtr)
{
    if (entryPtr->prevSiblingPtr != NULL) {
        entryPtr->prevSiblingPtr->nextSiblingPtr = entryPtr->nextSiblingPtr;
    }
    if (entryPtr->nextSiblingPtr != NULL) {
        entryPtr->nextSiblingPtr->prevSiblingPtr = entryPtr->prevSiblingPtr;
    }
    if (entryPtr->parentPtr != NULL) {
        if (entryPtr->parentPtr->firstChildPtr == entryPtr) {
            entryPtr->parentPtr->firstChildPtr = entryPtr->nextSiblingPtr;
        }
        if (entryPtr->parentPtr->lastChildPtr == entryPtr) {
            entryPtr->parentPtr->lastChildPtr = entryPtr->prevSiblingPtr;
        }
        entryPtr->parentPtr->numChildren--;
    }        
    entryPtr->nextSiblingPtr = entryPtr->prevSiblingPtr =
        entryPtr->parentPtr = NULL;
}

static void
DestroyEntry(Entry *entryPtr)
{
    TreeView *viewPtr;
    
    entryPtr->flags |= DELETED;         /* Mark the entry as destroyed. */

    viewPtr = entryPtr->viewPtr;

    /* Fix pointers to destroyed entry. */
    if (viewPtr->activePtr == entryPtr) {
        viewPtr->activePtr = entryPtr->parentPtr;
    }
    if (viewPtr->activeBtnPtr == entryPtr) {
        viewPtr->activeBtnPtr = NULL;
    }
    if (viewPtr->focusPtr == entryPtr) {
        viewPtr->focusPtr = entryPtr->parentPtr;
        Blt_SetFocusItem(viewPtr->bindTable, viewPtr->focusPtr, ITEM_ENTRY);
    }
    if (viewPtr->sel.anchorPtr == entryPtr) {
        viewPtr->sel.markPtr = viewPtr->sel.anchorPtr = NULL;
    }

    DeselectEntry(viewPtr, entryPtr);
    Blt_DeleteBindings(viewPtr->bindTable, entryPtr);
    if (entryPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&viewPtr->entryTable, entryPtr->hashPtr);
    }
    entryPtr->node = NULL;
    DetachEntry(entryPtr);
    iconsOption.clientData = viewPtr;
    cachedObjOption.clientData = viewPtr;
    labelOption.clientData = viewPtr;
    Blt_FreeOptions(entrySpecs, (char *)entryPtr, viewPtr->display, 0);
    if (viewPtr->rootPtr == entryPtr) {
        Blt_TreeNode root;

        /* Restore the root node back to the top of the tree. */
        root = Blt_Tree_RootNode(viewPtr->tree);
        viewPtr->rootPtr = NodeToEntry(viewPtr,root);
    }
    if (!Blt_Tree_TagTableIsShared(viewPtr->tree)) {
        /* Don't clear tags unless this client is the only one using the
         * tag table.*/
        Blt_Tree_ClearTags(viewPtr->tree, entryPtr->node);
    }
    if (entryPtr->gc != NULL) {
        Tk_FreeGC(viewPtr->display, entryPtr->gc);
    }
    /* Delete the chain of data cells from the entry. */
    if (entryPtr->cells != NULL) {
        Cell *cellPtr, *nextPtr;
        
        for (cellPtr = entryPtr->cells; cellPtr != NULL; 
             cellPtr = nextPtr) {
            nextPtr = cellPtr->nextPtr;
            DestroyCell(viewPtr, cellPtr);
        }
        entryPtr->cells = NULL;
    }
    FreePath(entryPtr);
    Tcl_EventuallyFree(entryPtr, FreeEntryProc);
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteEntries --
 *
 *      Deletes the child entries at a given parent entry.
 *
 * Results:
 *      Returns a standard TCL result. If a temporary array of entry
 *      pointers can't be allocated TCL_ERROR is returned.
 *
 *---------------------------------------------------------------------------
 */
static void
DeleteEntries(TreeView *viewPtr, Entry *parentPtr)
{
    Entry *entryPtr, *nextPtr;

    for (entryPtr = parentPtr->firstChildPtr; entryPtr != NULL; 
         entryPtr = nextPtr) {
        nextPtr = entryPtr->nextSiblingPtr;
        if (entryPtr->firstChildPtr != NULL) {
            DeleteEntries(viewPtr, entryPtr);
        }
        DestroyEntry(entryPtr);
    }
}

/*ARGSUSED*/
static void
FreeTreeProc(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Blt_Tree *treePtr = (Blt_Tree *)(widgRec + offset);

    if (*treePtr != NULL) {
        TreeView *viewPtr = clientData;

        /* 
         * Release the current tree, removing any entry fields. 
         */
        DeleteEntries(viewPtr, viewPtr->rootPtr);
        ClearSelection(viewPtr);
        Blt_Tree_Close(*treePtr);
        *treePtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EntrySwitchProc --
 *
 *      Convert a Tcl_Obj representing a node number into its integer
 *      value.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntrySwitchProc(ClientData clientData, Tcl_Interp *interp, 
               const char *switchName, Tcl_Obj *objPtr, char *record,
               int offset, int flags) 
{
    Entry **entryPtrPtr = (Entry **)(record + offset);
    TreeView *viewPtr = clientData;

    if (GetEntryFromObj(interp, viewPtr, objPtr, entryPtrPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTree --
 *
 *      Convert the string representing the name of a tree object into a
 *      tree token.
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
ObjToTree(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          Tcl_Obj *objPtr, char *widgRec, int offset, int flags)                {
    Blt_Tree tree = *(Blt_Tree *)(widgRec + offset);

    if (Blt_Tree_Attach(interp, tree, Tcl_GetString(objPtr)) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeToObj --
 *
 * Results:
 *      The string representation of the button boolean is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TreeToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          char *widgRec, int offset, int flags)  
{
    Blt_Tree tree = *(Blt_Tree *)(widgRec + offset);

    if (tree == NULL) {
        return Tcl_NewStringObj("", -1);
    } else {
        return Tcl_NewStringObj(Blt_Tree_Name(tree), -1);
    }
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
    const char *string;
    char c;
    int *modePtr = (int *)(widgRec + offset);

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'l') && (strcmp(string, "listbox") == 0)) {
        *modePtr = BLT_SCROLL_MODE_LISTBOX;
    } else if ((c == 't') && (strcmp(string, "treeview") == 0)) {
        *modePtr = BLT_SCROLL_MODE_HIERBOX;
    } else if ((c == 'c') && (strcmp(string, "canvas") == 0)) {
        *modePtr = BLT_SCROLL_MODE_CANVAS;
    } else {
        Tcl_AppendResult(interp, "bad scroll mode \"", string,
            "\": should be \"treeview\", \"listbox\", or \"canvas\"", 
                (char *)NULL);
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
 *      The string representation of the button boolean is returned.
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
        return Tcl_NewStringObj("listbox", -1);
    case BLT_SCROLL_MODE_HIERBOX:
        return Tcl_NewStringObj("hierbox", -1);
    case BLT_SCROLL_MODE_CANVAS:
        return Tcl_NewStringObj("canvas", -1);
    default:
        return Tcl_NewStringObj("unknown scroll mode", -1);
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
 *      Otherwise, TCL_ERROR is returned and an error message is left in
 *      interpreter's result field.
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

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 's') && (strcmp(string, "single") == 0)) {
        *modePtr = SELECT_MODE_SINGLE;
    } else if ((c == 'm') && (strcmp(string, "multiple") == 0)) {
        *modePtr = SELECT_MODE_MULTIPLE;
    } else if ((c == 'a') && (strcmp(string, "active") == 0)) {
        *modePtr = SELECT_MODE_SINGLE;
    } else if ((c == 'n') && (strcmp(string, "none") == 0)) {
        *modePtr = SELECT_MODE_NONE;
    } else {
        Tcl_AppendResult(interp, "bad select mode \"", string,
                         "\": should be \"single\", \"multiple\", or \"none\"",
                         (char *)NULL);
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
 *      The string representation of the button boolean is returned.
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
    case SELECT_MODE_NONE:
        return Tcl_NewStringObj("none", 4);
    case SELECT_MODE_SINGLE:
        return Tcl_NewStringObj("single", 6);
    case SELECT_MODE_MULTIPLE:
        return Tcl_NewStringObj("multiple", 8);
    default:
        return Tcl_NewStringObj("unknown scroll mode", -1);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToButton --
 *
 *      Convert a string to one of three values.
 *              0 - false, no, off
 *              1 - true, yes, on
 *              2 - auto
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.
 *      Otherwise, TCL_ERROR is returned and an error message is left in
 *      interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToButton(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
            Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    const char *string;
    int *flagsPtr = (int *)(widgRec + offset);

    string = Tcl_GetString(objPtr);
    if ((string[0] == 'a') && (strcmp(string, "auto") == 0)) {
        *flagsPtr &= ~ENTRY_BUTTON_MASK;
        *flagsPtr |= ENTRY_AUTO_BUTTON;
    } else {
        int bool;

        if (Tcl_GetBooleanFromObj(interp, objPtr, &bool) != TCL_OK) {
            return TCL_ERROR;
        }
        *flagsPtr &= ~ENTRY_BUTTON_MASK;
        if (bool) {
            *flagsPtr |= ENTRY_REQUEST_BUTTON;
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonToObj --
 *
 * Results:
 *      The string representation of the button boolean is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ButtonToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
            char *widgRec, int offset, int flags)  
{
    int bool;
    unsigned int buttonFlags = *(int *)(widgRec + offset);

    bool = (buttonFlags & ENTRY_BUTTON_MASK);
    if (bool == ENTRY_AUTO_BUTTON) {
        return Tcl_NewStringObj("auto", 4);
    } else {
        return Tcl_NewBooleanObj(bool);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSeparator --
 *
 *      Convert the string reprsenting a separator, to its numeric
 *      form.
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
ObjToSeparator(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
               Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    const char **sepPtr = (const char **)(widgRec + offset);
    const char *string;

    string = Tcl_GetString(objPtr);
    if (string[0] == '\0') {
        *sepPtr = SEPARATOR_LIST;
    } else if (strcmp(string, "none") == 0) {
        *sepPtr = SEPARATOR_NONE;
    } else {
        *sepPtr = Blt_AssertStrdup(string);
    } 
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SeparatorToObj --
 *
 * Results:
 *      The string representation of the separator is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SeparatorToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
               char *widgRec, int offset, int flags)  
{
    char *separator = *(char **)(widgRec + offset);

    if (separator == SEPARATOR_NONE) {
        return Tcl_NewStringObj("", -1);
    } else if (separator == SEPARATOR_LIST) {
        return Tcl_NewStringObj("list", -1);
    }  else {
        return Tcl_NewStringObj(separator, -1);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeSeparator --
 *
 *      Free the CACHEDOBJ from the widget record, setting it to NULL.
 *
 * Results:
 *      The CACHEDOBJ in the widget record is set to NULL.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
FreeSeparator(ClientData clientData, Display *display, char *widgRec, 
              int offset)
{
    char **sepPtr = (char **)(widgRec + offset);

    if ((*sepPtr != SEPARATOR_LIST) && (*sepPtr != SEPARATOR_NONE)) {
        Blt_Free(*sepPtr);
        *sepPtr = SEPARATOR_NONE;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLabel --
 *
 *      Convert the string representing the label. 
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
ObjToLabel(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Tcl_Obj **labelObjPtrPtr = (Tcl_Obj **)(widgRec + offset);
    const char *string;

    string = Tcl_GetString(objPtr);
    if (string[0] != '\0') {
        TreeView *viewPtr = clientData;

        if (*labelObjPtrPtr != NULL) {
            FreeCachedObj(viewPtr, *labelObjPtrPtr);
        }
        *labelObjPtrPtr = GetCachedObj(viewPtr, objPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LabelToObj --
 *
 * Results:
 *      The string of the entry's label is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
LabelToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)  
{
    Tcl_Obj *objPtr = *(Tcl_Obj **)(widgRec + offset);

    if (objPtr == NULL) {
        Entry *entryPtr  = (Entry *)widgRec;
        const char *string;

        string = Blt_Tree_NodeLabel(entryPtr->node);
        objPtr = Tcl_NewStringObj(string, -1);
    } 
    return objPtr;
}

/*ARGSUSED*/
static void
FreeLabel(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Tcl_Obj **labelObjPtrPtr = (Tcl_Obj **)(widgRec + offset);

    if (*labelObjPtrPtr != NULL) {
        TreeView *viewPtr = clientData;

        FreeCachedObj(viewPtr, *labelObjPtrPtr);
        *labelObjPtrPtr = NULL;
    }
}

static CellStyle *
FindStyle(Tcl_Interp *interp, TreeView *viewPtr, const char *styleName)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&viewPtr->styleTable, styleName);
    if (hPtr == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find cell style \"", styleName, 
                "\"", (char *)NULL);
        }
        return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

static int
GetStyle(Tcl_Interp *interp, TreeView *viewPtr, const char *name, 
         CellStyle **stylePtrPtr)
{
    CellStyle *stylePtr;

    stylePtr = FindStyle(interp, viewPtr, name);
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    stylePtr->refCount++;
    *stylePtrPtr = stylePtr;
    return TCL_OK;
}

static INLINE Blt_Bg
GetStyleBackground(Column *colPtr)
{ 
    CellStyle *stylePtr;
    Blt_Bg bg;

    bg = NULL;
    stylePtr = colPtr->stylePtr;
    if (stylePtr != NULL) {
        bg = (stylePtr->flags & HIGHLIGHT) ? 
            stylePtr->highlightBg : stylePtr->normalBg;
    }
    if (bg == NULL) {
        bg = colPtr->viewPtr->normalBg;
    }
    return bg;
}

static INLINE Blt_Font
GetStyleFont(Column *colPtr)
{
    CellStyle *stylePtr;

    stylePtr = colPtr->stylePtr;
    if ((stylePtr != NULL) && (stylePtr->font != NULL)) {
        return stylePtr->font;
    }
    return colPtr->viewPtr->font;
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


/*
 *---------------------------------------------------------------------------
 *
 * ObjToStyles --
 *
 *      Convert the list representing the field-name style-name pairs into
 *      stylePtr's.
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
ObjToStyles(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
            Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Entry *entryPtr = (Entry *)widgRec;
    TreeView *viewPtr;
    Tcl_Obj **objv;
    int objc;
    int i;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc & 1) {
        Tcl_AppendResult(interp, "odd number of field/style pairs in \"",
                         Tcl_GetString(objPtr), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    viewPtr = entryPtr->viewPtr;
    for (i = 0; i < objc; i += 2) {
        Cell *cellPtr;
        CellStyle *stylePtr;
        Column *colPtr;
        const char *string;
        
        if (GetColumn(interp, viewPtr, objv[i], &colPtr)!=TCL_OK) {
            return TCL_ERROR;
        }
        cellPtr = GetCell(entryPtr, colPtr);
        if (cellPtr == NULL) {
            return TCL_ERROR;
        }
        string = Tcl_GetString(objv[i+1]);
        stylePtr = NULL;
        if ((*string != '\0') && (GetStyle(interp, viewPtr, string,
                &stylePtr) != TCL_OK)) {
            return TCL_ERROR;                   /* No data ??? */
        }
        if (cellPtr->stylePtr != NULL) {
            FreeStyle(cellPtr->stylePtr);
        }
        cellPtr->stylePtr = stylePtr;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StylesToObj --
 *
 * Results:
 *      The string representation of the button boolean is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
StylesToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
            char *widgRec, int offset, int flags)  
{
    Entry *entryPtr = (Entry *)widgRec;
    Cell *cellPtr;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (cellPtr = entryPtr->cells; cellPtr != NULL; 
         cellPtr = cellPtr->nextPtr) {
        const char *styleName;
        Tcl_Obj *objPtr;

        objPtr = Tcl_NewStringObj(cellPtr->colPtr->key, -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        styleName = (cellPtr->stylePtr != NULL) ? cellPtr->stylePtr->name : "";
        objPtr = Tcl_NewStringObj(styleName, -1); 
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeCachedObjProc --
 *
 *      Free the CACHEDOBJ from the widget record, setting it to NULL.
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
        TreeView *viewPtr = clientData;

        FreeCachedObj(viewPtr, *objPtrPtr);
        *objPtrPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToCachedObj --
 *
 *      Converts to a cached Tcl_Obj. Cached Tcl_Obj's are hashed,
 *      and reference counted.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToCachedObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
               Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    TreeView *viewPtr = clientData;
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);

    *objPtrPtr = GetCachedObj(viewPtr, objPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CachedObjToObj --
 *
 *      Returns the cachedObj as a string.
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
        objPtr = Tcl_NewStringObj("", -1);
    } 
    return objPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * IconChangedProc
 *
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
IconChangedProc(
    ClientData clientData,
    int x,                              /* Not used. */
    int y,                              /* Not used. */
    int width,                          /* Not used. */
    int height,                         /* Not used. */
    int imageWidth,                     /* Not used. */
    int imageHeight)                    /* Not used. */
{
    TreeView *viewPtr = clientData;

    /* We don't know what or how many entries are affected by the change of
     * this image.  So recompute the geometry of the entire tree. */
    viewPtr->flags |= (GEOMETRY | LAYOUT_PENDING);
    EventuallyRedraw(viewPtr);
}

static Icon
GetIcon(TreeView *viewPtr, const char *iconName)
{
    Blt_HashEntry *hPtr;
    int isNew;
    struct _Icon *iconPtr;

    hPtr = Blt_CreateHashEntry(&viewPtr->iconTable, iconName, &isNew);
    if (isNew) {
        Tk_Image tkImage;
        int width, height;

        tkImage = Tk_GetImage(viewPtr->interp, viewPtr->tkwin, 
                (char *)iconName, IconChangedProc, viewPtr);
        if (tkImage == NULL) {
            Blt_DeleteHashEntry(&viewPtr->iconTable, hPtr);
            return NULL;
        }
        Tk_SizeOfImage(tkImage, &width, &height);
        iconPtr = Blt_AssertMalloc(sizeof(struct _Icon));
        iconPtr->tkImage = tkImage;
        iconPtr->viewPtr = viewPtr;
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
        TreeView *viewPtr;

        viewPtr = iconPtr->viewPtr;
        Blt_DeleteHashEntry(&viewPtr->iconTable, iconPtr->hashPtr);
        Tk_FreeImage(iconPtr->tkImage);
        Blt_Free(iconPtr);
    }
}

static void
DumpIconTable(TreeView *viewPtr)
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

/*ARGSUSED*/
static void
FreeIconsProc(ClientData clientData, Display *display, char *widgRec, 
              int offset)
{
    Icon **iconsPtr = (Icon **)(widgRec + offset);

    if (*iconsPtr != NULL) {
        Icon *ip;

        for (ip = *iconsPtr; *ip != NULL; ip++) {
            FreeIcon(*ip);
        }
        Blt_Free(*iconsPtr);
        *iconsPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToIcons --
 *
 *      Convert a list of image names into Tk images.
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
ObjToIcons(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Tcl_Obj **objv;
    TreeView *viewPtr = clientData;
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
            icons[i] = GetIcon(viewPtr, Tcl_GetString(objv[i]));
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
 * IconsToObj --
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
IconsToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)  
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
FreeIconProc(
    ClientData clientData,
    Display *display,                   /* Not used. */
    char *widgRec,
    int offset)
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
 *      Convert the name of an icon into a Tk image.
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
    TreeView *viewPtr = clientData;
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
    } else {
        return Tcl_NewStringObj(Blt_Image_Name((icon)->tkImage), -1);
    }
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
    const char *string;

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

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSortMark --
 *
 *      Convert the string reprsenting a column, to its numeric form.
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
ObjToSortMark(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              Tcl_Obj *objPtr, char *widgRec, int offset, int flags) 
{
    TreeView *viewPtr = (TreeView *)widgRec;
    Column **colPtrPtr = (Column **)(widgRec + offset);
    const char *string;

    string = Tcl_GetString(objPtr);
    if (string[0] == '\0') {
        *colPtrPtr = NULL;              /* Don't display mark. */
    } else {
        if (GetColumn(interp, viewPtr, objPtr, colPtrPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortMarkToObj --
 *
 * Results:
 *      The string representation of the column is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SortMarkToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              char *widgRec, int offset, int flags)  
{
    Column *colPtr = *(Column **)(widgRec + offset);

    if (colPtr == NULL) {
        return Tcl_NewStringObj("", -1);
    } else {
        return Tcl_NewStringObj(colPtr->key, -1);
    }
}

/*ARGSUSED*/
static void
FreeSortColumnsProc(ClientData clientData, Display *display, char *widgRec, 
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
 * ObjToSortColumns --
 *
 *      Convert the string reprsenting a column, to its numeric form.
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
ObjToSortColumns(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                 Tcl_Obj *objPtr, char *widgRec, int offset, int flags)       
{
    TreeView *viewPtr = (TreeView *)widgRec;
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
 * SortColumnsToObj --
 *
 * Results:
 *      The string representation of the column is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SortColumnsToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
        objPtr = Tcl_NewStringObj(colPtr->key, -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    return listObjPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToData --
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
ObjToData(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Tcl_Obj **objv;
    Entry *entryPtr = (Entry *)widgRec;
    const char *string;
    int objc;
    int i;

    string = Tcl_GetString(objPtr);
    if (*string == '\0') {
        return TCL_OK;
    } 
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 0) {
        return TCL_OK;
    }
    if (objc & 0x1) {
        Tcl_AppendResult(interp, "data \"", string, 
                 "\" must be in even name-value pairs", (char *)NULL);
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i += 2) {
        Cell *cellPtr;
        Column *colPtr;
        TreeView *viewPtr = entryPtr->viewPtr;

        if (GetColumn(interp, viewPtr, objv[i], &colPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (colPtr == NULL) {
            continue;
        }
        if (Blt_Tree_SetValueByKey(viewPtr->interp, viewPtr->tree, 
                entryPtr->node, colPtr->key, objv[i + 1]) != TCL_OK) {
            return TCL_ERROR;
        }
        viewPtr->flags |= LAYOUT_PENDING;
        cellPtr = GetCell(entryPtr, colPtr);
        if (cellPtr == NULL) {
            AddCell(entryPtr, colPtr);
        } else {
            cellPtr->flags |= GEOMETRY;
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DataToObj --
 *
 * Results:
 *      The string representation of the data is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
DataToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          char *widgRec, int offset, int flags)  
{
    Tcl_Obj *listObjPtr, *objPtr;
    Entry *entryPtr = (Entry *)widgRec;
    Cell *cellPtr;

    /* Add the key-value pairs to a new Tcl_Obj */
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (cellPtr = entryPtr->cells; cellPtr != NULL; 
         cellPtr = cellPtr->nextPtr) {
        objPtr = Tcl_NewStringObj(cellPtr->colPtr->key, -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        if (GetData(entryPtr, cellPtr->colPtr->key, &objPtr) != TCL_OK) {
            objPtr = Tcl_NewStringObj("", -1);
            Tcl_IncrRefCount(objPtr);
        } 
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    return listObjPtr;
}

/*ARGSUSED*/
static void
FreeStyleProc(ClientData clientData, Display *display, char *widgRec, 
              int offset)
{
    CellStyle **stylePtrPtr = (CellStyle **)(widgRec + offset);

    if (*stylePtrPtr != NULL) {
        FreeStyle(*stylePtrPtr);
        *stylePtrPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToStyle --
 *
 *      Convert the name of an icon into a treeview style.
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
    TreeView *viewPtr = clientData;
    CellStyle **stylePtrPtr = (CellStyle **)(widgRec + offset);
    CellStyle *stylePtr;
    const char *string;

    stylePtr = NULL;
    string = Tcl_GetString(objPtr);
    if ((string != NULL) && (string[0] != '\0')) {
        if (GetStyle(interp, viewPtr, Tcl_GetString(objPtr), &stylePtr) 
            != TCL_OK) {
            return TCL_ERROR;
        }
        stylePtr->flags |= STYLE_DIRTY;
    }
    if (*stylePtrPtr != NULL) {
        FreeStyle(*stylePtrPtr);
    }
    *stylePtrPtr = stylePtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleToObj --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
StyleToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)  
{
    CellStyle *stylePtr = *(CellStyle **)(widgRec + offset);

    if (stylePtr == NULL) {
        return Tcl_NewStringObj("", -1);
    } else {
        return Tcl_NewStringObj(stylePtr->name, -1);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToState --
 *
 *      Converts the string representing a state into a bitflag.
 *
 * Results:
 *      The return value is a standard TCL result.  The state flags are
 *      updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToState(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
    } else if ((c == 'p') && (strncmp(string, "posted", length) == 0)) {
        mask = POSTED;
        if (cellPtr != cellPtr->viewPtr->postPtr) {
            cellPtr->viewPtr->postPtr = cellPtr;
        }
    } else {
        Tcl_AppendResult(interp, "unknown state \"", string, 
            "\": should be disabled, posted, or normal.", (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~CELL_FLAGS_MASK;
    *flagsPtr |= mask;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StateToObj --
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
StateToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)  
{
    unsigned int state = *(unsigned int *)(widgRec + offset);
    const char *string;

    if (state & DISABLED) {
        string = "disabled";
    } else if (state & POSTED) {
        string = "posted";
    } else {
        string = "normal";
    }
    return Tcl_NewStringObj(string, -1);
}

static int
Apply(
    TreeView *viewPtr,
    Entry *entryPtr,                    /* Root entry of subtree. */
    TreeViewApplyProc *proc,            /* Procedure called for each entry. */
    unsigned int flags)
{
    if ((flags & HIDDEN) && (EntryIsHidden(entryPtr))) {
        return TCL_OK;                  /* Hidden node. */
    }
    if ((flags & entryPtr->flags) & HIDDEN) {
        return TCL_OK;                  /* Hidden node. */
    }
    if ((flags | entryPtr->flags) & CLOSED) {
        Entry *childPtr, *nextPtr;

        for (childPtr = FirstChild(entryPtr, 0); childPtr != NULL; 
             childPtr = nextPtr) {
            nextPtr = childPtr->nextSiblingPtr;
            /* 
             * Get the next child before calling Apply recursively.  This
             * is because the apply callback may delete the node and its
             * link.
             */
            if (Apply(viewPtr, childPtr, proc, flags) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    if ((*proc) (viewPtr, entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteNode --
 *
 *      Delete the node and its descendants.  Don't remove the root node,
 *      though.  If the root node is specified, simply remove all its
 *      children.
 *
 *---------------------------------------------------------------------------
 */
static void
DeleteNode(TreeView *viewPtr, Blt_TreeNode node)
{
    Blt_TreeNode root;

    if (!Blt_Tree_TagTableIsShared(viewPtr->tree)) {
        Blt_Tree_ClearTags(viewPtr->tree, node);
    }
    root = Blt_Tree_RootNode(viewPtr->tree);
    if (node == root) {
        Blt_TreeNode next;
        /* Don't delete the root node. Simply clean out the tree. */
        for (node = Blt_Tree_FirstChild(node); node != NULL; node = next) {
            next = Blt_Tree_NextSibling(node);
            Blt_Tree_DeleteNode(viewPtr->tree, node);
        }           
    } else if (Blt_Tree_IsAncestor(root, node)) {
        Blt_Tree_DeleteNode(viewPtr->tree, node);
    }
}

static Tcl_Obj *
TrimPathObj(TreeView *viewPtr, Tcl_Obj *objPtr)
{
    if (viewPtr->trimLeft != NULL) {
        const char *path;
        const char *s1, *s2;

        /* Trim off characters that we don't want */
        path = Tcl_GetString(objPtr);
        /* Trim off leading character string if one exists. */
        for (s1 = path, s2 = viewPtr->trimLeft; *s2 != '\0'; s2++, s1++) {
            if (*s1 != *s2) {
                break;
            }
        }
        if (*s2 == '\0') {
            return Tcl_NewStringObj(s1, s1 - path);
        }
    }
    Tcl_IncrRefCount(objPtr);
    return objPtr;
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * SkipSeparators --
 *
 *      Moves the character pointer past one of more separators.
 *
 * Results:
 *      Returns the updates character pointer.
 *
 *---------------------------------------------------------------------------
 */
static const char *
SkipSeparators(const char *path, const char *separator, int length)
{
    while ((path[0] == separator[0]) && 
           (strncmp(path, separator, length) == 0)) {
        path += length;
    }
    return path;
}

/*
 *---------------------------------------------------------------------------
 *
 * SplitPath --
 *
 *      Returns the trailing component of the given path.  Trailing separators
 *      are ignored.
 *
 * Results:
 *      Returns the string of the tail component.
 *
 *---------------------------------------------------------------------------
 */
static int
SplitPath(TreeView *viewPtr, const char *path, long *depthPtr, 
          const char ***listPtr)
{
    int skipLen, pathLen;
    long depth;
    size_t listSize;
    const char **list;
    char *p;
    char *sep;

    pathLen = strlen(path);
    if (viewPtr->pathSep == SEPARATOR_LIST) {
        int argc;
        const char **argv;
        
        if (Tcl_SplitList(NULL, path, &argc, &argv) != TCL_OK) {
            return TCL_ERROR;
        }
        /* Convert the list from TCL memory into normal memory. This is
         * because the list could be either a split list or a (malloc-ed)
         * generated list (like below). */
        list = Blt_ConvertListToList(argc, argv);
        Tcl_Free((char *)argv);
        *listPtr = list;
        *depthPtr = (long)argc;
        return TCL_OK;
    }
    skipLen = strlen(viewPtr->pathSep);
    path = SkipSeparators(path, viewPtr->pathSep, skipLen);
    depth = pathLen / skipLen;

    listSize = (depth + 1) * sizeof(char *);
    list = Blt_AssertMalloc(listSize + (pathLen + 1));
    p = (char *)list + listSize;
    strcpy(p, path);

    sep = strstr(p, viewPtr->pathSep);
    depth = 0;
    while ((*p != '\0') && (sep != NULL)) {
        *sep = '\0';
        list[depth++] = p;
        p = (char *)SkipSeparators(sep + skipLen, viewPtr->pathSep, skipLen);
        sep = strstr(p, viewPtr->pathSep);
    }
    if (*p != '\0') {
        list[depth++] = p;
    }
    list[depth] = NULL;
    *depthPtr = depth;
    *listPtr = list;
    return TCL_OK;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * SkipSeparators --
 *
 *      Moves the character pointer past one of more separators.
 *
 * Results:
 *      Returns the updates character pointer.
 *
 *---------------------------------------------------------------------------
 */
static const char *
SkipSeparators(const char *path, const char *sep, int length)
{
    while ((*path == *sep) && (strncmp(path, sep, length) == 0)) {
        path += length;
    }
    return path;
}


/*
 *---------------------------------------------------------------------------
 *
 * SplitPath --
 *
 *      Returns a Tcl_Obj list of the path components.  Trailing and
 *      multiple separators are ignored.
 *
 *---------------------------------------------------------------------------
 */
static Tcl_Obj *
SplitPath(Tcl_Interp *interp, Tcl_Obj *pathObjPtr, const char *sep)
{
    const char *path, *p, *endPtr;
    int sepLen;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    path = Tcl_GetString(pathObjPtr);
    sepLen = strlen(sep);

    /* Skip the first separator. */
    p = SkipSeparators(path, sep, sepLen);
    for (endPtr = strstr(p, sep); ((endPtr != NULL) && (*endPtr != '\0'));
         endPtr = strstr(p, sep)) {
        Tcl_Obj *objPtr;
        
        objPtr = Tcl_NewStringObj(p, endPtr - p);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        p = SkipSeparators(endPtr + sepLen, sep, sepLen);
    }
    /* Pick up last path component */
    if (p[0] != '\0') {
        Tcl_Obj *objPtr;
        
        objPtr = Tcl_NewStringObj(p, -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    return listObjPtr;
}


static Entry *
LastEntry(TreeView *viewPtr, Entry *entryPtr, unsigned int mask)
{
    Entry *nextPtr;

    nextPtr = LastChild(entryPtr, mask);
    while (nextPtr != NULL) {
        entryPtr = nextPtr;
        nextPtr = LastChild(entryPtr, mask);
    }
    return entryPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * ShowEntryApplyProc --
 *
 * Results:
 *      Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ShowEntryApplyProc(TreeView *viewPtr, Entry *entryPtr)
{
    entryPtr->flags &= ~HIDDEN;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HideEntryApplyProc --
 *
 * Results:
 *      Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HideEntryApplyProc(TreeView *viewPtr, Entry *entryPtr)
{
    entryPtr->flags |= HIDDEN;
    return TCL_OK;
}


static void
MapAncestors(TreeView *viewPtr, Entry *entryPtr)
{
    while (entryPtr != viewPtr->rootPtr) {
        entryPtr = entryPtr->parentPtr;
        if (entryPtr->flags & (CLOSED | HIDDEN)) {
            viewPtr->flags |= LAYOUT_PENDING;
            entryPtr->flags &= ~(CLOSED | HIDDEN);
        } 
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MapAncestorsApplyProc --
 *
 *      If a node in mapped, then all its ancestors must be mapped also.
 *      This routine traverses upwards and maps each unmapped ancestor.
 *      It's assumed that for any mapped ancestor, all it's ancestors will
 *      already be mapped too.
 *
 * Results:
 *      Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
static int
MapAncestorsApplyProc(TreeView *viewPtr, Entry *entryPtr)
{
    /*
     * Make sure that all the ancestors of this entry are mapped too.
     */
    while (entryPtr != viewPtr->rootPtr) {
        entryPtr = entryPtr->parentPtr;
        if ((entryPtr->flags & (HIDDEN | CLOSED)) == 0) {
            break;              /* Assume ancestors are also mapped. */
        }
        entryPtr->flags &= ~(HIDDEN | CLOSED);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FindPath --
 *
 *      Finds the node designated by the given path.  Each path component
 *      is searched for as the tree is traversed.
 *
 *      A leading character string is trimmed off the path if it matches
 *      the one designated (see the -trimleft option).
 *
 *      If no separator is designated (see the -separator configuration
 *      option), the path is considered a TCL list.  Otherwise the each
 *      component of the path is separated by a character string.  Leading
 *      and trailing separators are ignored.  Multiple separators are
 *      treated as one.
 *
 * Results:
 *      Returns the pointer to the designated node.  If any component can't
 *      be found, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Entry *
FindPath(Tcl_Interp *interp, TreeView *viewPtr, Entry *rootPtr, Tcl_Obj *objPtr)
{
    const char *name, *path;
    int numElems, result, i;
    Tcl_Obj **elems;
    Entry *entryPtr, *parentPtr;
    Tcl_Obj *listObjPtr;
    int length;

    path = Tcl_GetStringFromObj(objPtr, &length);
    if (length == 0) {
        return rootPtr;
    }
    name = path;
    objPtr = TrimPathObj(viewPtr, objPtr);
    entryPtr = parentPtr = rootPtr;
    listObjPtr = NULL;
    if (viewPtr->pathSep == SEPARATOR_NONE) {
        entryPtr = FindChild(parentPtr, name);
        if (entryPtr == NULL) {
            goto error;
        }
        return entryPtr;
    }
    if ((viewPtr->pathSep == SEPARATOR_NONE) || (viewPtr->pathSep[0] == '\0')) {
        listObjPtr = NULL;
        result = Tcl_ListObjGetElements(interp, objPtr, &numElems, &elems);
    } else {
        listObjPtr = SplitPath(interp, objPtr, viewPtr->pathSep);
        result = Tcl_ListObjGetElements(interp, listObjPtr, &numElems, &elems);
    }
    if (result != TCL_OK) {
        goto error;
    }
    for (i = 0; i < numElems; i++) {
        const char *name;

        name = Tcl_GetString(elems[i]);
        entryPtr = FindChild(parentPtr, name);
        if (entryPtr == NULL) {
            goto error;
        }
        parentPtr = entryPtr;
    }
    if (listObjPtr != NULL) {
        Tcl_DecrRefCount(listObjPtr);
    }
    Tcl_DecrRefCount(objPtr);
    return entryPtr;
 error:
    if (listObjPtr != NULL) {
        Tcl_DecrRefCount(listObjPtr);
    }
    Tcl_DecrRefCount(objPtr);
    {
        Tcl_DString ds;
        
        Tcl_DStringInit(&ds);
        GetPathFromRoot(viewPtr, parentPtr, FALSE, &ds);
        Tcl_AppendResult(interp, "can't find parent node \"", 
                         name, "\" in \"", 
                         Tcl_DStringValue(&ds), "\"", 
                         "\"", (char *)NULL);
        Tcl_DStringFree(&ds);
    }
    return NULL;

}

/*
 *---------------------------------------------------------------------------
 *
 * NearestEntry --
 *
 *      Finds the entry closest to the given screen X-Y coordinates in the
 *      viewport.
 *
 * Results:
 *      Returns the pointer to the closest node.  If no node is visible
 *      (nodes may be hidden), NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Entry *
NearestEntry(TreeView *viewPtr, int x, int y, int selectOne)
{
    Entry *lastPtr;
    Entry **p;

    /*
     * We implicitly can pick only visible entries.  So make sure that the
     * tree exists.
     */
    if (viewPtr->numVisibleEntries == 0) {
        return NULL;
    }
    if (y < viewPtr->titleHeight) {
        return (selectOne) ? viewPtr->visibleEntries[0] : NULL;
    }
    /*
     * Since the entry positions were previously computed in world
     * coordinates, convert Y-coordinate from screen to world coordinates
     * too.
     */
    y = WORLDY(viewPtr, y);
    lastPtr = viewPtr->visibleEntries[0];
    for (p = viewPtr->visibleEntries; *p != NULL; p++) {
        Entry *entryPtr;

        entryPtr = *p;
        /*
         * If the start of the next entry starts beyond the point, use the
         * last entry.
         */
        if (entryPtr->worldY > y) {
            return (selectOne) ? entryPtr : NULL;
        }
        if (y < (entryPtr->worldY + entryPtr->height)) {
            return entryPtr;            /* Found it. */
        }
        lastPtr = entryPtr;
    }
    return (selectOne) ? lastPtr : NULL;
}


/*
 *---------------------------------------------------------------------------
 *
 * GetEntryFromSpecialId --
 *
 *      Finds the entry given a special entry ID.
 *
 *      Special IDs are:
 *      @x,y
 *      "anchor"                Selection anchor.
 *      "current"               Currently under pointer.
 *      "down"                  Move down a row.
 *      "end                    Last entry.
 *      "focus"                 Focus.
 *      "next"                  Next entry.
 *      "previous"              Previous entry.
 *      "first"                 First entry.
 *      "up"                    Move up a row.
 *      "view.bottom"           Last entry in viewport.
 *      "view.top"              First entry in viewport.
 *
 *          "active"            - Currently active node.
 *          "anchor"            - anchor of selected region.
 *          "current"           - Currently picked node in bindtable.
 *          "focus"             - The node currently with focus.
 *          "view.first"        -
 *          "view.last"         - Last open node in the entire hierarchy.
 *          "view.next"         - Next open node from the currently active
 *                                node. Wraps around back to top.
 *          "view.prev"         - Previous open node from the currently active
 *                                node. Wraps around back to bottom.
 *          "view.up"           - Next open node from the currently active
 *                                node. Does not wrap around.
 *          "view.down"        - Previous open node from the currently active
 *                                node. Does not wrap around.
 *          "view.top"          - Top of viewport.
 *          "view.bottom"       - Bottom of viewport.
 *          @x,y                - Closest node to the specified X-Y position.
 *
 *---------------------------------------------------------------------------
 */
static int
GetEntryFromSpecialId(TreeView *viewPtr, Tcl_Obj *objPtr, Entry **entryPtrPtr)
{
    const char *string;
    Entry *fromPtr, *entryPtr;
    char c;
    int length;
    unsigned int mask;
    
    entryPtr = NULL;
    fromPtr = viewPtr->fromPtr;
    if (fromPtr == NULL) {
        fromPtr = viewPtr->focusPtr;
    } 
    if (fromPtr == NULL) {
        fromPtr = viewPtr->rootPtr;
    }
    string = Tcl_GetStringFromObj(objPtr, &length);

    mask = CLOSED | HIDDEN;             /* Only navigate to visible
                                         * entries. */
    c = string[0];
    if (c == '@') {
        int x, y;

        if (Blt_GetXY(viewPtr->interp, viewPtr->tkwin, string, &x, &y) 
            == TCL_OK) {
            *entryPtrPtr = NearestEntry(viewPtr, x, y, TRUE);
        }
        return TCL_OK;
    }
    if ((c == 'a') && (strncmp(string, "anchor", length) == 0)) {
        entryPtr = viewPtr->sel.anchorPtr;
    } else if ((c == 'c') && (strncmp(string, "current", length) == 0)) {
        TreeViewObj *objPtr;

        UpdateView(viewPtr);
        objPtr = Blt_GetCurrentItem(viewPtr->bindTable);
        if ((objPtr != NULL) && ((objPtr->flags & DELETED) == 0)) {
            ItemType type;
                
            type = (ItemType)Blt_GetCurrentHint(viewPtr->bindTable);
            switch (type) {
            case ITEM_ENTRY:
            case ITEM_BUTTON:
                entryPtr = (Entry *)objPtr;
                break;
            case ITEM_CELL:
                {
                    Cell *cellPtr;
                    
                    cellPtr = (Cell *)objPtr;
                    entryPtr = cellPtr->entryPtr;
                }
                break;
            default:
                break;
            }
        }
    } else if ((c == 'd') && (strncmp(string, "down", length) == 0)) {
        entryPtr = fromPtr;
        if (viewPtr->flags & FLAT) {
            int i;
            
            i = entryPtr->flatIndex + 1;
            if (i < viewPtr->numEntries) {
                entryPtr = viewPtr->flatArr[i];
            }
        } else {
            entryPtr = NextEntry(fromPtr, mask);
            if (entryPtr == NULL) {
                entryPtr = fromPtr;
            }
            if ((entryPtr == viewPtr->rootPtr) && 
                (viewPtr->flags & HIDE_ROOT)) {
                entryPtr = NextEntry(entryPtr, mask);
            }
        }
    } else if ((c == 'e') && (strncmp(string, "end", length) == 0)) {
        if (viewPtr->flags & FLAT) {
            entryPtr = viewPtr->flatArr[viewPtr->numEntries - 1];
        } else {
            entryPtr = LastEntry(viewPtr, viewPtr->rootPtr, mask);
        }
        *entryPtrPtr = entryPtr;
        return TCL_OK;
    } else if ((c == 'f') && (length > 1) &&
               (strncmp(string, "first", length) == 0)) {
        if (viewPtr->flags & FLAT) {
            entryPtr = viewPtr->flatArr[0];
        } else {
            entryPtr = viewPtr->rootPtr;
            if (viewPtr->flags & HIDE_ROOT) {
                entryPtr = NextEntry(entryPtr, mask);
            }
        }
    } else if ((c == 'f') && (length > 1) &&
               (strncmp(string, "focus", length) == 0)) {
        entryPtr = viewPtr->focusPtr;
        /* Fix the focus if it's the root node and we're not showing the
         * root node.  */
        if ((entryPtr == viewPtr->rootPtr) && (viewPtr->flags & HIDE_ROOT)) {
            entryPtr = NextEntry(viewPtr->rootPtr, mask);
        }
    } else if ((c == 'n') && (strncmp(string, "next", length) == 0)) {
        entryPtr = fromPtr;
        if (viewPtr->flags & FLAT) {
            int i;
            
            i = entryPtr->flatIndex + 1; 
            if (i >= viewPtr->numEntries) {
                i = 0;
            }
            entryPtr = viewPtr->flatArr[i];
        } else {
            entryPtr = NextEntry(fromPtr, mask);
            if (entryPtr == NULL) {
                if (viewPtr->flags & HIDE_ROOT) {
                    entryPtr = NextEntry(viewPtr->rootPtr, mask);
                } else {
                    entryPtr = viewPtr->rootPtr;
                }
            }
        }
    } else if ((c == 'p') && (strncmp(string, "previous", length) == 0)) {
        entryPtr = fromPtr;
        if (viewPtr->flags & FLAT) {
            int i;
            
            i = entryPtr->flatIndex - 1;
            if (i < 0) {
                i = viewPtr->numEntries - 1;
            }
            entryPtr = viewPtr->flatArr[i];
        } else {
            entryPtr = PrevEntry(fromPtr, mask);
            if (entryPtr == NULL) {
                entryPtr = LastEntry(viewPtr, viewPtr->rootPtr, mask);
            }
            if ((entryPtr == viewPtr->rootPtr) && 
                (viewPtr->flags & HIDE_ROOT)) {
                entryPtr = NextEntry(entryPtr, mask);
            }
        }
    } else if ((c == 'u') && (strcmp(string, "up") == 0)) {
        entryPtr = fromPtr;
        if (viewPtr->flags & FLAT) {
            int i;
                
            i = entryPtr->flatIndex - 1;
            if (i >= 0) {
                entryPtr = viewPtr->flatArr[i];
            }
        } else {
            entryPtr = PrevEntry(fromPtr, mask);
            if (entryPtr == NULL) {
                entryPtr = fromPtr;
            }
            if ((entryPtr == viewPtr->rootPtr) && 
                (viewPtr->flags & HIDE_ROOT)) {
                entryPtr = NextEntry(entryPtr, mask);
            }
        }
    } else if ((c == 'v') && (length > 5) &&
               (strncmp(string, "view.top", length) == 0)) {
        if (viewPtr->numVisibleEntries > 0) {
            entryPtr = viewPtr->visibleEntries[0];
        }
    } else if ((c == 'v') && (length > 5) &&
               (strncmp(string, "view.bottom", length) == 0)) {
        if (viewPtr->numVisibleEntries > 0) {
            entryPtr = viewPtr->visibleEntries[viewPtr->numVisibleEntries - 1];
        } 
    } else {
        return TCL_ERROR;
    }
    *entryPtrPtr = entryPtr;
    return TCL_OK;
}

static BindTagKey *
MakeBindTag(TreeView *viewPtr, ClientData clientData, ItemType type)
{
    Blt_HashEntry *hPtr;
    int isNew;                          /* Not used. */
    BindTagKey key;

    memset(&key, 0, sizeof(key));
    key.type = type;
    key.clientData = clientData;
    hPtr = Blt_CreateHashEntry(&viewPtr->bindTagTable, &key, &isNew);
    return Blt_GetHashKey(&viewPtr->bindTagTable, hPtr);
}

static BindTagKey *
MakeStringBindTag(TreeView *viewPtr, const char *string, ItemType type)
{
    Blt_HashEntry *hPtr;
    int isNew;                          /* Not used. */
    void *keyPtr;
    
    hPtr = Blt_CreateHashEntry(&viewPtr->uidTable, string, &isNew);
    keyPtr = Blt_GetHashKey(&viewPtr->uidTable, hPtr);
    return MakeBindTag(viewPtr, (ClientData)keyPtr, type);
}

/*ARGSUSED*/
static void
AddEntryTags(TreeView *viewPtr, Entry *entryPtr, Blt_Chain tags)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    for (hPtr = Blt_Tree_FirstTag(viewPtr->tree, &cursor); hPtr != NULL; 
        hPtr = Blt_NextHashEntry(&cursor)) {
        Blt_TreeTagEntry *tPtr;

        tPtr = Blt_GetHashValue(hPtr);
        hPtr = Blt_FindHashEntry(&tPtr->nodeTable, (char *)entryPtr->node);
        if (hPtr != NULL) {
            ClientData btag;

            btag = MakeStringBindTag(viewPtr, tPtr->tagName, ITEM_ENTRY);
            Blt_Chain_Append(tags, btag);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * AddTag --
 *
 *---------------------------------------------------------------------------
 */
static int
AddTag(Tcl_Interp *interp, TreeView *viewPtr, Blt_TreeNode node,
       Tcl_Obj *objPtr)
{
    Entry *entryPtr;
    const char *string;
    char c;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'r') && (strcmp(string, "root") == 0)) {
        Tcl_AppendResult(interp, "can't add reserved tag \"",
                         string, "\"", (char *)NULL);
        return TCL_ERROR;
    }
    if (isdigit(UCHAR(c))) {
        long inode;
        
        if (Tcl_GetLongFromObj(NULL, objPtr, &inode) == TCL_OK) {
            Tcl_AppendResult(interp, "invalid tag \"", string, 
                             "\": can't be a number.", (char *)NULL);
            return TCL_ERROR;
        } 
    }
    if (c == '@') {
        Tcl_AppendResult(interp, "invalid tag \"", string, 
                "\": can't start with \"@\"", (char *)NULL);
        return TCL_ERROR;
    } 
    viewPtr->fromPtr = NULL;
    if (GetEntryFromSpecialId(viewPtr, objPtr, &entryPtr) == TCL_OK) {
        Tcl_AppendResult(interp, "invalid tag \"", string, 
                "\": is a special id", (char *)NULL);
        return TCL_ERROR;
    }
    /* Add the tag to the node. */
    Blt_Tree_AddTag(viewPtr->tree, node, string);
    return TCL_OK;
}
    
static int
GetEntryIterator(Tcl_Interp *interp, TreeView *viewPtr, Tcl_Obj *objPtr,
                 EntryIterator *iterPtr)
{
    Entry *entryPtr;
    Blt_Tree tree = viewPtr->tree;
    Blt_TreeNode node;
    Blt_TreeIterator iter;
    
#ifdef notdef
    viewPtr->fromPtr = NULL;
#endif
    iterPtr->viewPtr = viewPtr;
    if (GetEntryFromSpecialId(viewPtr, objPtr, &entryPtr) == TCL_OK) {
        iterPtr->entryPtr = entryPtr;
        iterPtr->tagType = (TAG_RESERVED | TAG_SINGLE);
    } else if (Blt_Tree_GetNodeFromObj(NULL, tree, objPtr, &node) == TCL_OK) {
        iterPtr->entryPtr = NodeToEntry(viewPtr, node);
        iterPtr->tagType = (TAG_RESERVED | TAG_SINGLE);
    } else if (Blt_Tree_GetNodeIterator(interp, tree, objPtr, &iter)
               == TCL_OK) {
        iterPtr->iter = iter;
        iterPtr->tagType = TAG_MULTIPLE;
        node = Blt_Tree_FirstTaggedNode(&iter);
        iterPtr->entryPtr = NodeToEntry(viewPtr, node);
    } else {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static Entry *
FirstTaggedEntry(EntryIterator *iterPtr)
{
    return iterPtr->entryPtr;
}

static Entry *
NextTaggedEntry(EntryIterator *iterPtr)
{
    Blt_TreeNode node;

    if ((iterPtr->tagType & TAG_MULTIPLE) == 0) {
        return NULL;
    }
    node = Blt_Tree_NextTaggedNode(&iterPtr->iter);
    if (node == NULL) {
        return NULL;
    }
    return NodeToEntry(iterPtr->viewPtr, node);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetEntryFromObj2 --
 *
 *      Converts a string into node pointer.  The string may be in one of
 *      the following forms:
 *
 *          "active"            - Currently active node.
 *          "anchor"            - anchor of selected region.
 *          "current"           - Currently picked node in bindtable.
 *          "focus"             - The node currently with focus.
 *          "view.first"        -
 *          "view.last"         - Last open node in the entire hierarchy.
 *          "view.next"         - Next open node from the currently active
 *                                node. Wraps around back to top.
 *          "view.prev"         - Previous open node from the currently active
 *                                node. Wraps around back to bottom.
 *          "view.up"           - Next open node from the currently active
 *                                node. Does not wrap around.
 *          "view.down"        - Previous open node from the currently active
 *                                node. Does not wrap around.
 *          "view.top"          - Top of viewport.
 *          "view.bottom"       - Bottom of viewport.
 *          @x,y                - Closest node to the specified X-Y position.
 *
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.  The
 *      pointer to the node is returned via nodePtr.  Otherwise, TCL_ERROR
 *      is returned and an error message is left in interpreter's result
 *      field.
 *
 *---------------------------------------------------------------------------
 */
static int
GetEntryFromObj2(Tcl_Interp *interp, TreeView *viewPtr, Tcl_Obj *objPtr, 
                 Entry **entryPtrPtr)
{
    EntryIterator iter;

    if (GetEntryIterator(interp, viewPtr, objPtr, &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    *entryPtrPtr = FirstTaggedEntry(&iter);
    if (NextTaggedEntry(&iter) != NULL) {
        Tcl_AppendResult(interp, "more than one entry tagged as \"",
                         Tcl_GetString(objPtr), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;                      /* Singleton tag. */
}

static int
GetEntryFromObj(Tcl_Interp *interp, TreeView *viewPtr, Tcl_Obj *objPtr, 
                Entry **entryPtrPtr)
{
    viewPtr->fromPtr = NULL;
    return GetEntryFromObj2(interp, viewPtr, objPtr, entryPtrPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetEntry --
 *
 *      Returns an entry based upon its index.
 *
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.  The
 *      pointer to the node is returned via nodePtr.  Otherwise, TCL_ERROR
 *      is returned and an error message is left in interpreter's result
 *      field.
 *
 *---------------------------------------------------------------------------
 */
static int
GetEntry(Tcl_Interp *interp, TreeView *viewPtr, Tcl_Obj *objPtr,
         Entry **entryPtrPtr)
{
    Entry *entryPtr;

    if (GetEntryFromObj(interp, viewPtr, objPtr, &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (entryPtr == NULL) {
        if (interp != NULL) {
            Tcl_ResetResult(interp);
            Tcl_AppendResult(interp, "can't find entry \"",
                Tcl_GetString(objPtr), "\" in \"",
                Tk_PathName(viewPtr->tkwin), "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    *entryPtrPtr = entryPtr;
    return TCL_OK;
}


static int
GetCellByIndex(Tcl_Interp *interp, TreeView *viewPtr, Tcl_Obj *objPtr, 
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
            Entry *entryPtr;

            colPtr = NearestColumn(viewPtr, x, y, NULL);
            entryPtr = NearestEntry(viewPtr, x, y, FALSE);
            if ((entryPtr != NULL) && (colPtr != NULL)) {
                *cellPtrPtr = GetCell(entryPtr, colPtr);
            }
        }
        return TCL_OK;
    } else if ((c == 'a') && (length > 1) && 
               (strncmp(string, "active", length) == 0)) {
        *cellPtrPtr = viewPtr->activeCellPtr;
        return TCL_OK;
    } else if ((c == 'f') && (strncmp(string, "focus", length) == 0)) {
        *cellPtrPtr = viewPtr->focusCellPtr;
        return TCL_OK;
    } else if ((c == 'n') && (strncmp(string, "none", length) == 0)) {
        *cellPtrPtr = NULL;
        return TCL_OK;
    } else if ((c == 'c') && (strncmp(string, "current", length) == 0)) {
        TreeViewObj *objPtr;

        objPtr = Blt_GetCurrentItem(viewPtr->bindTable);
        if ((objPtr != NULL) && ((objPtr->flags & DELETED) == 0)) {
            ItemType type;

            type = (ItemType)Blt_GetCurrentHint(viewPtr->bindTable);
            if (type == ITEM_CELL) {
                *cellPtrPtr = (Cell *)objPtr;
            }
        }
        return TCL_OK;
    } 
    return TCL_CONTINUE;
}

static int
GetCellFromObj(Tcl_Interp *interp, TreeView *viewPtr, Tcl_Obj *objPtr, 
               Cell **cellPtrPtr)
{
    int objc;
    Tcl_Obj **objv;
    Entry *entryPtr;
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
    if ((GetEntry(interp, viewPtr, objv[0], &entryPtr) != TCL_OK) ||
        (GetColumn(interp, viewPtr, objv[1], &colPtr) != TCL_OK)) {
        return TCL_ERROR;
    }
    if ((colPtr != NULL) && (entryPtr != NULL)) {
        *cellPtrPtr = GetCell(entryPtr, colPtr);
    }
    return TCL_OK;
}

static Blt_TreeNode 
GetNthNode(Blt_TreeNode parent, long position)
{
    Blt_TreeNode node;
    long count;

    count = 0;
    for(node = Blt_Tree_FirstChild(parent); node != NULL; 
        node = Blt_Tree_NextSibling(node)) {
        if (count == position) {
            return node;
        }
    }
    return Blt_Tree_LastChild(parent);
}


/*
 *---------------------------------------------------------------------------
 *
 * SelectEntryApplyProc --
 *
 *      Sets the selection flag for a node.  The selection flag is
 *      set/cleared/toggled based upon the flag set in the treeview widget.
 *
 * Results:
 *      Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectEntryApplyProc(TreeView *viewPtr, Entry *entryPtr)
{
    Blt_HashEntry *hPtr;

    if ((viewPtr->flags & HIDE_ROOT) && (entryPtr == viewPtr->rootPtr)) {
        return TCL_OK;
    }
    switch (viewPtr->sel.flags & SELECT_MASK) {
    case SELECT_CLEAR:
        DeselectEntry(viewPtr, entryPtr);
        break;

    case SELECT_SET:
        SelectEntry(viewPtr, entryPtr);
        break;

    case SELECT_TOGGLE:
        hPtr = Blt_FindHashEntry(&viewPtr->sel.table, (char *)entryPtr);
        if (hPtr != NULL) {
            DeselectEntry(viewPtr, entryPtr);
        } else {
            SelectEntry(viewPtr, entryPtr);
        }
        break;
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * PruneSelection --
 *
 *      The root entry being deleted or closed.  Deselect any of its
 *      descendants that are currently selected.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      If any of the entry's descendants are deselected the widget is
 *      redrawn and the a selection command callback is invoked (if there's
 *      one configured).
 *
 *---------------------------------------------------------------------------
 */
static void
PruneSelection(TreeView *viewPtr, Entry *rootPtr)
{
    Blt_ChainLink link, next;
    Entry *entryPtr;
    int changed;

    /* 
     * Check if any of the currently selected entries are a descendant of
     * of the current root entry.  Deselect the entry and indicate that the
     * treeview widget needs to be redrawn.
     */
    changed = FALSE;
    for (link = Blt_Chain_FirstLink(viewPtr->sel.list); link != NULL; 
         link = next) {
        next = Blt_Chain_NextLink(link);
        entryPtr = Blt_Chain_GetValue(link);
        if (Blt_Tree_IsAncestor(rootPtr->node, entryPtr->node)) {
            DeselectEntry(viewPtr, entryPtr);
            changed = TRUE;
        }
    }
    if (changed) {
        EventuallyRedraw(viewPtr);
        if (viewPtr->sel.cmdObjPtr != NULL) {
            EventuallyInvokeSelectCmd(viewPtr);
        }
    }
}

static int
ConfigureEntry(TreeView *viewPtr, Entry *entryPtr, int objc, 
               Tcl_Obj *const *objv, int flags)
{
    GC newGC;
    Blt_ChainLink link;
    Column *colPtr;
    XGCValues gcValues;
    unsigned long gcMask;

    iconsOption.clientData = viewPtr;
    cachedObjOption.clientData = viewPtr;
    labelOption.clientData = viewPtr;
    if (Blt_ConfigureWidgetFromObj(viewPtr->interp, viewPtr->tkwin, 
        entrySpecs, objc, objv, (char *)entryPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    /* 
     * Check if there are cells that need to be added 
     */
    for (link = Blt_Chain_FirstLink(viewPtr->columns); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Cell *cellPtr;

        colPtr = Blt_Chain_GetValue(link);
        cellPtr = GetCell(entryPtr, colPtr);
        if (cellPtr == NULL) {
            AddCell(entryPtr, colPtr);
        }
    }
    newGC = NULL;
    if ((entryPtr->font != NULL) || (entryPtr->color != NULL)) {
        Blt_Font font;
        XColor *colorPtr;

        font = entryPtr->font;
        if (font == NULL) {
            font = GetStyleFont(&viewPtr->treeColumn);
        }
        colorPtr = CHOOSE(viewPtr->normalFg, entryPtr->color);
        gcMask = GCForeground | GCFont;
        gcValues.foreground = colorPtr->pixel;
        gcValues.font = Blt_Font_Id(font);
        newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    }
    if (entryPtr->gc != NULL) {
        Tk_FreeGC(viewPtr->display, entryPtr->gc);
    }
    entryPtr->gc = newGC;

    /* Rule GC */
    gcMask = GCForeground;
    gcValues.foreground = entryPtr->ruleColor->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (entryPtr->ruleGC != NULL) {
        Tk_FreeGC(viewPtr->display, entryPtr->ruleGC);
    }
    entryPtr->ruleGC = newGC;

    /* Assume all changes require a new layout. */
    if (Blt_ConfigModified(entrySpecs, "-font", (char *)NULL)) {
        viewPtr->flags |= UPDATE;
        viewPtr->flags |= GEOMETRY;     /* Forces geometry on everything. */
    }
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

static void
SizeOfIcons(Icon *icons, unsigned int *widthPtr, unsigned int *heightPtr)
{
    int i;
    unsigned int iw, ih;
    
    iw = ih = 0;
    for (i = 0; i < 2; i++) {
        if (icons[i] == NULL) {
            break;
        }
        if (iw < IconWidth(icons[i])) {
            iw = IconWidth(icons[i]);
        }
        if (ih < IconHeight(icons[i])) {
            ih = IconHeight(icons[i]);
        }
    }
    *widthPtr = iw;
    *heightPtr = ih;
}

static void
ConfigureButtons(TreeView *viewPtr)
{
    GC newGC;
    Button *butPtr = &viewPtr->button;
    XGCValues gcValues;
    unsigned long gcMask;

    gcMask = GCForeground;
    gcValues.foreground = butPtr->normalFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (butPtr->normalGC != NULL) {
        Tk_FreeGC(viewPtr->display, butPtr->normalGC);
    }
    butPtr->normalGC = newGC;

    gcMask = GCForeground;
    gcValues.foreground = butPtr->activeFgColor->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (butPtr->activeGC != NULL) {
        Tk_FreeGC(viewPtr->display, butPtr->activeGC);
    }
    butPtr->activeGC = newGC;

    if (butPtr->icons != NULL) {
        unsigned int bw, bh;

        SizeOfIcons(butPtr->icons, &bw, &bh);
        butPtr->height = bh;
        butPtr->width = bw;
    } else {
        int size;
        
        if (butPtr->reqSize > 0) {
            size = butPtr->reqSize;
        } else {
            Blt_FontMetrics fm;
            Blt_Font font;
            
            font = GetStyleFont(&viewPtr->treeColumn);
            Blt_Font_GetMetrics(font, &fm);
            size = fm.linespace * 375 / 1000;
        }
        butPtr->width = butPtr->height = ODD(size);
    }
    butPtr->width  += 2 * butPtr->borderWidth;
    butPtr->height += 2 * butPtr->borderWidth;
}



static void
FreeEntryProc(DestroyData data)
{
    TreeView *viewPtr;
    Entry *entryPtr = (Entry *)data;
    
    viewPtr = entryPtr->viewPtr;
    Blt_Pool_FreeItem(viewPtr->entryPool, entryPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewEntry --
 *
 *      This procedure is called by the Tree object when a node is created
 *      and inserted into the tree.  It adds a new treeview entry field to
 *      the node.
 *
 * Results:
 *      Returns the entry.
 *
 *---------------------------------------------------------------------------
 */
static Entry *
NewEntry(TreeView *viewPtr, Blt_TreeNode node, Entry *parentPtr)
{
    Entry *entryPtr;
    int isNew;
    Blt_HashEntry *hPtr;

    hPtr = Blt_CreateHashEntry(&viewPtr->entryTable, (char *)node, &isNew);
    if (isNew) {
        /* Create the entry structure */
        entryPtr = Blt_Pool_AllocItem(viewPtr->entryPool, sizeof(Entry));
        memset(entryPtr, 0, sizeof(Entry));
        entryPtr->flags = (unsigned short) 
            (viewPtr->buttonFlags | GEOMETRY | CLOSED);
        entryPtr->viewPtr = viewPtr;
        entryPtr->hashPtr = hPtr;
        entryPtr->node = node;
        Blt_SetHashValue(hPtr, entryPtr);
        AppendEntry(parentPtr, entryPtr);
        if (ConfigureEntry(viewPtr, entryPtr, 0, NULL, 0) != TCL_OK) {
            DestroyEntry(entryPtr);
            return NULL;                    /* Error configuring the entry. */
        }
    } else {
        entryPtr = Blt_GetHashValue(hPtr);
        DetachEntry(entryPtr);
        AppendEntry(parentPtr, entryPtr);
    }
    viewPtr->flags |= LAYOUT_PENDING;
    if (viewPtr->flags & TV_SORT_AUTO) {
        /* If we're auto-sorting, schedule the view to be resorted. */
        viewPtr->flags |= SORT_PENDING;
    }
    EventuallyRedraw(viewPtr);
    return entryPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateEntry --
 *
 *      This procedure is called by the Tree object when a node is created
 *      and inserted into the tree.  It adds a new treeview entry field to
 *      the node.
 *
 * Results:
 *      Returns the entry.
 *
 *---------------------------------------------------------------------------
 */
static Entry *
CreateEntry(
    TreeView *viewPtr,
    Blt_TreeNode node,                  /* Node that has just been
                                         * created. */
    int objc,
    Tcl_Obj *const *objv,
    int flags)
{
    Entry *entryPtr;
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&viewPtr->entryTable, (char *)node);
    if (!hPtr) {
        Blt_TreeNode parent;
        Entry *parentPtr;
                            
        parent = Blt_Tree_ParentNode(node);
        if (parent != NULL) {
            hPtr = Blt_FindHashEntry(&viewPtr->entryTable, (char *)parent);
            if (hPtr == NULL) {
                parentPtr = NULL;
            } else {
                parentPtr = Blt_GetHashValue(hPtr);
            }
        } else {
            parentPtr = NULL;
        }
        entryPtr = NewEntry(viewPtr, node, parentPtr);
        if (ConfigureEntry(viewPtr, entryPtr, objc, objv, flags) != TCL_OK) {
            DestroyEntry(entryPtr);
            return NULL;                /* Error configuring the entry. */
        }
    } else {
        entryPtr = Blt_GetHashValue(hPtr);
    }
    viewPtr->flags |= LAYOUT_PENDING;
    if (viewPtr->flags & TV_SORT_AUTO) {
        /* If we're auto-sorting, schedule the view to be resorted. */
        viewPtr->flags |= SORT_PENDING;
    }
    EventuallyRedraw(viewPtr);
    return entryPtr;
}

static void
AttachChildren(TreeView *viewPtr, Entry *parentPtr)
{
    Blt_TreeNode node;

    for (node = Blt_Tree_FirstChild(parentPtr->node); node != NULL; 
         node = Blt_Tree_NextSibling(node)) {
        Entry *entryPtr;
        
        entryPtr = NewEntry(viewPtr, node, parentPtr);
        if (Blt_Tree_NodeDegree(node) > 0) {
            AttachChildren(viewPtr, entryPtr);
        }
    }
}

static int
TreeEventProc(ClientData clientData, Blt_TreeNotifyEvent *eventPtr)
{
    Blt_TreeNode node;
    TreeView *viewPtr = clientData; 

    node = Blt_Tree_GetNodeFromIndex(eventPtr->tree, eventPtr->inode);
    switch (eventPtr->type) {
    case TREE_NOTIFY_CREATE:
        if (CreateEntry(viewPtr, node, 0, NULL, 0) == NULL) {
            return TCL_ERROR;
        }
        return TCL_OK;

    case TREE_NOTIFY_DELETE:
        /*  
         * Deleting the tree node triggers a call back to free the treeview
         * entry that is associated with it.
         */
        if (node != NULL) {
            Entry *entryPtr;

            entryPtr = FindEntry(viewPtr, node);
            if (entryPtr != NULL) {
                DestroyEntry(entryPtr);
                viewPtr->flags |= LAYOUT_PENDING;
                if (viewPtr->flags & TV_SORT_AUTO) {
                    viewPtr->flags |= SORT_PENDING;
                }
                EventuallyRedraw(viewPtr);
            }
        }
        break;

    case TREE_NOTIFY_RELABEL:
        if (node != NULL) {
            Entry *entryPtr;

            entryPtr = NodeToEntry(viewPtr, node);
            entryPtr->flags |= GEOMETRY;
            if (viewPtr->flags & TV_SORT_AUTO) {
                viewPtr->flags |= SORT_PENDING;
            }
            viewPtr->flags |= LAYOUT_PENDING;
        }
        viewPtr->flags |= (LAYOUT_PENDING | RESORT);
        EventuallyRedraw(viewPtr);

    case TREE_NOTIFY_MOVE:
        /* FIXME: reattach this node. */
        break;

    case TREE_NOTIFY_SORT:
        viewPtr->rootPtr =
            NewEntry(viewPtr, Blt_Tree_RootNode(viewPtr->tree), NULL);
        AttachChildren(viewPtr, viewPtr->rootPtr);
        viewPtr->flags |= (LAYOUT_PENDING | RESORT);
        EventuallyRedraw(viewPtr);
        break;

    default:
        /* empty */
        break;
    }   
    return TCL_OK;
}

Cell *
Blt_TreeView_FindCell(Entry *entryPtr, Column *colPtr)
{
    return GetCell(entryPtr, colPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * TreeTraceProc --
 *
 *      Mirrors the individual values of the tree object (they must also be
 *      listed in the widget's columns chain). This is because it must
 *      track and save the sizes of each individual data entry, rather than
 *      re-computing all the sizes each time the widget is redrawn.
 *
 *      This procedure is called by the Tree object when a node data value
 *      is set unset.
 *
 * Results:
 *      Returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TreeTraceProc(
    ClientData clientData,
    Tcl_Interp *interp,
    Blt_TreeNode node,                  /* Node that has just been
                                         * updated. */
    Blt_TreeKey key,                    /* Key of value that's been
                                         * updated. */
    unsigned int flags)
{
    Blt_HashEntry *hPtr;
    TreeView *viewPtr = clientData; 
    Column *colPtr;
    Entry *entryPtr;
    Cell *cellPtr, *nextPtr, *lastPtr;
    
    hPtr = Blt_FindHashEntry(&viewPtr->entryTable, (char *)node);
    if (hPtr == NULL) {
        return TCL_OK;                  /* Not a node that we're interested
                                         * in. */
    }
    entryPtr = Blt_GetHashValue(hPtr);
#define TRACE_FLAGS (TREE_TRACE_WRITES | TREE_TRACE_READS | TREE_TRACE_UNSETS)
    switch (flags & TRACE_FLAGS) {
    case TREE_TRACE_WRITES:
        hPtr = Blt_FindHashEntry(&viewPtr->columnTable, key);
        if (hPtr == NULL) {
            return TCL_OK;              /* Data value isn't used by widget. */
        }
        colPtr = Blt_GetHashValue(hPtr);
        if (colPtr != &viewPtr->treeColumn) {
            cellPtr = GetCell(entryPtr, colPtr);
            if (cellPtr == NULL) {
                AddCell(entryPtr, colPtr);
            } else {
                cellPtr->flags |= GEOMETRY;
            }
        }
        entryPtr->flags |= GEOMETRY;
        viewPtr->flags |= LAYOUT_PENDING;
        if (viewPtr->flags & TV_SORT_AUTO) {
            viewPtr->flags |= SORT_PENDING;
        }
        EventuallyRedraw(viewPtr);
        break;

    case TREE_TRACE_UNSETS:
        lastPtr = NULL;
        for(cellPtr = entryPtr->cells; cellPtr != NULL; cellPtr = nextPtr) {
            nextPtr = cellPtr->nextPtr;
            if (cellPtr->colPtr->key == key) { 
                DestroyCell(viewPtr, cellPtr);
                if (lastPtr == NULL) {
                    entryPtr->cells = nextPtr;
                } else {
                    lastPtr->nextPtr = nextPtr;
                }
                entryPtr->flags |= GEOMETRY;
                viewPtr->flags |= LAYOUT_PENDING;
                EventuallyRedraw(viewPtr);
                break;
            }
            lastPtr = cellPtr;
        }               
        break;

    default:
        break;
    }
    return TCL_OK;
}

static void
ComputeCellGeometry(Cell *cellPtr, CellStyle *stylePtr)
{
    TreeView *viewPtr;
    
    viewPtr = stylePtr->viewPtr;
    cellPtr->width = cellPtr->height = 0;

    stylePtr = GetCurrentStyle(viewPtr, cellPtr->colPtr, cellPtr);
    /* Measure the cell with the specified style. */
    (*stylePtr->classPtr->geomProc)(cellPtr, stylePtr);
}

static void
ComputeCellsGeometry(Entry *entryPtr, int *widthPtr, int *heightPtr)
{
    Cell *cellPtr;
    int w, h;                           /* Computed dimensions of row. */
    TreeView *viewPtr;

    viewPtr = entryPtr->viewPtr;
    w = h = 0;
    for (cellPtr = entryPtr->cells; cellPtr != NULL; 
         cellPtr = cellPtr->nextPtr) {
        CellStyle *stylePtr;

        stylePtr = GetCurrentStyle(viewPtr, cellPtr->colPtr, cellPtr);
        if ((entryPtr->flags|viewPtr->flags|cellPtr->flags) & GEOMETRY) {
            ComputeCellGeometry(cellPtr, stylePtr);
        }
        if (cellPtr->height > h) {
            h = cellPtr->height;
        }
        w += cellPtr->width;
    }       
    *widthPtr = w;
    *heightPtr = h;
}

static void
AddTags(TreeView *viewPtr, Blt_Chain tags, Tcl_Obj *objPtr, ItemType type)
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
AppendTagsProc(
    Blt_BindTable table,
    ClientData object,                  /* Object picked. */
    ClientData hint,                    /* Context of object. */
    Blt_Chain tags)                     /* (out) List of binding ids to be
                                         * applied for this object. */
{
    TreeView *viewPtr;
    TreeViewObj *objPtr;
    ItemType type = (ItemType)hint;

    objPtr = object;
    if (objPtr->flags & DELETED) {
        return;
    }
    viewPtr = objPtr->viewPtr;
    switch (type) {
    case ITEM_BUTTON:
        {
            Entry *entryPtr = object;
            
            Blt_Chain_Append(tags, MakeBindTag(viewPtr, entryPtr, type));
            if (viewPtr->button.bindTagsObjPtr != NULL) {
                AddTags(viewPtr, tags, viewPtr->button.bindTagsObjPtr, type);
            }
        }
        break;
    case ITEM_COLUMN_TITLE:
        {
            Column *colPtr = object;
            
            Blt_Chain_Append(tags, MakeBindTag(viewPtr, colPtr, type));
            if (colPtr->bindTagsObjPtr != NULL) {
                AddTags(viewPtr, tags, colPtr->bindTagsObjPtr, type);
            }
        }
        break;
    case ITEM_COLUMN_RESIZE:
        {
            Column *colPtr = object;
            
            Blt_Chain_Append(tags, MakeBindTag(viewPtr, colPtr, type));
            Blt_Chain_Append(tags, MakeStringBindTag(viewPtr, "all", type));
        }
        break;
        
    case ITEM_ENTRY:
        {
            Entry *entryPtr = object;
            
            /* Append pointer to entry. */
            Blt_Chain_Append(tags, MakeBindTag(viewPtr, entryPtr, type)); 
            if (entryPtr->bindTagsObjPtr != NULL) {
                AddTags(viewPtr, tags, entryPtr->bindTagsObjPtr, type);
            } 
        }
        break;

    case ITEM_CELL:
        {
            Cell *cellPtr = object;
            CellStyle *stylePtr;
            
            /* Append pointer to cell. */
            Blt_Chain_Append(tags, MakeBindTag(viewPtr, cellPtr, type));
            stylePtr = GetCurrentStyle(viewPtr, cellPtr->colPtr, cellPtr);
            Blt_Chain_Append(tags, MakeBindTag(viewPtr, cellPtr->colPtr, type));
            Blt_Chain_Append(tags, MakeBindTag(viewPtr,cellPtr->entryPtr,type));
            Blt_Chain_Append(tags, MakeStringBindTag(viewPtr, stylePtr->name,
                                                     type));
            Blt_Chain_Append(tags, MakeStringBindTag(viewPtr, 
                     stylePtr->classPtr->className, type));
            Blt_Chain_Append(tags, MakeStringBindTag(viewPtr, "all", type));
        }
        break;
    default:
        fprintf(stderr, "unknown item type %d\n", type);
        break;
    }
}

/*ARGSUSED*/
static ClientData
PickItem(
    ClientData clientData,
    int x, int y,                       /* Screen coordinates of the test
                                         * point. */
    ClientData *hintPtr)                /* (out) Context of item selected:
                                         * should be ITEM_ENTRY,
                                         * ITEM_ENTRY_BUTTON,
                                         * ITEM_COLUMN_TITLE,
                                         * ITEM_COLUMN_RESIZE, or
                                         * ITEM_STYLE. */
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;
    Column *colPtr;
    ItemType type;

    if (hintPtr != NULL) {
        *hintPtr = NULL;
    }
    /* Can't trust the selected entry if nodes have been added or
     * deleted. So recompute the layout. */
    UpdateView(viewPtr);
    colPtr = NearestColumn(viewPtr, x, y, &type);
    if (colPtr == NULL) {
        return NULL;                     /* No nearest column. We're not
                                          * within the widget. */
    }
    if (type != ITEM_NONE) {
        *hintPtr = (ClientData)type;
        return colPtr;
    }
    if (viewPtr->numVisibleEntries == 0) {
        return NULL;                    /* No visible entries. */
    }
    entryPtr = NearestEntry(viewPtr, x, y, FALSE);
    if (entryPtr == NULL) {
        return NULL;                    /* No nearest entry. */
    }
    x = WORLDX(viewPtr, x);
    y = WORLDY(viewPtr, y);
    if (colPtr == &viewPtr->treeColumn) {
        type = ITEM_ENTRY;
        if (entryPtr->flags & ENTRY_BUTTON) {
            Button *butPtr = &viewPtr->button;
            int x1, x2, y1, y2;
            
            x1 = entryPtr->worldX + entryPtr->buttonX - BUTTON_PAD;
            x2 = x1 + butPtr->width + 2 * BUTTON_PAD;
            y1 = entryPtr->worldY + entryPtr->buttonY - BUTTON_PAD;
            y2 = y1 + butPtr->height + 2 * BUTTON_PAD;
            if ((x >= x1) && (x < x2) && (y >= y1) && (y < y2)) {
                type = ITEM_BUTTON;
            }
        }
        if (hintPtr != NULL) {
            *hintPtr = (ClientData)type;
        }
        return entryPtr;
    }

    {
        Cell *cellPtr;
        
        cellPtr = GetCell(entryPtr, colPtr);
        if (cellPtr != NULL) {
            if (hintPtr != NULL) {
                *hintPtr = (ClientData)ITEM_CELL;
            }
            return cellPtr;
        }
    }
    return NULL;
}


/* 
 *  +--------------------------------------+
 *  |                                     ||
 *  |     [+] [icon] [text or image]      ||
 *  |                                     ||
 *  +--------------------------------------+
 *  |cp|ep|of|w1|g|w2|g|tw|ep|cp|rw|
 * cp = column pad;
 * ep = entry pad;
 * of = level offset;
 * w1 = max icon/button width for depth + gap;
 * g  = gap;
 * w2 = max icon/button width for depth + 1 + gap;
 * tw = text width
 * rw = column rule width
 * fp = focus pad
 *      +----------------------+
 *      |fp|text/image width|fp|
 *      +----------------------+
 * |rp|ep|max of bh, ih, th|ep|rp|rh|
 *
 * th = fp + text height + fp;
 * tw = fp + text width + fp;
 * iw = always there's a gap?
 */
static void
ComputeEntryGeometry(TreeView *viewPtr, Entry *entryPtr)
{
    int entryWidth, entryHeight;
    int width, height;
    unsigned int tw, th;
    Column *colPtr = &viewPtr->treeColumn;

    if ((entryPtr->flags & GEOMETRY) || (viewPtr->flags & UPDATE)) {
        Blt_Font font;
        Blt_FontMetrics fm;
        Icon *icons;
        const char *label;

        entryPtr->iconWidth = entryPtr->iconHeight = 0;
        icons = CHOOSE(viewPtr->icons, entryPtr->icons);
        if (icons != NULL) {
            unsigned int iw, ih;

            SizeOfIcons(icons, &iw, &ih);
            entryPtr->iconWidth  = iw + 2 * ICON_PADX;
            entryPtr->iconHeight = ih + 2 * ICON_PADY;
        } else if ((icons == NULL) || (icons[0] == NULL)) {
            entryPtr->iconWidth = DEF_ICON_WIDTH;
            entryPtr->iconHeight = DEF_ICON_HEIGHT;
        }
        entryHeight = MAX(entryPtr->iconHeight, viewPtr->button.height);
        font = entryPtr->font;
        if (font == NULL) {
            font = GetStyleFont(&viewPtr->treeColumn);
        }
        FreePath(entryPtr);
        Blt_Font_GetMetrics(font, &fm);
        entryPtr->lineHeight = fm.linespace;
        entryPtr->lineHeight += 2 * (FOCUS_PAD + LABEL_PADY) + viewPtr->leader;

        label = GETLABEL(entryPtr);
        if (label[0] == '\0') {
            tw = th = entryPtr->lineHeight;
        } else {
            TextStyle ts;

            Blt_Ts_InitStyle(ts);
            Blt_Ts_SetFont(ts, font);
            if (viewPtr->flags & FLAT) {
                label = PathFromRoot(viewPtr, entryPtr);
            }
            Blt_Ts_GetExtents(&ts, label, &tw, &th);
        }
        width = entryPtr->textWidth = tw;
        height = entryPtr->textHeight = th;
        width  += 2 * (FOCUS_PAD + LABEL_PADX);
        height += 2 * (FOCUS_PAD + LABEL_PADY);
        width = ODD(width);
        if (entryPtr->reqHeight > height) {
            height = entryPtr->reqHeight;
        } 
        height = ODD(height);
        entryWidth = width;
        if (entryHeight < height) {
            entryHeight = height;
        }
        entryPtr->labelWidth = width;
        entryPtr->labelHeight = height;
    } else {
        entryHeight = entryPtr->labelHeight;
        entryWidth = entryPtr->labelWidth;
    }
    entryHeight = MAX3(entryPtr->iconHeight, entryPtr->lineHeight, 
                       entryPtr->labelHeight);

    /*  
     * Find the maximum height of the data value entries. This also has the
     * side effect of contributing the maximum width of the column.
     */
    ComputeCellsGeometry(entryPtr, &width, &height);
    if (entryHeight < height) {
        entryHeight = height;
    }
    entryPtr->width = entryWidth + PADDING(colPtr->pad) + 2 * LABEL_PADX;
    entryPtr->height = entryHeight + viewPtr->leader + 2 * LABEL_PADY + 
        entryPtr->ruleHeight;

    /*
     * Force the height of the entry to an even number. This is to make the
     * dots of the vertical line segments coincide with the start of the
     * horizontal lines.
     */
    if (entryPtr->height & 0x01) {
        entryPtr->height++;
    }
    entryPtr->flags &= ~GEOMETRY;
}

static void
ConfigureColumn(TreeView *viewPtr, Column *colPtr)
{
    Drawable drawable;
    GC newGC;
    XGCValues gcValues;
    int ruleDrawn;
    unsigned long gcMask;
    unsigned int aw, ah, iw, ih, tw, th;
    Blt_Bg bg;

    colPtr->titleWidth = colPtr->titleHeight = 0;

    gcMask = GCForeground | GCFont;
    gcValues.font = Blt_Font_Id(colPtr->titleFont);

    /* Normal title text */
    gcValues.foreground = colPtr->titleFgColor->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (colPtr->titleGC != NULL) {
        Tk_FreeGC(viewPtr->display, colPtr->titleGC);
    }
    colPtr->titleGC = newGC;

    /* Active title text */
    gcValues.foreground = colPtr->activeTitleFgColor->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (colPtr->activeTitleGC != NULL) {
        Tk_FreeGC(viewPtr->display, colPtr->activeTitleGC);
    }
    colPtr->activeTitleGC = newGC;

    /* Rule GC */
    gcValues.foreground = colPtr->ruleColor->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (colPtr->ruleGC != NULL) {
        Tk_FreeGC(viewPtr->display, colPtr->ruleGC);
    }
    colPtr->ruleGC = newGC;

    colPtr->titleWidth  = 2 * (colPtr->borderWidth + TITLE_PADX) + 
        PADDING(colPtr->pad);
    colPtr->titleHeight = 2 * (colPtr->borderWidth + TITLE_PADY);

    iw = ih = 0;
    if (colPtr->titleIcon != NULL) {
        iw = IconWidth(colPtr->titleIcon);
        ih = IconHeight(colPtr->titleIcon);
        colPtr->titleWidth += iw;
    }
    tw = th = 0;
    if (colPtr->text != NULL) {
        TextStyle ts;

        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, colPtr->titleFont);
        Blt_Ts_GetExtents(&ts, colPtr->text,  &tw, &th);
        colPtr->textWidth = tw;
        colPtr->textHeight = th;
        colPtr->titleWidth += tw;
        if (iw > 0) {
            colPtr->titleWidth += TITLE_PADX;
        }
    }
    if ((colPtr->sortUp != NULL) && (colPtr->sortDown != NULL)) {
        aw = MAX(IconWidth(colPtr->sortUp), 
                 IconWidth(colPtr->sortDown));
        ah = MAX(IconHeight(colPtr->sortUp), 
                 IconHeight(colPtr->sortDown));
    } else {
        Blt_FontMetrics fm;
        
        Blt_Font_GetMetrics(colPtr->titleFont, &fm);
        ah = fm.linespace;
        aw = ah * 60 / 100;
    }
    colPtr->titleHeight += MAX3(ih, th, ah);
    colPtr->arrowHeight = ah;
    colPtr->arrowWidth = aw;
    colPtr->titleWidth += colPtr->arrowWidth + TITLE_PADX;
    gcMask = (GCFunction | GCLineWidth | GCLineStyle | GCForeground);

    /* 
     * If the rule is active, turn it off (i.e. draw again to erase it)
     * before changing the GC.  If the color changes, we won't be able to
     * erase the old line, since it will no longer be correctly XOR-ed with
     * the background.
     */
    drawable = Tk_WindowId(viewPtr->tkwin);
    ruleDrawn = ((viewPtr->flags & RULE_ACTIVE_COLUMN) &&
                 (viewPtr->colActiveTitlePtr == colPtr) && 
                 (drawable != None));
    if (ruleDrawn) {
        DrawRule(viewPtr, colPtr, drawable);
    }
    /* XOR-ed rule column divider */ 
    gcValues.line_width = LineWidth(colPtr->ruleLineWidth);
    gcValues.foreground = GetStyleForeground(colPtr)->pixel;
    if (LineIsDashed(colPtr->ruleDashes)) {
        gcValues.line_style = LineOnOffDash;
    } else {
        gcValues.line_style = LineSolid;
    }
    gcValues.function = GXxor;

    bg = GetStyleBackground(colPtr);
    gcValues.foreground ^= Blt_Bg_BorderColor(bg)->pixel; 
    newGC = Blt_GetPrivateGC(viewPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(colPtr->ruleDashes)) {
        Blt_SetDashes(viewPtr->display, newGC, &colPtr->ruleDashes);
    }
    if (colPtr->activeRuleGC != NULL) {
        Blt_FreePrivateGC(viewPtr->display, colPtr->activeRuleGC);
    }
    colPtr->activeRuleGC = newGC;
    if (ruleDrawn) {
        DrawRule(viewPtr, colPtr, drawable);
    }
    viewPtr->flags |= UPDATE;
}

static void
FreeColumn(DestroyData data) 
{
    TreeView *viewPtr;
    Column *colPtr = (Column *)data;

    viewPtr = colPtr->viewPtr;
    if (colPtr != &viewPtr->treeColumn) {
        Blt_Free(colPtr);
    }
}

static void
DestroyColumn(Column *colPtr)
{
    TreeView *viewPtr;

    colPtr->flags |= DELETED;           /* Mark the column as destroyed. */

    viewPtr = colPtr->viewPtr;
    cachedObjOption.clientData = viewPtr;
    iconOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;

    Blt_DeleteBindings(viewPtr->bindTable, colPtr);
    /* Fix pointers to destroyed column. */
    if (viewPtr->colActiveTitlePtr == colPtr) {
        viewPtr->colActiveTitlePtr = NULL;
    }
    if (viewPtr->colActivePtr == colPtr) {
        viewPtr->colActivePtr = NULL;
    }
    if (viewPtr->colResizePtr == colPtr) {
        viewPtr->colResizePtr = NULL;
    }
    Blt_FreeOptions(columnSpecs, (char *)colPtr, viewPtr->display, 0);
    if (colPtr->titleGC != NULL) {
        Tk_FreeGC(viewPtr->display, colPtr->titleGC);
    }
    if (colPtr->ruleGC != NULL) {
        Tk_FreeGC(viewPtr->display, colPtr->ruleGC);
    }
    if (colPtr->activeRuleGC != NULL) {
        Blt_FreePrivateGC(viewPtr->display, colPtr->activeRuleGC);
    }
    if (colPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&viewPtr->columnTable, colPtr->hashPtr);
    }
    if (colPtr->link != NULL) {
        Blt_Chain_DeleteLink(viewPtr->columns, colPtr->link);
    }
    if (colPtr == &viewPtr->treeColumn) {
        colPtr->link = NULL;
    } else {
        Tcl_EventuallyFree(colPtr, FreeColumn);
    }
}

static void
DestroyColumns(TreeView *viewPtr)
{
    if (viewPtr->columns != NULL) {
        Blt_ChainLink link;
        
        for (link = Blt_Chain_FirstLink(viewPtr->columns); link != NULL;
             link = Blt_Chain_NextLink(link)) {
            Column *colPtr;

            colPtr = Blt_Chain_GetValue(link);
            colPtr->link = NULL;
            colPtr->hashPtr = NULL;
            DestroyColumn(colPtr);
        }
        Blt_Chain_Destroy(viewPtr->columns);
        viewPtr->columns = NULL;
    }
    Blt_DeleteHashTable(&viewPtr->columnTable);
}

static int
InitColumn(TreeView *viewPtr, Column *colPtr, const char *name, 
           const char *defTitle)
{
    Blt_HashEntry *hPtr;
    int isNew;

    colPtr->key = Blt_Tree_GetKey(viewPtr->tree, name);
    colPtr->text = Blt_AssertStrdup(defTitle);
    colPtr->justify = TK_JUSTIFY_CENTER;
    colPtr->relief = TK_RELIEF_FLAT;
    colPtr->borderWidth = 0;
    colPtr->pad.side1 = colPtr->pad.side2 = 0;
    colPtr->state = STATE_NORMAL;
    colPtr->weight = 1.0;
    colPtr->ruleLineWidth = 1;
    colPtr->viewPtr = viewPtr;
    colPtr->titleBW = 2;
    colPtr->titleRelief = TK_RELIEF_RAISED;
    colPtr->titleIcon = NULL;
    colPtr->sortType = SORT_DICTIONARY;
    hPtr = Blt_CreateHashEntry(&viewPtr->columnTable, name, &isNew);
    Blt_SetHashValue(hPtr, colPtr);
    colPtr->hashPtr = hPtr;
    colPtr->name = Blt_GetHashKey(&viewPtr->columnTable, hPtr);
    cachedObjOption.clientData = viewPtr;
    iconOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(viewPtr->interp, viewPtr->tkwin, name, 
        "Column", columnSpecs, 0, (Tcl_Obj **)NULL, (char *)colPtr, 0) 
        != TCL_OK) {
        DestroyColumn(colPtr);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static Column *
CreateColumn(TreeView *viewPtr, Tcl_Obj *nameObjPtr, int objc, 
             Tcl_Obj *const *objv)
{
    Column *colPtr;

    colPtr = Blt_AssertCalloc(1, sizeof(Column));
    if (InitColumn(viewPtr, colPtr, Tcl_GetString(nameObjPtr),
        Tcl_GetString(nameObjPtr)) != TCL_OK) {
        return NULL;
    }
    cachedObjOption.clientData = viewPtr;
    iconOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(viewPtr->interp, viewPtr->tkwin, 
        colPtr->key, "Column", columnSpecs, objc, objv, (char *)colPtr, 
        BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        DestroyColumn(colPtr);
        return NULL;
    }
    ConfigureColumn(viewPtr, colPtr);
    return colPtr;
}

static int
InvokeCompare(Column *colPtr, Entry *e1, Entry *e2, Tcl_Obj *cmdPtr)
{
    int result;
    Tcl_Obj *cmdObjPtr, *objPtr;
    TreeView *viewPtr;

    viewPtr = colPtr->viewPtr;
    cmdObjPtr = Tcl_DuplicateObj(cmdPtr);
    objPtr = Tcl_NewStringObj(Tk_PathName(viewPtr->tkwin), -1);
    Tcl_ListObjAppendElement(viewPtr->interp, cmdObjPtr, objPtr);
    objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(e1->node));
    Tcl_ListObjAppendElement(viewPtr->interp, cmdObjPtr, objPtr);
    objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(e2->node));
    Tcl_ListObjAppendElement(viewPtr->interp, cmdObjPtr, objPtr);
    objPtr = Tcl_NewStringObj(colPtr->key, -1);         
    Tcl_ListObjAppendElement(viewPtr->interp, cmdObjPtr, objPtr);
             
    if (viewPtr->flags & FLAT) {
        objPtr = Tcl_NewStringObj(PathFromRoot(viewPtr, e1), -1);
        Tcl_ListObjAppendElement(viewPtr->interp, cmdObjPtr, objPtr);
        objPtr = Tcl_NewStringObj(PathFromRoot(viewPtr, e2), -1);
        Tcl_ListObjAppendElement(viewPtr->interp, cmdObjPtr, objPtr);
    } else {
        objPtr = Tcl_NewStringObj(GETLABEL(e1), -1);
        Tcl_ListObjAppendElement(viewPtr->interp, cmdObjPtr, objPtr);
        objPtr = Tcl_NewStringObj(GETLABEL(e2), -1);
        Tcl_ListObjAppendElement(viewPtr->interp, cmdObjPtr, objPtr);
    }
    Tcl_IncrRefCount(cmdObjPtr);
    result = Tcl_EvalObjEx(viewPtr->interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(cmdObjPtr);
    if ((result != TCL_OK) ||
        (Tcl_GetIntFromObj(viewPtr->interp, Tcl_GetObjResult(viewPtr->interp), 
                &result) != TCL_OK)) {
        Tcl_BackgroundError(viewPtr->interp);
    }
    Tcl_ResetResult(viewPtr->interp);
    return result;
}

static TreeView *treeViewInstance;

static int
CompareIntegers(const char *key, Entry *e1, Entry *e2)
{
    int i1, i2;
    Tcl_Obj *obj1, *obj2;

    if ((GetData(e1, key, &obj1) != TCL_OK) ||
        (Tcl_GetIntFromObj(NULL, obj1, &i1) != TCL_OK)) {
        obj1 = NULL;
    }
    if ((GetData(e2, key, &obj2) != TCL_OK) ||
        (Tcl_GetIntFromObj(NULL, obj2, &i2) != TCL_OK)) {
        obj2 = NULL;
    }
    if ((obj1 != NULL) && (obj2 != NULL)) {
        return i1 - i2;                 /* Both A and B exist. */
    } 
    if (obj1 != NULL) {
        return 1;                       /* A exists, B doesn't */
    } 
    if (obj2 != NULL) {
        return -1;                      /* B exists, A doesn't */
    }
    return 0;                           /* Both A and B don't exist. */
}

static int
CompareDoubles(const char *key, Entry *e1, Entry *e2)
{
    double d1, d2;
    Tcl_Obj *obj1, *obj2;

    if ((GetData(e1, key, &obj1) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(NULL, obj1, &d1) != TCL_OK)) {
        obj1 = NULL;
    }
    if ((GetData(e2, key, &obj2) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(NULL, obj2, &d2) != TCL_OK)) {
        obj2 = NULL;
    }
    if ((obj1 != NULL) && (obj2 != NULL)) {
        return (d1 > d2) ? 1 : (d1 < d2) ? -1 : 0;
    } 
    if (obj1 != NULL) {
        return 1;                       /* A exists, B doesn't */
    } 
    if (obj2 != NULL) {
        return -1;                      /* B exists, A doesn't */
    }
    return 0;                           /* Both A and B don't exist. */
}

static int
CompareDictionaryStrings(const char *key, Entry *e1, Entry *e2)
{
    const char *s1, *s2;
    Tcl_Obj *obj1, *obj2;

    s1 = s2 = NULL;
    if (GetData(e1, key, &obj1) == TCL_OK) {
        s1 = Tcl_GetString(obj1);
    }
    if (GetData(e2, key, &obj2) == TCL_OK) {
        s2 = Tcl_GetString(obj2);
    }
    if ((s1 != NULL) && (s2 != NULL)) {
        return Blt_DictionaryCompare(s1, s2);
    } 
    if (s1 != NULL) {
        return 1;                       /* A exists, B doesn't */
    } 
    if (s2 != NULL) {
        return -1;                      /* B exists, A doesn't */
    }
    return 0;                           /* Both A and B don't exist. */
}

static int
CompareAsciiStrings(const char *key, Entry *e1, Entry *e2)
{
    const char *s1, *s2;
    Tcl_Obj *obj1, *obj2;
    
    s1 = s2 = NULL;
    if (GetData(e1, key, &obj1) == TCL_OK) {
        s1 = Tcl_GetString(obj1);
    }
    if (GetData(e2, key, &obj2) == TCL_OK) {
        s2 = Tcl_GetString(obj2);
    }
    if ((s1 != NULL) && (s2 != NULL)) {
        return strcmp(s1, s2);
    } 
    if (s1 != NULL) {
        return 1;                       /* A exists, B doesn't */
    } 
    if (s2 != NULL) {
        return -1;                      /* B exists, A doesn't */
    }
    return 0;                           /* Both A and B don't exist. */
}

static int
CompareEntries(const void *a, const void *b)
{
    Blt_ChainLink link;
    Entry *e1 = *(Entry **)a;
    Entry *e2 = *(Entry **)b;
    TreeView *viewPtr;
    int result;

    viewPtr = e1->viewPtr;
    result = 0;

    for (link = Blt_Chain_FirstLink(viewPtr->sort.order); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Column *colPtr;
        SortType sortType;
        Tcl_Obj *cmdObjPtr;

        colPtr = Blt_Chain_GetValue(link);
        sortType = colPtr->sortType;
        cmdObjPtr = NULL;
        if (sortType == SORT_COMMAND) {
            /* Get the command for sorting. */
            cmdObjPtr = colPtr->sortCmdObjPtr;
            if (cmdObjPtr == NULL) {
                cmdObjPtr = viewPtr->sort.cmdObjPtr;
            }
            if (cmdObjPtr == NULL) {
                sortType = SORT_DICTIONARY; /* If the command doesn't exist,
                                             * revert to dictionary sort. */
            }
        }
        if (colPtr == &viewPtr->treeColumn) {
            /* Handle the tree view column specially. */
            if (sortType == SORT_COMMAND) {
                result = InvokeCompare(colPtr, e1, e2, cmdObjPtr);
            } else {
                const char *s1, *s2;

                if (viewPtr->flags & FLAT) {
                    s1 = PathFromRoot(viewPtr, e1);
                    s2 = PathFromRoot(viewPtr, e2);
                } else {
                    s1 = GETLABEL(e1);
                    s2 = GETLABEL(e2);
                } 
                if (sortType == SORT_ASCII) {
                    result = strcmp(s1, s2);
                } else {
                    result = Blt_DictionaryCompare(s1, s2);
                }
            }
        } else {
            switch (sortType) {
            case SORT_ASCII:
                result = CompareAsciiStrings(colPtr->key, e1, e2);
                break;

            case SORT_DICTIONARY:
                result = CompareDictionaryStrings(colPtr->key, e1, e2);
                break;

            case SORT_INTEGER:
                result = CompareIntegers(colPtr->key, e1, e2);
                break;
            
            case SORT_REAL:
                result = CompareDoubles(colPtr->key, e1, e2);
                break;

            case SORT_COMMAND:
                result = InvokeCompare(colPtr, e1, e2, cmdObjPtr);
                break;

            default:
                fprintf(stderr, "col is %s sorttype=%d\n", colPtr->key,
                        colPtr->sortType);
                abort();
            }
        }
        if (result != 0) {
            break;                      /* Found difference */
        }
    }
    if (result == 0) {
        result = strcmp(PathFromRoot(viewPtr, e1), PathFromRoot(viewPtr, e2));
    }
    if (viewPtr->sort.decreasing) {
        return -result;
    } 
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortChildren --
 *
 *      Sorts the child entries at a given parent entry.
 *
 * Results:
 *      Returns a standard TCL result. If a temporary array of entry
 *      pointers can't be allocated TCL_ERROR is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
SortChildren(TreeView *viewPtr, Entry *parentPtr)
{
    Entry **entries, *childPtr;
    long i;

    if ((viewPtr->flags & SORTED) &&
        (viewPtr->sort.decreasing == viewPtr->sort.viewIsDecreasing)) {
        return TCL_OK;
    }
    entries = Blt_Malloc(parentPtr->numChildren * sizeof(Entry *));
    if (entries == NULL) {
        Tcl_AppendResult(viewPtr->interp, "can't allocate sorting array.", 
        (char *)NULL);
        return TCL_ERROR;               /* Out of memory. */
    }
    for (i = 0, childPtr = parentPtr->firstChildPtr; childPtr != NULL; 
         childPtr = childPtr->nextSiblingPtr, i++) {
        entries[i] = childPtr;
    }
    if (parentPtr->numChildren > 1) {
        if (viewPtr->flags & SORTED) {
            int first, last;
            
            /* 
             * The children are already sorted but in the wrong direction.
             * Reverse the entries in the array.
             */
            for (first = 0, last = parentPtr->numChildren - 1; last > first; 
                 first++, last--) {
                Entry *tmpPtr;
                
                /* Swap first and last entries */
                tmpPtr = entries[first];
                entries[first] = entries[last];
                entries[last] = tmpPtr;
            }
        } else {
            qsort(entries, parentPtr->numChildren, sizeof(Entry *), CompareEntries);
        }
    }
    parentPtr->firstChildPtr = parentPtr->lastChildPtr = NULL;
    for (i = 0; i < parentPtr->numChildren; i++) {
        Entry *entryPtr;

        entryPtr = entries[i];
        entryPtr->prevSiblingPtr = entryPtr->nextSiblingPtr = NULL;
        if (parentPtr->firstChildPtr == NULL) {
            parentPtr->firstChildPtr = parentPtr->lastChildPtr = entryPtr;
        } else {
            entryPtr->prevSiblingPtr = parentPtr->lastChildPtr;
            parentPtr->lastChildPtr->nextSiblingPtr = entryPtr;
            parentPtr->lastChildPtr = entryPtr;
        }
        if (SortChildren(viewPtr, entryPtr) != TCL_OK) {
            Blt_Free(entries);
            return TCL_ERROR;
        }
    }
    Blt_Free(entries);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortFlatView --
 *
 *      Sorts the flatten array of entries.
 *
 *---------------------------------------------------------------------------
 */
static void
SortFlatView(TreeView *viewPtr)
{
    SortInfo *sortPtr;

    sortPtr = &viewPtr->sort;
    viewPtr->flags &= ~SORT_PENDING;
    if (viewPtr->numEntries < 2) {
        return;
    }
    if (viewPtr->flags & SORTED) {
        int first, last;
        Entry *hold;

        if (sortPtr->decreasing == sortPtr->viewIsDecreasing){
            return;
        }

        /* 
         * The view is already sorted but in the wrong direction.  Reverse
         * the entries in the array.
         */
        for (first = 0, last = viewPtr->numEntries - 1; last > first; 
             first++, last--) {
            hold = viewPtr->flatArr[first];
            viewPtr->flatArr[first] = viewPtr->flatArr[last];
            viewPtr->flatArr[last] = hold;
        }
        sortPtr->viewIsDecreasing = sortPtr->decreasing;
        viewPtr->flags |= SORTED | VISIBILITY;
        return;
    }
    qsort((char *)viewPtr->flatArr, viewPtr->numEntries, sizeof(Entry *),
          (QSortCompareProc *)CompareEntries);

    sortPtr->viewIsDecreasing = sortPtr->decreasing;
    viewPtr->flags |= SORTED | VISIBILITY;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortTreeView --
 *
 *      Sorts the tree array of entries.
 *
 *---------------------------------------------------------------------------
 */
static void
SortTreeView(TreeView *viewPtr)
{
    viewPtr->flags &= ~SORT_PENDING;
    treeViewInstance = viewPtr;
    SortChildren(viewPtr, viewPtr->rootPtr);
    viewPtr->sort.viewIsDecreasing = viewPtr->sort.decreasing;
    viewPtr->flags |= SORTED | VISIBILITY;
}

/*
 * TreeView Procedures
 */

/*
 *---------------------------------------------------------------------------
 *
 * NewView --
 *
 *---------------------------------------------------------------------------
 */
static TreeView *
NewView(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Tk_Window tkwin;
    TreeView *viewPtr;
    char *name;
    int result;

    name = Tcl_GetString(objPtr);
    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), name,
        (char *)NULL);
    if (tkwin == NULL) {
        return NULL;
    }
    Tk_SetClass(tkwin, "BltTreeView");

    viewPtr = Blt_AssertCalloc(1, sizeof(TreeView));
    viewPtr->tkwin = tkwin;
    viewPtr->display = Tk_Display(tkwin);
    viewPtr->interp = interp;
    viewPtr->flags = (HIDE_ROOT | SHOW_COLUMN_TITLES | GEOMETRY | 
                      LAYOUT_PENDING | REPOPULATE);
    viewPtr->dashes = 1;
    viewPtr->highlightWidth = 2;
    viewPtr->borderWidth = 2;
    viewPtr->relief = TK_RELIEF_SUNKEN;
    viewPtr->scrollMode = BLT_SCROLL_MODE_HIERBOX;
    viewPtr->button.closeRelief = viewPtr->button.openRelief = TK_RELIEF_SOLID;
    viewPtr->xScrollUnits = viewPtr->yScrollUnits = 20;
    viewPtr->lineWidth = 1;
    viewPtr->button.borderWidth = 1;
    viewPtr->columns = Blt_Chain_Create();
    viewPtr->buttonFlags = ENTRY_AUTO_BUTTON;
    viewPtr->userStyles = Blt_Chain_Create();
    viewPtr->sort.markPtr = NULL;
    viewPtr->sel.mode = SELECT_MODE_SINGLE;
    viewPtr->sel.list = Blt_Chain_Create();
    viewPtr->sel.flags = 0;
    Blt_InitHashTable(&viewPtr->sel.table, BLT_ONE_WORD_KEYS);
    Blt_InitHashTableWithPool(&viewPtr->entryTable, BLT_ONE_WORD_KEYS);
    Blt_InitHashTable(&viewPtr->columnTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&viewPtr->iconTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&viewPtr->cachedObjTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&viewPtr->styleTable, BLT_STRING_KEYS);
    viewPtr->bindTable = Blt_CreateBindingTable(interp, tkwin, viewPtr, 
        PickItem, AppendTagsProc);
    Blt_InitHashTable(&viewPtr->bindTagTable, sizeof(BindTagKey)/sizeof(int));
    Blt_InitHashTable(&viewPtr->uidTable, BLT_STRING_KEYS);

    viewPtr->entryPool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    viewPtr->cellPool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    Blt_SetWindowInstanceData(tkwin, viewPtr);
    viewPtr->cmdToken = Tcl_CreateObjCommand(interp,Tk_PathName(viewPtr->tkwin),
        TreeViewInstCmdProc, viewPtr, TreeViewInstCmdDeleteProc);

    Tk_CreateSelHandler(viewPtr->tkwin, XA_PRIMARY, XA_STRING, SelectionProc,
        viewPtr, XA_STRING);
    Tk_CreateEventHandler(viewPtr->tkwin, ExposureMask | StructureNotifyMask |
        FocusChangeMask, TreeViewEventProc, viewPtr);
    /* 
     * Create a default style. This must exist before we can create the
     * treeview column.
     */  
    viewPtr->stylePtr = Blt_TreeView_CreateStyle(interp, viewPtr, 
        STYLE_TEXTBOX, "default", 0, (Tcl_Obj **)NULL);
    if (viewPtr->stylePtr == NULL) {
        return NULL;
    }
    /*
     * By default create a tree. The name will be the same as the widget
     * pathname.
     */
    viewPtr->tree = Blt_Tree_Open(interp, Tk_PathName(viewPtr->tkwin), 
        TREE_CREATE);
    if (viewPtr->tree == NULL) {
        return NULL;
    }
    /* Create a default column to display the view of the tree. */
    result = InitColumn(viewPtr, &viewPtr->treeColumn, "treeView", "");
    if (result != TCL_OK) {
        return NULL;
    }
    Blt_Chain_Append(viewPtr->columns, &viewPtr->treeColumn);
    return viewPtr;
}

static void
TeardownEntries(TreeView *viewPtr)
{
    Blt_HashSearch iter;
    Blt_HashEntry *hPtr;

    /* Release the current tree, removing any entry fields. */
    for (hPtr = Blt_FirstHashEntry(&viewPtr->entryTable, &iter); hPtr != NULL; 
         hPtr = Blt_NextHashEntry(&iter)) {
        Entry *entryPtr;

        entryPtr = Blt_GetHashValue(hPtr);
        entryPtr->hashPtr = NULL;
        DestroyEntry(entryPtr);
    }
    Blt_DeleteHashTable(&viewPtr->entryTable);
}


/*
 *---------------------------------------------------------------------------
 *
 * DestroyTreeView --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the internal structure of a TreeView at a safe time (when
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
DestroyTreeView(DestroyData dataPtr)    /* Pointer to the widget record. */
{
    Blt_ChainLink link;
    TreeView *viewPtr = (TreeView *)dataPtr;
    Button *butPtr;
    CellStyle *stylePtr;

    if (viewPtr->flags & SELECT_PENDING) {
        Tcl_CancelIdleCall(SelectCmdProc, viewPtr);
    }
    if (viewPtr->flags & REDRAW_PENDING) {
        Tcl_CancelIdleCall(DisplayProc, viewPtr);
    }
    TeardownEntries(viewPtr);
    if (viewPtr->tree != NULL) {
        Blt_Tree_Close(viewPtr->tree);
        viewPtr->tree = NULL;
    }
    treeOption.clientData = viewPtr;
    iconsOption.clientData = viewPtr;
    Blt_FreeOptions(viewSpecs, (char *)viewPtr, viewPtr->display, 0);
    Blt_FreeOptions(sortSpecs, (char *)viewPtr, viewPtr->display, 0);
    if (viewPtr->tkwin != NULL) {
        Tk_DeleteSelHandler(viewPtr->tkwin, XA_PRIMARY, XA_STRING);
    }
    if (viewPtr->lineGC != NULL) {
        Tk_FreeGC(viewPtr->display, viewPtr->lineGC);
    }
    if (viewPtr->focusGC != NULL) {
        Blt_FreePrivateGC(viewPtr->display, viewPtr->focusGC);
    }
    if (viewPtr->selectedGC != NULL) {
        Tk_FreeGC(viewPtr->display, viewPtr->selectedGC);
    }
    if (viewPtr->visibleEntries != NULL) {
        Blt_Free(viewPtr->visibleEntries);
    }
    if (viewPtr->flatArr != NULL) {
        Blt_Free(viewPtr->flatArr);
    }
    if (viewPtr->levelInfo != NULL) {
        Blt_Free(viewPtr->levelInfo);
    }
    butPtr = &viewPtr->button;
    if (butPtr->activeGC != NULL) {
        Tk_FreeGC(viewPtr->display, butPtr->activeGC);
    }
    if (butPtr->normalGC != NULL) {
        Tk_FreeGC(viewPtr->display, butPtr->normalGC);
    }
    if (viewPtr->stylePtr != NULL) {
        FreeStyle(viewPtr->stylePtr);
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
    DestroyColumns(viewPtr);
    Blt_DestroyBindingTable(viewPtr->bindTable);
    Blt_Chain_Destroy(viewPtr->sel.list);
    Blt_DeleteHashTable(&viewPtr->bindTagTable);
    Blt_DeleteHashTable(&viewPtr->uidTable);

    /* Remove any user-specified style that might remain. */
    for (link = Blt_Chain_FirstLink(viewPtr->userStyles); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        stylePtr = Blt_Chain_GetValue(link);
        stylePtr->link = NULL;
        FreeStyle(stylePtr);
    }
    Blt_Chain_Destroy(viewPtr->userStyles);
    if (viewPtr->comboWin != NULL) {
        Tk_DestroyWindow(viewPtr->comboWin);
    }
    Blt_DeleteHashTable(&viewPtr->styleTable);
    Blt_DeleteHashTable(&viewPtr->sel.table);
    Blt_DeleteHashTable(&viewPtr->cachedObjTable);
    Blt_Pool_Destroy(viewPtr->entryPool);
    Blt_Pool_Destroy(viewPtr->cellPool);
    DumpIconTable(viewPtr);
    Blt_Free(viewPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeViewEventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for various events on
 *      treeview widgets.
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
TreeViewEventProc(ClientData clientData, XEvent *eventPtr)
{
    TreeView *viewPtr = clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedraw(viewPtr);
            Blt_PickCurrentItem(viewPtr->bindTable);
        }
    } else if (eventPtr->type == ConfigureNotify) {
        viewPtr->flags |= LAYOUT_PENDING;
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
        if (viewPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayProc, viewPtr);
        }
        if (viewPtr->flags & SELECT_PENDING) {
            Tcl_CancelIdleCall(SelectCmdProc, viewPtr);
        }
        if (viewPtr->tkwin != NULL) {
            viewPtr->tkwin = NULL;
            Tcl_DeleteCommandFromToken(viewPtr->interp, viewPtr->cmdToken);
        }
        Tcl_EventuallyFree(viewPtr, DestroyTreeView);
    }
}

/* Selection Procedures */
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
    TreeView *viewPtr = clientData;
    Entry *entryPtr;
    int size;

    if ((viewPtr->sel.flags & SELECT_EXPORT) == 0) {
        return -1;
    }
    /*
     * Retrieve the names of the selected entries.
     */
    Tcl_DStringInit(&ds);
    if (viewPtr->sel.flags & SELECT_SORTED) {
        Blt_ChainLink link;

        for (link = Blt_Chain_FirstLink(viewPtr->sel.list); 
             link != NULL; link = Blt_Chain_NextLink(link)) {
            entryPtr = Blt_Chain_GetValue(link);
            Tcl_DStringAppend(&ds, GETLABEL(entryPtr), -1);
            Tcl_DStringAppend(&ds, "\n", -1);
        }
    } else {
        for (entryPtr = viewPtr->rootPtr; entryPtr != NULL; 
             /* Only selection non-hidden entries. It's OK is an ancestor
              * is closed. */
             entryPtr = NextEntry(entryPtr, HIDDEN)) {
            if (EntryIsSelected(viewPtr, entryPtr)) {
                Tcl_DStringAppend(&ds, GETLABEL(entryPtr), -1);
                Tcl_DStringAppend(&ds, "\n", -1);
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
 * TreeViewInstCmdDeleteProc --
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
TreeViewInstCmdDeleteProc(ClientData clientData)
{
    TreeView *viewPtr = clientData;

    /*
     * This procedure could be invoked either because the window was
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

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureTreeView --
 *
 *      Updates the GCs and other information associated with the treeview
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
ConfigureTreeView(Tcl_Interp *interp, TreeView *viewPtr)        
{
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    /*
     * GC for dotted vertical line.
     */
    gcMask = (GCForeground | GCLineWidth);
    gcValues.foreground = viewPtr->lineColor->pixel;
    gcValues.line_width = viewPtr->lineWidth;
    if (viewPtr->dashes > 0) {
        gcMask |= (GCLineStyle | GCDashList);
        gcValues.line_style = LineOnOffDash;
        gcValues.dashes = viewPtr->dashes;
    }
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (viewPtr->lineGC != NULL) {
        Tk_FreeGC(viewPtr->display, viewPtr->lineGC);
    }
    viewPtr->lineGC = newGC;

    /*
     * GC for selection. Dotted line, focus rectangle.
     */
    gcMask = GCForeground | GCLineWidth;
    gcValues.foreground = viewPtr->selectedFg->pixel;
    gcValues.line_width = viewPtr->lineWidth;
    newGC = Blt_GetPrivateGC(viewPtr->tkwin, gcMask, &gcValues);
    if (viewPtr->dashes > 0) {
        gcMask |= (GCLineStyle | GCDashList);
        gcValues.line_style = LineOnOffDash;
        gcValues.dashes = viewPtr->dashes;
    }
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (viewPtr->selectedGC != NULL) {
        Tk_FreeGC(viewPtr->display, viewPtr->selectedGC);
    }
    viewPtr->selectedGC = newGC;


    /*
     * GC for active label. Dashed outline.
     */
    gcMask = GCForeground | GCLineStyle | GCJoinStyle;
    gcValues.foreground = viewPtr->focusColor->pixel;
    gcValues.line_style = (LineIsDashed(viewPtr->focusDashes))
        ? LineOnOffDash : LineSolid;
    gcValues.join_style = JoinMiter;
    newGC = Blt_GetPrivateGC(viewPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(viewPtr->focusDashes)) {
        viewPtr->focusDashes.offset = 2;
        Blt_SetDashes(viewPtr->display, newGC, &viewPtr->focusDashes);
    }
    if (viewPtr->focusGC != NULL) {
        Blt_FreePrivateGC(viewPtr->display, viewPtr->focusGC);
    }
    viewPtr->focusGC = newGC;

    ConfigureButtons(viewPtr);
    viewPtr->inset = viewPtr->highlightWidth + viewPtr->borderWidth + INSET_PAD;

    /*
     * If the tree object was changed, we need to setup the new one.
     */
    if (Blt_ConfigModified(viewSpecs, "-tree", (char *)NULL)) {
        TeardownEntries(viewPtr);
        Blt_InitHashTableWithPool(&viewPtr->entryTable, BLT_ONE_WORD_KEYS);
        ClearSelection(viewPtr);
        if (Blt_Tree_Attach(interp, viewPtr->tree, viewPtr->treeName) 
            != TCL_OK) {
            return TCL_ERROR;
        }
        viewPtr->flags |= REPOPULATE;
    }

    /*
     * These options change the layout of the box.  Mark the widget for update.
     */
    if (Blt_ConfigModified(viewSpecs, "-font", "-linespacing", "-*width", 
        "-height", "-hide*", "-tree", "-flat", (char *)NULL)) {
        viewPtr->flags |= LAYOUT_PENDING;
    }
    /*
     * If the tree view was changed, mark all the nodes dirty (we'll be
     * switching back to either the full path name or the label) and free
     * the array representing the flattened view of the tree.
     */
    if (Blt_ConfigModified(viewSpecs, "-hideleaves", "-flat", (char *)NULL)) {
        
        viewPtr->flags |= LAYOUT_PENDING;
        if (((viewPtr->flags & FLAT) == 0) && (viewPtr->flatArr != NULL)) {
            Blt_Free(viewPtr->flatArr);
            viewPtr->flatArr = NULL;
        }
    }

    if (viewPtr->flags & REPOPULATE) {
        Blt_TreeNode root;

        Blt_Tree_CreateEventHandler(viewPtr->tree, TREE_NOTIFY_ALL, 
                TreeEventProc, viewPtr);
        TraceColumns(viewPtr);
        root = Blt_Tree_RootNode(viewPtr->tree);
        viewPtr->rootPtr = NewEntry(viewPtr, root, NULL);
        AttachChildren(viewPtr, viewPtr->rootPtr);
        viewPtr->focusPtr = viewPtr->rootPtr;
        viewPtr->sel.markPtr = viewPtr->sel.anchorPtr = NULL;
        Blt_SetFocusItem(viewPtr->bindTable, viewPtr->rootPtr, ITEM_ENTRY);

        /* Automatically open the root node. */
        if (OpenEntry(viewPtr, viewPtr->rootPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (viewPtr->flags & TV_NEW_TAGS) {
            Blt_Tree_NewTagTable(viewPtr->tree);
        }
        viewPtr->flags &= ~REPOPULATE;
    }

    if (Blt_ConfigModified(viewSpecs, "-font", "-color", (char *)NULL)) {
        ConfigureColumn(viewPtr, &viewPtr->treeColumn);
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

static void
ConfigureStyle(TreeView *viewPtr, CellStyle *stylePtr)
{
    (*stylePtr->classPtr->configProc)(stylePtr);
    stylePtr->flags |= STYLE_DIRTY;
    EventuallyRedraw(viewPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetCoordinates --
 *
 *      Determines the maximum height of all visible entries.
 *
 *      1. Sets the worldY coordinate for all mapped/open entries.
 *      2. Determines if entry needs a button.
 *      3. Collects the minimum height of open/mapped entries. (Do for all
 *         entries upon insert).
 *      4. Figures out horizontal extent of each entry (will be width of 
 *         tree view column).
 *      5. Collects maximum icon size for each level.
 *      6. The height of its vertical line
 *
 * Results:
 *      Returns 1 if beyond the last visible entry, 0 otherwise.
 *
 * Side effects:
 *      The array of visible nodes is filled.
 *
 *---------------------------------------------------------------------------
 */
static void
ResetCoordinates(TreeView *viewPtr, Entry *entryPtr, int *yPtr, long *indexPtr)
{
    int depth;

    entryPtr->worldY = -1;
    if ((entryPtr != viewPtr->rootPtr) && (EntryIsHidden(entryPtr))) {
        return;                         /* If the entry is hidden, then do
                                         * nothing. */
    }
    entryPtr->worldY = *yPtr;
    *yPtr += entryPtr->height;
    entryPtr->flatIndex = *indexPtr;
    (*indexPtr)++;
    depth = EntryDepth(viewPtr, entryPtr) + 1;
    /* Track the widest label and icon in the widget.  */
    if (viewPtr->levelInfo[depth].labelWidth < entryPtr->labelWidth) {
        viewPtr->levelInfo[depth].labelWidth = entryPtr->labelWidth;
    }
    if (viewPtr->levelInfo[depth].iconWidth < entryPtr->iconWidth) {
        viewPtr->levelInfo[depth].iconWidth = entryPtr->iconWidth;
    }
    /* The icon width needs to be odd so that the dot patterns of the
     * vertical and horizontal lines match up. */
    viewPtr->levelInfo[depth].iconWidth |= 0x01;

    if (IsOpen(entryPtr)) {
        Entry *childPtr;

        /* Recursively handle each child of this node. */
        for (childPtr = FirstChild(entryPtr, HIDDEN); childPtr != NULL; 
             childPtr = NextSibling(childPtr, HIDDEN)){
            ResetCoordinates(viewPtr, childPtr, yPtr, indexPtr);
        }
    }
}

#ifdef notdef
static void
PrintFlags(TreeView *viewPtr, const char *string)
{    
    fprintf(stderr, "%s: flags=", string);
    if (viewPtr->flags & LAYOUT_PENDING) {
        fprintf(stderr, "layout ");
    }
    if (viewPtr->flags & REDRAW_PENDING) {
        fprintf(stderr, "redraw ");
    }
    if (viewPtr->flags & SCROLLX) {
        fprintf(stderr, "xscroll ");
    }
    if (viewPtr->flags & SCROLLY) {
        fprintf(stderr, "yscroll ");
    }
    if (viewPtr->flags & FOCUS) {
        fprintf(stderr, "focus ");
    }
    if (viewPtr->flags & GEOMETRY) {
        fprintf(stderr, "geometry ");
    }
    if (viewPtr->flags & UPDATE) {
        fprintf(stderr, "update ");
    }
    if (viewPtr->flags & RESORT) {
        fprintf(stderr, "resort ");
    }
    if (viewPtr->flags & SORTED) {
        fprintf(stderr, "sorted ");
    }
    if (viewPtr->flags & SORT_PENDING) {
        fprintf(stderr, "sort_pending ");
    }
    if (viewPtr->flags & REDRAW_BORDERS) {
        fprintf(stderr, "borders ");
    }
    fprintf(stderr, "\n");
}
#endif

static void
AdjustColumns(TreeView *viewPtr)
{
    Blt_ChainLink link;
    Column *lastPtr;
    double weight;
    int growth;
    int numOpen;

    growth = VPORTWIDTH(viewPtr) - viewPtr->worldWidth;
    lastPtr = NULL;
    numOpen = 0;
    weight = 0.0;
    /* Find out how many columns still have space available */
    for (link = Blt_Chain_FirstLink(viewPtr->columns); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Column *colPtr;

        colPtr = Blt_Chain_GetValue(link);
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
        for (link = Blt_Chain_FirstLink(viewPtr->columns); 
             link != NULL; link = Blt_Chain_NextLink(link)) {
            Column *colPtr;
            int size, avail;

            colPtr = Blt_Chain_GetValue(link);
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
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeFlatLayout --
 *
 *      Recompute the layout when entries are opened/closed, inserted/deleted,
 *      or when text attributes change (such as font, linespacing).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The world coordinates are set for all the opened entries.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeFlatLayout(TreeView *viewPtr)
{
    Blt_ChainLink link;
    Entry **p;
    Entry *entryPtr;
    int count;
    int maxX;
    int y;
    long index;

    viewPtr->flags &= ~GEOMETRY;
    /* 
     * Pass 1:  Reinitialize column sizes and loop through all nodes. 
     *
     *          1. Recalculate the size of each entry as needed. 
     *          2. The maximum depth of the tree. 
     *          3. Minimum height of an entry.  Dividing this by the
     *             height of the widget gives a rough estimate of the 
     *             maximum number of visible entries.
     *          4. Build an array to hold level information to be filled
     *             in on pass 2.
     */


    /* Reset the positions of all the columns and initialize the column
     * used to track the widest value. */
    index = 0;
    for (link = Blt_Chain_FirstLink(viewPtr->columns); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        Column *colPtr;

        colPtr = Blt_Chain_GetValue(link);
        colPtr->maxWidth = 0;
        colPtr->max = SHRT_MAX;
        if (colPtr->reqMax > 0) {
            colPtr->max = colPtr->reqMax;
        }
        colPtr->index = index;
        index++;
    }

    /* If the view needs to be resorted, free the old view. */
    if ((viewPtr->flags & (LAYOUT_PENDING|RESORT|SORT_PENDING|TV_SORT_AUTO)) && 
        (viewPtr->flatArr != NULL)) {
        Blt_Free(viewPtr->flatArr);
        viewPtr->flatArr = NULL;
    }
    /* Recreate the flat view of all the open and not-hidden entries. */
    if (viewPtr->flatArr == NULL) {
        count = 0;

        /* Count the number of open entries to allocate for the array. */
        for (entryPtr = viewPtr->rootPtr; entryPtr != NULL; 
             entryPtr = NextEntry(entryPtr, HIDDEN | CLOSED)) {
            if ((viewPtr->flags & HIDE_ROOT) &&
                (entryPtr == viewPtr->rootPtr)) {
                continue;
            }
            count++;
        }
        viewPtr->numEntries = count;

        /* Allocate an array for the flat view. */
        viewPtr->flatArr = Blt_AssertCalloc((count + 1), sizeof(Entry *));
        /* Fill the array with open and not-hidden entries */
        p = viewPtr->flatArr;
        for (entryPtr = viewPtr->rootPtr; entryPtr != NULL; 
             entryPtr = NextEntry(entryPtr, HIDDEN | CLOSED)) {
            if ((viewPtr->flags & HIDE_ROOT) && 
                (entryPtr == viewPtr->rootPtr)) {
                continue;
            }
            *p++ = entryPtr;
        }
        *p = NULL;
        viewPtr->flags &= ~SORTED;      /* Indicate the view isn't
                                         * sorted. */
    }

    /* Collect the extents of the entries in the flat view. */
    viewPtr->depth = 0;
    viewPtr->minRowHeight = SHRT_MAX;
    for (p = viewPtr->flatArr; *p != NULL; p++) {
        entryPtr = *p;
        if ((viewPtr->flags|entryPtr->flags) & GEOMETRY) {
            ComputeEntryGeometry(viewPtr, entryPtr);
        }
        if (viewPtr->minRowHeight > entryPtr->height) {
            viewPtr->minRowHeight = entryPtr->height;
        }
        entryPtr->flags &= ~ENTRY_BUTTON;
    }
    if (viewPtr->levelInfo != NULL) {
        Blt_Free(viewPtr->levelInfo);
    }
    viewPtr->levelInfo = 
        Blt_AssertCalloc(viewPtr->depth+2, sizeof(LevelInfo));
    viewPtr->flags &= ~(UPDATE | RESORT);
    if (viewPtr->flags & SORT_PENDING) {
        SortFlatView(viewPtr);
    }

    viewPtr->levelInfo[0].labelWidth = viewPtr->levelInfo[0].offset = 
            viewPtr->levelInfo[0].iconWidth = 0;
    /* 
     * Pass 2:  Loop through all open/mapped nodes. 
     *
     *          1. Set world y-coordinates for entries. We must defer
     *             setting the x-coordinates until we know the maximum 
     *             icon sizes at each level.
     *          2. Compute the maximum depth of the tree. 
     *          3. Build an array to hold level information.
     */
    y = 0;                      
    count = 0;
    for(p = viewPtr->flatArr; *p != NULL; p++) {
        entryPtr = *p;
        entryPtr->flatIndex = count++;
        entryPtr->worldY = y;
        y += entryPtr->height;
        if (viewPtr->levelInfo[0].labelWidth < entryPtr->labelWidth) {
            viewPtr->levelInfo[0].labelWidth = entryPtr->labelWidth;
        }
        if (viewPtr->levelInfo[0].iconWidth < entryPtr->iconWidth) {
            viewPtr->levelInfo[0].iconWidth = entryPtr->iconWidth;
        }
    }
    viewPtr->levelInfo[0].iconWidth |= 0x01;
    viewPtr->worldHeight = y;           /* Set the scroll height of the
                                         * hierarchy. */
    if (viewPtr->worldHeight < 1) {
        viewPtr->worldHeight = 1;
    }
    maxX = viewPtr->levelInfo[0].iconWidth + viewPtr->levelInfo[0].labelWidth;
    viewPtr->treeColumn.maxWidth = maxX;
    viewPtr->treeWidth = maxX;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeTreeLayout --
 *
 *      Recompute the layout when entries are opened/closed,
 *      inserted/deleted, or when text attributes change (such as font,
 *      linespacing).
 *
 *      |    [+] ... [icon] [text/image]  |
 *      column pad 
 *      entry pad
 *      level offset
 *      button 
 *      gap
 *      icon
 *      gap
 *      text/image
 *      
 * Results:
 *      None.
 *
 * Side effects:
 *      The world coordinates are set for all the opened entries.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeTreeLayout(TreeView *viewPtr)
{
    int y;
    Blt_ChainLink link;
    Entry *entryPtr;
    long index;

    /* 
     * Pass 1:  Reinitialize column sizes and loop through all nodes. 
     *
     *          1. Recalculate the size of each entry as needed. 
     *          2. The maximum depth of the tree. 
     *          3. Minimum height of an entry.  Dividing this by the
     *             height of the widget gives a rough estimate of the 
     *             maximum number of visible entries.
     *          4. Build an array to hold level information to be filled
     *             in on pass 2.
     */
    index = 0;
    for (link = Blt_Chain_FirstLink(viewPtr->columns); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        Column *colPtr;
        
        colPtr = Blt_Chain_GetValue(link);
        colPtr->maxWidth = 0;
        colPtr->max = SHRT_MAX;
        if (colPtr->reqMax > 0) {
            colPtr->max = colPtr->reqMax;
        }
        colPtr->index = index;
        index++;
    }

    /* Get the maximum depth of the tree.  We'll use this to allocate slots
     * in the level information array. */
    viewPtr->minRowHeight = SHRT_MAX;
    viewPtr->depth = 0;

    for (entryPtr = viewPtr->rootPtr; entryPtr != NULL; 
         entryPtr = NextEntry(entryPtr, 0)) {
        size_t depth;

        if ((viewPtr->flags|entryPtr->flags) & GEOMETRY) {
            ComputeEntryGeometry(viewPtr, entryPtr);
        }
        if (viewPtr->minRowHeight > entryPtr->height) {
            viewPtr->minRowHeight = entryPtr->height;
        }
        /* Set the flag that indicates if the entry needs a button drawn.
         * This is determined from either the ENTRY_REQUEST_BUTTON flag, or
         * the ENTRY_AUTO_BUTTON flag and the node has children. */
        entryPtr->flags &= ~ENTRY_BUTTON;
        if ((entryPtr->flags & ENTRY_REQUEST_BUTTON) ||
            ((entryPtr->flags & ENTRY_AUTO_BUTTON) &&
             (FirstChild(entryPtr, HIDDEN) != NULL))) {
            entryPtr->flags |= ENTRY_BUTTON;
        }
        depth = EntryDepth(viewPtr, entryPtr);
        /* Track the overall depth of the tree. */
        if (viewPtr->depth < depth) {
            viewPtr->depth = depth;
        }
    }

    if (viewPtr->levelInfo != NULL) {
        Blt_Free(viewPtr->levelInfo);
    }
    viewPtr->levelInfo = Blt_AssertCalloc(viewPtr->depth+2, sizeof(LevelInfo));
    viewPtr->flags &= ~(GEOMETRY | RESORT);

    if (viewPtr->flags & SORT_PENDING) {
        SortTreeView(viewPtr);
    }
    /* 
     * Pass 2:  Loop through all open/mapped nodes. 
     *
     *          1. Set world y-coordinates for entries. We must defer
     *             setting the x-coordinates until we know the maximum 
     *             icon sizes at each level.
     *          2. Compute the maximum depth of the tree. 
     *          3. Build an array to hold level information.
     */
    y = 0;
    if (viewPtr->flags & HIDE_ROOT) {
        /* If the root entry is to be hidden, cheat by offsetting the
         * y-coordinates by the height of the entry. */
        y = -(viewPtr->rootPtr->height);
    } 
    index = 0;
    ResetCoordinates(viewPtr, viewPtr->rootPtr, &y, &index);
    viewPtr->worldHeight = y;           /* Set the scroll height of the
                                         * hierarchy. */
    if (viewPtr->worldHeight < 1) {
        viewPtr->worldHeight = 1;
    }
    {
        int maxX;
        int sum;
        size_t i;

        sum = maxX = 0;
        i = 0;
        for (/*empty*/; i <= (viewPtr->depth + 1); i++) {
            int x;

            sum += viewPtr->levelInfo[i].iconWidth;
            if (i <= viewPtr->depth) {
                viewPtr->levelInfo[i + 1].offset = sum;
            }
            x = sum;
            if (((viewPtr->flags & HIDE_ROOT) == 0) || (i > 1)) {
                x += viewPtr->levelInfo[i].labelWidth;
            }
            if (x > maxX) {
                maxX = x;
            }
        }
        viewPtr->treeColumn.maxWidth = maxX;
        viewPtr->treeWidth = maxX;
    }
    viewPtr->flags &= ~GEOMETRY;
}

static void
LayoutColumns(TreeView *viewPtr)
{
    Blt_ChainLink link;
    int sum;

    /* The width of the widget (in world coordinates) is the sum of the
     * column widths. */

    viewPtr->worldWidth = viewPtr->titleHeight = 0;
    sum = 0;
    for (link = Blt_Chain_FirstLink(viewPtr->columns); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Column *colPtr;
        
        colPtr = Blt_Chain_GetValue(link);
        colPtr->width = 0;
        if (colPtr->flags & HIDDEN) {
            continue;
        }
        if ((viewPtr->flags & SHOW_COLUMN_TITLES) &&
            (viewPtr->titleHeight < colPtr->titleHeight)) {
            viewPtr->titleHeight = colPtr->titleHeight;
        }
        if (colPtr->reqWidth > 0) {
            colPtr->width = colPtr->reqWidth;
        } else {
            int colWidth;

            colWidth = colPtr->maxWidth + PADDING(colPtr->pad);
            /* The computed width of a column is the maximum of the title
             * width and the widest entry. */
            colPtr->width = MAX(colPtr->titleWidth, colWidth);
            /* Check that the width stays within any constraints that have
             * been set. */
            if ((colPtr->reqMin > 0) && (colPtr->reqMin > colPtr->width)) {
                colPtr->width = colPtr->reqMin;
            }
            if ((colPtr->reqMax > 0) && (colPtr->reqMax < colPtr->width)) {
                colPtr->width = colPtr->reqMax;
            }
        }
        colPtr->width += 2 * colPtr->titleBW;
        colPtr->worldX = sum;
        sum += colPtr->width;
    }
    viewPtr->worldWidth = sum;
    if (VPORTWIDTH(viewPtr) > sum) {
        AdjustColumns(viewPtr);
    }
    sum = 0;
    for (link = Blt_Chain_FirstLink(viewPtr->columns); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Column *colPtr;

        colPtr = Blt_Chain_GetValue(link);
        colPtr->worldX = sum;
        sum += colPtr->width;
    }
    if (viewPtr->titleHeight > 0) {
        /* If any headings are displayed, add some extra padding to the
         * height. */
        viewPtr->titleHeight += 4;
    }
    if (viewPtr->yScrollUnits < 1) {
        viewPtr->yScrollUnits = 1;
    }
    if (viewPtr->xScrollUnits < 1) {
        viewPtr->xScrollUnits = 1;
    }
    if (viewPtr->worldWidth < 1) {
        viewPtr->worldWidth = 1;
    }
    viewPtr->flags |= SCROLL_PENDING;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeLayout --
 *
 *      Recompute the layout when entries are opened/closed,
 *      inserted/deleted, or when text attributes change (such as font,
 *      linespacing).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The world coordinates are set for all the opened entries.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeLayout(TreeView *viewPtr)
{
    Blt_ChainLink link;
    Column *colPtr;
    Entry *entryPtr;
    Cell *cellPtr;


    if (viewPtr->flags & FLAT) {
        ComputeFlatLayout(viewPtr);
    } else {
        ComputeTreeLayout(viewPtr);
    }
    /*
     * Determine the width of each column based upon the entries that as
     * open (not hidden).  The widest entry in a column determines the
     * width of that column.
     */

    /* Reset the column sizes to 0. */
    for (link = Blt_Chain_FirstLink(viewPtr->columns); 
         link != NULL; link = Blt_Chain_NextLink(link)) {
        colPtr = Blt_Chain_GetValue(link);
        colPtr->maxWidth = 0;
        colPtr->max = SHRT_MAX;
        if (colPtr->reqMax > 0) {
            colPtr->max = colPtr->reqMax;
        }
    }
    /* The treeview column width was computed earlier. */
    viewPtr->treeColumn.maxWidth = viewPtr->treeWidth;

    /* 
     * Look at all open/non-hidden entries and their cells.  Determine the
     * column widths by tracking the maximum width cell in each column.
     */
    for (entryPtr = viewPtr->rootPtr; entryPtr != NULL; 
         entryPtr = NextEntry(entryPtr, HIDDEN | CLOSED)) {
        for (cellPtr = entryPtr->cells; cellPtr != NULL; 
             cellPtr = cellPtr->nextPtr) {
            if (cellPtr->colPtr->maxWidth < cellPtr->width) {
                cellPtr->colPtr->maxWidth = cellPtr->width;
            }
        }           
    }
    /* Now layout the columns with the proper sizes. */
    LayoutColumns(viewPtr);
    viewPtr->flags &= ~LAYOUT_PENDING;
    /* Changes to the layout mean that scrolling changes too. */
    viewPtr->flags |= SCROLL_PENDING;
}


/*
 *---------------------------------------------------------------------------
 *
 * ComputeVisibleEntries --
 *
 *      The entries visible in the viewport (the widget's window) are
 *      inserted into the array of visible nodes.
 *
 * Results:
 *      Returns 1 if beyond the last visible entry, 0 otherwise.
 *
 * Side effects:
 *      The array of visible nodes is filled.
 *
 *---------------------------------------------------------------------------
 */
static int
ComputeVisibleEntries(TreeView *viewPtr)
{
    int height;
    int numSlots;
    int maxX;
    int xOffset, yOffset;

    xOffset = Blt_AdjustViewport(viewPtr->xOffset, viewPtr->worldWidth,
        VPORTWIDTH(viewPtr), viewPtr->xScrollUnits, viewPtr->scrollMode);
    yOffset = Blt_AdjustViewport(viewPtr->yOffset, 
        viewPtr->worldHeight, VPORTHEIGHT(viewPtr), viewPtr->yScrollUnits, 
        viewPtr->scrollMode);

    if ((xOffset != viewPtr->xOffset) || (yOffset != viewPtr->yOffset)) {
        viewPtr->yOffset = yOffset;
        viewPtr->xOffset = xOffset;
    }
    height = VPORTHEIGHT(viewPtr);

    /* Allocate worst case number of slots for entry array. */
    numSlots = (height / viewPtr->minRowHeight) + 3;
    if (numSlots != viewPtr->numVisibleEntries) {
        if (viewPtr->visibleEntries != NULL) {
            Blt_Free(viewPtr->visibleEntries);
        }
        viewPtr->visibleEntries = Blt_AssertCalloc(numSlots + 1, sizeof(Entry *));
    }
    viewPtr->numVisibleEntries = 0;
    viewPtr->visibleEntries[numSlots] = viewPtr->visibleEntries[0] = NULL;

    if (viewPtr->rootPtr->flags & HIDDEN) {
        return TCL_OK;                  /* Root node is hidden. */
    }
    /* Find the node where the view port starts. */
    if (viewPtr->flags & FLAT) {
        Entry **epp;
        int y;
        long i;
        
        /* Map the positions of the entries.  */
        y = 0;
        for (i = 0; i < viewPtr->numEntries; i++) {
            Entry *entryPtr;
            
            entryPtr = viewPtr->flatArr[i];
            entryPtr->worldY = y;
            y += entryPtr->height;
        }

        /* Find the starting entry visible in the viewport. It can't be
         * hidden or any of it's ancestors closed. */
    again:
        for (epp = viewPtr->flatArr; *epp != NULL; epp++) {
            if (((*epp)->worldY + (*epp)->height) > viewPtr->yOffset) {
                break;
            }
        }           
        /*
         * If we can't find the starting node, then the view must be
         * scrolled down, but some nodes were deleted.  Reset the view back
         * to the top and try again.
         */
        if (*epp == NULL) {
            if (viewPtr->yOffset == 0) {
                return TCL_OK;          /* All entries are hidden. */
            }
            viewPtr->yOffset = 0;
            goto again;
        }

        maxX = 0;
        height += viewPtr->yOffset;
        for (/*empty*/; *epp != NULL; epp++) {
            int x;

            (*epp)->worldX = LEVELOFFSET(0) + viewPtr->treeColumn.worldX;
            x = (*epp)->worldX + ICONWIDTH(0) + (*epp)->width;
            if (x > maxX) {
                maxX = x;
            }
            if ((*epp)->worldY >= height) {
                break;
            }
            assert(viewPtr->numVisibleEntries < numSlots);
            viewPtr->visibleEntries[viewPtr->numVisibleEntries] = *epp;
            viewPtr->numVisibleEntries++;
        }
        viewPtr->visibleEntries[viewPtr->numVisibleEntries] = NULL;
    } else {
        Entry *entryPtr;
        int y;
        long index;
        
        y = 0;
        if (viewPtr->flags & HIDE_ROOT) {
            /* If the root entry is to be hidden, cheat by offsetting the
             * y-coordinates by the height of the entry. */
            y = -(viewPtr->rootPtr->height);
        } 
        index = 0;
        ResetCoordinates(viewPtr, viewPtr->rootPtr, &y, &index);

        entryPtr = viewPtr->rootPtr;
        while ((entryPtr->worldY + entryPtr->height) <= viewPtr->yOffset) {
            for (entryPtr = LastChild(entryPtr, HIDDEN | CLOSED);
                 entryPtr != NULL;
                 entryPtr = PrevSibling(entryPtr, HIDDEN | CLOSED)) {
                if (entryPtr->worldY <= viewPtr->yOffset) {
                    break;
                }
            }
            /*
             * If we can't find the starting node, then the view must be
             * scrolled down, but some nodes were deleted.  Reset the view
             * back to the top and try again.
             */
            if (entryPtr == NULL) {
                if (viewPtr->yOffset == 0) {
                    return TCL_OK;      /* All entries are hidden. */
                }
                viewPtr->yOffset = 0;
                continue;
            }
        }
        
        height += viewPtr->yOffset;
        maxX = 0;
        viewPtr->treeColumn.maxWidth = viewPtr->treeWidth;

        for (/*empty*/; entryPtr != NULL;
             entryPtr = NextEntry(entryPtr, HIDDEN | CLOSED)) {
            int x;
            int level;

            /*
             * Compute and save the entry's X-coordinate now that we know
             * the maximum level offset for the entire widget.
             */
            level = EntryDepth(viewPtr, entryPtr);
            entryPtr->worldX = LEVELOFFSET(level) + viewPtr->treeColumn.worldX;
            x = entryPtr->worldX + ICONWIDTH(level) + ICONWIDTH(level+1) +
                entryPtr->width;
            if (x > maxX) {
                maxX = x;
            }
            if (entryPtr->worldY >= height) {
                break;
            }
            assert(viewPtr->numVisibleEntries < numSlots);
            viewPtr->visibleEntries[viewPtr->numVisibleEntries] = entryPtr;
            viewPtr->numVisibleEntries++;
        }
        viewPtr->visibleEntries[viewPtr->numVisibleEntries] = NULL;
    }
    /*
     * Note: It's assumed that the view port always starts at or over an
     *       entry.  Check that a change in the hierarchy (e.g. closing a
     *       node) hasn't left the viewport beyond the last entry.  If so,
     *       adjust the viewport to start on the last entry.
     */
    if (viewPtr->xOffset > (viewPtr->worldWidth - viewPtr->xScrollUnits)) {
        viewPtr->xOffset = viewPtr->worldWidth - viewPtr->xScrollUnits;
    }
    if (viewPtr->yOffset > (viewPtr->worldHeight - viewPtr->yScrollUnits)) {
        viewPtr->yOffset = viewPtr->worldHeight - viewPtr->yScrollUnits;
    }
    viewPtr->xOffset = Blt_AdjustViewport(viewPtr->xOffset, 
        viewPtr->worldWidth, VPORTWIDTH(viewPtr), viewPtr->xScrollUnits, 
        viewPtr->scrollMode);
    viewPtr->yOffset = Blt_AdjustViewport(viewPtr->yOffset,
        viewPtr->worldHeight, VPORTHEIGHT(viewPtr), viewPtr->yScrollUnits,
        viewPtr->scrollMode);

    viewPtr->flags &= ~VISIBILITY;
    Blt_PickCurrentItem(viewPtr->bindTable);
    return TCL_OK;
}

static void
UpdateView(TreeView *viewPtr)
{
    if (viewPtr->flags & LAYOUT_PENDING) {
        ComputeLayout(viewPtr);
        viewPtr->flags |= VISIBILITY;
    } 
    if (viewPtr->flags & SCROLL_PENDING) {
        int w, h;

        w = VPORTWIDTH(viewPtr);
        h = VPORTHEIGHT(viewPtr);
        if ((viewPtr->flags & SCROLLX) && (viewPtr->xScrollCmdObjPtr != NULL)) {
            Blt_UpdateScrollbar(viewPtr->interp, viewPtr->xScrollCmdObjPtr, 
                viewPtr->xOffset, viewPtr->xOffset + w, viewPtr->worldWidth);
        }
        if ((viewPtr->flags & SCROLLY) && (viewPtr->yScrollCmdObjPtr != NULL)) {
            Blt_UpdateScrollbar(viewPtr->interp, viewPtr->yScrollCmdObjPtr,
                viewPtr->yOffset, viewPtr->yOffset + h, viewPtr->worldHeight);
        }
        viewPtr->flags &= ~SCROLL_PENDING;
        viewPtr->flags |= VISIBILITY;
    }
    if (viewPtr->flags & VISIBILITY) {
        ComputeVisibleEntries(viewPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawLines --
 *
 *      Draws vertical lines for the ancestor nodes.  While the entry of
 *      the ancestor may not be visible, its vertical line segment does
 *      extent into the viewport.  So walk back up the hierarchy drawing
 *      lines until we get to the root.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Vertical lines are drawn for the ancestor nodes.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawLines(
    TreeView *viewPtr,                  /* Widget record containing the
                                         * attribute information for
                                         * buttons. */
    GC gc,
    Drawable drawable)                  /* Pixmap or window to draw
                                         * into. */
{
    long i;
    Button *butPtr;
    Entry *entryPtr;                    /* Entry to be drawn. */

    entryPtr = viewPtr->visibleEntries[0];
    while (entryPtr != viewPtr->rootPtr) {
        int level;
        
        entryPtr = entryPtr->parentPtr;
        if (entryPtr == NULL) {
            break;
        }
        level = EntryDepth(viewPtr, entryPtr);
        if (entryPtr->lastChildPtr != NULL) {
            int ax, ay, by;
            int x;

            /*
             * World X-coordinates aren't computed for entries that are
             * outside the viewport.  So for each off-screen ancestor node
             * compute it here too.
             */
            entryPtr->worldX = LEVELOFFSET(level) + viewPtr->treeColumn.worldX;

            x = SCREENX(viewPtr, entryPtr->worldX);
            ax = x + ICONWIDTH(level) + ICONWIDTH(level + 1) / 2;
            ax |= 0x1;
            GetVerticalLineCoordinates(entryPtr, &ay, &by);

            if ((ay < Tk_Height(viewPtr->tkwin)) && (by > 0)) {
                XDrawLine(viewPtr->display, drawable, gc, ax, ay, ax, by);
            }
        }
    }
    butPtr = &viewPtr->button;
    for (i = 0; i < viewPtr->numVisibleEntries; i++) {
        int x, y, w, h;
        int buttonY, level;
        int x1, x2, y1, y2;

        entryPtr = viewPtr->visibleEntries[i];
        /* Entry is open, draw vertical line. */
        x = SCREENX(viewPtr, entryPtr->worldX);
        y = SCREENY(viewPtr, entryPtr->worldY);
        level = EntryDepth(viewPtr, entryPtr);
        w = ICONWIDTH(level);
        h = entryPtr->height;
        entryPtr->buttonX = (w - butPtr->width) / 2;
        entryPtr->buttonY = (h - butPtr->height) / 2;
        buttonY = y + entryPtr->buttonY;
        x1 = x + (w / 2);
        y1 = buttonY + (butPtr->height / 2);
        x2 = x1 + (ICONWIDTH(level) + ICONWIDTH(level + 1)) / 2;
        x1 |= 0x1;
        y1 |= 0x1;
        x2 |= 0x1;
        if (Blt_Tree_ParentNode(entryPtr->node) != NULL) {
            /*
             * For every node except root, draw a horizontal line from the
             * vertical bar to the middle of the icon.
             */
            XDrawLine(viewPtr->display, drawable, gc, x1, y1, x2, y1);
        }
        if ((IsOpen(entryPtr)) && (entryPtr->lastChildPtr != NULL)) {
            GetVerticalLineCoordinates(entryPtr, &y1, &y2);
            XDrawLine(viewPtr->display, drawable, gc, x2, y1, x2, y2);
        }
    }   
}

static void
DrawRule(
    TreeView *viewPtr,                  /* Widget record containing the
                                         * attribute information for
                                         * rules. */
    Column *colPtr,
    Drawable drawable)                  /* Pixmap or window to draw
                                         * into. */
{
    int x, y1, y2;

    x = SCREENX(viewPtr, colPtr->worldX) + 
        colPtr->width + viewPtr->ruleMark - viewPtr->ruleAnchor - 1;

    y1 = viewPtr->titleHeight + viewPtr->inset;
    y2 = Tk_Height(viewPtr->tkwin) - viewPtr->inset;
    XDrawLine(viewPtr->display, drawable, colPtr->activeRuleGC, x, y1, x, y2);
    viewPtr->flags = TOGGLE(viewPtr->flags, RULE_ACTIVE_COLUMN);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawButton --
 *
 *      Draws a button for the given entry. The button is drawn centered in
 *      the region immediately to the left of the origin of the entry
 *      (computed in the layout routines). The height and width of the
 *      button were previously calculated from the average row height.
 *
 *         button height = entry height - (2 * some arbitrary padding).
 *         button width = button height.
 *
 *      The button may have a border.  The symbol (either a plus or minus)
 *      is slight smaller than the width or height minus the border.
 *
 *          x,y origin of entry
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
 *      None.
 *
 * Side Effects:
 *      A button is drawn for the entry.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawButton(
    TreeView *viewPtr,                  /* Widget record containing the
                                         * attribute information for
                                         * buttons. */
    Entry *entryPtr,                    /* Entry. */
    Drawable drawable,                  /* Pixmap or window to draw
                                         * into. */
    int x, int y)                       /* Center of the button. */
{
    Blt_Bg bg;
    Button *butPtr = &viewPtr->button;
    Icon icon;
    int relief;
    int bw, bh;

    if ((butPtr->width == 0) || (butPtr->height == 0)) {
        return;
    }
    bg = (entryPtr == viewPtr->activeBtnPtr) 
        ? butPtr->activeBg : butPtr->normalBg;
    relief = (IsClosed(entryPtr)) ? butPtr->closeRelief : butPtr->openRelief;
    if (relief == TK_RELIEF_SOLID) {
        relief = TK_RELIEF_FLAT;
    }
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, butPtr->width,
                butPtr->height, butPtr->borderWidth, relief);
    x += butPtr->borderWidth;
    y += butPtr->borderWidth;
    bw = butPtr->width  - (2 * butPtr->borderWidth);
    bh = butPtr->height - (2 * butPtr->borderWidth);

    icon = NULL;
    if (butPtr->icons != NULL) {        /* Open or close button icon? */
        icon = ((IsOpen(entryPtr)) && (butPtr->icons[1] != NULL)) ?
            butPtr->icons[1] : butPtr->icons[0];
    }
    if (icon != NULL) {                 /* Icon or rectangle? */
        Tk_RedrawImage(IconBits(icon), 0, 0, bw, bh, drawable, x, y);
    } else {
        int y1, y2, x1, x2;
        XSegment segments[6];
        int count;
        GC gc;

        gc = (entryPtr == viewPtr->activeBtnPtr) 
            ? butPtr->activeGC : butPtr->normalGC;
        if (relief == TK_RELIEF_FLAT) {
            /* Draw the box outline */

            x1 = x - butPtr->borderWidth;
            y1 = y - butPtr->borderWidth;
            x2 = x1 + butPtr->width - 1;
            y2 = y1 + butPtr->height - 1;

            segments[0].x1 = x1;
            segments[0].x2 = x2;
            segments[0].y2 = segments[0].y1 = y1;
            segments[1].x2 = segments[1].x1 = x2;
            segments[1].y1 = y1;
            segments[1].y2 = y2;
            segments[2].x2 = segments[2].x1 = x1;
            segments[2].y1 = y1;
            segments[2].y2 = y2;
#ifdef WIN32
            segments[2].y2++;
#endif
            segments[3].x1 = x1;
            segments[3].x2 = x2;
            segments[3].y2 = segments[3].y1 = y2;
#ifdef WIN32
            segments[3].x2++;
#endif
        }
        y1 = y + bh / 2;
        x1 = x + BUTTON_IPAD;
        x2 = x + bw - BUTTON_IPAD;

        segments[4].y1 = segments[4].y2 = y1;
        segments[4].x1 = x1;
        segments[4].x2 = x2 - 1;
#ifdef WIN32
        segments[4].x2++;
#endif

        count = 5;
        if (IsClosed(entryPtr)) {       /* Draw the vertical line for the
                                         * plus. */
            y1 = y + BUTTON_IPAD;
            y2 = y + bh - BUTTON_IPAD;
            segments[5].y1 = y1;
            segments[5].y2 = y2 - 1;
            segments[5].x1 = segments[5].x2 = x + bw / 2;
#ifdef WIN32
            segments[5].y2++;
#endif
            count = 6;
        }
        XDrawSegments(viewPtr->display, drawable, gc, segments, count);
    }
}


static void
DrawEntryIcon(
    TreeView *viewPtr,                  /* Widget record containing the
                                         * attribute information for
                                         * buttons. */
    Entry *entryPtr,                    /* Entry to display. */
    Icon icon,
    Drawable drawable,                  /* Pixmap or window to draw
                                         * into. */
    int x, int y)
{
    int level;
    int maxY;
    int ix, iy, iw, ih;
    
    level = EntryDepth(viewPtr, entryPtr);
    ih = IconHeight(icon);
    iw = IconWidth(icon);
    if (viewPtr->flags & FLAT) {
        x += (ICONWIDTH(0) - iw) / 2;
    } else {
        x += (ICONWIDTH(level + 1) - iw) / 2;
    }       
    if (entryPtr->height > ih) {
        y += (entryPtr->height - ih) / 2;
    }
    maxY = Tk_Height(viewPtr->tkwin) - (viewPtr->inset - INSET_PAD);
    ix = iy = 0;
    if (x < 0) {
        iw += x;
        ix -= x;
    }
    if ((x + iw) > Tk_Width(viewPtr->tkwin)) {
        iw = Tk_Width(viewPtr->tkwin) - x;
    }
    if (y < 0) {
        ih += y;
        iy -= y;
    }
    if ((y + ih) >= maxY) {
        ih = maxY - y;
    }
    Tk_RedrawImage(IconBits(icon), ix, iy, iw, ih, drawable, x, y);
} 


static void
DrawEntryLabel(
    TreeView *viewPtr,                  /* Widget record. */
    Entry *entryPtr,                    /* Entry attribute information. */
    Drawable drawable,                  /* Pixmap or window to draw
                                         * into. */
    int x, int y,
    int maxLength,
    TkRegion rgn)                       
{
    const char *label;
    int isFocused, isSelected, isActive;
    int width, height;                  /* Width and height of label. */

    isFocused = ((entryPtr == viewPtr->focusPtr) && (viewPtr->flags & FOCUS));
    isSelected = EntryIsSelected(viewPtr, entryPtr);
    isActive = (entryPtr == viewPtr->activePtr);

    /* Includes padding, selection 3-D border, and focus outline. */
    width = entryPtr->labelWidth;
    height = entryPtr->labelHeight;

    /* Center the label, if necessary, vertically along the entry row. */
    if (height < entryPtr->height) {
        y += (entryPtr->height - height) / 2;
    }
    /* Focus outline */
    if (isFocused) {                    
        XSegment segments[4];

        if (isSelected) {
            XColor *color;

            color = viewPtr->selectedFg;
            XSetForeground(viewPtr->display, viewPtr->focusGC, color->pixel);
        }
        if (width > maxLength) {
            width = maxLength | 0x1;    /* Width has to be odd for the dots
                                         * in the focus rectangle to
                                         * align. */
        }
        if (rgn != NULL) {
            TkSetRegion(viewPtr->display, viewPtr->focusGC, rgn);
        }       
        /*
         *  +----0----+
         *  3         1
         *  +----2----+
         */
        y += 2, width -= 2; height -= 4;
        segments[0].x1 = x | 0x1;
        segments[0].x2 = (x + width)| 0x1;
        segments[0].y1 = segments[0].y2 = y | 0x1;

        segments[1].x1 = x | 0x1;
        segments[1].x2 = (x + width)| 0x1;
        segments[1].y1 = segments[1].y2 = (y + height) | 0x1;

        segments[2].x1 = segments[2].x2 = x | 0x1;
        segments[2].y1 = y | 0x1;
        segments[2].y2 = (y + height) | 0x1;

        segments[3].x1 = segments[3].x2 = (x + width) | 0x1;
        segments[3].y1 = y | 0x1;
        segments[3].y2 = (y + height) | 0x1;

        XDrawSegments(viewPtr->display, drawable, viewPtr->focusGC, segments,4);
        if (isSelected) {
            XSetForeground(viewPtr->display, viewPtr->focusGC, 
                viewPtr->focusColor->pixel);
        }
        if (rgn != NULL) {
            XSetClipMask(viewPtr->display, viewPtr->focusGC, None);
        }       
    }
    x += FOCUS_PAD + LABEL_PADX;
    y += FOCUS_PAD + LABEL_PADY;

    label = GETLABEL(entryPtr);
    if ((label[0] != '\0') && (maxLength > 0)) {
        Blt_Font font;
        TextLayout *textPtr;
        TextStyle ts;
        XColor *color;
        
        font = entryPtr->font;
        if (font == NULL) {
            font = GetStyleFont(&viewPtr->treeColumn);
        }
        if (isSelected) {
            color = viewPtr->selectedFg;
        } else if (entryPtr->color != NULL) {
            color = entryPtr->color;
        } else {
            color = GetStyleForeground(&viewPtr->treeColumn);
        }
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, font);
        Blt_Ts_SetForeground(ts, color);
        Blt_Ts_SetFontClipRegion(ts, rgn);
        Blt_Ts_SetMaxLength(ts, maxLength);

        if (viewPtr->flags & FLAT) {
            textPtr = Blt_Ts_CreateLayout(PathFromRoot(viewPtr, entryPtr),
                -1, &ts);
        } else {
            textPtr = Blt_Ts_CreateLayout(label, -1, &ts);
        }
        Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, textPtr, &ts, x, y);
        if (isActive) {
            Blt_Ts_UnderlineChars(viewPtr->tkwin, drawable, textPtr, 
                &ts, x, y);
        }
        Blt_Free(textPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawCell --
 *
 *      Draws a column cell for the given entry.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      A button is drawn for the entry.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawCell(
    TreeView *viewPtr,                  /* Widget record. */
    Cell *cellPtr,
    Drawable drawable,                  /* Pixmap or window to draw into. */
    int x, int y)
{
    CellStyle *stylePtr;

    stylePtr = GetCurrentStyle(viewPtr, cellPtr->colPtr, cellPtr);
    (*stylePtr->classPtr->drawProc)(cellPtr, drawable, stylePtr, x, y);
}

static void
DisplayCell(TreeView *viewPtr, Cell *cellPtr)
{
    Blt_Bg bg;
    CellStyle *stylePtr;
    Column *colPtr;
    Entry *entryPtr;
    int x, y, w, h;
    int x1, x2, y1, y2;

    stylePtr = cellPtr->stylePtr;
    if (stylePtr == NULL) {
        stylePtr = cellPtr->colPtr->stylePtr;
    }
    if (stylePtr->cursor != None) {
        if (cellPtr == viewPtr->activeCellPtr) {
            Tk_DefineCursor(viewPtr->tkwin, stylePtr->cursor);
        } else {
            if (viewPtr->cursor != None) {
                Tk_DefineCursor(viewPtr->tkwin, viewPtr->cursor);
            } else {
                Tk_UndefineCursor(viewPtr->tkwin);
            }
        }
    }
    colPtr = cellPtr->colPtr;
    entryPtr = cellPtr->entryPtr;
    x = SCREENX(viewPtr, colPtr->worldX);
    y = SCREENY(viewPtr, entryPtr->worldY);
    h = entryPtr->height;
    w = cellPtr->colPtr->width;

    /* Visible area for cells. */
    y1 = viewPtr->titleHeight + viewPtr->inset;
    y2 = Tk_Height(viewPtr->tkwin) - viewPtr->inset;
    x1 = viewPtr->inset;
    x2 = Tk_Width(viewPtr->tkwin) - viewPtr->inset;

    if (((x + w) < x1) || (x > x2) || ((y + h) < y1) || (y > y2)) {
        return;                         /* Cell is entirely clipped. */
    }

    /* Draw the background of the cell. */
    if ((cellPtr == viewPtr->activeCellPtr) ||
        (!EntryIsSelected(viewPtr, entryPtr))) {
        bg = GetStyleBackground(colPtr);
    } else {
        bg = CHOOSE(viewPtr->selectedBg, stylePtr->selectedBg);
    }
    /*FIXME*/
    /* bg = CHOOSE(viewPtr->selectedBg, stylePtr->selectedBg);  */
    if ((w > 0) && (h > 0)) {
        Drawable drawable;
        int pw, ph, px, py;

        drawable = Blt_GetPixmap(viewPtr->display, Tk_WindowId(viewPtr->tkwin), 
                w, h, Tk_Depth(viewPtr->tkwin)); 
        Blt_Bg_SetOrigin(viewPtr->tkwin, bg, x, y);
        Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, 0, 0, 
                w, h, 0, TK_RELIEF_FLAT);
        Blt_Bg_SetOrigin(viewPtr->tkwin, bg, 0, 0);
        DrawCell(viewPtr, cellPtr, drawable, 0, 0);

        px = py = 0;
        pw = w, ph = h;
        if (x < x1) {
            pw -= (x1 - x);             /* Subtract from the width */
            px += (x1 - x);             /* Start x  */
            x = x1;
        } else if ((x + w) >= x2) {
            pw -= (x + w) - x2;
        }
        if (y < y1) {
            ph -= y1 - y;
            py += y1 - y;
            y = y1;
        } else if ((y + h) >= y2) {
            ph -= (y + h) - y2;
        }
        XCopyArea(viewPtr->display, drawable, Tk_WindowId(viewPtr->tkwin), 
                viewPtr->lineGC, px, py, pw, ph, x, y);
        Tk_FreePixmap(viewPtr->display, drawable);
    }

}


/*
 *---------------------------------------------------------------------------
 *
 * DrawFlatEntry --
 *
 *      Draws a button for the given entry.  Note that buttons should only be
 *      drawn if the entry has sub-entries to be opened or closed.  It's the
 *      responsibility of the calling routine to ensure this.
 *
 *      The button is drawn centered in the region immediately to the left of
 *      the origin of the entry (computed in the layout routines). The height
 *      and width of the button were previously calculated from the average row
 *      height.
 *
 *              button height = entry height - (2 * some arbitrary padding).
 *              button width = button height.
 *
 *      The button has a border.  The symbol (either a plus or minus) is slight
 *      smaller than the width or height minus the border.
 *
 *          x,y origin of entry
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
 *      None.
 *
 * Side Effects:
 *      A button is drawn for the entry.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawFlatEntry(
    TreeView *viewPtr,                  /* Widget record containing the
                                         * attribute information for
                                         * buttons. */
    Entry *entryPtr,                    /* Entry to be drawn. */
    Drawable drawable)                  /* Pixmap or window to draw into. */
{
    int level;
    int x, y, xMax;
    Icon icon;

    entryPtr->flags &= ~ENTRY_REDRAW;
    x = SCREENX(viewPtr, entryPtr->worldX);
    y = SCREENY(viewPtr, entryPtr->worldY);
    icon = GetEntryIcon(viewPtr, entryPtr);
    if (icon != NULL) {
        DrawEntryIcon(viewPtr, entryPtr, icon, drawable, x, y);
    } else {
#ifdef notdef
        x -= (DEF_ICON_WIDTH * 2) / 3;
#endif
    }
    level = 0;
    x += ICONWIDTH(level);
    /* Entry label. */
    xMax = SCREENX(viewPtr, viewPtr->treeColumn.worldX) + 
        viewPtr->treeColumn.width - viewPtr->treeColumn.titleBW - 
        viewPtr->treeColumn.pad.side2;
    DrawEntryLabel(viewPtr, entryPtr, drawable, x, y, xMax - x, NULL);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawEntryInHierarchy --
 *
 *      Draws a button for the given entry.  Note that buttons should only
 *      be drawn if the entry has sub-entries to be opened or closed.  It's
 *      the responsibility of the calling routine to ensure this.
 *
 *      The button is drawn centered in the region immediately to the left
 *      of the origin of the entry (computed in the layout routines). The
 *      height and width of the button were previously calculated from the
 *      average row height.
 *
 *              button height = entry height - (2 * some arbitrary padding).
 *              button width = button height.
 *
 *      The button has a border.  The symbol (either a plus or minus) is
 *      slight smaller than the width or height minus the border.
 *
 *          x,y origin of entry
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
 *      None.
 *
 * Side Effects:
 *      A button is drawn for the entry.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawEntryInHierarchy(TreeView *viewPtr, Entry *entryPtr, Drawable drawable)
{
    Button *butPtr = &viewPtr->button;
    int level;
    int width, height;
    int x, y, xMax;
    Icon icon;

    entryPtr->flags &= ~ENTRY_REDRAW;
    x = SCREENX(viewPtr, entryPtr->worldX);
    y = SCREENY(viewPtr, entryPtr->worldY);

    level = EntryDepth(viewPtr, entryPtr);
    width = ICONWIDTH(level);
    height = entryPtr->height;

    entryPtr->buttonX = (width - butPtr->width) / 2;
    entryPtr->buttonY = (height - butPtr->height) / 2;

    if ((entryPtr->flags & ENTRY_BUTTON) && (entryPtr != viewPtr->rootPtr)) {
        /*
         * Except for the root, draw a button for every entry that needs
         * one.  The displayed button can be either an icon (Tk image) or a
         * line drawing (rectangle with plus or minus sign).
         */
        DrawButton(viewPtr, entryPtr, drawable,
                   (x + entryPtr->buttonX) | 0x1, 
                   (y + entryPtr->buttonY) | 0x1);
    }
    x += ICONWIDTH(level);

    icon = GetEntryIcon(viewPtr, entryPtr);
    if (icon != NULL) {
        DrawEntryIcon(viewPtr, entryPtr, icon, drawable, x, y);
    } else {
#ifdef notdef
        x -= (DEF_ICON_WIDTH * 2) / 3;
#endif
    }
    x += ICONWIDTH(level + 1);

    /* Entry label. */
    xMax = SCREENX(viewPtr, viewPtr->treeColumn.worldX) + 
        viewPtr->treeColumn.width - viewPtr->treeColumn.titleBW - 
        viewPtr->treeColumn.pad.side2;
    DrawEntryLabel(viewPtr, entryPtr, drawable, x, y, xMax - x, NULL);
}

static Blt_Picture
GetSortArrowPicture(TreeView *viewPtr, int w, int h)
{
    if (viewPtr->sort.decreasing) {
        if ((viewPtr->sort.upArrow == NULL) ||
            (Blt_Picture_Width(viewPtr->sort.upArrow) != w) ||
            (Blt_Picture_Height(viewPtr->sort.upArrow) != h)) {
            Blt_Picture picture;
            int ix, iy, iw, ih;
            
            if (viewPtr->sort.upArrow != NULL) {
                Blt_FreePicture(viewPtr->sort.upArrow);
            }
            iw = w * 45 / 100;
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
        if ((viewPtr->sort.downArrow == NULL) ||
            (Blt_Picture_Width(viewPtr->sort.downArrow) != w) ||
            (Blt_Picture_Height(viewPtr->sort.downArrow) != h)) {
            Blt_Picture picture;
            int ix, iy, iw, ih;

            if (viewPtr->sort.downArrow != NULL) {
                Blt_FreePicture(viewPtr->sort.downArrow);
            }
            iw = w * 45 / 100;
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
DrawColumnTitle(TreeView *viewPtr, Column *colPtr, Drawable drawable, 
                int x, int y)
{
    Blt_Bg bg;
    XColor *fg;
    int dw, dx;
    int colWidth, colHeight, need;
    int needArrow;
    int y0;

    if (viewPtr->titleHeight < 1) {
        return;
    }
    y0 = y;
    dx = x;
    colWidth = colPtr->width;
    colHeight = viewPtr->titleHeight;
    dw = colPtr->width;
    if (colPtr->index == (Blt_Chain_GetLength(viewPtr->columns) - 1)) {
        /* If there's any room left over, let the last column take it. */
        dw = Tk_Width(viewPtr->tkwin) - x;
    }
    if ((dw == 0) || (colHeight == 0)) {
        return;
    }
    if (colPtr == viewPtr->colActiveTitlePtr) {
        bg = colPtr->activeTitleBg;
        fg = colPtr->activeTitleFgColor;
    } else {
        bg = colPtr->titleBg;
        fg = colPtr->titleFgColor;
    }
    if (bg == NULL) {
        bg = viewPtr->normalBg;
    }
    if (fg == NULL) {
        fg = viewPtr->normalFg;
    }
    /* Clear the title area by drawing the background. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, dx, y, dw, 
        colHeight, 0, TK_RELIEF_FLAT);
    colWidth  -= 2 * (colPtr->titleBW + TITLE_PADX);
    colHeight -= 2 * colPtr->titleBW;
    x += colPtr->titleBW + TITLE_PADX;
    y += colPtr->titleBW;
    needArrow = (colPtr == viewPtr->sort.markPtr);

    need = colPtr->titleWidth - 2 * TITLE_PADX;
    if (!needArrow) {
        need -= colPtr->arrowWidth + TITLE_PADX;
    }
    if (colWidth > need) {
        switch (colPtr->titleJustify) {
        case TK_JUSTIFY_RIGHT:
            x += colWidth - need;
            break;
        case TK_JUSTIFY_CENTER:
            x += (colWidth - need) / 2;
            break;
        case TK_JUSTIFY_LEFT:
            break;
        }
    }
    if (colPtr->titleIcon != NULL) {
        int ix, iy, iw, ih, gap;

        ih = IconHeight(colPtr->titleIcon);
        iw = IconWidth(colPtr->titleIcon);
        ix = x;
        /* Center the icon vertically.  We already know the column title is
         * at least as tall as the icon. */
        iy = y;
        if (colHeight > ih) {
            iy += (colHeight - ih) / 2;
        }
        Tk_RedrawImage(IconBits(colPtr->titleIcon), 0, 0, iw, ih, 
                drawable, ix, iy);
        gap = (colPtr->textWidth > 0) ? TITLE_PADX : 0;
        x += iw + gap;
        colWidth -= iw + gap;
    }
    if (colPtr->textWidth > 0) {
        TextStyle ts;
        int ty;
        int maxLength;

        ty = y;
        maxLength = colWidth;
        if (colHeight > colPtr->textHeight) {
            ty += (colHeight - colPtr->textHeight) / 2;
        }
        if (needArrow) {
            maxLength -= colPtr->arrowWidth + TITLE_PADX;
        }
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, colPtr->titleFont);
        Blt_Ts_SetForeground(ts, fg);
        Blt_Ts_SetMaxLength(ts, maxLength);
        Blt_Ts_DrawText(viewPtr->tkwin, drawable, colPtr->text, -1, &ts, x, ty);
        x += MIN(colPtr->textWidth, maxLength);
    }
    if (needArrow) {
        int ay, ax, aw, ah;
        SortInfo *sortPtr;

        sortPtr = &viewPtr->sort;
        ax = x + TITLE_PADX;
        ay = y;
        aw = colPtr->arrowWidth;
        ah = colPtr->arrowHeight;
        if (colHeight > colPtr->arrowHeight) {
            ay += (colHeight - colPtr->arrowHeight) / 2;
        }
        if ((sortPtr->decreasing) && (colPtr->sortUp != NULL)) {
            Tk_RedrawImage(IconBits(colPtr->sortUp), 0, 0, aw, ah, 
                drawable, ax, ay);
        } else if (colPtr->sortDown != NULL) {
            Tk_RedrawImage(IconBits(colPtr->sortDown), 0, 0, aw, ah, 
                drawable, ax, ay);
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
    Blt_Bg_DrawRectangle(viewPtr->tkwin, drawable, bg, dx, y0, 
        dw, viewPtr->titleHeight, colPtr->titleBW, colPtr->titleRelief);
}

static void
DisplayColumnTitle(TreeView *viewPtr, Column *colPtr, Drawable drawable)
{
    int x, y, x1, x2;
    int clipped;

    y = viewPtr->inset;
    x1 = x = SCREENX(viewPtr, colPtr->worldX);
    x2 = x1 + colPtr->width;
    if ((x1 >= (Tk_Width(viewPtr->tkwin) - viewPtr->inset)) ||
        (x2 <= viewPtr->inset)) {
        return;                         /* Column starts after the window or
                                         * ends before the the window. */
    }
    clipped = FALSE;
    if (x1 < viewPtr->inset) {
        x1 = viewPtr->inset;
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
                w, viewPtr->titleHeight, Tk_Depth(viewPtr->tkwin));
        DrawColumnTitle(viewPtr, colPtr, pixmap, -dx, 0);
        XCopyArea(viewPtr->display, pixmap, drawable, colPtr->titleGC,
                  0, 0, w, viewPtr->titleHeight, x + dx, viewPtr->inset);
        Tk_FreePixmap(viewPtr->display, pixmap);
    } else {
        DrawColumnTitle(viewPtr, colPtr, drawable, x, y);
    }
}

static void
DrawColumnTitles(TreeView *viewPtr, Drawable drawable)
{
    Blt_ChainLink link;
    Column *colPtr;
    int x;

    if (viewPtr->titleHeight < 1) {
        return;
    }
    for (link = Blt_Chain_FirstLink(viewPtr->columns); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        colPtr = Blt_Chain_GetValue(link);
        if (colPtr->flags & HIDDEN) {
            continue;
        }
        x = SCREENX(viewPtr, colPtr->worldX);
        if ((x + colPtr->width) < 0) {
            continue;                   /* Don't draw columns before the left
                                         * edge. */
        }
        if (x > Tk_Width(viewPtr->tkwin)) {
            break;                      /* Discontinue when a column starts
                                         * beyond the right edge. */
        }
        DisplayColumnTitle(viewPtr, colPtr, drawable);
    }
}

static void
DrawEntryBackgrounds(TreeView *viewPtr, Drawable drawable, int x, int w, 
                     Column *colPtr)
{
    Blt_Bg normalBg;
    long i;
    int h;
    
    normalBg = GetStyleBackground(colPtr);
    h = Tk_Height(viewPtr->tkwin);
    if ((w == 0) || (h == 0)) {
        return;
    }
    /* This also fills the background where there are no entries. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, normalBg, x, 0, w, h,
        0, TK_RELIEF_FLAT);

    for (i = 0; i < viewPtr->numVisibleEntries; i++) {
        Blt_Bg bg;
        int y, rowHeight;
        Entry *rowPtr;

        rowPtr = viewPtr->visibleEntries[i];
        bg = normalBg;
        if (EntryIsSelected(viewPtr, rowPtr)) {
            bg = viewPtr->selectedBg;
        } else if ((viewPtr->altBg != NULL) && (rowPtr->flatIndex & 0x1)) {
            bg = viewPtr->altBg;
        } else {
            bg = normalBg;
        }
        rowHeight = rowPtr->height;
        if ((rowHeight == 0) || (w == 0)) {
            continue;
        }
        y = SCREENY(viewPtr, rowPtr->worldY);
        Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, w, rowHeight, 
                0, TK_RELIEF_FLAT);
        /* Draw Rule */
        if (colPtr->ruleWidth > 0) {
            XFillRectangle(viewPtr->display, drawable, colPtr->ruleGC, 
                           x + w - colPtr->ruleWidth, y, 
                           colPtr->ruleWidth, rowHeight);
        }
        if (rowPtr->ruleHeight > 0) {
            XFillRectangle(viewPtr->display, drawable, rowPtr->ruleGC, 
                           x, y + rowHeight - rowPtr->ruleHeight,
                           w, rowPtr->ruleHeight);
        }
    }
}

static void
DrawTree(TreeView *viewPtr, Drawable drawable, int x)
{
    long i, count;

    count = 0;
    for (i = 0; i < viewPtr->numVisibleEntries; i++) {
        Entry *entryPtr;

        entryPtr = viewPtr->visibleEntries[i];
        entryPtr->flags &= ~SELECTED;
        if (EntryIsSelected(viewPtr, entryPtr)) {
            entryPtr->flags |= SELECTED;
            count++;
        }
    }
    if ((viewPtr->lineWidth > 0) && (viewPtr->numVisibleEntries > 0)) { 
        /* Draw all the vertical lines from topmost node. */
        DrawLines(viewPtr, viewPtr->lineGC, drawable);
        if (count > 0) {
            TkRegion rgn;

            rgn = TkCreateRegion();
            for (i = 0; i < viewPtr->numVisibleEntries; i++) {
                Entry *entryPtr;

                entryPtr = viewPtr->visibleEntries[i];
                if (entryPtr->flags & SELECTED) {
                    XRectangle r;

                    r.x = 0;
                    r.y = SCREENY(viewPtr, entryPtr->worldY);
                    r.width = Tk_Width(viewPtr->tkwin);
                    r.height = entryPtr->height;
                    TkUnionRectWithRegion(&r, rgn, rgn);
                }
            }
            TkSetRegion(viewPtr->display, viewPtr->selectedGC, rgn);
            DrawLines(viewPtr, viewPtr->selectedGC, drawable);
            XSetClipMask(viewPtr->display, viewPtr->selectedGC, None);
            TkDestroyRegion(rgn);
        }
    }
    for (i = 0; i < viewPtr->numVisibleEntries; i++) {
        DrawEntryInHierarchy(viewPtr, viewPtr->visibleEntries[i], drawable);
    }
}


static void
DrawFlatView(TreeView *viewPtr, Drawable drawable, int x)
{
    long i;

    for (i = 0; i < viewPtr->numVisibleEntries; i++) {
        DrawFlatEntry(viewPtr, viewPtr->visibleEntries[i], drawable);
    }
}

static void
DrawOuterBorders(TreeView *viewPtr, Drawable drawable)
{
    /* Draw 3D border just inside of the focus highlight ring. */
    if (viewPtr->borderWidth > 0) {
        int w, h;

        w = Tk_Width(viewPtr->tkwin)  - 2 * viewPtr->highlightWidth;
        h = Tk_Height(viewPtr->tkwin) - 2 * viewPtr->highlightWidth;
        if ((w > 0) && (h > 0)) {
            Blt_Bg_DrawRectangle(viewPtr->tkwin, drawable, viewPtr->normalBg,
                viewPtr->highlightWidth, viewPtr->highlightWidth, w, h,
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
    viewPtr->flags &= ~REDRAW_BORDERS;
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
DisplayProc(ClientData clientData)      /* Information about widget. */
{
    Blt_ChainLink link;
    Pixmap drawable; 
    TreeView *viewPtr = clientData;
    int reqWidth, reqHeight;
    int count;

    viewPtr->flags &= ~REDRAW_PENDING;
    if (viewPtr->tkwin == NULL) {
        return;                         /* Window has been destroyed. */
    }
#ifdef notdef
    fprintf(stderr, "DisplayProc %s\n", Tk_PathName(viewPtr->tkwin));
#endif

    UpdateView(viewPtr);
    reqHeight = (viewPtr->reqHeight > 0) ? viewPtr->reqHeight : 
        viewPtr->worldHeight + viewPtr->titleHeight + 2 * viewPtr->inset + 1;
    reqWidth = (viewPtr->reqWidth > 0) ? viewPtr->reqWidth : 
        viewPtr->worldWidth + 2 * viewPtr->inset;
    if ((reqWidth != Tk_ReqWidth(viewPtr->tkwin)) || 
        (reqHeight != Tk_ReqHeight(viewPtr->tkwin))) {
        Tk_GeometryRequest(viewPtr->tkwin, reqWidth, reqHeight);
    }
    if (!Tk_IsMapped(viewPtr->tkwin)) {
        return;
    }
    drawable = Blt_GetPixmap(viewPtr->display, Tk_WindowId(viewPtr->tkwin), 
        Tk_Width(viewPtr->tkwin), Tk_Height(viewPtr->tkwin), 
        Tk_Depth(viewPtr->tkwin));

    if ((viewPtr->focusPtr == NULL) && (viewPtr->numVisibleEntries > 0)) {
        /* Re-establish the focus entry at the top entry. */
        viewPtr->focusPtr = viewPtr->visibleEntries[0];
    }
    if ((viewPtr->flags & RULE_ACTIVE_COLUMN) && (viewPtr->colResizePtr!=NULL)){
        DrawRule(viewPtr, viewPtr->colResizePtr, drawable);
    }
    count = 0;
    for (link = Blt_Chain_FirstLink(viewPtr->columns); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        Column *colPtr;
        int x;

        colPtr = Blt_Chain_GetValue(link);
        if (colPtr->flags & HIDDEN) {
            continue;
        }
        x = SCREENX(viewPtr, colPtr->worldX);
        if ((x + colPtr->width) < 0) {
            continue;                   /* Don't draw columns before the
                                         * left edge. */
        }
        if (x > Tk_Width(viewPtr->tkwin)) {
            break;                      /* Discontinue when a column starts
                                         * beyond the right edge. */
        }
        /* Clear the column background. */
        DrawEntryBackgrounds(viewPtr, drawable, x, colPtr->width, colPtr);
        if (colPtr != &viewPtr->treeColumn) {
            Entry **epp;
            
            for (epp = viewPtr->visibleEntries; *epp != NULL; epp++) {
                Cell *cellPtr;
                
                /* Check if there's a corresponding cell in the entry. */
                cellPtr = GetCell(*epp, colPtr);
                if (cellPtr != NULL) {
                    DrawCell(viewPtr, cellPtr, drawable, x, 
                        SCREENY(viewPtr,(*epp)->worldY));
                }
            }
        } else {
            if (viewPtr->flags & FLAT) {
                DrawFlatView(viewPtr, drawable, x);
            } else {
                DrawTree(viewPtr, drawable, x);
            }
        }
        count++;
    }
    if (count == 0) {
        Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, viewPtr->normalBg, 0, 0, 
                Tk_Width(viewPtr->tkwin), Tk_Height(viewPtr->tkwin), 
                viewPtr->borderWidth, viewPtr->relief);
    }
    if (viewPtr->flags & SHOW_COLUMN_TITLES) {
        DrawColumnTitles(viewPtr, drawable);
    }
    DrawOuterBorders(viewPtr, drawable);
    if ((viewPtr->flags & COLUMN_RULE_NEEDED) &&
        (viewPtr->colResizePtr != NULL)) {
        DrawRule(viewPtr, viewPtr->colResizePtr, drawable);
    }
    /* Now copy the new view to the window. */
    XCopyArea(viewPtr->display, drawable, Tk_WindowId(viewPtr->tkwin), 
        viewPtr->lineGC, 0, 0, Tk_Width(viewPtr->tkwin), 
        Tk_Height(viewPtr->tkwin), 0, 0);
    Tk_FreePixmap(viewPtr->display, drawable);
}



static int
DisplayLabel(TreeView *viewPtr, Entry *entryPtr, Drawable drawable)
{
    Blt_Bg bg;
    Column *colPtr;
    Icon icon;
    TkRegion rgn;
    XRectangle r;
    int level;
    int x, y, xMax, w, h;
    int y2, y1, x1, x2;

    x = SCREENX(viewPtr, entryPtr->worldX);
    colPtr = &viewPtr->treeColumn;
    y = SCREENY(viewPtr, entryPtr->worldY);
    h = entryPtr->height - 1;
    w = colPtr->width - (entryPtr->worldX - colPtr->worldX);
    xMax = SCREENX(viewPtr, colPtr->worldX) + colPtr->width - 
        colPtr->titleBW - colPtr->pad.side2;

    icon = GetEntryIcon(viewPtr, entryPtr);
    if (viewPtr->flags & FLAT) {
        x += ICONWIDTH(0);
        w -= ICONWIDTH(0);
        if (icon == NULL) {
            x -= (DEF_ICON_WIDTH * 2) / 3;
        }
    } else {
        level = EntryDepth(viewPtr, entryPtr);
        x += ICONWIDTH(level);
        w -= ICONWIDTH(level);
        if (icon != NULL) {
            x += ICONWIDTH(level + 1);
            w -= ICONWIDTH(level + 1);
        } else {
            x += ICONWIDTH(level);
            w -= ICONWIDTH(level);
        }
    }
    if (EntryIsSelected(viewPtr, entryPtr)) {
        bg = viewPtr->selectedBg;
    } else {
        bg = GetStyleBackground(&viewPtr->treeColumn);
        if ((viewPtr->altBg != NULL) && (entryPtr->flatIndex & 0x1))  {
            bg = viewPtr->altBg;
        }
    }
    x1 = viewPtr->inset;
    x2 = Tk_Width(viewPtr->tkwin) - viewPtr->inset;
    y1 = viewPtr->inset + viewPtr->titleHeight;
    y2 = Tk_Height(viewPtr->tkwin) - viewPtr->inset - INSET_PAD;

    /* Verify that the label is currently visible on screen. */
    if (((x + w) <  x1) || (x > x2) || ((y + h) < y1) || (y > y2)) {
        return 0;
    }
    r.x = x1;
    r.y = y1;
    r.width = x2 - x1;
    r.height = y2 - y1; 
    rgn = TkCreateRegion();
    TkUnionRectWithRegion(&r, rgn, rgn);

    /* Clear the entry label background. */
    Blt_Bg_SetClipRegion(viewPtr->tkwin, bg, rgn);
    if (((w - colPtr->ruleWidth) > 0) && ((h - entryPtr->ruleHeight) > 0)) {
        Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, 
                w - colPtr->ruleWidth, h - entryPtr->ruleHeight, 0, 
                TK_RELIEF_FLAT);
    }
    Blt_Bg_UnsetClipRegion(viewPtr->tkwin, bg);
    DrawEntryLabel(viewPtr, entryPtr, drawable, x, y, xMax - x, rgn);
    TkDestroyRegion(rgn);
    return 1;
}


static void
DisplayButton(TreeView *viewPtr, Entry *entryPtr)
{
    Drawable drawable;
    int sx, sy, dx, dy;
    int width, height;
    int left, right, top, bottom;

    dx = SCREENX(viewPtr, entryPtr->worldX) + entryPtr->buttonX;
    dy = SCREENY(viewPtr, entryPtr->worldY) + entryPtr->buttonY;
    width = viewPtr->button.width;
    height = viewPtr->button.height;

    top = viewPtr->titleHeight + viewPtr->inset;
    bottom = Tk_Height(viewPtr->tkwin) - viewPtr->inset;
    left = viewPtr->inset;
    right = Tk_Width(viewPtr->tkwin) - viewPtr->inset;

    if (((dx + width) < left) || (dx > right) ||
        ((dy + height) < top) || (dy > bottom)) {
        return;                         /* Cell is clipped. */
    }
    drawable = Blt_GetPixmap(viewPtr->display, Tk_WindowId(viewPtr->tkwin), 
        width, height, Tk_Depth(viewPtr->tkwin));
    /* Draw the background of the cell. */
    DrawButton(viewPtr, entryPtr, drawable, 0, 0);
    dx |= 1;
    dy |= 1;
    /* Clip the drawable if necessary */
    sx = sy = 0;
    if (dx < left) {
        width -= left - dx;
        sx += left - dx;
        dx = left;
    }
    if ((dx + width) >= right) {
        width -= (dx + width) - right;
    }
    if (dy < top) {
        height -= top - dy;
        sy += top - dy;
        dy = top;
    }
    if ((dy + height) >= bottom) {
        height -= (dy + height) - bottom;
    }
    XCopyArea(viewPtr->display, drawable, Tk_WindowId(viewPtr->tkwin), 
      viewPtr->lineGC, sx, sy, width,  height, dx, dy);
    Tk_FreePixmap(viewPtr->display, drawable);
}

/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 *      Turns on highlighting for a particular cell.  Only one cell can be
 *      active at a time.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 *      pathName activate cellSpec
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Cell *cellPtr, *lastActiveCellPtr;

    lastActiveCellPtr = viewPtr->activeCellPtr;
    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    if (cellPtr != lastActiveCellPtr) {
        if (lastActiveCellPtr != NULL) { /* Deactivate old cell */
            DisplayCell(viewPtr, lastActiveCellPtr);
        }
        if (cellPtr == NULL) {          /* Deactivate all cells. */
            viewPtr->activePtr = NULL;
            viewPtr->colActivePtr = NULL;
            viewPtr->activeCellPtr = NULL;
        } else {                        /* Activate new cell. */
            viewPtr->activePtr = cellPtr->entryPtr;
            viewPtr->colActivePtr = cellPtr->colPtr;
            viewPtr->activeCellPtr = cellPtr;
            DisplayCell(viewPtr, cellPtr);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BindOp --
 *
 *        pathName bind entryName sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BindOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    BindTagKey *keyPtr;
    Entry *entryPtr;
    TreeView *viewPtr = clientData;

    /*
     * Entries are selected by id only.  All other strings are interpreted as
     * a binding tag.
     */
    if (GetEntry(NULL, viewPtr, objv[2], &entryPtr) == TCL_OK) {
        if (entryPtr != NULL) {
            return TCL_OK;      /* Special id doesn't currently exist. */
        }
        keyPtr = MakeBindTag(viewPtr, entryPtr, ITEM_ENTRY);
    } else {
        keyPtr = MakeStringBindTag(viewPtr, Tcl_GetString(objv[2]), ITEM_ENTRY);
    } 
    return Blt_ConfigureBindingsFromObj(interp, viewPtr->bindTable, keyPtr, 
         objc - 3, objv + 3);
}


/*
 *---------------------------------------------------------------------------
 *
 * BboxOp --
 *
 *      pathName bbox entryName ?switches...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BboxOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Entry *entryPtr;
    Tcl_Obj *listObjPtr;
    TreeView *viewPtr = clientData;
    const char *string;
    int i;
    int screen;
    int x1, y1, x2, y2;

    UpdateView(viewPtr);
    x1 = viewPtr->worldWidth;
    y1 = viewPtr->worldHeight;
    x2 = y2 = 0;

    screen = FALSE;
    string = Tcl_GetString(objv[2]);
    if ((string[0] == '-') && (strcmp(string, "-screen") == 0)) {
        screen = TRUE;
        objc--, objv++;
    }
    for (i = 2; i < objc; i++) {
        int d, x, y;
        const char *string;
        int yBot;

        string = Tcl_GetString(objv[i]);
        if ((string[0] == 'a') && (strcmp(string, "all") == 0)) {
            x1 = y1 = 0;
            x2 = viewPtr->worldWidth;
            y2 = viewPtr->worldHeight;
            break;
        }
        if (GetEntryFromObj(interp, viewPtr, objv[i], &entryPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (entryPtr == NULL) {
            continue;
        }
        if (entryPtr->flags & HIDDEN) {
            continue;
        }
        x = SCREENX(viewPtr, entryPtr->worldX);
        y = SCREENY(viewPtr, entryPtr->worldY);
        yBot = y + entryPtr->height;
        if (y2 < yBot) {
            y2 = yBot;
        }
        if (y1 > y) {
            y1 = y;
        }
        d = EntryDepth(viewPtr, entryPtr);
        x += ICONWIDTH(d) + ICONWIDTH(d + 1);
        if (x2 < (x + entryPtr->width)) {
            x2 = x + entryPtr->width;
        }
        if (x1 > x) {
            x1 = x;
        }
    }
    if (screen) {
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
 * ButtonActivateOp --
 *
 *      Selects the button to appear active.
 *
 *      pathName button activate entryName 
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ButtonActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    Entry *oldPtr, *newPtr;
    TreeView *viewPtr = clientData;
    char *string;

    string = Tcl_GetString(objv[3]);
    if (string[0] == '\0') {
        newPtr = NULL;
    } else if (GetEntryFromObj(interp, viewPtr, objv[3], &newPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (viewPtr->treeColumn.flags & HIDDEN) {
        return TCL_OK;
    }
    if ((newPtr != NULL) && !(newPtr->flags & ENTRY_BUTTON)) {
        newPtr = NULL;
    }
    oldPtr = viewPtr->activeBtnPtr;
    viewPtr->activeBtnPtr = newPtr;
    if (!(viewPtr->flags & REDRAW_PENDING) && (newPtr != oldPtr)) {
        if ((oldPtr != NULL) && (oldPtr != viewPtr->rootPtr)) {
            DisplayButton(viewPtr, oldPtr);
        }
        if ((newPtr != NULL) && (newPtr != viewPtr->rootPtr)) {
            DisplayButton(viewPtr, newPtr);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonBindOp --
 *
 *        pathName button bind tag sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ButtonBindOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    BindTagKey *keyPtr;
    TreeView *viewPtr = clientData;
    Entry *entryPtr;
    Blt_TreeNode node;
    
    if ((Blt_Tree_GetNodeFromObj(NULL, viewPtr->tree, objv[3], &node) == TCL_OK)
        && (GetEntryFromObj(NULL, viewPtr, objv[3], &entryPtr) == TCL_OK) &&
        (entryPtr != NULL)) {
        keyPtr = MakeBindTag(viewPtr, entryPtr, ITEM_BUTTON);
    } else {
        keyPtr = MakeStringBindTag(viewPtr, Tcl_GetString(objv[3]),ITEM_BUTTON);
    } 
    /* Assume that this is a binding tag. */
    return Blt_ConfigureBindingsFromObj(interp, viewPtr->bindTable, keyPtr, 
        objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonCgetOp --
 *
 *      pathName button cget value
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ButtonCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, buttonSpecs, 
        (char *)viewPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonConfigureOp --
 *
 *      This procedure is called to process a list of configuration options
 *      database, in order to reconfigure the one of more entries in the
 *      widget.
 *
 *        .h button configure option value
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font, etc. get
 *      set for viewPtr; old resources get freed, if there were any.  The
 *      hypertext is redisplayed.
 *
 *      pathName button configure option value
 *---------------------------------------------------------------------------
 */
static int
ButtonConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;

    if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, buttonSpecs, 
                (char *)viewPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, buttonSpecs, 
                (char *)viewPtr, objv[3], 0);
    }
    iconsOption.clientData = viewPtr;
    if (Blt_ConfigureWidgetFromObj(viewPtr->interp, viewPtr->tkwin, buttonSpecs,
         objc - 3, objv + 3, (char *)viewPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    ConfigureButtons(viewPtr);
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonContainsOp --
 *
 *      Returns the index of the entry whose button is under the give
 *      x-y coordinates.  If no entry or button is at the point, then
 *      -1 is returned.
 *
 *      pathName button contains x y
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ButtonContainsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    Column *colPtr;
    Entry *entryPtr;
    TreeView *viewPtr = clientData;
    int x, y;
    long inode;
    
    if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)) {
        return TCL_ERROR;
    }
    /* Can't trust the selected entry if nodes have been added or
     * deleted. So recompute the layout. */
    UpdateView(viewPtr);
    inode = -1;
    colPtr = NearestColumn(viewPtr, x, y, NULL);
    if ((colPtr == NULL) || (colPtr != &viewPtr->treeColumn)) {
        goto notfound;
    }
    entryPtr = NearestEntry(viewPtr, x, y, FALSE);
    if (entryPtr == NULL) {
        goto notfound;
    }
    x = WORLDX(viewPtr, x);
    y = WORLDY(viewPtr, y);
    if (entryPtr->flags & ENTRY_BUTTON) {
        Button *butPtr = &viewPtr->button;
        int x1, x2, y1, y2;
        
        x1 = entryPtr->worldX + entryPtr->buttonX - BUTTON_PAD;
        x2 = x1 + butPtr->width + 2 * BUTTON_PAD;
        y1 = entryPtr->worldY + entryPtr->buttonY - BUTTON_PAD;
        y2 = y1 + butPtr->height + 2 * BUTTON_PAD;
        if ((x >= x1) && (x < x2) && (y >= y1) && (y < y2)) {
            inode = Blt_Tree_NodeId(entryPtr->node);
        }
    }
 notfound:
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ButtonOp --
 *
 *      This procedure handles button operations.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec buttonOps[] =
{
    {"activate",  1, ButtonActivateOp,  4, 4, "entryName"},
    {"bind",      1, ButtonBindOp,      4, 6, "tagName ?sequence command?",},
    {"cget",      2, ButtonCgetOp,      4, 4, "option"},
    {"configure", 4, ButtonConfigureOp, 3, 0, "?option value ...?"},
    {"contains",  4, ButtonContainsOp,  5, 5, "x y",},
    {"highlight", 1, ButtonActivateOp,  4, 4, "entryName"},
};

static int numButtonOps = sizeof(buttonOps) / sizeof(Blt_OpSpec);

static int
ButtonOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numButtonOps, buttonOps, BLT_OP_ARG2, objc, 
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
 * CellActivateOp --
 *
 *      Turns on highlighting for a particular cell.  Only one cell
 *      can be active at a time.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 *      .view cell activate ?cell?
 *
 *      pathName cell activate ?cellSpec?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Cell *cellPtr, *lastActiveCellPtr;

    if (objc == 3) {
        if (viewPtr->activeCellPtr != NULL) {
            Tcl_Obj *objPtr;

            objPtr = CellToIndexObj(interp, viewPtr->activeCellPtr);
            Tcl_SetObjResult(interp, objPtr);
        }
        return TCL_OK;
    } 
    lastActiveCellPtr = viewPtr->activeCellPtr;
    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    viewPtr->activeCellPtr = NULL;
    if (cellPtr != lastActiveCellPtr) {
        viewPtr->activeCellPtr = cellPtr;
        if (lastActiveCellPtr != NULL) { /* Deactivate old cell */
            DisplayCell(viewPtr, lastActiveCellPtr);
        }
        if (cellPtr != NULL) {
            DisplayCell(viewPtr, cellPtr);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellBindOp --
 *
 *        pathName cell bind cellName sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellBindOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    BindTagKey *keyPtr;
    Cell *cellPtr;
    TreeView *viewPtr = clientData;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) == TCL_OK) {
        if (cellPtr == NULL) {
            fprintf(stderr, "can't find %s\n", Tcl_GetString(objv[3]));
            return TCL_OK;
        }
        keyPtr = MakeBindTag(viewPtr, cellPtr, ITEM_CELL);
    } else {
        keyPtr = MakeStringBindTag(viewPtr, Tcl_GetString(objv[3]), ITEM_CELL);
    }
    return Blt_ConfigureBindingsFromObj(interp, viewPtr->bindTable, keyPtr, 
         objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * CellBboxOp --
 *
 *      pathName cell bbox cellName ?switches?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellBboxOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    BBoxSwitches switches;
    Cell *cellPtr;
    Tcl_Obj *listObjPtr;
    TreeView *viewPtr = clientData;
    int x1, y1, x2, y2;

    UpdateView(viewPtr);
    x1 = viewPtr->worldWidth;
    y1 = viewPtr->worldHeight;
    x2 = y2 = 0;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    /* Process switches  */
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, bboxSwitches, objc - 4, objv + 4, &switches,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    x1 = cellPtr->colPtr->worldX;
    x2 = cellPtr->colPtr->worldX + cellPtr->colPtr->width;
    y1 = cellPtr->entryPtr->worldY;
    y2 = cellPtr->entryPtr->worldY + cellPtr->entryPtr->height;

    if (cellPtr->colPtr == &viewPtr->treeColumn) {
        int d;
        
        d = EntryDepth(viewPtr, cellPtr->entryPtr);
        x1 += ICONWIDTH(d) + ICONWIDTH(d + 1);
        x2 -= ICONWIDTH(d) + ICONWIDTH(d + 1);
    }
    {
        int w, h;

        w = VPORTWIDTH(viewPtr);
        h = VPORTHEIGHT(viewPtr);

        /*
         * Test for the intersection of the viewport and the computed
         * bounding box.  If there is no intersection, return the empty
         * string.
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
    if (switches.flags & BBOX_ROOT) {
        int rootX, rootY;

        Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
        if (rootX < 0) {
            rootX = 0;
        }
        if (rootY < 0) {
            rootY = 0;
        }
        x1 += rootX;
        y1 += rootY;
        x2 += rootX;
        y2 += rootY;
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
 *      pathName cell cget cellSpec option 
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
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
 *      pathName cell configure cellSpec ? option value ...?
 *---------------------------------------------------------------------------
 */
static int
CellConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Cell *cellPtr;

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
    iconsOption.clientData = viewPtr;
    treeOption.clientData = viewPtr;
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, cellSpecs, 
        objc - 4, objv + 4, (char *)cellPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
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
 *      pathName cell deactivate 
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Cell *lastActiveCellPtr;

    lastActiveCellPtr = viewPtr->activeCellPtr;
    viewPtr->activeCellPtr = NULL;
    if (lastActiveCellPtr != NULL) {
        /* Redisplay the old cell.  */
        DisplayCell(viewPtr, lastActiveCellPtr);
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
 *      pathName cell focus ?cellName? 
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellFocusOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Cell *cellPtr;

    if (objc == 3) {
        if (viewPtr->focusCellPtr != NULL) {
            Tcl_Obj *objPtr;

            objPtr = CellToIndexObj(interp, viewPtr->focusCellPtr);
            Tcl_SetObjResult(interp, objPtr);
        }
        return TCL_OK;
    }
    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    viewPtr->focusCellPtr = cellPtr;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellIdentifyOp --
 *
 *      pathName cell identify cellSpec rootX rootY
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellIdentifyOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellStyle *stylePtr;
    Column *colPtr;
    Entry *rowPtr;
    TreeView *viewPtr = clientData;
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
    colPtr = cellPtr->colPtr;
    rowPtr = cellPtr->entryPtr;
    /* Convert from root coordinates to window-local coordinates to
     * cell-local coordinates */
    Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
    x -= rootX + SCREENX(viewPtr, colPtr->worldX);
    y -= rootY + SCREENY(viewPtr, rowPtr->worldY);
    string = NULL;
    stylePtr = GetCurrentStyle(viewPtr, colPtr, cellPtr);
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
 *      .view cell index $cell
 *
 *      pathName cell index cellSpec
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellIndexOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Tcl_Obj *objPtr;
    Cell *cellPtr;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    objPtr = CellToIndexObj(interp, cellPtr);
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellInvokeOp --
 *
 *      pathName cell invoke cellSpec
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellInvokeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellStyle *stylePtr;
    TreeView *viewPtr = clientData;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    stylePtr = GetCurrentStyle(viewPtr, cellPtr->colPtr, cellPtr);
    if (stylePtr->cmdObjPtr != NULL) {
        int result;
        Tcl_Obj *cmdObjPtr, *objPtr;
        
        /* Invoke command command cell. */
        cmdObjPtr = Tcl_DuplicateObj(stylePtr->cmdObjPtr);
        /* Add index of the cell */
        objPtr = CellToIndexObj(interp, cellPtr);
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
 *      pathName cell see cellSpec
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellSeeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    Column *colPtr;
    Entry *entryPtr;
    TreeView *viewPtr = clientData;
    int viewHeight, viewWidth;
    int x, y;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    UpdateView(viewPtr);

    colPtr = cellPtr->colPtr;
    entryPtr = cellPtr->entryPtr;
    viewWidth = VPORTWIDTH(viewPtr);
    viewHeight = VPORTHEIGHT(viewPtr);

    y = viewPtr->yOffset;
    x = viewPtr->xOffset;
    if (entryPtr->worldY < y) {
        y = entryPtr->worldY;
    } else if ((entryPtr->worldY + entryPtr->height) > (y + viewHeight)) {
        y = entryPtr->worldY + entryPtr->height - viewHeight;
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
        viewPtr->flags |= SCROLLX | VISIBILITY;
    }
    if (y != viewPtr->yOffset) {
        viewPtr->yOffset = y;
        viewPtr->flags |= SCROLLY | VISIBILITY;
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CellStyleOp --
 *
 *      pathName cell stype cellSpec
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellStyleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    CellStyle *stylePtr;
    TreeView *viewPtr = clientData;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    stylePtr = GetCurrentStyle(viewPtr, cellPtr->colPtr, cellPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), stylePtr->name, -1);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * CellWritableOp --
 *
 *      pathName cell writable cellSpec
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CellWritableOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    TreeView *viewPtr = clientData;
    int state;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    state = FALSE;
    if (cellPtr != NULL) {
        CellStyle *stylePtr;

        stylePtr = GetCurrentStyle(viewPtr, cellPtr->colPtr, cellPtr);
        state = (stylePtr->flags & EDITABLE);
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
    {"bbox",       2, CellBboxOp,        4, 0, "cellName ?switches ...?",},
    {"bind",       2, CellBindOp,        4, 6, "tagName ?sequence command?",}, 
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
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, viewSpecs,
        (char *)viewPtr, objv[2], 0);
}


/*ARGSUSED*/
static int
ChrootOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    long inode;

    if (objc == 3) {
        Entry *entryPtr;

        if (GetEntryFromObj(interp, viewPtr, objv[2], &entryPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        viewPtr->flags |= LAYOUT_PENDING;
        viewPtr->rootPtr = entryPtr;
        EventuallyRedraw(viewPtr);
    }
    inode = Blt_Tree_NodeId(viewPtr->rootPtr->node);
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*ARGSUSED*/
static int
CloseOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    CloseSwitches switches;
    Entry *entryPtr;
    EntryIterator iter;

    if (GetEntryIterator(interp, viewPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    /* Process switches  */
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, closeSwitches, objc - 3, objv + 3, &switches,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
         entryPtr = NextTaggedEntry(&iter)) {
        int result;

        /* 
         * Clear the selections for any entries that may have become hidden
         * by closing the node.
         */
        PruneSelection(viewPtr, entryPtr);
        
        /*
         *  Check if either the "focus" entry or selection anchor is in
         *  this hierarchy.  Must move it or disable it before we close the
         *  node.  Otherwise it may be deleted by a TCL "close" script, and
         *  we'll be left pointing to a bogus memory location.
         */
        if ((viewPtr->focusPtr != NULL) && 
            (Blt_Tree_IsAncestor(entryPtr->node, viewPtr->focusPtr->node))){
            viewPtr->focusPtr = entryPtr;
            Blt_SetFocusItem(viewPtr->bindTable, viewPtr->focusPtr, ITEM_ENTRY);
        }
        if ((viewPtr->sel.anchorPtr != NULL) && 
            (Blt_Tree_IsAncestor(entryPtr->node, viewPtr->sel.anchorPtr->node))) {
            viewPtr->sel.markPtr = viewPtr->sel.anchorPtr=NULL;
        }
        if ((viewPtr->activePtr != NULL) && 
            (Blt_Tree_IsAncestor(entryPtr->node,viewPtr->activePtr->node))){
            viewPtr->activePtr = entryPtr;
        }
        if (switches.flags & CLOSE_RECURSE) {
            result = Apply(viewPtr, entryPtr, CloseEntry, 0);
        } else {
            result = CloseEntry(viewPtr, entryPtr);
        }
        if (result != TCL_OK) {
            return TCL_ERROR;
        }   
    }
    /* Closing a node may affect the visible entries and the the world
     * layout of the entries. */
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnActivateOp --
 *
 *      Selects the button to appear active.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Column *colPtr, *activePtr;
    
    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (colPtr == NULL) {
        return TCL_OK;
    }
    if ((colPtr->flags & HIDDEN) || (colPtr->state == STATE_DISABLED)) {
        return TCL_OK;
    }
    activePtr = viewPtr->colActiveTitlePtr;
    viewPtr->colActiveTitlePtr = viewPtr->colActivePtr = colPtr;

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
 *        pathName bind tag type sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnBindOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    BindTagKey *keyPtr;
    Column *colPtr;
    ItemType type;
    TreeView *viewPtr = clientData;
    char c;
    const char *string;
    int length;
    
    string = Tcl_GetStringFromObj(objv[4], &length);
    c = string[0];
    if ((c == 'c') && (strncmp(string, "cell", length) == 0)) {
        type = ITEM_CELL;
    } else if ((c == 't') && (strncmp(string, "title", length) == 0)) {
        type = ITEM_COLUMN_TITLE;
    } else if ((c == 'r') && (strncmp(string, "resize", length) == 0)) {
        type = ITEM_COLUMN_RESIZE;
    } else {
        Tcl_AppendResult(interp, "Bad column bind tag type \"", string, "\"",
                         (char *)NULL);
        return TCL_ERROR;
    }
    if ((GetColumn(NULL, viewPtr, objv[3], &colPtr) == TCL_OK) && 
        (colPtr != NULL)) {
        keyPtr = MakeBindTag(viewPtr, colPtr, type);
    } else {
        keyPtr = MakeStringBindTag(viewPtr, Tcl_GetString(objv[3]), type);
    }
    return Blt_ConfigureBindingsFromObj(interp, viewPtr->bindTable, keyPtr,
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
ColumnCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Column *colPtr;

    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK){
        return TCL_ERROR;
    }
    if (colPtr == NULL) {
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
 *      pathName column configure col ?option value?
 *---------------------------------------------------------------------------
 */
static int
ColumnConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Column *colPtr;

    cachedObjOption.clientData = viewPtr;
    iconOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;

    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (colPtr == NULL) {
        return TCL_OK;
    }
    if (objc == 4) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, columnSpecs, 
                (char *)colPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 5) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, columnSpecs, 
                (char *)colPtr, objv[4], 0);
    }
    if (Blt_ConfigureWidgetFromObj(viewPtr->interp, viewPtr->tkwin, columnSpecs,
        objc - 4, objv + 4, (char *)colPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    ConfigureColumn(viewPtr, colPtr);

    if (Blt_ConfigModified(columnSpecs, "-*borderwidth", "-formatcommand",
                           "-hide", "-icon", "-pad", "-rulewidth",
                           "-show", "-text", "-style", "-title", "-titlefont",
                           "-titleborderwidth", "-width", "-min", "-max",
                           (char *)NULL)) {
        viewPtr->flags |= LAYOUT_PENDING;
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnDeactivateOp --
 *
 *      Deactivates all columns.  All column titles will be redraw in their
 *      normal foreground/background colors.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Column *activePtr;

    activePtr = viewPtr->colActiveTitlePtr;
    viewPtr->colActiveTitlePtr = viewPtr->colActivePtr = NULL;
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
 *      pathName column delete ?columnName ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    int i;

    for (i = 3; i < objc; i++) {
        Column *colPtr;
        Entry *entryPtr;

        if (GetColumn(interp, viewPtr, objv[i], &colPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (colPtr == NULL) {
            continue;
        }
        if (colPtr == &viewPtr->treeColumn) {
            continue;                   /* Can't delete the treeView
                                         * column, so just ignore the
                                         * request. */
        }
        /* Traverse the tree deleting cells associated with the column.  */
        for (entryPtr = viewPtr->rootPtr; entryPtr != NULL;
            entryPtr = NextEntry(entryPtr, 0)) {
            if (entryPtr != NULL) {
                Cell *cellPtr, *lastPtr, *nextPtr;
                
                lastPtr = NULL;
                for (cellPtr = entryPtr->cells; cellPtr != NULL; 
                     cellPtr = nextPtr) {
                    nextPtr = cellPtr->nextPtr;
                    if (cellPtr->colPtr == colPtr) {
                        DestroyCell(viewPtr, cellPtr);
                        if (lastPtr == NULL) {
                            entryPtr->cells = nextPtr;
                        } else {
                            lastPtr->nextPtr = nextPtr;
                        }
                        break;
                    }
                    lastPtr = cellPtr;
                }
            }
        }
        Blt_SetCurrentItem(viewPtr->bindTable, NULL, NULL);
        DestroyColumn(colPtr);
    }
    /* Deleting a column may affect the height of an entry. */
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnExistsOp --
 *
 *      pathName column exists $field
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    int exists;
    Column *colPtr;

    exists = FALSE;
    if ((GetColumn(NULL, viewPtr, objv[3], &colPtr) == TCL_OK) &&
        (colPtr != NULL)) {
        exists = TRUE;
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), exists);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ColumnIndexOp --
 *
 *      Returns the index of the column.
 *
 *      pathName column index column
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnIndexOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    long index;
    Column *colPtr;

    index = -1;
    if ((GetColumn(NULL, viewPtr, objv[3], &colPtr) == TCL_OK) &&
        (colPtr != NULL)) {
        index = colPtr->index;
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnInsertOp --
 *
 *      Add new columns to the tree.
 *
 *      pathName column insert insertPos fieldName ?option values ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnInsertOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Blt_ChainLink before;
    long insertPos;
    Column *colPtr;
    Entry *entryPtr;

    if (Blt_GetPositionFromObj(viewPtr->interp, objv[3], &insertPos) != TCL_OK){
        return TCL_ERROR;
    }
    if ((insertPos == -1) || 
        (insertPos >= Blt_Chain_GetLength(viewPtr->columns))) {
        before = NULL;          /* Insert at end of list. */
    } else {
        before =  Blt_Chain_GetNthLink(viewPtr->columns, insertPos);
    }
    if (GetColumn(NULL, viewPtr, objv[4], &colPtr) == TCL_OK) {
        Tcl_AppendResult(interp, "column \"", Tcl_GetString(objv[4]), 
                         "\" already exists", (char *)NULL);
        return TCL_ERROR;
    }
    colPtr = CreateColumn(viewPtr, objv[4], objc - 5, objv + 5);
    if (colPtr == NULL) {
        return TCL_ERROR;
    }
    if (before == NULL) {
        colPtr->link = Blt_Chain_Append(viewPtr->columns, colPtr);
    } else {
        colPtr->link = Blt_Chain_NewLink();
        Blt_Chain_SetValue(colPtr->link, colPtr);
        Blt_Chain_LinkBefore(viewPtr->columns, colPtr->link, before);
    }
    /* 
     * Traverse the tree adding column entries where needed.
     */
    for(entryPtr = viewPtr->rootPtr; entryPtr != NULL;
        entryPtr = NextEntry(entryPtr, 0)) {
        Cell *cellPtr;

        cellPtr = GetCell(entryPtr, colPtr);
        if (cellPtr == NULL) {
            AddCell(entryPtr, colPtr);
        }
    }
    TraceColumn(viewPtr, colPtr);
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnCurrentOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnCurrentOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Column *colPtr;

    colPtr = GetCurrentColumn(viewPtr);
    if (colPtr != NULL) {
        Tcl_SetStringObj(Tcl_GetObjResult(interp), colPtr->key, -1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnInvokeOp --
 *
 *      This procedure is called to invoke a column command.
 *
 *        .h column invoke columnName
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
    TreeView *viewPtr = clientData;
    Column *colPtr;
    char *string;
    Tcl_Obj *cmdObjPtr;

    string = Tcl_GetString(objv[3]);
    if (string[0] == '\0') {
        return TCL_OK;
    }
    if (GetColumn(interp, viewPtr, objv[3], &colPtr) != TCL_OK){
        return TCL_ERROR;
    }
    if (colPtr == NULL) {
        return TCL_OK;
    }
    cmdObjPtr = colPtr->cmdObjPtr;
    if (cmdObjPtr == NULL) {
        cmdObjPtr = viewPtr->colCmdObjPtr;
    }
    if ((colPtr->state == STATE_NORMAL) && (cmdObjPtr != NULL)) {
        int result;
        Tcl_Obj *objPtr;

        cmdObjPtr = Tcl_DuplicateObj(cmdObjPtr);
        objPtr = Tcl_NewStringObj(Tk_PathName(viewPtr->tkwin), -1);
        Tcl_ListObjAppendElement(viewPtr->interp, cmdObjPtr, objPtr);
        objPtr = Tcl_NewStringObj(colPtr->key, -1);
        Tcl_ListObjAppendElement(viewPtr->interp, cmdObjPtr, objPtr);
        Tcl_Preserve(viewPtr);
        Tcl_Preserve(colPtr);
        Tcl_IncrRefCount(cmdObjPtr);
        result = Tcl_EvalObjEx(viewPtr->interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(cmdObjPtr);
        Tcl_Release(colPtr);
        Tcl_Release(viewPtr);
        return result;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnMoveOp --
 *
 *      Move a column.
 *
 * .h column move field1 position
 *---------------------------------------------------------------------------
 */

/*
 *---------------------------------------------------------------------------
 *
 * ColumnNamesOp --
 *
 *      pathName column names ?pattern ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Blt_ChainLink link;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for(link = Blt_Chain_FirstLink(viewPtr->columns); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Column *colPtr;
        int found;
        int i;

        colPtr = Blt_Chain_GetValue(link);
        found = FALSE;
        for (i = 3; i < objc; i++) {
            const char *pattern;

            pattern = Tcl_GetString(objv[i]);
            found = Tcl_StringMatch(colPtr->key, pattern);
            if (found) {
                break;
            }
        }
        if ((objc == 2) || (found)) {
            Tcl_Obj *objPtr;

            objPtr = Tcl_NewStringObj(colPtr->key, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnNamesOp --
 *
 *      pathName column nearest x y ?switches ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnNearestOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    int x, y;                   /* Screen coordinates of the test point. */
    Column *colPtr;
    ItemType type;
    NearestSwitches switches;
    
    if (Tk_GetPixelsFromObj(interp, viewPtr->tkwin, objv[3], &x) != TCL_OK) {
        return TCL_ERROR;
    } 
    if (Tk_GetPixelsFromObj(interp, viewPtr->tkwin, objv[4], &y) != TCL_OK) {
        return TCL_ERROR;
    } 
    switches.flags = 0;
    if (Blt_ParseSwitches(interp, nearestColumnSwitches, objc - 5, objv + 5, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (switches.flags & NEAREST_ROOT) {
        int rootX, rootY;

        Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
        x -= rootX;
        y -= rootY;
    }
    if ((switches.flags & NEAREST_TITLE) == 0) {
        x = 0;
    }
    colPtr = NearestColumn(viewPtr, x, y, &type);
    if ((switches.flags & NEAREST_TITLE) && (type == ITEM_NONE)) {
        colPtr = NULL;
    }
    if (colPtr != NULL) {
        Tcl_SetStringObj(Tcl_GetObjResult(interp), colPtr->key, -1);
    }
    return TCL_OK;
}

static void
UpdateMark(TreeView *viewPtr, int newMark)
{
    Drawable drawable;
    Column *cp;
    int dx;
    int width;

    cp = viewPtr->colResizePtr;
    if (cp == NULL) {
        return;
    }
    drawable = Tk_WindowId(viewPtr->tkwin);
    if (drawable == None) {
        return;
    }

    /* Erase any existing rule. */
    if (viewPtr->flags & RULE_ACTIVE_COLUMN) { 
        DrawRule(viewPtr, cp, drawable);
    }
    
    dx = newMark - viewPtr->ruleAnchor; 
    width = cp->width - (PADDING(cp->pad) + 2 * cp->borderWidth);
    if ((cp->reqMin > 0) && ((width + dx) < cp->reqMin)) {
        dx = cp->reqMin - width;
    }
    if ((cp->reqMax > 0) && ((width + dx) > cp->reqMax)) {
        dx = cp->reqMax - width;
    }
    if ((width + dx) < 4) {
        dx = 4 - width;
    }
    viewPtr->ruleMark = viewPtr->ruleAnchor + dx;

    /* Redraw the rule if required. */
    if (viewPtr->flags & COLUMN_RULE_NEEDED) {
        DrawRule(viewPtr, cp, drawable);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnResizeActivateOp --
 *
 *      Turns on/off the resize cursor.
 *
 *      $t column resize activate $col
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Column *colPtr;

    if (GetColumn(interp, viewPtr, objv[4], &colPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (viewPtr->resizeCursor != None) {
        Tk_DefineCursor(viewPtr->tkwin, viewPtr->resizeCursor);
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
 *      $t column resize anchor $x
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeAnchorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    int x;

    if (Tcl_GetIntFromObj(NULL, objv[4], &x) != TCL_OK) {
        return TCL_ERROR;
    } 
    viewPtr->ruleAnchor = x;
    viewPtr->flags |= COLUMN_RULE_NEEDED;
    UpdateMark(viewPtr, x);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnResizeDectivateOp --
 *
 *      Turns off the resize cursor.
 *
 *      $t column resize deactivate
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                         Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    if (viewPtr->cursor != None) {
        Tk_DefineCursor(viewPtr->tkwin, viewPtr->cursor);
    } else {
        Tk_UndefineCursor(viewPtr->tkwin);
    }
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
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeMarkOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    int x;

    if (Tcl_GetIntFromObj(NULL, objv[4], &x) != TCL_OK) {
        return TCL_ERROR;
    } 
    viewPtr->flags |= COLUMN_RULE_NEEDED;
    UpdateMark(viewPtr, x);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnResizeSetOp --
 *
 *      Returns the new width of the column including the resize delta.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnResizeSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    viewPtr->flags &= ~COLUMN_RULE_NEEDED;
    UpdateMark(viewPtr, viewPtr->ruleMark);
    if (viewPtr->colResizePtr != NULL) {
        int width, delta;
        Column *colPtr;

        colPtr = viewPtr->colResizePtr;
        delta = (viewPtr->ruleMark - viewPtr->ruleAnchor);
        width = viewPtr->colResizePtr->width + delta - 
            (PADDING(colPtr->pad) + 2 * colPtr->borderWidth) - 1;
        Tcl_SetIntObj(Tcl_GetObjResult(interp), width);
    }
    return TCL_OK;
}

static Blt_OpSpec columnResizeOps[] =
{ 
    {"activate",   2, ColumnResizeActivateOp,   5, 5, "column",},
    {"anchor",     2, ColumnResizeAnchorOp,     5, 5, "x",},
    {"deactivate", 1, ColumnResizeDeactivateOp, 4, 4, "",},
    {"mark",       1, ColumnResizeMarkOp,       5, 5, "x",},
    {"set",        1, ColumnResizeSetOp,        4, 4, "",},
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
    int result;

    proc = Blt_GetOpFromObj(interp, numColumnResizeOps, columnResizeOps, 
        BLT_OP_ARG3, objc, objv,0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}


static Blt_OpSpec columnOps[] =
{
    {"activate",   1, ColumnActivateOp,   4, 4, "field",},
    {"bind",       1, ColumnBindOp,       5, 7, "tagName type ?sequence command?",},
    {"cget",       2, ColumnCgetOp,       5, 5, "field option",},
    {"configure",  2, ColumnConfigureOp,  4, 0, "field ?option value?...",},
    {"current",    2, ColumnCurrentOp,    3, 3, "",},
    {"deactivate", 3, ColumnDeactivateOp, 3, 3, "",},
    {"delete",     3, ColumnDeleteOp,     3, 0, "?field...?",},
    {"exists",     1, ColumnExistsOp,     4, 4, "field",},
    {"index",      3, ColumnIndexOp,      4, 4, "field",},
    {"insert",     3, ColumnInsertOp,     5, 0, 
        "position field ?field...? ?option value?...",},
    {"invoke",     3, ColumnInvokeOp,     4, 4, "field",},
    {"names",      2, ColumnNamesOp,      3, 3, "",},
    {"nearest",    2, ColumnNearestOp,    4, 5, "x ?y?",},
    {"resize",     1, ColumnResizeOp,     3, 0, "arg",},
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

    int result;
    proc = Blt_GetOpFromObj(interp, numColumnOps, columnOps, BLT_OP_ARG2, 
        objc, objv,0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
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
 *      Configuration information, such as text string, colors, font, etc. get
 *      set for viewPtr; old resources get freed, if there were any.  The widget
 *      is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;

    if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, 
                viewSpecs, (char *)viewPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, 
                viewSpecs, (char *)viewPtr, objv[2], 0);
    } 
    iconsOption.clientData = viewPtr;
    treeOption.clientData = viewPtr;
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, viewSpecs, 
        objc - 2, objv + 2, (char *)viewPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ConfigureTreeView(interp, viewPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*ARGSUSED*/
static int
CurselectionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (viewPtr->sel.flags & SELECT_SORTED) {
        Blt_ChainLink link;

        for (link = Blt_Chain_FirstLink(viewPtr->sel.list); link != NULL;
             link = Blt_Chain_NextLink(link)) {
            Entry *entryPtr;
            Tcl_Obj *objPtr;

            entryPtr = Blt_Chain_GetValue(link);
            objPtr = NodeToObj(entryPtr->node);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                        
        }
    } else {
        Entry *entryPtr;

        /* It's OK is an entry's ancestor is hidden, add the selected node
         * to the list. */
        for (entryPtr = viewPtr->rootPtr; entryPtr != NULL; 
             entryPtr = NextEntry(entryPtr, HIDDEN)) {
            if (EntryIsSelected(viewPtr, entryPtr)) {
                Tcl_Obj *objPtr;

                objPtr = NodeToObj(entryPtr->node);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryActivateOp --
 *
 *      Selects the entry to appear active.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *newPtr, *oldPtr;
    char *string;

    string = Tcl_GetString(objv[3]);
    if (string[0] == '\0') {
        newPtr = NULL;
    } else if (GetEntryFromObj(interp, viewPtr, objv[3], &newPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (viewPtr->treeColumn.flags & HIDDEN) {
        return TCL_OK;
    }
    oldPtr = viewPtr->activePtr;
    viewPtr->activePtr = newPtr;
    if (!(viewPtr->flags & REDRAW_PENDING) && (newPtr != oldPtr)) {
        Drawable drawable;
        drawable = Tk_WindowId(viewPtr->tkwin);
        if (oldPtr != NULL) {
            DisplayLabel(viewPtr, oldPtr, drawable);
        }
        if (newPtr != NULL) {
            DisplayLabel(viewPtr, newPtr, drawable);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryCgetOp --
 *
 *      pathName entry cget entryName option
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;

    if (GetEntry(interp, viewPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, entrySpecs, 
        (char *)entryPtr, objv[4], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryConfigureOp --
 *
 *      This procedure is called to process a list of configuration options
 *      database, in order to reconfigure the one of more entries in the
 *      widget.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for viewPtr; old resources get freed, if there were
 *      any.  The hypertext is redisplayed.
 *
 *      pathName entry configure entryName ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
static int
EntryConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;
    EntryIterator iter;

    iconsOption.clientData = viewPtr;
    cachedObjOption.clientData = viewPtr;

    /* If we're setting a configuration values, we handle multiple entries. */
    if (GetEntryIterator(interp, viewPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
         entryPtr = NextTaggedEntry(&iter)) {
        if (objc == 4) {
            return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, 
                entrySpecs, (char *)entryPtr, (Tcl_Obj *)NULL, 0);
        } else if (objc == 5) {
            return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, 
                entrySpecs, (char *)entryPtr, objv[4], 0);
        }
        if (ConfigureEntry(viewPtr, entryPtr, objc - 4, objv + 4, 
                           BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * EntryIndexOp --
 *
 *      Converts one of more words representing indices of the entries in
 *      the treeview widget to their respective serial identifiers.
 *
 * Results:
 *      A standard TCL result.  Interp->result will contain the identifier
 *      of each inode found. If an inode could not be found, then the
 *      serial identifier will be the empty string.
 *
 *      pathName entry index entryName ?switches ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryIndexOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;
    long inode;
    IndexSwitches switches;

    memset(&switches, 0, sizeof(switches));
    if (viewPtr->focusPtr != NULL) {
        switches.fromPtr = viewPtr->focusPtr;
    } else if (viewPtr->rootPtr == NULL) {
        switches.fromPtr = viewPtr->rootPtr;
    }
    /* Process switches  */
    entrySwitch.clientData = viewPtr;
    if (Blt_ParseSwitches(interp, indexSwitches, objc - 4, objv + 4, &switches,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    inode = -1;
    if (switches.flags & INDEX_USE_PATH) {
        entryPtr = FindPath(interp, viewPtr, switches.fromPtr, objv[3]);
        if (entryPtr != NULL) {
            inode = Blt_Tree_NodeId(entryPtr->node);
        }
    } else {
        viewPtr->fromPtr = switches.fromPtr;
        if ((GetEntryFromObj2(interp, viewPtr, objv[3], &entryPtr) == TCL_OK) 
            && (entryPtr != NULL)) {
            inode = Blt_Tree_NodeId(entryPtr->node);
        }
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryInvokeOp --
 *
 *      pathName entry invoke entryName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryInvokeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    EntryIterator iter;
    Entry *entryPtr;
    
    if (GetEntryIterator(interp, viewPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
         entryPtr = NextTaggedEntry(&iter)) {
        Tcl_Obj *cmdObjPtr;

        cmdObjPtr = (entryPtr->cmdObjPtr != NULL) ? entryPtr->cmdObjPtr :
            viewPtr->entryCmdObjPtr;
        if (cmdObjPtr != NULL) {
            int result;
            
            cmdObjPtr = PercentSubst(viewPtr, entryPtr, cmdObjPtr);
            Tcl_IncrRefCount(cmdObjPtr);
            Tcl_Preserve(entryPtr);
            result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
            Tcl_Release(entryPtr);
            Tcl_DecrRefCount(cmdObjPtr);
            if (result != TCL_OK) {
                return TCL_ERROR;
            }
        }
    } 
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryIsBeforeOp --
 *
 *      pathName entry isbefore e1Name e2Name
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryIsBeforeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *e1Ptr, *e2Ptr;
    int bool;

    if ((GetEntry(interp, viewPtr, objv[3], &e1Ptr) != TCL_OK) ||
        (GetEntry(interp, viewPtr, objv[4], &e2Ptr) != TCL_OK)) {
        return TCL_ERROR;
    }
    bool = IsBefore(e1Ptr, e2Ptr);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryIsExposedOp --
 *
 *      pathName entry isexposed entryName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryIsExposedOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;
    int bool;

    if (GetEntry(interp, viewPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    bool = ((entryPtr->flags & HIDDEN) == 0);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * EntryIsHiddenOp --
 *
 *      pathName entry ishidden entryName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryIsHiddenOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;
    int bool;

    if (GetEntry(interp, viewPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    bool = (entryPtr->flags & HIDDEN);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * EntryIsOpenOp --
 *
 *      pathName entry isopen entryName
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryIsOpenOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;
    int bool;

    if (GetEntry(interp, viewPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    bool = IsOpen(entryPtr);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryChildrenOp --
 *
 *      pathName entry children entryName ?switches ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryChildrenOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *parentPtr;
    Tcl_Obj *listObjPtr;
    ChildrenSwitches switches;
    Entry *entryPtr;

    if (GetEntry(interp, viewPtr, objv[3], &parentPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    switches.mask = 0;
    if (Blt_ParseSwitches(interp, childrenSwitches, objc - 4, objv + 4, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

    for (entryPtr = FirstChild(parentPtr, switches.mask); entryPtr != NULL; 
         entryPtr = NextSibling(entryPtr, switches.mask)) {
        Tcl_Obj *objPtr;

        objPtr = NodeToObj(entryPtr->node);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * EntryDeleteOp --
 *
 *      pathName entry degree entryName
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryDegreeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *parentPtr, *entryPtr;
    long count;

    if (GetEntry(interp, viewPtr, objv[3], &parentPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    count = 0;
    for (entryPtr = FirstChild(parentPtr, HIDDEN); entryPtr != NULL; 
         entryPtr = NextSibling(entryPtr, HIDDEN)) {
        count++;
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), count);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryDeleteOp --
 *
 *      Deletes the child node from the parent.
 *
 *      pathName entry delete entryName entryPos
 *      pathName entry delete entryName firstPos lastPos
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EntryDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;

    if (GetEntry(interp, viewPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 5) {
        long entryPos;
        Blt_TreeNode node;

        /*
         * Delete a single child node from a hierarchy specified by its
         * numeric position.
         */
        if (Blt_GetPositionFromObj(interp, objv[3], &entryPos) != TCL_OK) {
            return TCL_ERROR;
        }
        if (entryPos >= (long)Blt_Tree_NodeDegree(entryPtr->node)) {
            return TCL_OK;      /* Bad first index */
        }
        if (entryPos == END) {
            node = Blt_Tree_LastChild(entryPtr->node);
        } else {
            node = GetNthNode(entryPtr->node, entryPos);
        }
        DeleteNode(viewPtr, node);
    } else {
        long firstPos, lastPos;
        Blt_TreeNode node, first, last, next;
        long numEntries;
        /*
         * Delete range of nodes in hierarchy specified by first/last
         * positions.
         */
        if ((Blt_GetPositionFromObj(interp, objv[4], &firstPos) != TCL_OK) ||
            (Blt_GetPositionFromObj(interp, objv[5], &lastPos) != TCL_OK)) {
            return TCL_ERROR;
        }
        numEntries = Blt_Tree_NodeDegree(entryPtr->node);
        if (numEntries == 0) {
            return TCL_OK;
        }
        if (firstPos == END) {
            firstPos = numEntries - 1;
        }
        if (firstPos >= numEntries) {
            Tcl_AppendResult(interp, "first position \"", 
                Tcl_GetString(objv[4]), " is out of range", (char *)NULL);
            return TCL_ERROR;
        }
        if ((lastPos == END) || (lastPos >= numEntries)) {
            lastPos = numEntries - 1;
        }
        if (firstPos > lastPos) {
            Tcl_AppendResult(interp, "bad range: \"", Tcl_GetString(objv[4]), 
                " > ", Tcl_GetString(objv[5]), "\"", (char *)NULL);
            return TCL_ERROR;
        }
        first = GetNthNode(entryPtr->node, firstPos);
        last = GetNthNode(entryPtr->node, lastPos);
        for (node = first; node != NULL; node = next) {
            next = Blt_Tree_NextSibling(node);
            DeleteNode(viewPtr, node);
            if (node == last) {
                break;
            }
        }
    }
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EntrySizeOp --
 *
 *      Counts the number of entries at this node.
 *
 * Results:
 *      A standard TCL result.  If an error occurred TCL_ERROR is returned
 *      and interp->result will contain an error message.  Otherwise,
 *      TCL_OK is returned and interp->result contains the number of
 *      entries.
 *
 *      pathName entry size ?-recurse? entryName 
 *---------------------------------------------------------------------------
 */
static int
EntrySizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;
    int length, recurse;
    long sum;
    char *string;

    recurse = FALSE;
    string = Tcl_GetStringFromObj(objv[3], &length);
    if ((string[0] == '-') && (length > 1) &&
        (strncmp(string, "-recurse", length) == 0)) {
        objv++, objc--;
        recurse = TRUE;
    }
    if (objc == 3) {
        Tcl_AppendResult(interp, "missing node argument: should be \"",
            Tcl_GetString(objv[0]), " entry open node\"", (char *)NULL);
        return TCL_ERROR;
    }
    if (GetEntry(interp, viewPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    if (recurse) {
        sum = Blt_Tree_Size(entryPtr->node);
    } else {
        sum = Blt_Tree_NodeDegree(entryPtr->node);
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), sum);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryOp --
 *
 *      This procedure handles entry operations.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */

static Blt_OpSpec entryOps[] =
{
    {"activate",  1, EntryActivateOp,  4, 4, "entryName",},
    /*bbox*/
    /*bind*/
    {"cget",      2, EntryCgetOp,      5, 5, "entryName option",},
    {"children",  2, EntryChildrenOp,  4, 0, "entryName ?switches ...?",},
    /*close*/
    {"configure", 2, EntryConfigureOp, 4, 0, "entryName ?option value?...",},
    {"degree",    3, EntryDegreeOp,    4, 4, "entryName",},
    {"delete",    3, EntryDeleteOp,    5, 6, "entryName firstPos ?lastPos?",},
    /*focus*/
    /*hide*/
    {"highlight", 1, EntryActivateOp,  4, 4, "entryName",},
    {"index",     3, EntryIndexOp,     4, 0, "entryName ?switches ...?",},
    {"invoke",    3, EntryInvokeOp,    4, 4, "entryName",},
    {"isbefore",  3, EntryIsBeforeOp,  5, 5, "entryName beforeEntry",},
    {"isexposed", 3, EntryIsExposedOp, 4, 4, "entryName",},
    {"ishidden",  3, EntryIsHiddenOp,  4, 4, "entryName",},
    {"isopen",    3, EntryIsOpenOp,    4, 4, "entryName",},
    /*move*/
    /*nearest*/
    /*open*/
    /*see*/
    /*show*/
    {"size",      1, EntrySizeOp,      4, 5, "?-recurse? entryName",},
    /*toggle*/
};
static int numEntryOps = sizeof(entryOps) / sizeof(Blt_OpSpec);

static int
EntryOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numEntryOps, entryOps, BLT_OP_ARG2, objc, 
        objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
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
 * FindOp --
 *
 *      Find one or more nodes based upon the pattern provided.
 *
 * Results:
 *      A standard TCL result.  The interpreter result will contain a list
 *      of the node serial identifiers.
 *
 *---------------------------------------------------------------------------
 */
static int
FindOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *firstPtr, *lastPtr;
    int numMatches, maxMatches;
    char c;
    int length;
    CompareProc *compareProc;
    IterProc *nextProc;
    int invertMatch;                    /* Normal search mode (matching
                                         * entries) */
    char *namePattern, *fullPattern;
    int i;
    int result;
    char *pattern, *option;
    Blt_List options;
    Blt_ListNode node;
    Entry *entryPtr;
    char *string;
    Tcl_Obj *listObjPtr, *objPtr, *execCmdObjPtr, *addTagObjPtr, *withTagObjPtr;

    invertMatch = FALSE;
    maxMatches = 0;
    namePattern = fullPattern = NULL;
    execCmdObjPtr = NULL;
    compareProc = ExactCompare;
    nextProc = NextEntry;
    options = Blt_List_Create(BLT_ONE_WORD_KEYS);
    withTagObjPtr = addTagObjPtr = NULL;

    entryPtr = viewPtr->rootPtr;
    /*
     * Step 1:  Process flags for find operation.
     */
    for (i = 2; i < objc; i++) {
        string = Tcl_GetStringFromObj(objv[i], &length);
        if (string[0] != '-') {
            break;
        }
        option = string + 1;
        length--;
        c = option[0];
        if ((c == 'e') && (length > 2) &&
            (strncmp(option, "exact", length) == 0)) {
            compareProc = ExactCompare;
        } else if ((c == 'g') && (strncmp(option, "glob", length) == 0)) {
            compareProc = GlobCompare;
        } else if ((c == 'r') && (strncmp(option, "regexp", length) == 0)) {
            compareProc = RegexpCompare;
        } else if ((c == 'n') && (length > 1) &&
            (strncmp(option, "nonmatching", length) == 0)) {
            invertMatch = TRUE;
        } else if ((c == 'n') && (length > 1) &&
            (strncmp(option, "name", length) == 0)) {
            if ((i + 1) == objc) {
                goto missingArg;
            }
            i++;
            namePattern = Tcl_GetString(objv[i]);
        } else if ((c == 'f') && (strncmp(option, "full", length) == 0)) {
            if ((i + 1) == objc) {
                goto missingArg;
            }
            i++;
            fullPattern = Tcl_GetString(objv[i]);
        } else if ((c == 'e') && (length > 2) &&
            (strncmp(option, "exec", length) == 0)) {
            if ((i + 1) == objc) {
                goto missingArg;
            }
            i++;
            execCmdObjPtr = objv[i];
        } else if ((c == 'a') && (length > 1) &&
                   (strncmp(option, "addtag", length) == 0)) {
            if ((i + 1) == objc) {
                goto missingArg;
            }
            i++;
            addTagObjPtr = objv[i];
        } else if ((c == 't') && (length > 1) && 
                   (strncmp(option, "tag", length) == 0)) {
            if ((i + 1) == objc) {
                goto missingArg;
            }
            i++;
            withTagObjPtr = objv[i];
        } else if ((c == 'c') && (strncmp(option, "count", length) == 0)) {
            if ((i + 1) == objc) {
                goto missingArg;
            }
            i++;
            if (Tcl_GetIntFromObj(interp, objv[i], &maxMatches) != TCL_OK) {
                return TCL_ERROR;
            }
            if (maxMatches < 0) {
                Tcl_AppendResult(interp, "bad match count \"", objv[i],
                    "\": should be a positive number", (char *)NULL);
                Blt_List_Destroy(options);
                return TCL_ERROR;
            }
        } else if ((option[0] == '-') && (option[1] == '\0')) {
            break;
        } else {
            /*
             * Verify that the switch is actually an entry configuration
             * option.
             */
            if (Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, entrySpecs, 
                (char *)entryPtr, objv[i], 0) != TCL_OK) {
                Tcl_ResetResult(interp);
                Tcl_AppendResult(interp, "bad find switch \"", string, "\"",
                    (char *)NULL);
                Blt_List_Destroy(options);
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

    if ((objc - i) > 2) {
        Blt_List_Destroy(options);
        Tcl_AppendResult(interp, "too many args", (char *)NULL);
        return TCL_ERROR;
    }
    /*
     * Step 2:  Find the range of the search.  Check the order of two
     *          nodes and arrange the search accordingly.
     *
     *  Note:   Be careful to treat "end" as the end of all nodes, instead
     *          of the end of visible nodes.  That way, we can search the
     *          entire tree, even if the last folder is closed.
     */
    firstPtr = viewPtr->rootPtr;        /* Default to root node */
    lastPtr = LastEntry(viewPtr, firstPtr, 0);

    if (i < objc) {
        string = Tcl_GetString(objv[i]);
        if ((string[0] == 'e') && (strcmp(string, "end") == 0)) {
            firstPtr = LastEntry(viewPtr, viewPtr->rootPtr, 0);
        } else if (GetEntry(interp, viewPtr, objv[i], &firstPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        i++;
    }
    if (i < objc) {
        string = Tcl_GetString(objv[i]);
        if ((string[0] == 'e') && (strcmp(string, "end") == 0)) {
            lastPtr = LastEntry(viewPtr, viewPtr->rootPtr, 0);
        } else if (GetEntry(interp, viewPtr, objv[i], &lastPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (IsBefore(lastPtr, firstPtr)) {
        nextProc = PrevEntry;
    }
    numMatches = 0;

    /*
     * Step 3:  Search through the tree and look for nodes that match the
     *          current pattern specifications.  Save the name of each of
     *          the matching nodes.
     */
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (entryPtr = firstPtr; entryPtr != NULL; 
         entryPtr = (*nextProc) (entryPtr, 0)) {
        if (namePattern != NULL) {
            result = (*compareProc)(interp, Blt_Tree_NodeLabel(entryPtr->node),
                     namePattern);
            if (result == invertMatch) {
                goto nextEntry; /* Failed to match */
            }
        }
        if (fullPattern != NULL) {
            Tcl_DString ds;

            GetPathFromRoot(viewPtr, entryPtr, FALSE, &ds);
            result = (*compareProc) (interp, Tcl_DStringValue(&ds),fullPattern);
            Tcl_DStringFree(&ds);
            if (result == invertMatch) {
                goto nextEntry; /* Failed to match */
            }
        }
        if (withTagObjPtr != NULL) {
            result = Blt_Tree_HasTag(viewPtr->tree, entryPtr->node,
                        Tcl_GetString(withTagObjPtr));
            if (result == invertMatch) {
                goto nextEntry; /* Failed to match */
            }
        }
        for (node = Blt_List_FirstNode(options); node != NULL;
            node = Blt_List_NextNode(node)) {
            objPtr = (Tcl_Obj *)Blt_List_GetKey(node);
            Tcl_ResetResult(interp);
            Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, entrySpecs, 
                (char *)entryPtr, objPtr, 0);
            pattern = Blt_List_GetValue(node);
            objPtr = Tcl_GetObjResult(interp);
            result = (*compareProc) (interp, Tcl_GetString(objPtr), pattern);
            if (result == invertMatch) {
                goto nextEntry; /* Failed to match */
            }
        }
        /* 
         * Someone may actually delete the current node in the "exec"
         * callback.  Preserve the entry.
         */
        Tcl_Preserve(entryPtr);
        if (execCmdObjPtr != NULL) {
            Tcl_Obj *cmdObjPtr;

            cmdObjPtr = PercentSubst(viewPtr, entryPtr, execCmdObjPtr);
            Tcl_IncrRefCount(cmdObjPtr);
            result = Tcl_EvalObjEx(viewPtr->interp, cmdObjPtr, TCL_EVAL_GLOBAL);
            Tcl_DecrRefCount(cmdObjPtr);
            if (result != TCL_OK) {
                Tcl_Release(entryPtr);
                goto error;
            }
        }
        /* A NULL node reference in an entry indicates that the entry was
         * deleted, but its memory not released yet. */
        if (entryPtr->node != NULL) {
            /* Finally, save the matching node name. */
            objPtr = NodeToObj(entryPtr->node);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            if (addTagObjPtr != NULL) {
                if (AddTag(interp, viewPtr, entryPtr->node, addTagObjPtr)
                    != TCL_OK) {
                    goto error;
                }
            }
        }
            
        Tcl_Release(entryPtr);
        numMatches++;
        if ((numMatches == maxMatches) && (maxMatches > 0)) {
            break;
        }
      nextEntry:
        if (entryPtr == lastPtr) {
            break;
        }
    }
    Tcl_ResetResult(interp);
    Blt_List_Destroy(options);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;

  missingArg:
    Tcl_AppendResult(interp, "missing argument for find option \"", objv[i],
        "\"", (char *)NULL);
  error:
    Blt_List_Destroy(options);
    return TCL_ERROR;
}


/*
 *---------------------------------------------------------------------------
 *
 * FocusOp --
 *
 *      Find one or more nodes based upon the pattern provided.
 *
 * Results:
 *      A standard TCL result.  The interpreter result will contain a list
 *      of the node serial identifiers.
 *
 *      pathName focus ?entryName?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FocusOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    long inode;

    if (objc == 3) {
        Entry *entryPtr;

        if (GetEntryFromObj(interp, viewPtr, objv[2], &entryPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if ((entryPtr != NULL) && (entryPtr != viewPtr->focusPtr)) {
            if (entryPtr->flags & HIDDEN) {
                /* Doesn't make sense to set focus to a node you can't see. */
                MapAncestors(viewPtr, entryPtr);
            }
            /* Changing focus can only affect the visible entries.  The entry
             * layout stays the same. */
            if (viewPtr->focusPtr != NULL) {
                viewPtr->focusPtr->flags |= ENTRY_REDRAW;
            } 
            entryPtr->flags |= ENTRY_REDRAW;
            viewPtr->flags |= SCROLL_PENDING;
            viewPtr->focusPtr = entryPtr;
        }
        EventuallyRedraw(viewPtr);
    }
    Blt_SetFocusItem(viewPtr->bindTable, viewPtr->focusPtr, ITEM_ENTRY);
    inode = -1;
    if (viewPtr->focusPtr != NULL) {
        inode = Blt_Tree_NodeId(viewPtr->focusPtr->node);
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *
 *      Converts one or more node identifiers to its path component.  The
 *      path may be either the single entry name or the full path of the
 *      entry.
 *
 * Results:
 *      A standard TCL result.  The interpreter result will contain a list
 *      of the convert names.
 *
 *      pathName get -full entryName ...
 *---------------------------------------------------------------------------
 */
static int
GetOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Entry *entryPtr;
    EntryIterator iter;
    Tcl_DString d1, d2;
    TreeView *viewPtr = clientData;
    int count;
    int i;
    int useFullName;

    useFullName = FALSE;
    if (objc > 2) {
        char *string;

        string = Tcl_GetString(objv[2]);
        if ((string[0] == '-') && (strcmp(string, "-full") == 0)) {
            useFullName = TRUE;
            objv++, objc--;
        }
    }
    Tcl_DStringInit(&d1);       /* Result. */
    Tcl_DStringInit(&d2);       /* Last element. */
    count = 0;
    for (i = 2; i < objc; i++) {
        if (GetEntryIterator(interp, viewPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
             entryPtr = NextTaggedEntry(&iter)) {
            Tcl_DStringSetLength(&d2, 0);
            count++;
            if (entryPtr->node != NULL) {
                if (useFullName) {
                    GetPathFromRoot(viewPtr, entryPtr, FALSE, &d2);
                } else {
                    Tcl_DStringAppend(&d2, Blt_Tree_NodeLabel(entryPtr->node),
                        -1);
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
 * SearchAndApplyToTree --
 *
 *      Searches through the current tree and applies a procedure to
 *      matching nodes.  The search specification is taken from the
 *      following command-line arguments:
 *
 *      ?-exact? ?-glob? ?-regexp? ?-nonmatching?
 *      ?-data string?
 *      ?-name string?
 *      ?-full string?
 *      ?--?
 *      ?inode...?
 *
 * Results:
 *      A standard TCL result.  If the result is valid, and if the
 *      nonmatchPtr is specified, it returns a boolean value indicating
 *      whether or not the search was inverted.  This is needed to fix
 *      things properly for the "hide nonmatching" case.
 *
 *---------------------------------------------------------------------------
 */
static int
SearchAndApplyToTree(TreeView *viewPtr, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv, TreeViewApplyProc *proc,
                     int *nonMatchPtr)
{
    CompareProc *compareProc;
    int invertMatch;                    /* Normal search mode (matching
                                         * entries) */
    char *namePattern, *fullPattern;
    int i;
    int length;
    int result;
    char *option, *pattern;
    char c;
    Blt_List options;
    Entry *entryPtr;
    Blt_ListNode node;
    char *string;
    Tcl_Obj *withTagObjPtr;
    Tcl_Obj *objPtr;
    EntryIterator iter;

    options = Blt_List_Create(BLT_ONE_WORD_KEYS);
    invertMatch = FALSE;
    namePattern = fullPattern = NULL;
    compareProc = ExactCompare;
    withTagObjPtr = NULL;

    entryPtr = viewPtr->rootPtr;
    for (i = 0; i < objc; i++) {
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
            withTagObjPtr = objv[i];
        } else if ((option[0] == '-') && (option[1] == '\0')) {
            break;
        } else {
            /*
             * Verify that the switch is actually an entry configuration
             * option.
             */
            if (Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, entrySpecs, 
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
         * Search through the tree and look for nodes that match the
         * current spec.  Apply the input procedure to each of the matching
         * nodes.
         */
        for (entryPtr = viewPtr->rootPtr; entryPtr != NULL; 
             entryPtr = NextEntry(entryPtr, 0)) {
            if (namePattern != NULL) {
                result = (*compareProc) (interp, 
                        Blt_Tree_NodeLabel(entryPtr->node), namePattern);
                if (result == invertMatch) {
                    continue;           /* Failed to match */
                }
            }
            if (fullPattern != NULL) {
                Tcl_DString ds;

                GetPathFromRoot(viewPtr, entryPtr, FALSE, &ds);
                result = (*compareProc) (interp, Tcl_DStringValue(&ds), 
                        fullPattern);
                Tcl_DStringFree(&ds);
                if (result == invertMatch) {
                    continue;           /* Failed to match */
                }
            }
            if (withTagObjPtr != NULL) {
                result = Blt_Tree_HasTag(viewPtr->tree, entryPtr->node,
                        Tcl_GetString(withTagObjPtr));
                if (result == invertMatch) {
                    continue;           /* Failed to match */
                }
            }
            for (node = Blt_List_FirstNode(options); node != NULL;
                node = Blt_List_NextNode(node)) {
                objPtr = (Tcl_Obj *)Blt_List_GetKey(node);
                Tcl_ResetResult(interp);
                if (Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, 
                        entrySpecs, (char *)entryPtr, objPtr, 0) != TCL_OK) {
                    return TCL_ERROR;   /* This shouldn't happen. */
                }
                pattern = Blt_List_GetValue(node);
                objPtr = Tcl_GetObjResult(interp);
                result = (*compareProc)(interp, Tcl_GetString(objPtr), pattern);
                if (result == invertMatch) {
                    continue;           /* Failed to match */
                }
            }
            /* Finally, apply the procedure to the node */
            if ((*proc) (viewPtr, entryPtr) != TCL_OK) {
                return TCL_ERROR;
            }
        }
        Tcl_ResetResult(interp);
        Blt_List_Destroy(options);
    }
    /*
     * Apply the procedure to nodes that have been specified individually.
     */
    for ( /*empty*/ ; i < objc; i++) {
        if (GetEntryIterator(interp, viewPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
             entryPtr = NextTaggedEntry(&iter)) {
            if ((*proc) (viewPtr, entryPtr) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    if (nonMatchPtr != NULL) {
        *nonMatchPtr = invertMatch;     /* return "inverted search" status */
    }
    return TCL_OK;

  missingArg:
    Blt_List_Destroy(options);
    Tcl_AppendResult(interp, "missing pattern for search option \"", objv[i],
        "\"", (char *)NULL);
    return TCL_ERROR;

}

static int
FixSelectionsApplyProc(TreeView *viewPtr, Entry *entryPtr)
{
    if (entryPtr->flags & HIDDEN) {
        DeselectEntry(viewPtr, entryPtr);
        if ((viewPtr->focusPtr != NULL) &&
            (Blt_Tree_IsAncestor(entryPtr->node, viewPtr->focusPtr->node))) {
            if (entryPtr != viewPtr->rootPtr) {
                entryPtr = entryPtr->parentPtr;
                viewPtr->focusPtr = (entryPtr == NULL) 
                    ? viewPtr->focusPtr : entryPtr;
                Blt_SetFocusItem(viewPtr->bindTable, viewPtr->focusPtr, 
                                 ITEM_ENTRY);
            }
        }
        if ((viewPtr->sel.anchorPtr != NULL) &&
            (Blt_Tree_IsAncestor(entryPtr->node, 
                                 viewPtr->sel.anchorPtr->node))) {
            viewPtr->sel.markPtr = viewPtr->sel.anchorPtr = NULL;
        }
        if ((viewPtr->activePtr != NULL) &&
            (Blt_Tree_IsAncestor(entryPtr->node, viewPtr->activePtr->node))) {
            viewPtr->activePtr = NULL;
        }
        PruneSelection(viewPtr, entryPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HideOp --
 *
 *      Hides one or more nodes.  Nodes can be specified by their inode, or
 *      by matching a name or data value pattern.  By default, the patterns
 *      are matched exactly.  They can also be matched using glob-style and
 *      regular expression rules.
 *
 * Results:
 *      A standard TCL result.
 *
 *      pathName hide 
 *---------------------------------------------------------------------------
 */
static int
HideOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    int status, nonmatching;

    status = SearchAndApplyToTree(viewPtr, interp, objc - 2, objv + 2, 
        HideEntryApplyProc, &nonmatching);

    if (status != TCL_OK) {
        return TCL_OK;
    }
    /*
     * If this was an inverted search, scan back through the tree and make
     * sure that the parents for all visible nodes are also visible.  After
     * all, if a node is supposed to be visible, its parent can't be
     * hidden.
     */
    if (nonmatching) {
        Apply(viewPtr, viewPtr->rootPtr, MapAncestorsApplyProc, 0);
    }
    /*
     * Make sure that selections are cleared from any hidden nodes.  This
     * wasn't done earlier--we had to delay it until we fixed the
     * visibility status for the parents.
     */
    Apply(viewPtr, viewPtr->rootPtr, FixSelectionsApplyProc, 0);

    /* Hiding an entry only effects the visible nodes. */
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ShowOp --
 *
 *      Mark one or more nodes to be exposed.  Nodes can be specified by
 *      their inode, or by matching a name or data value pattern.  By
 *      default, the patterns are matched exactly.  They can also be
 *      matched using glob-style and regular expression rules.
 *
 * Results:
 *      A standard TCL result.
 *
 *      pathName show 
 *---------------------------------------------------------------------------
 */
static int
ShowOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    if (SearchAndApplyToTree(viewPtr, interp, objc - 2, objv + 2,
                ShowEntryApplyProc, (int *)NULL) != TCL_OK) {
        return TCL_ERROR;
    }
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IndexOp --
 *
 *      Converts one of more words representing indices of the entries in
 *      the treeview widget to their respective serial identifiers.
 *
 * Results:
 *      A standard TCL result.  Interp->result will contain the identifier
 *      of each inode found. If an inode could not be found, then the
 *      serial identifier will be the empty string.
 *
 *      pathName index entryName ?switches ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IndexOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;
    long inode;
    IndexSwitches switches;

    memset(&switches, 0, sizeof(switches));
    if (viewPtr->focusPtr != NULL) {
        switches.fromPtr = viewPtr->focusPtr;
    } else if (viewPtr->rootPtr == NULL) {
        switches.fromPtr = viewPtr->rootPtr;
    }
    /* Process switches  */
    entrySwitch.clientData = viewPtr;
    if (Blt_ParseSwitches(interp, indexSwitches, objc - 3, objv + 3, &switches,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    inode = -1;
    if (switches.flags & INDEX_USE_PATH) {
        entryPtr = FindPath(interp, viewPtr, switches.fromPtr, objv[2]);
        if (entryPtr != NULL) {
            inode = Blt_Tree_NodeId(entryPtr->node);
        }
    } else {
        viewPtr->fromPtr = switches.fromPtr;
        if ((GetEntryFromObj2(interp, viewPtr, objv[2], &entryPtr) 
             == TCL_OK) && (entryPtr != NULL)) {
            inode = Blt_Tree_NodeId(entryPtr->node);
        }
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InvokeOp --
 *
 *      pathName invoke entryName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InvokeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    EntryIterator iter;
    Entry *entryPtr;

    if (GetEntryIterator(interp, viewPtr, objv[2], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
         entryPtr = NextTaggedEntry(&iter)) {
        Tcl_Obj *cmdObjPtr;
        
        cmdObjPtr = (entryPtr->cmdObjPtr != NULL) ? 
            entryPtr->cmdObjPtr : viewPtr->entryCmdObjPtr;
        if (cmdObjPtr != NULL) {
            int result;
            
            cmdObjPtr = PercentSubst(viewPtr, entryPtr, cmdObjPtr);
            Tcl_IncrRefCount(cmdObjPtr);
            Tcl_Preserve(entryPtr);
            result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
            Tcl_Release(entryPtr);
            Tcl_DecrRefCount(cmdObjPtr);
            if (result != TCL_OK) {
                return TCL_ERROR;
            }
        }
    } 
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InsertOp --
 *
 *      Adds new entries into a hierarchy.  If no node is specified, new
 *      entries will be added to the root of the hierarchy.
 *
 *      pathName insert fullPath ?switches ...?
 *
 *---------------------------------------------------------------------------
 */
static int
InsertOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Entry *entryPtr, *parentPtr;
    InsertSwitches switches;
    Tcl_Obj **elems;
    Tcl_Obj *listObjPtr, *pathObjPtr;
    TreeView *viewPtr = clientData;
    const char *name;
    int i, numElems;
    int result;

    memset(&switches, 0, sizeof(switches));
    switches.insertPos = -1;
    if (viewPtr->rootPtr == NULL) {
        switches.rootPtr = viewPtr->rootPtr;
    }
    /* Process switches  */
    entrySwitch.clientData = viewPtr;
    if (Blt_ParseSwitches(interp, insertSwitches, objc - 3, objv + 3, &switches,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    
    pathObjPtr = TrimPathObj(viewPtr, objv[2]);
    if ((viewPtr->pathSep == SEPARATOR_NONE) || (viewPtr->pathSep[0] == '\0')) {
        listObjPtr = NULL;
        result = Tcl_ListObjGetElements(interp, pathObjPtr, &numElems, &elems);
    } else {
        listObjPtr = SplitPath(interp, pathObjPtr, viewPtr->pathSep);
        result = Tcl_ListObjGetElements(interp, listObjPtr, &numElems, &elems);
    }
    if (result != TCL_OK) {
        goto error;
    }
    parentPtr = switches.rootPtr;
    /* Verify each component in the path preceding the tail component.  */
    for (i = 0; i < (numElems - 1); i++) {
        name = Tcl_GetString(elems[i]);
        entryPtr = FindChild(parentPtr, name);
        if (entryPtr == NULL) {
            Blt_TreeNode node;

            if (switches.flags & INSERT_NODUPS) {
                Tcl_AppendResult(interp, "can't find path component \"",
                    name, "\" in \"", Tcl_GetString(pathObjPtr), "\"", 
                                 (char *)NULL);
                goto error;
            }
            node = Blt_Tree_CreateNode(viewPtr->tree, parentPtr->node, name, 
                END);
            if (node == NULL) {
                goto error;
            }
            entryPtr = CreateEntry(viewPtr, node, 0, NULL, 0);
            if (entryPtr == NULL) {
                goto error;
            }
        }
        parentPtr = entryPtr;
    }
    /* This is the tail of the path */
    name = Tcl_GetString(elems[i]);
    entryPtr = FindChild(parentPtr, name);
    if (entryPtr != NULL) {
        if ((viewPtr->flags & ALLOW_DUPLICATES) == 0) {
            Tcl_AppendResult(interp, "entry \"", name, 
                        "\" already exists in \"", Tcl_GetString(pathObjPtr), 
                        "\"", (char *)NULL);
            goto error;
        }
    } else {
        Blt_TreeNode node;

        node = Blt_Tree_CreateNode(viewPtr->tree, parentPtr->node, name, 
                                   switches.insertPos);
        if (node == NULL) {
            goto error;
        }
        entryPtr = CreateEntry(viewPtr, node, 0, NULL, 0);
        if (entryPtr != TCL_OK) {
            goto error;
        }
    }
    if (listObjPtr != NULL) {
        Tcl_DecrRefCount(listObjPtr);
    }
    Tcl_DecrRefCount(pathObjPtr);
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
    return TCL_OK;

  error:
    Tcl_DecrRefCount(pathObjPtr);
    if (listObjPtr != NULL) {
        Tcl_DecrRefCount(listObjPtr);
    }
    return TCL_ERROR;
}


/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *      Deletes nodes from the hierarchy. Deletes one or more entries
 *      (except root). In all cases, nodes are removed recursively.
 *
 *      Note: There's no need to explicitly clean up Entry structures or
 *            request a redraw of the widget. When a node is deleted in the
 *            tree, all of the Tcl_Objs representing the various data
 *            fields are also removed.  The treeview widget store the Entry
 *            structure in a data field. So it's automatically cleaned up
 *            when FreeEntryInternalRep is called.
 *
 *      pathName delete ?entryName ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    EntryIterator iter;
    Entry *entryPtr;
    int i;

    for (i = 2; i < objc; i++) {
        if (GetEntryIterator(interp,viewPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
             entryPtr = NextTaggedEntry(&iter)) {
            if (entryPtr == viewPtr->rootPtr) {
                Blt_TreeNode next, node;

                /* 
                 * Don't delete the root node.  We implicitly assume that
                 * even an empty tree has at a root.  Instead delete all
                 * the children regardless if they're closed or hidden.
                 */
                for (node = Blt_Tree_FirstChild(entryPtr->node); node != NULL; 
                     node = next) {
                    next = Blt_Tree_NextSibling(node);
                    DeleteNode(viewPtr, node);
                }
            } else {
                DeleteNode(viewPtr, entryPtr->node);
            }
        }
    } 
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * MoveOp --
 *
 *      Move an entry into a new location in the hierarchy.
 *
 *      pathName move entryName into|before|afer destName 
 *      pathName move entryName destName -before -after -into 
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MoveOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Blt_TreeNode parent;
    Entry *srcPtr, *destPtr;
    char c;
    int action;
    char *string;
    EntryIterator iter;

#define MOVE_INTO       (1<<0)
#define MOVE_BEFORE     (1<<1)
#define MOVE_AFTER      (1<<2)
    if (GetEntryIterator(interp, viewPtr, objv[2], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]);
    c = string[0];
    if ((c == 'i') && (strcmp(string, "into") == 0)) {
        action = MOVE_INTO;
    } else if ((c == 'b') && (strcmp(string, "before") == 0)) {
        action = MOVE_BEFORE;
    } else if ((c == 'a') && (strcmp(string, "after") == 0)) {
        action = MOVE_AFTER;
    } else {
        Tcl_AppendResult(interp, "bad position \"", string,
            "\": should be into, before, or after", (char *)NULL);
        return TCL_ERROR;
    }
    if (GetEntry(interp, viewPtr, objv[4], &destPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    for (srcPtr = FirstTaggedEntry(&iter); srcPtr != NULL; 
         srcPtr = NextTaggedEntry(&iter)) {
        /* Verify they aren't ancestors. */
        if (Blt_Tree_IsAncestor(srcPtr->node, destPtr->node)) {
            Tcl_DString ds;
            const char *path;

            path = GetPathFromRoot(viewPtr, srcPtr, 1, &ds);
            Tcl_AppendResult(interp, "can't move node: \"", path, 
                        "\" is an ancestor of \"", Tcl_GetString(objv[4]), 
                        "\"", (char *)NULL);
            Tcl_DStringFree(&ds);
            return TCL_ERROR;
        }
        parent = Blt_Tree_ParentNode(destPtr->node);
        if (parent == NULL) {
            action = MOVE_INTO;
        }
        switch (action) {
        case MOVE_INTO:
            Blt_Tree_MoveNode(viewPtr->tree, srcPtr->node, destPtr->node, 
                (Blt_TreeNode)NULL);
            break;
            
        case MOVE_BEFORE:
            Blt_Tree_MoveNode(viewPtr->tree, srcPtr->node, parent, 
                destPtr->node);
            break;
            
        case MOVE_AFTER:
            Blt_Tree_MoveNode(viewPtr->tree, srcPtr->node, parent, 
                Blt_Tree_NextSibling(destPtr->node));
            break;
        }
    }
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NearestOp --
 *
 *      Move an entry into a new location in the hierarchy.
 *
 *      pathName nearest screenX screenY ?switches ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NearestOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    int x, y;                           /* Screen coordinates of the test
                                         * point. */
    Entry *entryPtr;
    NearestSwitches switches;
    
    if ((Tk_GetPixelsFromObj(interp, viewPtr->tkwin, objv[2], &x) != TCL_OK) ||
        (Tk_GetPixelsFromObj(interp, viewPtr->tkwin, objv[3], &y) != TCL_OK)) {
        return TCL_ERROR;
    }
    switches.flags = 0;
    if (Blt_ParseSwitches(interp, nearestEntrySwitches, objc - 4, objv + 4, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (viewPtr->numVisibleEntries == 0) {
        return TCL_OK;
    }
    if (switches.flags & NEAREST_ROOT) {
        int rootX, rootY;

        Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
        x -= rootX;
        y -= rootY;
    }
    entryPtr = NearestEntry(viewPtr, x, y, TRUE);
    if (entryPtr == NULL) {
        return TCL_OK;
    }
    x = WORLDX(viewPtr, x);
    y = WORLDY(viewPtr, y);
    if (objc > 4) {
        const char *where;
        int lx, ly, depth;
        Icon icon;

        where = "";
        if (entryPtr->flags & ENTRY_BUTTON) {
            Button *butPtr = &viewPtr->button;
            int bx, by;

            bx = entryPtr->worldX + entryPtr->buttonX;
            by = entryPtr->worldY + entryPtr->buttonY;
            if ((x >= bx) && (x < (bx + butPtr->width)) &&
                (y >= by) && (y < (by + butPtr->height))) {
                where = "button";
                goto done;
            }
        } 
        depth = EntryDepth(viewPtr, entryPtr);

        icon = GetEntryIcon(viewPtr, entryPtr);
        if (icon != NULL) {
            int iw, ih, entryHeight;
            int ix, iy;
            
            entryHeight = MAX(entryPtr->iconHeight, viewPtr->button.height);
            ih = IconHeight(icon);
            iw = IconWidth(icon);
            ix = entryPtr->worldX + ICONWIDTH(depth);
            iy = entryPtr->worldY;
            if (viewPtr->flags & FLAT) {
                ix += (ICONWIDTH(0) - iw) / 2;
            } else {
                ix += (ICONWIDTH(depth + 1) - iw) / 2;
            }       
            iy += (entryHeight - ih) / 2;
            if ((x >= ix) && (x <= (ix + iw)) && (y >= iy) && (y < (iy + ih))) {
                where = "icon";
                goto done;
            }
        }
        lx = entryPtr->worldX + ICONWIDTH(depth);
        ly = entryPtr->worldY;
        if ((viewPtr->flags & FLAT) == 0) {
            lx += ICONWIDTH(depth + 1) + 4;
        }           
        if ((x >= lx) && (x < (lx + entryPtr->labelWidth)) &&
            (y >= ly) && (y < (ly + entryPtr->labelHeight))) {
            where = "label";
        }
    done:
        if (Tcl_SetVar(interp, Tcl_GetString(objv[4]), where, 
                TCL_LEAVE_ERR_MSG) == NULL) {
            return TCL_ERROR;
        }
    }
    Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * OpenOp --
 *
 *      pathName open entryName ?-recurse? 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
OpenOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    OpenSwitches switches;
    Entry *entryPtr;
    EntryIterator iter;

    if (GetEntryIterator(interp, viewPtr, objv[2], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    /* Process switches  */
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, openSwitches, objc - 3, objv + 3, &switches,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
         entryPtr = NextTaggedEntry(&iter)) {
        int result;

        if (switches.flags & OPEN_RECURSE) {
            result = Apply(viewPtr, entryPtr, OpenEntry, 0);
        } else {
            result = OpenEntry(viewPtr, entryPtr);
        }
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
        /* Make sure ancestors of this node aren't hidden. */
        MapAncestors(viewPtr, entryPtr);
    }
    /*FIXME: This is only for flattened entries.  */
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RangeOp --
 *
 *      Returns the node identifiers in a given range.
 *
 *      pathName range ?-open? firstName lastName
 *---------------------------------------------------------------------------
 */
static int
RangeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Entry *entryPtr, *firstPtr, *lastPtr;
    Tcl_Obj *listObjPtr, *objPtr;
    TreeView *viewPtr = clientData;
    const char *string;
    int length;
    unsigned int mask;

    mask = 0;
    lastPtr = firstPtr = NULL;          /* Suppress compiler warning. */
    string = Tcl_GetStringFromObj(objv[2], &length);
    if ((string[0] == '-') && (length > 1) && 
        (strncmp(string, "-open", length) == 0)) {
        objv++, objc--;
        mask |= CLOSED;
    }
    if (GetEntry(interp, viewPtr, objv[2], &firstPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc > 3) {
        if (GetEntry(interp, viewPtr, objv[3], &lastPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        lastPtr = LastEntry(viewPtr, firstPtr, mask);
    }    
    if (mask & CLOSED) {
        if (firstPtr->flags & HIDDEN) {
            Tcl_AppendResult(interp, "first node \"", Tcl_GetString(objv[2]), 
                "\" is hidden.", (char *)NULL);
            return TCL_ERROR;
        }
        if (lastPtr->flags & HIDDEN) {
            Tcl_AppendResult(interp, "last node \"", Tcl_GetString(objv[3]), 
                "\" is hidden.", (char *)NULL);
            return TCL_ERROR;
        }
    }

    /*
     * The relative order of the first/last markers determines the
     * direction.
     */
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (IsBefore(lastPtr, firstPtr)) {
        for (entryPtr = lastPtr; entryPtr != NULL; 
             entryPtr = PrevEntry(entryPtr, mask)) {
            objPtr = NodeToObj(entryPtr->node);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            if (entryPtr == firstPtr) {
                break;
            }
        }
    } else {
        for (entryPtr = firstPtr; entryPtr != NULL; 
             entryPtr = NextEntry(entryPtr, mask)) {
            objPtr = NodeToObj(entryPtr->node);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            if (entryPtr == lastPtr) {
                break;
            }
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
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
ScanOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tk_Window tkwin;
    TreeView *viewPtr = clientData;
    char *string;
    char c;
    int length, oper;
    int x, y;

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
 *      pathName see ?-anchor anchor? entryName
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SeeOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Entry *entryPtr;
    Tk_Anchor anchor;
    TreeView *viewPtr = clientData;
    const char *string;
    int left, right, top, bottom;
    int width, height;
    int x, y;

    string = Tcl_GetString(objv[2]);
    anchor = TK_ANCHOR_W;       /* Default anchor is West */
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
            "see ?-anchor anchor? entryName\"", (char *)NULL);
        return TCL_ERROR;
    }
    if (GetEntryFromObj(interp, viewPtr, objv[2], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (entryPtr == NULL) {
        return TCL_OK;
    }
    if (entryPtr->flags & HIDDEN) {
        /*
         * If the entry wasn't previously exposed, its world coordinates
         * aren't likely to be valid.  So re-compute the layout before we
         * try to see the viewport to the entry's location.
         */
        MapAncestors(viewPtr, entryPtr);
        viewPtr->flags |= LAYOUT_PENDING;
    }
    UpdateView(viewPtr);

    width = VPORTWIDTH(viewPtr);
    height = VPORTHEIGHT(viewPtr);

    /*
     * XVIEW:   If the entry is left or right of the current view, adjust the
     *          offset.  If the entry is nearby, adjust the view just a
     *          bit.  Otherwise, center the entry.
     */
    left = viewPtr->xOffset;
    right = viewPtr->xOffset + width;

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
            ICONWIDTH(EntryDepth(viewPtr, entryPtr)) - width;
        break;
    default:
        if (entryPtr->worldX < left) {
            x = entryPtr->worldX;
        } else if ((entryPtr->worldX + entryPtr->width) > right) {
            x = entryPtr->worldX + entryPtr->width - width;
        } else {
            x = viewPtr->xOffset;
        }
        break;
    }
    /*
     * YVIEW:   If the entry is above or below the current view, adjust
     *          the offset.  If the entry is nearby, adjust the view just
     *          a bit.  Otherwise, center the entry.
     */
    top = viewPtr->yOffset;
    bottom = viewPtr->yOffset + height;
    switch (anchor) {
    case TK_ANCHOR_N:
        y = viewPtr->yOffset;
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
            y = viewPtr->yOffset;
        }
        break;
    }
    if ((y != viewPtr->yOffset) || (x != viewPtr->xOffset)) {
        /* viewPtr->xOffset = x; */
        viewPtr->yOffset = y;
        viewPtr->flags |= SCROLL_PENDING | VISIBILITY;
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
 *      pathName selection anchor entryName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionAnchorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;

    if (GetEntryFromObj(interp, viewPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    /* Set both the anchor and the mark. Indicates that a single entry
     * is selected. */
    viewPtr->sel.anchorPtr = entryPtr;
    viewPtr->sel.markPtr = NULL;
    if (entryPtr != NULL) {
        Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
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
    TreeView *viewPtr = clientData;

    ClearSelection(viewPtr);
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
    TreeView *viewPtr = clientData;

    if (Blt_Chain_GetLength(viewPtr->sel.list) > 0) {
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
 *      pathName selection includes entryName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionIncludesOp(ClientData clientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;
    int bool;

    if (GetEntryFromObj(interp, viewPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    bool = FALSE;
    if (entryPtr != NULL) {
        bool = EntryIsSelected(viewPtr, entryPtr);
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionMarkOp --
 *
 *      Sets the selection mark to the element given by a index.  The
 *      selection anchor is the end of the selection that is movable while
 *      dragging out a selection with the mouse.  The index "mark" may be
 *      used to refer to the anchor element.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName selection mark entryName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionMarkOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;

    if (GetEntryFromObj(interp, viewPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (viewPtr->sel.anchorPtr == NULL) {
#ifdef notdef
        Tcl_AppendResult(interp, "selection anchor must be set first", 
                 (char *)NULL);
        return TCL_ERROR;
#else
        return TCL_OK;
#endif
    }
    if (viewPtr->sel.markPtr != entryPtr) {
        Blt_ChainLink link, next;

        /* Deselect entry from the list all the way back to the anchor. */
        for (link = Blt_Chain_LastLink(viewPtr->sel.list); link != NULL; 
             link = next) {
            Entry *selectPtr;

            next = Blt_Chain_PrevLink(link);
            selectPtr = Blt_Chain_GetValue(link);
            if (selectPtr == viewPtr->sel.anchorPtr) {
                break;
            }
            DeselectEntry(viewPtr, selectPtr);
        }
        viewPtr->sel.flags &= ~SELECT_MASK;
        viewPtr->sel.flags |= SELECT_SET;
        SelectRange(viewPtr, viewPtr->sel.anchorPtr, entryPtr);
        Tcl_SetObjResult(interp, NodeToObj(entryPtr->node));
        viewPtr->sel.markPtr = entryPtr;

        EventuallyRedraw(viewPtr);
        if (viewPtr->sel.cmdObjPtr != NULL) {
            EventuallyInvokeSelectCmd(viewPtr);
        }
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
 *      A standard TCL result.  interp->result will contain a boolean
 *      string indicating if there is a selection.
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
    TreeView *viewPtr = clientData;
    int bool;

    bool = (Blt_Chain_GetLength(viewPtr->sel.list) > 0);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
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
 *      pathName selection set firstEntry ?lastEntry?
 *      pathName selection toggle firstEntry ?lastEntry?
 *      pathName selection unset firstEntry ?lastEntry?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    char *string;

    viewPtr->sel.flags &= ~SELECT_MASK;
    UpdateView(viewPtr);
    string = Tcl_GetString(objv[2]);
    switch (string[0]) {
    case 's':
        viewPtr->sel.flags |= SELECT_SET;
        break;
    case 'c':
        viewPtr->sel.flags |= SELECT_CLEAR;
        break;
    case 't':
        viewPtr->sel.flags |= SELECT_TOGGLE;
        break;
    }
    if (objc > 4) {
        Entry *firstPtr, *lastPtr;

        if (GetEntryFromObj(interp, viewPtr, objv[3], &firstPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (firstPtr == NULL) {
            return TCL_OK;              /* Didn't pick an entry. */
        }
        if ((firstPtr->flags & HIDDEN) && 
            (!(viewPtr->sel.flags & SELECT_CLEAR))) {
            if (objc > 4) {
                Tcl_AppendResult(interp, "can't select hidden node \"", 
                        Tcl_GetString(objv[3]), "\"", (char *)NULL);
                return TCL_ERROR;
            } else {
                return TCL_OK;
            }
        }
        lastPtr = firstPtr;
        if (objc > 4) {
            if (GetEntry(interp, viewPtr, objv[4], &lastPtr) != TCL_OK) {
                return TCL_ERROR;
            }
            if ((lastPtr->flags & HIDDEN) && 
                (!(viewPtr->sel.flags & SELECT_CLEAR))) {
                Tcl_AppendResult(interp, "can't select hidden node \"", 
                        Tcl_GetString(objv[4]), "\"", (char *)NULL);
                return TCL_ERROR;
            }
        }
        if (firstPtr == lastPtr) {
            SelectEntryApplyProc(viewPtr, firstPtr);
        } else {
            SelectRange(viewPtr, firstPtr, lastPtr);
        }
        /* Set both the anchor and the mark. Indicates that a single entry is
         * selected. */
        if (viewPtr->sel.anchorPtr == NULL) {
            viewPtr->sel.anchorPtr = firstPtr;
        }
    } else {
        Entry *entryPtr;
        EntryIterator iter;

        if (GetEntryIterator(interp, viewPtr, objv[3], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
             entryPtr = NextTaggedEntry(&iter)) {
            if ((entryPtr->flags & HIDDEN) && 
                ((viewPtr->sel.flags & SELECT_CLEAR) == 0)) {
                continue;
            }
            SelectEntryApplyProc(viewPtr, entryPtr);
        }
        /* Set both the anchor and the mark. Indicates that a single entry is
         * selected. */
        if (viewPtr->sel.anchorPtr == NULL) {
            viewPtr->sel.anchorPtr = entryPtr;
        }
    }
    if (viewPtr->sel.flags & SELECT_EXPORT) {
        Tk_OwnSelection(viewPtr->tkwin, XA_PRIMARY, LostSelection, viewPtr);
    }
    EventuallyRedraw(viewPtr);
    if (viewPtr->sel.cmdObjPtr != NULL) {
        EventuallyInvokeSelectCmd(viewPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionOp --
 *
 *      This procedure handles the individual options for text selections.
 *      The selected text is designated by start and end indices into the
 *      text pool.  The selected segment has both a anchored and unanchored
 *      ends.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec selectionOps[] =
{
    {"anchor",   1, SelectionAnchorOp,   4, 4, "entryName",},
    {"clear",    5, SelectionSetOp,      4, 5, "firstEntry ?lastEntry?",},
    {"clearall", 6, SelectionClearallOp, 3, 3, "",},
    {"export",   1, SelectionExportOp,   3, 3, "",},
    {"includes", 1, SelectionIncludesOp, 4, 4, "entryName",},
    {"mark",     1, SelectionMarkOp,     4, 4, "entryName",},
    {"present",  1, SelectionPresentOp,  3, 3, "",},
    {"set",      1, SelectionSetOp,      4, 5, "firstEntry ?lastEntry?",},
    {"toggle",   1, SelectionSetOp,      4, 5, "firstEntry ?lastEntry?",},
};
static int numSelectionOps = sizeof(selectionOps) / sizeof(Blt_OpSpec);

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


static int
SortAutoOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    if (objc == 4) {
        int bool;
        int isAuto;

        isAuto = ((viewPtr->flags & TV_SORT_AUTO) != 0);
        if (Tcl_GetBooleanFromObj(interp, objv[3], &bool) != TCL_OK) {
            return TCL_ERROR;
        }
        if (isAuto != bool) {
            viewPtr->flags |= (LAYOUT_PENDING | SORT_PENDING | RESORT);
            EventuallyRedraw(viewPtr);
        }
        if (bool) {
            viewPtr->flags |= TV_SORT_AUTO;
        } else {
            viewPtr->flags &= ~TV_SORT_AUTO;
        }
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),(viewPtr->flags & TV_SORT_AUTO));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortCgetOp --
 *
 *      pathName sort cget option
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SortCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, sortSpecs, 
        (char *)viewPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * SortConfigureOp --
 *
 *      This procedure is called to process a list of configuration options
 *      database, in order to reconfigure the one of more entries in the
 *      widget.
 *
 *        .h sort configure option value
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for viewPtr; old resources get freed, if there were
 *      any.  The hypertext is redisplayed.
 *
 *      pathName sort configure ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
static int
SortConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Tcl_Obj *oldCmdPtr;
    Column *oldColumn;

    if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, sortSpecs, 
                (char *)viewPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, sortSpecs, 
                (char *)viewPtr, objv[3], 0);
    }
    oldColumn = viewPtr->sort.markPtr;
    oldCmdPtr = viewPtr->sort.cmdObjPtr;
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, sortSpecs, 
        objc - 3, objv + 3, (char *)viewPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((oldColumn != viewPtr->sort.markPtr)|| 
        (oldCmdPtr != viewPtr->sort.cmdObjPtr)) {
        viewPtr->flags &= ~SORTED;
        viewPtr->flags |= (LAYOUT_PENDING | RESORT);
    } 
    if (viewPtr->flags & TV_SORT_AUTO) {
        viewPtr->flags |= SORT_PENDING;
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}


/*ARGSUSED*/
/* .
 * SortChildren --
 *
 *      Sort the tree node 
 *
 *      pathName sort children ?entryName  ...?
 */
static int
SortChildrenOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    int i;

    treeViewInstance = viewPtr;
    for (i = 3; i < objc; i++) {
        Entry *entryPtr;
        EntryIterator iter;

        if (GetEntryIterator(interp, viewPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
             entryPtr = NextTaggedEntry(&iter)) {
            SortChildren(viewPtr, entryPtr);
        }
    }
    viewPtr->flags |= (LAYOUT_PENDING | UPDATE);
    viewPtr->flags &= ~(SORT_PENDING | RESORT);
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortListOp
 *
 *      Sorts the flatten array of entries.
 *
 *      pathName sort list entryName columnName
 *
 *---------------------------------------------------------------------------
 */
static int
SortListOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr, *childPtr;
    long i, numChildren, count;
    Entry **entries;
    Tcl_Obj *listObjPtr;

    if (GetEntry(interp, viewPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    numChildren = Blt_Tree_NodeDegree(entryPtr->node);
    if (numChildren < 2) {
        return TCL_OK;
    }
    entries = Blt_Malloc((numChildren) * sizeof(Entry *));
    if (entries == NULL) {
        Tcl_AppendResult(interp, "can't allocate sorting array.", (char *)NULL);
        return TCL_ERROR;               /* Out of memory. */
    }
    count = 0;
    for (childPtr = FirstChild(entryPtr, HIDDEN); childPtr != NULL; 
         childPtr = NextSibling(childPtr, HIDDEN)) {
        entries[count] = childPtr;
        count++;
    }
    treeViewInstance = viewPtr;
    qsort(entries, count, sizeof(Entry *), CompareEntries);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (i = 0; i < count; i++) {
        long inode;
        Entry *entryPtr;
        
        entryPtr = entries[i];
        inode = Blt_Tree_NodeId(entryPtr->node);
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewLongObj(inode));
    }
    Blt_Free(entries);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortOnceOp
 *
 *      Sorts the tree.
 *
 *      pathName sort once
 *
 *---------------------------------------------------------------------------
 */
static int
SortOnceOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;

    if (viewPtr->flags & FLAT) {
        SortFlatView(viewPtr);
    } else {
        SortTreeView(viewPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortOp --
 *
 *      Comparison routine (used by qsort) to sort a chain of subnodes.  A
 *      simple string comparison is performed on each node name.
 *
 *      pathName sort auto
 *      pathName sort once root -recurse root
 *
 * Results:
 *      1 is the first is greater, -1 is the second is greater, 0 if equal.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec sortOps[] =
{
    {"auto",      1, SortAutoOp,      3, 4, "?boolean?",},
    {"cget",      2, SortCgetOp,      4, 4, "option",},
    {"children",  2, SortChildrenOp,  3, 0, "node...",},
    {"configure", 2, SortConfigureOp, 3, 0, "?option value?...",},
    {"list",      1, SortListOp,      4, 4, "node",},
    {"once",      1, SortOnceOp,      3, 3, "",},
};
static int numSortOps = sizeof(sortOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
SortOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numSortOps, sortOps, BLT_OP_ARG2, objc, 
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
 * StyleActivateOp --
 *
 *      Turns on highlighting for a particular style.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 *      pathName style activate entryName columnName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Cell *oldCellPtr;
    Column *colPtr;
    Entry *entryPtr;
    Cell *cellPtr;

    oldCellPtr = viewPtr->activeCellPtr;
    if (GetEntry(interp, viewPtr, objv[3], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetColumn(interp, viewPtr, objv[4], &colPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((colPtr == NULL) || (entryPtr == NULL)) {
        cellPtr = NULL;
    } else {
        cellPtr = GetCell(entryPtr, colPtr);
    }
    if (cellPtr != oldCellPtr) {
        if (oldCellPtr != NULL) {
            /* Deactivate old cell */
            DisplayCell(viewPtr, oldCellPtr);
        }
        if (cellPtr == NULL) {
            /* Mark as deactivate */
            viewPtr->activePtr = NULL;
            viewPtr->colActivePtr = NULL;
            viewPtr->activeCellPtr = NULL;
        } else {
            /* Activate new cell. */
            viewPtr->activePtr = entryPtr;
            viewPtr->colActivePtr = colPtr;
            viewPtr->activeCellPtr = cellPtr;
            DisplayCell(viewPtr, cellPtr);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleCellsOp --
 *
 *        pathName style cells styleName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleCellsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    TreeView *viewPtr = clientData;
    Tcl_Obj *listObjPtr;
    CellStyle *stylePtr;

    stylePtr = FindStyle(interp, viewPtr, Tcl_GetString(objv[3]));
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    viewPtr = stylePtr->viewPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hPtr = Blt_FirstHashEntry(&viewPtr->entryTable, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        Entry *entryPtr;
        Cell *cellPtr;

        entryPtr = Blt_GetHashValue(hPtr);
        for (cellPtr = entryPtr->cells; cellPtr != NULL; 
             cellPtr = cellPtr->nextPtr) {
            CellStyle *currentPtr;

            currentPtr = GetCurrentStyle(viewPtr, cellPtr->colPtr, cellPtr);
            if (currentPtr == stylePtr) {
                Tcl_Obj *objPtr;

                objPtr = CellToIndexObj(interp, cellPtr);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleCgetOp --
 *
 *        pathName style cget styleName option
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    CellStyle *stylePtr;

    stylePtr = FindStyle(interp, viewPtr, Tcl_GetString(objv[3]));
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, 
        stylePtr->classPtr->specs, (char *)stylePtr, objv[4], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleCheckBoxOp --
 *
 *        pathName style checkbox styleName ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleCheckBoxOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    CellStyle *stylePtr;

    stylePtr = Blt_TreeView_CreateStyle(interp, viewPtr, STYLE_CHECKBOX, 
        Tcl_GetString(objv[3]), objc - 4, objv + 4);
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    stylePtr->link = Blt_Chain_Append(viewPtr->userStyles, stylePtr);
    ConfigureStyle(viewPtr, stylePtr);
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleComboBoxOp --
 *
 *        pathName style combobox styleName ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleComboBoxOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    CellStyle *stylePtr;

    stylePtr = Blt_TreeView_CreateStyle(interp, viewPtr, STYLE_COMBOBOX, 
        Tcl_GetString(objv[3]), objc - 4, objv + 4);
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    stylePtr->link = Blt_Chain_Append(viewPtr->userStyles, stylePtr);
    ConfigureStyle(viewPtr, stylePtr);
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleConfigureOp --
 *
 *      This procedure is called to process a list of configuration options
 *      database, in order to reconfigure a style.
 *
 *        pathName style configure "styleName" option value
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for stylePtr; old resources get freed, if there were
 *      any.
 *
 *      pathName style configure styleName ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
static int
StyleConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
                 Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    CellStyle *stylePtr;

    stylePtr = FindStyle(interp, viewPtr, Tcl_GetString(objv[3]));
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, 
            stylePtr->classPtr->specs, (char *)stylePtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 5) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, 
                stylePtr->classPtr->specs, (char *)stylePtr, objv[5], 0);
    }
    iconOption.clientData = viewPtr;
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, 
        stylePtr->classPtr->specs, objc - 4, objv + 4, (char *)stylePtr, 
        BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    (*stylePtr->classPtr->configProc)(stylePtr);
    stylePtr->flags |= STYLE_DIRTY;
    viewPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * StyleCreateOp --
 *
 *        pathName style create styleType styleName ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleCreateOp(TreeView *viewPtr, Tcl_Interp *interp, int objc, 
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
    } else if ((c == 'r') && (strncmp(string, "radiobutton", length) == 0)) {
        type = STYLE_RADIOBUTTON;
    } else {
        Tcl_AppendResult(interp, "unknown style type \"", string, 
        "\": should be textbox, checkbox, combobox, radiobutton, or imagebox.", 
                (char *)NULL);
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[4]);
    iconOption.clientData = viewPtr;
    stylePtr = Blt_TreeView_CreateStyle(interp, viewPtr, type, string, 
        objc - 5, objv + 5);
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    stylePtr->link = Blt_Chain_Append(viewPtr->userStyles, stylePtr);
    ConfigureStyle(viewPtr, stylePtr);
    Tcl_SetObjResult(interp, objv[4]);
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleDeactivateOp --
 *
 *      Turns on highlighting for *all* styles
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 *      pathName style deactivate 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Cell *oldCellPtr;

    oldCellPtr = viewPtr->activeCellPtr;
    viewPtr->activeCellPtr = NULL;
    if ((oldCellPtr != NULL)  && (viewPtr->activePtr != NULL)) {
        DisplayCell(viewPtr, oldCellPtr);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * StyleForgetOp --
 *
 *      Eliminates zero or more style names.  A style still may be in use
 *      after its name has been officially removed.  Only its hash table
 *      entry is removed.  The style itself remains until its reference
 *      count returns to zero (i.e. no one else is using it).
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 *        pathName style forget ?styleName ...?
 * 
 *---------------------------------------------------------------------------
 */
static int
StyleForgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    CellStyle *stylePtr;
    int i;

    for (i = 3; i < objc; i++) {
        stylePtr = FindStyle(interp, viewPtr, Tcl_GetString(objv[i]));
        if (stylePtr == NULL) {
            return TCL_ERROR;
        }
        if (viewPtr->stylePtr == stylePtr) {
            continue;                   /* Can't delete the default
                                         * style. */
        }
        /* 
         * Removing the style from the hash tables frees up the style name
         * again.  The style itself may not be removed until it's been
         * released by everything using it.
         */
        if (stylePtr->hashPtr != NULL) {
            Blt_DeleteHashEntry(&viewPtr->styleTable, stylePtr->hashPtr);
            stylePtr->hashPtr = NULL;
        } 
        FreeStyle(stylePtr);
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleHighlightOp --
 *
 *      Turns on/off highlighting for a particular style.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 *        pathName style highlight styleName on|off
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleHighlightOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    CellStyle *stylePtr;
    int bool, oldBool;

    stylePtr = FindStyle(interp, viewPtr, Tcl_GetString(objv[3]));
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_GetBooleanFromObj(interp, objv[4], &bool) != TCL_OK) {
        return TCL_ERROR;
    }
    oldBool = ((stylePtr->flags & HIGHLIGHT) != 0);
    if (oldBool != bool) {
        if (bool) {
            stylePtr->flags |= HIGHLIGHT;
        } else {
            stylePtr->flags &= ~HIGHLIGHT;
        }
        EventuallyRedraw(viewPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleNamesOp --
 *
 *      Lists the names of all the current styles in the treeview widget.
 *
 * Results:
 *      Always TCL_OK.
 *
 *      pathName style names ?pattern ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hPtr = Blt_FirstHashEntry(&viewPtr->styleTable, &cursor); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&cursor)) {
        CellStyle *stylePtr;
        int found;
        int i;

        stylePtr = Blt_GetHashValue(hPtr);
        found = FALSE;
        for (i = 3; i < objc; i++) {
            const char *pattern;

            pattern = Tcl_GetString(objv[i]);
            found = Tcl_StringMatch(stylePtr->name, pattern);
            if (found) {
                break;
            }
        }
        if ((objc == 2) || (found)) {
            Tcl_Obj *objPtr;

            objPtr = Tcl_NewStringObj(stylePtr->name, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * StyleGetOp --
 *
 *      pathName style get cellName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleGetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    CellStyle *stylePtr;
    Cell *cellPtr;

    if (GetCellFromObj(interp, viewPtr, objv[3], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    stylePtr = GetCurrentStyle(viewPtr, cellPtr->colPtr, cellPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), stylePtr->name, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleSetOp --
 *
 *      Sets a style for a given key for all the ids given.
 *
 *        pathName style set styleName key node...
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 *      pathName style set styleName fieldName ?entryName ...?
 *
 *---------------------------------------------------------------------------
 */
static int
StyleSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Blt_TreeKey key;
    CellStyle *stylePtr;
    int i;

    stylePtr = FindStyle(interp, viewPtr, Tcl_GetString(objv[3]));
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    key = Blt_Tree_GetKey(viewPtr->tree, Tcl_GetString(objv[4]));
    for (i = 5; i < objc; i++) {
        Entry *entryPtr;
        EntryIterator iter;

        if (GetEntryIterator(interp, viewPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
             entryPtr = NextTaggedEntry(&iter)) {
            Cell *cellPtr;

            for (cellPtr = entryPtr->cells; cellPtr != NULL; 
                 cellPtr = cellPtr->nextPtr) {
                if (cellPtr->colPtr->key == key) {
                    CellStyle *oldStylePtr;

                    stylePtr->refCount++;
                    oldStylePtr = cellPtr->stylePtr;
                    cellPtr->stylePtr = stylePtr;
                    if (oldStylePtr != NULL) {
                        FreeStyle(oldStylePtr);
                    }
                    break;
                }
            }
        }
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleTextBoxOp --
 *
 *        pathName style text styleName ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleTextBoxOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    CellStyle *stylePtr;

    stylePtr = Blt_TreeView_CreateStyle(interp, viewPtr, STYLE_TEXTBOX, 
        Tcl_GetString(objv[3]), objc - 4, objv + 4);
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    stylePtr->link = Blt_Chain_Append(viewPtr->userStyles, stylePtr);
    ConfigureStyle(viewPtr, stylePtr);
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleTypeOp --
 *
 *      Returns the type of the style.
 *
 * Results:
 *      A standard TCL result.  If the styleName exists, the type is 
 *      returned as a string. If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 *        pathName style type styleName
 *
 *---------------------------------------------------------------------------
 */
static int
StyleTypeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    CellStyle *stylePtr;

    stylePtr = FindStyle(interp, viewPtr, Tcl_GetString(objv[3]));
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), stylePtr->classPtr->className, 
        -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleUnsetOp --
 *
 *      Removes a style for a given key for all the ids given.  The cell's
 *      style is returned to its default state.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 *        pathName style unset styleName columnName ?entryName ...?
 *
 *---------------------------------------------------------------------------
 */
static int
StyleUnsetOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Blt_TreeKey key;
    CellStyle *stylePtr;
    int i;

    stylePtr = FindStyle(interp, viewPtr, Tcl_GetString(objv[3]));
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    key = Blt_Tree_GetKey(viewPtr->tree, Tcl_GetString(objv[4]));
    for (i = 5; i < objc; i++) {
        EntryIterator iter;
        Entry *entryPtr;

        if (GetEntryIterator(interp, viewPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
             entryPtr = NextTaggedEntry(&iter)) {
            Cell *cellPtr;

            for (cellPtr = entryPtr->cells; cellPtr != NULL; 
                 cellPtr = cellPtr->nextPtr) {
                if (cellPtr->colPtr->key == key) {
                    if (cellPtr->stylePtr != NULL) {
                        FreeStyle(cellPtr->stylePtr);
                        cellPtr->stylePtr = NULL;
                    }
                    break;
                }
            }
        }
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleOp --
 *
 *      pathName style activate $node $column
 *      pathName style activate 
 *      pathName style cget "highlight" -foreground
 *      pathName style configure "highlight" -fg blue -bg green
 *      pathName style checkbox "highlight"
 *      pathName style deactivate
 *      pathName style highlight "highlight" on|off
 *      pathName style combobox "highlight"
 *      pathName style text "highlight"
 *      pathName style forget "highlight"
 *      pathName style get "mtime" $node
 *      pathName style names
 *      pathName style set "mtime" "highlight" all
 *      pathName style unset "mtime" all
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec styleOps[] = {
    {"activate",    1, StyleActivateOp,    5, 5, "entry column",},
    {"cells",       2, StyleCellsOp,       4, 4, "styleName",}, 
    {"cget",        2, StyleCgetOp,        5, 5, "styleName option",},
    {"checkbox",    2, StyleCheckBoxOp,    4, 0, "styleName options...",},
    {"combobox",    3, StyleComboBoxOp,    4, 0, "styleName options...",},
    {"configure",   3, StyleConfigureOp,   4, 0, "styleName options...",},
    {"create",      2, StyleCreateOp,      5, 0, "styleName type options...",},
    {"deactivate",  1, StyleDeactivateOp,  3, 3, "",},
    {"forget",      1, StyleForgetOp,      3, 0, "styleName...",},
    {"get",         1, StyleGetOp,         4, 4, "cell",},
    {"highlight",   1, StyleHighlightOp,   5, 5, "styleName boolean",},
    {"names",       1, StyleNamesOp,       3, 3, "",}, 
    {"set",         1, StyleSetOp,         6, 6, "key styleName entryName...",},
    {"textbox",     2, StyleTextBoxOp,     4, 0, "styleName options...",},
    {"type",        2, StyleTypeOp,        4, 4, "styleName",}, 
    {"unset",       1, StyleUnsetOp,       5, 5, "key entryName",},
};

static int numStyleOps = sizeof(styleOps) / sizeof(Blt_OpSpec);

static int
StyleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    int result;
    Tcl_ObjCmdProc *proc;

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
 * TagForgetOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagForgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    int i;

    for (i = 3; i < objc; i++) {
        Blt_Tree_ForgetTag(viewPtr->tree, Tcl_GetString(objv[i]));
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagNamesOp --
 *
 *---------------------------------------------------------------------------
 */
static int
TagNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    objPtr = Tcl_NewStringObj("all", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    if (objc == 3) {
        Blt_HashEntry *hPtr;
        Blt_HashSearch cursor;
        Blt_TreeTagEntry *tPtr;

        objPtr = Tcl_NewStringObj("root", -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        for (hPtr = Blt_Tree_FirstTag(viewPtr->tree, &cursor); hPtr != NULL;
             hPtr = Blt_NextHashEntry(&cursor)) {
            tPtr = Blt_GetHashValue(hPtr);
            objPtr = Tcl_NewStringObj(tPtr->tagName, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    } else {
        int i;

        for (i = 3; i < objc; i++) {
            Blt_Chain tags;
            Blt_ChainLink link;
            Entry *entryPtr;

            if (GetEntry(interp, viewPtr, objv[i], &entryPtr) != TCL_OK) {
                return TCL_ERROR;
            }
            tags = Blt_Chain_Create();
            AddEntryTags(viewPtr, entryPtr, tags);
            for (link = Blt_Chain_FirstLink(tags); link != NULL; 
                 link = Blt_Chain_NextLink(link)) {
                objPtr = Tcl_NewStringObj(Blt_Chain_GetValue(link), -1);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
            Blt_Chain_Destroy(tags);
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagNodesOp --
 *
 *---------------------------------------------------------------------------
 */
static int
TagNodesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Blt_HashTable nodeTable;
    int i;

    Blt_InitHashTable(&nodeTable, BLT_ONE_WORD_KEYS);
    for (i = 3; i < objc; i++) {
        EntryIterator iter;
        Entry *entryPtr;

        if (GetEntryIterator(interp, viewPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
             entryPtr = NextTaggedEntry(&iter)) {
            int isNew;

            Blt_CreateHashEntry(&nodeTable, (char *)entryPtr->node, &isNew);
        }
    }
    {
        Blt_HashEntry *hPtr;
        Blt_HashSearch cursor;
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
        for (hPtr = Blt_FirstHashEntry(&nodeTable, &cursor); hPtr != NULL; 
             hPtr = Blt_NextHashEntry(&cursor)) {
            Blt_TreeNode node;
            Tcl_Obj *objPtr;
            
            node = (Blt_TreeNode)Blt_GetHashKey(&nodeTable, hPtr);
            objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(node));
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_SetObjResult(interp, listObjPtr);
    }
    Blt_DeleteHashTable(&nodeTable);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagAddOp --
 *
 *      pathName tag add tag nodeName
 *---------------------------------------------------------------------------
 */
static int
TagAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;
    int i;
    char *tagName;
    EntryIterator iter;

    tagName = Tcl_GetString(objv[3]);
    viewPtr->fromPtr = NULL;
    if (strcmp(tagName, "root") == 0) {
        Tcl_AppendResult(interp, "can't add reserved tag \"", tagName, "\"", 
                (char *)NULL);
        return TCL_ERROR;
    }
    if (isdigit(UCHAR(tagName[0]))) {
        long nodeId;
        
        if (Blt_GetLongFromObj(NULL, objv[3], &nodeId) == TCL_OK) {
            Tcl_AppendResult(viewPtr->interp, "invalid tag \"", tagName, 
                             "\": can't be a number.", (char *)NULL);
            return TCL_ERROR;
        } 
    }
    if (tagName[0] == '@') {
        Tcl_AppendResult(viewPtr->interp, "invalid tag \"", tagName, 
                "\": can't start with \"@\"", (char *)NULL);
        return TCL_ERROR;
    } 
    if (GetEntryFromSpecialId(viewPtr, objv[3], &entryPtr) == TCL_OK) {
        Tcl_AppendResult(interp, "invalid tag \"", tagName, 
                 "\": is a special id", (char *)NULL);
        return TCL_ERROR;
    }
    for (i = 4; i < objc; i++) {
        if (GetEntryIterator(interp, viewPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
             entryPtr = NextTaggedEntry(&iter)) {
            if (AddTag(interp, viewPtr, entryPtr->node, objv[3]) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagDeleteOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    char *tagName;
    Blt_HashTable *tablePtr;

    tagName = Tcl_GetString(objv[3]);
    tablePtr = Blt_Tree_TagHashTable(viewPtr->tree, tagName);
    if (tablePtr != NULL) {
        int i;

        for (i = 4; i < objc; i++) {
            Entry *entryPtr;
            EntryIterator iter;

            if (GetEntryIterator(interp, viewPtr, objv[i], &iter)!= TCL_OK) {
                return TCL_ERROR;
            }
            for (entryPtr = FirstTaggedEntry(&iter); 
                entryPtr != NULL; 
                entryPtr = NextTaggedEntry(&iter)) {
                Blt_HashEntry *hPtr;

                hPtr = Blt_FindHashEntry(tablePtr, (char *)entryPtr->node);
                if (hPtr != NULL) {
                    Blt_DeleteHashEntry(tablePtr, hPtr);
                }
           }
       }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagOp --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec tagOps[] = {
    {"add",    1, TagAddOp,    5, 0, "tag id...",},
    {"delete", 1, TagDeleteOp, 5, 0, "tag id...",},
    {"forget", 1, TagForgetOp, 4, 0, "tag...",},
    {"names",  2, TagNamesOp,  3, 0, "?id...?",}, 
    {"nodes",  2, TagNodesOp,  4, 0, "tag ?tag...?",},
};

static int numTagOps = sizeof(tagOps) / sizeof(Blt_OpSpec);

static int
TagOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int result;
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numTagOps, tagOps, BLT_OP_ARG2, objc, objv, 
        0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc)(clientData, interp, objc, objv);
    return result;
}

/*ARGSUSED*/
static int
ToggleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Entry *entryPtr;
    EntryIterator iter;
    int result;

    if (GetEntryIterator(interp, viewPtr, objv[2], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    result = TCL_OK;                    /* Suppress compiler warning. */
    for (entryPtr = FirstTaggedEntry(&iter); entryPtr != NULL; 
         entryPtr = NextTaggedEntry(&iter)) {
        if (entryPtr == NULL) {
            return TCL_OK;
        }
        if (IsClosed(entryPtr)) {
            result = OpenEntry(viewPtr, entryPtr);
        } else {
            PruneSelection(viewPtr, viewPtr->focusPtr);
            if ((viewPtr->focusPtr != NULL) && 
                (Blt_Tree_IsAncestor(entryPtr->node, viewPtr->focusPtr->node))){
                viewPtr->focusPtr = entryPtr;
                Blt_SetFocusItem(viewPtr->bindTable, entryPtr, ITEM_ENTRY);
            }
            if ((viewPtr->sel.anchorPtr != NULL) &&
                (Blt_Tree_IsAncestor(entryPtr->node, 
                        viewPtr->sel.anchorPtr->node))) {
                viewPtr->sel.anchorPtr = NULL;
            }
            result = CloseEntry(viewPtr, entryPtr);
        }
    }
    viewPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(viewPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TypeOp --
 *
 *      Returns the type of the cell.
 *
 *      pathName type cellSpec
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TypeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    Tcl_Obj *objPtr;
    Cell *cellPtr;
    CellStyle *stylePtr;
    
    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cellPtr == NULL) {
        return TCL_OK;
    }
    stylePtr = GetCurrentStyle(viewPtr, cellPtr->colPtr, cellPtr);
    objPtr = Tcl_NewStringObj(stylePtr->classPtr->type, -1);
    Tcl_SetObjResult(interp, objPtr);
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
    TreeView *viewPtr = clientData;
    int state;

    if (objc == 3) {
        if (Tcl_GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
            return TCL_ERROR;
        }
        if (state) {
            viewPtr->flags &= ~DONT_UPDATE;
            viewPtr->flags |= LAYOUT_PENDING;
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
 *        .view writable $cell
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
WritableOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Cell *cellPtr;
    TreeView *viewPtr = clientData;
    int state;

    if (GetCellFromObj(interp, viewPtr, objv[2], &cellPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    state = FALSE;
    if (cellPtr != NULL) {
        CellStyle *stylePtr;

        stylePtr = GetCurrentStyle(viewPtr, cellPtr->colPtr, cellPtr);
        state = (stylePtr->flags & EDITABLE);
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

static int
XViewOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    int w;

    w = VPORTWIDTH(viewPtr);
    if (objc == 2) {
        double first, last;
        Tcl_Obj *listObjPtr;

        /* Report first and last fractions */
        first = (double)viewPtr->xOffset / viewPtr->worldWidth;
        first = FCLAMP(first);
        last = (double)(viewPtr->xOffset + w) / viewPtr->worldWidth;
        last = FCLAMP(last);
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(first));
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(last));
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &viewPtr->xOffset,
            viewPtr->worldWidth, w, viewPtr->xScrollUnits, viewPtr->scrollMode) 
            != TCL_OK) {
        return TCL_ERROR;
    }
    viewPtr->flags |= SCROLLX | VISIBILITY;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

static int
YViewOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    TreeView *viewPtr = clientData;
    int h;

    h = VPORTHEIGHT(viewPtr);
    if (objc == 2) {
        double first, last;
        Tcl_Obj *listObjPtr;

        /* Report first and last fractions */
        first = (double)viewPtr->yOffset / viewPtr->worldHeight;
        first = FCLAMP(first);
        last = (double)(viewPtr->yOffset + h) / viewPtr->worldHeight;
        last = FCLAMP(last);
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(first));
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(last));
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &viewPtr->yOffset,
            viewPtr->worldHeight, h, viewPtr->yScrollUnits, viewPtr->scrollMode)
        != TCL_OK) {
        return TCL_ERROR;
    }
    viewPtr->flags |= SCROLLY | VISIBILITY;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeViewInstCmdProc --
 *
 *      This procedure is invoked to process commands on behalf of the
 *      treeview widget.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec viewOps[] =
{
    {"activate",     1, ActivateOp,      3, 3, "cell"},
    {"bbox",         2, BboxOp,          3, 0, "entryName ?entryName ...?",}, 
    {"bind",         2, BindOp,          3, 5, "tagName ?sequence command?",}, 
    {"button",       2, ButtonOp,        2, 0, "args",},
    {"cell",         2, CellOp,          2, 0, "args",}, 
    {"cget",         2, CgetOp,          3, 3, "option",}, 
    {"chroot",       2, ChrootOp,        2, 3, "entryName",}, 
    {"close",        2, CloseOp,         3, 0, "entryName ?switches ...?",}, 
    {"column",       3, ColumnOp,        2, 0, "oper args",}, 
    {"configure",    3, ConfigureOp,     2, 0, "?option value?...",},
    {"curselection", 2, CurselectionOp,  2, 2, "",},
    {"delete",       3, DeleteOp,        2, 0, "?entryName ...?",}, 
    {"entry",        2, EntryOp,         2, 0, "oper args",},
    {"find",         2, FindOp,          2, 0, "?flags...? ?first last?",}, 
    {"focus",        2, FocusOp,         3, 3, "entryName",}, 
    {"get",          1, GetOp,           2, 0, "?-full? entryName ?entryName...?",},
    {"hide",         1, HideOp,          2, 0, "?-exact? ?-glob? ?-regexp? ?-nonmatching? ?-name string? ?-full string? ?-data string? ?--? ?entryName...?",},
    {"index",        3, IndexOp,         3, 0, "entryName ?switches ...",},
    {"insert",       3, InsertOp,        3, 0, "fullPath ?switches...?",},
    {"invoke",       3, InvokeOp,        3, 3, "entryName",}, 
    {"move",         1, MoveOp,          5, 5, 
        "entryName into|before|after destName",},
    {"nearest",      1, NearestOp,       4, 5, "x y ?varName?",}, 
    {"open",         1, OpenOp,          3, 0, "entryName ?switches ...?",}, 
    {"range",        1, RangeOp,         4, 5, "?-open? firstEntry lastEntry",},
    {"scan",         2, ScanOp,          5, 5, "dragto|mark x y",},
    {"see",          3, SeeOp,           3, 0, "?-anchor anchor? entryName",},
    {"selection",    3, SelectionOp,     2, 0, "oper args",},
    {"show",         2, ShowOp,          2, 0, "?-exact? ?-glob? ?-regexp? ?-nonmatching? ?-name string? ?-full string? ?-data string? ?--? ?entryName...?",},
    {"sort",         2, SortOp,          2, 0, "args",},
    {"style",        2, StyleOp,         2, 0, "args",},
    {"tag",          2, TagOp,           2, 0, "oper args",},
    {"toggle",       2, ToggleOp,        3, 3, "entryName",},
    {"type",         2, TypeOp,          3, 3, "cell",},
    {"updates",      3, UpdatesOp,       2, 3, "?bool?",},
    {"writable",     1, WritableOp,      3, 3, "cell",},
    {"xview",        1, XViewOp,         2, 5, 
        "?moveto fract? ?scroll number what?",},
    {"yview",        1, YViewOp,         2, 5, 
        "?moveto fract? ?scroll number what?",},
};

static int numViewOps = sizeof(viewOps) / sizeof(Blt_OpSpec);

static int
TreeViewInstCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    TreeView *viewPtr = clientData;
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
 * TreeViewCmd --
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
TreeViewCmdProc(
    ClientData clientData,              /* Main window associated with
                                         * interpreter. */
    Tcl_Interp *interp,                 /* Current interpreter. */
    int objc,                           /* Number of arguments. */
    Tcl_Obj *const *objv)               /* Argument strings. */
{
    TreeView *viewPtr;
    Tcl_Obj *initObjv[2];
    char *string;
    int result;

    string = Tcl_GetString(objv[0]);
    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", string, 
                " pathName ?option value?...\"", (char *)NULL);
        return TCL_ERROR;
    }
    viewPtr = NewView(interp, objv[1]);
    if (viewPtr == NULL) {
        goto error;
    }

    /*
     * Invoke a procedure to initialize various bindings on treeview
     * entries.  If the procedure doesn't already exist, source it from
     * "$blt_library/bltTreeView.tcl".  We deferred sourcing the file until
     * now so that the variable $blt_library could be set within a script.
     */
    if (!Blt_CommandExists(interp, "::blt::TreeView::Initialize")) {
        if (Tcl_GlobalEval(interp, 
                "source [file join $blt_library bltTreeView.tcl]") != TCL_OK) {
            char info[200];

            Blt_FormatString(info, 200, "\n    (while loading bindings for %.50s)", 
                    Tcl_GetString(objv[0]));
            Tcl_AddErrorInfo(interp, info);
            goto error;
        }
    }
    /* 
     * Initialize the widget's configuration options here. The options need
     * to be set first, so that entry, column, and style components can use
     * them for their own GCs.
     */
    iconsOption.clientData = viewPtr;
    treeOption.clientData = viewPtr;
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, viewSpecs, 
        objc - 2, objv + 2, (char *)viewPtr, 0) != TCL_OK) {
        goto error;
    }
    if (Blt_ConfigureComponentFromObj(interp, viewPtr->tkwin, "button", 
        "Button", buttonSpecs, 0, (Tcl_Obj **)NULL, (char *)viewPtr,
        0) != TCL_OK) {
        goto error;
    }

    /* 
     * Rebuild the widget's GC and other resources that are predicated by
     * the widget's configuration options.  Do the same for the default
     * column.
     */
    if (ConfigureTreeView(interp, viewPtr) != TCL_OK) {
        goto error;
    }
    cachedObjOption.clientData = viewPtr;
    iconOption.clientData = viewPtr;
    styleOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(viewPtr->interp, viewPtr->tkwin, 
        "treeView", "Column", columnSpecs, 0, (Tcl_Obj **)NULL, 
        (char *)&viewPtr->treeColumn, 0) != TCL_OK) {
        goto error;
    }
    ConfigureColumn(viewPtr, &viewPtr->treeColumn);
    ConfigureStyle(viewPtr, viewPtr->stylePtr);

    /*
     * Invoke a procedure to initialize various bindings on treeview
     * entries.  
     */
    initObjv[0] = Tcl_NewStringObj("::blt::TreeView::Initialize", -1);
    initObjv[1] = objv[1];
    Tcl_IncrRefCount(initObjv[0]);
    Tcl_IncrRefCount(initObjv[1]);
    result = Tcl_EvalObjv(interp, 2, initObjv, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(initObjv[1]);
    Tcl_DecrRefCount(initObjv[0]);
    if (result != TCL_OK) {
        goto error;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(viewPtr->tkwin), -1);
    return TCL_OK;
  error:
    if (viewPtr != NULL) {
        Tk_DestroyWindow(viewPtr->tkwin);
    }
    return TCL_ERROR;
}

int
Blt_TreeViewCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpecs[] = { 
        { "treeview", TreeViewCmdProc, },
    };

    return Blt_InitCmds(interp, "::blt", cmdSpecs, 1);
}

#endif /*NO_TREEVIEW*/

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltListView.c --
 *
 * This module implements a listview widget for the BLT toolkit.
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

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif  /* HAVE_LIMITS_H */

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "bltAlloc.h"
#include "bltChain.h"
#include "bltHash.h"
#include "bltBind.h"
#include "bltImage.h"
#include "bltPicture.h"
#include "bltFont.h"
#include "bltText.h"
#include "bltBg.h"
#include "bltPainter.h"
#include "bltDataTable.h"
#include "bltSwitch.h"
#include "bltOp.h"
#include "bltInitCmd.h"

static const char emptyString[] = "";

#define REDRAW_PENDING   (1<<0)         /* The widget needs to be redrawn. */
#define LAYOUT_PENDING   (1<<1)         /* The layout of widget needs to be
                                         * recomputed. */
#define SORT_PENDING     (1<<3)         /* The items in the need to be
                                         * sorted. */
#define FOCUS            (1<<4)         /* The widget currently has focus. */
#define SORTED           (1<<5)         /* The items are currently sorted. */
#define SCROLLX          (1<<6)         /* The widget needs to be scrolled in
                                         * the x direction. */
#define SCROLLY          (1<<7)         /* The widget needs to be scrolled in
                                         * the y direction. */
#define SCROLL_PENDING   (SCROLLX|SCROLLY)

#define RESTRICT_MIN     (1<<10)
#define RESTRICT_MAX     (1<<11)
#define RESTRICT_NONE    (0)

#define SELECT_SINGLE    (1<<12)        /* Single mode: Select only one item
                                         * at a time.*/
#define SELECT_MULTIPLE  (1<<13)        /* Multiple mode: Select one or more
                                         * items. */
#define SELECT_MODE_MASK (SELECT_MULTIPLE|SELECT_SINGLE)

#define SELECT_EXPORT    (1<<16)        /* Export the selection to X11. */
#define SELECT_ORDERED   (1<<17)        /* Indicates that the selection should
                                         * be set in the order that the items
                                         * were selected. */
#define SELECT_PENDING   (1<<18)        /* A "selection" command idle task is
                                         * pending.  */
#define SELECT_SET       (1<<19)        /* Select the item. */
#define SELECT_CLEAR     (1<<20)        /* Deselect the item.  */
#define SELECT_TOGGLE    (SELECT_SET | SELECT_CLEAR)
#define SELECT_MASK      (SELECT_SET | SELECT_CLEAR)

#define REBUILD_TABLE    (1<<21)

#define SORT_AUTO        (1<<26)        /* Automatically sort the items as
                                         * items are added or deleted. */
#define SORT_DECREASING  (1<<27)        /* Sort items in decreasing order.  */
#define SORT_DICTIONARY  (1<<28)        /* Sort the items in dictionary
                                         * order. */
#define SORT_BY_TYPE     (1<<29)        /* Sort the items by their type. */
#define SORT_BY_TEXT     (1<<30)        /* Sort the items by their name.  */
#define SORT_BY_MASK     (SORT_BY_TYPE|SORT_BY_TEXT)

#define VAR_FLAGS (TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS)

#define PIXMAPX(l, wx)  ((wx) - (l)->xOffset)
#define PIXMAPY(l, wy)  ((wy) - (l)->yOffset)

#define SCREENX(l, wx)  ((wx) - (l)->xOffset + (l)->inset)
#define SCREENY(l, wy)  ((wy) - (l)->yOffset + (l)->inset)

#define WORLDX(l, sx)   ((sx) - (l)->inset + (l)->xOffset)
#define WORLDY(l, sy)   ((sy) - (l)->inset + (l)->yOffset)

#define VPORTWIDTH(l)   \
    (Tk_Width((l)->tkwin) - 2 * (l)->inset)
#define VPORTHEIGHT(l) \
    (Tk_Height((l)->tkwin) - 2 * (l)->inset)

#define FCLAMP(x)       ((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))
#define CLAMP(x,min,max) ((((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x)))

#define ITEM_IPAD          5
#define ITEM_XPAD          0
#define ITEM_YPAD          0

#define ITEM_REDRAW       (1<<2)        /* Item needs to be redrawn. */
#define ITEM_HIDDEN       (1<<5)        /* The item is hidden. */

/* Item state. */
#define ITEM_NORMAL       (1<<8)        /* Draw item normally. */
#define ITEM_DISABLED     (1<<9)        /* Item is disabled. */
#define ITEM_STATE_MASK   ((ITEM_DISABLED)|(ITEM_NORMAL))

#define DEF_MAXWIDTH                "1i"
#define DEF_AUTO_SORT               "0"
#define DEF_BORDERWIDTH             "1"
#define DEF_CURSOR                  ((char *)NULL)
#define DEF_EXPORT_SELECTION        "1"
#define DEF_HEIGHT                  "0"
#define DEF_HIGHLIGHT_BACKGROUND    STD_NORMAL_BACKGROUND
#define DEF_HIGHLIGHT_COLOR         RGB_BLACK
#define DEF_HIGHLIGHT_WIDTH         "2"
#define DEF_ICON_VARIABLE           ((char *)NULL)
#define DEF_LAYOUTMODE              "column"
#define DEF_RELIEF                  "sunken"
#define DEF_SELECTMODE              "single"
#define DEF_SORT_DICTIONARY         "0"
#define DEF_SORT_COMMAND            ((char *)NULL)
#define DEF_SORT_DECREASING         "0"
#define DEF_SORT_SELECTION          "1"
#define DEF_SORT_TYPE               "text"
#define DEF_TAKEFOCUS               "1"
#define DEF_TEXTVARIABLE            ((char *)NULL)
#define DEF_WIDTH                   "0"
#define DEF_XSCROLLCOMMAND          ((char *)NULL)
#define DEF_XSCROLLINCREMENT        "20"
#define DEF_YSCROLLCOMMAND          ((char *)NULL)
#define DEF_YSCROLLINCREMENT        "20"


#define DEF_ITEM_COMMAND            ((char *)NULL)
#define DEF_ITEM_DATA               ((char *)NULL)
#define DEF_ITEM_ICON               ((char *)NULL)
#define DEF_ITEM_IMAGE              ((char *)NULL)
#define DEF_ITEM_INDENT             "0"
#define DEF_ITEM_MENU               ((char *)NULL)
#define DEF_ITEM_STATE              "normal"
#define DEF_ITEM_STYLE              "default"
#define DEF_ITEM_TAGS               ((char *)NULL)
#define DEF_ITEM_TEXT               ((char *)NULL)
#define DEF_ITEM_TIP                ((char *)NULL)
#define DEF_ITEM_TYPE               ((char *)NULL)
#define DEF_STYLE_ACTIVE_BG         RGB_WHITE
#define DEF_STYLE_ACTIVE_FG         RGB_BLACK
#define DEF_STYLE_ACTIVE_RELIEF     "flat"
#define DEF_STYLE_BG                RGB_WHITE
#define DEF_STYLE_BORDERWIDTH       "0"
#define DEF_STYLE_DISABLED_ACCEL_FG STD_DISABLED_FOREGROUND
#define DEF_STYLE_DISABLED_BG       DISABLED_BACKGROUND
#define DEF_STYLE_DISABLED_FG       DISABLED_FOREGROUND
#define DEF_STYLE_FG                RGB_BLACK
#define DEF_STYLE_FONT              STD_FONT_SMALL
#define DEF_STYLE_RELIEF            "flat"
#define DEF_STYLE_SELECT_BG         RGB_SKYBLUE4
#define DEF_STYLE_SELECT_FG         RGB_WHITE
#define DEF_STYLE_SELECT_RELIEF     "flat"
#define DISABLED_BACKGROUND         RGB_GREY90
#define DISABLED_FOREGROUND         RGB_GREY70


static Blt_OptionFreeProc FreeStyleProc;
static Blt_OptionParseProc ObjToStyleProc;
static Blt_OptionPrintProc StyleToObjProc;
static Blt_CustomOption styleOption = {
    ObjToStyleProc, StyleToObjProc, FreeStyleProc, (ClientData)0
};

static Blt_OptionFreeProc FreeTagsProc;
static Blt_OptionParseProc ObjToTagsProc;
static Blt_OptionPrintProc TagsToObjProc;
static Blt_CustomOption tagsOption = {
    ObjToTagsProc, TagsToObjProc, FreeTagsProc, (ClientData)0
};

static Blt_OptionFreeProc FreeTextProc;
static Blt_OptionParseProc ObjToTextProc;
static Blt_OptionPrintProc TextToObjProc;
static Blt_CustomOption textOption = {
    ObjToTextProc, TextToObjProc, FreeTextProc, (ClientData)0
};

static Blt_OptionFreeProc FreeIconProc;
static Blt_OptionParseProc ObjToIconProc;
static Blt_OptionPrintProc IconToObjProc;
static Blt_CustomOption iconOption = {
    ObjToIconProc, IconToObjProc, FreeIconProc, (ClientData)0
};

static Blt_OptionParseProc ObjToStateProc;
static Blt_OptionPrintProc StateToObjProc;
static Blt_CustomOption stateOption = {
    ObjToStateProc, StateToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToRestrictProc;
static Blt_OptionPrintProc RestrictToObjProc;
static Blt_CustomOption restrictOption = {
    ObjToRestrictProc, RestrictToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToSelectmode;
static Blt_OptionPrintProc SelectmodeToObj;
static Blt_CustomOption selectModeOption = {
    ObjToSelectmode, SelectmodeToObj, NULL, NULL,
};

static Blt_OptionParseProc ObjToLayoutmode;
static Blt_OptionPrintProc LayoutmodeToObj;
static Blt_CustomOption layoutModeOption = {
    ObjToLayoutmode, LayoutmodeToObj, NULL, NULL,
};

static Blt_OptionParseProc ObjToSortType;
static Blt_OptionPrintProc SortTypeToObj;
static Blt_CustomOption sortTypeOption = {
    ObjToSortType, SortTypeToObj, NULL, NULL,
};

static Blt_OptionParseProc ObjToColumnProc;
static Blt_OptionPrintProc ColumnToObjProc;
static Blt_CustomOption columnOption = {
    ObjToColumnProc, ColumnToObjProc, NULL, NULL,
};

extern Blt_CustomOption bltLimitsOption;

typedef struct _ListView ListView;

typedef enum LayoutModes {
    LAYOUT_LIST_COLUMN,                 /* Layout items in a single list,
                                         * one item per row. */
    LAYOUT_LIST_ROW,                    /* Layout items in multiple columns. */
    LAYOUT_ICONS,                       /* Layout items in multiple rows 
                                         * using the big icon with the text 
                                         * underneath. */
    LAYOUT_TILES                        /* Layout items using the big icon
                                         * with 3 lines of text on the
                                         * right. */
} LayoutMode;
   
/*
 * Icon --
 *
 *      Since instances of the same Tk image can be displayed in different
 *      windows with possibly different color palettes, Tk internally stores
 *      each instance in a linked list.  But if the instances are used in the
 *      same widget and therefore use the same color palette, this adds a lot
 *      of overhead, especially when deleting instances from the linked list.
 *
 *      For the listview widget, we never need more than a single instance of
 *      an image, regardless of how many times it's used.  Cache the image,
 *      maintaining a reference count for each image used in the widget.  It's
 *      likely that the listview widget will use many instances of the same
 *      image.
 */
typedef struct _Icon {
    Tk_Image tkImage;                   /* The Tk image being cached. */
    Blt_HashEntry *hPtr;                /* Hash table pointer to the image. */
    int refCount;                       /* Reference count for this image. */
    short int width, height;            /* Dimensions of the cached image. */
} *Icon;

#define IconHeight(i)   ((i)->height)
#define IconWidth(i)    ((i)->width)
#define IconImage(i)    ((i)->tkImage)
#define IconName(i)     (Blt_Image_Name(IconImage(i)))

typedef struct {
    const char *name;
    Blt_HashEntry *hPtr;
    ListView *viewPtr;
    int refCount;                       /* Indicates if the style is currently
                                         * in use in the listview. */
    int borderWidth;
    int relief;
    int activeRelief;
    int selectRelief;

    Blt_Bg normalBg;
    Blt_Bg activeBg;
    Blt_Bg selectBg;
    Blt_Bg disabledBg;

    Blt_Font textFont;                  /* Font of the text */
    XColor *textNormalColor;            /* Color of text text. */
    XColor *textDisabledColor;          /* Color of text background. */
    XColor *textActiveColor;            /* Color of text background. */
    XColor *textSelectColor;            /* Color of text background. */

} Style;

static Blt_ConfigSpec styleSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", (char *)NULL, (char *)NULL, 
        DEF_STYLE_ACTIVE_BG, Blt_Offset(Style, activeBg), 0},
    {BLT_CONFIG_COLOR, "-activeforeground", (char *)NULL, (char *)NULL, 
        DEF_STYLE_ACTIVE_FG, Blt_Offset(Style, textActiveColor), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", (char *)NULL, (char *)NULL, 
        DEF_STYLE_ACTIVE_RELIEF, Blt_Offset(Style, activeRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", (char *)NULL, (char *)NULL,
        DEF_STYLE_BG, Blt_Offset(Style, normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_SYNONYM, "-bg", (char *)NULL, (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", (char *)NULL, (char *)NULL,
        DEF_STYLE_BORDERWIDTH, Blt_Offset(Style, borderWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-disabledbackground", (char *)NULL, (char *)NULL, 
        DEF_STYLE_DISABLED_BG, Blt_Offset(Style, disabledBg), 0},
    {BLT_CONFIG_COLOR, "-disabledforeground", (char *)NULL, (char *)NULL, 
        DEF_STYLE_DISABLED_FG, Blt_Offset(Style, textDisabledColor), 0},
    {BLT_CONFIG_SYNONYM, "-fg", (char *)NULL, (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-foreground", (char *)NULL, (char *)NULL, DEF_STYLE_FG, 
        Blt_Offset(Style, textNormalColor), 0},
    {BLT_CONFIG_FONT, "-font", (char *)NULL, (char *)NULL, DEF_STYLE_FONT, 
        Blt_Offset(Style, textFont), 0},
    {BLT_CONFIG_RELIEF, "-relief", (char *)NULL, (char *)NULL, 
        DEF_STYLE_RELIEF, Blt_Offset(Style, relief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", (char *)NULL, (char *)NULL, 
        DEF_STYLE_SELECT_BG, Blt_Offset(Style, selectBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", (char *)NULL, (char *)NULL, 
        DEF_STYLE_SELECT_FG, Blt_Offset(Style, textSelectColor), 0},
    {BLT_CONFIG_RELIEF, "-selectrelief", (char *)NULL, (char *)NULL, 
        DEF_STYLE_ACTIVE_RELIEF, Blt_Offset(Style, selectRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
        0, 0}
};

/*
 *
 *  [icon] [text] 
 *   
 * icon:                all entries.
 * text:                all entries.
 */
typedef struct  {
    ListView *viewPtr;                  /* ListView containing this item. */
    long index;                         /* Index of the item (numbered from 0)*/
    int worldX, worldY;                 /* Upper left world-coordinate of item
                                         * in menu. */
    Style *stylePtr;                    /* Style used by this item. */
    unsigned int flags;                 /* Contains various bits of
                                         * information about the item, such as
                                         * type, state. */
    Blt_ChainLink link;
    int relief;
    int indent;                         /* # of pixels to indent the icon. */
    Icon image;                         /* If non-NULL, image to be displayed
                                         * instead of text. */
    Icon icon;                          /* Small icon. */
    Icon bigIcon;                       /* Big icon. */
    const char *text;                   /* Text to be displayed. */
    Tcl_Obj *cmdObjPtr;                 /* Command to be invoked when item is
                                         * clicked. */
    Tcl_Obj *dataObjPtr;                /* User-data associated with this
                                         * item. */
    Tcl_Obj *tagsObjPtr;

    Tcl_Obj *tipObjPtr;
    const char *type;

    TextLayout *layoutPtr;
    short int textX, textY, textWidth, textHeight;
    short int iconX, iconY, iconWidth, iconHeight;
    short int width, height;
    short int worldWidth, worldHeight;
    BLT_TABLE_ROW row;
} Item;

static Blt_ConfigSpec itemSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-bigicon", (char *)NULL, (char *)NULL, DEF_ITEM_ICON, 
        Blt_Offset(Item, bigIcon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_OBJ, "-command", (char *)NULL, (char *)NULL, DEF_ITEM_COMMAND, 
        Blt_Offset(Item, cmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-data", (char *)NULL, (char *)NULL, DEF_ITEM_DATA, 
        Blt_Offset(Item, dataObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-icon", (char *)NULL, (char *)NULL, DEF_ITEM_ICON, 
        Blt_Offset(Item, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_CUSTOM, "-image", (char *)NULL, (char *)NULL, DEF_ITEM_IMAGE, 
        Blt_Offset(Item, image), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_PIXELS_NNEG, "-indent", (char *)NULL, (char *)NULL, 
        DEF_ITEM_INDENT, Blt_Offset(Item, indent), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-state", (char *)NULL, (char *)NULL, DEF_ITEM_STATE, 
        Blt_Offset(Item, flags), BLT_CONFIG_DONT_SET_DEFAULT, &stateOption},
    {BLT_CONFIG_CUSTOM, "-style", (char *)NULL, (char *)NULL, DEF_ITEM_STYLE, 
        Blt_Offset(Item, stylePtr), 0, &styleOption},
     {BLT_CONFIG_CUSTOM, "-tags", (char *)NULL, (char *)NULL,
        DEF_ITEM_TAGS, 0, BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_CUSTOM, "-text", (char *)NULL, (char *)NULL, DEF_ITEM_TEXT, 
        Blt_Offset(Item, text), BLT_CONFIG_NULL_OK, &textOption},
    {BLT_CONFIG_OBJ, "-tooltip", (char *)NULL, (char *)NULL, DEF_ITEM_TIP, 
         Blt_Offset(Item, tipObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-type", (char *)NULL, (char *)NULL, DEF_ITEM_TYPE, 
         Blt_Offset(Item, type), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
        0, 0}
};

typedef struct {
    BLT_TABLE_COLUMN column;
    BLT_TABLE_TRACE trace;
    BLT_TABLE_NOTIFIER notifier;
} ColumnInfo;

typedef struct {
    ColumnInfo text;                    /* Column containing item's text. */
    ColumnInfo icon;                    /* Column containing item's icon
                                         * name. */
    ColumnInfo bigIcon;                 /* Column containing the item's big
                                         * (64x64) icon name. */
    BLT_TABLE table;
} TableSource;

static Blt_ConfigSpec tableSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-textcolumn", (char *)NULL, (char *)NULL, 
        (char *)NULL, Blt_Offset(TableSource, text), BLT_CONFIG_NULL_OK, 
        &columnOption},
    {BLT_CONFIG_CUSTOM, "-iconcolumn", (char *)NULL, (char *)NULL, 
        (char *)NULL, Blt_Offset(TableSource, icon), BLT_CONFIG_NULL_OK, 
        &columnOption},
    {BLT_CONFIG_CUSTOM, "-bigiconcolumn", (char *)NULL, (char *)NULL, 
        (char *)NULL, Blt_Offset(TableSource, bigIcon), BLT_CONFIG_NULL_OK,
        &columnOption},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
        0, 0}
};

struct _ListView {
    Tk_Window tkwin;                    /* Window that embodies the frame.
                                         * NULL means that the window has been
                                         * destroyed but the data structures
                                         * haven't yet been cleaned up. */
    Display *display;                   /* Display containing widget.  Used,
                                         * among other things, so that
                                         * resources can be freed even after
                                         * tkwin has gone away. */
    Tcl_Interp *interp;                 /* Interpreter associated with widget.
                                         * Used to delete widget command. */
    Tcl_Command cmdToken;               /* Token for widget's command. */

    LayoutMode layoutMode;

    unsigned int flags;
    Tcl_Obj *iconVarObjPtr;             /* Name of TCL variable.  If non-NULL,
                                         * this variable will be set to the
                                         * name of the Tk image representing
                                         * the icon of the selected item.  */
    Tcl_Obj *textVarObjPtr;             /* Name of TCL variable.  If non-NULL,
                                         * this variable will be set to the
                                         * text string of the text of the
                                         * selected item. */
    Tcl_Obj *takeFocusObjPtr;           /* Value of -takefocus option; not
                                         * used in the C code, but used by
                                         * keyboard * traversal scripts. */
    Tk_Cursor cursor;                   /* Current cursor for window or None. */

    Blt_Limits reqWidth, reqHeight;     
    int relief;
    int borderWidth;

    int highlightWidth;                 /* Width in pixels of highlight to
                                         * draw around widget when it has the
                                         * focus.  <= 0 means don't draw a
                                         * highlight. */

    XColor *highlightColor;             /* Color for drawing traversal
                                         * highlight. */
    int inset;                          /* Sum of highlight thickness and
                                         * borderwidth. */

    XColor *focusColor;                 /* Color of focus highlight
                                         * rectangle. */
    GC focusGC;

    Style defStyle;                     /* Default style. */

    TableSource tableSource;            /* Alternative source for item
                                         * data. */

    int maxWidth;

    int xScrollUnits, yScrollUnits;

    /* Commands to control horizontal and vertical scrollbars. */
    Tcl_Obj *xScrollCmdObjPtr, *yScrollCmdObjPtr;

    struct _Blt_Tags tags;              /* Table of tags. */
    Blt_HashTable textTable;            /* Table of texts (hashtables). */
    Blt_HashTable iconTable;            /* Table of icons. */

    Blt_Chain items;

    Item *activePtr;                    /* If non-NULL, item that is currently
                                         * active. */
    Item *focusPtr;                     /* If non-NULL, item that currently
                                         * has focus. */

    int xOffset, yOffset;               /* Scroll offsets of viewport in
                                         * world. */ 
    int worldWidth, worldHeight;        /* Dimension of entire menu. */

    short int textWidth, iconWidth;
    short int textHeight, iconHeight;
    int itemHeight;

    Blt_HashTable styleTable;           /* Table of styles used. */
    /*
     * Scanning Information:
     */
    short int scanAnchorX;              /* Horizontal scan anchor in screen
                                         * x-coordinates. */
    short int scanAnchorY;              /* Vertical scan anchor in screen
                                         * y-coordinates. */
    int scanX;                          /* x-offset of the start of the
                                         * horizontal scan in world
                                         * coordinates.*/
    int scanY;                          /* y-offset of the start of the
                                         * vertical scan in world
                                         * coordinates.*/
    
    /*
     * Selection Information:
     */
    Item *selAnchorPtr;                 /* Fixed end of selection (i.e. item
                                         * at which selection was started.) */
    Item *selMarkPtr;
    
    Tcl_Obj *selCmdObjPtr;              /* TCL script that's invoked whenever
                                         * the selection changes. */

    Blt_HashTable selTable;             /* Hash table of currently selected
                                         * entries. */

    Blt_Chain selected;                /* Chain of currently selected entries.
                                        * Contains the same information as the
                                        * above hash table, but maintains the
                                        * order in which entries are
                                        * selected. */

    Tcl_Obj *sortCmdPtr;

    short int width, height;
    Blt_Painter painter;
    GC copyGC;
};

static Blt_ConfigSpec listViewSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground",
        "ActiveBackground", DEF_STYLE_ACTIVE_BG, 
        Blt_Offset(ListView, defStyle.activeBg), 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground",
        "ActiveForeground", DEF_STYLE_ACTIVE_FG, 
        Blt_Offset(ListView, defStyle.textActiveColor), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "ActiveRelief", 
        DEF_STYLE_ACTIVE_RELIEF, Blt_Offset(ListView, defStyle.activeRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_STYLE_BG, Blt_Offset(ListView, defStyle.normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_BORDERWIDTH, Blt_Offset(ListView, borderWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor", DEF_CURSOR, 
        Blt_Offset(ListView, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-disabledbackground", "disabledBackground",
        "DisabledBackground", DEF_STYLE_DISABLED_BG, 
        Blt_Offset(ListView, defStyle.disabledBg), 0},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground",
        "DisabledForeground", DEF_STYLE_DISABLED_FG, 
        Blt_Offset(ListView, defStyle.textDisabledColor), 0},
    {BLT_CONFIG_BITMASK, "-exportselection", "exportSelection",
        "ExportSelection", DEF_EXPORT_SELECTION, Blt_Offset(ListView, flags),
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SELECT_EXPORT},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-focuscolor", "focusColor", "FocusColor",
        DEF_HIGHLIGHT_COLOR, Blt_Offset(ListView, focusColor), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
        DEF_STYLE_FG, Blt_Offset(ListView, defStyle.textNormalColor), 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_STYLE_FONT, 
        Blt_Offset(ListView, defStyle.textFont), 0},
    {BLT_CONFIG_CUSTOM, "-height", "height", "Height", DEF_HEIGHT, 
        Blt_Offset(ListView, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT,
        &bltLimitsOption},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
        DEF_HIGHLIGHT_COLOR, Blt_Offset(ListView, highlightColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
        "HighlightThickness", DEF_HIGHLIGHT_WIDTH, 
        Blt_Offset(ListView, highlightWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-maxwidth", "maxWidth", "MaxWidth",
        DEF_MAXWIDTH, Blt_Offset(ListView, maxWidth), 0},
    {BLT_CONFIG_OBJ, "-iconvariable", "iconVariable", "IconVariable", 
        DEF_ICON_VARIABLE, Blt_Offset(ListView, iconVarObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-itemborderwidth", "itemBorderWidth", 
        "ItemBorderWidth", DEF_STYLE_BORDERWIDTH, 
        Blt_Offset(ListView, defStyle.borderWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-layoutmode", "layoutMode", "LayoutMode",
        DEF_LAYOUTMODE, Blt_Offset(ListView, layoutMode), 
        BLT_CONFIG_DONT_SET_DEFAULT, &layoutModeOption},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_RELIEF, 
        Blt_Offset(ListView, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-restrictwidth", "restrictWidth", "RestrictWidth", 
        (char *)NULL, Blt_Offset(ListView, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        &restrictOption},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", (char *)NULL, (char *)NULL, 
        DEF_STYLE_SELECT_BG, Blt_Offset(ListView, defStyle.selectBg), 0},
    {BLT_CONFIG_OBJ, "-selectcommand", "selectCommand", "SelectCommand",
        (char *)NULL, Blt_Offset(ListView, selCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-selectforeground", (char *)NULL, (char *)NULL, 
        DEF_STYLE_SELECT_FG, Blt_Offset(ListView, defStyle.textSelectColor),0},
    {BLT_CONFIG_RELIEF, "-selectrelief", (char *)NULL, (char *)NULL, 
        DEF_STYLE_ACTIVE_RELIEF, Blt_Offset(ListView, defStyle.selectRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-selectmode", "selectMode", "SelectMode",
        DEF_SELECTMODE, Blt_Offset(ListView, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, &selectModeOption},
    {BLT_CONFIG_BITMASK, "-orderselection", "orderSelection", "OrderSelection",
        DEF_SORT_SELECTION, Blt_Offset(ListView, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SELECT_ORDERED},
    {BLT_CONFIG_OBJ, "-textvariable", "textVariable", "TextVariable", 
        DEF_TEXTVARIABLE, Blt_Offset(ListView, textVarObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
        DEF_XSCROLLCOMMAND, Blt_Offset(ListView, xScrollCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_POS, "-xscrollincrement", "xScrollIncrement",
        "ScrollIncrement", DEF_XSCROLLINCREMENT, 
         Blt_Offset(ListView, xScrollUnits), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
        DEF_YSCROLLCOMMAND, Blt_Offset(ListView, yScrollCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_POS, "-yscrollincrement", "yScrollIncrement",
        "ScrollIncrement", DEF_YSCROLLINCREMENT, 
         Blt_Offset(ListView, yScrollUnits),BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-takefocus", "takeFocus", "TakeFocus",
        DEF_TAKEFOCUS, Blt_Offset(ListView, takeFocusObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-width", "width", "Width", DEF_WIDTH, 
        Blt_Offset(ListView, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT,
        &bltLimitsOption},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
        0, 0}
};

static Blt_ConfigSpec sortSpecs[] = {
    {BLT_CONFIG_BITMASK, "-autosort", "autoSort", "AutoSort", DEF_AUTO_SORT, 
        Blt_Offset(ListView, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)SORT_AUTO}, 
    {BLT_CONFIG_BITMASK, "-dictionary", "dictionary", "Dictionary",
        DEF_SORT_DICTIONARY, Blt_Offset(ListView, flags),
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SORT_DICTIONARY}, 
    {BLT_CONFIG_OBJ, "-command", "command", "Command", DEF_SORT_COMMAND, 
        Blt_Offset(ListView, sortCmdPtr),
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BITMASK, "-decreasing", "decreasing", "Decreasing",
        DEF_SORT_DECREASING, Blt_Offset(ListView, flags),
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SORT_DECREASING}, 
    {BLT_CONFIG_CUSTOM, "-by", "by", "By", DEF_SORT_TYPE, 
        Blt_Offset(ListView, flags), 0, &sortTypeOption},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
        0, 0}
};

/*
 * ItemIterator --
 *
 *      Items may be tagged with strings.  An item may have many tags.  The
 *      same tag may be used for many items.
 *      
 */

typedef enum { 
    ITER_SINGLE, ITER_ALL, ITER_TAG, ITER_PATTERN
} IteratorType;

typedef struct _Iterator {
    ListView *viewPtr;                  /* ListView that we're iterating
                                         * over. */

    IteratorType type;                  /* Type of iteration:
                                         * ITER_TAG     By item tag.
                                         * ITER_ALL     By every item.
                                         * ITER_SINGLE  Single item: either 
                                         *              tag or index.
                                         */

    Item *startPtr, *last;              /* Starting and ending item.  Starting
                                         * point of search, saved if iterator
                                         * is reused.  Used for ITER_ALL and
                                         * ITER_SINGLE searches. */
    Item *endPtr;                       /* Ending item (inclusive). */
    Item *nextPtr;                      /* Next item. */
    char *tagName;                      /* If non-NULL, is the tag that we are
                                         * currently iterating over. */

    Blt_HashTable *tablePtr;            /* Pointer to tag hash table. */

    Blt_HashSearch cursor;              /* Search iterator for tag hash
                                         * table. */
    Blt_ChainLink link;
} ItemIterator;

static Blt_SwitchParseProc ItemSwitch;
static Blt_SwitchCustom itemSwitch = {
    ItemSwitch, NULL, NULL,  (ClientData)0
};

static Blt_SwitchParseProc PatternSwitch;
static Blt_SwitchCustom patternSwitch = {
    PatternSwitch, NULL, NULL, (ClientData)0
};

#define FIND_GLOB       (0)             /* Default pattern type. */
#define FIND_REGEXP     (1<<0)
#define FIND_EXACT      (1<<1)
#define FIND_PATTERN_MASK (FIND_EXACT|FIND_GLOB|FIND_REGEXP)
#define FIND_HIDDEN     (1<<2)
#define FIND_DISABLED   (1<<3)
#define FIND_ANY        (FIND_HIDDEN|FIND_DISABLED)
#define FIND_WRAP       (1<<4)
#define FIND_REVERSE    (1<<5)

typedef struct {
    unsigned int flags;
    int count;
    Item *fromPtr, *toPtr;
} FindSwitches;

static Blt_SwitchSpec findSwitches[] = 
{
    {BLT_SWITCH_BITMASK, "-any", "", (char *)NULL,
        Blt_Offset(FindSwitches, flags), 0, FIND_ANY},
    {BLT_SWITCH_INT_NNEG, "-count", "number", (char *)NULL,
        Blt_Offset(FindSwitches, count), 0, 0},
    {BLT_SWITCH_BITMASK, "-disabled", "", (char *)NULL,
        Blt_Offset(FindSwitches, flags), 0, FIND_DISABLED},
    {BLT_SWITCH_CUSTOM, "-from", "item", (char *)NULL,
        Blt_Offset(FindSwitches, fromPtr), 0, 0, &itemSwitch},
    {BLT_SWITCH_BITMASK, "-hidden", "", (char *)NULL,
        Blt_Offset(FindSwitches, flags), 0, FIND_HIDDEN},
    {BLT_SWITCH_BITMASK, "-reverse", "", (char *)NULL,
        Blt_Offset(FindSwitches, flags), 0, FIND_REVERSE},
    {BLT_SWITCH_CUSTOM, "-to", "item", (char *)NULL,
        Blt_Offset(FindSwitches, toPtr), 0, 0, &itemSwitch},
    {BLT_SWITCH_CUSTOM, "-type", "glob|regexp|exact", (char *)NULL,
        Blt_Offset(FindSwitches, flags), 0, 0, &patternSwitch},
    {BLT_SWITCH_BITMASK, "-wrap", "", (char *)NULL,
        Blt_Offset(FindSwitches, flags), 0, FIND_WRAP},
    {BLT_SWITCH_END}
};

typedef int (ListViewCmdProc)(ListView *viewPtr, Tcl_Interp *interp, 
        int objc, Tcl_Obj *const *objv);
static int GetItemIterator(Tcl_Interp *interp, ListView *viewPtr,
        Tcl_Obj *objPtr, ItemIterator *iterPtr);
static int GetItemFromObj(Tcl_Interp *interp, ListView *viewPtr,
        Tcl_Obj *objPtr, Item **itemPtrPtr);

static Tcl_IdleProc DisplayItem;
static Tcl_IdleProc DisplayListView;
static Tcl_FreeProc DestroyListView;
static Tk_EventProc ListViewEventProc;
static Tcl_ObjCmdProc ListViewInstCmdProc;
static Tcl_CmdDeleteProc ListViewInstCmdDeletedProc;
static Tk_ImageChangedProc IconChangedProc;

static BLT_TABLE_NOTIFY_EVENT_PROC TableNotifyProc;
static BLT_TABLE_TRACE_PROC TableTraceProc;

/*
 *---------------------------------------------------------------------------
 *
 * ChildSwitch --
 *
 *      Convert a Tcl_Obj representing the label of a child node into its
 *      integer node id.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PatternSwitch(
    ClientData clientData,      /* Flag indicating if the node is considered
                                 * before or after the insertion position. */
    Tcl_Interp *interp,         /* Interpreter to send results back to */
    const char *switchName,     /* Not used. */
    Tcl_Obj *objPtr,            /* String representation */
    char *record,               /* Structure record */
    int offset,                 /* Not used. */
    int flags)                  /* Not used. */
{
    char c;
    const char *string;
    int length;
    unsigned int *flagsPtr = (unsigned int *)(record + offset);
    unsigned int flag;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'g') && (strncmp(string, "glob", length) == 0)) {
        flag = FIND_GLOB;
    } else if ((c == 'r') && (strncmp(string, "regexp", length) == 0)) {
        flag = FIND_REGEXP;
    } else if ((c == 'e') && (strncmp(string, "exact", length) == 0)) {
        flag = FIND_EXACT;
    } else {
        Tcl_AppendResult(interp, "unknown pattern type \"", string, 
                 "\": should be glob, regexp, or exact.", (char *)NULL); 
        return TCL_ERROR;
    }                     
    *flagsPtr &= ~FIND_PATTERN_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLayoutmode --
 *
 *      Convert the string reprsenting a layout mode, to its numeric
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
ObjToLayoutmode(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* Tcl_Obj representing the new
                                         * value. */
    char *widgRec,
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    ListView *viewPtr = (ListView *)widgRec;
    char *string;
    char c;
    int *modePtr = (int *)(widgRec + offset);

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'c') && (strcmp(string, "column") == 0)) {
        *modePtr = LAYOUT_LIST_COLUMN;
    } else if ((c == 'r') && (strcmp(string, "row") == 0)) {
        *modePtr = LAYOUT_LIST_ROW;
    } else if ((c == 'i') && (strcmp(string, "icons") == 0)) {
        *modePtr = LAYOUT_ICONS;
    } else if ((c == 't') && (strcmp(string, "tiles") == 0)) {
        *modePtr = LAYOUT_TILES;
    } else {
        Tcl_AppendResult(interp, "bad select mode \"", string,
            "\": should be column, row, icons, or tiles.", (char *)NULL);
        return TCL_ERROR;
    }
    viewPtr->flags |= LAYOUT_PENDING;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LayoutmodeToObj --
 *
 * Results:
 *      The string representation of the button boolean is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
LayoutmodeToObj(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    int mode = *(int *)(widgRec + offset);

    switch (mode) {
    case LAYOUT_LIST_COLUMN:
        return Tcl_NewStringObj("column", -1);
    case LAYOUT_LIST_ROW:
        return Tcl_NewStringObj("row", -1);
    case LAYOUT_ICONS:
        return Tcl_NewStringObj("icons", -1);
    case LAYOUT_TILES:
        return Tcl_NewStringObj("tiles", -1);
    default:
        return Tcl_NewStringObj("???", -1);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSelectmode --
 *
 *      Convert the string reprsenting a scroll mode, to its numeric
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
ObjToSelectmode(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* Tcl_Obj representing the new
                                         * value. */
    char *widgRec,
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    char *string;
    char c;
    int flag;
    int *flagsPtr = (int *)(widgRec + offset);

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 's') && (strcmp(string, "single") == 0)) {
        flag = SELECT_SINGLE;
    } else if ((c == 'm') && (strcmp(string, "multiple") == 0)) {
        flag = SELECT_MULTIPLE;
    } else {
        Tcl_AppendResult(interp, "bad select mode \"", string,
            "\": should be \"single\" or \"multiple\"", (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~SELECT_MODE_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectmodeToObj --
 *
 * Results:
 *      The string representation of the button boolean is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SelectmodeToObj(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    int mask = *(int *)(widgRec + offset);

    switch (mask & SELECT_MODE_MASK) {
    case SELECT_SINGLE:
        return Tcl_NewStringObj("single", -1);
    case SELECT_MULTIPLE:
        return Tcl_NewStringObj("multiple", -1);
    default:
        return Tcl_NewStringObj("???", -1);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSortType --
 *
 *      Convert the string reprsenting a scroll mode, to its numeric
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
ObjToSortType(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* Tcl_Obj representing the new
                                         * value. */
    char *widgRec,
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    char *string;
    char c;
    int flag;
    int *flagsPtr = (int *)(widgRec + offset);

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 't') && (strcmp(string, "text") == 0)) {
        flag = SORT_BY_TEXT;
    } else if ((c == 't') && (strcmp(string, "type") == 0)) {
        flag = SORT_BY_TYPE;
    } else {
        Tcl_AppendResult(interp, "bad sort mode \"", string,
            "\": should be \"text\" or \"type\"", (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~SORT_BY_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortTypeToObj --
 *
 * Results:
 *      The string representation of the button boolean is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SortTypeToObj(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    int mask = *(int *)(widgRec + offset);

    switch (mask & SORT_BY_MASK) {
    case SORT_BY_TYPE:
        return Tcl_NewStringObj("type", -1);
    case SORT_BY_TEXT:
        return Tcl_NewStringObj("text", -1);
    default:
        return Tcl_NewStringObj("???", -1);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToColumnProc --
 *
 *      Convert the string reprsenting a scroll mode, to its numeric
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
ObjToColumnProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* Tcl_Obj representing the new
                                         * value. */
    char *widgRec,
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    ColumnInfo *ciPtr = (ColumnInfo *)(widgRec + offset);
    TableSource *srcPtr = (TableSource *)(widgRec);
    const char *string;
    BLT_TABLE_COLUMN col;
    BLT_TABLE_TRACE trace;
    BLT_TABLE_NOTIFIER notifier;

    string = Tcl_GetString(objPtr);
    notifier = NULL;
    trace = NULL;
    col = NULL;
    if (string[0] != '\0') {
        ListView *viewPtr = clientData;

        col = blt_table_get_column(interp, srcPtr->table, objPtr);
        if (col == NULL) {
            return TCL_ERROR;
        }
        trace = blt_table_create_column_trace(srcPtr->table, col, 
                TABLE_TRACE_WCU, TableTraceProc, NULL, viewPtr);
        notifier =  blt_table_create_column_notifier(interp, srcPtr->table, col,
                TABLE_NOTIFY_COLUMN_CHANGED | TABLE_NOTIFY_ROW_CHANGED, 
                TableNotifyProc, NULL, viewPtr);
    }
    if (ciPtr->column != col) {
        if (ciPtr->column != NULL) {
            if (ciPtr->trace != NULL) {
                /* Release traces on this column. */
                blt_table_delete_trace(srcPtr->table, ciPtr->trace);
            }
        }
        ciPtr->column = col;
        ciPtr->trace = trace;
        ciPtr->notifier = notifier;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnToObjProc --
 *
 * Results:
 *      The string representation of the button boolean is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ColumnToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    BLT_TABLE_COLUMN col = *(BLT_TABLE_COLUMN *)(widgRec + offset);

    if (col == NULL) {
        return Tcl_NewStringObj("", -1);
    } 
    return Tcl_NewStringObj(blt_table_row_label(col), -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *      Tells the Tk dispatcher to call the listview display routine at the
 *      next idle point.  This request is made only if the window is displayed
 *      and no other redraw request is pending.
 *
 * Results: None.
 *
 * Side effects:
 *      The window is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyRedraw(ListView *viewPtr) 
{
    if ((viewPtr->tkwin != NULL) && !(viewPtr->flags & REDRAW_PENDING)) {
        Tcl_DoWhenIdle(DisplayListView, viewPtr);
        viewPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TableNotifyProc --
 *
 *      This procedure is called with a table column is modified 
 *      (rows are added or deleted).
 *
 *---------------------------------------------------------------------------
 */
static int
TableNotifyProc(ClientData clientData, BLT_TABLE_NOTIFY_EVENT *eventPtr)
{
    ListView *viewPtr = clientData; 

    viewPtr->flags |= REBUILD_TABLE;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TableTraceProc --
 *
 *      This procedure is called with a table value is set/unset or created.
 *      Simply reset the columns and redraw the widget.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TableTraceProc(ClientData clientData, BLT_TABLE_TRACE_EVENT *eventPtr)
{
    ListView *viewPtr = clientData; 

    viewPtr->flags |= REBUILD_TABLE;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectCmdProc --
 *
 *      Invoked at the next idle point whenever the current selection changes.
 *      Executes some application-specific code in the -selectcommand option.
 *      This provides a way for applications to handle selection changes.
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
    ListView *viewPtr = clientData;

    viewPtr->flags &= ~SELECT_PENDING;
    Tcl_Preserve(viewPtr);
    if (viewPtr->selCmdObjPtr != NULL) {
        if (Tcl_EvalObjEx(viewPtr->interp, viewPtr->selCmdObjPtr, 
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
EventuallyInvokeSelectCmd(ListView *viewPtr)
{
    if ((viewPtr->flags & SELECT_PENDING) == 0) {
        viewPtr->flags |= SELECT_PENDING;
        Tcl_DoWhenIdle(SelectCmdProc, viewPtr);
    }
}


static INLINE Item *
FirstItem(ListView *viewPtr)
{
    Blt_ChainLink link;

    link = Blt_Chain_FirstLink(viewPtr->items); 
    if (link != NULL) {
        return Blt_Chain_GetValue(link);
    }
    return NULL;
}

static INLINE Item *
LastItem(ListView *viewPtr)
{
    Blt_ChainLink link;

    link = Blt_Chain_LastLink(viewPtr->items); 
    if (link != NULL) {
        return Blt_Chain_GetValue(link);
    }
    return NULL;
}


static Item *
NextItem(Item *itemPtr)
{
    if (itemPtr != NULL) {
        Blt_ChainLink link;

        link = Blt_Chain_NextLink(itemPtr->link); 
        if (link != NULL) {
            return Blt_Chain_GetValue(link);
        }
    }
    return NULL;
}

static INLINE Item *
PrevItem(Item *itemPtr)
{
    if (itemPtr != NULL) {
        Blt_ChainLink link;

        link = Blt_Chain_PrevLink(itemPtr->link); 
        if (link != NULL) {
            return Blt_Chain_GetValue(link);
        }
    }
    return NULL;
}

static Item *
NextItemAvailable(Item *itemPtr) 
{
    for (itemPtr = NextItem(itemPtr); itemPtr != NULL; 
         itemPtr = NextItem(itemPtr)) {
        if ((itemPtr->flags & (ITEM_HIDDEN|ITEM_DISABLED)) == 0) {
            return itemPtr;
        }
    }
    return NULL;
}

static Item *
PrevItemAvailable(Item *itemPtr) 
{
    for (itemPtr = PrevItem(itemPtr); itemPtr != NULL; 
         itemPtr = PrevItem(itemPtr)) {
        if ((itemPtr->flags & (ITEM_HIDDEN|ITEM_DISABLED)) == 0) {
            return itemPtr;
        }
    }
    return NULL;
}

static int
ItemIsSelected(ListView *viewPtr, Item *itemPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&viewPtr->selTable, (char *)itemPtr);
    return (hPtr != NULL);
}

static void
SelectItem(ListView *viewPtr, Item *itemPtr)
{
    int isNew;
    Blt_HashEntry *hPtr;

    hPtr = Blt_CreateHashEntry(&viewPtr->selTable, (char *)itemPtr, &isNew);
    if (isNew) {
        Blt_ChainLink link;

        link = Blt_Chain_Append(viewPtr->selected, itemPtr);
        Blt_SetHashValue(hPtr, link);
    }
    if ((viewPtr->textVarObjPtr != NULL) && (itemPtr->text != NULL)) {
        Tcl_Obj *objPtr;
        
        objPtr = Tcl_NewStringObj(itemPtr->text, -1);
        if (Tcl_ObjSetVar2(viewPtr->interp, viewPtr->textVarObjPtr, NULL, 
                objPtr, TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG) == NULL) {
            return;
        }
    }
    if ((viewPtr->iconVarObjPtr != NULL) && (itemPtr->icon != NULL)) {
        Tcl_Obj *objPtr;
        
        objPtr = Tcl_NewStringObj(IconName(itemPtr->icon), -1);
        if (Tcl_ObjSetVar2(viewPtr->interp, viewPtr->iconVarObjPtr, NULL, 
                objPtr, TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG) == NULL) {
            return;
        }
    }
}

static void
DeselectItem(ListView *viewPtr, Item *itemPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&viewPtr->selTable, (char *)itemPtr);
    if (hPtr != NULL) {
        Blt_ChainLink link;

        link = Blt_GetHashValue(hPtr);
        Blt_Chain_DeleteLink(viewPtr->selected, link);
        Blt_DeleteHashEntry(&viewPtr->selTable, hPtr);
    }
}

static void
SelectItemUsingFlags(ListView *viewPtr, Item *itemPtr)
{
    switch (viewPtr->flags & SELECT_MASK) {
    case SELECT_CLEAR:
        DeselectItem(viewPtr, itemPtr);
        break;

    case SELECT_SET:
        SelectItem(viewPtr, itemPtr);
        break;

    case SELECT_TOGGLE:
        if (ItemIsSelected(viewPtr, itemPtr)) {
            DeselectItem(viewPtr, itemPtr);
        } else {
            SelectItem(viewPtr, itemPtr);
        }
        break;
    }   
}


static void
ClearSelection(ListView *viewPtr)
{
    Blt_DeleteHashTable(&viewPtr->selTable);
    Blt_InitHashTable(&viewPtr->selTable, BLT_ONE_WORD_KEYS);
    Blt_Chain_Reset(viewPtr->selected);
    EventuallyRedraw(viewPtr);
    if (viewPtr->selCmdObjPtr != NULL) {
        EventuallyInvokeSelectCmd(viewPtr);
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
 *      The existing selection is unhighlighted, and the window is
 *      marked as not containing a selection.
 *
 *---------------------------------------------------------------------------
 */
static void
LostSelection(ClientData clientData)
{
    ListView *viewPtr = clientData;

    if (viewPtr->flags & SELECT_EXPORT) {
        ClearSelection(viewPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectRange --
 *
 *      Sets the selection flag for a range of nodes.  The range is
 *      determined by two pointers which designate the first/last
 *      nodes of the range.
 *
 * Results:
 *      Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectRange(ListView *viewPtr, Item *fromPtr, Item *toPtr)
{
    Blt_ChainLink link;

    if (fromPtr->index > toPtr->index) {
        Item *tmpPtr;

        tmpPtr = fromPtr;
        fromPtr = toPtr;
        toPtr = tmpPtr;
    }
    for (link = fromPtr->link; link != NULL; link = Blt_Chain_NextLink(link)) {
        Item *itemPtr;

        itemPtr = Blt_Chain_GetValue(link);
        if ((itemPtr->flags & ITEM_DISABLED) == 0) {
            SelectItemUsingFlags(viewPtr, itemPtr);
        }
        if (itemPtr->index >= toPtr->index) {
            break;
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionProc --
 *
 *      This procedure is called back by Tk when the selection is requested by
 *      someone.  It returns part or all of the selection in a buffer provided
 *      by the caller.
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
    int maxBytes)                       /* Maximum number of bytes to place at
                                         * buffer, not including terminating
                                         * NULL character. */
{
    ListView *viewPtr = clientData;
    Tcl_DString ds;
    int size;

    if ((viewPtr->flags & SELECT_EXPORT) == 0) {
        return -1;
    }
    /*
     * Retrieve the names of the selected entries.
     */
    Tcl_DStringInit(&ds);
    if (viewPtr->flags & SELECT_ORDERED) {
        Blt_ChainLink link;

        for (link = Blt_Chain_FirstLink(viewPtr->selected); link != NULL; 
             link = Blt_Chain_NextLink(link)) {
            Item *itemPtr;
            
            itemPtr = Blt_Chain_GetValue(link);
            Tcl_DStringAppend(&ds, itemPtr->text, -1);
            Tcl_DStringAppend(&ds, "\n", -1);
        }
    } else {
        Item *itemPtr;

        for (itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
             itemPtr = NextItem(itemPtr)) {
            if (ItemIsSelected(viewPtr, itemPtr)) {
                Tcl_DStringAppend(&ds, itemPtr->text, -1);
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
 * EventuallyRedrawItem --
 *
 *      Tells the Tk dispatcher to call the listview display routine at the
 *      next idle point.  This request is made only if the window is displayed
 *      and no other redraw request is pending.
 *
 * Results: None.
 *
 * Side effects:
 *      The window is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyRedrawItem(Item *itemPtr) 
{
    ListView *viewPtr;

    if (itemPtr->flags & (ITEM_HIDDEN|ITEM_REDRAW)) {
        return;
    }
    viewPtr = itemPtr->viewPtr;
    if (viewPtr->flags & REDRAW_PENDING) {
        return;
    }
    if (viewPtr->tkwin != NULL) {
        Tcl_DoWhenIdle(DisplayItem, itemPtr);
        itemPtr->flags |= ITEM_REDRAW;
    }
}

static void
RemoveText(ListView *viewPtr, Item *itemPtr)
{
    Blt_HashEntry *hPtr;
        
    hPtr = Blt_FindHashEntry(&viewPtr->textTable, itemPtr->text);
    if (hPtr != NULL) {
        Blt_HashTable *tablePtr;
        Blt_HashEntry *h2Ptr;
        
        tablePtr = Blt_GetHashValue(hPtr);
        h2Ptr = Blt_FindHashEntry(tablePtr, (char *)itemPtr);
        if (h2Ptr != NULL) {
            itemPtr->text = emptyString;
            Blt_DeleteHashEntry(tablePtr, h2Ptr);
            if (tablePtr->numEntries == 0) {
                Blt_DeleteHashEntry(&viewPtr->textTable, hPtr);
                Blt_DeleteHashTable(tablePtr);
                Blt_Free(tablePtr);
            }
        }
    }
}

static void
DestroyItem(Item *itemPtr)
{
    ListView *viewPtr = itemPtr->viewPtr;

    DeselectItem(viewPtr, itemPtr);
    Blt_Tags_ClearTagsFromItem(&viewPtr->tags, itemPtr);
    iconOption.clientData = viewPtr;
    if (itemPtr->layoutPtr != NULL) {
        Blt_Free(itemPtr->layoutPtr);
    }
    RemoveText(viewPtr, itemPtr);
    Blt_FreeOptions(itemSpecs, (char *)itemPtr, viewPtr->display, 0);
    if (viewPtr->activePtr == itemPtr) {
        viewPtr->activePtr = NULL;
    }
    if (viewPtr->flags & SORT_AUTO) {
        viewPtr->flags |= SORT_PENDING;
    }
    viewPtr->flags |= LAYOUT_PENDING;
    Blt_Chain_DeleteLink(viewPtr->items, itemPtr->link);
}

static void
DestroyItems(ListView *viewPtr)
{
    Blt_ChainLink link, next;

    for (link = Blt_Chain_FirstLink(viewPtr->items); link != NULL; 
         link = next) {
        Item *itemPtr;

        next = Blt_Chain_NextLink(link);
        itemPtr = Blt_Chain_GetValue(link);
        DestroyItem(itemPtr);
    }
    if (viewPtr->flags & SORT_AUTO) {
        viewPtr->flags |= SORT_PENDING;
    }
    viewPtr->flags |= LAYOUT_PENDING;
    Blt_Chain_Destroy(viewPtr->items);
}

static Item *
NewItem(ListView *viewPtr)
{
    Item *itemPtr;
    Blt_ChainLink link;

    link = Blt_Chain_AllocLink(sizeof(Item));
    itemPtr = Blt_Chain_GetValue(link);
    itemPtr->viewPtr = viewPtr;
    itemPtr->flags |= ITEM_NORMAL;
    itemPtr->link = link;
    itemPtr->index = Blt_Chain_GetLength(viewPtr->items);
    Blt_Chain_LinkAfter(viewPtr->items, link, NULL);
    itemPtr->text = emptyString;
    return itemPtr;
}

static INLINE Item *
FindItemByIndex(ListView *viewPtr, long index)
{
    Blt_ChainLink link;

    if ((index < 1) || (index > Blt_Chain_GetLength(viewPtr->items))) {
        return NULL;
    }
    link = Blt_Chain_GetNthLink(viewPtr->items, index - 1);
    return Blt_Chain_GetValue(link);
}


static INLINE Item *
BeginItem(ListView *viewPtr)
{
    Blt_ChainLink link;

    link = Blt_Chain_FirstLink(viewPtr->items); 
    if (link != NULL) {
        return Blt_Chain_GetValue(link);
    }
    return NULL;
}

static INLINE Item *
EndItem(ListView *viewPtr)
{
    Blt_ChainLink link;

    link = Blt_Chain_LastLink(viewPtr->items); 
    if (link != NULL) {
        return Blt_Chain_GetValue(link);
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ActivateItem --
 *
 *      Marks the designated item as active.  The item is redrawn with its
 *      active colors.  The previously active item is deactivated.  If the new
 *      item is NULL, then this means that no new item is to be activated.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Menu items may be scheduled to be drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
ActivateItem(ListView *viewPtr, Item *itemPtr) 
{
    if ((viewPtr->activePtr == itemPtr) && (itemPtr != NULL)) {
        return;         /* Item is already active. */
    }
    if (viewPtr->activePtr != NULL) {
        EventuallyRedrawItem(viewPtr->activePtr);
    }
    viewPtr->activePtr = itemPtr;
    if (itemPtr != NULL) {
        EventuallyRedrawItem(itemPtr);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * GetBoundedWidth --
 *
 *      Bounds a given width value to the limits described in the limit
 *      structure.  The initial starting value may be overridden by the nominal
 *      value in the limits.
 *
 * Results:
 *      Returns the constrained value.
 *
 *---------------------------------------------------------------------------
 */
static int
GetBoundedWidth(ListView *viewPtr, int w)       
{
    /*
     * Check widgets for requested width values;
     */
    if (viewPtr->reqWidth.flags & LIMITS_NOM_SET) {
        w = viewPtr->reqWidth.nom;      /* Override initial value */
    }
    if (w < viewPtr->reqWidth.min) {
        w = viewPtr->reqWidth.min;      /* Bounded by minimum value */
    }
    if (w > viewPtr->reqWidth.max) {
        w = viewPtr->reqWidth.max;      /* Bounded by maximum value */
    }
    if (viewPtr->flags & (RESTRICT_MIN|RESTRICT_MAX)) {
        Tk_Window parent;

        parent = Tk_Parent(viewPtr->tkwin);
        if ((viewPtr->flags & RESTRICT_MIN) && (w < Tk_Width(parent))) {
            w = Tk_Width(parent);
        }
        if ((viewPtr->flags & RESTRICT_MAX) && (w > Tk_Width(parent))) {
            w = Tk_Width(parent);
        }
    }
    {
        int screenWidth, screenHeight;

        Blt_SizeOfScreen(viewPtr->tkwin, &screenWidth, &screenHeight);
        if (w > screenWidth) {
            w = screenWidth;
        }
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
GetBoundedHeight(ListView *viewPtr, int h)      
{
    /*
     * Check widgets for requested height values;
     */
    if (viewPtr->reqHeight.flags & LIMITS_NOM_SET) {
        h = viewPtr->reqHeight.nom;     /* Override initial value */
    }
    if (h < viewPtr->reqHeight.min) {
        h = viewPtr->reqHeight.min;     /* Bounded by minimum value */
    }
    if (h > viewPtr->reqHeight.max) {
        h = viewPtr->reqHeight.max;     /* Bounded by maximum value */
    }
    if (h > HeightOfScreen(Tk_Screen(viewPtr->tkwin))) {
        h = HeightOfScreen(Tk_Screen(viewPtr->tkwin));
    }
    return h;
}

static void
ComputeIconsItemGeometry(ListView *viewPtr, Item *itemPtr)
{
    Icon icon;
    
    /* Determine the height of the item.  It's the maximum height of all it's
     * components: left gadget (radiobutton or checkbutton), icon, text,
     * right gadget (cascade), and accelerator. */
    itemPtr->textWidth = itemPtr->textHeight = 0;
    itemPtr->iconWidth = itemPtr->iconHeight = 0;
    itemPtr->height = itemPtr->width = 0;

    if (itemPtr->flags & ITEM_HIDDEN) {
        return;
    }
    icon = itemPtr->bigIcon;
    if (icon != NULL) {
        itemPtr->iconWidth = IconWidth(icon) + 2;
        itemPtr->iconHeight = IconHeight(icon) + 2;
    }
    if (itemPtr->image != NULL) {
        itemPtr->textWidth = IconWidth(itemPtr->image);
        itemPtr->textHeight = IconHeight(itemPtr->image);
        itemPtr->textWidth  += 2 * itemPtr->stylePtr->borderWidth;
        itemPtr->textHeight += 2 * itemPtr->stylePtr->borderWidth;
        itemPtr->textWidth |= 0x1;
        itemPtr->textHeight |= 0x1;
    } else if (itemPtr->text != emptyString) {
        unsigned int w, h;
        int maxWidth;
        TextStyle ts;

        if (itemPtr->layoutPtr != NULL) {
            Blt_Free(itemPtr->layoutPtr);
        }
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, itemPtr->stylePtr->textFont);
        Blt_Ts_SetAnchor(ts, TK_ANCHOR_NW);
        Blt_Ts_SetJustify(ts, TK_JUSTIFY_CENTER);
        maxWidth = (viewPtr->maxWidth > 0) ? viewPtr->maxWidth : 10000;
        Blt_Ts_SetMaxLength(ts, maxWidth);
        itemPtr->layoutPtr = Blt_Ts_TitleLayout(itemPtr->text, -1, &ts);
        w = itemPtr->layoutPtr->width + 2 * itemPtr->stylePtr->borderWidth;
        h = itemPtr->layoutPtr->height + 2 * itemPtr->stylePtr->borderWidth;
        itemPtr->textWidth = w | 0x1;
        itemPtr->textHeight = h | 0x1;
    }
    if ((itemPtr->iconWidth > 0) && (itemPtr->iconHeight > 0)) {
        itemPtr->height += itemPtr->iconHeight;
        if (itemPtr->width < itemPtr->iconWidth) {
            itemPtr->width = itemPtr->iconWidth;
        }
    }
    if ((itemPtr->textWidth > 0) && (itemPtr->textHeight > 0)) {
        int w;
        w = itemPtr->textWidth /* + 6 */;
        itemPtr->height += itemPtr->textHeight + 6;
        if (itemPtr->width < w) {
            itemPtr->width = w;
        }
    }
    if ((itemPtr->textHeight > 0) && (itemPtr->iconHeight > 0)) {
        itemPtr->width += ITEM_IPAD;
    }
}

static void
ComputeListItemGeometry(ListView *viewPtr, Item *itemPtr)
{
    Icon icon;

    /* Determine the height of the item.  It's the maximum height of all it's
     * components: left gadget (radiobutton or checkbutton), icon, text,
     * right gadget (cascade), and accelerator. */
    itemPtr->textWidth = itemPtr->textHeight = 0;
    itemPtr->iconWidth = itemPtr->iconHeight = 0;
    itemPtr->height = itemPtr->width = 0;
    itemPtr->worldWidth = itemPtr->worldHeight = 0;
    if (itemPtr->flags & ITEM_HIDDEN) {
        return;
    }
    icon = (viewPtr->layoutMode == LAYOUT_ICONS) ? itemPtr->bigIcon :
        itemPtr->icon;
    if (icon != NULL) {
        itemPtr->iconWidth = IconWidth(icon) + 2;
        itemPtr->iconHeight = IconHeight(icon) + 2;
    }
    if (itemPtr->image != NULL) {
        itemPtr->textWidth = IconWidth(itemPtr->image);
        itemPtr->textHeight = IconHeight(itemPtr->image);
        itemPtr->textWidth  += 2 * itemPtr->stylePtr->borderWidth;
        itemPtr->textHeight += 2 * itemPtr->stylePtr->borderWidth;
        itemPtr->textWidth |= 0x1;
        itemPtr->textHeight |= 0x1;
    } else if (itemPtr->text != emptyString) {
        unsigned int w, h;
        TextStyle ts;
        
        if (itemPtr->layoutPtr != NULL) {
            Blt_Free(itemPtr->layoutPtr);
        }
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, itemPtr->stylePtr->textFont);
        Blt_Ts_SetAnchor(ts, TK_ANCHOR_NW);
        Blt_Ts_SetJustify(ts, TK_JUSTIFY_LEFT);
        Blt_Ts_SetMaxLength(ts, viewPtr->textWidth);
        itemPtr->layoutPtr = Blt_Ts_CreateLayout(itemPtr->text, -1, &ts);
        w = itemPtr->layoutPtr->width + 2 * itemPtr->stylePtr->borderWidth;
        h = itemPtr->layoutPtr->height + 2 * itemPtr->stylePtr->borderWidth;
        itemPtr->textWidth = w | 0x1;
        itemPtr->textHeight = h | 0x1;
    }
    if ((itemPtr->iconWidth > 0) && (itemPtr->iconHeight > 0)) {
        itemPtr->width += itemPtr->iconWidth;
        if (itemPtr->height < itemPtr->iconHeight) {
            itemPtr->height = itemPtr->iconHeight;
        }
        if (viewPtr->iconWidth < itemPtr->iconWidth) {
            viewPtr->iconWidth = itemPtr->iconWidth;
        }
        
    }
    if ((itemPtr->textWidth > 0) && (itemPtr->textHeight > 0)) {
        int h;

        itemPtr->width += itemPtr->textWidth + 6;
        h = itemPtr->textHeight + 6;
        if (itemPtr->height < h) {
            itemPtr->height = h;
        }
    }
    if ((itemPtr->textWidth > 0) && (itemPtr->iconWidth > 0)) {
        itemPtr->width += ITEM_IPAD;
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * ComputeRowLayout --
 *
 *      Computes the layout of the widget with the items displayed
 *      one per row.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeRowLayout(ListView *viewPtr)
{
    int x, y, w, h;
    int reqWidth, reqHeight;
    Item *itemPtr;
    int numEntries, maxWidth, minHeight, maxHeight, maxIconWidth;

    numEntries = maxWidth = maxHeight = maxIconWidth = 0;
    minHeight = SHRT_MAX;
    viewPtr->itemHeight = 0;
    for (itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
         itemPtr = NextItem(itemPtr)) {
        if (itemPtr->flags & ITEM_HIDDEN) {
            continue;
        }
        ComputeListItemGeometry(viewPtr, itemPtr);
        if (maxIconWidth < itemPtr->iconWidth) {
            maxIconWidth = itemPtr->iconWidth;
        }
        if (maxWidth < itemPtr->width) {
            maxWidth = itemPtr->width;
        }
        if (maxHeight < itemPtr->height) {
            maxHeight = itemPtr->height;
        }
        if (minHeight > itemPtr->height) {
            minHeight = itemPtr->height;
        }
        numEntries++;
    }
    if (numEntries == 0) {
        return;
    }
    /* Now compute the worldX positions */
    x = y = 0;
    for (itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
         itemPtr = NextItem(itemPtr)) {
        if (itemPtr->flags & ITEM_HIDDEN) {
            continue;
        }
        itemPtr->worldX = x;
        itemPtr->worldY = y;
        itemPtr->worldWidth = itemPtr->width;
        itemPtr->worldHeight = itemPtr->height;
        itemPtr->iconX = 1 + (maxIconWidth - itemPtr->iconWidth) / 2;
        itemPtr->iconY = 1 + (itemPtr->height - itemPtr->iconHeight) / 2;

        itemPtr->textX = 2 + maxIconWidth; 
        if ((itemPtr->textWidth > 0) && (itemPtr->iconWidth > 0)) {
            itemPtr->textX += ITEM_IPAD;
        }
        itemPtr->textY = 1 + (maxHeight - itemPtr->textHeight) / 2;
        y += itemPtr->height;
    }
    viewPtr->worldWidth = x;
    viewPtr->worldHeight = y;

    /* Figure out the requested size of the widget.  This will also tell us if
     * we need scrollbars. */
 
    reqWidth =  viewPtr->worldWidth  + 2 * viewPtr->inset;
    reqHeight = viewPtr->worldHeight + 2 * viewPtr->inset;

    w = GetBoundedWidth(viewPtr, reqWidth);
    h = GetBoundedHeight(viewPtr, reqHeight);

    if ((w != Tk_ReqWidth(viewPtr->tkwin)) || 
        (h != Tk_ReqHeight(viewPtr->tkwin))) {
        Tk_GeometryRequest(viewPtr->tkwin, w, h);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeColumnLayout --
 *
 *      Computes the layout of the widget with the items displayed multiple
 *      columns.  The current height of the widget is used as a basis for the
 *      number of rows in a column.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeColumnLayout(ListView *viewPtr)
{
    int x, y, w, h;
    int reqWidth, reqHeight;
    int maxWidth, maxHeight;
    int winHeight;
    int numRows, numEntries;
    int count, lastMaxWidth, maxIconWidth;
    Item *itemPtr;

    numEntries = maxWidth = maxHeight = maxIconWidth = 0;
    viewPtr->itemHeight = 0;
    for (itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
         itemPtr = NextItem(itemPtr)) {
        if (itemPtr->flags & ITEM_HIDDEN) {
            continue;
        }
        ComputeListItemGeometry(viewPtr, itemPtr);
        if (maxIconWidth < itemPtr->iconWidth) {
            maxIconWidth = itemPtr->iconWidth;
        }
        if (maxWidth < itemPtr->width) {
            maxWidth = itemPtr->width;
        }
        if (maxHeight < itemPtr->height) {
            maxHeight = itemPtr->height;
        }
        numEntries++;
    }
    if (numEntries == 0) {
        return;
    }
    winHeight = VPORTHEIGHT(viewPtr);
    if (winHeight <= 1) {
        winHeight = Tk_ReqHeight(viewPtr->tkwin) - 2 * viewPtr->inset;
    }
    numRows = winHeight / maxHeight;
    if (numRows <= 0) {
        numRows = 1;
    }
    /* Now compute the worldX positions */
    x = y = 0;
    count = 0;
    lastMaxWidth = 0;
    for (itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
         itemPtr = NextItem(itemPtr)) {
        if (itemPtr->flags & ITEM_HIDDEN) {
            continue;
        }
        itemPtr->worldX = x;
        itemPtr->worldY = y;
        itemPtr->worldWidth = itemPtr->width;
        itemPtr->worldHeight = maxHeight;
        if (itemPtr->width > lastMaxWidth) {
            lastMaxWidth = itemPtr->width;
        }
        itemPtr->iconX = 1 + (maxIconWidth - itemPtr->iconWidth) / 2;
        itemPtr->iconY = 1 + (maxHeight - itemPtr->iconHeight) / 2;

        itemPtr->textX = 2 + maxIconWidth; 
        if ((itemPtr->textWidth > 0) && (itemPtr->iconWidth > 0)) {
            itemPtr->textX += ITEM_IPAD;
        }
        itemPtr->textY = 1 + (maxHeight - itemPtr->textHeight) / 2;
        y += maxHeight;
        count++;
        if ((count % numRows) == 0) {
            x += lastMaxWidth;
            y = 0;
            lastMaxWidth = 0;
        }
    }
    if (count > 0) {
        x += lastMaxWidth;
    }
    viewPtr->worldWidth = x;
    viewPtr->worldHeight = numRows * maxHeight;
 
    reqWidth =  viewPtr->worldWidth  + 2 * viewPtr->inset;
    reqHeight = viewPtr->worldHeight + 2 * viewPtr->inset;

    w = GetBoundedWidth(viewPtr, reqWidth);
    h = GetBoundedHeight(viewPtr, reqHeight);

    if ((w != Tk_ReqWidth(viewPtr->tkwin)) || 
        (h != Tk_ReqHeight(viewPtr->tkwin))) {
        Tk_GeometryRequest(viewPtr->tkwin, w, h);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeIconsLayout --
 *
 *      Computes the layout of the widget with the items displayed multiple
 *      columns.  The current height of the widget is used as a basis for the
 *      number of rows in a column.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeIconsLayout(ListView *viewPtr)
{
    int x, y, w, h;
    int reqWidth, reqHeight;
    int maxHeight, maxWidth;
    int winWidth;
    int numColumns, numEntries;
    int count;
    Item *itemPtr;

    maxHeight = maxWidth = 0;
    numEntries = 0;
    for (itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
         itemPtr = NextItem(itemPtr)) {
        if (itemPtr->flags & ITEM_HIDDEN) {
            continue;
        }
        ComputeIconsItemGeometry(viewPtr, itemPtr);
        if (maxWidth < itemPtr->width) {
            maxWidth = itemPtr->width;
        }
        if (maxHeight < itemPtr->height) {
            maxHeight = itemPtr->height;
        }
        if (itemPtr->height > viewPtr->itemHeight) {
            viewPtr->itemHeight = itemPtr->height;
        }
        numEntries++;
    }
    if (numEntries == 0) {
        return;
    }
    winWidth = VPORTWIDTH(viewPtr);
    if (winWidth <= 1) {
        winWidth = Tk_ReqWidth(viewPtr->tkwin) - 2 * viewPtr->inset;
    }
    if ((viewPtr->maxWidth > 0) && (viewPtr->maxWidth < maxWidth)) {
        maxWidth = viewPtr->maxWidth;
    }
    numColumns = winWidth / maxWidth;
    if (numColumns <= 0) {
        numColumns = 1;
    }
    maxWidth = winWidth / numColumns;
    viewPtr->textWidth = maxWidth - 6;

    /* Now compute the worldX positions */
    x = y = 0;
    count = 0;
    maxHeight = 0;
    for (itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
         itemPtr = NextItem(itemPtr)) {
        if (itemPtr->flags & ITEM_HIDDEN) {
            continue;
        }
        if (itemPtr->height > maxHeight) {
            maxHeight = itemPtr->height;
        }
        itemPtr->worldX = x;
        itemPtr->worldY = y;
        itemPtr->worldWidth = maxWidth;
        itemPtr->worldHeight = maxHeight;
        itemPtr->iconX = (maxWidth - itemPtr->iconWidth) / 2;
        itemPtr->iconY = 1;
        itemPtr->textX = 1;
        if (maxWidth > itemPtr->textWidth) {
            itemPtr->textX = (maxWidth - itemPtr->textWidth) / 2;
        }
        itemPtr->textY = itemPtr->iconHeight;
        if ((itemPtr->textHeight > 0) && (itemPtr->iconHeight > 0)) {
            /* itemPtr->textY += ITEM_IPAD; */
        }
        x += maxWidth;
        count++;
        if ((count % numColumns) == 0) {
            y += maxHeight;
            viewPtr->worldHeight += maxHeight;
            maxHeight = 0;
            x = 0;
        }
    }
    if (count > 0) {
        x += maxWidth;
        viewPtr->worldHeight += maxHeight;
    }
    viewPtr->worldWidth = numColumns * maxWidth;

    reqWidth =  viewPtr->worldWidth  + 2 * viewPtr->inset;
    reqHeight = viewPtr->worldHeight + 2 * viewPtr->inset;

    w = GetBoundedWidth(viewPtr, reqWidth);
    h = GetBoundedHeight(viewPtr, reqHeight);

    if ((w != Tk_ReqWidth(viewPtr->tkwin)) || 
        (h != Tk_ReqHeight(viewPtr->tkwin))) {
        Tk_GeometryRequest(viewPtr->tkwin, w, h);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeLayout --
 *
 *      Computes the layout of the widget with the items displayed multiple
 *      columns.  The current height of the widget is used as a basis for the
 *      number of rows in a column.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeLayout(ListView *viewPtr)
{
    int viewWidth, viewHeight;

    viewPtr->worldWidth = viewPtr->worldHeight = 0;
    viewPtr->iconWidth = viewPtr->textWidth = 0;
    viewPtr->textWidth = -1;
    viewPtr->itemHeight = 0;
    switch (viewPtr->layoutMode) {
    case LAYOUT_LIST_ROW:
        ComputeRowLayout(viewPtr);
        break;
    case LAYOUT_LIST_COLUMN:
        ComputeColumnLayout(viewPtr);
        break;
    case LAYOUT_ICONS:
        ComputeIconsLayout(viewPtr);
        break;
    case LAYOUT_TILES:
        Blt_Warn("layout not implemented\n");
        break;
    }
    viewWidth = VPORTWIDTH(viewPtr);
    if (viewPtr->xOffset > (viewPtr->worldWidth - viewWidth)) {
        viewPtr->xOffset = viewPtr->worldWidth - viewWidth;
    }
    if (viewPtr->xOffset < 0) {
        viewPtr->xOffset = 0;
    }
    viewHeight = VPORTHEIGHT(viewPtr);
    if (viewPtr->yOffset > (viewPtr->worldHeight - viewHeight)) {
        viewPtr->yOffset = viewPtr->worldHeight - viewHeight;
    }
    if (viewPtr->yOffset < 0) {
        viewPtr->yOffset = 0;
    }
    viewPtr->flags &= ~LAYOUT_PENDING;
    viewPtr->flags |= SCROLL_PENDING;
}


/*
 *---------------------------------------------------------------------------
 *
 * SearchForItem --
 *
 *      Performs a binary search for the item at the given y-offset in world
 *      coordinates.  The range of items is specified by menu indices (high
 *      and low).  The item must be (visible) in the viewport.
 *
 * Results:
 *      Returns 0 if no item is found, other the index of the item (menu
 *      indices start from 1).
 *
 *---------------------------------------------------------------------------
 */
static Item *
SearchForItem(ListView *viewPtr, int x, int y)
{
    Item *itemPtr;
    

    for (itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
         itemPtr = NextItem(itemPtr)) {
        if (itemPtr->flags & ITEM_HIDDEN) {
            continue;
        }
        if ((x >= itemPtr->worldX) && 
            (x < (itemPtr->worldX+itemPtr->worldWidth)) &&
            (y >= itemPtr->worldY) && 
            (y < (itemPtr->worldY+itemPtr->worldHeight))) {
            return itemPtr;
        }
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * NearestItem --
 *
 *      Find the item closest to the x-y screen coordinate given.  The item
 *      must be (visible) in the viewport.
 *
 * Results:
 *      Returns the closest item.  If selectOne is set, then always returns an
 *      item (unless the menu is empty).  Otherwise, NULL is returned is the
 *      pointer is not over an item.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Item *
NearestItem(ListView *viewPtr, int x, int y, int selectOne)
{
    Item *itemPtr;

    if ((x < 0) || (x >= Tk_Width(viewPtr->tkwin)) || 
        (y < 0) || (y >= Tk_Height(viewPtr->tkwin))) {
        return NULL;                    /* Screen coordinates are outside of
                                         * menu. */
    }
    /*
     * Item positions are saved in world coordinates. Convert the text point
     * screen y-coordinate to a world coordinate.
     */
    itemPtr = SearchForItem(viewPtr, WORLDX(viewPtr, x), WORLDY(viewPtr, y));
    if (itemPtr == NULL) {
        if (!selectOne) {
            return NULL;
        }
        if (y < viewPtr->inset) {
            return FirstItem(viewPtr);
        }
        return LastItem(viewPtr);
    }
    return itemPtr;
}


static void
DestroyStyle(Style *stylePtr)
{
    ListView *viewPtr;

    stylePtr->refCount--;
    if (stylePtr->refCount > 0) {
        return;
    }
    viewPtr = stylePtr->viewPtr;
    iconOption.clientData = viewPtr;
    Blt_FreeOptions(styleSpecs, (char *)stylePtr, viewPtr->display, 0);
    if (stylePtr->hPtr != NULL) {
        Blt_DeleteHashEntry(&stylePtr->viewPtr->styleTable, stylePtr->hPtr);
    }
    if (stylePtr != &stylePtr->viewPtr->defStyle) {
        Blt_Free(stylePtr);
    }
        
}

static Style *
AddDefaultStyle(Tcl_Interp *interp, ListView *viewPtr)
{
    Blt_HashEntry *hPtr;
    int isNew;
    Style *stylePtr;

    hPtr = Blt_CreateHashEntry(&viewPtr->styleTable, "default", &isNew);
    if (!isNew) {
        Tcl_AppendResult(interp, "listview style \"", "default", 
                "\" already exists.", (char *)NULL);
        return NULL;
    }
    stylePtr = &viewPtr->defStyle;
    assert(stylePtr);
    stylePtr->refCount = 1;
    stylePtr->name = Blt_GetHashKey(&viewPtr->styleTable, hPtr);
    stylePtr->hPtr = hPtr;
    stylePtr->viewPtr = viewPtr;
    stylePtr->borderWidth = 0;
    stylePtr->activeRelief = TK_RELIEF_FLAT;
    Blt_SetHashValue(hPtr, stylePtr);
    return TCL_OK;
}

static void
DestroyStyles(ListView *viewPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&viewPtr->styleTable, &iter); 
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        Style *stylePtr;

        stylePtr = Blt_GetHashValue(hPtr);
        stylePtr->hPtr = NULL;
        stylePtr->refCount = 0;
        DestroyStyle(stylePtr);
    }
    Blt_DeleteHashTable(&viewPtr->styleTable);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetStyleFromObj --
 *
 *      Gets the style associated with the given name.  
 *
 *---------------------------------------------------------------------------
 */
static int 
GetStyleFromObj(Tcl_Interp *interp, ListView *viewPtr, Tcl_Obj *objPtr,
                Style **stylePtrPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&viewPtr->styleTable, Tcl_GetString(objPtr));
    if (hPtr == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find style \"", 
                Tcl_GetString(objPtr), "\" in listview \"", 
                Tk_PathName(viewPtr->tkwin), "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    *stylePtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetTag --
 *
 *      Associates a tag with a given row.  Individual row tags are
 *      stored in hash tables keyed by the tag name.  Each table is in
 *      turn stored in a hash table keyed by the row location.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      A tag is stored for a particular row.
 *
 *---------------------------------------------------------------------------
 */
static int
SetTag(Tcl_Interp *interp, Item *itemPtr, const char *tagName)
{
    ListView *viewPtr;
    long dummy;
    
    if ((strcmp(tagName, "all") == 0) || (strcmp(tagName, "end") == 0)) {
        return TCL_OK;                  /* Don't need to create reserved
                                         * tags. */
    }
    if (tagName[0] == '\0') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tagName, "\" can't be empty.", 
                (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (tagName[0] == '-') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tagName, 
                "\" can't start with a '-'.", (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (Blt_GetLong(NULL, (char *)tagName, &dummy) == TCL_OK) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tagName, "\" can't be a number.",
                             (char *)NULL);
        }
        return TCL_ERROR;
    }
    viewPtr = itemPtr->viewPtr;
    Blt_Tags_AddItemToTag(&viewPtr->tags, tagName, itemPtr);
    return TCL_OK;
}

static int
GetIcon(Tcl_Interp *interp, ListView *viewPtr, const char *string, 
        Icon *iconPtr)
{
    Blt_HashEntry *hPtr;
    struct _Icon *iPtr;
    int isNew;

    if (string[0] == '\0') {
        *iconPtr = NULL;
        return TCL_OK;
    }
    hPtr = Blt_CreateHashEntry(&viewPtr->iconTable, string, &isNew);
    if (isNew) {
        Tk_Image tkImage;
        int w, h;

        tkImage = Tk_GetImage(interp, viewPtr->tkwin, (char *)string, 
                IconChangedProc, viewPtr);
        if (tkImage == NULL) {
            Blt_DeleteHashEntry(&viewPtr->iconTable, hPtr);
            return TCL_ERROR;
        }
        Tk_SizeOfImage(tkImage, &w, &h);
        iPtr = Blt_AssertMalloc(sizeof(struct _Icon ));
        iPtr->tkImage = tkImage;
        iPtr->hPtr = hPtr;
        iPtr->refCount = 1;
        iPtr->width = w;
        iPtr->height = h;
        Blt_SetHashValue(hPtr, iPtr);
    } else {
        iPtr = Blt_GetHashValue(hPtr);
        iPtr->refCount++;
    }
    *iconPtr = iPtr;
    return TCL_OK;
}

static int
GetIconFromObj(Tcl_Interp *interp, ListView *viewPtr, Tcl_Obj *objPtr, 
               Icon *iconPtr)
{
    return GetIcon(interp, viewPtr, Tcl_GetString(objPtr), iconPtr);
}

static void
FreeIcon(ListView *viewPtr, struct _Icon *iPtr)
{
    iPtr->refCount--;
    if (iPtr->refCount == 0) {
        Blt_DeleteHashEntry(&viewPtr->iconTable, iPtr->hPtr);
        Tk_FreeImage(iPtr->tkImage);
        Blt_Free(iPtr);
    }
}

static void
DestroyIcons(ListView *viewPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&viewPtr->iconTable, &iter);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        Icon icon;

        icon = Blt_GetHashValue(hPtr);
        Tk_FreeImage(IconImage(icon));
        Blt_Free(icon);
    }
    Blt_DeleteHashTable(&viewPtr->iconTable);
}

static INLINE Item *
FindItemByText(ListView *viewPtr, const char *text)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&viewPtr->textTable, text);
    if (hPtr != NULL) {
        Blt_HashTable *tablePtr;
        Blt_HashEntry *h2Ptr;
        Blt_HashSearch iter;

        tablePtr = Blt_GetHashValue(hPtr);
        h2Ptr = Blt_FirstHashEntry(tablePtr, &iter);
        if (h2Ptr != NULL) {
            return Blt_GetHashValue(h2Ptr);
        }
    }
    return NULL;
}

static char *
NewText(Item *itemPtr, const char *text)
{
    Blt_HashEntry *hPtr, *h2Ptr;
    Blt_HashTable *tablePtr;
    int isNew;
    ListView *viewPtr;

    viewPtr = itemPtr->viewPtr;
    hPtr = Blt_CreateHashEntry(&viewPtr->textTable, text, &isNew);
    if (isNew) {
        tablePtr = Blt_AssertMalloc(sizeof(Blt_HashTable));
        Blt_InitHashTable(tablePtr, BLT_ONE_WORD_KEYS);
        Blt_SetHashValue(hPtr, tablePtr);
    } else {
        tablePtr = Blt_GetHashValue(hPtr);
    }
    h2Ptr = Blt_CreateHashEntry(tablePtr, (char *)itemPtr, &isNew);
    Blt_SetHashValue(h2Ptr, itemPtr);
    return Blt_GetHashKey(&viewPtr->textTable, hPtr);
}


static void
DestroyTexts(ListView *viewPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&viewPtr->textTable, &iter); 
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        Blt_HashTable *tablePtr;
        
        tablePtr = Blt_GetHashValue(hPtr);
        Blt_DeleteHashTable(tablePtr);
        Blt_Free(tablePtr);
    }
    Blt_DeleteHashTable(&viewPtr->textTable);
}

static void
MoveItem(ListView *viewPtr, Item *itemPtr, int dir, Item *wherePtr)
{
    if (Blt_Chain_GetLength(viewPtr->items) == 1) {
        return;                         /* Can't rearrange one item. */
    }
    Blt_Chain_UnlinkLink(viewPtr->items, itemPtr->link);
    switch(dir) {
    case 0:                             /* After */
        Blt_Chain_LinkAfter(viewPtr->items, itemPtr->link, wherePtr->link);
        break;
    case 1:                             /* At */
        Blt_Chain_LinkAfter(viewPtr->items, itemPtr->link, wherePtr->link);
        break;
    default:
    case 2:                             /* Before */
        Blt_Chain_LinkBefore(viewPtr->items, itemPtr->link, wherePtr->link);
        break;
    }
    {
        long count;

        for (count = 0, itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
             itemPtr = NextItem(itemPtr), count++) {
            itemPtr->index = count;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NextTaggedItem --
 *
 *      Returns the next item derived from the given tag.
 *
 * Results:
 *      Returns the row location of the first item.  If no more rows can be
 *      found, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Item *
NextTaggedItem(ItemIterator *iterPtr)
{
    Item *itemPtr;

    switch (iterPtr->type) {
    case ITER_TAG:
    case ITER_ALL:
        if (iterPtr->link != NULL) {
            itemPtr = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return itemPtr;
        }
        break;

    case ITER_PATTERN:
        {
            Blt_ChainLink link;
            
            for (link = iterPtr->link; link != NULL; 
                 link = Blt_Chain_NextLink(link)) {
                Item *itemPtr;
                
                itemPtr = Blt_Chain_GetValue(iterPtr->link);
                if (Tcl_StringMatch(itemPtr->text, iterPtr->tagName)) {
                    iterPtr->link = Blt_Chain_NextLink(link);
                    return itemPtr;
                }
            }
            break;
        }
    default:
        break;
    }   
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * FirstTaggedItem --
 *
 *      Returns the first item derived from the given tag.
 *
 * Results:
 *      Returns the row location of the first item.  If no more rows can be
 *      found, then -1 is returned.
 *
 *---------------------------------------------------------------------------
 */
static Item *
FirstTaggedItem(ItemIterator *iterPtr)
{
    Item *itemPtr;
            
    switch (iterPtr->type) {
    case ITER_TAG: 
    case ITER_ALL:
        if (iterPtr->link != NULL) {
            itemPtr = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return itemPtr;
        }
        break;

    case ITER_PATTERN:
        {
            Blt_ChainLink link;
            
            for (link = iterPtr->link; link != NULL; 
                 link = Blt_Chain_NextLink(link)) {
                Item *itemPtr;
                
                itemPtr = Blt_Chain_GetValue(iterPtr->link);
                if (Tcl_StringMatch(itemPtr->text, iterPtr->tagName)) {
                    iterPtr->link = Blt_Chain_NextLink(link);
                    return itemPtr;
                }
            }
        }
        break;

    case ITER_SINGLE:
        itemPtr = iterPtr->startPtr;
        iterPtr->nextPtr = NextTaggedItem(iterPtr);
        return itemPtr;
    } 
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetItemFromObj --
 *
 *      Get the item associated the given index, tag, or text.  This routine
 *      is used when you want only one item.  It's an error if more than one
 *      item is specified (e.g. "all" tag).  It's also an error if the tag is
 *      empty (no items are currently tagged).
 *
 *---------------------------------------------------------------------------
 */
static int 
GetItemFromObj(Tcl_Interp *interp, ListView *viewPtr, Tcl_Obj *objPtr,
              Item **itemPtrPtr)
{
    ItemIterator iter;
    Item *firstPtr;

    if (GetItemIterator(interp, viewPtr, objPtr, &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    firstPtr = FirstTaggedItem(&iter);
    if (firstPtr != NULL) {
        Item *nextPtr;

        nextPtr = NextTaggedItem(&iter);
        if (nextPtr != NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "multiple items specified by \"", 
                        Tcl_GetString(objPtr), "\"", (char *)NULL);
            }
            return TCL_ERROR;
        }
    }
    *itemPtrPtr = firstPtr;
    return TCL_OK;
}

static int
GetItemByIndex(Tcl_Interp *interp, ListView *viewPtr, const char *string, 
              int length, Item **itemPtrPtr)
{
    Item *itemPtr;
    char c;
    long pos;

    itemPtr = NULL;
    c = string[0];
    if ((isdigit(c)) && (Blt_GetLong(NULL, string, &pos) == TCL_OK)) {
        Blt_ChainLink link;

        link = Blt_Chain_GetNthLink(viewPtr->items, pos);
        if (link != NULL) {
            itemPtr = Blt_Chain_GetValue(link);
        } 
        if (itemPtr == NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "can't find item: bad index \"", 
                        string, "\"", (char *)NULL);
            }
            return TCL_ERROR;
        }               
    } else if ((c == 'n') && (strcmp(string, "next") == 0)) {
        itemPtr = NextItemAvailable(viewPtr->focusPtr);
        if (itemPtr == NULL) {
            itemPtr = viewPtr->focusPtr;
        }
    } else if ((c == 'p') && (strcmp(string, "previous") == 0)) {
        itemPtr = PrevItemAvailable(viewPtr->focusPtr);
        if (itemPtr == NULL) {
            itemPtr = viewPtr->focusPtr;
        }
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
        itemPtr = LastItem(viewPtr);
    } else if ((c == 'f') && (strcmp(string, "first") == 0)) {
        itemPtr = FirstItem(viewPtr);
    } else if ((c == 'l') && (strcmp(string, "last") == 0)) {
        itemPtr = LastItem(viewPtr);
    } else if ((c == 'n') && (strcmp(string, "none") == 0)) {
        itemPtr = NULL;
    } else if ((c == 'a') && (strcmp(string, "active") == 0)) {
        itemPtr = viewPtr->activePtr;
    } else if ((c == 'f') && (strcmp(string, "focus") == 0)) {
        itemPtr = viewPtr->focusPtr;
    } else if (c == '@') {
        int x, y;

        if (Blt_GetXY(viewPtr->interp, viewPtr->tkwin, string, &x, &y) 
            != TCL_OK) {
            return TCL_ERROR;
        }
        itemPtr = NearestItem(viewPtr, x, y, FALSE);
    } else {
        return TCL_CONTINUE;
    }
    *itemPtrPtr = itemPtr;
    return TCL_OK;
}

static Item *
GetItemByText(ListView *viewPtr, const char *string)
{
    Blt_HashEntry *hPtr;
    
    hPtr = Blt_FindHashEntry(&viewPtr->textTable, string);
    if (hPtr != NULL) {
        Blt_HashTable *tablePtr;
        Blt_HashEntry *h2Ptr;
        Blt_HashSearch iter;

        tablePtr = Blt_GetHashValue(hPtr);
        h2Ptr = Blt_FirstHashEntry(tablePtr, &iter);
        if (h2Ptr != NULL) {
            return Blt_GetHashValue(h2Ptr);
        }
    }
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 *
 * GetItemIterator --
 *
 *      Converts a string representing a item index into an item pointer.  The
 *      index may be in one of the following forms:
 *
 *       number         Item at index in the list of items.
 *       @x,y           Item closest to the specified X-Y screen coordinates.
 *       "active"       Item where mouse pointer is located.
 *       "posted"       Item is the currently posted cascade item.
 *       "next"         Next item from the focus item.
 *       "previous"     Previous item from the focus item.
 *       "end"          Last item.
 *       "none"         No item.
 *
 *       number         Item at position in the list of items.
 *       @x,y           Item closest to the specified X-Y screen coordinates.
 *       "active"       Item mouse is located over.
 *       "focus"        Item is the widget's focus.
 *       "select"       Currently selected item.
 *       "right"        Next item from the focus item.
 *       "left"         Previous item from the focus item.
 *       "up"           Next item from the focus item.
 *       "down"         Previous item from the focus item.
 *       "end"          Last item in list.
 *      "index:number"  Item at index number in list of items.
 *      "tag:string"    Item(s) tagged by "string".
 *      "text:pattern"  Item(s) with text matching "pattern".
 *      
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.  The
 *      pointer to the node is returned via itemPtrPtr.  Otherwise, TCL_ERROR
 *      is returned and an error message is left in interpreter's result
 *      field.
 *
 *---------------------------------------------------------------------------
 */
static int
GetItemIterator(Tcl_Interp *interp, ListView *viewPtr, Tcl_Obj *objPtr,
               ItemIterator *iterPtr)
{
    Item *itemPtr;
    Blt_Chain chain;
    char *string;
    char c;
    int numBytes;
    int length;
    int result;

    iterPtr->viewPtr = viewPtr;
    iterPtr->type = ITER_SINGLE;
    iterPtr->tagName = Tcl_GetStringFromObj(objPtr, &numBytes);
    iterPtr->nextPtr = NULL;
    iterPtr->link = NULL;
    iterPtr->startPtr = iterPtr->endPtr = NULL;

    if (viewPtr->flags & LAYOUT_PENDING) {
        ComputeLayout(viewPtr);
    }
    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    iterPtr->startPtr = iterPtr->endPtr = viewPtr->activePtr;
    iterPtr->type = ITER_SINGLE;
    result = GetItemByIndex(interp, viewPtr, string, length, &itemPtr);
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (result == TCL_OK) {
        iterPtr->startPtr = iterPtr->endPtr = itemPtr;
        return TCL_OK;
    }
    if ((c == 'a') && (strcmp(iterPtr->tagName, "all") == 0)) {
        iterPtr->type  = ITER_ALL;
        iterPtr->link = Blt_Chain_FirstLink(viewPtr->items);
    } else if ((c == 'i') && (length > 6) && 
               (strncmp(string, "index:", 6) == 0)) {
        if (GetItemByIndex(interp, viewPtr, string + 6, length - 6, &itemPtr) 
            != TCL_OK) {
            return TCL_ERROR;
        }
        iterPtr->startPtr = iterPtr->endPtr = itemPtr;
    } else if ((c == 't') && (length > 4) && 
               (strncmp(string, "tag:", 4) == 0)) {
        Blt_Chain chain;

        chain = Blt_Tags_GetItemList(&viewPtr->tags, string + 4);
        if (chain == NULL) {
            return TCL_OK;
        }
        iterPtr->tagName = string + 4;
        iterPtr->link = Blt_Chain_FirstLink(chain);
        iterPtr->type = ITER_TAG;
    } else if ((c == 'l') && (length > 6) && 
               (strncmp(string, "text:", 6) == 0)) {
        iterPtr->link = Blt_Chain_FirstLink(viewPtr->items);
        iterPtr->tagName = string + 6;
        iterPtr->type = ITER_PATTERN;
    } else if ((itemPtr = GetItemByText(viewPtr, string)) != NULL) {
        iterPtr->startPtr = iterPtr->endPtr = itemPtr;
    } else if ((chain = Blt_Tags_GetItemList(&viewPtr->tags, string)) != NULL) {
        iterPtr->tagName = string;
        iterPtr->link = Blt_Chain_FirstLink(chain);
        iterPtr->type = ITER_TAG;
    } else {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find item index, text, or tag \"", 
                string, "\" in \"", Tk_PathName(viewPtr->tkwin), "\"", 
                             (char *)NULL);
        }
        return TCL_ERROR;
    }   
    return TCL_OK;
}
    
static int
ConfigureItem(Tcl_Interp *interp, Item *itemPtr, int objc, Tcl_Obj *const *objv,
              int flags)
{
    ListView *viewPtr;

    viewPtr = itemPtr->viewPtr;
    iconOption.clientData = viewPtr;
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, itemSpecs, 
        objc, objv, (char *)itemPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    viewPtr->flags |= LAYOUT_PENDING;
    return TCL_OK;
}

static int
ConfigureStyle(Tcl_Interp *interp, Style *stylePtr, int objc, 
               Tcl_Obj *const *objv, int flags)
{
    ListView *viewPtr = stylePtr->viewPtr;

    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, styleSpecs, 
        objc, objv, (char *)stylePtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
ConfigureListView(Tcl_Interp *interp, ListView *viewPtr, int objc,
                   Tcl_Obj *const *objv, int flags)
{
    unsigned int gcMask;
    XGCValues gcValues;
    GC newGC;

    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, listViewSpecs, 
        objc, objv, (char *)viewPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ConfigureStyle(interp, &viewPtr->defStyle, 0, NULL, 
                       BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }   
    viewPtr->inset = viewPtr->borderWidth + viewPtr->highlightWidth;

    /* Focus rectangle */
    gcMask = GCForeground | GCLineWidth | GCLineStyle | GCDashList;
    gcValues.line_width = 0;
    gcValues.foreground = viewPtr->focusColor->pixel;
    gcValues.line_style = LineOnOffDash;
    gcValues.dashes = 1;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (viewPtr->focusGC != NULL) {
        Tk_FreeGC(viewPtr->display, viewPtr->focusGC);
    }
    viewPtr->focusGC = newGC;

    /* This is the equilavent of DefaultGC. */
    gcMask = 0;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (viewPtr->copyGC != NULL) {
        Tk_FreeGC(viewPtr->display, viewPtr->copyGC);
    }
    viewPtr->copyGC = newGC;
        
    return TCL_OK;
}

static int
RebuildTableItems(Tcl_Interp *interp, ListView *viewPtr, BLT_TABLE table)
{
    BLT_TABLE_ROW row;
    Blt_HashTable rowTable;
    Blt_Chain chain;

    Blt_InitHashTable(&rowTable, BLT_ONE_WORD_KEYS);
    if (viewPtr->tableSource.table != NULL) {
        Item *itemPtr;

        /* Create a hash table of row to item mappings. */
        for (itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
             itemPtr = NextItem(itemPtr)) {
            Blt_HashEntry *hPtr;
            int isNew;
            if (itemPtr->row == NULL) {
                continue;
            }
            hPtr = Blt_CreateHashEntry(&rowTable, (char *)itemPtr->row, &isNew);
            assert(isNew);
            Blt_SetHashValue(hPtr, itemPtr);
        }
    }
    chain = Blt_Chain_Create();
    for (row = blt_table_first_row(table); row != NULL; 
         row = blt_table_next_row(table, row)) {
        Item *itemPtr;
        Blt_HashEntry *hPtr;

        hPtr = Blt_FindHashEntry(&rowTable, (char *)row);
        if (hPtr == NULL) {
            itemPtr = NewItem(viewPtr);
            itemPtr->row = row;
            if (ConfigureItem(interp, itemPtr, 0, NULL, 0) != TCL_OK) {
                DestroyItem(itemPtr);
                return TCL_ERROR;       
            }
        } else {
            itemPtr = Blt_GetHashValue(hPtr);
        }
        itemPtr->index = blt_table_row_index(row) - 1;
        /* Move the item to the new list. */
        Blt_Chain_UnlinkLink(viewPtr->items, itemPtr->link);
        Blt_Chain_AppendLink(chain, itemPtr->link);
        if (viewPtr->tableSource.text.column != NULL) {
            const char *text;
            BLT_TABLE_COLUMN col;

            col = viewPtr->tableSource.text.column;
            text = blt_table_get_string(table, row, col);
            itemPtr->text = NewText(itemPtr, text);
        } else {
            itemPtr->text = NewText(itemPtr, blt_table_row_label(row));
        }
        if (viewPtr->tableSource.icon.column != NULL) {
            Icon icon;
            BLT_TABLE_COLUMN col;
            const char *string;

            col = viewPtr->tableSource.icon.column;
            string = blt_table_get_string(table, row, col);
            if (GetIcon(interp, viewPtr, string, &icon) != TCL_OK) {
                return TCL_ERROR;
            }
            itemPtr->icon = icon;
        } 
        if (viewPtr->tableSource.bigIcon.column != NULL) {
            Icon icon;
            BLT_TABLE_COLUMN col;
            const char *string;

            col = viewPtr->tableSource.bigIcon.column;
            string = blt_table_get_string(table, row, col);
            if (GetIcon(interp, viewPtr, string, &icon) != TCL_OK) {
                return TCL_ERROR;
            }
            itemPtr->bigIcon = icon;
        } 
    }
    Blt_DeleteHashTable(&rowTable);
    DestroyItems(viewPtr);
    viewPtr->items = chain;
    viewPtr->flags &= ~SORT_AUTO;
    viewPtr->flags |= LAYOUT_PENDING | SORTED;
    return TCL_OK;
}


/* Widget Callbacks */

/*
 *---------------------------------------------------------------------------
 *
 * ListViewEventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for various events on
 *      listview widgets.
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
ListViewEventProc(ClientData clientData, XEvent *eventPtr)
{
    ListView *viewPtr = clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedraw(viewPtr);
        }
    } else if (eventPtr->type == UnmapNotify) {
        EventuallyRedraw(viewPtr);
    } else if (eventPtr->type == ConfigureNotify) {
        viewPtr->flags |= (SCROLL_PENDING | LAYOUT_PENDING);
        EventuallyRedraw(viewPtr);
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
        if (eventPtr->xfocus.detail == NotifyInferior) {
            return;
        }
        if (eventPtr->type == FocusIn) {
            viewPtr->flags |= FOCUS;
        } else {
            viewPtr->flags &= ~FOCUS;
        }
        EventuallyRedraw(viewPtr);
    } else if (eventPtr->type == DestroyNotify) {
        if (viewPtr->tkwin != NULL) {
            viewPtr->tkwin = NULL; 
        }
        if (viewPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayListView, viewPtr);
        }
        Tcl_EventuallyFree(viewPtr, DestroyListView);
    }
}

/*ARGSUSED*/
static void
FreeStyleProc(ClientData clientData, Display *display, char *widgRec, 
              int offset)
{
    Style *stylePtr = *(Style **)(widgRec + offset);

    if ((stylePtr != NULL) && (stylePtr != &stylePtr->viewPtr->defStyle)) {
        DestroyStyle(stylePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToStyleProc --
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
static int
ObjToStyleProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing style. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    ListView *viewPtr;
    Item *itemPtr = (Item *)widgRec;
    Style **stylePtrPtr = (Style **)(widgRec + offset);
    Style *stylePtr;
    char *string;

    string = Tcl_GetString(objPtr);
    viewPtr = itemPtr->viewPtr;
    if ((string[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
        stylePtr = NULL;
    } else if (GetStyleFromObj(interp, viewPtr, objPtr, &stylePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    /* Release the old style. */
    if ((*stylePtrPtr != NULL) && (*stylePtrPtr != &viewPtr->defStyle)) {
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
 *      Return the name of the style.
 *
 * Results:
 *      The name representing the style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
StyleToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Widget information record */
    int offset,                         /* Offset to field in structure */
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
 * ObjToRestrictProc --
 *
 *      Convert the string representation of an item state into a flag.
 *
 * Results:
 *      The return value is a standard TCL result.  The state flags are
 *      updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToRestrictProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing state. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    char *string;
    int flag;

    string = Tcl_GetString(objPtr);
    if (strcmp(string, "min") == 0) {
        flag = RESTRICT_MIN;
    } else if (strcmp(string, "max") == 0) {
        flag = RESTRICT_MAX;
    } else if (strcmp(string, "both") == 0) {
        flag = RESTRICT_MIN|RESTRICT_MAX;
    } else if (strcmp(string, "none") == 0) {
        flag = 0;
    } else {
        Tcl_AppendResult(interp, "unknown state \"", string, 
                "\": should be active, disabled, or normal.", (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~(RESTRICT_MIN|RESTRICT_MAX);
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RestrictToObjProc --
 *
 *      Return the string representation of the restrict flags.
 *
 * Results:
 *      The name representing the style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
RestrictToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Widget information record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    const char *string;

    string = NULL;                      /* Suppress compiler warning. */
    switch (*flagsPtr & (RESTRICT_MIN|RESTRICT_MAX)) {
    case RESTRICT_MIN:  string = "min";         break;
    case RESTRICT_MAX:  string = "max";         break;
    case RESTRICT_NONE: string = "none";        break;
    case (RESTRICT_MIN|RESTRICT_MAX):
        string = "both"; break;
    }
    return Tcl_NewStringObj(string, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToStateProc --
 *
 *      Convert the string representation of an item state into a flag.
 *
 * Results:
 *      The return value is a standard TCL result.  The state flags are
 *      updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToStateProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing state. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Item *itemPtr = (Item *)(widgRec);
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    char *string;
    ListView *viewPtr;
    int flag;

    string = Tcl_GetString(objPtr);
    if (strcmp(string, "disabled") == 0) {
        flag = ITEM_DISABLED;
    } else if (strcmp(string, "normal") == 0) {
        flag = ITEM_NORMAL;
    } else {
        Tcl_AppendResult(interp, "unknown state \"", string, 
                "\": should be active, disabled, or normal.", (char *)NULL);
        return TCL_ERROR;
    }
    if (itemPtr->flags & flag) {
        return TCL_OK;                  /* State is already set to value. */
    }
    viewPtr = itemPtr->viewPtr;
    if (viewPtr->activePtr != itemPtr) {
        ActivateItem(viewPtr, NULL);
        viewPtr->activePtr = NULL;
    }
    *flagsPtr &= ~ITEM_STATE_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StateToObjProc --
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
StateToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Widget information record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    unsigned int state = *(unsigned int *)(widgRec + offset);
    Tcl_Obj *objPtr;

    if (state & ITEM_NORMAL) {
        objPtr = Tcl_NewStringObj("normal", -1);
    } else if (state & ITEM_DISABLED) {
        objPtr = Tcl_NewStringObj("disabled", -1);
    } else {
        objPtr = Tcl_NewStringObj("???", -1);
    }
    return objPtr;
}


/*ARGSUSED*/
static void
FreeTagsProc(
    ClientData clientData,
    Display *display,           /* Not used. */
    char *widgRec,
    int offset)
{
    ListView *viewPtr;
    Item *itemPtr = (Item *)widgRec;

    viewPtr = itemPtr->viewPtr;
    Blt_Tags_ClearTagsFromItem(&viewPtr->tags, itemPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTagsProc --
 *
 *      Convert the string representation of a list of tags.
 *
 * Results:
 *      The return value is a standard TCL result.  The tags are
 *      save in the widget.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTagsProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing style. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    ListView *viewPtr;
    Item *itemPtr = (Item *)widgRec;
    int i;
    char *string;
    int objc;
    Tcl_Obj **objv;

    viewPtr = itemPtr->viewPtr;
    Blt_Tags_ClearTagsFromItem(&viewPtr->tags, itemPtr);
    string = Tcl_GetString(objPtr);
    if ((string[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
        return TCL_OK;
    }
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i++) {
        SetTag(interp, itemPtr, Tcl_GetString(objv[i]));
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagsToObjProc --
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
TagsToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Widget information record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    ListView *viewPtr;
    Item *itemPtr = (Item *)widgRec;
    Tcl_Obj *listObjPtr;

    viewPtr = itemPtr->viewPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Blt_Tags_AppendTagsToObj(&viewPtr->tags,  itemPtr, listObjPtr);
    return listObjPtr;
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
IconChangedProc(
    ClientData clientData,
    int x, int y, int w, int h,         /* Not used. */
    int imageWidth, int imageHeight)    /* Not used. */
{
    ListView *viewPtr = clientData;

    viewPtr->flags |= (LAYOUT_PENDING | SCROLL_PENDING);
    EventuallyRedraw(viewPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToIconProc --
 *
 *      Convert a image into a hashed icon.
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
ObjToIconProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* Tcl_Obj representing the new
                                         * value. */
    char *widgRec,
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    ListView *viewPtr = clientData;
    Icon *iconPtr = (Icon *)(widgRec + offset);
    Icon icon;

    if (GetIconFromObj(interp, viewPtr, objPtr, &icon) != TCL_OK) {
        return TCL_ERROR;
    }
    if (*iconPtr != NULL) {
        FreeIcon(viewPtr, *iconPtr);
    }
    *iconPtr = icon;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IconToObjProc --
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
IconToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,
    int offset,                         /* Offset to field in structure */
    int flags)  
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
FreeIconProc(
    ClientData clientData,
    Display *display,                   /* Not used. */
    char *widgRec,
    int offset)
{
    Icon *iconPtr = (Icon *)(widgRec + offset);

    if (*iconPtr != NULL) {
        ListView *viewPtr = clientData;

        FreeIcon(viewPtr, *iconPtr);
        *iconPtr = NULL;
    }
}

/*ARGSUSED*/
static void
FreeTextProc(
    ClientData clientData,
    Display *display,                   /* Not used. */
    char *widgRec,
    int offset)
{
    Item *itemPtr = (Item *)widgRec;

    if (itemPtr->text != emptyString) {
        RemoveText(itemPtr->viewPtr, itemPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTextProc --
 *
 *      Save the text and add the item to the text hashtable.
 *
 * Results:
 *      A standard TCL result. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTextProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing style. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Item *itemPtr = (Item *)(widgRec);
    char *string;

    if (itemPtr->text != emptyString) {
        RemoveText(itemPtr->viewPtr, itemPtr);
    }
    string = Tcl_GetString(objPtr);
    if ((string[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
        return TCL_OK;
    }
    itemPtr->text = NewText(itemPtr, string);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextToObjProc --
 *
 *      Return the text of the item.
 *
 * Results:
 *      The text is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TextToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Widget information record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Item *itemPtr = (Item *)(widgRec);
    Tcl_Obj *objPtr;

    if (itemPtr->text == emptyString) {
        objPtr = Tcl_NewStringObj("", -1);
    } else {
        objPtr = Tcl_NewStringObj(itemPtr->text, -1);
    }
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ItemSwitch --
 *
 *      Convert a string representing an item into its pointer.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ItemSwitch(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Not used. */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    Item **itemPtrPtr = (Item **)(record + offset);
    ListView *viewPtr = clientData;
    Item *itemPtr;

    if (GetItemFromObj(interp, viewPtr, objPtr, &itemPtr) != TCL_OK) {
        return TCL_ERROR;
    } 
    *itemPtrPtr = itemPtr;
    return TCL_OK;
}


/* Widget Operations */

/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      .cm activate item
 *
 *---------------------------------------------------------------------------
 */
static int
ActivateOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Item *itemPtr;

    if (GetItemFromObj(NULL, viewPtr, objv[2], &itemPtr) != TCL_OK) {
        return TCL_ERROR;
    } 
    if (viewPtr->activePtr == itemPtr) {
        return TCL_OK;                  /* Item is already active. */
    }
    ActivateItem(viewPtr, NULL);
    viewPtr->activePtr = NULL;
    if ((itemPtr != NULL) &&
        ((itemPtr->flags & (ITEM_DISABLED|ITEM_HIDDEN)) == 0)) {
        ActivateItem(viewPtr, itemPtr);
        viewPtr->activePtr = itemPtr;
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * AddOp --
 *
 *      Appends a new item to the listview.
 *
 * Results:
 *      NULL is always returned.
 *
 * Side effects:
 *      The listview entry may become selected or deselected.
 *
 *   .cm add -text "fred" -tags ""
 *
 *---------------------------------------------------------------------------
 */
static int
AddOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Item *itemPtr;

    itemPtr = NewItem(viewPtr);
    if (ConfigureItem(interp, itemPtr, objc - 2, objv + 2, 0) != TCL_OK) {
        DestroyItem(itemPtr);
        return TCL_ERROR;               /* Error configuring the entry. */
    }
    if (viewPtr->flags & SORT_AUTO) {
        viewPtr->flags |= SORT_PENDING;
    }
    viewPtr->flags |= LAYOUT_PENDING;
    viewPtr->flags &= ~SORTED;
    EventuallyRedraw(viewPtr);
    Tcl_SetLongObj(Tcl_GetObjResult(interp), itemPtr->index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AddListOp --
 *
 *      Appends a list of items to the listview.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      New items are added to the listview.
 *
 *   .t load list $list -icon "image1" -tags ""
 *   .t load table $table -extcolumn -textcolumn -iconcolumn -bigiconcolumn 
 *
 *---------------------------------------------------------------------------
 */
static int
AddListOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    int i;
    int count;
    Tcl_Obj **names;
    Tcl_Obj *listObjPtr;

    if (Tcl_ListObjGetElements(interp, objv[2], &count, &names) != TCL_OK) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (i = 0; i < count; i++) {
        Tcl_Obj *objPtr;
        Item *itemPtr;

        itemPtr = NewItem(viewPtr);
        if (ConfigureItem(interp, itemPtr, objc - 3, objv + 3, 0) != TCL_OK) {
            DestroyItem(itemPtr);
            return TCL_ERROR;   
        }
        itemPtr->text = NewText(itemPtr, Tcl_GetString(names[i]));
        objPtr = Tcl_NewLongObj(itemPtr->index);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    if (viewPtr->flags & SORT_AUTO) {
        viewPtr->flags |= SORT_PENDING;
    }
    viewPtr->flags |= LAYOUT_PENDING;
    viewPtr->flags &= ~SORTED;
    EventuallyRedraw(viewPtr);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      .cm configure ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    int result;

    iconOption.clientData = viewPtr;
    if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, listViewSpecs, 
                (char *)viewPtr, (Tcl_Obj *)NULL,  0);
    } else if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, listViewSpecs, 
                (char *)viewPtr, objv[2], 0);
    }
    Tcl_Preserve(viewPtr);
    result = ConfigureListView(interp, viewPtr, objc - 2, objv + 2, 
                BLT_CONFIG_OBJV_ONLY);
    Tcl_Release(viewPtr);
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    viewPtr->flags |= (LAYOUT_PENDING | SCROLL_PENDING);
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      .cm cget option
 *
 *---------------------------------------------------------------------------
 */
static int
CgetOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    iconOption.clientData = viewPtr;
    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, listViewSpecs, 
        (char *)viewPtr, objv[2], 0);
}

/*ARGSUSED*/
static int
CurselectionOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (viewPtr->flags & SELECT_ORDERED) {
        Blt_ChainLink link;

        for (link = Blt_Chain_FirstLink(viewPtr->selected); link != NULL;
             link = Blt_Chain_NextLink(link)) {
            Item *itemPtr;
            Tcl_Obj *objPtr;

            itemPtr = Blt_Chain_GetValue(link);
            objPtr = Tcl_NewLongObj(itemPtr->index);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    } else {
        Item *itemPtr;

        for (itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
             itemPtr = NextItem(itemPtr)) {
            if (ItemIsSelected(viewPtr, itemPtr)) {
                Tcl_Obj *objPtr;

                objPtr = Tcl_NewLongObj(itemPtr->index);
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
 * DeleteOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      .cm delete item...
 *
 *---------------------------------------------------------------------------
 */
static int
DeleteOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int i;

    for (i = 2; i < objc; i++) {
        ItemIterator iter;
        Item *itemPtr, *nextPtr;

        if (GetItemIterator(interp, viewPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (itemPtr = FirstTaggedItem(&iter); itemPtr != NULL; 
             itemPtr = nextPtr) {
            nextPtr = NextTaggedItem(&iter);
            DestroyItem(itemPtr);
        }
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      .cm exists item 
 *
 *---------------------------------------------------------------------------
 */
static int
ExistsOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Item *itemPtr;
    int bool;

    bool = FALSE;
    if (GetItemFromObj(NULL, viewPtr, objv[2], &itemPtr) == TCL_OK) {
        if (itemPtr != NULL) {
            bool = TRUE;
        }
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FindOp --
 *
 *      Search for an item according to the string given.
 *
 * Results:
 *      The index of the found item is returned.  If no item is found
 *      -1 is returned.
 *
 *    .cm find string -from active -previous -all -count 1
 *    .cm find pattern \
 *      -type glob|regexp|exact \
 *      -from item -to item \
 *      -previous -wrap \
 *      -count 1
 *
 *---------------------------------------------------------------------------
 */
static int
FindOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    FindSwitches switches;
    Tcl_Obj *listObjPtr;
    const char *pattern;
    long count;

    /* Process switches  */
    pattern = Tcl_GetString(objv[2]);
    itemSwitch.clientData = viewPtr;
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, findSwitches, objc - 3, objv + 3, &switches,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (switches.fromPtr == NULL) {
        switches.fromPtr = FirstItem(viewPtr);
    } 
    if (switches.toPtr == NULL) {
        switches.toPtr = LastItem(viewPtr);
    } 
    if ((switches.fromPtr->index > switches.toPtr->index) &&
        ((switches.flags & FIND_REVERSE) == 0)) {
        switches.flags |= FIND_WRAP;
    }
    if ((switches.fromPtr->index < switches.toPtr->index) &&
        (switches.flags & FIND_REVERSE)) {
        switches.flags |= FIND_WRAP;
    }
    count = 0;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (switches.flags & FIND_REVERSE) {
        Item *itemPtr, *prevPtr;

        for (itemPtr = switches.fromPtr; itemPtr != NULL; itemPtr = prevPtr) {
            int found;

            prevPtr = PrevItem(itemPtr);
            if ((prevPtr == NULL) && (switches.flags & FIND_WRAP)) {
                itemPtr = LastItem(viewPtr);
            }
            if ((itemPtr->flags & ITEM_HIDDEN) && 
                ((switches.flags & FIND_HIDDEN) == 0)) {
                continue;
            }
            if ((itemPtr->flags & ITEM_DISABLED) && 
                ((switches.flags & FIND_DISABLED) == 0)) {
                continue;
            }
            if (switches.flags & FIND_EXACT) {
                found = (strcmp(itemPtr->text, pattern) == 0);
            } else if (switches.flags & FIND_REGEXP) {
                found = Tcl_RegExpMatch(NULL, itemPtr->text, pattern); 
            } else {
                found = Tcl_StringMatch(itemPtr->text, pattern);
            }
            if (found) {
                count++;
                Tcl_ListObjAppendElement(interp, listObjPtr,
                        Tcl_NewLongObj(itemPtr->index));
                if (switches.count == count) {
                    break;
                }
            }
            if ((itemPtr == switches.toPtr) && 
                ((switches.flags & FIND_WRAP) == 0)) {
                break;
            }
            if (prevPtr == switches.fromPtr) {
                break;
            }
        }
    } else {
        Item *itemPtr, *nextPtr;

        for (itemPtr = switches.fromPtr; itemPtr != NULL; itemPtr = nextPtr) {
            int found;
            
            nextPtr = NextItem(itemPtr);
            if ((nextPtr == NULL) && (switches.flags & FIND_WRAP)) {
                nextPtr = FirstItem(viewPtr);
            }
            if ((itemPtr->flags & ITEM_HIDDEN) && 
                ((switches.flags & FIND_HIDDEN) == 0)) {
                continue;
            }
            if ((itemPtr->flags & ITEM_DISABLED) && 
                ((switches.flags & FIND_DISABLED) == 0)) {
                continue;
            }
            if (switches.flags & FIND_EXACT) {
                found = (strcmp(itemPtr->text, pattern) == 0);
            } else if (switches.flags & FIND_REGEXP) {
                found = Tcl_RegExpMatch(NULL, itemPtr->text, pattern); 
            } else {
                found = Tcl_StringMatch(itemPtr->text, pattern);
            }
            if (found) {
                count++;
                Tcl_ListObjAppendElement(interp, listObjPtr,
                                         Tcl_NewLongObj(itemPtr->index));
                if (switches.count == count) {
                    break;
                }
            }
            if ((itemPtr == switches.toPtr) &&
                ((switches.flags & FIND_WRAP) == 0)) {
                break;
            }
            if (nextPtr == switches.fromPtr) {
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
 * FocusOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      .cm index item 
 *
 *---------------------------------------------------------------------------
 */
static int
FocusOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Item *itemPtr;
    int index;

    index = -1;
    if (GetItemFromObj(NULL, viewPtr, objv[2], &itemPtr) == TCL_OK) {
        viewPtr->focusPtr = itemPtr;
        if (itemPtr != NULL) {
            index = itemPtr->index;
        }
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IndexOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      .cm index item 
 *
 *---------------------------------------------------------------------------
 */
static int
IndexOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Item *itemPtr;
    int index;

    index = -1;
    if (GetItemFromObj(NULL, viewPtr, objv[2], &itemPtr) == TCL_OK) {
        if (itemPtr != NULL) {
            index = itemPtr->index;
        }
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
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
 *  .cm invoke item 
 *
 *---------------------------------------------------------------------------
 */
static int
InvokeOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Item *itemPtr;
    int result;

    if (GetItemFromObj(interp, viewPtr, objv[2], &itemPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((itemPtr == NULL) || (itemPtr->flags & ITEM_DISABLED)) {
        return TCL_OK;          /* Item is currently disabled. */
    }
    Tcl_Preserve(itemPtr);
    SelectItem(viewPtr, itemPtr);
    /*
     * We check numItems in addition to whether the item has a command because
     * that goes to zero if the listview is deleted (e.g., during command
     * evaluation).
     */
    result = TCL_OK;
    if ((Blt_Chain_GetLength(viewPtr->items) > 0) && 
        (itemPtr->cmdObjPtr != NULL)) {
        Tcl_IncrRefCount(itemPtr->cmdObjPtr);
        result = Tcl_EvalObjEx(interp, itemPtr->cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(itemPtr->cmdObjPtr);
    }
    Tcl_Release(itemPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * InsertOp --
 *
 *      Inserts a new item into the listview at the given index.
 *
 * Results:
 *      NULL is always returned.
 *
 * Side effects:
 *      The listview gets a new item.
 *
 *   .cm insert before 0 after 1 -text text 
 *
 *---------------------------------------------------------------------------
 */
static int
InsertOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Item *itemPtr, *wherePtr;
    int dir;
    static const char *dirs[] = { "after", "at", "before" , NULL};

    if (Tcl_GetIndexFromObj(interp, objv[2], dirs, "key", 0, &dir) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetItemFromObj(interp, viewPtr, objv[3], &wherePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (wherePtr == NULL) {
        Tcl_AppendResult(interp, "can't insert item: no index \"", 
                         Tcl_GetString(objv[3]), "\"", (char *)NULL);
        return TCL_ERROR;               /* No item. */
    }
    itemPtr = NewItem(viewPtr);
    if (ConfigureItem(interp, itemPtr, objc - 4, objv + 4, 0) != TCL_OK) {
        DestroyItem(itemPtr);
        return TCL_ERROR;               /* Error configuring the entry. */
    }
    MoveItem(viewPtr, itemPtr, dir, wherePtr);
    if (viewPtr->flags & SORT_AUTO) {
        viewPtr->flags |= SORT_PENDING;
    }
    viewPtr->flags |= LAYOUT_PENDING;
    viewPtr->flags &= ~SORTED;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ItemConfigureOp --
 *
 *      This procedure handles item operations.
 *
 * Results:
 *      A standard TCL result.
 *
 *      .cm item configure item ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
ItemConfigureOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    Item *itemPtr;
    ItemIterator iter;

    if (GetItemIterator(interp, viewPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    iconOption.clientData = viewPtr;
    for (itemPtr = FirstTaggedItem(&iter); itemPtr != NULL; 
         itemPtr = NextTaggedItem(&iter)) {
        int result;
        unsigned int flags;

        flags = BLT_CONFIG_OBJV_ONLY;
        if (objc == 4) {
            return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, itemSpecs, 
                (char *)itemPtr, (Tcl_Obj *)NULL, flags);
        } else if (objc == 5) {
            return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, itemSpecs, 
                (char *)itemPtr, objv[4], flags);
        }
        Tcl_Preserve(itemPtr);
        result = ConfigureItem(interp, itemPtr, objc - 4, objv + 4,  flags);
        Tcl_Release(itemPtr);
        if (result == TCL_ERROR) {
            return TCL_ERROR;
        }
    }
    viewPtr->flags |= (LAYOUT_PENDING | SCROLL_PENDING);
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ItemCgetOp --
 *
 *      This procedure handles item operations.
 *
 * Results:
 *      A standard TCL result.
 *
 *      .cm item cget item option
 *
 *---------------------------------------------------------------------------
 */
static int
ItemCgetOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Item *itemPtr;

    if (GetItemFromObj(interp, viewPtr, objv[3], &itemPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (itemPtr == NULL) {
        Tcl_AppendResult(interp, "can't retrieve item \"", 
                         Tcl_GetString(objv[3]), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    iconOption.clientData = viewPtr;
    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, itemSpecs,
        (char *)itemPtr, objv[4], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ItemOp --
 *
 *      This procedure handles item operations.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec itemOps[] = {
    {"cget",      2, ItemCgetOp,      5, 5, "item option",},
    {"configure", 2, ItemConfigureOp, 4, 0, "item ?option value?...",},
};
    
static int numItemOps = sizeof(itemOps) / sizeof(Blt_OpSpec);

static int
ItemOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ListViewCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numItemOps, itemOps, BLT_OP_ARG2, objc, objv, 
                0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (viewPtr, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *      .cm names pattern...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NamesOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;
    int i;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (i = 2; i < objc; i++) {
        const char *pattern;
        Item *itemPtr;

        pattern = Tcl_GetString(objv[i]);
        for (itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
             itemPtr = NextItem(itemPtr)) {
            if (Tcl_StringMatch(itemPtr->text, pattern)) {
                Tcl_Obj *objPtr;

                if (itemPtr->text == emptyString) {
                    objPtr = Tcl_NewStringObj("", -1);
                } else {
                    objPtr = Tcl_NewStringObj(itemPtr->text, -1);
                }
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*ARGSUSED*/
static int
NearestOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int x, y;                   
    int wx, wy;                 
    Item *itemPtr;
    int isRoot;
    char *string;

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
    if ((Tk_GetPixelsFromObj(interp, viewPtr->tkwin, objv[2], &x) != TCL_OK) ||
        (Tk_GetPixelsFromObj(interp, viewPtr->tkwin, objv[3], &y) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (isRoot) {
        int rootX, rootY;

        Tk_GetRootCoords(viewPtr->tkwin, &rootX, &rootY);
        x -= rootX;
        y -= rootY;
    }
    itemPtr = NearestItem(viewPtr, x, y, TRUE);
    if (itemPtr == NULL) {
        return TCL_OK;
    }
    x = WORLDX(viewPtr, x);
    y = WORLDY(viewPtr, y);
    wx = itemPtr->worldX + ITEM_XPAD;
    wy = itemPtr->worldY + ITEM_XPAD;
    if (objc > 4) {
        const char *where;

        where = "";
        if (itemPtr->icon != NULL) {
            int ix, iy, iw, ih;
            
            ih = IconHeight(itemPtr->icon);
            iw = IconWidth(itemPtr->icon);
            ix = wx;
            iy = wy;
            wx += viewPtr->iconWidth;
            if ((x >= ix) && (x <= (ix + iw)) && (y >= iy) && (y < (iy + ih))) {
                where = "icon";
                goto done;
            }
        }
        if ((itemPtr->text != emptyString) || (itemPtr->image != NULL)) {
            int lx, ly;

            lx = wx;
            ly = wy;

            wx += itemPtr->textWidth + ITEM_IPAD;
            if ((x >= lx) && (x < (lx + itemPtr->textWidth)) &&
                (y >= ly) && (y < (ly + itemPtr->textHeight))) {
                where = "text";
                goto done;
            }
        }
    done:
        if (Tcl_SetVar(interp, Tcl_GetString(objv[4]), where, 
                TCL_LEAVE_ERR_MSG) == NULL) {
            return TCL_ERROR;
        }
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), itemPtr->index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NextOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      .cm next item 
 *
 *---------------------------------------------------------------------------
 */
static int
NextOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Item *itemPtr;
    int index;

    index = -1;
    if (GetItemFromObj(NULL, viewPtr, objv[2], &itemPtr) == TCL_OK) {
        for (itemPtr = NextItem(itemPtr); itemPtr != NULL; 
             itemPtr = NextItem(itemPtr)) {
            if ((itemPtr->flags & (ITEM_HIDDEN|ITEM_DISABLED)) == 0) {
                break;
            }
        }
        if (itemPtr != NULL) {
            index = itemPtr->index;
        }
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PreviousOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      .cm previous item 
 *
 *---------------------------------------------------------------------------
 */
static int
PreviousOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Item *itemPtr;
    int index;

    index = -1;
    if (GetItemFromObj(NULL, viewPtr, objv[2], &itemPtr) == TCL_OK) {
        for (itemPtr = PrevItem(itemPtr); itemPtr != NULL; 
             itemPtr = PrevItem(itemPtr)) {
            if ((itemPtr->flags & (ITEM_HIDDEN|ITEM_DISABLED)) == 0) {
                break;
            }
        }
        if (itemPtr != NULL) {
            index = itemPtr->index;
        }
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
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
ScanOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int oper;
    int x, y;

#define SCAN_MARK       1
#define SCAN_DRAGTO     2
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
    if ((Blt_GetPixelsFromObj(interp, viewPtr->tkwin, objv[3], PIXELS_ANY, &x) 
         != TCL_OK) ||
        (Blt_GetPixelsFromObj(interp, viewPtr->tkwin, objv[4], PIXELS_ANY, &y) 
         != TCL_OK)) {
        return TCL_ERROR;
    }
    if (oper == SCAN_MARK) {
        viewPtr->scanAnchorX = x;
        viewPtr->scanAnchorY = y;
        viewPtr->scanX = viewPtr->xOffset;
        viewPtr->scanY = viewPtr->yOffset;
    } else {
        int xWorld, yWorld;
        int viewWidth, viewHeight;
        int dx, dy;

        dx = viewPtr->scanAnchorX - x;
        dy = viewPtr->scanAnchorY - y;
        xWorld = viewPtr->scanX + (10 * dx);
        yWorld = viewPtr->scanY + (10 * dy);

        viewWidth = VPORTWIDTH(viewPtr);
        if (xWorld > (viewPtr->worldWidth - viewWidth)) {
            xWorld = viewPtr->worldWidth - viewWidth;
        }
        if (xWorld < 0) {
            xWorld = 0;
        }
        viewHeight = VPORTHEIGHT(viewPtr);
        if (yWorld > (viewPtr->worldHeight - viewHeight)) {
            yWorld = viewPtr->worldHeight - viewHeight;
        }
        if (yWorld < 0) {
            yWorld = 0;
        }
        viewPtr->xOffset = xWorld;
        viewPtr->yOffset = yWorld;
        viewPtr->flags |= SCROLL_PENDING;
        EventuallyRedraw(viewPtr);
    }
    return TCL_OK;
}


/*ARGSUSED*/
static int
SeeOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Item *itemPtr;
    int x, y, width, height;
    int left, right, top, bottom;
    int maxWidth;

    if (GetItemFromObj(interp, viewPtr, objv[2], &itemPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (itemPtr == NULL) {
        return TCL_OK;
    }
    if (itemPtr->flags & ITEM_HIDDEN) {
        return TCL_OK;
    }
    width = VPORTWIDTH(viewPtr);
    height = VPORTHEIGHT(viewPtr);
    x = viewPtr->xOffset;
    y = viewPtr->yOffset;

    /*
     * XVIEW:   If the entry is left or right of the current view, adjust
     *          the offset.  If the entry is nearby, adjust the view just
     *          a bit.  Otherwise, center the entry.
     */
    left = viewPtr->xOffset;
    right = viewPtr->xOffset + width;
    top = viewPtr->yOffset;
    bottom = viewPtr->yOffset + height;
    
    maxWidth = itemPtr->width;
    if (viewPtr->maxWidth < maxWidth) {
        maxWidth = viewPtr->maxWidth;
    }
    if (itemPtr->worldX < left) {
        /* Adjust the scroll so that item starts at the left border. */
        x = itemPtr->worldX;
    } else if ((itemPtr->worldX + maxWidth) > right) {
        /* Adjust the scroll so that item ends at the right border. */
        x = itemPtr->worldX + maxWidth - width;
    }
    if (itemPtr->worldY < top) {
        /* Adjust the scroll so that item starts at the top border. */
        y = itemPtr->worldY;
    } else if ((itemPtr->worldY + itemPtr->height) > bottom) {
        /* Adjust the scroll so that item ends at the right border. */
        y = itemPtr->worldY + itemPtr->height - height;
    }
    if ((y != viewPtr->yOffset) || (x != viewPtr->xOffset)) {
        viewPtr->xOffset = x; 
        viewPtr->yOffset = y;
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
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionAnchorOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    Item *itemPtr;
    long index;

    index = -1;
    if (objc == 3) {
        if (viewPtr->selAnchorPtr != NULL) {
            index = viewPtr->selAnchorPtr->index;
        }
        Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
        return TCL_OK;
    }
    if (GetItemFromObj(interp, viewPtr, objv[3], &itemPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    /* Set both the anchor and the mark. Indicates that a single entry
     * is selected. */
    viewPtr->selAnchorPtr = itemPtr;
    viewPtr->selMarkPtr = NULL;
    if (itemPtr != NULL) {
        index = itemPtr->index;
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
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
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionClearallOp(ListView *viewPtr, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const *objv)
{
    ClearSelection(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionIncludesOp
 *
 *      Returns 1 if the element indicated by index is currently selected, 0
 *      if it isn't.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionIncludesOp(ListView *viewPtr, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const *objv)
{
    Item *itemPtr;
    int bool;

    if (GetItemFromObj(interp, viewPtr, objv[3], &itemPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    bool = FALSE;
    if (itemPtr == NULL) {
        bool = ItemIsSelected(viewPtr, itemPtr);
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
 *      dragging out a selection with the mouse.  The index "mark" may be used
 *      to refer to the anchor element.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionMarkOp(ListView *viewPtr, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    Item *itemPtr;
    long index;

    index = -1;
    if (objc == 3) {
        if (viewPtr->selMarkPtr != NULL) {
            index = viewPtr->selMarkPtr->index;
        }
        Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
        return TCL_OK;
    }
    if (GetItemFromObj(interp, viewPtr, objv[3], &itemPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (viewPtr->selAnchorPtr == NULL) {
        Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
        return TCL_OK;                  /* The selection anchor must be set
                                         * first. But don't flag this as
                                         * an error. */
    }
    if ((itemPtr != NULL) && (viewPtr->selMarkPtr != itemPtr)) {
        Blt_ChainLink link, prev;

        /* Deselect entry from the list all the way back to the anchor. */
        for (link = Blt_Chain_LastLink(viewPtr->selected); link != NULL; 
             link = prev) {
            Item *selectPtr;

            prev = Blt_Chain_PrevLink(link);
            selectPtr = Blt_Chain_GetValue(link);
            if (selectPtr == viewPtr->selAnchorPtr) {
                break;
            }
            DeselectItem(viewPtr, selectPtr);
        }
        viewPtr->flags &= ~SELECT_MASK;
        viewPtr->flags |= SELECT_SET;
        SelectRange(viewPtr, viewPtr->selAnchorPtr, itemPtr);
        viewPtr->selMarkPtr = itemPtr;
        EventuallyRedraw(viewPtr);
        index = itemPtr->index;
        if (viewPtr->selCmdObjPtr != NULL) {
            EventuallyInvokeSelectCmd(viewPtr);
        }
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), index);
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
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionPresentOp(ListView *viewPtr, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    int bool;

    bool = (Blt_Chain_GetLength(viewPtr->selected) > 0);
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
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionSetOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    char *string;

    viewPtr->flags &= ~SELECT_MASK;
    if (viewPtr->flags & LAYOUT_PENDING) {
        /*
         * The layout is dirty.  Recompute it now so that we can use
         * view.top and view.bottom for nodes.
         */
        ComputeLayout(viewPtr);
    }
    string = Tcl_GetString(objv[2]);
    switch (string[0]) {
    case 's':
        viewPtr->flags |= SELECT_SET;
        break;
    case 'c':
        viewPtr->flags |= SELECT_CLEAR;
        break;
    case 't':
        viewPtr->flags |= SELECT_TOGGLE;
        break;
    }
    if (objc > 4) {
        Item *firstPtr, *lastPtr;

        if (GetItemFromObj(interp, viewPtr, objv[3], &firstPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (firstPtr == NULL) {
            return TCL_OK;              /* Didn't pick an entry. */
        }
        if ((firstPtr->flags & ITEM_HIDDEN) && 
            ((viewPtr->flags & SELECT_CLEAR) == 0)) {
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
            if (GetItemFromObj(interp, viewPtr, objv[4], &lastPtr) != TCL_OK) {
                return TCL_ERROR;
            }
            if (lastPtr == NULL) {
                return TCL_OK;
            }
            if ((lastPtr->flags & ITEM_HIDDEN) && 
                ((viewPtr->flags & SELECT_CLEAR) == 0)) {
                Tcl_AppendResult(interp, "can't select hidden node \"", 
                        Tcl_GetString(objv[4]), "\"", (char *)NULL);
                return TCL_ERROR;
            }
        }
        if (firstPtr == lastPtr) {
            SelectItemUsingFlags(viewPtr, firstPtr);
        } else {
            SelectRange(viewPtr, firstPtr, lastPtr);
        }
        /* Set both the anchor and the mark. Indicates that a single entry is
         * selected. */
        if (viewPtr->selAnchorPtr == NULL) {
            viewPtr->selAnchorPtr = firstPtr;
        }
    } else {
        Item *itemPtr;
        ItemIterator iter;

        if (GetItemIterator(interp, viewPtr, objv[3], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (itemPtr = FirstTaggedItem(&iter); itemPtr != NULL; 
             itemPtr = NextTaggedItem(&iter)) {
            if ((itemPtr->flags & ITEM_HIDDEN) && 
                ((viewPtr->flags & SELECT_CLEAR) == 0)) {
                continue;
            }
            SelectItemUsingFlags(viewPtr, itemPtr);
        }
        /* Set both the anchor and the mark. Indicates that a single entry is
         * selected. */
        if (viewPtr->selAnchorPtr == NULL) {
            viewPtr->selAnchorPtr = itemPtr;
        }
    }
    if (viewPtr->flags & SELECT_EXPORT) {
        Tk_OwnSelection(viewPtr->tkwin, XA_PRIMARY, LostSelection, viewPtr);
    }
    EventuallyRedraw(viewPtr);
    if (viewPtr->selCmdObjPtr != NULL) {
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
 *      The selected text is designated by start and end indices into the text
 *      pool.  The selected segment has both a anchored and unanchored ends.
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
    {"anchor",   1, SelectionAnchorOp,   3, 4, "?item?",},
    {"clear",    5, SelectionSetOp,      4, 5, "first ?last?",},
    {"clearall", 6, SelectionClearallOp, 3, 3, "",},
    {"includes", 1, SelectionIncludesOp, 4, 4, "item",},
    {"mark",     1, SelectionMarkOp,     3, 4, "?item?",},
    {"present",  1, SelectionPresentOp,  3, 3, "",},
    {"set",      1, SelectionSetOp,      4, 5, "first ?last?",},
    {"toggle",   1, SelectionSetOp,      4, 5, "first ?last?",},
};
static int numSelectionOps = sizeof(selectionOps) / sizeof(Blt_OpSpec);

static int
SelectionOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    ListViewCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numSelectionOps, selectionOps, BLT_OP_ARG2, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (viewPtr, interp, objc, objv);
    return result;
}

/*ARGSUSED*/
static int
SizeOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_SetLongObj(Tcl_GetObjResult(interp), 
                   Blt_Chain_GetLength(viewPtr->items));
    return TCL_OK;
}


static int
InvokeCompare(ListView *viewPtr, const char *s1, const char *s2, 
              Tcl_Obj *cmdObjPtr)
{
    Tcl_Interp *interp;
    Tcl_Obj *objPtr;
    int result;

    interp = viewPtr->interp;
    cmdObjPtr = Tcl_DuplicateObj(cmdObjPtr);
    objPtr = Tcl_NewStringObj(Tk_PathName(viewPtr->tkwin), -1);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    objPtr = Tcl_NewStringObj(s1, -1);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    objPtr = Tcl_NewStringObj(s2, -1);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    Tcl_IncrRefCount(cmdObjPtr);
    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(cmdObjPtr);
    if ((result != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp),&result)!=TCL_OK)) {
        Tcl_BackgroundError(interp);
    }
    Tcl_ResetResult(interp);
    return result;
}

static ListView *listViewInstance;

static int
CompareLinks(Blt_ChainLink *aPtr, Blt_ChainLink *bPtr)
{
    ListView *viewPtr;
    Item *i1Ptr, *i2Ptr;
    const char *s1, *s2;
    int result;

    i1Ptr = (Item *)Blt_Chain_GetValue(*aPtr);
    i2Ptr = (Item *)Blt_Chain_GetValue(*bPtr);
    viewPtr = i1Ptr->viewPtr;
    if (viewPtr->flags & SORT_BY_TYPE) {
        s1 = i1Ptr->type;
        s2 = i2Ptr->type;
    } else {
        s1 = i1Ptr->text;
        s2 = i2Ptr->text;
    } 
    if (s1 == NULL) {
        s1 = "";
    }
    if (s2 == NULL) {
        s2 = "";
    }
    result = 0;
    if (viewPtr->sortCmdPtr != NULL) {
        result = InvokeCompare(viewPtr, s1, s2, viewPtr->sortCmdPtr);
        if (viewPtr->flags & SORT_DECREASING) {
            result = -result;
        } 
        return result;
    }
    if (viewPtr->flags & SORT_DICTIONARY) {
        result = Blt_DictionaryCompare(s1, s2);
    } else {
        result = strcmp(s1, s2);
    }
    if (result == 0) {
        if (viewPtr->flags & SORT_BY_TYPE) {
            s1 = i1Ptr->text;
            s2 = i2Ptr->text;
        } else {
            s1 = i1Ptr->type;
            s2 = i2Ptr->type;
        } 
        if (viewPtr->flags & SORT_DICTIONARY) {
            result = Blt_DictionaryCompare(s1, s2);
        } else {
            result = strcmp(s1, s2);
        }
    }   
    if (viewPtr->flags & SORT_DECREASING) {
        return -result;
    } 
    return result;
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
SortCgetOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
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
 *        .h sort configure option value
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
 *---------------------------------------------------------------------------
 */
static int
SortConfigureOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    int oldType;
    Tcl_Obj *oldCmdPtr;

    if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, sortSpecs, 
                (char *)viewPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, sortSpecs, 
                (char *)viewPtr, objv[3], 0);
    }
    oldType = viewPtr->flags & SORT_BY_MASK;
    oldCmdPtr = viewPtr->sortCmdPtr;
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, sortSpecs, 
        objc - 3, objv + 3, (char *)viewPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((oldType != (viewPtr->flags & SORT_BY_MASK)) ||
        (oldCmdPtr != viewPtr->sortCmdPtr)) {
        viewPtr->flags &= ~SORTED;
    } 
    if (viewPtr->flags & SORT_AUTO) {
        viewPtr->flags |= SORT_PENDING;
    }
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*ARGSUSED*/
static int
SortNowOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, sortSpecs, 
        objc - 3, objv + 3, (char *)viewPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    viewPtr->flags |= SORT_PENDING;
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
 *      .h sort now
 *
 * Results:
 *      1 is the first is greater, -1 is the second is greater, 0
 *      if equal.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec sortOps[] =
{
    {"cget",      2, SortCgetOp,      4, 4, "option",},
    {"configure", 2, SortConfigureOp, 3, 0, "?option value?...",},
    {"now",       1, SortNowOp,       3, 0, "?option value?",},
};
static int numSortOps = sizeof(sortOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
SortOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ListViewCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numSortOps, sortOps, BLT_OP_ARG2, objc, 
            objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (viewPtr, interp, objc, objv);
    return result;
}

/* .m style create name option value option value */
    
static int
StyleCreateOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    Style *stylePtr;
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&viewPtr->styleTable, Tcl_GetString(objv[3]),
                &isNew);
    if (!isNew) {
        Tcl_AppendResult(interp, "listview style \"", Tcl_GetString(objv[3]),
                "\" already exists.", (char *)NULL);
        return TCL_ERROR;
    }
    stylePtr = Blt_AssertCalloc(1, sizeof(Style));
    stylePtr->name = Blt_GetHashKey(&viewPtr->styleTable, hPtr);
    stylePtr->hPtr = hPtr;
    stylePtr->viewPtr = viewPtr;
    stylePtr->borderWidth = 0;
    stylePtr->activeRelief = TK_RELIEF_RAISED;
    Blt_SetHashValue(hPtr, stylePtr);
    iconOption.clientData = viewPtr;
    if (ConfigureStyle(interp, stylePtr, objc - 4, objv + 4, 0) != TCL_OK) {
        DestroyStyle(stylePtr);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
StyleCgetOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Style *stylePtr;

    if (GetStyleFromObj(interp, viewPtr, objv[3], &stylePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    iconOption.clientData = viewPtr;
    return Blt_ConfigureValueFromObj(interp, viewPtr->tkwin, styleSpecs,
        (char *)stylePtr, objv[4], 0);
}

static int
StyleConfigureOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    int result, flags;
    Style *stylePtr;

    if (GetStyleFromObj(interp, viewPtr, objv[3], &stylePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    iconOption.clientData = viewPtr;
    flags = BLT_CONFIG_OBJV_ONLY;
    if (objc == 1) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, 
                styleSpecs, (char *)stylePtr, (Tcl_Obj *)NULL, flags);
    } else if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, viewPtr->tkwin, 
                styleSpecs, (char *)stylePtr, objv[2], flags);
    }
    Tcl_Preserve(stylePtr);
    result = ConfigureStyle(interp, stylePtr, objc - 4, objv + 4, flags);
    Tcl_Release(stylePtr);
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    viewPtr->flags |= (LAYOUT_PENDING | SCROLL_PENDING);
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

static int
StyleDeleteOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    Style *stylePtr;

    if (GetStyleFromObj(interp, viewPtr, objv[3], &stylePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (stylePtr->refCount > 0) {
        Tcl_AppendResult(interp, "can't destroy listview style \"", 
                         stylePtr->name, "\": style in use.", (char *)NULL);
        return TCL_ERROR;
    }
    DestroyStyle(stylePtr);
    return TCL_OK;
}

static int
StyleNamesOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hPtr = Blt_FirstHashEntry(&viewPtr->styleTable, &iter); 
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
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
StyleOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ListViewCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numStyleOps, styleOps, BLT_OP_ARG2, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (viewPtr, interp, objc, objv);
    return result;
}


/*
 *---------------------------------------------------------------------------
 *
 * TableLinkOp --
 *
 *      Attaches a table as a data source for this widget.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      New items are added to the listview widget.
 *
 *      .t table link $t ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
TableLinkOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    int count;
    Tcl_Obj **names;
    BLT_TABLE table;

    if (blt_table_open(interp, Tcl_GetString(objv[3]), &table) != TCL_OK) {
        return TCL_ERROR;
    }
    if (viewPtr->tableSource.table != NULL) {   
        /* Flush all the items. */
        DestroyItems(viewPtr);
        viewPtr->items = Blt_Chain_Create();
        /* Notifiers and traces are also removed when the table is closed. */
        blt_table_close(viewPtr->tableSource.table);
    }
    viewPtr->tableSource.table = table;
    columnOption.clientData = viewPtr;
    if (Blt_ConfigureWidgetFromObj(interp, viewPtr->tkwin, tableSpecs, 
        objc - 4, objv + 4, (char *)&viewPtr->tableSource, 0) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_ListObjGetElements(interp, objv[2], &count, &names) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RebuildTableItems(interp, viewPtr, table) != TCL_OK) {
        return TCL_ERROR;
    }
    if (viewPtr->flags & SORT_AUTO) {
        viewPtr->flags |= SORT_PENDING;
    }
    viewPtr->flags |= LAYOUT_PENDING;
    viewPtr->flags &= ~SORTED;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TableUnlinkOp --
 *
 *      Appends a list of items to the listview.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      Items are removed from the listview widget.
 *
 *      .t table unlink
 *
 *---------------------------------------------------------------------------
 */
static int
TableUnlinkOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    if (viewPtr->tableSource.table != NULL) {
        /* Flush all the items. */
        DestroyItems(viewPtr);
        viewPtr->items = Blt_Chain_Create();
        /* Notifiers and traces are also removed when the table is closed. */
        blt_table_close(viewPtr->tableSource.table);
        viewPtr->tableSource.table = NULL;
        EventuallyRedraw(viewPtr);
    }
    return TCL_OK;
}

static Blt_OpSpec tableOps[] =
{
    {"link",    1, TableLinkOp,   4, 0, "table ?option value?",},
    {"unlink",  1, TableUnlinkOp, 3, 3, "",},
};

static int numTableOps = sizeof(tableOps) / sizeof(Blt_OpSpec);

static int
TableOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ListViewCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numTableOps, tableOps, BLT_OP_ARG2, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (viewPtr, interp, objc, objv);
    return result;
}


static int
XpositionOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Item *itemPtr;

    if (GetItemFromObj(interp, viewPtr, objv[3], &itemPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (itemPtr == NULL) {
        Tcl_AppendResult(interp, "can't get x-position of item: no item \"", 
                         Tcl_GetString(objv[3]), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), itemPtr->worldX-viewPtr->xOffset);
    return TCL_OK;
}

static int
XviewOp(ListView *viewPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int w;

    w = VPORTWIDTH(viewPtr);
    if (objc == 2) {
        double fract;
        Tcl_Obj *listObjPtr, *objPtr;

        /* Report first and last fractions */
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        /*
         * Note: we are bounding the fractions between 0.0 and 1.0 to support
         * the "canvas"-style of scrolling.
         */
        fract = (double)viewPtr->xOffset / (viewPtr->worldWidth+1);
        objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        fract = (double)(viewPtr->xOffset + w) / (viewPtr->worldWidth+1);
        objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &viewPtr->xOffset,
        viewPtr->worldWidth, w, viewPtr->xScrollUnits, 
        BLT_SCROLL_MODE_HIERBOX) != TCL_OK) {
        return TCL_ERROR;
    }
    viewPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

static int
YpositionOp(ListView *viewPtr, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Item *itemPtr;

    if (GetItemFromObj(interp, viewPtr, objv[3], &itemPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (itemPtr == NULL) {
        Tcl_AppendResult(interp, "can't get y-position of item: such index \"", 
                         Tcl_GetString(objv[3]), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), itemPtr->worldY-viewPtr->yOffset);
    return TCL_OK;
}

static int
YviewOp(
    ListView *viewPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    int height;

    height = VPORTHEIGHT(viewPtr);
    if (objc == 2) {
        double fract;
        Tcl_Obj *listObjPtr, *objPtr;

        /* Report first and last fractions */
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        /*
         * Note: we are bounding the fractions between 0.0 and 1.0 to support
         * the "canvas"-style of scrolling.
         */
        fract = (double)viewPtr->yOffset / (viewPtr->worldHeight+1);
        objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        fract = (double)(viewPtr->yOffset + height) /(viewPtr->worldHeight+1);
        objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &viewPtr->yOffset,
        viewPtr->worldHeight, height, viewPtr->yScrollUnits, 
        BLT_SCROLL_MODE_HIERBOX) != TCL_OK) {
        return TCL_ERROR;
    }
    viewPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(viewPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyListView --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the internal structure of the widget at a safe time (when
 *      no-one is using it anymore).
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Everything associated with the widget is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyListView(DestroyData dataPtr)    /* Pointer to the widget record. */
{
    ListView *viewPtr = (ListView *)dataPtr;

    DestroyItems(viewPtr);
    DestroyStyles(viewPtr);
    DestroyTexts(viewPtr);
    Blt_Tags_Reset(&viewPtr->tags);
    DestroyIcons(viewPtr);
    if (viewPtr->painter != NULL) {
        Blt_FreePainter(viewPtr->painter);
    }
    if (viewPtr->focusGC != NULL) {
        Tk_FreeGC(viewPtr->display, viewPtr->focusGC);
    }
    if (viewPtr->copyGC != NULL) {
        Tk_FreeGC(viewPtr->display, viewPtr->copyGC);
    }
    iconOption.clientData = viewPtr;
    Blt_FreeOptions(listViewSpecs, (char *)viewPtr, viewPtr->display, 0);
    Tcl_DeleteCommandFromToken(viewPtr->interp, viewPtr->cmdToken);
    Blt_Free(viewPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewListView --
 *
 *---------------------------------------------------------------------------
 */
static ListView *
NewListView(Tcl_Interp *interp, Tk_Window tkwin)
{
    ListView *viewPtr;

    viewPtr = Blt_AssertCalloc(1, sizeof(ListView));

    Tk_SetClass(tkwin, "BltListView");

    viewPtr->tkwin = tkwin;
    viewPtr->display = Tk_Display(tkwin);
    viewPtr->interp = interp;
    viewPtr->flags = LAYOUT_PENDING | SCROLL_PENDING | SELECT_EXPORT | 
        SELECT_ORDERED;
    viewPtr->relief = TK_RELIEF_SUNKEN;
    viewPtr->xScrollUnits = viewPtr->yScrollUnits = 20;
    viewPtr->borderWidth = 1;
    viewPtr->highlightWidth = 2;
    viewPtr->items = Blt_Chain_Create();
    viewPtr->layoutMode = LAYOUT_LIST_COLUMN;
    viewPtr->painter = Blt_GetPainter(tkwin, 1.0);
    Blt_ResetLimits(&viewPtr->reqWidth);
    Blt_ResetLimits(&viewPtr->reqHeight);
    Blt_InitHashTable(&viewPtr->iconTable,  BLT_STRING_KEYS);
    Blt_InitHashTable(&viewPtr->textTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&viewPtr->styleTable, BLT_STRING_KEYS);
    Blt_Tags_Init(&viewPtr->tags);
    Blt_InitHashTable(&viewPtr->selTable, BLT_ONE_WORD_KEYS);
    viewPtr->selected = Blt_Chain_Create();
    AddDefaultStyle(interp, viewPtr);
    Blt_SetWindowInstanceData(tkwin, viewPtr);
    return viewPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ListViewCmd --
 *
 *      This procedure is invoked to process the "listview" command.  See the
 *      user documentation for details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec listViewOps[] =
{
    {"activate",    2, ActivateOp,    3, 3, "item",},
    {"add",         2, AddOp,         2, 0, "?option value?",},
    {"cget",        2, CgetOp,        3, 3, "option",},
    {"configure",   2, ConfigureOp,   2, 0, "?option value?...",},
    {"curselection",2, CurselectionOp,2, 2, "",},
    {"delete",      1, DeleteOp,      2, 0, "items...",},
    {"exists",      1, ExistsOp,      3, 3, "item",},
    {"find",        2, FindOp,        3, 0, "string ?switches?",},
    {"focus",       2, FocusOp,       3, 3, "item",},
    {"index",       3, IndexOp,       3, 3, "item",},
    {"insert",      3, InsertOp,      3, 0, 
        "after|at|before index ?option value?",},
    {"invoke",      3, InvokeOp,      3, 3, "item",},
    {"item",        2, ItemOp,        2, 0, "oper args",},
    {"listadd",     1, AddListOp,     3, 0, "textList ?option value?",},
    {"names",       2, NamesOp,       2, 0, "?pattern...?",},
    {"nearest",     3, NearestOp,     4, 4, "x y",},
    {"next",        3, NextOp,        3, 3, "item",},
    {"previous",    1, PreviousOp,    3, 3, "item",},
    {"scan",        2, ScanOp,        5, 5, "dragto|mark x y",},
    {"see",         3, SeeOp,         3, 3, "item",},
    {"selection",   3, SelectionOp,   2, 0, "op ?args?",},
    {"size",        2, SizeOp,        2, 2, "",},
    {"sort",        2, SortOp,        2, 0, "op ?args...?",},
    {"style",       2, StyleOp,       2, 0, "op ?args...?",},
    {"table",       1, TableOp,       2, 0, "op ?args...?",},
    {"xposition",   2, XpositionOp,   3, 3, "item",},
    {"xview",       2, XviewOp,       2, 5, 
        "?moveto fract? ?scroll number what?",},
    {"yposition",   2, YpositionOp,   3, 3, "item",},
    {"yview",       2, YviewOp,       2, 5, 
        "?moveto fract? ?scroll number what?",},
};

static int numListViewOps = sizeof(listViewOps) / sizeof(Blt_OpSpec);

typedef int (ListViewInstOp)(ListView *viewPtr, Tcl_Interp *interp, int objc,
                          Tcl_Obj *const *objv);

static int
ListViewInstCmdProc(
    ClientData clientData,              /* Information about the widget. */
    Tcl_Interp *interp,                 /* Interpreter to report errors back
                                         * to. */
    int objc,                           /* # of arguments. */
    Tcl_Obj *const *objv)               /* Argument vector. */
{
    ListViewInstOp *proc;
    ListView *viewPtr = clientData;
    int result;

    proc = Blt_GetOpFromObj(interp, numListViewOps, listViewOps, BLT_OP_ARG1, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    Tcl_Preserve(viewPtr);
    result = (*proc) (viewPtr, interp, objc, objv);
    Tcl_Release(viewPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ListViewInstCmdDeletedProc --
 *
 *      This procedure can be called if the window was destroyed (tkwin will
 *      be NULL) and the command was deleted automatically.  In this case, we
 *      need to do nothing.
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
ListViewInstCmdDeletedProc(ClientData clientData)
{
    ListView *viewPtr = clientData;     /* Pointer to widget record. */

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
 * ListViewCmd --
 *
 *      This procedure is invoked to process the TCL command that corresponds
 *      to a widget managed by this module. See the user documentation for
 *      details on what it does.
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
ListViewCmd(
    ClientData clientData,              /* Main window associated with
                                         * interpreter. */
    Tcl_Interp *interp,                 /* Current interpreter. */
    int objc,                           /* Number of arguments. */
    Tcl_Obj *const *objv)               /* Argument strings. */
{
    ListView *viewPtr;
    Tk_Window tkwin;
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
     * various bindings on the listview widget.  If the procedure doesn't
     * already exist, source it from "$blt_library/bltListView.tcl".  We
     * deferred sourcing the file until now so that the variable $blt_library
     * could be set within a script.
     */
    if (!Blt_CommandExists(interp, "::blt::ListView::AutoScroll")) {
        if (Tcl_GlobalEval(interp, 
                "source [file join $blt_library bltListView.tcl]") != TCL_OK) {
            char info[200];
            Blt_FormatString(info, 200, "\n    (while loading bindings for %.50s)", 
                    Tcl_GetString(objv[0]));
            Tcl_AddErrorInfo(interp, info);
            return TCL_ERROR;
        }
    }
    path = Tcl_GetString(objv[1]);
    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), path, NULL);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    viewPtr = NewListView(interp, tkwin);
    if (ConfigureListView(interp, viewPtr, objc - 2, objv + 2, 0) != TCL_OK) {
        Tk_DestroyWindow(viewPtr->tkwin);
        return TCL_ERROR;
    }
    mask = (ExposureMask | StructureNotifyMask | FocusChangeMask);
    Tk_CreateEventHandler(tkwin, mask, ListViewEventProc, viewPtr);
    Tk_CreateSelHandler(tkwin, XA_PRIMARY, XA_STRING, SelectionProc, viewPtr, 
        XA_STRING);
    viewPtr->cmdToken = Tcl_CreateObjCommand(interp, path, 
        ListViewInstCmdProc, viewPtr, ListViewInstCmdDeletedProc);

    Tcl_SetObjResult(interp, objv[1]);
    return TCL_OK;
}

int
Blt_ListViewInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec[1] = { 
        { "listview", ListViewCmd }, 
    };
    return Blt_InitCmds(interp, "::blt", cmdSpec, 1);
}

static void
DrawItemBackground(Item *itemPtr, Drawable drawable, int x, int y)
{
    Blt_Bg bg;
    Style *stylePtr;
    ListView *viewPtr;    

    stylePtr = itemPtr->stylePtr;
    viewPtr = itemPtr->viewPtr;
    if (itemPtr->flags & ITEM_DISABLED) {
        bg = stylePtr->disabledBg;
    } else if (viewPtr->activePtr == itemPtr) {
        bg = stylePtr->activeBg;
    } else {
        bg = stylePtr->normalBg;
    }       
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, 
        itemPtr->worldWidth, itemPtr->worldHeight, stylePtr->borderWidth, 
        itemPtr->relief);
}

static void
DrawItem(Item *itemPtr, Drawable drawable, int x, int y)
{
    ListView *viewPtr;    
    Style *stylePtr;
    int textWidth;
    Icon icon;

    itemPtr->flags &= ~ITEM_REDRAW;
    stylePtr = itemPtr->stylePtr;
    viewPtr = itemPtr->viewPtr;

    textWidth = itemPtr->textWidth;
    if ((viewPtr->textWidth>0) && (viewPtr->textWidth<itemPtr->textWidth)) {
        textWidth = viewPtr->textWidth;
    }
    x += itemPtr->indent;
    /* Icon. */
    icon = (viewPtr->layoutMode == LAYOUT_ICONS) ? itemPtr->bigIcon : 
        itemPtr->icon;
    if (icon != NULL) {
        if ((Blt_IsPicture(IconImage(icon))) && 
            (itemPtr->flags & ITEM_DISABLED)) {
            Blt_Picture src, dst;
            Blt_Painter painter;
            int w, h;
            
            painter = Blt_GetPainter(viewPtr->tkwin, 1.0);
            src = Blt_GetPictureFromPictureImage(viewPtr->interp, 
                IconImage(icon));
            w = Blt_Picture_Width(src);
            h = Blt_Picture_Height(src);
            dst = Blt_ClonePicture(src);
            Blt_FadePicture(dst, 0, 0, w, h, 100 / 255.0);
            Blt_PaintPicture(painter, drawable, dst, 0, 0, 
                             IconWidth(icon), IconHeight(icon), 
                             x + itemPtr->iconX, y + itemPtr->iconY, 0);
            Blt_FreePicture(dst);
        } else {
            Tk_RedrawImage(IconImage(icon), 0, 0, IconWidth(icon), 
                        IconHeight(icon), drawable, x + itemPtr->iconX, 
                        y + itemPtr->iconY);
        }
    }
    /* Image or text. */
    if (ItemIsSelected(viewPtr, itemPtr)) {
        Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, 
                stylePtr->selectBg, 
                x + itemPtr->textX - 3, y + itemPtr->textY - 1,
                textWidth + 6, itemPtr->textHeight + 3, 
                stylePtr->borderWidth, stylePtr->selectRelief);
    }
    if (itemPtr->image != NULL) {
        Tk_RedrawImage(IconImage(itemPtr->image), 0, 0, 
                IconWidth(itemPtr->image), IconHeight(itemPtr->image), 
                drawable, x + itemPtr->textX, y + itemPtr->textY);
    } else if (itemPtr->text != emptyString) {
        TextStyle ts;
        XColor *fg;

        if (itemPtr->flags & ITEM_DISABLED) {
            fg = stylePtr->textDisabledColor;
        } else if (ItemIsSelected(viewPtr, itemPtr)) {
            fg = stylePtr->textSelectColor;
        } else if (viewPtr->activePtr == itemPtr) {
            fg = stylePtr->textActiveColor;
        } else {
            fg = stylePtr->textNormalColor;
        }
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, itemPtr->stylePtr->textFont);
        Blt_Ts_SetForeground(ts, fg);
        Blt_Ts_SetAnchor(ts, TK_ANCHOR_NW);
        Blt_Ts_SetJustify(ts, TK_JUSTIFY_LEFT);
        Blt_Ts_SetMaxLength(ts, textWidth);
        Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, itemPtr->layoutPtr, &ts,
                x + itemPtr->stylePtr->borderWidth + itemPtr->textX, 
                y + itemPtr->stylePtr->borderWidth +  itemPtr->textY);
        if (itemPtr == viewPtr->activePtr) {
            Blt_Ts_UnderlineLayout(viewPtr->tkwin, drawable, itemPtr->layoutPtr,
                 &ts, x + stylePtr->borderWidth + itemPtr->textX, 
             y + stylePtr->borderWidth + itemPtr->textY);
        }
    }
    if ((viewPtr->flags & FOCUS) && (viewPtr->focusPtr == itemPtr)) {
        if (ItemIsSelected(viewPtr, itemPtr)) {
            XSetForeground(viewPtr->display, viewPtr->focusGC, 
                stylePtr->textSelectColor->pixel);
        } else {
            XSetForeground(viewPtr->display, viewPtr->focusGC, 
                           stylePtr->textNormalColor->pixel);
        }
        XDrawRectangle(viewPtr->display, drawable, viewPtr->focusGC, 
                x - 2 + stylePtr->borderWidth + itemPtr->textX, 
                y - 2 + stylePtr->borderWidth + itemPtr->textY,
                textWidth + 3  - 2 * stylePtr->borderWidth, 
                itemPtr->textHeight + 3 - 2 * stylePtr->borderWidth);
    }
}

static void
DrawListView(ListView *viewPtr, Drawable drawable)
{
    int left, right, top, bottom;

    /* Draw each visible item. */
    Item *itemPtr;

    left = viewPtr->inset;
    right = VPORTWIDTH(viewPtr);
    top = viewPtr->inset;
    bottom = VPORTHEIGHT(viewPtr);

    for (itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
         itemPtr = NextItem(itemPtr)) {
        int x, y;
        
        if (itemPtr->flags & ITEM_HIDDEN) {
            continue;
        }
        x = SCREENX(viewPtr, itemPtr->worldX);
        y = SCREENY(viewPtr, itemPtr->worldY);
        if ((x > right) || ((x + itemPtr->width) < left) ||
            (y > bottom) || ((y + itemPtr->height) < top)) {
            continue;
        }
        DrawItemBackground(itemPtr, drawable, x, y);
        DrawItem(itemPtr, drawable, x, y);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayItem --
 *
 *      This procedure is invoked to display an item in the listview widget.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Commands are output to X to display the item.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayItem(ClientData clientData)
{
    Item *itemPtr = clientData;
    int x, y, w, h;
    Pixmap drawable;
    ListView *viewPtr;
    int sx, sy, x1, x2, y1, y2;

    /*
     * Create a pixmap the size of the item for double buffering.
     */
    viewPtr = itemPtr->viewPtr;
    h = itemPtr->worldHeight;
    w = itemPtr->worldWidth;
    if ((w < 1) || (h < 1)) {
        Blt_Warn("w=%d h=%d\n", w, h);
        return;
    }
    drawable = Blt_GetPixmap(viewPtr->display, Tk_WindowId(viewPtr->tkwin),
        w, h, Tk_Depth(viewPtr->tkwin));
#ifdef WIN32
    assert(drawable != None);
#endif
    DrawItemBackground(itemPtr, drawable, 0, 0);
    DrawItem(itemPtr, drawable, 0, 0);
    x = SCREENX(viewPtr, itemPtr->worldX);
    y = SCREENY(viewPtr, itemPtr->worldY);

    /* Don't let the item overlap the widget's border. Reduce the pixmap
     * accordingly. */
    x1 = y1 = viewPtr->inset;
    x2 = Tk_Width(viewPtr->tkwin) - viewPtr->inset;
    y2 = Tk_Height(viewPtr->tkwin) - viewPtr->inset;
    sx = sy = 0;
    if (x < x1) {                       /* Overlaps on the left. */
        sx = x1 - x;
        w -= sx;
        x = x1;
    }
    if ((x + w) > x2) {                 /* Overlaps on the right. */
        w = x2 - x;
    }
    if (y < y1) {                       /* Overlaps on the top. */
        sy = y1 - y;
        h -= sy;
        y = y1;
    }
    if ((y + h) > y2) {                 /* Overlaps on the bottom. */
        h = y2 - y;
    }
    XCopyArea(viewPtr->display, drawable, Tk_WindowId(viewPtr->tkwin),
        viewPtr->copyGC, sx, sy, w, h, x, y);
    Tk_FreePixmap(viewPtr->display, drawable);
}

/*
 *---------------------------------------------------------------------------
 *
 * SortItems --
 *
 *      Sorts the items in the list.
 *
 *---------------------------------------------------------------------------
 */
static void
SortItems(ListView *viewPtr)
{
    Item *itemPtr;
    long count;

    listViewInstance = viewPtr;
    viewPtr->flags &= ~SORT_PENDING;
    Blt_Chain_Sort(viewPtr->items, CompareLinks);
    viewPtr->flags |= SORTED;
    for (count = 0, itemPtr = FirstItem(viewPtr); itemPtr != NULL; 
         itemPtr = NextItem(itemPtr), count++) {
        itemPtr->index = count;
    }
    viewPtr->flags |= LAYOUT_PENDING;
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayListView --
 *
 *      This procedure is invoked to display a listview widget.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Commands are output to X to display the menu.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayListView(ClientData clientData)
{
    ListView *viewPtr = clientData;
    Pixmap drawable;
    int w, h;                           /* Window width and height. */

    viewPtr->flags &= ~REDRAW_PENDING;
    if (viewPtr->tkwin == NULL) {
        return;                         /* Window destroyed (should not get
                                         * here) */
    }
#ifdef notdef
    fprintf(stderr, "Calling DisplayListView(%s) w=%d h=%d\n", 
            Tk_PathName(viewPtr->tkwin), Tk_Width(viewPtr->tkwin),
            Tk_Height(viewPtr->tkwin));
#endif
    if ((viewPtr->tableSource.table != NULL) && 
        (viewPtr->flags & REBUILD_TABLE)) {
        RebuildTableItems(viewPtr->interp, viewPtr, viewPtr->tableSource.table);
    }
    if (viewPtr->flags & SORT_PENDING) {
        SortItems(viewPtr);
    }
    if (viewPtr->flags & LAYOUT_PENDING) {
        ComputeLayout(viewPtr);
    }
    viewPtr->width = Tk_Width(viewPtr->tkwin);
    viewPtr->height = Tk_Height(viewPtr->tkwin);
    if ((Tk_Width(viewPtr->tkwin) <= 1) || (Tk_Height(viewPtr->tkwin) <= 1)){
        /* Don't bother computing the layout until the window size is
         * something reasonable. */
        return;
    }
    if (!Tk_IsMapped(viewPtr->tkwin)) {
        /* The menu's window isn't displayed, so don't bother drawing
         * anything.  By getting this far, we've at least computed the
         * coordinates of the listview's new layout.  */
        return;
    }
    if (viewPtr->flags & SCROLL_PENDING) {
        int vw, vh;                     /* Viewport width and height. */
        /* 
         * The view port has changed. The visible items need to be recomputed
         * and the scrollbars updated.
         */
        vw = VPORTWIDTH(viewPtr);
        vh = VPORTHEIGHT(viewPtr);
        if ((viewPtr->xScrollCmdObjPtr != NULL) &&
            (viewPtr->flags & SCROLLX)) {
            Blt_UpdateScrollbar(viewPtr->interp, viewPtr->xScrollCmdObjPtr,
                viewPtr->xOffset, viewPtr->xOffset + vw, viewPtr->worldWidth);
        }
        if ((viewPtr->yScrollCmdObjPtr != NULL) && 
            (viewPtr->flags & SCROLLY)) {
            Blt_UpdateScrollbar(viewPtr->interp, viewPtr->yScrollCmdObjPtr,
                viewPtr->yOffset, viewPtr->yOffset+vh, viewPtr->worldHeight);
        }
        viewPtr->flags &= ~SCROLL_PENDING;
    }
    /*
     * Create a pixmap the size of the window for double buffering.
     */
    w = Tk_Width(viewPtr->tkwin);
    h = Tk_Height(viewPtr->tkwin);
    drawable = Blt_GetPixmap(viewPtr->display, Tk_WindowId(viewPtr->tkwin),
        w, h, Tk_Depth(viewPtr->tkwin));
#ifdef WIN32
    assert(drawable != None);
#endif
    /* Background */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, 
        viewPtr->defStyle.normalBg, 0, 0, w, h, 0, TK_RELIEF_FLAT);
    DrawListView(viewPtr, drawable);
    Blt_Bg_DrawRectangle(viewPtr->tkwin, drawable, 
        viewPtr->defStyle.normalBg, 0, 0, Tk_Width(viewPtr->tkwin), 
        Tk_Height(viewPtr->tkwin), viewPtr->borderWidth, viewPtr->relief);
    /* Draw focus highlight ring. */
    if ((viewPtr->highlightWidth > 0) && (viewPtr->flags & FOCUS)) {
        GC gc;

        gc = Tk_GCForColor(viewPtr->highlightColor, drawable);
        Tk_DrawFocusHighlight(viewPtr->tkwin, gc, viewPtr->highlightWidth,
            drawable);
    }
    XCopyArea(viewPtr->display, drawable, Tk_WindowId(viewPtr->tkwin),
        viewPtr->copyGC, 0, 0, w, h, 0, 0);
    Tk_FreePixmap(viewPtr->display, drawable);
}

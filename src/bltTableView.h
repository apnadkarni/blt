/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTableView.h --
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

#ifndef _BLT_TABLEVIEW_H
#define _BLT_TABLEVIEW_H

#define ODD(x)                  ((x) | 0x01)

#define GetKey(c) \
    ((CellKey *)Blt_GetHashKey(&(c)->viewPtr->cellTable, (c)->hashPtr))

#define SCREENX(v, x)   \
    ((x) - (v)->xOffset + (v)->inset + (v)->rowTitleWidth)
#define SCREENY(v, y)   \
    ((y) - (v)->yOffset + (v)->inset + (v)->colTitleHeight + \
        (v)->colFilterHeight)
#define VPORTWIDTH(v) \
    (Tk_Width((v)->tkwin) - (v)->rowTitleWidth - (2 * (v)->inset))
#define VPORTHEIGHT(v) \
    (Tk_Height((v)->tkwin) - (v)->colTitleHeight - (v)->colFilterHeight - \
        (2 * (v)->inset))

#define WORLDX(v, x)    \
    ((x) - (v)->inset - (v)->rowTitleWidth  + (v)->xOffset)
#define WORLDY(v, y)    \
    ((y) - (v)->inset - (v)->colTitleHeight - (v)->colFilterHeight + \
        (v)->yOffset)


/* The following flags are common for rows, columns, and cells */
/* 
 * A dirty flag on the view indicates the overall layout is unreliable (for
 * picking) and needs to be recomputed.
 *
 * Dirty row, column, and cell flags indicate that the cell geometry must
 * be recomputed along with the overall layout.  Row and column dirty flags
 * are set when a table value in the row or column has been set of unset
 * and when the row or column changes its style.  Cell dirty flags are set
 * when a style changes.
 * 
 * Style dirty flags are set when a style changed. We need to figure out
 * what cells were affected. The dirty flags are propagated to affected
 * cells.
 */
#define GEOMETRY        (1<<0)          /* Forces the geometry of the
                                         * cell/row/column/view to be
                                         * recomputed.  */
#define VISIBILITY      (1<<1)          /* Indicates that the visibility
                                         * of the cells in the widget need
                                         * to be recomputed. */
#define HIDDEN          (1<<2)          /* The row or column is hidden. No
                                         * geometry is computed for cells in
                                         * hidden rows and columns. */
#define SELECTED        (1<<3)          /* Row or column is selected. It
                                         * will be drawn in the select
                                         * foreground and background
                                         * colors. */
#define DISABLED        (1<<4)          /* Row, column or cell is
                                         * disabled. All the disabled cells
                                         * will be drawn the disabled
                                         * foreground and background
                                         * colors. */
#define HIGHLIGHT       (1<<5)          /* The cells in the row or column
                                         * or a cell itself are marked as
                                         * highlighted. */
#define EDIT            (1<<6)          /* The cells in the row or column
                                         * or a cell itself are marked as
                                         * editable.  The user may change
                                         * the contents of the cell (table
                                         * data). */
/* Cell, row, column flags */
#define TEXTALLOC       (1<<7)          /* Indicates that the cell's
                                         * formatted text was alloced and
                                         * must be freed. */
#define FOCUS           (1<<8)
#define REDRAW          (1<<9)          /* Indicates the widget needs to be
                                         * redrawn. Some changes may occur
                                         * to cells that are off-screen and
                                         * don't affect the cells that are
                                         * on screen (e.g. adding columns
                                         * or rows). Don't redraw the table
                                         * unless there are cells in the
                                         * visible portion that need to be
                                         * redrawn. */

/* Row and column only flags */
#define DELETED         (1<<10)         /* The row, column, cell has been
                                         * deleted. */

#define POSTED          (1<<11)         /* Cells can be posted. */
#define STICKY          (1<<12)
#define HAS_SELECTION   (1<<13)

/* These are tableview only flags. */
#define LAYOUT_PENDING  (1<<13)
#define REDRAW_PENDING  (1<<14)
#define SCROLLX         (1<<15)
#define SCROLLY         (1<<16)
#define SCROLL_PENDING  (SCROLLX|SCROLLY)
#define SELECT_PENDING  (1<<17)         /* A "selection" command idle task is
                                         * pending.  */
#define REINDEX_ROWS    (1<<18)
#define REINDEX_COLUMNS (1<<19)

#define SELECT_SORTED   (1<<22)         /* Indicates if the entries in the
                                         * selection should be sorted or
                                         * displayed in the order they were
                                         * selected. */
#define SELECT_EXPORT   (1<<23)         /* Export the selection to X11. */

#define DONT_UPDATE     (1<<24)

#define COLUMN_TITLES   (1<<25)         /* Display a header/label for each
                                         * column. */
#define ROW_TITLES      (1<<26)         /* Display a header/label for each
                                         * row. */
#define TITLES_MASK     (COLUMN_TITLES|ROW_TITLES)

#define AUTO_ROWS       (1<<27)         /* Create rows and columns as
                                         * needed when attached a
                                         * datatable. */
#define AUTO_COLUMNS    (1<<28)         /* Create rows and columns as
                                         * needed when attached a
                                         * datatable. */
#define AUTOCREATE      (AUTO_ROWS|AUTO_COLUMNS)
#define COLUMN_FILTERS  (1<<29)         /* Display combobox below each
                                         * column title to filter row
                                         * values. */
#define FILTERHIGHLIGHT (1<<30)         /* Display the filter with
                                         * highlighted
                                         * foreground/background colors */
/* Sort-related flags */
#define SORT_PENDING    (1<<0)          
#define SORT_ALWAYS     (1<<1)
#define SORTED          (1<<2)          /* The view is currently sorted.
                                         * This is used to simply reverse
                                         * the view when the sort
                                         * -decreasing flag is changed. */

/* Item types used picking objects in widget. */
typedef enum {
    ITEM_NONE,
    ITEM_COLUMN_FILTER,
    ITEM_COLUMN_TITLE,
    ITEM_COLUMN_RESIZE,
    ITEM_ROW_FILTER,
    ITEM_ROW_TITLE,
    ITEM_ROW_RESIZE,
    ITEM_CELL,
} ItemType;

#define ITEM_MASK            (0x7)

#define ITEM_STYLE          (0x10004)
    
#define SHOW_VALUES       (1<<20)

#define CELL_FLAGS_MASK         (DISABLED|POSTED|HIGHLIGHT)

typedef struct _BindTag {
    ClientData clientData;
    int type;
} *BindTag;

/*
 * Limits --
 *
 *      Defines the bounding of a size (width or height) in the table.  It may
 *      be related to the partition, entry, or table size.  The widget
 *      pointers are used to associate sizes with the requested size of other
 *      widgets.
 */

typedef struct {
    int flags;                  /* Flags indicate whether using default
                                 * values for limits or not. See flags
                                 * below. */
    int max, min;               /* Values for respective limits. */
    int nom;                    /* Nominal starting value. */
} Limits;

#define LIMITS_SET_BIT  1
#define LIMITS_SET_MIN  (LIMITS_SET_BIT<<0)
#define LIMITS_SET_MAX  (LIMITS_SET_BIT<<1)
#define LIMITS_SET_NOM  (LIMITS_SET_BIT<<2)

#define LIMITS_MIN      0       /* Default minimum limit  */
#define LIMITS_MAX      SHRT_MAX/* Default maximum limit */
#define LIMITS_NOM      -1000   /* Default nomimal value.  Indicates if a
                                 * partition has received any space yet */

typedef enum CellStyleTypes {
    STYLE_TEXTBOX, STYLE_CHECKBOX, STYLE_COMBOBOX, STYLE_IMAGEBOX,
    STYLE_PUSHBUTTON
} CellStyleType;

typedef struct _Cell Cell;
typedef struct _CellKey CellKey;
typedef struct _CellStyle CellStyle;
typedef struct _CellStyleClass CellStyleClass;
typedef struct _Column Column;
typedef struct _Row Row;
typedef struct _TableView TableView;

/*
 * Icon --
 *
 *      Uniquely store a single reference to a tkImage for the tableview
 *      widget.  We never need more than a single instance of an image,
 *      regardless of how many times it's used in the widget.  This acts as
 *      a cache for the image, maintaining a reference count for each image
 *      used in the widget.  It's likely that the tableview widget will use
 *      many instances of the same image.
 */
typedef struct _Icon {
    TableView *viewPtr;                 /* Widget using this icon. */
    Tk_Image tkImage;                   /* The Tk image being cached. */
    Blt_HashEntry *hashPtr;             /* Pointer to this entry in the
                                         * image cache hash table. */
    int refCount;                       /* Reference count for this
                                         * image. */
    short int width, height;            /* Dimensions of the cached
                                         * image. */
} *Icon;

#define IconHeight(icon)        ((icon)->height)
#define IconWidth(icon)         ((icon)->width) 
#define IconBits(icon)          ((icon)->tkImage)
#define IconName(icon)          (Blt_Image_Name((icon)->tkImage))

typedef void (CellStyleConfigureProc)(TableView *viewPtr, CellStyle *stylePtr);
typedef void (CellStyleDrawProc)(Cell *cellPtr, Drawable drawable, 
        CellStyle *stylePtr, int x, int y);
typedef void (CellStyleFreeProc)(CellStyle *stylePtr);
typedef void (CellStyleGeometryProc)(Cell *cellPtr, CellStyle *stylePtr);
typedef const char *(CellStyleIdentifyProc)(Cell *cellPtr, CellStyle *stylePtr,
        int x, int y);

typedef struct _TableObj {
    unsigned int flags;                /* Flags of the object. DELETE
                                         * indicates the object has been
                                         * deleted and should not be
                                         * picked. */
    Blt_HashEntry *hashPtr;
} TableObj;
    
/*
 * CellStyleClass --
 *
 *      Represents the methods and specifications for a class of styles.
 *      All the styles of the same class share this structure.  Currently
 *      the defined style classes are "textbox", "checkbox", "combobox",
 *      and "imagebox".
 */
struct _CellStyleClass {
    const char *type;                   /* Name of style class type. */
    const char *className;              /* Class name of the style. This is
                                         * used as the class name of the
                                         * tableview component for event
                                         * bindings. */
    Blt_ConfigSpec *specs;              /* Style-specific configuration
                                         * specifications. */
    CellStyleConfigureProc *configProc; /* Sets the GCs for the style. */
    CellStyleGeometryProc *geomProc;    /* Measures the area needed for the
                                         * cell with this style. */
    CellStyleDrawProc *drawProc;        /* Draws the cell in this style. */
    CellStyleIdentifyProc *identProc;   /* Routine to pick the style's
                                         * button.  Indicates if the mouse
                                         * pointer is over the * style's
                                         * button (if it has one). */
    CellStyleFreeProc *freeProc;        /* Routine to free the style's
                                         * resources. */
};

/*
 * CellStyle --
 *
 *      Represents the drawing attributes of a cell (not the cell data).
 *      The style tells how to display the table cell.  It contains the
 *      drawing parameters like font, color, icon, etc.  Individual cells,
 *      cells in a row, or cells in a column may specify style to use.
 *      There is a default style predefined for the tableview widget.
 *      Normally there will be many cells using a single style.
 */
struct _CellStyle {
    int refCount;                       /* Usage reference count.  A
                                         * reference count of zero
                                         * indicates that the style may be
                                         * freed. */
    unsigned int flags;                 /* Bit fields containing various
                                         * flags. */
    const char *name;                   /* Instance name. */
    CellStyleClass *classPtr;           /* Contains class-specific
                                         * information such as
                                         * configuration specifications and
                                         * routines how to configure, draw,
                                         * layout, etc the cell according
                                         * to the style. */
    Blt_HashEntry *hashPtr;             /* If non-NULL, points to the hash
                                         * table entry for the style.  A
                                         * style that's been deleted, but
                                         * still in use (non-zero reference
                                         * count) will have no hash table
                                         * entry. */
    Blt_HashTable table;                /* Table of cells that have this
                                         * style. We use this to mark the
                                         * cells dirty when the style
                                         * changes. */
    TableView *viewPtr;                 /* Widget using this style. */
    /* General style fields. */
    Tk_Cursor cursor;                   /* X Cursor */
    Icon icon;                          /* If non-NULL, is a Tk_Image to be
                                         * drawn in the cell. */
    int gap;                            /* # pixels gap between icon and
                                         * text. */
    Blt_Font font;
    XColor *normalFg;                   /* Normal color of the text. */
    XColor *activeFg;                   /* Color of the text when the cell
                                         * is active. */
    XColor *disableFg;                  /* Color of the text when the cell
                                         * is disabled. */
    XColor *highlightFg;                /* Color of the text when the cell
                                         * is highlighted. */
    XColor *selectFg;                   /* Color of the text when the cell
                                         * is selected. */
    XColor *focusColor;                 /* Background color of the focus. */
    Blt_Bg normalBg;                    /* Normal background color of
                                         * cell. */
    Blt_Bg activeBg;                    /* Background color when the cell
                                         * is active. Textboxes are usually
                                         * never active. */
    Blt_Bg altBg;                       /* Alternative normal
                                         * background. */
    Blt_Bg disableBg;                   /* Background color when the cell
                                         * is disabled. */
    Blt_Bg highlightBg;                 /* Background color when the cell
                                         * is highlighted. */
    Blt_Bg selectBg;                    /* Background color when the cell
                                         * is selected. */
    GC normalGC;                        /* Graphics context of normal
                                         * text. */
    GC activeGC;                        /* Graphics context of active
                                         * text. */
    GC disableGC;                       /* Graphics context of disabled
                                         * text. */
    GC highlightGC;                     /* Graphics context of highlighted
                                         * text. */
    GC selectGC;                        /* Graphics context of selected
                                         * text. */
    GC focusGC;                         /* Graphics context for focus
                                         * rectangle. */
    Tk_Justify justify;                 /* Indicates how the text or icon
                                         * is justified within the
                                         * column. */
    int borderWidth;                    /* Width of outer border
                                         * surrounding the entire box. */
    int relief, activeRelief;           /* Relief of outer border. */
    Tcl_Obj *cmdObjPtr;                 /* If non-NULL, TCL procedure
                                         * called to format the style is
                                         * invoked.*/
    XColor *rowRuleColor;               /* Color of the row's rule. */
    GC rowRuleGC;                       /* Graphics context of the row's
                                         * rule. */
    XColor *colRuleColor;               /* Color of the row's rule. */
    GC colRuleGC;                       /* Graphics context of the row's
                                         * rule. */
};

/*
 * Row --
 *
 *      Represents a row in the tableview widget.  It contains the table
 *      row that it really represents.  It has the y-coordinate of where
 *      this row starts in world coordinates and the height of the row
 *      (maximum height of all the cells in the row).
 */
struct _Row {
    unsigned int flags;
    Blt_HashEntry *hashPtr;
    TableView *viewPtr;                 /* The parent tableview widget that
                                         * manages this row. */
    struct _Row *nextPtr, *prevPtr;
    CellStyle *stylePtr;                /* Style for cells in the row.  If
                                         * NULL, uses global and row
                                         * defaults. Changing the style
                                         * means recomputing the extents of
                                         * each cell in the row. */
    Tcl_Obj *cmdObjPtr;                 /* Command associated with the row
                                         * title button. */
    Icon icon;
    const char *title;                  /* Title to be displayed for this
                                         * row.  If NULL, the label for the
                                         * row will be displayed. */
    short int titleWidth, titleHeight;  /* Extents of row title. */
    int titleRelief;                    /* Relief of the row title when the
                                         * title is normal or disabled. */
    int activeTitleRelief;              /* Relief of the row title when the
                                         * title is active */
    int titleJustify;
    int height;                         /* Maximum height of all the cells
                                         * in this row. */
    Limits reqHeight;                   /* If > 0, requested height of this
                                         * row.  This overrides the
                                         * computed height. */
    int min, max, nom;                  /* Min/Max/Nominal space allowed for
                                         * column. */
    int ruleHeight;
    size_t index;
    size_t visibleIndex;
    double weight;                      /* Growth factor for row.  If zero
                                         * the row can not be resized. */
    Tcl_Obj *bindTagsObjPtr;            /* List of binding tags for this
                                         * row. */
    BLT_TABLE_ROW row;                  /* Row in the datatable this
                                         * structure is associated with. */
    long worldY;                        /* Offset of row in world
                                         * coordinates. from the top of the
                                         * table. */
    Blt_ChainLink link;                 /* Selection link. */
};

/*
 * Column --
 *
 *      Represents a column in the tableview widget.  It contains the table
 *      column that it really represents.  It has the x-coordinate of where
 *      this column starts in world coordinates and the width of the column
 *      (maximum width of all the cells in the column).
 *
 *      Columns (different from rows) can be sorted.  There is a sort
 *      command that specify how the column is to be sorted.  This depends
 *      upon the type of the column in the table.
 */
struct _Column {
    unsigned int flags;
    Blt_HashEntry *hashPtr;
    TableView *viewPtr;                 /* The parent tableview widget that
                                         * manages this column. */
    struct _Column *nextPtr, *prevPtr;
    CellStyle *stylePtr;                /* Style for cells in the column.
                                         * If NULL, uses the global default
                                         * style. Changing the style means
                                         * recomputing the extents of each
                                         * cell in the column. */
    Tcl_Obj *cmdObjPtr;                 /* Command associated with the
                                         * column title button. */
    Icon icon;
    const char *title;                  /* Title to be displayed for this
                                         * column.  If NULL, the label for
                                         * the column will be displayed. */
    short int titleWidth, titleHeight;  /* Extents of column title. */
    int titleRelief;                    /* Relief of the row title when the
                                         * title is normal or disabled. */
    int activeTitleRelief;              /* Relief of the row title when the
                                         * title is active */
    int titleJustify;

    int width;                          /* Maximum width of all the cells
                                         * in this column. */
    Limits reqWidth;                    /* Requested width of this column.
                                         * This overrides the computed
                                         * width. */
    int min, max, nom;                  /* Min/Max/Nominal space allowed for
                                         * column. */
    int ruleWidth;
    size_t index;
    size_t visibleIndex;
    double weight;                      /* Growth factor for the column.
                                         * If zero the column can not be
                                         * resized. */
    Tcl_Obj *bindTagsObjPtr;            /* List of binding tags for this
                                         * column. */
    BLT_TABLE_COLUMN column;            /* Column in the datatable this
                                         * structure is associated with. */
    long worldX;                        /* Offset of column in world
                                         * coordinates from the left of the
                                         * table. */
    int sortType;
    Tcl_Obj *sortCmdObjPtr;             /* TCL script used to compare two
                                         * cells in the column. */
    short int textWidth, textHeight;
    Blt_ChainLink link;                 /* Selection link. */
    Tcl_Obj *fmtCmdObjPtr;              /* If non-NULL, TCL procedure
                                         * called to format the data
                                         * whenever data has changed and
                                         * needs to be redisplayed. */
    short int arrowWidth, arrowHeight;
    short int filterHeight;
    const char *filterText;             /* Text of last filter selected. */
    short int filterTextWidth, filterTextHeight;
    Icon filterIcon;                    /* Icon of last filter selected. */
    Blt_Font filterFont;                /* Font of last filter selected. */
    const char *filterValue;            /* Value of last filter selected. */
    Tcl_Obj *filterMenuObjPtr;          /* Name of menu attached to this
                                         * column to filter row values. */
    Tcl_Obj *filterDataObjPtr;
    Blt_Pad pad;                        /* Horizontal padding on either
                                         * side of the column. */
};

/*
 * CellKey --
 *
 *      Cells are keyed in the hash table by their row and column.
 */
struct _CellKey {
    Row *rowPtr;
    Column *colPtr;
};

/*
 * Cell --
 */
struct _Cell {
    unsigned int flags;
    Blt_HashEntry *hashPtr;             /* Row,column of table entry this
                                         * cell represents. */
    TableView *viewPtr;                 /* The parent tableview widget that
                                         * manages this cell. */
    
    const char *text;                   /* If non-NULL, represents the
                                         * formatted string of the cell
                                         * value. */
    Tk_Image tkImage;                   /* If non-NULL, represents a
                                         * Tk_Image image of the cell
                                         * value. */
    CellStyle *stylePtr;                /* If non-NULL, indicates an
                                         * overriding style for this
                                         * specific cell. */
    unsigned short width, height;       /* Dimension of cell contents. This
                                         * may include the style's
                                         * borderwidth, but not the row or
                                         * column borderwidth or
                                         * padding.  */
    unsigned short textWidth, textHeight;
};

/*
 * RowSelection Information:
 *
 * The selection is the rectangle that contains a selected row.  There may
 * be many selected rows.  It is displayed with the selected foreground and
 * background color designated by the cell's style.
 */
typedef struct {
    unsigned int flags;
    Row *anchorPtr;                     /* Fixed end of selection (i.e. row
                                         * at which selection was
                                         * started.) */
    Row *markPtr;
    Blt_Chain list;                     /* Chain of currently selected
                                         * rows.  Contains the same
                                         * information as the above hash
                                         * table, but maintains the order
                                         * in which rows are selected. */
} RowSelection;

/*
 * ColumnSelection Information:
 *
 * The selection is the rectangle that contains a selected column.  There
 * may be many selected columns.  It is displayed with the selected
 * foreground and background color designated by the cell's style.
 */
typedef struct {
    unsigned int flags;
    Column *anchorPtr;                  /* Fixed end of selection (i.e. row
                                         * at which selection was
                                         * started.) */
    Column *markPtr;                    /* The end of the selection where
                                         * it was finished. */
    Blt_Chain list;                     /* Chain of currently selected
                                         * rows.  Maintains the order in
                                         * which rows are selected. */
} ColumnSelection;

/*
 * CellSelection Information:
 *
 * The selection is the rectangle that contains a selected cell.  There may
 * be many selected cells.  It is displayed with the selected foreground
 * and background color designated by the cell's style.
 * 
 * Instead of a hashtable to track selected rows, we mark the cells
 * themselves as selected (or not selected).  Cell selections do not handle
 * order. The order is always based on the row and column of the cell.
 */
typedef struct {
    unsigned int flags;
    Blt_HashTable cellTable;            /* Currently selected cells. */
    CellKey *anchorPtr;                 /* Start of current selection
                                         * (i.e. cell at which selection
                                         * was started.) */
    CellKey *markPtr;                   /* End of current selection.
                                         * (i.e. cell at which selection
                                         * last stopped.) */
} CellSelection;

#define SELECT_SINGLE_ROW    (1<<0)     /* Only one row at a time can be
                                         * selected. */
#define SELECT_MULTIPLE_ROWS (1<<1)     /* Multiple rows can be selected.
                                         * The order of the selections is
                                         * preserved. */
#define SELECT_ROWS          (SELECT_SINGLE_ROW|SELECT_MULTIPLE_ROWS)

#define SELECT_SINGLE_COLUMN (1<<2)     /* Only one column at a time can be
                                         * selected. */
#define SELECT_MULTIPLE_COLUMNS (1<<3)  /* Multiple columns can be
                                         * selected.  The order of the
                                         * selections is preserved. */
#define SELECT_COLUMNS       (SELECT_SINGLE_COLUMN|SELECT_MULTIPLE_COLUMNS)

#define SELECT_CELLS         (1<<4)     /* Individual cells can be selected
                                         * by sweeping out a rectangle with
                                         * the mouse. The order of the
                                         * selections is not preserved.  */

#define SELECT_CLEAR            (1<<0)  /* Clear selection flag of entry. */
#define SELECT_SET              (1<<1)  /* Set selection flag of entry. */
/* Toggle selection flag * of entry. */
#define SELECT_TOGGLE (SELECT_SET | SELECT_CLEAR) 
/* Mask of selection set/clear/toggle flags.*/
#define SELECT_MASK   (SELECT_SET | SELECT_CLEAR) 

typedef struct {
    Column *firstPtr;                   /* Primary column to sort with. */
    Blt_Chain order;                    /* Columns to use as sorting
                                         * criteria. */
    int decreasing;                     /* Indicates entries should be
                                         * sorted in decreasing order. */
    int viewIsDecreasing;               /* Current sorting direction */
    int flags;
    Icon up;
    Icon down;
    Blt_Picture upArrow, downArrow;     /*  Cached/generated pictures. */
} SortInfo;

typedef struct {
    Column *activePtr;                  /* Column where the filter button is
                                         * currently active. */
    Column *postPtr;                    /* Column where the filter menu is
                                         * currently posted. */
    Tcl_Obj *menuObjPtr;                /* Menu to be posted. */
    Tcl_Obj *postCmdObjPtr;             /* Command to executed before the
                                         * menu is posted. */
    Blt_Font font;

    /* Column filters attributes. */
    int borderWidth;                    /* Border width of the column
                                         * filter. */
    int outerBorderWidth;               /* Outer border width of the column
                                         * filter. */
    int relief;                         /* Relief of the column filter when
                                         * the filter is normal or
                                         * disabled. */
    int activeRelief;                   /* Relief of the column filter when
                                         * the filter button is active */
    int selectRelief;                   /* Relief of the column filter when
                                         * the filter menu is posted. */
    Blt_Bg normalBg;                    /* Background color of the column
                                         * title. */
    Blt_Bg activeBg;                    /* Background color of the column
                                         * title when the title is
                                         * active. */
    Blt_Bg disabledBg;                  /* Background color of the column
                                         * title when the title is
                                         * disabled. */
    Blt_Bg selectBg;                    /* Background color of the column
                                         * title when the title is
                                         * selected. */
    Blt_Bg highlightBg;                 /* Background color of the column
                                         * filter when the filter is
                                         * highlighted. */
    XColor *normalFg;                   /* Text color of the column
                                         * title. */
    XColor *activeFg;                   /* Text color of the column title
                                         * when the title is active */
    XColor *disabledFg;                 /* Text color of the column title
                                         * when it is disabled. */
    XColor *selectFg;                   /* Text color of the column title
                                         * when it is selected. */
    XColor *highlightFg;                /* Foreground color of the column
                                         * filter text when the filter is
                                         * highlighted. */
    GC activeGC;                        /* GC for active column filters. */
    GC disabledGC;                      /* GC for disabled column filters. */
    GC normalGC;                        /* GC for normal column filters. */
    GC selectGC;                        /* GC for selected filters. */
    GC highlightGC;                     /* GC for highlighted filters. */
    Blt_Picture downArrow;
} FilterInfo;

/*
 * TableView --
 *
 *      Represents the tableview widget. The TableView widget displays a
 *      BLT_TABLE table.
 *
 *      Table cells are positioned in world coordinates, referring to the
 *      virtual tableview.  The widget's Tk window acts as view port into
 *      this virtual space. The tableview's xOffset and yOffset fields
 *      specify the location of the view port in the virtual world.  You
 *      scroll the viewport by changing the offsets and redrawing.
 */
struct _TableView {
    Tcl_Interp *interp;                 /* Interpreter to return
                                         * results. */
    Tcl_Command cmdToken;               /* Token for widget's TCL
                                         * command. */
    BLT_TABLE table;                    /* Token holding internal table. */
    Blt_HashEntry *hashPtr;             /* Pointer to this entry in the
                                         * interpreter-specific hash table
                                         * of tableview widgets. */
    /* TableView specific fields. */ 
    Tk_Window tkwin;                    /* Window that embodies the widget.
                                         * NULL means that the window has
                                         * been destroyed but the data
                                         * structures haven't yet been
                                         * cleaned up.*/
    Display *display;                   /* Display containing widget;
                                         * needed, among other things, to
                                         * release resources after tkwin
                                         * has already gone away. */
    unsigned int flags;                 /* For bitfield definitions, see
                                         * below */

    Blt_HashTable rowTable;             /* Hash table of rows keyed by the
                                         * BLT_TABLE_ROW. */
    Blt_HashTable columnTable;          /* Hash table of columns keyed by
                                         * the BLT_TABLE_COLUMN. */
    Blt_HashTable cellTable;            /* Hash table of cells keys by the
                                         * combination of the Row and
                                         * Column pointer addresses. */
    Blt_HashTable cachedObjTable;       /* Table of strings. */
    Blt_HashTable iconTable;            /* Table of icons. */
    Blt_HashTable styleTable;           /* Table of cell styles. */
    Blt_HashTable bindTagTable;         /* Table of row bindtags. */
    Blt_HashTable uidTable;             /* Table of strings. */
    Row *rowHeadPtr, *rowTailPtr;
    Column *colHeadPtr, *colTailPtr;
    Row **rowMap;                       /* Array of pointers to rows. This
                                         * represents the sorted view of
                                         * the table. */
    Column **columnMap;                 /* Array of pointers to
                                         * columns. This represents the
                                         * sorted view of the table. */
    Row **visibleRows;                  /* Array of pointers to visible
                                         * rows. This is a subset of the
                                         * above rows array. It contains
                                         * only pointers to rows that are
                                         * currently visible on the
                                         * screen. */
    Column **visibleColumns;            /* Array of pointers to visible
                                         * columns. This is a subset of the
                                         * above columns array.  It
                                         * contains only pointers to
                                         * columns that are currently
                                         * visible on the screen. */
    size_t numRows, numColumns;         /* Number or rows and columns in
                                         * the above arrays. */
    size_t numVisibleRows, numVisibleColumns;
    size_t numMappedRows, numMappedColumns;

    BLT_TABLE_NOTIFIER rowNotifier, colNotifier; 
                                        /* Notifier used to tell the viewer
                                         * that any rows or columns have
                                         * changed in the datatable. */
    short int rowTitleWidth, rowTitleHeight;
    short int colTitleWidth, colTitleHeight;
    short int colFilterHeight;
    int width, height;
    int worldWidth, worldHeight;        /* Dimensions of world view. */
    int xOffset, yOffset;               /* Translation between view port
                                         * and world origin. */
    Blt_Pool cellPool;                  /* Memory pool for cells. */ 
    Blt_Pool rowPool;                   /* Memory pool for row headers. */
    Blt_Pool columnPool;                /* Memory pool for column
                                         * headers. */

    /*
     * Selection Information:
     *
     * The selection is the rectangle that contains a selected entry.
     * There may be many selected entries.  It is displayed as a solid
     * colored box with optionally a 3D border.
     */
    int selectMode;                     /* Selection style: "single" or
                                         * "multiple", or "cells".  */
    RowSelection selectRows;
    ColumnSelection selectColumns;
    CellSelection selectCells;
    Tcl_Obj *selectCmdObjPtr;           /* TCL script that's invoked
                                         * whenever the selection
                                         * changes. */
    Cell *activePtr;                    /* The cell that is currently
                                         * active. */
    Cell *focusPtr;                     /* The cell that currently have
                                         * focus */
    Cell *postPtr;                      /* If non-NULL, this is the cell to
                                         * whicich all events are currently
                                         * being redirected. */
    const char *takeFocus;

    Row *rowActivePtr;                  /* Row that's currently active. */  
    Column *colActivePtr;               /* Column that's currently active. */  

    /* Attributes for row titles. */
    Row *rowActiveTitlePtr;             /* Row title that's currently
                                         * active.*/
    Row *rowResizePtr;                  /* Row that is being resized. */
    Blt_Font rowTitleFont;              /* Font to display row titles. */
    int rowTitleBorderWidth;            /* Border width of the row
                                         * title. */
    Blt_Bg rowNormalTitleBg;            /* Background color of the row
                                         * title. */
    Blt_Bg rowActiveTitleBg;            /* Background color of the row
                                         * title when the title is
                                         * active. */
    Blt_Bg rowDisabledTitleBg;          /* Background color of the row
                                         * title when the title is
                                         * disabled. */
    XColor *rowNormalTitleFg;           /* Text color of the row title. */
    XColor *rowActiveTitleFg;           /* Text color of the row title when
                                         * the title is active */
    XColor *rowDisabledTitleFg;         /* Text color of the row title when
                                         * it is disabled. */
    GC rowNormalTitleGC;                /* GC for row titles. */
    GC rowActiveTitleGC;                /* GC for active row titles. */
    GC rowDisabledTitleGC;              /* GC for disabled row titles. */

    /* Row resize attributes. */
    Tk_Cursor rowResizeCursor;          /* Resize cursor for rows. */
    Tcl_Obj *rowCmdObjPtr;              /* TCL script to be executed when
                                         * the row is invoked. */
    Blt_BindTable bindTable;            /* Binding information for cells. */

    Blt_Bg bg;                          /* Background when there's nothing */

    /* Column title attributes. */
    Column *colActiveTitlePtr;          /* Column title currently active. */  
    Column *colResizePtr;               /* Column that is being resized. */

    Blt_Font colTitleFont;              /* Font to display column titles. */
    int colTitleBorderWidth;            /* Border width of the column title. */
    Blt_Bg colNormalTitleBg;            /* Background color of the column
                                         * title. */
    Blt_Bg colActiveTitleBg;            /* Background color of the column
                                         * title when the title is active. */
    Blt_Bg colDisabledTitleBg;          /* Background color of the column
                                         * title when the title is
                                         * disabled. */
    XColor *colNormalTitleFg;           /* Text color of the column title. */
    XColor *colActiveTitleFg;           /* Text color of the column title
                                         * when the title is active */
    XColor *colDisabledTitleFg;         /* Text color of the column title
                                         * when it is disabled. */
    GC colActiveTitleGC;                /* GC for active column titles. */
    GC colDisabledTitleGC;              /* GC for disabled column titles. */
    GC colNormalTitleGC;                /* GC for column titles. */

    /* Column resize attributes. */
    Tk_Cursor colResizeCursor;          /* Resize cursor for columns. */
    Tcl_Obj *colCmdObjPtr;              /* TCL script to be executed when
                                         * the column is invoked. */
    int rowResizeAnchor, rowResizeMark;
    int colResizeAnchor, colResizeMark;

    /* Highlight focus ring. */
    int highlightWidth;                 /* Width in pixels of highlight to
                                         * draw around widget when it has
                                         * the focus.  <= 0 means don't
                                         * draw a highlight. */
    XColor *highlightBgColor;           /* Color for drawing traversal
                                         * highlight area when highlight is
                                         * off. */
    XColor *highlightColor;             /* Color for drawing traversal
                                         * highlight. */

    /* Widget border. */
    int borderWidth;
    int inset;                          /* The combination of the border
                                         * width and the highlight
                                         * thickness */
    int relief;

    /* Scrolling attibutes. */
    Tcl_Obj *xScrollCmdObjPtr;          /* TCL command to control the
                                         * horizontal scrollbar. */
    Tcl_Obj *yScrollCmdObjPtr;          /* TCL comment to control the
                                         * vertical scrollbar. */
    int xScrollUnits, yScrollUnits;     /* # of pixels per scroll unit. */
    int scrollMode;                     /* Selects mode of scrolling: either
                                         * BLT_SCROLL_MODE_HIERBOX,
                                         * BLT_SCROLL_MODE_LISTBOX, or
                                         * BLT_SCROLL_MODE_CANVAS. */
    /* Scanning information: */
    int scanAnchorX, scanAnchorY;       /* Scan anchor in screen
                                         * coordinates. */
    int scanX, scanY;                   /* X-Y world coordinate where the
                                         * scan started. */
    Blt_Painter painter;
    Tk_Cursor cursor;                   /* X Cursor */
    CellStyle *stylePtr;                /* Default cell style. */
    int reqWidth, reqHeight;            /* Requested dimensions of the
                                         * tableview widget's window. */
    int reqArrowWidth, arrowWidth;
    SortInfo sort;
    FilterInfo filter;
    int maxRowHeight;                   /* This sets the maximum for all row
                                         * heights.  This is needed for
                                         * cases where you don't know the
                                         * row heights beforehand (such as
                                         * when loading a table with
                                         * -table). */
    int maxColWidth;                    /* This sets the maximum for all
                                         * column widths.  This is needed
                                         * for cases where you don't know
                                         * the column widths beforehand
                                         * (such as when loading a table
                                         * with -table). */
                                         
};

BLT_EXTERN CellStyle *Blt_TableView_CreateCellStyle(Tcl_Interp *interp,
        TableView *viewPtr, int type, const char *styleName);
BLT_EXTERN void Blt_TableView_EventuallyRedraw(TableView *viewPtr);

#endif /*BLT_TABLEVIEW_H*/

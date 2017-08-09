/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTreeViewStyle.c --
 *
 * This module implements styles for treeview widget cells.
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

#ifndef NO_TREEVIEW

#ifdef HAVE_STRING_H
  #include <string.h>
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

#define CELL_PADX               1
#define CELL_PADY               1
#define STYLE_GAP               2
#define ARROW_WIDTH             13
#define FOCUS_PAD               3       /* 1 pixel either side of a 1 pixel
                                         * line */

#define GetData(entryPtr, key, objPtrPtr) \
        Blt_Tree_GetValueByKey((Tcl_Interp *)NULL, (entryPtr)->viewPtr->tree, \
              (entryPtr)->node, key, objPtrPtr)

/* Styles describe how to draw a particular cell. The style for a cell is
 * determined from the most local style specified: cell, entry, column,
 * or widget (global).
 *
 * TextBox:     Draws text with an optional icon.
 * CheckBox:    Draws check box with optional icon and text.
 * ComboBox:    Draws text and button with optional icon.
 * ImageBox:    Draws an image with optional icon and text.
 *
 * Combobox styles are tricky because they specify a single combomenu that
 * works only with one specific cell at a time.  Even though many cells may
 * be using the same combobox style.  Only the active (posted) cell can be
 * changed through the combomenu.
 */

/* Style-specific flags. */
#define SHOW_VALUE                      (1<<10)
#define SHOW_TEXT                       (1<<11)
#define ACTIVE_COLORS                   (1<<12)
#define UNDERLINE_ACTIVE                (1<<13)
#define TEXT_VAR_TRACED                 (1<<16)
#define ICON_VAR_TRACED                 (1<<17)
#define TRACE_VAR_FLAGS                 (TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|\
                                                TCL_TRACE_UNSETS)

#define DEF_ACTIVE_FG                   STD_ACTIVE_FOREGROUND
#define DEF_ALT_BG                      (char *)NULL
#define DEF_DISABLE_BG                  RGB_GREY97
#define DEF_DISABLE_FG                  RGB_GREY85
#define DEF_FOCUS_COLOR                 "black"
#define DEF_FOCUS_DASHES                "dot"
#define DEF_GAP                         "2"
#define DEF_HIGHLIGHT_BG                STD_NORMAL_BACKGROUND
#define DEF_HIGHLIGHT_FG                STD_NORMAL_FOREGROUND
#define DEF_SELECT_BG                   (char *)NULL
#define DEF_SELECT_FG                   STD_SELECT_FOREGROUND
#define DEF_ICON                        (char *)NULL
#define DEF_JUSTIFY                     "center"
#define DEF_NORMAL_BG                   (char *)NULL
#define DEF_NORMAL_FG                   STD_NORMAL_FOREGROUND
#define DEF_ACTIVE_BG                   (char *)NULL

#ifdef WIN32
  #define DEF_CHECKBOX_CURSOR             "arrow"
  #define DEF_RADIOBUTTON_CURSOR          "arrow"
  #define DEF_TEXTBOX_CURSOR              "arrow"
#else
  #define DEF_CHECKBOX_CURSOR             "hand2"
  #define DEF_RADIOBUTTON_CURSOR          "hand2"
  #define DEF_TEXTBOX_CURSOR              "hand2"
#endif /*WIN32*/

#define DEF_CHECKBOX_ACTIVE_COLORS      "0"
#define DEF_CHECKBOX_ACTIVE_RELIEF      "raised"
#define DEF_CHECKBOX_BORDERWIDTH        "1"
#define DEF_CHECKBOX_BOX_COLOR          RGB_GREY50
#define DEF_CHECKBOX_CHECK_COLOR        "red"
#define DEF_CHECKBOX_COMMAND            (char *)NULL
#define DEF_CHECKBOX_EDITABLE           "1"
#define DEF_CHECKBOX_FILL_COLOR         RGB_WHITE
#define DEF_CHECKBOX_FONT               (char *)NULL
#define DEF_CHECKBOX_GAP                "2"
#define DEF_CHECKBOX_LINEWIDTH          "2"
#define DEF_CHECKBOX_OFFVALUE           "0"
#define DEF_CHECKBOX_ONVALUE            "1"
#define DEF_CHECKBOX_RELIEF             "flat"
#define DEF_CHECKBOX_SHOWVALUE          "yes"
#define DEF_CHECKBOX_SIZE               "0"
#define DEF_CHECKBOX_UNDERLINE_ACTIVE   "1"
#define DEF_COMBOBOX_ACTIVE_ARROW_BG    RGB_GREY95
#define DEF_COMBOBOX_ACTIVE_COLORS      "0"
#define DEF_COMBOBOX_ACTIVE_RELIEF      "flat"
#define DEF_COMBOBOX_ARROW_BG           STD_NORMAL_BACKGROUND
#define DEF_COMBOBOX_ARROW_BORDERWIDTH  "2"
#define DEF_COMBOBOX_ARROW_RELIEF       "raised"
#define DEF_COMBOBOX_BORDERWIDTH        "1"
#define DEF_COMBOBOX_CURSOR             (char *)NULL
#define DEF_COMBOBOX_EDITABLE           "1"
#define DEF_COMBOBOX_FONT               (char *)NULL
#define DEF_COMBOBOX_ICON_VARIABLE      (char *)NULL
#define DEF_COMBOBOX_MENU               (char *)NULL
#define DEF_COMBOBOX_POSTED_RELIEF      "sunken"
#define DEF_COMBOBOX_POST_CMD           (char *)NULL
#define DEF_COMBOBOX_RELIEF             "flat"
#define DEF_COMBOBOX_STATE              "normal"
#define DEF_COMBOBOX_TEXT               (char *)NULL
#define DEF_COMBOBOX_TEXT_VARIABLE      (char *)NULL
#define DEF_COMBOBOX_UNDERLINE_ACTIVE   "1"
#define DEF_IMAGEBOX_ACTIVE_COLORS      "0"
#define DEF_IMAGEBOX_ACTIVE_RELIEF      "flat"
#define DEF_IMAGEBOX_BORDERWIDTH        "1"
#define DEF_IMAGEBOX_COMMAND            (char *)NULL
#define DEF_IMAGEBOX_CURSOR             (char *)NULL
#define DEF_IMAGEBOX_EDITABLE           "0"
#define DEF_IMAGEBOX_FONT               (char *)NULL
#define DEF_IMAGEBOX_RELIEF             "flat"
#define DEF_IMAGEBOX_SHOW_TEXT          "1"
#define DEF_IMAGEBOX_SIDE               "left"
#define DEF_IMAGEBOX_UNDERLINE_ACTIVE   "1"
#define DEF_RADIOBUTTON_ACTIVE_COLORS   "0"
#define DEF_RADIOBUTTON_ACTIVE_RELIEF   "raised"
#define DEF_RADIOBUTTON_BORDERWIDTH     "1"
#define DEF_RADIOBUTTON_BOX_COLOR       (char *)NULL
#define DEF_RADIOBUTTON_COMMAND         (char *)NULL
#define DEF_RADIOBUTTON_EDITABLE        "1"
#define DEF_RADIOBUTTON_FILL_COLOR      "white"
#define DEF_RADIOBUTTON_FONT            (char *)NULL
#define DEF_RADIOBUTTON_GAP             "2"
#define DEF_RADIOBUTTON_INDICATOR_COLOR "red"
#define DEF_RADIOBUTTON_LINEWIDTH       "2"
#define DEF_RADIOBUTTON_OFFVALUE        "0"
#define DEF_RADIOBUTTON_ONVALUE         "1"
#define DEF_RADIOBUTTON_RELIEF          "flat"
#define DEF_RADIOBUTTON_SHOWVALUE       "yes"
#define DEF_RADIOBUTTON_SIZE            "15"
#define DEF_RADIOBUTTON_UNDERLINE_ACTIVE        "1"
#define DEF_TEXTBOX_ACTIVE_COLORS       "0"
#define DEF_TEXTBOX_ACTIVE_RELIEF       "flat"
#define DEF_TEXTBOX_BORDERWIDTH "1"
#define DEF_TEXTBOX_COMMAND             (char *)NULL
#define DEF_TEXTBOX_EDITABLE            "0"
#define DEF_TEXTBOX_EDITOR              (char *)NULL
#define DEF_TEXTBOX_FONT                (char *)NULL
#define DEF_TEXTBOX_RELIEF              "flat"
#define DEF_TEXTBOX_SIDE                "left"
#define DEF_TEXTBOX_UNDERLINE_ACTIVE    "1"

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

/* 
 * TextBoxStyle --
 *
 *      Treats the cell as a plain text box that can be edited (via a popup
 *      text editor widget).  The text box consists of an option icon, and
 *      a text string.  The icon may be to the left or right of the text.
 */
typedef struct {
    int refCount;                       /* Usage reference count.  A
                                         * reference count of zero
                                         * indicates that the style is no
                                         * longer used and may be freed. */
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
    TreeView *viewPtr;                  /* Treeview widget containing this 
                                         * style. */
    Blt_ChainLink link;                 /* If non-NULL, pointer of the
                                         * style in a list of all newly
                                         * created styles. */
    /* General style fields. */
    Tk_Cursor cursor;                   /* X Cursor */
    Icon icon;                          /* If non-NULL, is a Tk_Image to be
                                         * drawn in the cell. */
    int gap;                            /* # pixels gap between icon and
                                         * text. */
    Blt_Font font;
    XColor *activeFg;                   /* Foreground color of cell when
                                         * active. */
    XColor *disableFg;                  /* Foreground color of cell when
                                         * disabled. */
    XColor *highlightFg;                /* Foreground color of cell when
                                         * highlighted. */
    XColor *normalFg;                   /* Normal foreground color of
                                         * cell. */
    XColor *selectedFg;                 /* Foreground color of a selected
                                         * cell. If non-NULL, overrides
                                         * default foreground color
                                         * specification. */
    Blt_Bg altBg;                       /* Alternative normal
                                         * background. */
    Blt_Bg activeBg;                    /* Background color of cell when
                                         * active. */
    Blt_Bg disabledBg;                  /* Background color of cell when
                                         * disabled. */
    Blt_Bg highlightBg;                 /* Background color of cell when
                                         * highlighted. */
    Blt_Bg normalBg;                    /* Normal background color of
                                         * cell. */
    Blt_Bg selectedBg;                  /* Background color of a selected
                                         * cell.  If non-NULL, overrides
                                         * the default background * color
                                         * specification. */
    GC activeGC;                        /* Graphics context of active
                                         * text. */
    GC disabledGC;                      /* Graphics context of disabled
                                         * text. */
    GC highlightGC;                     /* Graphics context of highlighted
                                         * text. */
    GC normalGC;                        /* Graphics context of normal
                                         * text. */
    GC selectedGC;                      /* Graphics context of selected
                                         * text. */
    Tk_Justify justify;                 /* Indicates how the text or icon
                                         * is justified within the
                                         * column. */
    int borderWidth;                    /* Width of outer border
                                         * surrounding the entire box. */
    int relief, activeRelief;           /* Relief of outer border. */
    Tcl_Obj *fmtCmdObjPtr;              /* If non-NULL, TCL procedure
                                         * called to format the style is
                                         * invoked.*/
    Blt_TreeKey key;                    /* Actual data resides in this tree
                                           cell. */
    Tcl_Obj *cmdObjPtr;
    /* TextBox-specific fields */
    Tcl_Obj *editorObjPtr;             /* If non-NULL, TCL procedure
                                         * called to allow the user to edit
                                         * the text string. */
    int side;                           /* Position of the text in relation to
                                         * the icon.  */
} TextBoxStyle;

typedef struct {
    int refCount;                       /* Usage reference count.  A
                                         * reference count of zero
                                         * indicates that the style may be
                                         * freed. */
    unsigned int flags;                 /* Bit field containing both the
                                         * style type and various flags. */
    const char *name;                   /* Instance name. */
    CellStyleClass *classPtr;           /* Contains class-specific
                                         * information such as
                                         * configuration specifications and
                                         * configure, draw, layout
                                         * etc. routines. */
    Blt_HashEntry *hashPtr;             /* If non-NULL, points to the hash
                                         * table entry for the style.  A
                                         * style that's been deleted, but
                                         * still in use (non-zero reference
                                         * count) will have no hash table
                                         * entry. */
    TreeView *viewPtr;                  
    Blt_ChainLink link;                 /* If non-NULL, pointer of the
                                         * style in a list of all newly
                                         * created styles. */
    /* General style fields. */
    Tk_Cursor cursor;                   /* X Cursor */
    Icon icon;                          /* If non-NULL, is a Tk_Image to be
                                         * drawn in the cell. */
    int gap;                            /* # pixels gap between icon and
                                         * text. */
    Blt_Font font;
    XColor *activeFg;                   /* Foreground color of cell when
                                         * active. */
    XColor *disableFg;                  /* Foreground color of cell when
                                         * disabled. */
    XColor *highlightFg;                /* Foreground color of cell when
                                         * highlighted. */
    XColor *normalFg;                   /* Normal foreground color of
                                         * cell. */
    XColor *selectedFg;                 /* Foreground color of a selected
                                         * cell. If non-NULL, overrides
                                         * default foreground color
                                         * specification. */
    Blt_Bg altBg;
    Blt_Bg activeBg;                    /* Background color of cell when
                                         * active. */
    Blt_Bg disabledBg;                  /* Background color of cell when
                                         * disabled. */
    Blt_Bg highlightBg;                 /* Background color of cell when
                                         * highlighted. */
    Blt_Bg normalBg;                    /* Normal background color of
                                         * cell. */
    Blt_Bg selectedBg;                  /* Background color of a selected
                                         * cell.  If non-NULL, overrides
                                         * the default background * color
                                         * specification. */
    GC activeGC;
    GC disabledGC;
    GC highlightGC;
    GC normalGC;
    GC selectedGC;
    Tk_Justify justify;                 /* Indicates how the text or icon
                                         * is justified within the
                                         * column. */
    int borderWidth;                    /* Width of outer border
                                         * surrounding the entire box. */
    int relief, activeRelief;           /* Relief of outer border. */
    Tcl_Obj *fmtCmdObjPtr;              /* If non-NULL, TCL procedure
                                         * called to format the style is
                                         * invoked.*/
    Blt_TreeKey key;                    /* Actual data resides in this tree
                                           cell. */
    Tcl_Obj *cmdObjPtr;
    /* Checkbox specific fields. */
    int size;                           /* Size of the checkbox. */
    Tcl_Obj *onValueObjPtr;
    Tcl_Obj *offValueObjPtr;
    int lineWidth;                      /* Linewidth of the surrounding
                                         * box. */
    XColor *boxColor;                   /* Rectangle (box) color (grey). */
    XColor *fillColor;                  /* Fill color (white) */
    XColor *checkColor;                 /* Check color (red). */

    TextLayout *onPtr, *offPtr;
    
    Blt_Painter painter;
    Blt_Picture selectedBox;
    Blt_Picture normalBox;
    Blt_Picture disabledBox;
} CheckBoxStyle;

typedef struct {
    int refCount;                       /* Usage reference count.  A
                                         * reference count of zero
                                         * indicates that the style may be
                                         * freed. */
    unsigned int flags;                 /* Bit field containing both the
                                         * style type and various flags. */
    const char *name;                   /* Instance name. */
    CellStyleClass *classPtr;           /* Contains class-specific
                                         * information such as
                                         * configuration specifications and
                                         * configure, draw, layout
                                         * etc. routines. */
    Blt_HashEntry *hashPtr;             /* If non-NULL, points to the hash
                                         * table entry for the style.  A
                                         * style that's been deleted, but
                                         * still in use (non-zero reference
                                         * count) will have no hash table
                                         * entry. */
    TreeView *viewPtr;                  
    Blt_ChainLink link;                 /* If non-NULL, pointer of the
                                         * style in a list of all newly
                                         * created styles. */
    /* General style fields. */
    Tk_Cursor cursor;                   /* X Cursor */
    Icon icon;                          /* If non-NULL, is a Tk_Image to be
                                         * drawn in the cell. */
    int gap;                            /* # pixels gap between icon and
                                         * text. */
    Blt_Font font;
    XColor *activeFg;                   /* Foreground color of cell when
                                         * active. */
    XColor *disableFg;                  /* Foreground color of cell when
                                         * disabled. */
    XColor *highlightFg;                /* Foreground color of cell when
                                         * highlighted. */
    XColor *normalFg;                   /* Normal foreground color of
                                         * cell. */
    XColor *selectedFg;                 /* Foreground color of a selected
                                         * cell. If non-NULL, overrides
                                         * default foreground color
                                         * specification. */
    Blt_Bg altBg;
    Blt_Bg activeBg;                    /* Background color of cell when
                                         * active. */
    Blt_Bg disabledBg;                  /* Background color of cell when
                                         * disabled. */
    Blt_Bg highlightBg;                 /* Background color of cell when
                                         * highlighted. */
    Blt_Bg normalBg;                    /* Normal background color of
                                         * cell. */
    Blt_Bg selectedBg;                  /* Background color of a selected
                                         * cell.  If non-NULL, overrides
                                         * the default background * color
                                         * specification. */
    GC activeGC;
    GC disabledGC;
    GC highlightGC;
    GC normalGC;
    GC selectedGC;
    Tk_Justify justify;                 /* Indicates how the text or icon is
                                         * justified within the column. */
    int borderWidth;                    /* Width of outer border surrounding
                                         * the entire box. */
    int relief, activeRelief;           /* Relief of outer border. */
    Tcl_Obj *fmtCmdObjPtr;              /* If non-NULL, TCL procedure
                                         * called to format the style is
                                         * invoked.*/
    Blt_TreeKey key;                    /* Actual data resides in this tree
                                           cell. */
    Tcl_Obj *cmdObjPtr;

    /* ComboBox-specific fields */

    int arrowBorderWidth;
    int scrollWidth;
    /*  */
    int postedRelief;

    int textLen;
    /*
     * The combobox contains an optional icon and text string. 
     */
    Tcl_Obj *iconVarObjPtr;             /* Name of TCL variable.  If
                                         * non-NULL, this variable contains
                                         * the name of an image
                                         * representing the icon.  This
                                         * overrides the value of the above
                                         * field. */
    const char *text;                   /* Text string to be displayed in
                                         * the button if an image has no
                                         * been designated. Its value is
                                         * overridden by the -textvariable
                                         * option. */
    Tcl_Obj *textVarObjPtr;             /* Name of TCL variable.  If
                                         * non-NULL, this variable contains
                                         * the text string to be displayed
                                         * in the button. This overrides
                                         * the above field. */

    int prefWidth;                      /* Desired width of window,
                                         * measured in average
                                         * characters. */
    int inset;
    short int iw, ih;
    short int width, height;
    Tcl_Obj *menuObjPtr;                /* Name of the menu to be posted by
                                         * this style. */
    Tcl_Obj *postCmdObjPtr;             /* If non-NULL, command to be
                                         * executed when this menu is
                                         * posted. */

    short int arrowWidth, arrowHeight;
    /*  
     * Arrow (button) Information:
     *
     * The arrow is a button with an optional 3D border.
     */
    int arrowPad;
    int arrowRelief;
    int reqArrowWidth;
    Blt_Bg arrowBg;
    Blt_Bg activeArrowBg;
    const char dummy1[2000];
    int arrow;
    Blt_Painter painter;
} ComboBoxStyle;

typedef struct {
    int refCount;                       /* Usage reference count.  A
                                         * reference count of zero
                                         * indicates that the style may be
                                         * freed. */
    unsigned int flags;                 /* Bit field containing both the
                                         * style type and various flags. */
    const char *name;                   /* Instance name. */
    CellStyleClass *classPtr;           /* Contains class-specific
                                         * information such as
                                         * configuration specifications and
                                         * configure, draw, layout
                                         * etc. routines. */
    Blt_HashEntry *hashPtr;             /* If non-NULL, points to the hash
                                         * table entry for the style.  A
                                         * style that's been deleted, but
                                         * still in use (non-zero reference
                                         * count) will have no hash table
                                         * entry. */
    TreeView *viewPtr;                  
    Blt_ChainLink link;                 /* If non-NULL, pointer of the
                                         * style in a list of all newly
                                         * created styles. */
    /* General style fields. */
    Tk_Cursor cursor;                   /* X Cursor */
    Icon icon;                          /* If non-NULL, is a Tk_Image to be
                                         * drawn in the cell. */
    int gap;                            /* # pixels gap between icon and
                                         * text. */
    Blt_Font font;
    XColor *activeFg;                   /* Foreground color of cell when
                                         * active. */
    XColor *disableFg;                  /* Foreground color of cell when
                                         * disabled. */
    XColor *highlightFg;                /* Foreground color of cell when
                                         * highlighted. */
    XColor *normalFg;                   /* Normal foreground color of
                                         * cell. */
    XColor *selectedFg;                 /* Foreground color of a selected
                                         * cell. If non-NULL, overrides
                                         * default foreground color
                                         * specification. */
    Blt_Bg altBg;
    Blt_Bg activeBg;                    /* Background color of cell when
                                         * active. */
    Blt_Bg disabledBg;                  /* Background color of cell when
                                         * disabled. */
    Blt_Bg highlightBg;                 /* Background color of cell when
                                         * highlighted. */
    Blt_Bg normalBg;                    /* Normal background color of
                                         * cell. */
    Blt_Bg selectedBg;                  /* Background color of a selected
                                         * cell.  If non-NULL, overrides
                                         * the default background * color
                                         * specification. */
    GC activeGC;
    GC disabledGC;
    GC highlightGC;
    GC normalGC;
    GC selectedGC;
    int borderWidth;                    /* Width of outer border
                                         * surrounding the entire box. */
    Tk_Justify justify;                 /* Indicates how the text or icon
                                         * is justified within the
                                         * column. */
    int relief, activeRelief;           /* Relief of outer border. */
    Blt_TreeKey key;                    /* Actual data resides in this tree
                                         * cell. */
    Tcl_Obj *cmdObjPtr;

    /* ImageBox-specific fields */
    int side;                           /* Position the text (top or
                                         * bottom) in relation to the
                                         * image.  */
} ImageBoxStyle;

typedef struct {
    int refCount;                       /* Usage reference count.  A
                                         * reference count of zero
                                         * indicates that the style may be
                                         * freed. */
    unsigned int flags;                 /* Bit field containing both the
                                         * style type and various flags. */
    const char *name;                   /* Instance name. */
    CellStyleClass *classPtr;           /* Contains class-specific
                                         * information such as
                                         * configuration specifications and
                                         * configure, draw, layout
                                         * etc. routines. */
    Blt_HashEntry *hashPtr;             /* If non-NULL, points to the hash
                                         * table entry for the style.  A
                                         * style that's been deleted, but
                                         * still in use (non-zero reference
                                         * count) will have no hash table
                                         * entry. */
    TreeView *viewPtr;                  
    Blt_ChainLink link;                 /* If non-NULL, pointer of the
                                         * style in a list of all newly
                                         * created styles. */
    /* General style fields. */
    Tk_Cursor cursor;                   /* X Cursor */
    Icon icon;                          /* If non-NULL, is a Tk_Image to be
                                         * drawn in the cell. */
    int gap;                            /* # pixels gap between icon and
                                         * text. */
    Blt_Font font;
    XColor *activeFg;                   /* Foreground color of cell when
                                         * active. */
    XColor *disableFg;                  /* Foreground color of cell when
                                         * disabled. */
    XColor *highlightFg;                /* Foreground color of cell when
                                         * highlighted. */
    XColor *normalFg;                   /* Normal foreground color of
                                         * cell. */
    XColor *selectedFg;                 /* Foreground color of a selected
                                         * cell. If non-NULL, overrides
                                         * default foreground color
                                         * specification. */
    Blt_Bg altBg;
    Blt_Bg activeBg;                    /* Background color of cell when
                                         * active. */
    Blt_Bg disabledBg;                  /* Background color of cell when
                                         * disabled. */
    Blt_Bg highlightBg;                 /* Background color of cell when
                                         * highlighted. */
    Blt_Bg normalBg;                    /* Normal background color of
                                         * cell. */
    Blt_Bg selectedBg;                  /* Background color of a selected
                                         * cell.  If non-NULL, overrides
                                         * the default background * color
                                         * specification. */
    GC activeGC;
    GC disabledGC;
    GC highlightGC;
    GC normalGC;
    GC selectedGC;
    Tk_Justify justify;                 /* Indicates how the text or icon
                                         * is justified within the
                                         * column. */
    int borderWidth;                    /* Width of outer border
                                         * surrounding the entire box. */
    int relief, activeRelief;           /* Relief of outer border. */
    Tcl_Obj *fmtCmdObjPtr;              /* If non-NULL, TCL procedure
                                         * called to format the style is
                                         * invoked.*/
    Blt_TreeKey key;                    /* Actual data resides in this tree
                                           cell. */
    Tcl_Obj *cmdObjPtr;

    /* Radiobutton specific fields. */
    int size;                           /* Size of the radiobutton. */
    Tcl_Obj *onValueObjPtr;
    Tcl_Obj *offValueObjPtr;
    int lineWidth;                      /* Linewidth of the surrounding
                                         * box. */
    XColor *boxColor;                   /* Rectangle (box) color (grey). */
    XColor *fillColor;                  /* Fill color (white) */
    XColor *checkColor;                 /* Check color (red). */

    int state;

    Blt_Painter painter;
} RadioButtonStyle;

static Blt_ConfigSpec checkBoxStyleSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
        "ActiveBackground", DEF_ACTIVE_BG, 
        Blt_Offset(CheckBoxStyle, activeBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", 
        (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_BITMASK, "-activecolors", "activeColors", "ActiveColors", 
        DEF_CHECKBOX_ACTIVE_COLORS, Blt_Offset(CheckBoxStyle, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)ACTIVE_COLORS},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeForeground", (char *)NULL, 
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
        DEF_NORMAL_BG, Blt_Offset(CheckBoxStyle, normalBg), BLT_CONFIG_NULL_OK},
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
        Blt_Offset(CheckBoxStyle, disabledBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground", 
       "DisabledForeground", DEF_DISABLE_FG, 
        Blt_Offset(CheckBoxStyle, disableFg), 0},
    {BLT_CONFIG_SYNONYM, "-disabledbg", "disabledBackground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-disabledfg", "disabledForeground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_BITMASK, "-edit", "edit", "Edit", DEF_CHECKBOX_EDITABLE, 
        Blt_Offset(CheckBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)EDITABLE},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_CHECKBOX_FONT, 
        Blt_Offset(CheckBoxStyle, font), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", 
        DEF_NORMAL_FG, Blt_Offset(CheckBoxStyle, normalFg), 0},
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
    {BLT_CONFIG_OBJ, "-offvalue", "offValue", "OffValue", 
        DEF_CHECKBOX_OFFVALUE, Blt_Offset(CheckBoxStyle, offValueObjPtr), 0},
    {BLT_CONFIG_OBJ, "-onvalue", "onValue", "OnValue", 
        DEF_CHECKBOX_ONVALUE, Blt_Offset(CheckBoxStyle, onValueObjPtr), 0},
    {BLT_CONFIG_STRING, "-key", "key", "key", (char *)NULL, 
        Blt_Offset(CheckBoxStyle, key), BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_CHECKBOX_RELIEF, 
        Blt_Offset(CheckBoxStyle, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
        "Foreground", DEF_SELECT_BG, Blt_Offset(CheckBoxStyle, selectedBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
        DEF_SELECT_FG, Blt_Offset(CheckBoxStyle, selectedFg), 0},
    {BLT_CONFIG_BITMASK, "-showvalue", "showValue", "ShowValue",
        DEF_CHECKBOX_SHOWVALUE, Blt_Offset(CheckBoxStyle, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SHOW_VALUE},    
    {BLT_CONFIG_BITMASK, "-underlineactive", "underlineActive", 
        "UnderlineActive", DEF_CHECKBOX_UNDERLINE_ACTIVE, 
        Blt_Offset(CheckBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)UNDERLINE_ACTIVE},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
};

static Blt_ConfigSpec comboBoxStyleSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activearrowbackground", "activeArrowBackground", 
        "ActiveArrowBackground", DEF_COMBOBOX_ACTIVE_ARROW_BG, 
        Blt_Offset(ComboBoxStyle, activeArrowBg), 0},
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
        "ActiveBackground", DEF_ACTIVE_BG, 
        Blt_Offset(ComboBoxStyle, activeBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_BITMASK, "-activecolors", "activeColors", "ActiveColors", 
        DEF_COMBOBOX_ACTIVE_COLORS, Blt_Offset(ComboBoxStyle, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)ACTIVE_COLORS},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeForeground", (char *)NULL, 
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
    {BLT_CONFIG_BACKGROUND, "-arrowbackground", "arrowBackground", 
        "ArrowBackground", DEF_COMBOBOX_ARROW_BG, 
        Blt_Offset(ComboBoxStyle, arrowBg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-arrowborderwidth", "arrowBorderWidth", 
        "ArrowBorderWidth", DEF_COMBOBOX_ARROW_BORDERWIDTH, 
        Blt_Offset(ComboBoxStyle, arrowBorderWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_NORMAL_BG, Blt_Offset(ComboBoxStyle, normalBg), BLT_CONFIG_NULL_OK},
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
        Blt_Offset(ComboBoxStyle, disabledBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground", 
       "DisabledForeground", DEF_DISABLE_FG, 
        Blt_Offset(ComboBoxStyle, disableFg), 0},
    {BLT_CONFIG_SYNONYM, "-disabledbg", "disabledBackground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-disabledfg", "disabledForeground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_BITMASK, "-edit", "edit", "Edit", DEF_COMBOBOX_EDITABLE, 
        Blt_Offset(ComboBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)EDITABLE},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_COMBOBOX_FONT, 
        Blt_Offset(ComboBoxStyle, font), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
        DEF_NORMAL_FG, Blt_Offset(ComboBoxStyle, normalFg), 0},
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
        "Foreground", DEF_SELECT_BG, Blt_Offset(ComboBoxStyle, selectedBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
        DEF_SELECT_FG, Blt_Offset(ComboBoxStyle, selectedFg), 0},
    {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_COMBOBOX_STATE, 
        Blt_Offset(ComboBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        &stateOption},
    {BLT_CONFIG_CUSTOM, "-text", "text", "Text", DEF_COMBOBOX_TEXT, 
        Blt_Offset(ComboBoxStyle, text), 0, &textOption},
    {BLT_CONFIG_CUSTOM, "-textvariable", "textVariable", "TextVariable", 
        DEF_COMBOBOX_TEXT_VARIABLE, Blt_Offset(ComboBoxStyle, textVarObjPtr), 
        BLT_CONFIG_NULL_OK, &textVarOption},
    {BLT_CONFIG_BITMASK, "-underlineactive", "underlineActive", 
        "UnderlineActive", DEF_COMBOBOX_UNDERLINE_ACTIVE, 
        Blt_Offset(ComboBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)UNDERLINE_ACTIVE},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
};

static Blt_ConfigSpec imageBoxStyleSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
        "ActiveBackground", DEF_ACTIVE_BG, 
        Blt_Offset(ImageBoxStyle, activeBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", 
        (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeFackground", 
        (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_BITMASK, "-activecolors", "activeColors", "ActiveColors", 
        DEF_IMAGEBOX_ACTIVE_COLORS, Blt_Offset(ImageBoxStyle, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)ACTIVE_COLORS},
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
        DEF_NORMAL_BG, Blt_Offset(ImageBoxStyle, normalBg), BLT_CONFIG_NULL_OK},
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
        Blt_Offset(ImageBoxStyle, disabledBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground", 
       "DisabledForeground", DEF_DISABLE_FG, 
        Blt_Offset(ImageBoxStyle, disableFg), 0},
    {BLT_CONFIG_SYNONYM, "-disabledbg", "disabledBackground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-disabledfg", "disabledForeground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_BITMASK, "-edit", "edit", "Edit", DEF_IMAGEBOX_EDITABLE, 
        Blt_Offset(ImageBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)EDITABLE},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_IMAGEBOX_FONT,
        Blt_Offset(ImageBoxStyle, font), BLT_CONFIG_NULL_OK},
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
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_IMAGEBOX_RELIEF, 
        Blt_Offset(ImageBoxStyle, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
        "Foreground", DEF_SELECT_BG, Blt_Offset(ImageBoxStyle, selectedBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
        DEF_SELECT_FG, Blt_Offset(ImageBoxStyle, selectedFg), 0},
    {BLT_CONFIG_BITMASK, "-showtext", "showText", "ShowText", 
        DEF_IMAGEBOX_SHOW_TEXT, Blt_Offset(ImageBoxStyle, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SHOW_TEXT},
    {BLT_CONFIG_SIDE, "-side", "side", "side", DEF_IMAGEBOX_SIDE, 
        Blt_Offset(ImageBoxStyle, side), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-underlineactive", "underlineActive", 
        "UnderlineActive", DEF_IMAGEBOX_UNDERLINE_ACTIVE, 
        Blt_Offset(ImageBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)UNDERLINE_ACTIVE},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
        0, 0}
};

static Blt_ConfigSpec radioButtonStyleSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
        "ActiveBackground", DEF_ACTIVE_BG, 
        Blt_Offset(RadioButtonStyle, activeBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", 
        (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_BITMASK, "-activecolors", "activeColors", "ActiveColors", 
        DEF_RADIOBUTTON_ACTIVE_COLORS, Blt_Offset(RadioButtonStyle, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)ACTIVE_COLORS},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeForeground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
        "ActiveForeground", DEF_ACTIVE_FG, 
        Blt_Offset(RadioButtonStyle, activeFg), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "ActiveRelief", 
        DEF_RADIOBUTTON_ACTIVE_RELIEF, Blt_Offset(RadioButtonStyle, activeRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-altbg", "alternateBackground", (char *)NULL,
        (char *)NULL, 0, 0},
    {BLT_CONFIG_BACKGROUND, "-alternatebackground", "alternateBackground", 
        "Background", DEF_ALT_BG, Blt_Offset(RadioButtonStyle, altBg), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_NORMAL_BG, Blt_Offset(RadioButtonStyle, normalBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_RADIOBUTTON_BORDERWIDTH, Blt_Offset(RadioButtonStyle, borderWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_POS, "-boxsize", "boxSize", "BoxSize", 
        DEF_RADIOBUTTON_SIZE, Blt_Offset(RadioButtonStyle, size), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", DEF_RADIOBUTTON_COMMAND, 
        Blt_Offset(RadioButtonStyle, cmdObjPtr), 0},
    {BLT_CONFIG_CURSOR, "-cursor", "cursor", "Cursor", DEF_RADIOBUTTON_CURSOR, 
        Blt_Offset(RadioButtonStyle, cursor), 0},
    {BLT_CONFIG_BACKGROUND, "-disabledbackground", "disabledBackground",
        "DisabledBackground", DEF_DISABLE_BG, 
        Blt_Offset(RadioButtonStyle, disabledBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground", 
       "DisabledForeground", DEF_DISABLE_FG, 
        Blt_Offset(RadioButtonStyle, disableFg), 0},
    {BLT_CONFIG_SYNONYM, "-disabledbg", "disabledBackground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-disabledfg", "disabledForeground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_BITMASK, "-edit", "edit", "Edit", DEF_RADIOBUTTON_EDITABLE, 
        Blt_Offset(RadioButtonStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)EDITABLE},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_RADIOBUTTON_FONT, 
        Blt_Offset(RadioButtonStyle, font), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", 
        DEF_NORMAL_FG, Blt_Offset(RadioButtonStyle, normalFg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-gap", "gap", "Gap", DEF_RADIOBUTTON_GAP, 
        Blt_Offset(RadioButtonStyle, gap), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground",
        "HighlightBackground", DEF_HIGHLIGHT_BG, 
        Blt_Offset(RadioButtonStyle, highlightBg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", 
        "HighlightForeground", DEF_HIGHLIGHT_FG, 
         Blt_Offset(RadioButtonStyle, highlightFg), 0},
    {BLT_CONFIG_SYNONYM, "-highlightbg", "highlightBackground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-highlightfg", "highlightForeground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", (char *)NULL, 
        Blt_Offset(RadioButtonStyle, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_STRING, "-key", "key", "key", (char *)NULL, 
        Blt_Offset(RadioButtonStyle, key), BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_JUSTIFY, 
        Blt_Offset(RadioButtonStyle, justify), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth",
        DEF_RADIOBUTTON_LINEWIDTH, Blt_Offset(RadioButtonStyle, lineWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_COLOR, "-checkcolor", "checkColor", "CheckColor", 
        DEF_RADIOBUTTON_INDICATOR_COLOR, Blt_Offset(RadioButtonStyle, 
        checkColor), 0},
    {BLT_CONFIG_COLOR, "-boxcolor", "boxColor", "BoxColor", 
        DEF_RADIOBUTTON_BOX_COLOR, Blt_Offset(RadioButtonStyle, boxColor), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-fillcolor", "fillColor", "FillColor", 
        DEF_RADIOBUTTON_FILL_COLOR, Blt_Offset(RadioButtonStyle, fillColor), 0},
    {BLT_CONFIG_STRING, "-key", "key", "key", (char *)NULL, 
        Blt_Offset(RadioButtonStyle, key), BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_OBJ, "-offvalue", "offValue", "OffValue", 
        DEF_RADIOBUTTON_OFFVALUE, Blt_Offset(RadioButtonStyle, offValueObjPtr),
        0},
    {BLT_CONFIG_OBJ, "-onvalue", "onValue", "OnValue", DEF_RADIOBUTTON_ONVALUE,
        Blt_Offset(RadioButtonStyle, onValueObjPtr), 0},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_RADIOBUTTON_RELIEF, 
        Blt_Offset(RadioButtonStyle, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
        "Foreground", DEF_SELECT_BG, Blt_Offset(RadioButtonStyle, selectedBg), 
        0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
        DEF_SELECT_FG, Blt_Offset(RadioButtonStyle, selectedFg), 0},
    {BLT_CONFIG_BITMASK, "-showvalue", "showValue", "ShowValue",
        DEF_RADIOBUTTON_SHOWVALUE, Blt_Offset(RadioButtonStyle, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SHOW_VALUE},    
    {BLT_CONFIG_BITMASK, "-underlineactive", "underlineActive", 
        "UnderlineActive", DEF_RADIOBUTTON_UNDERLINE_ACTIVE, 
        Blt_Offset(RadioButtonStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)UNDERLINE_ACTIVE},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
};

static Blt_ConfigSpec textBoxStyleSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
        "ActiveBackground", DEF_ACTIVE_BG, 
        Blt_Offset(TextBoxStyle, activeBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-activebg", "activeBackground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_BITMASK, "-activecolors", "activeColors", "ActiveColors", 
        DEF_TEXTBOX_ACTIVE_COLORS, Blt_Offset(TextBoxStyle, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)ACTIVE_COLORS},
    {BLT_CONFIG_SYNONYM, "-activefg", "activeForeground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", 
        "ActiveForeground", DEF_ACTIVE_FG, 
        Blt_Offset(TextBoxStyle, activeFg), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "ActiveRelief", 
        DEF_TEXTBOX_ACTIVE_RELIEF, Blt_Offset(TextBoxStyle, activeRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-altbg", "alternateBackground", (char *)NULL,
        (char *)NULL, 0, 0},
    {BLT_CONFIG_BACKGROUND, "-alternatebackground", "alternateBackground", 
        "Background", DEF_ALT_BG, Blt_Offset(TextBoxStyle, altBg), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_NORMAL_BG, Blt_Offset(TextBoxStyle, normalBg), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_TEXTBOX_BORDERWIDTH, Blt_Offset(TextBoxStyle, borderWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", DEF_TEXTBOX_COMMAND, 
        Blt_Offset(TextBoxStyle, fmtCmdObjPtr), 0},
    {BLT_CONFIG_CURSOR, "-cursor", "cursor", "Cursor", DEF_TEXTBOX_CURSOR, 
        Blt_Offset(TextBoxStyle, cursor), 0},
    {BLT_CONFIG_BACKGROUND, "-disabledbackground", "disabledBackground",
        "DisabledBackground", DEF_DISABLE_BG, 
        Blt_Offset(TextBoxStyle, disabledBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground", 
       "DisabledForeground", DEF_DISABLE_FG, 
        Blt_Offset(TextBoxStyle, disableFg), 0},
    {BLT_CONFIG_SYNONYM, "-disabledbg", "disabledBackground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-disabledfg", "disabledForeground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_BITMASK, "-edit", "edit", "Edit", DEF_TEXTBOX_EDITABLE, 
        Blt_Offset(TextBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)EDITABLE},
    {BLT_CONFIG_OBJ, "-editor", "editor", "Editor", DEF_TEXTBOX_EDITOR, 
        Blt_Offset(TextBoxStyle, editorObjPtr), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_TEXTBOX_FONT, 
        Blt_Offset(TextBoxStyle, font), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", 
        DEF_NORMAL_FG, Blt_Offset(TextBoxStyle, normalFg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-gap", "gap", "Gap", DEF_GAP, 
        Blt_Offset(TextBoxStyle, gap), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground",
        "HighlightBackground", DEF_HIGHLIGHT_BG, 
        Blt_Offset(TextBoxStyle, highlightBg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_SYNONYM, "-highlightbg", "highlightBackground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-highlightfg", "highlightForeground", (char *)NULL, 
        (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", 
        "HighlightForeground", DEF_HIGHLIGHT_FG, 
        Blt_Offset(TextBoxStyle, highlightFg), 0},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", (char *)NULL, 
        Blt_Offset(TextBoxStyle, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_STRING, "-key", "key", "key",   (char *)NULL, 
        Blt_Offset(TextBoxStyle, key), BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_JUSTIFY, 
        Blt_Offset(TextBoxStyle, justify), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_TEXTBOX_RELIEF, 
        Blt_Offset(TextBoxStyle, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
        "Foreground", DEF_SELECT_BG, Blt_Offset(TextBoxStyle, selectedBg), 0},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
        DEF_SELECT_FG, Blt_Offset(TextBoxStyle, selectedFg), 0},
    {BLT_CONFIG_SIDE, "-side", "side", "side", DEF_TEXTBOX_SIDE, 
        Blt_Offset(TextBoxStyle, side), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-underlineactive", "underlineActive", 
        "UnderlineActive", DEF_TEXTBOX_UNDERLINE_ACTIVE, 
        Blt_Offset(TextBoxStyle, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)UNDERLINE_ACTIVE},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
        0, 0}
};

static CellStyleConfigureProc CheckBoxStyleConfigureProc;
static CellStyleConfigureProc ComboBoxStyleConfigureProc;
static CellStyleConfigureProc ImageBoxStyleConfigureProc;
static CellStyleConfigureProc RadioButtonStyleConfigureProc;
static CellStyleConfigureProc TextBoxStyleConfigureProc;
static CellStyleDrawProc CheckBoxStyleDrawProc;
static CellStyleDrawProc ComboBoxStyleDrawProc;
static CellStyleDrawProc ImageBoxStyleDrawProc;
static CellStyleDrawProc RadioButtonStyleDrawProc;
static CellStyleDrawProc TextBoxStyleDrawProc;
static CellStyleFreeProc CheckBoxStyleFreeProc;
static CellStyleFreeProc ComboBoxStyleFreeProc;
static CellStyleFreeProc ImageBoxStyleFreeProc;
static CellStyleFreeProc RadioButtonStyleFreeProc;
static CellStyleFreeProc TextBoxStyleFreeProc;
static CellStyleGeometryProc CheckBoxStyleGeometryProc;
static CellStyleGeometryProc ComboBoxStyleGeometryProc;
static CellStyleGeometryProc ImageBoxStyleGeometryProc;
static CellStyleGeometryProc RadioButtonStyleGeometryProc;
static CellStyleGeometryProc TextBoxStyleGeometryProc;
static CellStyleIdentifyProc CheckBoxStyleIdentifyProc;
static CellStyleIdentifyProc ComboBoxStyleIdentifyProc;
static CellStyleIdentifyProc RadioButtonStyleIdentifyProc;

static int
RowSelected(Entry *entryPtr)
{
    Blt_HashEntry *hPtr;
    TreeView *viewPtr;

    viewPtr = entryPtr->viewPtr;
    hPtr = Blt_FindHashEntry(&viewPtr->sel.table, (char *)entryPtr);
    return (hPtr != NULL);
}

static INLINE CellStyle *
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

/* 
 *---------------------------------------------------------------------------
 *
 * PropagateStyleChanges --
 *
 *      Mark the cells using this style so that their geometry will be
 *      recomputed.  When a style causes the cell to change its size
 *      (usually by changing the font), we need to tell the widget which
 *      cells to re-measure.  This routine check which cells are currently
 *      using this style and marks them.
 *
 *---------------------------------------------------------------------------
 */
static void
PropagateStyleChanges(CellStyle *cellStylePtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    TreeView *viewPtr;

    viewPtr = cellStylePtr->viewPtr;
    for (hPtr = Blt_FirstHashEntry(&viewPtr->entryTable, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        Entry *entryPtr;
        Cell *cellPtr;

        entryPtr = Blt_GetHashValue(hPtr);
        for (cellPtr = entryPtr->cells; cellPtr != NULL; 
             cellPtr = cellPtr->nextPtr) {
            CellStyle *stylePtr;

            stylePtr = GetCurrentStyle(viewPtr, cellPtr->colPtr, cellPtr);
            if (stylePtr == cellStylePtr) {
                cellPtr->flags |= GEOMETRY;
                entryPtr->flags |= GEOMETRY;
                viewPtr->flags |= LAYOUT_PENDING;
            }
        }
    }
}
            
static Tcl_Obj *
FormatCell(CellStyle *stylePtr, Cell *cellPtr)
{
    Column *colPtr;
    Tcl_Obj *valueObjPtr, *fmtCmdObjPtr;
    Entry *rowPtr;

    if (cellPtr->dataObjPtr != NULL) {
        Tcl_DecrRefCount(cellPtr->dataObjPtr);
        cellPtr->dataObjPtr = NULL;
    }
    if (cellPtr->tkImage != NULL) {
        Tk_FreeImage(cellPtr->tkImage);
        cellPtr->tkImage = NULL;
    }
    colPtr = cellPtr->colPtr;
    rowPtr = cellPtr->entryPtr;
    if (GetData(rowPtr, colPtr->key, &valueObjPtr) != TCL_OK) {
        return NULL;                            /* No data ??? */
    }
    if (valueObjPtr == NULL) {
        return NULL;
    }
    if (stylePtr->fmtCmdObjPtr != NULL) {
        fmtCmdObjPtr = stylePtr->fmtCmdObjPtr;
    } else if (colPtr->fmtCmdObjPtr != NULL) {
        fmtCmdObjPtr = colPtr->fmtCmdObjPtr;
    } else {
        fmtCmdObjPtr = NULL;
    }
    if (fmtCmdObjPtr != NULL) {
        Tcl_Interp *interp = rowPtr->viewPtr->interp;
        Tcl_Obj *fmtCmdObjPtr, *objPtr;
        int result;
        
        fmtCmdObjPtr = Tcl_DuplicateObj(colPtr->fmtCmdObjPtr);
        objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(rowPtr->node));
        /* Add the node Id */
        Tcl_ListObjAppendElement(interp, fmtCmdObjPtr, objPtr);
        /* And the cell value. */
        Tcl_ListObjAppendElement(interp, fmtCmdObjPtr, valueObjPtr);
        Tcl_IncrRefCount(fmtCmdObjPtr);
        result = Tcl_EvalObjEx(interp, fmtCmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(fmtCmdObjPtr);
        if (result != TCL_OK) {
            Tcl_BackgroundError(interp);
            return NULL;
        }
        objPtr = Tcl_GetObjResult(interp);
        /* Need to make a copy of the result. */
        Tcl_IncrRefCount(objPtr);
        cellPtr->dataObjPtr = objPtr;
        return objPtr;
    } else {
        Tcl_IncrRefCount(valueObjPtr);
        cellPtr->dataObjPtr = valueObjPtr;
        return valueObjPtr;
    }
    return NULL;
}

static int
UpdateTextVariable(Tcl_Interp *interp, ComboBoxStyle *stylePtr) 
{
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    if (viewPtr->postPtr != NULL) {
        Tcl_Obj *resultObjPtr, *objPtr;

        objPtr = FormatCell((CellStyle *)stylePtr, viewPtr->postPtr);
        Tcl_IncrRefCount(objPtr);
        resultObjPtr = Tcl_ObjSetVar2(interp, stylePtr->textVarObjPtr, NULL, 
                objPtr, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
        Tcl_DecrRefCount(objPtr);
        if (resultObjPtr == NULL) {
            return TCL_ERROR;
        }
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
    Cell *cellPtr;
    Column *colPtr;
    Entry *rowPtr;
    TreeView *viewPtr = stylePtr->viewPtr;

    cellPtr = viewPtr->activeCellPtr;
    if (cellPtr == NULL) {
        return;                         /* No cell is active. */
    }
    rowPtr = cellPtr->entryPtr;
    colPtr = cellPtr->colPtr;
    Blt_Tree_SetValueByKey(viewPtr->interp, viewPtr->tree, rowPtr->node,
        colPtr->key, objPtr);
    cellPtr->flags |= GEOMETRY;
    colPtr->flags  |= GEOMETRY;
    rowPtr->flags |= GEOMETRY;
    stylePtr->viewPtr->flags |= GEOMETRY;
    Blt_TreeView_EventuallyRedraw(viewPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * IconChangedProc
 *
 *      Called when the image changes for the icon used in a cell style.
 *      Everything using style (cells, rows, and columns) are marked dirty.
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
#define MAX_ERR_MSG     1023
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

/*
 *---------------------------------------------------------------------------
 *
 * CellImageChangedProc
 *
 * Results:
 *      None.
 *
 * FIXME:  The tableview widget needs to know that the cell geometry 
 *         image needs to be recomputed.  All we have at this point is
 *         the cell.  We need the viewPtr;
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
 *      Converts the icon into its string representation (its name).
 *
 * Results:
 *      The name of the icon is returned.
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
 *      This procedure is invoked when someone changes the text variable
 *      associated with a cell in the treeview.  
 *
 * Results:
 *      NULL is always returned.
 *
 *---------------------------------------------------------------------------
 */
static char *
TextVarTraceProc(
    ClientData clientData,              /* Information about the item. */
    Tcl_Interp *interp,                 /* Interpreter containing variable. */
    const char *name1,                  /* First part of variable's name. */
    const char *name2,                  /* Second part of variable's name. */
    int flags)                          /* Describes what just happened. */
{
    ComboBoxStyle *stylePtr = clientData;

    assert(stylePtr->textVarObjPtr != NULL);
    if (flags & TCL_INTERP_DESTROYED) {
        return NULL;                    /* Interpreter is going away. */

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
 *      This procedure is invoked when someone changes the state
 *      variable associated with combobutton. 
 *
 * Results:
 *      NULL is always returned.
 *
 *---------------------------------------------------------------------------
 */
static char *
IconVarTraceProc(
    ClientData clientData,              /* Information about the item. */
    Tcl_Interp *interp,                 /* Interpreter containing variable. */
    const char *name1,                  /* First part of variable's name. */
    const char *name2,                  /* Second part of variable's name. */
    int flags)                          /* Describes what just happened. */
{
    ComboBoxStyle *stylePtr = clientData;

    assert(stylePtr->iconVarObjPtr != NULL);
    if (flags & TCL_INTERP_DESTROYED) {
        return NULL;                    /* Interpreter is going away. */

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
        const char *iconName;
        int length;
        
        /*
         * Update the combobutton's icon with the image whose name is
         * stored in the variable.
         */
        valueObjPtr = Tcl_ObjGetVar2(interp, stylePtr->iconVarObjPtr, NULL, 
                TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
        if (valueObjPtr == NULL) {
            return GetInterpResult(interp);
        }
        iconName = Tcl_GetStringFromObj(valueObjPtr, &length); 
        if (length > 0) {
            icon = GetIcon((CellStyle *)stylePtr, iconName);
            if (icon == NULL) {
                return GetInterpResult(interp);
            }
        } else {
            icon = NULL;
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
    Display *display,                   /* Not used. */
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
 *      Convert the variable to a traced variable.
 *
 * Results:
 *      The return value is a standard TCL result.  The icon variable is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToIconVarProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to report results. */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing style. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
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
 *      Return the name of the style.
 *
 * Results:
 *      The name representing the icon variable is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
IconVarToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Widget information record */
    int offset,                         /* Offset to field in structure */
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
ObjToStateProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to report results. */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing state. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    const char *string;
    char c;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'n') && (strncmp(string, "normal", length) == 0)) {
        *flagsPtr &= ~DISABLED;
    } else if ((c == 'p') && (strncmp(string, "disabled", length) == 0)) {
        *flagsPtr |= DISABLED;
    } else {
        Tcl_AppendResult(interp, "unknown state \"", string, 
            "\": should be disabled or normal.", (char *)NULL);
        return TCL_ERROR;
    }
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
    const char *string;

    if (state & DISABLED) {
        string = "disabled";
    } else {
        string = "normal";
    }
    return Tcl_NewStringObj(string, -1);
}

/*ARGSUSED*/
static void
FreeTextVarProc(
    ClientData clientData,
    Display *display,                   /* Not used. */
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
 *      Convert the variable to a traced variable.
 *
 * Results:
 *      The return value is a standard TCL result.  The text variable is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTextVarProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to report results. */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing style. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
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
 *      Return the name of the text variable.
 *
 * Results:
 *      The name representing the text variable is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TextVarToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Widget information record */
    int offset,                         /* Offset to field in structure */
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
    Tcl_Interp *interp,                 /* Interpreter to report results. */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing style. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
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
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)(widgRec);

    if (stylePtr->text == NULL) {
        return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(stylePtr->text, stylePtr->textLen);
}

static CellStyleClass textBoxStyleClass = {
    "textbox",
    "TextBoxStyle",
    textBoxStyleSpecs,
    TextBoxStyleConfigureProc,
    TextBoxStyleGeometryProc,
    TextBoxStyleDrawProc,
    NULL,                               /* identProc */
    TextBoxStyleFreeProc,
};

static CellStyleClass checkBoxStyleClass = {
    "checkbox",
    "CheckBoxStyle",
    checkBoxStyleSpecs,
    CheckBoxStyleConfigureProc,
    CheckBoxStyleGeometryProc,
    CheckBoxStyleDrawProc,
    CheckBoxStyleIdentifyProc,          
    CheckBoxStyleFreeProc,
};

static CellStyleClass comboBoxClass = {
    "combobox", 
    "ComboBoxStyle", 
    comboBoxStyleSpecs,
    ComboBoxStyleConfigureProc,
    ComboBoxStyleGeometryProc,
    ComboBoxStyleDrawProc,
    ComboBoxStyleIdentifyProc,          
    ComboBoxStyleFreeProc,
};

static CellStyleClass imageBoxStyleClass = {
    "imagebox",
    "ImageBoxStyle",
    imageBoxStyleSpecs,
    ImageBoxStyleConfigureProc,
    ImageBoxStyleGeometryProc,
    ImageBoxStyleDrawProc,
    NULL,                               /* identProc */
    ImageBoxStyleFreeProc,
};

static CellStyleClass radioButtonStyleClass = {
    "radiobutton",
    "RadioButtonStyle",
    radioButtonStyleSpecs,
    RadioButtonStyleConfigureProc,
    RadioButtonStyleGeometryProc,
    RadioButtonStyleDrawProc,
    RadioButtonStyleIdentifyProc,          
    RadioButtonStyleFreeProc,
};

/*
 *---------------------------------------------------------------------------
 *
 * NewTextBoxStyle --
 *
 *      Creates a "textbox" style.
 *
 * Results:
 *      A pointer to the new style structure.
 *
 *---------------------------------------------------------------------------
 */
static TextBoxStyle *
NewTextBoxStyle(TreeView *viewPtr, Blt_HashEntry *hPtr)
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
    stylePtr->flags = UNDERLINE_ACTIVE;
    stylePtr->refCount = 1;
    stylePtr->borderWidth = 1;
    stylePtr->link = NULL;
    Blt_SetHashValue(hPtr, stylePtr);
    return stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextBoxStyleConfigureProc --
 *
 *      Configures a "textbox" style.  This routine performs generates the GCs
 *      required for a textbox style.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      GCs are created for the style.
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

    gcMask = GCForeground | GCFont | GCDashList | GCLineWidth | GCLineStyle;
    gcValues.dashes = 1;
    gcValues.font = Blt_Font_Id(CHOOSE(viewPtr->font, stylePtr->font));
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
    if (stylePtr->disabledGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->disabledGC);
    }
    stylePtr->disabledGC = newGC;

    /* Selected text. */
    gcValues.foreground = stylePtr->selectedFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->selectedGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->selectedGC);
    }
    stylePtr->selectedGC = newGC;

    /* Active text. */
    gcValues.foreground = stylePtr->activeFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->activeGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    stylePtr->activeGC = newGC;

    /* Highlight GC  */
    gcValues.foreground = stylePtr->highlightFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    if (Blt_ConfigModified(stylePtr->classPtr->specs, "-font", (char *)NULL)) {
        /* Font sizes can change the size of the cell. */
        PropagateStyleChanges(cellStylePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TextBoxStyleGeometryProc --
 *
 *      Determines the space requirements for the "textbox" given the cell
 *      to be displayed.  Depending upon whether an icon or text is
 *      displayed and their relative placements, this routine computes the
 *      space needed for the text entry.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The width and height fields of *cellPtr* are set with the computed
 *      dimensions.
 *
 *  -----------
 *  borderwidth          |bw|fp|
 *  focus pad
 *  cell pad y
 *  text height
 *  cell pad y
 *  focus pad
 *  borderwidth
 *  rule height
 * ------------
 *---------------------------------------------------------------------------
 */
static void
TextBoxStyleGeometryProc(Cell *cellPtr, CellStyle *cellStylePtr)
{
    TextBoxStyle *stylePtr = (TextBoxStyle *)cellStylePtr;
    TreeView *viewPtr;
    int gap;
    unsigned int iw, ih, tw, th;
    Entry *rowPtr;
    Column *colPtr;

            
    viewPtr = stylePtr->viewPtr;
    cellPtr->flags &= ~GEOMETRY;        /* Remove the geometry flag from
                                         * the cell. */
    rowPtr = cellPtr->entryPtr;
    colPtr = cellPtr->colPtr;

    cellPtr->width  = 2 * (stylePtr->borderWidth + CELL_PADX + FOCUS_PAD);
    cellPtr->height = 2 * (stylePtr->borderWidth + CELL_PADY + FOCUS_PAD);
    cellPtr->width  += colPtr->ruleWidth + PADDING(colPtr->pad);
    cellPtr->height += rowPtr->ruleHeight;
    FormatCell(cellStylePtr, cellPtr);
    /* Now compute the geometry. */
    tw = th = iw = ih = 0;
    gap = 0;

    if (stylePtr->icon != NULL) {
        iw = IconWidth(stylePtr->icon);
        ih = IconHeight(stylePtr->icon);
    } 
    if (cellPtr->dataObjPtr != NULL) {        /* Text string defined. */
        TextStyle ts;
        const char *string;

        string = Tcl_GetString(cellPtr->dataObjPtr);
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
        Blt_Ts_GetExtents(&ts, string, &tw, &th);
        if (stylePtr->icon != NULL) {
            gap = stylePtr->gap;
        }
    } 
    if (stylePtr->side & (SIDE_TOP | SIDE_BOTTOM)) {
        cellPtr->width  += ODD(MAX(tw, iw));
        cellPtr->height += ODD(ih + gap + th);
    } else {
        cellPtr->width  += ODD(iw + gap + tw);
        cellPtr->height += ODD(MAX(th, ih));
    }
    cellPtr->textWidth = tw;
    cellPtr->textHeight = th;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextBoxStyleDrawProc --
 *
 *      Draws the "textbox" given the screen coordinates and the cell to be
 *      displayed.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The textbox cell is drawn.
 *
 * 1+3+1
 *---------------------------------------------------------------------------
 */
static void
TextBoxStyleDrawProc(Cell *cellPtr, Drawable drawable, CellStyle *cellStylePtr,
                     int x, int y)
{
    Blt_Bg bg;
    Column *colPtr;
    Entry *rowPtr;
    GC gc;
    TextBoxStyle *stylePtr = (TextBoxStyle *)cellStylePtr;
    TreeView *viewPtr;
    int gap, colWidth, rowHeight, cellWidth, cellHeight;
    int ix, iy, iw, ih;
    int relief;
    int tx, ty, tw, th;

    viewPtr = stylePtr->viewPtr;
    colPtr = cellPtr->colPtr;
    rowPtr = cellPtr->entryPtr;
    relief = stylePtr->relief;
    rowHeight = rowPtr->height - rowPtr->ruleHeight;
    colWidth  = colPtr->width - colPtr->ruleWidth;
    if ((colWidth <= 0) || (rowHeight <= 0)) {
        return;
    }
    if ((cellPtr->flags|rowPtr->flags|colPtr->flags) & DISABLED) {
        /* Disabled */
        bg = CHOOSE(viewPtr->disabledBg, stylePtr->disabledBg);
        gc = stylePtr->disabledGC;
    } else if ((stylePtr->flags & ACTIVE_COLORS) && 
               (viewPtr->activeCellPtr == cellPtr)) {
        /* Active */
        bg = CHOOSE(viewPtr->activeBg, stylePtr->activeBg);
        gc = stylePtr->activeGC;
        relief = stylePtr->activeRelief;
    } else if (RowSelected(rowPtr)) {
        /* Selected */
        bg = CHOOSE(viewPtr->selectedBg, stylePtr->selectedBg);
        gc = stylePtr->selectedGC;
    } else if ((cellPtr->flags|rowPtr->flags|colPtr->flags) & HIGHLIGHT) {
        /* Highlight */
        bg = stylePtr->highlightBg;
        gc = stylePtr->highlightGC;
    } else {
        /* Normal */
        if (rowPtr->flatIndex & 0x1) {
            bg = CHOOSE(viewPtr->altBg, stylePtr->altBg);
        } else {
            bg = CHOOSE(viewPtr->normalBg, stylePtr->normalBg);
        }            
        gc = stylePtr->normalGC;
    }

    /* Draw background. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, colWidth,
        rowHeight, stylePtr->borderWidth, relief);

    /* Draw Rule */
    if (rowPtr->ruleHeight > 0) {
        XFillRectangle(viewPtr->display, drawable, rowPtr->ruleGC, 
                x, y + rowHeight, colWidth, rowPtr->ruleHeight);
    }
    if (colPtr->ruleWidth > 0) {
        XFillRectangle(viewPtr->display, drawable, colPtr->ruleGC, 
                x + colWidth, y, colPtr->ruleWidth, rowHeight);
    }
    rowHeight -= 2 * stylePtr->borderWidth;
    colWidth  -= 2 * stylePtr->borderWidth - PADDING(colPtr->pad);

    x += stylePtr->borderWidth + colPtr->pad.side1;
    y += stylePtr->borderWidth;

    /* Draw the focus ring if this cell has focus. */
    if ((viewPtr->flags & FOCUS) && (viewPtr->focusCellPtr == cellPtr)) {
        XDrawRectangle(viewPtr->display, drawable, gc, x+2, y+2, colWidth - 5, 
                       rowHeight - 4);
    }
    x += CELL_PADX + FOCUS_PAD;
    y += CELL_PADY + FOCUS_PAD;
    rowHeight -= 2 * (FOCUS_PAD + CELL_PADY);
    colWidth  -= 2 * (FOCUS_PAD + CELL_PADX);

    cellHeight = cellPtr->height - 
        2 * (stylePtr->borderWidth + CELL_PADY + FOCUS_PAD);
    cellWidth  = cellPtr->width  - PADDING(colPtr->pad) - 
        2 * (stylePtr->borderWidth + CELL_PADX + FOCUS_PAD);

    /* Justify (x) and center (y) the contents of the cell. */
    if (rowHeight > cellHeight) {
        y += (rowHeight - cellHeight) / 2;
        rowHeight = cellHeight;
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
    tw = th = iw = ih = 0;              /* Suppress compiler warning. */
    gap = 0;
    if (stylePtr->icon != NULL) {
        iw = IconWidth(stylePtr->icon);
        ih = IconHeight(stylePtr->icon);
    }
    if (cellPtr->dataObjPtr != NULL) {
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
        if (rowHeight > th) {
            ty += (rowHeight - th) / 2;
        }
        ix = tx + tw + gap;
        if (rowHeight > ih) {
            iy += (rowHeight - ih) / 2;
        }
        break;
    case SIDE_LEFT:
        ix = x;
        if (rowHeight > ih) {
            iy += (rowHeight - ih) / 2;
        }
        tx = ix + iw + gap;
        if (rowHeight > th) {
            ty += (rowHeight - th) / 2;
        }
        break;
    case SIDE_TOP:
        iy = y;
        if (colWidth > iw) {
            ix = x + (colWidth - iw) / 2;
        }
        ty = iy + ih + gap;
        if (colWidth > tw) {
            tx = x + (colWidth - tw) / 2;
        }
        break;
    case SIDE_BOTTOM:
        ty = y;
        if (colWidth > tw) {
            tx = x + (colWidth - tw) / 2;
        }
        iy = ty + th + gap;
        if (colWidth > iw) {
            ix = x + (colWidth - iw) / 2;
        }
        break;
    }
    if (stylePtr->icon != NULL) {
        Tk_RedrawImage(IconBits(stylePtr->icon), 0, 0, iw, ih,drawable, ix, iy);
    }
    if (cellPtr->dataObjPtr != NULL) {
        TextStyle ts;
        int xMax;
        TextLayout *textPtr;
        const char *string;
        int length;

        string = Tcl_GetStringFromObj(cellPtr->dataObjPtr, &length);
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
        Blt_Ts_SetGC(ts, gc);
        xMax = colWidth - iw - gap;
        Blt_Ts_SetMaxLength(ts, xMax);
        textPtr = Blt_Ts_CreateLayout(string, length, &ts);
        Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, textPtr, &ts, tx, ty);
        if ((stylePtr->flags & UNDERLINE_ACTIVE) && 
            (viewPtr->activeCellPtr == cellPtr)) {
            Blt_Ts_UnderlineChars(viewPtr->tkwin, drawable, textPtr,
                &ts, tx, ty);
        }
        Blt_Free(textPtr);
    }
    stylePtr->flags &= ~STYLE_DIRTY;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextBoxStyleFreeProc --
 *
 *      Releases resources allocated for the textbox. The resources freed by
 *      this routine are specific only to the "textbox".  Other resources
 *      (common to all styles) are freed in the Blt_TreeView_FreeStyle
 *      routine.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      GCs allocated for the textbox are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
TextBoxStyleFreeProc(CellStyle *cellStylePtr)
{
    TextBoxStyle *stylePtr = (TextBoxStyle *)cellStylePtr;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    if (stylePtr->icon != NULL) {
        FreeIcon(stylePtr->icon);
    }
    if (stylePtr->selectedGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->selectedGC);
    }
    if (stylePtr->highlightGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->disabledGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->disabledGC);
    }
    if (stylePtr->activeGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->normalGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NewCheckBoxStyle --
 *
 *      Creates a "checkbox" style.
 *
 * Results:
 *      A pointer to the new style structure.
 *
 *---------------------------------------------------------------------------
 */
static CheckBoxStyle *
NewCheckBoxStyle(TreeView *viewPtr, Blt_HashEntry *hPtr)
{
    CheckBoxStyle *stylePtr;

    stylePtr = Blt_AssertCalloc(1, sizeof(CheckBoxStyle));
    stylePtr->classPtr = &checkBoxStyleClass;
    stylePtr->viewPtr = viewPtr;
    stylePtr->gap = 4;
    stylePtr->lineWidth = 2;
    stylePtr->name = Blt_GetHashKey(&viewPtr->styleTable, hPtr);
    stylePtr->hashPtr = hPtr;
    stylePtr->flags = SHOW_VALUE | EDITABLE | UNDERLINE_ACTIVE;
    stylePtr->relief = TK_RELIEF_FLAT;
    stylePtr->activeRelief = TK_RELIEF_FLAT;
    stylePtr->borderWidth = 1;
    stylePtr->refCount = 1;
    Blt_SetHashValue(hPtr, stylePtr);
    return stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckBoxStyleConfigureProc --
 *
 *      Configures a "checkbox" style.  This routine performs generates the
 *      GCs required for a checkbox style.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      GCs are created for the style.
 *
 *---------------------------------------------------------------------------
 */
static void
CheckBoxStyleConfigureProc(CellStyle *cellStylePtr)
{
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    GC newGC;
    TreeView *viewPtr;
    XGCValues gcValues;
    unsigned long gcMask;
    Blt_Font font;
    
    viewPtr = stylePtr->viewPtr;
    font = CHOOSE(viewPtr->font, stylePtr->font);
    gcMask = GCForeground | GCFont | GCDashList | GCLineWidth | GCLineStyle;
    gcValues.dashes = 1;
    gcValues.font = Blt_Font_Id(font);
    gcValues.line_width = 0;
    gcValues.line_style = LineOnOffDash;

    if (stylePtr->size <= 0) {
        Blt_FontMetrics fm;

        Blt_Font_GetMetrics(font, &fm);
        stylePtr->size = fm.linespace * 75 / 100;
    }
    
    /* Normal text. */
    gcValues.foreground = CHOOSE(viewPtr->normalFg, stylePtr->normalFg)->pixel;
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
    if (stylePtr->disabledGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->disabledGC);
    }
    stylePtr->disabledGC = newGC;

    /* Highlight text. */
    gcValues.foreground = stylePtr->highlightFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    /* Selected text. */
    gcValues.foreground = stylePtr->selectedFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->selectedGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->selectedGC);
    }
    stylePtr->selectedGC = newGC;

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
    if ((stylePtr->flags & SHOW_VALUE) && 
        (Blt_ConfigModified(stylePtr->classPtr->specs, "-font", (char *)NULL))){
        /* Font sizes can change the size of the cell. */
        PropagateStyleChanges(cellStylePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckBoxStyleGeometryProc --
 *
 *      Determines the space requirements for the "checkbox" given the
 *      value to be displayed.  Depending upon whether an icon or text is
 *      displayed and their relative placements, this routine computes the
 *      space needed for the text entry.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The width and height fields of *valuePtr* are set with the computed
 *      dimensions.
 *
 *---------------------------------------------------------------------------
 */
static void
CheckBoxStyleGeometryProc(Cell *cellPtr, CellStyle *cellStylePtr)
{
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    unsigned int bw, bh, iw, ih, tw, th, gap;
    TreeView *viewPtr;
    Entry *rowPtr;
    Column *colPtr;


    viewPtr = stylePtr->viewPtr;
    cellPtr->flags &= ~GEOMETRY;        /* Remove the geometry flag from
                                         * the cell. */
    rowPtr = cellPtr->entryPtr;
    colPtr = cellPtr->colPtr;

    bw = bh = ODD(stylePtr->size);
    tw = th = iw = ih = 0;

    cellPtr->width = 2 * (stylePtr->borderWidth + FOCUS_PAD + CELL_PADX);
    cellPtr->height = 2 * (stylePtr->borderWidth + FOCUS_PAD + CELL_PADY);
    cellPtr->width += colPtr->ruleWidth + PADDING(colPtr->pad);
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
    FormatCell(cellStylePtr, cellPtr);
    gap = 0;
    cellPtr->textWidth = cellPtr->textHeight =  0;
    if (stylePtr->flags & SHOW_VALUE) {
        TextStyle ts;
        const char *string;

        FormatCell(cellStylePtr, cellPtr);
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
        string = Tcl_GetString(stylePtr->onValueObjPtr);
        stylePtr->onPtr = Blt_Ts_CreateLayout(string, -1, &ts);
        string = Tcl_GetString(stylePtr->offValueObjPtr);
        stylePtr->offPtr = Blt_Ts_CreateLayout(string, -1, &ts);
        tw = MAX(stylePtr->offPtr->width, stylePtr->onPtr->width);
        th = MAX(stylePtr->offPtr->height, stylePtr->onPtr->height);
        if (stylePtr->icon != NULL) {
            gap = stylePtr->gap;
        }
        cellPtr->textWidth = tw;
        cellPtr->textHeight = th;
    }
    cellPtr->width += stylePtr->gap + bw + iw + gap + tw;
    cellPtr->height += MAX3(bh, th, ih);
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckBoxStyleDrawProc --
 *
 *      Draws the "checkbox" given the screen coordinates and the
 *      cell to be displayed.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The checkbox cell is drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
CheckBoxStyleDrawProc(Cell *cellPtr, Drawable drawable, CellStyle *cellStylePtr,
                      int x, int y)
{
    Blt_Bg bg;
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    Column *colPtr;
    TextLayout *textPtr;
    TreeView *viewPtr;
    int bool;
    int relief;
    int gap, colWidth, rowHeight, cellHeight, cellWidth;
    int ix, iy, iw, ih;
    int tx, ty, th;
    int bx, by, bw, bh;
    Entry *rowPtr;
    GC gc;

    viewPtr = stylePtr->viewPtr;
    colPtr = cellPtr->colPtr;
    rowPtr = cellPtr->entryPtr;
    relief = stylePtr->relief;
    rowHeight = rowPtr->height - rowPtr->ruleHeight;
    colWidth  = colPtr->width - colPtr->ruleWidth;
    if ((colWidth <= 0) || (rowHeight <= 0)) {
        return;
    }
    if ((cellPtr->flags|rowPtr->flags|colPtr->flags) & DISABLED) {
        /* Disabled */
        bg = CHOOSE(viewPtr->disabledBg, stylePtr->disabledBg);
        gc = stylePtr->disabledGC;
    } else if ((stylePtr->flags & ACTIVE_COLORS) && 
               (viewPtr->activeCellPtr == cellPtr)) {
        /* Active */
        bg = CHOOSE(viewPtr->activeBg, stylePtr->activeBg);
        gc = stylePtr->activeGC;
        relief = stylePtr->activeRelief;
    } else if (RowSelected(rowPtr)) {
        /* Selected */
        bg = CHOOSE(viewPtr->selectedBg, stylePtr->selectedBg);
        gc = stylePtr->selectedGC;
        if (viewPtr->activeCellPtr == cellPtr) {
            relief = stylePtr->activeRelief;
        }
    } else if ((cellPtr->flags|rowPtr->flags|colPtr->flags) & HIGHLIGHT) {
        /* Highlighted */
        bg = stylePtr->highlightBg;
        gc = stylePtr->highlightGC;
    } else {
        /* Normal */
        if (rowPtr->flatIndex & 0x1) {
            bg = CHOOSE(viewPtr->altBg, stylePtr->altBg);
        } else {
            bg = CHOOSE(viewPtr->normalBg, stylePtr->normalBg);
        }            
        gc = stylePtr->normalGC;
    }

    /* Draw background. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, 
                colWidth, rowHeight, stylePtr->borderWidth, relief);

    /* Draw Rule */
    if (rowPtr->ruleHeight > 0) {
        XFillRectangle(viewPtr->display, drawable, rowPtr->ruleGC, 
                x, y + rowHeight, colWidth, rowPtr->ruleHeight);
    }
    if (colPtr->ruleWidth > 0) {
        XFillRectangle(viewPtr->display, drawable, colPtr->ruleGC, 
                x + colWidth, y, colPtr->ruleWidth, rowHeight);
    }
    rowHeight -= 2 * stylePtr->borderWidth;
    colWidth  -= 2 * stylePtr->borderWidth - PADDING(colPtr->pad);

    x += stylePtr->borderWidth + colPtr->pad.side1;
    y += stylePtr->borderWidth;

    /* Draw the focus ring if this cell has focus. */
    if ((viewPtr->flags & FOCUS) && (viewPtr->focusCellPtr == cellPtr)) {
        XDrawRectangle(viewPtr->display, drawable, gc, x+2, y+2, colWidth - 5, 
                       rowHeight - 4);
    }
    x += CELL_PADX + FOCUS_PAD;
    y += CELL_PADY + FOCUS_PAD;
    rowHeight -= 2 * (FOCUS_PAD + CELL_PADY);
    colWidth  -= 2 * (FOCUS_PAD + CELL_PADX);

    cellHeight = cellPtr->height - 
        2 * (stylePtr->borderWidth + CELL_PADY + FOCUS_PAD);
    cellWidth =  cellPtr->width  - PADDING(colPtr->pad) - 
        2 * (stylePtr->borderWidth + CELL_PADX + FOCUS_PAD);

    /* Justify (x) and center (y) the contents of the cell. */
    if (rowHeight > cellHeight) {
        y += (rowHeight - cellHeight) / 2;
        rowHeight = cellHeight;
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
    th = iw = ih = 0;                   /* Suppress compiler warning. */
    gap = 0;

    if (cellPtr->dataObjPtr == NULL) {
        bool = 0;
    } else {
        const char *s1, *s2;

        s1 = Tcl_GetString(stylePtr->onValueObjPtr);
        s2 = Tcl_GetString(cellPtr->dataObjPtr);
        bool = (strcmp(s1, s2) == 0);
    }
    textPtr = (bool) ? stylePtr->onPtr : stylePtr->offPtr;

    /*
     * Draw the box and check. 
     *
     *          +-----------+
     *          |           |
     *          |         * |
     *          |        *  |
     *          | *     *   |
     *          |  *   *    |
     *          |   * *     |
     *          |    *      |
     *          +-----------+
     */
    bw = bh = ODD(stylePtr->size);
    bx = x;
    by = y;
    if (rowHeight > bh) {
        by += (rowHeight - bh) / 2;
    }
    {
        Blt_Picture picture;

        if (bx < 0) {
            bw += bx;
            bx = 0;
        }
        if (by < 0) {
            bh += by;
            by = 0;
        }
        if ((bx + bw) > Tk_Width(viewPtr->tkwin)) {
            bw = Tk_Width(viewPtr->tkwin) - bx;
        }
        if ((by + bh) > Tk_Height(viewPtr->tkwin)) {
            bh = Tk_Height(viewPtr->tkwin) - by;
        }
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
    if (stylePtr->flags & SHOW_VALUE) {
        th = textPtr->height;
        if (stylePtr->icon != NULL) {
            gap = stylePtr->gap;
        }
    }
    x = bx + bw + stylePtr->gap;

    /* The icon sits to the left of the text. */
    ix = x;
    iy = y + (rowHeight - ih) / 2;
    tx = ix + iw + gap;
    ty = y + (rowHeight - th) / 2;
    if (stylePtr->icon != NULL) {
        Tk_RedrawImage(IconBits(stylePtr->icon), 0, 0, iw, 
                       ih, drawable, ix, iy);
    }
    if ((stylePtr->flags & SHOW_VALUE) && (textPtr != NULL)) {
        TextStyle ts;
        int xMax;

        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
        Blt_Ts_SetGC(ts, gc);
        xMax = colWidth - iw - bw - gap - stylePtr->gap;
        Blt_Ts_SetMaxLength(ts, xMax);
        Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, textPtr, &ts, tx, ty);
        if ((stylePtr->flags & UNDERLINE_ACTIVE) && 
            (viewPtr->activeCellPtr == cellPtr)) {
            Blt_Ts_UnderlineChars(viewPtr->tkwin, drawable, textPtr,
                &ts, tx, ty);
        }
    }
    stylePtr->flags &= ~STYLE_DIRTY;
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckBoxStyleIdentifyProc --
 *
 *      Determines if the pointer is over the box of "checkbox".
 *
 * Results:
 *      Returns a string "button" is over the button and "text" otherwise.
 *
 *---------------------------------------------------------------------------
 */
static const char *
CheckBoxStyleIdentifyProc(Cell *cellPtr, CellStyle *cellStylePtr, 
                          int x, int y)
{
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    int tx;
    unsigned int bw;
    Column *colPtr;

    colPtr = cellPtr->colPtr;
    bw = ODD(stylePtr->size);
    tx = stylePtr->borderWidth + FOCUS_PAD + CELL_PADX + 
        colPtr->pad.side1 + bw + stylePtr->gap;
    if ((x >= 0) && (x < tx)) {
        return "button";
    }
    return "text";
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckBoxStyleFreeProc --
 *
 *      Releases resources allocated for the checkbox. The resources freed
 *      by this routine are specific only to the "checkbox".  Other
 *      resources (common to all styles) are freed in the
 *      Blt_TreeView_FreeStyle routine.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      GCs allocated for the checkbox are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
CheckBoxStyleFreeProc(CellStyle *cellStylePtr)
{
    CheckBoxStyle *stylePtr = (CheckBoxStyle *)cellStylePtr;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    if (stylePtr->icon != NULL) {
        FreeIcon(stylePtr->icon);
    }
    if (stylePtr->offPtr != NULL) {
        Blt_Free(stylePtr->offPtr);
    }
    if (stylePtr->onPtr != NULL) {
        Blt_Free(stylePtr->onPtr);
    }
    if (stylePtr->selectedBox != NULL) {
        Blt_FreePicture(stylePtr->selectedBox);
    }
    if (stylePtr->normalBox != NULL) {
        Blt_FreePicture(stylePtr->normalBox);
    }
    if (stylePtr->disabledBox != NULL) {
        Blt_FreePicture(stylePtr->disabledBox);
    }
    if (stylePtr->selectedGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->selectedGC);
    }
    if (stylePtr->highlightGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->disabledGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->disabledGC);
    }
    if (stylePtr->activeGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->normalGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NewComboBoxStyle --
 *
 *      Creates a "combobox" style.
 *
 * Results:
 *      A pointer to the new style structure.
 *
 *---------------------------------------------------------------------------
 */
static ComboBoxStyle *
NewComboBoxStyle(TreeView *viewPtr, Blt_HashEntry *hPtr)
{
    ComboBoxStyle *stylePtr;

    stylePtr = Blt_AssertCalloc(1, sizeof(ComboBoxStyle));
    stylePtr->classPtr = &comboBoxClass;
    stylePtr->gap = STYLE_GAP;
    stylePtr->arrowRelief = TK_RELIEF_RAISED;
    stylePtr->postedRelief = TK_RELIEF_SUNKEN;
    stylePtr->relief = stylePtr->activeRelief = TK_RELIEF_FLAT;
    stylePtr->arrowBorderWidth = 1;
    stylePtr->borderWidth = 1;
    stylePtr->name = Blt_GetHashKey(&viewPtr->styleTable, hPtr);
    stylePtr->hashPtr = hPtr;
    stylePtr->link = NULL;
    stylePtr->flags = EDITABLE | UNDERLINE_ACTIVE;
    stylePtr->refCount = 1;
    stylePtr->viewPtr = viewPtr;
    Blt_SetHashValue(hPtr, stylePtr);
    return stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleConfigureProc --
 *
 *      Configures a "combobox" style.  This routine performs generates the
 *      GCs required for a combobox style.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      GCs are created for the style.
 *
 *---------------------------------------------------------------------------
 */
static void
ComboBoxStyleConfigureProc(CellStyle *cellStylePtr)
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    GC newGC;
    TreeView *viewPtr;
    XGCValues gcValues;
    unsigned long gcMask;

    viewPtr = stylePtr->viewPtr;

    gcMask = GCForeground | GCFont | GCDashList | GCLineWidth | GCLineStyle;
    gcValues.dashes = 1;
    gcValues.font = Blt_Font_Id(CHOOSE(viewPtr->font, stylePtr->font));
    gcValues.line_width = 0;
    gcValues.line_style = LineOnOffDash;

    /* Normal text. */
    gcValues.foreground = CHOOSE(viewPtr->normalFg, stylePtr->normalFg)->pixel;
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
    if (stylePtr->disabledGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->disabledGC);
    }
    stylePtr->disabledGC = newGC;

    /* Highlight text. */
    gcValues.foreground = stylePtr->highlightFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    /* Selected text. */
    gcValues.foreground = stylePtr->selectedFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->selectedGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->selectedGC);
    }
    stylePtr->selectedGC = newGC;

    if (Blt_ConfigModified(stylePtr->classPtr->specs, "-font", (char *)NULL)) {
        /* Font sizes can change the size of the cell. */
        PropagateStyleChanges(cellStylePtr);
    }
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
        Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
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
 *      Determines the space requirements for the "combobox" given the
 *      value to be displayed.  Depending upon whether an icon or text is
 *      displayed and their relative placements, this routine computes the
 *      space needed for the text entry.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The width and height fields of *valuePtr* are set with the computed
 *      dimensions.
 *
 *      |b|p|f|x|i|g|t|g|a|x|f|p|b|
 *      +-----------------+     
 *      ||Icon| |text| |v||     
 *      +--------------+--+
 *  
 *---------------------------------------------------------------------------
 */
static void
ComboBoxStyleGeometryProc(Cell *cellPtr, CellStyle *cellStylePtr)
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    TreeView *viewPtr;
    unsigned int iw, ih, tw, th, ah, aw;
    Column *colPtr;
    Entry *rowPtr;
    Blt_Font font;
    Blt_FontMetrics fm;
    int gap;

    viewPtr = stylePtr->viewPtr;
    cellPtr->flags &= ~GEOMETRY;        /* Remove the dirty flag from the
                                         * cell. */
    rowPtr = cellPtr->entryPtr;
    colPtr = cellPtr->colPtr;

    tw = th = iw = ih = 0;
    cellPtr->width = cellPtr->height = 2 * (stylePtr->borderWidth + FOCUS_PAD);
    cellPtr->width  += 2 * CELL_PADX;
    cellPtr->height += 2 * CELL_PADY;
    cellPtr->width  += colPtr->ruleWidth + PADDING(colPtr->pad);
    cellPtr->height += rowPtr->ruleHeight;

    FormatCell(cellStylePtr, cellPtr);
    if (stylePtr->icon != NULL) {
        iw = IconWidth(stylePtr->icon);
        ih = IconHeight(stylePtr->icon);
    } 
    gap = 0;
    /* We don't know if the menu changed.  Do this once for the style. */
    if (stylePtr->menuObjPtr != NULL) {
        GetComboMenuGeometry(viewPtr->interp, viewPtr, stylePtr, &tw, &th);
    } else if (cellPtr->dataObjPtr != NULL) {
        TextStyle ts;
        const char *string;

        string = Tcl_GetString(cellPtr->dataObjPtr);
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
        Blt_Ts_GetExtents(&ts, string, &tw, &th);
        if (stylePtr->icon != NULL) {
            gap = stylePtr->gap;
        }
    }
    cellPtr->textWidth = tw;
    cellPtr->textHeight = th;

    font = CHOOSE(viewPtr->font, stylePtr->font);
    Blt_Font_GetMetrics(font, &fm);
    stylePtr->arrowWidth = fm.ascent - 2;
    aw = ah = (2 * stylePtr->arrowBorderWidth) + stylePtr->arrowWidth;
     aw += 2 * 1;
    ah += 2 * 1;
    cellPtr->width  += iw + 2 * gap + aw + tw;
    cellPtr->height += MAX3(th, ih, ah);
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleDrawProc --
 *
 *      Draws the "combobox" given the screen coordinates and the value to
 *      be displayed.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The combobox value is drawn.
 *
 *      +-----------------+     
 *      ||Icon| |text| |v||     
 *      +--------------+--+
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
    GC gc;
    Entry *rowPtr;
    int ix, iy, tx, ty;
    unsigned int gap, colWidth, rowHeight, cellWidth, cellHeight;
    unsigned int iw, ih, th;
    int relief;
    XColor *fg;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    rowPtr = cellPtr->entryPtr;
    colPtr = cellPtr->colPtr;
    relief = stylePtr->relief;
    rowHeight = rowPtr->height - rowPtr->ruleHeight;
    colWidth  = colPtr->width - colPtr->ruleWidth;
    if ((colWidth <= 0) || (rowHeight <= 0)) {
        return;
    }

    if ((cellPtr->flags|rowPtr->flags|colPtr->flags) & DISABLED) {
        /* Disabled */
        bg = CHOOSE(viewPtr->disabledBg, stylePtr->disabledBg);
        gc = stylePtr->disabledGC;
    } else if ((stylePtr->flags & ACTIVE_COLORS) && 
               (viewPtr->activeCellPtr == cellPtr)) {
        /* Active */
        bg = CHOOSE(viewPtr->activeBg, stylePtr->activeBg);
        gc = stylePtr->activeGC;
        relief = stylePtr->activeRelief;
    } else if (RowSelected(rowPtr)) {
        /* Selected */
        bg = CHOOSE(viewPtr->selectedBg, stylePtr->selectedBg);
        gc = stylePtr->selectedGC;
    } else if ((cellPtr->flags|rowPtr->flags|colPtr->flags) & HIGHLIGHT) {
        /* Highlight */
        bg = stylePtr->highlightBg;
        gc = stylePtr->highlightGC;
    } else {
        /* Normal */
        if (rowPtr->flatIndex & 0x1) {
            bg = CHOOSE(viewPtr->altBg, stylePtr->altBg);
        } else {
            bg = CHOOSE(viewPtr->normalBg, stylePtr->normalBg);
        }            
        gc = stylePtr->normalGC;
    }

    /* Draw background. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, colWidth,
        rowHeight, stylePtr->borderWidth, relief);

    /* Draw Rule */
    if (rowPtr->ruleHeight > 0) {
        XFillRectangle(viewPtr->display, drawable, rowPtr->ruleGC, 
                x, y + rowHeight, colWidth, rowPtr->ruleHeight);
    }
    if (colPtr->ruleWidth > 0) {
        XFillRectangle(viewPtr->display, drawable, colPtr->ruleGC, 
                x + colWidth, y, colPtr->ruleWidth, rowHeight);
    }

    rowHeight -= 2 * stylePtr->borderWidth;
    colWidth  -= 2 * stylePtr->borderWidth - PADDING(colPtr->pad);

    x += stylePtr->borderWidth + colPtr->pad.side1;
    y += stylePtr->borderWidth;

    /* Draw the focus ring if this cell has focus. */
    if ((viewPtr->flags & FOCUS) && (viewPtr->focusCellPtr == cellPtr)) {
        XDrawRectangle(viewPtr->display, drawable, gc, x+2, y+2, colWidth - 5, 
                       rowHeight - 4);
    }
    x += CELL_PADX + FOCUS_PAD;
    y += CELL_PADY + FOCUS_PAD;
    rowHeight -= 2 * (FOCUS_PAD + CELL_PADY);
    colWidth  -= 2 * (FOCUS_PAD + CELL_PADX);

    cellHeight = cellPtr->height - 
        2 * (stylePtr->borderWidth + CELL_PADY + FOCUS_PAD);
    cellWidth  = cellPtr->width  - PADDING(colPtr->pad) - 
        2 * (stylePtr->borderWidth + CELL_PADX + FOCUS_PAD);

    /* Justify (x) and center (y) the contents of the cell. */
    if (rowHeight > cellHeight) {
        y += (rowHeight - cellHeight) / 2;
        rowHeight = cellHeight;
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
    th = iw = ih = 0;                   /* Suppress compiler warning. */
    gap = 0;
    if (stylePtr->icon != NULL) {
        iw = IconWidth(stylePtr->icon);
        ih = IconHeight(stylePtr->icon);
    }
    if (cellPtr->dataObjPtr != NULL) {
        th = cellPtr->textHeight;
        if (stylePtr->icon != NULL) {
            gap = stylePtr->gap;
        }
    }    
    ix = tx = x, iy = ty = y;
    if (rowHeight > ih) {
        iy += (rowHeight - ih) / 2;
    }
    if (rowHeight > th) {
        ty += (rowHeight - th) / 2;
    }
    tx = ix + gap + iw;

    if (stylePtr->icon != NULL) {
        Tk_RedrawImage(IconBits(stylePtr->icon), 0, 0, iw, ih, drawable, ix,iy);
    }
    if (cellPtr->dataObjPtr != NULL) {
        TextStyle ts;
        int xMax;
        TextLayout *textPtr;
        const char *string;
        int length;

        string = Tcl_GetStringFromObj(cellPtr->dataObjPtr, &length);
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
        Blt_Ts_SetGC(ts, gc);
        xMax = SCREENX(viewPtr, colPtr->worldX) + colWidth - 
            stylePtr->arrowWidth;
        Blt_Ts_SetMaxLength(ts, xMax - tx);
        textPtr = Blt_Ts_CreateLayout(string, length, &ts);
        Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, textPtr, &ts, tx, ty);
        if ((stylePtr->flags & UNDERLINE_ACTIVE) && 
            (viewPtr->activeCellPtr == cellPtr)) {
            Blt_Ts_UnderlineChars(viewPtr->tkwin, drawable, textPtr,
                &ts, tx, ty);
        }
        Blt_Free(textPtr);
    }
    if ((stylePtr->flags & EDITABLE) && (viewPtr->activeCellPtr == cellPtr)) {
        int ax, ay;
        unsigned int aw, ah;
        XColor *color;
        
        aw = stylePtr->arrowWidth + (2 * stylePtr->arrowBorderWidth);
        ah = aw;
        ax = x + colWidth - aw - stylePtr->gap;
        ay = y;
        
        if (rowHeight > ah) {
            ay += (cellHeight - ah) / 2;
        }
        color = stylePtr->activeFg;
        bg = (viewPtr->activeCellPtr == cellPtr) ? 
            stylePtr->activeArrowBg : 
            CHOOSE(viewPtr->activeBg, stylePtr->activeBg);
        relief = (viewPtr->postPtr == cellPtr) ? 
            stylePtr->postedRelief : stylePtr->arrowRelief;
        Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, ax, ay, aw, ah, 
                stylePtr->arrowBorderWidth, relief);
        aw -= 2 * stylePtr->arrowBorderWidth;
        ax += stylePtr->arrowBorderWidth;
        ih = aw * 55 / 100;
        iw = aw * 80 / 100;
        {
            Blt_Picture picture;
            int ix, iy;
            Blt_PaintBrush brush;
            
            picture = Blt_CreatePicture(aw, ah);
            Blt_BlankPicture(picture, 0x0);
            iy = (ah - ih) / 2;
            ix = (aw - iw) / 2;
            brush = Blt_NewColorBrush(Blt_XColorToPixel(color));
            /*
              Blt_PaintRectangle(picture, 0, 0, aw, ah, 4, 2, brush, 1);
            */
            Blt_PaintArrowHead(picture, ix, iy, iw, ih,
                               Blt_XColorToPixel(color), ARROW_DOWN);
            if (stylePtr->painter == NULL) {
                stylePtr->painter = Blt_GetPainter(viewPtr->tkwin, 1.0);
            }
            Blt_PaintPicture(stylePtr->painter, drawable,
                             picture, 0, 0, aw, ah, ax, ay, 0);
            Blt_FreeBrush(brush);
            Blt_FreePicture(picture);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleIdentifyProc --
 *
 *      Draws the "combobox" given the screen coordinates and the cell to
 *      be displayed.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The checkbox cell is drawn.
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
    aw = stylePtr->arrowWidth + (2 * stylePtr->arrowBorderWidth);
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
 *      Releases resources allocated for the combobox. The resources freed
 *      by this routine are specific only to the "combobox".  Other
 *      resources (common to all styles) are freed in the
 *      Blt_TreeView_FreeStyle routine.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      GCs allocated for the combobox are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
ComboBoxStyleFreeProc(CellStyle *cellStylePtr)
{
    ComboBoxStyle *stylePtr = (ComboBoxStyle *)cellStylePtr;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    if (stylePtr->icon != NULL) {
        FreeIcon(stylePtr->icon);
    }
    if (stylePtr->selectedGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->selectedGC);
    }
    if (stylePtr->highlightGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->disabledGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->disabledGC);
    }
    if (stylePtr->activeGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->normalGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NewImageBoxStyle --
 *
 *      Creates a "imagebox" style.
 *
 * Results:
 *      A pointer to the new style structure.
 *
 *---------------------------------------------------------------------------
 */
static ImageBoxStyle *
NewImageBoxStyle(TreeView *viewPtr, Blt_HashEntry *hPtr)
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
    stylePtr->flags = SHOW_TEXT | UNDERLINE_ACTIVE;
    stylePtr->refCount = 1;
    Blt_SetHashValue(hPtr, stylePtr);
    return stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageBoxStyleConfigureProc --
 *
 *      Configures a "imagebox" style.  This routine performs generates the
 *      GCs required for a combobox style.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      GCs are created for the style.
 *
 *---------------------------------------------------------------------------
 */
static void
ImageBoxStyleConfigureProc(CellStyle *cellStylePtr)
{
    ImageBoxStyle *stylePtr = (ImageBoxStyle *)cellStylePtr;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    gcMask = GCForeground | GCFont | GCDashList | GCLineWidth | GCLineStyle;
    gcValues.dashes = 1;
    gcValues.font = Blt_Font_Id(CHOOSE(viewPtr->font, stylePtr->font));
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
    if (stylePtr->disabledGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->disabledGC);
    }
    stylePtr->disabledGC = newGC;

    /* Selected text. */
    gcValues.foreground = stylePtr->selectedFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->selectedGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->selectedGC);
    }
    stylePtr->selectedGC = newGC;

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

    if ((stylePtr->flags & SHOW_TEXT) && 
        (Blt_ConfigModified(stylePtr->classPtr->specs, "-font", (char *)NULL))){
        /* Font sizes can change the size of the cell. */
        PropagateStyleChanges(cellStylePtr);
    }
}

static int
ParseImageFormat(Tcl_Interp *interp, TreeView *viewPtr, Cell *cellPtr, 
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
    if (cellPtr->dataObjPtr != NULL) {
        Tcl_DecrRefCount(cellPtr->dataObjPtr);
        cellPtr->dataObjPtr = NULL;
    }
    if (objc == 2) {
        Tcl_IncrRefCount(objv[1]);
        cellPtr->dataObjPtr = objv[1];
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ImageBoxStyleGeometryProc --
 *
 *      Determines the space requirements for the imagebox given the image,
 *      text, and icon to be displayed.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The width and height fields of *cellPtr* are set with the computed
 *      dimensions.
 *
 *      +--------------+            +--------------+
 *      ||Icon| |Image||            ||Icon| |text| |
 *      |       |text| |            |       |Image||
 *      +--------------+            +--------------+
 *  
 *---------------------------------------------------------------------------
 */
static void
ImageBoxStyleGeometryProc(Cell *cellPtr, CellStyle *cellStylePtr)
{
    ImageBoxStyle *stylePtr = (ImageBoxStyle *)cellStylePtr;
    unsigned int iw, ih, pw, ph, tw, th;
    TreeView *viewPtr;
    Tcl_Interp *interp;
    Tcl_Obj *objPtr;
    Entry *rowPtr;
    Column *colPtr;

    viewPtr = stylePtr->viewPtr;
    cellPtr->flags &= ~GEOMETRY;        /* Remove the geometry flag from
                                         * the cell. */
    rowPtr = cellPtr->entryPtr;
    colPtr = cellPtr->colPtr;

    pw = ph = iw = ih = tw = th = 0;
    cellPtr->width = cellPtr->height = 2 * (stylePtr->borderWidth + FOCUS_PAD);
    cellPtr->width += 2 * CELL_PADX;
    cellPtr->height += 2 * CELL_PADY;
    cellPtr->width += colPtr->ruleWidth + PADDING(colPtr->pad);
    cellPtr->height += rowPtr->ruleHeight;

    interp = viewPtr->interp;
    objPtr = FormatCell(cellStylePtr, viewPtr->postPtr);
    if (objPtr != NULL) {
        int result;

        Tcl_IncrRefCount(objPtr);
        result = ParseImageFormat(interp, viewPtr, cellPtr, objPtr);
        Tcl_DecrRefCount(objPtr);
        if (result != TCL_OK) {
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
    if ((stylePtr->flags & SHOW_TEXT) && (cellPtr->dataObjPtr != NULL)) {
        TextStyle ts;
        const char *string;

        string = Tcl_GetString(cellPtr->dataObjPtr);
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
        Blt_Ts_GetExtents(&ts, string, &tw, &th);
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
 *      Draws the "combobox" given the screen coordinates and the
 *      value to be displayed.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The imagebox value is drawn.
 *
 *      +--------------+            +--------------+
 *      ||Icon| |Image||            ||Icon| |text| |
 *      |       |text| |            |       |Image||
 *      +--------------+            +--------------+
 *  
 *---------------------------------------------------------------------------
 */
static void
ImageBoxStyleDrawProc(Cell *cellPtr, Drawable drawable, CellStyle *cellStylePtr,
                      int x, int y)
{
    Blt_Bg bg;
    Entry *rowPtr;
    Column *colPtr;
    GC gc;
    ImageBoxStyle *stylePtr = (ImageBoxStyle *)cellStylePtr;
    int gap, colWidth, rowHeight, cellWidth, cellHeight;
    int ix, iy, px, py, tx, ty;
    unsigned int pw, ph, iw, ih, tw, th;
    int relief;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    rowPtr = cellPtr->entryPtr;
    colPtr = cellPtr->colPtr;
    relief = stylePtr->relief;
    rowHeight = rowPtr->height - rowPtr->ruleHeight;
    colWidth  = colPtr->width - colPtr->ruleWidth;
    if ((colWidth <= 0) || (rowHeight <= 0)) {
        return;
    }
    if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & DISABLED) {
        /* Disabled */
        bg = CHOOSE(viewPtr->disabledBg, stylePtr->disabledBg);
        gc = stylePtr->disabledGC;
    } else if ((stylePtr->flags & ACTIVE_COLORS) && 
               (viewPtr->activeCellPtr == cellPtr)) {
        /* Active */
        bg = CHOOSE(viewPtr->activeBg, stylePtr->activeBg);
        gc = stylePtr->activeGC;
        relief = stylePtr->activeRelief;
    } else if (RowSelected(rowPtr)) { 
        /* Selected */
        bg = CHOOSE(viewPtr->selectedBg, stylePtr->selectedBg);
        gc = stylePtr->selectedGC;
    } else if ((rowPtr->flags|colPtr->flags|cellPtr->flags) & HIGHLIGHT) { 
        /* Highlighted */
        bg = stylePtr->highlightBg;
        gc = stylePtr->highlightGC;
    } else {            
        /* Normal */
        if (rowPtr->flatIndex & 0x1) {
            bg = CHOOSE(viewPtr->altBg, stylePtr->altBg);
        } else {
            bg = CHOOSE(viewPtr->normalBg, stylePtr->normalBg);
        }            
        gc = stylePtr->normalGC;
    }

    /* Draw background. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, colWidth,
        rowHeight, stylePtr->borderWidth, relief);

    /* Draw Rule */
    if (rowPtr->ruleHeight > 0) {
        XFillRectangle(viewPtr->display, drawable, rowPtr->ruleGC, 
                x, y + rowHeight - rowPtr->ruleHeight, 
                colWidth, rowPtr->ruleHeight);
        rowHeight -= rowPtr->ruleHeight;
    }
    if (colPtr->ruleWidth > 0) {
        XFillRectangle(viewPtr->display, drawable, colPtr->ruleGC, 
                       x + colWidth - colPtr->ruleWidth, y, 
                       colPtr->ruleWidth, rowHeight);
        colWidth -= colPtr->ruleWidth;
    }

    colWidth  -= 2 * stylePtr->borderWidth - PADDING(colPtr->pad);
    rowHeight -= 2 * stylePtr->borderWidth + 3;
    colWidth  -= 2 * stylePtr->borderWidth + 3;
    x += stylePtr->borderWidth + 1;
    y += stylePtr->borderWidth + 1;
    /* Draw the focus ring if this cell has focus. */
    if ((viewPtr->flags & FOCUS) && (viewPtr->focusCellPtr == cellPtr)) {
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
        rowHeight = cellHeight;
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
    if ((stylePtr->icon != NULL) && (cellPtr->dataObjPtr != NULL)) {
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
    if ((stylePtr->flags & SHOW_TEXT) && (cellPtr->dataObjPtr != NULL)) {
        TextStyle ts;
        int xMax;
        TextLayout *textPtr;
        const char *string;
        int length;

        string = Tcl_GetStringFromObj(cellPtr->dataObjPtr, &length);
        ty = y + ph + gap;
        tx = x;
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
        Blt_Ts_SetGC(ts, gc);
        xMax = colWidth - iw - gap;
        Blt_Ts_SetMaxLength(ts, xMax);
        textPtr = Blt_Ts_CreateLayout(string, length, &ts);
        Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, textPtr, &ts, tx, ty);
        if ((stylePtr->flags & UNDERLINE_ACTIVE) && 
            (viewPtr->activeCellPtr == cellPtr)) {
            Blt_Ts_UnderlineChars(viewPtr->tkwin, drawable, textPtr,
                &ts, tx, ty);
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
 *      None.
 *
 * Side Effects:
 *      The checkbox value is drawn.
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
 *      Releases resources allocated for the imagebox. 
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      GCs allocated for the combobox are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
ImageBoxStyleFreeProc(CellStyle *cellStylePtr)
{
    ImageBoxStyle *stylePtr = (ImageBoxStyle *)cellStylePtr;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    iconOption.clientData = viewPtr;
    Blt_FreeOptions(stylePtr->classPtr->specs, (char *)stylePtr, 
        viewPtr->display, 0);
    if (stylePtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&viewPtr->styleTable, stylePtr->hashPtr);
    } 
    if (stylePtr->selectedGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->selectedGC);
    }
    if (stylePtr->highlightGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->disabledGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->disabledGC);
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
 * NewRadioButtonStyle --
 *
 *      Creates a "radiobutton" style.
 *
 * Results:
 *      A pointer to the new style structure.
 *
 *---------------------------------------------------------------------------
 */
static RadioButtonStyle *
NewRadioButtonStyle(TreeView *viewPtr, Blt_HashEntry *hPtr)
{
    RadioButtonStyle *stylePtr;

    stylePtr = Blt_AssertCalloc(1, sizeof(RadioButtonStyle));
    stylePtr->classPtr = &radioButtonStyleClass;
    stylePtr->viewPtr = viewPtr;
    stylePtr->gap = 4;
    stylePtr->size = 15;
    stylePtr->lineWidth = 2;
    stylePtr->name = Blt_GetHashKey(&viewPtr->styleTable, hPtr);
    stylePtr->hashPtr = hPtr;
    stylePtr->flags = SHOW_VALUE | EDITABLE | UNDERLINE_ACTIVE;
    stylePtr->relief = TK_RELIEF_FLAT;
    stylePtr->activeRelief = TK_RELIEF_FLAT;
    stylePtr->borderWidth = 1;
    stylePtr->refCount = 1;
    Blt_SetHashValue(hPtr, stylePtr);
    return stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * RadioButtonStyleConfigureProc --
 *
 *      Configures a "radiobutton" style.  This routine performs generates the
 *      GCs required for a checkbox style.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      GCs are created for the style.
 *
 *---------------------------------------------------------------------------
 */
static void
RadioButtonStyleConfigureProc(CellStyle *cellStylePtr)
{
    RadioButtonStyle *stylePtr = (RadioButtonStyle *)cellStylePtr;
    GC newGC;
    TreeView *viewPtr;
    XGCValues gcValues;
    unsigned long gcMask;

    viewPtr = stylePtr->viewPtr;

    gcMask = GCForeground | GCFont | GCDashList | GCLineWidth | GCLineStyle;
    gcValues.dashes = 1;
    gcValues.font = Blt_Font_Id(CHOOSE(viewPtr->font, stylePtr->font));
    gcValues.line_width = 0;
    gcValues.line_style = LineOnOffDash;

    /* Normal text. */
    gcValues.foreground = CHOOSE(viewPtr->normalFg, stylePtr->normalFg)->pixel;
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
    if (stylePtr->disabledGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->disabledGC);
    }
    stylePtr->disabledGC = newGC;

    /* Highlight text. */
    gcValues.foreground = stylePtr->highlightFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    /* Selected text. */
    gcValues.foreground = stylePtr->selectedFg->pixel;
    newGC = Tk_GetGC(viewPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->selectedGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->selectedGC);
    }
    stylePtr->selectedGC = newGC;

    if ((stylePtr->flags & SHOW_VALUE) && 
        (Blt_ConfigModified(stylePtr->classPtr->specs, "-font", (char *)NULL))){
        /* Font sizes can change the size of the cell. */
        PropagateStyleChanges(cellStylePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * RadioButtonStyleGeometryProc --
 *
 *      Determines the space requirements for the "radiobutton" given the
 *      value to be displayed.  Depending upon whether an icon or text is
 *      displayed and their relative placements, this routine computes the
 *      space needed for the text entry.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The width and height fields of *valuePtr* are set with the computed
 *      dimensions.
 *
 *---------------------------------------------------------------------------
 */
static void
RadioButtonStyleGeometryProc(Cell *cellPtr, CellStyle *cellStylePtr)
{
    RadioButtonStyle *stylePtr = (RadioButtonStyle *)cellStylePtr;
    unsigned int bw, bh, iw, ih, tw, th, gap;
    TreeView *viewPtr;
    Entry *rowPtr;
    Column *colPtr;


    viewPtr = stylePtr->viewPtr;
    cellPtr->flags &= ~GEOMETRY;        /* Remove the geometry flag from
                                         * the cell. */
    rowPtr = cellPtr->entryPtr;
    colPtr = cellPtr->colPtr;

    bw = bh = ODD(stylePtr->size);
    tw = th = iw = ih = 0;

    cellPtr->width  = 2 * (stylePtr->borderWidth + FOCUS_PAD + CELL_PADX);
    cellPtr->height = 2 * (stylePtr->borderWidth + FOCUS_PAD + CELL_PADY);
    cellPtr->width  += colPtr->ruleWidth + PADDING(colPtr->pad);
    cellPtr->height += rowPtr->ruleHeight;

    if (stylePtr->icon != NULL) {
        iw = IconWidth(stylePtr->icon);
        ih = IconHeight(stylePtr->icon);
    } 
    FormatCell(cellStylePtr, cellPtr);
    gap = 0;
    cellPtr->textWidth = cellPtr->textHeight =  0;
    if (stylePtr->flags & SHOW_VALUE) {
        TextStyle ts;
        const char *string;
        unsigned int tw, th;

        FormatCell(cellStylePtr, cellPtr);
        string = Tcl_GetString(cellPtr->dataObjPtr);
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
        Blt_Ts_GetExtents(&ts, string, &tw, &th);
        if (stylePtr->icon != NULL) {
            gap = stylePtr->gap;
        }
        cellPtr->textWidth = tw;
        cellPtr->textHeight = th;
    }
    cellPtr->width += stylePtr->gap + bw + iw + gap + tw;
    cellPtr->height += MAX3(bh, th, ih);
}

/*
 *---------------------------------------------------------------------------
 *
 * RadioButtonStyleDrawProc --
 *
 *      Draws the "radiobutton" given the screen coordinates and the cell to
 *      be displayed.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The checkbox cell is drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
RadioButtonStyleDrawProc(Cell *cellPtr, Drawable drawable, CellStyle *cellStylePtr,
                      int x, int y)
{
    Blt_Bg bg;
    RadioButtonStyle *stylePtr = (RadioButtonStyle *)cellStylePtr;
    Column *colPtr;
    TreeView *viewPtr;
    int bool;
    int relief;
    int gap, colWidth, rowHeight, cellHeight, cellWidth;
    int ix, iy, iw, ih;
    int tx, ty, th;
    int bx, by, bw, bh;
    Entry *rowPtr;
    GC gc;

    viewPtr = stylePtr->viewPtr;
    colPtr = cellPtr->colPtr;
    rowPtr = cellPtr->entryPtr;
    relief = stylePtr->relief;
    rowHeight = rowPtr->height - rowPtr->ruleHeight;
    colWidth  = colPtr->width - colPtr->ruleWidth;
    if ((colWidth <= 0) || (rowHeight <= 0)) {
        return;
    }
    if ((cellPtr->flags|rowPtr->flags|colPtr->flags) & DISABLED) {
        /* Disabled */
        bg = CHOOSE(viewPtr->disabledBg, stylePtr->disabledBg);
        gc = stylePtr->disabledGC;
    } else if ((stylePtr->flags & ACTIVE_COLORS) && 
               (viewPtr->activeCellPtr == cellPtr)) {
        /* Active */
        bg = CHOOSE(viewPtr->activeBg, stylePtr->activeBg);
        gc = stylePtr->activeGC;
        relief = stylePtr->activeRelief;
    } else if (RowSelected(rowPtr)) {
        /* Selected */
        bg = CHOOSE(viewPtr->selectedBg, stylePtr->selectedBg);
        gc = stylePtr->selectedGC;
        if (viewPtr->activeCellPtr == cellPtr) {
            relief = stylePtr->activeRelief;
        }
    } else if ((cellPtr->flags|rowPtr->flags|colPtr->flags) & HIGHLIGHT) {
        /* Highlighted */
        bg = stylePtr->highlightBg;
        gc = stylePtr->highlightGC;
    } else {
        /* Normal */
        if (rowPtr->flatIndex & 0x1) {
            bg = CHOOSE(viewPtr->altBg, stylePtr->altBg);
        } else {
            bg = CHOOSE(viewPtr->normalBg, stylePtr->normalBg);
        }            
        gc = stylePtr->normalGC;
    }

    /* Draw background. */
    Blt_Bg_FillRectangle(viewPtr->tkwin, drawable, bg, x, y, 
                colWidth, rowHeight, stylePtr->borderWidth, relief);

    /* Draw Rule */
    if (rowPtr->ruleHeight > 0) {
        XFillRectangle(viewPtr->display, drawable, rowPtr->ruleGC, 
                x, y + rowHeight, colWidth, rowPtr->ruleHeight);
    }
    if (colPtr->ruleWidth > 0) {
        XFillRectangle(viewPtr->display, drawable, colPtr->ruleGC, 
                x + colWidth, y, colPtr->ruleWidth, rowHeight);
    }
    rowHeight -= 2 * stylePtr->borderWidth;
    colWidth  -= 2 * stylePtr->borderWidth - PADDING(colPtr->pad);

    x += stylePtr->borderWidth + colPtr->pad.side1;
    y += stylePtr->borderWidth;

    /* Draw the focus ring if this cell has focus. */
    if ((viewPtr->flags & FOCUS) && (viewPtr->focusCellPtr == cellPtr)) {
        XDrawRectangle(viewPtr->display, drawable, gc, x+2, y+2, colWidth - 5, 
                       rowHeight - 4);
    }
    x += CELL_PADX + FOCUS_PAD;
    y += CELL_PADY + FOCUS_PAD;
    rowHeight -= 2 * (FOCUS_PAD + CELL_PADY);
    colWidth  -= 2 * (FOCUS_PAD + CELL_PADX);

    cellHeight = cellPtr->height - 
        2 * (stylePtr->borderWidth + CELL_PADY + FOCUS_PAD);
    cellWidth =  cellPtr->width  - PADDING(colPtr->pad) - 
        2 * (stylePtr->borderWidth + CELL_PADX + FOCUS_PAD);

    /* Justify (x) and center (y) the contents of the cell. */
    if (rowHeight > cellHeight) {
        y += (rowHeight - cellHeight) / 2;
        rowHeight = cellHeight;
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
    th = iw = ih = 0;                   /* Suppress compiler warning. */
    gap = 0;

    bool = 0;
    if (cellPtr->dataObjPtr == NULL) {
        int value;

        if (Tcl_GetIntFromObj(NULL, cellPtr->dataObjPtr, &value) == TCL_OK) {
            bool = value;
        }
    } else {
        const char *s1, *s2;

        s1 = Tcl_GetString(stylePtr->onValueObjPtr);
        s2 = Tcl_GetString(cellPtr->dataObjPtr);
        bool = (strcmp(s1, s2) == 0);
    }

    /*
     * Draw the box and check. 
     *
     *          +-----------+
     *          |           |
     *          |         * |
     *          |        *  |
     *          | *     *   |
     *          |  *   *    |
     *          |   * *     |
     *          |    *      |
     *          +-----------+
     */
    bw = bh = ODD(stylePtr->size);
    bx = x;
    by = y;
    if (rowHeight > bh) {
        by += (rowHeight - bh) / 2;
    }
    {
        Blt_Picture picture;
        unsigned int bw, bh;

        bw = bh = ODD(stylePtr->size) + 2;
        picture = Blt_PaintRadioButton(bw, bh, bg,
                stylePtr->fillColor, stylePtr->checkColor, bool);
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
    if (stylePtr->flags & SHOW_VALUE) {
        th = cellPtr->textHeight;
        if (stylePtr->icon != NULL) {
            gap = stylePtr->gap;
        }
    }
    x = bx + bw + stylePtr->gap;

    /* The icon sits to the left of the text. */
    ix = x;
    iy = y + (rowHeight - ih) / 2;
    tx = ix + iw + gap;
    ty = y + (rowHeight - th) / 2;
    if (stylePtr->icon != NULL) {
        Tk_RedrawImage(IconBits(stylePtr->icon), 0, 0, iw, 
                       ih, drawable, ix, iy);
    }
    if (stylePtr->flags & SHOW_VALUE) {
        TextStyle ts;
        int xMax;
        const char *string;
        int length;
        TextLayout *textPtr;

        string = Tcl_GetStringFromObj(cellPtr->dataObjPtr, &length);
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, CHOOSE(viewPtr->font, stylePtr->font));
        Blt_Ts_SetGC(ts, gc);
        xMax = colWidth - iw - bw - gap - stylePtr->gap;
        Blt_Ts_SetMaxLength(ts, xMax);
        textPtr = Blt_Ts_CreateLayout(string, length, &ts);
        Blt_Ts_DrawLayout(viewPtr->tkwin, drawable, textPtr, &ts, tx, ty);
        if ((stylePtr->flags & UNDERLINE_ACTIVE) && 
            (viewPtr->activeCellPtr == cellPtr)) {
            Blt_Ts_UnderlineChars(viewPtr->tkwin, drawable, textPtr,
                &ts, tx, ty);
        }
        Blt_Free(textPtr);
    }
    stylePtr->flags &= ~STYLE_DIRTY;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboBoxStyleIdentifyProc --
 *
 *      Draws the "combobox" given the screen coordinates and the cell to
 *      be displayed.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The checkbox cell is drawn.
 *
 *---------------------------------------------------------------------------
 */
static const char *
RadioButtonStyleIdentifyProc(Cell *cellPtr, CellStyle *cellStylePtr, 
                          int x, int y)
{
    RadioButtonStyle *stylePtr = (RadioButtonStyle *)cellStylePtr;
    int tx;
    unsigned int bw;
    Column *colPtr;

    colPtr = cellPtr->colPtr;
    bw = ODD(stylePtr->size);
    tx = stylePtr->borderWidth + FOCUS_PAD + CELL_PADX + 
        colPtr->pad.side1 + bw + stylePtr->gap;
    if ((x >= 0) && (x < tx)) {
        return "button";
    }
    return "text";
}

/*
 *---------------------------------------------------------------------------
 *
 * RadioButtonStyleFreeProc --
 *
 *      Releases resources allocated for the checkbox. The resources freed
 *      by this routine are specific only to the "radiobutton".  Other
 *      resources (common to all styles) are freed in the
 *      Blt_TreeView_FreeStyle routine.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      GCs allocated for the checkbox are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
RadioButtonStyleFreeProc(CellStyle *cellStylePtr)
{
    RadioButtonStyle *stylePtr = (RadioButtonStyle *)cellStylePtr;
    TreeView *viewPtr;

    viewPtr = stylePtr->viewPtr;
    if (stylePtr->icon != NULL) {
        FreeIcon(stylePtr->icon);
    }
    if (stylePtr->selectedGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->selectedGC);
    }
    if (stylePtr->highlightGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->disabledGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->disabledGC);
    }
    if (stylePtr->activeGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->normalGC != NULL) {
        Tk_FreeGC(viewPtr->display, stylePtr->normalGC);
    }
}

CellStyle *
Blt_TreeView_CreateStyle(
    Tcl_Interp *interp,
    TreeView *viewPtr,                  /* Blt_TreeView_ widget. */
    int type,                           /* Type of style: either
                                         * STYLE_TEXTBOX, STYLE_COMBOBOX,
                                         * STYLE_RADIOBUTTON, or
                                         * STYLE_CHECKBOX */
    const char *styleName,              /* Name of the new style. */
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
        stylePtr = (CellStyle *)NewTextBoxStyle(viewPtr, hPtr);      break;
    case STYLE_COMBOBOX:
        stylePtr = (CellStyle *)NewComboBoxStyle(viewPtr, hPtr);     break;
    case STYLE_CHECKBOX:
        stylePtr = (CellStyle *)NewCheckBoxStyle(viewPtr, hPtr);     break;
    case STYLE_IMAGEBOX:
        stylePtr = (CellStyle *)NewImageBoxStyle(viewPtr, hPtr);     break;
    case STYLE_RADIOBUTTON:
        stylePtr = (CellStyle *)NewRadioButtonStyle(viewPtr, hPtr);     break;
    default:
        return NULL;
    }
    iconOption.clientData = viewPtr;
    if (Blt_ConfigureComponentFromObj(interp, viewPtr->tkwin, styleName, 
        stylePtr->classPtr->className, stylePtr->classPtr->specs, 
        objc, objv, (char *)stylePtr, 0) != TCL_OK) {
        (*stylePtr->classPtr->freeProc)(stylePtr);
        return NULL;
    }
    return stylePtr;
}

#endif /*TREEVIEW*/

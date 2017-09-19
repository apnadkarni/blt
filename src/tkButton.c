/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * tkButton.c --
 *
 *      This module implements a collection of button-like
 *      widgets for the Tk toolkit.  The widgets implemented
 *      include labels, buttons, check buttons, and radio
 *      buttons.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 *
 *   This software is copyrighted by the Regents of the University of
 *   California, Sun Microsystems, Inc., and other parties.  The following
 *   terms apply to all files associated with the software unless
 *   explicitly disclaimed in individual files.
 * 
 *   The authors hereby grant permission to use, copy, modify, distribute,
 *   and license this software and its documentation for any purpose,
 *   provided that existing copyright notices are retained in all copies
 *   and that this notice is included verbatim in any distributions. No
 *   written agreement, license, or royalty fee is required for any of the
 *   authorized uses.  Modifications to this software may be copyrighted by
 *   their authors and need not follow the licensing terms described here,
 *   provided that the new terms are clearly indicated on the first page of
 *   each file where they apply.
 * 
 *   IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 *   FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 *   ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 *   DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 * 
 *   THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 *   NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND
 *   THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 *   MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *   GOVERNMENT USE: If you are acquiring this software on behalf of the
 *   U.S. government, the Government shall have only "Restricted Rights" in
 *   the software and related documentation as defined in the Federal
 *   Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 *   are acquiring the software on behalf of the Department of Defense, the
 *   software shall be classified as "Commercial Computer Software" and the
 *   Government shall have only "Restricted Rights" as defined in Clause
 *   252.227-7013 (b) (3) of DFARs.  Notwithstanding the foregoing, the
 *   authors grant the U.S. Government and others acting in its behalf
 *   permission to use and distribute the software in accordance with the
 *   terms specified in this license.
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
#ifndef NO_TKBUTTON

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltFont.h"
#include "bltText.h"
#include "bltImage.h"
#include "bltPicture.h"
#include "bltPainter.h"
#include "bltBg.h"
#include "bltInitCmd.h"

#define GAP 4

/*
 * The definitions below provide symbolic names for the default colors.
 * NORMAL_BG -          Normal background color.
 * ACTIVE_BG -          Background color when widget is active.
 * SELECT_BG -          Background color for selected text.
 * TROUGH -             Background color for troughs in scales and scrollbars.
 * INDICATOR -          Color for indicator when button is selected.
 * DISABLED -           Foreground color when widget is disabled.
 */

#define NORMAL_BG       "#d9d9d9"
#define ACTIVE_BG       "#ececec"
#define SELECT_BG       "#c3c3c3"
#define TROUGH          "#c3c3c3"
#define INDICATOR       "#b03060"
#define DISABLED        "#a3a3a3"

#define DEF_BUTTON_ANCHOR               "center"
#define DEF_BUTTON_ACTIVE_BACKGROUND    STD_ACTIVE_BACKGROUND
#define DEF_BUTTON_ACTIVE_BG_MONO       RGB_BLACK
#define DEF_BUTTON_ACTIVE_FOREGROUND    RGB_BLACK
#define DEF_BUTTON_ACTIVE_FG_MONO       RGB_WHITE
#define DEF_BUTTON_BACKGROUND           STD_NORMAL_BACKGROUND
#define DEF_BUTTON_BG_MONO              RGB_WHITE
#define DEF_BUTTON_BITMAP               ""
#define DEF_BUTTON_BORDERWIDTH          "2"
#define DEF_PUSHBUTTON_BORDERWIDTH      "1"
#define DEF_BUTTON_CURSOR               ""
#define DEF_BUTTON_COMMAND              ""
#define DEF_BUTTON_COMPOUND             "none"
#define DEF_BUTTON_DEFAULT              "disabled"
#define DEF_BUTTON_DISABLED_FOREGROUND  STD_DISABLED_FOREGROUND
#define DEF_BUTTON_DISABLED_FG_MONO     ""
#define DEF_BUTTON_FG                   RGB_BLACK
#define DEF_BUTTON_FONT                 STD_FONT
#define DEF_BUTTON_HEIGHT               "0"
#define DEF_BUTTON_HIGHLIGHT_BG         STD_NORMAL_BACKGROUND
#define DEF_BUTTON_HIGHLIGHT            RGB_BLACK
#define DEF_LABEL_HIGHLIGHT_WIDTH       "0"
#define DEF_PUSHBUTTON_HIGHLIGHT_WIDTH  "0"
#define DEF_BUTTON_HIGHLIGHT_WIDTH      "2"
#define DEF_BUTTON_IMAGE                (char *) NULL
#define DEF_BUTTON_INDICATOR            "1"
#define DEF_BUTTON_JUSTIFY              "center"
#define DEF_BUTTON_OFF_VALUE            "0"
#define DEF_BUTTON_ON_VALUE             "1"
#define DEF_BUTTON_ONIMAGE              (char *)NULL
#define DEF_BUTTON_OFFIMAGE             (char *)NULL
#define DEF_BUTTON_OVER_RELIEF          "raised"
#define DEF_BUTTON_PADX                 "3m"
#define DEF_LABCHKRAD_PADX              "1"
#define DEF_BUTTON_PADY                 "1m"
#define DEF_LABCHKRAD_PADY              "1"

#define DEF_PUSHBUTTON_PADX             "2"
#define DEF_PUSHBUTTON_PADY             "2"

#define DEF_BUTTON_RELIEF               "raised"
#define DEF_BUTTON_REPEAT_DELAY         "0"
#define DEF_LABCHKRAD_RELIEF            "flat"
#define DEF_LABCHKRAD_OVER_RELIEF       "flat"
#define DEF_BUTTON_SELECT_BACKGROUND    RGB_WHITE
#define DEF_BUTTON_SELECT_FOREGROUND    STD_INDICATOR_COLOR
#define DEF_BUTTON_SELECT_MONO          RGB_BLACK
#define DEF_BUTTON_SELECT_IMAGE         (char *)NULL
#define DEF_BUTTON_STATE                "normal"
#define DEF_LABEL_TAKE_FOCUS            "0"
#define DEF_BUTTON_TAKE_FOCUS           (char *) NULL
#define DEF_BUTTON_TEXT                 ""
#define DEF_BUTTON_TEXT_VARIABLE        ""
#define DEF_BUTTON_UNDERLINE            "-1"
#define DEF_BUTTON_VALUE                ""
#define DEF_BUTTON_WIDTH                "0"
#define DEF_BUTTON_WRAP_LENGTH          "0"
#define DEF_RADIOBUTTON_VARIABLE        "selectedButton"
#define DEF_CHECKBUTTON_VARIABLE        ""

/*
 * A data structure of the following type is kept for each
 * widget managed by this file:
 */

typedef struct {
    Tk_Window tkwin;            /* Window that embodies the button.  NULL
                                 * means that the window has been destroyed. */
    Display *display;           /* Display containing widget.  Needed to
                                 * free up resources after tkwin is gone. */
    Tcl_Interp *interp;         /* Interpreter associated with button. */
    Tcl_Command widgetCmd;      /* Token for button's widget command. */
    int type;                   /* Type of widget:  restricts operations
                                 * that may be performed on widget.  See
                                 * below for possible values. */

    /*
     * Information about what's in the button.
     */

    const char *text;           /* Text to display in button (malloc'ed)
                                 * or NULL. */
    int numChars;               /* # of characters in text. */
    int underline;              /* Index of character to underline.  < 0
                                 * means don't underline anything. */
    const char *textVarName;    /* Name of variable (malloc'ed) or NULL.
                                 * If non-NULL, button displays the contents
                                 * of this variable. */
    Pixmap bitmap;              /* Bitmap to display or None.  If not None
                                 * then text and textVar are ignored. */
    Tk_Image image;             /* Image to display in window, or NULL if
                                 * none. */
    Tk_Image selectImage;       /* Image to display in window when selected,
                                 * or NULL if none.  Ignored if image is
                                 * NULL. */

    /*
     * Information used when displaying widget:
     */

    int state;                          /* State of button for display
                                         * purposes: normal, active, or
                                         * disabled. */
    Blt_Bg normalBg;                    /* Structure used to draw 3-D border
                                         * and background when window isn't
                                         * active.  NULL means no such border
                                         * exists. */
    Blt_Bg activeBg;                    /* Structure used to draw 3-D border
                                         * and background when window is
                                         * active.  NULL means no such border
                                         * exists. */
    int borderWidth;                    /* Width of border. */
    int relief;                         /* 3-d effect: TK_RELIEF_RAISED,
                                         * etc. */
    int overRelief;                     /* Value of -overrelief option:
                                         * specifies a 3-d effect for the
                                         * border, such as TK_RELIEF_RAISED,
                                         * to be used when the mouse is over
                                         * the button. */
    int highlightWidth;                 /* Width in pixels of highlight to
                                         * draw around widget when it has the
                                         * focus.  <= 0 means don't draw a
                                         * highlight. */
    Blt_Bg highlightBg;                 /* Color for drawing traversal
                                         * highlight area when highlight is
                                         * off. */
    XColor *highlightColorPtr;          /* Color for drawing traversal
                                         * highlight. */
    int inset;                          /* Total width of all borders,
                                         * including traversal highlight and
                                         * 3-D * border.  Indicates how much
                                         * interior stuff * must be offset
                                         * from outside edges to leave * room
                                         * for borders. */
    Blt_Font font;                      /* Information about text font, or
                                         * NULL. */
    XColor *normalFg;                   /* Foreground color in normal
                                         * mode. */
    XColor *activeFg;                   /* Foreground color in active mode.
                                         * NULL means use normalFg instead. */
    XColor *disabledFg;                 /* Foreground color when disabled.
                                         * NULL means use normalFg with a
                                         * 50% stipple instead. */
    GC normalTextGC;                    /* GC for drawing text in normal
                                         * mode.  Also used to copy from
                                         * off-screen pixmap onto
                                         * screen. */
    GC activeTextGC;                    /* GC for drawing text in active
                                         * mode (NULL means use
                                         * normalTextGC). */
    Pixmap gray;                        /* Pixmap for displaying disabled
                                         * text if disabledFg is NULL. */
    GC disabledGC;                      /* Used to produce disabled effect.
                                         * If disabledFg isn't NULL, this
                                         * GC is used to draw button text
                                         * or icon.  Otherwise text or icon
                                         * is drawn with normalGC and this
                                         * GC is used to stipple background
                                         * across it.  For labels this is
                                         * None. */
    GC copyGC;                          /* Used for copying information
                                         * from an off-screen pixmap to the
                                         * screen. */
    const char *widthString;            /* Value of -width option.
                                         * Malloc'ed. */
    const char *heightString;           /* Value of -height option.
                                         * Malloc'ed. */
    int width, height;                  /* If > 0, these specify dimensions
                                         * to request for window, in
                                         * characters for text and in
                                         * pixels for bitmaps.  In this
                                         * case the actual size of the text
                                         * string or bitmap is ignored in
                                         * computing desired window
                                         * size. */
    int wrapLength;                     /* Line length (in pixels) at which
                                         * to wrap onto next line.  <= 0
                                         * means don't wrap except at
                                         * newlines. */
    int padX, padY;                     /* Extra space around text (pixels
                                         * to leave on each side).  Ignored
                                         * for bitmaps and images. */
    Tk_Anchor anchor;                   /* Where text/bitmap should be
                                         * displayed inside button
                                         * region. */
    Tk_Justify justify;                 /* Justification to use for
                                         * multi-line text. */
    int indicatorOn;                    /* True means draw indicator, false
                                         * means don't draw it. */
    Blt_Bg selectBg;                    /* For drawing indicator
                                         * background, or perhaps widget
                                         * background, when selected. */
    XColor *selectFg;                   /* For drawing indicator
                                         * background, or perhaps widget
                                         * background, when selected. */
    int textWidth;                      /* Width needed to display text as
                                         * requested, in pixels. */
    int textHeight;                     /* Height needed to display text as
                                         * requested, in pixels. */
    Tk_TextLayout textLayout;           /* Saved text layout
                                         * information. */
    int indicatorSpace;                 /* Horizontal space (in pixels)
                                         * allocated for display of
                                         * indicator. */
    int indicatorDiameter;              /* Diameter of indicator, in
                                         * pixels. */

    int defaultState;                   /* Used in 8.0 (not here) */

    /*
     * For check and radio buttons, the fields below are used to manage the
     * variable indicating the button's state.
     */

    const char *selVarName;             /* Name of variable used to control
                                         * selected state of button.
                                         * Malloc'ed (if not NULL). */
    const char *onValue;                /* Value to store in variable when
                                         * this button is selected.  Malloc'ed
                                         * (if not NULL). */
    const char *offValue;               /* Value to store in variable when
                                         * this button isn't selected.
                                         * Malloc'ed (if * not NULL).  Valid
                                         * only for check * buttons. */
    const char *value;                  /* Value to store in variable when
                                         * this button is selected.  Malloc'ed
                                         * (if not NULL). */

    /*
     * Miscellaneous information:
     */
    Tk_Cursor cursor;                   /* Current cursor for window, or
                                         * None. */
    const char *takeFocus;              /* Value of -takefocus option; not
                                         * used in the C code, but used by
                                         * keyboard traversal scripts.
                                         * Malloc'ed, but may be NULL. */
    Tcl_Obj *cmdObjPtr;                 /* Command to execute when button is
                                         * invoked; valid for buttons only. */
    const char *compound;               /* Value of -compound option;
                                         * specifies whether the button
                                         * should show both an image and
                                         * text, and, if so, how. */
    int repeatDelay;                    /* Value of -repeatdelay option;
                                         * specifies the number of ms after
                                         * which the button will start to
                                         * auto-repeat its command. */
    int repeatInterval;                 /* Value of -repeatinterval option;
                                         * specifies the number of ms
                                         * between auto-repeat invocataions
                                         * of the button command. */
    int flags;                          /* Various flags;  see below for
                                         * definitions. */
    Blt_Picture selectedPicture;
    Blt_Picture normalPicture;
    Blt_Picture disabledPicture;

} Button;

/*
 * Possible "type" values for buttons.  These are the kinds of widgets
 * supported by this file.  The ordering of the type numbers is
 * significant: greater means more features and is used in the code.
 */

#define TYPE_LABEL              0
#define TYPE_BUTTON             1
#define TYPE_PUSH_BUTTON        2
#define TYPE_CHECK_BUTTON       3
#define TYPE_RADIO_BUTTON       4

/*
 * Class names for buttons, indexed by one of the type values above.
 */

static const char *classNames[] = {
    "BltTkLabel", 
    "BltTkButton", 
    "BltTkPushbutton",  
    "BltTkCheckbutton", 
    "BltTkRadiobutton", 
};

/*
 * Flag bits for buttons:
 *
 * REDRAW_PENDING:              Non-zero means a DoWhenIdle handler
 *                              has already been queued to redraw
 *                              this window.
 * SELECTED:                    Non-zero means this button is selected,
 *                              so special highlight should be drawn.
 * GOT_FOCUS:                   Non-zero means this button currently
 *                              has the input focus.
 */

#define REDRAW_PENDING          1
#define SELECTED                2
#define GOT_FOCUS               4

/*
 * Mask values used to selectively enable entries in the configuration
 * specs:
 */

#define LABEL_MASK              BLT_CONFIG_USER_BIT
#define BUTTON_MASK             BLT_CONFIG_USER_BIT << 1
#define PUSH_BUTTON_MASK        BLT_CONFIG_USER_BIT << 2
#define CHECK_BUTTON_MASK       BLT_CONFIG_USER_BIT << 3
#define RADIO_BUTTON_MASK       BLT_CONFIG_USER_BIT << 4
#define ALL_MASK                (LABEL_MASK | BUTTON_MASK \
        | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | PUSH_BUTTON_MASK)

#define ALL_BUTTONS             (BUTTON_MASK | CHECK_BUTTON_MASK | \
                                 RADIO_BUTTON_MASK | PUSH_BUTTON_MASK)

static int configFlags[] = {
    LABEL_MASK, 
    BUTTON_MASK,
    PUSH_BUTTON_MASK,
    CHECK_BUTTON_MASK, 
    RADIO_BUTTON_MASK
};

static Blt_OptionParseProc ObjToImage;
static Blt_OptionPrintProc ImageToObj;
static Blt_OptionFreeProc FreeImageProc;
static Blt_CustomOption imageOption =
{
    ObjToImage, ImageToObj, FreeImageProc, (ClientData)0
};

/*
 * Information used for parsing configuration specs:
 */

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", "Foreground",
        DEF_BUTTON_ACTIVE_BACKGROUND, Blt_Offset(Button, activeBg),
        BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | PUSH_BUTTON_MASK |
        BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", "Foreground",
        DEF_BUTTON_ACTIVE_BG_MONO, Blt_Offset(Button, activeBg),
        BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | PUSH_BUTTON_MASK
        | BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", "ActiveBackground",
        DEF_BUTTON_ACTIVE_FOREGROUND, Blt_Offset(Button, activeFg),
        BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | PUSH_BUTTON_MASK
        | BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", "ActiveForeground",
        DEF_BUTTON_ACTIVE_FG_MONO, Blt_Offset(Button, activeFg),
        BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | PUSH_BUTTON_MASK
        | BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor",
        DEF_BUTTON_ANCHOR, Blt_Offset(Button, anchor), ALL_MASK},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BUTTON_BACKGROUND, Blt_Offset(Button, normalBg),
        ALL_MASK | BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BUTTON_BG_MONO, Blt_Offset(Button, normalBg),
        ALL_MASK | BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL,
        (char *)NULL, 0, ALL_MASK},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL,
        (char *)NULL, 0, ALL_MASK},
    {BLT_CONFIG_BITMAP, "-bitmap", "bitmap", "Bitmap",
        DEF_BUTTON_BITMAP, Blt_Offset(Button, bitmap),
        ALL_MASK | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_BUTTON_BORDERWIDTH, Blt_Offset(Button, borderWidth), 
        BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | LABEL_MASK},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_PUSHBUTTON_BORDERWIDTH, Blt_Offset(Button, borderWidth), 
        PUSH_BUTTON_MASK},
    {BLT_CONFIG_OBJ, "-command", "command", "Command",
        DEF_BUTTON_COMMAND, Blt_Offset(Button, cmdObjPtr),
        BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | PUSH_BUTTON_MASK |
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-compound", "compound", "Compound",
        DEF_BUTTON_COMPOUND, Blt_Offset(Button, compound), 
        ALL_MASK | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
        DEF_BUTTON_CURSOR, Blt_Offset(Button, cursor),
        ALL_MASK | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STATE, "-default", "default", "Default",
        DEF_BUTTON_DEFAULT, Blt_Offset(Button, defaultState), BUTTON_MASK},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground",
        "DisabledForeground", DEF_BUTTON_DISABLED_FOREGROUND,
        Blt_Offset(Button, disabledFg), BUTTON_MASK | CHECK_BUTTON_MASK | 
        RADIO_BUTTON_MASK | PUSH_BUTTON_MASK | BLT_CONFIG_COLOR_ONLY | 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground",
        "DisabledForeground", DEF_BUTTON_DISABLED_FG_MONO,
        Blt_Offset(Button, disabledFg), BUTTON_MASK | CHECK_BUTTON_MASK
        | RADIO_BUTTON_MASK | BLT_CONFIG_MONO_ONLY | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL,
        (char *)NULL, 0, ALL_MASK},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_BUTTON_FONT, 
        Blt_Offset(Button, font), ALL_MASK},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
        DEF_BUTTON_FG, Blt_Offset(Button, normalFg), ALL_MASK},
    {BLT_CONFIG_STRING, "-height", "height", "Height",
        DEF_BUTTON_HEIGHT, Blt_Offset(Button, heightString), ALL_MASK},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground",
        "HighlightBackground", DEF_BUTTON_HIGHLIGHT_BG,
        Blt_Offset(Button, highlightBg), ALL_MASK},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
        DEF_BUTTON_HIGHLIGHT, Blt_Offset(Button, highlightColorPtr),
        ALL_MASK},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
        "HighlightThickness",
        DEF_LABEL_HIGHLIGHT_WIDTH, Blt_Offset(Button, highlightWidth),
        PUSH_BUTTON_MASK | LABEL_MASK},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
        "HighlightThickness",
        DEF_BUTTON_HIGHLIGHT_WIDTH, Blt_Offset(Button, highlightWidth),
        BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK},
    {BLT_CONFIG_CUSTOM, "-image", "image", "Image", DEF_BUTTON_IMAGE, 
        Blt_Offset(Button, image), ALL_MASK | BLT_CONFIG_NULL_OK, &imageOption},
    {BLT_CONFIG_BOOLEAN, "-indicatoron", "indicatorOn", "IndicatorOn",
        DEF_BUTTON_INDICATOR, Blt_Offset(Button, indicatorOn),
        CHECK_BUTTON_MASK | RADIO_BUTTON_MASK},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify",
        DEF_BUTTON_JUSTIFY, Blt_Offset(Button, justify), ALL_MASK},
    {BLT_CONFIG_SYNONYM, "-offimage", "image", (char *)NULL,
        (char *)NULL, 0, CHECK_BUTTON_MASK | PUSH_BUTTON_MASK},
    {BLT_CONFIG_STRING, "-offvalue", "offValue", "Value",
        DEF_BUTTON_OFF_VALUE, Blt_Offset(Button, offValue),
        CHECK_BUTTON_MASK | PUSH_BUTTON_MASK },
    {BLT_CONFIG_STRING, "-onvalue", "onValue", "Value",
        DEF_BUTTON_ON_VALUE, Blt_Offset(Button, onValue),
        CHECK_BUTTON_MASK | PUSH_BUTTON_MASK | BLT_CONFIG_NULL_OK },
    {BLT_CONFIG_SYNONYM, "-onimage", "selectImage", (char *)NULL,
        (char *)NULL, 0, CHECK_BUTTON_MASK | PUSH_BUTTON_MASK},
    {BLT_CONFIG_RELIEF, "-overrelief", "overRelief", "OverRelief",
        DEF_BUTTON_OVER_RELIEF, Blt_Offset(Button, overRelief),
        BUTTON_MASK | PUSH_BUTTON_MASK},
    {BLT_CONFIG_RELIEF, "-overrelief", "overRelief", "OverRelief",
        DEF_LABCHKRAD_OVER_RELIEF, Blt_Offset(Button, overRelief),
        LABEL_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK},
    {BLT_CONFIG_PIXELS_NNEG, "-padx", "padX", "Pad",
        DEF_BUTTON_PADX, Blt_Offset(Button, padX), BUTTON_MASK},
    {BLT_CONFIG_PIXELS_NNEG, "-padx", "padX", "Pad",
        DEF_LABCHKRAD_PADX, Blt_Offset(Button, padX),
        LABEL_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK},
    {BLT_CONFIG_PIXELS_NNEG, "-padx", "padX", "Pad",
        DEF_PUSHBUTTON_PADX, Blt_Offset(Button, padX), PUSH_BUTTON_MASK},
    {BLT_CONFIG_PIXELS_NNEG, "-pady", "padY", "Pad",
        DEF_BUTTON_PADY, Blt_Offset(Button, padY), BUTTON_MASK},
    {BLT_CONFIG_PIXELS_NNEG, "-pady", "padY", "Pad",
        DEF_LABCHKRAD_PADY, Blt_Offset(Button, padY),
        LABEL_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK},
    {BLT_CONFIG_PIXELS_NNEG, "-pady", "padY", "Pad",
        DEF_PUSHBUTTON_PADY, Blt_Offset(Button, padY), PUSH_BUTTON_MASK},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief",
        DEF_BUTTON_RELIEF, Blt_Offset(Button, relief), 
        BUTTON_MASK | PUSH_BUTTON_MASK},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief",
        DEF_LABCHKRAD_RELIEF, Blt_Offset(Button, relief),
        LABEL_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK},
    {BLT_CONFIG_INT, "-repeatdelay", "repeatDelay", "RepeatDelay",
        DEF_BUTTON_REPEAT_DELAY, Blt_Offset(Button, repeatDelay),
        BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | PUSH_BUTTON_MASK},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", 
        "SelectForeground", DEF_BUTTON_SELECT_FOREGROUND, 
        Blt_Offset(Button, selectFg),
        CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | PUSH_BUTTON_MASK | 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
        "SelectBackground", DEF_BUTTON_SELECT_BACKGROUND, 
        Blt_Offset(Button, selectBg),
        CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | PUSH_BUTTON_MASK | 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-selectcolor", "selectBackground", (char *)NULL,
        (char *)NULL, 0, 
        CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | PUSH_BUTTON_MASK | 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-selectimage", "selectImage", "SelectImage",
        DEF_BUTTON_SELECT_IMAGE, Blt_Offset(Button, selectImage),
        CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | PUSH_BUTTON_MASK | 
        BLT_CONFIG_NULL_OK, &imageOption},
    {BLT_CONFIG_STATE, "-state", "state", "State",
        DEF_BUTTON_STATE, Blt_Offset(Button, state),
        BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | PUSH_BUTTON_MASK},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
        DEF_LABEL_TAKE_FOCUS, Blt_Offset(Button, takeFocus),
        LABEL_MASK | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
        DEF_BUTTON_TAKE_FOCUS, Blt_Offset(Button, takeFocus),
        BUTTON_MASK | CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | PUSH_BUTTON_MASK |
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-text", "text", "Text",
        DEF_BUTTON_TEXT, Blt_Offset(Button, text), ALL_MASK},
    {BLT_CONFIG_STRING, "-textvariable", "textVariable", "Variable",
        DEF_BUTTON_TEXT_VARIABLE, Blt_Offset(Button, textVarName),
        ALL_MASK | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_INT, "-underline", "underline", "Underline",
        DEF_BUTTON_UNDERLINE, Blt_Offset(Button, underline), ALL_MASK},
    {BLT_CONFIG_STRING, "-value", "value", "Value",
        DEF_BUTTON_VALUE, Blt_Offset(Button, onValue),
        RADIO_BUTTON_MASK},
    {BLT_CONFIG_STRING, "-value", "value", "Value", (char *)NULL, 
        Blt_Offset(Button, value), PUSH_BUTTON_MASK|BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-variable", "variable", "Variable",
        DEF_RADIOBUTTON_VARIABLE, Blt_Offset(Button, selVarName),
        RADIO_BUTTON_MASK},
    {BLT_CONFIG_STRING, "-variable", "variable", "Variable",
        DEF_CHECKBUTTON_VARIABLE, Blt_Offset(Button, selVarName),
        CHECK_BUTTON_MASK | PUSH_BUTTON_MASK | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-width", "width", "Width",
        DEF_BUTTON_WIDTH, Blt_Offset(Button, widthString), ALL_MASK},
    {BLT_CONFIG_PIXELS_NNEG, "-wraplength", "wrapLength", "WrapLength",
        DEF_BUTTON_WRAP_LENGTH, Blt_Offset(Button, wrapLength), ALL_MASK},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
};

/*
 * String to print out in error messages, identifying options for widget
 * commands for different types of labels or buttons:
 */

static const char *optionStrings[] =
{
    "cget or configure",
    "cget, configure, flash, or invoke",
    "cget, configure, deselect, flash, invoke, select, or toggle",
    "cget, configure, deselect, flash, invoke, or select"
};

/*
 * Forward declarations for procedures defined later in this file:
 */
static void ButtonCmdDeletedProc (ClientData clientData);
static int ButtonCreate (ClientData clientData,
        Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, int type);
static void ButtonEventProc (ClientData clientData,
        XEvent *eventPtr);
static char *ButtonTextVarProc (ClientData clientData,
        Tcl_Interp *interp, const char *name1, const char *name2,
        int flags);
static char *ButtonVarProc (ClientData clientData,
        Tcl_Interp *interp, const char *name1, const char *name2,
        int flags);
static int ButtonWidgetCmd (ClientData clientData,
        Tcl_Interp *interp, int objc, Tcl_Obj *const *objv);
static void ComputeButtonGeometry (Button *butPtr);
static int ConfigureButton (Tcl_Interp *interp,
        Button *butPtr, int objc, Tcl_Obj *const *objv,
        int flags);
static void DestroyButton (Button *butPtr);
static void DisplayButton (ClientData clientData);
static int InvokeButton (Button *butPtr);

static Tcl_ObjCmdProc ButtonCmd, LabelCmd, CheckbuttonCmd, RadiobuttonCmd;

#ifndef USE_TK_STUBS
BLT_EXTERN int TkCopyAndGlobalEval (Tcl_Interp *interp, char *script);

BLT_EXTERN void TkComputeAnchor (Tk_Anchor anchor, Tk_Window tkwin, 
        int padX, int padY, int innerWidth, int innerHeight, int *xPtr, 
        int *yPtr);
#endif

static void
EventuallyRedraw(ClientData clientData)
{
    Button *butPtr = clientData;

    if (butPtr->tkwin != NULL) {
        if (Tk_IsMapped(butPtr->tkwin) && !(butPtr->flags & REDRAW_PENDING)) {
            Tcl_DoWhenIdle(DisplayButton, (ClientData)butPtr);
            butPtr->flags |= REDRAW_PENDING;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageChangedProc
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ImageChangedProc(ClientData clientData, int x, int y, int width, int height,
                 int imageWidth, int imageHeight)        
{
    Button *butPtr = clientData;

    if (butPtr->tkwin != NULL) {
        ComputeButtonGeometry(butPtr);
    }
    EventuallyRedraw(clientData);
}


/*ARGSUSED*/
static void
FreeImageProc(ClientData clientData, Display *display, char *widgRec, 
              int offset)
{
    Tk_Image *imagePtr = (Tk_Image *)(widgRec + offset);

    if (*imagePtr != NULL) {
        Tk_FreeImage(*imagePtr);
        *imagePtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToImage --
 *
 *      Given an image name, get the Tk image associated with it.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToImage(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    Button *butPtr = (Button *)(widgRec);
    Tk_Image *imagePtr = (Tk_Image *)(widgRec + offset);
    Tk_Image image;
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    if ((flags & BLT_CONFIG_NULL_OK) && (length == 0)) {
        image = NULL;
    } else {
        image = Tk_GetImage(interp, butPtr->tkwin, string, ImageChangedProc,
                butPtr);
        if (image == NULL) {
            return TCL_ERROR;
        }
    }
    if (*imagePtr != NULL) {
        Tk_FreeImage(*imagePtr);
    }
    *imagePtr = image;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageToObj --
 *
 *      Convert the image name into a string Tcl_Obj.
 *
 * Results:
 *      The string representation of the image is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ImageToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)  
{
    Tk_Image image = *(Tk_Image *)(widgRec + offset);

    if (image == NULL) {
        return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(Blt_Image_Name(image), -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonCmd, CheckbuttonCmd, LabelCmd, RadiobuttonCmd, PushbuttonCmd --
 *
 *      These procedures are invoked to process the "button", "label",
 *      "radiobutton", "checkbutton", and "pushbutton" TCL commands.  
 *      See the user documentation for details on what they do.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.  These procedures are just wrappers;
 *      they call ButtonCreate to do all of the real work.
 *
 *---------------------------------------------------------------------------
 */

static int
ButtonCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    return ButtonCreate(clientData, interp, objc, objv, TYPE_BUTTON);
}

static int
CheckbuttonCmd(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    return ButtonCreate(clientData, interp, objc, objv, TYPE_CHECK_BUTTON);
}

static int
LabelCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    return ButtonCreate(clientData, interp, objc, objv, TYPE_LABEL);
}

static int
RadiobuttonCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    return ButtonCreate(clientData, interp, objc, objv, TYPE_RADIO_BUTTON);
}

static int
PushbuttonCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    return ButtonCreate(clientData, interp, objc, objv, TYPE_PUSH_BUTTON);
}

static void
FreeButton(DestroyData dataPtr)
{
    Blt_Free(dataPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonCreate --
 *
 *      This procedure does all the real work of implementing the
 *      "button", "label", "radiobutton", and "checkbutton" Tcl
 *      commands.  See the user documentation for details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

/*ARGSUSED*/
static int
ButtonCreate(
    ClientData clientData,      /* Main window associated with
                                 * interpreter. */
    Tcl_Interp *interp,         /* Current interpreter. */
    int objc,                   /* Number of arguments. */
    Tcl_Obj *const *objv,       /* Argument strings. */
    int type)                   /* Type of button to create: TYPE_LABEL,
                                 * TYPE_BUTTON, TYPE_CHECK_BUTTON, or
                                 * TYPE_RADIO_BUTTON. */
{
    Button *butPtr;
    Tk_Window tkwin;
    char *path;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " pathName ?options?\"", (char *)NULL);
        return TCL_ERROR;
    }
    /*
     * First time in this interpreter, set up procs and initialize various
     * bindings for the widget.  If the proc doesn't already exist, source
     * it from "$blt_library/bltPushButton.tcl".  We've deferred sourcing
     * this file until now so that the user could reset the variable
     * $blt_library from within her script.
     */
    if (!Blt_CommandExists(interp, "::blt::Button::Up")) {
        static char cmd[] = "source [file join $blt_library bltPushButton.tcl]";
        if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
            char info[200];
            Blt_FormatString(info, 200, "\n    (while loading bindings for %.50s)", 
                    Tcl_GetString(objv[0]));
            Tcl_AddErrorInfo(interp, info);
            return TCL_ERROR;
        }
    }
    /*
     * Create the new window.
     */

    path = Tcl_GetString(objv[1]);
    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), path, 
        (char *)NULL);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    /*
     * Initialize the data structure for the button.
     */

    butPtr = Blt_AssertCalloc(1, sizeof(Button));
    butPtr->tkwin = tkwin;
    butPtr->display = Tk_Display(tkwin);
    butPtr->widgetCmd = Tcl_CreateObjCommand(interp, Tk_PathName(butPtr->tkwin),
        ButtonWidgetCmd, butPtr, ButtonCmdDeletedProc);

    butPtr->interp = interp;
    butPtr->type = type;
    butPtr->underline = -1;
    butPtr->state = STATE_NORMAL;
    butPtr->relief = TK_RELIEF_FLAT;
    butPtr->anchor = TK_ANCHOR_CENTER;
    butPtr->justify = TK_JUSTIFY_CENTER;
    butPtr->defaultState = STATE_DISABLED;
    butPtr->overRelief = TK_RELIEF_RAISED;


    Tk_SetClass(tkwin, classNames[type]);
    Tk_CreateEventHandler(butPtr->tkwin,
        ExposureMask | StructureNotifyMask | FocusChangeMask,
        ButtonEventProc, butPtr);
    if (ConfigureButton(interp, butPtr, objc - 2, objv + 2,
            configFlags[type]) != TCL_OK) {
        Tk_DestroyWindow(butPtr->tkwin);
        return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(butPtr->tkwin), -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonWidgetCmd --
 *
 *      This procedure is invoked to process the TCL command
 *      that corresponds to a widget managed by this module.
 *      See the user documentation for details on what it does.
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
ButtonWidgetCmd(
    ClientData clientData,      /* Information about button widget. */
    Tcl_Interp *interp,         /* Current interpreter. */
    int objc,                   /* Number of arguments. */
    Tcl_Obj *const *objv)       /* Argument strings. */
{
    Button *butPtr = clientData;
    char *string;
    int c;
    int length;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " option ?arg arg ...?\"", 
                (char *)NULL);
        return TCL_ERROR;
    }
    Tcl_Preserve(butPtr);
    string = Tcl_GetStringFromObj(objv[1], &length);
    c = string[0];
    if ((c == 'c') && (length >= 2) && (strncmp(string, "cget", length) == 0)) {
        if (objc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " cget option\"", (char *)NULL);
            goto error;
        }
        if (Blt_ConfigureValueFromObj(interp, butPtr->tkwin, configSpecs,
                (char *)butPtr, objv[2], configFlags[butPtr->type]) != TCL_OK) {
            goto error;
        }
    } else if ((c == 'c') && (length >= 2) && 
               (strncmp(string, "configure", length) == 0)) {
        if (objc == 2) {
            if (Blt_ConfigureInfoFromObj(interp, butPtr->tkwin, configSpecs, 
                        (char *)butPtr, (Tcl_Obj *)NULL, 
                        configFlags[butPtr->type]) != TCL_OK) {
                goto error;
            }
        } else if (objc == 3) {
            if (Blt_ConfigureInfoFromObj(interp, butPtr->tkwin, configSpecs, 
                (char *)butPtr, objv[2], configFlags[butPtr->type]) != TCL_OK) {
                goto error;
            }
        } else {
            if (ConfigureButton(interp, butPtr, objc - 2, objv + 2,
                configFlags[butPtr->type] | BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
                goto error;
            }
        }
    } else if ((c == 'd') && (strncmp(string, "deselect", length) == 0) && 
               (butPtr->type >= TYPE_PUSH_BUTTON)) {
        if (objc > 2) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " deselect\"", (char *)NULL);
            goto error;
        }
        if (butPtr->type == TYPE_CHECK_BUTTON) {
            if (Tcl_SetVar(interp, butPtr->selVarName, butPtr->offValue,
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
                goto error;
            }
        } else if (butPtr->type == TYPE_PUSH_BUTTON) {
            if (Tcl_SetVar(interp, butPtr->selVarName, butPtr->offValue,
                        TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
                goto error;
            }
        } else if (butPtr->flags & SELECTED) {
            if (Tcl_SetVar(interp, butPtr->selVarName, "",
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
                goto error;
            };
        }
    } else if ((c == 'f') && (strncmp(string, "flash", length) == 0)
        && (butPtr->type != TYPE_LABEL)) {
        int i;

        if (objc > 2) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " flash\"", (char *)NULL);
            goto error;
        }
        if (butPtr->state != STATE_DISABLED) {
            for (i = 0; i < 4; i++) {
                butPtr->state = (butPtr->state == STATE_NORMAL)
                    ? STATE_ACTIVE : STATE_NORMAL;
                Blt_Bg_SetFromBackground(butPtr->tkwin,
                    (butPtr->state == STATE_ACTIVE) ? butPtr->activeBg
                    : butPtr->normalBg);
                DisplayButton(butPtr);

                /*
                 * Special note: must cancel any existing idle handler
                 * for DisplayButton;  it's no longer needed, and DisplayButton
                 * cleared the REDRAW_PENDING flag.
                 */

                Tcl_CancelIdleCall(DisplayButton, butPtr);
#if !defined(WIN32) && !defined(MACOSX)
                XFlush(butPtr->display);
#endif
                Tcl_Sleep(50);
            }
        }
    } else if ((c == 'i') && (strncmp(string, "invoke", length) == 0)
        && (butPtr->type > TYPE_LABEL)) {
        if (objc > 2) {
            Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " invoke\"", (char *)NULL);
            goto error;
        }
        if (butPtr->state != STATE_DISABLED) {
            if (InvokeButton(butPtr) != TCL_OK) {
                goto error;
            }
        }
    } else if ((c == 's') && (strncmp(string, "select", length) == 0) && 
               (butPtr->type >= TYPE_PUSH_BUTTON)) {
        const char *string;
        if (objc > 2) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " select\"", (char *)NULL);
            goto error;
        }
        string = (butPtr->value != NULL) ? butPtr->value : butPtr->onValue;
        if (Tcl_SetVar(interp, butPtr->selVarName, string,
                       TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
            goto error;
        }
    } else if ((c == 't') && (strncmp(string, "toggle", length) == 0) && 
               (length >= 2) && (butPtr->type == TYPE_PUSH_BUTTON)) {
        if (objc > 2) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " toggle\"", (char *)NULL);
            goto error;
        }
        if (butPtr->flags & SELECTED) {
            if (Tcl_SetVar(interp, butPtr->selVarName, butPtr->offValue,
                    TCL_GLOBAL_ONLY) == NULL) {
                goto error;
            }
        } else {
            const char *string;

            string = (butPtr->value != NULL) ? butPtr->value : butPtr->onValue;
            if (Tcl_SetVar(interp, butPtr->selVarName, string, TCL_GLOBAL_ONLY)
                == NULL) {
                goto error;
            }
        }
    } else {
        Tcl_AppendResult(interp, "bad option \"", Tcl_GetString(objv[1]), 
                "\": must be ", optionStrings[butPtr->type], (char *)NULL);
        goto error;
    }
    Tcl_Release(butPtr);
    return TCL_OK;

  error:
    Tcl_Release(butPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyButton --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *      to clean up the internal structure of a button at a safe time
 *      (when no-one is using it anymore).
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
DestroyButton(Button *butPtr)
{
    /*
     * Free up all the stuff that requires special handling, then
     * let Blt_FreeOptions handle all the standard option-related
     * stuff.
     */

    if (butPtr->textVarName != NULL) {
        Tcl_UntraceVar(butPtr->interp, butPtr->textVarName,
            TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
            ButtonTextVarProc, butPtr);
    }
    if (butPtr->normalTextGC != None) {
        Tk_FreeGC(butPtr->display, butPtr->normalTextGC);
    }
    if (butPtr->activeTextGC != None) {
        Tk_FreeGC(butPtr->display, butPtr->activeTextGC);
    }
    if (butPtr->gray != None) {
        Tk_FreeBitmap(butPtr->display, butPtr->gray);
    }
    if (butPtr->disabledGC != None) {
        Tk_FreeGC(butPtr->display, butPtr->disabledGC);
    }
    if (butPtr->selectedPicture != NULL) {
        Blt_FreePicture(butPtr->selectedPicture);
    }
    if (butPtr->disabledPicture != NULL) {
        Blt_FreePicture(butPtr->disabledPicture);
    }
    if (butPtr->normalPicture != NULL) {
        Blt_FreePicture(butPtr->normalPicture);
    }
    if (butPtr->copyGC != None) {
        Tk_FreeGC(butPtr->display, butPtr->copyGC);
    }
    if (butPtr->selVarName != NULL) {
        Tcl_UntraceVar(butPtr->interp, butPtr->selVarName,
            TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
            ButtonVarProc, (ClientData)butPtr);
    }
    Blt_TkTextLayout_Free(butPtr->textLayout);
    Blt_FreeOptions(configSpecs, (char *)butPtr, butPtr->display,
        configFlags[butPtr->type]);
    Tcl_EventuallyFree((ClientData)butPtr, FreeButton);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureButton --
 *
 *      This procedure is called to process an objv/objc list, plus
 *      the Tk option database, in order to configure (or
 *      reconfigure) a button widget.
 *
 * Results:
 *      The return value is a standard TCL result.  If TCL_ERROR is
 *      returned, then interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for butPtr;  old resources get freed, if there
 *      were any.  The button is redisplayed.
 *
 *---------------------------------------------------------------------------
 */

static int
ConfigureButton(
    Tcl_Interp *interp,         /* Used for error reporting. */
    Button *butPtr,             /* Information about widget;  may or may
                                 * not already have values for some fields. */
    int objc,                   /* Number of valid entries in objv. */
    Tcl_Obj *const *objv,       /* Arguments. */
    int flags)                  /* Flags to pass to Blt_ConfigureWidget. */
{
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;

    /*
     * Eliminate any existing trace on variables monitored by the button.
     */
    if (butPtr->textVarName != NULL) {
        Tcl_UntraceVar(interp, butPtr->textVarName,
            TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
            ButtonTextVarProc, butPtr);
    }
    if (butPtr->selVarName != NULL) {
        Tcl_UntraceVar(interp, butPtr->selVarName,
            TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
            ButtonVarProc, butPtr);
    }
    if (Blt_ConfigureWidgetFromObj(interp, butPtr->tkwin, configSpecs,
            objc, objv, (char *)butPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    /*
     * A few options need special processing, such as setting the
     * background from a 3-D border, or filling in complicated
     * defaults that couldn't be specified to Blt_ConfigureWidget.
     */

    if ((butPtr->state == STATE_ACTIVE) && !Tk_StrictMotif(butPtr->tkwin)) {
        Blt_Bg_SetFromBackground(butPtr->tkwin, butPtr->activeBg);
    } else {
        Blt_Bg_SetFromBackground(butPtr->tkwin, butPtr->normalBg);
        if ((butPtr->state != STATE_NORMAL) && (butPtr->state != STATE_ACTIVE)
            && (butPtr->state != STATE_DISABLED)) {
            Tcl_AppendResult(interp, "bad state value \"", 
                Blt_Itoa(butPtr->state), 
                "\": must be normal, active, or disabled", (char *)NULL);
            butPtr->state = STATE_NORMAL;
            return TCL_ERROR;
        }
    }
    if (butPtr->normalBg != NULL) {
        Blt_Bg_SetChangedProc(butPtr->normalBg, EventuallyRedraw, butPtr);
    }
    if (butPtr->activeBg != NULL) {
        Blt_Bg_SetChangedProc(butPtr->activeBg, EventuallyRedraw, butPtr);
    }
    if (butPtr->selectBg != NULL) {
        Blt_Bg_SetChangedProc(butPtr->selectBg, EventuallyRedraw, butPtr);
    }

    if ((butPtr->defaultState != STATE_ACTIVE)
        && (butPtr->defaultState != STATE_DISABLED)
        && (butPtr->defaultState != STATE_NORMAL)) {
        Tcl_AppendResult(interp, "bad -default value \"", butPtr->defaultState,
            "\": must be normal, active, or disabled", (char *)NULL);
        butPtr->defaultState = STATE_DISABLED;
        return TCL_ERROR;
    }
    if (butPtr->highlightWidth < 0) {
        butPtr->highlightWidth = 0;
    }
    gcValues.font = Blt_Font_Id(butPtr->font);
    gcValues.foreground = butPtr->normalFg->pixel;
    gcValues.background = Blt_Bg_BorderColor(butPtr->normalBg)->pixel;

    /*
     * Note: GraphicsExpose events are disabled in normalTextGC because it's
     * used to copy stuff from an off-screen pixmap onto the screen (we know
     * that there's no problem with obscured areas).
     */

    gcValues.graphics_exposures = False;
    newGC = Tk_GetGC(butPtr->tkwin,
        GCForeground | GCBackground | GCFont | GCGraphicsExposures,
        &gcValues);
    if (butPtr->normalTextGC != None) {
        Tk_FreeGC(butPtr->display, butPtr->normalTextGC);
    }
    butPtr->normalTextGC = newGC;

    if (butPtr->activeFg != NULL) {
        gcValues.font = Blt_Font_Id(butPtr->font);
        gcValues.foreground = butPtr->activeFg->pixel;
        gcValues.background = Blt_Bg_BorderColor(butPtr->activeBg)->pixel;
        newGC = Tk_GetGC(butPtr->tkwin,
            GCForeground | GCBackground | GCFont, &gcValues);
        if (butPtr->activeTextGC != None) {
            Tk_FreeGC(butPtr->display, butPtr->activeTextGC);
        }
        butPtr->activeTextGC = newGC;
    }
    if (butPtr->type != TYPE_LABEL) {
        gcValues.font = Blt_Font_Id(butPtr->font);
        gcValues.background = Blt_Bg_BorderColor(butPtr->normalBg)->pixel;
        if ((butPtr->disabledFg != NULL) && (butPtr->image == NULL)) {
            gcValues.foreground = butPtr->disabledFg->pixel;
            mask = GCForeground | GCBackground | GCFont;
        } else {
            gcValues.foreground = gcValues.background;
            if (butPtr->gray == None) {
                butPtr->gray = Tk_GetBitmap(interp, butPtr->tkwin,
                    Tk_GetUid("gray50"));
                if (butPtr->gray == None) {
                    return TCL_ERROR;
                }
            }
            gcValues.fill_style = FillStippled;
            gcValues.stipple = butPtr->gray;
            mask = GCForeground | GCFillStyle | GCStipple;
        }
        newGC = Tk_GetGC(butPtr->tkwin, mask, &gcValues);
        if (butPtr->disabledGC != None) {
            Tk_FreeGC(butPtr->display, butPtr->disabledGC);
        }
        butPtr->disabledGC = newGC;
    }
    if (butPtr->copyGC == None) {
        butPtr->copyGC = Tk_GetGC(butPtr->tkwin, 0, &gcValues);
    }
    if (butPtr->padX < 0) {
        butPtr->padX = 0;
    }
    if (butPtr->padY < 0) {
        butPtr->padY = 0;
    }
    if (butPtr->type >= TYPE_PUSH_BUTTON) {
        const char *value;

        if (butPtr->selVarName == NULL) {
            butPtr->selVarName = Blt_AssertStrdup(Tk_Name(butPtr->tkwin));
        }
        /*
         * Select the button if the associated variable has the
         * appropriate value, initialize the variable if it doesn't
         * exist, then set a trace on the variable to monitor future
         * changes to its value.
         */
        value = Tcl_GetVar(interp, butPtr->selVarName, TCL_GLOBAL_ONLY);
        butPtr->flags &= ~SELECTED;
        if (value != NULL) {
            const char *string;

            string = ((butPtr->type == TYPE_PUSH_BUTTON) && 
                     (butPtr->value != NULL)) ? butPtr->value : butPtr->onValue;
            if (strcmp(value, string) == 0) {
                butPtr->flags |= SELECTED;
            }
        } else {
            if (butPtr->type == TYPE_PUSH_BUTTON) {
                if (butPtr->value != NULL) {
                    if (Tcl_SetVar(interp, butPtr->selVarName, butPtr->value,
                        TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
                        return TCL_ERROR;
                    }
                }
            } else {
                const char *value;

                value = (butPtr->type == TYPE_CHECK_BUTTON) ? 
                    butPtr->offValue : "";
                if (Tcl_SetVar(interp, butPtr->selVarName, value,
                               TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
                    return TCL_ERROR;
                }
            }
        }
        Tcl_TraceVar(interp, butPtr->selVarName,
            TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
            ButtonVarProc, (ClientData)butPtr);
    }
    /*
     * Get the images for the widget, if there are any.  Allocate the
     * new images before freeing the old ones, so that the reference
     * counts don't go to zero and cause image data to be discarded.
     */

    if ((butPtr->image == NULL) && (butPtr->bitmap == None)
        && (butPtr->textVarName != NULL)) {
        /*
         * The button must display the value of a variable: set up a trace
         * on the variable's value, create the variable if it doesn't
         * exist, and fetch its current value.
         */

        const char *value;

        value = Tcl_GetVar(interp, butPtr->textVarName, TCL_GLOBAL_ONLY);
        if (value == NULL) {
            if (Tcl_SetVar(interp, butPtr->textVarName, butPtr->text,
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
                return TCL_ERROR;
            }
        } else {
            if (butPtr->text != NULL) {
                Blt_Free(butPtr->text);
            }
            butPtr->text = Blt_AssertStrdup(value);
        }
        Tcl_TraceVar(interp, butPtr->textVarName,
            TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
            ButtonTextVarProc, (ClientData)butPtr);
    }
    if ((butPtr->bitmap != None) || (butPtr->image != NULL)) {
        if (Tk_GetPixels(interp, butPtr->tkwin, butPtr->widthString,
                &butPtr->width) != TCL_OK) {
          widthError:
            Tcl_AddErrorInfo(interp, "\n    (processing -width option)");
            return TCL_ERROR;
        }
        if (Tk_GetPixels(interp, butPtr->tkwin, butPtr->heightString,
                &butPtr->height) != TCL_OK) {
          heightError:
            Tcl_AddErrorInfo(interp, "\n    (processing -height option)");
            return TCL_ERROR;
        }
    } else {
        if (Tcl_GetInt(interp, butPtr->widthString, &butPtr->width)
            != TCL_OK) {
            goto widthError;
        }
        if (Tcl_GetInt(interp, butPtr->heightString, &butPtr->height)
            != TCL_OK) {
            goto heightError;
        }
    }
    ComputeButtonGeometry(butPtr);

    /*
     * Lastly, arrange for the button to be redisplayed.
     */
    EventuallyRedraw(butPtr);
    return TCL_OK;
}

static void
DrawCheckButton(Tk_Window tkwin, Drawable drawable, Button *butPtr, 
                int x, int y) 
{
    Blt_Picture picture;
    Blt_Painter painter;
    int on, dim;
    int w, h;

    dim = butPtr->indicatorDiameter;
    x -= butPtr->indicatorSpace;
    y -= dim / 2;

    w = h = butPtr->indicatorSpace /* - 2 * butPtr->borderWidth; */;
    on = (butPtr->flags & SELECTED);
    picture = NULL;
    if (butPtr->state & STATE_DISABLED) {
        if (butPtr->disabledPicture == NULL) {
            butPtr->disabledPicture = 
                Blt_PaintCheckbox(dim, dim, 
                        Blt_Bg_BorderColor(butPtr->normalBg), 
                        butPtr->disabledFg, butPtr->disabledFg, on);
        } 
        picture = butPtr->disabledPicture;
    } else if (butPtr->flags & SELECTED) {
        if (butPtr->selectedPicture == NULL) {
            butPtr->selectedPicture = 
                Blt_PaintCheckbox(dim, dim, 
                        Blt_Bg_BorderColor(butPtr->selectBg), 
                        butPtr->activeFg, 
                        butPtr->selectFg,
                        TRUE);
        } 
        picture = butPtr->selectedPicture;
    } else {
        if (butPtr->normalPicture == NULL) {
            butPtr->normalPicture = 
                Blt_PaintCheckbox(dim, dim, 
                        Blt_Bg_BorderColor(butPtr->selectBg),
                        butPtr->normalFg, 
                        butPtr->selectFg,
                        FALSE);
        } 
        picture = butPtr->normalPicture;
    }
    painter = Blt_GetPainter(tkwin, 1.0);
    Blt_PaintPicture(painter, drawable, picture, 0, 0, w, h, x, y, 0);
}

static void
DrawRadioButton(Tk_Window tkwin, Drawable drawable, Button *butPtr, 
                Blt_Bg bg, int x, int y) 
{
    Blt_Picture picture;
    Blt_Painter painter;
    int on, dim;

    dim = butPtr->indicatorDiameter;
    x -= butPtr->indicatorSpace + butPtr->borderWidth;
    y -= dim / 2;

    on = (butPtr->flags & SELECTED);
    picture = NULL;
#ifndef notdef
    if (butPtr->state & STATE_DISABLED) {
        picture = Blt_PaintRadioButton2(dim, dim, bg, 
                Blt_Bg_BorderColor(butPtr->normalBg), 
                butPtr->disabledFg, on);
    } else if (butPtr->flags & SELECTED) {
        picture = Blt_PaintRadioButton2(dim, dim, bg,
                        Blt_Bg_BorderColor(butPtr->selectBg), 
                        butPtr->selectFg,
                        TRUE);
    } else {
        picture = Blt_PaintRadioButton2(dim, dim, bg,
                        Blt_Bg_BorderColor(butPtr->selectBg),
                        butPtr->selectFg,
                        FALSE);
    }
#else
    if (butPtr->state & STATE_DISABLED) {
        picture = Blt_PaintRadioButtonOld(dim, dim, Blt_Bg_BorderColor(bg), 
                Blt_Bg_BorderColor(butPtr->normalBg), 
                butPtr->disabledFg, butPtr->disabledFg, on);
    } else if (butPtr->flags & SELECTED) {
        picture = Blt_PaintRadioButtonOld(dim, dim, Blt_Bg_BorderColor(bg),
                        Blt_Bg_BorderColor(butPtr->selectBg), 
                        butPtr->activeFg, 
                        butPtr->selectFg,
                        TRUE);
    } else {
        picture = Blt_PaintRadioButtonOld(dim, dim, Blt_Bg_BorderColor(bg),
                        Blt_Bg_BorderColor(butPtr->selectBg),
                        butPtr->normalFg, 
                        butPtr->selectFg,
                        FALSE);
    }
#endif
    painter = Blt_GetPainter(tkwin, 1.0);
    Blt_PaintPicture(painter, drawable, picture, 0, 0, dim, dim, x, y, 0);
    Blt_FreePicture(picture);
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayButton --
 *
 *      This procedure is invoked to display a button widget.  It is
 *      normally invoked as an idle handler.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Commands are output to X to display the button in its
 *      current mode.  The REDRAW_PENDING flag is cleared.
 *
 *---------------------------------------------------------------------------
 */

static void
DisplayButton(ClientData clientData)
{
    Button *butPtr = clientData;
    GC gc;
    Blt_Bg bg;
    Pixmap pixmap;
    int x = 0;                  /* Initialization only needed to stop
                                 * compiler warning. */
    int y, relief;
    Tk_Window tkwin = butPtr->tkwin;
    int w, h;
    int offset;                 /* 0 means this is a label widget.  1 means
                                 * it is a flavor of button, so we offset
                                 * the text to make the button appear to
                                 * move up and down as the relief changes. */

    butPtr->flags &= ~REDRAW_PENDING;
    if ((butPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
        return;
    }
    w = Tk_Width(butPtr->tkwin);
    h = Tk_Height(butPtr->tkwin);
    if ((w < 2) || (h < 2)) {
        return;
    }
    bg = butPtr->normalBg;
    if ((butPtr->state == STATE_DISABLED) && (butPtr->disabledFg != NULL)) {
        gc = butPtr->disabledGC;
    } else if ((butPtr->state == STATE_ACTIVE)
        && !Tk_StrictMotif(butPtr->tkwin)) {
        gc = butPtr->activeTextGC;
        bg = butPtr->activeBg;
    } else {
        gc = butPtr->normalTextGC;
    }
    if ((butPtr->flags & SELECTED) && (butPtr->state != STATE_ACTIVE) && 
        (butPtr->selectBg != NULL) && (!butPtr->indicatorOn)) {
        bg = butPtr->selectBg;
    }
    if ((butPtr->type == TYPE_PUSH_BUTTON) && (butPtr->flags & SELECTED) && 
        (butPtr->state == STATE_ACTIVE) && (butPtr->selectBg != NULL) && 
        (!butPtr->indicatorOn)) {
        bg = butPtr->selectBg;
    }
    /*
     * Override the relief specified for the button if this is a
     * checkbutton or radiobutton and there's no indicator.
     */
    relief = butPtr->relief;
    if ((butPtr->type >= TYPE_PUSH_BUTTON) && (!butPtr->indicatorOn)) {
        relief = (butPtr->flags & SELECTED) ? TK_RELIEF_SUNKEN
            : butPtr->relief;
    }
    offset = (butPtr->type == TYPE_BUTTON) && !Tk_StrictMotif(butPtr->tkwin);

    /*
     * In order to avoid screen flashes, this procedure redraws
     * the button in a pixmap, then copies the pixmap to the
     * screen in a single operation.  This means that there's no
     * point in time where the on-sreen image has been cleared.
     */

    pixmap = Tk_GetPixmap(butPtr->display, Tk_WindowId(tkwin), w, h,
                          Tk_Depth(tkwin));
    Blt_Bg_FillRectangle(tkwin, pixmap, bg, 0, 0, w, h, 0, TK_RELIEF_FLAT);

    /*
     * Display image or bitmap or text for button.
     */

    if (butPtr->image != NULL) {
        int iw, ih;
        int dx, dy;

        Tk_SizeOfImage(butPtr->image, &iw, &ih);
        TkComputeAnchor(butPtr->anchor, tkwin, butPtr->padX, butPtr->padY,
            butPtr->indicatorSpace + iw, ih, &x, &y);
        x += butPtr->indicatorSpace;
        x += offset;
        y += offset;
        if (relief == TK_RELIEF_RAISED) {
            x -= offset;
            y -= offset;
        } else if (relief == TK_RELIEF_SUNKEN) {
            x += offset;
            y += offset;
        }
        dx = x, dy = y;
        if (dx < 0) {
            iw += dx;
            dx = 0;
        }
        if (dy < 0) {
            ih += dy;
            dy = 0;
        }
        if ((dx + iw) > w) {
            iw = w - dx;
        }
        if ((dy + ih) > h) {
            ih = h - dy;
        }
        if ((butPtr->selectImage != NULL) && (butPtr->flags & SELECTED)) {
            Tk_RedrawImage(butPtr->selectImage, 0, 0, iw, ih, pixmap,
                           dx, dy);
        } else {
            Tk_RedrawImage(butPtr->image, 0, 0, iw, ih, pixmap, dx, dy);
        }
        y += h / 2;
    } else if (butPtr->bitmap != None) {
        int iw, ih;

        Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &iw, &ih);
        TkComputeAnchor(butPtr->anchor, tkwin, butPtr->padX, butPtr->padY,
            butPtr->indicatorSpace + iw, ih, &x, &y);
        x += butPtr->indicatorSpace;
        x += offset;
        y += offset;
        if (relief == TK_RELIEF_RAISED) {
            x -= offset;
            y -= offset;
        } else if (relief == TK_RELIEF_SUNKEN) {
            x += offset;
            y += offset;
        }
        XSetClipOrigin(butPtr->display, gc, x, y);
        XCopyPlane(butPtr->display, butPtr->bitmap, pixmap, gc, 0, 0,
                   (unsigned int)w, (unsigned int)h, x, y, 1);
        XSetClipOrigin(butPtr->display, gc, 0, 0);
        y += h / 2;
    } else {
        TkComputeAnchor(butPtr->anchor, tkwin, butPtr->padX, butPtr->padY,
            butPtr->indicatorSpace + butPtr->textWidth,
            butPtr->textHeight, &x, &y);

        x += butPtr->indicatorSpace;

        x += offset;
        y += offset;
        if (relief == TK_RELIEF_RAISED) {
            x -= offset;
            y -= offset;
        } else if (relief == TK_RELIEF_SUNKEN) {
            x += offset;
            y += offset;
        }
        Blt_TkTextLayout_Draw(butPtr->display, pixmap, gc, butPtr->textLayout,
            x, y, 0, -1);
        Blt_TkTextLayout_UnderlineSingleChar(butPtr->display, pixmap, gc,
            butPtr->textLayout, x, y, butPtr->underline);
        y += butPtr->textHeight / 2;
    }

    /*
     * Draw the indicator for check buttons and radio buttons.  At this
     * point x and y refer to the top-left corner of the text or image
     * or bitmap.
     */
    if (butPtr->indicatorOn) {
        if (butPtr->type == TYPE_CHECK_BUTTON) {
            DrawCheckButton(tkwin, pixmap, butPtr, x, y);
            x -= GAP;
        } else if (butPtr->type == TYPE_RADIO_BUTTON) {
            DrawRadioButton(tkwin, pixmap, butPtr, bg, x, y);
        } 
    }
    /*
     * If the button is disabled with a stipple rather than a special
     * foreground color, generate the stippled effect.  If the widget
     * is selected and we use a different background color when selected,
     * must temporarily modify the GC.
     */
    if ((butPtr->state == STATE_DISABLED) && 
        ((butPtr->disabledFg == NULL) || (butPtr->image != NULL))) {
        if ((butPtr->flags & SELECTED) && (!butPtr->indicatorOn) && 
            (butPtr->selectBg != NULL)) {
            XSetForeground(butPtr->display, butPtr->disabledGC,
                Blt_Bg_BorderColor(butPtr->selectBg)->pixel);
        }
        XFillRectangle(butPtr->display, pixmap, butPtr->disabledGC,
            butPtr->inset, butPtr->inset,
            (unsigned)(w - 2 * butPtr->inset),
            (unsigned)(h - 2 * butPtr->inset));
        if ((butPtr->flags & SELECTED) && !butPtr->indicatorOn
            && (butPtr->selectBg != NULL)) {
            XSetForeground(butPtr->display, butPtr->disabledGC,
                Blt_Bg_BorderColor(butPtr->normalBg)->pixel);
        }
    }

    /*
     * Draw the border and traversal highlight last.  This way, if the
     * button's contents overflow they'll be covered up by the border.
     */
    if (relief != TK_RELIEF_FLAT) {
        int inset = butPtr->highlightWidth;
        int rw, rh;

        rw = w - 2 * inset;
        rh = h - 2 * inset;
        if ((rw > 0) && (rh > 0))  {
            if (butPtr->defaultState == STATE_ACTIVE) {
                inset += 2;
                Blt_Bg_DrawRectangle(tkwin, pixmap, bg, inset, inset, rw, rh,
                        1, TK_RELIEF_SUNKEN);
                inset += 3;
            }
            Blt_Bg_DrawRectangle(tkwin, pixmap, bg, inset, inset, rw, rh, 
                butPtr->borderWidth, relief);
        }
    }
    if (butPtr->highlightWidth != 0) {
        if (butPtr->flags & GOT_FOCUS) {
            GC highlightGC;

            highlightGC = Tk_GCForColor(butPtr->highlightColorPtr, pixmap);
            Tk_DrawFocusHighlight(tkwin, highlightGC, butPtr->highlightWidth, 
                pixmap);
        } else {
            Blt_Bg_DrawFocus(tkwin, butPtr->highlightBg, butPtr->highlightWidth,
                pixmap);
        }
    }
    /*
     * Copy the information from the off-screen pixmap onto the screen,
     * then delete the pixmap.
     */
    XCopyArea(butPtr->display, pixmap, Tk_WindowId(tkwin), butPtr->copyGC, 
        0, 0, (unsigned)w, (unsigned)h, 0, 0);
    Tk_FreePixmap(butPtr->display, pixmap);
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeButtonGeometry --
 *
 *      After changes in a button's text or bitmap, this procedure
 *      recomputes the button's geometry and passes this information
 *      along to the geometry manager for the window.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The button's window may change size.
 *
 *---------------------------------------------------------------------------
 */

static void
ComputeButtonGeometry(Button *butPtr)
{
    int width, height;

    if (butPtr->highlightWidth < 0) {
        butPtr->highlightWidth = 0;
    }
    butPtr->inset = butPtr->highlightWidth + butPtr->borderWidth;

    /*
     * Leave room for the default ring if needed.
     */

    if (butPtr->defaultState == STATE_ACTIVE) {
        butPtr->inset += 5;
    }
    butPtr->indicatorSpace = 0;
    if (butPtr->image != NULL) {
        Tk_SizeOfImage(butPtr->image, &width, &height);
      imageOrBitmap:
        if (butPtr->width > 0) {
            width = butPtr->width;
        }
        if (butPtr->height > 0) {
            height = butPtr->height;
        }
        if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
            butPtr->indicatorSpace = height;
            if (butPtr->type == TYPE_CHECK_BUTTON) {
                butPtr->indicatorDiameter = (65 * height) / 100;
            } else {
                butPtr->indicatorDiameter = (75 * height) / 100;
            }
        }
    } else if (butPtr->bitmap != None) {
        Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &width, &height);
        goto imageOrBitmap;
    } else {
        int avgWidth;
        Blt_FontMetrics fm;

        if (butPtr->textLayout != NULL) {
            Blt_TkTextLayout_Free(butPtr->textLayout);
        }
        butPtr->textLayout = Blt_TkTextLayout_Compute(butPtr->font,
            butPtr->text, -1, butPtr->wrapLength, butPtr->justify, 0,
            &butPtr->textWidth, &butPtr->textHeight);
        width = butPtr->textWidth;
        height = butPtr->textHeight;
        avgWidth = Blt_TextWidth(butPtr->font, "0", 1);
        Blt_Font_GetMetrics(butPtr->font, &fm);

        if (butPtr->width > 0) {
            width = butPtr->width * avgWidth;
        }
        if (butPtr->height > 0) {
            height = butPtr->height * fm.linespace;
        }
        if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
            butPtr->indicatorDiameter = fm.linespace;
            if (butPtr->type == TYPE_CHECK_BUTTON) {
                butPtr->indicatorDiameter = 
                    (85 * butPtr->indicatorDiameter) / 100;
            }
            butPtr->indicatorSpace = butPtr->indicatorDiameter + avgWidth;
        }
    }

    /*
     * When issuing the geometry request, add extra space for the indicator,
     * if any, and for the border and padding, plus two extra pixels so the
     * display can be offset by 1 pixel in either direction for the raised
     * or lowered effect.
     */

#ifdef notdef
    if ((butPtr->image == NULL) && (butPtr->bitmap == None)) {
        width += 2 * butPtr->padX;
        height += 2 * butPtr->padY;
    }
#else 
    width += 2 * butPtr->padX;
    height += 2 * butPtr->padY;
#endif
    if ((butPtr->type == TYPE_BUTTON) && !Tk_StrictMotif(butPtr->tkwin)) {
        width += 2;
        height += 2;
    }
    Tk_GeometryRequest(butPtr->tkwin, (int)(width + butPtr->indicatorSpace
            + 2 * butPtr->inset), (int)(height + 2 * butPtr->inset));
    Tk_SetInternalBorder(butPtr->tkwin, butPtr->inset);
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonEventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for various
 *      events on buttons.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      When the window gets deleted, internal structures get
 *      cleaned up.  When it gets exposed, it is redisplayed.
 *
 *---------------------------------------------------------------------------
 */

static void
ButtonEventProc(ClientData clientData, XEvent *eventPtr)
{
    Button *butPtr = clientData;
    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
        goto redraw;
    } else if (eventPtr->type == ConfigureNotify) {
        /*
         * Must redraw after size changes, since layout could have changed
         * and borders will need to be redrawn.
         */

        goto redraw;
    } else if (eventPtr->type == DestroyNotify) {
        if (butPtr->tkwin != NULL) {
            butPtr->tkwin = NULL;
            Tcl_DeleteCommandFromToken(butPtr->interp, butPtr->widgetCmd);
        }
        if (butPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayButton, (ClientData)butPtr);
        }
        /* This is a hack to workaround a bug in 8.3.3. */
        DestroyButton((ClientData)butPtr);
        /* Tcl_EventuallyFree((ClientData)butPtr, (Tcl_FreeProc *)Blt_Free); */
    } else if (eventPtr->type == FocusIn) {
        if (eventPtr->xfocus.detail != NotifyInferior) {
            butPtr->flags |= GOT_FOCUS;
            if (butPtr->highlightWidth > 0) {
                goto redraw;
            }
        }
    } else if (eventPtr->type == FocusOut) {
        if (eventPtr->xfocus.detail != NotifyInferior) {
            butPtr->flags &= ~GOT_FOCUS;
            if (butPtr->highlightWidth > 0) {
                goto redraw;
            }
        }
    }
    return;

  redraw:
    EventuallyRedraw(butPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonCmdDeletedProc --
 *
 *      This procedure is invoked when a widget command is deleted.  If
 *      the widget isn't already in the process of being destroyed,
 *      this command destroys it.
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
ButtonCmdDeletedProc(ClientData clientData)
{
    Button *butPtr = clientData;
    Tk_Window tkwin = butPtr->tkwin;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
        butPtr->tkwin = NULL;
        Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * InvokeButton --
 *
 *      This procedure is called to carry out the actions associated
 *      with a button, such as invoking a TCL command or setting a
 *      variable.  This procedure is invoked, for example, when the
 *      button is invoked via the mouse.
 *
 * Results:
 *      A standard TCL return value.  Information is also left in
 *      interp->result.
 *
 * Side effects:
 *      Depends on the button and its associated command.
 *
 *---------------------------------------------------------------------------
 */

static int
InvokeButton(Button *butPtr)
{
    if (butPtr->type == TYPE_PUSH_BUTTON) {
        if (butPtr->flags & SELECTED) {
            if (Tcl_SetVar(butPtr->interp, butPtr->selVarName, butPtr->offValue,
                           TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
                return TCL_ERROR;
            }
        } else {
            const char *string;

            string = (butPtr->value != NULL) ? butPtr->value : butPtr->onValue;
            if (Tcl_SetVar(butPtr->interp, butPtr->selVarName, string,
                    TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
                return TCL_ERROR;
            }
        }
    } else if (butPtr->type == TYPE_CHECK_BUTTON) {
        const char *value;

        value = (butPtr->flags & SELECTED) ? butPtr->offValue : butPtr->onValue;
        if (Tcl_SetVar(butPtr->interp, butPtr->selVarName, value,
                       TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
            return TCL_ERROR;
        }
    } else if (butPtr->type == TYPE_RADIO_BUTTON) {
        if (Tcl_SetVar(butPtr->interp, butPtr->selVarName, butPtr->onValue,
                TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
            return TCL_ERROR;
        }
    }
    if ((butPtr->type != TYPE_LABEL) && (butPtr->cmdObjPtr != NULL)) {
        return Tcl_EvalObjEx(butPtr->interp, butPtr->cmdObjPtr,TCL_EVAL_GLOBAL);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonVarProc --
 *
 *      This procedure is invoked when someone changes the
 *      state variable associated with a radio button.  Depending
 *      on the new value of the button's variable, the button
 *      may be selected or deselected.
 *
 * Results:
 *      NULL is always returned.
 *
 * Side effects:
 *      The button may become selected or deselected.
 *
 *---------------------------------------------------------------------------
 */

 /* ARGSUSED */
static char *
ButtonVarProc(
    ClientData clientData,      /* Information about button. */
    Tcl_Interp *interp,         /* Interpreter containing variable. */
    const char *name1,          /* Name of variable. */
    const char *name2,          /* Second part of variable name. */
    int flags)                  /* Information about what happened. */
{
    Button *butPtr = clientData;
    const char *value, *string;

    /*
     * If the variable is being unset, then just re-establish the
     * trace unless the whole interpreter is going away.
     */

    if (flags & TCL_TRACE_UNSETS) {
        butPtr->flags &= ~SELECTED;
        if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
            Tcl_TraceVar(interp, butPtr->selVarName,
                TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                ButtonVarProc, clientData);
        }
        goto redisplay;
    }
    /*
     * Use the value of the variable to update the selected status of
     * the button.
     */

    value = Tcl_GetVar(interp, butPtr->selVarName, TCL_GLOBAL_ONLY);
    if (value == NULL) {
        value = "";
    }
    string = butPtr->onValue;
    if ((butPtr->type == TYPE_PUSH_BUTTON) && (butPtr->value != NULL)) {
        string = butPtr->value;
    }
    if (strcmp(value, string) == 0) {
        if (butPtr->flags & SELECTED) {
            return NULL;                /* Already selected. */
        }
        butPtr->flags |= SELECTED;
    } else if (butPtr->flags & SELECTED) {
        butPtr->flags &= ~SELECTED;
    } else {
        return NULL;                    /* Already deselected. */
    }
  redisplay:
    EventuallyRedraw(butPtr);
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonTextVarProc --
 *
 *      This procedure is invoked when someone changes the variable
 *      whose contents are to be displayed in a button.
 *
 * Results:
 *      NULL is always returned.
 *
 * Side effects:
 *      The text displayed in the button will change to match the
 *      variable.
 *
 *---------------------------------------------------------------------------
 */

 /* ARGSUSED */
static char *
ButtonTextVarProc(
    ClientData clientData,      /* Information about button. */
    Tcl_Interp *interp,         /* Interpreter containing variable. */
    const char *name1,          /* Not used. */
    const char *name2,          /* Not used. */
    int flags)                  /* Information about what happened. */
{
    Button *butPtr = clientData;
    const char *value;

    /*
     * If the variable is unset, then immediately recreate it unless
     * the whole interpreter is going away.
     */

    if (flags & TCL_TRACE_UNSETS) {
        if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
            Tcl_SetVar(interp, butPtr->textVarName, butPtr->text,
                TCL_GLOBAL_ONLY);
            Tcl_TraceVar(interp, butPtr->textVarName,
                TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                ButtonTextVarProc, clientData);
        }
        return (char *) NULL;
    }
    value = Tcl_GetVar(interp, butPtr->textVarName, TCL_GLOBAL_ONLY);
    if (value == NULL) {
        value = "";
    }
    if (butPtr->text != NULL) {
        Blt_Free(butPtr->text);
    }
    butPtr->text = Blt_AssertStrdup(value);
    ComputeButtonGeometry(butPtr);

    EventuallyRedraw(butPtr);
    return (char *) NULL;
}

int
Blt_ButtonCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpecs[] = {
        {"button", ButtonCmd,},
        {"pushbutton", PushbuttonCmd,},
        {"checkbutton", CheckbuttonCmd,},
        {"radiobutton", RadiobuttonCmd,},
        {"label", LabelCmd,},
    };
    return Blt_InitCmds(interp, "::blt::tk", cmdSpecs, 5);
}

#endif /* NO_TKBUTTON */


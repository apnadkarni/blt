/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltComboEntry.c --
 *
 * This module implements a comboentry widget for the BLT toolkit.
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
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "bltAlloc.h"
#include "bltHash.h"
#include "bltChain.h"
#include "bltFont.h"
#include "bltText.h"
#include "bltImage.h"
#include "bltBg.h"
#include "bltPicture.h"
#include "bltPainter.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define CharIndexToByteOffset(s, n)     (Tcl_UtfAtIndex(s, n) - s)

#define FCLAMP(x)       ((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))
#define IPAD            4               /* Internal pad between components. */
#define XPAD            1
#define YPAD            1               /* Internal pad between
                                         * components. */
#define ICWIDTH         2               /* External pad between border and
                                         * arrow. */
#define ARROW_HEIGHT    13

#define EVENT_MASK       (ExposureMask|StructureNotifyMask|FocusChangeMask)
#define CHILD_EVENT_MASK (ExposureMask|StructureNotifyMask)

#define REDRAW_PENDING   (1<<0)         /* The widget is scheduled to be
                                         * redrawn. */
#define LAYOUT_PENDING   (1<<1)         /* The widget's layout needs to be
                                         * recomputed. */
#define ICURSOR          (1<<2)         /* Insertion cursor is active.
                                         * Depending upon the timer
                                         * interval, it may be drawn or not
                                         * drawn. */
#define SCROLL_PENDING   (1<<3)         /* The widget needs to be
                                           scrolled. */
#define FOCUS            (1<<4)         /* The widget has focus. */
#define SELECT_PENDING   (1<<5)         /* The widget is scheduled to
                                         * invoke a -selectcommand in
                                         * response to a change in its
                                         * selection. */
#define INVOKE_PENDING   (1<<6)         /* The widget is scheduled to
                                         * invoke a -command. */
#define READONLY         (1<<8)         /* The widget's editing functions
                                         * are disabled. */
#define EXPORT_SELECTION (1<<9)         /* The selection is exported to the
                                         * clipboard. */
#define OWN_SELECTION    (1<<10)        /* The widget owns the selection. */

#define DISABLED         (1<<11)        /* The widget is is disabled. */
#define POSTED           (1<<12)        /* The widget has posted a menu. */
#define STATE_MASK       ((DISABLED)|(POSTED))

#define ICURSOR_ON       (1<<13)        /* The insertion cursor is
                                         * currently visible on screen. */
#define ARROW            (1<<14)        /* Display the arrow button on the
                                         * far right.*/
#define XBUTTON          (1<<15)        /* Display the x button on the
                                         * right when text has been
                                         * entered. */
#define ACTIVE_ARROW     (1<<16)        /* The arrow button is currently
                                           active. */
#define ACTIVE_BUTTON    (1<<17)        /* The x button is currently
                                         * active. */
#define ACTIVE_MASK      ((ACTIVE_ARROW)|(ACTIVE_BUTTON))

#define MODIFIED         (1<<18)        /* The contents of the text of the
                                         * entry have been modified. */
#define TRACE_VAR_FLAGS (TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS)


#define DEF_ARROW_ACTIVE_BG     STD_ACTIVE_BACKGROUND
#define DEF_ARROW_ACTIVE_FG     STD_ACTIVE_FOREGROUND
#define DEF_ARROW_ACTIVE_RELIEF "raised"
#define DEF_ARROW_BORDERWIDTH   "2"
#define DEF_ARROW_DISABLED_BG   STD_DISABLED_BACKGROUND
#define DEF_ARROW_DISABLED_FG   STD_DISABLED_FOREGROUND
#define DEF_ARROW_NORMAL_BG     STD_NORMAL_BACKGROUND
#define DEF_ARROW_NORMAL_FG     STD_NORMAL_FOREGROUND
#define DEF_ARROW_POSTED_BG     STD_NORMAL_BACKGROUND
#define DEF_ARROW_POSTED_FG     STD_NORMAL_FOREGROUND
#define DEF_ARROW_PAD           "0"
#define DEF_ARROW_RELIEF        "raised"
#define DEF_ARROW_WIDTH         "0"
#define DEF_BORDERWIDTH         "0"
#define DEF_XBUTTON             "0"
#define DEF_CMD                 ((char *)NULL)
#define DEF_HIDE_ARROW          "0"
#define DEF_CURSOR              ((char *)NULL)
#define DEF_DISABLED_BG         STD_DISABLED_BACKGROUND
#define DEF_DISABLED_FG         STD_DISABLED_FOREGROUND
#define DEF_EDITABLE            "1"
#define DEF_EXPORTSELECTION     "1"
#define DEF_FONT                STD_FONT_NORMAL
#define DEF_HEIGHT              "0"
#define DEF_HIGHLIGHT_BG_COLOR  ((char *)NULL)
#define DEF_HIGHLIGHT_COLOR     "black"
#define DEF_HIGHLIGHT_WIDTH     "2"
#define DEF_ICON                ((char *)NULL)
#define DEF_ICON_VARIABLE       ((char *)NULL)
#define DEF_IMAGE               ((char *)NULL)
#define DEF_INSERT_COLOR        STD_NORMAL_FOREGROUND
#define DEF_INSERT_OFFTIME      "300"
#define DEF_INSERT_ONTIME       "600"
#define DEF_JUSTIFY             "left"
#define DEF_MENU                ((char *)NULL)
#define DEF_MENU_ANCHOR         "sw"
#define DEF_NORMAL_BG           STD_NORMAL_BACKGROUND
#define DEF_NORMAL_FG           STD_NORMAL_FOREGROUND
#define DEF_RELIEF              "sunken"
#define DEF_SCROLL_CMD          ((char *)NULL)
#define DEF_SCROLL_INCR         "2"
#define DEF_SELECT_BG           RGB_SKYBLUE4
#define DEF_SELECT_BORDERWIDTH  "0"
#define DEF_SELECT_CMD          ((char *)NULL)
#define DEF_SELECT_FG           RGB_WHITE
#define DEF_SELECT_RELIEF       "flat"
#define DEF_SHOW                (char *)NULL
#define DEF_STATE               "normal"
#define DEF_TAKE_FOCUS          "1"
#define DEF_TEXT                (char *)NULL
#define DEF_TEXT_FOCUS_BG       RGB_WHITE
#define DEF_TEXT_FOCUS_FG       RGB_BLACK
#define DEF_TEXT_NORMAL_BG      RGB_WHITE
#define DEF_TEXT_NORMAL_FG      RGB_BLACK
#define DEF_TEXT_VARIABLE       ((char *)NULL)
#define DEF_UNDERLINE           "-1"
#define DEF_WIDTH               "0"

#define DEF_BUTTON_ACTIVEBACKGROUND     RGB_RED
#define DEF_BUTTON_ACTIVEFOREGROUND     RGB_WHITE
#define DEF_BUTTON_ACTIVERELIEF         "raised"
#define DEF_BUTTON_BACKGROUND           RGB_LIGHTBLUE0
#define DEF_BUTTON_BORDERWIDTH          "1"
#define DEF_BUTTON_COMMAND              (char *)NULL
#define DEF_BUTTON_FOREGROUND           RGB_LIGHTBLUE3
#define DEF_BUTTON_RELIEF               "flat"

static Tcl_VarTraceProc TextVarTraceProc;
static Tcl_VarTraceProc IconVarTraceProc;

static Blt_OptionFreeProc FreeTextProc;
static Blt_OptionParseProc ObjToText;
static Blt_OptionPrintProc TextToObj;
static Blt_CustomOption textOption = {
    ObjToText, TextToObj, FreeTextProc, (ClientData)0
};

static Blt_OptionFreeProc FreeIconProc;
static Blt_OptionParseProc ObjToIcon;
static Blt_OptionPrintProc IconToObj;
static Blt_CustomOption iconOption = {
    ObjToIcon, IconToObj, FreeIconProc, (ClientData)0
};

static Blt_OptionFreeProc FreeTextVarProc;
static Blt_OptionParseProc ObjToTextVar;
static Blt_OptionPrintProc TextVarToObj;
static Blt_CustomOption textVarOption = {
    ObjToTextVar, TextVarToObj, FreeTextVarProc, (ClientData)0
};
static Blt_OptionFreeProc FreeIconVarProc;
static Blt_OptionParseProc ObjToIconVar;
static Blt_OptionPrintProc IconVarToObj;
static Blt_CustomOption iconVarOption = {
    ObjToIconVar, IconVarToObj, FreeIconVarProc, (ClientData)0
};

static Blt_OptionParseProc ObjToState;
static Blt_OptionPrintProc StateToObj;
static Blt_CustomOption stateOption = {
    ObjToState, StateToObj, NULL, (ClientData)0
};

/*
 * Icon --
 *
 *      Since instances of the same Tk image can be displayed in different
 *      windows with possibly different color palettes, Tk internally
 *      stores each instance in a linked list.  But if the instances are
 *      used in the same widget and therefore use the same color palette,
 *      this adds a lot of overhead, especially when deleting instances
 *      from the linked list.
 *
 *      For the comboentry widget, we never need more than a single
 *      instance of an image, regardless of how many times it's used.
 *      Cache the image, maintaining a reference count for each image used
 *      in the widget.  It's likely that the comboview widget will use many
 *      instances of the same image.
 */

typedef struct _Icon {
    Tk_Image tkImage;                   /* Tk image being cached. */
    short int width, height;            /* Dimensions of the cached image. */
} *Icon;

#define IconHeight(i)   ((i)->height)
#define IconWidth(i)    ((i)->width)
#define IconImage(i)    ((i)->tkImage)
#define IconName(i)     (Blt_Image_Name((i)->tkImage))

#define INSERT_OP       1
#define DELETE_OP       2

/*
 * XButton --
 */
typedef struct {
    int borderWidth;                    /* Width of 3D border around the
                                         * tab's button. */
    Blt_Pad padX;                       /* Extra padding around button. */
    Blt_Pad padY;                       /* Extra padding around button. */
    int activeRelief;                   /* 3D relief when the button is
                                         * active. */
    int relief;                         /* 3D relief of button. */
    XColor *normalFgColor;              /* Color or the button's X symbol. */
    XColor *normalBgColor;              /* Background color of the button. */
    XColor *activeFgColor;              /* Color or the button's X symbol
                                         * when the button is active. */
    XColor *activeBgColor;              /* Background color of the button
                                         * when the button is active. */
    Tcl_Obj *cmdObjPtr;                 /* Command to be executed when the
                                         * the button is invoked. */
    Blt_Picture normalPicture;          /* If non-NULL, image to be
                                         * displayed when button is
                                         * displayed. */
    Blt_Picture activePicture;          /* If non-NULL, image to be
                                         * displayed when the button is
                                         * active. */
    short int x, y;                     /* Location of the button in the
                                         * entry. Used for picking. */
    short int width, height;            /* Dimension of the button. */
} XButton;


static Blt_ConfigSpec xButtonSpecs[] =
{
    {BLT_CONFIG_COLOR, "-activebackground", "activeBackrgound", 
        "ActiveBackground", DEF_BUTTON_ACTIVEBACKGROUND, 
        Blt_Offset(XButton, activeBgColor), 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForergound", 
        "ActiveForeground", DEF_BUTTON_ACTIVEFOREGROUND, 
        Blt_Offset(XButton, activeFgColor), 0},
    {BLT_CONFIG_COLOR, "-background", "backrgound", "Background", 
        DEF_BUTTON_BACKGROUND, Blt_Offset(XButton, normalBgColor), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "ActiveRelief",
        DEF_BUTTON_ACTIVERELIEF, Blt_Offset(XButton, activeRelief), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth"},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_BUTTON_BORDERWIDTH, Blt_Offset(XButton, borderWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", DEF_BUTTON_COMMAND, 
        Blt_Offset(XButton, cmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-foreground", "forergound", "Foreground", 
        DEF_BUTTON_FOREGROUND, Blt_Offset(XButton, normalFgColor), 0},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_BUTTON_RELIEF, 
        Blt_Offset(XButton, relief), 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
};

typedef int CharIndex;                  /* Character index regardless of
                                         * how many bytes (UTF) are used. */
typedef int ByteOffset;                 /* Offset in bytes from the start of
                                         * the text string.  This may be
                                         * different between the normal text
                                         * and the screen text if -show is
                                         * used. */

static char emptyString[] = "";

typedef struct _EditRecord {
    struct _EditRecord *nextPtr;
    int type;
    CharIndex insertIndex;              /* Current index of the cursor. */
    CharIndex index;                    /* Character index where text was
                                           inserted. */
    int numBytes;                       /* # of bytes in text string. */
    int numChars;                       /* # of characters in text string. */
    char text[1];
} EditRecord;

typedef struct  {
    Tcl_Interp *interp;                 /* Interpreter associated with
                                         * entry. */
    Tk_Window tkwin;                    /* Window that embodies the
                                         * comboentry. If NULL, indicates the
                                         * window has been destroyed but the
                                         * data structures haven't yet been
                                         * cleaned up.*/
    Display *display;                   /* Display containing widget.  Used,
                                         * among other things, so that
                                         * resources can be freed even after
                                         * tkwin has gone away. */
    Blt_Painter painter;
    Tcl_Command cmdToken;               /* Token for comboentry's widget
                                         * command. */
    Tk_Cursor cursor;                   /* Current cursor for window or
                                         * None. */
    int reqWidth, reqHeight;     
    int relief;
    int borderWidth;

    Blt_Bg inFocusBg;
    Blt_Bg outFocusBg;
    Blt_Bg normalBg;                    /* Background color of the text
                                         * area. */
    Blt_Bg disabledBg;                  /* Background color or the text
                                         * area when the widget is
                                         * disabled.  */
    XColor *disabledFgColor;            /* Color of the text when the widget
                                         * is disabled. */
    XColor *normalFgColor;              /* Color of the text. */

    Tcl_Obj *takeFocusObjPtr;           /* Value of -takefocus option; not
                                         * used in the C code, but used by
                                         * keyboard traversal scripts. */

    /*
     * Selection Information:
     *
     * The selection is the rectangle that contains selected text.  It is
     * displayed as a solid colored entry with optionally a 3D border.
     */
    CharIndex selAnchor;                /* Character index representing the
                                         * fixed end of selection. Used to
                                         * extend the selection while
                                         * maintaining the other end of the
                                         * selection. */
    CharIndex selFirst;                 /* Character index of the first
                                         * character in the selection. */
    CharIndex selLast;                  /* Character Index of the last
                                         * character in the selection. */
    int selRelief;                      /* Relief of selected
                                         * items. Currently is always
                                         * raised. */
    int selBW;                          /* Border width of a selected
                                         * text.*/
    XColor *selFgColor;                 /* Color of the selected text. */
    GC selectGC;
    Tcl_Obj *selCmdObjPtr;

    Blt_Bg selectBg;

    XButton xButton;

    /*
     * Scanning Information:
     */
    int scanAnchor;                     /* Scan anchor in screen
                                         * coordinates. */
    int scanX;                          /* x-offset of the start of the
                                         * scan in world coordinates.*/
    /*
     * Scrolling Information:
     */
    Tcl_Obj *scrollCmdObjPtr;           /* If non-NULL, command prefix for
                                         * communicating with horizontal
                                         * scrollbar. */
    int scrollUnits;                    /* # of pixels per scroll unit. */
    int scrollX;                        /* x-offset of the start of visible
                                         * text in the viewport. */
    int viewWidth;                      /* Width of the viewport. */
    /*
     * In/Out Focus Highlight Ring:
     */
    XColor *highlightColor;
    XColor *highlightBgColor;
    GC highlightBgGC;
    GC highlightGC;
    int highlightWidth;

    /* 
     * Entry entry:
     *
     * The entry contains optionally an icon and a text string. The
     * rectangle surrounding an entry may have a 3D border.
     */
    Icon icon;                          /* If non-NULL, image to be
                                         * displayed in entry. Its value
                                         * may be overridden by the
                                         * -iconvariable option. */
    Tcl_Obj *iconVarObjPtr;             /* Name of TCL variable.  If
                                         * non-NULL, this variable contains
                                         * the name of an image
                                         * representing the icon.  This
                                         * overrides the value of the above
                                         * field. */
    Icon image;                         /* If non-NULL, image to be
                                         * displayed instead of text in the
                                         * entry. */
    char *text;                         /* Text string to be displayed in
                                         * the entry if an image has no
                                         * been designated. Its value is
                                         * overridden by the -textvariable
                                         * option. */
    char *screenText;                   /* Text string to be displayed on
                                         * the screen.  If the -show option
                                         * is used this string may consist
                                         * of different characters from the
                                         * above string.*/
    Tcl_Obj *textVarObjPtr;             /* Name of TCL variable.  If
                                         * non-NULL, this variable contains
                                         * the text string to * be
                                         * displayed in the entry. This
                                         * overrides the above field. */
    Blt_Font font;                      /* Font of text to be display in
                                         * entry. */
    XColor *textInFocusColor;
    XColor *textOutFocusColor;
    GC textInFocusGC;
    GC textOutFocusGC;

    short int numChars;                 /* # character in text string. */
    short int numBytes;                 /* # bytes of in actual text
                                         * string. */
    short int numScreenBytes;           /* # bytes in displayed text. */

    /*  
     * Arrow Information:
     */
    int arrowBorderWidth;
    int arrowRelief;
    int arrowActiveRelief;
    int reqArrowWidth;
    int arrowPad;
    short int arrowWidth, arrowHeight;
    short int arrowX, arrowY;
    XColor *arrowActiveFgColor;         /* Color of the arrow symbol and
                                         * outline rectangle when it is
                                         * active. */
    XColor *arrowDisabledFgColor;       /* Color of the arrow symbol and
                                         * outline rectangle when it is
                                         * disabled. */
    XColor *arrowNormalFgColor;         /* Color of the arrow symbol and
                                         * outline rectangle. */
    XColor *arrowPostedFgColor;         /* Color of the arrow symbol and
                                         * outline rectangle. */
    Blt_Bg arrowActiveBg;               /* Fill color or the arrow
                                         * button when it is active. */
    Blt_Bg arrowDisabledBg;             /* Fill color or the arrow
                                         * button when it is disabled. */
    Blt_Bg arrowNormalBg;               /* Fill color or the arrow
                                         * button */
    Blt_Bg arrowPostedBg;               /* Fill color or the arrow
                                         * button */
    Blt_Picture disabledArrow;
    Blt_Picture postedArrow;
    Blt_Picture activeArrow;
    Blt_Picture normalArrow;

    /*
     * Insertion cursor information:
     */
    GC insertGC;
    XColor *insertColor;                /* Color used to draw vertical bar
                                         * for insertion cursor. */
    int insertOffTime;                  /* Time in milliseconds cursor
                                         * should spend in "off" state for
                                         * each blink. */
    int insertOnTime;                   /* Time in milliseconds cursor
                                         * should spend in "off" state for
                                         * each blink. */
    Tcl_TimerToken insertTimerToken;    /* Handle for a timer event called
                                         * periodically to blink the
                                         * insertion cursor. */
    int insertWidth;                    /* Total width of insert cursor. */
    CharIndex insertIndex;              /* Character index of the insertion
                                         * cursor.  */
    int prefTextWidth;                  /* Desired width of text, measured
                                         * in average characters. */
    int prefIconWidth;                  /* Desired width of icon, measured
                                         * in pixels. */
    int inset;
    short int iconWidth, iconHeight;
    short int entryWidth, entryHeight;
    short int textWidth, textHeight;
    short int width, height;


    ByteOffset firstOffset, lastOffset; /* Byte offset of first and last
                                         * characters visible in
                                         * viewport. */
    int firstX, lastX;                  /* x-coordinates of first and last
                                         * characters visible in
                                         * viewport. */ 
    Tcl_Obj *cmdObjPtr;                 /* If non-NULL, command to be
                                         * executed when this menu is
                                         * posted. */
    Tcl_Obj *menuObjPtr;        
    Tk_Window menuWin;
    Tcl_Obj *postCmdObjPtr;             /* If non-NULL, command to be
                                         * executed when this menu is
                                         * posted. */
    unsigned int flags;
    EditRecord *undoPtr, *redoPtr;
    const char *cipher;                 /* If non-NULL, this is the
                                         * character to display for every
                                         * character of text. */
} ComboEntry;

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_RELIEF, "-activearrowrelief", "activeArrowRelief",
        "ActiveArrowRelief",  DEF_ARROW_ACTIVE_RELIEF,
        Blt_Offset(ComboEntry, arrowActiveRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-activearrowbackground", "activeArrowBackground", 
        "ActiveArrowBackground", DEF_ARROW_ACTIVE_BG, 
        Blt_Offset(ComboEntry, arrowActiveBg), 0},
    {BLT_CONFIG_COLOR, "-activearrowforeground", "activeArrowForeground", 
        "ActiveArrowForeground", DEF_ARROW_ACTIVE_FG, 
        Blt_Offset(ComboEntry, arrowActiveFgColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-arrowborderwidth", "arrowBorderWidth", 
        "ArrowBorderWidth", DEF_ARROW_BORDERWIDTH, 
        Blt_Offset(ComboEntry, arrowBorderWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_BACKGROUND, "-arrowbackground", "arrowBackground", 
        "arrowBackground", DEF_ARROW_NORMAL_BG, 
        Blt_Offset(ComboEntry, arrowNormalBg), 0},
    {BLT_CONFIG_COLOR, "-arrowforeground", "arrowForeground", "arrowForeground",
        DEF_ARROW_NORMAL_FG, Blt_Offset(ComboEntry, arrowNormalFgColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-arrowpad", "arrowPad", "ArrowPad", 
        DEF_ARROW_PAD, Blt_Offset(ComboEntry, arrowPad), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-arrowrelief", "arrowRelief","ArrowRelief",
        DEF_ARROW_RELIEF, Blt_Offset(ComboEntry, arrowRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-arrowwidth", "arrowWidth","ArrowWidth",
        DEF_ARROW_WIDTH, Blt_Offset(ComboEntry, reqArrowWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background", 
        DEF_NORMAL_BG, Blt_Offset(ComboEntry, normalBg), 0 },
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth"},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_BORDERWIDTH, Blt_Offset(ComboEntry, borderWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", 
        DEF_CMD, Blt_Offset(ComboEntry, cmdObjPtr), 
        BLT_CONFIG_NULL_OK, },
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
        DEF_CURSOR, Blt_Offset(ComboEntry, cursor), 
        BLT_CONFIG_NULL_OK, },
    {BLT_CONFIG_BACKGROUND, "-disabledarrowbackground",
        "disabledArrowBackground", "DisabledArrowBackground",
        DEF_ARROW_DISABLED_BG, Blt_Offset(ComboEntry, arrowDisabledBg), 0, },
    {BLT_CONFIG_COLOR, "-disabledarrowforeground", "disabledArrowForeground",
        "DisabledArrowForeground", DEF_ARROW_DISABLED_FG, 
        Blt_Offset(ComboEntry, arrowDisabledFgColor), 0, },
    {BLT_CONFIG_BACKGROUND, "-disabledbackground", "disabledBackground", 
        "DisabledBackground", DEF_DISABLED_BG, 
        Blt_Offset(ComboEntry, disabledBg), 0, },
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground",
        "DisabledForeground", DEF_DISABLED_FG, 
        Blt_Offset(ComboEntry, disabledFgColor), 0, },
    {BLT_CONFIG_BITMASK_INVERT, "-editable", "editable", "Editable", 
        DEF_EDITABLE, Blt_Offset(ComboEntry, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)READONLY},
    {BLT_CONFIG_BITMASK, "-exportselection", "exportSelection", 
        "ExportSelection", DEF_EXPORTSELECTION, Blt_Offset(ComboEntry, flags),
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)EXPORT_SELECTION},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground"},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_FONT,
        Blt_Offset(ComboEntry, font), 0, },
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground", 
        DEF_NORMAL_FG, Blt_Offset(ComboEntry, normalFgColor), 0, },
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height", DEF_HEIGHT, 
        Blt_Offset(ComboEntry, reqHeight), 
        BLT_CONFIG_DONT_SET_DEFAULT, },
    {BLT_CONFIG_BITMASK_INVERT, "-hidearrow", "hideArrow", "HideArrow", 
        DEF_HIDE_ARROW, Blt_Offset(ComboEntry, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)ARROW},
    {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground", 
        "HighlightBackground", DEF_HIGHLIGHT_BG_COLOR, 
        Blt_Offset(ComboEntry, highlightBgColor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
        DEF_HIGHLIGHT_COLOR, Blt_Offset(ComboEntry, highlightColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
        "HighlightThickness", DEF_HIGHLIGHT_WIDTH, 
        Blt_Offset(ComboEntry, highlightWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", DEF_ICON, 
        Blt_Offset(ComboEntry, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_CUSTOM, "-iconvariable", "iconVariable", "IconVariable", 
        DEF_TEXT_VARIABLE, Blt_Offset(ComboEntry, iconVarObjPtr), 
        BLT_CONFIG_NULL_OK, &iconVarOption},
    {BLT_CONFIG_PIXELS_NNEG, "-iconwidth", "iconWidth", "IconWidth",
        DEF_WIDTH, Blt_Offset(ComboEntry, prefIconWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-image", "image", "Image", DEF_IMAGE, 
        Blt_Offset(ComboEntry, image), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_COLOR, "-insertcolor", "insertColor", "InsertColor",
        DEF_INSERT_COLOR, Blt_Offset(ComboEntry, insertColor), 0},
    {BLT_CONFIG_INT, "-insertofftime", "insertOffTime", "OffTime",
        DEF_INSERT_OFFTIME, Blt_Offset(ComboEntry, insertOffTime), 
        BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_INT, "-insertontime", "insertOnTime", "OnTime",
        DEF_INSERT_ONTIME, Blt_Offset(ComboEntry, insertOnTime), 
        BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_OBJ, "-menu", "menu", "Menu", DEF_MENU, 
        Blt_Offset(ComboEntry, menuObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-postcommand", "postCommand", "PostCommand", 
        DEF_CMD, Blt_Offset(ComboEntry, postCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-postedarrowbackground", "postedArrowBackground", 
        "PostedArrowBackground", DEF_ARROW_POSTED_BG, 
        Blt_Offset(ComboEntry, arrowPostedBg), 0},
    {BLT_CONFIG_COLOR, "-postedarrowforeground", "postedArrowForeground", 
        "PostedArrowForeground", DEF_ARROW_POSTED_FG, 
        Blt_Offset(ComboEntry, arrowPostedFgColor), 0},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_RELIEF, 
        Blt_Offset(ComboEntry, relief), 0},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
        "Foreground", DEF_SELECT_BG, Blt_Offset(ComboEntry, selectBg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-selectborderwidth", "selectBorderWidth", 
        "BorderWidth", DEF_SELECT_BORDERWIDTH, Blt_Offset(ComboEntry, selBW), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-selectcommand", "selectCommand", "SelectCommand",
        DEF_SELECT_CMD, Blt_Offset(ComboEntry, selCmdObjPtr),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
        DEF_SELECT_FG, Blt_Offset(ComboEntry, selFgColor), 0},
    {BLT_CONFIG_RELIEF, "-selectrelief", "selectRelief", "Relief",
        DEF_SELECT_RELIEF, Blt_Offset(ComboEntry, selRelief),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-show", "show", "Show", DEF_SHOW, 
        Blt_Offset(ComboEntry, cipher), 
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_STATE, 
        Blt_Offset(ComboEntry, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        &stateOption},
    {BLT_CONFIG_OBJ, "-takefocus", "takeFocus", "TakeFocus", DEF_TAKE_FOCUS, 
        Blt_Offset(ComboEntry, takeFocusObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-text", "text", "Text", DEF_TEXT, 
        Blt_Offset(ComboEntry, text), 0, &textOption},
    {BLT_CONFIG_BACKGROUND, "-textbackground", "textBackground", "Background", 
        DEF_TEXT_NORMAL_BG, Blt_Offset(ComboEntry, outFocusBg), 0},
    {BLT_CONFIG_BACKGROUND, "-textfocusbackground", "textFocusBackground",
        "FocusBackground", DEF_TEXT_FOCUS_BG, 
        Blt_Offset(ComboEntry, inFocusBg), 0},
    {BLT_CONFIG_COLOR, "-textfocusforeground", "textFocusForeground",
        "focusForeground", DEF_TEXT_FOCUS_FG, 
        Blt_Offset(ComboEntry, textInFocusColor), 0},
    {BLT_CONFIG_COLOR, "-textforeground", "textForeground", "TextForeground",
        DEF_TEXT_NORMAL_FG, Blt_Offset(ComboEntry, textOutFocusColor), 0},
    {BLT_CONFIG_CUSTOM, "-textvariable", "textVariable", "TextVariable", 
        DEF_TEXT_VARIABLE, Blt_Offset(ComboEntry, textVarObjPtr), 
        BLT_CONFIG_NULL_OK, &textVarOption},
    {BLT_CONFIG_PIXELS_NNEG, "-textwidth", "textWidth", "TextWidth",
        DEF_WIDTH, Blt_Offset(ComboEntry, prefTextWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width",
        DEF_WIDTH, Blt_Offset(ComboEntry, reqWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-xbutton", "xButton", "xButton", 
        DEF_XBUTTON, Blt_Offset(ComboEntry, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)XBUTTON},
    {BLT_CONFIG_OBJ, "-xbuttoncommand", "xButtonCommand", "XButtonCommand", 
        DEF_BUTTON_COMMAND, Blt_Offset(ComboEntry, xButton.cmdObjPtr), 
        BLT_CONFIG_NULL_OK },
    {BLT_CONFIG_OBJ, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
        DEF_SCROLL_CMD, Blt_Offset(ComboEntry, scrollCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_POS, "-xscrollincrement", "xScrollIncrement",
        "ScrollIncrement", DEF_SCROLL_INCR, Blt_Offset(ComboEntry, scrollUnits),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
        0, 0}
};

static Tcl_CmdDeleteProc ComboEntryInstCmdDeletedProc;
static Tcl_FreeProc FreeComboEntryProc;
static Tcl_IdleProc ComboEntryInvokeCmdProc;
static Tcl_IdleProc ComboEntrySelectCmdProc;
static Tcl_IdleProc DisplayComboEntry;
static Tcl_ObjCmdProc ComboEntryInstCmdProc;
static Tcl_TimerProc BlinkCursorTimerProc;
static Tk_EventProc ComboEntryEventProc;
static Tk_LostSelProc ComboEntryLostSelProc;
static Tk_SelectionProc ComboEntrySelectionProc;

typedef int (ComboEntryCmdProc)(ComboEntry *comboPtr, Tcl_Interp *interp, 
        int objc, Tcl_Obj *const *objv);

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *      Tells the Tk dispatcher to call the comboentry display routine at
 *      the next idle point.  This request is made only if the window is
 *      displayed and no other redraw request is pending.
 *
 * Results: None.
 *
 * Side effects:
 *      The window is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyRedraw(ComboEntry *comboPtr) 
{
    if ((comboPtr->tkwin != NULL) && ((comboPtr->flags & REDRAW_PENDING)==0)) {
        comboPtr->flags |= REDRAW_PENDING;
        Tcl_DoWhenIdle(DisplayComboEntry, comboPtr);
    }
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
EventuallyInvokeSelectCmd(ComboEntry *comboPtr) 
{
    if ((comboPtr->flags & SELECT_PENDING) == 0) {
        comboPtr->flags |= SELECT_PENDING;
        Tcl_DoWhenIdle(ComboEntrySelectCmdProc, comboPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyInvokeCmd --
 *
 *      Queues a request to execute the -command code associated with
 *      the widget at the next idle point.  Invoked whenever the text
 *      changes via its text variable.
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
EventuallyInvokeCmd(ComboEntry *comboPtr) 
{
    if ((comboPtr->flags & INVOKE_PENDING) == 0) {
        comboPtr->flags |= INVOKE_PENDING;
        Tcl_DoWhenIdle(ComboEntryInvokeCmdProc, comboPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateModifiedEvent --
 *
 *      Send an event that the text was modified. This is equivalent to
 *         event generate $textWidget <<Modified>>
 *
 * Results:
 *      None
 *
 * Side effects:
 *      May force the text window into existence.
 *
 *----------------------------------------------------------------------
 */
static void
GenerateModifiedEvent(ComboEntry *comboPtr)
{
    XVirtualEvent event;

    Tk_MakeWindowExist(comboPtr->tkwin);
    memset(&event, 0, sizeof(event));
    event.type = VirtualEvent;
    event.serial = NextRequest(comboPtr->display);
    event.send_event = False;
    event.event = Tk_WindowId(comboPtr->tkwin);
    event.display = comboPtr->display;
    event.name = Tk_GetUid("Modified");
    Tk_HandleEvent((XEvent *)&event);
    comboPtr->flags &= ~MODIFIED;
}

static int
InvokeCommand(Tcl_Interp *interp, ComboEntry *comboPtr) 
{
    int result;

    Tcl_Preserve(comboPtr);
    Tcl_IncrRefCount(comboPtr->cmdObjPtr);
    result = Tcl_EvalObjEx(interp, comboPtr->cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(comboPtr->cmdObjPtr);
    Tcl_Release(comboPtr);
    return result;
}

static int
UpdateTextVariable(Tcl_Interp *interp, ComboEntry *comboPtr) 
{
    Tcl_Obj *resultObjPtr, *objPtr;
    const char *varName;

    objPtr = Tcl_NewStringObj(comboPtr->text, comboPtr->numBytes);
    varName = Tcl_GetString(comboPtr->textVarObjPtr); 
    Tcl_UntraceVar(interp, varName, TRACE_VAR_FLAGS, TextVarTraceProc,comboPtr);
    Tcl_IncrRefCount(objPtr);
    resultObjPtr = Tcl_ObjSetVar2(interp, comboPtr->textVarObjPtr, NULL, 
        objPtr, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    Tcl_DecrRefCount(objPtr);
    Tcl_TraceVar(interp, varName, TRACE_VAR_FLAGS, TextVarTraceProc, comboPtr);
    if (resultObjPtr == NULL) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static void
FreeUndoRecords(ComboEntry *comboPtr)
{
    EditRecord *recPtr, *nextPtr;

    for (recPtr = comboPtr->undoPtr; recPtr != NULL; recPtr = nextPtr) {
        nextPtr = recPtr->nextPtr;
        Blt_Free(recPtr);
    }
    comboPtr->undoPtr = NULL;
}

static void
FreeRedoRecords(ComboEntry *comboPtr)
{
    EditRecord *recPtr, *nextPtr;

    for (recPtr = comboPtr->redoPtr; recPtr != NULL; recPtr = nextPtr) {
        nextPtr = recPtr->nextPtr;
        Blt_Free(recPtr);
    }
    comboPtr->redoPtr = NULL;
}

static void
RecordEdit(ComboEntry *comboPtr, int type, int index, const char *text, 
           int numBytes)
{
    EditRecord *recPtr;

    recPtr = Blt_AssertMalloc(sizeof(EditRecord) + numBytes);
    recPtr->type = type;
    recPtr->insertIndex = comboPtr->insertIndex;
    recPtr->index = index;
    recPtr->numChars = Tcl_NumUtfChars(text, numBytes);
    recPtr->numBytes = numBytes;
    memcpy(recPtr->text, text, numBytes);
    recPtr->nextPtr = comboPtr->undoPtr;
    comboPtr->undoPtr = recPtr;
}

static void
CleanText(ComboEntry *comboPtr)
{
    if (comboPtr->screenText != NULL) {
        Blt_Free(comboPtr->screenText);
    }
    if (comboPtr->cipher != NULL) {
        int i, charSize;
        Tcl_UniChar dummy;
        char *p;
    
        charSize = Tcl_UtfToUniChar(comboPtr->cipher, &dummy);
        comboPtr->numScreenBytes = comboPtr->numChars * charSize;
        comboPtr->screenText = Blt_AssertMalloc(comboPtr->numScreenBytes + 1);
        for (p = comboPtr->screenText, i = 0; i < comboPtr->numChars; 
             i++, p += charSize){
            strncpy(p, comboPtr->cipher, charSize);
        }
        comboPtr->screenText[comboPtr->numScreenBytes] = '\0';
    } else {
        char *p, *q, *pend;

        comboPtr->numScreenBytes = comboPtr->numBytes;
        comboPtr->screenText = Blt_AssertMalloc(comboPtr->numScreenBytes + 1);
        for (p = comboPtr->text, q = comboPtr->screenText, 
                 pend = p + comboPtr->numBytes; p < pend; p++, q++) {
            if ((*p == '\n') || (*p == '\t')) {
                *q = ' ';
            } else {
                *q = *p;
            }
        }
    }
} 

static void
DeleteText(ComboEntry *comboPtr, CharIndex firstIndex, CharIndex lastIndex)
{
    ByteOffset first, last;
    int i, j;

    /* Kill the selection */
    comboPtr->selFirst = comboPtr->selLast = -1;
    /* Fix the insertion cursor index if necessary. */
    if (comboPtr->insertIndex >= firstIndex) {
        if (comboPtr->insertIndex >= lastIndex) {
            comboPtr->insertIndex -= (lastIndex - firstIndex);
        } else {
            comboPtr->insertIndex = firstIndex;
        }
    }
    comboPtr->numChars -= lastIndex - firstIndex;
    /* Remove the requested character range from the actual text */
    first = CharIndexToByteOffset(comboPtr->text, firstIndex);
    last  = CharIndexToByteOffset(comboPtr->text, lastIndex);
    for (i = first, j = last; j < comboPtr->numBytes; i++, j++) {
        comboPtr->text[i] = comboPtr->text[j];
    }
    comboPtr->numBytes -= last - first;
    comboPtr->text[comboPtr->numBytes] = '\0';
    CleanText(comboPtr);
    if (comboPtr->textVarObjPtr != NULL) {
        UpdateTextVariable(comboPtr->interp, comboPtr);
    }
    comboPtr->flags |= (MODIFIED | LAYOUT_PENDING | SCROLL_PENDING);
}

static int
InsertText(ComboEntry *comboPtr, CharIndex index, int numBytes, 
           const char *insertText)
{
    char *text;
    ByteOffset offset;
    int numChars;

    /* Create a larger buffer to hold the text. */
    text = Blt_Malloc(comboPtr->numBytes + numBytes);
    if (text == NULL) {
        return TCL_ERROR;
    }
    numChars = Tcl_NumUtfChars(insertText, numBytes);
    /* Copy the old + extra to the new text. */
    offset = CharIndexToByteOffset(comboPtr->text, index);
    memcpy(text, comboPtr->text, offset);
    memcpy(text + offset, insertText, numBytes);
    memcpy(text + offset + numBytes, comboPtr->text + offset, 
           comboPtr->numBytes - offset);
    comboPtr->numBytes += numBytes;
    if (comboPtr->text != emptyString) {
        Blt_Free(comboPtr->text);
    }
    comboPtr->text = text;
    comboPtr->numChars += numChars;

    /* If the cursor index is after the insert index, then move the cursor
     * down by the number of characters inserted. */
    if (comboPtr->insertIndex >= index) {
        comboPtr->insertIndex += numChars;
    }
    comboPtr->selFirst = comboPtr->selLast = -1;
    CleanText(comboPtr);
    if (comboPtr->textVarObjPtr != NULL) {
        UpdateTextVariable(comboPtr->interp, comboPtr);
    }
    comboPtr->flags |= (MODIFIED | LAYOUT_PENDING | SCROLL_PENDING);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *  H
 *  C
 *  L
 *  P
 *  max of icon/text/image/arrow
 *  P
 *  L
 *  C
 *  H
 *
 * |H|C|L|P| icon |P| text/image |P|L|B| arrow |B|C|H|
 * 
 * H = highlight thickness
 * C = comboentry borderwidth
 * L = label borderwidth
 * P = pad
 * I = icon
 * T = text or image
 *---------------------------------------------------------------------------
 */
static void
ComputeGeometry(ComboEntry *comboPtr)
{
    /* Determine the height of the entry.  It's the maximum height of all
     * it's components: icon, label, clear button, and arrow. */
    comboPtr->iconWidth  = comboPtr->iconHeight  = 0;
    comboPtr->entryWidth = comboPtr->entryHeight = 0;
    comboPtr->textWidth  = comboPtr->textHeight  = 0;
    comboPtr->arrowWidth = comboPtr->arrowHeight = 0;

    comboPtr->inset = comboPtr->borderWidth + comboPtr->highlightWidth;
    comboPtr->width = comboPtr->height = 0;

    if (comboPtr->icon != NULL) {
        comboPtr->iconWidth  = IconWidth(comboPtr->icon) + IPAD;
        comboPtr->iconHeight = IconHeight(comboPtr->icon) + 2 * YPAD;
    }
    if (comboPtr->prefIconWidth > 0) {
        comboPtr->iconWidth = comboPtr->prefIconWidth + IPAD;
    }
    comboPtr->entryWidth += comboPtr->iconWidth;
    if (comboPtr->entryHeight < comboPtr->iconHeight) {
        comboPtr->entryHeight = comboPtr->iconHeight;
    }
    if (comboPtr->image != NULL) {
        comboPtr->textWidth  = IconWidth(comboPtr->image) + IPAD;
        comboPtr->textHeight = IconHeight(comboPtr->image) + YPAD * 2;
    } else {
        unsigned int w, h;

        CleanText(comboPtr);
        if (comboPtr->numScreenBytes == 0) {
            Blt_FontMetrics fm;
            Blt_GetTextExtents(comboPtr->font, 0, "0", 1, &w, &h);
            Blt_Font_GetMetrics(comboPtr->font, &fm);
            h = fm.linespace;
        } else {
            Blt_GetTextExtents(comboPtr->font, 0, comboPtr->screenText, 
                comboPtr->numScreenBytes, &w, &h);
        }
        comboPtr->textWidth  = w;
        comboPtr->textHeight = h + 2 * YPAD;
        if (comboPtr->prefTextWidth > 0) {
            w = Blt_TextWidth(comboPtr->font, "0", 1);
            comboPtr->entryWidth += comboPtr->prefTextWidth * w;
        } else {
            comboPtr->entryWidth += comboPtr->textWidth;
        }
        comboPtr->entryWidth += IPAD;
    } 
    if (comboPtr->entryHeight < comboPtr->textHeight) {
        comboPtr->entryHeight = comboPtr->textHeight;
    }
    comboPtr->width = comboPtr->entryWidth;
    comboPtr->height = comboPtr->entryHeight;
    if (comboPtr->flags & ARROW) {
        Blt_FontMetrics fm;

        Blt_Font_GetMetrics(comboPtr->font, &fm);
        comboPtr->arrowHeight = fm.linespace;
        if (comboPtr->reqArrowWidth > 0) {
            comboPtr->arrowWidth = comboPtr->reqArrowWidth;
        } else {
            comboPtr->arrowWidth = comboPtr->arrowHeight * 60 / 100;
        }
        comboPtr->arrowWidth  += 2 * (comboPtr->arrowBorderWidth + comboPtr->arrowPad + 1);
        comboPtr->arrowHeight += 2 * (comboPtr->arrowBorderWidth + 1);
        if (comboPtr->entryHeight < comboPtr->arrowHeight) {
            comboPtr->height = comboPtr->entryHeight = comboPtr->arrowHeight;
        }
        comboPtr->arrowWidth |= 0x1;
        comboPtr->width += comboPtr->arrowWidth;
    }
    if (comboPtr->flags & XBUTTON) {
        XButton *butPtr = &comboPtr->xButton;
        int bw, bh;

        bw = butPtr->width +  (2 * butPtr->borderWidth) + PADDING(butPtr->padX);
        bh = butPtr->height + (2 * butPtr->borderWidth) + PADDING(butPtr->padY);
        if (bh > comboPtr->entryHeight) {
            comboPtr->height = comboPtr->entryHeight = bh;
        }
        comboPtr->width += bw;
    }
    comboPtr->width  += 2 * comboPtr->inset + IPAD;
    comboPtr->height += 2 * comboPtr->inset;
    {
        int w, h;
        w = (comboPtr->reqWidth > 0) ? comboPtr->reqWidth : comboPtr->width;
        h = (comboPtr->reqHeight > 0) ? comboPtr->reqHeight : comboPtr->height;
        if ((w != Tk_ReqWidth(comboPtr->tkwin)) || 
            (h != Tk_ReqHeight(comboPtr->tkwin))) {
            Tk_GeometryRequest(comboPtr->tkwin, w, h);
        }
    }
    comboPtr->flags &= ~LAYOUT_PENDING;
}

static int
UpdateIconVariable(Tcl_Interp *interp, ComboEntry *comboPtr) 
{
    Tcl_Obj *resultObjPtr, *objPtr;
    
    if (comboPtr->icon != NULL) {
        objPtr = Tcl_NewStringObj(IconName(comboPtr->icon), -1);
    } else {
        objPtr = Tcl_NewStringObj("", -1);
    }
    Tcl_IncrRefCount(objPtr);
    resultObjPtr = Tcl_ObjSetVar2(interp, comboPtr->iconVarObjPtr, NULL, 
        objPtr, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    Tcl_DecrRefCount(objPtr);
    if (resultObjPtr == NULL) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static void
FreeIcon(ComboEntry *comboPtr, Icon icon)
{
    Tk_FreeImage(IconImage(icon));
    Blt_Free(icon);
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
SetTextFromObj(ComboEntry *comboPtr, Tcl_Obj *objPtr) 
{
    int numBytes;
    char *string;

    if (comboPtr->text != emptyString) {
        Blt_Free(comboPtr->text);
    }
    string = Tcl_GetStringFromObj(objPtr, &numBytes);
    comboPtr->text = Blt_AssertMalloc(numBytes + 1);
    memcpy(comboPtr->text, string, numBytes);
    comboPtr->text[numBytes] = '\0';
    comboPtr->numBytes = numBytes;
    CleanText(comboPtr);
    comboPtr->flags |= (ICURSOR | SCROLL_PENDING | LAYOUT_PENDING);
    comboPtr->scrollX = 0;
    comboPtr->selFirst = comboPtr->selLast = -1;
    comboPtr->insertIndex = comboPtr->numChars = 
        Tcl_NumUtfChars(comboPtr->text, comboPtr->numBytes);
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
IconChangedProc(ClientData clientData, int x, int y, int w, int h,
                int imageWidth, int imageHeight)
{
    ComboEntry *comboPtr = clientData;

    comboPtr->flags |= (LAYOUT_PENDING | SCROLL_PENDING);
    EventuallyRedraw(comboPtr);
}

static int
GetIconFromObj(Tcl_Interp *interp, ComboEntry *comboPtr, Tcl_Obj *objPtr, 
               Icon *iconPtr)
{
    Tk_Image tkImage;
    const char *iconName;

    iconName = Tcl_GetString(objPtr);
    if (iconName[0] == '\0') {
        *iconPtr = NULL;
        return TCL_OK;
    }
    tkImage = Tk_GetImage(interp, comboPtr->tkwin, iconName, IconChangedProc, 
        comboPtr);
    if (tkImage != NULL) {
        struct _Icon *ip;
        int width, height;

        ip = Blt_AssertMalloc(sizeof(struct _Icon));
        Tk_SizeOfImage(tkImage, &width, &height);
        ip->tkImage = tkImage;
        ip->width = width;
        ip->height = height;
        *iconPtr = ip;
        return TCL_OK;
    }
    return TCL_ERROR;
}

static void
BlinkCursor(ComboEntry *comboPtr)
{
    int time;

    if (comboPtr->flags & ICURSOR_ON) {
        comboPtr->flags &= ~ICURSOR_ON;
        time = comboPtr->insertOffTime;
    } else {
        comboPtr->flags |= ICURSOR_ON;
        time = comboPtr->insertOnTime;
    }
    comboPtr->insertTimerToken = Tcl_CreateTimerHandler(time, 
        BlinkCursorTimerProc, comboPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * BlinkCursorTimerProc --
 *
 *      This procedure is called as a timer handler to blink the insertion
 *      cursor off and on.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The cursor gets turned on or off, redisplay gets invoked, and this
 *      procedure reschedules itself.
 *
 *---------------------------------------------------------------------------
 */
static void
BlinkCursorTimerProc(ClientData clientData)
{
    ComboEntry *comboPtr = clientData;

    if (((comboPtr->flags & FOCUS) == 0)||(comboPtr->insertOffTime == 0)) {
        return;
    }
    if (comboPtr->flags & ICURSOR) {
        BlinkCursor(comboPtr);
        EventuallyRedraw(comboPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboEntryEventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for various events
 *      on comboentry widgets.
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
ComboEntryEventProc(ClientData clientData, XEvent *eventPtr)
{
    ComboEntry *comboPtr = clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedraw(comboPtr);
        }
    } else if (eventPtr->type == ConfigureNotify) {
        comboPtr->flags |= SCROLL_PENDING;
        EventuallyRedraw(comboPtr);
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
        if (eventPtr->xfocus.detail == NotifyInferior) {
            return;
        }
        if (eventPtr->type == FocusIn) {
            comboPtr->flags |= FOCUS;
        } else {
            comboPtr->flags &= ~FOCUS;
        }
        if (comboPtr->insertTimerToken != NULL) {
            Tcl_DeleteTimerHandler(comboPtr->insertTimerToken);
            comboPtr->insertTimerToken = NULL;
        }
        if ((comboPtr->flags & (FOCUS|ICURSOR|READONLY))==(FOCUS|ICURSOR)) {
            if (comboPtr->flags & ICURSOR_ON) {
                comboPtr->flags &= ~ICURSOR_ON;
            } else {
                comboPtr->flags |= ICURSOR_ON;
            }
            if (comboPtr->insertOffTime != 0) {
                BlinkCursor(comboPtr);
            }
        }
        EventuallyRedraw(comboPtr);
    } else if (eventPtr->type == DestroyNotify) {
        if (comboPtr->tkwin != NULL) {
            comboPtr->tkwin = NULL; 
        }
        if (comboPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayComboEntry, comboPtr);
        }
        if (comboPtr->flags & SELECT_PENDING) {
            Tcl_CancelIdleCall(ComboEntrySelectCmdProc, comboPtr);
        }
        if (comboPtr->flags & INVOKE_PENDING) {
            Tcl_CancelIdleCall(ComboEntryInvokeCmdProc, comboPtr);
        }
        if (comboPtr->insertTimerToken != NULL) {
            Tcl_DeleteTimerHandler(comboPtr->insertTimerToken);
        }
        Tcl_EventuallyFree(comboPtr, FreeComboEntryProc);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboEntryLostSelProc --
 *
 *      This procedure is called back by Tk when the selection is grabbed
 *      away from the comboentry widget.
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
ComboEntryLostSelProc(ClientData clientData)
{
    ComboEntry *comboPtr = clientData;

    if (comboPtr->flags & (OWN_SELECTION|EXPORT_SELECTION)) {
        comboPtr->selFirst = comboPtr->selLast = -1;
        comboPtr->flags &= ~OWN_SELECTION;
        EventuallyRedraw(comboPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ChildEventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for various events
 *      on sub-menus of comboentry widgets.
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
ChildEventProc(ClientData clientData, XEvent *eventPtr)
{
    ComboEntry *comboPtr = clientData;

    if (eventPtr->type == DestroyNotify) {
        comboPtr->flags &= ~STATE_MASK;
        comboPtr->menuWin = NULL;
    } else if (eventPtr->type == UnmapNotify) {
        comboPtr->flags &= ~STATE_MASK;
    } else if (eventPtr->type == MapNotify) {
        comboPtr->flags &= ~STATE_MASK;
        comboPtr->flags |= POSTED;
    } else {
        return;
    }
    EventuallyRedraw(comboPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboEntryInvokeCmdProc --
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
ComboEntryInvokeCmdProc(ClientData clientData) 
{
    ComboEntry *comboPtr = clientData;

    comboPtr->flags &= ~INVOKE_PENDING;
    if (comboPtr->cmdObjPtr != NULL) {
        if (InvokeCommand(comboPtr->interp, comboPtr) != TCL_OK) {
            Tcl_BackgroundError(comboPtr->interp);
        }
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * SelectText --
 *
 *      Modify the selection by moving its un-anchored end.  This could
 *      make the selection either larger or smaller.
 *
 *        1) If index is before the anchor point, sets the selection to the
 *           characters from index up to but not including the anchor
 *           point.
 *        2) If index is the same as the anchor point, does nothing.
 *        3) If index is after the anchor point, set the selection to the
 *           characters from the anchor point up to but not including
 *           index.  The anchor point is determined by the most recent
 *           select from or select adjust command in this widget.
 *        4) If the selection isn't in this widget then a new selection is
 *           created using the most recent anchor point specified for the
 *           widget.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The widget is possibly redrawn with the new selection.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectText(ComboEntry *comboPtr, CharIndex index)
{
    CharIndex first, last;

    /*
     * Grab the selection if we don't own it already.
     */
    if ((comboPtr->flags & (OWN_SELECTION|EXPORT_SELECTION)) == 
        EXPORT_SELECTION) {
        Tk_OwnSelection(comboPtr->tkwin, XA_PRIMARY, ComboEntryLostSelProc, 
                comboPtr);
        comboPtr->flags |= OWN_SELECTION;
    }
    /* If the anchor hasn't been set yet, assume the beginning of the
     * text*/
    if (comboPtr->selAnchor < 0) {
        comboPtr->selAnchor = 0;
    }
    if (comboPtr->selAnchor <= index) {
        first = comboPtr->selAnchor;
        last = index;
    } else {
        first = index;
        last = comboPtr->selAnchor;
    }
    if (((comboPtr->selFirst != first) || (comboPtr->selLast != last)) && 
        (first != last)) {
        comboPtr->selFirst = first;
        comboPtr->selLast = last;
        EventuallyRedraw(comboPtr);
        if (comboPtr->selCmdObjPtr != NULL) {
            EventuallyInvokeSelectCmd(comboPtr);
        }
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ComboEntrySelectCmdProc --
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
ComboEntrySelectCmdProc(ClientData clientData) 
{
    ComboEntry *comboPtr = clientData;

    if (comboPtr->selCmdObjPtr != NULL) {
        int result;

        comboPtr->flags &= ~SELECT_PENDING;
        Tcl_Preserve(comboPtr);
        Tcl_IncrRefCount(comboPtr->selCmdObjPtr);
        result = Tcl_EvalObjEx(comboPtr->interp, comboPtr->selCmdObjPtr, 
                TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(comboPtr->selCmdObjPtr);
        Tcl_Release(comboPtr);
        if (result != TCL_OK) {
            Tcl_BackgroundError(comboPtr->interp);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboEntrySelectionProc --
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
ComboEntrySelectionProc(
    ClientData clientData,              /* Information about the widget. */
    int offset,                         /* Offset within the selection of
                                         * the first character to be
                                         * returned. */
    char *buffer,                       /* Location in which to place
                                         * selection. */
    int maxBytes)                       /* Maximum number of bytes to place
                                         * at buffer, not including
                                         * terminating NULL character. */
{
    ComboEntry *comboPtr = clientData;
    int size;

    size = 0;
    if (comboPtr->selFirst >= 0) {
        ByteOffset first, last;

        first = CharIndexToByteOffset(comboPtr->screenText,comboPtr->selFirst);
        last = CharIndexToByteOffset(comboPtr->screenText, comboPtr->selLast);
        size = last - first - offset;
        assert(size >= 0);
        if (size > maxBytes) {
            size = maxBytes;
        }
        memcpy(buffer, comboPtr->screenText + first + offset, size);
        buffer[size] = '\0';
    }
    return size;
}


/*
 *---------------------------------------------------------------------------
 * 
 * TextVarTraceProc --
 *
 *      This procedure is invoked when someone changes the state variable
 *      associated with a comboentry entry.  The entry's selected state is
 *      set to match the value of the variable.
 *
 * Results:
 *      NULL is always returned.
 *
 * Side effects:
 *      The comboentry entry may become selected or deselected.
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
    ComboEntry *comboPtr = clientData;

    assert(comboPtr->textVarObjPtr != NULL);
    if (flags & TCL_INTERP_DESTROYED) {
        return NULL;                    /* Interpreter is going away. */
    }
    /*
     * If the variable is being unset, then re-establish the trace.
     */
    if (flags & TCL_TRACE_UNSETS) {
        if (flags & TCL_TRACE_DESTROYED) {
            Tcl_SetVar(interp, name1, comboPtr->text, TCL_GLOBAL_ONLY);
            Tcl_TraceVar(interp, name1, TRACE_VAR_FLAGS, TextVarTraceProc, 
                clientData);
        }
        return NULL;
    }
    if (comboPtr->flags & DISABLED) {
        return NULL;
    }
    if (flags & TCL_TRACE_WRITES) {
        Tcl_Obj *valueObjPtr;

        /*
         * Update the comboentry's text with the value of the variable,
         * unless the widget already has that value (this happens when the
         * variable changes value because we changed it because someone
         * typed in the entry).
         */
        valueObjPtr = Tcl_ObjGetVar2(interp, comboPtr->textVarObjPtr, NULL, 
                TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
        if (valueObjPtr == NULL) {
            return GetInterpResult(interp);
        } else {
            SetTextFromObj(comboPtr, valueObjPtr);
            if (comboPtr->cmdObjPtr != NULL) {
                EventuallyInvokeCmd(comboPtr);
            }
        }
        EventuallyRedraw(comboPtr);
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 * 
 * IconVarTraceProc --
 *
 *      This procedure is invoked when someone changes the state variable
 *      associated with a comboentry entry.  The entry's selected state is
 *      set to match the value of the variable.
 *
 * Results:
 *      NULL is always returned.
 *
 * Side effects:
 *      The comboentry entry may become selected or deselected.
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
    ComboEntry *comboPtr = clientData;

    assert(comboPtr->iconVarObjPtr != NULL);
    if (flags & TCL_INTERP_DESTROYED) {
        return NULL;                    /* Interpreter is going away. */

    }
    /*
     * If the variable is being unset, then re-establish the trace.
     */
    if (flags & TCL_TRACE_UNSETS) {
        if (flags & TCL_TRACE_DESTROYED) {
            Tcl_SetVar(interp, name1, IconName(comboPtr->icon),TCL_GLOBAL_ONLY);
            Tcl_TraceVar(interp, name1, TRACE_VAR_FLAGS, IconVarTraceProc, 
                clientData);
        }
        return NULL;
    }
    if (comboPtr->flags & DISABLED) {
        return NULL;
    }
    if (flags & TCL_TRACE_WRITES) {
        Icon icon;
        Tcl_Obj *valueObjPtr;

        /*
         * Update the comboentry's icon with the image whose name is stored
         * in the variable.
         */
        valueObjPtr = Tcl_ObjGetVar2(interp, comboPtr->iconVarObjPtr, NULL, 
                TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
        if (valueObjPtr == NULL) {
            return GetInterpResult(interp);
        }
        if (GetIconFromObj(interp, comboPtr, valueObjPtr, &icon) != TCL_OK) {
            return GetInterpResult(interp);
        }
        if (comboPtr->icon != NULL) {
            FreeIcon(comboPtr, comboPtr->icon);
        }
        comboPtr->icon = icon;
        comboPtr->flags |= LAYOUT_PENDING;
        EventuallyRedraw(comboPtr);
    }
    return NULL;
}

/*ARGSUSED*/
static void
FreeIconVarProc(ClientData clientData, Display *display, char *widgRec,
                int offset)
{
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);

    if (*objPtrPtr != NULL) {
        ComboEntry *comboPtr = (ComboEntry *)widgRec;

        Tcl_UntraceVar(comboPtr->interp, Tcl_GetString(*objPtrPtr), 
                TRACE_VAR_FLAGS, IconVarTraceProc, comboPtr);
        Tcl_DecrRefCount(*objPtrPtr);
        *objPtrPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToIconVar --
 *
 *      Convert the variable to a traced variable.
 *
 * Results:
 *      The return value is a standard TCL result.  The color pointer is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToIconVar(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
             Tcl_Obj *objPtr, char *widgRec, int offset, int flags)    
{
    ComboEntry *comboPtr = (ComboEntry *)(widgRec);
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);
    char *varName;
    Tcl_Obj *valueObjPtr;

    /* Remove the current trace on the variable. */
    if (*objPtrPtr != NULL) {
        Tcl_UntraceVar(interp, Tcl_GetString(*objPtrPtr), TRACE_VAR_FLAGS, 
                       IconVarTraceProc, comboPtr);
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

        if (GetIconFromObj(interp, comboPtr, valueObjPtr, &icon) != TCL_OK) {
            return TCL_ERROR;
        }
        if (comboPtr->icon != NULL) {
            FreeIcon(comboPtr, comboPtr->icon);
        }
        comboPtr->icon = icon;
    }
    *objPtrPtr = objPtr;
    Tcl_IncrRefCount(objPtr);
    Tcl_TraceVar(interp, varName, TRACE_VAR_FLAGS, IconVarTraceProc, comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IconVarToObj --
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
IconVarToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
             char *widgRec, int offset, int flags)     
{
    Tcl_Obj *objPtr = *(Tcl_Obj **)(widgRec + offset);

    if (objPtr == NULL) {
        objPtr = Tcl_NewStringObj("", -1);
    } 
    return objPtr;
}

/*ARGSUSED*/
static void
FreeTextVarProc(ClientData clientData, Display *display, char *widgRec,
                int offset)
{
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);

    if (*objPtrPtr != NULL) {
        ComboEntry *comboPtr = (ComboEntry *)(widgRec);
        const char *varName;

        varName = Tcl_GetString(*objPtrPtr);
        Tcl_UntraceVar(comboPtr->interp, varName, TRACE_VAR_FLAGS, 
                TextVarTraceProc, comboPtr);
        Tcl_DecrRefCount(*objPtrPtr);
        *objPtrPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTextVar --
 *
 *      Convert the variable to a traced variable.
 *
 * Results:
 *      The return value is a standard TCL result.  The color pointer is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTextVar(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
             Tcl_Obj *objPtr, char *widgRec, int offset, int flags)    
{
    ComboEntry *comboPtr = (ComboEntry *)(widgRec);
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);
    const char *varName;
    Tcl_Obj *valueObjPtr;

    /* Remove the current trace on the variable. */
    if (*objPtrPtr != NULL) {
        varName = Tcl_GetString(*objPtrPtr);
        Tcl_UntraceVar(interp, varName, TRACE_VAR_FLAGS, TextVarTraceProc, 
                comboPtr);
        Tcl_DecrRefCount(*objPtrPtr);
        *objPtrPtr = NULL;
    }
    varName = Tcl_GetString(objPtr);
    if ((varName[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
        return TCL_OK;
    }

    valueObjPtr = Tcl_ObjGetVar2(interp, objPtr, NULL, TCL_GLOBAL_ONLY);
    if (valueObjPtr != NULL) {
        SetTextFromObj(comboPtr, valueObjPtr);
        if (comboPtr->textVarObjPtr != NULL) {
            if (UpdateTextVariable(interp, comboPtr) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    *objPtrPtr = objPtr;
    Tcl_IncrRefCount(objPtr);
    Tcl_TraceVar(interp, varName, TRACE_VAR_FLAGS, TextVarTraceProc, comboPtr);
    comboPtr->flags |= MODIFIED;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextVarToObj --
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
TextVarToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
 * ObjToState --
 *
 *      Converts the string represents an entry state into a bitflag.
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
    ComboEntry *comboPtr = (ComboEntry *)(widgRec);
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    char *string;
    int flag;

    string = Tcl_GetString(objPtr);
    if (strcmp(string, "normal") == 0) {
        flag = 0;
    } else if (strcmp(string, "posted") == 0) {
        flag = POSTED;
    } else if (strcmp(string, "disabled") == 0) {
        flag = DISABLED;
    } else {
        Tcl_AppendResult(interp, "unknown state \"", string, 
                "\": should be active, disabled, normal, or posted.", 
                (char *)NULL);
        return TCL_ERROR;
    }
    if (comboPtr->flags & flag) {
        return TCL_OK;                  /* State is already set to value. */
    }
    *flagsPtr &= ~STATE_MASK;
    *flagsPtr |= flag;
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

/*ARGSUSED*/
static void
FreeIconProc(
    ClientData clientData,
    Display *display,           /* Not used. */
    char *widgRec,
    int offset)
{
    Icon icon = *(Icon *)(widgRec + offset);

    if (icon != NULL) {
        ComboEntry *comboPtr = (ComboEntry *)widgRec;

        FreeIcon(comboPtr, icon);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToIcon --
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
ObjToIcon(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          Tcl_Obj *objPtr, char *widgRec, int offset, int flags)    
{
    ComboEntry *comboPtr = (ComboEntry *)widgRec;
    Icon *iconPtr = (Icon *)(widgRec + offset);
    Icon icon;

    if (GetIconFromObj(interp, comboPtr, objPtr, &icon) != TCL_OK) {
        return TCL_ERROR;
    }
    if (*iconPtr != NULL) {
        FreeIcon(comboPtr, *iconPtr);
    }
    *iconPtr = icon;
    if (comboPtr->iconVarObjPtr != NULL) {
        if (UpdateIconVariable(interp, comboPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
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
FreeTextProc(ClientData clientData, Display *display, char *widgRec, int offset)
{
    ComboEntry *comboPtr = (ComboEntry *)(widgRec);

    if (comboPtr->text != emptyString) {
        Blt_Free(comboPtr->text);
        Blt_Free(comboPtr->screenText);
        comboPtr->text = emptyString;
        comboPtr->screenText = NULL;
        comboPtr->numScreenBytes = comboPtr->numBytes = 0;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToText --
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
ObjToText(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          Tcl_Obj *objPtr, char *widgRec, int offset, int flags)    
{
    ComboEntry *comboPtr = (ComboEntry *)(widgRec);

    if (comboPtr->text != emptyString) {
        Blt_Free(comboPtr->text);
        Blt_Free(comboPtr->screenText);
        comboPtr->text = emptyString;
        comboPtr->screenText = NULL;
        comboPtr->numScreenBytes = comboPtr->numBytes = 0;
    }
    SetTextFromObj(comboPtr, objPtr);
    if (comboPtr->textVarObjPtr != NULL) {
        if (UpdateTextVariable(interp, comboPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    comboPtr->flags |= MODIFIED;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextToObj --
 *
 *      Returns the current text of the entry.
 *
 * Results:
 *      The text is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TextToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          char *widgRec, int offset, int flags)     
{
    ComboEntry *comboPtr = (ComboEntry *)(widgRec);

    return Tcl_NewStringObj(comboPtr->text, comboPtr->numBytes);
}

static  int
PrevUtfOffset(const char *string)
{
    int i;

    for (i = 1; i <= TCL_UTF_MAX; i++) {
        unsigned char byte;

        string--;
        byte = *(unsigned char *)string;
        if (byte < 0x80) {
            break;
        }
        if (byte >= 0xC0) {
            return i;
        }
    }
    return i;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTextIndex --
 *
 *      Converts a string representing a item index into an item pointer.
 *      The index may be in one of the following forms:
 *
 *       number         Specifies the character as a numerical index, 
 *                      where 0 corresponds to the first character in 
 *                      the string.
 *       "anchor"       Indicates the anchor point for the selection, 
 *                      which is set with the select from and select 
 *                      adjust widget commands.
 *       "end"          Indicates the character just after the last one  
 *                      in the entry's string.  This is equivalent to 
 *                      specifying a numerical index equal to the length 
 *                      of the entry's string.
 *       "insert"       Indicates the character adjacent to and immediately 
 *                      following the insertion cursor.
 *       "sel.first"    Indicates the first character in the selection.  
 *                      It is an error to use this form if the selection 
 *                      isn't in the entry window.
 *       "sel.last"     Indicates the character just  after the last one 
 *                      in the selection.  It is an error to use this form 
 *                      if  the  selection isn't in the entry window.
 *       @x             X-coordinate in the entry's window;  the character 
 *                      spanning that x-coordinate is used.  For example, 
 *                      "@0" indicates the left-most character in the window.
 *
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.  The
 *      pointer to the node is returned via itemPtrPtr.  Otherwise,
 *      TCL_ERROR is returned and an error message is left in interpreter's
 *      result field.
 *
 *---------------------------------------------------------------------------
 */
static int
GetTextIndex(Tcl_Interp *interp, ComboEntry *comboPtr, Tcl_Obj *objPtr,
             CharIndex *indexPtr)
{
    char *string;
    char c;
    CharIndex index;

    if (Tcl_GetIntFromObj((Tcl_Interp *)NULL, objPtr, &index) == TCL_OK) {
        /* Convert the character index into a byte offset. */
        if (comboPtr->screenText == NULL) {
            *indexPtr = 0;
            return TCL_OK;
        }
        if (index < 0) {
            *indexPtr = 0;
        } else {
            *indexPtr = index;
        }
        return TCL_OK;
    }
    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'a') && (strcmp(string, "anchor") == 0)) {
        if (comboPtr->selAnchor < 0) {
            Tcl_AppendResult(interp, "bad index \"", string, 
                             "\": no selection present.", (char *)NULL);
            return TCL_ERROR;
        }
        *indexPtr = comboPtr->selAnchor;
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
        *indexPtr = comboPtr->numChars;
    } else if ((c == 'i') && (strcmp(string, "insert") == 0)) {
        *indexPtr = comboPtr->insertIndex;
    } else if ((c == 'n') && (strcmp(string, "next") == 0)) {
        index = comboPtr->insertIndex;
        if (index < comboPtr->numChars) {
            index++;
        }
        *indexPtr = index;
    } else if ((c == 'p') && (strcmp(string, "previous") == 0)) {
        index = comboPtr->insertIndex;
        if (index > 0) {
            index--;
        }
        *indexPtr = index;
    } else if ((c == 's') && (strcmp(string, "sel.first") == 0)) {
        *indexPtr = (int)comboPtr->selFirst;
    } else if ((c == 's') && (strcmp(string, "sel.last") == 0)) {
        *indexPtr = (int)comboPtr->selLast;
    } else if (c == '@') {
        int x, dummy, numBytes;

        if (Tcl_GetInt(interp, string+1, &x) != TCL_OK) {
            return TCL_ERROR;
        }
        /* Convert screen position to character index */
        x -= comboPtr->inset + comboPtr->iconWidth;
        x += comboPtr->scrollX;
        numBytes = Blt_Font_Measure(comboPtr->font, comboPtr->screenText, 
                comboPtr->numScreenBytes, x, TK_PARTIAL_OK|TK_AT_LEAST_ONE, 
                &dummy);
        *indexPtr = Tcl_NumUtfChars(comboPtr->screenText, numBytes);
    } else {
        Tcl_AppendResult(interp, "unknown index \"", string, "\"",(char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyXButton --
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyXButton(ComboEntry *comboPtr, XButton *butPtr)
{
    iconOption.clientData = comboPtr;
    Blt_FreeOptions(xButtonSpecs, (char *)butPtr, comboPtr->display, 0);
    if (butPtr->activePicture != NULL) {
        Blt_FreePicture(butPtr->activePicture);
    }
    if (butPtr->normalPicture != NULL) {
        Blt_FreePicture(butPtr->normalPicture);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureXButton --
 *
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) the widget.
 *
 * Results:
 *      The return value is a standard TCL result.  If TCL_ERROR is
 *      returned, then interp->result contains an error message.
 *
 * Side Effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for setPtr; old resources get freed, if there were
 *      any.  The widget is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureXButton(
    Tcl_Interp *interp,                 /* Interpreter to report errors. */
    ComboEntry *comboPtr,               /* Information about widget; may or
                                         * may not already have values for
                                         * some fields. */
    int objc,
    Tcl_Obj *const *objv,
    int flags)
{
    XButton *butPtr = &comboPtr->xButton;

    iconOption.clientData = comboPtr;
    if (Blt_ConfigureWidgetFromObj(interp, comboPtr->tkwin, xButtonSpecs, 
        objc, objv, (char *)butPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    butPtr->width = butPtr->height = 0;
    if (comboPtr->flags & XBUTTON) {
        Blt_FontMetrics fm;

        Blt_Font_GetMetrics(comboPtr->font, &fm);
        butPtr->width = butPtr->height = 8 * fm.linespace / 10 -
            (2 * butPtr->borderWidth);
    }
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}


static int
ConfigureComboEntry(Tcl_Interp *interp, ComboEntry *comboPtr, int objc,
                    Tcl_Obj *const *objv, int flags)
{
    unsigned int gcMask;
    XGCValues gcValues;
    GC newGC;

    if (Blt_ConfigureWidgetFromObj(interp, comboPtr->tkwin, configSpecs, objc, 
                objv, (char *)comboPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    if (comboPtr->flags & READONLY) {
        comboPtr->flags &= ~ICURSOR;
    } else {
        comboPtr->flags |= ICURSOR;
    }
    /* Text in/out focus GCs. */
    gcMask = GCForeground | GCFont;
    if (comboPtr->flags & DISABLED) {
        gcValues.foreground = comboPtr->disabledFgColor->pixel;
    } else {
        gcValues.foreground = comboPtr->textInFocusColor->pixel;
    }   
    gcValues.font = Blt_Font_Id(comboPtr->font);
    newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    if (comboPtr->textInFocusGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->textInFocusGC);
    }
    comboPtr->textInFocusGC = newGC;

    gcMask = GCForeground | GCFont;
    if (comboPtr->flags & DISABLED) {
        gcValues.foreground = comboPtr->disabledFgColor->pixel;
    } else {
        gcValues.foreground = comboPtr->textOutFocusColor->pixel;
    }   
    gcValues.font = Blt_Font_Id(comboPtr->font);
    newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    if (comboPtr->textOutFocusGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->textOutFocusGC);
    }
    comboPtr->textOutFocusGC = newGC;

    /* Selection foreground. */
    gcMask = GCForeground | GCFont;
    gcValues.foreground = comboPtr->selFgColor->pixel;
    gcValues.font = Blt_Font_Id(comboPtr->font);
    newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    if (comboPtr->selectGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->selectGC);
    }
    comboPtr->selectGC = newGC;

    /* Focus highlight GCs */
    gcMask = GCForeground;
    gcValues.foreground = comboPtr->highlightColor->pixel;
    newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    if (comboPtr->highlightGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->highlightGC);
    }
    comboPtr->highlightGC = newGC;

    if (comboPtr->highlightBgColor != NULL) {
        gcMask = GCForeground;
        gcValues.foreground = comboPtr->highlightBgColor->pixel;
        newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    } else {
        newGC = NULL;
    }
    if (comboPtr->highlightBgGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->highlightBgGC);
    }
    comboPtr->highlightBgGC = newGC;

    /* Insert cursor. */
    gcMask = GCForeground;
    gcValues.foreground = comboPtr->insertColor->pixel;
    newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    if (comboPtr->insertGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->insertGC);
    }
    comboPtr->insertGC = newGC;
    ComputeGeometry(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 *      Activates
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      pathName activate what
 *
 *---------------------------------------------------------------------------
 */
static int
ActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    const char *string;
    unsigned int old;

    if (comboPtr->flags & DISABLED) {
        return TCL_OK;                  /* Writing is currently disabled. */
    }
    string = Tcl_GetString(objv[2]);
    old = (comboPtr->flags & ACTIVE_MASK);
    comboPtr->flags &= ~ACTIVE_MASK;
    if (strcmp(string, "button") == 0) {
        comboPtr->flags |= ACTIVE_BUTTON;
    } else if (strcmp(string, "arrow") == 0) {
        comboPtr->flags |= ACTIVE_ARROW;
    }   
    if (old != (comboPtr->flags & ACTIVE_MASK)) {
        EventuallyRedraw(comboPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonActivateOp --
 *
 *      Activates the button. 
 *
 * Results:
 *      Standard TCL result.
 *
 *      pathName button activate
 *
 *---------------------------------------------------------------------------
 */
static int
ButtonActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    unsigned int oldFlags;

    if (comboPtr->flags & DISABLED) {
        return TCL_OK;                  /* The widget is currently disabled. */
    }
    oldFlags = (comboPtr->flags & ACTIVE_MASK);
    comboPtr->flags &= ~ACTIVE_MASK;
    comboPtr->flags |= ACTIVE_BUTTON;
    if (oldFlags != (comboPtr->flags & ACTIVE_MASK)) {
        EventuallyRedraw(comboPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonCgetOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ButtonCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;

    iconOption.clientData = comboPtr;
    return Blt_ConfigureValueFromObj(interp, comboPtr->tkwin, xButtonSpecs,
        (char *)&comboPtr->xButton, objv[2], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonConfigureOp --
 *
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) the widget.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 * Side Effects:
 *      Configuration information, such as text string, colors, font, etc. get
 *      set for comboPtr; old resources get freed, if there were any.  The
 *      widget is redisplayed.
 *
 *      pathName button configure ?option value?
 *
 *---------------------------------------------------------------------------
 */
static int
ButtonConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;

    iconOption.clientData = comboPtr;
    if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, xButtonSpecs,
            (char *)&comboPtr->xButton, (Tcl_Obj *)NULL, 0);
    } else if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, xButtonSpecs,
            (char *)&comboPtr->xButton, objv[2], 0);
    }
    if (ConfigureXButton(interp, comboPtr, objc - 3, objv + 3, 
                BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonDeactivateOp --
 *
 *      Deactivates the button. 
 *
 * Results:
 *      Standard TCL result.
 *
 *      pathName button deactivate
 *
 *---------------------------------------------------------------------------
 */
static int
ButtonDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;

    if (comboPtr->flags & DISABLED) {
        return TCL_OK;                  /* The widget is currently disabled. */
    }
    if (comboPtr->flags & ACTIVE_BUTTON) {
        EventuallyRedraw(comboPtr);
    }
    comboPtr->flags &= ~ACTIVE_BUTTON;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonInvokeOp --
 *
 *      This procedure is called to invoke a button command.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 *        pathName button invoke
 * 
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ButtonInvokeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;

    if (comboPtr->flags & (READONLY|DISABLED)) {
        return TCL_OK;                  /* Writing is currently disabled. */
    }
    if (comboPtr->xButton.cmdObjPtr != NULL) {
        Tcl_Obj *cmdObjPtr;
        int result;

        cmdObjPtr = Tcl_DuplicateObj(comboPtr->xButton.cmdObjPtr);
        Tcl_IncrRefCount(cmdObjPtr);
        result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(cmdObjPtr);
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        /* Record the delete for futher redo/undos.  */
        RecordEdit(comboPtr, DELETE_OP, 0, comboPtr->text, comboPtr->numBytes);
        DeleteText(comboPtr, 0, comboPtr->numChars);
        FreeRedoRecords(comboPtr);
    }
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ButtonOp --
 *
 *      This procedure handles tab operations.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec buttonOps[] =
{
    {"activate",   1, ButtonActivateOp,   3, 3, "",},
    {"cget",       2, ButtonCgetOp,       4, 4, "option",},
    {"configure",  2, ButtonConfigureOp,  3, 0, "?option value?...",},
    {"deactivate", 1, ButtonDeactivateOp, 3, 3, "",},
    {"invoke",     1, ButtonInvokeOp,     3, 3, "",},
};
static int numButtonOps = sizeof(buttonOps) / sizeof(Blt_OpSpec);


static int
ButtonOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numButtonOps, buttonOps, 
        BLT_OP_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (comboPtr, interp, objc, objv);
    return result;
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
 *      pathName cget option
 *
 *---------------------------------------------------------------------------
 */
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;

    iconOption.clientData = comboPtr;
    return Blt_ConfigureValueFromObj(interp, comboPtr->tkwin, configSpecs,
        (char *)comboPtr, objv[2], BLT_CONFIG_OBJV_ONLY);
}

/*
 *---------------------------------------------------------------------------
 *
 * ClosestOp --
 *
 *      Returns the index of the edge closest to the given x-coordinate.
 *
 * Results:
 *      A standard TCL result.  If the argument does not represent a valid
 *      index, then TCL_ERROR is returned and the interpreter result will
 *      contain an error message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ClosestOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    ByteOffset offset;
    CharIndex index;
    int x;
    
    if (Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) {
        return TCL_ERROR;
    }
    /* Convert screen position to character index */
    x -= comboPtr->inset;
    if (comboPtr->icon != NULL) {
        x -= comboPtr->iconWidth;
    }
    x += comboPtr->scrollX;
    if (x <= 0) {
        offset = 0;
    } else if (x >= comboPtr->textWidth) {
        offset = comboPtr->numScreenBytes;
    } else {
        int prev;
        int dummy, leftEdge, rightEdge, mid;

        offset = Blt_Font_Measure(comboPtr->font, comboPtr->screenText, 
                comboPtr->numScreenBytes, x, TK_PARTIAL_OK|TK_AT_LEAST_ONE, 
                &dummy);
        /* Get the previous character */
        prev = offset - PrevUtfOffset(comboPtr->screenText + offset);
        /* Measure the two strings. */
        rightEdge = Blt_TextWidth(comboPtr->font, comboPtr->screenText, offset);
        leftEdge = Blt_TextWidth(comboPtr->font, comboPtr->screenText, prev);
        mid = (rightEdge + leftEdge + 1) / 2;
        if (x <= mid) {
            offset = prev;
        }
    }
    index = Tcl_NumUtfChars(comboPtr->screenText, offset);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), index);
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
 *      pathName configure ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    int result;

    iconOption.clientData = comboPtr;
    if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, configSpecs, 
                (char *)comboPtr, (Tcl_Obj *)NULL, BLT_CONFIG_OBJV_ONLY);
    } else if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, configSpecs, 
                (char *)comboPtr, objv[2], BLT_CONFIG_OBJV_ONLY);
    }
    Tcl_Preserve(comboPtr);
    result = ConfigureComboEntry(interp, comboPtr, objc - 2, objv + 2, 
                BLT_CONFIG_OBJV_ONLY);
    Tcl_Release(comboPtr);
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    comboPtr->flags |= (LAYOUT_PENDING | SCROLL_PENDING);
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *      Deletes one of more characters in the label's text label.  The range
 *      of characters is specified by the range first/last.  If no last
 *      argument is provided, then only the single character is deleted.
 *
 *      Tv\a
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      pathName delete first ?last?
 *
 *---------------------------------------------------------------------------
 */
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    CharIndex first, last;
    ByteOffset firstOffset, lastOffset;
    const char *text;

    if (comboPtr->flags & (READONLY|DISABLED)) {
        return TCL_OK;                  /* Writing is currently disabled. */
    }

    if (GetTextIndex(interp, comboPtr, objv[2], &first) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        if (GetTextIndex(interp, comboPtr, objv[3], &last) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        last = first + 1;
    }
    if ((first == -1) || (last == -1)) {
        return TCL_OK;
    }
    /* Record the delete for futher redo/undos.  */
    firstOffset = CharIndexToByteOffset(comboPtr->text, first);
    lastOffset  = CharIndexToByteOffset(comboPtr->text, last);
    text = comboPtr->text + firstOffset;
    RecordEdit(comboPtr, DELETE_OP, first, text, lastOffset - firstOffset);
    DeleteText(comboPtr, first, last);
    FreeRedoRecords(comboPtr);
    if (comboPtr->textVarObjPtr != NULL) {
        if (UpdateTextVariable(interp, comboPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    comboPtr->flags |= MODIFIED;
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *      Returns the current text string in the widget.
 *
 * Results:
 *      Standard TCL result.
 *
 *      pathName get 
 *
 *---------------------------------------------------------------------------
 */
static int
GetOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    Tcl_Obj *objPtr;

    objPtr = Tcl_NewStringObj(comboPtr->text, comboPtr->numBytes);
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IndexOp --
 *
 *      Returns the actual character index of the index supplied.  This
 *      converts text indices such as "end" to the number of UTF characters in
 *      the text string.
 *
 *      It's an error if the index refers to a non-present selection.  Empty
 *      text strings always return an index of 0.
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      pathName index indexName
 *
 *---------------------------------------------------------------------------
 */
static int
IndexOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    CharIndex index;

    if (GetTextIndex(interp, comboPtr, objv[2], &index) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IcursorOp --
 *
 *      Sets the cursor to a new location.
 *
 * Results:
 *      A standard TCL result.  If the argument does not represent a valid
 *      index, then TCL_ERROR is returned and the interpreter result will
 *      contain an error message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IcursorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    CharIndex index;

    if (comboPtr->flags & DISABLED) {
        return TCL_OK;          /* Widget is currently disabled. */
    }
    if (GetTextIndex(interp, comboPtr, objv[2], &index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (index == -1) {
        return TCL_OK;
    }
    comboPtr->insertIndex = index;
    comboPtr->flags |= ICURSOR;
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * IdentifyOp --
 *
 *      Returns the name of the element under the point given by x and y
 *      (such  as  arrow1), or an empty string if the point does not lie
 *      in any element of the comboentry.  X and Y must be pixel 
 *      coordinates relative to the widget.
 *
 * Results:
 *      A standard TCL result.  If the argument does not represent a valid
 *      index, then TCL_ERROR is returned and the interpreter result will
 *      contain an error message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IdentifyOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    int x, y, width, height;
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
    if ((Tk_GetPixelsFromObj(interp, comboPtr->tkwin, objv[2], &x) != TCL_OK) ||
        (Tk_GetPixelsFromObj(interp, comboPtr->tkwin, objv[3], &y) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (isRoot) {
        int rootX, rootY;

        Tk_GetRootCoords(comboPtr->tkwin, &rootX, &rootY);
        x -= rootX;
        y -= rootY;
    }
    width = Tk_Width(comboPtr->tkwin);
    height = Tk_Height(comboPtr->tkwin);
    if ((x < 0) || (x >= width) || (y < 0) || (y >= height)) {
        return TCL_OK;
    }
    if (comboPtr->icon) {
        int iconX;
        
        iconX = comboPtr->inset;
        if ((x >= iconX) && (x < (iconX + IconWidth(comboPtr->icon)))) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("icon", 4));
            return TCL_OK;
        }
    }
    if (comboPtr->flags & ARROW) {
        if ((x >= comboPtr->arrowX) && 
            (x < (comboPtr->arrowX + comboPtr->arrowWidth))) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("arrow", 5));
            return TCL_OK;
        }
    }
    if (comboPtr->flags & XBUTTON) {
        XButton *butPtr = &comboPtr->xButton;

        if ((x >= butPtr->x) && (x < (butPtr->x + butPtr->width)) &&
            (y >= butPtr->y) && (y < (butPtr->y + butPtr->height))) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("button", 6));
            return TCL_OK;
        }
    }
    if (comboPtr->flags & READONLY) {
        if (comboPtr->flags & ARROW) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("arrow", 5));
        }
        return TCL_OK;
    }
    {
        int tx;

        tx = comboPtr->inset;
        if (comboPtr->iconWidth > 0) {
            tx += comboPtr->iconWidth;
        }
        if ((x >= tx) && (x < (tx + comboPtr->textWidth))) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("text", 4));
            return TCL_OK;
        }
    }
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
 *      pathName invoke item 
 *
 *---------------------------------------------------------------------------
 */
static int
InvokeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    int result;

    if (comboPtr->flags & DISABLED) {
        return TCL_OK;          /* Item is currently disabled. */
    }
    result = TCL_OK;
    if (comboPtr->cmdObjPtr != NULL) {
        result = InvokeCommand(interp, comboPtr);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * InsertOp --
 *
 *      Inserts a new item into the comboentry at the given index.
 *
 * Results:
 *      NULL is always returned.
 *
 * Side effects:
 *      The comboentry gets a new item.
 *
 *      pathName insert indexName textString
 *
 *---------------------------------------------------------------------------
 */
static int
InsertOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    CharIndex index;
    char *insertText;
    int numBytes;

    if (comboPtr->flags & (READONLY|DISABLED)) {
        return TCL_OK;          /* Writing is currently disabled. */
    }
    if (GetTextIndex(interp, comboPtr, objv[2], &index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (index == -1) {
        return TCL_OK;
    }
    insertText = Tcl_GetStringFromObj(objv[3], &numBytes);

    /* Record the operation for future undo/redos. */
    RecordEdit(comboPtr, INSERT_OP, index, insertText, numBytes);

    if (InsertText(comboPtr, index, numBytes, insertText) != TCL_OK) {
        return TCL_ERROR;
    }
    FreeRedoRecords(comboPtr);
    if (comboPtr->textVarObjPtr != NULL) {
        if (UpdateTextVariable(interp, comboPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    comboPtr->flags |= MODIFIED;
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PostOp --
 *
 *      Posts the menu associated with this widget.
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *  pathName post
 *
 *---------------------------------------------------------------------------
 */
static int
PostOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    char *menuName;
    Tk_Window menuWin;
    
    if (comboPtr->flags & (POSTED|DISABLED)) {
        return TCL_OK;                  /* Entry's menu is currently posted
                                         * or entry is disabled. */
    }
    if (comboPtr->menuObjPtr == NULL) {
        return TCL_OK;
    }
    menuName = Tcl_GetString(comboPtr->menuObjPtr);
    menuWin = Tk_NameToWindow(interp, menuName, comboPtr->tkwin);
    comboPtr->menuWin = menuWin;
    if (menuWin == NULL) {
        return TCL_ERROR;
    }
    if (Tk_Parent(menuWin) != comboPtr->tkwin) {
        Tcl_AppendResult(interp, "can't post \"", Tk_PathName(menuWin), 
                "\": it isn't a descendant of ", Tk_PathName(comboPtr->tkwin),
                (char *)NULL);
        return TCL_ERROR;
    }
    if (comboPtr->menuWin != NULL) {
        Tk_DeleteEventHandler(comboPtr->menuWin, CHILD_EVENT_MASK, 
                ChildEventProc, comboPtr);
    } 
    comboPtr->menuWin = menuWin;
    Tk_CreateEventHandler(menuWin, CHILD_EVENT_MASK, ChildEventProc, comboPtr);
    if (comboPtr->postCmdObjPtr) {
        int result;

        Tcl_Preserve(comboPtr);
        Tcl_IncrRefCount(comboPtr->postCmdObjPtr);
        result = Tcl_EvalObjEx(interp, comboPtr->postCmdObjPtr, 
                               TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(comboPtr->postCmdObjPtr);
        Tcl_Release(comboPtr);
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
    }
    {
        Tcl_Obj *cmdObjPtr, *objPtr;
        int result;

        /* menu post -align right */
        cmdObjPtr = Tcl_DuplicateObj(comboPtr->menuObjPtr);
        objPtr = Tcl_NewStringObj("post", 4);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        objPtr = Tcl_NewStringObj("-align", 6);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        objPtr = Tcl_NewStringObj("right", 5);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        {
            int x1, y1, x2, y2, rootX, rootY;
            Tcl_Obj *listObjPtr;
            
            Tk_GetRootCoords(comboPtr->tkwin, &rootX, &rootY);
            x1 = rootX;
            x2 = Tk_Width(comboPtr->tkwin) + rootX;
            y1 = comboPtr->inset + 1 + rootY;
            y2 = Tk_Height(comboPtr->tkwin) + rootY;

            objPtr = Tcl_NewStringObj("-box", 4);
            Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
            Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x1));
            Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y1));
            Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x2));
            Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y2));
            Tcl_ListObjAppendElement(interp, cmdObjPtr, listObjPtr);
        }
        Tcl_IncrRefCount(cmdObjPtr);
        Tcl_Preserve(comboPtr);
        result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_Release(comboPtr);
        Tcl_DecrRefCount(cmdObjPtr);
        if (result == TCL_OK) {
            comboPtr->flags &= ~STATE_MASK;
            comboPtr->flags |= POSTED;
        }
        return result;
    }
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
    ComboEntry *comboPtr = clientData;
    int oper;
    int x;

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
    if (objc == 3) {
        if (oper == SCAN_MARK) {
            Tcl_SetIntObj(Tcl_GetObjResult(interp), comboPtr->scanAnchor);
        }
        return TCL_OK;
    }
    if (comboPtr->flags & DISABLED) {
        return TCL_OK;          /* Widget is currently disabled. */
    }
    if (Blt_GetPixelsFromObj(interp, comboPtr->tkwin, objv[3], PIXELS_ANY, &x) 
         != TCL_OK) {
        return TCL_ERROR;
    }
    if (oper == SCAN_MARK) {
        comboPtr->scanAnchor = x;
        comboPtr->scanX = comboPtr->scrollX;
    } else {
        int worldX, xMax;
        int dx;

        dx = comboPtr->scanAnchor - x;
        worldX = comboPtr->scanX + (10 * dx);
        xMax = comboPtr->viewWidth - ICWIDTH;

        if (worldX < 0) {
            worldX = 0;
        } else if ((worldX + xMax) >= comboPtr->textWidth) {
            worldX = comboPtr->textWidth; /* - (8 * xMax / 10); */
        }
        comboPtr->scrollX = worldX;
        comboPtr->flags |= SCROLL_PENDING;
        EventuallyRedraw(comboPtr);
    }
    return TCL_OK;
}

static int
SeeOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    CharIndex index;
    ByteOffset offset;

    if (comboPtr->flags & DISABLED) {
        return TCL_OK;          /* Widget is currently disabled. */
    }
    if (GetTextIndex(interp, comboPtr, objv[2], &index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (index == -1) {
        return TCL_OK;
    }
    offset = CharIndexToByteOffset(comboPtr->screenText, index);
    if ((offset <= comboPtr->firstOffset) || 
        (offset >= (comboPtr->lastOffset - 1))) {
        int xMax, x;

        x = Blt_TextWidth(comboPtr->font, comboPtr->screenText, offset);
        xMax = comboPtr->viewWidth - ICWIDTH;
        if (x >= xMax) {
            x -= xMax;
            if (offset <= comboPtr->firstOffset) {
                x += 9 * xMax / 10;
            } else {
                x += 1 * xMax / 10;
            }
        } else {
            x = 0;
        }
        comboPtr->scrollX = x;
    }
    comboPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionAdjustOp --
 *
 *      Locates the end of the selection nearest to the character given by
 *      index, and adjusts that end of the selection to be at index
 *      (i.e. including but not going beyond index).  The other end of the
 *      selection is made the anchor point for future select to commands.
 *      If the selection isn't currently in the comboentry, then a new
 *      selection is created to include the characters between index and
 *      the most recent selection anchor point, inclusive.
 *
 *      This procedure is called back by Tk when the selection is requested
 *      by someone.  It returns part or all of the selection in a buffer
 *      provided by the caller.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      The widget is possibly redrawn with the new selection.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionAdjustOp(ClientData clientData, Tcl_Interp *interp, int objc,
                  Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    CharIndex index;
    int half1, half2;

    if (comboPtr->flags & DISABLED) {
        return TCL_OK;                  /* Widget is currently disabled. */
    }
    if (GetTextIndex(interp, comboPtr, objv[3], &index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (index == -1) {
        return TCL_OK;
    }
    half1 = (comboPtr->selFirst + comboPtr->selLast) / 2;
    half2 = (comboPtr->selFirst + comboPtr->selLast + 1) / 2;
    if (index < half1) {
        comboPtr->selAnchor = comboPtr->selLast;
    } else if (index > half2) {
        comboPtr->selAnchor = comboPtr->selFirst;
    }
    return SelectText(comboPtr, index);
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionClearOp --
 *
 *      Clears the selection.  
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      The widget is possibly redrawn.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionClearOp(ClientData clientData, Tcl_Interp *interp, int objc,
                 Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;

    if (comboPtr->flags & DISABLED) {
        return TCL_OK;                  /* Widget is currently disabled. */
    }
    if (comboPtr->selFirst != -1) {
        comboPtr->selFirst = comboPtr->selLast = -1;
        EventuallyRedraw(comboPtr);
        if (comboPtr->selCmdObjPtr != NULL) {
            EventuallyInvokeSelectCmd(comboPtr);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionFromOp --
 *
 *      Sets the selection anchor point to just before the character
 *      designated by the given index.  Doesn't change the selection, just
 *      resets the anchor of the existing selection. Returns an empty
 *      string.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionFromOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    CharIndex index;

    if (comboPtr->flags & DISABLED) {
        return TCL_OK;                  /* Widget is currently disabled. */
    }
    if (GetTextIndex(interp, comboPtr, objv[3], &index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (index == -1) {
        return TCL_OK;
    }
    comboPtr->selAnchor = index;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionPresentOp --
 *
 *      Indicates if there are characters selected in the comboentry.
 *
 * Results:
 *      Returns in the interpreter result, 1 if there is are characters
 *      selected, 0 if nothing is selected.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionPresentOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;

    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (comboPtr->selFirst!=-1));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionRangeOp --
 *
 *      Sets the selection to include the characters starting with the one
 *      indexed by start and ending with the one just before end.  If end
 *      refers to the same character as start or an earlier one, then the
 *      entry's selection is cleared.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionRangeOp(ClientData clientData, Tcl_Interp *interp, int objc,
                 Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    CharIndex first, last;

    if (comboPtr->flags & DISABLED) {
        return TCL_OK;                  /* Widget is currently disabled. */
    }
    if (GetTextIndex(interp, comboPtr, objv[3], &first) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetTextIndex(interp, comboPtr, objv[4], &last) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((first == -1) || (last == -1)) {
        return TCL_OK;
    }
    comboPtr->selAnchor = first;
    return SelectText(comboPtr, last);
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionToOp --
 *
 *      Resets the selection depending upon the given new index.  Returns an
 *      empty string.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionToOp(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    CharIndex index;

    if (comboPtr->flags & DISABLED) {
        return TCL_OK;                  /* Widget is currently disabled. */
    }
    if (GetTextIndex(interp, comboPtr, objv[3], &index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (index == -1) {
        return TCL_OK;
    }
    return SelectText(comboPtr, index);
}


static Blt_OpSpec selectionOps[] =
{
    {"adjust",  1, SelectionAdjustOp,  4, 4, "index",},
    {"clear",   1, SelectionClearOp,   3, 3, "",},
    {"from",    1, SelectionFromOp,    4, 4, "index"},
    {"present", 1, SelectionPresentOp, 3, 3, ""},
    {"range",   1, SelectionRangeOp,   5, 5, "start end",},
    {"to",      1, SelectionToOp,      4, 4, "index"},
};

static int numSelectionOps = sizeof(selectionOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * SelectionOp --
 *
 *      This procedure handles the individual options for text
 *      selections.  
 *
 *---------------------------------------------------------------------------
 */
static int
SelectionOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numSelectionOps, selectionOps, BLT_OP_ARG2, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (comboPtr, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * RedoOp --
 *
 *      Inserts a new item into the comboentry at the given index.
 *
 * Results:
 *      NULL is always returned.
 *
 * Side effects:
 *      The comboentry gets a new item.
 *
 *   pathName insert index string
 *
 *---------------------------------------------------------------------------
 */
static int
RedoOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;

    if (comboPtr->flags & (READONLY|DISABLED)) {
        return TCL_OK;                 /* Writing is currently disabled. */
    }
    if (comboPtr->redoPtr != NULL) {
        EditRecord *recPtr;

        recPtr = comboPtr->redoPtr;
        if (recPtr->type == INSERT_OP) {
            InsertText(comboPtr, recPtr->index, recPtr->numBytes, 
                recPtr->text);
        } else if (recPtr->type == DELETE_OP) {
            DeleteText(comboPtr, recPtr->index, recPtr->index+recPtr->numChars);
        } else {
            Tcl_AppendResult(interp, "unknown record type \"", 
                             Blt_Itoa(recPtr->type), "\"", (char *)NULL);
            return TCL_ERROR;
        }
        comboPtr->insertIndex = recPtr->insertIndex;
        comboPtr->redoPtr = recPtr->nextPtr;
        recPtr->nextPtr = comboPtr->undoPtr;
        comboPtr->undoPtr = recPtr;
        EventuallyRedraw(comboPtr);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * UndoOp --
 *
 *      Inserts a new item into the comboentry at the given index.
 *
 * Results:
 *      NULL is always returned.
 *
 * Side effects:
 *      The comboentry gets a new item.
 *
 *      pathName insert index string
 *
 *---------------------------------------------------------------------------
 */
static int
UndoOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;

    if (comboPtr->flags & (READONLY|DISABLED)) {
        return TCL_OK;                 /* Writing is currently disabled. */
    }
    if (comboPtr->undoPtr != NULL) {
        EditRecord *recPtr;

        recPtr = comboPtr->undoPtr;
        if (recPtr->type == INSERT_OP) {
            DeleteText(comboPtr, recPtr->index, recPtr->index+recPtr->numChars);
        } else if (recPtr->type == DELETE_OP) {
            InsertText(comboPtr, recPtr->index, recPtr->numBytes, recPtr->text);
        } else {
            Tcl_AppendResult(interp, "unknown record type \"", 
                             Blt_Itoa(recPtr->type), "\"", (char *)NULL);
            return TCL_ERROR;
        }
        comboPtr->insertIndex = recPtr->insertIndex;
        comboPtr->undoPtr = recPtr->nextPtr;
        recPtr->nextPtr = comboPtr->redoPtr;
        comboPtr->redoPtr = recPtr;
        EventuallyRedraw(comboPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * UnpostOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      pathName unpost
 *
 *---------------------------------------------------------------------------
 */
static int
UnpostOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;

    if (comboPtr->menuWin != NULL) {
        comboPtr->flags &= ~STATE_MASK;
        Blt_UnmapToplevelWindow(comboPtr->menuWin);
        if (Tk_IsMapped(comboPtr->menuWin)) {
            Tk_UnmapWindow(comboPtr->menuWin);
        }
    }
    return TCL_OK;
}
    
static int
XviewOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr = clientData;

    int width;

    if (comboPtr->flags & (DISABLED|READONLY)) {
        return TCL_OK;                  /* Widget is currently disabled. */
    }
    width = comboPtr->viewWidth;
    if (objc == 2) {
        double fract;
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        /*
         * Note: We are bounding the fractions between 0.0 and 1.0 to
         * support the "canvas"-style of scrolling.
         */
        fract = (double)comboPtr->scrollX / comboPtr->textWidth;
        fract = FCLAMP(fract);
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
        fract = (double)(comboPtr->scrollX + width) / comboPtr->textWidth;
        fract = FCLAMP(fract);
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(fract));
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, 
        &comboPtr->scrollX, comboPtr->textWidth, width, 
        comboPtr->scrollUnits, BLT_SCROLL_MODE_HIERBOX) != TCL_OK) {
        return TCL_ERROR;
    }
    comboPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeComboEntryProc --
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
FreeComboEntryProc(DestroyData dataPtr) /* Pointer to the widget record. */
{
    ComboEntry *comboPtr = (ComboEntry *)dataPtr;

    iconOption.clientData = comboPtr;
    Blt_FreeOptions(configSpecs, (char *)comboPtr, comboPtr->display, 0);
    if (comboPtr->textInFocusGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->textInFocusGC);
    }
    if (comboPtr->textOutFocusGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->textOutFocusGC);
    }
    FreeUndoRecords(comboPtr);
    FreeRedoRecords(comboPtr);
    DestroyXButton(comboPtr, &comboPtr->xButton);
    if (comboPtr->screenText != NULL) {
        Blt_Free(comboPtr->screenText);
    }
    if (comboPtr->selectGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->selectGC);
    }
    if (comboPtr->highlightGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->highlightGC);
    }
    if (comboPtr->highlightBgGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->highlightBgGC);
    }
    if (comboPtr->insertGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->insertGC);
    }
    if (comboPtr->insertTimerToken != NULL) {
        Tcl_DeleteTimerHandler(comboPtr->insertTimerToken);
    }
    if (comboPtr->menuWin != NULL) {
        Tk_DeleteEventHandler(comboPtr->menuWin, CHILD_EVENT_MASK, 
                ChildEventProc, comboPtr);
    }
    if (comboPtr->tkwin != NULL) {
        Tk_DeleteSelHandler(comboPtr->tkwin, XA_PRIMARY, XA_STRING);
        Tk_DeleteEventHandler(comboPtr->tkwin, EVENT_MASK, 
                ComboEntryEventProc, comboPtr);
    }
    if (comboPtr->insertTimerToken != NULL) {
        Tcl_DeleteTimerHandler(comboPtr->insertTimerToken);
    }
    if (comboPtr->painter != NULL) {
        Blt_FreePainter(comboPtr->painter);
    }
    if (comboPtr->normalArrow != NULL) {
        Blt_FreePicture(comboPtr->normalArrow);
    }
    if (comboPtr->disabledArrow != NULL) {
        Blt_FreePicture(comboPtr->disabledArrow);
    }
    if (comboPtr->postedArrow != NULL) {
        Blt_FreePicture(comboPtr->postedArrow);
    }
    if (comboPtr->activeArrow != NULL) {
        Blt_FreePicture(comboPtr->activeArrow);
    }
    Tcl_DeleteCommandFromToken(comboPtr->interp, comboPtr->cmdToken);
    Blt_Free(comboPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewComboEntry --
 *
 *---------------------------------------------------------------------------
 */
static ComboEntry *
NewComboEntry(Tcl_Interp *interp, Tk_Window tkwin)
{
    ComboEntry *comboPtr;
    
    comboPtr = Blt_AssertCalloc(1, sizeof(ComboEntry));

    comboPtr->arrowActiveRelief = TK_RELIEF_SUNKEN;
    comboPtr->arrowBorderWidth = 2;
    comboPtr->arrowRelief = TK_RELIEF_RAISED;
    comboPtr->borderWidth = 2;
    comboPtr->display = Tk_Display(tkwin);
    comboPtr->flags |= (LAYOUT_PENDING|SCROLL_PENDING|EXPORT_SELECTION);
    comboPtr->highlightWidth = 2;
    comboPtr->insertOffTime = 300;
    comboPtr->insertOnTime = 600;
    comboPtr->interp = interp;
    comboPtr->relief = TK_RELIEF_SUNKEN;
    comboPtr->scrollUnits = 2;
    comboPtr->selAnchor = -1;
    comboPtr->selFirst = comboPtr->selLast = -1;
    comboPtr->text = emptyString;
    comboPtr->numScreenBytes = comboPtr->numBytes = 0;
    comboPtr->tkwin = tkwin;
    comboPtr->flags |= ARROW;
    comboPtr->painter = Blt_GetPainter(tkwin, 1.0);
    return comboPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboEntryCmd --
 *
 *      This procedure is invoked to process the "comboentry" command.  See
 *      the user documentation for details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec comboEntryOps[] =
{
    {"activate",  1, ActivateOp,  3, 3, "what",},
    {"button",    2, ButtonOp,    2, 0, "args",},
    {"cget",      2, CgetOp,      3, 3, "option",},
    {"closest",   2, ClosestOp,   3, 3, "x",},
    {"configure", 2, ConfigureOp, 2, 0, "?option value?...",},
    {"delete",    1, DeleteOp,    2, 0, "first ?last?",},
    {"get",       1, GetOp,       2, 2, "",},
    {"icursor",   2, IcursorOp,   3, 3, "index",},
    {"identify",  2, IdentifyOp,  4, 5, "x y",},
    {"index",     3, IndexOp,     3, 3, "index",},
    {"insert",    3, InsertOp,    3, 0, "index string",},
    {"invoke",    3, InvokeOp,    2, 2, "",},
    {"post",      1, PostOp,      2, 2, "",},
    {"redo",      1, RedoOp,      2, 2, "",},
    {"scan",      2, ScanOp,      3, 4, "dragto|mark x",},
    {"see",       3, SeeOp,       3, 3, "index",},
    {"selection", 3, SelectionOp, 2, 0, "args",},
    {"undo",      3, UndoOp,      2, 2, "",},
    {"unpost",    3, UnpostOp,    2, 2, "",},
#ifdef notdef
    {"validate",  1, ValidateOp,  3, 3, "item",},
#endif
    {"xview",     1, XviewOp,     2, 5, "?moveto fract? ?scroll number what?",},
};

static int numComboEntryOps = sizeof(comboEntryOps) / sizeof(Blt_OpSpec);

static int
ComboEntryInstCmdProc(
    ClientData clientData,      /* Information about the widget. */
    Tcl_Interp *interp,         /* Interpreter to report errors back to. */
    int objc,                   /* Number of arguments. */
    Tcl_Obj *const *objv)       /* Argument vector. */
{
    Tcl_ObjCmdProc *proc;
    ComboEntry *comboPtr = clientData;
    int result;

    proc = Blt_GetOpFromObj(interp, numComboEntryOps, comboEntryOps, 
                BLT_OP_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    Tcl_Preserve(comboPtr);
    result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(comboPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboEntryInstCmdDeletedProc --
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
ComboEntryInstCmdDeletedProc(ClientData clientData)
{
    ComboEntry *comboPtr = clientData;  /* Pointer to widget record. */

    if (comboPtr->tkwin != NULL) {
        Tk_Window tkwin;

        tkwin = comboPtr->tkwin;
        comboPtr->tkwin = NULL;
        Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboEntryCmd --
 *
 *      This procedure is invoked to process the TCL command that
 *      corresponds to a widget managed by this module. See the user
 *      documentation for details on what it does.
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
ComboEntryCmd(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    ComboEntry *comboPtr;
    Tk_Window tkwin;
    char *path;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " pathName ?option value?...\"", 
                (char *)NULL);
        return TCL_ERROR;
    }
    /*
     * First time in this interpreter, set up procs and initialize various
     * bindings for the widget.  If the proc doesn't already exist, source
     * it from "$blt_library/bltComboEntry.tcl".  We've deferred sourcing
     * this file until now so that the user could reset the variable
     * $blt_library from within her script.
     */
    if (!Blt_CommandExists(interp, "::blt::ComboEntry::PostMenu")) {
        static char cmd[] = "source [file join $blt_library bltComboEntry.tcl]";

        if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
            char info[200];

            Blt_FormatString(info, 200, "\n    (while loading bindings for %.50s)", 
                    Tcl_GetString(objv[0]));
            Tcl_AddErrorInfo(interp, info);
            return TCL_ERROR;
        }
    }
    path = Tcl_GetString(objv[1]);
    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), path, 
        (char *)NULL);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    comboPtr = NewComboEntry(interp, tkwin);
    Tk_CreateEventHandler(tkwin, EVENT_MASK, ComboEntryEventProc, comboPtr);
    Tk_CreateSelHandler(tkwin, XA_PRIMARY, XA_STRING, ComboEntrySelectionProc,
        comboPtr, XA_STRING);
    Tk_SetClass(tkwin, "BltComboEntry");
    comboPtr->cmdToken = Tcl_CreateObjCommand(interp, path, 
        ComboEntryInstCmdProc, comboPtr, ComboEntryInstCmdDeletedProc);
    Blt_SetWindowInstanceData(tkwin, comboPtr);
    if (ConfigureComboEntry(interp, comboPtr, objc-2, objv+2, 0) != TCL_OK) {
        Tk_DestroyWindow(comboPtr->tkwin);
        return TCL_ERROR;
    }
    if (ConfigureXButton(interp, comboPtr, 0, NULL, 0) != TCL_OK) {
        Tk_DestroyWindow(comboPtr->tkwin);
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objv[1]);
    return TCL_OK;
}

int
Blt_ComboEntryInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {"comboentry", ComboEntryCmd };

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawTextForEntry --
 *
 *      Draw the editable text associated with the entry.  The widget may
 *      be scrolled so the text may be clipped.  We use a temporary pixmap
 *      to draw the visible portion of the text.
 *
 *      We assume that text strings will be small for the most part.  The
 *      bad part of this is that we measure the text string 5 times.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawTextForEntry(ComboEntry *comboPtr, Drawable drawable, int x, int y, int w, int h) 
{
    Blt_FontMetrics fm;
    Pixmap pixmap;
    int insertX;
    int textX, textY;
    Blt_Bg bg;
    GC gc;
    ByteOffset insertOffset, firstOffset, lastOffset;

#define TEXT_FLAGS (TK_PARTIAL_OK | TK_AT_LEAST_ONE)
    if ((h < 2) || (w < 2)) {
        return;
    }
    if (comboPtr->textHeight <= 0) {
        return;
    }
    if (h > comboPtr->entryHeight) {
        h = comboPtr->entryHeight;
    }
    if (comboPtr->image != NULL) {
        int imgWidth;

        imgWidth = comboPtr->textWidth;
        if (comboPtr->scrollX < imgWidth) {
            int imgX, imgY, imgHeight;

            imgX = comboPtr->scrollX;
            imgY = y;
            /* FIXME: Why is image at 0 instead of centered? */
            if (comboPtr->entryHeight > comboPtr->iconHeight) {
                imgY += (comboPtr->entryHeight - comboPtr->iconHeight) / 2;
            }
            imgWidth -= comboPtr->scrollX;
            if (imgWidth > w) {
                imgWidth = w;
            }
            imgHeight = MIN(h, comboPtr->iconHeight);
            Tk_RedrawImage(IconImage(comboPtr->image), imgX, 0, imgWidth, 
                       imgHeight, drawable, x, y);
        }
        return;
    }
    Blt_Font_GetMetrics(comboPtr->font, &fm);
    textY = fm.ascent;
    if (comboPtr->entryHeight > comboPtr->textHeight) {
        textY += (comboPtr->entryHeight - comboPtr->textHeight) / 2;
    }

#ifdef WIN32
    assert(drawable != None);
#endif
    /*
     * Create a pixmap the size of visible text area. This will be used for
     * clipping the scrolled text string.
     */
    pixmap = Blt_GetPixmap(comboPtr->display, Tk_WindowId(comboPtr->tkwin),
        w, h, Tk_Depth(comboPtr->tkwin));

    if ((comboPtr->flags & (FOCUS|READONLY)) == FOCUS) {
        bg = comboPtr->inFocusBg;
        gc = comboPtr->textInFocusGC;
    } else {
        bg = comboPtr->outFocusBg;
        gc = comboPtr->textOutFocusGC;
    }
    /* Text background. */
    if ((w > 0) && (h > 0)) {
        int xOrigin, yOrigin;

        Blt_Bg_GetOrigin(bg, &xOrigin, &yOrigin);
        Blt_Bg_SetOrigin(comboPtr->tkwin, bg, xOrigin + x, yOrigin + y);
        Blt_Bg_FillRectangle(comboPtr->tkwin, pixmap, bg, 0, 0, w, h, 
                0, TK_RELIEF_FLAT);
        Blt_Bg_SetOrigin(comboPtr->tkwin, bg, xOrigin, yOrigin);
    }   
    if (comboPtr->flags & SCROLL_PENDING) {
        int firstX, textWidth;

        /* Find the range of visible characters in both bytes and pixels. */
        comboPtr->firstOffset = comboPtr->lastOffset = 
            Blt_Font_Measure(comboPtr->font, comboPtr->screenText, 
                comboPtr->numScreenBytes, comboPtr->scrollX, 0, &firstX);
        comboPtr->lastOffset +=  
            Blt_Font_Measure(comboPtr->font, 
                comboPtr->screenText + comboPtr->firstOffset, 
                comboPtr->numScreenBytes - comboPtr->firstOffset,
                w, TEXT_FLAGS, &textWidth);
        comboPtr->firstX = firstX;
        comboPtr->lastX = textWidth + firstX;
    }
    /* 
     * The viewport starts somewhere over the first visible character, but
     * not necessarily at the start of the character.  Subtract the
     * viewport offset from the start of the first character.  This is zero
     * or a negative x-coordinate, indicating where start drawing the text
     * so that it's properly clipped by the temporary pixmap.
     */
    textX = comboPtr->firstX - comboPtr->scrollX;
        
    insertX = -1;
    insertOffset = CharIndexToByteOffset(comboPtr->screenText, 
        comboPtr->insertIndex);
    if (((comboPtr->flags & (FOCUS|ICURSOR_ON|DISABLED|READONLY|DISABLED)) 
         == (FOCUS|ICURSOR_ON)) && (comboPtr->selFirst == -1) && 
        (insertOffset >= comboPtr->firstOffset) && 
        (insertOffset <= comboPtr->lastOffset)) {
        insertX = textX;
        if (insertOffset > comboPtr->firstOffset) { 
            insertX += Blt_TextWidth(comboPtr->font, 
                comboPtr->screenText + comboPtr->firstOffset, 
                insertOffset - comboPtr->firstOffset);
        }
        if (insertX > (comboPtr->lastX - comboPtr->firstX)) {
            insertX = -1;
        }
    }

    /*
     *  Text is drawn in (up to) three segments.
     *
     *    1) Any text before the start the selection.  
     *    2) The selected text (drawn with a flat border) 
     *    3) Any text following the selection.  This step will draw the
     *       text string if there is no selection.
     */

    /* Step 1. Draw any text preceding the selection that's still visible
     *         in the viewport. */
    firstOffset = CharIndexToByteOffset(comboPtr->screenText, 
        comboPtr->selFirst);
    lastOffset  = CharIndexToByteOffset(comboPtr->screenText, 
        comboPtr->selLast);

    if (firstOffset >= comboPtr->firstOffset) {
        int numPixels, len, numBytes;
        ByteOffset offset;

        offset = firstOffset;
        if (offset > comboPtr->lastOffset) {
            offset = comboPtr->lastOffset;
        }
        len = offset - comboPtr->firstOffset;
        numBytes = Blt_Font_Measure(comboPtr->font, 
                comboPtr->screenText + comboPtr->firstOffset, len, w, 
                TEXT_FLAGS, &numPixels);
        Blt_Font_Draw(comboPtr->display, pixmap, gc, comboPtr->font, 
                Tk_Depth(comboPtr->tkwin), 0.0f, 
                comboPtr->screenText + comboPtr->firstOffset, numBytes, 
                textX, textY);
        textX += numPixels;
    }
    /* Step 2. Draw the selection itself, if it's visible in the
     *         viewport. Otherwise step 1 drew as much as we need. */
    if ((firstOffset >= 0) && (firstOffset <= comboPtr->lastOffset)) {  
        Blt_Bg bg;
        int numBytes, numPixels;
        ByteOffset first, last;

        /* The background of the selection rectangle is different depending
         * whether the widget has focus or not. */
        bg = comboPtr->selectBg;
        first = firstOffset;
        if (first < comboPtr->firstOffset) {
            first = comboPtr->firstOffset;
        }
        last = lastOffset;
        if (last > comboPtr->lastOffset) {
            last = comboPtr->lastOffset;
        }
        numBytes = Blt_Font_Measure(comboPtr->font, 
                comboPtr->screenText + first, last - first, w, 
                TEXT_FLAGS, &numPixels);
        if ((numPixels > 0) && (h > 0)) {
            Blt_Bg_FillRectangle(comboPtr->tkwin, pixmap, bg, textX, 0, 
                numPixels, h, 0, TK_RELIEF_FLAT);
            Blt_Font_Draw(comboPtr->display, pixmap, comboPtr->selectGC, 
                comboPtr->font, Tk_Depth(comboPtr->tkwin), 0.0f, 
                comboPtr->screenText + first, numBytes, textX, textY);
        }
        textX += numPixels;
    }
    /* Step 3.  Draw any text following the selection that's visible in the
     *          viewport. In the case of no selection, we draw the entire
     *          text string. */
    if (lastOffset < comboPtr->lastOffset) {            
        ByteOffset offset;

        offset = lastOffset;
        if (offset < comboPtr->firstOffset) {
            offset = comboPtr->firstOffset;
        }
        Blt_Font_Draw(comboPtr->display, pixmap, gc, 
                comboPtr->font, Tk_Depth(comboPtr->tkwin), 0.0f, 
                comboPtr->screenText + offset,
                comboPtr->lastOffset - offset, 
                textX, textY);
    }
    /* Draw the insertion cursor, if one is needed. */
    if (insertX >= 0) {
        int y1, y2;

        y1 = 1;
        y2 = h - 2;
        XDrawLine(comboPtr->display, pixmap, comboPtr->insertGC, insertX, y1, 
                insertX, y2);
#ifdef notdef
        XDrawLine(comboPtr->display, pixmap, comboPtr->insertGC, insertX + 1, 
                y1, insertX + 1, y2);
#endif
    }
    XCopyArea(comboPtr->display, pixmap, drawable, gc, 0, 0, w, h, 
        x, y);
    Tk_FreePixmap(comboPtr->display, pixmap);
}

static Blt_Picture
GetArrowPicture(ComboEntry *comboPtr, int w, int h)
{
    Blt_Picture *pictPtr;
    XColor *colorPtr;

    if (comboPtr->flags & POSTED) {
        colorPtr = comboPtr->arrowPostedFgColor;
        pictPtr = &comboPtr->postedArrow;
    } else if (comboPtr->flags & ACTIVE_ARROW) {
        colorPtr = comboPtr->arrowActiveFgColor;
        pictPtr = &comboPtr->activeArrow;
    } else if (comboPtr->flags & DISABLED) {
        colorPtr = comboPtr->arrowDisabledFgColor;
        pictPtr = &comboPtr->disabledArrow;
    } else {
        colorPtr = comboPtr->arrowNormalFgColor;
        pictPtr = &comboPtr->normalArrow;
    }
    if (((*pictPtr) == NULL) ||
        (Blt_Picture_Width(*pictPtr) != w) ||
        (Blt_Picture_Height(*pictPtr) != h)) {
        Blt_Picture picture;
        int ix, iy, ih, iw;

        if (*pictPtr != NULL) {
            Blt_FreePicture(*pictPtr);
        }
        ih = h * 40 / 100;
        iw = w * 80 / 100;
        picture = Blt_CreatePicture(w, h);
        Blt_BlankPicture(picture, 0x0);
        iy = (h - ih) / 2;
        ix = (w - iw) / 2;
        Blt_PaintArrowHead(picture, ix, iy, iw, ih, 
                           Blt_XColorToPixel(colorPtr), ARROW_DOWN);
        *pictPtr = picture;
    }
    return *pictPtr;
}

static void
DrawEntry(ComboEntry *comboPtr, Drawable drawable)
{
    int drawButton;
    int x0, y0, cavityWidth, cavityHeight, tx, ty, w, h;

    cavityWidth = Tk_Width(comboPtr->tkwin) - (2 * comboPtr->inset);
    cavityHeight = Tk_Height(comboPtr->tkwin) - (2 * comboPtr->inset);
    
    /* Background (just inside of focus highlight ring). */
    x0 = y0 = comboPtr->inset;

    /* Label: includes icon and text. */
    if (comboPtr->flags & ARROW) {
        cavityWidth -= comboPtr->arrowWidth;
    }
        
    drawButton = ((comboPtr->flags & XBUTTON) && (comboPtr->numBytes > 0));
#ifdef notdef
    if (drawButton) {
        cavityWidth -= butPtr->width + 2 * butPtr->borderWidth + PADDING(butPtr->padX);
    }
#endif
    if (cavityHeight > comboPtr->entryHeight) {
        y0 += (cavityHeight - comboPtr->entryHeight) / 2;
    }
    /* Draw Icon. */
    if (comboPtr->icon != NULL) {
        int ix, iy, iw, ih;
        
        ix = x0 + IPAD;
        iy = y0 + YPAD;
        iw = MIN(cavityWidth, comboPtr->iconWidth);
        ih = MIN(cavityHeight, comboPtr->iconHeight);
        if (comboPtr->iconHeight < comboPtr->entryHeight) {
            iy += (comboPtr->entryHeight - comboPtr->iconHeight) / 2;
        }
        if ((Blt_IsPicture(IconImage(comboPtr->icon))) && 
            (comboPtr->flags & DISABLED)) {
            Blt_Picture src, dst;

            src = Blt_GetPictureFromPictureImage(IconImage(comboPtr->icon));
            dst = Blt_ClonePicture(src);
            Blt_FadePicture(dst, 0, 0, Blt_Picture_Width(dst),
                            Blt_Picture_Height(dst), 1.0 - (100 / 255.0));
            if (comboPtr->painter == NULL) {
                comboPtr->painter = Blt_GetPainter(comboPtr->tkwin, 1.0);
            }
            Blt_PaintPicture(comboPtr->painter, drawable, dst, 0, 0, iw, ih,
                             ix, iy,0);
            Blt_FreePicture(dst);
        } else {
            Tk_RedrawImage(IconImage(comboPtr->icon), 0, 0, iw, ih, drawable, 
                ix, iy);
        }
        x0 += comboPtr->iconWidth;
        cavityWidth -= comboPtr->iconWidth;
    }
    tx = x0 + IPAD;
    ty = y0 + 1;
    /* Clear button. */
    if (drawButton) {
        Blt_Picture picture;
        XButton *butPtr = &comboPtr->xButton;
        int bx, by, bw, bh;

        if (comboPtr->flags & ACTIVE_BUTTON) {
            if (butPtr->activePicture == NULL) {
                butPtr->activePicture =
                    Blt_PaintDelete(butPtr->width, butPtr->height, 
                                    Blt_XColorToPixel(butPtr->activeBgColor),
                                    Blt_XColorToPixel(butPtr->activeFgColor),
                                    TRUE);
            } 
            picture = butPtr->activePicture;
        } else {
            if (butPtr->normalPicture == NULL) {
                butPtr->normalPicture =
                    Blt_PaintDelete(butPtr->width, butPtr->height, 
                                    Blt_XColorToPixel(butPtr->normalBgColor),
                                    Blt_XColorToPixel(butPtr->normalFgColor),
                                    FALSE);
            } 
            picture = butPtr->normalPicture;
        }
        bw = butPtr->width + 2 * butPtr->borderWidth + PADDING(butPtr->padX);
        bh = butPtr->height + 2 * butPtr->borderWidth + PADDING(butPtr->padY);
        comboPtr->viewWidth -= bw + comboPtr->inset;
        bx = Tk_Width(comboPtr->tkwin) -
            comboPtr->inset - comboPtr->arrowWidth - bw;
        if (bx < 0) {
            bx = comboPtr->inset;
        }
        by = y0;
        cavityWidth -= bw;
        if (comboPtr->entryHeight > bh) {
            by += ((comboPtr->entryHeight - bh) + 1) / 2;
        }
        bx += butPtr->borderWidth + butPtr->padX.side1;
        by += butPtr->borderWidth + butPtr->padY.side1;
        Blt_PaintPicture(comboPtr->painter, drawable, picture, 0, 0, 
                butPtr->width, butPtr->height, bx, by, 0);
        butPtr->x = bx;
        butPtr->y = by;
    }
    /* Arrow. */
    if (comboPtr->flags & ARROW) {
        int aw, ah, ax, ay;
        int relief;
        Blt_Bg bg;

        relief = comboPtr->arrowRelief;
        if (comboPtr->flags & DISABLED) {
            bg = comboPtr->arrowDisabledBg;
        } else if (comboPtr->flags & ACTIVE_ARROW) {
            bg = comboPtr->arrowActiveBg;
        } else {
            bg = comboPtr->arrowNormalBg;
        }
        if (comboPtr->flags & POSTED) {
            relief = comboPtr->arrowActiveRelief;
        }
        ax = Tk_Width(comboPtr->tkwin) - comboPtr->inset -
            comboPtr->arrowWidth;
        ay = y0;
        if (ax < 0) {
            ax = comboPtr->inset;
        }
        aw = comboPtr->arrowWidth - 2 * comboPtr->arrowPad;
        ah = cavityHeight -  2 * comboPtr->arrowPad;
        ax += comboPtr->arrowPad;
        ay += comboPtr->arrowPad;
        if ((aw > 2) && (ah > 2)) {
            Blt_Picture picture;

            Blt_Bg_FillRectangle(comboPtr->tkwin, drawable, bg, ax, ay, 
                aw, ah, comboPtr->arrowBorderWidth, relief);
            ax += comboPtr->arrowBorderWidth + XPAD;
            ay += comboPtr->arrowBorderWidth;
            aw -= 2 * comboPtr->arrowBorderWidth + XPAD;
            ah -= 2 * comboPtr->arrowBorderWidth;
            picture = GetArrowPicture(comboPtr, aw, ah);
            if (comboPtr->painter != NULL) {
                comboPtr->painter = Blt_GetPainter(comboPtr->tkwin, 1.0);
            }
            Blt_PaintPicture(comboPtr->painter, drawable,
                             picture, 0, 0, aw, ah, ax, ay, 0);
        }
        comboPtr->arrowX = ax;
        comboPtr->arrowY = ay;
    }
    comboPtr->viewWidth = cavityWidth;
    if ((cavityWidth > 0) && (cavityHeight > 0)) {
        DrawTextForEntry(comboPtr, drawable, tx, ty, cavityWidth - 4, cavityHeight- 2);
    }

    /* Draw focus highlight ring. */
    if (comboPtr->highlightWidth > 0) {
        GC gc;

        if ((comboPtr->flags & (FOCUS|READONLY)) == FOCUS) {
            gc = comboPtr->highlightGC;
            Tk_DrawFocusHighlight(comboPtr->tkwin, gc, comboPtr->highlightWidth,
                 drawable);
        } else {
            Blt_Bg_DrawFocus(comboPtr->tkwin, comboPtr->normalBg, 
                comboPtr->highlightWidth, drawable);
        }           
    }
    w = Tk_Width(comboPtr->tkwin)  - 2 * comboPtr->highlightWidth;
    h = Tk_Height(comboPtr->tkwin) - 2 * comboPtr->highlightWidth;
    if ((comboPtr->relief != TK_RELIEF_FLAT) && (w > 0) && (h > 0) &&
        (comboPtr->borderWidth > 0)) {
        Blt_Bg_DrawRectangle(comboPtr->tkwin, drawable, 
                comboPtr->normalBg, comboPtr->highlightWidth,
                comboPtr->highlightWidth, w, h, comboPtr->borderWidth,
                comboPtr->relief);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayComboEntry --
 *
 *      This procedure is invoked to display a comboentry widget.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Commands are output to X to display the comboentry.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayComboEntry(ClientData clientData)
{
    ComboEntry *comboPtr = clientData;
    Pixmap drawable;
    int w, h;                   /* Window width and height. */
    Blt_Bg bg;

    comboPtr->flags &= ~REDRAW_PENDING;
    if (comboPtr->tkwin == NULL) {
        return;                 /* Window destroyed (should not get here) */
    }
    w = Tk_Width(comboPtr->tkwin);
    h = Tk_Height(comboPtr->tkwin);
    if ((w <= 1) || (h <=1)) {
        /* Don't bother computing the layout until the window size is
         * something reasonable. */
        return;
    }
    if (comboPtr->flags & LAYOUT_PENDING) {
        ComputeGeometry(comboPtr);
        comboPtr->flags |= SCROLL_PENDING;
    }
    if (!Tk_IsMapped(comboPtr->tkwin)) {
        /* The widget's window isn't displayed, so don't bother drawing
         * anything.  By getting this far, we've at least computed the
         * coordinates of the comboentry's new layout.  */
        return;
    }
    /*
     * Create a pixmap the size of the window for double buffering.
     */
    if ((w > 0) && (h > 0)) {
        drawable = Blt_GetPixmap(comboPtr->display,
                                 Tk_WindowId(comboPtr->tkwin),
                                 w, h, Tk_Depth(comboPtr->tkwin));
#ifdef WIN32
        assert(drawable != None);
#endif
        bg = (comboPtr->flags & FOCUS) ? comboPtr->inFocusBg :
            comboPtr->outFocusBg;
        Blt_Bg_FillRectangle(comboPtr->tkwin, drawable, bg, 0, 0, w, h, 0, 
                             TK_RELIEF_FLAT);
        DrawEntry(comboPtr, drawable);
        XCopyArea(comboPtr->display, drawable, Tk_WindowId(comboPtr->tkwin),
                  comboPtr->highlightGC, 0, 0, w, h, 0, 0);
        Tk_FreePixmap(comboPtr->display, drawable);
    }

    if (comboPtr->flags & SCROLL_PENDING) {
        if (comboPtr->scrollCmdObjPtr != NULL) {
            Blt_UpdateScrollbar(comboPtr->interp, comboPtr->scrollCmdObjPtr,
                comboPtr->scrollX, comboPtr->scrollX + comboPtr->viewWidth,
                comboPtr->textWidth);
        }
        comboPtr->flags &= ~SCROLL_PENDING;
    }
    if (comboPtr->flags & MODIFIED) {
        GenerateModifiedEvent(comboPtr);
    }
}

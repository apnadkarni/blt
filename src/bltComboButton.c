/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltComboButton.c --
 *
 * This module implements a combo button widget for the BLT toolkit.
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

#include "bltAlloc.h"
#include "bltChain.h"
#include "bltHash.h"
#include "bltFont.h"
#include "bltText.h"
#include "bltImage.h"
#include "bltBg.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define YPAD            3               /* External pad between components. */
#define XPAD            3               /* External pad between border and
                                         * button. */
#define MENU_EVENT_MASK (ExposureMask|StructureNotifyMask)

#define ARROW_WIDTH      13
#define ARROW_HEIGHT    13

#define STATE_NORMAL    (0)             /* Draw widget normally. */
#define STATE_ACTIVE    (1<<0)          /* Widget is currently active. */
#define STATE_DISABLED  (1<<1)          /* Widget is disabled. */
#define STATE_POSTED    (1<<2)          /* Widget is currently posting its
                                         * menu. */
#define STATE_MASK      (STATE_ACTIVE|STATE_DISABLED|STATE_POSTED)
#define REDRAW_PENDING  (1<<3)          /* Widget is scheduled to be
                                         * redrawn. */
#define LAYOUT_PENDING  (1<<4)          /* Widget layout needs to be
                                         * recomputed. */
#define FOCUS           (1<<5)          /* Widget has focus. */

#define ARROW           (1<<8)
#define TEXT_VAR_TRACED (1<<16)
#define ICON_VAR_TRACED (1<<17)


#define TRACE_VAR_FLAGS         (TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|\
                                 TCL_TRACE_UNSETS)

#define DEF_ACTIVEBACKGROUND    STD_ACTIVE_BACKGROUND
#define DEF_ACTIVEFOREGROUND    STD_ACTIVE_FOREGROUND
#define DEF_BORDERWIDTH         "2"
#define DEF_CMD                 ((char *)NULL)
#define DEF_CURSOR              ((char *)NULL)
#define DEF_DIRECTION           ((char *)NULL)
#define DEF_DISABLED_BG         STD_DISABLED_BACKGROUND
#define DEF_DISABLED_FG         STD_DISABLED_FOREGROUND
#define DEF_ENTRY_BG            RGB_GREY90
#define DEF_FONT                STD_FONT
#define DEF_HEIGHT              "0"
#define DEF_HIGHLIGHTBACKGROUND ""
#define DEF_HIGHLIGHTCOLOR      "black"
#define DEF_HIGHLIGHTTHICKNESS  "2"
#define DEF_ICON                ((char *)NULL)
#define DEF_ICON_VARIABLE       ((char *)NULL)
#define DEF_IMAGE               ((char *)NULL)
#define DEF_ARROW_ON            "0"
#define DEF_ARROW_WIDTH         "0"
#define DEF_INDICTOR_ACTIVE_FG  STD_ACTIVE_FOREGROUND
#define DEF_ARROW_BORDERWIDTH   "2"
#define DEF_ARROW_DISABLED_FG   STD_DISABLED_FOREGROUND
#define DEF_ARROW_POSTED_FG     STD_DISABLED_FOREGROUND
#define DEF_ARROW_NORMAL_FG     STD_NORMAL_FOREGROUND
#define DEF_ARROW_RELIEF        "flat"
#define DEF_JUSTIFY             "left"
#define DEF_MENU                ((char *)NULL)
#define DEF_MENU_ANCHOR         "sw"
#define DEF_NORMAL_BG           STD_NORMAL_BACKGROUND
#define DEF_NORMAL_FG           STD_NORMAL_FOREGROUND
#define DEF_POSTED_BG           RGB_SKYBLUE4
#define DEF_POSTED_FG           RGB_WHITE
#define DEF_NORMAL_RELIEF       "raised"
#define DEF_POSTED_RELIEF       "flat"
#define DEF_ACTIVERELIEF        "raised"
#define DEF_STATE               "normal"
#define DEF_TAKE_FOCUS          "1"
#define DEF_TEXT                ((char *)NULL)
#define DEF_TEXT_VARIABLE       ((char *)NULL)
#define DEF_TYPE                "button"
#define DEF_UNDERLINE           "-1"
#define DEF_WIDTH               "0"

static Tcl_VarTraceProc TextVarTraceProc;
static Tcl_VarTraceProc IconVarTraceProc;

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

static Blt_OptionFreeProc FreeTextVarProc;
static Blt_OptionParseProc ObjToTextVarProc;
static Blt_OptionPrintProc TextVarToObjProc;
static Blt_CustomOption textVarOption = {
    ObjToTextVarProc, TextVarToObjProc, FreeTextVarProc, (ClientData)0
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

static const char *emptyString = "";

/*
 * Icon --
 *
 *      Since instances of the same Tk image can be displayed in different
 *      windows with possibly different color palettes, Tk internally stores
 *      each instance in a linked list.  But if the instances are used in the
 *      same widget and therefore use the same color palette, this adds a lot
 *      of overhead, especially when deleting instances from the linked list.
 *
 *      For the combobutton widget, we never need more than a single instance
 *      of an image, regardless of how many times it's used.  Cache the image,
 *      maintaining a reference count for each image used in the widget.  It's
 *      likely that the comboview widget will use many instances of the same
 *      image.
 */

typedef struct _Icon {
    Tk_Image tkImage;                   /* The Tk image being cached. */
    short int width, height;            /* Dimensions of the cached image. */
} *Icon;

#define IconHeight(i)   ((i)->height)
#define IconWidth(i)    ((i)->width)
#define IconImage(i)    ((i)->tkImage)
#define IconName(i)     (Blt_Image_Name((i)->tkImage))

typedef struct  {
    Tcl_Interp *interp;                 /* Interpreter associated with
                                         * button. */
    Tk_Window tkwin;                    /* Window that embodies the combo
                                         * button. If NULL, indicates the
                                         * window has been destroyed but the
                                         * data structures haven't yet been
                                         * cleaned up.*/
    Display *display;                   /* Display containing widget.  Used,
                                         * among other things, so that
                                         * resources can be freed even after
                                         * tkwin has gone away. */
    Tcl_Command cmdToken;               /* Token for widget command. */
    int reqWidth, reqHeight;
    int relief, postedRelief, activeRelief;
    int borderWidth;
    Blt_Bg normalBg;
    Blt_Bg activeBg;
    Blt_Bg postedBg;
    Blt_Bg disabledBg;
    Tcl_Obj *takeFocusObjPtr;           /* Value of -takefocus option; not
                                         * used in the C code, but used by
                                         * keyboard traversal scripts. */

    /*
     * In/Out Focus Highlight Ring:
     */
    XColor *highlightColor;
    GC highlightGC;
    Blt_Bg highlightBg;
    GC highlightBgGC;
    int highlightWidth;

    /* 
     * The button contains an optional icon and text string. 
     */
    Icon icon;                          /* If non-NULL, image to be displayed
                                         * in button. Its value may be
                                         * overridden by the -iconvariable
                                         * option. */

    Tcl_Obj *iconVarObjPtr;             /* Name of TCL variable.  If non-NULL,
                                         * this variable contains the name of
                                         * an image representing the icon.
                                         * This overrides the value of the
                                         * above field. */
    Icon image;                         /* If non-NULL, image to be displayed
                                         * instead of text in the button. */
    const char *text;                   /* Text string to be displayed in the
                                         * button if an image has no been
                                         * designated. Its value is overridden
                                         * by the -textvariable option. */
    Tcl_Obj *textVarObjPtr;             /* Name of TCL variable.  If non-NULL,
                                         * this variable contains the text
                                         * string to be displayed in the
                                         * button. This overrides the above
                                         * field. */
    Blt_Font font;                      /* Font of text to be display in
                                         * button. */
    Tk_Justify justify;                 /* Justification to use for text
                                         * within the button. */
    int textLen;                        /* # bytes of text. */
    int underline;                      /* Character index of character to be
                                         * underlined. If -1, no character is
                                         * underlined. */
    XColor *textNormalColor;
    XColor *textActiveColor;
    XColor *textPostedColor;
    XColor *textDisabledColor;

    /*  
     * Arrow (button) Information:
     *
     * The arrow is a button with an optional 3D border.
     */
    int arrowBW;
    int arrowRelief;
    int reqArrowWidth;

    Tk_Cursor cursor;                   /* Current cursor or * None. */
    int prefWidth;                      /* Desired width of window, measured
                                         * in average characters. */
    int inset;
    short int arrowWidth, arrowHeight;
    short int iconWidth, iconHeight;
    short int textWidth, textHeight;
    short int width, height;
    Tcl_Obj *cmdObjPtr;                 /* If non-NULL, command to be executed
                                         * when this menu is posted. */
    Tcl_Obj *menuObjPtr;        
    Tk_Window menuWin;
    Tcl_Obj *postCmdObjPtr;             /* If non-NULL, command to be executed
                                         * when this menu is posted. */
    unsigned int flags;
} ComboButton;

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground",
        "ActiveBackground", DEF_ACTIVEBACKGROUND, 
        Blt_Offset(ComboButton, activeBg),0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground",
        "ActiveForeground", DEF_ACTIVEFOREGROUND, 
        Blt_Offset(ComboButton, textActiveColor), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "Relief", 
        DEF_ACTIVERELIEF, Blt_Offset(ComboButton, activeRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-arrowon", "arrowOn", "ArrowOn", 
        DEF_ARROW_ON, Blt_Offset(ComboButton, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)ARROW},
    {BLT_CONFIG_PIXELS_NNEG, "-arrowborderwidth", "arrowBorderWidth", 
        "ArrowBorderWidth", DEF_ARROW_BORDERWIDTH, 
        Blt_Offset(ComboButton, arrowBW), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-arrowrelief", "arrowRelief","ArrowRelief",
        DEF_ARROW_RELIEF, Blt_Offset(ComboButton, arrowRelief), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-arrowwidth", "arrowWidth","ArrowWidth",
        DEF_ARROW_WIDTH, Blt_Offset(ComboButton, reqArrowWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_NORMAL_BG, Blt_Offset(ComboButton, normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_BORDERWIDTH, Blt_Offset(ComboButton, borderWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", 
        DEF_CMD, Blt_Offset(ComboButton, cmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
        DEF_CURSOR, Blt_Offset(ComboButton, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-disabledbackground", "disabledBackground",
        "DisabledBackground", DEF_DISABLED_BG, 
        Blt_Offset(ComboButton, disabledBg), 0},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground",
        "DisabledForeground", DEF_DISABLED_FG, 
        Blt_Offset(ComboButton, textDisabledColor), 0},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_FONT, 
        Blt_Offset(ComboButton, font), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
        DEF_NORMAL_FG, Blt_Offset(ComboButton, textNormalColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height", DEF_HEIGHT, 
        Blt_Offset(ComboButton, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-highlightbackground", "highlightBackground", 
        "HighlightBackground", DEF_HIGHLIGHTBACKGROUND, 
        Blt_Offset(ComboButton, highlightBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
        DEF_HIGHLIGHTCOLOR, Blt_Offset(ComboButton, highlightColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
        "HighlightThickness", DEF_HIGHLIGHTTHICKNESS, 
        Blt_Offset(ComboButton, highlightWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "Icon", DEF_ICON, 
        Blt_Offset(ComboButton, icon), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_CUSTOM, "-iconvariable", "iconVariable", "IconVariable", 
        DEF_TEXT_VARIABLE, Blt_Offset(ComboButton, iconVarObjPtr), 
        BLT_CONFIG_NULL_OK, &iconVarOption},
    {BLT_CONFIG_CUSTOM, "-image", "image", "Image", DEF_IMAGE, 
        Blt_Offset(ComboButton, image), BLT_CONFIG_NULL_OK, &iconOption},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_JUSTIFY, 
        Blt_Offset(ComboButton, justify), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-menu", "menu", "Menu", DEF_MENU, 
        Blt_Offset(ComboButton, menuObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-postcommand", "postCommand", "PostCommand", 
        DEF_CMD, Blt_Offset(ComboButton, postCmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-postedbackground", "postedBackground",
        "PostedBackground", DEF_POSTED_BG, Blt_Offset(ComboButton, postedBg),0},
    {BLT_CONFIG_COLOR, "-postedforeground", "postedForeground",
        "PostedForeground", DEF_POSTED_FG, 
        Blt_Offset(ComboButton, textPostedColor), 0},
    {BLT_CONFIG_RELIEF, "-postedrelief", "postedRelief", "PostedRelief", 
        DEF_POSTED_RELIEF, Blt_Offset(ComboButton, postedRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_NORMAL_RELIEF, 
        Blt_Offset(ComboButton, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_STATE, 
        Blt_Offset(ComboButton, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        &stateOption},
    {BLT_CONFIG_OBJ, "-takefocus", "takeFocus", "TakeFocus", DEF_TAKE_FOCUS, 
        Blt_Offset(ComboButton, takeFocusObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-text", "text", "Text", DEF_TEXT, 
        Blt_Offset(ComboButton, text), 0, &textOption},
    {BLT_CONFIG_CUSTOM, "-textvariable", "textVariable", "TextVariable", 
        DEF_TEXT_VARIABLE, Blt_Offset(ComboButton, textVarObjPtr), 
        BLT_CONFIG_NULL_OK, &textVarOption},
    {BLT_CONFIG_INT, "-underline", "underline", "Underline", DEF_UNDERLINE, 
        Blt_Offset(ComboButton, underline), BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width", DEF_WIDTH, 
        Blt_Offset(ComboButton, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
        0, 0}
};

static Tcl_IdleProc DisplayComboButton;
static Tcl_FreeProc DestroyComboButton;
static Tk_EventProc ComboButtonEventProc;
static Tk_EventProc MenuEventProc;
static Tcl_ObjCmdProc ComboButtonInstCmdProc;
static Tcl_CmdDeleteProc ComboButtonInstCmdDeletedProc;

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *      Tells the Tk dispatcher to call the combobutton display routine at the
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
EventuallyRedraw(ComboButton *comboPtr) 
{
    if ((comboPtr->tkwin != NULL) && ((comboPtr->flags & REDRAW_PENDING) == 0)){
        comboPtr->flags |= REDRAW_PENDING;
        Tcl_DoWhenIdle(DisplayComboButton, comboPtr);
    }
}

static int
UpdateTextVar(Tcl_Interp *interp, ComboButton *comboPtr) 
{
    Tcl_Obj *resultObjPtr, *objPtr;
        
    objPtr = Tcl_NewStringObj(comboPtr->text, comboPtr->textLen);
    Tcl_IncrRefCount(objPtr);
    resultObjPtr = Tcl_ObjSetVar2(interp, comboPtr->textVarObjPtr, NULL, 
        objPtr, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    Tcl_DecrRefCount(objPtr);
    if (resultObjPtr == NULL) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
UpdateIconVar(Tcl_Interp *interp, ComboButton *comboPtr) 
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
FreeIcon(ComboButton *comboPtr, Icon icon)
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
SetTextFromObj(ComboButton *comboPtr, Tcl_Obj *objPtr) 
{
    int numBytes;
    const char *string;
            
    if (comboPtr->text != emptyString) {
        Blt_Free(comboPtr->text);
    }
    string = Tcl_GetStringFromObj(objPtr, &numBytes);
    comboPtr->text = Blt_AssertMalloc(numBytes + 1);
    strcpy((char *)comboPtr->text, string);
    comboPtr->textLen = numBytes;
    comboPtr->flags |= LAYOUT_PENDING;
    comboPtr->underline = -1;
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
    ComboButton *comboPtr = clientData;

    comboPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(comboPtr);
}

static int
GetIconFromObj(
    Tcl_Interp *interp, 
    ComboButton *comboPtr, 
    Tcl_Obj *objPtr, 
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

/*
 *---------------------------------------------------------------------------
 *
 * ComboButtonEventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for various events on
 *      combobutton widgets.
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
ComboButtonEventProc(ClientData clientData, XEvent *eventPtr)
{
    ComboButton *comboPtr = clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedraw(comboPtr);
        }
    } else if (eventPtr->type == ConfigureNotify) {
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
        EventuallyRedraw(comboPtr);
    } else if (eventPtr->type == DestroyNotify) {
        if (comboPtr->tkwin != NULL) {
            comboPtr->tkwin = NULL; 
        }
        if (comboPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayComboButton, comboPtr);
        }
        Tcl_EventuallyFree(comboPtr, DestroyComboButton);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MenuEventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for various events on
 *      sub-menus of combobutton widgets.
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
MenuEventProc(ClientData clientData, XEvent *eventPtr)
{
    ComboButton *comboPtr = clientData;

    if (eventPtr->type == DestroyNotify) {
        comboPtr->flags &= ~STATE_MASK;
        comboPtr->flags |= STATE_NORMAL;
        comboPtr->menuWin = NULL;
    }  else if (eventPtr->type == UnmapNotify) {
        comboPtr->flags &= ~STATE_MASK;
        comboPtr->flags |= STATE_NORMAL;
    } else if (eventPtr->type == MapNotify) {
        comboPtr->flags &= ~STATE_MASK;
        comboPtr->flags |= STATE_POSTED;
    } 
    EventuallyRedraw(comboPtr);
}

/*
 *---------------------------------------------------------------------------
 * 
 * TextVarTraceProc --
 *
 *      This procedure is invoked when someone changes the state variable
 *      associated with a combobutton.  
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
    ComboButton *comboPtr = clientData;

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
            comboPtr->flags |= TEXT_VAR_TRACED;
        }
        return NULL;
    }
    if (flags & TCL_TRACE_WRITES) {
        Tcl_Obj *valueObjPtr;

        /*
         * Update the combobutton's text with the value of the variable,
         * unless the widget already has that value (this happens when the
         * variable changes value because we changed it because someone typed
         * in the entry).
         */
        valueObjPtr = Tcl_ObjGetVar2(interp, comboPtr->textVarObjPtr, NULL, 
                TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
        if (valueObjPtr == NULL) {
            return GetInterpResult(interp);
        } else {
            SetTextFromObj(comboPtr, valueObjPtr);
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
    ComboButton *comboPtr = clientData;

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
            comboPtr->flags |= ICON_VAR_TRACED;
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
FreeIconVarProc(
    ClientData clientData,
    Display *display,                   /* Not used. */
    char *widgRec,
    int offset)
{
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);

    if (*objPtrPtr != NULL) {
        ComboButton *comboPtr = (ComboButton *)widgRec;

        Tcl_UntraceVar(comboPtr->interp, Tcl_GetString(*objPtrPtr), 
                TRACE_VAR_FLAGS, IconVarTraceProc, comboPtr);
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
 *      The return value is a standard TCL result.  The color pointer is
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
    ComboButton *comboPtr = (ComboButton *)(widgRec);
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
 * IconVarToObjProc --
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
        ComboButton *comboPtr = (ComboButton *)(widgRec);
        char *varName;

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
 * ObjToTextVarProc --
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
ObjToTextVarProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to report results. */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing style. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    ComboButton *comboPtr = (ComboButton *)(widgRec);
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);
    char *varName;
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
            if (UpdateTextVar(interp, comboPtr) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    *objPtrPtr = objPtr;
    Tcl_IncrRefCount(objPtr);
    Tcl_TraceVar(interp, varName, TRACE_VAR_FLAGS, TextVarTraceProc, comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextVarToObjProc --
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
    ComboButton *comboPtr = (ComboButton *)(widgRec);
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    char *string;
    int flag;

    string = Tcl_GetString(objPtr);
    if (strcmp(string, "disabled") == 0) {
        flag = STATE_DISABLED;
    } else if (strcmp(string, "normal") == 0) {
        flag = STATE_NORMAL;
    } else if (strcmp(string, "active") == 0) {
        flag = STATE_ACTIVE;
    } else {
        Tcl_AppendResult(interp, "unknown state \"", string, 
            "\": should be active, disabled, or normal.", (char *)NULL);
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

    switch (state & STATE_MASK) {
    case STATE_NORMAL:          string = "normal";      break;
    case STATE_ACTIVE:          string = "active";      break;
    case STATE_POSTED:          string = "posted";      break;
    case STATE_DISABLED:        string = "disabled";    break;
    default:                    string = Blt_Itoa(state & STATE_MASK);
                break;
    }
    return Tcl_NewStringObj(string, -1);
}

/*ARGSUSED*/
static void
FreeIconProc(
    ClientData clientData,
    Display *display,                   /* Not used. */
    char *widgRec,
    int offset)
{
    Icon icon = *(Icon *)(widgRec + offset);

    if (icon != NULL) {
        ComboButton *comboPtr = (ComboButton *)widgRec;

        FreeIcon(comboPtr, icon);
    }
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
    Tcl_Interp *interp,                 /* Interpreter to report results. */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* Tcl_Obj representing the new
                                         * value. */
    char *widgRec,
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    ComboButton *comboPtr = (ComboButton *)widgRec;
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
        if (UpdateIconVar(interp, comboPtr) != TCL_OK) {
            return TCL_ERROR;
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
FreeTextProc(ClientData clientData, Display *display, char *widgRec, int offset)
{
    ComboButton *comboPtr = (ComboButton *)(widgRec);

    if (comboPtr->text != emptyString) {
        Blt_Free(comboPtr->text);
        comboPtr->text = emptyString;
        comboPtr->textLen = 0;
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
    ComboButton *comboPtr = (ComboButton *)(widgRec);

    if (comboPtr->text != emptyString) {
        Blt_Free(comboPtr->text);
        comboPtr->text = emptyString;
        comboPtr->textLen = 0;
    }
    SetTextFromObj(comboPtr, objPtr);
    if (comboPtr->textVarObjPtr != NULL) {
        if (UpdateTextVar(interp, comboPtr) != TCL_OK) {
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
    ComboButton *comboPtr = (ComboButton *)(widgRec);

    return Tcl_NewStringObj(comboPtr->text, comboPtr->textLen);
}

/* 
 *---------------------------------------------------------------------------
 *  inset
 *  YPAD
 *  max of icon/text/image/button
 *  YPAD
 *  inset
 *
 * |i|x|icon|x|text/image|x|B|p|arrow|p|B|x|i|
 * |i|x|icon|x|text/image|x|i|                  (without arrow)
 * |i|x|text/image|x|B|p|arrow|p|B|x|i|         (without icon)
 * |i|x|text/image|x|i|                         (without arrow & icon)
 *  
 * i = inset (highlight thickness + combobutton borderwidth)
 * x = xpad
 * p = arrow button pad
 * B = borderwidth
 *---------------------------------------------------------------------------
 */
static void
ComputeGeometry(ComboButton *comboPtr)
{
    int w, h;

    /* Determine the height of the button.  It's the maximum height of all
     * it's components: icon, label, and button. */
    comboPtr->iconWidth = comboPtr->iconHeight = 0;
    comboPtr->textWidth = comboPtr->textHeight = 0;
    comboPtr->arrowWidth = comboPtr->arrowHeight = 0;
    comboPtr->inset = comboPtr->arrowWidth + comboPtr->highlightWidth;

    w = h = 0;
    if (comboPtr->icon != NULL) {
        comboPtr->iconWidth  = IconWidth(comboPtr->icon);
        comboPtr->iconHeight = IconHeight(comboPtr->icon);
    }
    w += comboPtr->iconWidth;
    if (h < comboPtr->iconHeight) {
        h = comboPtr->iconHeight;
    }
    if (comboPtr->image != NULL) {
        comboPtr->textWidth  = IconWidth(comboPtr->image);
        comboPtr->textHeight = IconHeight(comboPtr->image);
    } else if (comboPtr->text != NULL) {
        unsigned int tw, th;

        if (comboPtr->text[0] == '\0') {
            Blt_FontMetrics fm;

            Blt_Font_GetMetrics(comboPtr->font, &fm);
            comboPtr->textHeight = fm.linespace;
        } else {
            Blt_GetTextExtents(comboPtr->font, 0, comboPtr->text, 
                               comboPtr->textLen, &tw, &th);
            comboPtr->textWidth  = tw;
            if (comboPtr->underline >= 0) {
                th += 2;
            }
            comboPtr->textHeight = th;
        }
    }
    w += comboPtr->textWidth;
    if ((comboPtr->iconWidth > 0) && (comboPtr->textWidth > 0)) {
        w += XPAD;
    }
    if (h < comboPtr->textHeight) {
        h = comboPtr->textHeight;
    }
    if (comboPtr->flags & ARROW) {
        comboPtr->arrowHeight = ARROW_HEIGHT;
        if (comboPtr->reqArrowWidth > 0) {
            comboPtr->arrowWidth = comboPtr->reqArrowWidth;
        } else {
            comboPtr->arrowWidth = Blt_TextWidth(comboPtr->font, "0", 1) + 2;
        }
        comboPtr->arrowWidth  += 2 * comboPtr->arrowBW;
        comboPtr->arrowHeight += 2 * comboPtr->arrowBW;
        if (h < comboPtr->arrowHeight) {
            h = comboPtr->arrowHeight;
        }
        w += comboPtr->arrowWidth + XPAD;
    }
    w += 2 * (comboPtr->inset + XPAD);
    h += 2 * (comboPtr->inset + YPAD);
    comboPtr->width  = w;
    comboPtr->height = h;
    if (comboPtr->reqWidth > 0) {
        w = comboPtr->reqWidth;
    }
    if (comboPtr->reqHeight > 0) {
        h = comboPtr->reqHeight;
    }
    if ((w != Tk_ReqWidth(comboPtr->tkwin)) || 
        (h != Tk_ReqHeight(comboPtr->tkwin))) {
        Tk_GeometryRequest(comboPtr->tkwin, w, h);
    }
    comboPtr->flags &= ~LAYOUT_PENDING;
}

static int
ConfigureComboButton(Tcl_Interp *interp, ComboButton *comboPtr, int objc,
                     Tcl_Obj *const *objv, int flags)
{
    unsigned int gcMask;
    XGCValues gcValues;
    GC newGC;

    if (Blt_ConfigureWidgetFromObj(interp, comboPtr->tkwin, configSpecs, objc, 
                objv, (char *)comboPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }

    /* Focus highlight GCs */
    gcMask = GCForeground;
    gcValues.foreground = comboPtr->highlightColor->pixel;
    newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    if (comboPtr->highlightGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->highlightGC);
    }
    comboPtr->highlightGC = newGC;
    ComputeGeometry(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 *      Activates the combo button.
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      pathName activate
 *
 *---------------------------------------------------------------------------
 */
static int
ActivateOp(ComboButton *comboPtr, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    if (comboPtr->flags & (STATE_POSTED|STATE_DISABLED)) {
        return TCL_OK;                  /* Writing is currently disabled. */
    }
    comboPtr->flags |= STATE_ACTIVE;
    EventuallyRedraw(comboPtr);
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
 *      pathName cget option
 *
 *---------------------------------------------------------------------------
 */
static int
CgetOp(ComboButton *comboPtr, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    iconOption.clientData = comboPtr;
    return Blt_ConfigureValueFromObj(interp, comboPtr->tkwin, configSpecs,
        (char *)comboPtr, objv[2], BLT_CONFIG_OBJV_ONLY);
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
 *      pathName configure ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ComboButton *comboPtr, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    int result;

    iconOption.clientData = comboPtr;
    if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, configSpecs, 
                (char *)comboPtr, (Tcl_Obj *)NULL,  BLT_CONFIG_OBJV_ONLY);
    } else if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, configSpecs, 
                (char *)comboPtr, objv[2], BLT_CONFIG_OBJV_ONLY);
    }
    Tcl_Preserve(comboPtr);
    result = ConfigureComboButton(interp, comboPtr, objc - 2, objv + 2, 
                BLT_CONFIG_OBJV_ONLY);
    Tcl_Release(comboPtr);
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    comboPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeactivateOp --
 *
 *      Activates
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      pathName activate bool
 *
 *---------------------------------------------------------------------------
 */
static int
DeactivateOp(ComboButton *comboPtr, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    if (comboPtr->flags & (STATE_POSTED|STATE_DISABLED)) {
        return TCL_OK;                  /* Writing is currently disabled. */
    }
    comboPtr->flags &= ~STATE_ACTIVE;
    EventuallyRedraw(comboPtr);
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
 *  pathName invoke item 
 *
 *---------------------------------------------------------------------------
 */
static int
InvokeOp(ComboButton *comboPtr, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    int result;

    if (comboPtr->flags & STATE_DISABLED) {
        return TCL_OK;                  /* Item is currently disabled. */
    }
    result = TCL_OK;
    if (comboPtr->cmdObjPtr != NULL) {
        Tcl_Preserve(comboPtr);
        Tcl_IncrRefCount(comboPtr->cmdObjPtr);
        result = Tcl_EvalObjEx(interp, comboPtr->cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(comboPtr->cmdObjPtr);
        Tcl_Release(comboPtr);
    }
    return result;
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
 *  .ce post 
 *
 *---------------------------------------------------------------------------
 */
static int
PostOp(ComboButton *comboPtr, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    char *menuName;
    Tk_Window tkwin;
    
    if (comboPtr->flags & (STATE_POSTED|STATE_DISABLED)) {
        return TCL_OK;                  /* Button's menu is currently posted
                                         * or entry is disabled. */
    }
    if (comboPtr->menuObjPtr == NULL) {
        return TCL_OK;
    }
    menuName = Tcl_GetString(comboPtr->menuObjPtr);
    tkwin = Tk_NameToWindow(interp, menuName, comboPtr->tkwin);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    if (Tk_Parent(tkwin) != comboPtr->tkwin) {
        Tcl_AppendResult(interp, "can't post \"", Tk_PathName(tkwin), 
                "\": it isn't a descendant of ", Tk_PathName(comboPtr->tkwin),
                (char *)NULL);
        return TCL_ERROR;
    }

    if (comboPtr->menuWin != NULL) {
        Tk_DeleteEventHandler(comboPtr->menuWin, MENU_EVENT_MASK, 
                MenuEventProc, comboPtr);
    } 
    comboPtr->menuWin = tkwin;
    Tk_CreateEventHandler(tkwin, MENU_EVENT_MASK, MenuEventProc, comboPtr);

    if (comboPtr->postCmdObjPtr) {
        int result;

        Tcl_Preserve(comboPtr);
        Tcl_IncrRefCount(comboPtr->postCmdObjPtr);
        result = Tcl_EvalObjEx(interp, comboPtr->postCmdObjPtr,TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(comboPtr->postCmdObjPtr);
        Tcl_Release(comboPtr);
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (Tk_IsMapped(comboPtr->tkwin)) {
        Tcl_Obj *cmdObjPtr, *objPtr;
        int result;

        cmdObjPtr = Tcl_DuplicateObj(comboPtr->menuObjPtr);
        objPtr = Tcl_NewStringObj("post", 4);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        Tcl_IncrRefCount(cmdObjPtr);
        Tcl_Preserve(comboPtr);
        result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_Release(comboPtr);
        Tcl_DecrRefCount(cmdObjPtr);
        if (result == TCL_OK) {
            comboPtr->flags &= ~STATE_MASK;
            comboPtr->flags |= STATE_POSTED;
        }
        EventuallyRedraw(comboPtr);
        return result;
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
 *  .ce unpost
 *
 *---------------------------------------------------------------------------
 */
static int
UnpostOp(
    ComboButton *comboPtr, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    if ((comboPtr->menuObjPtr != NULL) && (comboPtr->flags & STATE_POSTED)) {
        char *menuName;
        Tk_Window menuWin;

        comboPtr->flags &= ~STATE_MASK;
        comboPtr->flags |= STATE_NORMAL;
        menuName = Tcl_GetString(comboPtr->menuObjPtr);
        menuWin = Tk_NameToWindow(interp, menuName, comboPtr->tkwin);
        if (menuWin == NULL) {
            return TCL_ERROR;
        }
        if (Tk_Parent(menuWin) != comboPtr->tkwin) {
            Tcl_AppendResult(interp, "can't unpost \"", Tk_PathName(menuWin), 
                        "\": it isn't a descendant of ", 
                        Tk_PathName(comboPtr->tkwin), (char *)NULL);
            return TCL_ERROR;
        }
        if (Tk_IsMapped(menuWin)) {
            Tk_UnmapWindow(menuWin);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyComboButton --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the internal structure of the widget at a safe time (when
 *      no-one is using it anymore).
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Everything associated with the widget is freed.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyComboButton(DestroyData dataPtr) /* Pointer to the widget record. */
{
    ComboButton *comboPtr = (ComboButton *)dataPtr;

    iconOption.clientData = comboPtr;
    Blt_FreeOptions(configSpecs, (char *)comboPtr, comboPtr->display, 0);
    if (comboPtr->highlightGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->highlightGC);
    }
    if (comboPtr->menuWin != NULL) {
        Tk_DeleteEventHandler(comboPtr->menuWin, MENU_EVENT_MASK, 
                MenuEventProc, comboPtr);
    }
    Tcl_DeleteCommandFromToken(comboPtr->interp, comboPtr->cmdToken);
    Blt_Free(comboPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewComboButton --
 *
 *---------------------------------------------------------------------------
 */
static ComboButton *
NewComboButton(Tcl_Interp *interp, Tk_Window tkwin)
{
    ComboButton *comboPtr;

    comboPtr = Blt_AssertCalloc(1, sizeof(ComboButton));

    comboPtr->borderWidth = 1;
    comboPtr->display = Tk_Display(tkwin);
    comboPtr->flags = (LAYOUT_PENDING | STATE_NORMAL);
    comboPtr->highlightWidth = 2;
    comboPtr->arrowBW = 2;
    comboPtr->arrowRelief = TK_RELIEF_FLAT;
    comboPtr->interp = interp;
    comboPtr->relief = TK_RELIEF_RAISED;
    comboPtr->postedRelief = TK_RELIEF_FLAT;
    comboPtr->activeRelief = TK_RELIEF_RAISED;
    comboPtr->text = emptyString;
    comboPtr->textLen = 0;
    comboPtr->tkwin = tkwin;
    comboPtr->underline = -1;
    return comboPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboButtonCmd --
 *
 *      This procedure is invoked to process the "combobutton" command.  See
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
static Blt_OpSpec comboButtonOps[] =
{
    {"activate",  1, ActivateOp,  2, 2, "",},
    {"cget",      2, CgetOp,      3, 3, "option",},
    {"configure", 2, ConfigureOp, 2, 0, "?option value ...?",},
    {"deactivate",1, DeactivateOp,2, 2, "",},
    {"invoke",    1, InvokeOp,    2, 2, "",},
    {"post",      1, PostOp,      2, 2, "",},
    {"unpost",    1, UnpostOp,    2, 2, "",},
};

static int numComboButtonOps = sizeof(comboButtonOps) / sizeof(Blt_OpSpec);

typedef int (ComboInstOp)(ComboButton *comboPtr, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv);

static int
ComboButtonInstCmdProc(
    ClientData clientData,              /* Information about the widget. */
    Tcl_Interp *interp,                 /* Interpreter to report errors. */
    int objc,                           /* Number of arguments. */
    Tcl_Obj *const *objv)               /* Argument vector. */
{
    ComboInstOp *proc;
    ComboButton *comboPtr = clientData;
    int result;

    proc = Blt_GetOpFromObj(interp, numComboButtonOps, comboButtonOps, BLT_OP_ARG1, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    Tcl_Preserve(comboPtr);
    result = (*proc) (comboPtr, interp, objc, objv);
    Tcl_Release(comboPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboButtonInstCmdDeletedProc --
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
ComboButtonInstCmdDeletedProc(ClientData clientData)
{
    ComboButton *comboPtr = clientData; /* Pointer to widget record. */

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
 * ComboButtonCmd --
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
ComboButtonCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    ComboButton *comboPtr;
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
     * it from "$blt_library/bltComboButton.tcl".  We've deferred sourcing
     * this file until now so that the user could reset the variable
     * $blt_library from within her script.
     */
    if (!Blt_CommandExists(interp, "::blt::ComboButton::Post")) {
        static char cmd[] =
            "source [file join $blt_library bltComboButton.tcl]";

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
    comboPtr = NewComboButton(interp, tkwin);
#define EVENT_MASK      (ExposureMask|StructureNotifyMask|FocusChangeMask)
    Tk_CreateEventHandler(tkwin, EVENT_MASK, ComboButtonEventProc, comboPtr);
    Tk_SetClass(tkwin, "BltComboButton");
    comboPtr->cmdToken = Tcl_CreateObjCommand(interp, path, 
        ComboButtonInstCmdProc, comboPtr, ComboButtonInstCmdDeletedProc);
    Blt_SetWindowInstanceData(tkwin, comboPtr);
    if (ConfigureComboButton(interp, comboPtr, objc-2, objv+2, 0) != TCL_OK) {
        Tk_DestroyWindow(comboPtr->tkwin);
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objv[1]);
    return TCL_OK;
}

int
Blt_ComboButtonInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "combobutton", ComboButtonCmd, };

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawLabel --
 *
 *      Draws the text associated with the label.  This is used when the
 *      widget acts like a standard label.
 *
 * Results:
 *      Nothing.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawLabel(ComboButton *comboPtr, Drawable drawable, int x, int y, int w, int h) 
{
    if (comboPtr->image != NULL) {
        int imgWidth, imgHeight;
                
        imgWidth = MIN(w, comboPtr->textWidth);
        imgHeight = MIN(h, comboPtr->textHeight);
        Tk_RedrawImage(IconImage(comboPtr->image), 0, 0, imgWidth, imgHeight, 
                drawable, x, y);
    } else {
        TextStyle ts;
        XColor *fg;

        if (comboPtr->flags & STATE_POSTED) {
            fg = comboPtr->textPostedColor;
        } else if (comboPtr->flags & STATE_ACTIVE) {
            fg = comboPtr->textActiveColor;
        } else if (comboPtr->flags & STATE_DISABLED) {
            fg = comboPtr->textDisabledColor;
        } else {
            fg = comboPtr->textNormalColor;
        }
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, comboPtr->font);
        Blt_Ts_SetAnchor(ts, TK_ANCHOR_NW);
        Blt_Ts_SetJustify(ts, comboPtr->justify);
        Blt_Ts_SetUnderline(ts, comboPtr->underline);
        Blt_Ts_SetMaxLength(ts, w);
        Blt_Ts_SetForeground(ts, fg);
        Blt_Ts_DrawText(comboPtr->tkwin, drawable, comboPtr->text, 
                comboPtr->textLen, &ts, x, y);
    }
}

static void
DrawComboButton(ComboButton *comboPtr, Drawable drawable)
{
    Blt_Bg bg;
    int x, y;
    int w, h;
    int relief;

    /* ComboButton background (just inside of focus highlight ring). */

    if (comboPtr->flags & STATE_POSTED) {
        bg = comboPtr->postedBg;
    } else if (comboPtr->flags & STATE_ACTIVE) {
        bg = comboPtr->activeBg;
    } else if (comboPtr->flags & STATE_DISABLED) {
        bg = comboPtr->disabledBg;
    } else {
        bg = comboPtr->normalBg;
    }
    /* Draw background */
    Blt_Bg_FillRectangle(comboPtr->tkwin, drawable, bg, 0, 0,
        Tk_Width(comboPtr->tkwin), Tk_Height(comboPtr->tkwin),
        comboPtr->borderWidth, TK_RELIEF_FLAT);

    x = y = comboPtr->inset;
    w  = Tk_Width(comboPtr->tkwin)  - 2 * (comboPtr->inset + XPAD);
    h = Tk_Height(comboPtr->tkwin) -  2 * (comboPtr->inset + YPAD);
    x += XPAD;
    y += YPAD;
    /* Draw Icon. */
    if (comboPtr->icon != NULL) {
        int gap, ix, iy, iw, ih;
        
        ix = x;
        iy = y;
        if (h > comboPtr->iconHeight) {
            iy += (h - comboPtr->iconHeight) / 2;
        }
        iw = MIN(w, comboPtr->iconWidth);
        ih = MIN(h, comboPtr->iconHeight);
        Tk_RedrawImage(IconImage(comboPtr->icon), 0, 0, iw, ih,drawable, ix,iy);
        gap = 0;
        if (comboPtr->textWidth > 0) {
            gap = XPAD;
        }
        x += comboPtr->iconWidth + gap;
        w -= comboPtr->iconWidth + gap;
    }
    if ((w > 0) && (h > 0)) {
        int tx, ty, tw, th;
        
        tx = x;
        ty = y;
        if (h > comboPtr->textHeight) {
            ty += (h - comboPtr->textHeight) / 2;
        }
        tw = MIN(w, comboPtr->textWidth);
        th = MIN(h, comboPtr->textHeight);
        DrawLabel(comboPtr, drawable, tx, ty, tw, th);
        x += tw;
    }
    /* Arrow button. */
    if (comboPtr->flags & ARROW) {
        XColor *color;

        /*  */
        x = Tk_Width(comboPtr->tkwin) - XPAD -  comboPtr->inset - comboPtr->arrowWidth;
        y = comboPtr->inset;
        if (h > comboPtr->arrowHeight) {
            y += (h - comboPtr->arrowHeight) / 2;
        }
        if (x < 0) {
            x = comboPtr->inset;
        }
        Blt_Bg_FillRectangle(comboPtr->tkwin, drawable, bg, x, y, 
            comboPtr->arrowWidth, comboPtr->arrowHeight, comboPtr->arrowBW,
            comboPtr->arrowRelief);
        if (comboPtr->flags & STATE_POSTED) {
            color = comboPtr->textPostedColor;
        } else if (comboPtr->flags & STATE_ACTIVE) {
            color = comboPtr->textActiveColor;
        } else if (comboPtr->flags & STATE_DISABLED) {
            color = comboPtr->textDisabledColor;
        } else {
            color = comboPtr->textNormalColor;
        }
        Blt_DrawArrow(comboPtr->display, drawable, color, x, y, 
                comboPtr->arrowWidth, comboPtr->arrowHeight,
                comboPtr->arrowBW, ARROW_DOWN);
    }
    /* Draw focus highlight ring. */
    if (comboPtr->highlightWidth > 0) {
        if (comboPtr->flags & FOCUS) {
            Tk_DrawFocusHighlight(comboPtr->tkwin, comboPtr->highlightGC, 
                comboPtr->highlightWidth, drawable);
        } else {
            if (comboPtr->highlightBg != NULL) {
                Blt_Bg_DrawFocus(comboPtr->tkwin, comboPtr->highlightBg, 
                                 comboPtr->highlightWidth, drawable);
            } else {
                Blt_Bg_DrawFocus(comboPtr->tkwin, bg, 
                                 comboPtr->highlightWidth, drawable);
            }
        }           
    }
    if (comboPtr->flags & STATE_POSTED) {
        relief = comboPtr->postedRelief;
    } else if (comboPtr->flags & STATE_ACTIVE) {
        relief = comboPtr->activeRelief;
    } else {
        relief = comboPtr->relief;
    }
    if (relief != TK_RELIEF_FLAT) {
        Blt_Bg_DrawRectangle(comboPtr->tkwin, drawable, bg, 
                comboPtr->highlightWidth, comboPtr->highlightWidth,
                Tk_Width(comboPtr->tkwin)  - 2 * comboPtr->highlightWidth,
                Tk_Height(comboPtr->tkwin) - 2 * comboPtr->highlightWidth,
                comboPtr->borderWidth, relief);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayComboButton --
 *
 *      This procedure is invoked to display a combobutton widget.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Commands are output to X to display the button.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayComboButton(ClientData clientData)
{
    ComboButton *comboPtr = clientData;
    Pixmap drawable;
    int w, h;                           /* Window width and height. */

    comboPtr->flags &= ~REDRAW_PENDING;
    if (comboPtr->tkwin == NULL) {
        return;                         /* Window destroyed (should not get
                                         * here) */
    }
#ifdef notdef
    fprintf(stderr, "Calling DisplayComboButton(%s)\n", 
            Tk_PathName(comboPtr->tkwin));
#endif
    w = Tk_Width(comboPtr->tkwin);
    h = Tk_Height(comboPtr->tkwin);
    if ((w <= 1) || (h <=1)) {
        /* Don't bother computing the layout until the window size is
         * something reasonable. */
        return;
    }
    if (comboPtr->flags & LAYOUT_PENDING) {
        ComputeGeometry(comboPtr);
    }
    if (!Tk_IsMapped(comboPtr->tkwin)) {
        /* The widget's window isn't displayed, so don't bother drawing
         * anything.  By getting this far, we've at least computed the
         * coordinates of the combobutton's new layout.  */
        return;
    }

    /* Create a pixmap the size of the window for double buffering. */
    drawable = Blt_GetPixmap(comboPtr->display, Tk_WindowId(comboPtr->tkwin),
                w, h, Tk_Depth(comboPtr->tkwin));
#ifdef WIN32
    assert(drawable != None);
#endif
    DrawComboButton(comboPtr, drawable);
    XCopyArea(comboPtr->display, drawable, Tk_WindowId(comboPtr->tkwin),
        comboPtr->highlightGC, 0, 0, w, h, 0, 0);
    Tk_FreePixmap(comboPtr->display, drawable);
}

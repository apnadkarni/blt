/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltCanvLabel.c --
 *
 * This file implements a rotated label for canvas widgets.  It also
 * provides font scaling and truncation based on unrotated length.

 * Copyright 2017 George A. Howlett. All rights reserved.  
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
 * To do:
 *
 *      -background colorName  ; same as -fill
 *      -rotate numDegrees
 *      -minfontsize numPoints
 *      -maxfontsize numPoints       
 *      -truncate ellipsis | no | yes
 *      -fill colorName
 *      -font fontName
 *      -foreground colorName  ; same as -outline
 *      -outline colorName
 *      -padx numPixels
 *      -pady numPixels
 *      -justify left|right|center
 *      -textwidth numPixels or relative
 *      -width numPixels
 *      -height numPixels
 *      -anchor anchorName
 *      
 */
#define USE_OLD_CANVAS  1

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include <bltAlloc.h>
#include "bltMath.h"
#include "bltChain.h"
#include "bltPicture.h"
#include "bltPs.h"
#include "bltImage.h"
#include "bltPainter.h"

#define DEF_ACTIVE_FILL_COLOR           (char *)NULL
#define DEF_ACTIVE_OUTLINE_COLOR        (char *)NULL
#define DEF_ANCHOR                      "nw"
#define DEF_DISABLED_FILL_COLOR         (char *)NULL
#define DEF_DISABLED_OUTLINE_COLOR      (char *)NULL
#define DEF_FONT                        STD_FONT
#define DEF_HEIGHT                      "0"
#define DEF_LINEWIDTH                   "0"
#define DEF_MAXFONTSIZE                 "-1"
#define DEF_MINFONTSIZE                 "-1"
#define DEF_NORMAL_FILL_COLOR           (char *)NULL
#define DEF_NORMAL_OUTLINE_COLOR        RGB_BLACK
#define DEF_ROTATE                      "0"
#define DEF_SCALE                       "1.0"
#define DEF_STATE                       "normal"
#define DEF_TAGS                        (char *)NULL
#define DEF_TEXT                        (char *)NULL
#define DEF_TEXTANCHOR                  "center"
#define DEF_WIDTH                       "0"

typedef struct {
    Display display;
    XColor color;
} LabelGCKey;

typedef struct {
    int refCount;
    GC gc;
    Blt_HashEntry *hashPtr;
} LabelGC;

/* Global table of label GCs, shared among all the label items. */
static Blt_HashTable gcTable;
static int initialized = 0;

/*
 * The structure below defines the record for each label item.
 */
typedef struct {
    Tk_Item header;                     /* Generic stuff that's the same for
                                         * all types.  MUST BE FIRST IN
                                         * STRUCTURE. */
    Tk_Canvas canvas;                   /* Canvas containing the label item. */
    int lastWidth, lastHeight;          /* Last known dimensions of the label
                                         * item.  This is used to know if the
                                         * picture preview needs to be
                                         * resized. */
    Tcl_Interp *interp;

    float angle;			/* Angle to rotate the label. */
    const char *text;			/* Text string to be displayed.
					 * The string may contain newlines,
					 * but not tabs. */
    int textWidth, textHeight;		/* Unrotated dimensions of item. */
    int width, height;			/* Possibly rotated dimensions of
					 * item. */
    double fontScale;			/* Font scaling factor. */

    GC fillGC;                          /* Graphics context to fill background
                                         * of image outline if no preview
                                         * image was present. */
    /* User configurable fields */

    double x, y;                        /* Requested anchor in canvas
                                         * coordinates of the label item */
    Tk_Anchor anchor;

    int textAnchor;                     /* Anchors the text within the
                                         * background.  The default is
                                         * center. */
    Region2d bbox;                      /* Represent the location and
                                         * dimensions of the unrotated
                                         * text. */

    int reqWidth, reqHeight;            /* Requested dimension of label
                                         * item in canvas
                                         * coordinates. These are the
                                         * unrotated dimensions of the
                                         * item.  If non-zero, they
                                         * override the dimension computed
                                         * from the normal size of the
                                         * text. */
    int state;                          /* State of item: TK_STATE_HIDDEN, 
                                         * TK_STATE_NORMAL, TK_STATE_ACTIVE,
                                         * or TK_STATE_DISABLED */
    XColor *normalFg;                   /* Label text and outline color. */
    XColor *disabledFg;                 /* If non-NULL, disabled label text
                                         * and outline color. */
    XColor *activeFg;                   /* If non-NULL, active label text
                                         * and outline color. */
    Blt_Bg normalBg;                    /* If non-NULL, fill background
                                         * color. Otherwise the background
                                         * is transparent. */
    Blt_Bg disabledBg;                  /* If non-NULL, disabled fill
                                         * background color. Otherwise uses
                                         * the normal background color. */
    Blt_Bg activeBg;                    /* If non-NULL, active fill
                                         * background color. Otherwise uses
                                         * normal the background color. */
    LabelGC *normalLabelGC;             /* Graphics context to draw
                                         * text and outline normally. */
    LabelGC *disabledLabelGC;           /* Graphics context to draw a
                                         * disabled label's text and
                                         * outline. */
    LabelGC *activeLabelGC;             /* Graphics context to draw a
                                         * active label's text and
                                         * outline. */
    Blt_Font baseFont;                  /* Base font for item.  This is the
                                         * unscaled font. */
    Blt_Font scaledFont;                /* If non-NULL, is the base font at
                                         * the current scale factor. */
    Point2d outlinePts[5];              /* Points representing the rotated
                                         * polygon (outline) of the
                                         * bounding box.  This is used to
                                         * draw the rotated background and
                                         * rectangle and to determine if
                                         * the text is visible on the
                                         * screen.  */
    XPoint points[5];
    int rotWidth, rotHeight;            /* Rotated width and height of
                                         * region occupied by label. */
    TkRegion clipRegion;

} LabelItem;

/*
 * Information used for parsing configuration specs:
 */

static int StringToFont(ClientData clientData, Tcl_Interp *interp,
        Tk_Window tkwin, const char *string, char *widgRec, int offset);
#if (_TCL_VERSION >= _VERSION(8,6,0)) 
static const char *FontToString (ClientData clientData, Tk_Window tkwin, 
        char *widgRec, int offset, Tcl_FreeProc **proc);
#else
static char *FontToString (ClientData clientData, Tk_Window tkwin, 
        char *widgRec, int offset, Tcl_FreeProc **proc);
#endif

static Tk_CustomOption fontOption = {
    StringToFont, FontToString, (ClientData)0
};

static int StringToBg(ClientData clientData, Tcl_Interp *interp,
        Tk_Window tkwin, const char *string, char *widgRec, int offset);
#if (_TCL_VERSION >= _VERSION(8,6,0)) 
static const char *BgToString (ClientData clientData, Tk_Window tkwin, 
        char *widgRec, int offset, Tcl_FreeProc **proc);
#else
static char *BgToString (ClientData clientData, Tk_Window tkwin, 
        char *widgRec, int offset, Tcl_FreeProc **proc);
#endif

static Tk_CustomOption bgOption = {
    StringToBg, BgToString, (ClientData)0
};

static int StringToState(ClientData clientData, Tcl_Interp *interp,
        Tk_Window tkwin, const char *string, char *widgRec, int offset);
#if (_TCL_VERSION >= _VERSION(8,6,0)) 
static const char *StateToString (ClientData clientData, Tk_Window tkwin, 
        char *widgRec, int offset, Tcl_FreeProc **proc);
#else
static char *StateToString (ClientData clientData, Tk_Window tkwin, 
        char *widgRec, int offset, Tcl_FreeProc **proc);
#endif

static Tk_CustomOption stateOption = {
    StringToState, StateToString, (ClientData)0
};

static Tk_CustomOption tagsOption;

BLT_EXTERN Tk_CustomOption bltPadOption;
BLT_EXTERN Tk_CustomOption bltDistanceOption;


static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_SYNONYM, (char *)"-activebg", "activeFill", (char *)NULL, 
        (char *)NULL, 0, 0},
    {TK_CONFIG_SYNONYM, (char *)"-activebackground", "activeFill", 
        (char *)NULL, (char *)NULL, 0, 0},
    {TK_CONFIG_SYNONYM, (char *)"-activefg", "activeOutline", 
        (char *)NULL, (char *)NULL, 0, 0},
    {TK_CONFIG_CUSTOM, (char *)"-activefill", "activeFill", (char *)NULL,
        DEF_ACTIVE_FILL_COLOR, Blt_Offset(LabelItem, activeBg), 
        TK_CONFIG_NULL_OK, &fillOption},
    {TK_CONFIG_SYNONYM, (char *)"-activeforeground", "activeOutline", 
        (char *)NULL, (char *)NULL, 0, 0},
    {TK_CONFIG_COLOR, (char *)"-activeoutline", "activeOutline", (char *)NULL,
        DEF_ACTIVE_OUTLINE_COLOR, Blt_Offset(LabelItem, activeFg),  
        TK_CONFIG_NULL_OK},
    {TK_CONFIG_ANCHOR, (char *)"-anchor", (char *)NULL, (char *)NULL,
        DEF_ANCHOR, Blt_Offset(LabelItem, anchor), TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_SYNONYM, (char *)"-bg", "fill", (char *)NULL, (char *)NULL, 
        0, 0},
    {TK_CONFIG_SYNONYM, (char *)"-background", "fill", (char *)NULL, 
        (char *)NULL, 0, 0},
    {TK_CONFIG_CUSTOM, (char *)"-borderwidth", "borderWidth", (char *)NULL,
        DEF_BORDERWIDTH, Blt_Offset(LabelItem, borderWidth),
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_SYNONYM, (char *)"-disabledbg", "disabledFill", (char *)NULL, 
        (char *)NULL, 0, 0},
    {TK_CONFIG_SYNONYM, (char *)"-disabledbackground", "disabledFill", 
        (char *)NULL, (char *)NULL, 0, 0},
    {TK_CONFIG_SYNONYM, (char *)"-disabledfg", "disabledOutline", (char *)NULL, 
        (char *)NULL, 0, 0},
    {TK_CONFIG_CUSTOM, (char *)"-disabledfill", "disabledFill", (char *)NULL,
        DEF_DISABLED_FILL_COLOR, Blt_Offset(LabelItem, disabledBg), 
        TK_CONFIG_NULL_OK, &bgOption},
    {TK_CONFIG_SYNONYM, (char *)"-disabledforeground", "disabledOutline", 
        (char *)NULL, (char *)NULL, 0, 0},
    {TK_CONFIG_COLOR, (char *)"-disabledoutline", "disabledOutline", 
        (char *)NULL, DEF_DISABLED_OUTLINE_COLOR, 
        Blt_Offset(LabelItem, disabledFg), TK_CONFIG_NULL_OK},

    {TK_CONFIG_CUSTOM, (char *)"-fill", "fill", (char *)NULL,
        DEF_NORMAL_FILL_COLOR, Blt_Offset(LabelItem, normalBg), 
        TK_CONFIG_NULL_OK, &bgOption},

    {TK_CONFIG_CUSTOM, (char *)"-font", (char *)NULL, (char *)NULL,
        DEF_FONT, Blt_Offset(LabelItem, font), 0, &fontOption},
    {TK_CONFIG_SYNONYM, (char *)"-fg", "fill", (char *)NULL, (char *)NULL, 
        0, 0},
    {TK_CONFIG_SYNONYM, (char *)"-foreground", "fill", (char *)NULL, 
        (char *)NULL, 0, 0},
    {TK_CONFIG_CUSTOM, (char *)"-height", (char *)NULL, (char *)NULL,
        DEF_HEIGHT, Blt_Offset(LabelItem, reqHeight),
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_ANCHOR, (char *)"-textanchor", (char *)NULL, (char *)NULL,
        DEF_JUSTIFY, Blt_Offset(LabelItem, textAnchor),
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_CUSTOM, (char *)"-linewidth", (char *)NULL, (char *)NULL,
        DEF_LINEWIDTH, Blt_Offset(LabelItem, lineWidth),
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_INT, (char *)"-maxfontsize", (char *)NULL, (char *)NULL,
        DEF_MAXFONTSIZE, Blt_Offset(LabelItem, maxFontSize), 
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_INT, (char *)"-minfontsize", (char *)NULL, (char *)NULL,
        DEF_MINFONTSIZE, Blt_Offset(LabelItem, minFontSize), 
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_COLOR, (char *)"-outline", "outline", (char *)NULL,
        DEF_NORMAL_OUTLINE_COLOR, Blt_Offset(LabelItem, outline), 0},
    {TK_CONFIG_CUSTOM, (char *)"-padx", (char *)NULL, (char *)NULL,
        DEF_PADX, Blt_Offset(LabelItem, xPad),
        TK_CONFIG_DONT_SET_DEFAULT, &bltPadOption},
    {TK_CONFIG_CUSTOM, (char *)"-pady", (char *)NULL, (char *)NULL,
        DEF_PADY, Blt_Offset(LabelItem, yPad),
        TK_CONFIG_DONT_SET_DEFAULT, &bltPadOption},
    {TK_CONFIG_DOUBLE, (char *)"-rotate", (char *)NULL, (char *)NULL,
        DEF_ROTATE, Blt_Offset(LabelItem, angle),
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_DOUBLE, (char *)"-scale", (char *)NULL, (char *)NULL,
        DEF_SCALE, Blt_Offset(LabelItem, scale),
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_CUSTOM, (char *)"-state", (char *)NULL, (char *)NULL,
        DEF_STATE, Blt_Offset(LabelItem, state), TK_CONFIG_DONT_SET_DEFAULT, 
        &stateOption},
    {TK_CONFIG_CUSTOM, (char *)"-tags", (char *)NULL, (char *)NULL,
        DEF_TAGS, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_STRING, (char *)"-text", (char *)NULL, (char *)NULL,
        DEF_TEXT, Blt_Offset(LabelItem, text), TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, (char *)"-width", (char *)NULL, (char *)NULL,
        DEF_WIDTH, Blt_Offset(LabelItem, reqWidth),
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
};

/* Prototypes for procedures defined in this file: */
static Tk_ImageChangedProc ImageChangedProc;
static Tk_ItemCoordProc CoordsProc;
static Tk_ItemAreaProc AreaProc;
static Tk_ItemPointProc PointProc;
static Tk_ItemConfigureProc ConfigureProc;
static Tk_ItemCreateProc CreateProc;
static Tk_ItemDeleteProc DeleteProc;
static Tk_ItemDisplayProc DisplayProc;
static Tk_ItemScaleProc ScaleProc;
static Tk_ItemTranslateProc TranslateProc;
static Tk_ItemPostscriptProc PostScriptProc;

static void ComputeBbox(Tk_Canvas canvas, LabelItem *imgPtr);
static int ReadPostScript(Tcl_Interp *interp, LabelItem *labelPtr);


/*
 *---------------------------------------------------------------------------
 *
 * StringToFont --
 *
 *      Converts string to Blt_Font structure.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StringToFont(ClientData clientData,Tcl_Interp *interp, Tk_Window tkwin,
             const char *string,  char *widgRec, int offset)
{
    Blt_Font *fontPtr = (Blt_Font *)(widgRec + offset);
    Blt_Font font;

    font = Blt_GetFont(interp, tkwin, string);
    if (font == NULL) {
        return TCL_ERROR;
    }
    *fontPtr = font;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FontToString --
 *
 *      Returns the name of the Blt_Font.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
#if (_TCL_VERSION >= _VERSION(8,6,0)) 
static const char *
#else
static char *
#endif
FontToString(ClientData clientData, Tk_Window tkwin, char *widgRec,
             int offset, Tcl_FreeProc **freeProcPtr)
{
    Blt_Font font = *(Blt_Font *)(widgRec + offset);
    const char *string;

    string = Blt_Font_Name(font);
    *freeProcPtr = (Tcl_FreeProc *)TCL_STATIC;
    return (char *)string;
}

/*
 *---------------------------------------------------------------------------
 *
 * StringToBg --
 *
 *      Converts string to Blt_Bg structure.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StringToBg(ClientData clientData,Tcl_Interp *interp, Tk_Window tkwin,
           const char *string,  char *widgRec, int offset)
{
    Blt_Bg *bgPtr = (Blt_Font *)(widgRec + offset);

    return Blt_GetBg(interp, tkwin, string, bgPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * BgToString --
 *
 *      Returns the name of the Blt_Bg.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
#if (_TCL_VERSION >= _VERSION(8,6,0)) 
static const char *
#else
static char *
#endif
BgToString(ClientData clientData, Tk_Window tkwin, char *widgRec,
             int offset, Tcl_FreeProc **freeProcPtr)
{
    Blt_Bg bg = *(Blt_Bg *)(widgRec + offset);
    const char *string;

    string = (bg != NULL) ? Blt_Bg_Name(bg) : "";
    *freeProcPtr = (Tcl_FreeProc *)TCL_STATIC;
    return (char *)string;
}

/*
 *---------------------------------------------------------------------------
 *
 * StringToState --
 *
 *      Converts string to state flags.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StringToState(ClientData clientData,Tcl_Interp *interp, Tk_Window tkwin,
              const char *string,  char *widgRec, int offset)
{
    int  *statePtr = (int *)(widgRec + offset);
    char c;
    int length;

    c = string[0];
    length = strlen(value);
    if ((c == 'd') && (strncmp(string, "disabled", length) == 0)) {
        *statePtr = TK_STATE_DISABLED;
    } else if ((c == 'a') && (strncmp(string, "active", length) == 0)) {
        *statePtr = TK_STATE_ACTIVE;
    } else if ((c == 'n') && (strncmp(string, "normal", length) == 0)) {
        *statePtr = TK_STATE_NORMAL;
    } else if ((c == 'h') && (strncmp(string, "hidden", length) == 0)) {
        *statePtr = TK_STATE_HIDDEN:
    } else {
        Tcl_ApepndResult(interp, "unknown state \"", string, 
             "\": should be active, disabled, hidden or normal", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StateToString --
 *
 *      Returns the name of the state flags.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
#if (_TCL_VERSION >= _VERSION(8,6,0)) 
static const char *
#else
static char *
#endif
StringToState(ClientData clientData, Tk_Window tkwin, char *widgRec,
              int offset, Tcl_FreeProc **freeProcPtr)
{
    unsigned int state = *(unsigned int *)(widgRec + offset);
    const char *string;

    switch (state) {
    case TK_STATE_DISABLE:
        string = "disabled";          break;
    case TK_STATE_HIDDEN:
        string = "hidden";            break;
    case TK_STATE_NORMAL:
        string = "normal";            break;
    case TK_STATE_ACTIVE:
        string = "active";            break;
    default:
        string = "???";               break;
    } 
    *freeProcPtr = (Tcl_FreeProc *)TCL_STATIC;
    return (char *)string;
}

static LabelGC *
GetLabelGC(Tk_Window tkwin, XColor *colorPtr)
{
    LabelGCKey key;
    LabelGC *gcPtr;

    key.color = *colorPtr;
    key.display = Tk_Display(tkwin);
    hPtr = Blt_CreateHashEntry(&gcTable, (char *)&key, &isNew);
    if (isNew) {
        unsigned int gcMask;
        XGCValues gcValues;
        int isNew;

        gcMask = GCForeground;
        gcValues.foreground = colorPtr->pixel;
        newGC = Blt_GetPrivateGC(tkwin, gcMask, &gcValues);
        gcPtr = Blt_AssertMalloc(sizeof(LabelGC));
        gcPtr->gc = newGC;
        gcPtr->refCount = 1;
        gcPtr->hashPtr = hPtr;
        Blt_SetHashValue(hPtr, gcPtr);
    } else {
        gcPtr = Blt_GetHashValue(hPtr);
        gcPtr->refCount++;
    }
    return gcPtr;
}

static void
FreeLabelGC(Display display, LabelGC *gcPtr)
{
    gcPtr->refCount--;
    if (gcPtr->refCount <= 0) {
        if (gcPtr->gc != NULL) {
            Blt_FreePrivateGC(display, gcPtr->gc);
        }
        Blt_DeleteHashEntry(&gcTable, gcPtr->hashPtr);
    }
    Blt_Free(gcPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteProc --
 *
 *      This procedure is called to clean up the data structure associated
 *      with a label item.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Resources associated with labelPtr are released.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
DeleteProc( 
    Tk_Canvas canvas,                   /* Info about overall canvas
                                         * widget. */
    Tk_Item *itemPtr,                   /* Item that is being deleted. */
    Display *display)                   /* Display containing window for
                                         * canvas. */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;

    Tk_FreeOptions(configSpecs, (char *)labelPtr, display, 0);
    if (labelPtr->scaledFont != NULL) {
        Blt_Font_Free(labelPtr->scaledFont);
    }
    if (labelPtr->clipRegion != NULL) {
        TkDestroyRegion(labelPtr->clipRegion);
    }
    if (labelPtr->normalLabelGC != NULL) {
        FreeLabelGC(labelPtr->display, labelPtr->normalLabelGC);
    }
    if (labelPtr->disabledLabelGC != NULL) {
        FreeLabelGC(labelPtr->display, labelPtr->disabledLabelGC);
    }
    if (labelPtr->activeLabelGC != NULL) {
        FreeLabelGC(labelPtr->display, labelPtr->activeLabelGC);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateProc --
 *
 *      This procedure is invoked to create a new label item.
 *
 * Results:
 *      A standard TCL return value.  If an error occurred in creating the
 *      item, then an error message is left in interp->result; in this case
 *      labelPtr is left uninitialized, so it can be safely freed by the
 *      caller.
 *
 * Side effects:
 *      A new label item is created.
 *
 *      pathName create label x y ?option value...?
 *
 *---------------------------------------------------------------------------
 */
static int
CreateProc(
    Tcl_Interp *interp,                 /* Interpreter for error reporting. */
    Tk_Canvas canvas,                   /* Canvas to hold new item. */
    Tk_Item *itemPtr,                   /* Pointer to the structure of the
                                         * new item; header has been
                                         * previously initialized by the
                                         * caller. */
    int argc,                           /* # of arguments in argv. */
    char **argv)                        /* Arguments describing item. */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;
    Tk_Window tkwin;
    double x, y;

    tkwin = Tk_CanvasTkwin(canvas);
    if (argc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
            Tk_PathName(tkwin), " create ", itemPtr->typePtr->name,
            " x y ?option value...?\"", (char *)NULL);
        return TCL_ERROR;
    }
    /* Get the anchor point of the label on the canvas. */
    if ((Tk_CanvasGetCoord(interp, canvas, argv[0], &x) != TCL_OK) ||
        (Tk_CanvasGetCoord(interp, canvas, argv[1], &y) != TCL_OK)) {
        return TCL_ERROR;
    }

    /* Initialize the rest of label item structure, below the header. */
    memset((char *)labelPtr + sizeof(Tk_Item), 0, 
           sizeof(LabelItem) - sizeof(Tk_Item));

    labelPtr->x = x;
    labelPtr->y = y;
    labelPtr->anchor = TK_ANCHOR_NW;
    labelPtr->canvas = canvas;
    labelPtr->interp = interp;
    labelPtr->tkwin  = tkwin;
    labelPtr->display = Tk_Display(tkwin);
    if (ConfigureProc(interp, canvas, itemPtr, argc - 2, argv + 2, 0) 
        != TCL_OK) {
        DeleteProc(canvas, itemPtr, Tk_Display(tkwin));
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureProc --
 *
 *      This procedure is invoked to configure various aspects of an label
 *      item, such as its background color.
 *
 * Results:
 *      A standard TCL result code.  If an error occurs, then an error message
 *      is left in interp->result.
 *
 * Side effects:
 *      Configuration information may be set for labelPtr.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureProc(
    Tcl_Interp *interp,                 /* Used for error reporting. */
    Tk_Canvas canvas,                   /* Canvas containing labelPtr. */
    Tk_Item *itemPtr,                   /* Label item to reconfigure. */
    int argc,                           /* # of elements in argv.  */
    char **argv,                        /* Arguments describing things to
                                         * configure. */
    int flags)                          /* Flags to pass to
                                         * Tk_ConfigureWidget. */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;
    Tk_Window tkwin;
    LabelGC *newLabelGC;

    tkwin = Tk_CanvasTkwin(canvas);
    if (Tk_ConfigureWidget(interp, tkwin, configSpecs, argc, (const char**)argv,
                (char *)labelPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_OldConfigModified(configSpecs, "-rotate", "-*fontsize", 
                              "-font", "-pad*", "-width",
                              "-height", "-anchor", "-linewidth",
                              (char *)NULL)) {
        ComputeGeometry(labelPtr);
    }
    labelPtr->angle = (float)FMOD(labelPtr->angle, 360.0);
    if (labelPtr->angle < 0.0f) {
        labelPtr->angle += 360.0f;
    }
    newLabelGC = NULL;

    /* These are all LabelGCs because we will be changing the font as the
     * label item scales. */
    if (labelPtr->normalFg != NULL) {
        newLabelGC = GetLabelGC(tkwin, labelPtr->normalFg);
    }
    if (labelPtr->normalLabelGC != NULL) {
        FreeLabelGC(labelPtr->display, labelPtr->normalLabelGC);
    }
    labelPtr->normalLabelGC = newLabelGC;
    newLabelGC = NULL;
    if (labelPtr->disabledFg != NULL) {
        newLabelGC = GetLabelGC(tkwin, labelPtr->disabledFg);
    }
    if (labelPtr->disabledLabelGC != NULL) {
        FreeLabelGC(labelPtr->display, labelPtr->disabledLabelGC);
    }
    labelPtr->disabledLabelGC = newLabelGC;
    newLabelGC = NULL;
    if (labelPtr->activeFg != NULL) {
        newLabelGC = GetLabelGC(tkwin, labelPtr->activeFg);
    }
    if (labelPtr->activeLabelGC != NULL) {
        FreeLabelGC(labelPtr->display, labelPtr->activeLabelGC);
    }
    labelPtr->activeLabelGC = newLabelGC;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CoordsProc --
 *
 *      This procedure is invoked to process the "coords" widget command on
 *      label items.  See the user documentation for details on what it
 *      does.
 *
 * Results:
 *      Returns TCL_OK or TCL_ERROR, and sets interp->result.
 *
 * Side effects:
 *      The coordinates for the given item may be changed.
 *
 *---------------------------------------------------------------------------
 */
static int
CoordsProc(
    Tcl_Interp *interp,                 /* Used for error reporting. */
    Tk_Canvas canvas,                   /* Canvas containing item. */
    Tk_Item *itemPtr,                   /* Item whose coordinates are to be
                                         * read or modified. */
    int argc,                           /* Number of coordinates supplied in
                                         * argv. */
    char **argv)                        /* Array of coordinates: x1, y1, x2,
                                         * y2, ... */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;

    if ((argc != 0) && (argc != 2)) {
        Tcl_AppendResult(interp, "wrong # coordinates: expected 0 or 2, got ",
            Blt_Itoa(argc), (char *)NULL);
        return TCL_ERROR;
    }
    if (argc == 2) {
        double x, y;                    /* Don't overwrite old coordinates
                                         * on errors */

        if ((Tk_CanvasGetCoord(interp, canvas, argv[0], &x) != TCL_OK) ||
            (Tk_CanvasGetCoord(interp, canvas, argv[1], &y) != TCL_OK)) {
            return TCL_ERROR;
        }
        labelPtr->x = x;
        labelPtr->y = y;
        ComputeGeometry(labelPtr);
    }
    Tcl_AppendElement(interp, Blt_Dtoa(interp, labelPtr->x));
    Tcl_AppendElement(interp, Blt_Dtoa(interp, labelPtr->y));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeGeometry --
 *
 *      Computes the geometry of the possibly rotated label.  
 *      
 *---------------------------------------------------------------------------
 */
 /* ARGSUSED */
static void
ComputeGeometry(LabelItem *labelPtr)
{
    unsigned int w, h;
    TextLayout *layoutPtr;

    labelPtr->width = labelPtr->height = 0;
    if (labelPtr->text == NULL) {
        return;
    }
    labelPtr->numBytes = strlen(labelPtr->text);
    Blt_Ts_InitStyle(&ts);
    Blt_Ts_SetFont(&ts, font);
    Blt_Ts_SetPadding(&ts, labelPtr->xPad.side1, labelPtr->xPad.side2,
                      labelPtr->yPad.side1, labelPtr->yPad.side2);
    layoutPtr = Blt_Ts_CreateLayout(labelPtr->text, labelPtr->numBytes, &ts);

    if (labelPtr->layoutPtr != NULL) {
        Blt_Free(labelPtr->layoutPtr);
    }
    labelPtr->layoutPtr = layoutPtr;
    w = layoutPtr->width;
    h = layoutPtr->height;
    if (labelPtr->reqWidth > 0) {
        xOffset = labelPtr->reqWidth - w;
        w = labelPtr->reqWidth;
    }
    if (labelPtr->reqHeight > 0) {
        yOffset = labelPtr->reqHeight - h;
        h = labelPtr->reqHeight;
    }
    /* Compute the outline polygon (isolateral or rectangle) given the
     * width and height. The center of the box is 0,0. */
    Blt_GetBoundingBox(w, h, labelPtr->angle, &rw, &rh, labelPtr->outlinePts);
    labelPtr->rotWidth = rw;
    labelPtr->rotHeight = rh;

    /* The label's x,y position is in world coordinates. This point and the
     * anchor tell us where is the anchor position of the label, which is
     * the upper-left corner of the bounding box around the possible
     * rotated item. */
    labelPtr->anchorPos = Blt_AnchorPoint(labelPtr->x, labelPtr->y, rw, rh, 
        labelPtr->anchor);

    /* Translate back to the the upper-left corner. */
    for (i = 0; i < 4; i++) {
        labelPtr->outlinePts[i].x += rw * 0.5;
        labelPtr->outlinePts[i].y += rh * 0.5;
    }
    labelPtr->outlinePts[4] = labelPtr->outlinePts[0];

    /* Compute the starting positions of the text. This also encompasses
     * justification. */
    switch (labelPtr->textAnchor) {
    case TK_ANCHOR_NW:
    case TK_ANCHOR_W:
    case TK_ANCHOR_SW:
        xOffset = 0;
        break;
    case TK_ANCHOR_N:
    case TK_ANCHOR_C:
    case TK_ANCHOR_S:
        xOffset = (w - layoutPtr->width) / 2;
        break;
    case TK_ANCHOR_NE:
    case TK_ANCHOR_E:
    case TK_ANCHOR_SE:
        xOffset = w - layoutPtr->width;
        break;
    }
    switch (labelPtr->textAnchor) {
    case TK_ANCHOR_NW:
    case TK_ANCHOR_N:
    case TK_ANCHOR_NE:
        yOffset = 0;
        break;
    case TK_ANCHOR_W:
    case TK_ANCHOR_C:
    case TK_ANCHOR_E:
        yOffset = (h - layoutPtr->height) / 2;
        break;
    case TK_ANCHOR_SW:
    case TK_ANCHOR_S:
    case TK_ANCHOR_SE:
        yOffset = h - layoutPtr->height;
        break;
    }
    /* Offset to center of unrotated box. */
    off1.x = (double)w * 0.5 + xOffset;
    off1.y = (double)h * 0.5 + yOffset;
    /* Offset to center of rotated box. */
    off2.x = rw * 0.5;
    off2.y = rh * 0.5;
    radians = -angle * DEG2RAD;
    sinTheta = sin(radians), cosTheta = cos(radians);

    /* Rotate starting positions of the text. */
    for (i = 0; i < layoutPtr->numFragments; i++) {
        Point2d p, q;
        TextFragment *fragPtr;

        fragPtr = layoutPtr->fragments + i;
        /* Translate the start of the fragement to the center of box. */
        p.x = (fragPtr->x + xOffset) - off1.x;
        p.y = (fragPtr->y + yOffset) - off1.y;
        /* Rotate the point. */
        q.x = (p.x * cosTheta) - (p.y * sinTheta);
        q.y = (p.x * sinTheta) + (p.y * cosTheta);
        /* Translate the point back toward the upper-left corner of the
         * rotated bounding box. */
        q.x += off2.x;
        q.y += off2.y;
        fragPtr->x1 = ROUND(q.x);
        fragPtr->y1 = ROUND(q.y);
    }
}

static void
MapItem(Tk_Canvas canvas, LabelItem *labelPtr)
{
    short int sx, sy;                   /* Screen coordinates */
    int rw2, rh2;

    /* Convert anchor from world coordinates to screen. */
    Tk_CanvasDrawableCoords(canvas, labelPtr->anchorPos.x, 
                            labelPtr->anchorPos.y, &sx, &sy);

    /* Map the outline relative to the upper-left corner. */
    rw2 = (int)(labelPtr->rotWidth * 0.5);
    rh2 = (int)(labelPtr->rowHeight * 0.5);
    for (i = 0; i < 5; i++) {
        labelPtr->points[i].x += sx + rw2;
        labelPtr->points[i].y += sy + rh2;
    }
    labelPtr->points[4] = labelPtr->points[0];
    if (labelPtr->clipsRegion != NULL) {
        TkDestroyRegion(labelPtr->clipsRegion);
    }
    labelPtr->clipRegion = XPolygonRegion(labelPtr->points, 5, EvenOddRule);
    labelPtr->flags &= LAYOUT_PENDING;
}

static void
LabelInsideRegion(LabelItem *labelPtr, int rx, int dy, int rw, int rh)
{
    Region2d region;

    /* Test to see if the rotated bounding box of the label is inside the
     * region. */
    region.x1 = rx;
    region.y1 = ry;
    region.x2 = rx + rw;
    region.y2 = ry + rh;
    return Blt_RegionInPolygon(&region, labelPtr->outlinePts, 5, TRUE);
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayProc --
 *
 *      This procedure is invoked to draw the label item in a given
 *      drawable.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      ItemPtr is drawn in drawable using the transformation information in
 *      canvas.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayProc(
    Tk_Canvas canvas,                   /* Canvas that contains item. */
    Tk_Item *itemPtr,                   /* Item to be displayed. */
    Display *display,                   /* Display on which to draw item. */
    Drawable drawable,                  /* Pixmap or window in which to
                                         * draw item. */
    int rx, int ry, 
    int rw, int rh)                     /* Describes region of canvas that
                                         * must be redisplayed. */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;
    Tk_Window tkwin;
    const char *title;
    int w, h;
    short int dx, dy;
    GC gc;
    Blt_Bg bg;

    if (labelPtr->state == TK_STATE_HIDDEN) {
        return;                         /* Item is hidden. */
    }
    if (labelPtr->flags & LAYOUT_PENDING) {
        MapItem(labelPtr);
    }
    if (!LabelInsideRegion(labelPtr, rx, ry, rw, rh)) {
        return;
    }
    tkwin = Tk_CanvasTkwin(canvas);

    /*
     * Translate the world coordinates of the anchor to screen coordinates.
     */
    Tk_CanvasDrawableCoords(canvas, labelPtr->anchorPos.x, 
                            labelPtr->anchorPos.y, &x, &y);
    switch (labelPtr->state) {
    case TK_STATE_DISABLED:
        gc = labelPtr->disabledLabelGC->gc;  
        bg = labelPtr->disabledBg;  
        break;
    case TK_STATE_ACTIVE:
        bg = labelPtr->activeBg;  
        gc = labelPtr->activeLabelGC->gc;    
        break;
    case TK_STATE_NORMAL:
        bg = labelPtr->normalBg;   
        gc = labelPtr->normalLabelGC->gc;    
        break;
    case TK_STATE_HIDDEN:
        break;
    }
    TkSetRegion(labelPtr->display, gc, labelPtr->clipRegion);
    if (bg != NULL) {                   /* Background polygon */
        Blt_Bg_FillPolygon(tkwin, drawable, bg, x, y, labelPtr->points, 5, 0,
                           TK_RELIEF_FLAT);
    }
    if (labelPtr->lineWidth > 0) {      /* Outline */
        XDrawLines(labelPtr->display, drawable, gc, labelPtr->points, 5, 
                   CoordModeOrigin);
    }
    if (labelPtr->text != NULL) {       /* Text itself */
        Blt_Font font;
        int maxLength;

        maxLength = (labelPtr->flags & ELLIPSIS) ? labelPtr->width : -1;
        font = (labelPtr->scaledFont != NULL) ?
            labelPtr->scaledFont : labelPtr->baseFont;
        XSetFont(labelPtr->display, gc, font);
        Blt_DrawLayout(tkwin, drawable, gc, font, Tk_Depth(tkwin),
            labelPtr->angle, x, y, labelPtr->layoutPtr, maxLength);
        XSetFont(labelPtr->display, gc, None);
    }
    XSetClipMask(labelPtr->display, gc, None);
}

/*
 *---------------------------------------------------------------------------
 *
 * PointProc --
 *
 *      Computes the distance from a given sample point to a label item.
 *
 * Results:
 *      Returns 0 if the sample point is inside the (possibly rotated)
 *      label item.  Otherwise returns the minimum distance of the
 *      projection from the point to the sides of the bounding box.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static double
PointProc(
    Tk_Canvas canvas,                   /* Canvas containing item. */
    Tk_Item *itemPtr,                   /* Label item to check. */
    double *pts)                        /* Array of 2 values representing
                                         * the x,y world coordinate of the
                                         * sample point on the canvas. */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;
    double minDist;
    Point2d sample, p, q;
    int i;

    /* Translate the point where the origin is the label's center. */
    sample.x = pts[0] - (labelPtr->anchorPos.x + labelPtr->rotWidth * 0.5);
    sample.y = pts[1] - (labelPtr->anchorPos.y + labelPtr->rotHeight * 0.5);

    if ((labelPtr->state == TK_STATE_DISABLED) ||
        (labelPtr->state == TK_STATE_HIDDEN)) {
        return FLT_MAX;
    }
    /* Test the sample point to see if it is inside the polygon. */
    if (Blt_PointInPolygon(&sample, labelPtr->outlinePts, 4)) {
        return 0.0;
    }
    /* Otherwise return the distance to the nearest segment of the
     * polygon. */
    p = points[0];
    q = points[1];
    minDist = 1e36;
    for (i = 0; i < 4; i++) {
        Point2d t;
        double x1, x2, y1, y2;
        double dist;
        
        t = Blt_GetProjection((int)sample.x, (int)sample.y, &p, &q);
        if (p.x > q.x) {
            x2 = p.x, x1 = q.x;
        } else {
            x2 = q.x, x1 = p.x;
        }
        if (p.y > q.y) {
            y2 = p.y, y1 = q.y;
        } else {
            y2 = q.y, y1 = p.y;
        }
        p.x = BOUND(t.x, x1, x2);
        p.y = BOUND(t.y, y1, y2);
        dist = hypot(p.x - sample.x, p.y - sample.y);
        if (dist < minDist) {
            minDist = dist;
        }
    }
    return minDist;
}

/*
 *---------------------------------------------------------------------------
 *
 * AreaProc --
 *
 *      This procedure is called to determine whether a label item lies
 *      entirely inside, entirely outside, or overlapping a given
 *      rectangle.
 *
 * Results:
 *      Returns -1 if the item is entirely outside the region given, 0 if
 *      it overlaps, and 1 if the item is entirely inside the given region.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AreaProc(
    Tk_Canvas canvas,                   /* Canvas containing the item. */
    Tk_Item *itemPtr,                   /* Label item to check. */
    double pts[])                       /* Array of 4 values representing
                                         * the opposite two corners in
                                         * world coordinates (x1, y1, x2,
                                         * y2) of the region to test.  */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;

    if ((labelPtr->state == TK_STATE_DISABLED) ||
        (labelPtr->state == TK_STATE_HIDDEN)) {
        return -1;
    }
    if (labelPtr->style.angle != 0.0f) {
        Region2d region;

        /* Translate region so that the center the label is the origin. */
        region.x1 = pts[0] - labelPtr->anchorPos.x + (labelPtr->rotWidth * 0.5);
        region.y1 = pts[1] - labelPtr->anchorPos.y + (labelPtr->rotHeight*0.5);
        region.x2 = pts[2] - labelPtr->anchorPos.x + (labelPtr->rotWidth*0.5);
        region.y2 = pts[3] - labelPtr->anchorPos.y + (labelPtr->rotHeight*0.5);

        /*  
         * First check if the polygon (outline of the label) overlaps the
         * region.  If it doesn't, that means the label is completely
         * outside of the region.
         */
        if (!Blt_RegionInPolygon(&region, labelPtr->outlinePts, 5, TRUE)) {
            return -1;                   /* Outside. */
        }
        /* Test again checking if the polygon is completely enclosed in the
         * region. */
        return Blt_RegionInPolygon(&region, labelPtr->outlinePts, points, 5, 
                                   FALSE /*enclosed*/);
    } 
    /* Simpler test of unrotated label. */
    /* Min-max test of two rectangles. */
    if ((pts[2] < labelPtr->anchorPos.x) || 
        (pts[0] >= (labelPtr->anchorPos.x + labelPtr->rotWidth)) ||
        (pts[3] < labelPtr->anchorPos.y) || 
        (pts[1] >= (labelPtr->anchosPos.y + labelPtr->rotHeight))) {
        return -1;                      /* Completely outside. */
    }
    if ((pts[0] >= labelPtr->anchosPos.x) && 
        (pts[2] < (labelPtr->anchorPos.x + labelPtr->rotWidth)) && 
        (pts[1] >= labelPtr->anchorPos.y) &&
        (pts[3] < (labelPtr->anchorPos.y + labelPtr->rotHeight))) {
        return 1;                       /* Completely inside. */
    }
    return 0;                           /* Overlaps. */
}

/*
 *---------------------------------------------------------------------------
 *
 * ScaleProc --
 *
 *      This procedure is invoked to rescale a label item.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The item referred to by labelPtr is rescaled so that the
 *      following transformation is applied to all point coordinates:
 *              x' = x0 + xs*(x-x0)
 *              y' = y0 + ys*(y-y0)
 *
 *---------------------------------------------------------------------------
 */
static void
ScaleProc(
    Tk_Canvas canvas,                   /* Canvas containing rectangle. */
    Tk_Item *itemPtr,                   /* Label item to be scaled. */
    double x0, double y0,               /* Origin wrt scale rect. */
    double xs, double ys)
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;
    double scale;
    double newFontSize;
    double incrScale;

    incrScale = MAX(xs, ys);
    labelPtr->scale *= incrScale;        /* Used to track overall scale */
    newFontSize = scale * Blt_Font_Size(labelPtr->baseFont);
    labelPtr->flags |= DISPLAY_TEXT;
    if (newFontSize < labelPtr->minFontSize) {
        labelPtr->flags &= ~DISPLAY_TEXT;
    } else if (newFontSize <= labelPtr->maxFontSize) {
        Blt_Font font;
        
        font = Blt_Font_Duplicate(labelPtr->baseFont, newFontSize);
        if (font == NULL) {
            labelPtr->flags &= ~DISPLAY_TEXT;
        }
        if (labelPtr->scaleFont != NULL) {
            Blt_Font_Free(labelPtr->scaleFont);
        }
        labelPtr->scaleFont = font;
    } 
    /* Compute new size of font based upon the xScale and yScale */
    /* Need to track overall scale. */
    /* Handle min/max size limits.  If too small, don't display anything.
    * If too big stop growing. */
    labelPtr->width = (labelPtr->width * xs);
    labelPtr->height = (labelPtr->height * ys);
    ComputeGeometry(labelPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TranslateProc --
 *
 *      This procedure is called to move an item by a given amount.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The position of the item is offset by (dx, dy), and the bounding box
 *      is updated in the generic part of the item structure.
 *
 *---------------------------------------------------------------------------
 */
static void
TranslateProc(
    Tk_Canvas canvas,                   /* Canvas containing item. */
    Tk_Item *itemPtr,                   /* Item that is being moved. */
    double dx, double dy)               /* Amount by which item is to be
                                         * moved. */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;

    /* Translate anchor. */
    labelPtr->anchorPos.x += dx;
    labelPtr->anchorPos.y += dy;

    /* Translate bounding box. */
    labelPtr->item.x1 = ROUND(labelPtr->anchorPos.x);
    labelPtr->item.x2 = ROUND(labelPtr->anchorPos.x + labelPtr->rotWidth);
    labelPtr->item.y1 = ROUND(labelPtr->anchorPos.y);
    labelPtr->item.y2 = ROUND(labelPtr->anchorPos.y + labelPtr->rotHeight);
}

/*
 *---------------------------------------------------------------------------
 *
 * PostScriptProc --
 *
 *      This procedure is called to generate PostScript for label items.
 *
 * Results:
 *      The return value is a standard TCL result.  If an error occurs in
 *      generating PostScript then an error message is left in interp->result,
 *      replacing whatever used to be there.  If no errors occur, then
 *      PostScript output for the item is appended to the interpreter result.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

static int
PostScriptProc(
    Tcl_Interp *interp,                 /* Interpreter to hold generated
                                         * PostScript or reports errors back
                                         * to. */
    Tk_Canvas canvas,                   /* Information about overall
                                         * canvas. */
    Tk_Item *itemPtr,                   /* Label item. */
    int prepass)                        /* If 1, this is a prepass to collect
                                         * font information; 0 means final *
                                         * PostScript is being created. */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;
    Blt_Ps ps;
    double xScale, yScale;
    double x, y, w, h;
    PageSetup setup;

    if ((labelPtr->state == TK_STATE_DISABLED) ||
        (labelPtr->state == TK_STATE_HIDDEN)) {
        return TCL_OK;
    }
    memset(&setup, 0, sizeof(setup));
    ps = Blt_Ps_Create(interp, &setup);

    /* Lower left corner of item on page. */
    x = labelPtr->anchorPos.x;
    y = Tk_CanvasPsY(canvas, labelPtr->anchorPos.y);
    w = labelPtr->rotWidth;
    h = labelPtr->rotHeight;

    if (labelPtr->fileName == NULL) {
        /* No PostScript file, generate PostScript of resized image instead. */
        if (labelPtr->picture != NULL) {
            Blt_Ps_Format(ps, "gsave\n");
            /*
             * First flip the PostScript y-coordinate axis so that the origin
             * is the upper-left corner like our picture.
             */
            Blt_Ps_Format(ps, "  %g %g translate\n", x, y + h);
            Blt_Ps_Format(ps, "  1 -1 scale\n");

            Blt_Ps_DrawPicture(ps, labelPtr->picture, 0.0, 0.0);
            Blt_Ps_Format(ps, "grestore\n");

            Blt_Ps_SetInterp(ps, interp);
            Blt_Ps_Free(ps);
        }
        return TCL_OK;
    }

    /* Copy in the PostScript prolog for EPS encapsulation. */
    if (Blt_Ps_IncludeFile(interp, ps, "bltCanvEps.pro") != TCL_OK) {
        goto error;
    }
    Blt_Ps_Append(ps, "BeginEPSF\n");

    xScale = w / (double)(labelPtr->urx - labelPtr->llx);
    yScale = h / (double)(labelPtr->ury - labelPtr->lly);

    /* Set up scaling and translation transformations for the EPS item */

    Blt_Ps_Format(ps, "%g %g translate\n", x, y);
    Blt_Ps_Format(ps, "%g %g scale\n", xScale, yScale);
    Blt_Ps_Format(ps, "%d %d translate\n", -(labelPtr->llx), -(labelPtr->lly));

    /* FIXME: Why clip against the old bounding box? */
    Blt_Ps_Format(ps, "%d %d %d %d SetClipRegion\n", labelPtr->llx, 
        labelPtr->lly, labelPtr->urx, labelPtr->ury);

    Blt_Ps_VarAppend(ps, "%% including \"", labelPtr->fileName, "\"\n\n",
         (char *)NULL);

    Blt_Ps_AppendBytes(ps, Tcl_DStringValue(&labelPtr->ds), 
        Tcl_DStringLength(&labelPtr->ds));
    Blt_Ps_Append(ps, "EndEPSF\n");
    Blt_Ps_SetInterp(ps, interp);
    Blt_Ps_Free(ps);
    return TCL_OK;

  error:
    Blt_Ps_Free(ps);
    return TCL_ERROR;
}


/*
 * The structures below defines the label item type in terms of procedures
 * that can be invoked by generic item code.
 */
static Tk_ItemType itemType = {
    (char *)"label",                    /* name */
    sizeof(LabelItem),                  /* itemSize */
    CreateProc,         
    configSpecs,                        /* configSpecs */
    ConfigureProc,      
    CoordsProc,
    DeleteProc,
    DisplayProc,
    0,                                  /* alwaysRedraw */
    PointProc,
    AreaProc,
    PostScriptProc,
    ScaleProc,
    TranslateProc,
    (Tk_ItemIndexProc *)NULL,           /* indexProc */
    (Tk_ItemCursorProc *)NULL,          /* icursorProc */
    (Tk_ItemSelectionProc *)NULL,       /* selectionProc */
    (Tk_ItemInsertProc *)NULL,          /* insertProc */
    (Tk_ItemDCharsProc *)NULL,          /* dTextProc */
    (Tk_ItemType *)NULL                 /* nextPtr */
};

/*ARGSUSED*/
void
Blt_RegisterCanvasLabelItem(void)
{
    Tk_CreateItemType(&itemType);
    /* Initialize custom canvas option routines. */
    tagsOption.parseProc = Tk_CanvasTagsParseProc;
    tagsOption.printProc = Tk_CanvasTagsPrintProc;
}

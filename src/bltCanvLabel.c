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
 * o Try to match rectangle item size on rescale.
 * o Use XDrawRectangle for drawing right-angle rotations.
 * o PostScriptProc.  Can it handle gradients?
 * o Test -ellipsis flag
 * o Figure out better minSize, maxSize to dynamically accommodate 
 *   initialize font size better.
 * o Does alwaysRedraw flag fix problem with clipped drawable?
 * o Problem with snapping drawable.  
 * o Write documentation
 */
#define USE_OLD_CANVAS  1
#define DEBUG 1

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include <X11/Xutil.h>

#include <bltAlloc.h>
#include "bltMath.h"
#include "bltChain.h"
#include "bltPaintBrush.h"
#include "bltPicture.h"
#include "bltPs.h"
#include "bltImage.h"
#include "bltPainter.h"

#define BOUND(x, lo, hi)         \
        (((x) > (hi)) ? (hi) : ((x) < (lo)) ? (lo) : (x))

#define LAYOUT_PENDING          (1<<0)
#define ELLIPSIS                (1<<1)
#define DISPLAY_TEXT            (1<<2)
#define ORTHOGONAL              (1<<3)
#define CLIP                    (1<<4)

#define DEF_ACTIVE_DASHES               "0"
#define DEF_ACTIVE_DASH_OFFSET          "0"
#define DEF_ACTIVE_FILL_COLOR           (char *)NULL
#define DEF_ACTIVE_LINEWIDTH            "0"
#define DEF_ACTIVE_OUTLINE_COLOR        STD_ACTIVE_FOREGROUND
#define DEF_ANCHOR                      "nw"
#define DEF_DISABLED_DASHES             "0"
#define DEF_DISABLED_DASH_OFFSET        "0"
#define DEF_DISABLED_FILL_COLOR         (char *)NULL
#define DEF_DISABLED_LINEWIDTH          "0"
#define DEF_DISABLED_OUTLINE_COLOR      STD_DISABLED_FOREGROUND
#define DEF_FONT                        STD_FONT_NORMAL
#define DEF_HEIGHT                      "0"
#define DEF_MAXFONTSIZE                 "-1"
#define DEF_MINFONTSIZE                 "-1"
#define DEF_NORMAL_DASHES               "0"
#define DEF_NORMAL_DASH_OFFSET          "0"
#define DEF_NORMAL_FILL_COLOR           (char *)NULL
#define DEF_NORMAL_LINEWIDTH            "0"
#define DEF_NORMAL_OUTLINE_COLOR        STD_NORMAL_FOREGROUND
#define DEF_PADX                        "2"
#define DEF_PADY                        "2"
#define DEF_ROTATE                      "0"
#define DEF_STATE                       "normal"
#define DEF_TAGS                        (char *)NULL
#define DEF_TEXT                        (char *)NULL
#define DEF_TEXTANCHOR                  "center"
#define DEF_WIDTH                       "0"
#define DEF_XSCALE                      "1.0"
#define DEF_YSCALE                      "1.0"

/* Key that uniquely describes the label's GC. */
typedef struct {
    Display *display;
    unsigned long pixel;
    int lineWidth;
    int dashes;
    int dashOffset;
} LabelGCKey;

typedef struct {
    int refCount;
    GC gc;
    Blt_HashEntry *hashPtr;
} LabelGC;

typedef  struct {
    int dashes;                         /* Dashes on/off pattern. If zero,
                                         * outline is drawn solid. */
    int dashOffset;                     /* Offset of dash pattern. This is
                                         * used to create the "marching
                                         * ants" effect. */
    int lineWidth;                      /* Line width of outline. If zero,
                                         * no outline is drawn. */
    XColor *fgColor;                    /* Outline and text color. */
    XColor *bgColor;                    /* Fill color. */
    Blt_PaintBrush brush;               /* Fill background color. */
    LabelGC *labelGC;                   /* Graphics context to draw text
                                         * and outline. */
}  StateAttributes;

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
    unsigned int flags;
    Display *display;
    Tcl_Interp *interp;
    Tk_Window tkwin;
    Tk_Canvas canvas;                   /* Canvas containing the label item. */
    int textWidth, textHeight;		/* Unrotated dimensions of item. */
    double width, height;               /* Unrotated dimensions of the item. 
                                         * Could be scaled. */
    double fontSize;			/* Current font size. */

    /* User configurable fields */
    double x, y;                        /* Requested anchor in canvas
                                         * coordinates of the label item */
    double angle;			/* Angle to rotate the label. */
    Tk_Anchor anchor;                   /* Indicates how to anchor the *
                                         * the label item according to its
                                         * x,y position  */
    int textAnchor;                     /* Anchors the text within the
                                         * label background.  The default
                                         * is center. */
    int reqWidth, reqHeight;            /* Requested dimension of label
                                         * item in canvas
                                         * coordinates. These are the
                                         * unrotated dimensions of the
                                         * item.  If non-zero, they
                                         * override the dimension computed
                                         * from the normal size of the
                                         * text. */
    Blt_Pad xPad, yPad;                 /* Horizonal and vertical padding
                                         * around the label's text (adds to
                                         * the width and height of the
                                         * background. */
    int state;                          /* State of item: TK_STATE_HIDDEN,
                                         * TK_STATE_NORMAL,
                                         * TK_STATE_ACTIVE, or
                                         * TK_STATE_DISABLED. Selects one of
                                         * the attributes structures
                                         * below. */
    StateAttributes normal, active, disabled;

    const char *text;			/* Text string to be displayed.
					 * The string may contain newlines,
					 * but not tabs. */
    int numBytes;                       /* # of bytes in above string. */
    int maxFontSize, minFontSize;
    Blt_Font baseFont;                  /* Base font for item.  This is the
                                         * unscaled font. */
    /* Computed values. */
    Blt_Font scaledFont;                /* If non-NULL, is the base font at
                                         * the current scale factor. */
    double rotWidth, rotHeight;         /* Rotated width and height of
                                         * region occupied by label. */
    double xScale, yScale;
    TextLayout *layoutPtr;              /* Contains positions of the text
                                         * in label coordinates. */
    XPoint points[5];                   /* Points representing the rotated
                                         * polygon (outline) of the
                                         * bounding box in screen
                                         * coordinates. This is used to
                                         * draw the rotated background and
                                         * rectangle and to determine if
                                         * the text is visible on the
                                         * screen. */
    Point2d anchorPos;                  /* Anchor-translated position
                                         * (upper-left corner) of the label
                                         * in world coordinates. */
    Point2d outlinePts[5];              /* Points representing the rotated
                                         * polygon (outline) of the item's
                                         * bounding box in label
                                         * coordinates. */
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

static int StringToBrush(ClientData clientData, Tcl_Interp *interp,
        Tk_Window tkwin, const char *string, char *widgRec, int offset);
#if (_TCL_VERSION >= _VERSION(8,6,0)) 
static const char *BrushToString (ClientData clientData, Tk_Window tkwin, 
        char *widgRec, int offset, Tcl_FreeProc **proc);
#else
static char *BrushToString (ClientData clientData, Tk_Window tkwin, 
        char *widgRec, int offset, Tcl_FreeProc **proc);
#endif

static Tk_CustomOption brushOption = {
    StringToBrush, BrushToString, (ClientData)0
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
    {TK_CONFIG_CUSTOM, (char *)"-activedashes", (char *)NULL, (char *)NULL,  
        DEF_ACTIVE_DASHES, Tk_Offset(LabelItem, active.dashes), 
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_CUSTOM, (char *)"-activedashoffset", (char *)NULL, (char *)NULL, 
        DEF_ACTIVE_DASH_OFFSET, Tk_Offset(LabelItem, active.dashOffset), 
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_SYNONYM, (char *)"-activefg", "activeOutline", 
        (char *)NULL, (char *)NULL, 0, 0},
    {TK_CONFIG_CUSTOM, (char *)"-activefill", "activeFill", (char *)NULL,
        DEF_ACTIVE_FILL_COLOR, Tk_Offset(LabelItem, active.brush), 
        TK_CONFIG_NULL_OK, &brushOption},
    {TK_CONFIG_SYNONYM, (char *)"-activeforeground", "activeOutline", 
        (char *)NULL, (char *)NULL, 0, 0},
    {TK_CONFIG_CUSTOM, (char *)"-activelinewidth", (char *)NULL, (char *)NULL,
        DEF_ACTIVE_LINEWIDTH, Tk_Offset(LabelItem, active.lineWidth),
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_COLOR, (char *)"-activeoutline", "activeOutline", (char *)NULL,
        DEF_ACTIVE_OUTLINE_COLOR, Tk_Offset(LabelItem, active.fgColor)},
    {TK_CONFIG_ANCHOR, (char *)"-anchor", (char *)NULL, (char *)NULL,
        DEF_ANCHOR, Blt_Offset(LabelItem, anchor), TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_SYNONYM, (char *)"-bg", "fill", (char *)NULL, (char *)NULL, 
        0, 0},
    {TK_CONFIG_SYNONYM, (char *)"-background", "fill", (char *)NULL, 
        (char *)NULL, 0, 0},
    {TK_CONFIG_CUSTOM, (char *)"-dashes", (char *)NULL, (char *)NULL,  
        DEF_NORMAL_DASHES, Blt_Offset(LabelItem, normal.dashes), 
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_CUSTOM, (char *)"-dashoffset", (char *)NULL, (char *)NULL,  
        DEF_NORMAL_DASH_OFFSET, Tk_Offset(LabelItem, normal.dashOffset), 
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_SYNONYM, (char *)"-disabledbg", "disabledFill", (char *)NULL, 
        (char *)NULL, 0, 0},
    {TK_CONFIG_SYNONYM, (char *)"-disabledbackground", "disabledFill", 
        (char *)NULL, (char *)NULL, 0, 0},
    {TK_CONFIG_CUSTOM, (char *)"-disableddashes", (char *)NULL, (char *)NULL,  
        DEF_DISABLED_DASHES, Tk_Offset(LabelItem, disabled.dashes), 
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_CUSTOM, (char *)"-disableddashoffset", (char *)NULL, 
        (char *)NULL,  DEF_ACTIVE_DASH_OFFSET, 
        Blt_Offset(LabelItem, disabled.dashOffset), TK_CONFIG_DONT_SET_DEFAULT, 
        &bltDistanceOption},
    {TK_CONFIG_SYNONYM, (char *)"-disabledfg", "disabledOutline", (char *)NULL, 
        (char *)NULL, 0, 0},
    {TK_CONFIG_CUSTOM, (char *)"-disabledfill", "disabledFill", (char *)NULL,
        DEF_DISABLED_FILL_COLOR, Tk_Offset(LabelItem, disabled.brush), 
        TK_CONFIG_NULL_OK, &brushOption},
    {TK_CONFIG_SYNONYM, (char *)"-disabledforeground", "disabledOutline", 
        (char *)NULL, (char *)NULL, 0, 0},
    {TK_CONFIG_CUSTOM, (char *)"-disabledlinewidth", (char *)NULL, (char *)NULL,
        DEF_DISABLED_LINEWIDTH, Tk_Offset(LabelItem, disabled.lineWidth),
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_COLOR, (char *)"-disabledoutline", "disabledOutline", 
        (char *)NULL, DEF_DISABLED_OUTLINE_COLOR, 
        Tk_Offset(LabelItem, disabled.fgColor)},
    {TK_CONFIG_CUSTOM, (char *)"-fill", "fill", (char *)NULL,
        DEF_NORMAL_FILL_COLOR, Tk_Offset(LabelItem, normal.brush), 
        TK_CONFIG_NULL_OK, &brushOption},
    {TK_CONFIG_CUSTOM, (char *)"-font", (char *)NULL, (char *)NULL,
        DEF_FONT, Tk_Offset(LabelItem, baseFont), 0, &fontOption},
    {TK_CONFIG_SYNONYM, (char *)"-fg", "fill", (char *)NULL, (char *)NULL, 
        0, 0},
    {TK_CONFIG_SYNONYM, (char *)"-foreground", "fill", (char *)NULL, 
        (char *)NULL, 0, 0},
    {TK_CONFIG_CUSTOM, (char *)"-height", (char *)NULL, (char *)NULL,
        DEF_HEIGHT, Tk_Offset(LabelItem, reqHeight),
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_ANCHOR, (char *)"-textanchor", (char *)NULL, (char *)NULL,
        DEF_TEXTANCHOR, Blt_Offset(LabelItem, textAnchor),
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_CUSTOM, (char *)"-linewidth", (char *)NULL, (char *)NULL,
        DEF_NORMAL_LINEWIDTH, Tk_Offset(LabelItem, normal.lineWidth),
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_INT, (char *)"-maxfontsize", (char *)NULL, (char *)NULL,
        DEF_MAXFONTSIZE, Tk_Offset(LabelItem, maxFontSize), 
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_INT, (char *)"-minfontsize", (char *)NULL, (char *)NULL,
        DEF_MINFONTSIZE, Tk_Offset(LabelItem, minFontSize), 
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_COLOR, (char *)"-outline", "outline", (char *)NULL,
        DEF_NORMAL_OUTLINE_COLOR, Tk_Offset(LabelItem, normal.fgColor)},
    {TK_CONFIG_CUSTOM, (char *)"-padx", (char *)NULL, (char *)NULL,
        DEF_PADX, Tk_Offset(LabelItem, xPad),
        TK_CONFIG_DONT_SET_DEFAULT, &bltPadOption},
    {TK_CONFIG_CUSTOM, (char *)"-pady", (char *)NULL, (char *)NULL,
        DEF_PADY, Tk_Offset(LabelItem, yPad),
        TK_CONFIG_DONT_SET_DEFAULT, &bltPadOption},
    {TK_CONFIG_DOUBLE, (char *)"-rotate", (char *)NULL, (char *)NULL,
        DEF_ROTATE, Tk_Offset(LabelItem, angle),
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_CUSTOM, (char *)"-state", (char *)NULL, (char *)NULL,
        DEF_STATE, Tk_Offset(LabelItem, state), TK_CONFIG_DONT_SET_DEFAULT, 
        &stateOption},
    {TK_CONFIG_CUSTOM, (char *)"-tags", (char *)NULL, (char *)NULL,
        DEF_TAGS, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_STRING, (char *)"-text", (char *)NULL, (char *)NULL,
        DEF_TEXT, Tk_Offset(LabelItem, text), TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, (char *)"-width", (char *)NULL, (char *)NULL,
        DEF_WIDTH, Tk_Offset(LabelItem, reqWidth),
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_DOUBLE, (char *)"-xscale", (char *)NULL, (char *)NULL,
        DEF_XSCALE, Tk_Offset(LabelItem, xScale),
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_DOUBLE, (char *)"-yscale", (char *)NULL, (char *)NULL,
        DEF_YSCALE, Tk_Offset(LabelItem, yScale),
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
};

/* Prototypes for procedures defined in this file: */
static Tk_ItemCoordProc CoordsProc;
static Tk_ItemAreaProc AreaProc;
static Tk_ItemPointProc PointProc;
static Tk_ItemConfigureProc ConfigureProc;
static Tk_ItemCreateProc CreateProc;
static Tk_ItemDeleteProc DeleteProc;
static Tk_ItemScaleProc ScaleProc;
static Tk_ItemTranslateProc TranslateProc;
static Tk_ItemDisplayProc DisplayProc;
static Tk_ItemPostscriptProc PostScriptProc;

static void ComputeGeometry(LabelItem *itemPtr);


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
 * StringToBrush --
 *
 *      Converts string to Blt_PainBrush structure.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StringToBrush(ClientData clientData,Tcl_Interp *interp, Tk_Window tkwin,
              const char *string,  char *widgRec, int offset)
{
    Blt_PaintBrush *brushPtr = (Blt_PaintBrush *)(widgRec + offset);

    return Blt_GetPaintBrush(interp, string, brushPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * BrushToString --
 *
 *      Returns the name of the Blt_PaintBrush.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
#if (_TCL_VERSION >= _VERSION(8,6,0)) 
static const char *
#else
static char *
#endif
BrushToString(ClientData clientData, Tk_Window tkwin, char *widgRec,
              int offset, Tcl_FreeProc **freeProcPtr)
{
    Blt_PaintBrush brush = *(Blt_PaintBrush *)(widgRec + offset);
    const char *string;

    string = (brush != NULL) ? Blt_GetBrushName(brush) : "";
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
    length = strlen(string);
    if ((c == 'd') && (strncmp(string, "disabled", length) == 0)) {
        *statePtr = TK_STATE_DISABLED;
    } else if ((c == 'a') && (strncmp(string, "active", length) == 0)) {
        *statePtr = TK_STATE_ACTIVE;
    } else if ((c == 'n') && (strncmp(string, "normal", length) == 0)) {
        *statePtr = TK_STATE_NORMAL;
    } else if ((c == 'h') && (strncmp(string, "hidden", length) == 0)) {
        *statePtr = TK_STATE_HIDDEN;
    } else {
        Tcl_AppendResult(interp, "unknown state \"", string, 
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
StateToString(ClientData clientData, Tk_Window tkwin, char *widgRec,
              int offset, Tcl_FreeProc **freeProcPtr)
{
    unsigned int state = *(unsigned int *)(widgRec + offset);
    const char *string;

    switch (state) {
    case TK_STATE_DISABLED:
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
GetLabelGC(Tk_Window tkwin, XColor *colorPtr, int lineWidth, int dashes, 
           int dashOffset)
{
    LabelGCKey key;
    LabelGC *gcPtr;
    Blt_HashEntry *hPtr;
    int isNew;
    
    memset(&key, 0, sizeof(key));
    key.pixel = colorPtr->pixel;
    key.display = Tk_Display(tkwin);
    key.lineWidth = lineWidth;
    key.dashes = dashes;
    key.dashOffset = dashOffset;
    hPtr = Blt_CreateHashEntry(&gcTable, (char *)&key, &isNew);
    if (isNew) {
        unsigned int gcMask;
        XGCValues gcValues;
        GC newGC;
        
        gcMask = GCForeground | GCLineWidth;
        gcValues.foreground = colorPtr->pixel;
         gcValues.line_width = lineWidth;
        if (dashes > 0) {
            gcMask |= (GCLineStyle | GCDashList | GCDashOffset);
            gcValues.line_style = LineOnOffDash;
            gcValues.dashes = dashes;
            gcValues.dash_offset = dashOffset;
        }
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
FreeLabelGC(Display *display, LabelGC *gcPtr)
{
    gcPtr->refCount--;
    if (gcPtr->refCount <= 0) {
        if (gcPtr->gc != NULL) {
            Blt_FreePrivateGC(display, gcPtr->gc);
        }
        Blt_DeleteHashEntry(&gcTable, gcPtr->hashPtr);
        Blt_Free(gcPtr);
    }
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
    double w, h, rw, rh;
    int i;

        Blt_Font font;
        
        font = (labelPtr->scaledFont != NULL) ?
            labelPtr->scaledFont : labelPtr->baseFont;
#if DEBUG
    fprintf(stderr, "Enter ComputeGeometry label=%s\n", labelPtr->text);
#endif
    labelPtr->flags &= ~CLIP;
    if (labelPtr->numBytes == 0) {
        w = h = 0;
    } else {
        TextStyle ts;
        TextLayout *layoutPtr;
        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetFont(ts, font);
        Blt_Ts_SetPadding(ts, labelPtr->xPad.side1, labelPtr->xPad.side2,
                          labelPtr->yPad.side1, labelPtr->yPad.side2);
        layoutPtr = Blt_Ts_CreateLayout(labelPtr->text, labelPtr->numBytes, &ts);
        if (labelPtr->layoutPtr != NULL) {
            Blt_Free(labelPtr->layoutPtr);
        }
        labelPtr->layoutPtr = layoutPtr;
        
        /* Let the requested width and height override the computed size. */
        w = (labelPtr->reqWidth > 0) ? labelPtr->reqWidth : layoutPtr->width;
        h = (labelPtr->reqHeight > 0) ? labelPtr->reqHeight : layoutPtr->height;
    }
    labelPtr->width = labelPtr->xScale * w;
    labelPtr->height = labelPtr->yScale * h;
    if ((labelPtr->width < labelPtr->layoutPtr->width) ||
        (labelPtr->height < labelPtr->layoutPtr->height)) {
        labelPtr->flags |= CLIP;    /* Turn on clipping of text. */
    }
    fprintf(stderr, "sw=%g sh=%g\n", labelPtr->width, labelPtr->height);
    /* Compute the outline polygon (isolateral or rectangle) given the
     * width and height. The center of the box is 0,0. */
    Blt_GetBoundingBox(labelPtr->width, labelPtr->height, labelPtr->angle, 
        &rw, &rh, labelPtr->outlinePts);
    labelPtr->rotWidth = rw;
    labelPtr->rotHeight = rh;
    /* The label's x,y position is in world coordinates. This point and the
     * anchor tell us where is the anchor position of the label, which is
     * the upper-left corner of the bounding box around the possibly
     * rotated item. */
    labelPtr->anchorPos = Blt_AnchorPoint(labelPtr->x, labelPtr->y, rw, rh, 
                                          labelPtr->anchor);
    fprintf(stderr, "x1=%g y1=%g x2=%g y2=%g x2r=%g y2r=%g\n", 
            labelPtr->anchorPos.x, labelPtr->anchorPos.y,
            labelPtr->anchorPos.x + labelPtr->width, 
            labelPtr->anchorPos.y + labelPtr->height,
            labelPtr->anchorPos.x + labelPtr->rotWidth, 
            labelPtr->anchorPos.y + labelPtr->rotHeight);
    for (i = 0; i < 4; i++) {
        labelPtr->outlinePts[i].x += rw * 0.5;
        labelPtr->outlinePts[i].y += rh * 0.5;
    }
    labelPtr->outlinePts[4] = labelPtr->outlinePts[0];

    if (labelPtr->layoutPtr != NULL) {
        Point2d off1, off2;
        double radians, sinTheta, cosTheta;
        int xOffset, yOffset;

        /* Compute the starting positions of the text. This also encompasses
         * justification. */
        switch (labelPtr->textAnchor) {
        case TK_ANCHOR_NW:
        case TK_ANCHOR_W:
        case TK_ANCHOR_SW:
            xOffset = 0;
            break;
        case TK_ANCHOR_N:
        case TK_ANCHOR_CENTER:
        case TK_ANCHOR_S:
            xOffset = (labelPtr->width - labelPtr->layoutPtr->width) / 2;
            break;
        case TK_ANCHOR_NE:
        case TK_ANCHOR_E:
        case TK_ANCHOR_SE:
            xOffset = labelPtr->width - labelPtr->layoutPtr->width;
            break;
        }
        switch (labelPtr->textAnchor) {
        case TK_ANCHOR_NW:
        case TK_ANCHOR_N:
        case TK_ANCHOR_NE:
            yOffset = 0;
            break;
        case TK_ANCHOR_W:
        case TK_ANCHOR_CENTER:
        case TK_ANCHOR_E:
            yOffset = (labelPtr->height - labelPtr->layoutPtr->height) / 2;
            break;
        case TK_ANCHOR_SW:
        case TK_ANCHOR_S:
        case TK_ANCHOR_SE:
            yOffset = labelPtr->height - labelPtr->layoutPtr->height;
            break;
        }
        
        /* Offset to center of unrotated box. */
        off1.x = (double)labelPtr->width * 0.5 - xOffset;
        off1.y = (double)labelPtr->height * 0.5 - yOffset;
        /* Offset to center of rotated box. */
        off2.x = rw * 0.5;
        off2.y = rh * 0.5;
        radians = -labelPtr->angle * DEG2RAD;
        sinTheta = sin(radians), cosTheta = cos(radians);
        
        /* Rotate starting positions of the text. */
        for (i = 0; i < labelPtr->layoutPtr->numFragments; i++) {
            Point2d p, q;
            TextFragment *fragPtr;
            
            fragPtr = labelPtr->layoutPtr->fragments + i;
            /* Translate the start of the fragement to the center of box. */
            p.x = fragPtr->x - off1.x;
            p.y = fragPtr->y - off1.y;
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
    labelPtr->header.x1 = ROUND(labelPtr->anchorPos.x);
    labelPtr->header.x2 = ROUND(labelPtr->anchorPos.x + labelPtr->rotWidth) + 1;
    labelPtr->header.y1 = ROUND(labelPtr->anchorPos.y);
    labelPtr->header.y2 = ROUND(labelPtr->anchorPos.y + labelPtr->rotHeight) + 1;
}



/*
 *---------------------------------------------------------------------------
 *
 * GetClipRegion --
 *
 *      For right-angle rotations, generate the clip region from the extents
 *      of the label item.
 *
 *      For other rotations, generate the clip region from the translated
 *      (to drawable coordinates) outline polygon.
 *
 *---------------------------------------------------------------------------
 */
static TkRegion
GetClipRegion(LabelItem *labelPtr, int x, int y)
{
    TkRegion clipRegion;

    if ((labelPtr->flags & CLIP) == 0) {
        return None;
    }
    if (labelPtr->flags & ORTHOGONAL) {
        XRectangle r;

        /* Text clip routines don't like negative clip coordinates. */
        r.x = (x < 0) ? 0 : x;
        r.y = (y < 0) ? 0 : y;
        r.width  = x + ROUND(labelPtr->width);
        r.height = y + ROUND(labelPtr->height);
        if ((r.width <= 0) || (r.height <= 0)) {
            return None;
        }
        clipRegion = TkCreateRegion();
        TkUnionRectWithRegion(&r, clipRegion, clipRegion);
    } else {
        XPoint points[5];
        int i;
            
        /* Text clip routines don't like negative clip coordinates. */
        for (i = 0; i < 5; i++) {
            points[i].x = labelPtr->points[i].x;
            points[i].y = labelPtr->points[i].y;
            if (points[i].x < 0) {
                points[i].x = 0;
            }
            if (points[i].y < 0) {
                points[i].y = 0;
            }
        }
        clipRegion = (TkRegion)XPolygonRegion(points, 5, EvenOddRule);
    }
    return clipRegion;
}

static void
MapLabel(LabelItem *labelPtr, int x, int y)
{
    int i;
    
#if DEBUG
    fprintf(stderr, "Enter MapLabel label=%s\n", labelPtr->text);
#endif
    /* Map the outline relative to the screen anchor point. */
    for (i = 0; i < 5; i++) {
        labelPtr->points[i].x = ROUND(labelPtr->outlinePts[i].x + x);
        labelPtr->points[i].y = ROUND(labelPtr->outlinePts[i].y + y);
    }
}

static int
LabelInsideRegion(LabelItem *labelPtr, int rx, int ry, int rw, int rh)
{
    Region2d region;
    int state;

    /* Test to see if the rotated bounding box of the label is inside the
     * region. */
    region.left = rx - labelPtr->anchorPos.x;
    region.top = ry - labelPtr->anchorPos.y;
    region.right = rx + rw;
    region.bottom = ry + rh;
    state = Blt_PolygonInRegion(labelPtr->outlinePts, 5, &region, FALSE);
    fprintf(stderr, "LabelInsideRegion state =%d\n", state);
    return state;
}


/*
 *---------------------------------------------------------------------------
 *
 * FillBackground --
 *
 *      For opaque backgrounds, use the normal X drawing routine to draw
 *      either background rectangle or isolateral.
 *
 *      For special paintbrushes (semi-transparent or gradients, etc) draw
 *      the polygon into a picture and then composite the picture into the
 *      canvas. This means taking a snapshot of the canvas drawable. 
 *
 *---------------------------------------------------------------------------
 */
static void
FillBackground(Tk_Window tkwin, Drawable drawable, LabelItem *labelPtr,
               int x, int y, Blt_PaintBrush brush, XColor *bgColor)
{
    if ((Blt_GetBrushType(brush) == BLT_PAINTBRUSH_COLOR) &&
        (Blt_GetBrushAlpha(brush) == 0xFF)) {
        GC gc;

        /* For opaque backgrounds, use the normal X drawing routine to draw
         * either background rectangle or isolateral. */
        gc = Tk_GCForColor(bgColor, drawable);
        if (labelPtr->flags & ORTHOGONAL) {
            XFillRectangle(Tk_Display(tkwin), drawable, gc, x, y, 
                ROUND(labelPtr->width), ROUND(labelPtr->height));
        } else {
            XFillPolygon(Tk_Display(tkwin), drawable, gc, labelPtr->points, 5, 
                Convex, CoordModeOrigin);
        }
    } else {
        int w, h;
        Blt_Painter painter;
        Blt_Picture picture;
        
        w = ROUND(labelPtr->rotWidth);
        h = ROUND(labelPtr->rotHeight);
        picture = Blt_DrawableToPicture(tkwin, drawable, x, y, w, h, 1.0);
        if (picture == NULL) {
            return;                         /* Background is obscured. */
        }
        Blt_SetBrushRegion(brush, 0, 0, w, h);
        Blt_PaintPolygon(picture, 5, labelPtr->outlinePts, brush);
        painter = Blt_GetPainter(tkwin, 1.0);
        Blt_PaintPicture(painter, drawable, picture, 0, 0, w, h, x, y, 0);
        Blt_FreePicture(picture);
    }
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

#if DEBUG
    fprintf(stderr, "Enter DeleteProc label=%s\n", labelPtr->text);
#endif
    Tk_FreeOptions(configSpecs, (char *)labelPtr, display, 0);
    if (labelPtr->scaledFont != NULL) {
        Blt_Font_Free(labelPtr->scaledFont);
    }
    if (labelPtr->normal.labelGC != NULL) {
        FreeLabelGC(labelPtr->display, labelPtr->normal.labelGC);
    }
    if (labelPtr->disabled.labelGC != NULL) {
        FreeLabelGC(labelPtr->display, labelPtr->disabled.labelGC);
    }
    if (labelPtr->active.labelGC != NULL) {
        FreeLabelGC(labelPtr->display, labelPtr->active.labelGC);
    }
    if (labelPtr->normal.brush != NULL) {
        Blt_FreeBrush(labelPtr->normal.brush);
    }
    if (labelPtr->active.brush != NULL) {
        Blt_FreeBrush(labelPtr->active.brush);
    }
    if (labelPtr->disabled.brush != NULL) {
        Blt_FreeBrush(labelPtr->disabled.brush);
    }
    if (labelPtr->normal.bgColor != NULL) {
        Tk_FreeColor(labelPtr->normal.bgColor);
    }
    if (labelPtr->active.bgColor != NULL) {
        Tk_FreeColor(labelPtr->active.bgColor);
    }
    if (labelPtr->disabled.bgColor != NULL) {
        Tk_FreeColor(labelPtr->disabled.bgColor);
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

#if DEBUG
    fprintf(stderr, "Enter CreateProc\n");
#endif
    if (!initialized) {
        Blt_InitHashTable(&gcTable, sizeof(LabelGCKey) / sizeof(int));
        initialized = TRUE;
    }
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

    labelPtr->anchor = TK_ANCHOR_NW;
    labelPtr->canvas = canvas;
    labelPtr->display = Tk_Display(tkwin);
    labelPtr->interp = interp;
    labelPtr->xScale = labelPtr->yScale = 1.0;
    labelPtr->state = TK_STATE_NORMAL;
    labelPtr->textAnchor = TK_ANCHOR_NW;
    labelPtr->tkwin  = tkwin;
    labelPtr->x = x;
    labelPtr->xPad.side1 = labelPtr->xPad.side2 = 2;
    labelPtr->y = y;
    labelPtr->yPad.side1 = labelPtr->yPad.side2 = 2;
    if (ConfigureProc(interp, canvas, itemPtr, argc - 2, argv + 2, 0) 
        != TCL_OK) {
        DeleteProc(canvas, itemPtr, Tk_Display(tkwin));
        return TCL_ERROR;
    }
    ComputeGeometry(labelPtr);
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
    XColor *colorPtr;
    
#if DEBUG
    fprintf(stderr, "Enter ConfigureProc label=%s\n", labelPtr->text);
#endif
    tkwin = Tk_CanvasTkwin(canvas);
    if (Tk_ConfigureWidget(interp, tkwin, configSpecs, argc, (const char**)argv,
                (char *)labelPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    labelPtr->angle = FMOD(labelPtr->angle, 360.0);
    if (labelPtr->angle < 0.0) {
        labelPtr->angle += 360.0;
    }
    if (Blt_OldConfigModified(configSpecs, "-rotate", "-*fontsize", 
                              "-font", "-pad*", "-width",
                              "-height", "-anchor", "-linewidth", "-text",
                              (char *)NULL)) {
        ComputeGeometry(labelPtr);
    }
    /* Check if the label is a right-angle rotation.  */
    if (FMOD(labelPtr->angle, 90.0) == 0.0) {
        labelPtr->flags |= ORTHOGONAL;
    } else {
        labelPtr->flags &= ~ORTHOGONAL;
    }
    if (labelPtr->text == NULL) {
        labelPtr->numBytes = 0;
    } else {
        labelPtr->numBytes = strlen(labelPtr->text);
    }
    labelPtr->fontSize = Blt_Font_Size(labelPtr->baseFont);
    if (labelPtr->angle != 0.0) {
        if (!Blt_Font_CanRotate(labelPtr->baseFont, labelPtr->angle)) {
            fprintf(stderr, "can't rotate font %s\n", 
                    Blt_Font_Name(labelPtr->baseFont));
        }
    }

    /* Update only the GC for the specified-state. */
    switch (labelPtr->state) {
    case TK_STATE_NORMAL:
        newLabelGC = GetLabelGC(tkwin, labelPtr->normal.fgColor,
             labelPtr->normal.lineWidth, labelPtr->normal.dashes,
             labelPtr->normal.dashOffset);
        if (labelPtr->normal.labelGC != NULL) {
            FreeLabelGC(labelPtr->display, labelPtr->normal.labelGC);
        }
        labelPtr->normal.labelGC = newLabelGC;
        if (labelPtr->normal.brush != NULL) {
            colorPtr = Blt_GetXColorFromBrush(tkwin, labelPtr->normal.brush);
        } else {
            colorPtr = NULL;
        }
        if (labelPtr->normal.bgColor != NULL) {
            Tk_FreeColor(labelPtr->normal.bgColor);
        }
        labelPtr->normal.bgColor = colorPtr;
        break;
    case TK_STATE_DISABLED:
        newLabelGC = GetLabelGC(tkwin, labelPtr->disabled.fgColor, 
                labelPtr->disabled.lineWidth, labelPtr->disabled.dashes, 
                labelPtr->disabled.dashOffset);
        if (labelPtr->disabled.labelGC != NULL) {
            FreeLabelGC(labelPtr->display, labelPtr->disabled.labelGC);
        }
        labelPtr->disabled.labelGC = newLabelGC;
        if (labelPtr->disabled.brush != NULL) {
            colorPtr = Blt_GetXColorFromBrush(tkwin, labelPtr->disabled.brush);
        } else {
            colorPtr = NULL;
        }
        if (labelPtr->disabled.bgColor != NULL) {
            Tk_FreeColor(labelPtr->disabled.bgColor);
        }
        labelPtr->disabled.bgColor = colorPtr;
        break;
    case TK_STATE_ACTIVE:
        newLabelGC = GetLabelGC(tkwin, labelPtr->active.fgColor,
                labelPtr->active.lineWidth, labelPtr->active.dashes,
                labelPtr->active.dashOffset);
        if (labelPtr->active.labelGC != NULL) {
            FreeLabelGC(labelPtr->display, labelPtr->active.labelGC);
        }
        labelPtr->active.labelGC = newLabelGC;
        if (labelPtr->active.brush != NULL) {
            colorPtr = Blt_GetXColorFromBrush(tkwin, labelPtr->active.brush);
        } else {
            colorPtr = NULL;
        }
        if (labelPtr->active.bgColor != NULL) {
            Tk_FreeColor(labelPtr->active.bgColor);
        }
        labelPtr->active.bgColor = colorPtr;
        break;
    }
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

#if DEBUG
    fprintf(stderr, "Enter CoordsProc label=%s\n", labelPtr->text);
#endif
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

    /* Translate the test point from world coordinates to label
     * coordinates, where the origin is the label's upper-left corner. */
    sample.x = pts[0] - labelPtr->anchorPos.x;
    sample.y = pts[1] - labelPtr->anchorPos.y;

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
    minDist = 1e36;
    for (i = 0; i < 4; i++) {
        Point2d t;
        double x1, x2, y1, y2;
        double dist;
        
        p = labelPtr->outlinePts[i];
        q = labelPtr->outlinePts[i+1];
        t = Blt_GetProjection(ROUND(sample.x), ROUND(sample.y), &p, &q);
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

#if DEBUG
    fprintf(stderr, "Enter AreaProc label=%s x1=%g, y1=%g x2=%g y2=%g\n", 
            labelPtr->text, pts[0], pts[1], pts[2], pts[3]);
#endif
    if ((labelPtr->state == TK_STATE_DISABLED) ||
        (labelPtr->state == TK_STATE_HIDDEN)) {
        return -1;
    }
    if (labelPtr->angle != 0.0) {
        Region2d region;

        /* Translate the test region to label coordinates. */
        region.left =   pts[0] - labelPtr->anchorPos.x;
        region.top =    pts[1] - labelPtr->anchorPos.y;
        region.right =  pts[2] - labelPtr->anchorPos.x;
        region.bottom = pts[3] - labelPtr->anchorPos.y;

        /*  
         * First check if the polygon (outline of the label) overlaps the
         * region.  If it doesn't, that means the label is completely
         * outside of the region.
         */
        if (!Blt_PolygonInRegion(labelPtr->outlinePts, 5, &region, TRUE)) {
            return -1;                   /* Outside. */
        }
        /* Test again checking if the polygon is completely enclosed in the
         * region. */
        return Blt_PolygonInRegion(labelPtr->outlinePts, 5, &region, FALSE);
    } 
    /* Simpler test of unrotated label. */
    /* Min-max test of two rectangles. */
    if ((pts[2] < labelPtr->anchorPos.x) || 
        (pts[0] >= (labelPtr->anchorPos.x + labelPtr->rotWidth)) ||
        (pts[3] < labelPtr->anchorPos.y) || 
        (pts[1] >= (labelPtr->anchorPos.y + labelPtr->rotHeight))) {
        return -1;                      /* Completely outside. */
    }
    if ((pts[0] >= labelPtr->anchorPos.x) && 
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
    double xOrigin, double yOrigin,     /* Origin wrt scale rect. */
    double xScale, double yScale)
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;
    double newFontSize;
    
    labelPtr->xScale *= xScale;        /* Used to track overall scale */
    labelPtr->yScale *= yScale;

    newFontSize = MAX(labelPtr->xScale, labelPtr->yScale) *
        Blt_Font_Size(labelPtr->baseFont);
#if DEBUG
    fprintf(stderr, "Enter ScaleProc label=%s xOrigin=%g, yOrigin=%g xScale=%g yScale=%g new=%g,%g, newFontSize=%g\n", 
            labelPtr->text, xOrigin, yOrigin, xScale, yScale, labelPtr->xScale,
            labelPtr->yScale, newFontSize);
#endif
    labelPtr->flags |= DISPLAY_TEXT;
    if ((labelPtr->minFontSize > 0) && (newFontSize < labelPtr->minFontSize)) {
        labelPtr->flags &= ~DISPLAY_TEXT;
    } else if ((labelPtr->maxFontSize > 0) &&
               (newFontSize <= labelPtr->maxFontSize)) {
    } else {
        Blt_Font font;
        
        font = Blt_Font_Duplicate(labelPtr->tkwin, labelPtr->baseFont,
                                  newFontSize);
        if (font == NULL) {
            fprintf(stderr, "can't resize font\n");
            labelPtr->flags &= ~DISPLAY_TEXT;
        }
        if (labelPtr->scaledFont != NULL) {
            Blt_Font_Free(labelPtr->scaledFont);
        }
        labelPtr->scaledFont = font;
        if (labelPtr->angle != 0.0) {
            if (!Blt_Font_CanRotate(font, labelPtr->angle)) {
                fprintf(stderr, "can't rotate font %s\n", 
                        Blt_Font_Name(font));
            }
        }
        labelPtr->fontSize = Blt_Font_Size(font);
    } 
    labelPtr->x = xOrigin + xScale * (labelPtr->x - xOrigin);
    labelPtr->y = yOrigin + yScale * (labelPtr->y - yOrigin);
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

#if DEBUG
    fprintf(stderr, "Enter TranslateProc label=%s dx=%g, dy=%g\n", 
            labelPtr->text, dx, dy);
#endif
    /* Translate anchor. */
    labelPtr->anchorPos.x += dx;
    labelPtr->anchorPos.y += dy;
    labelPtr->x += dx;
    labelPtr->y += dy;

    /* Translate bounding box. */
    labelPtr->header.x1 = ROUND(labelPtr->anchorPos.x);
    labelPtr->header.x2 = ROUND(labelPtr->anchorPos.x + labelPtr->rotWidth);
    labelPtr->header.y1 = ROUND(labelPtr->anchorPos.y);
    labelPtr->header.y2 = ROUND(labelPtr->anchorPos.y + labelPtr->rotHeight);
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
    short int x, y;
    StateAttributes *attrPtr;
#if DEBUG
    fprintf(stderr, "Enter DisplayProc rx=%d, ry=%d, rw=%d rh=%d\n",
            rx, ry, rw, rh);
#endif
    if (labelPtr->state == TK_STATE_HIDDEN) {
        fprintf(stderr, "item is hidden\n");
        return;                         /* Item is hidden. */
    }
    /* Convert anchor from world coordinates to screen. */
    Tk_CanvasDrawableCoords(canvas, labelPtr->anchorPos.x, 
                            labelPtr->anchorPos.y, &x, &y);
    MapLabel(labelPtr, x, y);
#ifdef notdef
    if (!LabelInsideRegion(labelPtr, rx, ry, rw, rh)) {
        return;
    }
#endif
    switch (labelPtr->state) {
    case TK_STATE_DISABLED:
        attrPtr = &labelPtr->disabled;
        break;
    case TK_STATE_ACTIVE:
        attrPtr = &labelPtr->active;
        break;
    case TK_STATE_NORMAL:
        attrPtr = &labelPtr->normal;
        break;
    case TK_STATE_HIDDEN:
    default:
        attrPtr = NULL;
        break;
    }
    assert(attrPtr != NULL);
    tkwin = Tk_CanvasTkwin(canvas);
    if (attrPtr->brush != NULL) {       /* Fill rectangle or isolateral. */
        FillBackground(tkwin, drawable, labelPtr, x, y, attrPtr->brush,
                       attrPtr->bgColor);
    }
    if (attrPtr->lineWidth > 0) {      /* Outline */
        XDrawLines(display, drawable, attrPtr->labelGC->gc, labelPtr->points, 5,
            CoordModeOrigin);
    }
    if (labelPtr->text != NULL) {       /* Text itself */
        Blt_Font font;
        int maxLength;
        TkRegion clipRegion;

        maxLength = (labelPtr->flags & ELLIPSIS) ? labelPtr->width : -1;
        font = (labelPtr->scaledFont) ?
            labelPtr->scaledFont : labelPtr->baseFont;
        clipRegion = GetClipRegion(labelPtr, x, y);
        if (clipRegion != None) {
            fprintf(stderr, "setting clipRegion to font\n");
            Blt_Font_SetClipRegion(font, clipRegion);
        }
        XSetFont(display, attrPtr->labelGC->gc, Blt_Font_Id(font));
        Blt_DrawLayout(tkwin, drawable, attrPtr->labelGC->gc, font,
                Tk_Depth(tkwin), labelPtr->angle, x, y, labelPtr->layoutPtr,
                maxLength);
        if (clipRegion != None) {
            Blt_Font_SetClipRegion(font, None);
            TkDestroyRegion(clipRegion);
        }
    }
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

#if DEBUG
    fprintf(stderr, "Enter PostScriptProc label=%s prepass=%d\n",  
            labelPtr->text, prepass);
#endif
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

    Blt_Ps_Append(ps, "BeginEPSF\n");

    xScale = w / (double)(labelPtr->header.x2 - labelPtr->header.x1);
    yScale = h / (double)(labelPtr->header.y2 - labelPtr->header.y1);

    /* Set up scaling and translation transformations for the EPS item */

    Blt_Ps_Format(ps, "%g %g translate\n", x, y);
    Blt_Ps_Format(ps, "%g %g scale\n", xScale, yScale);
    Blt_Ps_Format(ps, "%d %d translate\n", -(labelPtr->header.x1),
                  -(labelPtr->header.y1));

    /* FIXME: Why clip against the old bounding box? */
    Blt_Ps_Format(ps, "%d %d %d %d SetClipRegion\n", labelPtr->header.x1, 
        labelPtr->header.y1, labelPtr->header.x2, labelPtr->header.y2);


    Blt_Ps_Append(ps, "EndEPSF\n");
    Blt_Ps_SetInterp(ps, interp);
    Blt_Ps_Free(ps);
    return TCL_OK;
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

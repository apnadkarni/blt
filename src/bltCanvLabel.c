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
#define DEF_JUSTIFY                     "center"
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
#define DEF_WIDTH                       "0"

/*
 * The structure below defines the record for each label item.
 */
typedef struct {
    Tk_Item item;                       /* Generic stuff that's the same for
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

    Blt_Painter painter;
    Blt_Picture original;               /* The original photo or PostScript
                                         * preview image converted to a
                                         * picture. */
    int origFromPicture;
    Blt_Picture picture;                /* Holds resized preview image.
                                         * Created and deleted internally. */
    int firstLine, lastLine;            /* First and last line numbers of the
                                         * PostScript preview.  They are used
                                         * to skip over the preview when
                                         * encapsulating PostScript for the
                                         * canvas item. */
    GC fillGC;                          /* Graphics context to fill background
                                         * of image outline if no preview
                                         * image was present. */
    int llx, lly, urx, ury;             /* Lower left and upper right
                                         * coordinates of PostScript bounding
                                         * box, retrieved from file's
                                         * "BoundingBox:" field. */
    const char *title;                  /* Title, retrieved from the file's
                                         * "Title:" field, to be displayed
                                         * over the top of the EPS preview
                                         * (malloc-ed).  */
    Tcl_DString ds;                     /* Contains the encapsulated
                                         * PostScript. */

    /* User configurable fields */

    double x, y;                        /* Requested anchor in canvas
                                         * coordinates of the label item */
    Tk_Anchor anchor;

    Region2d bbox;

    int width, height;                  /* Requested dimension of EPS item in
                                         * canvas coordinates.  If non-zero,
                                         * this overrides the dimension
                                         * computed from the "%%BoundingBox:"
                                         * specification in the EPS file
                                         * used. */
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
    int justify;                        /* Justification of multi-line
                                         * text. */
    TextStyle titleStyle;               /* Font, color, etc. for title */
    Blt_Font baseFont;                  /* Base font for item.  This is the
                                         * unscale font. */
    Blt_Font scaledFont;
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
        DEF_HEIGHT, Blt_Offset(LabelItem, height),
        TK_CONFIG_DONT_SET_DEFAULT, &bltDistanceOption},
    {TK_CONFIG_JUSTIFY, (char *)"-justify", "justify", "Justify",
        DEF_JUSTIFY, Blt_Offset(LabelItem, justify),
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
        DEF_WIDTH, Blt_Offset(LabelItem, width),
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
    Tk_Item *itemPtr,               /* Item that is being deleted. */
    Display *display)                   /* Display containing window for
                                         * canvas. */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;

    Tk_FreeOptions(configSpecs, (char *)labelPtr, display, 0);
    if (labelPtr->scaledFont != NULL) {
        Blt_Font_Free(labelPtr->scaledFont);
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
 *---------------------------------------------------------------------------
 */
static int
CreateProc(
    Tcl_Interp *interp,                 /* Interpreter for error reporting. */
    Tk_Canvas canvas,                   /* Canvas to hold new item. */
    Tk_Item *itemPtr,               /* Record to hold new item; header has
                                         * been initialized by caller. */
    int argc,                           /* Number of arguments in argv. */
    char **argv)                        /* Arguments describing rectangle. */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;
    Tk_Window tkwin;
    double x, y;

    tkwin = Tk_CanvasTkwin(canvas);
    if (argc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
            Tk_PathName(tkwin), " create ", itemPtr->typePtr->name,
            " x1 y1 ?options?\"", (char *)NULL);
        return TCL_ERROR;
    }
    /* Initialize the label item structure. */
    memset((char *)labelPtr + sizeof(Tk_Item), 0, 
           sizeof(LabelItem) - sizeof(Tk_Item));
    labelPtr->anchor = TK_ANCHOR_NW;
    labelPtr->canvas = canvas;
    labelPtr->interp = interp;
    Tcl_DStringInit(&labelPtr->ds);
    Blt_Ts_InitStyle(labelPtr->titleStyle);
#define PAD     8
    Blt_Ts_SetPadding(labelPtr->titleStyle, PAD, PAD, PAD, PAD);

    /* Process the arguments to fill in the item record. */
    if ((Tk_CanvasGetCoord(interp, canvas, argv[0], &x) != TCL_OK) ||
        (Tk_CanvasGetCoord(interp, canvas, argv[1], &y) != TCL_OK)) {
        DeleteProc(canvas, itemPtr, Tk_Display(tkwin));
        return TCL_ERROR;
    }
    labelPtr->x = x;
    labelPtr->y = y;
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
    Tk_Item *itemPtr,               /* Label item to reconfigure. */
    int argc,                           /* # of elements in argv.  */
    char **argv,                        /* Arguments describing things to
                                         * configure. */
    int flags)                          /* Flags to pass to
                                         * Tk_ConfigureWidget. */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;
    Tk_Window tkwin;
    XGCValues gcValues;
    unsigned long gcMask;
    GC newGC;
    int width, height;
    Blt_Painter painter;

    tkwin = Tk_CanvasTkwin(canvas);
    if (Tk_ConfigureWidget(interp, tkwin, configSpecs, argc, (const char**)argv,
                (char *)labelPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    painter = Blt_GetPainter(tkwin, 1.0);
    if (labelPtr->painter != NULL) {
        Blt_FreePainter(labelPtr->painter);
    }
    labelPtr->painter = painter;

    /* Determine the size of the label item */
    /*
     * Check for a "-image" option specifying an image to be displayed
     * representing the label canvas item.
     */
    if (Blt_OldConfigModified(configSpecs, "-image", (char *)NULL)) {
        if (labelPtr->preview != NULL) {
            Tk_FreeImage(labelPtr->preview);     /* Release old Tk image */
            if ((!labelPtr->origFromPicture) && (labelPtr->original != NULL)) {
                Blt_FreePicture(labelPtr->original);
            }
            labelPtr->original = NULL;
            if (labelPtr->picture != NULL) {
                Blt_FreePicture(labelPtr->picture);
            }
            labelPtr->picture = NULL;
            labelPtr->preview = NULL;
            labelPtr->origFromPicture = FALSE;
        }
        if (labelPtr->previewImageName != NULL) {
            int result;

            /* Allocate a new image, if one was named. */
            labelPtr->preview = Tk_GetImage(interp, tkwin, 
                        labelPtr->previewImageName, ImageChangedProc, labelPtr);
            if (labelPtr->preview == NULL) {
                Tcl_AppendResult(interp, "can't find an image \"",
                    labelPtr->previewImageName, "\"", (char *)NULL);
                Blt_Free(labelPtr->previewImageName);
                labelPtr->previewImageName = NULL;
                return TCL_ERROR;
            }
            result = Blt_GetPicture(interp, labelPtr->previewImageName, 
                                    &labelPtr->original);
            if (result == TCL_OK) {
                labelPtr->origFromPicture = TRUE;
            } else {
                Tk_PhotoHandle photo;   /* Photo handle to Tk image. */

                photo = Tk_FindPhoto(interp, labelPtr->previewImageName);
                if (photo == NULL) {
                    Tcl_AppendResult(interp, "image \"", 
                        labelPtr->previewImageName,
                        "\" is not a picture or photo image", (char *)NULL);
                    return TCL_ERROR;
                }
                labelPtr->original = Blt_PhotoToPicture(photo);
                labelPtr->origFromPicture = FALSE;
            }
        }
    }
    if (Blt_OldConfigModified(configSpecs, "-file", (char *)NULL)) {
        CloseEpsFile(labelPtr);
        if ((!labelPtr->origFromPicture) && (labelPtr->original != NULL)) {
            Blt_FreePicture(labelPtr->original);
            labelPtr->original = NULL;
        }
        if (labelPtr->picture != NULL) {
            Blt_FreePicture(labelPtr->picture);
            labelPtr->picture = NULL;
        }
        labelPtr->firstLine = labelPtr->lastLine = -1;
        if (labelPtr->fileName != NULL) {
            if (OpenEpsFile(interp, labelPtr) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    /* Compute the normal width and height of the item, but let the
     * user-requested dimensions override them. */
    width = height = 0;
    if (labelPtr->preview != NULL) {
        /* Default dimension is the size of the image. */
        Tk_SizeOfImage(labelPtr->preview, &width, &height);
    }
    if (labelPtr->fileName != NULL) {
        /* Use dimensions provided by the BoundingBox. */
        width = (labelPtr->urx - labelPtr->llx); 
        height = (labelPtr->ury - labelPtr->lly); 
    }
    if (labelPtr->width == 0) {
        labelPtr->width = width;
    }
    if (labelPtr->height == 0) {
        labelPtr->height = height;
    }

    if (Blt_OldConfigModified(configSpecs, "-quick", (char *)NULL)) {
        labelPtr->lastWidth = labelPtr->lastHeight = 0;
    }
    /* Fill color GC */

    newGC = NULL;
    if (labelPtr->fillColor != NULL) {
        gcMask = GCForeground;
        gcValues.foreground = labelPtr->fillColor->pixel;
        if (labelPtr->stipple != None) {
            gcMask |= (GCStipple | GCFillStyle);
            gcValues.stipple = labelPtr->stipple;
            if (labelPtr->border != NULL) {
                gcValues.foreground = Tk_3DBorderColor(labelPtr->border)->pixel;
                gcValues.background = labelPtr->fillColor->pixel;
                gcMask |= GCBackground;
                gcValues.fill_style = FillOpaqueStippled;
            } else {
                gcValues.fill_style = FillStippled;
            }
        }
        newGC = Tk_GetGC(tkwin, gcMask, &gcValues);
    }
    if (labelPtr->fillGC != NULL) {
        Tk_FreeGC(Tk_Display(tkwin), labelPtr->fillGC);
    }
    labelPtr->fillGC = newGC;
    CloseEpsFile(labelPtr);
    ComputeBbox(canvas, labelPtr);
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
    Tk_Item *itemPtr,               /* Item whose coordinates are to be
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
        ComputeBbox(canvas, labelPtr);
        return TCL_OK;
    }
    Tcl_AppendElement(interp, Blt_Dtoa(interp, labelPtr->x));
    Tcl_AppendElement(interp, Blt_Dtoa(interp, labelPtr->y));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *---------------------------------------------------------------------------
 */
 /* ARGSUSED */
static void
ComputeGeometry(LabelItem *labelPtr)
{
    labelPtr->width = labelPtr->height = 0;
    Graph *graphPtr = markerPtr->obj.graphPtr;
    TextMarker *tmPtr = (TextMarker *)markerPtr;
    Region2d extents;
    Point2d anchorPt;
    int i;
    unsigned int w, h;
    double rw, rh;

    tmPtr->width = tmPtr->height = 0;
    if (tmPtr->string == NULL) {
        return;
    }
    Blt_Ts_GetExtents(&labelPtr->style, labelPtr->text, &w, &h);
    labelPtr->textWidth = w;
    labelPtr->textHeight = h;
    Blt_GetBoundingBox(w, h, labelPtr->style.angle, &rw, &rh,
		       labelPtr->outlinePts);
    labelPtr->width = ROUND(rw);
    labelPtr->height = ROUND(rh);
    for (i = 0; i < 4; i++) {
        labelPtr->outlinePts[i].x += ROUND(rw * 0.5);
        labelPtr->outlinePts[i].y += ROUND(rh * 0.5);
    }
    labelPtr->outlinePts[4].x = labelPtr->outlinePts[0].x;
    labelPtr->outlinePts[4].y = labelPtr->outlinePts[0].y;
    anchorPos = MapPoint(markerPtr->worldPts, &markerPtr->axes);
    anchorPos = Blt_AnchorPoint(anchorPos.x, anchorPos.y,
	(double)(labelPtr->width), (double)(labelPtr->height), labelPtr->anchor);
    /*
     * Determine the bounding box of the text and test to see if it is at
     * least partially contained within the plotting area.
     */
    extents.x1 = anchorPos.x;
    extents.y1 = anchorPos.y;
    extents.x2 = anchorPos.x + tmPtr->width - 1;
    extents.y2 = anchorPos.y + tmPtr->height - 1;
    markerPtr->offScreen = BoxesDontOverlap(graphPtr, &extents);
    labelPtr->anchorPos = anchorPos;
    
}


/*
 *---------------------------------------------------------------------------
 *
 * ComputeBbox --
 *
 *      This procedure is invoked to compute the bounding box of all the
 *      pixels that may be drawn as part of a label item.  This procedure
 *      is where the preview image's placement is computed.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The fields x1, y1, x2, and y2 are updated in the item for labelPtr.
 *
 *---------------------------------------------------------------------------
 */
 /* ARGSUSED */
static void
ComputeBbox(Tk_Canvas canvas, LabelItem *labelPtr)
{
    Point2d anchorPos;

    /* Translate the coordinates wrt the anchor. */
    anchorPos = Blt_AnchorPoint(labelPtr->x, labelPtr->y, (double)labelPtr->width, 
        (double)labelPtr->height, labelPtr->anchor);

    /*
     * Note: The x2 and y2 coordinates are exterior the label item.
     */
    labelPtr->bbox.x1 = anchorPos.x;
    labelPtr->bbox.y1 = anchorPos.y;
    labelPtr->bbox.x2 = labelPtr->bbox.x1 + labelPtr->width;
    labelPtr->bbox.y2 = labelPtr->bbox.y1  + labelPtr->height;
    /* Computes the bounding box of the label. This is the rectangular
     * region the enclosed the possibly rotated text. */
    /* Min x,y one corner, max x,y the other. */
    labelPtr->item.x1 = ROUND(labelPtr->bbox.x1);
    labelPtr->item.y1 = ROUND(labelPtr->bbox.y1);
    labelPtr->item.x2 = ROUND(labelPtr->bbox.x2);
    labelPtr->item.y2 = ROUND(labelPtr->bbox.y2);
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
                                         * must be redisplayed (not
                                         * used). */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;
    Tk_Window tkwin;
    const char *title;
    int w, h;
    short int dx, dy;
    Region2d region;

    if (labelPtr->state == TK_STATE_HIDDEN) {
        return;                         /* Item is hidden. */
    }

    /* Test to see if the rotated bounding box of the label is inside the
     * region. */
    region.x1 = rx;
    region.y1 = ry;
    region.x2 = rx + rw;
    region.y2 = ry + rh;
    if (!Blt_RegionInPolygon(&region, labelPtr->outlinePts, 5, TRUE)) {
        return;                         /* Outside region. */
    }
    w = (int)(labelPtr->bbox.x2 - labelPtr->bbox.x1);
    h = (int)(labelPtr->bbox.y2 - labelPtr->bbox.y1);
    if ((w < 1) || (h < 1)) {
        return;
    }
    tkwin = Tk_CanvasTkwin(canvas);

    /*
     * Translate the coordinates to the label item, then redisplay it.
     */
    Tk_CanvasDrawableCoords(canvas, labelPtr->bbox.x1, labelPtr->bbox.y1, 
        &dx, &dy);

    if (labelPtr->text != NULL) {
        TextLayout *textPtr;
        double rw, rh;
        int dw, dh;

        /* Translate the anchor position within the label item */
        labelPtr->titleStyle.font = labelPtr->font;
        textPtr = Blt_Ts_CreateLayout(title, -1, &labelPtr->titleStyle);
        Blt_GetBoundingBox(textPtr->width, textPtr->height, 
             labelPtr->titleStyle.angle, &rw, &rh, (Point2d *)NULL);
        dw = (int)ceil(rw);
        dh = (int)ceil(rh);
        if ((dw <= w) && (dh <= h)) {
            int tx, ty;

            Blt_TranslateAnchor(dx, dy, w, h, labelPtr->titleStyle.anchor, 
                &tx, &ty);
            if (picture == NULL) {
                tx += labelPtr->borderWidth;
                ty += labelPtr->borderWidth;
            }
            Blt_Ts_DrawLayout(tkwin, drawable, textPtr, &labelPtr->titleStyle, 
                tx, ty);
        }
        Blt_Free(textPtr);
    }
    if ((picture == NULL) && (labelPtr->border != NULL) && 
        (labelPtr->borderWidth > 0)) {
        Blt_Draw3DRectangle(tkwin, drawable, labelPtr->border, dx, dy,
            labelPtr->width, labelPtr->height, labelPtr->borderWidth, 
                            labelPtr->relief);
    }
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
    Tk_Item *itemPtr,               /* Label item to check. */
    double *pts)                        /* Array of x and y coordinates
                                         * representing the sample
                                         * point. */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;
    double minDist;
    Point2d points[5];
    Point2d sample, p, q;
    int i;

    sample.x = pts[0];
    sample.y = pts[1];

    if ((labelPtr->state == TK_STATE_DISABLED) ||
        (labelPtr->state == TK_STATE_HIDDEN)) {
        return FLT_MAX;
    }
    /*  
     * Generate the bounding polygon (rectangle or isolateral) representing
     * the label's possibly rotated bounding box and see if the point is
     * inside of it.
     */
    for (i = 0; i < 4; i++) {
        points[i].x = labelPtr->outlinePts[i].x + labelPtr->anchorPos.x;
        points[i].y = labelPtr->outlinePts[i].y + labelPtr->anchorPos.y;
    }
    if (Blt_PointInPolygon(&sample, points, 4)) {
        return 0.0;
    }
    /* Otherwise return the distance to the nearest segment of the
     * polygon. */
    p = points[0];
    q = points[1];
    minDist = 10000;
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
        dist = hypot(p.x - samplePtr->x, p.y - samplePtr->y);
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
    Tk_Item *itemPtr,               /* Label item to check. */
    double pts[])                       /* Array of four coordinates (x1,
                                         * y1, x2, y2) describing the
                                         * region to test.  */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;

    if ((labelPtr->state == TK_STATE_DISABLED) ||
        (labelPtr->state == TK_STATE_HIDDEN)) {
        return -1;
    }
    if (labelPtr->style.angle != 0.0f) {
        Point2d points[5];
        Region2d region;
        int i;

        region.x1 = pts[0];
        region.y1 = pts[1];
        region.x2 = pts[2];
        region.y2 = pts[3];

        /*  
         * Generate the bounding polygon (rectangle or isolateral)
         * representing the label's possibly rotated bounding box and see
         * if the region is inside of it.
         */
        for (i = 0; i < 5; i++) {
            points[i].x = labelPtr->outlinePts[i].x + labelPtr->anchorPos.x;
            points[i].y = labelPtr->outlinePts[i].y + labelPtr->anchorPos.y;
        }
        if (!Blt_RegionInPolygon(&region, points, 5, TRUE /*overlapping*/)) {
            return -1;                   /* Outside. */
        }
        /* Test again if it's enclosed. */
        return Blt_RegionInPolygon(&region, points, 5, FALSE /*enclosed*/);
    } 
    /* Simpler test of unrotated label. */
    /* Min-max test of two rectangles. */
    if ((pts[2] <= labelPtr->bbox.x1) || (pts[0] >= labelPtr->bbox.x2) ||
        (pts[3] <= labelPtr->bbox.y1) || (pts[1] >= labelPtr->bbox.y2)) {
        return -1;                      /* Completely outside. */
    }
    if ((pts[0] <= labelPtr->bbox.x1) && (pts[1] <= labelPtr->bbox.y1) &&
        (pts[2] >= labelPtr->bbox.x2) && (pts[3] >= labelPtr->bbox.y2)) {
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
    Tk_Item *itemPtr,		/* Label item to be scaled. */
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
    
    /* Use the smaller scale */
    labelPtr->bbox.x1 = x0 + xs * (labelPtr->bbox.x1 - x0);
    labelPtr->bbox.x2 = x0 + xs * (labelPtr->bbox.x2 - x0);
    labelPtr->bbox.y1 = y0 + ys * (labelPtr->bbox.y1 - y0);
    labelPtr->bbox.y2 = y0 + ys * (labelPtr->bbox.y2 - y0);

    /* Reset the user-requested values to the newly scaled values. */
    labelPtr->width  = ROUND(labelPtr->bbox.x2 - labelPtr->bbox.x1);
    labelPtr->height = ROUND(labelPtr->bbox.y2 - labelPtr->bbox.y1);
    labelPtr->x = ROUND(labelPtr->bbox.x1);
    labelPtr->y = ROUND(labelPtr->bbox.y1);

    labelPtr->item.x1 = ROUND(labelPtr->bbox.x1);
    labelPtr->item.y1 = ROUND(labelPtr->bbox.y1);
    labelPtr->item.x2 = ROUND(labelPtr->bbox.x2);
    labelPtr->item.y2 = ROUND(labelPtr->bbox.y2);
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
    Tk_Item *itemPtr,		/* Item that is being moved. */
    double dx, double dy)               /* Amount by which item is to be
                                         * moved. */
{
    LabelItem *labelPtr = (LabelItem *)itemPtr;

    labelPtr->bbox.x1 += dx;
    labelPtr->bbox.x2 += dx;
    labelPtr->bbox.y1 += dy;
    labelPtr->bbox.y2 += dy;

    labelPtr->x = labelPtr->bbox.x1;
    labelPtr->y = labelPtr->bbox.y1;

    labelPtr->item.x1 = ROUND(labelPtr->bbox.x1);
    labelPtr->item.x2 = ROUND(labelPtr->bbox.x2);
    labelPtr->item.y1 = ROUND(labelPtr->bbox.y1);
    labelPtr->item.y2 = ROUND(labelPtr->bbox.y2);
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
    x = labelPtr->bbox.x1;
    y = Tk_CanvasPsY(canvas, labelPtr->bbox.y2);
    w = labelPtr->bbox.x2 - labelPtr->bbox.x1;
    h = labelPtr->bbox.y2 - labelPtr->bbox.y1;

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

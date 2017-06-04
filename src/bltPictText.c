/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictText.c --
 *
 * This module implements text drawing for picture images in the BLT
 * toolkit.
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

#include "bltInt.h"
#include <string.h>

#ifdef HAVE_LIBXFT
  #include <ft2build.h>
  #include FT_FREETYPE_H
  #ifndef TT_CONFIG_OPTION_SUBPIXEL_HINTING
    #define TT_CONFIG_OPTION_SUBPIXEL_HINTING 0
  #endif 
  #include <X11/Xft/Xft.h>
#endif  /* HAVE_LIBXFT */

#include <X11/Xutil.h>
#include "bltMath.h"
#include "bltPicture.h"
#include "bltPictInt.h"
#include "bltBg.h"
#include "bltPainter.h"
#include "bltFont.h"
#include "bltText.h"
#include "bltOp.h"
#include "tkIntBorder.h"

#define imul8x8(a,b,t)  ((t) = (a)*(b)+128,(((t)+((t)>>8))>>8))
#define CLAMP(c)        ((((c) < 0.0) ? 0.0 : ((c) > 255.0) ? 255.0 : (c)))

#define DEBUG 0

#if defined (HAVE_FT2BUILD_H)
#  define HAVE_FT2
#  include <ft2build.h>
#  include FT_FREETYPE_H
#  ifndef TT_CONFIG_OPTION_SUBPIXEL_HINTING
#    define TT_CONFIG_OPTION_SUBPIXEL_HINTING 0
#  endif 
#endif
#ifndef WIN32 
#  if defined (HAVE_LIBXFT) && defined(HAVE_X11_XFT_XFT_H)
#    define HAVE_XFT
#    include <X11/Xft/Xft.h>
#  endif        /* HAVE_XFT */
#endif /*WIN32*/

#ifndef HAVE_FT2
#  define DRAWTEXT 0
#  error("No Freetype")
#else 
#  ifdef WIN32
#    define DRAWTEXT 1
#  else 
#    ifdef HAVE_XFT
#      define DRAWTEXT 1
#    else
#      error("!WIN32 &&  !XFT")
#      define DRAWTEXT 0
#    endif
#  endif
#endif

static FT_Library ftLibrary;

/* 
 * FtFont --
 *
 *      This structure contains the various pieces to draw text using the
 *      freetype library into a picture image.
 */
typedef struct {
    FT_Face face;                       /* Type face of font */
    FT_Matrix matrix;                   /* Rotation matrix for font. Used
                                        * for rotated fonts. */
#ifdef HAVE_XFT
    XftFont *xftFont;                   /* Xft font handle used to get face
                                         * from Xft font. */
#endif
    int fontSize;                       /* Point size of font.  */
    float angle;                        /* Rotation of font. */
    int height;                         /* Line height of font.  */
    int ascent, descent;                /* Ascent and descent of font. */
} FtFont;    

typedef struct {
    size_t numValues;
    void *values;
} Array;

static Blt_SwitchParseProc AnchorSwitchProc;
static Blt_SwitchCustom anchorSwitch = {
    AnchorSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ShadowSwitchProc;
static Blt_SwitchCustom shadowSwitch = {
    ShadowSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc JustifySwitchProc;
static Blt_SwitchCustom justifySwitch = {
    JustifySwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc PaintBrushSwitchProc;
static Blt_SwitchFreeProc PaintBrushFreeProc;
static Blt_SwitchCustom paintbrushSwitch =
{
    PaintBrushSwitchProc, NULL, PaintBrushFreeProc, (ClientData)0,
};

typedef struct {
    int kerning;                        /* Indicates whether to kern
                                           text. */
    Blt_PaintBrush brush;               /* Color of text. */
    Blt_Shadow shadow;                  /*  */
    int fontSize;                       /* Size of requested font. */
    Tcl_Obj *fontObjPtr;                /* Requested font.  If the name of
                                         * the font starts with a '@' sign,
                                         * this is the path to a font file.
                                         * Otherwise this is the name of
                                         * font. We will use the font
                                         * handling routines to get a
                                         * corresponding face. */
    int justify;
    Tk_Anchor anchor;
    float angle;
} TextSwitches;

static Blt_SwitchSpec textSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,  "-anchor",   "anchor", (char *)NULL,
        Blt_Offset(TextSwitches, anchor), 0, 0, &anchorSwitch},
    {BLT_SWITCH_CUSTOM,  "-color",    "colorName", (char *)NULL,
        Blt_Offset(TextSwitches, brush),  0, 0, &paintbrushSwitch},
    {BLT_SWITCH_OBJ,     "-font",     "fontName", (char *)NULL,
        Blt_Offset(TextSwitches, fontObjPtr), 0},
    {BLT_SWITCH_CUSTOM,  "-justify", "left|right|center", (char *)NULL,
        Blt_Offset(TextSwitches, justify), 0, 0, &justifySwitch},
    {BLT_SWITCH_BOOLEAN, "-kerning",  "bool", (char *)NULL,
        Blt_Offset(TextSwitches, kerning),  0},
    {BLT_SWITCH_FLOAT,   "-rotate",   "angle", (char *)NULL,
        Blt_Offset(TextSwitches, angle), 0},
    {BLT_SWITCH_INT,     "-size",     "number", (char *)NULL,
        Blt_Offset(TextSwitches, fontSize),  0}, 
    {BLT_SWITCH_CUSTOM, "-shadow", "offset", (char *)NULL,
        Blt_Offset(TextSwitches, shadow), 0, 0, &shadowSwitch},
    {BLT_SWITCH_END}
};

DLLEXPORT extern Tcl_AppInitProc Blt_PictureTextInit, Blt_PictureTextSafeInit;
static Tcl_AppInitProc InitTextOp;

/*ARGSUSED*/
static void
PaintBrushFreeProc(ClientData clientData, char *record, int offset, int flags)
{
    Blt_PaintBrush *brushPtr = (Blt_PaintBrush *)(record + offset);

    if (*brushPtr != NULL) {
        Blt_FreeBrush(*brushPtr);
        *brushPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PaintBrushSwitchProc --
 *
 *      Convert a Tcl_Obj representing a paint brush.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PaintBrushSwitchProc(ClientData clientData, Tcl_Interp *interp,
                     const char *switchName, Tcl_Obj *objPtr, char *record,
                     int offset, int flags)      
{
    Blt_PaintBrush *brushPtr = (Blt_PaintBrush *)(record + offset);
    Blt_PaintBrush brush;

    if (Blt_GetPaintBrushFromObj(interp, objPtr, &brush) != TCL_OK) {
        return TCL_ERROR;
    }
    if (*brushPtr != NULL) {
        Blt_FreeBrush(*brushPtr);
    }
    *brushPtr = brush;
    return TCL_OK;
}

static size_t GetTextWidth(FtFont *fontPtr, const char *string, size_t length,
                           int kerning);

/*
 *---------------------------------------------------------------------------
 *
 * AnchorSwitchProc --
 *
 *      Convert a Tcl_Obj representing an anchor.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AnchorSwitchProc(ClientData clientData, Tcl_Interp *interp,
                 const char *switchName, Tcl_Obj *objPtr, char *record,
                 int offset, int flags)      
{
    Tk_Anchor *anchorPtr = (Tk_Anchor *)(record + offset);

    if (Tk_GetAnchorFromObj(interp, objPtr, anchorPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * JustifySwitchProc --
 *
 *      Convert a Tcl_Obj representing an anchor.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
JustifySwitchProc(ClientData clientData, Tcl_Interp *interp,
                  const char *switchName, Tcl_Obj *objPtr, char *record,
                  int offset, int flags)      
{
    Tk_Justify *justifyPtr = (Tk_Justify *)(record + offset);

    if (Tk_GetJustifyFromObj(interp, objPtr, justifyPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ShadowSwitchProc --
 *
 *      Convert a Tcl_Obj representing a number for the alpha value.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ShadowSwitchProc(ClientData clientData, Tcl_Interp *interp,
                 const char *switchName, Tcl_Obj *objPtr, char *record,
                 int offset, int flags)      
{
    Blt_Shadow *shadowPtr = (Blt_Shadow *)(record + offset);
    int objc;
    Tcl_Obj **objv;
    int i, width;

    shadowPtr->offset = 2;
    shadowPtr->width = 2;
    shadowPtr->color.u32 = 0xA0000000;
    if (Tcl_GetIntFromObj(NULL, objPtr, &width) == TCL_OK) {
        shadowPtr->width = shadowPtr->offset = width;
        return TCL_OK;
    }
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i += 2) {
        const char *string;
        int length;
        char c;

        string = Tcl_GetStringFromObj(objv[i], &length);
        if (string[0] != '-') {
            Tcl_AppendResult(interp, "bad shadow option \"", string, 
                "\": should be -offset, -width, or -color", (char *)NULL);
            return TCL_ERROR;
        }
        c = string[1];
        if ((c == 'w') && (strncmp(string, "-width", length) == 0)) {
            if (Tcl_GetIntFromObj(interp, objv[i+1], &shadowPtr->width) 
                != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'o') && (strncmp(string, "-offset", length) == 0)) {
            if (Tcl_GetIntFromObj(interp, objv[i+1], &shadowPtr->offset) 
                != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'c') && (strncmp(string, "-color", length) == 0)) {
            if (Blt_GetPixelFromObj(interp, objv[i+1], &shadowPtr->color) 
                != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'a') && (strncmp(string, "-alpha", length) == 0)) {
            int alpha;

            if (Tcl_GetIntFromObj(interp, objv[i+1], &alpha) != TCL_OK) {
                return TCL_ERROR;
            }
            shadowPtr->color.Alpha = alpha;
        } else {
            Tcl_AppendResult(interp, "unknown shadow option \"", string, 
                "\": should be -offset, -width, or -color", (char *)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

#if DEBUG
static void
DebugFace(FT_Face face)
{
    fprintf(stderr, "num_faces=%ld\n", face->num_faces);
    fprintf(stderr, "face_flags=%lx\n", face->face_flags);
    fprintf(stderr, "style_flags=%lx\n", face->style_flags);
    fprintf(stderr, "num_glyphs=%ld\n", face->num_glyphs);
    fprintf(stderr, "family_name=%s\n", face->family_name);
    fprintf(stderr, "style_name=%s\n", face->style_name);
    fprintf(stderr, "num_fixed_sizes=%d\n", face->num_fixed_sizes);
    fprintf(stderr, "num_charmaps=%d\n", face->num_charmaps);
    fprintf(stderr, "units_per_EM=%d\n", face->units_per_EM);
    fprintf(stderr, "face->size->metrics.height=%ld\n", face->size->metrics.height);
    fprintf(stderr, "face->size->metrics.ascender=%ld\n", face->size->metrics.ascender);
    fprintf(stderr, "face->size->metrics.descender=%ld\n", face->size->metrics.descender);
    fprintf(stderr, "ascender=%d\n", face->ascender);
    fprintf(stderr, "descender=%d\n", face->descender);
    fprintf(stderr, "height=%d\n", face->height);
    fprintf(stderr, "max_advance_width=%d\n", face->max_advance_width);
    fprintf(stderr, "max_advance_height=%d\n", face->max_advance_height);
    fprintf(stderr, "underline_position=%d\n", face->underline_position);
    fprintf(stderr, "underline_thickness=%d\n", (int)face->underline_thickness);
}

static void
DebugGlyph(FT_GlyphSlot slot)
{
    fprintf(stderr, "  slot.bitmap.width=%d\n", (int)slot->bitmap.width);
    fprintf(stderr, "  slot.bitmap.rows=%d\n", slot->bitmap.rows);
    fprintf(stderr, "  slot.bitmap_left=%d\n", (int)slot->bitmap_left);
    fprintf(stderr, "  slot.bitmap_top=%d\n", (int)slot->bitmap_top);
    fprintf(stderr, "  slot.bitmap.pixel_mode=%x\n", slot->bitmap.pixel_mode);
    fprintf(stderr, "  slot.advance.x=%d\n", (int)slot->advance.x);
    fprintf(stderr, "  slot.advance.y=%d\n", (int)slot->advance.y);
    
    fprintf(stderr, "  slot.format=%c%c%c%c\n", 
            (slot->format >> 24) & 0xFF, 
            (slot->format >> 16) & 0xFF, 
            (slot->format >> 8) & 0xFF, 
            (slot->format & 0xFF));
}
#endif  /* DEBUG */

/*
 *---------------------------------------------------------------------------
 *
 * CreateSimpleTextLayout --
 *
 *      Get the extents of a possibly multiple-lined text string.
 *
 * Results:
 *      Returns via *widthPtr* and *heightPtr* the dimensions of the text
 *      string.
 *
 *---------------------------------------------------------------------------
 */
static TextLayout *
CreateSimpleTextLayout(FtFont *fontPtr, const char *text, int textLen, 
                       TextStyle *tsPtr)
{
    TextFragment *fp;
    TextLayout *layoutPtr;
    size_t count;                       /* Count # of characters on each
                                         * line. */
    int lineHeight;
    int maxHeight, maxWidth;
    int numFrags;
    const char *p, *endp, *start;
    int i;
    size_t size;

    numFrags = 0;
    endp = text + ((textLen < 0) ? strlen(text) : textLen);
    for (p = text; p < endp; p++) {
        if (*p == '\n') {
            numFrags++;
        }
    }
    if ((p != text) && (*(p - 1) != '\n')) {
        numFrags++;
    }
    size = sizeof(TextLayout) + (sizeof(TextFragment) * (numFrags - 1));

    layoutPtr = Blt_AssertCalloc(1, size);
    layoutPtr->numFragments = numFrags;

    numFrags = count = 0;
    maxWidth = 0;
    maxHeight = tsPtr->padTop;
    lineHeight = fontPtr->height;

    fp = layoutPtr->fragments;
    for (p = start = text; p < endp; p++) {
        if (*p == '\n') {
            size_t w;

            if (count > 0) {
                w = GetTextWidth(fontPtr, start, count, 1);
                if (w > maxWidth) {
                    maxWidth = w;
                }
            } else {
                w = 0;
            }
            fp->w = w;
            fp->numBytes = count;
            fp->ry = fp->y = maxHeight + fontPtr->ascent;
            fp->text = start;
            maxHeight += lineHeight;
            fp++;
            numFrags++;
            start = p + 1;              /* Start the text on the next line */
            count = 0;                  /* Reset to indicate the start of a
                                         * new line */
            continue;
        }
        count++;
    }
    if (numFrags < layoutPtr->numFragments) {
        size_t w;

        w = GetTextWidth(fontPtr, start, count, 1);
        if (w > maxWidth) {
            maxWidth = w;
        }
        fp->w = w;
        fp->numBytes = count;
        fp->ry = fp->y = maxHeight + fontPtr->ascent;
        fp->text = start;
        maxHeight += lineHeight;
        numFrags++;
    }
    maxHeight += tsPtr->padBottom;
    maxWidth += PADDING(tsPtr->padX);
    fp = layoutPtr->fragments;
    for (i = 0; i < numFrags; i++, fp++) {
        switch (tsPtr->justify) {
        default:
        case TK_JUSTIFY_LEFT:
            /* No offset for left justified text strings */
            fp->rx = fp->x = tsPtr->padLeft;
            break;
        case TK_JUSTIFY_RIGHT:
            fp->rx = fp->x = (maxWidth - fp->w) - tsPtr->padRight;
            break;
        case TK_JUSTIFY_CENTER:
            fp->rx = fp->x = (maxWidth - fp->w) / 2;
            break;
        }
    }
    if (tsPtr->underline >= 0) {
        fp = layoutPtr->fragments;
        for (i = 0; i < numFrags; i++, fp++) {
            int first, last;

            first = fp->text - text;
            last = first + fp->numBytes;
            if ((tsPtr->underline >= first) && (tsPtr->underline < last)) {
                layoutPtr->underlinePtr = fp;
                layoutPtr->underline = tsPtr->underline - first;
                break;
            }
        }
    }
    layoutPtr->width = maxWidth;
    layoutPtr->height = maxHeight - tsPtr->leader;
    return layoutPtr;
}

#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { -1, 0 } };

static const char *
FtError(FT_Error ftError)
{
    struct ft_errors {                                          
        int          code;             
        const char*  msg;
    };
    static struct ft_errors ft_err_mesgs[] = 
#include FT_ERRORS_H            

    struct ft_errors *fp;
    for (fp = ft_err_mesgs; fp->msg != NULL; fp++) {
        if (fp->code == ftError) {
            return fp->msg;
        }
    }
    return "unknown Freetype error";
}

static size_t
GetTextWidth(FtFont *fontPtr, const char *string, size_t length, int kerning)
{
    FT_Vector pen;                      /* Untransformed origin  */
    FT_GlyphSlot  slot;
    FT_Matrix matrix;                   /* Transformation matrix. */
    int maxX;
    const char *p, *pend;
    double radians;
    FT_Face face;
    int previous;
    FT_Error ftError;

    radians = 0.0;
    matrix.yy = matrix.xx = (FT_Fixed)(cos(radians) * 65536.0);
    matrix.yx = (FT_Fixed)(sin(radians) * 65536.0);
    matrix.xy = -matrix.yx;

    face = fontPtr->face;
    slot = face->glyph;
    
    maxX = 0;
    pen.y = pen.x = 0;
#ifdef notdef
    fprintf(stderr, "face->height=%d, face->size->height=%d\n",
            face->height, (int)face->size->metrics.height);
#endif
    previous = -1;
    for (p = string, pend = p + length; p < pend; p++) {
        int current;

        current = FT_Get_Char_Index(face, *p);
        if ((kerning) && (previous >= 0)) { 
            FT_Vector delta; 
                
            FT_Get_Kerning(face, previous, current, FT_KERNING_DEFAULT, &delta);
            pen.x += delta.x; 
        } 
        FT_Set_Transform(face, &matrix, &pen);
        previous = current;
        /* load glyph image into the slot (erase previous one) */  
        ftError = FT_Load_Glyph(face, current, FT_LOAD_DEFAULT); 
        if (ftError) {
            Blt_Warn("can't load character \"%c\" (%d): %s\n", *p, current, 
                FtError(ftError));
            continue;                   /* Ignore errors. */
        }
        pen.x += slot->advance.x;
        pen.y += slot->advance.y;
        if (pen.x > maxX) {
            maxX = pen.x;
        }
    }   
    return maxX >> 6;
}

static INLINE void
BlendPixels(Blt_Pixel *bgPtr, Blt_Pixel *colorPtr)
{
    unsigned char beta;
    int t;

    /* Note: Color and background are always premultiplied. */
    beta = colorPtr->Alpha ^ 0xFF;
    bgPtr->Red   = colorPtr->Red   + imul8x8(beta, bgPtr->Red, t);
    bgPtr->Green = colorPtr->Green + imul8x8(beta, bgPtr->Green, t);
    bgPtr->Blue  = colorPtr->Blue  + imul8x8(beta, bgPtr->Blue, t);
    bgPtr->Alpha = colorPtr->Alpha + imul8x8(beta, bgPtr->Alpha, t);
}

#ifdef notdef
static void
BlitGlyph(Pict *destPtr, FT_GlyphSlot slot, int dx, int dy, int xx, int yy,
          Blt_PaintBrush brush)
{
    int x1, y1, x2, y2;
#if DEBUG
    fprintf(stderr, "dx=%d, dy=%d\n", dx, dy);
    DebugGlyph(slot);
#endif
    if ((dx >= destPtr->width) || ((dx + slot->bitmap.width) <= 0) ||
        (dy >= destPtr->height) || ((dy + slot->bitmap.rows) <= 0)) {
        return;                         /* No portion of the glyph is visible
                                         * in the picture. */
    }
    /* By default, set the region to cover the entire glyph */
    x1 = y1 = 0;
    x2 = slot->bitmap.width;
    y2 = slot->bitmap.rows;

    /* Determine the portion of the glyph inside the picture. */

    if (dx < 0) {                       /* Left side of glyph overhangs. */
        x1 -= dx;               
        x2 += dx;
        dx = 0;         
    }
    if (dy < 0) {                       /* Top of glyph overhangs. */
        y1 -= dy;
        y2 += dy;
        dy = 0;
    }
    if ((dx + x2) > destPtr->width) {   /* Right side of glyph overhangs. */
        x2 = destPtr->width - dx;
    }
    if ((dy + y2) > destPtr->height) {  /* Bottom of glyph overhangs. */
        y2 = destPtr->height - dy;
    }
    if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
        Blt_Pixel *destRowPtr;
        unsigned char *srcRowPtr;
        int y;

        srcRowPtr = slot->bitmap.buffer + (y1 * slot->bitmap.pitch);
        destRowPtr = Blt_Picture_Pixel(destPtr, xx, yy);
        for (y = y1; y < y2; y++) {
            Blt_Pixel *dp;
            int x;
            
            dp = destRowPtr;
            for (x = x1; x < x2; x++, dp++) {
                int pixel;

                pixel = srcRowPtr[x >> 3] & (1 << (7 - (x & 0x7)));
                if (pixel != 0x0) {
                    Blt_Pixel color;

                    color.u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
                    BlendPixels(dp, &color);
                }
            }
            srcRowPtr += slot->bitmap.pitch;
            destRowPtr += destPtr->pixelsPerRow;
        }
    } else {
        Blt_Pixel *destRowPtr;
        unsigned char *srcRowPtr;
        int y;

        srcRowPtr = slot->bitmap.buffer + ((y1 * slot->bitmap.pitch) + x1);
        destRowPtr = Blt_Picture_Pixel(destPtr, dx, dy);
        for (y = y1; y < y2; y++) {
            Blt_Pixel *dp;
            unsigned char *sp;
            int x;

            for (dp = destRowPtr, sp = srcRowPtr, x = x1; x < x2; 
                 x++, sp++, dp++) {
                if (*sp != 0x0) {
                    Blt_Pixel color;

                    color.u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
                    Blt_FadeColor(&color, *sp);
                    BlendPixels(dp, &color);
                }
            }
            srcRowPtr += slot->bitmap.pitch;
            destRowPtr += destPtr->pixelsPerRow;
        }
    }
}

static void
CopyGrayGlyph(Pict *destPtr, FT_GlyphSlot slot, int xx, int yy, 
              Blt_PaintBrush brush)
{
    int x1, y1, x2, y2;

#if DEBUG
    fprintf(stderr, "dx=%d, dy=%d\n", dx, dy);
    DebugGlyph(slot);
#endif
    if ((xx >= destPtr->width) || ((xx + (int)slot->bitmap.width) <= 0) ||
        (yy >= destPtr->height) || ((yy + (int)slot->bitmap.rows) <= 0)) {
        return;                 /* No portion of the glyph is visible in the
                                 * picture. */
    }

    /* By default, set the region to cover the entire glyph */
    x1 = y1 = 0;
    x2 = slot->bitmap.width;
    y2 = slot->bitmap.rows;

    /* Determine the portion of the glyph inside the picture. */

    if (xx < 0) {                       /* Left side of glyph overhangs. */
        x1 -= xx;               
        x2 += xx;
        xx = 0;         
    }
    if (yy < 0) {                       /* Top of glyph overhangs. */
        y1 -= yy;
        y2 += yy;
        yy = 0;
    }
    if ((xx + x2) > destPtr->width) {   /* Right side of glyph overhangs. */
        x2 = destPtr->width - xx;
    }
    if ((yy + y2) > destPtr->height) {  /* Bottom of glyph overhangs. */
        y2 = destPtr->height - yy;
    }

    {
        Blt_Pixel *destRowPtr;
        unsigned char *srcRowPtr;
        int y;

        srcRowPtr = slot->bitmap.buffer + ((y1 * slot->bitmap.pitch) + x1);
        destRowPtr = Blt_Picture_Pixel(destPtr, xx, yy);
        for (y = y1; y < y2; y++) {
            Blt_Pixel *dp;
            unsigned char *sp;
            int x;

            dp = destRowPtr;
            sp = srcRowPtr;
            for (x = x1; x < x2; x++, sp++, dp++) {
                if (*sp != 0x0) {
                    Blt_Pixel color;

                    color.u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
                    Blt_FadeColor(&color, *sp);
                    dp->u32 = color.u32;
                }
            }
            srcRowPtr += slot->bitmap.pitch;
            destRowPtr += destPtr->pixelsPerRow;
        }
    }
}
#endif

static void
PaintGrayGlyph(Pict *destPtr, FT_GlyphSlot slot, int xx, int yy, 
               Blt_PaintBrush brush)
{
    int x1, y1, x2, y2;

#if DEBUG
    fprintf(stderr, "dx=%d, dy=%d\n", dx, dy);
    DebugGlyph(slot);
#endif
    if ((xx >= destPtr->width) || ((xx + (int)slot->bitmap.width) <= 0) ||
        (yy >= destPtr->height) || ((yy + (int)slot->bitmap.rows) <= 0)) {
        return;                         /* No portion of the glyph is visible
                                         * in the picture. */
    }

    /* By default, set the region to cover the entire glyph */
    x1 = y1 = 0;
    x2 = slot->bitmap.width;
    y2 = slot->bitmap.rows;

    /* Determine the portion of the glyph inside the picture. */

    if (xx < 0) {                       /* Left side of glyph overhangs. */
        x1 -= xx;               
        x2 += xx;
        xx = 0;         
    }
    if (yy < 0) {                       /* Top of glyph overhangs. */
        y1 -= yy;
        y2 += yy;
        yy = 0;
    }
    if ((xx + x2) > destPtr->width) {   /* Right side of glyph overhangs. */
        x2 = destPtr->width - xx;
    }
    if ((yy + y2) > destPtr->height) {  /* Bottom of glyph overhangs. */
        y2 = destPtr->height - yy;
    }

    {
        Blt_Pixel *destRowPtr;
        unsigned char *srcRowPtr;
        int y;

        srcRowPtr = slot->bitmap.buffer + ((y1 * slot->bitmap.pitch) + x1);
        destRowPtr = Blt_Picture_Pixel(destPtr, xx, yy);
        for (y = y1; y < y2; y++) {
            Blt_Pixel *dp;
            unsigned char *sp;
            int x;

            dp = destRowPtr;
            sp = srcRowPtr;
            for (x = x1; x < x2; x++, sp++, dp++) {
                if (*sp != 0x0) {
                    Blt_Pixel color;

                    color.u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
                    Blt_FadeColor(&color, *sp);
                    BlendPixels(dp, &color);
                }
            }
            srcRowPtr += slot->bitmap.pitch;
            destRowPtr += destPtr->pixelsPerRow;
        }
    }
}

static void
CopyMonoGlyph(Pict *destPtr, FT_GlyphSlot slot, int xx, int yy,
          Blt_PaintBrush brush)
{
    int x1, y1, x2, y2;

#if DEBUG
    fprintf(stderr, "dx=%d, dy=%d\n", dx, dy);
    DebugGlyph(slot);
#endif
    
    if ((xx >= destPtr->width) || ((xx + (int)slot->bitmap.width) <= 0) ||
        (yy >= destPtr->height) || ((yy + (int)slot->bitmap.rows) <= 0)) {
        return;                         /* No portion of the glyph is visible
                                         * in the picture. */
    }
    /* By default, set the region to cover the entire glyph */
    x1 = y1 = 0;
    x2 = slot->bitmap.width;
    y2 = slot->bitmap.rows;

    /* Determine the portion of the glyph inside the picture. */

    if (xx < 0) {                       /* Left side of glyph overhangs. */
        x1 -= xx;               
        x2 += xx;
        xx = 0;         
    }
    if (yy < 0) {                       /* Top of glyph overhangs. */
        y1 -= yy;
        y2 += yy;
        yy = 0;
    }
    if ((xx + x2) > destPtr->width) {   /* Right side of glyph overhangs. */
        x2 = destPtr->width - xx;
    }
    if ((yy + y2) > destPtr->height) {  /* Bottom of glyph overhangs. */
        y2 = destPtr->height - yy;
    }
    {
        Blt_Pixel *destRowPtr;
        unsigned char *srcRowPtr;
        int y;

        srcRowPtr = slot->bitmap.buffer + (y1 * slot->bitmap.pitch);
        destRowPtr = Blt_Picture_Pixel(destPtr, xx, yy);
        for (y = y1; y < y2; y++) {
            Blt_Pixel *dp;
            int x;
            
            dp = destRowPtr;
            for (x = x1; x < x2; x++, dp++) {
                int pixel;

                pixel = srcRowPtr[x >> 3] & (1 << (7 - (x & 0x7)));
                if (pixel != 0x0) {
                    dp->u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
                }
            }
            srcRowPtr += slot->bitmap.pitch;
            destRowPtr += destPtr->pixelsPerRow;
        }
    } 
}

static int 
ScaleFont(Tcl_Interp *interp, FtFont *fontPtr, FT_F26Dot6 size)
{
    FT_Error ftError;
    int xdpi, ydpi;

    Blt_ScreenDPI(Tk_MainWindow(interp), &xdpi, &ydpi);
    ftError = FT_Set_Char_Size(fontPtr->face, size, size, xdpi, ydpi);
    if (ftError) {
        Tcl_AppendResult(interp, "can't set font size to \"", Blt_Itoa(size), 
                "\": ", FtError(ftError), (char *)NULL);
        return TCL_ERROR;
    }
    fontPtr->height  = fontPtr->face->size->metrics.height >> 6;
    fontPtr->ascent  = fontPtr->face->size->metrics.ascender >> 6;
    fontPtr->descent = fontPtr->face->size->metrics.descender >> 6;
    return TCL_OK;
}

static void
RotateFont(FtFont *fontPtr, float angle)
{
    /* Set up the transformation matrix. */
    double theta; 

    theta = angle * DEG2RAD;
    fontPtr->matrix.yy = fontPtr->matrix.xx = (FT_Fixed)(cos(theta) * 65536.0);
    fontPtr->matrix.yx = (FT_Fixed)(sin(theta) * 65536.0);
    fontPtr->matrix.xy = -fontPtr->matrix.yx;
}


static int
OpenFont(Tcl_Interp *interp, Tcl_Obj *objPtr, size_t fontSize,
         FtFont **fontPtrPtr) 
{
    FtFont *fontPtr;
    FT_Error ftError;
    FT_Face face;
    const char *fileName, *fontName;
    Tcl_Obj *fileObjPtr;

    fontName = Tcl_GetString(objPtr);
    fileObjPtr = NULL;
    face = NULL;
    if (fontName[0] == '@') {
        fileName = fontName + 1;
        fontSize <<= 6;
    } else {
        double size;

        fileObjPtr = Blt_Font_GetFile(interp, objPtr, &size);
        if (fileObjPtr == NULL) {
            return TCL_ERROR;
        }
        if (fontSize == 0) {
            fontSize = (int)(size * 64.0 + 0.5);
        }
        Tcl_IncrRefCount(fileObjPtr);
        fileName = Tcl_GetString(fileObjPtr);
    }
    ftError = FT_New_Face(ftLibrary, fileName, 0, &face);
    if (ftError) {
        Tcl_AppendResult(interp, "can't create face from font file \"", 
                fileName, "\": ", FtError(ftError), (char *)NULL);
        goto error;
    }
    if (!FT_IS_SCALABLE(face)) {
        Tcl_AppendResult(interp, "can't use font \"", fontName, 
                         "\": font isn't scalable.", (char *)NULL);
        goto error;
    }
    if (fileObjPtr != NULL) {
        Tcl_DecrRefCount(fileObjPtr);
    }
    fontPtr = Blt_AssertCalloc(1, sizeof(FtFont));
    fontPtr->face = face;
    if (ScaleFont(interp, fontPtr, fontSize) != TCL_OK) {
        Blt_Free(fontPtr);
        goto error;
    }
    RotateFont(fontPtr, 0.0f);          /* Initializes the rotation matrix. */
    *fontPtrPtr = fontPtr;
    return TCL_OK;
 error:
    if (fileObjPtr != NULL) {
        Tcl_DecrRefCount(fileObjPtr);
    }
    if (face != NULL) {
        FT_Done_Face(face);
    }   
    return TCL_ERROR;
}

static void
CloseFont(FtFont *fontPtr) 
{
#ifdef HAVE_LIBXFT
    if (fontPtr->xftFont != NULL) {
        XftUnlockFace(fontPtr->xftFont);
    } else {
        FT_Done_Face(fontPtr->face);
    }
#else
    FT_Done_Face(fontPtr->face);
#endif /* HAVE_LIBXFT */ 
    Blt_Free(fontPtr);
}


static int
PaintText(Pict *destPtr, FtFont *fontPtr, const char *string, size_t length,
          int x, int y, int kerning, Blt_PaintBrush brush)
{
    FT_Error ftError;
    int h;

    FT_Vector pen;                      /* Untransformed origin  */
    const char *p, *pend;
    FT_GlyphSlot slot;
    FT_Face face;                       /* Face object. */  

    h = destPtr->height;
    face = fontPtr->face;
    slot = face->glyph;
    int yy;
    int previous;

#if DEBUG
    DebugFace(face);
#endif
    yy = y;
    previous = -1;
    FT_Set_Transform(face, &fontPtr->matrix, NULL);
    pen.y = (h - y) << 6;
    pen.x = x << 6;
    for (p = string, pend = p + length; p < pend; p++) {
        int current;

        current = FT_Get_Char_Index(face, *p);
        if ((kerning) && (previous >= 0)) { 
            FT_Vector delta; 
                
            FT_Get_Kerning(face, previous, current, FT_KERNING_DEFAULT, &delta);
            pen.x += delta.x; 
        } 
        FT_Set_Transform(face, &fontPtr->matrix, &pen);
        previous = current;
        /* load glyph image into the slot (erase previous one) */  
        ftError = FT_Load_Glyph(face, current, FT_LOAD_DEFAULT); 
        if (ftError) {
            Blt_Warn("can't load character \"%c\": %s\n", *p, 
                     FtError(ftError));
            continue;                 /* Ignore errors */
        }
        ftError = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
        if (ftError) {
            Blt_Warn("can't render glyph \"%c\": %s\n", *p, FtError(ftError));
            continue;                 /* Ignore errors */
        }
        switch(slot->bitmap.pixel_mode) {
        case FT_PIXEL_MODE_MONO:
            CopyMonoGlyph(destPtr, slot, pen.x >> 6, yy - slot->bitmap_top, 
                brush);
            break;
        case FT_PIXEL_MODE_LCD:
        case FT_PIXEL_MODE_LCD_V:
        case FT_PIXEL_MODE_GRAY:
#ifdef notdef
            fprintf(stderr, "h=%d, slot->bitmap_top=%d\n", h, slot->bitmap_top);
#endif
            {
                int xx, yy;

                xx = slot->bitmap_left;
                yy = h - slot->bitmap_top;
                if ((xx < destPtr->width) && ((xx + (int)slot->bitmap.width) >= 0) &&
                    (yy < destPtr->height) && ((yy + (int)slot->bitmap.rows) >= 0)) {
                    PaintGrayGlyph(destPtr, slot, slot->bitmap_left, 
                                   h - slot->bitmap_top, brush);
                }
            }
        case FT_PIXEL_MODE_GRAY2:
        case FT_PIXEL_MODE_GRAY4:
            break;
        }
        pen.x += slot->advance.x; 
        pen.y += slot->advance.y;
        previous = -1;
    }   
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextOp --
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *      image draw text string x y switches 
 *---------------------------------------------------------------------------
 */
static int
TextOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv) 
{ 
    Pict *destPtr = clientData;
    Blt_Shadow *shadowPtr;
    FtFont *fontPtr;
    TextSwitches switches;
    const char *string;
    int length;
    int x, y;

    string = Tcl_GetStringFromObj(objv[3], &length);
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
        return TCL_ERROR;
    }
    /* Process switches  */
    switches.anchor = TK_ANCHOR_NW;
    switches.angle = 0.0;
    Blt_Shadow_Set(&switches.shadow, 0, 0, 0x0, 0xA0);
    if (Blt_GetPaintBrush(interp, "black", &switches.brush) 
        != TCL_OK) {
        return TCL_ERROR;
    }
    switches.fontObjPtr = Tcl_NewStringObj("Arial 12", -1);
    switches.fontSize = 0;
    switches.kerning = FALSE;
    switches.justify = TK_JUSTIFY_LEFT;
    shadowPtr = &switches.shadow;
    if (Blt_ParseSwitches(interp, textSwitches, objc - 6, objv + 6, &switches, 
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (OpenFont(interp, switches.fontObjPtr, switches.fontSize, &fontPtr)
        != TCL_OK) {
        return TCL_ERROR;
    }
#ifdef notdef
    if ((destPtr->flags & BLT_PIC_PREMULT_COLORS) == 0) {
        Blt_PremultiplyColors(destPtr);
    }
#endif
    if (switches.angle != 0.0) {
        TextStyle ts;
        TextLayout *layoutPtr;

        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetJustify(ts, switches.justify);
        layoutPtr = CreateSimpleTextLayout(fontPtr, string, length, &ts);
        if (fontPtr->face->face_flags & FT_FACE_FLAG_SCALABLE) {
            double rw, rh;
            TextFragment *fp, *fend;

            Blt_RotateStartingTextPositions(layoutPtr, switches.angle);
            Blt_GetBoundingBox((double)layoutPtr->width, 
                (double)layoutPtr->height, 
                switches.angle, &rw, &rh, (Point2d *)NULL);
            Blt_TranslateAnchor(x, y, (int)(rw), (int)(rh), switches.anchor, 
                &x, &y);
            RotateFont(fontPtr, switches.angle);
            for (fp = layoutPtr->fragments, fend = fp + layoutPtr->numFragments;
                fp < fend; fp++) {
                PaintText(destPtr, fontPtr, fp->text, fp->numBytes, x + fp->rx, 
                        y + fp->ry, switches.kerning, switches.brush);
            }
        } else {
            Blt_Picture tmp;
            Pict *rotPtr;
            int i;
            
            tmp = Blt_CreatePicture(layoutPtr->width, layoutPtr->height);
            Blt_BlankPicture(tmp, 0x00FF0000);
            for (i = 0; i < layoutPtr->numFragments; i++) {
                TextFragment *fp;

                fp = layoutPtr->fragments + i;
                PaintText(tmp, fontPtr, fp->text, fp->numBytes, fp->rx, fp->ry, 
                        switches.kerning, switches.brush);
            }
            rotPtr = Blt_RotatePicture(tmp, switches.angle);
            Blt_FreePicture(tmp);
            Blt_TranslateAnchor(x, y, rotPtr->width, rotPtr->height, 
                switches.anchor, &x, &y);
            Blt_CompositeRegion(destPtr, rotPtr, 0, 0, rotPtr->width, 
                            rotPtr->height, x, y);
            Blt_FreePicture(rotPtr);
        }
        Blt_Free(layoutPtr);
    } else { 
        int w, h;
        TextStyle ts;
        TextLayout *layoutPtr;

        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetJustify(ts, switches.justify);
        layoutPtr = CreateSimpleTextLayout(fontPtr, string, length, &ts);
        Blt_TranslateAnchor(x, y, layoutPtr->width, layoutPtr->height, 
                switches.anchor, &x, &y);
        w = layoutPtr->width;
        h = layoutPtr->height;
        if ((w > 1) && (h > 1) && (switches.shadow.width > 0)) {
            Blt_Pixel color;
            Pict *tmpPtr;
            int extra;
            Blt_PaintBrush brush;
            int i;

            extra = 2 * shadowPtr->width;
            tmpPtr = Blt_CreatePicture(w + extra, h + extra);
            color.u32 = shadowPtr->color.u32;
            color.Alpha = 0x0;
            Blt_BlankPicture(tmpPtr, color.u32);
            brush = Blt_NewColorBrush(shadowPtr->color.u32);
            for (i = 0; i < layoutPtr->numFragments; i++) {
                TextFragment *fp;

                fp = layoutPtr->fragments + i;
                PaintText(tmpPtr, fontPtr, fp->text, fp->numBytes, 
                          fp->x + shadowPtr->width, 
                          fp->y + shadowPtr->width, 
                          switches.kerning, brush);
            }
            Blt_FreeBrush(brush);
            Blt_BlurPicture(tmpPtr, tmpPtr, shadowPtr->width, 3);
            Blt_CompositeRegion(destPtr, tmpPtr, 0, 0, tmpPtr->width, 
                            tmpPtr->height, x, y);
            for (i = 0; i < layoutPtr->numFragments; i++) {
                TextFragment *fp;

                fp = layoutPtr->fragments + i;
                PaintText(destPtr, fontPtr, fp->text, fp->numBytes, 
                    x + fp->x + shadowPtr->width - shadowPtr->offset,
                    y + fp->y + shadowPtr->width - shadowPtr->offset, 
                    switches.kerning, switches.brush);
            }
            Blt_FreePicture(tmpPtr);
        } else {
            int i;

            for (i = 0; i < layoutPtr->numFragments; i++) {
                TextFragment *fp;

                fp = layoutPtr->fragments + i;
                PaintText(destPtr, fontPtr, fp->text, fp->numBytes, 
                        x + fp->x, y + fp->y, switches.kerning, 
                        switches.brush);
            }
        }
        Blt_Free(layoutPtr);
    }
    CloseFont(fontPtr);
    Blt_FreeSwitches(textSwitches, (char *)&switches, 0);
    return TCL_OK;
}

static int
InitTextOp(Tcl_Interp *interp)
{
    FT_Error ftError;

    ftError = FT_Init_FreeType(&ftLibrary);
    if (ftError) {
        Tcl_AppendResult(interp, "can't initialize freetype library: ", 
                         FtError(ftError), (char *)NULL);
        return TCL_ERROR;
    }
    if (Blt_PictureRegisterProc(interp, "text", TextOp) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PictureTextInit --
 *
 *      This procedure is invoked to initialize the "text" operation
 *      of picture instance commands.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Creates the new command and adds a new entry into a global Tcl
 *      associative array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_PictureTextInit(Tcl_Interp *interp)
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
        return TCL_ERROR;
    };
#endif
#ifdef USE_TK_STUBS
    if (Tk_InitStubs(interp, TK_VERSION_COMPILED, PKG_ANY) == NULL) {
        return TCL_ERROR;
    };
#endif
#ifdef USE_BLT_STUBS
    if (Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    };
    if (Blt_InitTkStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    };
#else
    if (Tcl_PkgRequire(interp, "blt_tcl", BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_PkgRequire(interp, "blt_tk", BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    }
#endif    
    if (InitTextOp(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, "blt_picture_text", BLT_VERSION) != TCL_OK) { 
        return TCL_ERROR;
    }
    return TCL_OK;
}

int 
Blt_PictureTextSafeInit(Tcl_Interp *interp) 
{
    return Blt_PictureTextInit(interp);
}

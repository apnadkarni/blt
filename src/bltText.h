/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltText.h --
 *
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

#ifndef _BLT_TEXT_H
#define _BLT_TEXT_H

#include "bltBg.h"

#define DEF_TEXT_FLAGS (TK_PARTIAL_OK | TK_IGNORE_NEWLINES)
#define UPDATE_GC       1

/*
 * TextFragment --
 */
typedef struct {
    const char *text;                   /* Text string to be displayed */
    size_t numBytes;                    /* Number of bytes in text. The
                                         * actual character count may
                                         * differ because of multi-byte UTF
                                         * encodings. */
    short x, y;                         /* X-Y offset of the baseline from
                                         * the upper-left corner of the
                                         * bbox. */
    float rx, ry;                       /* Starting offset of fragment
                                         * after rotation. This is used to
                                         * position the text when using a
                                         * rotated font. */
    int w;                              /* Width of segment in pixels. This
                                         * is used justify the fragment
                                         * with other fragments and to
                                         * indicate if an ellipsis is
                                         * needed. */
} TextFragment;


/*
 * TextItem --
 * 
 *      Parsed form for markup string.  Each item is a scrap of text
 *      describes the font, position, and characters to be displayed.
 *      
 *      subscript x_y  very small subset of latex markup.
 *      superscript x^y
 *      grouping a^{x+y} a_{i,j}
 *      supersuper a^{10^8}
 *      \hat{a} \bar{b} \vec{c}
 *      \overline{} \underline{}
 *      \frac \tfrac
 *      \Alpha \Beta ...
 *      \mathbf{} \mathit{} \mathrm{}  \boldsymbol{}
 *      \angstrom \degree 
 *
 *      -mathtext instead of -text 
 *
 *      Can use TextItem where you don't directly edit the text:
 *        label, treeview, graph, barchart...
 *
 *      Font selector (bold, italic, size adjust) from base font.
 *      Global font table reference counted. 
 *
 */
typedef struct {
    const char *text;                   /* Text string to be displayed */
    int numBytes;                       /* Number of bytes in text. The
                                         * actual character count may
                                         * differ because of multi-byte UTF
                                         * encodings. */
    short int x, y;                     /* X-Y offset of the baseline from
                                         * the upper-left corner of the
                                         * bbox. */
    short int sx, sy;                   /* Starting offset of text using
                                         * rotated font. */
    Blt_Font font;                      /* Allocated font for this chunk.
                                         * If NULL, use the global font. */
    int underline;                      /* Text is underlined */
    int width;                          /* Width of segment in pixels. This
                                         * information is used to draw
                                         * PostScript strings the same
                                         * width as X. (deprecated) */
} TextItem;

/*
 * TextLayout --
 */
typedef struct {
    TextFragment *underlinePtr;
    int underline;
    int width, height;                  /* Dimensions of text bounding
                                         * box */
    int numFragments;                   /* # fragments of text */
    TextFragment fragments[1];          /* Information about each fragment of
                                         * text */
} TextLayout;

/*
 * TextStyle --
 *
 *      A somewhat convenient structure to hold text attributes that determine
 *      how a text string is to be displayed on the screen or drawn with
 *      PostScript commands.  The alternative is to pass lots of parameters to
 *      the drawing and printing routines. This seems like a more efficient
 *      and less cumbersome way of passing parameters.
 */
typedef struct {
    unsigned int state;                 /* If non-zero, indicates to draw
                                         * text in the active color */
    XColor *color;                      /* Color to draw the text. */
    Blt_Font font;                      /* Font to use to draw text */
    Blt_Bg bg;                          /* Background color of text.  This
                                         * is also used for drawing
                                         * disabled text. */
    float angle;                        /* Rotation of text in degrees. */
    Tk_Justify justify;                 /* Justification of the text
                                         * string. This only matters if the
                                         * text is composed of multiple
                                         * lines. */
    Tk_Anchor anchor;                   /* Indicates how the text is
                                         * anchored around its x,y
                                         * coordinates. */
    Blt_Pad padX, padY;                 /* # pixels padding of around text
                                         * region. */
    unsigned short int leader;          /* # pixels spacing between lines
                                         * of text. */
    short int underline;                /* Index of character to be underlined,
                                         * -1 if no underline. */
    int maxLength;                      /* Maximum length in pixels of
                                         * text */
    /* Private fields. */
    unsigned short flags;
    GC gc;                              /* GC used to draw the text */
    TkRegion rgn;
} TextStyle;

BLT_EXTERN void Blt_DrawText(Tk_Window tkwin, Drawable drawable, 
        const char *string, TextStyle *tsPtr, int x, int y);

BLT_EXTERN void Blt_DrawText2(Tk_Window tkwin, Drawable drawable, 
        const char *string, TextStyle *tsPtr, int x, int y, Dim2d * dimPtr);

BLT_EXTERN int Blt_DrawTextWithRotatedFont(Tk_Window tkwin, Drawable drawable, 
        float angle, TextStyle *tsPtr, TextLayout *textPtr, int x, int y);

BLT_EXTERN void Blt_DrawLayout(Tk_Window tkwin, Drawable drawable, GC gc, 
        Blt_Font font, int depth, float angle, int x, int y, 
        TextLayout *layoutPtr, int maxLength);

BLT_EXTERN void Blt_GetTextExtents(Blt_Font font, int leader, const char *text, 
        int textLen, unsigned int *widthPtr, unsigned int *heightPtr);

BLT_EXTERN int Blt_MeasureText(Blt_Font font, const char *text, int textLen,
        int maxLength, int *nBytesPtr);

BLT_EXTERN void Blt_RotateStartingTextPositions(TextLayout *textPtr,
        float angle);

BLT_EXTERN int Blt_TkTextLayout_CharBbox(Tk_TextLayout layout, int index, 
        int *xPtr, int *yPtr, int *widthPtr, int *heightPtr);

BLT_EXTERN Tk_TextLayout Blt_TkTextLayout_Compute(Blt_Font font, 
        const char *string, int numChars, int wrapLength, Tk_Justify justify, 
        int flags, int *widthPtr, int *heightPtr);

BLT_EXTERN void Blt_TkTextLayout_Draw(Display *display, Drawable drawable, 
        GC gc, Tk_TextLayout layout, int x, int y, int firstChar, int lastChar);

BLT_EXTERN void Blt_TkTextLayout_Free(Tk_TextLayout layout);

BLT_EXTERN void Blt_TkTextLayout_UnderlineSingleChar(Display *display, 
        Drawable drawable, GC gc, Tk_TextLayout layout, int x, int y, 
        int underline);

BLT_EXTERN Pixmap Blt_Ts_Bitmap(Tk_Window tkwin, TextLayout *textPtr, 
        TextStyle *tsPtr, int *widthPtr, int *heightPtr);

BLT_EXTERN TextLayout *Blt_Ts_CreateLayout(const char *string, int length, 
        TextStyle *tsPtr);

BLT_EXTERN void Blt_Ts_DrawLayout(Tk_Window tkwin, Drawable drawable, 
        TextLayout *textPtr, TextStyle *tsPtr, int x, int y);

BLT_EXTERN void Blt_Ts_DrawText(Tk_Window tkwin, Drawable drawable, 
        const char *text, int textLen, TextStyle *tsPtr, int x, int y);

BLT_EXTERN void Blt_Ts_FreeStyle(Display *display, TextStyle *tsPtr);

BLT_EXTERN void Blt_Ts_GetExtents(TextStyle *tsPtr, const char *text, 
        unsigned int *widthPtr, unsigned int *heightPtr);

BLT_EXTERN void Blt_Ts_ResetStyle(Tk_Window tkwin, TextStyle *tsPtr);

BLT_EXTERN void Blt_Ts_SetDrawStyle (TextStyle *tsPtr, Blt_Font font, GC gc, 
        XColor *fgColor, float angle, Tk_Anchor anchor, Tk_Justify justify, 
        int leader);

BLT_EXTERN TextLayout *Blt_Ts_TitleLayout(const char *string, int length, 
        TextStyle *tsPtr);

BLT_EXTERN void Blt_Ts_UnderlineChars(Tk_Window tkwin, Drawable drawable, 
        TextLayout *layoutPtr, TextStyle *tsPtr, int x, int y);


#define Blt_Ts_GetAnchor(ts)            ((ts).anchor)
#define Blt_Ts_GetAngle(ts)             ((ts).angle)
#define Blt_Ts_GetBackground(ts)        ((ts).bg)
#define Blt_Ts_GetFont(ts)              ((ts).font)
#define Blt_Ts_GetForeground(ts)        ((ts).color)
#define Blt_Ts_GetJustify(ts)           ((ts).justify)
#define Blt_Ts_GetLeader(ts)            ((ts).leader)

#define Blt_Ts_SetAnchor(ts, a) ((ts).anchor = (a))
#define Blt_Ts_SetAngle(ts, r)          ((ts).angle = (float)(r))
#define Blt_Ts_SetBackground(ts, b)     ((ts).bg = (b))
#define Blt_Ts_SetFont(ts, f)           \
        (((ts).font != (f)) ? ((ts).font = (f), (ts).flags |= UPDATE_GC) : 0)
#define Blt_Ts_SetForeground(ts, c)    \
        (((ts).color != (c)) ? ((ts).color = (c), (ts).flags |= UPDATE_GC) : 0)
#define Blt_Ts_SetGC(ts, g)     ((ts).gc = (g), (ts).flags &= ~UPDATE_GC)
#define Blt_Ts_SetJustify(ts, j)        ((ts).justify = (j))
#define Blt_Ts_SetLeader(ts, l) ((ts).leader = (l))
#define Blt_Ts_SetMaxLength(ts, l)      ((ts).maxLength = (l))
#define Blt_Ts_SetPadding(ts, l, r, t, b)       \
        ((ts).padX.side1 = (l),                 \
        (ts).padX.side2 = (r),                  \
        (ts).padY.side1 = (t),                  \
        (ts).padY.side2 = (b))
#define Blt_Ts_SetState(ts, s)          ((ts).state = (s))
#define Blt_Ts_SetUnderlineChar(ts, ul)     ((ts).underline = (ul))
#define Blt_Ts_SetFontClipRegion(ts, r) ((ts).rgn = (r))

#define Blt_Ts_InitStyle(ts)                    \
    ((ts).anchor = TK_ANCHOR_NW,                \
     (ts).color = (XColor *)NULL,               \
     (ts).font = NULL,                          \
     (ts).justify = TK_JUSTIFY_LEFT,            \
     (ts).leader = 0,                           \
     (ts).underline = -1,                       \
     (ts).padX.side1 = (ts).padX.side2 = 0,     \
     (ts).padY.side1 = (ts).padY.side2 = 0,     \
     (ts).state = 0,                            \
     (ts).flags = 0,                            \
     (ts).gc = NULL,                            \
     (ts).maxLength = -1,                       \
     (ts).angle = 0.0,                          \
     (ts).rgn = NULL)

#endif /* _BLT_TEXT_H */

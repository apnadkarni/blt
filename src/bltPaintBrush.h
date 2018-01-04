/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPaintBrush.h --
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

#ifndef _BLT_PAINTBRUSH_H
#define _BLT_PAINTBRUSH_H

#ifndef _BLT_PALETTE_H
typedef struct _Blt_Palette *Blt_Palette;
#endif  /* _BLT_PALTTE_H */

typedef struct _Blt_PaintBrushClass Blt_PaintBrushClass;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PaintBrush --
 *
 *      Represents either a solid color, gradient, tile, or texture.  Used
 *      to paint basic geometric objects.
 *
 *---------------------------------------------------------------------------
 */
typedef struct _Blt_PaintBrush *Blt_PaintBrush;
typedef struct _Blt_PaintBrushNotifier *Blt_PaintBrushNotifier;

typedef void (Blt_BrushChangedProc)(ClientData clientData,
        Blt_PaintBrush brush);

typedef int (Blt_PaintBrushCalcProc)(ClientData clientData, int x, int y,
        double *valuePtr);

struct _Blt_PaintBrush {
    Blt_PaintBrushClass *classPtr;
    const char *name;                   /* If non-NULL, the name of
                                         * brush. This is only for brushes
                                         * created by the blt::paintbrush
                                         * command. Points to hash table
                                         * key. */
    int refCount;                       /* # of clients using this brush.
                                         * If zero, this brush can be
                                         * deleted. */
    int xOrigin, yOrigin;               /* Offset of gradient from top of
                                         * window. */
    unsigned int flags;
    int alpha;                          /* Opacity of the brush. */
    Blt_Jitter jitter;                  /* Generates a random value to be
                                         * added when interpolating the
                                         * color */
    ClientData clientData;
    Blt_Chain notifiers;                /* List of client notifiers. */
    Blt_Palette palette;                /* If non-NULL, palette to use for
                                         * coloring the gradient. */

};

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ColorBrush --
 *
 *      Represents a single color.  The color may be transparent.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    Blt_PaintBrushClass *classPtr;
    const char *name;                   /* If non-NULL, the name of
                                         * brush. This is only for brushes
                                         * created by the blt::paintbrush
                                         * command. Points to hash table
                                         * key. */
    int refCount;                       /* # of clients using this brush.
                                         * If zero, this brush can be
                                         * deleted. */
    int xOrigin, yOrigin;               /* Offset of gradient from top of
                                         * window. */
    unsigned int flags;
    int alpha;                          /* Opacity of the brush. */
    Blt_Jitter jitter;                  /* Generates a random value to be
                                         * added when interpolating the
                                         * color */
    ClientData clientData;
    Blt_Chain notifiers;                /* List of client notifiers. */

    Blt_Pixel reqColor;                 /* Requested color of the brush. */
    Blt_Pixel color;                    /* Color of the brush w/
                                         * opacity.  */
} Blt_ColorBrush;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LinearGradientBrush --
 *
 *      Represents a linear gradient.  It contains information to compute
 *      the gradient color value at each pixel.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    Blt_PaintBrushClass *classPtr;
    const char *name;                   /* If non-NULL, the name of
                                         * brush. This is only for brushes
                                         * created by the blt::paintbrush
                                         * command. Points to hash table
                                         * key. */
    int refCount;                       /* # of clients using this brush.
                                         * If zero, this brush can be
                                         * deleted. */
    int xOrigin, yOrigin;               /* Offset of gradient from top of
                                         * window. */
    unsigned int flags;
    int alpha;                          /* Opacity of the brush. */
    Blt_Jitter jitter;                  /* Generates a random value to be
                                         * added when interpolating the
                                         * color */
    ClientData clientData;
    Blt_Chain notifiers;                /* List of client notifiers. */

    /* Gradient-specific fields. */
    Blt_Palette palette;                /* If non-NULL, palette to use for
                                         * coloring the gradient. */
    Blt_PaintBrushCalcProc *calcProc;
    Blt_Pixel low, high;                /* Starting and ending colors of
                                         * the gradient. This is overridden
                                         * by the palette. */
    int aRange, rRange, gRange, bRange; /*  */
    double angle;                       /* Angle of rotation for the line
                                         * segment (not implemented). */
    Point2d from, to;                   /* Starting and ending positions of
                                         * a line segment defining the
                                         * gradient. The coordinates are
                                         * relative to the width and height
                                         * of the region. */
    /* Computed values. */
    double length;                       /* Length of line segment. */
    int x1, y1, x2, y2;                  /* Line segment in pixels after
                                          * specifying the region. */
    double scaleFactor;                  /* 1 / length  */
} Blt_LinearGradientBrush;


/*
 *---------------------------------------------------------------------------
 *
 * Blt_TileBrush --
 *
 *      Represents a tile brush.  It contains information to compute
 *      the tile color value at each pixel.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    Blt_PaintBrushClass *classPtr;
    const char *name;                   /* If non-NULL, the name of
                                         * brush. This is only for brushes
                                         * created by the blt::paintbrush
                                         * command. Points to hash table
                                         * key. */
    int refCount;                       /* # of clients using this brush.
                                         * If zero, this brush can be
                                         * deleted. */
    int xOrigin, yOrigin;               /* Offset of tile from top of
                                         * window. */
    unsigned int flags;
    int alpha;                          /* Opacity of the background
                                         * color. */
    Blt_Jitter jitter;                  /* Generates a random value to be
                                         * added when interpolating the
                                         * color */
    ClientData clientData;
    Blt_Chain notifiers;                /* List of client notifiers. */

    /* Tile-specific fields. */
    Tk_Image tkImage;                   /* Tk image used for tiling. */
    Blt_Picture tile;                   /* If non-NULL, picture to use for
                                         * tiling. This is converted from
                                         * the Tk image. */
    Blt_Pixel color;                    /* Color to display for the background
                                         * of the tile. */
    int x, y;                           /* Start of tile. */
} Blt_TileBrush;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_StripesBrush --
 *
 *      Represents a stripes brush.  It contains information to compute
 *      the stripe color value at each point.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    Blt_PaintBrushClass *classPtr;
    const char *name;                   /* If non-NULL, the name of
                                         * brush. This is only for brushes
                                         * created by the blt::paintbrush
                                         * command. Points to hash table
                                         * key. */
    int refCount;                       /* # of clients using this brush.
                                         * If zero, this brush can be
                                         * deleted. */
    int xOrigin, yOrigin;               /* Offset of tile from top of
                                         * window. */
    unsigned int flags;
    int alpha;
    Blt_Jitter jitter;                  /* Generates a random value to be
                                         * added when interpolating the
                                         * color */
    ClientData clientData;
    Blt_Chain notifiers;                /* List of client notifiers. */

    /* Stripes-specific fields. */
    Blt_Pixel low, high;                /* Texture or gradient colors. */
    int aRange, rRange, gRange, bRange;
    int stride;
} Blt_StripesBrush;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CheckersBrush --
 *
 *      Represents a checkers brush.  It contains information to compute
 *      the checker color value at each point.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    Blt_PaintBrushClass *classPtr;
    const char *name;                   /* If non-NULL, the name of
                                         * brush. This is only for brushes
                                         * created by the blt::paintbrush
                                         * command. Points to hash table
                                         * key. */
    int refCount;                       /* # of clients using this brush.
                                         * If zero, this brush can be
                                         * deleted. */
    int xOrigin, yOrigin;               /* Offset of tile from top of
                                         * window. */
    unsigned int flags;
    int alpha;
    Blt_Jitter jitter;                  /* Generates a random value to be
                                         * added when interpolating the
                                         * color */
    ClientData clientData;
    Blt_Chain notifiers;                /* List of client notifiers. */

    /* Checkers-specific fields. */
    Blt_Pixel low, high;                /* On/Off colors. */
    int aRange, rRange, gRange, bRange;
    int stride;
    int x, y;
} Blt_CheckersBrush;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_RadialGradrientBrush --
 *
 *      Represents a radial gradient brush.  It contains information to
 *      compute the gradient color value at each pixel of a region.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    Blt_PaintBrushClass *classPtr;
    const char *name;                   /* If non-NULL, the name of
                                         * brush. This is only for brushes
                                         * created by the blt::paintbrush
                                         * command. Points to hash table
                                         * key. */
    int refCount;                       /* # of clients using this brush.
                                         * If zero, this brush can be
                                         * deleted. */
    int xOrigin, yOrigin;               /* Offset of gradient from top of
                                         * window. */
    unsigned int flags;
    int alpha;                          /* Opacity of the brush. */
    Blt_Jitter jitter;                  /* Generates a random value to be
                                         * added when interpolating the
                                         * color */
    ClientData clientData;
    Blt_Chain notifiers;                /* List of client notifiers. */

    /* Radial gradient-specific fields. */
    Blt_Palette palette;                /* If non-NULL, palette to use for
                                         * coloring the gradient. */
    Blt_PaintBrushCalcProc *calcProc;
    Blt_Pixel low, high;                /* Texture or gradient colors. */
    int aRange, rRange, gRange, bRange;
    double angle;                       /* Angle of rotation for the
                                         * gradient. */
    Point2d center;                     /* Center of the radial
                                         * gradient. This point is a
                                         * relative to the size of the
                                         * region. */
    double width, height;               /* Width and height of an
                                         * elliptical gradient. */
    double diameter;                    /* If non-zero, diameter of a
                                         * circular gradient. This
                                         * overrides the width and height
                                         * for elliptical gradients */
    /* Computed values. */
    int cx, cy;                          /* Center of the gradient after
                                          * specifying the region. */
    int a, b;                            /* Radii (x and y axis) of the
                                          * gradient. */
} Blt_RadialGradientBrush;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConicalGradrientBrush --
 *
 *      Represents a conical gradient brush.  It contains information to
 *      compute the gradient color value at each pixel of a region.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    Blt_PaintBrushClass *classPtr;
    const char *name;                   /* If non-NULL, the name of
                                         * brush. This is only for brushes
                                         * created by the blt::paintbrush
                                         * command. Points to hash table
                                         * key. */
    int refCount;                       /* # of clients using this brush.
                                         * If zero, this brush can be
                                         * deleted. */
    int xOrigin, yOrigin;               /* Offset of gradient from top of
                                         * window. */
    unsigned int flags;
    int alpha;                          /* Opacity of the brush. */
    Blt_Jitter jitter;                  /* Generates a random value to be
                                         * added when interpolating the
                                         * color */
    ClientData clientData;
    Blt_Chain notifiers;                /* List of client notifiers. */

    /* Conical gradient-specific fields. */
    Blt_Palette palette;                /* If non-NULL, palette to use for
                                         * coloring the gradient. */

    Blt_PaintBrushCalcProc *calcProc;
    Blt_Pixel low, high;                /* Texture or gradient colors. */
    int aRange, rRange, gRange, bRange;
    double angle;                       /* Angle of rotation for the
                                         * gradient in degrees. */
    Point2d center;                     /* Center of the conical
                                         * gradient. This point is a
                                         * relative to the size of the
                                         * region. */
    /* Computed values. */
    double theta;                       /* Angle of rotation for the
                                         * gradient in radians. */
    int cx, cy;                         /* Center of the gradient after
                                         * specifying the region. */
} Blt_ConicalGradientBrush;

typedef enum Blt_PaintBrushTypes {
    BLT_PAINTBRUSH_SOLID,
    BLT_PAINTBRUSH_TEXTURE,
    BLT_PAINTBRUSH_GRADIENT,
    BLT_PAINTBRUSH_NEWGRADIENT,
    BLT_PAINTBRUSH_TILE,
    BLT_PAINTBRUSH_LINEAR,
    BLT_PAINTBRUSH_RADIAL,
    BLT_PAINTBRUSH_COLOR,
    BLT_PAINTBRUSH_CONICAL,
    BLT_PAINTBRUSH_CHECKERS,
    BLT_PAINTBRUSH_STRIPES
} Blt_PaintBrushType;

#define BLT_PAINTBRUSH_DECREASING      (1<<0)
#define BLT_PAINTBRUSH_VERTICAL        (1<<1)
#define BLT_PAINTBRUSH_HORIZONTAL      (1<<2)
#define BLT_PAINTBRUSH_DIAGONAL        (1<<3)
#define BLT_PAINTBRUSH_SCALING_LINEAR  (1<<4)
#define BLT_PAINTBRUSH_SCALING_LOG     (1<<5)
#define BLT_PAINTBRUSH_REPEAT_NORMAL   (1<<7)
#define BLT_PAINTBRUSH_REPEAT_OPPOSITE (1<<8)

BLT_EXTERN Blt_PaintBrush Blt_NewTileBrush(void);
BLT_EXTERN Blt_PaintBrush Blt_NewLinearGradientBrush(void);
BLT_EXTERN Blt_PaintBrush Blt_NewStripesBrush(void);
BLT_EXTERN Blt_PaintBrush Blt_NewCheckersBrush(void);
BLT_EXTERN Blt_PaintBrush Blt_NewRadialGradientBrush(void);
BLT_EXTERN Blt_PaintBrush Blt_NewConicalGradientBrush(void);
BLT_EXTERN Blt_PaintBrush Blt_NewColorBrush(unsigned int color);

BLT_EXTERN const char *Blt_GetBrushTypeName(Blt_PaintBrush brush);
BLT_EXTERN const char *Blt_GetBrushName(Blt_PaintBrush brush);
BLT_EXTERN const char *Blt_GetBrushColorName(Blt_PaintBrush brush);
BLT_EXTERN Blt_Pixel *Blt_GetBrushPixel(Blt_PaintBrush brush);
BLT_EXTERN Blt_PaintBrushType Blt_GetBrushType(Blt_PaintBrush brush);
BLT_EXTERN XColor *Blt_GetXColorFromBrush(Tk_Window tkwin,
        Blt_PaintBrush brush);
BLT_EXTERN int Blt_ConfigurePaintBrush(Tcl_Interp *interp,
        Blt_PaintBrush brush);
BLT_EXTERN int Blt_GetBrushTypeFromObj(Tcl_Interp *interp,
        Tcl_Obj *objPtr, Blt_PaintBrushType *typePtr);

BLT_EXTERN void Blt_FreeBrush(Blt_PaintBrush brush);
BLT_EXTERN int Blt_GetPaintBrushFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        Blt_PaintBrush *brushPtr);

BLT_EXTERN int Blt_GetPaintBrush(Tcl_Interp *interp, const char *string,
        Blt_PaintBrush *brushPtr);

BLT_EXTERN void Blt_SetLinearGradientBrushPalette(Blt_PaintBrush brush, 
        Blt_Palette palette);
BLT_EXTERN void Blt_SetLinearGradientBrushCalcProc(Blt_PaintBrush brush, 
        Blt_PaintBrushCalcProc *proc, ClientData clientData);
BLT_EXTERN void Blt_SetLinearGradientBrushColors(Blt_PaintBrush brush, 
        Blt_Pixel *lowPtr, Blt_Pixel *highPtr);
BLT_EXTERN void Blt_SetTileBrushPicture(Blt_PaintBrush brush, Blt_Picture tile);
BLT_EXTERN void Blt_SetColorBrushColor(Blt_PaintBrush brush,
        unsigned int value);
BLT_EXTERN void Blt_SetBrushOrigin(Blt_PaintBrush brush, int x, int y);
BLT_EXTERN void Blt_SetBrushOpacity(Blt_PaintBrush brush, double percent);
BLT_EXTERN void Blt_SetBrushArea(Blt_PaintBrush brush, int x, int y, 
        int w,  int h);

BLT_EXTERN int Blt_GetBrushAlpha(Blt_PaintBrush brush);
BLT_EXTERN void Blt_GetBrushOrigin(Blt_PaintBrush brush, int *xPtr, int *yPtr);
BLT_EXTERN unsigned int Blt_GetAssociatedColorFromBrush(Blt_PaintBrush brush, 
        int x, int y);
BLT_EXTERN int Blt_IsVerticalLinearBrush(Blt_PaintBrush brush);
BLT_EXTERN int Blt_IsHorizontalLinearBrush(Blt_PaintBrush brush);

BLT_EXTERN void Blt_PaintRectangle(Blt_Picture picture, int x, int y, int w, 
        int h, int dx, int dy, Blt_PaintBrush brush, int composite);

#ifdef _BLT_INT_H
BLT_EXTERN void Blt_PaintPolygon(Blt_Picture picture, int n, Point2d *vertices,
        Blt_PaintBrush brush);
#endif  /* _BLT_INT_H */

BLT_EXTERN void Blt_CreateBrushNotifier(Blt_PaintBrush brush,
        Blt_BrushChangedProc *notifyProc, ClientData clientData);
BLT_EXTERN void Blt_DeleteBrushNotifier(Blt_PaintBrush brush,
        Blt_BrushChangedProc *notifyProc, ClientData clientData);

#endif /*_BLT_PAINTBRUSH_H*/

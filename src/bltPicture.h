/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPicture.h --
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

#ifndef _BLT_PICTURE_H
#define _BLT_PICTURE_H

#include <bltHash.h>

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Pixel --
 *
 *      A union representing either a pixel as a RGBA quartet or a single
 *      word value.
 *
 *---------------------------------------------------------------------------
 */
typedef union {
    unsigned int u32;           
    struct {
#ifdef WORDS_BIGENDIAN
        unsigned char a, r, g, b;
#else 
        unsigned char b, g, r, a;
#endif    
    } u8;
} Blt_Pixel;

#define Red     u8.r
#define Blue    u8.b
#define Green   u8.g
#define Alpha   u8.a

#define ALPHA_OPAQUE            (0xFF)
#define ALPHA_TRANSPARENT       (0)
#define MAXINTENSITY            (0xFF)

#define GAMMA   (1.0f)

struct _Blt_Ps;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Picture --
 *
 *      The structure below represents a picture.  Each pixel occupies a
 *      32-bit word of memory: one byte for each of the red, green, and
 *      blue color intensities, and another for alpha-channel image
 *      compositing (e.g. transparency).
 *
 *---------------------------------------------------------------------------
 */
struct _Blt_Picture {
    short int width, height;            /* Size of the image in pixels. */
    short int flags;                    /* Flags describing the picture. */
    short int pixelsPerRow;             /* Stride of the image. */
    short int delay;
    short int reserved;
    void *buffer;                       /* Unaligned (malloc'ed) memory for
                                         * pixels. */
    Blt_Pixel *bits;                    /* Array of pixels containing the
                                         * RGBA values. Points into buffer
                                         * array.*/
};

#define BLT_PIC_COLOR  (1<<0)           /* Indicates if color or
                                         * greyscale. */
#define BLT_PIC_BLEND  (1<<1)           /* Picture has partial opaque
                                         * pixels. */
#define BLT_PIC_MASK   (1<<2)           /* Pixels are either 100% opaque or
                                         * transparent. The separate BLEND
                                         * and MASK flags are so that don't
                                         * premultiply alphas for masks. */

#define BLT_PIC_ASSOCIATED_COLORS (1<<3)/* Indicates if RGB components have
                                         * been premultiplied by their
                                         * alphas. */

#define BLT_PIC_DIRTY (1<<4)            /* Indicates that the picture
                                         * contents have changed. Cached
                                         * items may need to be
                                         * refreshed. For example, may need
                                         * to premultiply alphas again. */ 

#define BLT_PIC_UNINITIALIZED (1<<5)    /* Indicates that the contents of
                                         * the picture haven't been
                                         * initialized yet. */

#define BLT_PAINTER_DITHER              (1<<10)

#define BLT_PAINTER_BLEND_MASK          (0x0F)
#define BLT_PAINTER_BLEND_NONE          (0)
#define BLT_PAINTER_BLEND_MIN_ALPHAS    (1<<1)
#define BLT_PAINTER_BLEND_MAX_ALPHAS    (1<<2)
#define BLT_PAINTER_BLEND_DIFF          (1<<3)
#define BLT_PAINTER_BLEND_MULTIPLY      (1<<4)
#define BLT_PAINTER_BLEND_UNDER         (1<<6)

typedef struct _Blt_PictureImage *Blt_PictureImage;
typedef struct _Blt_ResampleFilter *Blt_ResampleFilter;
typedef struct _Blt_ConvolveFilter *Blt_ConvolveFilter;
typedef struct _Blt_Picture *Blt_Picture;

struct _Blt_Chain;

/*
 * Blt_Picture is supposed to be an opaque type.  Use the macros below to
 * access its members.
 */

#define Blt_Picture_Bits(p)      ((p)->bits)
#define Blt_Picture_Flags(p)     ((p)->flags)
#define Blt_Picture_Height(p)    ((p)->height)
#define Blt_Picture_Pixel(p,x,y) ((p)->bits + ((p)->pixelsPerRow * (y)) + (x))
#define Blt_Picture_Width(p)     ((p)->width)
#define Blt_Picture_Stride(p)    ((p)->pixelsPerRow)
#define Blt_Picture_Delay(p)     ((p)->delay)

#define Blt_Picture_IsDirty(p)  ((p)->flags & BLT_PIC_DIRTY)
#define Blt_Picture_IsOpaque(p) \
        (((p)->flags & (BLT_PIC_BLEND | BLT_PIC_MASK)) == 0)
#define Blt_Picture_IsMasked(p)  ((p)->flags &  BLT_PIC_MASK) 
#define Blt_Picture_IsBlended(p) ((p)->flags &  BLT_PIC_BLEND)
#define Blt_Picture_IsColor(p)   ((p)->flags &  BLT_PIC_COLOR)
#define Blt_Picture_IsGreyscale(p)   (!Blt_Picture_IsColor(p))
#define Blt_Picture_IsAssociated(p) ((p)->flags &  BLT_PIC_ASSOCIATED_COLORS)

typedef enum PictureArithOps {
    PIC_ARITH_ADD,
    PIC_ARITH_AND,
    PIC_ARITH_NAND,
    PIC_ARITH_NOR,
    PIC_ARITH_OR,
    PIC_ARITH_RSUB,
    PIC_ARITH_SUB,
    PIC_ARITH_XOR,
    PIC_ARITH_MIN,
    PIC_ARITH_MAX,
} Blt_PictureArithOps;

typedef struct {
    unsigned int x, y;
} PictureCoordinate;

typedef enum BlendingModes {
    BLT_BLEND_COLORBURN,                /* C = (1 - B) / F */
    BLT_BLEND_COLORDODGE,               /* C = B / (1 - F) */ 
    BLT_BLEND_DARKEN,                   /* C = min(F,B) */
    BLT_BLEND_DIFFERENCE,               /* C = |F - B| */
    BLT_BLEND_DIVIDE,
    BLT_BLEND_EXCLUSION,
    BLT_BLEND_HARDLIGHT,
    BLT_BLEND_HARDMIX,
    BLT_BLEND_LIGHTEN,                  /* C = max(F,B) */
    BLT_BLEND_LINEARBURN,
    BLT_BLEND_LINEARDODGE,
    BLT_BLEND_LINEARLIGHT,
    BLT_BLEND_MULTIPLY,                 /* C = F * B */
    BLT_BLEND_NORMAL,                   /* C = F */
    BLT_BLEND_OVERLAY,                  /* C = B*(F + (2*F)*(1-B)) */
    BLT_BLEND_PINLIGHT,
    BLT_BLEND_SCREEN,                   /* C = 1 - (1 - F * B */
    BLT_BLEND_SOFTLIGHT,
    BLT_BLEND_SOFTLIGHT2,
    BLT_BLEND_SUBTRACT,
    BLT_BLEND_VIVIDLIGHT,
} Blt_BlendingMode;

BLT_EXTERN Blt_ResampleFilter bltBoxFilter; /* The ubiquitous box filter */
BLT_EXTERN Blt_ResampleFilter bltMitchellFilter; 
BLT_EXTERN Blt_ResampleFilter bltBellFilter; 
BLT_EXTERN Blt_ResampleFilter bltTentFilter; 

BLT_EXTERN Blt_ResampleFilter bltTableFilter;

typedef struct {
    int x, y, w, h;
} PictRegion;

#define Blt_AddPictures(dest, src) \
    Blt_ApplyPictureToPicture(dest, src, 0, 0, (src)->width, (src)->height, \
        0, 0, PIC_ARITH_ADD)
#define Blt_SubtractPictures(dest, src) \
    Blt_ApplyPictureToPicture(dest, src, 0, 0, (src)->width, (src)->height, \
        0, 0, PIC_ARITH_SUB)
#define Blt_AndPictures(dest, src) \
    Blt_ApplyPictureToPicture(dest, src, 0, 0, (src)->width, (src)->height, \
        0, 0, PIC_ARITH_AND)
#define Blt_OrPictures(dest, src) \
    Blt_ApplyPictureToPicture(dest, src, 0, 0, (src)->width, (src)->height, \
        0, 0, PIC_ARITH_OR)
#define Blt_XorPictures(dest, src) \
    Blt_ApplyPictureToPicture(dest, src, 0, 0, (src)->width, (src)->height, \
        0, 0, PIC_ARITH_XOR)

typedef unsigned int (*Blt_ColorLookupTable)[33][33];

typedef struct {
    double offset, range;
    Blt_Random random;
} Blt_Jitter;

/* Prototypes of picture routines */

BLT_EXTERN void Blt_ApplyPictureToPicture(Blt_Picture dest, Blt_Picture src,
        int x, int y, int w, int h, int dx, int dy, Blt_PictureArithOps op);

BLT_EXTERN void Blt_ApplyScalarToPicture(Blt_Picture dest, Blt_Pixel *colorPtr, 
        Blt_PictureArithOps op);

BLT_EXTERN void Blt_ApplyPictureToPictureWithMask(Blt_Picture dest, 
        Blt_Picture src, Blt_Picture mask, int x, int y, int w, int h, 
        int dx, int dy, int invert, Blt_PictureArithOps op);

BLT_EXTERN void Blt_ApplyScalarToPictureWithMask(Blt_Picture dest, 
        Blt_Pixel *colorPtr, Blt_Picture mask, int invert, 
        Blt_PictureArithOps op);

BLT_EXTERN void Blt_MaskPicture(Blt_Picture dest, Blt_Picture mask, 
        int x, int y, int w, int h, int dx, int dy, Blt_Pixel *colorPtr);

BLT_EXTERN void Blt_BlankPicture(Blt_Picture picture, unsigned int colorValue);
BLT_EXTERN void Blt_BlankRegion(Blt_Picture picture, int x, int y, int w, int h,
        unsigned int colorValue);

BLT_EXTERN void Blt_BlurPicture(Blt_Picture dest, Blt_Picture src, int radius, 
        int numPasses);

BLT_EXTERN void Blt_ResizePicture(Blt_Picture picture, int w, int h);
BLT_EXTERN void Blt_AdjustPictureSize(Blt_Picture picture, int w, int h);

BLT_EXTERN Blt_Picture Blt_ClonePicture(Blt_Picture picture);

BLT_EXTERN void Blt_ConvolvePicture(Blt_Picture dest, Blt_Picture src, 
        Blt_ConvolveFilter vFilter, Blt_ConvolveFilter hFilter);

BLT_EXTERN Blt_Picture Blt_CreatePicture(int w, int h);

BLT_EXTERN Blt_Picture Blt_DitherPicture(Blt_Picture picture, 
        Blt_Pixel *palette);

BLT_EXTERN void Blt_FlipPicture(Blt_Picture picture, int vertically);

BLT_EXTERN void Blt_FreePicture(Blt_Picture picture);

BLT_EXTERN Blt_Picture Blt_GreyscalePicture(Blt_Picture picture);

BLT_EXTERN Blt_Picture Blt_QuantizePicture (Blt_Picture picture, int numColors);

BLT_EXTERN void Blt_ResamplePicture (Blt_Picture dest, Blt_Picture src, 
        Blt_ResampleFilter hFilter, Blt_ResampleFilter vFilter);

BLT_EXTERN Blt_Picture Blt_ScalePicture(Blt_Picture picture, int x, int y, 
        int w, int h, int dw, int dh);

BLT_EXTERN Blt_Picture Blt_ScalePictureArea(Blt_Picture picture, int x, int y,
        int w, int h, int dw, int dh);

BLT_EXTERN Blt_Picture Blt_ReflectPicture(Blt_Picture picture, int side);

BLT_EXTERN Blt_Picture Blt_RotatePictureByShear(Blt_Picture picture, 
        float angle);
BLT_EXTERN Blt_Picture Blt_RotatePicture(Blt_Picture picture, float angle);

BLT_EXTERN Blt_Picture Blt_ProjectPicture(Blt_Picture picture, float *srcPts,
        float *destPts, Blt_Pixel *bg);

BLT_EXTERN void Blt_TilePicture(Blt_Picture dest, Blt_Picture src, int xOrigin, 
        int yOrigin, int x, int y, int w, int h);

BLT_EXTERN int Blt_PictureToPsData(Blt_Picture picture, int numComponents, 
        Tcl_DString *resultPtr, const char *prefix);

BLT_EXTERN void Blt_SelectPixels(Blt_Picture dest, Blt_Picture src, 
        Blt_Pixel *lowerPtr, Blt_Pixel *upperPtr);

BLT_EXTERN int Blt_GetPicture(Tcl_Interp *interp, const char *string, 
        Blt_Picture *picturePtr);

BLT_EXTERN int Blt_GetPictureFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        Blt_Picture *picturePtr);

BLT_EXTERN int Blt_GetResampleFilterFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
        Blt_ResampleFilter *filterPtr);

BLT_EXTERN const char *Blt_NameOfResampleFilter(Blt_ResampleFilter filter);

BLT_EXTERN void Blt_AssociateColor(Blt_Pixel *colorPtr);

BLT_EXTERN void Blt_UnassociateColor(Blt_Pixel *colorPtr);

BLT_EXTERN void Blt_AssociateColors(Blt_Picture picture);

BLT_EXTERN void Blt_UnassociateColors(Blt_Picture picture);

BLT_EXTERN void Blt_MultiplyPixels(Blt_Picture picture, float value);

BLT_EXTERN int Blt_GetBBoxFromObjv(Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv, PictRegion *regionPtr);

BLT_EXTERN int Blt_AdjustRegionToPicture(Blt_Picture picture, 
        PictRegion *regionPtr);

BLT_EXTERN int Blt_GetPixelFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        Blt_Pixel *pixelPtr);

BLT_EXTERN int Blt_GetPixel(Tcl_Interp *interp, const char *string, 
        Blt_Pixel *pixelPtr);

BLT_EXTERN const char *Blt_NameOfPixel(Blt_Pixel *pixelPtr);

BLT_EXTERN void Blt_NotifyImageChanged(Blt_PictureImage image);

BLT_EXTERN int Blt_QueryColors(Blt_Picture picture, Blt_HashTable *tablePtr);

BLT_EXTERN void Blt_ClassifyPicture(Blt_Picture picture);

BLT_EXTERN void Blt_TentHorizontally(Blt_Picture dest, Blt_Picture src);
BLT_EXTERN void Blt_TentVertically(Blt_Picture dest, Blt_Picture src);
BLT_EXTERN void Blt_ZoomHorizontally(Blt_Picture dest, Blt_Picture src, 
        Blt_ResampleFilter filter);
BLT_EXTERN void Blt_ZoomVertically(Blt_Picture dest, Blt_Picture src, 
        Blt_ResampleFilter filter);
BLT_EXTERN void Blt_BlendRegion(Blt_Picture dest, Blt_Picture src, 
        int sx, int sy, int w, int h, int dx, int dy);

BLT_EXTERN void Blt_ColorBlendPictures(Blt_Picture dest, Blt_Picture src, 
        Blt_BlendingMode mode);

BLT_EXTERN void Blt_FadePicture(Blt_Picture picture, int x, int y, int w, int h,
        double factor);

BLT_EXTERN void Blt_CopyPictureBits(Blt_Picture dest, Blt_Picture src, 
        int sx, int sy, int w, int h, int dx, int dy);

BLT_EXTERN void Blt_GammaCorrectPicture(Blt_Picture dest, Blt_Picture src, 
        float gamma);

BLT_EXTERN void Blt_SharpenPicture(Blt_Picture dest, Blt_Picture src);


BLT_EXTERN void Blt_ApplyColorToPicture(Blt_Picture pict, Blt_Pixel *colorPtr);

BLT_EXTERN void Blt_SizeOfPicture(Blt_Picture pict, int *wPtr, int *hPtr);

#ifdef _BLT_DBUFFER_H
BLT_EXTERN Blt_DBuffer Blt_PictureToDBuffer(Blt_Picture picture, int numComp);
#endif  /* _BLT_DBUFFER_H */

BLT_EXTERN int Blt_ResetPicture(Tcl_Interp *interp, const char *imageName, 
        Blt_Picture picture);

BLT_EXTERN void Blt_MapColors(Blt_Picture dest, Blt_Picture src, 
        Blt_ColorLookupTable clut);

BLT_EXTERN Blt_ColorLookupTable Blt_GetColorLookupTable(
        struct _Blt_Chain *chainPtr, int numReqColors);

BLT_EXTERN void Blt_FadePictureWithGradient(Blt_Picture picture, int side,
        double low, double high, int scale, Blt_Jitter *jitterPtr);

BLT_EXTERN Blt_Picture Blt_ReflectPicture2(Blt_Picture picture, int side);

BLT_EXTERN void Blt_SubtractColor(Blt_Picture picture, Blt_Pixel *colorPtr);

#ifdef _TK
BLT_EXTERN Blt_Picture Blt_PhotoToPicture (Tk_PhotoHandle photo);
BLT_EXTERN Blt_Picture Blt_PhotoAreaToPicture (Tk_PhotoHandle photo, 
        int x, int y, int w, int h);
BLT_EXTERN Blt_Picture Blt_DrawableToPicture(Tk_Window tkwin, 
        Drawable drawable, int x, int y, int w, int h, float gamma);
BLT_EXTERN Blt_Picture Blt_WindowToPicture(Display *display, 
        Drawable drawable, int x, int y, int w, int h, float gamma);
BLT_EXTERN void Blt_PictureToPhoto(Blt_Picture picture, Tk_PhotoHandle photo);
BLT_EXTERN int Blt_SnapPhoto(Tcl_Interp *interp, Tk_Window tkwin, 
        Drawable drawable, int sx, int sy, int w, int h, int dw, int dh, 
        const char *photoName, float gamma);
BLT_EXTERN int Blt_SnapPicture(Tcl_Interp *interp, Tk_Window tkwin, 
        Drawable drawable, int sx, int sy, int w, int h, int dw, int dh, 
        const char *imageName, float gamma);
BLT_EXTERN unsigned int Blt_XColorToPixel(XColor *colorPtr);
BLT_EXTERN int Blt_IsPicture(Tk_Image tkImage);
BLT_EXTERN Blt_Picture Blt_GetPictureFromImage(Tcl_Interp *interp, 
        Tk_Image tkImage, int *isPhotoPtr);
BLT_EXTERN Blt_Picture Blt_GetPictureFromPictureImage(Tcl_Interp *interp,
        Tk_Image tkImage);
BLT_EXTERN struct _Blt_Chain *Blt_GetPicturesFromPictureImage(
        Tcl_Interp *interp, Tk_Image tkImage);
BLT_EXTERN Blt_Picture Blt_GetPictureFromPhotoImage(Tcl_Interp *interp,
        Tk_Image tkImage);
BLT_EXTERN Blt_Picture Blt_CanvasToPicture(Tcl_Interp *interp, Tk_Window tkwin,
        float gamma);
BLT_EXTERN Blt_Picture Blt_GraphToPicture(Tcl_Interp *interp, Tk_Window tkwin,
        float gamma);
#endif  /* _TK */

BLT_EXTERN int Blt_PictureRegisterProc(Tcl_Interp *interp, const char *name,
        Tcl_ObjCmdProc *proc);

typedef struct {
    Blt_Pixel color;
    int offset;                         /* Offset of shadow. */
    int width;                          /* Blur width. */
} Blt_Shadow;

BLT_EXTERN void Blt_Shadow_Set(Blt_Shadow *sPtr, int width, int offset, 
        int color, int alpha);

BLT_EXTERN Blt_Picture Blt_EmbossPicture(Blt_Picture picture, double azimuth, 
        double elevation, unsigned short width45);
BLT_EXTERN void Blt_FadeColor(Blt_Pixel *colorPtr, unsigned int alpha);

BLT_EXTERN int Blt_Dissolve2(Blt_Picture dest, Blt_Picture src, long start,
        int numSteps);
BLT_EXTERN void Blt_CrossFade(Blt_Picture dest, Blt_Picture from,
        Blt_Picture to, double opacity);
BLT_EXTERN void Blt_FadeFromColor(Blt_Picture dest, Blt_Picture to,
        Blt_Pixel *colorPtr, double opacity);
BLT_EXTERN void Blt_FadeToColor(Blt_Picture dest, Blt_Picture from,
        Blt_Pixel *colorPtr, double opacity);
BLT_EXTERN void Blt_WipePictures(Blt_Picture dest, Blt_Picture from,
        Blt_Picture to, int orientation, double position);


#endif /*_BLT_PICTURE_H*/

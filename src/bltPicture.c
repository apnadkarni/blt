/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPicture.c --
 *
 * This module implements basic picture processing routines for the BLT
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
 * RotateByAreaMapping routine is from Leptonia. 
 *
 * Copyright (C) 2001 Leptonica.  All rights reserved.  
 *
 *   This software is distributed in the hope that it will be useful, but
 *   with NO WARRANTY OF ANY KIND.
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include <limits.h>

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltHash.h"
#include "bltChain.h"
#include <bltDBuffer.h>
#include "bltPicture.h"
#include "bltPictInt.h"
#include "bltImage.h"
#include "bltRound.h"

#ifdef HAVE_MALLOC_H
  #include <malloc.h>
#endif /* HAVE_MALLOC_H */

#define ALIGNMENT       16              /* # of bytes alignment for picture
                                         * rows. */

#define JCLAMP(c)       ((((c) < 0.0) ? 0.0 : ((c) > 1.0) ? 1.0 : (c)))

#define CLAMP(c)        ((((c) < 0.0) ? 0.0 : ((c) > 255.0) ? 255.0 : (c)))

#define imul8x8(a,b,t) ((t) = (a)*(b)+128,(((t)+((t)>>8))>>8))

static Blt_ResampleFilterProc DefaultFilter;
static Blt_ResampleFilterProc BellFilter;
static Blt_ResampleFilterProc BesselFilter;
static Blt_ResampleFilterProc BoxFilter;
static Blt_ResampleFilterProc BSplineFilter;
static Blt_ResampleFilterProc CatRomFilter;
static Blt_ResampleFilterProc DummyFilter;
static Blt_ResampleFilterProc GaussianFilter;
static Blt_ResampleFilterProc GiFilter;
static Blt_ResampleFilterProc Lanczos3Filter;
static Blt_ResampleFilterProc MitchellFilter;
static Blt_ResampleFilterProc SincFilter;
static Blt_ResampleFilterProc TriangleFilter;

static Blt_ApplyPictureToPictureProc ApplyPictureToPicture;
static Blt_ApplyScalarToPictureProc  ApplyScalarToPicture;
static Blt_ApplyPictureToPictureWithMaskProc ApplyPictureToPictureWithMask;
static Blt_ApplyScalarToPictureWithMaskProc  ApplyScalarToPictureWithMask;
static Blt_TentHorizontallyProc TentHorizontally;
static Blt_TentVerticallyProc TentVertically;
static Blt_ZoomHorizontallyProc ZoomHorizontally;
static Blt_ZoomVerticallyProc ZoomVertically;
static Blt_CompositeAreaProc CompositeArea;
static Blt_CompositePicturesProc CompositePictures;
static Blt_CrossFadePicturesProc CrossFadePictures;
static Blt_SelectPixelsProc SelectPixels;
static Blt_PremultiplyColorsProc PremultiplyColors;
static Blt_UnmultiplyColorsProc UnassociateColors;
static Blt_CopyAreaProc CopyArea;
static Blt_CopyPictureBitsProc CopyPictureBits;
static Blt_BlankPictureProc BlankPicture;

static Blt_PictureProcs stdPictureProcs = {
    ApplyPictureToPicture,
    ApplyScalarToPicture,
    ApplyPictureToPictureWithMask,
    ApplyScalarToPictureWithMask,
    TentHorizontally,
    TentVertically,
    ZoomHorizontally,
    ZoomVertically,
    CompositeArea,
    CompositePictures,
    SelectPixels,
    PremultiplyColors,
    UnassociateColors,
    CopyArea,
    CopyPictureBits,
    CrossFadePictures,
    BlankPicture
};

Blt_PictureProcs *bltPictProcsPtr = &stdPictureProcs;

void 
Blt_ApplyPictureToPicture(Blt_Picture dest, Blt_Picture src, 
                          int x, int y, int w, int h, int dx, int dy,
                          Blt_PictureArithOps op)
{
    (*bltPictProcsPtr->applyPictureToPictureProc)(dest, src, x, y, w, h, 
        dx, dy, op);
}

void 
Blt_ApplyScalarToPicture(Blt_Picture dest, Blt_Pixel *colorPtr,
                         Blt_PictureArithOps op)
{
    (*bltPictProcsPtr->applyScalarToPictureProc)(dest, colorPtr, op);
}

void
Blt_ApplyPictureToPictureWithMask(Blt_Picture dest, Blt_Picture src, 
                                  Blt_Picture mask, int x, int y, int w, int h,
                                  int dx, int dy, int invert, 
                                  Blt_PictureArithOps op)
{
    (*bltPictProcsPtr->applyPictureToPictureWithMaskProc)(dest, src, mask,
        x, y, w, h, dx, dy, invert, op);
}

void
Blt_ApplyScalarToPictureWithMask(Blt_Picture dest, Blt_Pixel *colorPtr,
                                 Blt_Picture mask, int invert, 
                                 Blt_PictureArithOps op)
{
    (*bltPictProcsPtr->applyScalarToPictureWithMaskProc)(dest, colorPtr, 
        mask, invert, op);
}

void 
Blt_TentHorizontally(Blt_Picture dest, Blt_Picture src)
{
    (*bltPictProcsPtr->tentHorizontallyProc)(dest, src);
}

void 
Blt_TentVertically(Blt_Picture dest, Blt_Picture src)
{
    (*bltPictProcsPtr->tentVerticallyProc)(dest, src);
}

void 
Blt_ZoomHorizontally(Blt_Picture dest, Blt_Picture src, 
                     Blt_ResampleFilter filter)
{
    (*bltPictProcsPtr->zoomHorizontallyProc)(dest, src, filter);
}

void 
Blt_ZoomVertically(Blt_Picture dest, Blt_Picture src, Blt_ResampleFilter filter)
{
    (*bltPictProcsPtr->zoomVerticallyProc)(dest, src, filter);
}

void 
Blt_CompositeArea(Blt_Picture dest, Blt_Picture src, int x, int y, int w,
                    int h, int dx, int dy)
{
    (*bltPictProcsPtr->compositeAreaProc)(dest, src, x, y, w, h, dx, dy);
}

void 
Blt_CompositePictures(Blt_Picture dest, Blt_Picture src)
{
    (*bltPictProcsPtr->compositePicturesProc)(dest, src);
}

void
Blt_CrossFadePictures(Blt_Picture dest, Blt_Picture from, Blt_Picture to,
                      double opacity) 
{
    (*bltPictProcsPtr->crossFadePicturesProc)(dest, from, to, opacity);
}

void
Blt_BlankPicture(Blt_Picture dest, unsigned int value) 
{
    (*bltPictProcsPtr->blankPictureProc)(dest, value);
}

void 
Blt_SelectPixels(Blt_Picture dest, Blt_Picture src, Blt_Pixel *lowPtr, 
                 Blt_Pixel *highPtr)
{
    (*bltPictProcsPtr->selectPixelsProc)(dest, src, lowPtr, highPtr);
}

void 
Blt_PremultiplyColors(Blt_Picture picture)
{
    (*bltPictProcsPtr->premultiplyColorsProc)(picture);
}

void 
Blt_UnmultiplyColors(Blt_Picture picture)
{
    (*bltPictProcsPtr->unassociateColorsProc)(picture);
}

void 
Blt_CopyArea(Blt_Picture dest, Blt_Picture src, int x, int y, int w, 
                    int h, int dx, int dy)
{
    (*bltPictProcsPtr->copyAreaProc)(dest, src, x, y, w, h, dx, dy);
}

void 
Blt_CopyPictureBits(Blt_Picture dest, Blt_Picture src)
{
    (*bltPictProcsPtr->copyPictureBitsProc)(dest, src);
}

/* 
 * Quick and dirty random number generator. 
 *
 * http://www.shadlen.org/ichbin/random/generators.htm#quick 
 */
#define JITTER_SEED     31337
#define JITTER_A        1099087573U
#define RANDOM_SCALE    2.3283064370807974e-10

#ifdef notdef
static INLINE void 
RandomSeed(Blt_Random *randomPtr, unsigned int seed) {
    randomPtr->value = seed;
}

static INLINE void
RandomInit(Blt_Random *randomPtr) 
{
    RandomSeed(randomPtr, JITTER_SEED);
}
#endif

static INLINE double
RandomNumber(Blt_Random *randomPtr)
{
#if (SIZEOF_INT == 8) 
    /* mask the lower 32 bits on machines where int is a 64-bit quantity */
    randomPtr->value = ((1099087573  * (randomPtr->value))) & ((unsigned int) 0xffffffff);
#else
    /* on machines where int is 32-bits, no need to mask */
    randomPtr->value = (JITTER_A  * randomPtr->value);
#endif  /* SIZEOF_INT == 8 */
    return (double)randomPtr->value * RANDOM_SCALE;
}

#ifdef notdef
static INLINE void
JitterInit(Blt_Jitter *jitterPtr) 
{
    RandomInit(&jitterPtr->random);
    jitterPtr->range = 0.1;
    jitterPtr->offset = -0.05;          /* Jitter +/-  */
}
#endif

static INLINE double 
Jitter(Blt_Jitter *jitterPtr) {
    double value;

    value = RandomNumber(&jitterPtr->random);
    return (value * jitterPtr->range) + jitterPtr->offset;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreatePicture --
 *
 *      Allocates a picture of a designated height and width.
 *
 *      This routine will be augmented with other types of information such
 *      as a color table, etc.
 *
 * Results:
 *      Returns the new color pict.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture
Blt_CreatePicture(int w, int h)
{
    Pict *destPtr;
    int pixelsPerRow, numRows;
    size_t numBytes;
    unsigned char *buffer;
    ptrdiff_t ptr;

    assert((w > 0) && (w <= SHRT_MAX));
    assert((h > 0) && (h <= SHRT_MAX));

    destPtr = Blt_AssertMalloc(sizeof(Pict));
    destPtr->width  = w;
    destPtr->height = h;
    destPtr->flags  = BLT_PIC_UNINITIALIZED;
    destPtr->delay = 0;

    /* 
     * Be careful. There's a bunch of picture routines that assume an even
     * number of pixels per row. 
     */
    pixelsPerRow = (w + 3) & ~3;        /* Pad the row so that the row size
                                         * is a 4-pixel multiple.  This is
                                         * for SIMD routines: ensures data
                                         * is quad aligned and lets us
                                         * process 4 pixels at a time. */
    numRows = ((h + 3) / 4) * 4;        /* Make # of rows multiple of 4. */

    /* Over-allocate a buffer so that we can align it (if needed) to a
     * 16-byte boundary. */
    numBytes = (pixelsPerRow * numRows * sizeof(Blt_Pixel)) + ALIGNMENT;
    buffer = Blt_AssertCalloc(1, numBytes); /* All zeros. */
    ptr = (ptrdiff_t)buffer;
    destPtr->bits = (Blt_Pixel *)(ptr + (ptr & (ALIGNMENT-1)));
    destPtr->buffer = buffer;
    destPtr->pixelsPerRow = pixelsPerRow;
    return destPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FreePicture --
 *
 *      Deallocates the given picture.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_FreePicture(Pict *pictPtr)
{
    Blt_Free(pictPtr->buffer);
    Blt_Free(pictPtr);
}

/* 
 *---------------------------------------------------------------------------
 *
 * Blt_ClonePicture --
 *
 *      Creates a copy of the given picture.  
 *
 * Results:  
 *      Returns the new copy.
 *
 * -------------------------------------------------------------------------- 
 */
Blt_Picture
Blt_ClonePicture(Pict *srcPtr)
{
    Pict *destPtr;

    destPtr = Blt_CreatePicture(srcPtr->width, srcPtr->height);
    Blt_CopyPictureBits(destPtr, srcPtr);
    destPtr->delay = srcPtr->delay;
    return destPtr;
}

void
Blt_SizeOfPicture(Pict *pictPtr, int *widthPtr , int *heightPtr) 
{
    *widthPtr = pictPtr->width;
    *heightPtr = pictPtr->height;
}

void
Blt_ResizePicture(Pict *srcPtr, int w, int h)   
{
    assert((w > 0) && (w <= SHRT_MAX));
    assert((h > 0) && (h <= SHRT_MAX));

    if ((w != srcPtr->width) || (h != srcPtr->height)) {
        int pixelsPerRow, numRows;
        size_t numBytes;
        ptrdiff_t ptr;
        void *buffer;

        pixelsPerRow = (w + 3) & ~3;    /* Align each row on a 16-byte
                                         * boundary. */
        numRows = ((h + 3) / 4) * 4;    /* Make # of rows multiple of 4. */
        numBytes = (pixelsPerRow * numRows * sizeof(Blt_Pixel)) + ALIGNMENT;
        buffer = Blt_Realloc(srcPtr->buffer, numBytes);
        assert(buffer != NULL);
        srcPtr->buffer = buffer;
        ptr = (ptrdiff_t)buffer;
        srcPtr->bits = (Blt_Pixel *)(ptr + (ptr & (ALIGNMENT-1)));
        srcPtr->width = w;
        srcPtr->height = h;
        srcPtr->pixelsPerRow = pixelsPerRow;
        srcPtr->flags = BLT_PIC_DIRTY;
    }
}

void
Blt_AdjustPictureSize(Pict *srcPtr, int w, int h)       
{
    assert((w > 0) && (w <= SHRT_MAX));
    assert((h > 0) && (h <= SHRT_MAX));

    if ((w != srcPtr->width) || (h != srcPtr->height)) {
        int pixelsPerRow, numRows, bytesPerRow;
        size_t numBytes;
        void *buffer;
        ptrdiff_t ptr;
        Blt_Pixel *bits;

        /* Reallocate the buffer to the new size. */
        pixelsPerRow = (w + 3) & ~3;    /* Align each row on a 16-byte
                                         * boundary. */
        numRows = ((h + 3) / 4) * 4;    /* Make # of rows multiple of 4. */
        
        numBytes = (pixelsPerRow * numRows * sizeof(Blt_Pixel)) + ALIGNMENT;
        buffer = Blt_AssertMalloc(numBytes);

        ptr = (ptrdiff_t)buffer;
        bits = (Blt_Pixel *)(ptr + (ptr & (ALIGNMENT-1)));

        if (srcPtr->bits != NULL && (srcPtr->pixelsPerRow > 0)) {
            Blt_Pixel *srcRowPtr, *destRowPtr;
            int y, numRows;

            bytesPerRow = sizeof(Blt_Pixel) * 
                MIN(pixelsPerRow, srcPtr->pixelsPerRow);
            numRows = MIN(srcPtr->height, h);
            srcRowPtr = srcPtr->bits, destRowPtr = bits;
            for (y = 0; y < numRows; y++) {
                memcpy(destRowPtr, srcRowPtr, bytesPerRow);
                destRowPtr += pixelsPerRow;
                srcRowPtr  += srcPtr->pixelsPerRow;
            }
            Blt_Free(srcPtr->buffer);
        }
        srcPtr->pixelsPerRow = pixelsPerRow;
        srcPtr->width = w;
        srcPtr->height = h;
        srcPtr->bits = bits;
        srcPtr->buffer = buffer;
        srcPtr->flags = BLT_PIC_DIRTY;
    }
}


static void
BlankPicture(Pict *destPtr, unsigned int value) 
{
    Blt_Pixel *dp, *dend, color;
    size_t numPixels;
    
    color.u32 = value;
    Blt_PremultiplyColor(&color);
    numPixels = destPtr->height * destPtr->pixelsPerRow;
    for (dp = destPtr->bits, dend = dp + numPixels; dp < dend; dp++) {
        dp->u32 = color.u32;
    }
    destPtr->flags |= BLT_PIC_DIRTY;
    destPtr->flags &= ~(BLT_PIC_COMPOSITE | BLT_PIC_MASK);
    if (color.Alpha == 0x00) {
        destPtr->flags |= BLT_PIC_MASK | BLT_PIC_COMPOSITE;
    } else if (color.Alpha != 0xFF) {
        destPtr->flags |= BLT_PIC_COMPOSITE;
    }
    destPtr->flags |= BLT_PIC_PREMULT_COLORS;
}

void
Blt_BlankArea(Pict *destPtr, int x, int y, int w, int h, 
                unsigned int colorValue) 
{
    Blt_Pixel *destRowPtr;
    int y1, x1;
    Blt_Pixel color;

    assert ((x >= 0) && (y >= 0));
    if ((x >= destPtr->width) || (y >= destPtr->height)) {
        return;
    }
    x1 = x + w;
    y1 = y + h;
    if (x1 > destPtr->width) {
        x1 = destPtr->width;
    }
    if (y1 > destPtr->height) {
        y1 = destPtr->height;
    }
    w = x1 - x;
    h = y1 - y;
    destRowPtr = destPtr->bits + (y * destPtr->pixelsPerRow) + x;
    for (y = 0; y < h; y++) {
        int n;
        Blt_Pixel *dp;

        dp = destRowPtr;
        n = (w + 7) / 8;                /* width > 0 assumed */
        switch (w & 0x7) {
        case 0:        do {  dp->u32 = colorValue, dp++;
        case 7:              dp->u32 = colorValue, dp++;
        case 6:              dp->u32 = colorValue, dp++;
        case 5:              dp->u32 = colorValue, dp++;
        case 4:              dp->u32 = colorValue, dp++;
        case 3:              dp->u32 = colorValue, dp++;
        case 2:              dp->u32 = colorValue, dp++;
        case 1:              dp->u32 = colorValue, dp++;
                        } while (--n > 0);
        }
        destRowPtr += destPtr->pixelsPerRow;
    }
    destPtr->flags |= BLT_PIC_DIRTY;
    destPtr->flags &= ~(BLT_PIC_COMPOSITE | BLT_PIC_MASK);
    color.u32 = colorValue;
    if (color.Alpha == 0x00) {
        destPtr->flags |= BLT_PIC_MASK;
    } else if (color.Alpha != 0xFF) {
        destPtr->flags |= BLT_PIC_COMPOSITE;
    }
}

#ifdef notdef
static void
ComputeGammaTable(float gamma, unsigned char lut[])
{
    int i;
    double iGamma;

    iGamma = 1.0 / (double)gamma;
    for (i = 0; i < 256; i++) {
        double value;

        value = 255.0 * pow((double)i / 255.0, iGamma);
        lut[i] = (unsigned char)CLAMP(value);
    }
}
#endif

static void
ComputeGammaTable2(float gamma, unsigned char lut[])
{
    int i;
    double iGamma;
    double cutoff, exp;

    iGamma = 1.0 / (double)gamma;
    cutoff = 0.018;
    exp = (1.099 * pow(cutoff, iGamma) - 0.099) / cutoff;
    for (i = 0; i < 256; i++) {
        double value, intensity;

        intensity = (double)i / 255.0;
        if (intensity < cutoff) {
            intensity *= exp;
        } else {
            intensity = (1.099 * pow(intensity, iGamma) - 0.099);
        }
        value = 255.0 * intensity;
        lut[i] = (unsigned char)CLAMP(value);
    }
}

void
Blt_GammaCorrectPicture(Pict *destPtr, Pict *srcPtr, float gamma)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;
    unsigned char lut[256];
    
    ComputeGammaTable2(gamma, lut);
    srcRowPtr = srcPtr->bits;
    destRowPtr = destPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *dp, *send;

        sp = srcRowPtr, dp = destRowPtr;
        for (send = sp + srcPtr->width; sp < send; sp++, dp++) {
            dp->Red = lut[(unsigned int)sp->Red];
            dp->Green = lut[(unsigned int)sp->Green];
            dp->Blue = lut[(unsigned int)sp->Blue];
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GreyscalePicture --
 *
 *      Returns a new picture converted to grey scale.  All three picture
 *      color components will have the same value.
 *
 *      Luminosity is computed using the formula,
 *
 *          Y = 0.212671 * Red + 0.715160 * Green + 0.072169 * Blue
 *
 *      which is supposed to better represent contemporary monitors.
 *
 * Results:
 *      Returns a new greyscale picture.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture 
Blt_GreyscalePicture(Pict *srcPtr)
{
    Pict *destPtr;
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    /*
     * We can use scaled integers (20-bit fraction) to compute the
     * luminosity with reasonable accuracy considering it's stored in an
     * 8-bit result.
     */
#define YR      223002          /* 0.212671 */
#define YG      749900          /* 0.715160 */
#define YB      75675           /* 0.072169 */
#define YMAX    267386880       /* 255.0 */
#define YCLAMP(s) \
        (unsigned char)((s) > YMAX) ? 255 : ((((s) + 524288) >> 20))

    destPtr = Blt_CreatePicture(srcPtr->width, srcPtr->height);
    srcRowPtr = srcPtr->bits;
    destRowPtr = destPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *dp, *send;

        sp = srcRowPtr, dp = destRowPtr;
        for (send = sp + srcPtr->width; sp < send; sp++, dp++) {
            unsigned int Y;

            Y = YR * sp->Red + YG * sp->Green + YB * sp->Blue;
            dp->Red = dp->Green = dp->Blue = YCLAMP(Y);
            dp->Alpha = sp->Alpha;
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
    destPtr->flags = srcPtr->flags;
    destPtr->flags |= BLT_PIC_DIRTY | BLT_PIC_GREYSCALE;
    return destPtr;
}

void 
Blt_FadePicture(Pict *srcPtr, int x, int y, int w, int h, double factor)
{
    Blt_Pixel *srcRowPtr;
    int alpha;

    if ((srcPtr->flags & BLT_PIC_PREMULT_COLORS) == 0) {
        Blt_PremultiplyColors(srcPtr);
    }
    alpha = (int)((1.0 - factor) * 255.0 + 0.5);
    assert(alpha >= 0 && alpha <= 0xff);
    srcRowPtr = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *send;

        for (sp = srcRowPtr, send = sp + w; sp < send; sp++) {
            int t;

            sp->Red   = imul8x8(sp->Red,   alpha, t);
            sp->Green = imul8x8(sp->Green, alpha, t);
            sp->Blue  = imul8x8(sp->Blue,  alpha, t);
            sp->Alpha = imul8x8(sp->Alpha, alpha, t);
        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    srcPtr->flags |= BLT_PIC_COMPOSITE;
}

void
Blt_FadeColor(Blt_Pixel *colorPtr, unsigned int alpha)
{
    int t;

    colorPtr->Red   = imul8x8(colorPtr->Red, alpha, t);
    colorPtr->Green = imul8x8(colorPtr->Green, alpha, t);
    colorPtr->Blue  = imul8x8(colorPtr->Blue, alpha, t);
    colorPtr->Alpha = imul8x8(colorPtr->Alpha, alpha, t);
}

void
Blt_PremultiplyColor(Blt_Pixel *colorPtr)
{
    if (colorPtr->Alpha != 0xFF) {
        int t;
                    
        colorPtr->Red   = imul8x8(colorPtr->Alpha, colorPtr->Red, t);
        colorPtr->Green = imul8x8(colorPtr->Alpha, colorPtr->Green, t);
        colorPtr->Blue  = imul8x8(colorPtr->Alpha, colorPtr->Blue, t);
    }
}

void
Blt_UnmultiplyColor(Blt_Pixel *colorPtr)
{
    /* No conversion necessary if 100% transparent or opaque. */
    if ((colorPtr->Alpha != 0xFF) && (colorPtr->Alpha != 0x00)) {
        int bias;
        
        bias = colorPtr->Alpha >> 1;
        colorPtr->Red   = (mul255(colorPtr->Red)   + bias) / colorPtr->Alpha;
        colorPtr->Green = (mul255(colorPtr->Green) + bias) / colorPtr->Alpha;
        colorPtr->Blue  = (mul255(colorPtr->Blue)  + bias) / colorPtr->Alpha;
    }
}

static void
PremultiplyColors(Pict *srcPtr)
{
    Blt_Pixel *srcRowPtr;
    int y;

    if (srcPtr->flags & BLT_PIC_PREMULT_COLORS) {
        return;
    }
    srcPtr->flags |= BLT_PIC_PREMULT_COLORS;
    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;
        
        sp = srcRowPtr;
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            if (sp->Alpha != 0xFF) {
                int t;
                    
                sp->Red   = imul8x8(sp->Alpha, sp->Red, t);
                sp->Green = imul8x8(sp->Alpha, sp->Green, t);
                sp->Blue  = imul8x8(sp->Alpha, sp->Blue, t);
            }
        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
}

#ifdef notdef
void
Blt_DumpPictures(Pict *srcPtr, Pict *destPtr)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    srcRowPtr = srcPtr->bits;
    destRowPtr = destPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send, *dp;
        
        sp = srcRowPtr;
        dp = destRowPtr;
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            fprintf(stderr, "%d,%d old = 0x%02x%02x%02x%02x new = 0x%02x%02x%02x%02x\n",
                    y, (int)(sp - srcRowPtr),
                    sp->Alpha, sp->Red, sp->Green, sp->Blue, 
                    dp->Alpha, dp->Red, dp->Green, dp->Blue);
            dp++;
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
}
#endif

static void
UnassociateColors(Pict *srcPtr)
{
    if (srcPtr->flags & BLT_PIC_PREMULT_COLORS) {
        Blt_Pixel *srcRowPtr;
        int y;

        srcRowPtr = srcPtr->bits;
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *send;
            
            sp = srcRowPtr;
            for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
                Blt_UnmultiplyColor(sp);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
        }
        srcPtr->flags &= ~BLT_PIC_PREMULT_COLORS;
    }
}

/*
 * CompositeArea --
 *
 */  
static void
CompositeArea(Pict *destPtr, Pict *srcPtr, int sx, int sy, int w, int h,
                int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    if (((dx + w) < 0) || ((dy + h) < 0)) {
        return;
    }
    if (dx < 0) {
        w += dx;
        sx -= dx;
        dx = 0;
    }
    if (dy < 0) {
        h += dy;
        sy -= dy;
        dy = 0;
    }
    if (destPtr->width < (dx + w)) {
        w = destPtr->width - dx;
    }
    if (destPtr->height < (dy + h)) {
        h = destPtr->height - dy;
    }
    if (srcPtr->width < w) {
        w = srcPtr->width;
    }
    if (srcPtr->height < h) {
        h = srcPtr->height;
    }
    /* Convert the pictures to use associated colors if not already. */
    if ((srcPtr->flags & BLT_PIC_PREMULT_COLORS) == 0) {
        Blt_PremultiplyColors(srcPtr);
    }
    if ((destPtr->flags & BLT_PIC_PREMULT_COLORS) == 0) {
        Blt_PremultiplyColors(destPtr);
    }
    
    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((sy * srcPtr->pixelsPerRow) + sx);

    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *dend;

        sp = srcRowPtr;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp++, sp++) {
            /* Blend the foreground and background together. */
            if ((dp->Alpha == 0x0) || (sp->Alpha == 0xFF)) {
                dp->u32 = sp->u32;
            } else if (sp->Alpha != 0x00) {
                int alpha, beta, t;
                int r, g, b, a;

                alpha = sp->Alpha;
                beta = alpha ^ 0xFF; /* beta = 1 - alpha */
                r = sp->Red   + imul8x8(beta, dp->Red, t);
                g = sp->Green + imul8x8(beta, dp->Green,t);
                b = sp->Blue  + imul8x8(beta, dp->Blue, t);
                a = alpha     + imul8x8(beta, dp->Alpha, t);
                dp->Red   = UCLAMP(r);
                dp->Green = UCLAMP(g);
                dp->Blue  = UCLAMP(b);
                dp->Alpha = UCLAMP(a);
            }
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
}

static void
CompositePictures(Pict *destPtr, Pict *srcPtr)
{
    CompositeArea(destPtr, srcPtr, 0, 0, srcPtr->width, srcPtr->height, 0, 0);
}


static INLINE int
ColorBurn(int src, int dst)
{
    int c;
    
    if (src == 0) {
        return src;
    }
    c = 255 - (((255 - dst) << 8) / src);
    if (c < 0) {
        return 0;
    }
    return c;
}

/* 
 * r = dst / (1 - src);
 *
 *      dst/255.0 / (1 - src/255.0);
 *
 *      dst / (255 - src)
 */
static INLINE int
ColorDodge(int src, int dst)
{
    int c;
    
    if (src == 0xFF) {
        return src;
    }
    c = (dst * 255) / (255 - src);
    if (c > 0xFF) {
        return 0xFF;
    }
    return c;
}

#ifdef notdef
#define ChannelBlend_Normal(A,B)     ((uint8)(A))
#define ChannelBlend_Lighten(A,B)    ((uint8)((B > A) ? B:A))
#define ChannelBlend_Darken(A,B)     ((uint8)((B > A) ? A:B))
#define ChannelBlend_Multiply(A,B)   ((uint8)((A * B) / 255))
#define ChannelBlend_Average(A,B)    ((uint8)((A + B) / 2))
#define ChannelBlend_Add(A,B)        ((uint8)(min(255, (A + B))))
#define ChannelBlend_Subtract(A,B)   ((uint8)((A + B < 255) ? 0:(A + B - 255)))
#define ChannelBlend_Difference(A,B) ((uint8)(abs(A - B)))
#define ChannelBlend_Negation(A,B)   ((uint8)(255 - abs(255 - A - B)))
#define ChannelBlend_Screen(A,B)     ((uint8)(255 - (((255 - A) * (255 - B)) >> 8)))
#define ChannelBlend_Exclusion(A,B)  ((uint8)(A + B - 2 * A * B / 255))
#define ChannelBlend_Overlay(A,B)    ((uint8)((B < 128) ? (2 * A * B / 255):(255 - 2 * (255 - A) * (255 - B) / 255)))
#define ChannelBlend_SoftLight(A,B)  ((uint8)((B < 128)?(2*((A>>1)+64))*((float)B/255):(255-(2*(255-((A>>1)+64))*(float)(255-B)/255))))
#define ChannelBlend_HardLight(A,B)  (ChannelBlend_Overlay(B,A))
#define ChannelBlend_ColorDodge(A,B) ((uint8)((B == 255) ? B:min(255, ((A << 8 ) / (255 - B)))))
#define ChannelBlend_ColorBurn(A,B)  ((uint8)((B == 0) ? B:max(0, (255 - ((255 - A) << 8 ) / B))))
#define ChannelBlend_LinearDodge(A,B)(ChannelBlend_Add(A,B))
#define ChannelBlend_LinearBurn(A,B) (ChannelBlend_Subtract(A,B))
#define ChannelBlend_LinearLight(A,B)((uint8)(B < 128)?ChannelBlend_LinearBurn(A,(2 * B)):ChannelBlend_LinearDodge(A,(2 * (B - 128))))
#define ChannelBlend_VividLight(A,B) ((uint8)(B < 128)?ChannelBlend_ColorBurn(A,(2 * B)):ChannelBlend_ColorDodge(A,(2 * (B - 128))))
#define ChannelBlend_PinLight(A,B)   ((uint8)(B < 128)?ChannelBlend_Darken(A,(2 * B)):ChannelBlend_Lighten(A,(2 * (B - 128))))
#define ChannelBlend_HardMix(A,B)    ((uint8)((ChannelBlend_VividLight(A,B) < 128) ? 0:255))
#define ChannelBlend_Reflect(A,B)    ((uint8)((B == 255) ? B:min(255, (A * A / (255 - B)))))
#define ChannelBlend_Glow(A,B)       (ChannelBlend_Reflect(B,A))
#define ChannelBlend_Phoenix(A,B)    ((uint8)(min(A,B) - max(A,B) + 255))
#define ChannelBlend_Alpha(A,B,O)    ((uint8)(O * A + (1 - O) * B))
#define ChannelBlend_AlphaF(A,B,F,O) (ChannelBlend_Alpha(F(A,B),A,O))

#define ColorBlend_Buffer(T,A,B,M)      (T)[0] = ChannelBlend_##M((A)[0], (B)[0]),                                        (T)[1] = ChannelBlend_##M((A)[1], (B)[1]),
                                        (T)[2] = ChannelBlend_##M((A)[2], (B)[2])

#define ColorBlend_Normal(T,A,B)        (ColorBlend_Buffer(T,A,B,Normal))
#define ColorBlend_Lighten(T,A,B)       (ColorBlend_Buffer(T,A,B,Lighten))
#define ColorBlend_Darken(T,A,B)        (ColorBlend_Buffer(T,A,B,Darken))
#define ColorBlend_Multiply(T,A,B)      (ColorBlend_Buffer(T,A,B,Multiply))
#define ColorBlend_Average(T,A,B)       (ColorBlend_Buffer(T,A,B,Average))
#define ColorBlend_Add(T,A,B)           (ColorBlend_Buffer(T,A,B,Add))
#define ColorBlend_Subtract(T,A,B)      (ColorBlend_Buffer(T,A,B,Subtract))
#define ColorBlend_Difference(T,A,B)    (ColorBlend_Buffer(T,A,B,Difference))
#define ColorBlend_Negation(T,A,B)      (ColorBlend_Buffer(T,A,B,Negation))
#define ColorBlend_Screen(T,A,B)        (ColorBlend_Buffer(T,A,B,Screen))
#define ColorBlend_Exclusion(T,A,B)     (ColorBlend_Buffer(T,A,B,Exclusion))
#define ColorBlend_Overlay(T,A,B)       (ColorBlend_Buffer(T,A,B,Overlay))
#define ColorBlend_SoftLight(T,A,B)     (ColorBlend_Buffer(T,A,B,SoftLight))
#define ColorBlend_HardLight(T,A,B)     (ColorBlend_Buffer(T,A,B,HardLight))
#define ColorBlend_ColorDodge(T,A,B)    (ColorBlend_Buffer(T,A,B,ColorDodge))
#define ColorBlend_ColorBurn(T,A,B)     (ColorBlend_Buffer(T,A,B,ColorBurn))
#define ColorBlend_LinearDodge(T,A,B)   (ColorBlend_Buffer(T,A,B,LinearDodge))
#define ColorBlend_LinearBurn(T,A,B)    (ColorBlend_Buffer(T,A,B,LinearBurn))
#define ColorBlend_LinearLight(T,A,B)   (ColorBlend_Buffer(T,A,B,LinearLight))
#define ColorBlend_VividLight(T,A,B)    (ColorBlend_Buffer(T,A,B,VividLight))
#define ColorBlend_PinLight(T,A,B)      (ColorBlend_Buffer(T,A,B,PinLight))
#define ColorBlend_HardMix(T,A,B)       (ColorBlend_Buffer(T,A,B,HardMix))
#define ColorBlend_Reflect(T,A,B)       (ColorBlend_Buffer(T,A,B,Reflect))
#define ColorBlend_Glow(T,A,B)          (ColorBlend_Buffer(T,A,B,Glow))
#define ColorBlend_Phoenix(T,A,B)       (ColorBlend_Buffer(T,A,B,Phoenix))
#endif
    
static INLINE int
HardLight(int src, int dst)
{
    int t;
    
    if (dst < 128) {
        return 2 * imul8x8(src, dst, t);
    } else {
        return 255 - (2 * imul8x8(255-src, 255-dst, t));
    }        
}

static INLINE int
Overlay(int src, int dst)
{
    int t;
    
    if (src < 128) {
        return 2 * imul8x8(src, dst, t);
    } else {
        return 255 - (2 * (imul8x8(255-src, 255-dst, t)));
    }        
}

static INLINE int
SoftLight(int src, int dst)
{
    if (dst < 128) {
        return (2 * ((src >> 1) + 64)) * ((double)dst / 255.0);
    } else {
        return 255 - (2 * (255 - ((src >> 1) + 64)) * ((255 - dst) / 255.0));
    }
}

static INLINE int
SoftLight2(int src, int dst)
{
    double s, d, x;

    d = dst / 255.0;
    s = src / 255.0;
    x = round(255 * ((1 - s) * s * d + s * (1 - (1 - d) * (1 - s))));
    return (int)x;
}
                                            

static INLINE int
VividLight(int src, int dst)
{
    if (src < 128) {
        return ColorBurn(2 * src, dst);
    } else {
        return ColorDodge(2 * (src - 128), dst);
    }        
}

static INLINE int
Add(int src, int dst)
{
    int c;

    c = src + dst;
    if (c > 0xFF) {
        return 0xFF;
    }
    return c;
}

static INLINE int
Subtract(int src, int dst)
{
    int c;

    c = dst + src;
    if (c < 255) {
        return 0;
    }
    return c - 255;
}

static INLINE int
LinearDodge(int src, int dst)
{
    return Add(src, dst);
}

static INLINE int
LinearBurn(int src, int dst)
{
    int c;

    c = src + dst;
    if (c < 255) {
        return 0;
    }
    return c - 255;
}

static INLINE int
LinearLight(int src, int dst)
{
    if (src < 128) {
        return LinearBurn(2 * src, dst);
    } else {
        return LinearDodge(2 * (src - 128), dst);
    }
}

static INLINE int
PinLight(int src, int dst)
{
    if (src < 128) {
        return MIN(2 * src, dst);
    } else {
        return MAX(2 * (src - 128),   dst);
    }
}

static INLINE int
Divide(int src, int dst)
{
    if (src == 0) {
        return 0;
    } else {
        return (int)((dst / (double)src) + 0.5) * 255;
    }
}

static INLINE int
Exclusion(int src, int dst)
{
    int t;
    
    return (src + dst - 2 * imul8x8(src, dst, t));    
}


static void
ColorBlendRegion(
    Pict *destPtr,                      /* (in/out) Background picture.
                                         * Composite overwrites region in
                                         * background. */
    Pict *srcPtr,                       /* Foreground picture. */
    Blt_BlendingMode mode,              /* Blend mode. */
    int sx, int sy,                     /* Origin of foreground region in
                                         * source. */
    int w, int h,                       /* Dimension of region to be
                                           blended. */
    int dx, int dy)                     /* Origin of background region in
                                         * destination. */
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    if ((srcPtr->flags & BLT_PIC_PREMULT_COLORS) == 0) {
        Blt_PremultiplyColors(srcPtr);
    }
    if ((destPtr->flags & BLT_PIC_PREMULT_COLORS) == 0) {
        Blt_PremultiplyColors(destPtr);
    }
    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((sy * srcPtr->pixelsPerRow) + sx);

    switch (mode) {
    case BLT_BLEND_NORMAL:              /* C = F */
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                dp->u32 = sp->u32;
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;

    case BLT_BLEND_MULTIPLY:            /* C = F * B */
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = imul8x8(dp->Red, sp->Red, t);
                dp->Green = imul8x8(dp->Green, sp->Green, t);
                dp->Blue  = imul8x8(dp->Blue, sp->Blue, t);
                dp->Alpha = imul8x8(dp->Alpha, sp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
        
    case BLT_BLEND_COLORDODGE:          /* C = B / (1 - F) */
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = ColorDodge(sp->Red, dp->Red);
                dp->Green = ColorDodge(sp->Green, dp->Green);
                dp->Blue  = ColorDodge(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;

    case BLT_BLEND_COLORBURN:           /* C = B / F; */
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = ColorBurn(sp->Red, dp->Red);
                dp->Green = ColorBurn(sp->Green, dp->Green);
                dp->Blue  = ColorBurn(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;

    case BLT_BLEND_OVERLAY:             /* Multiplies or screens depending
                                         * upon background color. */
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = Overlay(sp->Red,   dp->Red);
                dp->Green = Overlay(sp->Green, dp->Green);
                dp->Blue  = Overlay(sp->Blue,  dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;

    case BLT_BLEND_SCREEN:              /* C = 1 - ((1 - F) * B) */
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = (255 - (imul8x8(255-sp->Red,   255-dp->Red, t)));
                dp->Green = (255 - (imul8x8(255-sp->Green, 255-dp->Green, t)));
                dp->Blue  = (255 - (imul8x8(255-sp->Blue,  255-dp->Blue, t)));
                dp->Alpha = imul8x8(dp->Alpha, sp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
        
    case BLT_BLEND_DARKEN:              /* C = min(F,B) */
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = MIN(sp->Red,   dp->Red);
                dp->Green = MIN(sp->Green, dp->Green);
                dp->Blue  = MIN(sp->Blue,  dp->Blue);
                dp->Alpha = imul8x8(dp->Alpha, sp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
        
    case BLT_BLEND_LIGHTEN:             /* C = max(F,B) */
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = MIN(sp->Red,   dp->Red);
                dp->Green = MIN(sp->Green, dp->Green);
                dp->Blue  = MIN(sp->Blue,  dp->Blue);
                dp->Alpha = imul8x8(dp->Alpha, sp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
        
    case BLT_BLEND_DIFFERENCE:          /* C = |F - B| */
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = ABS(sp->Red   - dp->Red);
                dp->Green = ABS(sp->Green - dp->Green);
                dp->Blue  = ABS(sp->Blue  - dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
        
    case BLT_BLEND_HARDLIGHT:
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = HardLight(sp->Red, dp->Red);
                dp->Green = HardLight(sp->Green, dp->Green);
                dp->Blue  = HardLight(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
        
    case BLT_BLEND_SOFTLIGHT:
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = SoftLight(sp->Red, dp->Red);
                dp->Green = SoftLight(sp->Green, dp->Green);
                dp->Blue  = SoftLight(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
        
    case BLT_BLEND_SOFTLIGHT2:
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = SoftLight2(sp->Red, dp->Red);
                dp->Green = SoftLight2(sp->Green, dp->Green);
                dp->Blue  = SoftLight2(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
        
    case BLT_BLEND_VIVIDLIGHT:
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = VividLight(sp->Red, dp->Red);
                dp->Green = VividLight(sp->Green, dp->Green);
                dp->Blue  = VividLight(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
        
    case BLT_BLEND_LINEARDODGE:
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = LinearDodge(sp->Red, dp->Red);
                dp->Green = LinearDodge(sp->Green, dp->Green);
                dp->Blue  = LinearDodge(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
        
    case BLT_BLEND_LINEARBURN:
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = LinearBurn(sp->Red, dp->Red);
                dp->Green = LinearBurn(sp->Green, dp->Green);
                dp->Blue  = LinearBurn(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
        
    case BLT_BLEND_LINEARLIGHT:
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = LinearLight(sp->Red, dp->Red);
                dp->Green = LinearLight(sp->Green, dp->Green);
                dp->Blue  = LinearLight(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;

    case BLT_BLEND_PINLIGHT:
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = PinLight(sp->Red, dp->Red);
                dp->Green = PinLight(sp->Green, dp->Green);
                dp->Blue  = PinLight(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
        
    case BLT_BLEND_DIVIDE:
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;
                
                dp->Red   = Divide(sp->Red, dp->Red);
                dp->Green = Divide(sp->Green, dp->Green);
                dp->Blue  = Divide(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;

    case BLT_BLEND_EXCLUSION:
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = Exclusion(sp->Red, dp->Red);
                dp->Green = Exclusion(sp->Green, dp->Green);
                dp->Blue  = Exclusion(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
        
    case BLT_BLEND_HARDMIX:
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = PinLight(sp->Red, dp->Red);
                dp->Green = PinLight(sp->Green, dp->Green);
                dp->Blue  = PinLight(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;

    case BLT_BLEND_SUBTRACT:
        for (y = 0; y < h; y++) {
            Blt_Pixel *sp, *send, *dp;

            dp = destRowPtr;
            for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
                int t;

                dp->Red   = Subtract(sp->Red, dp->Red);
                dp->Green = Subtract(sp->Green, dp->Green);
                dp->Blue  = Subtract(sp->Blue, dp->Blue);
                dp->Alpha = imul8x8(sp->Alpha, dp->Alpha, t);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
    }
}

void
Blt_ColorBlendPictures(Pict *destPtr, Pict *srcPtr, Blt_BlendingMode mode)
{
    ColorBlendRegion(destPtr, srcPtr, mode, 0, 0, srcPtr->width, 
                     srcPtr->height, 0, 0);
}

/*
 *      filter function definitions
 */

/*
 *---------------------------------------------------------------------------
 *
 * ByteToAsciiHex --
 *
 *      Convert a byte to its ASCII hexidecimal equivalent.
 *
 * Results:
 *      The converted 2 ASCII character string is returned.
 *
 *---------------------------------------------------------------------------
 */
INLINE static void
ByteToAsciiHex(unsigned char byte, char *string)
{
    static char hexDigits[] = "0123456789ABCDEF";

    string[0] = hexDigits[byte >> 4];
    string[1] = hexDigits[byte & 0x0F];
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PictureToDBuffer --
 *
 *      Converts a picture to PostScript RGB (3 components) or Greyscale (1
 *      component) output.  With 3 components, we assume the "colorimage"
 *      operator is available.
 *
 *      Note: The picture is converted from bottom to top, to conform with
 *            the PostScript coordinate system.
 *
 * Results:
 *      The PostScript data comprising the picture is written into the dynamic
 *      string.
 *
 *---------------------------------------------------------------------------
 */
Blt_DBuffer 
Blt_PictureToDBuffer(Pict *srcPtr, int numComponents)
{
    Blt_DBuffer dBuffer;
    int length;

    dBuffer = Blt_DBuffer_Create();
    length = srcPtr->width * srcPtr->height * numComponents;
    Blt_DBuffer_Resize(dBuffer, length);
    if (numComponents == 3) {
        Blt_Pixel *srcRowPtr;
        int y;
        unsigned char *bp;
        
        bp = Blt_DBuffer_Pointer(dBuffer);
        srcRowPtr = srcPtr->bits + ((srcPtr->height-1) * srcPtr->pixelsPerRow);
        for (y = (srcPtr->height - 1); y >= 0; y--) {
            Blt_Pixel *sp, *send;
            
            for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
                bp[0] = sp->Red;
                bp[1] = sp->Green;
                bp[2] = sp->Blue;
                bp += 3;
            }
            srcRowPtr -= srcPtr->pixelsPerRow;
        }
    } else if (numComponents == 1) {
        Blt_Pixel *srcRowPtr;
        int y;
        unsigned char *bp;
        
        bp = Blt_DBuffer_Pointer(dBuffer);
        srcRowPtr = srcPtr->bits + ((srcPtr->height-1) * srcPtr->pixelsPerRow);
        for (y = (srcPtr->height - 1); y >= 0; y--) {
            Blt_Pixel *sp, *send;
            
            for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
                *bp++ = sp->Red;
            }
            srcRowPtr -= srcPtr->pixelsPerRow;
        }
    }
    Blt_DBuffer_SetLength(dBuffer, length);
    return dBuffer;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PictureToPsData --
 *
 *      Converts a picture to PostScript RGB (3 components) or Greyscale (1
 *      component) output.  With 3 components, we assume the "colorimage"
 *      operator is available.
 *
 *      Note: The picture is converted from bottom to top, to conform with
 *            the PostScript coordinate system.
 *
 * Results:
 *      The PostScript data comprising the picture is written into the
 *      dynamic string.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_PictureToPsData(
    Pict *srcPtr,                       /* Picture to be represented in
                                         * PostScript. */
    int numComponents,                  /* # of color components (1 or 3).
                                         * If it's 1, we only look at red
                                         * for color information. */
    Tcl_DString *resultPtr,             /* (out) Holds the generated
                                         * postscript */
    const char *prefix)                 /* Indicates how to prefix the
                                         * start of each line of
                                         * output. This is normally used
                                         * for PostScript previews, where
                                         * each line is * comment "% ". */
{
    int count, numLines;

    numLines = count = 0;
    if (numComponents == 3) {
        Blt_Pixel *srcRowPtr;
        int y;

        srcRowPtr = srcPtr->bits + ((srcPtr->height-1) * srcPtr->pixelsPerRow);
        for (y = (srcPtr->height - 1); y >= 0; y--) {
            Blt_Pixel *sp, *send;

            for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
                char string[10];

                if (count == 0) {
                    Tcl_DStringAppend(resultPtr, prefix, -1);
                    Tcl_DStringAppend(resultPtr, " ", -1);
                }
                count += 6;
                ByteToAsciiHex(sp->Red, string);
                ByteToAsciiHex(sp->Green, string + 2);
                ByteToAsciiHex(sp->Blue, string + 4);
                string[6] = '\0';
                if (count >= 60) {
                    string[6] = '\n';
                    string[7] = '\0';
                    count = 0;
                    numLines++;
                }
                Tcl_DStringAppend(resultPtr, string, -1);
            }
            srcRowPtr -= srcPtr->pixelsPerRow;
        }
    } else if (numComponents == 1) {
        int y;
        Blt_Pixel *srcRowPtr;

        srcRowPtr = srcPtr->bits + ((srcPtr->height-1) * srcPtr->pixelsPerRow);
        for (y = (srcPtr->height - 1); y >= 0; y--) {
            Blt_Pixel *sp, *send;

            for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
                char string[10];
                char byte;

                if (count == 0) {
                    Tcl_DStringAppend(resultPtr, prefix, -1);
                    Tcl_DStringAppend(resultPtr, " ", -1);
                }
                count += 2;
                byte = ~(sp->Red);
                ByteToAsciiHex(byte, string);
                string[2] = '\0';
                if (count >= 60) {
                    string[2] = '\n';
                    string[3] = '\0';
                    count = 0;
                    numLines++;
                }
                Tcl_DStringAppend(resultPtr, string, -1);
            }
            srcRowPtr -= srcPtr->pixelsPerRow;
        }
    }
    if (count != 0) {
        Tcl_DStringAppend(resultPtr, "\n", -1);
        numLines++;
    }
    return numLines;
}

static double
DefaultFilter(double x)
{
    if (x < 0.0) {
        x = -x;
    }
    if (x < 1.0) {
        /* f(x) = 2x^3 - 3x^2 + 1, -1 <= x <= 1 */
        return (2.0 * x - 3.0) * x * x + 1.0;
    }
    return 0.0;
}

/* Just for testing */
static double
DummyFilter(double x)
{
    return FABS(x);
}

/*
 *
 * Finite filters in increasing order:
 *      Box (constant)
 *      Triangle (linear)
 *      Bell
 *      BSpline (cubic)
 *
 */
static double
BoxFilter(double x)
{
    if ((x < -0.5) || (x > 0.5)) {
        return 0.0;
    }
    return 1.0;
}

static double
TriangleFilter(double x)
{
    if (x < 0.0) {
        x = -x;
    }
    if (x < 1.0) {
        return (1.0 - x);
    }
    return 0.0;
}

static double
BellFilter(double x)
{
    if (x < 0.0) {
        x = -x;
    }
    if (x < 0.5) {
        return (0.75 - (x * x));
    }
    if (x < 1.5) {
        x = (x - 1.5);
        return (0.5 * (x * x));
    }
    return 0.0;
}

static double
BSplineFilter(double x)
{
    double x2;

    if (x < 0.0) {
        x = -x;
    }
    if (x < 1) {
        x2 = x * x;
        return ((.5 * x2 * x) - x2 + (2.0 / 3.0));
    } else if (x < 2) {
        x = 2 - x;
        return ((x * x * x) / 6.0);
    }
    return 0.0;
}

/*
 *
 * Infinite Filters:
 *      Sinc            perfect lowpass filter
 *      Bessel          circularly symmetric 2-D filter
 *      Gaussian
 *      Lanczos3
 *      Mitchell
 */

static double
SincFilter(double x)
{
    x *= M_PI;
    if (x == 0.0) {
        return 1.0;
    }
    return (sin(x) / x);
}

static double
BesselFilter(double x)
{
    /*
     * See Pratt "Digital Image Processing" p. 97 for Bessel functions
     * zeros are at approx x=1.2197, 2.2331, 3.2383, 4.2411, 5.2428,
     * 6.2439, 7.2448, 8.2454
     */
    return (x == 0.0) ? M_PI / 4.0 : j1(M_PI * x) / (x + x);
}

#define SQRT_2PI        0.79788456080286541     /* sqrt(2.0 / M_PI) */

static double
GaussianFilter(double x)
{
    return exp(-2.0 * x * x) * SQRT_2PI;
}

static double
Lanczos3Filter(double x)
{
    if (x < 0) {
        x = -x;
    }
    if (x < 3.0) {
        return (SincFilter(x) * SincFilter(x / 3.0));
    }
    return 0.0;
}

#define B               0.3333333333333333      /* (1.0 / 3.0) */
#define C               0.3333333333333333      /* (1.0 / 3.0) */

static double
MitchellFilter(double x)
{
    double x2;

    x2 = x * x;
    if (x < 0) {
        x = -x;
    }
    if (x < 1.0) {
        x = (((12.0 - 9.0 * B - 6.0 * C) * (x * x2)) +
            ((-18.0 + 12.0 * B + 6.0 * C) * x2) + (6.0 - 2 * B));
        return (x / 6.0);
    } else if (x < 2.0) {
        x = (((-1.0 * B - 6.0 * C) * (x * x2)) + ((6.0 * B + 30.0 * C) * x2) +
            ((-12.0 * B - 48.0 * C) * x) + (8.0 * B + 24 * C));
        return (x / 6.0);
    }
    return 0.0;
}

/*
 * Catmull-Rom spline
 */
static double
CatRomFilter(double x)
{
    if (x < -2.) {
        return 0.0;
    }
    if (x < -1.0) {
        return 0.5 * (4.0 + x * (8.0 + x * (5.0 + x)));
    }
    if (x < 0.0) {
        return 0.5 * (2.0 + x * x * (-5.0 + x * -3.0));
    }
    if (x < 1.0) {
        return 0.5 * (2.0 + x * x * (-5.0 + x * 3.0));
    }
    if (x < 2.0) {
        return 0.5 * (4.0 + x * (-8.0 + x * (5.0 - x)));
    }
    return 0.0;
}

/* approximation to the gaussian integral [x, inf) */
static double
GiFilter(double x)
{
    if (x > 1.5) {
        return 0.0;
    } else if (x < -1.5) {
        return 1.0;
    } else {
#define I6 0.166666666666667
#define I4 0.25
#define I3 0.333333333333333
        double x2 = x * x;
        double x3 = x2 * x;

        if (x > 0.5) {
            return .5625  - ( x3 * I6 - 3 * x2 * I4 + 1.125 * x);
        } else if (x > -0.5) {
            return 0.5    - (0.75 * x - x3 * I3);
        } else {
            return 0.4375 + (-x3 * I6 - 3 * x2 * I4 - 1.125 * x);
        }
    }
}

static ResampleFilter resampleFilters[] =
{
    /* Name,     Function,                   Support */
    {"bell",     BellFilter,                 1.5         },
    {"bessel",   BesselFilter,               3.2383      },
    {"box",      BoxFilter,                  0.5         },
    {"bspline",  BSplineFilter,              2.0         },
    {"catrom",   CatRomFilter,               2.0         },
    {"default",  DefaultFilter,              1.0         },
    {"dummy",    DummyFilter,                0.5         },
    {"gauss8",   GaussianFilter,             8.0         },
    {"gaussian", GaussianFilter,             1.25        },
    {"gi",       GiFilter,                   4.0         },
    {"gi8",      GiFilter,                   8.0         },
    {"lanczos3", Lanczos3Filter,             3.0         },
    {"mitchell", MitchellFilter,             2.0         },
    {"none",     (Blt_ResampleFilterProc *)NULL, 0.0     },
    {"sinc",     SincFilter,                 4.0         },
    {"sinc8",    SincFilter,                 8.0         },
    {"sinc12",   SincFilter,                 12.0        }, 
    {"tent",     TriangleFilter,             1.0         },
    {"triangle", TriangleFilter,             1.0         },
};

static int numFilters = sizeof(resampleFilters) / sizeof(ResampleFilter);

Blt_ResampleFilter bltBoxFilter = &(resampleFilters[2]);
Blt_ResampleFilter bltMitchellFilter = &(resampleFilters[11]);
Blt_ResampleFilter bltBellFilter = &(resampleFilters[0]);
Blt_ResampleFilter bltTentFilter = &(resampleFilters[16]);
Blt_ResampleFilter bltGaussianFilter = &(resampleFilters[9]);

const char *
Blt_NameOfResampleFilter(ResampleFilter *filterPtr)
{
    return filterPtr->name;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetResampleFilterFromObj --
 *
 *      Finds a 1-D filter associated by the given filter name.
 *
 * Results:
 *      A standard TCL result.  Returns TCL_OK is the filter was found.
 *      The filter information (proc and support) is returned via
 *      filterPtrPtr. Otherwise TCL_ERROR is returned and an error message
 *      is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetResampleFilterFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                             ResampleFilter **filterPtrPtr)
{
    ResampleFilter *fp, *fend;
    const char *string;

    string = Tcl_GetString(objPtr);
    for (fp = resampleFilters, fend = fp + numFilters; fp < fend; fp++) {
        if (strcmp(string, fp->name) == 0) {
            *filterPtrPtr = (fp->proc == NULL) ? NULL : fp;
            return TCL_OK;
        }
    }
    if (interp != NULL) {
        Tcl_AppendResult(interp, "can't find filter \"", string, "\"", 
                     (char *)NULL);
    }
    return TCL_ERROR;
}

#ifdef notdef
int
Blt_GetTableFilterFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                          TableFilter **filterPtrPtr)
{
    TableFilter *filterPtr;
    Tcl_Obj **objv;
    double scale;
    int objc;

    /* Next see if it's a table (list of numbers) */
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;       /* Can't split object. */
    }
    if (Tcl_GetDoubleFromObj(interp, objv[0], &scale) != TCL_OK) {
        return TCL_ERROR;
    }
    objc--, objv++;
    if ((objc & 0x1) == 0) {
        Tcl_AppendResult(interp, "# of values in 1-D filter \"", string, 
                "\" must be odd", (char *)NULL);
        return TCL_ERROR;
    }
    filterPtr = Blt_AssertMalloc(sizeof(TableFilter) + (sizeof(int) * objc));
    filterPtr->numWeights = objc;
    filterPtr->scale = (float)scale;
    { 
        int *tp, *tend, i;

        i = 0;
        for (tp = filterPtr->table, tend < tp + objc; tp < tend; tp++, i++) {
            if (Tcl_GetIntFromObj(interp, objv[i], tp) != TCL_OK) {
                Blt_Free(filterPtr);
                return TCL_ERROR;
            }
        }
    }
    *filterPtrPtr = filterPtr;
    return TCL_OK;
}
#endif

#ifdef notdef
unsigned int
Blt_FreeSamples(Sample *sampPtr)
{
    Blt_Free(sampPtr->buffer);
    Blt_Free(sampPtr);
}
#endif

unsigned int
Blt_ComputeWeights(unsigned int sw, unsigned int dw, ResampleFilter *filterPtr,
                   Sample **sampPtrPtr)
{
    Sample *samples;
    double scale;
    size_t bytesPerSample;

    /* Pre-calculate filter contributions for a row or column. */
    scale = (double)dw / (double)sw;
    if (scale < 1.0) {
        Sample *sampPtr;
        double r, fscale;
        int filterSize, x;

        /* Downsample */

        r = filterPtr->support / scale;
        fscale = 1.0 / scale;
        filterSize = (int)(r * 2 + 2);

        bytesPerSample = sizeof(Sample) + 
            ((filterSize - 1) * sizeof(PixelWeight));
        samples = Blt_AssertCalloc(dw, bytesPerSample);
        sampPtr = samples;
#define DEBUG 0
#if DEBUG
        fprintf(stderr, "downscale=%g, fscale=%g, radius=%g\n", 
                scale, fscale, r);
#endif
        for (x = 0; x < (int)dw; x++) {
            PixelWeight *wp;
            double sum, center, factor;
            int i, left, right;         /* filter bounds */

            center = ((double)x) * fscale;

            /* Determine bounds of filter and its density */
#ifndef notdef
            left = (int)/*floor*/(center - r);
#else
            left = CRoundToInt(center - r);
#endif
            if (left < 0) {
                left = 0;
            }
#ifndef notdef
            right = (int)/*floor*/(center + r);
#else
            right = CRoundToInt(center + r);
#endif
            if (right >= sw) {
                right = sw - 1;
            }
            sampPtr->start = left;
            sampPtr->numWeights = (right - left + 1);
            sampPtr->wend = sampPtr->weights + (right - left + 1);

            sum = 0.0;
            for (wp = sampPtr->weights, i = left; i <= right; i++, wp++) {
                double val = ((double)i - center) * scale;
                wp->f32 = (float)(*filterPtr->proc)(val);
                sum += wp->f32;
            }
            factor = (sum == 0.0) ? 1.0 : (1.0 / sum);
            for (wp = sampPtr->weights; wp < sampPtr->wend; wp++) {
                wp->f32 = (float)(wp->f32 * factor);
                wp->i32 = float2si(wp->f32);
            }
            sampPtr = (Sample *)((char *)sampPtr + bytesPerSample);
        }
    } else {
        Sample *sampPtr;
        double fscale;
        int filterSize, x;

        /* Upsample */

        filterSize = (int)(filterPtr->support * 2 + 2);
        bytesPerSample = sizeof(Sample) + 
            ((filterSize - 1) * sizeof(PixelWeight));
        samples = Blt_AssertCalloc(dw, bytesPerSample);
        fscale = 1.0 / scale;

        sampPtr = samples;
#if DEBUG
        fprintf(stderr, "upscale=%g, fscale=%g, radius=%g\n",
                    scale, fscale, filterPtr->support);
#endif
        for (x = 0; x < (int)dw; x++) {
            PixelWeight *wp;
            double sum, center, factor;
            int i, left, right;         /* filter bounds */

            center = ((double)x) * fscale;
#ifndef notdef
            left = (int)(center - filterPtr->support);
#else
            left = CRoundToInt(center - filterPtr->support);
#endif
            if (left < 0) {
                left = 0;
            }
#ifndef notdef
            right = (int)(center + filterPtr->support);
#else
            right = CRoundToInt(center + filterPtr->support);
#endif
            if (right >= sw) {
                right = sw - 1;
            }
            sampPtr->start = left;
            sampPtr->numWeights = (right - left + 1);
            sampPtr->wend = sampPtr->weights + (right - left + 1);

            /* Sum the contributions for each pixel in the filter. */
            sum = 0.0;
            for (wp = sampPtr->weights, i = left; i <= right;i++, wp++) {
                wp->f32 = (float) (*filterPtr->proc) ((double)i - center);
                sum += wp->f32;
            }

            /* The sum of the contributions should not be greater than 1.0 */
            factor = (sum == 0.0) ? 1.0 : (1.0 / sum);

            for (wp = sampPtr->weights; wp < sampPtr->wend; wp++) {
#ifdef notdef
                fprintf(stderr, "w[%d]=%g, %g sum=%g\n", 
                        wp - sampPtr->weights, 
                        wp->f32, wp->f32 * factor, sum);
#endif
                wp->f32 = (float)(wp->f32 * factor);
                /* Convert each weight into a fixed-point scaled integer */
                wp->i32 = float2si(wp->f32);

            }
            /* Go to the next sample. */
            sampPtr = (Sample *)((char *)sampPtr + bytesPerSample);
        }
    }
    *sampPtrPtr = samples;
    return bytesPerSample;
}

static void
ZoomVertically(Pict *destPtr, Pict *srcPtr, Blt_ResampleFilter filter)
{
    Sample *samples, *send;
    int x;
    int bytesPerSample;                 /* Size of sample. */

    /* Pre-calculate filter contributions for each row. */
    bytesPerSample = Blt_ComputeWeights(srcPtr->height, destPtr->height, 
        filter, &samples);
    send = (Sample *)((char *)samples + (destPtr->height * bytesPerSample));

    /* Apply filter to each row. */
    for (x = 0; x < srcPtr->width; x++) {
        Blt_Pixel *dp, *srcColPtr;
        Sample *sampPtr;

        srcColPtr = srcPtr->bits + x;
        dp = destPtr->bits + x;
        for (sampPtr = samples; sampPtr < send; 
             sampPtr = (Sample *)((char *)sampPtr + bytesPerSample)) {
            Blt_Pixel *sp;
            int r, g, b, a;
            PixelWeight *wp;

            r = g = b = a = 0;
            sp = srcColPtr + (sampPtr->start * srcPtr->pixelsPerRow);
            for (wp = sampPtr->weights; wp < sampPtr->wend; wp++) {
                a += wp->i32 * sp->Alpha;
                r += wp->i32 * sp->Red;
                g += wp->i32 * sp->Green;
                b += wp->i32 * sp->Blue;
                sp += srcPtr->pixelsPerRow;
            }
            dp->Alpha = SICLAMP(a);
            dp->Red   = SICLAMP(r);
            dp->Green = SICLAMP(g);
            dp->Blue  = SICLAMP(b);
#ifdef notdef
            if (dp->Alpha != 0xFF) {
                fprintf(stdout, "v1: alpha=0x%x\n", dp->Alpha);
            }
#endif
            dp += destPtr->pixelsPerRow;

        }
    }
    /* Free the memory allocated for filter weights. */
    Blt_Free(samples);
}

static void
ZoomHorizontally(Pict *destPtr, Pict *srcPtr, Blt_ResampleFilter filter)
{
    Sample *samples, *send;
    int y;
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int bytesPerSample;                 /* Size of sample. */

    /* Pre-calculate filter contributions for each column. */
    bytesPerSample = Blt_ComputeWeights(srcPtr->width, destPtr->width, 
        filter, &samples);
    send = (Sample *)((char *)samples + (destPtr->width * bytesPerSample));

    /* Apply filter to each column. */
    srcRowPtr = srcPtr->bits;
    destRowPtr = destPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *dp;
        Sample *sampPtr;

        dp = destRowPtr;
        for (sampPtr = samples; sampPtr < send; 
             sampPtr = (Sample *)((char *)sampPtr + bytesPerSample)) {
            Blt_Pixel *sp;
            int r, g, b, a;
            PixelWeight *wp;

            r = g = b = a = 0;
            sp = srcRowPtr + sampPtr->start;
            for (wp = sampPtr->weights; wp < sampPtr->wend; wp++) {
                a += wp->i32 * sp->Alpha;
                r += wp->i32 * sp->Red;
                g += wp->i32 * sp->Green;
                b += wp->i32 * sp->Blue;
                sp++;
            }
            dp->Alpha = SICLAMP(a);
            dp->Red   = SICLAMP(r);
            dp->Green = SICLAMP(g);
            dp->Blue  = SICLAMP(b);
#ifdef notdef
            if (dp->Alpha != 0xFF) {
                fprintf(stdout, "h1: alpha=0x%x\n", dp->Alpha);
            }
#endif
            dp++;
        }
        srcRowPtr  += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
    /* Free the memory allocated for horizontal filter weights. */
    Blt_Free(samples);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ResamplePicture --
 *
 *      Resamples a given picture using 1-D filters and returns a new
 *      picture of the designated size.
 *
 * Results:
 *      Returns the resampled picture. The original picture is left intact.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ResamplePicture(Pict *destPtr, Pict *srcPtr, Blt_ResampleFilter hFilter, 
                    Blt_ResampleFilter vFilter)
{
    Pict *tmpPtr;

    tmpPtr = Blt_CreatePicture(destPtr->width, srcPtr->height);
    if ((srcPtr->flags & (BLT_PIC_COMPOSITE | BLT_PIC_PREMULT_COLORS)) == 
        BLT_PIC_COMPOSITE) {
        Blt_PremultiplyColors(srcPtr);
    }
    if ((destPtr->flags & (BLT_PIC_COMPOSITE | BLT_PIC_PREMULT_COLORS)) == 
        BLT_PIC_COMPOSITE) {
        Blt_PremultiplyColors(destPtr);
    }

    /* 
     * It's usually faster to zoom vertically last.  This has to do with
     * the fact that pictures are stored in contiguous rows.
     */
    Blt_ZoomHorizontally(tmpPtr, srcPtr, hFilter);
    Blt_ZoomVertically(destPtr, tmpPtr, vFilter);
    Blt_FreePicture(tmpPtr);
    destPtr->flags = srcPtr->flags;
    destPtr->flags |= BLT_PIC_DIRTY;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ResamplePicture2 --
 *
 *      Resamples a given picture using 1-D filters and returns a new
 *      picture of the designated size.
 *
 * Results:
 *      Returns the resampled picture. The original picture is left intact.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ResamplePicture2(Pict *destPtr, Pict *srcPtr, Blt_ResampleFilter hFilter, 
                    Blt_ResampleFilter vFilter)
{
    Pict *tmpPtr;

    tmpPtr = Blt_CreatePicture(destPtr->width, srcPtr->height);
    if ((srcPtr->flags & (BLT_PIC_COMPOSITE | BLT_PIC_PREMULT_COLORS)) == 
        BLT_PIC_COMPOSITE) {
        Blt_PremultiplyColors(srcPtr);
    }
    if ((destPtr->flags & (BLT_PIC_COMPOSITE | BLT_PIC_PREMULT_COLORS)) == 
        BLT_PIC_COMPOSITE) {
        Blt_PremultiplyColors(destPtr);
    }

    /* 
     * It's usually faster to zoom vertically last.  This has to do with
     * the fact that pictures are stored in contiguous rows.
     */
    ZoomHorizontally(tmpPtr, srcPtr, hFilter);
    ZoomVertically(destPtr, tmpPtr, vFilter);
    Blt_FreePicture(tmpPtr);
    destPtr->flags = srcPtr->flags;
    destPtr->flags |= BLT_PIC_DIRTY;
}

static void
FillScaleTables(
    int sw, int sh,                     /* Dimension of source. */
    int ax, int ay,                     /* Origin of requested area. */
    int aw, int ah,                     /* Dimension of requested area. */
    int dw, int destHeight,             /* Desired new dimension. */
    int *mapX, int *mapY)               /* (out) Resulting mapping
                                           tables. */
{
    int left, right, top, bottom;
    double xScale, yScale;
    int x, y;

    left = ax;
    top = ay;
    right = MIN(sw, ax + aw) - 1;
    bottom = MIN(sh, ay + ah) - 1;

    xScale = (double)aw / (double)dw;
    for (x = 0; x < dw; x++) {
        int sx;
        
        sx = (int)(xScale * (double)x);
        sx += left;
        if (sx > right) {
            sx = right;
        }
        mapX[x] = sx;
    }
    yScale = (double)ah / (double)destHeight;
    for (y = 0; y < destHeight; y++) {
        int sy;
        
        sy = (int)(yScale * (double)y);
        sy += top;
        if (sy > bottom) {
            sy = bottom;
        }
        mapY[y] = sy;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ScalePicture --
 *
 *      Scales the region of the source picture to the size requested.
 *      This routine performs raw scaling of the image and unlike
 *      Blt_ResamplePhoto does not do any filtering.
 *
 *          src picture
 *      +===================+
 *      |                   |              dest picture
 *      |                   |            +==============+
 *      | x,y               |            |              |
 *      |  *-------*        |            |              |
 *      |  | area  |        |            |              |
 *      |  |       |        |            |              |
 *      |  |       | height |            |              | 
 *      |  |       |        |            |              |
 *      |  *-------*        |            |              |
 *      |    width          |            |              |
 *      |                   |            +==============+
 *      |                   |
 *      +===================+
 *
 *              x ratio = dest width / area width
 *              y ratio = dest height / area height
 *              
 *
 * Results:
 *      Returns the new resized picture.  The original picture is left intact.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture
Blt_ScalePicture(
    Pict *srcPtr,                       /* Source picture to be scaled. */
    int sx, int sy,                     /* Area of source picture to
                                         * scaled. */
    int sw, int sh,
    int reqWidth, int reqHeight)        /* Requested dimensions of the
                                         * scaled picture. */
{
    Pict *destPtr;
    Blt_Pixel *destRowPtr;
    int *mapX, *mapY;
    int y;

    mapX = (int *)Blt_AssertMalloc(sizeof(int) * reqWidth);
    mapY = (int *)Blt_AssertMalloc(sizeof(int) * reqHeight);
    FillScaleTables(srcPtr->width, srcPtr->height, sx, sy, sw, sh, 
        reqWidth, reqHeight, mapX, mapY);

    destPtr = Blt_CreatePicture(reqWidth, reqHeight);
    destRowPtr = destPtr->bits;
    for (y = 0; y < reqHeight; y++) {
        Blt_Pixel *dp;
        Blt_Pixel *srcRowPtr;
        int x;

        dp = destRowPtr;
        srcRowPtr = srcPtr->bits + (srcPtr->pixelsPerRow * mapY[y]);
        for (x = 0; x < reqWidth; x++) {
            Blt_Pixel *sp;

            sp = srcRowPtr + mapX[x];
            dp->u32 = sp->u32;          /* Copy the pixel. */
            dp++;
        }
        destRowPtr += destPtr->pixelsPerRow;
    }
    Blt_Free(mapX), Blt_Free(mapY);
    destPtr->flags = (srcPtr->flags | BLT_PIC_DIRTY);
    return destPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ScalePictureArea --
 *
 *      Scales the region of the source picture to the size of the
 *      destination image.  This routine performs raw scaling of the image
 *      and unlike Blt_ResamplePhoto does not perform any antialiasing.
 *
 * Results:
 *      Returns the new resized picture.  The original picture is left
 *      intact.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture
Blt_ScalePictureArea(
    Pict *srcPtr,                       /* Source picture to be scaled. */
    int ax, int ay,                     /* Origin of area in source
                                           picture. */
    int aw, int ah,                     /* Dimension of area to be
                                           scaled. */
    int dw, int dh)                     /* Dimensions of the destination
                                           scaled image. */
{
    Pict *destPtr;
    Blt_Pixel *srcRowPtr, *destRowPtr;
    double xScale, yScale;
    int *mapX, *mapY;
    int x, y;

    xScale = (double)srcPtr->width / (double)dw;
    yScale = (double)srcPtr->height / (double)dh;
    mapX = Blt_AssertMalloc(sizeof(int) * aw);
    mapY = Blt_AssertMalloc(sizeof(int) * ah);

    /* Precompute scaling factors for each row and column. */
    for (x = 0; x < aw; x++) {
        int sx;

        sx = (int)(xScale * (double)(x + ax));
        if (sx >= srcPtr->width) {
            sx = srcPtr->width - 1;
        } 
        mapX[x] = sx;
    }
    for (y = 0; y < ah; y++) {
        int sy;

        sy = (int)(yScale * (double)(y + ay));
        if (sy > srcPtr->height) {
            sy = srcPtr->height - 1;
        } 
        mapY[y] = sy;
    }

    destPtr = Blt_CreatePicture(aw, ah);
    destRowPtr = destPtr->bits;
    for (y = 0; y < ah; y++) {
        Blt_Pixel *dp;

        dp = destRowPtr;
        srcRowPtr = srcPtr->bits + (srcPtr->pixelsPerRow * mapY[y]);
        for (x = 0; x < aw; x++) {
            Blt_Pixel *sp;

            sp = srcRowPtr + mapX[x];
            dp->u32 = sp->u32;          /* Copy the pixel. */
            dp++;
        }
        destRowPtr += destPtr->pixelsPerRow;
    }
    Blt_Free(mapX), Blt_Free(mapY);
    return destPtr;
}


/* 
 *---------------------------------------------------------------------------
 *
 * ShearY --
 *
 *      Shears a row horizontally.  Antialiasing limited to filtering two
 *      adjacent pixels.  So the shear angle must be between +-45 degrees.
 *      
 * Results:   
 *      None.
 *
 * Side Effects:
 *      The sheared image is drawn into the destination picture.
 *
 *---------------------------------------------------------------------------
 */
static void 
ShearY(
    Pict *destPtr, 
    Pict *srcPtr,
    int y,                              /* Designates the row to be
                                           sheared */
    int offset,                         /* Difference between of.  Note:
                                         * don't assume that offset is
                                         * always positive.  */
    double frac,
    Blt_Pixel *bg)
{
    Blt_Pixel *sp, *dp;
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int x, dx;
    Blt_Pixel left;
    unsigned char alpha;
    int t;

    assert(frac >= 0.0 && frac <= 1.0);
    destRowPtr = destPtr->bits + (y * destPtr->pixelsPerRow);
    srcRowPtr = srcPtr->bits + (y * srcPtr->pixelsPerRow);

    for (dp = destRowPtr, x = 0; x < offset; x++, dp++) { 
        dp->u32 = bg->u32;
    }
    dp = destRowPtr + offset;
    sp = srcRowPtr;
    dx = offset;

    alpha = (unsigned char)(frac * 255.0 + 0.5);
    left.Red   = imul8x8(alpha, bg->Red, t);
    left.Green = imul8x8(alpha, bg->Green, t);
    left.Blue  = imul8x8(alpha, bg->Blue, t);
    left.Alpha = imul8x8(alpha, bg->Alpha, t);

    for (x = 0; x < srcPtr->width; x++, dx++) { /* Loop through row pixels */
        Blt_Pixel right;
        int t;

        right.Red   = imul8x8(alpha, sp->Red, t);
        right.Green = imul8x8(alpha, sp->Green, t);
        right.Blue  = imul8x8(alpha, sp->Blue, t);
        right.Alpha = imul8x8(alpha, sp->Alpha, t);
        if ((dx >= 0) && (dx < destPtr->width)) {
            int r, b, g, a;

            r = sp->Red   - (right.Red   - left.Red);
            g = sp->Green - (right.Green - left.Green);
            b = sp->Blue  - (right.Blue  - left.Blue);
            a = sp->Alpha - (right.Alpha - left.Alpha);
            if (sp->Alpha == 0) {
                a = 0;
            }
            dp->Red   = UCLAMP(r);
            dp->Green = UCLAMP(g);
            dp->Blue  = UCLAMP(b);
            dp->Alpha = UCLAMP(a);
        }
        left.u32 = right.u32;
        sp++, dp++;
    }
    x = srcPtr->width + offset;  
    dp = destPtr->bits + (y * destPtr->pixelsPerRow) + x;
    if (x < destPtr->width) {
        dp->u32 = left.u32;
        dp++;
    }
    for (x++; x < destPtr->width; x++, dp++) {
        dp->u32 = bg->u32;
    }
}  

/* 
 *---------------------------------------------------------------------------
 *
 * ShearX --
 *
 *      Shears a column. Antialiasing is limited to filtering two adjacent
 *      pixels.  So the shear angle must be between +-45 degrees.
 *      
 * Results:   
 *      None.
 *
 * Side Effects:
 *      The sheared image is drawn into the destination picture.
 *
 * -------------------------------------------------------------------------- 
 */
static void
ShearX(
    Pict *destPtr, 
    Pict *srcPtr,
    int x,                              /* Column in source image to be
                                         * sheared. */
    int offset,                         /* Offset of */
    double frac,                        /* Fraction of subpixel. */
    Blt_Pixel *bg)
{
    Blt_Pixel *sp, *dp;
    int y, dy;
    unsigned char alpha;
    Blt_Pixel left;                     /* Old left values of shear. */
    int t;

    assert(frac >= 0.0 && frac <= 1.0);
    dp = destPtr->bits + x;
    for (y = 0; y < offset; y++) {
        dp->u32 = bg->u32;
        dp += destPtr->pixelsPerRow;
    }


    dp = destPtr->bits + (offset * destPtr->pixelsPerRow) + x;
    sp = srcPtr->bits + x;
    alpha = (unsigned char)(frac * 255.0 + 0.5);
    left.Red   = imul8x8(alpha, bg->Red, t);
    left.Green = imul8x8(alpha, bg->Green, t);
    left.Blue  = imul8x8(alpha, bg->Blue, t);
    left.Alpha = imul8x8(alpha, bg->Alpha, t);
    for (dy = offset, y = 0; y < srcPtr->height; y++, dy++) {
        Blt_Pixel right;
        int t;

        right.Red   = imul8x8(alpha, sp->Red, t);
        right.Green = imul8x8(alpha, sp->Green, t);
        right.Blue  = imul8x8(alpha, sp->Blue, t);
        right.Alpha = imul8x8(alpha, sp->Alpha, t);
        if ((dy >= 0) && (dy < destPtr->height)) {
            int r, g, b, a;

            r = sp->Red   - (right.Red   - left.Red);
            g = sp->Green - (right.Green - left.Green);
            b = sp->Blue  - (right.Blue  - left.Blue);
            a = sp->Alpha - (right.Alpha - left.Alpha);
            if (sp->Alpha == 0) {
                a = 0;
            }
            dp->Red   = UCLAMP(r);
            dp->Green = UCLAMP(g);
            dp->Blue  = UCLAMP(b);
            dp->Alpha = UCLAMP(a);
        }
        left.u32 = right.u32;
        sp += srcPtr->pixelsPerRow; 
        dp += destPtr->pixelsPerRow;
    }
    y = srcPtr->height + offset;  

    dp = destPtr->bits + (y * destPtr->pixelsPerRow) + x;
    if (y < destPtr->height) {
        dp->u32 = left.u32;
        dp += destPtr->pixelsPerRow;
    }
    
    for (y++; y < destPtr->height; y++) {
        dp->u32 = bg->u32;
        dp += destPtr->pixelsPerRow; 
    }
}  

/* 
 *---------------------------------------------------------------------------
 *
 * Rotate45 --
 *
 *      Rotates an image by a given angle.  The angle must be in the range
 *      -45.0 to 45.0 inclusive.  Anti-aliasing filtering is performed on
 *      two adjacent pixels, so the angle can't be so great as to force a
 *      sheared pixel to occupy 3 destination pixels.  Performs a three
 *      shear rotation described below.
 *
 *      Reference: Alan W. Paeth, "A Fast Algorithm for General Raster
 *                 Rotation", Graphics Gems, pp 179-195.  
 *
 *
 * Results:  
 *      Returns a newly allocated rotated image.
 *
 * -------------------------------------------------------------------------- 
 */
static Blt_Picture
Rotate45(Pict *srcPtr, float angle, Blt_Pixel *bg)
{
    Pict *shear1Ptr, *shear2Ptr, *destPtr;
    double  skewf;
    double sinTheta, cosTheta, tanTheta;
    int skewi;
    int shearWidth, shearHeight;
    int x, y;

    sinTheta = sin(angle);
    cosTheta = cos(angle);
    tanTheta = tan(angle * 0.5);
    
    shearWidth = srcPtr->width + (int)(srcPtr->height * FABS(tanTheta));
    shearHeight = srcPtr->height;

    /* 1st shear */

    shear1Ptr = Blt_CreatePicture(shearWidth, shearHeight);
    if (tanTheta >= 0.0) {              /* Positive angle */
        for (y = 0; y < shearHeight; y++) {  
            skewf = (y + 0.5) * tanTheta;
            skewi = (int)floor(skewf);
            ShearY(shear1Ptr, srcPtr, y, skewi, skewf - skewi, bg);
        }
    } else {                            /* Negative angle */
        for (y = 0; y < shearHeight; y++) {  
            skewf = ((y - srcPtr->height) + 0.5) * tanTheta;
            skewi = (int)floor(skewf);
            ShearY(shear1Ptr, srcPtr, y, skewi, skewf - skewi, bg);
        }
    }
    shearHeight = 
        (int)(srcPtr->width * FABS(sinTheta) + srcPtr->height * cosTheta) + 1;

    shear2Ptr = Blt_CreatePicture(shearWidth, shearHeight);
    /* 2nd shear */

    if (sinTheta > 0.0) {               /* Positive angle */
        skewf = (srcPtr->width - 1) * sinTheta;
    } else {                            /* Negative angle */
        skewf = (srcPtr->width - shearWidth) * -sinTheta;
    }
    for (x = 0; x < shearWidth; x++) {
        skewi = (int)floor(skewf);
        ShearX(shear2Ptr, shear1Ptr, x, skewi, skewf - skewi, bg);
        skewf -= sinTheta;
    }

    Blt_FreePicture(shear1Ptr);

    /* 3rd shear */

    shearWidth = 
        (int)(srcPtr->height * FABS(sinTheta) + srcPtr->width * cosTheta) + 1;

    destPtr = Blt_CreatePicture(shearWidth, shearHeight);
    if (sinTheta >= 0.0) {              /* Positive angle */
        skewf = (srcPtr->width - 1) * sinTheta * -tanTheta;
    } else {                            /* Negative angle */
        skewf = tanTheta * ((srcPtr->width - 1) * -sinTheta - (shearHeight-1));
    }
    for (y = 0; y < shearHeight; y++) {
        skewi = (int)floor(skewf);
        ShearY(destPtr, shear2Ptr, y, skewi, skewf - skewi, bg);
        skewf += tanTheta;
    }
    Blt_FreePicture(shear2Ptr);
    destPtr->flags |= BLT_PIC_COMPOSITE;
    return destPtr;      
}

/* 
 *---------------------------------------------------------------------------
 *
 * Rotate90 --
 *
 *      Rotates the given picture by 90 degrees.  This is part of the
 *      special case right-angle rotations that do not create subpixel
 *      aliasing.
 *
 * Results:  
 *      Returns a newly allocated, rotated picture.
 *
 * -------------------------------------------------------------------------- 
 */
static Blt_Picture
Rotate90(Pict *srcPtr)
{
    Pict *destPtr;
    Blt_Pixel *srcRowPtr;
    int offset;
    int x, y;

    destPtr = Blt_CreatePicture(srcPtr->height, srcPtr->width);
    offset = (destPtr->height - 1) * destPtr->pixelsPerRow;
    srcRowPtr = srcPtr->bits;
    for (x = 0; x < destPtr->width; x++) {
        Blt_Pixel *dp, *sp;

        sp = srcRowPtr;
        dp = destPtr->bits + offset + x;
        for (y = 0; y < destPtr->height; y++) {
            *dp = *sp++;
            dp -= destPtr->pixelsPerRow;
        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    destPtr->flags = srcPtr->flags;
    return destPtr;
}

/* 
 *---------------------------------------------------------------------------
 *
 * Rotate180 --
 *
 *      Rotates the given picture by 180 degrees.  This is one of the
 *      special case orthogonal rotations that do not create subpixel
 *      aliasing.
 *
 * Results:  
 *      Returns a newly allocated, rotated picture.
 *
 * -------------------------------------------------------------------------- 
 */
static Blt_Picture
Rotate180(Pict *srcPtr)
{
    Pict *destPtr;
    Blt_Pixel *srcRowPtr;
    int offset;
    int x, y;

    destPtr = Blt_CreatePicture(srcPtr->width, srcPtr->height);
    offset = (destPtr->height - 1) * destPtr->pixelsPerRow;
    srcRowPtr = srcPtr->bits;
    for (y = 0; y < destPtr->height; y++) {
        Blt_Pixel *dp, *sp;

        sp = srcRowPtr;
        dp = destPtr->bits + offset + destPtr->width - 1;
        for (x = 0; x < destPtr->width; x++) {
            *dp-- = *sp++;
        }
        offset -= destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    destPtr->flags = srcPtr->flags;
    return destPtr;
}

/* 
 *---------------------------------------------------------------------------
 *
 * Rotate270 --
 *
 *      Rotates the given picture by 270 degrees.  This is part of the
 *      special case right-angle rotations that do not create subpixel
 *      aliasing.
 *
 * Results:  
 *      Returns a newly allocated, rotated picture.
 *
 * -------------------------------------------------------------------------- 
 */
static Blt_Picture
Rotate270(Pict *srcPtr)
{
    Pict *destPtr;
    Blt_Pixel *srcRowPtr;
    int x, y;

    destPtr = Blt_CreatePicture(srcPtr->height, srcPtr->width);

    srcRowPtr = srcPtr->bits;
    for (x = destPtr->width - 1; x >= 0; x--) {
        Blt_Pixel *sp, *dp;

        sp = srcRowPtr;
        dp = destPtr->bits + x;
        for (y = 0; y < destPtr->height; y++) {
            *dp = *sp++;
            dp += destPtr->pixelsPerRow;
        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    destPtr->flags = srcPtr->flags;
    return destPtr;
}

static void
GetRotatedSize(int w, int h, float angle, int *rotWidthPtr, int *rotHeightPtr)
{
    Point2d corner[4];
    double sinTheta, cosTheta;
    double radians;
    double xMax, yMax;
    int i;

    
    /* Set the four corners of the rectangle whose center is the origin. 
     *  0---1
     *  |   |
     *  3---2
    */
    corner[1].x = corner[2].x = (double)w * 0.5;
    corner[0].x = corner[3].x = -corner[1].x;
    corner[2].y = corner[3].y = (double)h * 0.5;
    corner[0].y = corner[1].y = -corner[2].y;

    radians = -angle * DEG2RAD;
    sinTheta = sin(radians), cosTheta = cos(radians);
    xMax = yMax = 0.0;

    /* Rotate the four corners and find the maximum X and Y coordinates */

    for (i = 0; i < 4; i++) {
        double x, y;

        x = (corner[i].x * cosTheta) - (corner[i].y * sinTheta);
        y = (corner[i].x * sinTheta) + (corner[i].y * cosTheta);
        if (x > xMax) {
            xMax = x;
        }
        if (y > yMax) {
            yMax = y;
        }
    }
    /*
     * By symmetry, the width and height of the bounding box are twice the
     * maximum x and y coordinates.
     */
    *rotWidthPtr = ROUND(xMax + xMax);
    *rotHeightPtr = ROUND(yMax + yMax);
}

/* 
 *---------------------------------------------------------------------------
 *
 * RotateByAreaMapping --
 *
 *      Rotates the given picture using area mapping.  
 *
 * Results:  
 *      Returns a newly allocated, rotated picture.
 *
 *      Copyright (C) 2001 Leptonica.  All rights reserved.  This software
 *      is distributed in the hope that it will be useful, but with NO
 *      WARRANTY OF ANY KIND.
 *
 *      No author or distributor accepts responsibility to anyone for the
 *      consequences of using this software, or for whether it serves any
 *      particular purpose or works at all, unless he or she says so in
 *      writing.  Everyone is granted permission to copy, modify and
 *      redistribute this source code, for commercial or non-commercial
 *      purposes, with the following restrictions: (1) the origin of this
 *      source code must not be misrepresented; (2) modified versions must
 *      be plainly marked as such; and (3) this notice may not be removed
 *      or altered from any source or modified source distribution.
 *
 * -------------------------------------------------------------------------- 
 */
static Pict *
RotateByAreaMapping(Pict *srcPtr, float angle, Blt_Pixel *bg)
{
    int y, wm2, hm2;
    float  sinTheta, cosTheta;
    Blt_Pixel *destRowPtr;
    Pict *destPtr;
    int rotWidth, rotHeight;
    int srcCx, srcCy, destCx, destCy;   /* Centers of source and
                                         * destination picture. */

    /* Find the new dimensions required to hold the image after rotation */
    GetRotatedSize(srcPtr->width, srcPtr->height, angle, &rotWidth, 
                   &rotHeight);
    destPtr = Blt_CreatePicture(rotWidth, rotHeight);
    srcCx = srcPtr->width / 2;
    srcCy = srcPtr->height / 2;
    destCx = destPtr->width / 2;
    destCy = destPtr->height / 2;
    wm2 = srcPtr->width - 2;
    hm2 = srcPtr->height - 2;
    sinTheta = 16.0 * sin(angle);
    cosTheta = 16.0 * cos(angle);

    destRowPtr = destPtr->bits;
    for (y = 0; y < destPtr->height; y++) {
        Blt_Pixel *dp, *dend;
        int deltaY;
        int x;

        deltaY = destCy - y;
        for (x = 0, dp = destRowPtr, dend = dp + destPtr->width; dp < dend; 
             dp++, x++) {
            int r, g, b, a;
            Blt_Pixel  *p00, *p01, *p10, *p11;
            Blt_Pixel *srcRowPtr;
            int srcX, srcY;
            int deltaX;
            int xpm, ypm, xf, yf;

            deltaX = destCx - x;
            xpm = (int)(-deltaX * cosTheta - deltaY * sinTheta);
            ypm = (int)(-deltaY * cosTheta + deltaX * sinTheta);
            srcX = srcCx + (xpm >> 4);
            srcY = srcCy + (ypm >> 4);
            xf = xpm & 0x0f;
            yf = ypm & 0x0f;

            /* If outside of the source image, use the default color. */
            if ((srcX < 0) || (srcY < 0) || (srcX > wm2) || (srcY > hm2)) {
                dp->u32 = bg->u32;
                continue;
            }
            srcRowPtr = srcPtr->bits + srcY * srcPtr->pixelsPerRow;
            /* do area weighting.  Without this, we would
             * simply do:
             *   *(lined + x) = *(lines + srcX);
             * which is faster but gives lousy results!
             */
            p00 = srcRowPtr + srcX;
            p01 = p00 + 1;
            p10 = p00 + srcPtr->pixelsPerRow;
            p11 = p01 + srcPtr->pixelsPerRow;
            r = ((16 - xf) * (16 - yf) * p00->Red + 
                 xf * (16 - yf) * p01->Red + (16 - xf) * yf * p10->Red +
                 xf * yf * p11->Red + 128) / 256;
            g = ((16 - xf) * (16 - yf) * p00->Green +
                 xf * (16 - yf) * p01->Green + (16 - xf) * yf * p10->Green +
                 xf * yf * p11->Green + 128) / 256;
            b = ((16 - xf) * (16 - yf) * p00->Blue +
                 xf * (16 - yf) * p01->Blue + (16 - xf) * yf * p10->Blue +
                 xf * yf * p11->Blue + 128) / 256;
            a = ((16 - xf) * (16 - yf) * p00->Alpha +
                 xf * (16 - yf) * p01->Alpha + (16 - xf) * yf * p10->Alpha +
                 xf * yf * p11->Alpha + 128) / 256;
            dp->Red   = UCLAMP(r);
            dp->Green = UCLAMP(g);
            dp->Blue  = UCLAMP(b);
            dp->Alpha = UCLAMP(a);
        }
        destRowPtr += destPtr->pixelsPerRow;
    }
    return destPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_RotatePictureByShear --
 *
 *      Rotates a picture by a given # of degrees.
 *
 * Results:
 *      Returns a newly allocated, rotated picture.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture
Blt_RotatePictureByShear(Pict *srcPtr, float angle)
{
    Pict *destPtr, *tmpPtr;
    int quadrant;
    
    tmpPtr = srcPtr;                    /* Suppress compiler warning. */

    /* Make the angle positive between 0 and 360 degrees. */ 
    angle = FMOD(angle, 360.0f);
    if (angle < 0.0) {
        angle += 360.0;
    }
    quadrant = ROTATE_0;
    if ((angle > 45.0) && (angle <= 135.0)) {
        quadrant = ROTATE_90;
        angle -= 90.0;
    } else if ((angle > 135.0) && (angle <= 225.0)) { 
        quadrant = ROTATE_180;
        angle -= 180.0;
    } else if ((angle > 225.0) && (angle <= 315.0)) { 
        quadrant = ROTATE_270;
        angle -= 270.0;
    } else if (angle > 315.0) {
        angle -= 360.0;
    }
    /* 
     * If necessary, create a temporary image that's rotated by a
     * right-angle.  We'll then rotate this picture between -45 to 45
     * degrees to arrive at its final angle.
     */
    switch (quadrant) {
    case ROTATE_270:                    /* 270 degrees */
        tmpPtr = Rotate270(srcPtr);
        break;

    case ROTATE_90:                     /* 90 degrees */
        tmpPtr = Rotate90(srcPtr);
        break;

    case ROTATE_180:                    /* 180 degrees */
        tmpPtr = Rotate180(srcPtr);
        break;

    case ROTATE_0:                      /* 0 degrees */
        if (angle == 0.0) {
            tmpPtr = Blt_ClonePicture(srcPtr); /* Make a copy of the source. */
        } 
        break;
    }

    assert((angle >= -45.0) && (angle <= 45.0));

    destPtr = tmpPtr;
    if (angle != 0.0) {
        Blt_Pixel bg;

        bg.u32 = 0x00000000;
        angle *= DEG2RAD;
        destPtr = Rotate45(tmpPtr, angle, &bg);
        if (tmpPtr != srcPtr) {
            Blt_FreePicture(tmpPtr);
        }
    } 
    return destPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_RotatePicture --
 *
 *      Rotates a picture by a given # of degrees.
 *
 * Results:
 *      Returns a newly allocated, rotated picture.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture
Blt_RotatePicture(Pict *srcPtr, float angle)
{
    Pict *destPtr;
    int angleInt;
    Blt_Pixel bg;

    bg.u32 = 0x00000000;

    /* Make the angle positive between 0 and 360 degrees. */ 
    angle = FMOD(angle, 360.0f);
    if (angle < 0.0) {
        angle += 360.0;
    }
    /* 
     * Handle the easy cases.
     */
    angleInt = (int)angle;
    if ((angle - angleInt) < 0.05) {
        switch (angleInt) {
        case 270:                       /* 270 degrees */
            return Rotate270(srcPtr);
            break;
            
        case 90:                        /* 90 degrees */
            return Rotate90(srcPtr);
            break;
            
        case 180:                       /* 180 degrees */
            return Rotate180(srcPtr);
            break;
            
        case ROTATE_0:                  /* 0 degrees */
            if (angle == 0.0) {
                /* Just make a copy of the source. */
                return Blt_ClonePicture(srcPtr);
            } 
            break;
        }
    }
    destPtr = RotateByAreaMapping(srcPtr, -angle, &bg);
    destPtr->flags |= BLT_PIC_COMPOSITE;
    return destPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FlipPicture --
 *
 *      Flips a picture vertically or horizonally.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_FlipPicture(Pict *srcPtr, int vertically)
{
    if (vertically) {
        Blt_Pixel *s1RowPtr, *s2RowPtr;
        int y;
        
        s1RowPtr = srcPtr->bits;
        s2RowPtr = srcPtr->bits + ((srcPtr->height - 1) * srcPtr->pixelsPerRow);
        for (y = 0; y < srcPtr->height / 2; y++) {
            Blt_Pixel *s1, *s2, *send;
            
            s1 = s1RowPtr, s2 = s2RowPtr;
            for (send = s1 + srcPtr->width; s1 < send; s1++, s2++) {
                Blt_Pixel tmp;
                
                tmp.u32 = s1->u32;
                s1->u32 = s2->u32;
                s2->u32 = tmp.u32;
            }
            s1RowPtr += srcPtr->pixelsPerRow;
            s2RowPtr -= srcPtr->pixelsPerRow;
        }
    } else {
        Blt_Pixel *s1ColumnPtr, *s2ColumnPtr;
        int x;
        
        s1ColumnPtr = srcPtr->bits;
        s2ColumnPtr = srcPtr->bits + (srcPtr->width - 1);
        for (x = 0; x < srcPtr->width / 2; x++) {
            Blt_Pixel *s1, *s2, *send;
            
            s1 = s1ColumnPtr, s2 = s2ColumnPtr;
            for (send = s1 + (srcPtr->height * srcPtr->pixelsPerRow); s1 < send;
                 s1 += srcPtr->pixelsPerRow, s2 += srcPtr->pixelsPerRow) {
                Blt_Pixel tmp;
                
                tmp.u32 = s1->u32;
                s1->u32 = s2->u32;
                s2->u32 = tmp.u32;
            }
            s1ColumnPtr++;
            s2ColumnPtr--;
        }
    }
    srcPtr->flags |= BLT_PIC_DIRTY;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ReflectPicture --
 *
 *      Reflects a picture vertically or horizonally.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture
Blt_ReflectPicture(Pict *srcPtr, int side)
{
    Pict *destPtr;
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int alpha;
    int y;
    int w2, h2;

    h2 = w2 = 0;                        /* Suppress compiler warning. */
    if (side & (SIDE_BOTTOM | SIDE_TOP)) {
        h2 = srcPtr->height / 2;
        destPtr = Blt_CreatePicture(srcPtr->width, srcPtr->height + h2);
    } else {
        w2 = srcPtr->width / 2;
        destPtr = Blt_CreatePicture(srcPtr->width + w2, srcPtr->height);
    }
    switch (side) {
    case SIDE_BOTTOM:
        srcRowPtr = srcPtr->bits + (srcPtr->height - 1) * srcPtr->pixelsPerRow;
        destRowPtr = destPtr->bits + srcPtr->height * destPtr->pixelsPerRow;
        for (y = 0; y < h2; y++) {
            Blt_Pixel *sp, *dp, *dend;
            double t;

            t = (double)y / (h2 - 1);
            alpha = 255 - (int)((255.0 * t) + 0.5);
            alpha -= 80;
            alpha = UCLAMP(alpha);
            for (sp = srcRowPtr, dp = destRowPtr, dend = dp + destPtr->width; 
                 dp < dend; sp++, dp++) {
                dp->u32 = sp->u32;
                dp->Alpha = alpha;
            }
            destRowPtr += destPtr->pixelsPerRow;
            srcRowPtr -= srcPtr->pixelsPerRow;
        }
        break;
    case SIDE_TOP:
        Blt_CopyArea(destPtr, srcPtr, 0, 0, srcPtr->width, srcPtr->height,
                       0, h2);
        srcRowPtr = srcPtr->bits;
        destRowPtr = destPtr->bits + (h2 - 1) * destPtr->pixelsPerRow;
        for (y = 0; y < h2; y++) {
            Blt_Pixel *sp, *dp, *dend;
            double t;

            t = (double)y / (h2 - 1);
            alpha = 255 - (int)((255.0 * t) + 0.5);
            alpha -= 30;
            alpha = UCLAMP(alpha);
            for (sp = srcRowPtr, dp = destRowPtr, dend = dp + destPtr->width; 
                 dp < dend; sp++, dp++) {
                dp->u32 = sp->u32;
                dp->Alpha = alpha;
            }
            destRowPtr -= destPtr->pixelsPerRow;
            srcRowPtr += srcPtr->pixelsPerRow;
        }
        break;
    case SIDE_LEFT:
        Blt_CopyArea(destPtr, srcPtr, 0, 0, srcPtr->width, srcPtr->height,
                       w2, 0);
        srcRowPtr = srcPtr->bits;
        destRowPtr = destPtr->bits + (w2 - 1);
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *dp;
            int x;

            for (x = 0, sp = srcRowPtr, dp = destRowPtr; x < w2; 
                 sp++, dp--, x++) {
                double t;
                
                t = (double)x / (w2 - 1);
                alpha = 255 - (int)((255.0 * t) + 0.5);
                alpha -= 30;
                alpha = UCLAMP(alpha);
                dp->u32 = sp->u32;
                dp->Alpha = alpha;
            }
            destRowPtr += destPtr->pixelsPerRow;
            srcRowPtr += srcPtr->pixelsPerRow;
        }
        break;
    case SIDE_RIGHT:
        Blt_CopyArea(destPtr, srcPtr, 0, 0, srcPtr->width, srcPtr->height,
                       0, 0);
        srcRowPtr = srcPtr->bits + (srcPtr->width - 1);
        destRowPtr = destPtr->bits + srcPtr->width;
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *dp;
            int x;

            for (x = 0, sp = srcRowPtr, dp = destRowPtr; x < w2; 
                 sp++, dp++, x++) {
                double t;
                
                t = (double)x / (w2 - 1);
                alpha = 255 - (int)((255.0 * t) + 0.5);
                alpha -= 30;
                alpha = UCLAMP(alpha);
                dp->u32 = sp->u32;
                dp->Alpha = alpha;
            }
            destRowPtr += destPtr->pixelsPerRow;
            srcRowPtr += srcPtr->pixelsPerRow;
        }
        break;
    }
    destPtr->flags |= BLT_PIC_DIRTY | BLT_PIC_COMPOSITE;
    return destPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FadePictureWithGradient --
 *
 *      Reflects a picture vertically or horizonally.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_FadePictureWithGradient(Pict *srcPtr, int side, double low, double high,
                            int scale, Blt_Jitter *jitterPtr)
{
    Blt_Pixel *srcRowPtr;
    int alpha;
    int y;
    
    if (srcPtr->flags & BLT_PIC_PREMULT_COLORS) {
        Blt_UnmultiplyColors(srcPtr);
    }
    srcRowPtr = srcPtr->bits;

    switch (side) {
    case SIDE_BOTTOM:
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *send;
            double t;
            int alpha;
            
            /* 1..0 */
            t = 1.0 - ((double)y / (srcPtr->height - 1));
            if (scale == SCALE_LOG) {
                t = log10(9.0 * t + 1.0);
            }
            for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
                int a, t1;
                double m;
                
                m = t;
                if (jitterPtr->range > 0.0) {
                    m += Jitter(jitterPtr);
                    m = JCLAMP(m);
                }
                alpha = (int)((low + ((high - low) * m)) * 255.0 + 0.5);
                a = imul8x8(sp->Alpha, alpha, t1);
                sp->Alpha = UCLAMP(a);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
        }
        break;

    case SIDE_TOP:
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *send;
            double t;

            /* 0..1 */
            t = 1.0 - ((double)y / (srcPtr->height - 1));
            if (scale == SCALE_LOG) {
                t = log10(9.0 * t + 1.0);
            }
            for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
                int a, t1;
                double m;
                
                m = t;
                if (jitterPtr->range > 0.0) {
                    m += Jitter(jitterPtr);
                    m = JCLAMP(m);
                }
                alpha = (int)((low + ((high - low) * m)) * 255.0 + 0.5);
                a = imul8x8(sp->Alpha, alpha, t1);
                sp->Alpha = UCLAMP(a);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
        }
        break;

    case SIDE_LEFT:
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *send;
            int x;
            
            for (x = 0, sp = srcRowPtr, send = sp + srcPtr->width; sp < send; 
                 sp++, x++) {
                int a, alpha, t1;
                double t;

                /* 0..1 */
                t = (double)x / (srcPtr->width - 1);
                if (scale == SCALE_LOG) {
                    t = log10(9.0 * t + 1.0);
                }
                if (jitterPtr->range > 0.0) {
                    t += Jitter(jitterPtr);
                    t = JCLAMP(t);
                }
                alpha = (int)((low + ((high - low) * t)) * 255.0 + 0.5);
                a = imul8x8(sp->Alpha, alpha, t1);
                sp->Alpha = UCLAMP(a);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
        }
        break;

    case SIDE_RIGHT:
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *send;
            int x;
            
            for (x = 0, sp = srcRowPtr, send = sp + srcPtr->width; sp < send; 
                 sp++, x++) {
                int a, alpha, t1;
                double t;

                /* 1..0 */
                t = 1.0 - ((double)x / (srcPtr->width - 1));
                if (scale == SCALE_LOG) {
                    t = log10(9.0 * t + 1.0);
                }
                if (jitterPtr->range > 0.0) {
                    t += Jitter(jitterPtr);
                    t = JCLAMP(t);
                }
                alpha = (int)((low + ((high - low) * t)) * 255.0 + 0.5);
                a = imul8x8(sp->Alpha, alpha, t1);
                sp->Alpha = UCLAMP(a);
            }
            srcRowPtr += srcPtr->pixelsPerRow;
        }
        break;
    }
    srcPtr->flags |= BLT_PIC_DIRTY | BLT_PIC_COMPOSITE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ReflectPicture2 --
 *
 *      Reflects a picture vertically or horizonally.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture
Blt_ReflectPicture2(Pict *srcPtr, int side)
{
    Pict *destPtr;
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    destPtr = Blt_CreatePicture(srcPtr->width, srcPtr->height);
    switch (side) {
    case SIDE_BOTTOM:
        srcRowPtr = srcPtr->bits + (srcPtr->height - 1) * srcPtr->pixelsPerRow;
        destRowPtr = destPtr->bits;
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *dp, *dend;

            for (sp = srcRowPtr, dp = destRowPtr, dend = dp + destPtr->width; 
                 dp < dend; sp++, dp++) {
                dp->u32 = sp->u32;
            }
            destRowPtr += destPtr->pixelsPerRow;
            srcRowPtr -= srcPtr->pixelsPerRow;
        }
        break;
    case SIDE_TOP:
        srcRowPtr = srcPtr->bits;
        destRowPtr = destPtr->bits + (srcPtr->height-1) * destPtr->pixelsPerRow;
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *dp, *dend;

            for (sp = srcRowPtr, dp = destRowPtr, dend = dp + destPtr->width; 
                 dp < dend; sp++, dp++) {
                dp->u32 = sp->u32;
            }
            destRowPtr -= destPtr->pixelsPerRow;
            srcRowPtr += srcPtr->pixelsPerRow;
        }
        break;
    case SIDE_LEFT:
        srcRowPtr = srcPtr->bits;
        destRowPtr = destPtr->bits + (srcPtr->width - 1);
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *dp, *dend;

            for (sp = srcRowPtr, dp = destRowPtr, dend = dp + destPtr->width; 
                 dp < dend; sp++, dp--) {
                dp->u32 = sp->u32;
            }
            destRowPtr += destPtr->pixelsPerRow;
            srcRowPtr += srcPtr->pixelsPerRow;
        }
        break;
    case SIDE_RIGHT:
        srcRowPtr = srcPtr->bits + (srcPtr->width - 1);
        destRowPtr = destPtr->bits + srcPtr->width;
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *dp, *dend;

            for (sp = srcRowPtr, dp = destRowPtr, dend = dp + destPtr->width; 
                 dp < dend; sp++, dp++) {
                dp->u32 = sp->u32;
            }
            destRowPtr += destPtr->pixelsPerRow;
            srcRowPtr += srcPtr->pixelsPerRow;
        }
        break;
    }
    destPtr->flags = srcPtr->flags;
    return destPtr;
}

#define NC              256
enum ColorDirections { RED, GREEN, BLUE };

#define R0      (cubePtr->r0)
#define R1      (cubePtr->r1)
#define G0      (cubePtr->g0)
#define G1      (cubePtr->g1)
#define B0      (cubePtr->b0)
#define B1      (cubePtr->b1)

typedef struct {
    int r0, r1;                         /* min, max values: min exclusive
                                         * max inclusive */
    int g0, g1;
    int b0, b1;
    int vol;
} Cube;

/*
 *---------------------------------------------------------------------------
 *
 * Histogram is in elements 1..HISTSIZE along each axis, element 0 is for
 * base or marginal value NB: these must start out 0!
 * 
 *---------------------------------------------------------------------------
 */
typedef struct {
    long int wt[33][33][33];            /* # pixels in voxel */
    long int mR[33][33][33];            /* Sum over voxel of red pixel
                                           values */
    long int mG[33][33][33];            /* Sum over voxel of green pixel
                                         * values */
    long int mB[33][33][33];            /* Sum over voxel of blue pixel
                                         * values */
    float gm2[33][33][33];              /* Variance */
} PictStats;

/*
 * Build 3-D color histogram of counts, R/G/B, c^2
 */
static void
Hist3d(PictStats *s, Pict *srcPtr)
{
#define MAX_INTENSITIES 256
    Blt_Pixel *srcRowPtr;
    int y;
    float tab[MAX_INTENSITIES];

    {
        int i;

        /* Precompute table of squares. */
        for (i = 0; i < MAX_INTENSITIES; i++) {
            tab[i] = (float)(i * i);
        }
    }
    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp;
        int x;

        for (sp = srcRowPtr, x = 0; x < srcPtr->width; x++, sp++) {
            int r, g, b;

            /*
             * Reduce the number of bits (5) per color component. This will
             * keep the table size (2^15) reasonable without perceptually
             * affecting the final image.
             */
            r = (sp->Red >> 3) + 1;
            g = (sp->Green >> 3) + 1;
            b = (sp->Blue >> 3) + 1;
            
            s->wt[r][g][b] += 1;
            s->mR[r][g][b] += sp->Red;
            s->mG[r][g][b] += sp->Green;
            s->mB[r][g][b] += sp->Blue;
            s->gm2[r][g][b] += tab[sp->Red] + tab[sp->Green] + tab[sp->Blue];
        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  At conclusion of the histogram step, we can interpret
 *      wt[r][g][b] = sum over voxel of P(c)
 *      mR[r][g][b] = sum over voxel of r*P(c)  ,  similarly for mG, mB
 *      m2[r][g][b] = sum over voxel of c^2*P(c)
 * 
 *  Actually each of these should be divided by 'size' to give the usual
 *  interpretation of P() as ranging from 0 to 1, but we needn't do that
 *  here.
 * 
 *---------------------------------------------------------------------------
 */

/*
 *---------------------------------------------------------------------------
 * 
 *     We now convert histogram into moments so that we can rapidly
 *     calculate the sums of the above quantities over any desired box.
 * 
 *---------------------------------------------------------------------------
 */
/* Compute cumulative moments. */
static void
M3d(PictStats *s)
{
    unsigned char i, r, g, b, r0;
    long int area[33], rArea[33], gArea[33], bArea[33];
    float area2[33];

    for (r = 1, r0 = 0; r <= 32; r++, r0++) {
        for (i = 0; i <= 32; ++i) {
            area2[i] = 0.0f;
            area[i] = rArea[i] = gArea[i] = bArea[i] = 0;
        }
        for (g = 1; g <= 32; g++) {
            long int line, rLine, gLine, bLine;
            float line2;

            line2 = 0.0f;
            line = rLine = gLine = bLine = 0;
            for (b = 1; b <= 32; b++) {
                /* ind1 = RGBIndex(r, g, b); */

                line  += s->wt[r][g][b];
                rLine += s->mR[r][g][b];
                gLine += s->mG[r][g][b];
                bLine += s->mB[r][g][b];
                line2 += s->gm2[r][g][b];

                area[b] += line;
                rArea[b] += rLine;
                gArea[b] += gLine;
                bArea[b] += bLine;
                area2[b] += line2;

                /* ind2 = ind1 - 1089; [r0][g][b] */
                s->wt[r][g][b] = s->wt[r0][g][b] + area[b];
                s->mR[r][g][b] = s->mR[r0][g][b] + rArea[b];
                s->mG[r][g][b] = s->mG[r0][g][b] + gArea[b];
                s->mB[r][g][b] = s->mB[r0][g][b] + bArea[b];
                s->gm2[r][g][b] = s->gm2[r0][g][b] + area2[b];
            }
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  Vol --
 *
 *      Compute sum over a box of any given statistic
 *
 *---------------------------------------------------------------------------
 */
static INLINE long int
Vol(Cube *cubePtr, long int m[33][33][33])
{
    return (m[R1][G1][B1] - m[R1][G1][B0] - m[R1][G0][B1] + m[R1][G0][B0] -
            m[R0][G1][B1] + m[R0][G1][B0] + m[R0][G0][B1] - m[R0][G0][B0]);
}

/*
 *---------------------------------------------------------------------------
 * 
 * Bottom -- 
 *
 *      The next two routines allow a slightly more efficient calculation
 *      of Vol() for a proposed subbox of a given box.  The sum of Top()
 *      and Bottom() is the Vol() of a subbox split in the given direction
 *      and with the specified new upper bound.
 *
 *---------------------------------------------------------------------------
 */
/* Compute part of Vol(cubePtr, mmt) that doesn't depend on r1, g1, or
 * b1 */
/* (depending on dir) */
static long int
Bottom(
    Cube *cubePtr,
    enum ColorDirections dir,
    long int m[33][33][33])             /* Moment */
{
    switch (dir) {
    case RED:
        return -m[R0][G1][B1] + m[R0][G1][B0] + m[R0][G0][B1] - m[R0][G0][B0];
    case GREEN:
        return -m[R1][G0][B1] + m[R1][G0][B0] + m[R0][G0][B1] - m[R0][G0][B0];
    case BLUE:
        return -m[R1][G1][B0] + m[R1][G0][B0] + m[R0][G1][B0] - m[R0][G0][B0];
    }
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * Top --
 *
 *      Compute remainder of Vol(cubePtr, mmt), substituting pos for r1,
 *      g1, or b1 (depending on dir).
 *
 *---------------------------------------------------------------------------
 */
static long int
Top(Cube *cubePtr, enum ColorDirections dir, int pos, long int m[33][33][33])
{
    switch (dir) {
    case RED:
        return (m[pos][G1][B1] - m[pos][G1][B0] - 
                m[pos][G0][B1] + m[pos][G0][B0]);

    case GREEN:
        return (m[R1][pos][B1] - m[R1][pos][B0] - 
                m[R0][pos][B1] + m[R0][pos][B0]);

    case BLUE:
        return (m[R1][G1][pos] - m[R1][G0][pos] -
                m[R0][G1][pos] + m[R0][G0][pos]);
    }
    return 0;
}

/*
 *---------------------------------------------------------------------------
 * 
 * Var --
 *
 *      Compute the weighted variance of a box NB: as with the raw
 *      statistics, this is really the (variance * size)
 *
 *---------------------------------------------------------------------------
 */
static float
Var(Cube *cubePtr, PictStats *s)
{
    long int dR, dG, dB;
    float xx;

    dR = Vol(cubePtr, s->mR);
    dG = Vol(cubePtr, s->mG);
    dB = Vol(cubePtr, s->mB);
    xx = (s->gm2[R1][G1][B1] - s->gm2[R1][G1][B0] -
          s->gm2[R1][G0][B1] + s->gm2[R1][G0][B0] -
          s->gm2[R0][G1][B1] + s->gm2[R0][G1][B0] +
          s->gm2[R0][G0][B1] - s->gm2[R0][G0][B0]);
    return (xx - (float)(dR * dR + dG * dG + dB * dB) / (float)Vol(cubePtr, s->wt));
}

/*
 *---------------------------------------------------------------------------
 *
 * Maximize --
 *
 *      We want to minimize the sum of the variances of two subboxes.  The
 *      sum(c^2) terms can be ignored since their sum over both subboxes is
 *      the same (the sum for the whole box) no matter where we split.  The
 *      remaining terms have a minus sign in the variance formula, so we
 *      drop the minus sign and MAXIMIZE the sum of the two terms.
 *
 *---------------------------------------------------------------------------
 */
static float
Maximize(
    Cube *cubePtr,
    enum ColorDirections dir,
    int first, int last, int *cut,
    long rWhole, long gWhole, long bWhole, long wWhole,
    PictStats *s)
{
    long int rHalf, gHalf, bHalf, wHalf;
    long int rBase, gBase, bBase, wBase;
    int i;
    float temp, max;

    rBase = Bottom(cubePtr, dir, s->mR);
    gBase = Bottom(cubePtr, dir, s->mG);
    bBase = Bottom(cubePtr, dir, s->mB);
    wBase = Bottom(cubePtr, dir, s->wt);
    max = 0.0;
    *cut = -1;
    for (i = first; i < last; i++) {
        rHalf = rBase + Top(cubePtr, dir, i, s->mR);
        gHalf = gBase + Top(cubePtr, dir, i, s->mG);
        bHalf = bBase + Top(cubePtr, dir, i, s->mB);
        wHalf = wBase + Top(cubePtr, dir, i, s->wt);

        /* Now half_x is sum over lower half of box, if split at i */
        if (wHalf == 0) {               /* subbox could be empty of pixels! */
            continue;                   /* never split into an empty box */
        } else {
            temp = ((float)rHalf * rHalf + (float)gHalf * gHalf +
                    (float)bHalf * bHalf) / wHalf;
        }
        rHalf = rWhole - rHalf;
        gHalf = gWhole - gHalf;
        bHalf = bWhole - bHalf;
        wHalf = wWhole - wHalf;
        if (wHalf == 0) {               /* Subbox could be empty of pixels! */
            continue;                   /* never split into an empty box */
        } else {
            temp += ((float)rHalf * rHalf + (float)gHalf * gHalf +
                (float)bHalf * bHalf) / wHalf;
        }
        if (temp > max) {
            max = temp;
            *cut = i;
        }
    }
    return max;
}

/*
 *---------------------------------------------------------------------------
 *
 * Cut --
 *
 *---------------------------------------------------------------------------
 */
static int
Cut(Cube *set1, Cube *set2, PictStats *s)
{
    enum ColorDirections dir;
    int rCut, gCut, bCut;
    float rMax, gMax, bMax;
    long int rWhole, gWhole, bWhole, wWhole;

    rWhole = Vol(set1, s->mR);
    gWhole = Vol(set1, s->mG);
    bWhole = Vol(set1, s->mB);
    wWhole = Vol(set1, s->wt);

    rMax = Maximize(set1, RED, set1->r0 + 1, set1->r1, &rCut,
        rWhole, gWhole, bWhole, wWhole, s);
    gMax = Maximize(set1, GREEN, set1->g0 + 1, set1->g1, &gCut,
        rWhole, gWhole, bWhole, wWhole, s);
    bMax = Maximize(set1, BLUE, set1->b0 + 1, set1->b1, &bCut,
        rWhole, gWhole, bWhole, wWhole, s);

    if ((rMax >= gMax) && (rMax >= bMax)) {
        dir = RED;
        if (rCut < 0) {
            return 0;                   /* can't split the box */
        }
    } else {
        dir = ((gMax >= rMax) && (gMax >= bMax)) ? GREEN : BLUE;
    }
    set2->r1 = set1->r1;
    set2->g1 = set1->g1;
    set2->b1 = set1->b1;

    switch (dir) {
    case RED:
        set2->r0 = set1->r1 = rCut;
        set2->g0 = set1->g0;
        set2->b0 = set1->b0;
        break;

    case GREEN:
        set2->g0 = set1->g1 = gCut;
        set2->r0 = set1->r0;
        set2->b0 = set1->b0;
        break;

    case BLUE:
        set2->b0 = set1->b1 = bCut;
        set2->r0 = set1->r0;
        set2->g0 = set1->g0;
        break;
    }
    set1->vol = (set1->r1 - set1->r0) * (set1->g1 - set1->g0) *
        (set1->b1 - set1->b0);
    set2->vol = (set2->r1 - set2->r0) * (set2->g1 - set2->g0) *
        (set2->b1 - set2->b0);
    return 1;
}


/*
 *---------------------------------------------------------------------------
 *
 * SplitCS --
 *
 *---------------------------------------------------------------------------
 */
static int
SplitCS(PictStats *s, Cube *cubes, int numReqColors)
{
    float *vv, temp;
    int i;
    int next, k;
    int nc;

    vv = Blt_AssertMalloc(sizeof(float) * numReqColors);
    nc = numReqColors;
    cubes[0].r0 = cubes[0].g0 = cubes[0].b0 = 0;
    cubes[0].r1 = cubes[0].g1 = cubes[0].b1 = 32;
    next = 0;
    for (i = 1; i < numReqColors; i++) {
        if (Cut(cubes + next, cubes + i, s)) {
            /* Volume test ensures we won't try to cut one-cell box */
            vv[next] = (cubes[next].vol > 1) ? Var(cubes + next, s) : 0.0f;
            vv[i]    = (cubes[i].vol > 1)    ? Var(cubes + i, s)    : 0.0f;
        } else {
            vv[next] = 0.0f;            /* don't try to split this box again */
            i--;                        /* didn't create box i */
        }
        
        next = 0;
        temp = vv[0];
        for (k = 1; k <= i; k++) {
            if (vv[k] > temp) {
                temp = vv[k];
                next = k;
            }
        }
        if (temp <= 0.0) {
            nc = i + 1;
            Blt_Warn("Only got %d boxes\n", nc);
            break;
        }
    }
    Blt_Free(vv);
    return nc;
}

/*
 *---------------------------------------------------------------------------
 *
 * Mark --
 *
 *---------------------------------------------------------------------------
 */
static void
Mark(Cube *cubePtr, int label, Blt_ColorLookupTable tag)
{
    int r, g, b;

    for (r = R0 + 1; r <= R1; r++) {
        for (g = G0 + 1; g <= G1; g++) {
            for (b = B0 + 1; b <= B1; b++) {
                tag[r][g][b] = label;
            }
        }
    }
}

static Blt_ColorLookupTable
MakeCLuT(PictStats *s, Cube *cubes, int numColors)
{
    Blt_ColorLookupTable clut;
    Cube *cp, *cend;

    clut = Blt_AssertCalloc(sizeof(unsigned int), 33 * 33 * 33);
    for (cp = cubes, cend = cp + numColors; cp < cend; cp++) {
        unsigned int r, g, b;
        long int weight;
        Blt_Pixel pixel;

        weight = Vol(cp, s->wt);
        if (weight) {
            r = (unsigned int)((Vol(cp, s->mR) / (float)weight) * (NC + 1));
            g = (unsigned int)((Vol(cp, s->mG) / (float)weight) * (NC + 1));
            b = (unsigned int)((Vol(cp, s->mB) / (float)weight) * (NC + 1));
        } else {
#ifdef notdef
            Blt_Warn("bogus box %d\n", cp - cubes);
#endif
            r = g = b = 0;
        }
        pixel.Red = r / 257;
        pixel.Green = g / 257;
        pixel.Blue = b / 257;
        pixel.Alpha = ALPHA_OPAQUE;
        Mark(cp, pixel.u32, clut);
    }
    return clut;
}

void
Blt_MapColors(Pict *destPtr, Pict *srcPtr, Blt_ColorLookupTable clut)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    /* Apply the color lookup table against the original image */
    srcRowPtr = srcPtr->bits;    
    destRowPtr = destPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *dp, *sp, *send;
        
        sp = srcRowPtr, dp = destRowPtr;
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            int r, g, b, a;

            r = (sp->Red >> 3) + 1;
            g = (sp->Green >> 3) + 1;
            b = (sp->Blue >> 3) + 1;
            a = sp->Alpha;
            dp->u32 = clut[r][g][b];
            dp->Alpha = a;
            dp++;
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_QuantizePicture --
 *
 *      C Implementation of Wu's Color Quantizer (v. 2) (see Graphics Gems
 *      vol. II, pp. 126-133)
 *
 *      Author: Xiaolin Wu
 *              Dept. of Computer Science Univ. of Western
 *              Ontario London, Ontario
 *              N6A 5B7
 *              wu@csd.uwo.ca
 *
 *              Greedy orthogonal bipartition of RGB space for variance
 *              minimization aided by inclusion-exclusion tricks.  For
 *              speed no nearest neighbor search is done. Slightly better
 *              performance can be expected by more sophisticated but more
 *              expensive versions.
 *
 *              The author thanks Tom Lane at Tom_Lane@G.GP.CS.CMU.EDU for
 *              much of additional documentation and a cure to a previous
 *              bug.
 *
 *              Free to distribute, comments and suggestions are appreciated.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture
Blt_QuantizePicture(Pict *srcPtr, int numReqColors)
{
    Cube *cubes;
    PictStats *statsPtr;
    int nc;
    Blt_ColorLookupTable clut;
    Pict *destPtr;

    /*
     * Allocated a structure to hold color statistics.
     */
    statsPtr = Blt_AssertCalloc(1, sizeof(PictStats));
    Hist3d(statsPtr, srcPtr);
    M3d(statsPtr);

    cubes = Blt_AssertMalloc(sizeof(Cube) * numReqColors);
    nc = SplitCS(statsPtr, cubes, numReqColors);
    assert(nc <= numReqColors);

    clut = MakeCLuT(statsPtr, cubes, nc);
    Blt_Free(statsPtr);
    Blt_Free(cubes);
    destPtr = Blt_CreatePicture(srcPtr->width, srcPtr->height);
    Blt_MapColors(destPtr, srcPtr, clut);
    Blt_Free(clut);
    return destPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_QuantizePicture --
 *
 *      C Implementation of Wu's Color Quantizer (v. 2) (see Graphics Gems
 *      vol. II, pp. 126-133)
 *
 *      Author: Xiaolin Wu
 *              Dept. of Computer Science Univ. of Western
 *              Ontario London, Ontario
 *              N6A 5B7
 *              wu@csd.uwo.ca
 *
 *              Greedy orthogonal bipartition of RGB space for variance
 *              minimization aided by inclusion-exclusion tricks.  For
 *              speed no nearest neighbor search is done. Slightly better
 *              performance can be expected by more sophisticated but more
 *              expensive versions.
 *
 *              The author thanks Tom Lane at Tom_Lane@G.GP.CS.CMU.EDU for
 *              much of additional documentation and a cure to a previous
 *              bug.
 *
 *              Free to distribute, comments and suggestions are
 *              appreciated.
 *
 *---------------------------------------------------------------------------
 */
Blt_ColorLookupTable
Blt_GetColorLookupTable(Blt_Chain chain, int numReqColors)
{
    Cube *cubes;
    PictStats *statsPtr;
    int nc;
    Blt_ColorLookupTable clut;
    Blt_ChainLink link;

    /*
     * Allocated a structure to hold color statistics.
     */
    statsPtr = Blt_AssertCalloc(1, sizeof(PictStats));
    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        Pict *srcPtr;

        srcPtr = Blt_Chain_GetValue(link);
        Hist3d(statsPtr, srcPtr);
    }
    M3d(statsPtr);
    cubes = Blt_AssertMalloc(sizeof(Cube) * numReqColors);
    nc = SplitCS(statsPtr, cubes, numReqColors);
    assert(nc <= numReqColors);

    clut = MakeCLuT(statsPtr, cubes, nc);
    Blt_Free(statsPtr);
    Blt_Free(cubes);
    return clut;
}

/* 
 *---------------------------------------------------------------------------
 *
 * CopyArea --
 *
 *      Creates a copy of the given picture.  
 *
 * Results:  
 *      Returns the new copy.
 *
 * -------------------------------------------------------------------------- 
 */
static void
CopyArea(Pict *destPtr, Pict *srcPtr, int x, int y, int w, int h, int dx,
           int dy)
{
    int *srcRowPtr, *destRowPtr;
    int bottom;
    int dw, dh;
    int width, height;

    if (((dx + w) < 0) || ((dy + h) < 0)) {
        return;
    }
    if (dx < 0) {
        w += dx;
        dx = 0;
    }
    if (dy < 0) {
        h += dy;
        dy = 0;
    }
    if (destPtr->width < (dx + w)) {
        w = destPtr->width - dx;
    }
    if (destPtr->height < (dy + h)) {
        h = destPtr->height - dy;
    }
    if (srcPtr->width < w) {
        w = srcPtr->width;
    }
    if (srcPtr->height < h) {
        h = srcPtr->height;
    }

    dw = destPtr->width - dx;
    dh = destPtr->height - dy;

    width  = MIN(dw, w);
    height = MIN(dh, h);
    
    bottom = height + y;
    srcRowPtr = (int *)(srcPtr->bits + ((srcPtr->pixelsPerRow * y) + x));
    destRowPtr = (int *)(destPtr->bits + ((destPtr->pixelsPerRow * dy) + dx));

    for (/*empty*/; y < bottom; y++) {
        int n;
        int *sp, *dp;

        sp = srcRowPtr, dp = destRowPtr;
        n = (width + 7) / 8;            /* count > 0 assumed */
        switch (width & 0x07) {
        case 0:        do {  *dp++ = *sp++;
        case 7:              *dp++ = *sp++;
        case 6:              *dp++ = *sp++;
        case 5:              *dp++ = *sp++;
        case 4:              *dp++ = *sp++;
        case 3:              *dp++ = *sp++;
        case 2:              *dp++ = *sp++;
        case 1:              *dp++ = *sp++;
                        } while (--n > 0);
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
    destPtr->flags = (srcPtr->flags | BLT_PIC_DIRTY);
}

/* 
 *---------------------------------------------------------------------------
 *
 * CopyPictureBits --
 *
 *      Creates a copy of the given picture.  
 *
 * Results:  
 *      Returns the new copy.
 *
 * -------------------------------------------------------------------------- 
 */
static void
CopyPictureBits(Pict *destPtr, Pict *srcPtr)
{
    CopyArea(destPtr, srcPtr, 0, 0, srcPtr->width, srcPtr->height, 0, 0);
}

/*
 * Image dithering routines.
 *
 * Reference: 
 *      Victor Ostromoukhov http://www.iro.umontreal.ca/~ostrom/.  
 *
 *      Victor Ostromoukhov, "A Simple and Efficient Error-Diffusion
 *      Algorithm" SIGGRAPH'01
 *
 *      University of Montreal, http://www.iro.umontreal.ca/~ostrom/
 *
 */
typedef struct {
    short int r;                        /* Right */
    short int dl;                       /* Down-left */
    short int d;                        /* Down */
    short int sum;                      /* Sum */
} VarCoefs;

static VarCoefs coefTable[256] = {
    {    13,     0,     5,    18,   },  /*    0 */
    {    13,     0,     5,    18,   },  /*    1 */
    {    21,     0,    10,    31,   },  /*    2 */
    {     7,     0,     4,    11,   },  /*    3 */
    {     8,     0,     5,    13,   },  /*    4 */
    {    47,     3,    28,    78,   },  /*    5 */
    {    23,     3,    13,    39,   },  /*    6 */
    {    15,     3,     8,    26,   },  /*    7 */
    {    22,     6,    11,    39,   },  /*    8 */
    {    43,    15,    20,    78,   },  /*    9 */
    {     7,     3,     3,    13,   },  /*   10 */
    {   501,   224,   211,   936,   },  /*   11 */
    {   249,   116,   103,   468,   },  /*   12 */
    {   165,    80,    67,   312,   },  /*   13 */
    {   123,    62,    49,   234,   },  /*   14 */
    {   489,   256,   191,   936,   },  /*   15 */
    {    81,    44,    31,   156,   },  /*   16 */
    {   483,   272,   181,   936,   },  /*   17 */
    {    60,    35,    22,   117,   },  /*   18 */
    {    53,    32,    19,   104,   },  /*   19 */
    {   237,   148,    83,   468,   },  /*   20 */
    {   471,   304,   161,   936,   },  /*   21 */
    {     3,     2,     1,     6,   },  /*   22 */
    {   459,   304,   161,   924,   },  /*   23 */
    {    38,    25,    14,    77,   },  /*   24 */
    {   453,   296,   175,   924,   },  /*   25 */
    {   225,   146,    91,   462,   },  /*   26 */
    {   149,    96,    63,   308,   },  /*   27 */
    {   111,    71,    49,   231,   },  /*   28 */
    {    63,    40,    29,   132,   },  /*   29 */
    {    73,    46,    35,   154,   },  /*   30 */
    {   435,   272,   217,   924,   },  /*   31 */
    {   108,    67,    56,   231,   },  /*   32 */
    {    13,     8,     7,    28,   },  /*   33 */
    {   213,   130,   119,   462,   },  /*   34 */
    {   423,   256,   245,   924,   },  /*   35 */
    {     5,     3,     3,    11,   },  /*   36 */
    {   281,   173,   162,   616,   },  /*   37 */
    {   141,    89,    78,   308,   },  /*   38 */
    {   283,   183,   150,   616,   },  /*   39 */
    {    71,    47,    36,   154,   },  /*   40 */
    {   285,   193,   138,   616,   },  /*   41 */
    {    13,     9,     6,    28,   },  /*   42 */
    {    41,    29,    18,    88,   },  /*   43 */
    {    36,    26,    15,    77,   },  /*   44 */
    {   289,   213,   114,   616,   },  /*   45 */
    {   145,   109,    54,   308,   },  /*   46 */
    {   291,   223,   102,   616,   },  /*   47 */
    {    73,    57,    24,   154,   },  /*   48 */
    {   293,   233,    90,   616,   },  /*   49 */
    {    21,    17,     6,    44,   },  /*   50 */
    {   295,   243,    78,   616,   },  /*   51 */
    {    37,    31,     9,    77,   },  /*   52 */
    {    27,    23,     6,    56,   },  /*   53 */
    {   149,   129,    30,   308,   },  /*   54 */
    {   299,   263,    54,   616,   },  /*   55 */
    {    75,    67,    12,   154,   },  /*   56 */
    {    43,    39,     6,    88,   },  /*   57 */
    {   151,   139,    18,   308,   },  /*   58 */
    {   303,   283,    30,   616,   },  /*   59 */
    {    38,    36,     3,    77,   },  /*   60 */
    {   305,   293,    18,   616,   },  /*   61 */
    {   153,   149,     6,   308,   },  /*   62 */
    {   307,   303,     6,   616,   },  /*   63 */
    {     1,     1,     0,     2,   },  /*   64 */
    {   101,   105,     2,   208,   },  /*   65 */
    {    49,    53,     2,   104,   },  /*   66 */
    {    95,   107,     6,   208,   },  /*   67 */
    {    23,    27,     2,    52,   },  /*   68 */
    {    89,   109,    10,   208,   },  /*   69 */
    {    43,    55,     6,   104,   },  /*   70 */
    {    83,   111,    14,   208,   },  /*   71 */
    {     5,     7,     1,    13,   },  /*   72 */
    {   172,   181,    37,   390,   },  /*   73 */
    {    97,    76,    22,   195,   },  /*   74 */
    {    72,    41,    17,   130,   },  /*   75 */
    {   119,    47,    29,   195,   },  /*   76 */
    {     4,     1,     1,     6,   },  /*   77 */
    {     4,     1,     1,     6,   },  /*   78 */
    {     4,     1,     1,     6,   },  /*   79 */
    {     4,     1,     1,     6,   },  /*   80 */
    {     4,     1,     1,     6,   },  /*   81 */
    {     4,     1,     1,     6,   },  /*   82 */
    {     4,     1,     1,     6,   },  /*   83 */
    {     4,     1,     1,     6,   },  /*   84 */
    {     4,     1,     1,     6,   },  /*   85 */
    {    65,    18,    17,   100,   },  /*   86 */
    {    95,    29,    26,   150,   },  /*   87 */
    {   185,    62,    53,   300,   },  /*   88 */
    {    30,    11,     9,    50,   },  /*   89 */
    {    35,    14,    11,    60,   },  /*   90 */
    {    85,    37,    28,   150,   },  /*   91 */
    {    55,    26,    19,   100,   },  /*   92 */
    {    80,    41,    29,   150,   },  /*   93 */
    {   155,    86,    59,   300,   },  /*   94 */
    {     5,     3,     2,    10,   },  /*   95 */
    {     5,     3,     2,    10,   },  /*   96 */
    {     5,     3,     2,    10,   },  /*   97 */
    {     5,     3,     2,    10,   },  /*   98 */
    {     5,     3,     2,    10,   },  /*   99 */
    {     5,     3,     2,    10,   },  /*  100 */
    {     5,     3,     2,    10,   },  /*  101 */
    {     5,     3,     2,    10,   },  /*  102 */
    {     5,     3,     2,    10,   },  /*  103 */
    {     5,     3,     2,    10,   },  /*  104 */
    {     5,     3,     2,    10,   },  /*  105 */
    {     5,     3,     2,    10,   },  /*  106 */
    {     5,     3,     2,    10,   },  /*  107 */
    {   305,   176,   119,   600,   },  /*  108 */
    {   155,    86,    59,   300,   },  /*  109 */
    {   105,    56,    39,   200,   },  /*  110 */
    {    80,    41,    29,   150,   },  /*  111 */
    {    65,    32,    23,   120,   },  /*  112 */
    {    55,    26,    19,   100,   },  /*  113 */
    {   335,   152,   113,   600,   },  /*  114 */
    {    85,    37,    28,   150,   },  /*  115 */
    {   115,    48,    37,   200,   },  /*  116 */
    {    35,    14,    11,    60,   },  /*  117 */
    {   355,   136,   109,   600,   },  /*  118 */
    {    30,    11,     9,    50,   },  /*  119 */
    {   365,   128,   107,   600,   },  /*  120 */
    {   185,    62,    53,   300,   },  /*  121 */
    {    25,     8,     7,    40,   },  /*  122 */
    {    95,    29,    26,   150,   },  /*  123 */
    {   385,   112,   103,   600,   },  /*  124 */
    {    65,    18,    17,   100,   },  /*  125 */
    {   395,   104,   101,   600,   },  /*  126 */
    {     4,     1,     1,     6,   },  /*  127 */
    {     4,     1,     1,     6,   },  /*  128 */
    {   395,   104,   101,   600,   },  /*  129 */
    {    65,    18,    17,   100,   },  /*  130 */
    {   385,   112,   103,   600,   },  /*  131 */
    {    95,    29,    26,   150,   },  /*  132 */
    {    25,     8,     7,    40,   },  /*  133 */
    {   185,    62,    53,   300,   },  /*  134 */
    {   365,   128,   107,   600,   },  /*  135 */
    {    30,    11,     9,    50,   },  /*  136 */
    {   355,   136,   109,   600,   },  /*  137 */
    {    35,    14,    11,    60,   },  /*  138 */
    {   115,    48,    37,   200,   },  /*  139 */
    {    85,    37,    28,   150,   },  /*  140 */
    {   335,   152,   113,   600,   },  /*  141 */
    {    55,    26,    19,   100,   },  /*  142 */
    {    65,    32,    23,   120,   },  /*  143 */
    {    80,    41,    29,   150,   },  /*  144 */
    {   105,    56,    39,   200,   },  /*  145 */
    {   155,    86,    59,   300,   },  /*  146 */
    {   305,   176,   119,   600,   },  /*  147 */
    {     5,     3,     2,    10,   },  /*  148 */
    {     5,     3,     2,    10,   },  /*  149 */
    {     5,     3,     2,    10,   },  /*  150 */
    {     5,     3,     2,    10,   },  /*  151 */
    {     5,     3,     2,    10,   },  /*  152 */
    {     5,     3,     2,    10,   },  /*  153 */
    {     5,     3,     2,    10,   },  /*  154 */
    {     5,     3,     2,    10,   },  /*  155 */
    {     5,     3,     2,    10,   },  /*  156 */
    {     5,     3,     2,    10,   },  /*  157 */
    {     5,     3,     2,    10,   },  /*  158 */
    {     5,     3,     2,    10,   },  /*  159 */
    {     5,     3,     2,    10,   },  /*  160 */
    {   155,    86,    59,   300,   },  /*  161 */
    {    80,    41,    29,   150,   },  /*  162 */
    {    55,    26,    19,   100,   },  /*  163 */
    {    85,    37,    28,   150,   },  /*  164 */
    {    35,    14,    11,    60,   },  /*  165 */
    {    30,    11,     9,    50,   },  /*  166 */
    {   185,    62,    53,   300,   },  /*  167 */
    {    95,    29,    26,   150,   },  /*  168 */
    {    65,    18,    17,   100,   },  /*  169 */
    {     4,     1,     1,     6,   },  /*  170 */
    {     4,     1,     1,     6,   },  /*  171 */
    {     4,     1,     1,     6,   },  /*  172 */
    {     4,     1,     1,     6,   },  /*  173 */
    {     4,     1,     1,     6,   },  /*  174 */
    {     4,     1,     1,     6,   },  /*  175 */
    {     4,     1,     1,     6,   },  /*  176 */
    {     4,     1,     1,     6,   },  /*  177 */
    {     4,     1,     1,     6,   },  /*  178 */
    {   119,    47,    29,   195,   },  /*  179 */
    {    72,    41,    17,   130,   },  /*  180 */
    {    97,    76,    22,   195,   },  /*  181 */
    {   172,   181,    37,   390,   },  /*  182 */
    {     5,     7,     1,    13,   },  /*  183 */
    {    83,   111,    14,   208,   },  /*  184 */
    {    43,    55,     6,   104,   },  /*  185 */
    {    89,   109,    10,   208,   },  /*  186 */
    {    23,    27,     2,    52,   },  /*  187 */
    {    95,   107,     6,   208,   },  /*  188 */
    {    49,    53,     2,   104,   },  /*  189 */
    {   101,   105,     2,   208,   },  /*  190 */
    {     1,     1,     0,     2,   },  /*  191 */
    {   307,   303,     6,   616,   },  /*  192 */
    {   153,   149,     6,   308,   },  /*  193 */
    {   305,   293,    18,   616,   },  /*  194 */
    {    38,    36,     3,    77,   },  /*  195 */
    {   303,   283,    30,   616,   },  /*  196 */
    {   151,   139,    18,   308,   },  /*  197 */
    {    43,    39,     6,    88,   },  /*  198 */
    {    75,    67,    12,   154,   },  /*  199 */
    {   299,   263,    54,   616,   },  /*  200 */
    {   149,   129,    30,   308,   },  /*  201 */
    {    27,    23,     6,    56,   },  /*  202 */
    {    37,    31,     9,    77,   },  /*  203 */
    {   295,   243,    78,   616,   },  /*  204 */
    {    21,    17,     6,    44,   },  /*  205 */
    {   293,   233,    90,   616,   },  /*  206 */
    {    73,    57,    24,   154,   },  /*  207 */
    {   291,   223,   102,   616,   },  /*  208 */
    {   145,   109,    54,   308,   },  /*  209 */
    {   289,   213,   114,   616,   },  /*  210 */
    {    36,    26,    15,    77,   },  /*  211 */
    {    41,    29,    18,    88,   },  /*  212 */
    {    13,     9,     6,    28,   },  /*  213 */
    {   285,   193,   138,   616,   },  /*  214 */
    {    71,    47,    36,   154,   },  /*  215 */
    {   283,   183,   150,   616,   },  /*  216 */
    {   141,    89,    78,   308,   },  /*  217 */
    {   281,   173,   162,   616,   },  /*  218 */
    {     5,     3,     3,    11,   },  /*  219 */
    {   423,   256,   245,   924,   },  /*  220 */
    {   213,   130,   119,   462,   },  /*  221 */
    {    13,     8,     7,    28,   },  /*  222 */
    {   108,    67,    56,   231,   },  /*  223 */
    {   435,   272,   217,   924,   },  /*  224 */
    {    73,    46,    35,   154,   },  /*  225 */
    {    63,    40,    29,   132,   },  /*  226 */
    {   111,    71,    49,   231,   },  /*  227 */
    {   149,    96,    63,   308,   },  /*  228 */
    {   225,   146,    91,   462,   },  /*  229 */
    {   453,   296,   175,   924,   },  /*  230 */
    {    38,    25,    14,    77,   },  /*  231 */
    {   459,   304,   161,   924,   },  /*  232 */
    {     3,     2,     1,     6,   },  /*  233 */
    {   471,   304,   161,   936,   },  /*  234 */
    {   237,   148,    83,   468,   },  /*  235 */
    {    53,    32,    19,   104,   },  /*  236 */
    {    60,    35,    22,   117,   },  /*  237 */
    {   483,   272,   181,   936,   },  /*  238 */
    {    81,    44,    31,   156,   },  /*  239 */
    {   489,   256,   191,   936,   },  /*  240 */
    {   123,    62,    49,   234,   },  /*  241 */
    {   165,    80,    67,   312,   },  /*  242 */
    {   249,   116,   103,   468,   },  /*  243 */
    {   501,   224,   211,   936,   },  /*  244 */
    {     7,     3,     3,    13,   },  /*  245 */
    {    43,    15,    20,    78,   },  /*  246 */
    {    22,     6,    11,    39,   },  /*  247 */
    {    15,     3,     8,    26,   },  /*  248 */
    {    23,     3,    13,    39,   },  /*  249 */
    {    47,     3,    28,    78,   },  /*  250 */
    {     8,     0,     5,    13,   },  /*  251 */
    {     7,     0,     4,    11,   },  /*  252 */
    {    21,     0,    10,    31,   },  /*  253 */
    {    13,     0,     5,    18,   },  /*  254 */
    {    13,     0,     5,    18,   },  /*  255 */
};

static void 
DistributeError(
    double (*cl0)[3], double (*cl1)[3],
    int x, 
    double rDiff, double gDiff, double bDiff,
    int dir, Blt_Pixel *sp)
{
    VarCoefs *coefsPtr;
    double sum, r, dl, d;

    coefsPtr = coefTable + sp->Red;
    sum = coefsPtr->sum;
    r = coefsPtr->r * rDiff / sum;
    dl = coefsPtr->dl * rDiff / sum;
    d = rDiff - (r + dl);

    cl0[x + dir][0] += r;
    cl1[x - dir][0] += dl;
    cl1[x][0]       += d;

    coefsPtr = coefTable + sp->Green;
    sum = coefsPtr->sum;
    r = coefsPtr->r * gDiff / sum;
    dl = coefsPtr->dl * gDiff / sum;
    d = gDiff - (r + dl);

    cl0[x + dir][1] += r;
    cl1[x - dir][1] += dl;
    cl1[x][1]       += d;

    coefsPtr = coefTable + sp->Blue;
    sum = coefsPtr->sum;
    r = coefsPtr->r * bDiff / sum;
    dl = coefsPtr->dl * bDiff / sum;
    d = bDiff - (r + dl);

    cl0[x + dir][2] += r;
    cl1[x - dir][2] += dl;
    cl1[x][2]       += d;
}

#ifdef notdef
static void 
DistributeErrorFS(
    double (*cl0)[3], double (*cl1)[3],
    int x, 
    double rDiff, double gDiff, double bDiff,
    int dir, Blt_Pixel *sp)
{
    double d2, d3, d5, d7;

    d2 = rDiff + rDiff;
    d3 = rDiff + d2;
    d5 = d3 + d2;
    d7 = d5 + d2;

    cl0[x + dir][0] += (d7 / 16);
    cl1[x - dir][0] += (d3 / 16);
    cl1[x][0]       += (d5 / 16);
    cl1[x + dir][0] += (rDiff / 16);
    
    d2 = gDiff + gDiff;
    d3 = gDiff + d2;
    d5 = d3 + d2;
    d7 = d5 + d2;

    cl0[x + dir][1] += (d7 / 16);
    cl1[x - dir][1] += (d3 / 16);
    cl1[x][1]       += (d5 / 16);
    cl1[x + dir][1] += (gDiff / 16);

    d2 = bDiff + bDiff;
    d3 = bDiff + d2;
    d5 = d3 + d2;
    d7 = d5 + d2;

    cl0[x + dir][2] += (d7 / 16);
    cl1[x - dir][2] += (d3 / 16);
    cl1[x][2]       += (d5 / 16);
    cl1[x + dir][2] += (bDiff / 16);
}
#endif

static void 
ShiftCarryBuffers(double (**cl0)[3], double (**cl1)[3], int width)
{
    double (*tmp)[3];
    int i;

    /* Swap cl0 and cl1 */
    tmp = *cl0, *cl0 = *cl1, *cl1 = tmp;

    /* Clear c11 */
    for (i = 0; i < width; ++i) {
        (*cl1)[i][0] = (*cl1)[i][1] = (*cl1)[i][2] = 0.0;
    }
}

/* 
 *---------------------------------------------------------------------------
 *
 * Blt_DitherPicture --
 *
 *      Dithers a 24-bit picture using a given reduced-color palette. 
 *
 *      Reference: 
 *              Victor Ostromoukhov http://www.iro.umontreal.ca/~ostrom/.  
 *
 *              Victor Ostromoukhov, "A Simple and Efficient
 *              Error-Diffusion Algorithm" SIGGRAPH'01
 *
 *              University of Montreal, http://www.iro.umontreal.ca/~ostrom/
 *
 * Results:
 *      A new picture is allocated, dithered and returned. Returns NULL
 *      only if memory can't be allocated for the dithered picture.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture 
Blt_DitherPicture(Pict *srcPtr, Blt_Pixel *palette)
{
    Pict *destPtr;
    Blt_Pixel *srcRowPtr, *destRowPtr;
    double (*cl0)[3];
    double (*cl1)[3];
    int y;
    
    /* allocate carry_line_0 and carry_line_1 */
    cl0 = Blt_Calloc(srcPtr->width + 2, sizeof(*cl0));
    if (cl0 == NULL) {
        return NULL;
    }
    cl1 = Blt_Calloc(srcPtr->width + 2, sizeof(*cl1));
    if (cl1 == NULL) {
        Blt_Free(cl0);
        return NULL;
    }
    ++cl0;
    ++cl1;

    destPtr = Blt_CreatePicture(srcPtr->width, srcPtr->height);

    srcRowPtr = srcPtr->bits, destRowPtr = destPtr->bits;
    for (y = 0; y < srcPtr->height; ++y) {
        Blt_Pixel *sp, *dp;
        int start, finish, step;
        int x;

        if (y & 1) {
            start = srcPtr->width - 1;
            finish = -1;
            step = -1;
        } else {
            start = 0;
            finish = srcPtr->width;
            step = 1;
        }
        sp = srcRowPtr + start, dp = destRowPtr + start;
        for (x = start; x != finish; x += step) {
            double rCorrected, gCorrected, bCorrected;
            double rDiff, gDiff, bDiff;
            int rIntensity, gIntensity, bIntensity;

            rCorrected = sp->Red +   cl0[x][0];
            gCorrected = sp->Green + cl0[x][1];
            bCorrected = sp->Blue +  cl0[x][2];

            rIntensity = palette[(int)CLAMP(rCorrected)].Red;
            gIntensity = palette[(int)CLAMP(gCorrected)].Green;
            bIntensity = palette[(int)CLAMP(bCorrected)].Blue;

            rDiff = rCorrected - rIntensity;
            gDiff = gCorrected - gIntensity;
            bDiff = bCorrected - bIntensity;

            DistributeError(cl0, cl1, x, rDiff, gDiff, bDiff, step, sp); 
            dp->Red = rIntensity;
            dp->Green = gIntensity;
            dp->Blue = bIntensity;
            dp += step, sp += step;
        }
        ShiftCarryBuffers(&cl0, &cl1, srcPtr->width);
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
    Blt_Free(cl0 - 1);
    Blt_Free(cl1 - 1);
    return destPtr;
}

#ifdef notdef
static void
BoxX(Pict *destPtr, Pict *srcPtr)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    srcRowPtr = srcPtr->bits;
    destRowPtr = destPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *dp, *send;
        Blt_Pixel *lp, *cp, *rp;        /* Pointers to left, center, and
                                         * right pixels.  */
        Blt_Pixel hold;
        double r, g, b, a;

        dp = destRowPtr;
        cp = lp = srcRowPtr, rp = srcRowPtr + 1;
        r = (double)(lp->Red + cp->Red + rp->Red) * 0.333333333333333;
        g = (double)(lp->Green + cp->Green + rp->Green) * 0.333333333333333;
        b = (double)(lp->Blue + cp->Blue + rp->Blue) * 0.333333333333333;
        a = (double)(lp->Alpha + cp->Alpha + rp->Alpha) * 0.333333333333333;
        hold = *lp;
        dp->Red = (unsigned char)CLAMP(r);
        dp->Green = (unsigned char)CLAMP(g);
        dp->Blue = (unsigned char)CLAMP(b);
        dp->Alpha = (unsigned char)CLAMP(a);
        dp++, cp++, rp++;

        for (send = srcRowPtr + srcPtr->width; rp < send; rp++, cp++, lp++) {
            r = (double)(lp->Red + cp->Red + rp->Red) * 0.333333333333333;
            g = (double)(lp->Green + cp->Green + rp->Green) * 0.333333333333333;
            b = (double)(lp->Blue + cp->Blue + rp->Blue) * 0.333333333333333;
            a = (double)(lp->Alpha + cp->Alpha + rp->Alpha) * 0.333333333333333;
            dp->Red = (unsigned char)CLAMP(r);
            dp->Green = (unsigned char)CLAMP(g);
            dp->Blue = (unsigned char)CLAMP(b);
            dp->Alpha = (unsigned char)CLAMP(a);
            hold = *lp;
            dp++;
        }

        rp = cp;
        r = (double)(lp->Red + cp->Red + rp->Red) * 0.333333333333333;
        g = (double)(lp->Green + cp->Green + rp->Green) * 0.333333333333333;
        b = (double)(lp->Blue + cp->Blue + rp->Blue) * 0.333333333333333;
        a = (double)(lp->Alpha + cp->Alpha + rp->Alpha) * 0.333333333333333;
        dp->Red = (unsigned char)CLAMP(r);
        dp->Green = (unsigned char)CLAMP(g);
        dp->Blue = (unsigned char)CLAMP(b);
        dp->Alpha = (unsigned char)CLAMP(a);

        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
}
    
static void
BoxY(Pict *destPtr, Pict *srcPtr)
{
    Blt_Pixel *srcColPtr, *destColumnPtr;
    int x;

    srcColPtr = srcPtr->bits;
    destColumnPtr = destPtr->bits;
    for (x = 0; x < srcPtr->width; x++) {
        Blt_Pixel *dp, *rp, *lp, *cp, *send;
        double r, g, b, a;
        Blt_Pixel hold;

        dp = destColumnPtr;
        cp = lp = srcColPtr, rp = srcColPtr + srcPtr->pixelsPerRow;
        r = (lp->Red + cp->Red + rp->Red) * 0.333333333333333;
        g = (lp->Green + cp->Green + rp->Green) * 0.333333333333333;
        b = (lp->Blue + cp->Blue + rp->Blue) * 0.333333333333333;
        a = (lp->Alpha + cp->Alpha + rp->Alpha) * 0.333333333333333;
        hold = *lp;
        dp->Red = (unsigned char)CLAMP(r);
        dp->Green = (unsigned char)CLAMP(g);
        dp->Blue = (unsigned char)CLAMP(b);
        dp->Alpha = (unsigned char)CLAMP(a);
        dp += destPtr->pixelsPerRow;
        cp += srcPtr->pixelsPerRow;
        rp += srcPtr->pixelsPerRow;

        for (send = srcColPtr + (srcPtr->height * srcPtr->pixelsPerRow); 
             rp < send; /* empty */) {
            r = (lp->Red + cp->Red + rp->Red) * 0.333333333333333;
            g = (lp->Green + cp->Green + rp->Green) * 0.333333333333333;
            b = (lp->Blue + cp->Blue + rp->Blue) * 0.333333333333333;
            a = (lp->Alpha + cp->Alpha + rp->Alpha) * 0.333333333333333;
            hold = *lp;
            dp->Red = (unsigned char)CLAMP(r);
            dp->Green = (unsigned char)CLAMP(g);
            dp->Blue = (unsigned char)CLAMP(b);
            dp->Alpha = (unsigned char)CLAMP(a);
            dp += destPtr->pixelsPerRow;
            rp += srcPtr->pixelsPerRow;
            lp += srcPtr->pixelsPerRow; 
            cp += srcPtr->pixelsPerRow;
        }       
        rp = cp;
        r = (lp->Red + cp->Red + rp->Red) * 0.333333333333333;
        g = (lp->Green + cp->Green + rp->Green) * 0.333333333333333;
        b = (lp->Blue + cp->Blue + rp->Blue) * 0.333333333333333;
        a = (lp->Alpha + cp->Alpha + rp->Alpha) * 0.333333333333333;
        dp->Red = (unsigned char)CLAMP(r);
        dp->Green = (unsigned char)CLAMP(g);
        dp->Blue = (unsigned char)CLAMP(b);
        dp->Alpha = (unsigned char)CLAMP(a);
        srcColPtr++, destColumnPtr++;
    }
}
#endif

static void
TentHorizontally(Pict *destPtr, Pict *srcPtr)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    srcRowPtr = srcPtr->bits;
    destRowPtr = destPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel left, center, right;
        Blt_Pixel *dp, *sp, *send;

        dp = destRowPtr;
        sp = srcRowPtr + 1;
        left = *srcRowPtr, center = left, right = *sp;
        dp->Red = (left.Red + (center.Red << 1) + right.Red) >> 2;
        dp->Green = (left.Green + (center.Green << 1) + right.Green) >> 2;
        dp->Blue = (left.Blue + (center.Blue << 1) + right.Blue) >> 2;
        dp->Alpha = (left.Alpha + (center.Alpha << 1) + right.Alpha) >> 2;
        center = right;
        dp++, sp++;

        for (send = srcRowPtr + srcPtr->width; sp < send; /*empty*/) {
            right = *sp;
            dp->Red = (left.Red + (center.Red << 1) + right.Red) >> 2;
            dp->Green = (left.Green + (center.Green << 1) + right.Green) >> 2;
            dp->Blue = (left.Blue + (center.Blue << 1) + right.Blue) >> 2;
            dp->Alpha = (left.Alpha + (center.Alpha << 1) + right.Alpha) >> 2;
            left = center;
            center = right;
            dp++, sp++;
        }

        right = center;
        dp->Red = (left.Red + (center.Red << 1) + right.Red) >> 2;
        dp->Green = (left.Green + (center.Green << 1) + right.Green) >> 2;
        dp->Blue = (left.Blue + (center.Blue << 1) + right.Blue) >> 2;
        dp->Alpha = (left.Alpha + (center.Alpha << 1) + right.Alpha) >> 2;

        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
    return;
}

static void
TentVertically(Pict *destPtr, Pict *srcPtr)
{
    Blt_Pixel *srcColPtr, *destColumnPtr;
    int x;

    srcColPtr = srcPtr->bits;
    destColumnPtr = destPtr->bits;
    for (x = 0; x < srcPtr->width; x++) {
        Blt_Pixel left, center, right;
        Blt_Pixel *dp, *sp, *send;

        dp = destColumnPtr;
        sp = srcColPtr + srcPtr->pixelsPerRow;
        left = *srcColPtr, center = left, right = *sp;
        dp->Red = (left.Red + (center.Red << 1) + right.Red) >> 2;
        dp->Green = (left.Green + (center.Green << 1) + right.Green) >> 2;
        dp->Blue = (left.Blue + (center.Blue << 1) + right.Blue) >> 2;
        dp->Alpha = (left.Alpha + (center.Alpha << 1) + right.Alpha) >> 2;
        center = right;
        dp += destPtr->pixelsPerRow;
        sp += srcPtr->pixelsPerRow;

        for (send = srcColPtr + (srcPtr->height * srcPtr->pixelsPerRow); 
             sp < send; /* empty */) {
            right = *sp;
            dp->Red = (left.Red + (center.Red << 1) + right.Red) >> 2;
            dp->Green = (left.Green + (center.Green << 1) + right.Green) >> 2;
            dp->Blue = (left.Blue + (center.Blue << 1) + right.Blue) >> 2;
            dp->Alpha = (left.Alpha + (center.Alpha << 1) + right.Alpha) >> 2;
            left = center;
            center = right;
            dp += destPtr->pixelsPerRow;
            sp += srcPtr->pixelsPerRow;
        }       
        right = center;
        dp->Red = (left.Red + (center.Red << 1) + right.Red) >> 2;
        dp->Green = (left.Green + (center.Green << 1) + right.Green) >> 2;
        dp->Blue = (left.Blue + (center.Blue << 1) + right.Blue) >> 2;
        dp->Alpha = (left.Alpha + (center.Alpha << 1) + right.Alpha) >> 2;

        srcColPtr++, destColumnPtr++;
    }
}

static void
ApplyPictureToPicture(Pict *destPtr, Pict *srcPtr, int x, int y, 
                      int w, int h, int dx, int dy, Blt_PictureArithOps op)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    if ((x + w) > srcPtr->width) {
        w -= srcPtr->width - x;
    }
    if ((y + h) > srcPtr->height) {
        h -= srcPtr->height - y;
    }
    if ((dx + w) > destPtr->width) {
        w -= destPtr->width - dx;
    }
    if ((dy + h) > destPtr->height) {
        h -= destPtr->height - dy;
    }
    srcRowPtr = srcPtr->bits + (srcPtr->pixelsPerRow * y) + x;
    destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * dy) + dx;
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *dend;
        
        sp = srcRowPtr;
        dp = destRowPtr, dend = dp + w;
        switch(op) {

        case PIC_ARITH_ADD:
            while (dp < dend) {
                int i;
                i = dp->Red + sp->Red;
                dp->Red = (i > MAXINTENSITY) ? MAXINTENSITY : i;
                i = dp->Green + sp->Green;
                dp->Green = (i > MAXINTENSITY) ? MAXINTENSITY : i;
                i = dp->Blue + sp->Blue;
                dp->Blue =  (i > MAXINTENSITY) ? MAXINTENSITY : i;
                i = dp->Alpha + sp->Alpha;
                dp->Alpha = (i > MAXINTENSITY) ? MAXINTENSITY : i;
                sp++, dp++;
            }
            break;
            
        case PIC_ARITH_SUB:
            while (dp < dend) {
                int i;

                i = dp->Red - sp->Red;
                dp->Red =   (i < 0) ? 0 : i;
                i = dp->Green - sp->Green;
                dp->Green = (i < 0) ? 0 : i;
                i = dp->Blue - sp->Blue;
                dp->Blue =  (i < 0) ? 0 : i;
                i = dp->Alpha - sp->Alpha;
                dp->Alpha = (i < 0) ? 0 : i;
                sp++, dp++;
            }
            break;

        case PIC_ARITH_RSUB:
            while (dp < dend) {
                int i;

                i = sp->Red - dp->Red;
                dp->Red =   (i < 0) ? 0 : i;
                i = sp->Green - dp->Green;
                dp->Green = (i < 0) ? 0 : i;
                i = sp->Blue - dp->Blue;
                dp->Blue =  (i < 0) ? 0 : i;
                i = sp->Alpha - dp->Alpha;
                dp->Alpha = (i < 0) ? 0 : i;
                sp++, dp++;
            }
            break;

        case PIC_ARITH_AND:
            while (dp < dend) {
                dp->u32 &= sp->u32;
                sp++, dp++;
            }
            break;

        case PIC_ARITH_OR:
            while (dp < dend) {
                dp->u32 |= sp->u32;
                sp++, dp++;
            }
            break;

        case PIC_ARITH_XOR:
            while (dp < dend) {
                dp->u32 ^= sp->u32;
                sp++, dp++;
            }
            break;

        case PIC_ARITH_NAND:
            while (dp < dend) {
                dp->u32 = ~(dp->u32 & sp->u32);
                sp++, dp++;
            }
            break;

        case PIC_ARITH_NOR:
            while (dp < dend) {
                dp->u32 = ~(dp->u32 | sp->u32);
                sp++, dp++;
            }
            break;

        case PIC_ARITH_MIN:
            while (dp < dend) {
                dp->Red = MIN(dp->Red, sp->Red);
                dp->Green = MIN(dp->Green, sp->Green);
                dp->Blue = MIN(dp->Blue, sp->Blue);
                dp->Alpha = MIN(dp->Alpha, sp->Alpha);
                sp++, dp++;
            }
            break;

        case PIC_ARITH_MAX:
            while (dp < dend) {
                dp->Red = MAX(dp->Red, sp->Red);
                dp->Green = MAX(dp->Green, sp->Green);
                dp->Blue = MAX(dp->Blue, sp->Blue);
                dp->Alpha = MAX(dp->Alpha, sp->Alpha);
                sp++, dp++;
            }
            break;

        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
}

void
Blt_ApplyColorToPicture(Pict *srcPtr, Blt_Pixel *colorPtr)
{
    Blt_Pixel *srcRowPtr;
    int y;
    
    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;
        
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            if (sp->Alpha != 0x0) {
                sp->Red = colorPtr->Red;
                sp->Green = colorPtr->Green;
                sp->Blue = colorPtr->Blue;
            }
        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
}

static void
ApplyScalarToPicture(Pict *srcPtr, Blt_Pixel *colorPtr, Blt_PictureArithOps op)
{
    Blt_Pixel *srcRowPtr;
    int y;
    
    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;
        
        sp = srcRowPtr;
        send = sp + srcPtr->width;
        switch(op) {
            
        case PIC_ARITH_ADD:
            while (sp < send) {
                int i;
                
                i = sp->Red + colorPtr->Red;
                sp->Red = (i > MAXINTENSITY) ? MAXINTENSITY : i;
                i = sp->Green + colorPtr->Green;
                sp->Green = (i > MAXINTENSITY) ? MAXINTENSITY : i;
                i = sp->Blue + colorPtr->Blue;
                sp->Blue =  (i > MAXINTENSITY) ? MAXINTENSITY : i;
                i = sp->Alpha + colorPtr->Alpha;
                sp->Alpha = (i > MAXINTENSITY) ? MAXINTENSITY : i;
                sp++;
            }
            break;

        case PIC_ARITH_SUB:
            while (sp < send) {
                int i;

                i = sp->Red - colorPtr->Red;
                sp->Red =   (i < 0) ? 0 : i;
                i = sp->Green - colorPtr->Green;
                sp->Green = (i < 0) ? 0 : i;
                i = sp->Blue - colorPtr->Blue;
                sp->Blue =  (i < 0) ? 0 : i;
                i = sp->Alpha - colorPtr->Alpha;
                sp->Alpha = (i < 0) ? 0 : i;
                sp++;
            }
            break;

        case PIC_ARITH_RSUB:
            while (sp < send) {
                int i;

                i = colorPtr->Red - sp->Red;
                sp->Red =   (i < 0) ? 0 : i;
                i = colorPtr->Green - sp->Green;
                sp->Green = (i < 0) ? 0 : i;
                i = colorPtr->Blue - sp->Blue;
                sp->Blue =  (i < 0) ? 0 : i;
                i = colorPtr->Alpha - sp->Alpha;
                sp->Alpha = (i < 0) ? 0 : i;
                sp++;
            }
            break;

        case PIC_ARITH_AND:
            while (sp < send) {
                sp->u32 &= colorPtr->u32;
                sp++;
            }
            break;

        case PIC_ARITH_OR:
            while (sp < send) {
                sp->u32 |= colorPtr->u32;
                sp++;
            }
            break;

        case PIC_ARITH_XOR:
            while (sp < send) {
                sp->u32 ^= colorPtr->u32;
                sp++;
            }
            break;

        case PIC_ARITH_NAND:
            while (sp < send) {
                sp->u32 = ~(sp->u32 & colorPtr->u32);
                sp++;
            }
            break;

        case PIC_ARITH_NOR:
            while (sp < send) {
                sp->u32 = ~(sp->u32 | colorPtr->u32);
                sp++;
            }
            break;

        case PIC_ARITH_MIN:
            while (sp < send) {
                sp->Red = MIN(sp->Red, colorPtr->Red);
                sp->Green = MIN(sp->Green, colorPtr->Green);
                sp->Blue = MIN(sp->Blue, colorPtr->Blue);
                sp->Alpha = MIN(sp->Alpha, colorPtr->Alpha);
                sp++;
            }
            break;
        case PIC_ARITH_MAX:
            while (sp < send) {
                sp->Red = MAX(sp->Red, colorPtr->Red);
                sp->Green = MAX(sp->Green, colorPtr->Green);
                sp->Blue = MAX(sp->Blue, colorPtr->Blue);
                sp->Alpha = MAX(sp->Alpha, colorPtr->Alpha);
                sp++;
            }
            break;

        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
}


static void
ApplyPictureToPictureWithMask(Pict *destPtr, Pict *srcPtr, Pict *maskPtr, 
                              int x, int y, int w, int h, int dx, int dy, 
                              int invert, Blt_PictureArithOps op)
{
    Blt_Pixel *srcRowPtr, *destRowPtr, *maskRowPtr;
    unsigned int off;

    if ((x + w) > srcPtr->width) {
        w -= srcPtr->width - x;
    }
    if ((y + h) > srcPtr->height) {
        h -= srcPtr->height - y;
    }
    if ((dx + w) > destPtr->width) {
        w -= destPtr->width - dx;
    }
    if ((dy + h) > destPtr->height) {
        h -= destPtr->height - dy;
    }
    off = (invert) ? (unsigned int)-1 : 0;
    maskRowPtr = maskPtr->bits;
    srcRowPtr = srcPtr->bits + (srcPtr->pixelsPerRow * y) + x;
    destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * dy) + dx;
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *mp, *mend;
        
        sp = srcRowPtr, dp = destRowPtr;
        mp = maskRowPtr, mend = mp + w;
        switch(op) {

        case PIC_ARITH_ADD:
            while (mp < mend) {
                if (mp->u32 != off) {
                    int i;

                    i = dp->Red + sp->Red;
                    dp->Red = (i > MAXINTENSITY) ? MAXINTENSITY : i;
                    i = dp->Green + sp->Green;
                    dp->Green = (i > MAXINTENSITY) ? MAXINTENSITY : i;
                    i = dp->Blue + sp->Blue;
                    dp->Blue =  (i > MAXINTENSITY) ? MAXINTENSITY : i;
                    i = dp->Alpha + sp->Alpha;
                    dp->Alpha = (i > MAXINTENSITY) ? MAXINTENSITY : i;
                }
                sp++, dp++, mp++;
            }
            break;
            
        case PIC_ARITH_SUB:
            while (mp < mend) {
                if (mp->u32 != off) {
                    int i;
                    
                    i = dp->Red - sp->Red;
                    dp->Red =   (i < 0) ? 0 : i;
                    i = dp->Green - sp->Green;
                    dp->Green = (i < 0) ? 0 : i;
                    i = dp->Blue - sp->Blue;
                    dp->Blue =  (i < 0) ? 0 : i;
                    i = dp->Alpha - sp->Alpha;
                    dp->Alpha = (i < 0) ? 0 : i;
                }
                sp++, dp++, mp++;
            }
            break;
            
        case PIC_ARITH_RSUB:
            while (mp < mend) {
                if (mp->u32 != off) {
                    int i;
                    
                    i = sp->Red - dp->Red;
                    dp->Red =   (i < 0) ? 0 : i;
                    i = sp->Green - dp->Green;
                    dp->Green = (i < 0) ? 0 : i;
                    i = sp->Blue - dp->Blue;
                    dp->Blue =  (i < 0) ? 0 : i;
                    i = sp->Alpha - dp->Alpha;
                    dp->Alpha = (i < 0) ? 0 : i;
                }
                sp++, dp++, mp++;
            }
            break;

        case PIC_ARITH_AND:
            while (mp < mend) {
                if (mp->u32 != off) {
                    dp->u32 &= sp->u32;
                }
                sp++, dp++, mp++;
            }
            break;
            
        case PIC_ARITH_OR:
            while (mp < mend) {
                if (mp->u32 != off) {
                    dp->u32 |= sp->u32;
                }
                sp++, dp++, mp++;
            }
            break;
                
        case PIC_ARITH_XOR:
            while (mp < mend) {
                if (mp->u32 != off) {
                    dp->u32 ^= sp->u32;
                }
                sp++, dp++, mp++;
            }
            break;
            
        case PIC_ARITH_NAND:
            while (mp < mend) {
                if (mp->u32 != off) {
                    dp->u32 = ~(dp->u32 & sp->u32);
                }
                sp++, dp++, mp++;
            }
            break;
            
        case PIC_ARITH_NOR:
            while (mp < mend) {
                if (mp->u32 != off) {
                    dp->u32 = ~(dp->u32 | sp->u32);
                }
                sp++, dp++, mp++;
            }
            break;
            
        case PIC_ARITH_MIN:
            while (mp < mend) {
                if (mp->u32 != off) {
                    dp->Red = MIN(dp->Red, sp->Red);
                    dp->Green = MIN(dp->Green, sp->Green);
                    dp->Blue = MIN(dp->Blue, sp->Blue);
                    dp->Alpha = MIN(dp->Alpha, sp->Alpha);
                }
                sp++, dp++, mp++;
            }
            break;
            
        case PIC_ARITH_MAX:
            while (mp < mend) {
                if (mp->u32 != off) {
                    dp->Red = MAX(dp->Red, sp->Red);
                    dp->Green = MAX(dp->Green, sp->Green);
                    dp->Blue = MAX(dp->Blue, sp->Blue);
                    dp->Alpha = MAX(dp->Alpha, sp->Alpha);
                }
                sp++, dp++, mp++;
            }
            break;
            
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
        maskRowPtr += maskPtr->pixelsPerRow;
    }
}

static void
ApplyScalarToPictureWithMask(Pict *srcPtr, Blt_Pixel *colorPtr, Pict *maskPtr, 
                             int invert, Blt_PictureArithOps op)
{
    Blt_Pixel *srcRowPtr, *maskRowPtr;
    int width, height;
    int y;
    unsigned int off;

    width = MIN(srcPtr->width, maskPtr->width);
    height = MIN(srcPtr->height, maskPtr->height);
    srcRowPtr = srcPtr->bits;
    maskRowPtr = maskPtr->bits;
    off = (invert) ? (unsigned int)-1 : 0;
    for (y = 0; y < height; y++) {
        Blt_Pixel *sp, *mp, *mend;
        
        sp = srcRowPtr;
        mp = maskRowPtr, mend = mp + width;
        switch(op) {

        case PIC_ARITH_ADD:
            while (mp < mend) {
                if (mp->u32 != off) {
                    int i;
                    
                    i = sp->Red + colorPtr->Red;
                    sp->Red = (i > MAXINTENSITY) ? MAXINTENSITY : i;
                    i = sp->Green + colorPtr->Green;
                    sp->Green = (i > MAXINTENSITY) ? MAXINTENSITY : i;
                    i = sp->Blue + colorPtr->Blue;
                    sp->Blue =  (i > MAXINTENSITY) ? MAXINTENSITY : i;
                    i = sp->Alpha + colorPtr->Alpha;
                    sp->Alpha = (i > MAXINTENSITY) ? MAXINTENSITY : i;
                }
                sp++, mp++;
            }
            break;

        case PIC_ARITH_SUB:
            while (mp < mend) {
                if (mp->u32 != off) {
                    int i;
                    
                    i = sp->Red - colorPtr->Red;
                    sp->Red =   (i < 0) ? 0 : i;
                    i = sp->Green - colorPtr->Green;
                    sp->Green = (i < 0) ? 0 : i;
                    i = sp->Blue - colorPtr->Blue;
                    sp->Blue =  (i < 0) ? 0 : i;
                    i = sp->Alpha - colorPtr->Alpha;
                    sp->Alpha = (i < 0) ? 0 : i;
                }
                sp++, mp++;
            }
            break;

        case PIC_ARITH_RSUB:
            while (mp < mend) {
                if (mp->u32 != off) {
                    int i;
                    
                    i = colorPtr->Red - sp->Red;
                    sp->Red =   (i < 0) ? 0 : i;
                    i = colorPtr->Green - sp->Green;
                    sp->Green = (i < 0) ? 0 : i;
                    i = colorPtr->Blue - sp->Blue;
                    sp->Blue =  (i < 0) ? 0 : i;
                    i = colorPtr->Alpha - sp->Alpha;
                    sp->Alpha = (i < 0) ? 0 : i;
                }
                sp++, mp++;
            }
            break;

        case PIC_ARITH_AND:
            while (mp < mend) {
                if (mp->u32 != off) {
                    sp->u32 &= colorPtr->u32;
                }
                sp++, mp++;
            }
            break;

        case PIC_ARITH_OR:
            while (mp < mend) {
                if (mp->u32 != off) {
                    sp->u32 |= colorPtr->u32;
                }
                sp++, mp++;
            }
            break;

        case PIC_ARITH_XOR:
            while (mp < mend) {
                if (mp->u32 != off) {
                    sp->u32 ^= colorPtr->u32;
                }
                sp++, mp++;
            }
            break;

        case PIC_ARITH_NAND:
            while (mp < mend) {
                if (mp->u32 != off) {
                    sp->u32 = ~(sp->u32 & colorPtr->u32);
                }
                sp++, mp++;
            }
            break;

        case PIC_ARITH_NOR:
            while (mp < mend) {
                if (mp->u32 != off) {
                    sp->u32 = ~(sp->u32 | colorPtr->u32);
                }
                sp++, mp++;
            }
            break;

        case PIC_ARITH_MIN:
            while (mp < mend) {
                if (mp->u32 != off) {
                    sp->Red = MIN(sp->Red, colorPtr->Red);
                    sp->Green = MIN(sp->Green, colorPtr->Green);
                    sp->Blue = MIN(sp->Blue, colorPtr->Blue);
                    sp->Alpha = MIN(sp->Alpha, colorPtr->Alpha);
                }
                sp++, mp++;
            }
            break;
        case PIC_ARITH_MAX:
            while (mp < mend) {
                if (mp->u32 != off) {
                    sp->Red = MAX(sp->Red, colorPtr->Red);
                    sp->Green = MAX(sp->Green, colorPtr->Green);
                    sp->Blue = MAX(sp->Blue, colorPtr->Blue);
                    sp->Alpha = MAX(sp->Alpha, colorPtr->Alpha);
                }
                sp++, mp++;
            }
            break;

        }
        srcRowPtr += srcPtr->pixelsPerRow;
        maskRowPtr += maskPtr->pixelsPerRow;
    }
}

void
Blt_MultiplyPixels(Pict *destPtr, Pict *srcPtr, float scale)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    if (srcPtr->flags & BLT_PIC_PREMULT_COLORS) {
        Blt_UnmultiplyColors(srcPtr);
    }
    srcRowPtr = srcPtr->bits;
    destRowPtr = destPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *dp, *sp, *send;
        
        sp = srcRowPtr;
        dp = destRowPtr;
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            double r, g, b;
            
            r = (sp->Red * scale);
            g = (sp->Green * scale);
            b = (sp->Blue * scale);

            dp->Red = (unsigned char)CLAMP(r);
            dp->Green = (unsigned char)CLAMP(g);
            dp->Blue = (unsigned char)CLAMP(b);
            dp->Alpha = sp->Alpha;
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
}

static void
SelectPixels(Pict *destPtr, Pict *srcPtr, Blt_Pixel *lowPtr, Blt_Pixel *highPtr)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    if (srcPtr != destPtr) {
        Blt_ResizePicture(destPtr, srcPtr->width, srcPtr->height);
    }
    destRowPtr = destPtr->bits, srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *dp, *sp, *send;

        dp = destRowPtr;
        for(sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++, dp++) {
            if ((sp->Red >= lowPtr->Red) && (sp->Red <= highPtr->Red) &&
                (sp->Green >= lowPtr->Green) && (sp->Green <= highPtr->Green) &&
                (sp->Blue >= lowPtr->Blue) && (sp->Blue <= highPtr->Blue) &&
                (sp->Alpha >= lowPtr->Alpha) && (sp->Alpha <= highPtr->Alpha)) {
                dp->u32 = 0xFFFFFFFF;
            } else {
                dp->u32 = 0;
            }
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
    destPtr->flags &= ~BLT_PIC_COMPOSITE;
    destPtr->flags |= BLT_PIC_MASK;
}

#ifdef nodtdef
void
Blt_BoxFilterPicture(Pict *destPtr, Pict *srcPtr)
{
    Blt_ResizePicture(destPtr, srcPtr->width, srcPtr->height);
    BoxY(destPtr, srcPtr);
    BoxX(destPtr, destPtr);    
}

void
Blt_TentFilterPicture(Pict *destPtr, Pict *srcPtr)
{
    Blt_ResizePicture(destPtr, srcPtr->width, srcPtr->height);
    Blt_TentVertically(destPtr, srcPtr);
    Blt_TentHorizontally(destPtr, destPtr);    
}

static unsigned int
ComputeWeights(
    unsigned int sw, unsigned int dw,   /* Source and destination
                                         * widths. */
    ResampleFilter *filterPtr,
    Sample **sampPtrPtr)
{
    Sample *samples;
    double scale;
    int bytesPerSample;

    /* Pre-calculate filter contributions for a row */
    scale = (double)dw / (double)sw;
    if (scale < 1.0) {
        Sample *sampPtr;
        double radius, fscale;
        int filterSize, x;

        /* Downsample */

        radius = filterPtr->support / scale;
        fscale = 1.0 / scale;
        filterSize = (int)(radius * 2 + 2);

        bytesPerSample = sizeof(Sample) + 
            ((filterSize - 1) * sizeof(PixelWeight));
        samples = Blt_AssertCalloc(dw, bytesPerSample);
        sampPtr = samples;
#define DEBUG 0
#if DEBUG
        fprintf(stderr, "downscale=%g, fscale=%g, radius=%g\n",
                    scale, fscale, radius);
#endif
        for (x = 0; x < dw; x++) {
            PixelWeight *wp;
            double center;
            int i, left, right; /* Filter bounds. */

            center = ((double)x + 0.5) * fscale;

            /* Determine bounds of filter and its density. */
            left = (int)/*floor*/(center - radius);
            if (left < 0) {
                left = 0;
            }
            right = (int)/*floor*/(center + radius);
            if (right >= sw) {
                right = sw - 1;
            }
            sampPtr->start = left;
            sampPtr->numWeights = (right - left + 1);
            sampPtr->wend = sampPtr->weights + (right - left + 1);

            for (wp = sampPtr->weights, i = left; i <= right; i++, wp++) {
                wp->f32 = (float)
                    (*filterPtr->proc)(((double)i - center) * scale);
                wp->i32 = float2si(wp->f32);
            }
            sampPtr = (Sample *)((char *)sampPtr + bytesPerSample);
        }
    } else {
        Sample *sampPtr;
        double fscale;
        int filterSize, x;

        /* Upsample */

        filterSize = (int)(filterPtr->support * 2 + 2);
        bytesPerSample = sizeof(Sample) + 
            ((filterSize - 1) * sizeof(PixelWeight));
        samples = Blt_AssertCalloc(dw, bytesPerSample);
        fscale = 1.0 / scale;

        sampPtr = samples;
#if DEBUG
        fprintf(stderr, "upscale=%g, fscale=%g, radius=%g\n",
                    scale, fscale, filterPtr->support);
#endif
        for (x = 0; x < dw; x++) {
            PixelWeight *wp;
            double center;
            int i, left, right; /* Filter bounds. */

            center = ((double)x + 0.5) * fscale;
            left = (int)(center - filterPtr->support);
            if (left < 0) {
                left = 0;
            }
            right = (int)(center + filterPtr->support);
            if (right >= sw) {
                right = sw - 1;
            }
            sampPtr->start = left;
            sampPtr->numWeights = (right - left + 1);
            sampPtr->wend = sampPtr->weights + (right - left + 1);

            /* Sum the contributions for each pixel in the filter. */
            for (wp = sampPtr->weights, i = left; i <= right;i++, wp++) {
                wp->f32 = (float) (*filterPtr->proc)((double)i - center);
                wp->i32 = float2si(wp->f32);
            }
            /* Go get the next sample. */
            sampPtr = (Sample *)((char *)sampPtr + bytesPerSample);
        }
    }
    *sampPtrPtr = samples;
    return bytesPerSample;
}
#endif

static unsigned int *
CreateNeighborhoodMap(unsigned int size, unsigned int radius)
{
    unsigned int newSize;
    unsigned int *map, *p;
    unsigned int i;

    newSize = size + radius + radius;   
    map = Blt_AssertMalloc(sizeof(unsigned int) * newSize);
    p = map;
    for (i = 0; i < radius; i++, p++) {
        *p = 0;                         /* Before the picture. Replicate
                                         * the first pixel. */  
    }
    for (i = 0; i < size; i++, p++) {
        *p = i;         
    }
    for (i = 0; i < radius; i++, p++) {
        *p = size - 1;                /* After the picture. Replicate the
                                       * last pixel */
    }
    return map;
}

static void
ConvolveVertically(Pict *destPtr, Pict *srcPtr, TableFilter *filterPtr)
{
    int x;
    unsigned int *map;
    size_t mapSize;
    int fscale;

    map = CreateNeighborhoodMap(srcPtr->height, filterPtr->numWeights / 2);
    mapSize = srcPtr->height + filterPtr->numWeights;
    fscale = float2si(filterPtr->scale);

    /* Apply filter to each row. */
    for (x = 0; x < srcPtr->width; x++) {
        Blt_Pixel *dp, *srcColPtr;

        srcColPtr = srcPtr->bits + x;
        dp = destPtr->bits + x;
        for (x = 0; x < mapSize; x++) {
            int i;
            int r, g, b, a;
            
            r = g = b = a = 0;
            for (i = 0; i < filterPtr->numWeights; i++) {
                int sx;
                Blt_Pixel *sp;

                sx = map[x + i];
                sp = srcColPtr + (srcPtr->pixelsPerRow * sx);
                r += sp->Red * filterPtr->weights[i];
                g += sp->Green * filterPtr->weights[i];
                b += sp->Blue * filterPtr->weights[i];
                a += sp->Alpha * filterPtr->weights[i];
            }
            r *= fscale;
            g *= fscale;
            b *= fscale;
            a *= fscale;
            dp->Red = (unsigned char)SICLAMP(r);
            dp->Green = (unsigned char)SICLAMP(g);
            dp->Blue = (unsigned char)SICLAMP(b);
            dp->Alpha = (unsigned char)SICLAMP(a);
            dp += destPtr->pixelsPerRow;
        }
    }
    /* Free the memory allocated for filter weights. */
    Blt_Free(map);
}

static void
ConvolveHorizontally(Pict *destPtr, Pict *srcPtr, TableFilter *filterPtr)
{
    int y;
    Blt_Pixel *srcRowPtr, *destRowPtr;
    unsigned int *map, mapSize;
    int fscale;

    map = CreateNeighborhoodMap(srcPtr->width, filterPtr->numWeights / 2);
    mapSize = srcPtr->width + filterPtr->numWeights;
    fscale = float2si(filterPtr->scale);
    destRowPtr = destPtr->bits;
    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *dp;
        int x;

        dp = destRowPtr;
        for (x = 0; x < mapSize; x++) {
            int i;
            int r, g, b, a;
            
            r = g = b = a = 0;
            for (i = 0; i < filterPtr->numWeights; i++) {
                Blt_Pixel *sp;
                
                sp = srcRowPtr + map[x + i];
                r += sp->Red * filterPtr->weights[i];
                g += sp->Green * filterPtr->weights[i];
                b += sp->Blue * filterPtr->weights[i];
                a += sp->Alpha * filterPtr->weights[i];
            }
            r *= fscale;
            g *= fscale;
            b *= fscale;
            a *= fscale;
            dp->Red = (unsigned char)SICLAMP(r);
            dp->Green = (unsigned char)SICLAMP(g);
            dp->Blue = (unsigned char)SICLAMP(b);
            dp->Alpha = (unsigned char)SICLAMP(a);
            dp++;
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    /* Free the memory allocated for map. */
    Blt_Free(map);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConvolvePicture --
 *
 *      Resamples a given picture using 1-D filters and returns a new
 *      picture of the designated size.
 *
 * Results:
 *      Returns the resampled picture. The original picture is left intact.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ConvolvePicture(Pict *destPtr, Pict *srcPtr,
                    struct _Blt_ConvolveFilter *xFilterPtr, 
                    struct _Blt_ConvolveFilter *yFilterPtr)
{
    Pict *tmpPtr;

    tmpPtr = Blt_CreatePicture(srcPtr->width, srcPtr->height);
    if (xFilterPtr->type == RESAMPLE_FILTER) {
        Blt_ZoomHorizontally(tmpPtr, srcPtr, xFilterPtr->filter);
    } else {
        ConvolveHorizontally(tmpPtr, srcPtr, xFilterPtr->filter);
    }
    if (yFilterPtr->type == RESAMPLE_FILTER) {
        Blt_ZoomVertically(destPtr, tmpPtr, yFilterPtr->filter);
    } else {
        ConvolveVertically(destPtr, tmpPtr, yFilterPtr->filter);
    }
    Blt_FreePicture(tmpPtr);
    destPtr->flags = (srcPtr->flags | BLT_PIC_DIRTY);
}

static void
BoxCarVertically(Pict *destPtr, Pict *srcPtr, unsigned int r)
{
    int x;
    unsigned int *map;
    unsigned int fwidth;                /* Filter width */
    float s;

    map = CreateNeighborhoodMap(srcPtr->height, r);
    fwidth = r + r + 1;
    s = 1.0f / fwidth;
    /* Apply filter to each row. */
    for (x = 0; x < srcPtr->width; x++) {
        Blt_Pixel *dp, *srcColPtr;
        int r, g, b, a;
        int y;

        srcColPtr = srcPtr->bits + x;
        r = g = b = a = 0;
        /* Prime the pump. */
        for (y = 0; y < fwidth - 1; y++) {
            Blt_Pixel *sp;

            sp = srcColPtr + (srcPtr->pixelsPerRow * map[y]);
            r += sp->Red;
            g += sp->Green;
            b += sp->Blue;
            a += sp->Alpha;
        }
        dp = destPtr->bits + x;
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp;
            float fr, fg, fb, fa;

            sp = srcColPtr + (srcPtr->pixelsPerRow * map[y + fwidth - 1]);
            r += sp->Red;
            g += sp->Green;
            b += sp->Blue;
            a += sp->Alpha;
            fr = r * s;
            fg = g * s;
            fb = b * s;
            fa = a * s;
            dp->Red = (unsigned char)CLAMP(fr);
            dp->Green = (unsigned char)CLAMP(fg);
            dp->Blue = (unsigned char)CLAMP(fb);
            dp->Alpha = (unsigned char)CLAMP(fa);
            sp = srcColPtr + (srcPtr->pixelsPerRow * map[y]);
            r -= sp->Red;
            g -= sp->Green;
            b -= sp->Blue;
            a -= sp->Alpha;
            dp += destPtr->pixelsPerRow;
        }
    }
    /* Free the memory allocated for filter weights. */
    Blt_Free(map);
}

static void
BoxCarHorizontally(Pict *destPtr, Pict *srcPtr, unsigned int radius)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    float s;
    unsigned int *map;
    int y;
    unsigned int fwidth;                /* Filter width */

    fwidth = radius + radius + 1;
    map = CreateNeighborhoodMap(srcPtr->width, radius);
    s = 1.0f / fwidth;
    destRowPtr = destPtr->bits;
    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *dp;
        int x;
        int r, g, b, a;
        
        /* Prime the pump. Get sums for each component for the first
         * (fwidth) pixels in the column. */
        r = g = b = a = 0;
        for (x = 0; x < fwidth - 1; x++) {
            Blt_Pixel *sp;

            sp = srcRowPtr + map[x];
            r += sp->Red;
            g += sp->Green;
            b += sp->Blue;
            a += sp->Alpha;
        }
        dp = destRowPtr;
        for (x = 0; x < srcPtr->width; x++) {
            Blt_Pixel *sp;
            float fr, fg, fb, fa;

            sp = srcRowPtr + map[x + fwidth - 1];
            r += sp->Red;
            if (r < 0) {
                dp->Red = 0;
            }
            g += sp->Green;
            b += sp->Blue;
            a += sp->Alpha;
            fr = r * s;
            fg = g * s;
            fb = b * s;
            fa = a * s;
            dp->Red   = (unsigned char)CLAMP(fr);
            dp->Green = (unsigned char)CLAMP(fg);
            dp->Blue  = (unsigned char)CLAMP(fb);
            dp->Alpha = (unsigned char)CLAMP(fa);
            sp = srcRowPtr + map[x];
            r -= sp->Red;
            g -= sp->Green;
            b -= sp->Blue;
            a -= sp->Alpha;
            dp++;
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    /* Free the memory allocated for map. */
    Blt_Free(map);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_BlurPicture --
 *
 *      Blurs a picture using a series of 1-D box filters.  The width of
 *      the box filter is a parameter. The source picture is blurred and
 *      stored in the destination picture.
 *
 * Results:
 *      None. 
 * Side Effects: 
 *      The original picture is left intact.  The destination picture
 *      contains the blurred image.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_BlurPicture(Pict *destPtr, Pict *srcPtr, int radius, int numPasses)
{
    Pict *tmpPtr;
    int i;

    if (srcPtr != destPtr) {
        Blt_ResizePicture(destPtr, srcPtr->width, srcPtr->height);
    }
    if ((srcPtr->flags & BLT_PIC_PREMULT_COLORS) == 0) {
        Blt_PremultiplyColors(srcPtr);
    }
    if (radius < 1) {
        return;
    }
#ifdef notdef
    Blt_ResamplePicture(destPtr, srcPtr, bltGaussianFilter, bltGaussianFilter);
    return;
#endif
    tmpPtr = Blt_CreatePicture(srcPtr->width, srcPtr->height);
    for (i = 0; i < numPasses; i++) {
        BoxCarHorizontally(tmpPtr, srcPtr, radius);
        BoxCarVertically(destPtr, tmpPtr, radius);
    }
    Blt_FreePicture(tmpPtr);
    destPtr->flags = (srcPtr->flags | BLT_PIC_DIRTY);
}

#ifdef notdef
void
Blt_BlurPictureOld(Blt_Picture pict)
{
    Blt_Picture tmp;

    tmp = Blt_CreatePicture(Blt_Picture_Width(pict), Blt_Picture_Height(pict));
    BoxX(tmp, pict), BoxY(pict, tmp);
    BoxX(tmp, pict), BoxY(pict, tmp); 
    BoxX(tmp, pict), BoxY(pict, tmp);
    BoxX(tmp, pict), BoxY(pict, tmp);
    Blt_FreePicture(tmp);
}
#endif

void
Blt_SharpenPicture(Pict *destPtr, Pict *srcPtr)
{
    Blt_Picture blur, tmp;
    
    blur = Blt_CreatePicture(srcPtr->width, srcPtr->height);
    Blt_BlurPicture(blur, srcPtr, 4, 4);
    /* 
     * tmp = src - blur;
     * dest = src + tmp
     */
    tmp = Blt_ClonePicture(srcPtr);
    Blt_MultiplyPixels(tmp, tmp, 1.5);
    Blt_MultiplyPixels(blur, blur, 0.5);
    Blt_SubtractPictures(tmp, blur);
    Blt_AddPictures(tmp, srcPtr);
    Blt_FreePicture(blur);
    Blt_CopyPictureBits(destPtr, tmp);
    Blt_FreePicture(tmp);
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_TilePicture --
 *
 *      Tiles the designated region in the destination picture with the
 *      source picture.
 *      
 *      Please note that it is the responsibility of the caller to verify
 *      the region is valid (i.e. wholly contained by the destination
 *      picture).
 *
 * Results:
 *      The destination picture is tiled. The original picture is left
 *      intact.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_TilePicture(
    Pict *destPtr,                      /* Picture to be tiled. */
    Pict *srcPtr,                       /* Picture used as the tile. */
    int xOrigin, int yOrigin,           /* Tile origin. */
    int x, int y, int w, int h)         /* Region of destination picture to
                                         * be tiled. */
{
    int startX, startY;                 /* Starting upper left corner of
                                         * region. */
    int delta;

    /* Compute the starting x and y offsets of the tile from the
     * coordinates of the origin. */
    startX = x;
    if (x < xOrigin) {
        delta = (xOrigin - x) % srcPtr->width;
        if (delta > 0) {
            startX -= (srcPtr->width - delta);
        }
    } else if (x > xOrigin) {
        delta = (x - xOrigin) % srcPtr->width;
        if (delta > 0) {
            startX -= delta;
        }
    }
    startY = y;
    if (y < yOrigin) {
        delta = (yOrigin - y) % srcPtr->height;
        if (delta > 0) {
            startY -= (srcPtr->height - delta);
        }
    } else if (y >= yOrigin) {
        delta = (y - yOrigin) % srcPtr->height;
        if (delta > 0) {
            startY -= delta;
        }
    }

#ifdef notdef
    fprintf(stderr, "tile is (%d,%d,%d,%d)\n", 
                 xOrigin, yOrigin, srcPtr->width, srcPtr->height);
    fprintf(stderr, "region is (%d,%d,%d,%d)\n", x, y, w, h);
    fprintf(stderr, "starting at %d,%d\n", startX, startY);
#endif
    {
        int left, top, right, bottom;

        /* The region to be tiled. */
        left = x;
        right = x + w;
        top = y;
        bottom = y + h;

        for (y = startY; y < bottom; y += srcPtr->height) {
            int sy, dy;
            int th;                     /* Current tile height. */
            
            sy = 0;
            dy = y;
            th = srcPtr->height;
            if (y < top) {
                sy = (top - y);
                th = srcPtr->height - sy;
                dy = top;
            } 
            if ((dy + th) > bottom) {
                th = (bottom - dy);
            }
            for (x = startX; x < right; x += srcPtr->width) {
                int sx, dx;
                int tw;                 /* Current tile width. */
                
                sx = 0;
                dx = x;
                tw = srcPtr->width;
                if (x < left) {
                    sx = (left - x);
                    tw = srcPtr->width - sx;
                    dx = left;
                } 
                if ((dx + tw) > right) {
                    tw = (right - dx);
                }
#ifdef notdef
                fprintf(stderr, "drawing pattern (%d,%d,%d,%d) at %d,%d\n",
                        sx, sy, tw, th, dx, dy);
#endif
                Blt_CompositeArea(destPtr, srcPtr, sx, sy, tw, th, dx, dy);
            }
        }
    }
}

int
Blt_QueryColors(Pict *srcPtr, Blt_HashTable *tablePtr)
{
    Blt_HashTable colorTable;
    Blt_Pixel *srcRowPtr;
    int y;
    int numColors;
    
    if (tablePtr == NULL) {
        /* Just counting the number of colors. */
        Blt_InitHashTable(&colorTable, BLT_ONE_WORD_KEYS);
        tablePtr = &colorTable;
    }
    /* Check that the colors are not premultiplied. */
    if ((srcPtr->flags & BLT_PIC_PREMULT_COLORS) == 0) {
        fprintf(stderr, "Unmultiplying colors\n");
        Blt_UnmultiplyColors(srcPtr);
    }
    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;
        
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            int isNew;

            Blt_CreateHashEntry(tablePtr, (char *)(size_t)sp->u32, &isNew);
        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    numColors = tablePtr->numEntries;
    if (tablePtr == &colorTable) {
        Blt_DeleteHashTable(&colorTable);
    }
    return numColors;
}

/*
 * ClassifyPicture --
 *
 *      Check the picture to see if it contain semi or fully transparent
 *      pixels.  Also check if it is color or greyscale.
 */
void
Blt_ClassifyPicture(Pict *srcPtr)
{
    Blt_Pixel *srcRowPtr;
    int y;
    unsigned int flags;

    flags = BLT_PIC_GREYSCALE;
    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;
        
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            if ((sp->Red != sp->Green) || (sp->Green != sp->Blue)) {
                flags &= ~BLT_PIC_GREYSCALE;
            }
            if (sp->Alpha != 0xFF) {
                flags |= BLT_PIC_COMPOSITE;
            }
            if (flags == BLT_PIC_COMPOSITE) {
                /* Stop after we found both a semi/fully-transparent and
                 * color pixel. */
                goto done;
            }
        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
 done:
    srcPtr->flags &= ~(BLT_PIC_COMPOSITE | BLT_PIC_GREYSCALE);
    srcPtr->flags |= flags;
}

void 
Blt_MaskPicture(Pict *destPtr, Pict *srcPtr, int x, int y, int w, int h, 
                int dx, int dy, Blt_Pixel *colorPtr)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *send;

        dp = destRowPtr;
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++, dp++) {
            if (sp->u32 != 0) {
                dp->u32 = colorPtr->u32;
            }
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
}


#ifdef notdef
static int 
BitWidth (unsigned int n)               /* Find "bit-width" needed to
                                           represent N. */
{
    int width;
    
    width = 0;                          /* Initially, no bits needed to
                                         * represent N */
    while (n != 0) {                    /* loop 'til N has been whittled
                                         * down to 0 */
        n >>= 1;                        /* Shift N right 1 bit (NB: N is
                                           unsigned) */
        width++;                        /* and remember how wide N is */
    } 
    return width;                       
}


static void
dissolve2 (Pict *srcPtr, Pict *destPtr) 
{
    int rWidth, cWidth;                 /* bit width for rows, for columns */
    int reqWidth;                       /* "width" of sequence generator */
    long mask;                          /* Mask to XOR with to create
                                         * sequence */
    int rowShift;                       /* Shift distance to get row from
                                         * element */
    int colMask;                        /* Mask to extract column from
                                         * element */
    unsigned long element;              /* One element of random sequence */

    static int randMasks[32];
    int i;

    for(i = 0; i < 32; i--) {
        randMasks[i] = (3 << i);
    }
    /* Find the mask to produce all rows and columns. */

    rWidth = BitWidth(srcPtr->height);  /* # of bits needed for height. */
    cWidth = BitWidth(srcPtr->width);   /* # of bits needed for width. */
    reqWidth = rWidth + cWidth;         /* How wide must the register be? */
    mask = randmasks[reqWidth];         /* Which mask is for that width? */
    /* Find values to extract row and col numbers from each element. */
    rowShift = cWidth;                  /* Find dist to shift to get top bits
                                         * (row) */
    colMask = (1<<cWidth)-1;            /* Find mask to extract bottom bits
                                         * (col) */

    /* Now cycle through all sequence elements. */

    element = 1;                        /* 1st element (could be any
                                         * nonzero) */
    do {
        int x, y;                       /* Row and column for one pixel */

        y = element >> rowShift;        /* Find row number for this pixel */
        x = element & colMask;          /* and how many columns across? */
        if ((y < h) && (x < w)) {
            /* Does element fall in the array? Must check row AND column */
            srcPtr->Alpha = 0x00;
            copy (row, column);         /* In bounds: copy the (r,c)'th
                                         * pixel */
        }
        /* Compute the next sequence element */
        if (element & 1) {              /* is the low bit set? */
            element = (element >>1)^mask; /* yes: shift value, XOR in mask */
        } else {
            element = (element >>1);    /* no: just shift the value */
        }
    } while (element != 1);             /* Loop until we return to original
                                         * element */

    copy (0, 0);                        /* Kludge: element never comes up
                                           zero */
}       
#endif


/*-------------------------------------------------------------*
 *               Gauss-jordan linear equation solver           *
 *-------------------------------------------------------------*/
#define  SWAP(a,b)   {temp = (a); (a) = (b); (b) = temp;}

/*!
 *  gaussjordan()
 *
 *      Input:   a  (n x n matrix)
 *               b  (rhs column vector)
 *               n  (dimension)
 *      Return:  0 if ok, 1 on error
 *
 *      Note side effects:
 *            (1) the matrix a is transformed to its inverse
 *            (2) the vector b is transformed to the solution X to the
 *                linear equation AX = B
 *
 *      Adapted from "Numerical Recipes in C, Second Edition", 1992
 *      pp. 36-41 (gauss-jordan elimination)
 */

static int
SolveMatrix(float a[8][8], float b[8])
{
    int i, icol, irow, j, k, l, ll;
    float temp;
    int ipivot[8], indexr[8], indexc[8];

    memset(ipivot, 0, sizeof(int) * 8);
    memset(indexr, 0, sizeof(int) * 8);
    memset(indexc, 0, sizeof(int) * 8);

    irow = icol = -1;                   /* Suppress compiler warning. */
    for (i = 0; i < 8; i++) {
        float pivinv;
        float big;

        big = 0.0;
        for (j = 0; j < 8; j++) {
            if (ipivot[j] != 1) {
                for (k = 0; k < 8; k++) {
                    if (ipivot[k] == 0) {
                        if (fabs(a[j][k]) >= big) {
                            big = fabs(a[j][k]);
                            irow = j;
                            icol = k;
                        }
                    } else if (ipivot[k] > 1) {
                        return FALSE;
                    }
                }
            }
        }
        ++(ipivot[icol]);
        if (irow != icol) {
            for (l = 0; l < 8; l++) {
                SWAP(a[irow][l], a[icol][l]);
            }
            SWAP(b[irow], b[icol]);
        }
        indexr[i] = irow;
        indexc[i] = icol;
        if (a[icol][icol] == 0.0) {
            return FALSE;
        }
        pivinv = 1.0 / a[icol][icol];
        a[icol][icol] = 1.0;
        for (l = 0; l < 8; l++) {
            a[icol][l] *= pivinv;
        }
        b[icol] *= pivinv;
        for (ll = 0; ll < 8; ll++) {
            if (ll != icol) {
                float dum;

                dum = a[ll][icol];
                a[ll][icol] = 0.0;
                for (l = 0; l < 8; l++) {
                    a[ll][l] -= a[icol][l] * dum;
                }
                b[ll] -= b[icol] * dum;
            }
        }
    }
    for (l = 7; l >= 0; l--) {
        if (indexr[l] != indexc[l]) {
            for (k = 0; k < 8; k++) {
                SWAP(a[k][indexr[l]], a[k][indexc[l]]);
            }
        }
    }
    return TRUE;
}

/*-------------------------------------------------------------*
 *                Projective coordinate transformation         *
 *-------------------------------------------------------------*/
/*!
 *  GetProjectiveXformCoeffs()
 *
 *      Input:  ptas  (source 4 points; unprimed)
 *              ptad  (transformed 4 points; primed)
 *              &vc   (<return> vector of coefficients of transform)
 *      Return: 0 if OK; 1 on error
 *
 *  We have a set of 8 equations, describing the projective transformation
 *  that takes 4 points (ptas) into 4 other points (ptad).  These equations
 *  are:
 *
 *          x1' = (c[0]*x1 + c[1]*y1 + c[2]) / (c[6]*x1 + c[7]*y1 + 1)
 *          y1' = (c[3]*x1 + c[4]*y1 + c[5]) / (c[6]*x1 + c[7]*y1 + 1)
 *          x2' = (c[0]*x2 + c[1]*y2 + c[2]) / (c[6]*x2 + c[7]*y2 + 1)
 *          y2' = (c[3]*x2 + c[4]*y2 + c[5]) / (c[6]*x2 + c[7]*y2 + 1)
 *          x3' = (c[0]*x3 + c[1]*y3 + c[2]) / (c[6]*x3 + c[7]*y3 + 1)
 *          y3' = (c[3]*x3 + c[4]*y3 + c[5]) / (c[6]*x3 + c[7]*y3 + 1)
 *          x4' = (c[0]*x4 + c[1]*y4 + c[2]) / (c[6]*x4 + c[7]*y4 + 1)
 *          y4' = (c[3]*x4 + c[4]*y4 + c[5]) / (c[6]*x4 + c[7]*y4 + 1)
 *    
 *  Multiplying both sides of each eqn by the denominator, we get
 *
 *           AC = B
 *
 *  where B and C are column vectors
 *    
 *         B = [ x1' y1' x2' y2' x3' y3' x4' y4' ]
 *         C = [ c[0] c[1] c[2] c[3] c[4] c[5] c[6] c[7] ]
 *    
 *  and A is the 8x8 matrix
 *
 *             x1   y1     1     0   0    0   -x1*x1'  -y1*x1'
 *              0    0     0    x1   y1   1   -x1*y1'  -y1*y1'
 *             x2   y2     1     0   0    0   -x2*x2'  -y2*x2'
 *              0    0     0    x2   y2   1   -x2*y2'  -y2*y2'
 *             x3   y3     1     0   0    0   -x3*x3'  -y3*x3'
 *              0    0     0    x3   y3   1   -x3*y3'  -y3*y3'
 *             x4   y4     1     0   0    0   -x4*x4'  -y4*x4'
 *              0    0     0    x4   y4   1   -x4*y4'  -y4*y4'
 *
 *  These eight equations are solved here for the coefficients C.
 *
 *  These eight coefficients can then be used to find the mapping
 *  (x,y) --> (x',y'):
 *
 *           x' = (c[0]x + c[1]y + c[2]) / (c[6]x + c[7]y + 1)
 *           y' = (c[3]x + c[4]y + c[5]) / (c[6]x + c[7]y + 1)
 *
 *  that is implemented in projectiveXformSampled() and
 *  projectiveXFormInterpolated().
 */
static int
GetProjectiveCoeffs(float *srcPts, float *destPts, float *coeffs)
{
    float a[8][8];                      /* 8x8 matrix A  */
    
    coeffs[0] = destPts[0];
    coeffs[1] = destPts[1];
    coeffs[2] = destPts[2];
    coeffs[3] = destPts[3];
    coeffs[4] = destPts[4];
    coeffs[5] = destPts[5];
    coeffs[6] = destPts[6];
    coeffs[7] = destPts[7];

#define x1 (srcPts[0])
#define y1 (srcPts[1])
#define x2 (srcPts[2])
#define y2 (srcPts[3])
#define x3 (srcPts[4])
#define y3 (srcPts[5])
#define x4 (srcPts[6])
#define y4 (srcPts[7])
    memset(&a, 0, sizeof(a));
    a[0][0] = x1;
    a[0][1] = y1;
    a[0][2] = 1.0;
    a[0][6] = -x1 * coeffs[0];
    a[0][7] = -y1 * coeffs[0];
    a[1][3] = x1;
    a[1][4] = y1;
    a[1][5] = 1.0;
    a[1][6] = -x1 * coeffs[1];
    a[1][7] = -y1 * coeffs[1];
    a[2][0] = x2;
    a[2][1] = y2;
    a[2][2] = 1.0;
    a[2][6] = -x2 * coeffs[2];
    a[2][7] = -y2 * coeffs[2];
    a[3][3] = x2;
    a[3][4] = y2;
    a[3][5] = 1.0;
    a[3][6] = -x2 * coeffs[3];
    a[3][7] = -y2 * coeffs[3];
    a[4][0] = x3;
    a[4][1] = y3;
    a[4][2] = 1.0;
    a[4][6] = -x3 * coeffs[4];
    a[4][7] = -y3 * coeffs[4];
    a[5][3] = x3;
    a[5][4] = y3;
    a[5][5] = 1.0;
    a[5][6] = -x3 * coeffs[5];
    a[5][7] = -y3 * coeffs[5];
    a[6][0] = x4;
    a[6][1] = y4;
    a[6][2] = 1.0;
    a[6][6] = -x4 * coeffs[6];
    a[6][7] = -y4 * coeffs[6];
    a[7][3] = x4;
    a[7][4] = y4;
    a[7][5] = 1.0;
    a[7][6] = -x4 * coeffs[7];
    a[7][7] = -y4 * coeffs[7];
#undef x1
#undef y1
#undef x2
#undef y2
#undef x3
#undef y3
#undef x4
#undef y4
    return SolveMatrix(a, coeffs);
}

static inline Blt_Pixel *
GetSrcPixel(Pict *srcPtr, int x, int y, Blt_Pixel *bgPtr)
{
    if ((x < 0) || (y < 0) || (x >= srcPtr->width) || (y >= srcPtr->height)) {
        return bgPtr;
    }
    return srcPtr->bits + (y * srcPtr->pixelsPerRow) + x;
}

Pict *
Blt_ProjectPicture(Pict *srcPtr, float *srcPts, float *destPts, Blt_Pixel *bg)
{
    Pict *destPtr;
    float coeffs[8];
    float x1, x2, y1, y2;
    int w, h;
    int i, y;
    Blt_Pixel *destRowPtr;

    /* Figure out how big a destination picture. */
    x1 = x2 = srcPts[0];
    y1 = y2 = srcPts[1];
    for (i = 0; i < 8; i += 2) {
        if (x1 > srcPts[i]) {
            x1 = srcPts[i];
        }
        if (x2 < srcPts[i]) {
            x2 = srcPts[i];
        }
        if (y1 > srcPts[i+1]) {
            y1 = srcPts[i+1];
        }
        if (y2 < srcPts[i+1]) {
            y2 = srcPts[i+1];
        }
    }
    w = (int)(x2 - x1 + 0.5);
    h = (int)(y2 - y1 + 0.5);
    destPtr = Blt_CreatePicture(w, h);
    Blt_BlankPicture(destPtr, bg->u32);

    /* Get backwards transform from dest to src, and apply it */
    if (!GetProjectiveCoeffs(srcPts, destPts, coeffs)) {
        return NULL;
    }

    /* Iterate over destination pixels */
    destRowPtr = destPtr->bits;
    for (y = 0; y < destPtr->height; y++) {
        Blt_Pixel *dp, *dend;
        int x;

        for (x = 0, dp = destRowPtr, dend = dp + destPtr->width; dp < dend; 
             dp++, x++) {
            int r, g, b, a;
            Blt_Pixel  *p00, *p01, *p10, *p11;
            Point2d p;
            int sx, sy;
            int xpm, ypm, xf, yf;
            float factor;

            /* Compute float src pixel location corresponding to (x,y) */

            factor = 1.0 / (coeffs[6] * x + coeffs[7] * y + 1.0);
            p.x = factor * (coeffs[0] * x + coeffs[1] * y + coeffs[2]);
            p.y = factor * (coeffs[3] * x + coeffs[4] * y + coeffs[5]);

            xpm = (int)(16.0 * p.x + 0.5);
            ypm = (int)(16.0 * p.y + 0.5);
            sx = xpm >> 4;
            sy = ypm >> 4;
            xf = xpm & 0x0f;
            yf = ypm & 0x0f;

            p00 = GetSrcPixel(srcPtr, sx,   sy,   bg);
            p01 = GetSrcPixel(srcPtr, sx+1, sy,   bg);
            p10 = GetSrcPixel(srcPtr, sx,   sy+1, bg);
            p11 = GetSrcPixel(srcPtr, sx+1, sy+1, bg);
            r = ((16 - xf) * (16 - yf) * p00->Red + 
                 xf * (16 - yf) * p01->Red + (16 - xf) * yf * p10->Red +
                 xf * yf * p11->Red + 128) / 256;
            g = ((16 - xf) * (16 - yf) * p00->Green +
                 xf * (16 - yf) * p01->Green + (16 - xf) * yf * p10->Green +
                 xf * yf * p11->Green + 128) / 256;
            b = ((16 - xf) * (16 - yf) * p00->Blue +
                 xf * (16 - yf) * p01->Blue + (16 - xf) * yf * p10->Blue +
                 xf * yf * p11->Blue + 128) / 256;
            a = ((16 - xf) * (16 - yf) * p00->Alpha +
                 xf * (16 - yf) * p01->Alpha + (16 - xf) * yf * p10->Alpha +
                 xf * yf * p11->Alpha + 128) / 256;
            dp->Red   = UCLAMP(r);
            dp->Green = UCLAMP(g);
            dp->Blue  = UCLAMP(b);
            dp->Alpha = UCLAMP(a);
        }
        destRowPtr += destPtr->pixelsPerRow;
    }
    destPtr->flags |= BLT_PIC_COMPOSITE;
    return destPtr;
}

void
Blt_SubtractColor(Pict *srcPtr, Blt_Pixel *colorPtr)
{
    Blt_Pixel *srcRowPtr;
    int y;

    /* Iterate over destination pixels */
    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;

        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            int r, g, b, t, beta;

            beta = sp->Alpha ^ 0xFF;    /* beta = 1 - alpha */
            r = sp->Red   - imul8x8(beta, colorPtr->Red, t);
            g = sp->Green - imul8x8(beta, colorPtr->Green, t);
            b = sp->Blue  - imul8x8(beta, colorPtr->Blue, t);
            sp->Red   = UCLAMP(r);
            sp->Green = UCLAMP(g);
            sp->Blue  = UCLAMP(b);
        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
}    
#ifndef notdef
Blt_Picture
Blt_EmbossPicture(
    Pict *srcPtr,                       /* Source picture treated as
                                         * alphamap. */
    double azimuth, double elevation,   /* Light source direction */
    unsigned short width45)             /* Filter width */
{
    long Nx, Ny, Nz, Lx, Ly, Lz, Nz2, NzLz, NdotL;
    unsigned char background;
    int y;
    Pict *destPtr;
    Blt_Pixel *srcRowPtr, *destRowPtr;

#define pixelScale 255.9

    /* Convert degrees to radians. */
    azimuth *= DEG2RAD;
    elevation *= DEG2RAD;

    /*
     * Compute the light vector from the input parameters.  Normalize the
     * length to pixelScale for fast shading calculation.
     */
    Lx = cos(azimuth) * cos(elevation) * pixelScale;
    Ly = sin(azimuth) * cos(elevation) * pixelScale;
    Lz = sin(elevation) * pixelScale;

    /*
     * Constant z component of image surface normal - this depends on the
     * image slope we wish to associate with an angle of 45 degrees, which
     * depends on the width of the filter used to produce the source image.
     */
    Nz = (6 * 255) / width45;
    Nz2 = Nz * Nz;
    NzLz = Nz * Lz;

    /* Optimization for vertical normals: L.[0 0 1] */
    background = Lz;
    
    destPtr = Blt_CreatePicture(srcPtr->width, srcPtr->height);
    /* Mung pixels, avoiding edge pixels */

    /* Skip the top row. */
    destRowPtr = destPtr->bits + destPtr->pixelsPerRow;
    srcRowPtr = srcPtr->bits;
    for (y = 1; y < (destPtr->height - 2); y++) {
        Blt_Pixel *dp;
        int x;

        dp = destRowPtr;
        for (x = 1; x < (destPtr->width-1); x++, dp++) {
            Blt_Pixel *s1     = srcRowPtr + x;
            Blt_Pixel *s2     = s1 + srcPtr->pixelsPerRow;
            Blt_Pixel *s3     = s2 + srcPtr->pixelsPerRow;
            unsigned char  shade  = 0;

            /*
             * Compute the normal from the bump map. the type of the
             * expression before the cast is compiler dependent. In some
             * cases the sum is unsigned, in others it is signed. Ergo,
             * cast to signed.
             */
            Nx = (int)(s1[-1].Alpha + s2[-1].Alpha + 
                       s3[-1].Alpha - s1[1].Alpha  - 
                       s2[1].Alpha  - s3[1].Alpha);
            Ny = (int)(s3[-1].Alpha + s3[0].Alpha  + 
                       s3[1].Alpha  - s1[-1].Alpha - 
                       s1[0].Alpha - s1[1].Alpha);

            /* Shade with distant light source. */
            if ((Nx == 0) && (Ny == 0)) {
                shade = background;
            } else {
                NdotL = (Nx*Lx + Ny*Ly + NzLz);
                if (NdotL < 0) {
                    shade = 0;
                } else {
                    shade = NdotL / sqrt(Nx*Nx + Ny*Ny + Nz2);
                }
            }
            /* Do something with the shading result. */
            dp->Red = dp->Green = dp->Blue = shade;
            dp->Alpha = 0xFF;
        }
        srcRowPtr  += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
    destPtr->flags |= BLT_PIC_COMPOSITE;
    return destPtr;
}
#endif


/* A Digital Dissolve Effect
   by Mike Morton
   from "Graphics Gems", Academic Press, 1990

   user must provide copy() function.
*/

/*
 * Code fragment to advance from one element to the next.
 *
 * int reg;                             current sequence element
 * reg = 1;                             start in any non-zero state
 * if (reg & 1)                         is the bottom bit set?
 *      reg = (reg >>1) ^ MASK;         yes: toss out 1 bit; XOR in mask
 * else reg = reg >>1;                  no: toss out 0 bit 
 */

static size_t randMasks[32] = {
    /* Mask                             Bit Width       Maximum */ 
    0x0,                                /* 1                    */
    0x03,                               /* 2                  3 */
    0x06,                               /* 3                  7 */
    0x0C,                               /* 4                 15 */
    0x14,                               /* 5                 31 */
    0x30,                               /* 6                 63 */
    0x60,                               /* 7                127 */
    0xB8,                               /* 8                255 */
    0x0110,                             /* 9,               511 */
    0x0240,                             /* 10,             1023 */
    0x0500,                             /* 11,             2047 */
    0x0CA0,                             /* 12              4095 */
    0x1B00,                             /* 13              8191 */
    0x3500,                             /* 14             16383 */
    0x6000,                             /* 15             32767 */
    0xB400,                             /* 16             65535 */
    0x00012000,                         /* 17            131071 */
    0x00020400,                         /* 18            262143 */
    0x00072000,                         /* 19            524287 */
    0x00090000,                         /* 20           1048575 */
    0x00140000,                         /* 21           2097151 */
    0x00300000,                         /* 22           4194303 */
    0x00400000,                         /* 23           8388607 */
    0x00D80000,                         /* 24          16777215 */
    0x01200000,                         /* 25          33554431 */
    0x03880000,                         /* 26          67108863 */
    0x07200000,                         /* 27         134217747 */
    0x09000000,                         /* 28         268435455 */
    0x14000000,                         /* 29         536870911 */
    0x32800000,                         /* 30        1073741823 */
    0x48000000,                         /* 31        2147483647 */
    0xA3000000,                         /* 32        4294967295 */
};
    
/* Computes the number of bits needed for mask of n elements. */
static int
BitWidth(size_t n)             /* find "bit-width" needed to represent N */
{
    int width = 0;              /* initially, no bits needed to represent N */

    while (n != 0) {            /* loop 'til N has been whittled down to 0 */
        n >>= 1;                /* shift N right 1 bit (NB: N is unsigned) */
        width++;                /* and remember how wide N is */
    }                           /* end of loop shrinking N down to nothing */
    return width;               /* return bit positions counted */
} 

/* Fast version of the dissolve algorithm */
long
Blt_Dissolve2(Pict *destPtr, Pict *srcPtr, long start, long finish) 
{
    int rowWidth2, colWidth2;           /* Bit width of rows and columns */
    int regWidth;                       /* "width" of sequence generator */
    long mask;                          /* Mask to XOR with to create
                                         * sequence */
    int rowShift;                       /* Shift distance to get row from
                                         * element */
    int colMask;                        /* Mask to extract column from
                                         * element */
    size_t element;                     /* One element of random
                                         * sequence */
    long count;

    count = finish - start;
    
    /* Find the mask to produce all rows and columns. */
    rowWidth2 = BitWidth(srcPtr->height); 
    colWidth2 = BitWidth(srcPtr->width);  
    regWidth = rowWidth2 + colWidth2;  

    mask = randMasks[regWidth - 1];             

    /* Find values to extract row and col numbers from each element. */
    rowShift = colWidth2;            
    colMask = (1 << colWidth2) - 1; 

    /* bottom bits (col) */

    /* Now cycle through all sequence elements. */
    element = start;                    /* 1st element (could be any
                                         * nonzero) */
    while (count >= 0) {
        size_t x, y;
        
        x = element >> rowShift;
        y = element & colMask;
        if ((y < srcPtr->height) && (x < srcPtr->width))        {
            Blt_Pixel *sp, *dp;
            
            sp = Blt_Picture_Pixel(srcPtr, x, y);
            dp = Blt_Picture_Pixel(destPtr, x, y);
            dp->u32 = sp->u32;
            count--;
        }
        /* Compute the next sequence element */
        if (element & 1) {              
            element = (element >>1) ^ mask; 
        } else {
            element = (element >>1);    
        }
        if (element == 1) {
            element = 0;
            break;
        }
    } 
    {
        Blt_Pixel *sp, *dp;
            
        sp = Blt_Picture_Pixel(srcPtr, 0, 0);
        dp = Blt_Picture_Pixel(destPtr, 0, 0);
        dp->u32 = sp->u32;
    }
    return element;
}                                


/* Crossfade from into to leaving result in destination */
static void
CrossFadePictures(Pict *destPtr, Pict *fromPtr, Pict *toPtr, double opacity) 
{
    int alpha, beta;
    Blt_Pixel *fromRowPtr, *toRowPtr, *destRowPtr;
    int y;

    alpha = (int)(opacity * 255);
    beta = alpha ^ 0xFF; /* beta = 1 - alpha */

    fromRowPtr = fromPtr->bits;
    toRowPtr = toPtr->bits;
    destRowPtr = destPtr->bits;
    for (y = 0; y < destPtr->height; y++) {
        Blt_Pixel *sp1, *sp2, *dp, *send;

        sp1 = fromRowPtr, sp2 = toRowPtr, dp = destRowPtr;
        for (send = sp1 + destPtr->width; sp1 < send; sp1++, sp2++, dp++) {
            int r, g, b, t1, t2;
            
            r = imul8x8(beta, sp1->Red, t1) + imul8x8(alpha, sp2->Red, t2);
            g = imul8x8(beta, sp1->Green, t1) + imul8x8(alpha, sp2->Green, t2);
            b = imul8x8(beta, sp1->Blue, t1) + imul8x8(alpha, sp2->Blue, t2);
            dp->Red   = UCLAMP(r);
            dp->Green = UCLAMP(g);
            dp->Blue  = UCLAMP(b);
            dp->Alpha = 0xFF;
        }
        fromRowPtr += fromPtr->pixelsPerRow;
        toRowPtr   += toPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
}                                

/* Crossfade from into to leaving result in destination */
void
Blt_FadeToColor(Pict *destPtr, Pict *fromPtr, Blt_Pixel *colorPtr,
                double opacity) 
{
    int alpha, beta;
    Blt_Pixel *fromRowPtr, *destRowPtr;
    int y;
    Blt_Pixel color;
    int t;
    
    alpha = (int)(opacity * 255);
    beta = alpha ^ 0xFF; /* beta = 1 - alpha */

    fromRowPtr = fromPtr->bits;
    destRowPtr = destPtr->bits;
    color.Red   = imul8x8(alpha, colorPtr->Red,   t);
    color.Green = imul8x8(alpha, colorPtr->Green, t);
    color.Blue  = imul8x8(alpha, colorPtr->Blue,  t);
    for (y = 0; y < destPtr->height; y++) {
        Blt_Pixel *sp, *dp, *send;

        sp = fromRowPtr, dp = destRowPtr;
        for (send = sp + destPtr->width; sp < send; sp++, dp++) {
            int r, g, b, t;
            
            r = color.Red    + imul8x8(beta, sp->Red, t);
            g = color.Green  + imul8x8(beta, sp->Green, t);
            b = color.Blue   + imul8x8(beta, sp->Blue, t);
            dp->Red   = UCLAMP(r);
            dp->Green = UCLAMP(g);
            dp->Blue  = UCLAMP(b);
            dp->Alpha = 0xFF;
        }
        fromRowPtr += fromPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
}                                

/* Crossfade from into to leaving result in destination */
void
Blt_FadeFromColor(Pict *destPtr, Pict *toPtr, Blt_Pixel *colorPtr,
                  double opacity) 
{
    int alpha, beta;
    Blt_Pixel *toRowPtr, *destRowPtr;
    int y;
    Blt_Pixel color;
    int t;
    
    alpha = (int)(opacity * 255);
    beta = alpha ^ 0xFF; /* beta = 1 - alpha */

    toRowPtr = toPtr->bits;
    destRowPtr = destPtr->bits;
    color.Red = imul8x8(beta, colorPtr->Red, t);
    color.Green = imul8x8(beta, colorPtr->Green, t);
    color.Blue = imul8x8(beta, colorPtr->Blue, t);
    for (y = 0; y < destPtr->height; y++) {
        Blt_Pixel *sp, *dp, *send;

        sp = toRowPtr, dp = destRowPtr;
        for (send = sp + destPtr->width; sp < send; sp++, dp++) {
            int r, g, b, t;
            
            r = color.Red + imul8x8(alpha, sp->Red, t);
            g = color.Green + imul8x8(alpha, sp->Green, t);
            b = color.Blue + imul8x8(alpha, sp->Blue, t);
            dp->Red   = UCLAMP(r);
            dp->Green = UCLAMP(g);
            dp->Blue  = UCLAMP(b);
            dp->Alpha = 0xFF;
        }
        toRowPtr += toPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
}                                

void
Blt_WipePictures(Pict *destPtr, Pict *fromPtr, Pict *toPtr, int direction,
                 double position) 
{
    switch (direction) {
    case TK_ANCHOR_E:
        {
            int x;
            
            x = (int)(position * (fromPtr->width - 1));
            if (x >= fromPtr->width) {
                x = fromPtr->width;
            } else if (x < 0) {
                x = 0;
            }
            Blt_CopyArea(destPtr, toPtr, 0, 0, x, fromPtr->height, 0, 0);
            Blt_CopyArea(destPtr, fromPtr, x, 0, fromPtr->width - x,
                           fromPtr->height, x, 0);
        }
        break;

    case TK_ANCHOR_W:
        {
            int x;
            
            x = (int)((1 - position) * (fromPtr->width - 1));
            if (x >= fromPtr->width) {
                x = fromPtr->width;
            } else if (x < 0) {
                x = 0;
            }
            Blt_CopyArea(destPtr, fromPtr, 0, 0, x, fromPtr->height, 0,0);
            Blt_CopyArea(destPtr, toPtr, x, 0, fromPtr->width - x,
                           fromPtr->height, x, 0);
        }
        break;

    case TK_ANCHOR_N:
        {
            int y;
            
            y = (int)((1 - position) * (fromPtr->height - 1));
            if (y >= fromPtr->width) {
                y = fromPtr->width;
            } else if (y < 0) {
                y = 0;
            }
            Blt_CopyArea(destPtr, fromPtr, 0, 0, fromPtr->width, y, 0, 0);
            Blt_CopyArea(destPtr, toPtr, 0, y, fromPtr->width,
                           fromPtr->height - y, 0, y);
        }
        break;

    case TK_ANCHOR_S:
        {
            int y;
            
            y = (int)(position * (fromPtr->height - 1));
            if (y >= fromPtr->width) {
                y = fromPtr->width;
            } else if (y < 0) {
                y = 0;
            }
            Blt_CopyArea(destPtr, fromPtr, 0, 0, fromPtr->width, y, 0, 0);
            Blt_CopyArea(destPtr, toPtr, 0, y, fromPtr->width,
                           fromPtr->height - y, 0, y);
        }
        break;
    }
}                                


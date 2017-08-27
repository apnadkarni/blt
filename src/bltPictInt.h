/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictInt.h --
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

#ifndef _BLT_PIC_INT_H
#define _BLT_PIC_INT_H

/*
 * Scaled integers are fixed point values.  The upper 18 bits is the
 * integer portion, the lower 14 bits the fractional remainder.  Must be
 * careful not to overflow the values (especially during multiplication).
 *
 * The following operations are defined:
 * 
 *      S * n           Scaled integer times an integer.
 *      S1 + S2         Scaled integer plus another scaled integer.
 */
#define float2si(f)     (int)((f) * 16383.0 + ((f < 0) ? -0.5 : 0.5))
#define uchar2si(b)     (((int)(b)) << 14)
#define si2int(s)       (((s) + 8192) >> 14)
#define int2si(i)       (((i) << 14) - 8192)
#define si2float(s)     ((s) / 16383.0)

/* 
 * The following macro converts a fixed-point scaled integer to a byte,
 * clamping the value between 0 and 255.
 */
#define SICLAMP(s) \
    (unsigned char)(((s) < 0) ? 0 : ((s) > 4177920) ? 255 : (si2int(s)))

#define div255(i)       ((((int)(i) + 1) + (((int)(i) + 1) >> 8) ) >> 8)
#define div257(t)       (((t)+((t)>>8))>>8)
#define mul255(i)       (((int)(i) << 8) - ((int)(i)))

#define RGBIndex(r,g,b) (((r)<<10) + ((r)<<6) + (r) + ((g) << 5) + (g) + (b))

#define ROTATE_0        0
#define ROTATE_90       1
#define ROTATE_180      2
#define ROTATE_270      3

#define UCLAMP(s) (unsigned char)(((s) < 0) ? 0 : ((s) > 255) ? 255 : (s))

/*
 *---------------------------------------------------------------------------
 *
 * ResampleFilterProc --
 *
 *      A function implementing a 1-D filter.
 *
 *---------------------------------------------------------------------------
 */
typedef double (Blt_ResampleFilterProc) (double value);

/*
 *---------------------------------------------------------------------------
 *
 * ResampleFilter --
 *
 *      Contains information about a 1-D filter (its support and
 *      the procedure implementing the filter).
 *
 *---------------------------------------------------------------------------
 */
struct _Blt_ResampleFilter {
    const char *name;                   /* Name of the filter */
    Blt_ResampleFilterProc *proc;       /* 1-D filter procedure. */
    double support;                     /* Width of 1-D filter */
};

/*
 *---------------------------------------------------------------------------
 *
 * TableFilter --
 *
 *      Contains information about a 1-D filter (its support and the
 *      procedure implementing the filter).
 *
 *---------------------------------------------------------------------------
 */
struct _Blt_TableFilter {
    float scale;
    int numWeights;                     /* Width of 1-D filter */
    int weights[1];
};

#define RESAMPLE_FILTER 0
#define TABLE_FILTER    1
struct _Blt_ConvolveFilter {
    int type;
    void *filter;
};

/*
 *---------------------------------------------------------------------------
 *
 * Filter2D --
 *
 *      Defines a convolution mask for a 2-D filter.  Used to smooth or
 *      enhance images.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    double support;                     /* Radius of filter */
    double sum, scale;                  /* Sum of kernel */
    double *kernel;                     /* Array of values (malloc-ed)
                                         * representing the discrete 2-D
                                         * filter. */
} Filter2D;

/*
 * We can use scaled integers (20-bit fraction) to compute the luminosity with
 * reasonable accuracy considering it's stored in an 8-bit result.
 */
#define YR      223002                  /* 0.212671 */
#define YG      749900                  /* 0.715160 */
#define YB      75675                   /* 0.072169 */
#define YMAX    267386880               /* 255.0 */
#define YCLAMP(s) \
        (unsigned char)((s) > YMAX) ? 255 : ((((s) + 524288) >> 20))
            
#define JITTER(x)       ((x) * (0.05 - drand48() * 0.10))
#define JCLAMP(c)       ((((c) < 0.0) ? 0.0 : ((c) > 1.0) ? 1.0 : (c)))

typedef union {
    int i32;                            /* Fixed point (scaled
                                         * integer). 14-bit fraction. */
    float f32;
} PixelWeight;

typedef struct {
    int start;
    int numWeights;
    PixelWeight *wend;                  /* Points to just beyond the last
                                         * weight.  Tracks the number of
                                         * weights in array below. */
    PixelWeight weights[1];             /* Array of weights. */
    
} Sample;

typedef struct _Blt_Picture Pict;
typedef struct _Blt_ResampleFilter ResampleFilter;
typedef struct _Blt_TableFilter TableFilter;

BLT_EXTERN unsigned int Blt_ComputeWeights(unsigned int sw, unsigned int dw, 
        ResampleFilter *filterPtr, Sample **samplePtrPtr);

typedef void (Blt_ApplyPictureToPictureProc)(Blt_Picture dest, Blt_Picture src,
        int x, int y, int w, int h, int dx, int dy, Blt_PictureArithOps op);
typedef void (Blt_ApplyScalarToPictureProc)(Blt_Picture dest, 
        Blt_Pixel *colorPtr, Blt_PictureArithOps op);
typedef void (Blt_ApplyPictureToPictureWithMaskProc)(Blt_Picture dest, 
        Blt_Picture src, Blt_Picture mask, int x, int y, int w, int h, 
        int dx, int dy, int invert, Blt_PictureArithOps op);
typedef void (Blt_ApplyScalarToPictureWithMaskProc)(Blt_Picture dest, 
        Blt_Pixel *colorPtr, Blt_Picture mask, 
        int invert, Blt_PictureArithOps op);
typedef void (Blt_TentHorizontallyProc)(Blt_Picture dest, Blt_Picture src);
typedef void (Blt_TentVerticallyProc)(Blt_Picture dest, Blt_Picture src);
typedef void (Blt_ZoomHorizontallyProc)(Blt_Picture dest, Blt_Picture src, 
        Blt_ResampleFilter filter);
typedef void (Blt_ZoomVerticallyProc)(Blt_Picture dest, Blt_Picture src, 
        Blt_ResampleFilter filter);
typedef void (Blt_CompositeRegionProc)(Blt_Picture dest, Blt_Picture src, 
        int sx, int sy, int w, int h, int dx, int dy);
typedef void (Blt_CompositePicturesProc)(Blt_Picture dest, Blt_Picture src);
typedef void (Blt_CopyPictureBitsProc)(Blt_Picture dest, Blt_Picture src);
typedef void (Blt_SelectPixelsProc)(Blt_Picture dest, Blt_Picture src, 
        Blt_Pixel *lowPtr , Blt_Pixel *highPtr);
typedef void (Blt_PremultiplyColorsProc)(Blt_Picture picture);
typedef void (Blt_UnmultiplyColorsProc)(Blt_Picture picture);
typedef void (Blt_CopyRegionProc)(Blt_Picture dest, Blt_Picture src, 
        int sx, int sy, int w, int h, int dx, int dy);
typedef void (Blt_CrossFadePicturesProc)(Blt_Picture dest, Blt_Picture from,
        Blt_Picture to, double opacity);
typedef void (Blt_BlankPictureProc)(Blt_Picture dest, unsigned int value);

typedef struct {
    Blt_ApplyPictureToPictureProc *applyPictureToPictureProc;
    Blt_ApplyScalarToPictureProc  *applyScalarToPictureProc;
    Blt_ApplyPictureToPictureWithMaskProc *applyPictureToPictureWithMaskProc;
    Blt_ApplyScalarToPictureWithMaskProc *applyScalarToPictureWithMaskProc;
    Blt_TentHorizontallyProc *tentHorizontallyProc;
    Blt_TentVerticallyProc *tentVerticallyProc;
    Blt_ZoomHorizontallyProc *zoomHorizontallyProc;
    Blt_ZoomVerticallyProc *zoomVerticallyProc;
    Blt_CompositeRegionProc *compositeRegionProc;
    Blt_CompositePicturesProc *compositePicturesProc;
    Blt_SelectPixelsProc *selectPixelsProc;
    Blt_PremultiplyColorsProc *premultiplyColorsProc;
    Blt_UnmultiplyColorsProc *unassociateColorsProc;
    Blt_CopyRegionProc *copyRegionProc;
    Blt_CopyPictureBitsProc *copyPictureBitsProc;
    Blt_CrossFadePicturesProc *crossFadePicturesProc;
    Blt_BlankPictureProc *blankPictureProc;
    Blt_ZoomHorizontallyProc *zoomHorizontallyProc2;
    Blt_ZoomVerticallyProc *zoomVerticallyProc2;
} Blt_PictureProcs;

BLT_EXTERN Blt_PictureProcs *bltPictProcsPtr;

#endif /*_BLT_PIC_INT_H*/

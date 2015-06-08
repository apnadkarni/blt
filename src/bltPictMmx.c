/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictMMX.c --
 *
 * This module implements image processing procedures for the BLT toolkit.
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
  #include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltChain.h"
#include "bltPicture.h"
#include "bltPictInt.h"

#define imul8x8(a,b,t)  ((t) = (a)*(b)+128,(((t)+((t)>>8))>>8))

#ifdef HAVE_X86_ASM

#define FEATURE_MMX                (1L << 23)
#define FEATURE_MMXEXT             (1L << 24)
#define FEATURE_3DNOW              (1L << 31)
#define FEATURE_SSE                (1L << 25)
#define FEATURE_SSE2               (1L << 26)
#define FEATURE_SSSE3              (1L << 41)
#define FEATURE_SSE41              (1L << 51)
#define FEATURE_SSE42              (1L << 52)

typedef struct  {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
} X86Registers;

static Blt_ApplyPictureToPictureProc ApplyPictureToPicture;
static Blt_ApplyScalarToPictureProc  ApplyScalarToPicture;
#ifdef notdef
static Blt_ApplyPictureToPictureWithMaskProc ApplyPictureToPictureWithMask;
static Blt_ApplyScalarToPictureWithMaskProc  ApplyScalarToPictureWithMask;
static Blt_UnmultiplyColorsProc UnassociateColors;
#endif
static Blt_CompositePicturesProc CompositePictures;
static Blt_CopyPicturesProc CopyPictures;
static Blt_CrossFadePicturesProc CrossFadePictures;
static Blt_PremultiplyColorsProc PremultiplyColors;
static Blt_SelectPixelsProc SelectPixels;
static Blt_TentHorizontallyProc TentHorizontally;
static Blt_TentVerticallyProc TentVertically;
static Blt_ZoomHorizontallyProc ZoomHorizontally;
static Blt_ZoomVerticallyProc ZoomVertically;
static Blt_BlankPictureProc BlankPicture;

/* 
 * unmultiply,  zoomvertically, zoomhorizontally
 */
/* 
 * sR sG sB sA 
 * dA dA dA dA 
 * dR sG dB dA
 * sA sA sA sA 
 *
 */
static void
SelectPixels(Pict *destPtr, Pict *srcPtr, Blt_Pixel *lowerPtr,
             Blt_Pixel *upperPtr)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    if (srcPtr != destPtr) {
        Blt_ResizePicture(destPtr, srcPtr->width, srcPtr->height);
    }

    asm volatile (
        /* Put lower and upper pixels in registers. */
        "movd %0, %%mm4         # mm4 = L\n\t"
        "movd %1, %%mm5         # mm5 = H\n\t"
        "pxor %%mm6, %%mm6      # mm6 = 0\n\t"
        "punpckldq %%mm4, %%mm4 # mm4 = L,L\n\t" 
        "punpckldq %%mm5, %%mm5 # mm5 = H,H\n\t" 
        : /* outputs */ 
        : /* inputs */
          "r" (lowerPtr->u32), 
          "r" (upperPtr->u32));

    destRowPtr = destPtr->bits, srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *dp, *sp, *send;

        dp = destRowPtr;
        for(sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp += 2) {
            asm volatile (
                /* Compare two pixels at a time */
                "movq (%1), %%mm3       # mm3 = S1,S2\n\t"
                "movq %%mm4, %%mm0      # mm0 = L,L\n\t"

                /* We want to test (S >= L) && (S <= H). Since the operands
                 * are all unsigned, pcmp* ops are out.  Instead use
                 * saturated, unsigned subtraction.  ((L psub S) == 0) is
                 * the same as (S >= L) */

                "psubusb %%mm3, %%mm0   # mm0 = L - S\n\t"
                "movq %%mm3, %%mm1      # mm1 = S\n\t"
                "psubusb %%mm5, %%mm1   # mm1 = S - H\n\t"

                /* "or" the two results and compare 32-bit values to 0
                 * (inverting the logic). */

                "por %%mm1, %%mm0       # mm0 = (S >= L)|(H >= S)\n\t"
                "pcmpeqd %%mm6, %%mm0   # invert logic\n\t"
                "movq %%mm0, (%0)       # dp = new value\n" 
                : /* outputs */
                  "+r" (dp) 
                : /* inputs */
                  "r" (sp));
            dp += 2;
        }
        srcRowPtr  += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
    asm volatile ("emms");
    destPtr->flags &= ~BLT_PIC_COMPOSITE;
    destPtr->flags |= BLT_PIC_MASK;
}

static void
AddPictureToPicture(Pict *destPtr, Pict *srcPtr, int x, int y, int w, int h, 
                    int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    asm volatile (
        /* Generate constants needed below. */
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pcmpeqw %mm7, %mm7     # mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *dend;

        sp = srcRowPtr;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
            asm volatile (
                "movq (%0), %%mm0\n\t" 
                "paddusb (%1), %%mm0\n\t" 
                "movq %%mm0, (%0)" 
                : /* outputs */
                  "+r" (dp) 
                : /* inputs */
                  "r" (sp));
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

static void
SubPictureToPicture(Pict *destPtr, Pict *srcPtr, int x, int y, int w, int h, 
                    int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    asm volatile (
        /* Generate constants needed below. */
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pcmpeqw %mm7, %mm7     # mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *dend;

        sp = srcRowPtr;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
            asm volatile (
                "movq (%0), %%mm0\n\t" 
                "psubusb (%1), %%mm0\n\t" 
                "movq %%mm0, (%0)" 
                : /* outputs */
                  "+r" (dp) 
                : /* inputs */
                  "r" (sp));
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

static void
RSubPictureToPicture(Pict *destPtr, Pict *srcPtr, int x, int y, int w, int h, 
                     int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    asm volatile (
        /* Generate constants needed below. */
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pcmpeqw %mm7, %mm7     # mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *dend;

        sp = srcRowPtr;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
            asm volatile (
                "movq (%1), %%mm1\n\t" 
                "psubusb (%0), %%mm1\n\t" 
                "movq %%mm1, (%0)" 
                : /* outputs */
                  "+r" (dp) 
                : /* inputs */
                  "r" (sp));
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

static void
AndPictureToPicture(Pict *destPtr, Pict *srcPtr, int x, int y,  int w, int h, 
                    int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    asm volatile (
        /* Generate constants needed below. */
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pcmpeqw %mm7, %mm7     # mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *dend;

        sp = srcRowPtr;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
            asm volatile (
                "movq (%0), %%mm0\n\t" 
                "pand (%1), %%mm0\n\t" 
                "movq %%mm0, (%0)" 
                : /* outputs */
                  "+r" (dp) 
                : /* inputs */
                  "r" (sp));
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

static void
OrPictureToPicture(Pict *destPtr, Pict *srcPtr, int x, int y, int w, int h, 
                   int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    asm volatile (
        /* Generate constants needed below. */
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pcmpeqw %mm7, %mm7     # mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *dend;

        sp = srcRowPtr;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
            asm volatile (
                "movq (%0), %%mm0\n\t" 
                "por (%1), %%mm0\n\t" 
                "movq %%mm0, (%0)" 
                : /* outputs */
                  "+r" (dp) 
                : /* inputs */
                  "r" (sp));
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

static void
NandPictureToPicture(Pict *destPtr, Pict *srcPtr, int x, int y, int w, int h, 
                     int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    asm volatile (
        /* Generate constants needed below. */
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pcmpeqw %mm7, %mm7     # mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *dend;

        sp = srcRowPtr;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
            asm volatile (
                "movq (%0), %%mm0\n\t" 
                "pand (%1), %%mm0\n\t" 
                "pxor %%mm7, %%mm0\n\t" 
                "movq %%mm0, (%0)" 
                : /* outputs */
                  "+r" (dp) 
                : /* inputs */
                  "r" (sp));
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

static void
NorPictureToPicture(Pict *destPtr, Pict *srcPtr, int x, int y, int w, int h, 
                    int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    asm volatile (
        /* Generate constants needed below. */
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pcmpeqw %mm7, %mm7     # mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *dend;

        sp = srcRowPtr;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
            asm volatile (
                "movq (%0), %%mm0\n\t" 
                "por (%1), %%mm0\n\t" 
                "pxor %%mm7, %%mm0\n\t" 
                "movq %%mm0, (%0)" 
                : /* outputs */
                  "+r" (dp) 
                : /* inputs */
                  "r" (sp));
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

static void
XorPictureToPicture(Pict *destPtr, Pict *srcPtr, int x, int y, int w, int h, 
                    int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    asm volatile (
        /* Generate constants needed below. */
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pcmpeqw %mm7, %mm7     # mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *dend;

        sp = srcRowPtr;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
            asm volatile (
                "movq (%0), %%mm0\n\t" 
                "pxor (%1), %%mm0\n\t" 
                "movq %%mm0, (%0)" 
                : /* outputs */
                  "+r" (dp) 
                : /* inputs */
                  "r" (sp));
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

static void
MinPictureToPicture(Pict *destPtr, Pict *srcPtr, int x, int y, int w, int h, 
                    int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    asm volatile (
        /* Generate constants needed below. */
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pcmpeqw %mm7, %mm7     # mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *dend;

        sp = srcRowPtr;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
            asm volatile (
                "movq (%0), %%mm0               # mm0 = A\n\t" 
                "movq (%1), %%mm1               # mm1 = B\n\t" 
                "movq %%mm0, %%mm2              # mm2 = A\n\t" 
                "psubusb %%mm1, %%mm2           # mm2 = A - B\n\t"
                "pcmpeqb %%mm6, %%mm2           # mm2 = 0s A>B 1s A<=B\n\t"
                "pand %%mm2, %%mm0              # mm2 = mask & A\n\t" 
                "pxor %%mm7, %%mm2              # mm2 = ~mask\n\t" 
                "pand %%mm2, %%mm1              # mm0 = ~mask & B\n\t" 
                "por %%mm1, %%mm0               # mm0 = R1 | R2\n\t" 
                "movq %%mm0, (%0)" 
                : /* outputs */
                  "+r" (dp) 
                : /* inputs */
                  "r" (sp));
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

static void
MaxPictureToPicture(Pict *destPtr, Pict *srcPtr, int x, int y, int w, int h, 
                    int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    asm volatile (
        /* Generate constants needed below. */
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pcmpeqw %mm7, %mm7     # mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *dend;

        sp = srcRowPtr;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
            asm volatile (
                "movq (%0), %%mm0               # mm0 = A\n\t" 
                "movq (%1), %%mm1               # mm1 = B\n\t" 
                "movq %%mm0, %%mm2              # mm2 = A\n\t" 
                "psubusb %%mm1, %%mm2           # mm2 = A - B\n\t"
                "pcmpeqb %%mm6, %%mm2           # mm2 = 0s A>B 1s A<=B\n\t"
                "pand %%mm2, %%mm1              # mm1 = mask & B\n\t" 
                "pxor %%mm7, %%mm2              # mm2 = ~mask\n\t" 
                "pand %%mm2, %%mm0              # mm0 = ~mask & A\n\t" 
                "por %%mm1, %%mm0               # mm3 = R1 | R2\n\t" 
                "movq %%mm0, (%0)" 
                : /* outputs */
                  "+r" (dp) 
                : /* inputs */
                  "r" (sp));
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

static void
ApplyPictureToPicture(
    Pict *destPtr, 
    Pict *srcPtr, 
    int x, int y, int w, int h, int dx, int dy, 
    Blt_PictureArithOps op)
{
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
    switch(op) {
    case PIC_ARITH_ADD:
        AddPictureToPicture(destPtr, srcPtr, x, y, w, h, dx, dy);
        break;
    case PIC_ARITH_SUB:
        SubPictureToPicture(destPtr, srcPtr, x, y, w, h, dx, dy);
        break;
    case PIC_ARITH_RSUB:
        RSubPictureToPicture(destPtr, srcPtr, x, y, w, h, dx, dy);
        break;
    case PIC_ARITH_AND:
        AndPictureToPicture(destPtr, srcPtr, x, y, w, h, dx, dy);
        break;
    case PIC_ARITH_OR:
        OrPictureToPicture(destPtr, srcPtr, x, y, w, h, dx, dy);
        break;
    case PIC_ARITH_NAND:
        NandPictureToPicture(destPtr, srcPtr, x, y, w, h, dx, dy);
        break;
    case PIC_ARITH_NOR:
        NorPictureToPicture(destPtr, srcPtr, x, y, w, h, dx, dy);
        break;
    case PIC_ARITH_XOR:
        XorPictureToPicture(destPtr, srcPtr, x, y, w, h, dx, dy);
        break;
    case PIC_ARITH_MIN:
        MinPictureToPicture(destPtr, srcPtr, x, y, w, h, dx, dy);
        break;
    case PIC_ARITH_MAX:
        MaxPictureToPicture(destPtr, srcPtr, x, y, w, h, dx, dy);
        break;
    }
}


static void
ApplyScalarToPicture(
    Pict *srcPtr, 
    Blt_Pixel *colorPtr,
    Blt_PictureArithOps op)
{
    Blt_Pixel *srcRowPtr;
    int y;
    
    /*
     * mm7 = -1
     * mm6 = 0x0
     * mm4 = scalar,scalar
     */
    asm volatile (
        /* Generate constants needed below. */
        "pxor %%mm6, %%mm6        # mm6 = 0\n\t"
        "pcmpeqw %%mm7, %%mm7     # mm5 = -1 \n\t"
        /* Put the scalar into hi/lo 32-bit words.*/
        "movd %0, %%mm4           # mm4 = scalar\n\t"
        "punpckldq %%mm4, %%mm4   # mm2 = S,S\n" 
        : /* outputs */
        : /* inputs */
          "r" (colorPtr->u32));

    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;

        sp = srcRowPtr;
        send = sp + srcPtr->width;
        switch(op) {
        case PIC_ARITH_ADD:
            while (sp < send) {
                asm volatile (
                    "movq (%0), %%mm0\n\t" 
                    "paddusb %%mm4, %%mm0\n\t" 
                    "movq %%mm0, (%0)" 
                    : /* outputs */
                      "+r" (sp));
                sp += 2;
            }
            break;

        case PIC_ARITH_SUB:
            while (sp < send) {
                asm volatile (
                     "movq (%0), %%mm0\n\t" 
                     "psubusb %%mm4, %%mm0\n\t" 
                     "movq %%mm0, (%0)" 
                     : /* outputs */
                       "+r" (sp));
                sp += 2;
            }
            break;

        case PIC_ARITH_RSUB:
            while (sp < send) {
                asm volatile (
                     "movq (%0), %%mm0\n\t" 
                     "movq %%mm4, %%mm1\n\t"
                     "psubusb %%mm0, %%mm1\n\t" 
                     "movq %%mm1, (%0)" 
                     : /* outputs */
                       "+r" (sp));
                sp += 2;
            }
            break;

        case PIC_ARITH_AND:
            while (sp < send) {
                asm volatile (
                    "movq (%0), %%mm0\n\t" 
                    "pand %%mm4, %%mm0\n\t" 
                    "movq %%mm0, (%0)" 
                    : /* outputs */
                      "+r" (sp));
                sp += 2;
            }
            break;

        case PIC_ARITH_OR:
            while (sp < send) {
                asm volatile (
                    "movq (%0), %%mm0\n\t" 
                    "por %%mm4, %%mm0\n\t" 
                    "movq %%mm0, (%0)" 
                    : /* outputs */
                      "+r" (sp));
                sp += 2;
            }
            break;

        case PIC_ARITH_NAND:
            while (sp < send) {
                asm volatile (
                    "movq (%0), %%mm0\n\t" 
                    "pand %%mm4, %%mm0\n\t" 
                    "pxor %%mm7, %%mm0\n\t" 
                    "movq %%mm0, (%0)" 
                    : /* outputs */
                      "+r" (sp));
                sp += 2;
            }
            break;

        case PIC_ARITH_NOR:
            while (sp < send) {
                asm volatile (
                    "movq (%0), %%mm0\n\t" 
                    "por %%mm4, %%mm0\n\t" 
                    "pxor %%mm7, %%mm0\n\t" 
                    "movq %%mm0, (%0)" 
                    : /* outputs */
                      "+r" (sp));
                sp += 2;
            }
            break;

        case PIC_ARITH_XOR:
            while (sp < send) {
                asm volatile (
                    "movq (%0), %%mm0\n\t" 
                    "pxor %%mm4, %%mm0\n\t" 
                    "movq %%mm0, (%0)" 
                    : /* outputs */
                      "+r" (sp));
                sp += 2;
            }
            break;

        case PIC_ARITH_MIN:
            while (sp < send) {
                asm volatile (
                    "movq (%0), %%mm0     # mm0 = Color\n\t" 
                    "movq %%mm0, %%mm1    # mm1 = Color\n\t" 
                    "psubusb %%mm4, %%mm1 # mm1 = C - S\n\t"
                    "pcmpeqb %%mm6, %%mm1 # mm2 = mask: 0s C>S 1s C<=S\n\t"
                    "pand %%mm1, %%mm0    # mm0 = mask & C\n\t" 
                    "pxor %%mm7, %%mm1    # mm1 = ~mask\n\t" 
                    "pand %%mm4, %%mm1    # mm1 = S & ~mask\n\t" 
                    "por %%mm1, %%mm0     # mm0 = (S&~mask)|(mask&C)\n\t" 
                    "movq %%mm0, (%0)" 
                    : /* outputs */
                      "+r" (sp));
                sp += 2;
            }
            break;

        case PIC_ARITH_MAX:
            while (sp < send) {
                asm volatile (
                    "movq (%0), %%mm0     # mm0 = Color\n\t" 
                    "movq %%mm4, %%mm1    # mm1 = Scalar\n\t" 
                    "psubusb %%mm0, %%mm1 # mm1 = S - C\n\t"
                    "pcmpeqb %%mm6, %%mm1 # mm1 = mask: 0s S>C 1s S<=C\n\t"
                    "pand %%mm1, %%mm0    # mm0 = mask & C\n\t" 
                    "pxor %%mm7, %%mm1    # mm1 = ~mask\n\t" 
                    "pand %%mm4, %%mm1    # mm1 = S & ~mask\n\t" 
                    "por %%mm1, %%mm0     # mm0 = (S&~mask)|(mask&C)\n\t" 
                    "movq %%mm0, (%0)" 
                    : /* outputs */
                      "+r" (sp));
                sp += 2;
            }
            break;
        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

static void
ZoomVertically(Pict *destPtr, Pict *srcPtr, ResampleFilter *filterPtr)
{
    Sample *samples, *send;
    int x;
    int bytesPerSample;                 /* Size of sample. */

    /* Pre-calculate filter contributions for each row. */
    bytesPerSample = Blt_ComputeWeights(srcPtr->height, destPtr->height, 
        filterPtr, &samples);
    send = (Sample *)((char *)samples + (destPtr->height * bytesPerSample));

    asm volatile (
        /* Generate constants needed below. */
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pcmpeqw %mm2, %mm2     # mm2 = -1 \n\t"
        "psubw %mm6, %mm2       # mm2 = 1,1,1,1\n\t"
        "psllw $4, %mm2         # mm2 = BIAS\n");
    /* Apply filter to each row. */
    for (x = 0; x < srcPtr->width; x++) {
        Blt_Pixel *dp, *srcColPtr;
        Sample *splPtr;

        srcColPtr = srcPtr->bits + x;
        dp = destPtr->bits + x;
        for (splPtr = samples; splPtr < send; 
             splPtr = (Sample *)((char *)splPtr + bytesPerSample)) {
            Blt_Pixel *sp;
            PixelWeight *wp;

            asm volatile (
                /* Clear the accumulator mm5. */
                "pxor %mm5, %mm5        #  mm5 = 0\n\t");
            sp = srcColPtr + (splPtr->start * srcPtr->pixelsPerRow);
            for (wp = splPtr->weights; wp < splPtr->wend; wp++) {
                asm volatile (
                    /* Load the weighting factor into mm1. */
                    "movd (%0), %%mm1          #  mm1 = 0,0,0,W\n\t"
                    /* Load the source pixel into mm0. */
                    "movd (%1), %%mm0          #  mm0 = S\n\t" 
                    /* Unpack the weighting factor into mm1. */
                    "punpcklwd %%mm1, %%mm1    #  mm1 = 0,0,W,W\n\t"
                    /* Unpack the pixel components into 16-bit words.*/
                    "punpcklbw %%mm6, %%mm0    #  mm0 = Sa,Sb,Sg,Sr\n\t" 
                    /*  */
                    "punpcklwd %%mm1, %%mm1    #  mm1 = W,W,W,W\n\t"
                    /* Scale the 8-bit components to 15 bits. (S * 257) >> 1 */
                    "movq %%mm0, %%mm3         #  mm3 = S8\n\t" 
                    "psllw $8, %%mm3           #  mm3 = S8 * 256\n\t" 
                    "paddw %%mm3, %%mm0        #  mm0 = S16\n\t" 
                    "psrlw $1, %%mm0           #  mm0 = S15\n\t" 
                    /* Multiple each pixel component by the weight.  Note that
                     * the lower 16-bits of the product are truncated (bad)
                     * creating round-off error in the sum. */
                    "pmulhw %%mm1, %%mm0       #  mm0 = S15 * W14\n\t" 
                    /* Accumulate upper 16-bit results of product in mm5. */
                    "paddsw %%mm0, %%mm5       #  mm5 = prod + mm5\n\t" 
                    : /* outputs */ 
                    : /* inputs */
                      "r" (wp), 
                      "r" (sp));
                sp += srcPtr->pixelsPerRow;
            }
            asm volatile (
                /* Add a rounding bias to the pixel sum */
                "paddw %%mm2, %%mm5        # mm5 = A13 + BIAS\n\t" 
                /* Shift off fractional part */
                "psraw $5, %%mm5           # mm5 = A8\n\t" 
                /* Pack 16-bit components into lower 4 bytes. */
                "packuswb  %%mm5, %%mm5    # Pack 4 low-order bytes.\n\t" 
                /* Save the word (pixel) in the destination. */
                "movd %%mm5,(%0)           # dp = word\n" 
                : /* outputs */ 
                  "+r" (dp));
            dp += destPtr->pixelsPerRow;

        }
    }
    asm volatile ("emms");
    /* Free the memory allocated for filter weights. */
    Blt_Free(samples);
}

static void
ZoomHorizontally(Pict *destPtr, Pict *srcPtr, ResampleFilter *filterPtr)
{
    Sample *samples, *send;
    int y;
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int bytesPerSample;                 /* Size of sample. */

    /* Pre-calculate filter contributions for each column. */
    bytesPerSample = Blt_ComputeWeights(srcPtr->width, destPtr->width, 
        filterPtr, &samples);
    send = (Sample *)((char *)samples + (destPtr->width * bytesPerSample));

    /* Apply filter to each column. */
    srcRowPtr = srcPtr->bits;
    destRowPtr = destPtr->bits;

    asm volatile (
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pxor %mm3, %mm3        # mm3 = 0\n\t"
        "pcmpeqw %mm2, %mm2     # mm2 = -1\n\t"
        "psubw %mm3, %mm2       # mm2 = 1,1,1,1\n\t"
        "psllw $4, %mm2         # mm2 = BIAS\n");

    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *dp;
        Sample *splPtr;

        dp = destRowPtr;
        for (splPtr = samples; splPtr < send; 
             splPtr = (Sample *)((char *)splPtr + bytesPerSample)) {
            Blt_Pixel *sp;
            PixelWeight *wp;

            sp = srcRowPtr + splPtr->start;
            asm volatile (
                /* Clear the accumulator mm5. */
                "pxor %mm5, %mm5        #  mm5 = 0\n\n");
            for (wp = splPtr->weights; wp < splPtr->wend; wp++) {
                asm volatile (
                   /* Load the weighting factor into mm1. */
                   "movd (%0), %%mm1         #  mm1 = W\n\t" 
                   /* Get the source RGBA pixel. */
                   "movd (%1), %%mm0         #  mm0 = sp\n\t" 
                   /* Unpack the weighting factor into mm1. */
                   "punpcklwd %%mm1, %%mm1   #  mm1 = 0,0,W,W\n\t"
                   /* Unpack the pixel into mm0. */
                   "punpcklbw %%mm6, %%mm0   #  mm0 = Sa,Sr,Sg,Sb\n\t" 
                   /*  */
                   "punpcklwd %%mm1, %%mm1   #  mm1 = W,W,W,W\n\t"
                   /* Scale the 8-bit components to 15 bits: (S * 257) >> 1 */
                   "movq %%mm0, %%mm3        #  mm3 = S8\n\t" 
                   "psllw $8, %%mm3          #  mm3 = S8 * 256\n\t" 
                   "paddw %%mm3, %%mm0       #  mm0 = S16\n\t" 
                   "psrlw $1, %%mm0          #  mm0 = S15\n\t" 
                   /* Multiple each pixel component by the weight.  It's a
                    * signed mulitply because weights can be negative. Note
                    * that the lower 16-bits of the product are truncated
                    * (bad) creating round-off error in the sum. */
                   "pmulhw %%mm1, %%mm0      #  mm0 = S15 * W14\n\t"  
                   /* Add the 16-bit components to mm5. */
                   "paddsw %%mm0, %%mm5      #  mm5 = A13 + mm5\n\t" 
                   : /* outputs */ 
                   : /* inputs */
                     "r" (wp), 
                     "r" (sp));
                sp++;
            }                   
            asm volatile (
                /* Add a rounding bias to the pixel sum. */
                "paddw %%mm2, %%mm5       # mm5 = A13 + BIAS\n\t" 
                /* Shift off fractional portion. */
                "psraw $5, %%mm5          # mm5 = A8\n\t" 
                /* Pack 16-bit components into lower 4 bytes. */
                "packuswb %%mm5, %%mm5    # Pack A8 into low 4 bytes.\n\t" 
                /* Store the word (pixel) in the destination. */
                "movd %%mm5,(%0)           # dp = word\n" 
                : /* outputs */ 
                  "+r" (dp));
            dp++;
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
    asm volatile ("emms");
    /* Free the memory allocated for horizontal filter weights. */
    Blt_Free(samples);
}


static void
TentVertically(Pict *destPtr, Pict *srcPtr) 
{
    Blt_Pixel *srcColPtr, *destColumnPtr;
    int x;
    size_t numPixels;
    
    asm volatile (
        /* Establish constants used below. */
        "pxor %mm6, %mm6        # mm6 = 0\n");

    numPixels = srcPtr->height * srcPtr->pixelsPerRow; 
    srcColPtr = srcPtr->bits;
    destColumnPtr = destPtr->bits;
    for (x = 0; x < srcPtr->width; x++) {
        Blt_Pixel *dp, *rp, *rend;

        /* 
         * mm0 = 
         * mm1 = unpacked center pixel 
         * mm2 = unpacked left pixel  
         * mm3 = unpacked right pixel 
         * mm4 = 
         * mm5 = 
         * mm6 = 0
         * mm7 = 
         */
        dp = destColumnPtr;
        rp = srcColPtr + srcPtr->pixelsPerRow;
        asm volatile (
            "movd (%2), %%mm1         # mm1 = cp\n\t"
            "movd (%1), %%mm3         # mm3 = rp\n\t"
            "punpcklbw %%mm6, %%mm1   # mm1 = S8\n\t"
            "movq %%mm1, %%mm2        # mm2 = lp = S8\n\t"
            "punpcklbw %%mm6, %%mm3   # mm3 = S8\n\t"
            "movq  %%mm1, %%mm0       # mm0 = cp\n\t"
            "psllw $1, %%mm0          # mm0 = cp << 1\n\t"
            "paddw %%mm2, %%mm0       # mm0 = lp + (cp << 1)\n\t"
            "paddw %%mm3, %%mm0       # mm0 = lp + (cp << 1) + rp\n\t"
            "psraw $2, %%mm0          # mm0 = (lp + (cp << 1) + rp) >> 2\n\t"
            "packuswb %%mm0, %%mm0    # Pack into low 4 bytes.\n\t"  
            "movd %%mm0,(%0)          # dp = word\n\t"
            "movq %%mm3, %%mm1        # cp = rp\n" 
            : /* outputs */ 
              "=r" (dp), 
              "+r" (rp) 
            : /* inputs */
              "r" (srcColPtr));
        dp += destPtr->pixelsPerRow;
        rp += srcPtr->pixelsPerRow;

        for (rend = srcColPtr + numPixels; rp < rend; /*empty*/) {
            asm volatile (
                "movd (%1), %%mm3         #  mm3 = rp\n\t" 
                "punpcklbw %%mm6, %%mm3   #  mm3 = S8\n\t"
                "movq  %%mm1, %%mm0       #  mm0 = cp\n\t"
                "psllw $1, %%mm0          #  mm0 = cp << 1\n\t"
                "paddw %%mm2, %%mm0       #  mm0 = lp + (cp << 1)\n\t"
                "paddw %%mm3, %%mm0       #  mm0 = lp + (cp << 1) + rp\n\t"
                "psraw $2, %%mm0          #  mm0 = (lp + (cp<<1) + rp) >> 2\n\t"
                "packuswb %%mm0, %%mm0    #  Pack into low 4 bytes.\n\t"  
                "movd %%mm0,(%0)          #  dp = word\n\t" 
                "movq %%mm1, %%mm2        #  lp = cp\n\t"
                "movq %%mm3, %%mm1        #  cp = rp\n"
                : /* outputs */ 
                  "=r" (dp), 
                  "+r" (rp));
            dp += destPtr->pixelsPerRow;
            rp += srcPtr->pixelsPerRow;
        }       
        asm volatile (
            "movq %%mm1, %%mm3        #  rp = cp\n\t"
            "movq %%mm1, %%mm0        #  mm0 = cp\n\t"
            "psllw $1, %%mm0          #  mm0 = cp << 1\n\t"
            "paddw %%mm2, %%mm0       #  mm0 = lp + (cp << 1)\n\t"
            "paddw %%mm3, %%mm0       #  mm0 = lp + (cp << 1) + rp\n\t"
            "psraw $2, %%mm0          #  mm0 = (lp + (cp << 1) + rp) >> 2\n\t"
            "packuswb %%mm0, %%mm0    #  Pack into low 4 bytes.\n\t"  
            "movd %%mm0,(%0)          #  dp = word\n" 
            : /* outputs */ 
              "=r" (dp));

        srcColPtr++, destColumnPtr++;
    }
    asm volatile ("emms");
}

static void
TentHorizontally(Pict *destPtr, Pict *srcPtr) 
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    asm volatile (
        /* Establish constants used below. */
        "pxor %mm6, %mm6        # mm6 = 0\n");

    srcRowPtr = srcPtr->bits;
    destRowPtr = destPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *dp;
        Blt_Pixel *rp, *rend;
        
        /* 
         * mm0 = 
         * mm1 = unpacked center pixel 
         * mm2 = unpacked left pixel  
         * mm3 = unpacked right pixel 
         * mm4 = 
         * mm5 = 
         * mm6 = 0
         * mm7 = 
         */
        dp = destRowPtr;
        rp = srcRowPtr + 1;
        asm volatile (
            "movd (%2), %%mm1         #  mm1 = cp\n\t"
            "movq %%mm1, %%mm2        #  mm2 = lp\n\t"
            "movd (%1), %%mm3         #  mm3 = rp\n\t"
            "punpcklbw %%mm6, %%mm1   #  mm1 = S8\n\t"
            "punpcklbw %%mm6, %%mm2   #  mm2 = S8\n\t"
            "punpcklbw %%mm6, %%mm3   #  mm3 = S8\n\t"
            "movq  %%mm1, %%mm0       #  mm0 = cp\n\t"
            "psllw $1, %%mm0          #  mm0 = cp << 1\n\t"
            "paddw %%mm2, %%mm0       #  mm0 = lp + (cp << 1)\n\t"
            "paddw %%mm3, %%mm0       #  mm0 = lp + (cp << 1) + rp\n\t"
            "psraw $2, %%mm0          #  mm0 = (lp + (cp << 1) + rp) >> 2\n\t"
            "packuswb %%mm0, %%mm0    #  Pack into low 4 bytes.\n\t"  
            "movd %%mm0,(%0)          #  dp = word\n\t"
            "movq %%mm3, %%mm1        #  cp = rp\n" 
            : /* outputs */ 
              "=r" (dp), 
              "+r" (rp) 
            : /* inputs */
              "r" (srcRowPtr));
        dp++, rp++;

        for (rend = srcRowPtr + srcPtr->width; rp < rend; /*empty*/) {
            asm volatile (
                "movd (%1), %%mm3         # mm3 = rp\n\t" 
                "punpcklbw %%mm6, %%mm3   # mm3 = S8\n\t"
                "movq  %%mm1, %%mm0       #  mm0 = cp\n\t"
                "psllw $1, %%mm0          #  mm0 = cp << 1\n\t"
                "paddw %%mm2, %%mm0       #  mm0 = lp + (cp << 1)\n\t"
                "paddw %%mm3, %%mm0       #  mm0 = lp + (cp << 1) + rp\n\t"
                "psraw $2, %%mm0          #  mm0 = (lp + (cp<<1) + rp) >> 2\n\t"
                "packuswb %%mm0, %%mm0    #  Pack into low 4 bytes.\n\t"  
                "movd %%mm0,(%0)          #  dp = word\n\t" 
                "movq %%mm1, %%mm2        #  lp = cp\n\t"
                "movq %%mm3, %%mm1        #  cp = rp\n" 
                : /* outputs */ 
                  "=r" (dp), 
                  "+r" (rp));
            dp++, rp++;
        }

        asm volatile (
            "movq %%mm1, %%mm3        #  rp = cp\n\t"
            "movq %%mm1, %%mm0        #  mm0 = cp\n\t"
            "psllw $1, %%mm0          #  mm0 = cp << 1\n\t"
            "paddw %%mm2, %%mm0       #  mm0 = lp + (cp << 1)\n\t"
            "paddw %%mm3, %%mm0       #  mm0 = lp + (cp << 1) + rp\n\t"
            "psraw $2, %%mm0          #  mm0 = (lp + (cp << 1) + rp) >> 2\n\t"
            "packuswb %%mm0, %%mm0    #  Pack into low 4 bytes.\n\t"  
            "movd %%mm0,(%0)          #  dp = word\n" 
            : /* outputs */ 
              "=r" (dp));

        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

#ifdef notdef
static void
ConvolvePictureVertically(Pict *destPtr, Pict *srcPtr, 
                          ResampleFilter *filterPtr)
{
    Sample *samples, *send;
    int x;
    int bytesPerSample;         /* Size of sample. */
    long bytesPerRow;

    /* Pre-calculate filter contributions for each row. */
    bytesPerSample = Blt_ComputeWeights(srcPtr->height, destPtr->height, 
        filterPtr, &samples);
    bytesPerRow = sizeof(Blt_Pixel) * srcPtr->pixelsPerRow;
    send = (Sample *)((char *)samples + (destPtr->height * bytesPerSample));

    asm volatile (
        /* Generate constants needed below. */
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pcmpeqw %mm2, %mm2     # mm2 = -1 \n\t"
        "psubw %mm6, %mm2       # mm2 = 1,1,1,1\n\t"
        "psllw $4, %mm2         # mm2 = BIAS\n");

    /* Apply filter to each row. */
    for (x = 0; x < srcPtr->width; x++) {
        Blt_Pixel *dp, *srcColPtr;
        Sample *splPtr;

        srcColPtr = srcPtr->bits + x;
        dp = destPtr->bits + x;
        for (splPtr = samples; splPtr < send; 
             splPtr = (Sample *)((char *)splPtr + bytesPerSample)) {
            Blt_Pixel *sp;

            sp = srcColPtr + (splPtr->start * srcPtr->pixelsPerRow);
            asm volatile (
                /* Clear the accumulator mm5. */
                 "pxor %%mm5, %%mm5         # mm5 = 0\n\n" 
                 ".Lasm%=:\n\t" 
                 /* Load the weighting factor into mm1. */
                 "movd (%1), %%mm1          # mm1 = 0,0,0,W\n\t"
                 /* Load the source pixel into mm0. */
                 "movd (%3), %%mm0          # mm0 = S\n\t" 
                 /* Unpack the weighting factor into mm1. */
                 "punpcklwd %%mm1, %%mm1    # mm1 = 0,0,W,W\n\t"
                 "punpcklwd %%mm1, %%mm1    # mm1 = W,W,W,W\n\t"
                 /* Unpack the pixel components into 16-bit words.*/
                 "punpcklbw %%mm6, %%mm0    # mm0 = Sa,Sb,Sg,Sr\n\t" 
                 /* Scale the 8-bit components to 14 bits. (S * 257) >> 2 */
                 "movq %%mm0, %%mm3         # mm3 = S8\n\t" 
                 "psllw $8, %%mm3           # mm3 = S8 * 256\n\t" 
                 "paddw %%mm3, %%mm0        # mm0 = S16\n\t" 
                 "psrlw $1, %%mm0           # mm0 = S15\n\t" 
                 /* Multiple each pixel component by the weight.  Note that
                  * the lower 16-bits of the product are truncated (bad)
                  * creating round-off error in the sum. */
                 "pmulhw %%mm1, %%mm0       # mm0 = S15 * W14\n\t" 
                 /* Move the pointers to the next weight and pixel */
                 "add $4, %1                # wp++\n\t" 
                 "add %4, %3                # sp++\n\t"
                 /* Accumulate upper 16-bit results of product in mm5. */
                 "paddsw %%mm0, %%mm5        # mm5 = prod + mm5\n\t" 
                 "cmp %2, %1                # wend == wp\n\t" 
                 "jnz .Lasm%=\n\t" 
                 /* end loop */
                 /* Add a rounding bias to the pixel sum */
                 "paddw %%mm2, %%mm5        # mm5 = A13 + BIAS\n\t" 
                 /* Shift off fractional part */
                 "psraw $5, %%mm5           # mm5 = A8\n\t" 
                 /* Pack 16-bit components into lower 4 bytes. */
                 "packuswb  %%mm5, %%mm5    # Pack 4 low-order bytes.\n\t" 
                 /* Save the word (pixel) in the destination. */
                 "movd %%mm5,(%0)           # dp = word\n" 
                 /* outputs */ 
                 : "=r" (dp) 
                 /* inputs */
                 : "r" (splPtr->weights), 
                   "r" (splPtr->wend), 
                   "r" (sp),
                   "r" (bytesPerRow));
#ifdef notdef
            if (dp->Alpha != 0xFF) {g
                fprintf(stdout, "mmx v-alpha=0x%x\n", dp->Alpha);
            }
#endif
            dp += destPtr->pixelsPerRow;

        }
    }
    asm volatile ("emms");
    /* Free the memory allocated for filter weights. */
    Blt_Free(samples);
}

static void
ConvolvePictureHorizontally(Pict *destPtr, Pict *srcPtr, 
                            ResampleFilter *filterPtr)
{
    Sample *samples, *send;
    int y;
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int bytesPerSample;         /* Size of sample. */

    /* Pre-calculate filter contributions for each column. */
    bytesPerSample = Blt_ComputeWeights(srcPtr->width, destPtr->width, 
        filterPtr, &samples);
    send = (Sample *)((char *)samples + (destPtr->width * bytesPerSample));

    /* Apply filter to each column. */
    srcRowPtr = srcPtr->bits;
    destRowPtr = destPtr->bits;

    asm volatile (
        "pxor %mm6, %mm6        # mm6 = 0\n\t"
        "pxor %mm3, %mm3        # mm3 = 0\n\t"
        "pcmpeqw %mm2, %mm2     # mm2 = -1\n\t"
        "psubw %mm3, %mm2       # mm2 = 1,1,1,1\n\t"
        "psllw $4, %mm2         # mm2 = BIAS\n");

    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *dp;
        Sample *splPtr;

        dp = destRowPtr;
        for (splPtr = samples; splPtr < send; 
             splPtr = (Sample *)((char *)splPtr + bytesPerSample)) {

            Blt_Pixel *sp;
            sp = srcRowPtr + splPtr->start;
            asm volatile (
                /* Clear the accumulator mm5. */
                 "pxor %%mm5, %%mm5        #  mm5 = 0\n\n" 
                 ".Lasm%=:\n\t" 
                 /* Load the weighting factor into mm1. */
                 "movd (%1), %%mm1         #  mm1 = W\n\t" 
                 /* Get the source RGBA pixel. */
                 "movd (%3), %%mm0         # mm0 = sp\n\t" 
                 /* Unpack the weighting factor into mm1. */
                 "punpcklwd %%mm1, %%mm1   # mm1 = 0,0,W,W\n\t"
                 "punpcklwd %%mm1, %%mm1   # mm1 = W,W,W,W\n\t"
                 /* Unpack the pixel into mm0. */
                 "punpcklbw %%mm6, %%mm0   # mm0 = Sa,Sr,Sg,Sb\n\t" 
                 /* Scale the 8-bit components to 14 bits: (S * 257) >> 2 */
                 "movq %%mm0, %%mm3        # mm3 = S8\n\t" 
                 "psllw $8, %%mm3          # mm3 = S8 * 256\n\t" 
                 "paddw %%mm3, %%mm0       # mm0 = S16\n\t" 
                 "psrlw $1, %%mm0          # mm0 = S15\n\t" 
                 /* Multiple each pixel component by the weight.  Note that
                  * the lower 16-bits of the product are truncated (bad)
                  * creating round-off error in the sum. */
                 "movq %%mm0, %%mm7        # mm5 = S15\n\t" 
                 "pmulhw %%mm1, %%mm0      # mm0 = S15 * W14\n\t" 
                 "pmullw %%mm1, %%mm7      # mm0 = S15 * W14\n\t" 
                 "psrlw $15, %%mm7         # mm0 = S1\n\t" 
                 "paddsw %%mm7, %%mm0      # mm5 = A13 + mm5\n\t" 
                 
                 /* Add the 16-bit components to mm5. */
                 "paddsw %%mm0, %%mm5      # mm5 = A13 + mm5\n\t" 
                 /* Move the pointers to the next weight and pixel */
                 "add $4, %1               # wp++\n\t" 
                 "add $4, %3               # sp++\n\t" 
                 "cmp %2, %1               # wend == wp\n\t" 
                 "jnz .Lasm%=\n\t" 
                 /* end loop */
                 /* Add a rounding bias to the pixel sum. */
                 "paddw %%mm2, %%mm5       # mm5 = A13 + BIAS\n\t" 
                 /* Shift off fractional portion. */
                 "psraw $5, %%mm5          # mm5 = A8\n\t" 
                 /* Pack 16-bit components into lower 4 bytes. */
                 "packuswb %%mm5, %%mm5    # Pack A8 into low 4 bytes.\n\t" 
                 /* Store the word (pixel) in the destination. */
                 "movd %%mm5,(%0)          # dp = word\n" 
                 /* outputs */ 
                 : "=r" (dp) 
                 /* inputs */
                 : "r" (splPtr->weights), 
                   "r" (splPtr->wend), 
                   "r" (sp));
#ifdef notdef
            if (dp->Alpha != 0xFF) {
                fprintf(stdout, "mmx h-alpha=0x%x\n", dp->Alpha);
            }
#endif
            dp++;
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
    asm volatile ("emms");
    /* Free the memory allocated for horizontal filter weights. */
    Blt_Free(samples);
}
#endif


#ifdef notdef
/* 
 * 
 */
static void
UnassociateColors(Pict *srcPtr)         /* (in/out) picture */
{
    Blt_Pixel *srcRowPtr;
    int y;
    Blt_Pixel mask;

    /* Create mask for alpha component.  We'll use this mask to make sure we
     * don't change the alpha component of a pixel.  */
    mask.u32 = 0;
    mask.Alpha = 0xFF;
    asm volatile (
        /* Generate constants needed below. */
        "pxor %%mm6, %%mm6      # mm6 = 0\n\t"
        "pcmpeqw %%mm5, %%mm5   # mm5 = -1 \n\t"
        "psubw %%mm6, %%mm5     # mm5 = 1,1,1,1\n\t"
        "movd %0, %%mm4         # mm4 = mask\n\t" 
        "psllw $7, %%mm5        # mm5 = ROUND = 128\n" 
        "punpcklbw %%mm6, %%mm4 # mm4 = 0,0,0,FF\n\t" 
        : /* inputs */
        : "r" (mask.u32));

    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;

        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            unsigned int alpha;
                
            /*
             * Small wins:  
             *
             * We can compute 
             *      dest = (fg * alpha) + (beta * bg);
             * for all RGBA components at once. 
             *
             * C = (Ca * ia) + bias >> 16 
             * Packing unsigned with saturation performs the necessary
             * clamping without the branch misprediction penalty.
             */
            alpha = sp->Alpha;
            asm volatile (
                /* 
                 * mm0 = 
                 * mm1 = sp
                 * mm2 = alpha
                 * mm3 = temp
                 * mm4 = 0,0,0,FF
                 * mm5 = ROUND = 128,128,128,128
                 * mm6 = 0
                 * mm7 = 
                 */
                "movd %1, %%mm2           #  mm2 = 0,0,0,IA\n\t" 
                "movd (%0), %%mm1         #  mm1 = sp\n\t" 
                "punpcklwd %%mm2, %%mm2   #  mm2 = 0,0,IA,IA\n\t"
                "punpcklbw %%mm6, %%mm1   #  mm1 = Sa,Sr,Sg,Sb\n\t" 
                "punpcklwd %%mm2, %%mm2   #  mm2 = IA,IA,IA,IA\n\t"
                "por %%mm4, %%mm2         #  mm2 = 0xff,IA,IA,IA\n\t"
                "pmullw %%mm1, %%mm2      #  mm2 = S*IA\n\t" 
                "paddw %%mm5, %%mm2       #  mm2 = (S*A)+ROUND\n\t"
                "movq %%mm2, %%mm3        #  mm3 = P16\n\t" 
                "psrlw $8, %%mm3          #  mm3 = P16 / 256\n\t"
                "paddw %%mm2, %%mm3       #  mm3 = (P16 / 256) + P16\n\t" 
                "psrlw $8, %%mm3          #  mm3 = P8 ~= P16 / 257\n\t"
                "packuswb %%mm3, %%mm3    #  Pack 4 low bytes.\n\t" 
                "movd %%mm3, (%0)         #  *sp = word\n" 
                : "+r" (sp) 
                : "r" (alpha));
        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    asm volatile ("emms");
    srcPtr->flags &= ~BLT_PIC_PREMULT_COLORS;
}
#endif

/* 
 *---------------------------------------------------------------------------
 *
 * CopyPictures --
 *
 *      Creates a copy of the given picture using SSE xmm registers.
 *      Pictures are guaranteed to be quadword aligned (16 bytes).  The
 *      stride (pixelsPerRow) is always a multiple of 4 pixels.  This lets
 *      us copy 4 pixels at a time using the movdqa instruction.
 * 
 * Results: 
 *      None.
 *
 * Side effects: 
 *      The source picture is copied to the destination picture.
 *
 * -------------------------------------------------------------------------- 
 */
static void
CopyPictures(Pict *destPtr, Pict *srcPtr)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    assert((srcPtr->width == destPtr->width) &&
           (srcPtr->height == destPtr->height));
    srcRowPtr  = srcPtr->bits;
    destRowPtr = destPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send, *dp;

        dp = destRowPtr;
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp += 4) {
            asm volatile (
                "movdqa (%1), %%xmm1\n\t"
                "movdqa %%xmm1, (%0)\n\t" 
                : /* outputs */
                  "+r" (dp) 
                : /* inputs */
                  "r" (sp));
            dp += 4;
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
    destPtr->flags = (srcPtr->flags | BLT_PIC_DIRTY);
    asm volatile ("emms");
}

/* 
 *---------------------------------------------------------------------------
 *
 * PremultiplyColors --
 *
 *      Multiplies the alpha value with each color component using SSSE3
 *      operations.  It is assumed that the already does not have already
 *      have premultiplied colors.  
 *
 *              Dr = Sr * Sa
 *              Dg = Sg * Sg
 *              Db = Sb * Sb
 *              Da = Sa
 *
 *      Pictures are also guaranteed to be quadword aligned (16 bytes).
 *      The stride (pixelsPerRow) is always a multiple of 4 pixels.  This
 *      lets us blend 4 associate 4 pixels at a time.  We also use Jim
 *      Blinn's method of multiplying two 8-bit numbers togther.
 *
 *      Reference: Jim Blinn's Corner: Dirty Pixels.
 *
 * Results: 
 *      None.
 *
 * -------------------------------------------------------------------------- 
 */
static uint8_t preMultMap[16] __attribute__((aligned(16))) = {
    /* B G R A B G R A */
    /* The first 3 words, get the first alpha. The fourth is zero. */
    0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0xff,
    /* The second 3 words, get the second alpha. The fourth is zero. */
    0xff, 0x07, 0xff, 0x07, 0xff, 0x07, 0xff, 0xff,
};

static void
PremultiplyColors(Pict *srcPtr)
{
    int y;
    Blt_Pixel *srcRowPtr;
    
    if (srcPtr->flags & BLT_PIC_PREMULT_COLORS) {
        return;
    }
    srcPtr->flags |= BLT_PIC_PREMULT_COLORS;
    asm volatile (
        "pcmpeqw   %%xmm0, %%xmm0       # xmm0 = ffff x 8\n\t"
        "pxor      %%xmm6, %%xmm6       # xmm6 = 0\n\t"
        "pxor      %%xmm0, %%xmm0       # xmm0 = 0\n\t"
        "mov       $0xFF00,%%edx        # edx =  ff00 \n\t"
        /* Fix the 4th and 8th 16-bit alpha values to ff00 (1)  */
        "pinsrw    $3,%%edx,%%xmm0      # xmm0 = ____ ____ ____ ff__ \n\t"
        "pinsrw    $7,%%edx,%%xmm0      # xmm0 = ____ ____ ____ ff__ \n\t"
        "movdqa    (%0), %%xmm7         # xmm7 = shuffle map\n\t"
        "pcmpeqw   %%xmm1, %%xmm1       # xmm1 = ffff x 8\n\t"
        "punpckhbw %%xmm6, %%xmm1	# xmm1 = 00ff x 8\n\t"
        "psrlw     $7, %%xmm1           # xmm1 = 0001 x 8\n\t"
        "psllw     $7, %%xmm1           # xmm1 = 0010 x 8\n\t"
        : /* outputs */
        : /* inputs */
          "r" (preMultMap)
        : /* clobbers */
          "edx");
    /*
     * xmm0 = mask      ____ ____ ____ _1__ x 4   alphas | mask 
     * xmm1 = bias      __8_ __8_ __8_ ____ x 4   3 bias + 0000
     * xmm6 = 0         ____ ____ ____ ____ x 4
     * xmm7 = shuffle   
     */
    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;

        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp += 4) {
            asm volatile (
		"movdqa    (%0), %%xmm4         # xmm4 = P x 4\n\t" 

		"movdqa    %%xmm4, %%xmm2       # xmm2 = xmm4\n\t"
                /* Move the upper quadword to lower so that we can use the
                 * same shuffle mask for both pairs of pixels. */
		"movhlps   %%xmm4, %%xmm3       # xmm3 = P2 P3\n\t"
                /* Unpack background. */
		"punpcklbw %%xmm6, %%xmm2	# xmm2 = _b_g_r_a x 2\n\t"
		"movdqa    %%xmm3, %%xmm5       # xmm5 = xmm3\n\t"
		"punpcklbw %%xmm6, %%xmm3	# xmm3 = _b_g_r_a x 2\n\t"
                /* Shift color components to upper byte */
                "psllw     $8, %%xmm2           # xmm2 = b_g_r_a_\n\t"
                "psllw     $8, %%xmm3           # xmm3 = b_g_r_a_\n\t"
                /* Shuffle bytes in pixels to get alphas in high bytes. */
		"pshufb	   %%xmm7, %%xmm4       # xmm4 = a_a_a___ x 2\n\t"
		"pshufb	   %%xmm7, %%xmm5       # xmm5 = a_a_a___ x 2\n\t"
                /* Or in 0x100 for the alpha multiplier. */
		"por	   %%xmm0, %%xmm4       # xmm5 = a_a_a_1_ x 2\n\t"
		"por	   %%xmm0, %%xmm5       # xmm5 = a_a_a_1_ x 2\n\t"
                /* Multiple each color component by its matching alpha. */
		"pmulhuw   %%xmm2, %%xmm4	# xmm4 = a*b,a*g,a*r,a*ff\n\t"
		"pmulhuw   %%xmm3, %%xmm5	# xmm5 = a*b,a*g,a*r,a*ff\n\t"
                 /* Add the bias. */
                "paddsw    %%xmm1, %%xmm4       # xmm4 += bias\n\t"
                "paddsw    %%xmm1, %%xmm5       # xmm5 += bias\n\t"
                /* Save copy of original (bg * beta) + bias. */
                "movdqa    %%xmm4, %%xmm2       # xmm2 = xmm4\n"
                "movdqa    %%xmm5, %%xmm3       # xmm3 = xmm5\n"
                /* Approximate dividing by 257. (((x >> 8) + x) >> 8) */
                "psrlw     $8, %%xmm4           # xmm4 /= 256\n\t"
                "psrlw     $8, %%xmm5           # xmm5 /= 256\n\t"
                /* Add original */
                "paddw     %%xmm2, %%xmm4       # xmm4 += xmm2\n\t"
                "paddw     %%xmm3, %%xmm5       # xmm5 += xmm3\n\t"
                /* Divide again by 256 */
                "psrlw     $8, %%xmm4           # P / 257\n\t"
                "psrlw     $8, %%xmm5           # P / 257\n\t"
		/* Convert words to bytes  */
		"packuswb  %%xmm5, %%xmm4       # xmm4 = P0 P1 P2 P3\n\t"
		"movdqa    %%xmm4, (%0)     	# Save 4 pixels.\n\t"
		: /* outputs */
                  "+r" (sp));
        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
}

/* 
 *---------------------------------------------------------------------------
 *
 * CompositePictures --
 *
 *      Composites two pictures (over operation) using SSSE3 operations.
 *      It is assumed that the background and foreground pictures already
 *      have premultiplied alphas.  Blending is therefore implemented as
 *
 *              pixel = F + (B * (1 - Fa))
 *              
 *      Pictures are also guaranteed to be quadword aligned (16 bytes).
 *      The stride (pixelsPerRow) is always a multiple of 4 pixels.  This
 *      lets us blend 4 pixels at a time.  We also use Jim Blinn's method
 *      of multiplying two 8-bit numbers togther.
 *
 *      Reference: Jim Blinn's Corner: Dirty Pixels.
 *
 * Results: 
 *      None.
 *
 * -------------------------------------------------------------------------- 
 */
static uint8_t compositeMap[16] __attribute__((aligned(16))) = {
    /* B G R A B G R A */
    /* The first 4 words, get the first alpha. */
    0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 
    /* and the second 4 words, get the second alpha. */
    0xff, 0x07, 0xff, 0x07, 0xff, 0x07, 0xff, 0x07,
};

static void
CompositePictures(Pict *destPtr, Pict *srcPtr)
{
    int y;
    Blt_Pixel *destRowPtr, *srcRowPtr;

    assert((srcPtr->width == destPtr->width) &&
           (srcPtr->height == destPtr->height));
    if ((srcPtr->flags & BLT_PIC_PREMULT_COLORS) == 0) {
        Blt_PremultiplyColors(srcPtr);
    }
    if ((destPtr->flags & BLT_PIC_PREMULT_COLORS) == 0) {
        Blt_PremultiplyColors(destPtr);
    }
    asm volatile (
        "pxor      %%xmm6, %%xmm6       # xmm6 = 0\n\t"
        "movdqa    (%0), %%xmm7         # xmm7 = shuffle map\n\t"
        : /* outputs */
        : /* imputs */
          "r" (compositeMap));

    destRowPtr = destPtr->bits, srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *dp, *sp, *send;

        dp = destRowPtr;
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp += 4) {
            asm volatile (
		"movdqa    (%1), %%xmm0         # xmm0 = FG0FG1FG2FG3\n\t" 
		"movdqa    (%0), %%xmm1         # xmm1 = BG0BG1BG2BG3\n\t"
		"movdqa    %%xmm0, %%xmm2       # xmm2 = m0 (fg)\n\t"
                /* Move the upper quadword to lower so that we can use the
                 * same shuffle mask for both pairs of pixels. */
		"movhlps   %%xmm0, %%xmm3       # xmm3 = (F1)\n\t"
                /* Unpack background */
		"movdqa    %%xmm1, %%xmm4       # xmm4 = xmm1 (bg)\n\t"
		"movhlps   %%xmm1, %%xmm5       # xmm5 = xmm1 << 64\n\t"
		"punpcklbw %%xmm6, %%xmm4	# xmm4 = _Bb_Bg_Br_Ba x 2\n\t"
		"punpcklbw %%xmm6, %%xmm5	# xmm5 = _Bb_Bg_Br_Ba x 2\n\t"
                /* Shift color components to upper byte */
                "psllw     $8, %%xmm4           # xmm4 = Bb_Bg_Br_Ba_\n\t"
                "psllw     $8, %%xmm5           # xmm5 = Bb_Bg_Br_Ba_\n\t"
                /* Create XOR mask for beta values. */
                "pcmpeqw   %%xmm1, %%xmm1       # xmm1 = ffff x 8\n\t"
                "psllw     $8, %%xmm1           # xmm1 = ff00 x 8\n\t"
                /* Shuffle bytes in foreground pixels to get alphas. */
		"pshufb	   %%xmm7, %%xmm2       # xmm2 = Fa_Fa_Fa_Fa_ x 2\n\t"
		"pshufb	   %%xmm7, %%xmm3       # xmm3 = Fa_Fa_Fa_Fa_ x 2\n\t"
                /* beta = alpha ^ 0xFF. */
                "pxor      %%xmm1, %%xmm2       # xmm2 = B_B_B_B_ x 2\n\t"
                "pxor      %%xmm1, %%xmm3       # xmm3 = B_B_B_B_ x 2\n\t"
                /* Multiple each background component by beta. */
		"pmulhuw   %%xmm2, %%xmm4	# xmm4 = B*BG0,B*BG1\n\t"
                /* Create rounding bias 128. */
                "pcmpeqw   %%xmm1, %%xmm1       # xmm1 = ffff x 8\n\t"
		"punpckhbw %%xmm6, %%xmm1	# xmm1 = 00ff x 8\n\t"
                "psrlw     $7, %%xmm1           # xmm1 = 0001 x 8\n\t"
                "psllw     $7, %%xmm1           # xmm1 = 0010 x 8\n\t"
                /* Multiple each background component by beta. */
                /* Something weird here.  If multiple instructions are
                 * consecutive, the result is corrupted. */
		"pmulhuw   %%xmm3, %%xmm5	# xmm5 = B*BG2,B*BG3\n\t"
                 /* Add the bias. */
                "paddsw    %%xmm1, %%xmm4       # xmm4 += bias\n\t"
                "paddsw    %%xmm1, %%xmm5       # xmm5 += bias\n\t"
                /* Save copy of original (bg * beta) + bias. */
                "movdqa    %%xmm4, %%xmm2       # xmm2 = xmm4\n"
                "movdqa    %%xmm5, %%xmm3       # xmm3 = xmm5\n"
                /* Approximate dividing by 257. (((x >> 8) + x) >> 8) */
                "psrlw     $8, %%xmm2           # xmm2 /= 256\n\t"
                "psrlw     $8, %%xmm3           # xmm3 /= 256\n\t"
                /* Add original */
                "paddw     %%xmm2, %%xmm4       # xmm4 += xmm4\n\t"
                "paddw     %%xmm3, %%xmm5       # xmm5 += xmm5\n\t"
                /* Divide again by 256 */
                "psrlw     $8, %%xmm4           # BG / 257\n\t"
                "psrlw     $8, %%xmm5           # BG / 257\n\t"
		/* Convert words to bytes  */
		"packuswb  %%xmm5, %%xmm4       # xmm5 = BG0 BG1 BG2 BG3\n\t"
		/* Add to background to foreground */
		"paddusb   %%xmm4, %%xmm0	# xmm0 = BG + FG\n\t"
		"movdqa    %%xmm0, (%0)     	# Save 4 pixels.\n\t"
		: /* outputs */
                  "+r" (dp)
		: /* imputs */
                  "r" (sp));
            dp += 4;
        }
        destRowPtr += destPtr->pixelsPerRow;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
}

/* 
 *---------------------------------------------------------------------------
 *
 * CrossFadePictures --
 *
 *      Cross-fades two pictures (blending percentages of both from and to
 *      picture) using SSSE3 operations.  It is assumed that the from and
 *      to pictures already have premultiplied alphas.  Blending is
 *      therefore implemented as
 *
 *              pixel = (T * a) + (F * (1 - a))
 *              
 *      where F = "from" pixel components, T = "to" pixel components, and 
 *      a = alpha value.
 *
 *      Pictures are also guaranteed to be quadword aligned (16 bytes).
 *      The stride (pixelsPerRow) is always a multiple of 4 pixels.  This
 *      lets us crossfade 4 pixels at a time.  We also use Jim Blinn's
 *      method of multiplying two 8-bit numbers togther.
 *
 *      Reference: Jim Blinn's Corner: Dirty Pixels.
 *
 * Results: 
 *      None.
 *
 * -------------------------------------------------------------------------- 
 */

/* Map to convert lower quadwords (pixels) into B_G_R_A_B_G_R_A */
static uint8_t crossFadeMap[16] __attribute__((aligned(16))) = {
    /* B G R A B G R A */
    0xff, 0x00, 0xff, 0x01, 0xff, 0x02, 0xff, 0x03, 
    0xff, 0x04, 0xff, 0x05, 0xff, 0x06, 0xff, 0x07,
};

static uint8_t bytes16[16] __attribute__((aligned(16))) = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void
CrossFadePictures(Pict *destPtr, Pict *fromPtr, Pict *toPtr, double opacity)
{
    int alpha;
    Blt_Pixel *fromRowPtr, *toRowPtr, *destRowPtr;
    int y, i;

    assert((fromPtr->width == toPtr->width) &&
           (fromPtr->height == toPtr->height));
    if ((fromPtr->flags & BLT_PIC_PREMULT_COLORS) == 0) {
        Blt_PremultiplyColors(fromPtr);
    }
    if ((toPtr->flags & BLT_PIC_PREMULT_COLORS) == 0) {
        Blt_PremultiplyColors(toPtr);
    }
    alpha = (int)(opacity * 255);

    for (i = 0; i < 16; i += 2) {
        bytes16[i]   = 0;
        bytes16[i+1] = alpha;
    }
    /*
     * 7=shuffle map
     * 6=alpha/beta
     * 5=work
     * 4=work
     * 3=work
     * 2=from
     * 1=bias
     * 0=ff00 xor mask 
     */
    asm volatile (
        "movdqa    (%0), %%xmm7         # xmm7 = shuffle map\n\t"
        "movdqa    (%1), %%xmm6         # xmm6 = A_ x  8\n\t"
        /* Create rounding bias 128. */
        "pxor      %%xmm0, %%xmm0       # xmm0 = 0\n\t"
        "pcmpeqw   %%xmm1, %%xmm1       # xmm1 = ffff x 8\n\t"
        "punpckhbw %%xmm0, %%xmm1	# xmm1 = 00ff x 8\n\t"
        "psrlw     $7, %%xmm1           # xmm1 = 0001 x 8\n\t"
        "psllw     $7, %%xmm1           # xmm1 = 0010 x 8\n\t"
        : /* outputs */
        : /* imputs */
          "r" (crossFadeMap),
          "r" (bytes16));

    fromRowPtr = fromPtr->bits;
    toRowPtr = toPtr->bits;
    destRowPtr = destPtr->bits;
    for (y = 0; y < fromPtr->height; y++) {
        Blt_Pixel *dp, *fp, *tp, *fend;

        dp = destRowPtr;
        tp = toRowPtr;
        for (fp = fromRowPtr, fend = fp + fromPtr->width; fp < fend; fp += 4) {
            asm volatile (
		"movdqa    (%1), %%xmm2         # xmm2 = to T0T1T2T3\n\t"
                /* Create 0xff mask for xor-ing. */
                "pxor      %%xmm5, %%xmm5       # xmm5 = 0\n\t"
                "pcmpeqw   %%xmm0, %%xmm0       # xmm0 = ffff x 8\n\t"
		"punpckhbw %%xmm5, %%xmm0	# xmm0 = 00ff x 8\n\t"
                /* Move the upper quadword to lower so that we can use the
                 * same shuffle mask for both pairs of pixels. */
		"movhlps   %%xmm2, %%xmm3       # xmm3 = xmm2 << 64\n\t"
                /* Unpack background */
		"pshufb	   %%xmm7, %%xmm2       # xmm2 = Tb_Tg_Tr_Ta_ x 2\n\t"
		"pshufb	   %%xmm7, %%xmm3       # xmm3 = Tb_Tg_Tr_Ta_ x 2\n\t"
                /* Multiple each "to" color component by alpha. */
		"pmulhuw   %%xmm6, %%xmm2	# xmm2 = alpha * to\n\t"
                "psllw     $8, %%xmm0           # xmm0 = ff00 x 8\n\t"
		"pmulhuw   %%xmm6, %%xmm3	# xmm3 = alpha * to\n\t"
                 /* Add the bias. */
                "paddsw    %%xmm1, %%xmm2       # xmm2 += bias\n\t"
                "paddsw    %%xmm1, %%xmm3       # xmm3 += bias\n\t"
                /* Save copy of original (to * alpha) + bias. */
                "movdqa    %%xmm2, %%xmm4       # xmm4 = xmm2\n"
                "movdqa    %%xmm3, %%xmm5       # xmm5 = xmm3\n"
                /* Approximate dividing by 257. (((x >> 8) + x) >> 8) */
                "psrlw     $8, %%xmm2           # xmm2 /= 256\n\t"
                "psrlw     $8, %%xmm3           # xmm3 /= 256\n\t"
                /* Add original */
                "paddw     %%xmm4, %%xmm2       # xmm2 += xmm4\n\t"
                "paddw     %%xmm5, %%xmm3       # xmm3 += xmm5\n\t"
                /* Divide again by 256 */
                "psrlw     $8, %%xmm2           # to / 257\n\t"
                "psrlw     $8, %%xmm3           # to / 257\n\t"
		/* Convert words to bytes  */
		"packuswb  %%xmm3, %%xmm2       # xmm2 = T0T1T2T3\n\t"

		"movdqa    (%2), %%xmm3         # xmm3 = from F0F1F2F3\n\t"
                /* Change alpha to beta by xor-ing by 0xFF. */
                "pxor      %%xmm0, %%xmm6       # xmm6 = b_b_b_b_ x 2\n\t"

                /* Move the upper quadword to lower so that we can use the
                 * same shuffle mask for both pairs of pixels. */
		"movhlps   %%xmm3, %%xmm4       # xmm4 = xmm3 << 64\n\t"
                /* Unpack background */
		"pshufb	   %%xmm7, %%xmm3       # xmm3 = Fb_Fg_Fr_Fa_ x 2\n\t"
		"pshufb	   %%xmm7, %%xmm4       # xmm4 = Fb_Fg_Fr_Fa_ x 2\n\t"
                /* Multiple each "from" color component by beta. */
		"pmulhuw   %%xmm6, %%xmm3	# xmm3 = from * beta\n\t"
                "movdqa    %%xmm7, %%xmm5       # Dummy move\n\t"
		"pmulhuw   %%xmm6, %%xmm4	# xmm4 = from * beta\n\t"
                /* Change beta back to alpha. */
                "pxor      %%xmm0, %%xmm6       # xmm6 = a_a_a_a_ x 2\n\t"
                 /* Add the bias. */
                "paddsw    %%xmm1, %%xmm3       # xmm3 += bias\n\t"
                "paddsw    %%xmm1, %%xmm4       # xmm4 += bias\n\t"
                /* Save copy of original (from * beta) + bias. */
                "movdqa    %%xmm3, %%xmm0       # xmm0 = xmm3\n"
                "movdqa    %%xmm4, %%xmm5       # xmm5 = xmm4\n"
                /* Approximate dividing by 257. (((x >> 8) + x) >> 8) */
                "psrlw     $8, %%xmm3           # xmm3 /= 256\n\t"
                "psrlw     $8, %%xmm4           # xmm4 /= 256\n\t"
                /* Add original */
                "paddw     %%xmm0, %%xmm3       # xmm3 += xmm0\n\t"
                "paddw     %%xmm5, %%xmm4       # xmm4 += xmm5\n\t"
                /* Divide again by 256 */
                "psrlw     $8, %%xmm3           # from / 257\n\t"
                "psrlw     $8, %%xmm4           # from / 257\n\t"
		/* Convert words to bytes  */
		"packuswb  %%xmm4, %%xmm3       # xmm0 = F0 F1 F2 F3\n\t"
		/* Add to background to foreground */
		"paddusb   %%xmm2, %%xmm3	# xmm3 = from + to\n\t"
		"movdqa    %%xmm3, (%0)     	# Save 4 pixels.\n\t"
		: /* outputs */
                  "+r" (dp)
		: /* imputs */
                  "r" (tp),
                  "r" (fp));
            dp += 4;
            tp += 4;
        }
        destRowPtr += destPtr->pixelsPerRow;
        fromRowPtr += fromPtr->pixelsPerRow;
        toRowPtr += toPtr->pixelsPerRow;
    }
}

/* 
 *---------------------------------------------------------------------------
 *
 * BlankPicture --
 *
 *      Sets the picture to the specified color value.  The color is 
 *      unassociated.  
 *      
 * Results: 
 *      None.
 *
 * Side effects: 
 *      The source picture is set to the specified color.
 *
 * -------------------------------------------------------------------------- 
 */
static void
BlankPicture(Pict *destPtr, unsigned int colorValue)
{
    Blt_Pixel *destRowPtr;
    int y;
    Blt_Pixel pixel;
    int i;
    
    pixel.u32 = colorValue;
    Blt_PremultiplyColor(&pixel);
    for (i = 0; i < 16; i += 4) {
        bytes16[i]   = pixel.Blue;
        bytes16[i+1] = pixel.Green;
        bytes16[i+2] = pixel.Red;
        bytes16[i+3] = pixel.Alpha;
    }
    asm volatile (
        "movdqa    (%0), %%xmm6         # xmm6 = color\n\t"
        : /* outputs */
        : /* inputs */
          "r" (bytes16));

    destRowPtr = destPtr->bits;
    for (y = 0; y < destPtr->height; y++) {
        Blt_Pixel *dp, *dend;

        for (dp = destRowPtr, dend = dp + destPtr->width; dp < dend; dp += 4) {
            asm volatile (
                "movdqa %%xmm6, (%0)    # Save the 4 pixels. \n\t" 
                : /* outputs */
                  "+r" (dp));
        }
        destRowPtr += destPtr->pixelsPerRow;
    }
    destPtr->flags &= ~(BLT_PIC_COMPOSITE | BLT_PIC_MASK);
    destPtr->flags |= (BLT_PIC_DIRTY | BLT_PIC_PREMULT_COLORS);
    if (pixel.Alpha == 0x00) {
        destPtr->flags |= BLT_PIC_MASK | BLT_PIC_COMPOSITE;
    } else if (pixel.Alpha != 0xFF) {
        destPtr->flags |= BLT_PIC_COMPOSITE;
    }
}

static void
ZoomVertically2(Pict *destPtr, Pict *srcPtr, ResampleFilter *filterPtr)
{
    Sample *samples, *send;
    int x;
    int bytesPerSample;                 /* Size of sample. */

    /* Pre-calculate filter contributions for each row. */
    bytesPerSample = Blt_ComputeWeights(srcPtr->height, destPtr->height, 
        filterPtr, &samples);
    send = (Sample *)((char *)samples + (destPtr->height * bytesPerSample));

    /* 
     * xmm7 = unused
     * xmm6 = zero
     * xmm5 = accum  2 pixels (16 x 8)
     * xmm4 = unused
     * xmm3 = work 256 * pixel
     * xmm2 = bias
     * xmm1 = weights
     * xmm0 = pixel
     */
    /* 
     * xmm7 = accum  1 pixel (16 x 4)
     * xmm6 = work
     * xmm5 = work
     * xmm4 = bias
     * xmm3 = weights 3+4
     * xmm2 = weights 1+2
     * xmm1 = pixel 2+3
     * xmm0 = pixel 1+2
     */
    asm volatile (
        /* Generate constants needed below. */
        "pxor    %xmm6, %xmm6     # xmm6 = 0\n\t"
        "pcmpeqw %xmm2, %xmm2     # xmm2 = -1 x 8 \n\t"
        "psubw   %xmm6, %xmm2     # xmm2 = _1_1_1_1 x 4\n\t"
        "psllw   $4, %xmm2        # mm2 = bias\n");
    /* Apply filter to each row. */
    for (x = 0; x < srcPtr->width; x++) {
        Blt_Pixel *dp, *srcColPtr;
        Sample *splPtr;

        srcColPtr = srcPtr->bits + x;
        dp = destPtr->bits + x;
        for (splPtr = samples; splPtr < send; 
             splPtr = (Sample *)((char *)splPtr + bytesPerSample)) {
            Blt_Pixel *sp;
            PixelWeight *wp;

            asm volatile (
                /* Clear the accumulator mm5. */
                "pxor   %xmm5, %xmm5        #  xmm5 = 0\n\t");

            sp = srcColPtr + (splPtr->start * srcPtr->pixelsPerRow);
            for (wp = splPtr->weights; wp < splPtr->wend; wp++) {
                asm volatile (
                    /* Load the weighting factor into mm1. */
                    "movdqa    (%0), %%xmm1    # xmm1 = 0,0,0,W\n\t"
                    /* Load the source pixel into mm0. */
                    "movdqa    (%1), %%xmm0    # xmm0 = S\n\t" 
                    /* Unpack the weighting factor into mm1. */
                    "punpcklwd %%xmm1, %%xmm1  # xmm1 = 0,0,W,W\n\t"
                    /* Unpack the pixel components into 16-bit words.*/
                    "punpcklbw %%xmm6, %%xmm0  # xmm0 = Sa,Sb,Sg,Sr\n\t" 
                    /*  */
                    "punpcklwd %%xmm1, %%xmm1  # xmm1 = W,W,W,W\n\t"
                    /* Scale the 8-bit components to 15 bits. (S * 257) >> 1 */
                    "movdqa    %%xmm0, %%xmm3  # xmm3 = S8\n\t" 
                    "psllw     $8, %%xmm3      # xmm3 = S8 * 256\n\t" 
                    "paddw     %%xmm3, %%xmm0  # xmm0 = S16\n\t" 
                    "psrlw $1, %%xmm0          # xmm0 = S15\n\t" 
                    /* Multiple each pixel component by the weight.  Note that
                     * the lower 16-bits of the product are truncated (bad)
                     * creating round-off error in the sum. */
                    "pmulhw %%xmm1, %%xmm0     # xmm0 = S15 * W14\n\t" 
                    /* Accumulate upper 16-bit results of product in xmm5. */
                    "paddsw %%xmm0, %%xmm5     # xmm5 = prod + xmm5\n\t" 
                    : /* outputs */ 
                    : /* inputs */
                      "r" (wp), 
                      "r" (sp));
                sp += srcPtr->pixelsPerRow;
            }

            asm volatile (
                /* Add a rounding bias to the pixel sum */
                "paddw    %%xmm2, %%xmm5   # xmm5 = A13 + BIAS\n\t" 
                /* Shift off fractional part */
                "psraw    $5, %%xmm5       # xmm5 = A8\n\t" 
                /* Pack 16-bit components into lower 4 bytes. */
                "packuswb %%xmm5, %%xmm5   # Pack 4 low-order bytes.\n\t" 
                /* Save the word (pixel) in the destination. */
                "movdqa   %%xmm5,(%0)      # dp = word\n" 
                : /* outputs */ 
                  "+r" (dp));
            dp += destPtr->pixelsPerRow;

        }
    }
    /* Free the memory allocated for filter weights. */
    Blt_Free(samples);
}

static void
ZoomHorizontally2(Pict *destPtr, Pict *srcPtr, ResampleFilter *filterPtr)
{
    Sample *samples, *send;
    int y;
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int bytesPerSample;                 /* Size of sample. */

    /* Pre-calculate filter contributions for each column. */
    bytesPerSample = Blt_ComputeWeights(srcPtr->width, destPtr->width, 
        filterPtr, &samples);
    send = (Sample *)((char *)samples + (destPtr->width * bytesPerSample));

    /* Apply filter to each column. */
    srcRowPtr = srcPtr->bits;
    destRowPtr = destPtr->bits;

    asm volatile (
        "pxor    %xmm6, %xmm6   # xmm6 = 0\n\t"
        "pxor    %xmm3, %xmm3   # xmm3 = 0\n\t"
        "pcmpeqw %xmm2, %xmm2   # xmm2 = -1 x 8\n\t"
        "psubw   %xmm3, %xmm2   # xmm2 = _1_1_1_1 x 4\n\t"
        "psllw   $4, %xmm2      # xmm2 = bias\n");

    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *dp;
        Sample *splPtr;

        dp = destRowPtr;
        for (splPtr = samples; splPtr < send; 
             splPtr = (Sample *)((char *)splPtr + bytesPerSample)) {
            Blt_Pixel *sp;
            PixelWeight *wp;

            sp = srcRowPtr + splPtr->start;
            asm volatile (
                /* Clear the accumulator mm5. */
                "pxor    %xmm5, %xmm5        #  mm5 = 0\n\n");

            for (wp = splPtr->weights; wp < splPtr->wend; wp++) {
                asm volatile (
                   /* Load the weighting factor into mm1. */
                   "movdqa      (%0), %%xmm1    # xmm1 = W x 4\n\t" 
                   /* Get the source RGBA pixel. */
                   "movdqa      (%1), %%xmm0    # xmm0 = sp\n\t" 
                   /* Unpack the weighting factor into mm1. */
                   "punpcklwd    %%xmm1, %%xmm1 #  mm1 = 0,0,W,W\n\t"
                   /* Unpack the pixel into mm0. */
                   "punpcklbw   %%xmm6, %%xmm0  # xmm0 = _b_g_r_a x 4\n\t" 
                   /*  */
                   "punpcklwd   %%xmm1, %%xmm1  # xmm1 = _W_W_W_W x 4\n\t"
                   /* Scale the 8-bit color components to 15 bits: (S *
                    * 257) >> 1 */
                   "movdqa      %%xmm0, %%xmm3  # xmm3 = xmm0\n\t" 
                   "psllw       $8, %%xmm3      # xmm3 = b_g_r_a_ x 4\n\t" 
                   "paddw       %%xmm3, %%xmm0  # xmm0 = P * 257\n\t" 
                   "psrlw       $1, %%xmm0      # xmm0 = S15\n\t" 
                   /* Multiple each pixel component by the weight.  It's a
                    * signed mulitply because weights can be negative. Note
                    * that the lower 16-bits of the product are truncated
                    * (bad) creating round-off error in the sum. */
                   "pmulhw      %%xmm1, %%xmm0  # xmm0 = S15 * W14\n\t"  
                   /* Add the 16-bit components to mm5. */
                   "paddsw      %x%mm0, %%xmm5  # xmm5 = A13 + mm5\n\t" 
                   : /* outputs */ 
                   : /* inputs */
                     "r" (wp), 
                     "r" (sp));
                sp++;
            }                   

            asm volatile (
                /* Add a rounding bias to the pixel sum. */
                "paddw    %%xmm2, %%xmm5   # xmm5 = A13 + BIAS\n\t" 
                /* Shift off fractional portion. */
                "psraw    $5, %%xmm5       # xmm5 = A8\n\t" 
                /* Pack 16-bit components into lower 4 bytes. */
                "packuswb %%xmm5, %%xmm5   # Pack A8 into low 4 bytes.\n\t" 
                /* Store the word (pixel) in the destination. */
                "movdqa   %%xmm5,(%0)      # Save the 2 pixels.\n" 
                : /* outputs */ 
                  "+r" (dp));
            dp++;
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;
    }
    /* Free the memory allocated for horizontal filter weights. */
    Blt_Free(samples);
}


/* 
 *---------------------------------------------------------------------------
 *
 * HaveCpuIdInstruction --
 *
 *      Checks if X86 "cpuid" instruction is available. 
 *      
 * Results: 
 *      Return 1 if the cpuid instruction is available, 0 otherwise
 *
 * -------------------------------------------------------------------------- 
 */
static int
HaveCpuIdInstruction(void)
{
    unsigned int ecx;

    asm volatile (
        /* See if ID instruction is supported. Save a copy of EFLAGS in eax
         * and ecx */
#ifdef __amd64__
        "pushq %%rbx\n\t"
#else 
        "push %%ebx\n\t"
#endif
        "pushf\n\t"
#ifdef __amd64__
        "popq %%rax\n\t" 
#else 
        "pop %%eax\n\t" 
#endif
        "mov %%eax, %%ecx\n\t"
        /* Toggle the CPUID bit in one copy and store to the EFLAGS
         * reg */
        "xorl $0x200000, %%eax\n\t"
#ifdef __amd64__
        "pushq %%rax\n\t"
#else 
        "push %%eax\n\t"
#endif
        "popf\n\t"
        /* Get the (hopefully modified) EFLAGS */
        "pushf\n\t"
#ifdef __amd64__
        "popq %%rax\n\t"
#else 
        "pop %%eax\n\t"
#endif
        /* Compare the result with the previous. */
        "xor %%eax, %%ecx\n\t"
        "movl %%ecx, %0\n\t" 
#ifdef __amd64__
        "popq %%rbx\n\t"
#else 
        "pop %%ebx\n\t"
#endif
        : "=c" (ecx));
    if (ecx & 0x200000) {
        return TRUE;
    } 
    return FALSE;
}

/* 
 *---------------------------------------------------------------------------
 *
 * CallCpuId --
 *
 *      Calls the X86 "cpuid" instruction with the specified value.
 *      
 * Results: 
 *      Return 1 if the cpuid instruction is available and 0 otherwise.
 *      The register values are returned via *regPtr*.
 *
 * -------------------------------------------------------------------------- 
 */
static int
CallCpuId(int eax, X86Registers *regsPtr)
{
    if (!HaveCpuIdInstruction()) {
        return FALSE;
    }
    asm volatile (
#if defined(__i386__) 
        "push %%ebx\n\t"        
#endif
        "movl %0, %%eax\n\t"
        "cpuid\n\t"
#if defined(__i386__) 
        "pop %%ebx\n\t" 
#endif
        : "=a" (regsPtr->eax),
#if defined(__i386__) && defined(__PIC__)
          "=r" (regsPtr->ebx),
#else
          "=b" (regsPtr->ebx),
#endif
          "=c" (regsPtr->ecx),
          "=d" (regsPtr->edx)
        : "a" (eax)
        : "memory");
    return TRUE;
}

/* 
 *---------------------------------------------------------------------------
 *
 * GetCpuVerion --
 *
 *      Gets the X86 cpu version string.
 *      
 * Results: 
 *      Return 1 if the cpuid instruction is available and 0 otherwise.
 *      The cpu version string is returned via the character array 
 *      *version*.  It must store at least 13 characters.
 *
 * -------------------------------------------------------------------------- 
 */
static int
GetCpuVersion(char *version)
{
    X86Registers regs;

    if (!CallCpuId(0, &regs)) {
        return FALSE;
    }
    memcpy (version,   &regs.ebx, 4);
    memcpy (version+4, &regs.edx, 4);
    memcpy (version+8, &regs.ecx, 4);
    version[12] = '\0';
#ifdef notdef
    fprintf(stderr, "version=%s\n", version);
#endif
    return TRUE;
}

/* 
 *---------------------------------------------------------------------------
 *
 * GetCpuFlags --
 *
 *      Gets the X86 cpu flags that specify what instruction sets are
 *      available.
 *      
 * Results: 
 *      Return 0 if the cpuid instruction is not available. Otherwise.
 *      the cpu flags (either 32-bits or 64-bits) is returned.
 *
 * -------------------------------------------------------------------------- 
 */
static unsigned long
GetCpuFlags(void)
{
    X86Registers regs;
    unsigned long flags;

    if (!CallCpuId(1, &regs)) {
        return 0L;
    }
#if (SIZEOF_LONG == 8) 
    flags = (((unsigned long)regs.ecx) << 32) | regs.edx;    
#else 
    flags = regs.edx;
#endif
    return flags;
}

/* 
 *---------------------------------------------------------------------------
 *
 * SaveFeatures --
 *
 *     Sets a TCL variable "::blt::cpu_flags" that is a list representing
 *     the X86 cpu feature flags.
 *      
 * Results: 
 *      None.
 *
 * -------------------------------------------------------------------------- 
 */
static void
SaveFeatures(Tcl_Interp *interp, unsigned long flags)
{
    char version[13];
    Tcl_Obj *objPtr, *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    GetCpuVersion(version);
    objPtr = Tcl_NewStringObj(version, 12);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    Tcl_AppendElement(interp, version);
    if (flags & FEATURE_MMX) {
        objPtr = Tcl_NewStringObj("mmx", 3);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    if (flags & FEATURE_MMXEXT) {
        objPtr = Tcl_NewStringObj("mmxext", 6);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    if (flags & FEATURE_3DNOW) {
        objPtr = Tcl_NewStringObj("3dnow", 5);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    if (flags & FEATURE_SSE) {
        objPtr = Tcl_NewStringObj("sse", 3);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    if (flags & FEATURE_SSE2) {
        objPtr = Tcl_NewStringObj("sse2", 4);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
#if (SIZEOF_LONG == 8) 
    if (flags & FEATURE_SSSE3) {
        objPtr = Tcl_NewStringObj("ssse3", 5);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    if (flags & FEATURE_SSE41) {
        objPtr = Tcl_NewStringObj("sse4.1", 6);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    if (flags & FEATURE_SSE42) {
        objPtr = Tcl_NewStringObj("sse4.2", 6);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
#endif
    Tcl_SetVar2Ex(interp, "::blt::cpu_info", NULL, listObjPtr, 
                  TCL_GLOBAL_ONLY);
}

/* 
 *---------------------------------------------------------------------------
 *
 * Blt_CpuFeatures --
 *
 *      Gets the cpu features flags and overrides the standard picture
 *      routines with faster SIMD versions.  Also a TCL variable
 *      "::blt::cpu_flags" is set to contain the found cpu feature flags.
 *      
 * Results: 
 *      Always TCL_OK.
 *
 * -------------------------------------------------------------------------- 
 */
int
Blt_CpuFeatures(Tcl_Interp *interp, unsigned long *flagsPtr)
{
    unsigned long flags;

    flags = GetCpuFlags();
    if (flags & FEATURE_MMX) {
        bltPictProcsPtr->applyPictureToPictureProc = ApplyPictureToPicture;
        bltPictProcsPtr->applyScalarToPictureProc = ApplyScalarToPicture;
        bltPictProcsPtr->tentHorizontallyProc = TentHorizontally;
        bltPictProcsPtr->tentVerticallyProc = TentVertically;
        bltPictProcsPtr->zoomHorizontallyProc = ZoomHorizontally;
        bltPictProcsPtr->zoomVerticallyProc = ZoomVertically;
        bltPictProcsPtr->selectPixelsProc = SelectPixels;
#if (SIZEOF_LONG == 8) 
        if (flags & FEATURE_SSSE3) {
#ifdef notdef
#endif
            bltPictProcsPtr->premultiplyColorsProc = PremultiplyColors;
            bltPictProcsPtr->copyPicturesProc = CopyPictures;
            bltPictProcsPtr->compositePicturesProc = CompositePictures;
            bltPictProcsPtr->blankPictureProc = BlankPicture;
            bltPictProcsPtr->crossFadePicturesProc = CrossFadePictures;
        }
#endif  /* SIZEOF_LONG == 8 */
    }
    if (flagsPtr != NULL) {
        *flagsPtr = flags;
    }
    if (interp != NULL) {
        SaveFeatures(interp, flags);
    }
    return TCL_OK;
}

#else 

int
Blt_CpuFeatures(Tcl_Interp *interp, unsigned long *flagsPtr)
{
    if (flagsPtr != NULL) {
        *flagsPtr = 0L;
    }
    return TCL_OK;
}

#endif /* HAVE_X86_ASM */


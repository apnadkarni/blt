

/*
 * bltPictMMX.c --
 *
 * This module implements image processing procedures for the BLT toolkit.
 *
 *	Copyright 1997-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltChain.h"
#include "bltPicture.h"
#include "bltPictInt.h"

#ifdef HAVE_X86_ASM

static Blt_ApplyPictureToPictureProc ApplyPictureToPicture;
static Blt_ApplyScalarToPictureProc  ApplyScalarToPicture;
#ifdef notdef
static Blt_ApplyPictureToPictureWithMaskProc ApplyPictureToPictureWithMask;
static Blt_ApplyScalarToPictureWithMaskProc  ApplyScalarToPictureWithMask;
#endif
static Blt_TentHorizontallyProc TentHorizontally;
static Blt_TentVerticallyProc TentVertically;
static Blt_ZoomHorizontallyProc ZoomHorizontally;
static Blt_ZoomVerticallyProc ZoomVertically;
static Blt_BlendPicturesProc BlendPictures;
static Blt_SelectPixelsProc SelectPixels;
static Blt_AssociateColorsProc AssociateColors;
static Blt_UnassociateColorsProc UnassociateColors;
static Blt_CopyPictureBitsProc CopyPictureBits;

#ifdef notdef
static Blt_PictureProcs mmxPictureProcs = {
    ApplyPictureToPicture,
    ApplyScalarToPicture,
    NULL,				/* ApplyPictureToPictureWithMask, */
    NULL,				/* ApplyScalarToPictureWithMask, */
    TentHorizontally,
    TentVertically,
    ZoomHorizontally,
    ZoomVertically,
    BlendPictures,
    SelectPixels,
    AssociateColors,
    UnassociateColors,
    CopyPictureBits
};
#endif

static void
SelectPixels(
    Pict *destPtr,
    Pict *srcPtr, 
    Blt_Pixel *lowerPtr,
    Blt_Pixel *upperPtr)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    if (srcPtr != destPtr) {
	Blt_ResizePicture(destPtr, srcPtr->width, srcPtr->height);
    }

    asm volatile (
	/* Put lower and upper pixels in registers. */
	"movd %0, %%mm4	        # mm4 = L\n\t"
	"movd %1, %%mm5	        # mm5 = H\n\t"
	"pxor %%mm6, %%mm6	# mm6 = 0\n\t"
	"punpckldq %%mm4, %%mm4 # mm4 = L,L\n\t" 
	"punpckldq %%mm5, %%mm5 # mm5 = H,H\n\t" :
	/* output registers */ :
	/* input registers */
	"r" (lowerPtr->u32), "r" (upperPtr->u32));

    destRowPtr = destPtr->bits, srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
	Blt_Pixel *dp, *sp, *send;

	dp = destRowPtr;
	for(sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp += 2) {
	    asm volatile (
		/* Compare two pixels at a time */
		"movq (%1), %%mm3	# mm3 = S1,S2\n\t"
		"movq %%mm4, %%mm0	# mm0 = L,L\n\t"

		/* We want to test (S >= L) && (S <= H). Since the operands
		 * are all unsigned, pcmp* ops are out.  Instead use
		 * saturated, unsigned subtraction.  ((L psub S) == 0) is the
		 * same as (S >= L) */

		"psubusb %%mm3, %%mm0	# mm0 = L - S\n\t"
		"movq %%mm3, %%mm1	# mm1 = S\n\t"
		"psubusb %%mm5, %%mm1	# mm1 = S - H\n\t"

		/* "or" the two results and compare 32-bit values to 0
		 * (inverting the logic). */

		"por %%mm1, %%mm0	# mm0 = (S >= L)|(H >= S)\n\t"
 		"pcmpeqd %%mm6, %%mm0	# invert logic\n\t"
		"movq %%mm0, (%0)	# dp = new value\n" :
		/* output registers */
		"+r" (dp) :
		/* input registers */
		"r" (sp));
	    dp += 2;
	}
	srcRowPtr  += srcPtr->pixelsPerRow;
	destRowPtr += destPtr->pixelsPerRow;
    }
    asm volatile ("emms");
    destPtr->flags &= ~BLT_PIC_BLEND;
    destPtr->flags |= BLT_PIC_MASK;
}

static void
AddPictureToPicture(Pict *destPtr, Pict *srcPtr, int x, int y, int w, int h, 
		    int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    asm volatile (
        /* Generate constants needed below. */
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pcmpeqw %mm7, %mm7	# mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
	Blt_Pixel *sp, *dp, *dend;

	sp = srcRowPtr;
	for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
	    asm volatile (
	        "movq (%0), %%mm0\n\t" 
		"paddusb (%1), %%mm0\n\t" 
		"movq %%mm0, (%0)" : 
		/* output registers */
		"+r" (dp) : 
		/* input registers */
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
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pcmpeqw %mm7, %mm7	# mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
	Blt_Pixel *sp, *dp, *dend;

	sp = srcRowPtr;
	for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
	    asm volatile (
		"movq (%0), %%mm0\n\t" 
		"psubusb (%1), %%mm0\n\t" 
		"movq %%mm0, (%0)" : 
		/* output registers */
		"+r" (dp) : 
		/* input registers */
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
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pcmpeqw %mm7, %mm7	# mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
	Blt_Pixel *sp, *dp, *dend;

	sp = srcRowPtr;
	for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
	    asm volatile (
		"movq (%1), %%mm1\n\t" 
		"psubusb (%0), %%mm1\n\t" 
		"movq %%mm1, (%0)" : 
		/* output registers */
		"+r" (dp) : 
		/* input registers */
		"r" (sp));
	}
	destRowPtr += destPtr->pixelsPerRow;
	srcRowPtr += srcPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

static void
AndPictureToPicture(Pict *destPtr, Pict *srcPtr, int x, int y, 	int w, int h, 
		    int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;

    asm volatile (
        /* Generate constants needed below. */
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pcmpeqw %mm7, %mm7	# mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
	Blt_Pixel *sp, *dp, *dend;

	sp = srcRowPtr;
	for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
	    asm volatile (
		"movq (%0), %%mm0\n\t" 
		"pand (%1), %%mm0\n\t" 
		"movq %%mm0, (%0)" : 
		/* output registers */
		"+r" (dp) : 
		/* input registers */
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
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pcmpeqw %mm7, %mm7	# mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
	Blt_Pixel *sp, *dp, *dend;

	sp = srcRowPtr;
	for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
	    asm volatile (
		"movq (%0), %%mm0\n\t" 
		"por (%1), %%mm0\n\t" 
		"movq %%mm0, (%0)" : 
		/* output registers */
		"+r" (dp) : 
		/* input registers */
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
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pcmpeqw %mm7, %mm7	# mm5 = -1 \n");

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
		"movq %%mm0, (%0)" : 
		/* output registers */
		"+r" (dp) : 
		/* input registers */
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
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pcmpeqw %mm7, %mm7	# mm5 = -1 \n");

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
		/* output registers */
		: "+r" (dp) 
		/* input registers */
		: "r" (sp));
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
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pcmpeqw %mm7, %mm7	# mm5 = -1 \n");

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
		/* output registers */
		: "+r" (dp) 
		/* input registers */
		: "r" (sp));
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
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pcmpeqw %mm7, %mm7	# mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
	Blt_Pixel *sp, *dp, *dend;

	sp = srcRowPtr;
	for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
	    asm volatile (
		"movq (%0), %%mm0		# mm0 = A\n\t" 
		"movq (%1), %%mm1		# mm1 = B\n\t" 
		"movq %%mm0, %%mm2		# mm2 = A\n\t" 
		"psubusb %%mm1, %%mm2		# mm2 = A - B\n\t"
		"pcmpeqb %%mm6, %%mm2		# mm2 = 0s A>B 1s A<=B\n\t"
		"pand %%mm2, %%mm0		# mm2 = mask & A\n\t" 
		"pxor %%mm7, %%mm2		# mm2 = ~mask\n\t" 
		"pand %%mm2, %%mm1		# mm0 = ~mask & B\n\t" 
		"por %%mm1, %%mm0		# mm0 = R1 | R2\n\t" 
		"movq %%mm0, (%0)" 
		/* output registers */
		: "+r" (dp) 
		/* input registers */
		: "r" (sp));
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
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pcmpeqw %mm7, %mm7	# mm5 = -1 \n");

    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr  = srcPtr->bits + ((y * srcPtr->pixelsPerRow) + x);
    for (y = 0; y < h; y++) {
	Blt_Pixel *sp, *dp, *dend;

	sp = srcRowPtr;
	for (dp = destRowPtr, dend = dp + w; dp < dend; dp += 2, sp += 2) {
	    asm volatile (
		"movq (%0), %%mm0		# mm0 = A\n\t" 
		"movq (%1), %%mm1		# mm1 = B\n\t" 
		"movq %%mm0, %%mm2		# mm2 = A\n\t" 
		"psubusb %%mm1, %%mm2		# mm2 = A - B\n\t"
		"pcmpeqb %%mm6, %%mm2		# mm2 = 0s A>B 1s A<=B\n\t"
		"pand %%mm2, %%mm1		# mm1 = mask & B\n\t" 
		"pxor %%mm7, %%mm2		# mm2 = ~mask\n\t" 
		"pand %%mm2, %%mm0		# mm0 = ~mask & A\n\t" 
		"por %%mm1, %%mm0		# mm3 = R1 | R2\n\t" 
		"movq %%mm0, (%0)" 
		/* output registers */
		: "+r" (dp) 
		/* input registers */
		: "r" (sp));
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
	"pxor %%mm6, %%mm6	  # mm6 = 0\n\t"
	"pcmpeqw %%mm7, %%mm7	  # mm5 = -1 \n\t"
	/* Put the scalar into hi/lo 32-bit words.*/
	"movd %0, %%mm4		  # mm4 = scalar\n\t"
	"punpckldq %%mm4, %%mm4   # mm2 = S,S\n" 
	/* output registers */
	:
	/* input registers */
	: "r" (colorPtr->u32));

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
		    "movq %%mm0, (%0)" : 
		    /* output registers */
		    "+r" (sp));
		sp += 2;
	    }
	    break;

	case PIC_ARITH_SUB:
	    while (sp < send) {
		asm volatile (
		     "movq (%0), %%mm0\n\t" 
		     "psubusb %%mm4, %%mm0\n\t" 
		     "movq %%mm0, (%0)" : 
		     /* output registers */
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
		     "movq %%mm1, (%0)" : 
		     /* output registers */
		     "+r" (sp));
		sp += 2;
	    }
	    break;

	case PIC_ARITH_AND:
	    while (sp < send) {
		asm volatile (
		    "movq (%0), %%mm0\n\t" 
		    "pand %%mm4, %%mm0\n\t" 
		    "movq %%mm0, (%0)" : 
		    /* output registers */
		    "+r" (sp));
		sp += 2;
	    }
	    break;

	case PIC_ARITH_OR:
	    while (sp < send) {
		asm volatile (
		    "movq (%0), %%mm0\n\t" 
		    "por %%mm4, %%mm0\n\t" 
		    "movq %%mm0, (%0)" : 
		    /* output registers */
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
		    "movq %%mm0, (%0)" : 
		    /* output registers */
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
		    "movq %%mm0, (%0)" : 
		    /* output registers */
		    "+r" (sp));
		sp += 2;
	    }
	    break;

	case PIC_ARITH_XOR:
	    while (sp < send) {
		asm volatile (
		    "movq (%0), %%mm0\n\t" 
		    "pxor %%mm4, %%mm0\n\t" 
		    "movq %%mm0, (%0)" : 
		    /* output registers */
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
		    /* output registers */
		    : "+r" (sp));
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
		    /* output registers */
		    : "+r" (sp));
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
    int bytesPerSample;		/* Size of sample. */
    long bytesPerRow;

    /* Pre-calculate filter contributions for each row. */
    bytesPerSample = Blt_ComputeWeights(srcPtr->height, destPtr->height, 
	filterPtr, &samples);
    bytesPerRow = sizeof(Blt_Pixel) * srcPtr->pixelsPerRow;
    send = (Sample *)((char *)samples + (destPtr->height * bytesPerSample));

    asm volatile (
        /* Generate constants needed below. */
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pcmpeqw %mm2, %mm2	# mm2 = -1 \n\t"
	"psubw %mm6, %mm2	# mm2 = 1,1,1,1\n\t"
	"psllw $4, %mm2	        # mm2 = BIAS\n");

    /* Apply filter to each row. */
    for (x = 0; x < srcPtr->width; x++) {
	Blt_Pixel *dp, *srcColumnPtr;
	Sample *splPtr;

	srcColumnPtr = srcPtr->bits + x;
	dp = destPtr->bits + x;
	for (splPtr = samples; splPtr < send; 
	     splPtr = (Sample *)((char *)splPtr + bytesPerSample)) {
	    Blt_Pixel *sp;

	    sp = srcColumnPtr + (splPtr->start * srcPtr->pixelsPerRow);
#ifdef notdef
	    fprintf(stderr, "*dp(%x)=%x\n", dp, dp->u32);
	    fprintf(stderr, "*sp(%x)=%x\n", sp, sp->u32);
	    fprintf(stderr, "*splPtr(%x) weight=%x, wend=%x\n", 
		    splPtr, splPtr->weights, splPtr->wend);
#endif
	    asm volatile (
		/* Clear the accumulator mm5. */
                 "pxor %%mm5, %%mm5	    #  mm5 = 0\n\n" 
                 ".Lasm%=:\n\t" 
		 /* Load the weighting factor into mm1. */
		 "movd (%1), %%mm1	    #  mm1 = 0,0,0,W\n\t"
		 /* Load the source pixel into mm0. */
                 "movd (%3), %%mm0          #  mm0 = S\n\t" 
		 /* Unpack the weighting factor into mm1. */
		 "punpcklwd %%mm1, %%mm1    #  mm1 = 0,0,W,W\n\t"
		 /* Unpack the pixel components into 16-bit words.*/
                 "punpcklbw %%mm6, %%mm0    #  mm0 = Sa,Sb,Sg,Sr\n\t" 
		 /*  */
		 "punpcklwd %%mm1, %%mm1    #  mm1 = W,W,W,W\n\t"
		 /* Scale the 8-bit components to 14 bits. (S * 257) >> 2 */
                 "movq %%mm0, %%mm3         #  mm3 = S8\n\t" 
                 "psllw $8, %%mm3           #  mm3 = S8 * 256\n\t" 
                 "paddw %%mm3, %%mm0        #  mm0 = S16\n\t" 
                 "psrlw $1, %%mm0           #  mm0 = S15\n\t" 
		 /* Multiple each pixel component by the weight.  Note that
		  * the lower 16-bits of the product are truncated (bad)
		  * creating round-off error in the sum. */
                 "pmulhw %%mm1, %%mm0       #  mm0 = S15 * W14\n\t" 
                 "add $4, %1                #  wp++\n\t" 
                 "add %4, %3                #  sp++\n\t" 
                 /* Accumulate upper 16-bit results of product in mm5. */
                 "paddsw %%mm0, %%mm5       #  mm5 = prod + mm5\n\t" 
                 /* Move the pointers to the next weight and pixel */
                 "cmp %2, %1                #  wend == wp\n\t" 
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
  		 /* output registers */ 
		 : "+r" (dp) 
		 /* input registers */
		 : "r" (splPtr->weights), 
		   "r" (splPtr->wend), 
		   "r" (sp),
		   "r" (bytesPerRow));
#ifdef notdef
	    if (dp->Alpha != 0xFF) {
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
ZoomHorizontally(
    Pict *destPtr,
    Pict *srcPtr, 
    ResampleFilter *filterPtr)
{
	Sample *splPtr;
    Sample *samples, *send;
    int y;
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int bytesPerSample;			/* Size of sample. */

    /* Pre-calculate filter contributions for each column. */
    bytesPerSample = Blt_ComputeWeights(srcPtr->width, destPtr->width, 
	filterPtr, &samples);
    send = (Sample *)((char *)samples + (destPtr->width * bytesPerSample));

#ifdef notdef
    for (splPtr = samples; splPtr < send; 
	 splPtr = (Sample *)((char *)splPtr + bytesPerSample)) {
	Blt_Pixel *sp;

	fprintf(stderr, "splPtr=%x, start=%d weights=%x wend=%x\n",
		splPtr, splPtr->start, splPtr->weights, splPtr->wend);
	sp = srcRowPtr + splPtr->start;
    }

    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
	Blt_Pixel *sp;
	int x;
 
	sp = srcRowPtr;
	for (x = 0; x < srcPtr->width; x++) {
 	    fprintf(stderr, "sp[%d,%d]=%x\n", x, y, sp->u32);
	    sp++;
	}
	srcRowPtr += srcPtr->pixelsPerRow;
    }
    srcRowPtr = destPtr->bits;
    for (y = 0; y < destPtr->height; y++) {
	Blt_Pixel *dp;
	int x;

	dp = srcRowPtr;
	for (x = 0; x < destPtr->pixelsPerRow; x++) {
	    dp->u32 = 0xFFFFFFFF;
	    fprintf(stderr, "dp(%x)[%d,%d]=%x\n", dp, x, y, dp->u32);
	    dp++;
	}
	srcRowPtr += destPtr->pixelsPerRow;
    }

    fprintf(stderr, "destPtr w=%d h=%d bits=%x pixelsPerRows=%d\n", 
	    destPtr->width,
	    destPtr->height, destPtr->bits, destPtr->pixelsPerRow);
#endif

    /* Apply filter to each column. */
    srcRowPtr = srcPtr->bits;
    destRowPtr = destPtr->bits;

    asm volatile (
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pxor %mm3, %mm3	# mm3 = 0\n\t"
	"pcmpeqw %mm2, %mm2	# mm2 = -1\n\t"
	"psubw %mm3, %mm2	# mm2 = 1,1,1,1\n\t"
	"psllw $4, %mm2	        # mm2 = BIAS\n");

    for (y = 0; y < srcPtr->height; y++) {
	Blt_Pixel *dp;
	Sample *splPtr;

	dp = destRowPtr;
	for (splPtr = samples; splPtr < send; 
	     splPtr = (Sample *)((char *)splPtr + bytesPerSample)) {
	    Blt_Pixel *sp;

	    sp = srcRowPtr + splPtr->start;
#ifdef notdef
	    fprintf(stderr, "*dp(%x)=%x\n", dp, dp->u32);
	    fprintf(stderr, "*sp(%x)=%x\n", sp, sp->u32);
	    fprintf(stderr, "*splPtr(%x) weight=%x, wend=%x\n", 
		    splPtr, splPtr->weights, splPtr->wend);
#endif
	    asm volatile (
		/* Clear the accumulator mm5. */
                 "pxor %%mm5, %%mm5        #  mm5 = 0\n\n" 
                 ".Lasm%=:\n\t" 
		 /* Load the weighting factor into mm1. */
                 "movd (%1), %%mm1         #  mm1 = W\n\t" 
		 /* Get the source RGBA pixel. */
                 "movd (%3), %%mm0         #  mm0 = sp\n\t" 
		 /* Unpack the weighting factor into mm1. */
		 "punpcklwd %%mm1, %%mm1   #  mm1 = 0,0,W,W\n\t"
		 /* Unpack the pixel into mm0. */
                 "punpcklbw %%mm6, %%mm0   #  mm0 = Sa,Sr,Sg,Sb\n\t" 
		 /*  */
		 "punpcklwd %%mm1, %%mm1   #  mm1 = W,W,W,W\n\t"
		 /* Scale the 8-bit components to 14 bits: (S * 257) >> 2 */
                 "movq %%mm0, %%mm3        #  mm3 = S8\n\t" 
                 "psllw $8, %%mm3          #  mm3 = S8 * 256\n\t" 
                 "paddw %%mm3, %%mm0       #  mm0 = S16\n\t" 
                 "psrlw $1, %%mm0          #  mm0 = S15\n\t" 
		 /* Multiple each pixel component by the weight.  Note
		  * that the lower 16-bits of the product are
		  * truncated (bad) creating round-off error in the
		  * sum. */
                 "pmulhw %%mm1, %%mm0      #  mm0 = S15 * W14\n\t"  
                 "add $4, %1		   #  wp++\n\t" 
                 "add $4, %3               #  sp++\n\t" 
                 /* Add the 16-bit components to mm5. */
                 "paddsw %%mm0, %%mm5      #  mm5 = A13 + mm5\n\t" 
                 /* Move the pointers to the next weight and pixel */
                 "cmp %2, %1               #  wend == wp\n\t" 
                 "jnz .Lasm%=\n\t" 
                 /* end loop */
                 /* Add a rounding bias to the pixel sum. */
                 "paddw %%mm2, %%mm5       # mm5 = A13 + BIAS\n\t" 
                 /* Shift off fractional portion. */
                 "psraw $5, %%mm5          # mm5 = A8\n\t" 
		 /* Pack 16-bit components into lower 4 bytes. */
                 "packuswb %%mm5, %%mm5    # Pack A8 into low 4 bytes.\n\t" 
		 /* Store the word (pixel) in the destination. */
                 "movd %%mm5,(%0)	   # dp = word\n" 
  		 /* output registers */ 
		 : "+r" (dp) 
		 /* input registers */
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


static void
TentVertically(Pict *destPtr, Pict *srcPtr) 
{
    Blt_Pixel *srcColumnPtr, *destColumnPtr;
    int x;
    size_t numPixels;
    
    asm volatile (
	/* Establish constants used below. */
	"pxor %mm6, %mm6	# mm6 = 0\n");

    numPixels = srcPtr->height * srcPtr->pixelsPerRow; 
    srcColumnPtr = srcPtr->bits;
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
	rp = srcColumnPtr + srcPtr->pixelsPerRow;
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
	    "movd %%mm0,(%0)	      # dp = word\n\t"
	    "movq %%mm3, %%mm1	      # cp = rp\n" :
	    /* output registers */ 
	    "+r" (dp), "+r" (rp) :
	    /* input registers */
	    "r" (srcColumnPtr));
	dp += destPtr->pixelsPerRow;
	rp += srcPtr->pixelsPerRow;

	for (rend = srcColumnPtr + numPixels; rp < rend; /*empty*/) {
	    asm volatile (
	        "movd (%1), %%mm3         #  mm3 = rp\n\t" 
	        "punpcklbw %%mm6, %%mm3   #  mm3 = S8\n\t"
		"movq  %%mm1, %%mm0       #  mm0 = cp\n\t"
		"psllw $1, %%mm0          #  mm0 = cp << 1\n\t"
		"paddw %%mm2, %%mm0       #  mm0 = lp + (cp << 1)\n\t"
		"paddw %%mm3, %%mm0       #  mm0 = lp + (cp << 1) + rp\n\t"
		"psraw $2, %%mm0          #  mm0 = (lp + (cp<<1) + rp) >> 2\n\t"
		"packuswb %%mm0, %%mm0    #  Pack into low 4 bytes.\n\t"  
		"movd %%mm0,(%0)	  #  dp = word\n\t" 
  	        "movq %%mm1, %%mm2        #  lp = cp\n\t"
  	        "movq %%mm3, %%mm1        #  cp = rp\n"
		/* output registers */ 
		: "+r" (dp), 
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
	    "movd %%mm0,(%0)	      #  dp = word\n" 
	    /* output registers */ 
	    : "+r" (dp));

	srcColumnPtr++, destColumnPtr++;
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
	"pxor %mm6, %mm6	# mm6 = 0\n");

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
	    "movd %%mm0,(%0)	      #  dp = word\n\t"
	    "movq %%mm3, %%mm1	      #  cp = rp\n" :
	    /* output registers */ 
	    "+r" (dp), "+r" (rp) :
	    /* input registers */
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
		"movd %%mm0,(%0)	  #  dp = word\n\t" 
  	        "movq %%mm1, %%mm2        #  lp = cp\n\t"
  	        "movq %%mm3, %%mm1        #  cp = rp\n" 
		/* output registers */ 
		: "+r" (dp), 
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
	    "movd %%mm0,(%0)	      #  dp = word\n" 
	    /* output registers */ 
	    : "+r" (dp));

	srcRowPtr += srcPtr->pixelsPerRow;
	destRowPtr += destPtr->pixelsPerRow;
    }
    asm volatile ("emms");
}

static void
BlendPictures(
    Pict *destPtr,		/* (in/out) Background picture. Composite
				 * overwrites region in background. */
    Pict *srcPtr,		/* Foreground picture. */
    int sx, int sy,		/* Origin of foreground region in source. */
    int w, int h,		/* Dimension of area to be blended. */
    int dx, int dy)		/* Origin of background region in
				 * destination. */
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;

    if ((srcPtr->flags & BLT_PIC_ASSOCIATED_COLORS) == 0) {
	Blt_AssociateColors(srcPtr);
    }
    if ((destPtr->flags & BLT_PIC_ASSOCIATED_COLORS) == 0) {
	Blt_AssociateColors(destPtr);
    }
    destRowPtr = destPtr->bits + ((dy * destPtr->pixelsPerRow) + dx);
    srcRowPtr = srcPtr->bits + ((sy * srcPtr->pixelsPerRow) + sx);

    asm volatile (
        /* Generate constants needed below. */
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pcmpeqw %mm5, %mm5	# mm5 = -1 \n\t"
	"psubw %mm6, %mm5	# mm5 = 1,1,1,1\n\t"
	"psllw $7, %mm5	        # mm5 = ROUND = 128\n");

    for (y = 0; y < h; y++) {
	Blt_Pixel *sp, *send, *dp;

	dp = destRowPtr;
	for (sp = srcRowPtr, send = sp + w; sp < send; sp++, dp++) {
	    /* Blend the foreground and background together. */
	    if (sp->Alpha == 0xFF) {
		*dp = *sp;
	    } else if (sp->Alpha != 0x00) {
		unsigned long beta;
		
		beta = sp->Alpha ^ 0xFF; /* beta = 1 - alpha */

		/*
		 * Small wins:  
		 *
		 * We can compute 
		 *      dest = fg + (beta * bg);
		 * for all RGBA components at once. 
		 *
		 * Packing unsigned with saturation performs the
		 * necessary clamping without the branch misprediction
		 * penalty.
		 *
		 * FIXME: 
		 *     Check if it's faster to do the blend calcution
		 *     all the time (even when alpha is 0 or
		 *     255). There's a good probability that the
		 *     majority of pixels are opaque (interior) or
		 *     completely transparent (exterior).  Only the
		 *     edge pixels would require blending.
		 */
  	        asm volatile (
		    /* 
		     * mm0 = dp
		     * mm1 = sp
		     * mm2 = beta = 1 - alpha
		     * mm3 = temp
		     * mm4 = 
		     * mm5 = ROUND = 128,128,128,128
		     * mm6 = 0
		     * mm7 = 
		     */
		    "movd (%0), %%mm0         #  mm0 = dp\n\t" 
		    "movd (%1), %%mm1         #  mm1 = sp\n\t" 
 		    "movd %2, %%mm2           #  mm2 = beta\n\t" 
		    "punpcklbw %%mm6, %%mm0   #  mm0 = Da,Dr,Dg,Db\n\t" 
 		    "punpcklwd %%mm2, %%mm2   #  mm2 = 0,0,B,B\n\t"
		    "punpcklbw %%mm6, %%mm1   #  mm1 = Sa,Sr,Sg,Sb\n\t" 
		    "punpcklwd %%mm2, %%mm2   #  mm2 = B,B,B,B\n\t"
		    "pmullw %%mm0, %%mm2      #  mm2 = D*B\n\t" 
		    "paddw %%mm5, %%mm2       #  mm2 = (D*B)+ROUND\n\t"
		    "movq %%mm2, %%mm3	      #  mm3 = P16\n\t" 
		    "psrlw $8, %%mm3          #  mm3 = P16 / 256\n\t"
		    "paddw %%mm2, %%mm3       #  mm3 = (P16 / 256) + P16\n\t" 
		    "psrlw $8, %%mm3          #  mm3 = P8 ~= P16 / 257\n\t"
		    "paddw %%mm1, %%mm3       #  mm3 = S + P\n\t"
		    "packuswb %%mm3, %%mm3    #  Pack 4 low bytes.\n\t" 
		    "movd %%mm3, (%0)         #  *dp = word\n" 
		    : "+r" (dp) 
		    : "r" (sp), 
		      "r" (beta));
	    }
	}
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
    int bytesPerSample;		/* Size of sample. */
    long bytesPerRow;

    /* Pre-calculate filter contributions for each row. */
    bytesPerSample = Blt_ComputeWeights(srcPtr->height, destPtr->height, 
	filterPtr, &samples);
    bytesPerRow = sizeof(Blt_Pixel) * srcPtr->pixelsPerRow;
    send = (Sample *)((char *)samples + (destPtr->height * bytesPerSample));

    asm volatile (
        /* Generate constants needed below. */
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pcmpeqw %mm2, %mm2	# mm2 = -1 \n\t"
	"psubw %mm6, %mm2	# mm2 = 1,1,1,1\n\t"
	"psllw $4, %mm2	        # mm2 = BIAS\n");

    /* Apply filter to each row. */
    for (x = 0; x < srcPtr->width; x++) {
	Blt_Pixel *dp, *srcColumnPtr;
	Sample *splPtr;

	srcColumnPtr = srcPtr->bits + x;
	dp = destPtr->bits + x;
	for (splPtr = samples; splPtr < send; 
	     splPtr = (Sample *)((char *)splPtr + bytesPerSample)) {
	    Blt_Pixel *sp;

	    sp = srcColumnPtr + (splPtr->start * srcPtr->pixelsPerRow);
	    asm volatile (
		/* Clear the accumulator mm5. */
                 "pxor %%mm5, %%mm5	    # mm5 = 0\n\n" 
                 ".Lasm%=:\n\t" 
		 /* Load the weighting factor into mm1. */
		 "movd (%1), %%mm1	    # mm1 = 0,0,0,W\n\t"
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
  		 /* output registers */ 
		 : "+r" (dp) 
		 /* input registers */
		 : "r" (splPtr->weights), 
		   "r" (splPtr->wend), 
		   "r" (sp),
		   "r" (bytesPerRow));
#ifdef notdef
	    if (dp->Alpha != 0xFF) {
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
    int bytesPerSample;		/* Size of sample. */

    /* Pre-calculate filter contributions for each column. */
    bytesPerSample = Blt_ComputeWeights(srcPtr->width, destPtr->width, 
	filterPtr, &samples);
    send = (Sample *)((char *)samples + (destPtr->width * bytesPerSample));

    /* Apply filter to each column. */
    srcRowPtr = srcPtr->bits;
    destRowPtr = destPtr->bits;

    asm volatile (
	"pxor %mm6, %mm6	# mm6 = 0\n\t"
	"pxor %mm3, %mm3	# mm3 = 0\n\t"
	"pcmpeqw %mm2, %mm2	# mm2 = -1\n\t"
	"psubw %mm3, %mm2	# mm2 = 1,1,1,1\n\t"
	"psllw $4, %mm2	        # mm2 = BIAS\n");

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
                 "add $4, %1		   # wp++\n\t" 
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
                 "movd %%mm5,(%0)	   # dp = word\n" 
  		 /* output registers */ 
		 : "+r" (dp) 
		 /* input registers */
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

static void
AssociateColors(Pict *srcPtr) /* (in/out) picture */
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
	"pxor %%mm6, %%mm6	# mm6 = 0\n\t"
	"pcmpeqw %%mm5, %%mm5	# mm5 = -1 \n\t"
	"psubw %%mm6, %%mm5	# mm5 = 1,1,1,1\n\t"
	"movd %0, %%mm4         # mm4 = mask\n\t" 
	"psllw $7, %%mm5        # mm5 = ROUND = 128\n" 
	"punpcklbw %%mm6, %%mm4 # mm4 = 0,0,0,FF\n\t" 
	/* outputs */
	: 
	/* inputs */
	: "r" (mask.u32));

    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
	Blt_Pixel *sp, *send;

#ifdef notdef
	for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp += 2) {
	    Blt_Pixel *p, *q;
		
	    /*
	     * Small wins:  
	     *
	     * We can compute 
	     *      dest = (fg * alpha) + (beta * bg);
	     * for all RGBA components at once. 
	     *
	     * Packing unsigned with saturation performs the necessary
	     * clamping without the branch misprediction penalty.
	     */
	    p = sp, q = sp+1;
	    asm volatile (
		/* 
		 * mm0 = P
		 * mm1 = Q
		 * mm2 = Pa
		 * mm3 = Qa
		 * mm4 = temp
		 * mm5 = ROUND = 128,128,128,128
		 * mm6 = 0
		 * mm7 = 0,0,0,FF
		 */
		"movd (%0), %%mm0         #  mm0 = P\n\t" 
		"movd (%1), %%mm1         #  mm1 = Q\n\t" 
		"movd %2, %%mm2           #  mm2 = 0,0,0,Pa\n\t" 
		"movd %3, %%mm3           #  mm2 = 0,0,0,Qa\n\t" 
		"punpcklwd %%mm2, %%mm2   #  mm2 = 0,0,Pa,Pa\n\t"
		"punpcklwd %%mm3, %%mm3   #  mm3 = 0,0,Qa,Qa\n\t"
		"punpcklbw %%mm6, %%mm0   #  mm0 = Pa,Pr,Pg,Pb\n\t" 
		"punpcklwd %%mm2, %%mm2   #  mm2 = Pa,Pa,Pa,Pa\n\t"
		"punpcklbw %%mm6, %%mm1   #  mm1 = Qa,Qr,Qg,Qb\n\t" 
		"por %%mm7, %%mm2	  #  mm2 = 0xff,Pa,Pa,Pa\n\t"
		"punpcklwd %%mm3, %%mm3   #  mm3 = Qa,Qa,Qa,Qa\n\t"
		"por %%mm7, %%mm3	  #  mm2 = 0xff,Qa,Qa,Qa\n\t"
		"pmullw %%mm0, %%mm2      #  mm2 = P*Pa\n\t" 
		"pmullw %%mm1, %%mm3      #  mm3 = Q*Qa\n\t" 
		"paddw %%mm5, %%mm2       #  mm2 = (P*Pa)+ROUND\n\t"
		"paddw %%mm5, %%mm3       #  mm3 = (Q*Qa)+ROUND\n\t"
		"movq %%mm2, %%mm0	  #  mm0 = P16\n\t" 
		"movq %%mm3, %%mm1	  #  mm1 = Q16\n\t" 
		"psrlw $8, %%mm0          #  mm0 = P16 / 256\n\t"
		"psrlw $8, %%mm1          #  mm1 = Q16 / 256\n\t"
		"paddw %%mm2, %%mm0       #  mm0 = (P16 / 256) + P16\n\t" 
		"paddw %%mm3, %%mm1       #  mm1 = (Q16 / 256) + Q16\n\t" 
		"psrlw $8, %%mm0          #  mm0 = P8 ~= P16 / 257\n\t"
		"psrlw $8, %%mm1          #  mm1 = Q8 ~= Q16 / 257\n\t"
		"packuswb %%mm0, %%mm0    #  Pack 4 low bytes.\n\t" 
		"packuswb %%mm1, %%mm1    #  Pack 4 low bytes.\n\t" 
		"movd %%mm0, (%0)         #  *P = word\n" 
		"movd %%mm1, (%1)         #  *Q = word\n" 
		/* outputs */
		: "+r" (p), 
		  "+r" (q)
		/* inputs */
		: "r" ((unsigned int)(p->Alpha)), 
		  "r" ((unsigned int)(q->Alpha)));
	}
	srcRowPtr += srcPtr->pixelsPerRow;
    }
#else 
	for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
	    unsigned int alpha;
		
	    /*
	     * Small wins:  
	     *
	     * We can compute 
	     *      dest = (fg * alpha) + (beta * bg);
	     * for all RGBA components at once. 
	     *
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
		"movd %1, %%mm2           #  mm2 = 0,0,0,A\n\t" 
		"movd (%0), %%mm1         #  mm1 = sp\n\t" 
		"punpcklwd %%mm2, %%mm2   #  mm2 = 0,0,A,A\n\t"
		"punpcklbw %%mm6, %%mm1   #  mm1 = Sa,Sr,Sg,Sb\n\t" 
		"punpcklwd %%mm2, %%mm2   #  mm2 = A,A,A,A\n\t"
		"por %%mm4, %%mm2	  #  mm2 = 0xff,A,A,A\n\t"
		"pmullw %%mm1, %%mm2      #  mm2 = S*A\n\t" 
		"paddw %%mm5, %%mm2       #  mm2 = (S*A)+ROUND\n\t"
		"movq %%mm2, %%mm3	  #  mm3 = P16\n\t" 
		"psrlw $8, %%mm3          #  mm3 = P16 / 256\n\t"
		"paddw %%mm2, %%mm3       #  mm3 = (P16 / 256) + P16\n\t" 
		"psrlw $8, %%mm3          #  mm3 = P8 ~= P16 / 257\n\t"
		"packuswb %%mm3, %%mm3    #  Pack 4 low bytes.\n\t" 
		"movd %%mm3, (%0)         #  *sp = word\n" :
		"+r" (sp) :
		"r" (alpha));
	}
	srcRowPtr += srcPtr->pixelsPerRow;
    }
#endif
    asm volatile ("emms");
    srcPtr->flags |= BLT_PIC_ASSOCIATED_COLORS;
}


static void
UnassociateColors(Pict *srcPtr)		/* (in/out) picture */
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
	"pxor %%mm6, %%mm6	# mm6 = 0\n\t"
	"pcmpeqw %%mm5, %%mm5	# mm5 = -1 \n\t"
	"psubw %%mm6, %%mm5	# mm5 = 1,1,1,1\n\t"
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
	     * Packing unsigned with saturation performs the
	     * necessary clamping without the branch misprediction
	     * penalty.
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
		"por %%mm4, %%mm2	  #  mm2 = 0xff,IA,IA,IA\n\t"
		"pmullw %%mm1, %%mm2      #  mm2 = S*IA\n\t" 
		"paddw %%mm5, %%mm2       #  mm2 = (S*A)+ROUND\n\t"
		"movq %%mm2, %%mm3	  #  mm3 = P16\n\t" 
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
    srcPtr->flags &= ~BLT_PIC_ASSOCIATED_COLORS;
}

/* 
 *---------------------------------------------------------------------------
 *
 * CopyPictureBits --
 *
 *	Creates a copy of the given picture using SSE xmm registers.  
 * 
 *	FIXME: This is broken since it uses an double-world aligned quad word
 *	move instruction.
 *	
 * Results: 
 *	None.
 *
 * Side effects: 
 *	The area specified in the source picture is copied to the
 *	destination picture.
 *
 * -------------------------------------------------------------------------- 
 */
static void
CopyPictureBits(Pict *destPtr, Pict *srcPtr, int sx, int sy, 
		int w, int h, int dx, int dy)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;
    int dw, dh, width, height;

    dw = destPtr->width - dx;
    dh = destPtr->height - dy;
    width  = MIN(dw, w);
    height = MIN(dh, h);
    
    srcRowPtr  = srcPtr->bits  + (srcPtr->pixelsPerRow * sy)  + sx;
    destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * dy) + dx;
    for (y = 0; y < height; y++) {
	Blt_Pixel *sp, *send, *dp;

	dp = destRowPtr;
	for (sp = srcRowPtr, send = sp + (width & ~3); sp < send; sp += 4, 
		 dp += 4) {
	    asm volatile (
		"movdqa (%1), %%xmm1\n\t"
		"movdqa %%xmm1, (%0)\n\t" 
		: "+r" (dp) 
		: "r" (sp));
	}
	switch (width & 3) {
	case 3:		dp->u32 = sp->u32; sp++; dp++;
	case 2:		dp->u32 = sp->u32; sp++; dp++;
	case 1:		dp->u32 = sp->u32; sp++; dp++;
	case 0:		break;
	}
	srcRowPtr += srcPtr->pixelsPerRow;
	destRowPtr += destPtr->pixelsPerRow;
    }
    destPtr->flags = (srcPtr->flags | BLT_PIC_DIRTY);
    asm volatile ("emms");
}

#ifdef notdef
static void
BoxCarVertically(Pict *destPtr, Pict *srcPtr, size_t r)
{
    unsigned int x;
    size_t *map;
    int fscale;
    size_t fwidth;			/* Filter width */
    float s;
    Blt_Pixel mask;

    map = CreateNeighborhoodMap(srcPtr->height, r);
    fwidth = r + r + 1;
    s = 1.0f / fwidth;
    fscale = float2si(s);

    fwidth--;
    /* 
     * mm3 = r16 g16 b16 a16  accumulators
     */
    /* Apply filter to each row. */

    /* Create mask for alpha component.  We'll use this mask to make sure we
     * don't change the alpha component of a pixel.  */
    mask.u32 = 0;
    mask.Alpha = 0xFF;
    asm volatile (
        /* Generate constants needed below. */
	"pxor %%mm6, %%mm6	# mm6 = 0\n\t"
	"pcmpeqw %%mm5, %%mm5	# mm5 = -1 \n\t"
	"psubw %%mm6, %%mm5	# mm5 = 1,1,1,1\n\t"
	"movd %0, %%mm4         # mm4 = mask\n\t" 
	"psllw $7, %%mm5        # mm5 = ROUND = 128\n" 
	"punpcklbw %%mm6, %%mm4 # mm4 = 0,0,0,FF\n\t" 
	: /* inputs */
	: "r" (mask.u32));

    for (x = 0; x < srcPtr->width; x++) {
	Blt_Pixel *dp, *srcColPtr;
	int r, g, b, a;
	unsigned int y;

	srcColPtr = srcPtr->bits + x;
	asm volatile (
	    /* Initialize 16 bit accumulators. */
	    "pxor %%mm4, %%mm4	# mm4 = 0\n\t");
	/* Prime the pump. */
	for (y = 0; y < fwidth; y++) {
	    Blt_Pixel *sp;

	    sp = srcColPtr + (srcPtr->pixelsPerRow * map[y]);
	    asm volatile (
		/* 
		 * mm0 = 
		 * mm1 = sp
		 * mm4 = accumulator
		 * mm6 = 0
		 * mm7 = 
		 */
		"movd (%0), %%mm1         #  mm1 = sp\n\t" 
		"punpcklbw %%mm6, %%mm1   #  mm1 = Sa,Sr,Sg,Sb\n\t" 
		"paddw %%mm1, %%mm4       #  mm4 = accumulator\n\t"
		: "+r" (sp));
	}
	dp = destPtr->bits + x;
	for (y = 0; y < srcPtr->height; y++) {
	    Blt_Pixel *s1, s2;
	    int fr, fg, fb, fa;

	    s1 = srcColPtr + (srcPtr->pixelsPerRow * map[y + fwidth]);
	    s2 = srcColPtr + (srcPtr->pixelsPerRow * map[y]);
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
		"movd (%0), %%mm1         #  mm1 = next\n\t" 
		"movd (%1), %%mm2         #  mm2 = prev\n\t" 
		"punpcklbw %%mm6, %%mm1   #  mm1 = Sa,Sr,Sg,Sb\n\t" 
		"punpcklbw %%mm6, %%mm2   #  mm2 = Sa,Sr,Sg,Sb\n\t" 
		"paddw %%mm1, %%mm4       #  mm4 = accumulator\n\t"
		"movq %%mm4, %%mm5	  #  mm3 = P16\n\t" 
                "pmulhw %%mm3, %%mm5      #  mm0 = S15 * W14\n\t"  
		"psrlw $8, %%mm3          #  mm3 = P16 / 256\n\t"
		"paddw %%mm2, %%mm3       #  mm3 = (P16 / 256) + P16\n\t" 
		"psrlw $8, %%mm3          #  mm3 = P8 ~= P16 / 257\n\t"
		"packuswb %%mm3, %%mm3    #  Pack 4 low bytes.\n\t" 
		"movd %%mm3, (%0)         #  *sp = word\n" 
		: "+r" (s1) "+r" (s2) 
		: "r" (alpha));
	}
	    r += sp->Red;
	    g += sp->Green;
	    b += sp->Blue;
	    a += sp->Alpha;
	    fr = r * fscale;
	    fg = g * fscale;
	    fb = b * fscale;
	    fa = a * fscale;
	    dp->Red = (unsigned char)SICLAMP(fr);
	    dp->Green = (unsigned char)SICLAMP(fg);
	    dp->Blue = (unsigned char)SICLAMP(fb);
	    dp->Alpha = (unsigned char)SICLAMP(fa);
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
BoxCarHorizontally(Pict *destPtr, Pict *srcPtr, size_t r)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    float s;
    size_t *map;
    int fscale;
    unsigned int y;
    size_t fwidth;		/* Filter width */

    fwidth = r + r + 1;
    map = CreateNeighborhoodMap(srcPtr->width, r);
    s = 1.0f / fwidth;
    fscale = float2si(s);
    destRowPtr = destPtr->bits;
    srcRowPtr = srcPtr->bits;
    fwidth--;
    for (y = 0; y < srcPtr->height; y++) {
	Blt_Pixel *dp;
	unsigned int x;
	int r, g, b, a;
	
	/* Prime the pump. Get sums for each component for the first (fwidth)
	 * pixels in the column. */
	r = g = b = a = 0;
	for (x = 0; x < fwidth; x++) {
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
	    int fr, fg, fb, fa;

	    sp = srcRowPtr + map[x + fwidth];
	    r += sp->Red;
	    g += sp->Green;
	    b += sp->Blue;
	    a += sp->Alpha;
	    fr = r * fscale;
	    fg = g * fscale;
	    fb = b * fscale;
	    fa = a * fscale;
	    dp->Red = (unsigned char)SICLAMP(fr);
	    dp->Green = (unsigned char)SICLAMP(fg);
	    dp->Blue = (unsigned char)SICLAMP(fb);
	    dp->Alpha = (unsigned char)SICLAMP(fa);
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
#endif


#ifdef notdef
/* 
 *---------------------------------------------------------------------------
 *
 * BlankPicture --
 *
 *	Creates a copy of the given picture using SSE xmm registers.  
 *	
 * Results: 
 *	None.
 *
 * Side effects: 
 *	The area specified in the source picture is copied to the
 *	destination picture.
 *
 * -------------------------------------------------------------------------- 
 */
static void
BlankPicture(Pict *destPtr, unsigned int colorValue)
{
    Blt_Pixel *destRowPtr;
    int y;

    asm volatile (
	/* How do you load a xmm register? */
        /* Generate constants needed below. */
	"pxor %%mm6, %%mm6	# mm6 = 0\n\t"
	"pcmpeqw %%mm5, %%mm5	# mm5 = -1 \n\t"
	"psubw %%mm6, %%mm5	# mm5 = 1,1,1,1\n\t"
	"movd %0, %%mm4         # mm4 = C\n\t" 
	"psllw $7, %%mm5        # mm5 = ROUND = 128\n" 
	"punpcklbw %%mm6, %%mm4 # mm4 = 0,0,0,FF\n\t" 
	: /* inputs */
	: "r" (colorValue));
    destRowPtr = destPtr->bits;
    for (y = 0; y < destPtr->height; y++) {
	Blt_Pixel *dp, *dend;

	for (dp = destRowPtr, dend = dp + (destPtr->width & ~3); dp < dend; 
	     dp += 4)
	    asm volatile (
		"movdqa %%xmm1, (%0)\n\t" 
		: "+r" (dp));
	}
	switch (width & 3) {
	case 3:		dp->u32 = colorValue; dp++;
	case 2:		dp->u32 = colorValue; dp++;
	case 1:		dp->u32 = colorValue; dp++;
	case 0:		break;
	}
	destRowPtr += destPtr->pixelsPerRow;
    }
    destPtr->flags = (srcPtr->flags | BLT_PIC_DIRTY);
    asm volatile ("emms");
}
#endif

static int
x86HaveCpuId(void)
{
    unsigned int ecx;

    asm volatile (
	/* See if ID instruction is supported. Save a copy of
	 * EFLAGS in eax and ecx */
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
	return 1;
    } 
    return 0;
}

static int
x86GetCpuVersion(char *version)
{
    unsigned int ebx, ecx, edx;

    if (!x86HaveCpuId()) {
	return 0;
    }
    asm volatile (
	/* See if ID instruction is supported. Save a copy of
	 * EFLAGS in eax and ecx */

	/* Get standard CPUID information, and go to a specific vendor
	 * section */
#ifdef __amd64__
	"pushq %%rbx\n\t" 
#else 
	"push %%ebx\n\t" 
#endif
	"xor %%eax, %%eax\n\t"
	"cpuid\n\t"
	"movl %%ebx, %0\n\n\t"	
#ifdef __amd64__
	"popq %%rbx\n\n\t"	
#else 
	"pop %%ebx\n\n\t"	
#endif
	: "=a" (ebx), "=d" (edx), "=c" (ecx)
	: /* no input */
	);
    memcpy (version,   &ebx, 4);
    memcpy (version+4, &edx, 4);
    memcpy (version+8, &ecx, 4);
    version[12] = '\0';
#ifdef notdef
    fprintf(stderr, "version=%s\n", version);
#endif
    return 1;
}

static unsigned int
x86GetCpuFlags(void)
{
    unsigned int edx;

    asm volatile (
	/* Get the CPUID flags (eax=1). */
#ifdef __amd64__
	"pushq %%rbx\n\t"	
#else 
	"push %%ebx\n\t"	
#endif
	"movl $1, %%eax\n\t"
	"cpuid\n\t"
#ifdef __amd64__
	"popq %%rbx\n\t"	
#else 
	"pop %%ebx\n\t"	
#endif
	: "=d" (edx));
    return edx;
}

#define CPU_FEATURE_NONE  (0)
#define CPU_FEATURE_MMX   (1<<0)
#define CPU_FEATURE_SSE   (1<<1)
#define CPU_FEATURE_SSE2  (1<<2)
#define CPU_FEATURE_3DNOW (1<<3)

#define CPU_FEATURE_AMD_MMXEXT     (1 << 22)
#define CPU_FEATURE_AMD_3DNOW      (1 << 31)
#define CPU_FEATURE_CENTAUR_3DNOW  (1 << 31)
#define CPU_FEATURE_CENTAUR_MMX    (1 << 23)
#define CPU_FEATURE_CENTAUR_MMXEXT (1 << 24)
#define CPU_FEATURE_CYRIX_MMX      (1 << 23)
#define CPU_FEATURE_CYRIX_MMXEXT   (1 << 24)
#define CPU_FEATURE_INTEL_MMX      (1 << 23)
#define CPU_FEATURE_INTEL_SSE      (1 << 25)
#define CPU_FEATURE_INTEL_SSE2     (1 << 26)
#define CPU_FEATURE_AMD_MMX        (1 << 23)
#define CPU_FEATURE_AMD_SSE        (1 << 25)
#define CPU_FEATURE_AMD_SSE2       (1 << 26)

static void
PrintFeatures(Tcl_Interp *interp, unsigned int features)
{
    if (features & CPU_FEATURE_MMX) {
	Tcl_AppendElement(interp, "mmx");
    }
    if (features & CPU_FEATURE_SSE) {
	Tcl_AppendElement(interp, "sse");
    }
    if (features & CPU_FEATURE_SSE2) {
	Tcl_AppendElement(interp, "sse2");
    }
    if (features & CPU_FEATURE_3DNOW) {
	Tcl_AppendElement(interp, "3dnow");
    }
    if (features & CPU_FEATURE_AMD_MMXEXT) {
	Tcl_AppendElement(interp, "amd_mmxext");
    }
    if (features & CPU_FEATURE_AMD_3DNOW) {
	Tcl_AppendElement(interp, "amd_mmxext");
    }
    if (features & CPU_FEATURE_CENTAUR_3DNOW) {
	Tcl_AppendElement(interp, "amd_mmxext");
    }
    if (features & CPU_FEATURE_CENTAUR_MMX) {
	Tcl_AppendElement(interp, "amd_mmxext");
    }
    if (features & CPU_FEATURE_CENTAUR_MMXEXT) {
	Tcl_AppendElement(interp, "amd_mmxext");
    }
    if (features & CPU_FEATURE_CYRIX_MMX) {
	Tcl_AppendElement(interp, "cyrix_mmx");
    }	
    if (features & CPU_FEATURE_CYRIX_MMXEXT) {
	Tcl_AppendElement(interp, "cyrix_mmxext");
    }
    if (features & CPU_FEATURE_INTEL_MMX) {
	Tcl_AppendElement(interp, "intel_mmx");
    }
    if (features & CPU_FEATURE_INTEL_SSE) {
	Tcl_AppendElement(interp, "intel_sse");
    }
    if (features & CPU_FEATURE_INTEL_SSE2) {
	Tcl_AppendElement(interp, "intel_sse2");
    }
    if (features & CPU_FEATURE_AMD_MMX) {
	Tcl_AppendElement(interp, "amd_mmax");
    }
    if (features & CPU_FEATURE_AMD_SSE) {
	Tcl_AppendElement(interp, "amd_sse");
    }
    if (features & CPU_FEATURE_AMD_SSE2) {
	Tcl_AppendElement(interp, "amd_sse2");
    }
}

static int
x86CpuFeatures(void)
{
    char version[13];
    unsigned int flags, featureFlags;

    featureFlags = CPU_FEATURE_NONE;
    if (!x86GetCpuVersion(version)) {
	return featureFlags;
    }
    if (strcmp(version, "GenuineIntel") == 0) {
	flags = x86GetCpuFlags();
	if (flags & CPU_FEATURE_INTEL_MMX) {
	    featureFlags |= CPU_FEATURE_MMX;
	} 
	if (flags & CPU_FEATURE_INTEL_SSE) {
	    featureFlags |= CPU_FEATURE_SSE;
	}
	if (flags & CPU_FEATURE_INTEL_SSE2) {
	    featureFlags |= CPU_FEATURE_SSE2;
	}
    } else if (strcmp(version, "AuthenticAMD") == 0) {
	flags = x86GetCpuFlags();
	if (flags & CPU_FEATURE_AMD_MMX) {
	    featureFlags |= CPU_FEATURE_MMX;
	}
	if (flags & CPU_FEATURE_AMD_SSE) {
	    featureFlags |= CPU_FEATURE_SSE;
	}
	if (flags & CPU_FEATURE_AMD_SSE2) {
	    featureFlags |= CPU_FEATURE_SSE2;
	}
	if (flags & CPU_FEATURE_AMD_3DNOW) {
	    featureFlags |= CPU_FEATURE_3DNOW;
	}
    } else if (strcmp(version, "CyrixInstead") == 0) {
	flags = x86GetCpuFlags();
	if (flags & CPU_FEATURE_CYRIX_MMX) {
	    featureFlags |= CPU_FEATURE_MMX;
	} 
    } 
    return featureFlags;
}

int
Blt_CpuFeatures(Tcl_Interp *interp, int *featuresPtr)
{
    unsigned int features;

    features = x86CpuFeatures();
    if (features & CPU_FEATURE_MMX) {
#ifdef notdef
	bltPictProcsPtr = &mmxPictureProcs;
#else 
	bltPictProcsPtr->applyPictureToPictureProc = ApplyPictureToPicture;
	bltPictProcsPtr->applyScalarToPictureProc = ApplyScalarToPicture;
	bltPictProcsPtr->tentHorizontallyProc = TentHorizontally;
	bltPictProcsPtr->tentVerticallyProc = TentVertically;
	bltPictProcsPtr->zoomHorizontallyProc = ZoomHorizontally;
	bltPictProcsPtr->zoomVerticallyProc = ZoomVertically;
#ifdef notdef
	bltPictProcsPtr->blendPicturesProc = BlendPictures;
#endif
	bltPictProcsPtr->associateColorsProc = AssociateColors;
	bltPictProcsPtr->selectPixelsProc = SelectPixels;
#ifdef notdef
	if (features & CPU_FEATURE_SSE) {
	    /*
	      bltPictProcsPtr->copyPictureBitsProc = CopyPictureBits;
	    */
	}
#endif
#endif
    }
    if (featuresPtr != NULL) {
	*featuresPtr = features;
    }
    if (interp != NULL) {
	PrintFeatures(interp, features);
    }
    return TCL_OK;
}

#else 

int
Blt_CpuFeatures(Tcl_Interp *interp, int *featuresPtr)
{
    if (featuresPtr != NULL) {
	*featuresPtr = 0;
    }
    return TCL_OK;
}

#endif /* HAVE_X86_ASM */

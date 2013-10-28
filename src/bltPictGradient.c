/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltPictGrad.c --
 *
 *	Copyright 2004 George A Howlett.
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

#include <limits.h>

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include "bltMath.h"
#include "bltPicture.h"
#include "bltPictInt.h"

/* 
 * Quick and dirty random number generator. 
 *
 * http://www.shadlen.org/ichbin/random/generators.htm#quick 
 */
#define JITTER_SEED	31337
#define JITTER_A	1099087573U
#define RANDOM_SCALE    2.3283064370807974e-10

static void 
RandomSeed(Blt_Random *randomPtr, unsigned int seed) {
    randomPtr->value = seed;
}

static void
RandomInit(Blt_Random *randomPtr) 
{
    RandomSeed(randomPtr, JITTER_SEED);
}

static INLINE double
RandomNumber(Blt_Random *randomPtr)
{
#if (SIZEOF_INT == 8) 
    /* mask the lower 32 bits on machines where int is a 64-bit quantity */
    randomPtr->value = ((1099087573  * (randomPtr->value))) & ((unsigned int) 0xffffffff);
#else
    /* on machines where int is 32-bits, no need to mask */
    randomPtr->value = (JITTER_A  * randomPtr->value);
#endif	/* SIZEOF_INT == 8 */
    return (double)randomPtr->value * RANDOM_SCALE;
}

void
Blt_Jitter_Init(Blt_Jitter *jitterPtr) 
{
    RandomInit(&jitterPtr->random);
    jitterPtr->range = 0.1;
    jitterPtr->offset = -0.05;		/* Jitter +/-  */
}

static INLINE double 
Jitter(Blt_Jitter *jitterPtr) {
    double value;

    value = RandomNumber(&jitterPtr->random);
    return (value * jitterPtr->range) + jitterPtr->offset;
}

void
Blt_GradientPicture(
    Pict *destPtr,			/* (out) Picture to contain the new
					 * gradient. */
    Blt_Pixel *maxPtr,			/* Color to represent 1.0 */
    Blt_Pixel *minPtr,			/* Color to represent 0.0 */
    Blt_Gradient *gradientPtr,
    Blt_Jitter *jitterPtr)
{
    double rRange, gRange, bRange, aRange;

    /* Compute the ranges for each color component. */
    rRange = (double)(maxPtr->Red   - minPtr->Red);
    gRange = (double)(maxPtr->Green - minPtr->Green);
    bRange = (double)(maxPtr->Blue  - minPtr->Blue);
    aRange = (double)(maxPtr->Alpha - minPtr->Alpha);

    switch (gradientPtr->type) {
    case BLT_GRADIENT_TYPE_HORIZONTAL:
	{
	    Blt_Pixel *copyRowPtr, *destRowPtr, *dp;
	    int x, y;
	    double scaleFactor;

	    /* Draw the gradient on the first row, then copy the row to
	     * each subsequent row. */
	    
	    destRowPtr = destPtr->bits;
	    scaleFactor = 0.0;
	    if (destPtr->width > 1) {
		scaleFactor = 1.0 / (destPtr->width - 1);
	    }
	    for (dp = destRowPtr, x = 0; x < destPtr->width; x++, dp++) {
		double t;
		
		t = (double)x * scaleFactor;
		if (jitterPtr->range > 0.0) {
		    t += Jitter(jitterPtr);
		    t = JCLAMP(t);
		}
#ifdef notdef
		if (gradientPtr->scale == BLT_GRADIENT_SCALE_LOG) {
		    t = log10(9.0 * t + 1.0);
		} else if (gradientPtr->scale == BLT_GRADIENT_SCALE_ATAN) {
		    t = atan(18.0 * (t-0.05) + 1.0) / M_PI_2;
		}
#endif
		dp->Red   = (unsigned char)(minPtr->Red   + t * rRange);
		dp->Green = (unsigned char)(minPtr->Green + t * gRange);
		dp->Blue  = (unsigned char)(minPtr->Blue  + t * bRange);
		dp->Alpha = (unsigned char)(minPtr->Alpha + t * aRange);
	    }
	    destRowPtr += destPtr->pixelsPerRow;;
	    copyRowPtr = destPtr->bits;
	    for (y = 1; y < destPtr->height; y++) {
		Blt_Pixel *dp, *sp, *send;
		
		for (dp = destRowPtr, sp = copyRowPtr, 
			 send = sp + destPtr->width; sp < send; dp++, sp++) {
		    dp->u32 = sp->u32;
		}
		copyRowPtr += destPtr->pixelsPerRow;
		destRowPtr += destPtr->pixelsPerRow;
	    }
	}
	break;

    case BLT_GRADIENT_TYPE_VERTICAL:
	{
	    Blt_Pixel *destRowPtr;
	    int y;
	    double scaleFactor;

	    scaleFactor = 0.0;
	    if (destPtr->height > 1) {
		scaleFactor = 1.0 / (destPtr->height - 1);
	    }
	    destRowPtr = destPtr->bits;
	    for (y = 0; y < destPtr->height; y++) {
		Blt_Pixel *dp, *dend;
		double t;
		Blt_Pixel color;
		
		/* Compute the color value for the row and then replicate
		 * it in every pixel in the row. */
		
		dp = destRowPtr;
		t = (double)y * scaleFactor;
		if (jitterPtr->range > 0.0) {
		    t += Jitter(jitterPtr);
		    t = JCLAMP(t);
		}
		if (gradientPtr->scale == BLT_GRADIENT_SCALE_LOG) {
		    t = log10(9.0 * t + 1.0);
		} else if (gradientPtr->scale == BLT_GRADIENT_SCALE_ATAN) {
		    t = atan(18.0 * (t-0.05) + 1.0) / M_PI_2;
		}
		color.Red =   (unsigned char)(minPtr->Red + t * rRange);
		color.Green = (unsigned char)(minPtr->Green +  t * gRange);
		color.Blue =  (unsigned char)(minPtr->Blue +  t * bRange);
		color.Alpha = (unsigned char)(minPtr->Alpha +  t * aRange);
		for (dp = destRowPtr, dend = dp + destPtr->width; dp < dend;
		     dp++) {
		    dp->u32 = color.u32;
		}
		destRowPtr += destPtr->pixelsPerRow;
	    }
	}
	break;
	
    case BLT_GRADIENT_TYPE_DIAGONAL_DOWN:
    case BLT_GRADIENT_TYPE_DIAGONAL_UP:
	{
	    Blt_Pixel *destRowPtr;
	    int y;
	    float sinTheta, cosTheta;	/* Rotation of diagonal. */
	    float cx, cy;		/* Offset to the center of the
					 * diagonal. */
	    float length;		/* Length of diagonal. */
	    float scaleFactor;

	    cx = destPtr->width * 0.5;
	    cy = destPtr->height * 0.5;
	    length = sqrt(destPtr->width * destPtr->width + 
			  destPtr->height * destPtr->height);
	    cosTheta = destPtr->width / length;
	    sinTheta = destPtr->height / length;
	    if (gradientPtr->type == BLT_GRADIENT_TYPE_DIAGONAL_DOWN) {
		sinTheta = -sinTheta;
	    }
	    scaleFactor = 0.0;
	    if (length > 2) {
		scaleFactor = 1.0 / (length - 1);
	    } 
	    destRowPtr = destPtr->bits;
	    for (y = 0; y < destPtr->height; y++) {
		Blt_Pixel *dp;
		int x;
		
		for (dp = destRowPtr, x = 0; x < destPtr->width; x++, dp++) {
		    double t;
		    double tx, ty, rx;
		    
		    /* Translate to the center of the reference window. */
		    tx = x - cx;
		    ty = y - cy;
		    /* Rotate by the slope of the diagonal. */
		    rx = (tx * cosTheta) - (ty * sinTheta);
		    /* Translate back.  */
		    rx += length * 0.5;
		    
		    assert(rx >= 0 && rx < length);
		    t = rx * scaleFactor;
		    if (jitterPtr->range > 0.0) {
			t += Jitter(jitterPtr);
			t = JCLAMP(t);
		    }
		    if (gradientPtr->scale == BLT_GRADIENT_SCALE_LOG) {
			t = log10(9.0 * t + 1.0);
		    } else if (gradientPtr->scale == BLT_GRADIENT_SCALE_ATAN) {
			t = atan(18.0 * (t-0.05) + 1.0) / M_PI_2;
		    }
		    dp->Red   = (unsigned char)(minPtr->Red + t * rRange);
		    dp->Green = (unsigned char)(minPtr->Green + t * gRange);
		    dp->Blue  = (unsigned char)(minPtr->Blue + t * bRange);
		    dp->Alpha = (unsigned char)(minPtr->Alpha + t * aRange);
		}
		destRowPtr += destPtr->pixelsPerRow;
	    }
	}
	break;

    case BLT_GRADIENT_TYPE_RADIAL:
	{
	    Blt_Pixel *destRowPtr;
	    int y;
	    destRowPtr = destPtr->bits;
	    float cx, cy;
	    float scaleFactor;
	    float length, halfLength;

	    cx = destPtr->width * 0.5;
	    cy = destPtr->height * 0.5;
	    length = sqrt(destPtr->width * destPtr->width + 
			  destPtr->height * destPtr->height);
	    halfLength = length * 0.5;
	    scaleFactor = 0.0;
	    if (halfLength > 2) {
		scaleFactor = 1.0 / (halfLength - 1);
	    } 

	    /* Center coordinates. */
	    for (y = 0; y < destPtr->height; y++) {
		int x;
		Blt_Pixel *dp;
		
		for (dp = destRowPtr, x = 0; x < destPtr->width; x++, dp++) {
		    double dx, dy, d;
		    double t;
		    
		    /* Translate to the center of the reference window. */
		    dx = x - cx;
		    dy = y - cy;
		    d = sqrt(dx * dx + dy * dy);
		    t = 1.0 - (d * scaleFactor);
		    if (jitterPtr->range > 0.0) {
			t += Jitter(jitterPtr);
			t = JCLAMP(t);
		    }
		    if (gradientPtr->scale == BLT_GRADIENT_SCALE_LOG) {
			t = log10(9.0 * t + 1.0);
		    } else if (gradientPtr->scale == BLT_GRADIENT_SCALE_ATAN) {
			t = atan(18.0 * (t-0.05) + 1.0) / M_PI_2;
		    }
		    dp->Red = (unsigned char)(minPtr->Red + t * rRange);
		    dp->Green = (unsigned char)(minPtr->Green + t * gRange);
		    dp->Blue = (unsigned char)(minPtr->Blue + t * bRange);
		    dp->Alpha = (unsigned char)(t * aRange);
		}
		destRowPtr += destPtr->pixelsPerRow;
	    }
	}
	break;

    case BLT_GRADIENT_TYPE_CONICAL:
	{
	    Blt_Pixel *destRowPtr;
	    int y;
	    float cx, cy;

	    destRowPtr = destPtr->bits;
	    cx = destPtr->width * 0.5;
	    cy = destPtr->height * 0.5;
	    /* Center coordinates. */
	    for (y = 0; y < destPtr->height; y++) {
		int x;
		Blt_Pixel *dp;
		
		for (dp = destRowPtr, x = 0; x < destPtr->width; x++, dp++) {
		    double dx, dy, d;
		    double t;
		    /* Translate to the center of the reference window. */

		    dx = x - cx;
		    dy = y - cy;
		    if (dx == 0.0) {
			d = 0.0;
		    } else {
			d = cos(atan(dy / dx));
		    }
		    d = fabs(fmod(d, 360.0));
		    /*t = 1.0 - (d * scaleFactor) * (1 / 2 * M_PI); */
		    fprintf(stderr, "d=%g angle=%g\n", d, atan(dy / dx) *RAD2DEG);
			t = 1.0 - (d / (M_PI));
			t = d;
		    if (jitterPtr->range > 0.0) {
			t += Jitter(jitterPtr);
		    }
		    if (gradientPtr->scale == BLT_GRADIENT_SCALE_LOG) {
			t = log10(9.0 * t + 1.0);
		    } else if (gradientPtr->scale == BLT_GRADIENT_SCALE_ATAN) {
			t = atan(18.0 * (t-0.05) + 1.0) / M_PI_2;
		    }
		    t = JCLAMP(t);
		    dp->Red   = (unsigned char)(minPtr->Red   + t * rRange);
		    dp->Green = (unsigned char)(minPtr->Green + t * gRange);
		    dp->Blue  = (unsigned char)(minPtr->Blue  + t * bRange);
		    dp->Alpha = 0xFF /*(unsigned char)(t * aRange); */;
		}
		destRowPtr += destPtr->pixelsPerRow;
	    }
	}
	break;
	
    case BLT_GRADIENT_TYPE_RECTANGULAR:
	{
	    Blt_Pixel *destRowPtr;
	    int y;
	    
	    destRowPtr = destPtr->bits;
	    for (y = 0; y < destPtr->height; y++) {
		Blt_Pixel *dp;
		int x;
		double ty;
		
		ty = (double)y / (double)(destPtr->height - 1);
		if (ty > 0.5) {
		    ty = 1.0 - ty;
		}
		dp = destRowPtr;
		for (x = 0; x < destPtr->width; x++) {
		    double t;
		    double tx;

		    tx = (double)x / (double)(destPtr->width - 1);
		    if (tx > 0.5) {
			tx = 1.0 - tx;
		    }
		    t = MIN(tx, ty);
		    t += t;
		    if (jitterPtr->range > 0.0) {
			t += Jitter(jitterPtr);
			t = JCLAMP(t);
		    }
		    if (gradientPtr->scale == BLT_GRADIENT_SCALE_LOG) {
			t = log10(9.0 * t + 1.0);
		    } else if (gradientPtr->scale == BLT_GRADIENT_SCALE_ATAN) {
			t = atan(18.0 * (t-0.05) + 1.0) / M_PI_2;
		    }
		    dp->Red = (unsigned char)(minPtr->Red + t * rRange);
		    dp->Green = (unsigned char)(minPtr->Green + t * gRange);
		    dp->Blue = (unsigned char)(minPtr->Blue + t * bRange);
		    dp->Alpha = (unsigned char)(t * aRange);
		    dp++;
		}
		destRowPtr += destPtr->pixelsPerRow;
	    }
	}
	break;
    }
}

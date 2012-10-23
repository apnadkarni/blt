
/*
 * bltWinPainter.h --
 *
 * This header contains the private definitions for a painter in 
 * the BLT toolkit.
 *
 *	Copyright 1998-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The color allocation routines are adapted from tkImgPhoto.c of the
 * Tk library distrubution.  The photo image type was designed and
 * implemented by Paul Mackerras.
 *
 *	Copyright (c) 1987-1993 The Regents of the University of
 *	California.
 *
 *	Copyright (c) 19941998 Sun Microsystems, Inc.
 * 
 */

#ifndef _BLT_WIN_PAINTER_H
#define _BLT_WIN_PAINTER_H

#ifdef notdef
#define PAINTER_COLOR_WINDOW		(1<<0)
#define PAINTER_BW			(1<<1)
#define PAINTER_MAP_COLORS		(1<<2)
#endif

/*
 * Painter --
 *
 * This structure represents a painter used to display picture images.  A
 * painter is specified by a combination of display, colormap, depth, and
 * monitor gamma value.  Painters contain information necessary to display a
 * picture.  This includes both an RGB to pixel map, and a RGB to allocated
 * color map.
 *
 * Painters may be shared by more than one client and are reference counted.
 * When no clients are using the painter, it is freed.
 */

struct _Blt_Painter {
    Display *display;		/* Display of painter. Used to free colors
				 * allocated. */

    int depth;			/* Pixel depth of the display. */

    float gamma;		/* Gamma correction value of monitor. */

    Colormap colormap;

    unsigned int flags;		/* Flags listed below. */

    int refCount;		/* # of clients using this painter. If zero,
				 * # the painter is freed. */

    Blt_HashEntry *hashPtr;	/* Used to delete the painter entry from the
				 * hash table of painters. */

    GC gc;			/* GC used to draw the image. */

    unsigned char gammaTable[256]; /* Input gamma lookup table. Used to map
				 * non-linear monitor values back to RGB
				 * values. This is used whenever we take a
				 * snapshot of the screen (e.g. alpha
				 * blending).  Computes the power mapping.  D
				 * = I^gamma. */

    unsigned char igammaTable[256]; /* Output gamma lookup table. Used to map
				 * RGB values to non-linear monitor
				 * values. Computes the inverse power mapping.
				 * I~ = D^1/gamma. */

    Blt_Pixel palette[256];	/* Maps the picture's 8-bit RGB values to the
				 * RGB values of the colors actually
				 * allocated. This is used for dithering the
				 * picture. */

};

typedef struct _Blt_Painter Painter;

#endif /* _BLT_WIN_PAINTER_H */

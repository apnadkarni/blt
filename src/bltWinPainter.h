/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltWinPainter.h --
 *
 * This header contains the private definitions for a painter in 
 * the BLT toolkit.
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
 * The color allocation routines are adapted from tkImgPhoto.c of the Tk
 * library distrubution.  The photo image type was designed and implemented
 * by Paul Mackerras.
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 *
 * Copyright (c) 19941998 Sun Microsystems, Inc.
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

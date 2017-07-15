/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltUnixPainter.c --
 *
 * This module implements X11-specific image processing procedures for the
 * BLT toolkit.
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
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 * 
 *   This software is copyrighted by the Regents of the University of
 *   California, Sun Microsystems, Inc., and other parties.  The following
 *   terms apply to all files associated with the software unless
 *   explicitly disclaimed in individual files.
 * 
 *   The authors hereby grant permission to use, copy, modify, distribute,
 *   and license this software and its documentation for any purpose,
 *   provided that existing copyright notices are retained in all copies
 *   and that this notice is included verbatim in any distributions. No
 *   written agreement, license, or royalty fee is required for any of the
 *   authorized uses.  Modifications to this software may be copyrighted by
 *   their authors and need not follow the licensing terms described here,
 *   provided that the new terms are clearly indicated on the first page of
 *   each file where they apply.
 * 
 *   IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 *   FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 *   ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 *   DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 * 
 *   THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 *   NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND
 *   THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 *   MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *   GOVERNMENT USE: If you are acquiring this software on behalf of the
 *   U.S. government, the Government shall have only "Restricted Rights" in
 *   the software and related documentation as defined in the Federal
 *   Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 *   are acquiring the software on behalf of the Department of Defense, the
 *   software shall be classified as "Commercial Computer Software" and the
 *   Government shall have only "Restricted Rights" as defined in Clause
 *   252.227-7013 (b) (3) of DFARs.  Notwithstanding the foregoing, the
 *   authors grant the U.S. Government and others acting in its behalf
 *   permission to use and distribute the software in accordance with the
 *   terms specified in this license.
 */


#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
#include <X11/extensions/Xrender.h>
#endif  /* HAVE_X11_EXTENSIONS_XRENDER_H */
#ifdef HAVE_X11_EXTENSIONS_XCOMPOSITE_H
#include <X11/extensions/Xcomposite.h>
#endif  /* HAVE_X11_EXTENSIONS_XCOMPOSITE_H */
#ifdef HAVE_X11_EXTENSIONS_XSHM_H
#include <X11/extensions/XShm.h>
#include <X11/extensions/shmproto.h>
#endif  /* HAVE_X11_EXTENSIONS_XSHM_H */

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltHash.h"
#include "bltPicture.h"
#include "bltUnixPainter.h"
#include "bltBg.h"
#include "bltPainter.h"

typedef struct _Blt_Picture Pict;

#define CFRAC(i, n)     ((i) * 65535 / (n))
/* As for CFRAC, but apply exponent of g. */
#define CGFRAC(i, n, g) ((int)(65535 * pow((double)(i) / (n), (g))))

#define MAXIMAGESIZE(dpy)       (XMaxRequestSize(dpy) << 2) - 24

#define CLAMP(c)        ((((c) < 0.0) ? 0.0 : ((c) > 255.0) ? 255.0 : (c)))

#define DEBUG 0

static Blt_HashTable painterTable;
static int painterTableInitialized = 0;

#define COLOR_WINDOW            (1<<0)
#define BLACK_AND_WHITE         (1<<1)
#define MAP_COLORS              (1<<2)

/*
 * PainterKey --
 *
 *      This structure represents the key used to uniquely identify
 *      painters.  A painter is specified by a combination of display,
 *      visual, colormap, depth, and monitor gamma value.
 */
typedef struct {
    Display *display;                   /* Display of painter. Used to free
                                         * colors allocated. */
    Visual *visualPtr;                  /* Visual information for the class
                                         * of windows displaying the
                                         * image. */
    Colormap colormap;                  /* Colormap used.  This may be the
                                         * default colormap, or an
                                         * allocated private map. */
    int depth;                          /* Pixel depth of the display. */
    float gamma;                        /* Gamma correction value for the
                                         * monitor. */
} PainterKey;


#define GC_PRIVATE      1               /* Indicates if the GC in the
                                         * painter was shared (allocated by
                                         * Tk_GetGC) or private (by
                                         * XCreateGC). */

static Tcl_FreeProc FreePainter;

#ifdef notdef
static void
PrintFlags(unsigned int flags)
{
    if (flags & BLT_PIC_GREYSCALE) {
        fprintf(stderr, " greyscale");
    }
    if (flags & BLT_PIC_PREMULT_COLORS) {
        fprintf(stderr, " premultcolors");
    }
    if (flags & BLT_PIC_DIRTY) {
        fprintf(stderr, " dirty");
    }
    if (flags & BLT_PIC_COMPOSITE) {
         fprintf(stderr, " composite");
    }
    if (flags & BLT_PIC_MASK) {
         fprintf(stderr, " mask");
    }
    if (flags & BLT_PIC_UNINITIALIZED) {
         fprintf(stderr, " uninitialized");
    }
    if (flags & BLT_PAINTER_DITHER) {
         fprintf(stderr, " dither");
    }
    fprintf(stderr, "\n");
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * FindShift --
 *
 *      Returns the position of the least significant (low) bit in the
 *      given mask.
 *
 *      For TrueColor and DirectColor visuals, a pixel value is formed by
 *      OR-ing the red, green, and blue colormap indices into a single
 *      32-bit word.  The visual's color masks tell you where in the word
 *      the indices are supposed to be.  The masks contain bits only where
 *      the index is found.  By counting the leading zeros in the mask, we
 *      know how many bits to shift to the individual red, green, and blue
 *      values to form a pixel.
 *
 * Results:
 *      The number of the least significant bit.
 *
 *---------------------------------------------------------------------------
 */
static int
FindShift(unsigned int mask)            /* 32-bit word */
{
    int bit;

    for (bit = 0; bit < 32; bit++) {
        if (mask & (1 << bit)) {
            break;
        }
    }
    return bit;
}

/*
 *---------------------------------------------------------------------------
 *
 * CountBits --
 *
 *      Returns the number of bits set in the given 32-bit mask.
 *
 *          Reference: Graphics Gems Volume II.
 *      
 * Results:
 *      The number of bits to set in the mask.
 *
 *---------------------------------------------------------------------------
 */
static int
CountBits(unsigned long mask)           /* 32  1-bit tallies */
{
    /* 16  2-bit tallies */
    mask = (mask & 0x55555555) + ((mask >> 1) & (0x55555555));  
    /* 8  4-bit tallies */
    mask = (mask & 0x33333333) + ((mask >> 2) & (0x33333333)); 
    /* 4  8-bit tallies */
    mask = (mask & 0x07070707) + ((mask >> 4) & (0x07070707));  
    /* 2 16-bit tallies */
    mask = (mask & 0x000F000F) + ((mask >> 8) & (0x000F000F));  
    /* 1 32-bit tally */
    mask = (mask & 0x0000001F) + ((mask >> 16) & (0x0000001F));  
    return mask;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeGammaTables --
 *
 *      Initializes both the power and inverse power tables for the painter
 *      with a given gamma value.  These tables are used to/from map linear
 *      RGB values to/from non-linear monitor intensities.
 *      
 * Results:
 *      The *gammaTable* and *igammaTable* arrays are filled out to contain
 *      the mapped values.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeGammaTables(Painter *p)
{
    int i;
    double igamma, gamma;
    
    gamma = (double)p->gamma;
    igamma = 1.0 / gamma;
    for (i = 0; i < 256; i++) {
        double value, y;

        y = i / 255.0;
        value = pow(y, gamma) * 255.0 + 0.5;
        p->gammaTable[i] = (unsigned char)CLAMP(value);
        value = pow(y, igamma) * 255.0 + 0.5;
        p->igammaTable[i] = (unsigned char)CLAMP(value);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * QueryPalette --
 *
 *      Queries the X display server for the colors currently used in the
 *      colormap.  These values will then be used to map screen pixels back
 *      to RGB values (see Blt_DrawableToPicture). The queried non-linear
 *      color intensities are reverse mapped back to to linear RGB values.
 *      
 * Results:
 *      The *palette* array is filled in with the RGB color values of the
 *      colors allocated.
 *
 *---------------------------------------------------------------------------
 */
static void
QueryPalette(Painter *p, Blt_Pixel *palette)
{
    Visual *visualPtr;
    XColor colors[256];

    visualPtr = p->visualPtr;
    assert(visualPtr->map_entries <= 256);

    if ((visualPtr->class == DirectColor) || (visualPtr->class == TrueColor)) {
        XColor *cp, *cend;
        int numRed, numGreen, numBlue;
        unsigned int  r, g, b;
        
        r = g = b = 0;
        numRed =   (p->rMask >> p->rShift) + 1;
        numGreen = (p->gMask >> p->gShift) + 1;
        numBlue =  (p->bMask >> p->bShift) + 1;

        for (cp = colors, cend = cp + visualPtr->map_entries; cp < cend; cp++) {
            cp->pixel = ((r << p->rShift)|(g << p->gShift) | (b << p->bShift));
            cp->pad = 0;
            r++, b++, g++;
            if (r >= numRed) {
                r = 0;
            }
            if (g >= numGreen) {
                g = 0;
            }
            if (b >= numBlue) {
                b = 0;
            }
        }
    } else {
        XColor *cp;
        int i;

        for (cp = colors, i = 0; i < visualPtr->map_entries; i++, cp++) {
            cp->pixel = i;
            cp->pad = 0;
        }
    }

    XQueryColors(p->display, p->colormap, colors, visualPtr->map_entries);
    
    /* Scale to convert XColor component value (0..65535) to unsigned
     * char (0..255). */
    if (p->gamma == 1.0f) {
        Blt_Pixel *dp;
        XColor *cp;
        int i;
        double a;
        
        a = 1.0 / 257.0;
        cp = colors, dp = palette;
        for (i = 0; i < visualPtr->map_entries; i++) {
            dp->Red   = (unsigned char)(cp->red   * a + 0.5);
            dp->Green = (unsigned char)(cp->green * a + 0.5);
            dp->Blue  = (unsigned char)(cp->blue  * a + 0.5);
            cp++, dp++;
        }
    } else {
        Blt_Pixel *dp;
        XColor *cp;
        int i;
        double a;

        a = 1.0 / 257.0;
        cp = colors, dp = palette;
        for (i = 0; i < visualPtr->map_entries; i++) {
            dp->Red   = p->gammaTable[(int)(cp->red   * a + 0.5)];
            dp->Green = p->gammaTable[(int)(cp->green * a + 0.5)];
            dp->Blue  = p->gammaTable[(int)(cp->blue  * a + 0.5)];
            cp++, dp++;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorRamp --
 *
 *      Computes a smooth color ramp based upon the number of colors
 *      available for each color component.  It returns an array of the
 *      desired colors (XColor structures).  The screen gamma is factored
 *      into the desired colors.
 *      
 * Results:
 *      Returns the number of colors desired.  The *colors* array is filled
 *      out to contain the component values.
 *
 *---------------------------------------------------------------------------
 */
static int
ColorRamp(Painter *p, XColor *colors)
{
    int numColors;
    XColor *cp;
    double rScale, gScale, bScale;
    int i;

    numColors = 0;                      /* Suppress compiler warning. */

    /*
     * Calculate the RGB coordinates of the colors we want to allocate and
     * store them in *colors.
     */
    rScale = 255.0 / (p->numRed - 1);
    gScale = 255.0 / (p->numGreen - 1);
    bScale = 255.0 / (p->numBlue - 1);

    switch (p->visualPtr->class) {
    case TrueColor:
    case DirectColor:
        
        numColors = MAX3(p->numRed, p->numGreen, p->numBlue);
        if (p->isMonochrome) {
            numColors = p->numBlue = p->numGreen = p->numRed;
        } 

        /* Compute the 16-bit RGB values from each possible 8-bit value. */
        cp = colors;
        for (i = 0; i < numColors; i++) {
            int r, g, b;
            
            r = (int)(i * rScale + 0.5);
            g = (int)(i * gScale + 0.5);
            b = (int)(i * bScale + 0.5);

            r = p->igammaTable[r];
            g = p->igammaTable[g];
            b = p->igammaTable[b];

            cp->red = (r << 8) + r;
            cp->green = (g << 8) + g;
            cp->blue = (b << 8) + b;
            cp++;
        }
        break;

    case PseudoColor:
    case StaticColor:
    case GrayScale:
    case StaticGray:

        numColors = (p->numRed * p->numGreen * p->numBlue);
        if (p->isMonochrome) {
            numColors = p->numRed;
        } 
        if (!p->isMonochrome) {
            XColor *cp;
            int i;
            
            cp = colors;
            for (i = 0; i < p->numRed; i++) {
                int j;
                unsigned char r;
                
                r = (unsigned char)(i * rScale + 0.5);
                r = p->igammaTable[r];
                for (j = 0; j < p->numGreen; j++) {
                    int k;
                    unsigned int g;

                    g = (unsigned char)(j * gScale + 0.5);
                    g = p->igammaTable[g];
                    for (k = 0; k < p->numBlue; k++) {
                        unsigned int b;

                        b = (unsigned char)(k * bScale + 0.5);
                        b = p->igammaTable[b];
                        cp->red = (r << 8) | r;
                        cp->green = (g << 8) | g;
                        cp->blue = (b << 8) | b;
                        cp++;
                    } 
                }
            }
        }
        break;

    default:                            /* Monochrome */
        {
            XColor *cp;
            double scale;
            int i;

            scale = 255.0 / (numColors - 1);

            cp = colors;
            for (i = 0; i < numColors; ++i) {
                int c;

                c = (int)(i * scale + 0.5);
                c = p->igammaTable[c];
                cp->red = cp->green = cp->blue = (c << 8) | c;
                cp++;
            }
        }
    } /* end switch */
    return numColors;
}       

/*
 *---------------------------------------------------------------------------
 *
 * AllocateColors --
 *
 *      Individually allocates each of the desired colors (as specified by
 *      the *colors* array).  If a color can't be allocated the desired
 *      colors allocated to that point as released, the number of component
 *      intensities is reduced, and 0 is returned.
 *
 *      For TrueColor visuals, we don't need to allocate colors at all,
 *      since we can compute them directly.
 *      
 * Results:
 *      Returns 1 if all desired colors were allocated successfully.  If
 *      unsuccessful, returns 0.  All colors allocated up to that point are
 *      freed and a smaller color palette size is computed and reset in the
 *      painter structure.
 *
 *---------------------------------------------------------------------------
 */
static int 
AllocateColors(Painter *p, XColor *colors, int numColors)
{
    if (p->visualPtr->class == TrueColor) {
        XColor *cp, *cend;

        /* 
         * For TrueColor visuals, don't call XAllocColor, compute the pixel
         * value directly.
         */
        for (cp = colors, cend = cp + numColors; cp < cend; cp++) {
            unsigned int r, g, b;

            r = ((cp->red >> 8) >> p->rAdjust);
            g = ((cp->green >> 8) >> p->gAdjust);
            b = ((cp->blue >> 8) >> p->bAdjust);

            /* Shift each color into the proper location of the pixel
             * index. */
            r = (r << p->rShift) & p->rMask;
            g = (g << p->gShift) & p->gMask;
            b = (b << p->bShift) & p->bMask;
            cp->pixel = (r | g | b);
        }
        p->numPixels = 0;               /* This will indicate that we
                                         * didn't use XAllocColor to obtain
                                         * pixel values. */
        return TRUE;
    } else {
        int i;
        XColor *cp;

        cp = colors;
        for (i = 0; i < numColors; i++) {
            if (!XAllocColor(p->display, p->colormap, cp)){
                break;
            }
            p->pixels[i] = cp->pixel;
            cp++;
        }
        p->numPixels = i;               /* # of pixels in array */
        if (i == numColors) {
            return TRUE;                /* Success. */
        }
    }
    /*
     * If we didn't get all of the colors, free the current palette, reduce
     * the palette RGB component sizes.
     */
#ifdef notdef
    fprintf(stderr, "can't allocate %d/%d/%d colors\n", p->numRed, p->numGreen, 
        p->numBlue);
#endif
    XFreeColors(p->display, p->colormap, p->pixels, p->numPixels, 0);
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * FillPalette --
 *
 *      Base upon the colors allocated, generate two mappings from the
 *      picture's 8-bit RGB components.
 *
 *      1) Map 8-bit RGB values to the bits of the pixel.  Each component
 *         contains a portion of the pixel value.  For mapped visuals
 *         (pseudocolor, staticcolor, grayscale, and staticgray) this pixel
 *         value will be translated to the actual pixel used by the
 *         display.
 *
 *      2) Map 8-bit RGB values to the actual color values used.  The color
 *         ramp generated may be only a subset of the possible color
 *         values.  The resulting palette is used in dithering the image,
 *         using the error between the desired picture RGB value and the
 *         actual value used.
 *
 * Results:
 *      Color palette and pixel maps are filled in.
 *
 *---------------------------------------------------------------------------
 */
static void
FillPalette(Painter *p, XColor *colors, int numColors) 
{
    p->numColors = numColors;
    if (!p->isMonochrome) {
        p->flags |= COLOR_WINDOW;
        
        if ((p->visualPtr->class != DirectColor) && 
            (p->visualPtr->class != TrueColor)) {
            p->flags |= MAP_COLORS;
        }
    }
    if (p->isMonochrome) {
        int i;
        
        for (i = 0; i < 256; i++) {
            int c;
            
            c = (i + 127) / 255;
            p->rBits[i] = colors[c].pixel;
            p->palette[i].Blue = p->palette[i].Green = p->palette[i].Red = 
                (unsigned char)(c * 255 + 0.5);
        } 
    } else {
        int i, rMult;
        double rScale, gScale, bScale;
        
        rMult = p->numGreen * p->numBlue;
        
        rScale = 255.0 / (p->numRed - 1);
        gScale = 255.0 / (p->numGreen - 1);
        bScale = 255.0 / (p->numBlue - 1);
        
        for (i = 0; i < 256; i++) {
            int r, g, b;
            
            r = (i * (p->numRed   - 1) + 127) / 255;
            g = (i * (p->numGreen - 1) + 127) / 255;
            b = (i * (p->numBlue  - 1) + 127) / 255;
            
            if ((p->visualPtr->class == DirectColor) || 
                (p->visualPtr->class == TrueColor)) {
                p->rBits[i] = colors[r].pixel & p->rMask;
                p->gBits[i] = colors[g].pixel & p->gMask;
                p->bBits[i] = colors[b].pixel & p->bMask;
            } else {
                p->rBits[i] = r * rMult;
                p->gBits[i] = g * p->numBlue;
                p->bBits[i] = b;
            }
            p->palette[i].Red   = (unsigned char)(r * rScale + 0.5);
            p->palette[i].Green = (unsigned char)(g * gScale + 0.5);
            p->palette[i].Blue  = (unsigned char)(b * bScale + 0.5);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * AllocatePalette --
 *
 *      This procedure allocates the colors required by a color table, and
 *      sets up the fields in the color table data structure which are used
 *      in dithering.
 *
 *      This routine essentially mimics what is done in tkImgPhoto.c.  It's
 *      purpose is to allocate exactly the same color ramp as the photo
 *      image. That way both image types can co-exist without fighting over
 *      available colors.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Colors are allocated from the X server.  The color palette and
 *      pixel indices are updated.
 *
 *---------------------------------------------------------------------------
 */
static void
AllocatePalette(Painter *p)             /* Pointer to the color table
                                         * requiring colors to be
                                         * allocated. */
{
    XColor colors[256];
    int numColors;
    static int stdPalettes[13][3] = {
     /* numRed, numGreen, numBlue */
        { 2,  2,  2  },                 /* 3 bits, 8 colors */
        { 2,  3,  2  },                 /* 4 bits, 12 colors */
        { 3,  4,  2  },                 /* 5 bits, 24 colors */
        { 4,  5,  3  },                 /* 6 bits, 60 colors */
        { 5,  6,  4  },                 /* 7 bits, 120 colors */ 
        { 7,  7,  4  },                 /* 8 bits, 198 colors */
        { 8,  10, 6  },                 /* 9 bits, 480 colors */
        { 10, 12, 8  },                 /* 10 bits, 960 colors */
        { 14, 15, 9  },                 /* 11 bits, 1890 colors */
        { 16, 20, 12 },                 /* 12 bits, 3840 colors */
        { 20, 24, 16 },                 /* 13 bits, 7680 colors */
        { 26, 30, 20 },                 /* 14 bits, 15600 colors */
        { 32, 32, 30 },                 /* 15 bits, 30720 colors */
    };

    p->isMonochrome = FALSE; 
    switch (p->visualPtr->class) {
    case TrueColor:
    case DirectColor:
        p->numRed =   1 << CountBits(p->rMask);
        p->numGreen = 1 << CountBits(p->gMask);
        p->numBlue =  1 << CountBits(p->bMask);
        break;

    case GrayScale:
    case StaticGray:
    case PseudoColor:
    case StaticColor:
        if (p->depth > 15) {
            p->numRed = p->numGreen = p->numBlue = 32;
        } else if (p->depth >= 3) {
            int *ip = stdPalettes[p->depth - 3];
            p->numRed =   ip[0];
            p->numGreen = ip[1];
            p->numBlue =  ip[2];
        }
        break;

    default:
        p->numGreen = p->numBlue = 0;
        p->numRed = 1 << p->depth;
        p->isMonochrome = TRUE;
        break;
    }

    /*
     * Each time around this loop, we reduce the number of colors we're
     * trying to allocate until we succeed in allocating all of the colors
     * we need.
     */
    for (;;) {
        /*
         * If we are using 1 bit/pixel, we don't need to allocate any
         * colors (we just use the foreground and background colors in the
         * GC).
         */
        if ((p->isMonochrome) && (p->numRed <= 2)) {
            p->flags |= BLACK_AND_WHITE;
            /* return; */
        }
        /*
         * Calculate the RGB values of a color ramp, given the some number
         * of red, green, blue intensities available.
         */
        numColors = ColorRamp(p, colors);

        /* Now try to allocate the colors we've calculated. */

        if (AllocateColors(p, colors, numColors)) {
            break;              /* Success. */
        }
        if (!p->isMonochrome) {
            if ((p->numRed == 2) && (p->numGreen == 2) && (p->numBlue == 2)) {
                break;
                /* Fall back to 1-bit monochrome display. */
                /* p->mono = TRUE; */
            } else {
                /*
                 * Reduce the number of shades of each primary to about 3/4
                 * of the previous value.  This will reduce the total
                 * number of colors required to less than half (27/64) the
                 * previous value for PseudoColor displays.
                 */
                p->numRed = (p->numRed * 3 + 2) / 4;
                p->numGreen = (p->numGreen * 3 + 2) / 4;
                p->numBlue = (p->numBlue * 3 + 2) / 4;
            }
        } else {
            p->numRed /= 2;
        }
    }
    FillPalette(p, colors, numColors);
}


/*
 *---------------------------------------------------------------------------
 *
 * NewPainter --
 *
 *      Creates a new painter to be used to paint pictures. Painters are
 *      keyed by the combination of display, colormap, visual, depth, and
 *      gamma value used.
 *
 * Results:
 *      A pointer to the new painter is returned.
 *
 * Side Effects:
 *      A color ramp is allocated (not true for TrueColor visuals).  Gamma
 *      tables are computed and filled.
 *
 *---------------------------------------------------------------------------
 */
static Painter *
NewPainter(PainterKey *keyPtr)
{
    Painter *p;
    
    p = Blt_AssertCalloc(1, sizeof(Painter));
    p->colormap = keyPtr->colormap;
    p->depth = keyPtr->depth;
    p->display = keyPtr->display;
    p->gamma = keyPtr->gamma;
    p->visualPtr = keyPtr->visualPtr;
    
    p->refCount = 0;
    p->rMask = (unsigned int)p->visualPtr->red_mask;
    p->gMask = (unsigned int)p->visualPtr->green_mask;
    p->bMask = (unsigned int)p->visualPtr->blue_mask;

    p->rShift = FindShift(p->rMask);
    p->gShift = FindShift(p->gMask);
    p->bShift = FindShift(p->bMask);

    p->rAdjust = p->gAdjust = p->bAdjust = 0;

    {
        int numRedBits, numGreenBits, numBlueBits;

        numRedBits = CountBits(p->rMask);
        numGreenBits = CountBits(p->gMask);
        numBlueBits = CountBits(p->bMask);
        if (numRedBits < 8) {
            p->rAdjust = 8 - numRedBits;
        }
        if (numGreenBits < 8) {
            p->gAdjust = 8 - numGreenBits;
        }
        if (numBlueBits < 8) {
            p->bAdjust = 8 - numBlueBits;
        }
    }
    ComputeGammaTables(p);
    AllocatePalette(p);
    return p;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreePainter --
 *
 *      Called when the TCL interpreter is idle, this routine frees the
 *      painter. Painters are reference counted. Only when no clients are
 *      using the painter (the count is zero) is the painter actually
 *      freed.  By deferring its deletion, this allows client code to call
 *      Blt_GetPainter after Blt_FreePainter without incurring a
 *      performance penalty.
 *
 *---------------------------------------------------------------------------
 */
static void
FreePainter(DestroyData data)
{
    Painter *p = (Painter *)data;

    if (p->refCount <= 0) {
        if (p->numColors > 0) {
            XFreeColors(p->display, p->colormap, p->pixels, p->numPixels, 0);
        }
        Blt_DeleteHashEntry(&painterTable, p->hashPtr);
        if (p->gc != NULL) {
            if (p->flags & GC_PRIVATE) {
                XFreeGC(p->display, p->gc);
            } else {
                Tk_FreeGC(p->display, p->gc);
            }
            p->gc = NULL;
        }
        Blt_Free(p);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetPainter --
 *
 *      Attempts to retrieve a painter for a particular combination of
 *      display, colormap, visual, depth, and gamma value.  If no specific
 *      painter exists, then one is created.
 *
 * Results:
 *      A pointer to the new painter is returned.
 *
 * Side Effects:
 *      If no current painter exists, a new painter is added to the hash
 *      table of painters.  Otherwise, the current painter's reference
 *      count is incremented indicated how many clients are using the
 *      painter.
 *
 *---------------------------------------------------------------------------
 */
static Painter *
GetPainter(Display *display, Colormap colormap, Visual *visualPtr, int depth,
           float gamma)
{
    Painter *p;
    PainterKey key;
    int isNew;
    Blt_HashEntry *hPtr;

    if (!painterTableInitialized) {
        Blt_InitHashTable(&painterTable, sizeof(PainterKey) / sizeof(int));
        painterTableInitialized = TRUE;
    }
    key.display = display;
    key.colormap = colormap;
    key.visualPtr = visualPtr;
    key.depth = depth;
    key.gamma = gamma;

    hPtr = Blt_CreateHashEntry(&painterTable, (char *)&key, &isNew);
    if (isNew) {
        p = NewPainter(&key);
        p->hashPtr = hPtr;
        Blt_SetHashValue(hPtr, p);
    } else {
        p = Blt_GetHashValue(hPtr);
    }
    p->refCount++;
    return p;
}


/*
 *---------------------------------------------------------------------------
 *
 * PaintXImage --
 *
 *      Draw the given XImage. If the size of the image exceeds the maximum
 *      request size of the X11 protocol, the image is drawn using
 *      XPutImage in multiples of rows that fit within the limit.
 *
 *---------------------------------------------------------------------------
 */
static void
PaintXImage(Painter *p, Drawable drawable, XImage *imgPtr, int sx, int sy,
            int w, int h, int dx, int dy)
{
    int y;
    int n;
    long maxPixels;

    maxPixels = Blt_MaxRequestSize(p->display, sizeof(Blt_Pixel));
    n = (maxPixels + w - 1) / w;
    if (n < 1) {
        n = 1;
    } 
    if (n > h ) {
        n = h;
    }
#ifdef notdef
    if (n > 90) {
        n = 90;
    }
#endif
    for (y = 0; y < h; y += n) {
        if ((y + n) > h) {
            n = h - y;
        }
        XPutImage(p->display, drawable, p->gc, imgPtr, sx, sy+y, 
                  dx, dy+y, w, n);
    }
}

static void
PictureToXImage(Painter *p, Pict *srcPtr, int sx, int sy, int w, int h,
                XImage *imgPtr)
{
    Blt_Pixel *srcRowPtr;
    int dw, dh;
    int y;
    unsigned char *destRowPtr;

#ifdef WORD_BIGENDIAN
    static int nativeByteOrder = MSBFirst;
#else
    static int nativeByteOrder = LSBFirst;
#endif  /* WORD_BIGENDIAN */

    /* 
     * Set the byte order to the platform's native byte order. We'll let
     * Xlib handle byte swapping.
     */
    imgPtr->byte_order = nativeByteOrder;
    srcRowPtr = srcPtr->bits + ((sy * srcPtr->pixelsPerRow) + sx);
    destRowPtr = (unsigned char *)imgPtr->data;
    
    dw = MIN(w, srcPtr->width);
    dh = MIN(h, srcPtr->height);
    switch (p->visualPtr->class) {
    case TrueColor:

        /* Directly compute the pixel 8, 16, 24, or 32 bit values from the
         * RGB components. */
        
        switch (imgPtr->bits_per_pixel) {
        case 32:
            for (y = 0; y < dh; y++) {
                Blt_Pixel *sp, *send;
                unsigned int *dp;

                dp = (unsigned int *)destRowPtr;
                for (sp = srcRowPtr, send = sp + dw; sp < send; sp++) {
                    unsigned int r, g, b;

                    r = (p->igammaTable[sp->Red]   >> p->rAdjust) << p->rShift;
                    g = (p->igammaTable[sp->Green] >> p->gAdjust) << p->gShift;
                    b = (p->igammaTable[sp->Blue]  >> p->bAdjust) << p->bShift;
                    *dp = r | g | b;
                    dp++;
                }
                destRowPtr += imgPtr->bytes_per_line;
                srcRowPtr += srcPtr->pixelsPerRow;
            }
            break;
        case 24:
            for (y = 0; y < dh; y++) {
                Blt_Pixel *sp, *send;
                unsigned char *dp;

                dp = destRowPtr;
                for (sp = srcRowPtr, send = sp + dw; sp < send; sp++) {
                    unsigned long pixel;
                    unsigned int r, g, b;
                    
                    r = (p->igammaTable[sp->Red]   >> p->rAdjust) << p->rShift;
                    g = (p->igammaTable[sp->Green] >> p->gAdjust) << p->gShift;
                    b = (p->igammaTable[sp->Blue]  >> p->bAdjust) << p->bShift;
                    pixel = r | g | b;
                    
                    *dp++ = pixel & 0xFF;
                    *dp++ = (pixel >> 8) & 0xFF;
                    *dp++ = (pixel >> 16) & 0xFF;
                }
                destRowPtr += imgPtr->bytes_per_line;
                srcRowPtr += srcPtr->pixelsPerRow;
            }
            break;

        case 16:
            for (y = 0; y < dh; y++) {
                Blt_Pixel *sp, *send;
                unsigned short *dp;

                dp = (unsigned short *)destRowPtr;
                for (sp = srcRowPtr, send = sp + dw; sp < send; sp++) {
                    unsigned long pixel;
                    unsigned int r, g, b;
                    
                    r = (p->igammaTable[sp->Red]   >> p->rAdjust) << p->rShift;
                    g = (p->igammaTable[sp->Green] >> p->gAdjust) << p->gShift;
                    b = (p->igammaTable[sp->Blue]  >> p->bAdjust) << p->bShift;
                    pixel = r | g | b;
                    *dp = pixel;
                    dp++;
                }
                destRowPtr += imgPtr->bytes_per_line;
                srcRowPtr += srcPtr->pixelsPerRow;
            }
            break;

        case 8:
            for (y = 0; y < dh; y++) {
                Blt_Pixel *sp, *send;
                unsigned char *dp;

                dp = destRowPtr;
                for (sp = srcRowPtr, send = sp + dw; sp < send; sp++) {
                    unsigned long pixel;
                    unsigned int r, g, b;
                    
                    r = (p->igammaTable[sp->Red]   >> p->rAdjust) << p->rShift;
                    g = (p->igammaTable[sp->Green] >> p->gAdjust) << p->gShift;
                    b = (p->igammaTable[sp->Blue]  >> p->bAdjust) << p->bShift;

                    pixel = r | g | b;
                    *dp++ = pixel & 0xFF;
                }
                destRowPtr += imgPtr->bytes_per_line;
                srcRowPtr += srcPtr->pixelsPerRow;
            }
            break;
        }
        break;

    case DirectColor:

        /* Translate the RGB components to 8, 16, 24, or 32-bit pixel
         * values. */

        switch (imgPtr->bits_per_pixel) {
        case 32:
            for (y = 0; y < dh; y++) {
                Blt_Pixel *sp, *send;
                unsigned char *dp;
                
                dp = destRowPtr;
                for (sp = srcRowPtr, send = sp + dw; sp < send; sp++) {
                    unsigned long pixel;
                    
                    pixel = (p->rBits[sp->Red] | p->gBits[sp->Green] |
                             p->bBits[sp->Blue]);
                    *(unsigned int *)dp = pixel;
                    dp += 4;
                }
                destRowPtr += imgPtr->bytes_per_line;
                srcRowPtr += srcPtr->pixelsPerRow;
            }
            break;

        case 24:
            for (y = 0; y < dh; y++) {
                Blt_Pixel *sp, *send;
                unsigned char *dp;
                
                dp = destRowPtr;
                for (sp = srcRowPtr, send = sp + dw; sp < send; sp++) {
                    unsigned long pixel;
                    
                    pixel = (p->rBits[sp->Red] | p->gBits[sp->Green] |
                             p->bBits[sp->Blue]);
                    *dp++ = pixel & 0xFF;
                    *dp++ = (pixel >> 8) & 0xFF;
                    *dp++ = (pixel >> 16) & 0xFF;
                }
                destRowPtr += imgPtr->bytes_per_line;
                srcRowPtr += srcPtr->pixelsPerRow;
            }
            break;

        case 16:
            for (y = 0; y < dh; y++) {
                Blt_Pixel *sp, *send;
                unsigned char *dp;
                
                dp = destRowPtr;
                for (sp = srcRowPtr, send = sp + dw; sp < send; sp++) {
                    unsigned long pixel;
                    
                    pixel = (p->rBits[sp->Red] | p->gBits[sp->Green] |
                             p->bBits[sp->Blue]);
                    *(unsigned short *)dp = pixel;
                    dp += 2;
                }
                destRowPtr += imgPtr->bytes_per_line;
                srcRowPtr += srcPtr->pixelsPerRow;
            }
            break;

        case 8:
            for (y = 0; y < dh; y++) {
                Blt_Pixel *sp, *send;
                unsigned char *dp;
                
                dp = destRowPtr;
                for (sp = srcRowPtr, send = sp + dw; sp < send; sp++) {
                    unsigned long pixel;
                    
                    pixel = (p->rBits[sp->Red] | p->gBits[sp->Green] |
                             p->bBits[sp->Blue]);
                    *dp++ = pixel & 0xFF;
                }
                break;
            }
            destRowPtr += imgPtr->bytes_per_line;
            srcRowPtr += srcPtr->pixelsPerRow;
        }
        break;

    case PseudoColor:
    case StaticColor:
    case GrayScale:
    case StaticGray:

        /* Translate RGB components to the correct 8-bit or 4-bit
         * pixel values. */

        if (imgPtr->bits_per_pixel == 8) {
            for (y = 0; y < dh; y++) {
                Blt_Pixel *sp, *send;
                unsigned char *dp;

                dp = destRowPtr;
                for (sp = srcRowPtr, send = sp + dw; sp < send; sp++) {
                    unsigned long pixel;

                    pixel = (p->rBits[sp->Red] + p->gBits[sp->Green] +
                             p->bBits[sp->Blue]);
                    pixel = p->pixels[pixel];
                    *dp++ = (pixel & 0xFF);
                }
                destRowPtr += imgPtr->bytes_per_line;
                srcRowPtr += srcPtr->pixelsPerRow;
            }
        } else {
            for (y = 0; y < dh; y++) {
                Blt_Pixel *sp;
                int x;
                unsigned char *dp;

                dp = destRowPtr, sp = srcRowPtr;
                for (x = 0; x < dw; x++, sp++) {
                    unsigned long pixel;

                    pixel = (p->rBits[sp->Red] + p->gBits[sp->Green] +
                             p->bBits[sp->Blue]);
                    pixel = p->pixels[pixel];
                    if (x & 1) {        
                        *dp |= (pixel & 0x0F) << 4;
                        /* Move to the next address after odd nybbles. */
                        dp++;
                    } else {
                        *dp = (pixel & 0x0F);
                    }
                }
                destRowPtr += imgPtr->bytes_per_line;
                srcRowPtr += srcPtr->pixelsPerRow;
            }
        }
        break;

    default:
        Blt_Panic("unknown visual class");
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * XImageToPicture --
 *
 *      Converts an XImage to a picture image. 
 *
 * Results:
 *      Returns a pointer to the picture if successful. Otherwise NULL is
 *      returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Picture
XImageToPicture(Painter *p, XImage *imgPtr)
{
    Blt_Pixel *destRowPtr;
    Blt_Pixel palette[256];
    Pict *destPtr;
    int shift[4];
    unsigned char *srcRowPtr;
    int x, y;
    
    /* Allocate a picture to hold the screen snapshot. */
    destPtr = Blt_CreatePicture(imgPtr->width, imgPtr->height);

    /* Get the palette of the current painter/window */
    QueryPalette(p, palette);

    /* Suppress compiler warnings. */
    shift[0] = shift[1] = shift[2] = shift[3] = 0; 

    switch (p->visualPtr->class) {
    case TrueColor:
    case DirectColor:
        if (imgPtr->byte_order == MSBFirst) {
            shift[0] = 24, shift[1] = 16, shift[2] = 8, shift[3] = 0;
        } else {
            switch (imgPtr->bits_per_pixel) {
            case 32:
                shift[0] = 0, shift[1] = 8, shift[2] = 16, shift[3] = 24;
                break;
            case 24:
                shift[1] = 0, shift[2] = 8, shift[3] = 16;
                break;
            case 16:
                shift[2] = 0, shift[3] = 8;
                break;
            case 8:
                shift[3] = 0;
                break;
            }
        }
        srcRowPtr = (unsigned char *)imgPtr->data;
        destRowPtr = destPtr->bits; 
        switch (imgPtr->bits_per_pixel) {
        case 32:
            for (y = 0; y < imgPtr->height; y++) {
                unsigned char *sp;
                Blt_Pixel *dp, *dend;
                
                sp = srcRowPtr;
                for (dp = destRowPtr, dend = dp + imgPtr->width; dp < dend;
                     dp++) {
                    int r, g, b;
                    unsigned long pixel;
                    
                    /* Get the next pixel from the image. */
                    pixel = ((sp[0] << shift[0]) | (sp[1] << shift[1]) |
                             (sp[2] << shift[2]) | (sp[3] << shift[3]));
                    
                    /* Convert the pixel to RGB, correcting for input
                     * gamma. */
                    r = ((pixel & p->rMask) >> p->rShift);
                    g = ((pixel & p->gMask) >> p->gShift);
                    b = ((pixel & p->bMask) >> p->bShift);
                    dp->Red = palette[r].Red;
                    dp->Green = palette[g].Green;
                    dp->Blue = palette[b].Blue;
                    dp->Alpha = ALPHA_OPAQUE;
                    sp += 4;
                }
                destRowPtr += destPtr->pixelsPerRow;
                srcRowPtr += imgPtr->bytes_per_line;
            }
            break;

        case 24:
            for (y = 0; y < imgPtr->height; y++) {
                unsigned char *sp;
                Blt_Pixel *dp, *dend;
                
                sp = srcRowPtr;
                for (dp = destRowPtr, dend = dp + imgPtr->width; dp < dend;
                     dp++) {
                    int r, g, b;
                    unsigned long pixel;
                    
                    /* Get the next pixel from the image. */
                    pixel = ((sp[0] << shift[1]) | (sp[1] << shift[2]) |
                             (sp[2] << shift[3]));
                    
                    /* Convert the pixel to RGB, correcting for input
                     * gamma. */
                    r = ((pixel & p->rMask) >> p->rShift);
                    g = ((pixel & p->gMask) >> p->gShift);
                    b = ((pixel & p->bMask) >> p->bShift);
                    dp->Red = palette[r].Red;
                    dp->Green = palette[g].Green;
                    dp->Blue = palette[b].Blue;
                    dp->Alpha = ALPHA_OPAQUE;
                    sp += 3;
                }
                destRowPtr += destPtr->pixelsPerRow;
                srcRowPtr += imgPtr->bytes_per_line;
            }
            break;
            
        case 16:
            for (y = 0; y < imgPtr->height; y++) {
                unsigned char *sp;
                Blt_Pixel *dp, *dend;
                
                sp = srcRowPtr;
                for (dp = destRowPtr, dend = dp + imgPtr->width; dp < dend;
                     dp++) {
                    int r, g, b;
                    unsigned long pixel;
                    
                    /* Get the next pixel from the image. */
                    pixel = ((sp[0] << shift[2]) | (sp[1] << shift[3]));

                    /* Convert the pixel to RGB, correcting for input
                     * gamma. */
                    r = ((pixel & p->rMask) >> p->rShift);
                    g = ((pixel & p->gMask) >> p->gShift);
                    b = ((pixel & p->bMask) >> p->bShift);
                    dp->Red = palette[r].Red;
                    dp->Green = palette[g].Green;
                    dp->Blue = palette[b].Blue;
                    dp->Alpha = ALPHA_OPAQUE;
                    sp += 2;
                }
                destRowPtr += destPtr->pixelsPerRow;
                srcRowPtr += imgPtr->bytes_per_line;
            }
            break;
            
        case 8:
            for (y = 0; y < imgPtr->height; y++) {
                unsigned char *sp;
                Blt_Pixel *dp, *dend;
                
                sp = srcRowPtr;
                for (dp = destRowPtr, dend = dp + imgPtr->width; dp < dend;
                     dp++) {
                    int r, g, b;
                    unsigned long pixel;
                    
                    /* Get the next pixel from the image. */
                    pixel = (*sp << shift[3]);

                    /* Convert the pixel to RGB, correcting for input
                     * gamma. */
                    r = ((pixel & p->rMask) >> p->rShift);
                    g = ((pixel & p->gMask) >> p->gShift);
                    b = ((pixel & p->bMask) >> p->bShift);
                    dp->Red = palette[r].Red;
                    dp->Green = palette[g].Green;
                    dp->Blue = palette[b].Blue;
                    dp->Alpha = ALPHA_OPAQUE;
                    sp++;
                }
                destRowPtr += destPtr->pixelsPerRow;
                srcRowPtr += imgPtr->bytes_per_line;
            }
            break;
        }
        break;

    case PseudoColor:
    case StaticColor:
    case GrayScale:
    case StaticGray:
        if ((imgPtr->bits_per_pixel != 8) && (imgPtr->bits_per_pixel != 4)) {
            return NULL;                /* Can only handle 4 or 8 bit
                                         * pixels. */
        }
        srcRowPtr = (unsigned char *)imgPtr->data;
        destRowPtr = destPtr->bits;
        for (y = 0; y < imgPtr->height; y++) {
            unsigned char *sp;
            Blt_Pixel *dp;

            sp = srcRowPtr, dp = destRowPtr;
            for (x = 0; x < imgPtr->width; x++) {
                unsigned long pixel;

                if (imgPtr->bits_per_pixel == 8) {
                    pixel = *sp++;
                } else {
                    if (x & 1) {        /* Odd: pixel is high nybble. */
                        pixel = (*sp & 0xF0) >> 4;
                        sp++;
                    } else {            /* Even: pixel is low nybble. */
                        pixel = (*sp & 0x0F);
                    }
                } 
                /* Convert the pixel to RGB, correcting for input gamma. */
                dp->Red = palette[pixel].Red;
                dp->Green = palette[pixel].Green;
                dp->Blue = palette[pixel].Blue;
                dp->Alpha = ALPHA_OPAQUE;
                dp++;
            }
            srcRowPtr += imgPtr->bytes_per_line;
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
    default:
        break;
    }
    /* Opaque image, set associate colors flag.  */
    destPtr->flags |= BLT_PIC_PREMULT_COLORS;
    return destPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * PainterErrorProc --
 *
 *      Error handling routine for the XGetImage request below. Sets the
 *      flag passed via *clientData* to TCL_ERROR indicating an error
 *      occurred.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
PainterErrorProc(ClientData clientData, XErrorEvent *errEventPtr) 
{
    int *errorPtr = clientData;

    *errorPtr = TCL_ERROR;
    return 0;
}

#ifdef HAVE_XSHMQUERYEXTENSION

/*
 *---------------------------------------------------------------------------
 *
 * SnapPictureWithXShm --
 *
 *      Attempts to snap the image from the drawable into an XImage
 *      structure (using XShmGetImage).  This may fail if the coordinates
 *      of the region in the drawable are obscured.
 *
 * Results:
 *      Returns a pointer to the XImage if successful. Otherwise NULL is
 *      returned.
 *
 *---------------------------------------------------------------------------
 */
static int
SnapPictureWithXShm(Painter *p, Drawable drawable, int x, int y, int w, int h,
                     Blt_Picture *picturePtr)
{
    XImage *imgPtr;
    int code;
    Tk_ErrorHandler handler;
    XShmSegmentInfo xssi;

    code = TCL_OK;
    if (p->flags & PAINTER_DONT_USE_SHM) {
        return FALSE;
    }
    if (!XShmQueryExtension(p->display)) {
        p->flags |= PAINTER_DONT_USE_SHM;
        return FALSE;
    }

    imgPtr = XShmCreateImage(p->display, p->visualPtr, p->depth, ZPixmap,
                             NULL, &xssi, w, h); 

    xssi.shmid = shmget(IPC_PRIVATE,
                        imgPtr->bytes_per_line * imgPtr->height,
                        IPC_CREAT|0777);
    if (xssi.shmid == -1) {
        Blt_Warn("shmget: %s\n", strerror(errno));
        return FALSE;
    }

    xssi.shmaddr = imgPtr->data = shmat(xssi.shmid, NULL, 0);
    if ((xssi.shmaddr == (void *)-1) ||  (xssi.shmaddr == NULL)) {
        Blt_Warn("shmat: %s\n", strerror(errno));
        return FALSE;
    }
    handler = Tk_CreateErrorHandler(p->display, -1, -1, X_ShmAttach, 
                                    PainterErrorProc, &code);
    xssi.readOnly = False;
    XShmAttach(p->display, &xssi);
    XSync(p->display, False);
    Tk_DeleteErrorHandler(handler);
    if (code != TCL_OK) {
        shmdt(xssi.shmaddr);
        shmctl (xssi.shmid, IPC_RMID, 0);
        p->flags |= PAINTER_DONT_USE_SHM;
        return FALSE;
    }
    handler = Tk_CreateErrorHandler(p->display, -1, -1, X_ShmGetImage, 
                                    PainterErrorProc, &code);
    XShmGetImage(p->display, drawable, imgPtr, x, y, AllPlanes);
    XSync(p->display, False);
    Tk_DeleteErrorHandler(handler);
    if ((imgPtr == NULL) || (code != TCL_OK)) {
        if (imgPtr != NULL) {
            XDestroyImage(imgPtr);
        }
        return FALSE;
    }
    *picturePtr = XImageToPicture(p, imgPtr);
    XShmDetach(p->display, &xssi);
    shmdt(xssi.shmaddr);
    shmctl (xssi.shmid, IPC_RMID, 0);
    XDestroyImage(imgPtr);
    return TRUE;
}
#endif  /* HAVE_XSHMQUERYEXTENSION */

/*
 *---------------------------------------------------------------------------
 *
 * SnapPicture --
 *
 *      Attempts to snap the image from the drawable into an XImage
 *      structure (using XGetImage).  This may fail is the coordinates of
 *      the region in the drawable are obscured.
 *
 * Results:
 *      Returns a pointer to the XImage if successful. Otherwise NULL is
 *      returned.
 *
 *---------------------------------------------------------------------------
 */
static int
SnapPicture(Painter *p, Drawable drawable, int x, int y, int w, int h,
             Blt_Picture *picturePtr)
{
    XImage *imgPtr;
    int code;
    Tk_ErrorHandler handler;

#ifdef HAVE_XSHMQUERYEXTENSION
    if (SnapPictureWithXShm(p, drawable, x, y, w, h, picturePtr)) {
        return (*picturePtr != NULL);
    }
#endif  /* HAVE_XSHMQUERYEXTENSION */
    code = TCL_OK;
    handler = Tk_CreateErrorHandler(p->display, -1, X_GetImage, -1, 
                                    PainterErrorProc, &code);
    imgPtr = XGetImage(p->display, drawable, x, y, w, h, AllPlanes,
                       ZPixmap);
    XSync(p->display, False);
    Tk_DeleteErrorHandler(handler);
    if ((imgPtr == NULL) || (code != TCL_OK)) {
        if (imgPtr != NULL) {
            XDestroyImage(imgPtr);
        }
        return FALSE;
    }
    *picturePtr = XImageToPicture(p, imgPtr);
    XDestroyImage(imgPtr);
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawableToPicture --
 *
 *      Takes a snapshot of an X drawable (pixmap or window) and converts
 *      it to a picture.
 *
 * Results:
 *      Returns a picture of the drawable.  If an error occurred (a portion
 *      of the region specified is obscured), then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Picture
DrawableToPicture(
    Painter *p,
    Drawable drawable,
    int x, int y,                       /* Coordinates of region in source
                                         * drawable. */
    int w, int h)                       /* Dimension of the region in the
                                         * source drawable. Region must be
                                         * completely contained by the
                                         * drawable. */
{
    Pict *destPtr;

    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (!SnapPicture(p, drawable, x, y, w, h, &destPtr)) {
        int dw, dh;

        /* 
         * Failed to acquire an XImage from the drawable. The drawable may
         * be partially obscured (if it's a window) or too small for the
         * requested area.  Try it again, after fixing the area with the
         * dimensions of the drawable.
         */
        if (Blt_GetWindowRegion(p->display, drawable, NULL, NULL, &dw, &dh)
            == TCL_OK) {
            if ((x + w) > dw) {
                w = dw - x;
            }
            if ((y + h) > dh) {
                h = dh - y;
            }
            if (!SnapPicture(p, drawable, x, y, w, h, &destPtr)) {
                return NULL;
            }
        }
    }
    /* Opaque image, set associate colors flag.  */
    destPtr->flags |= BLT_PIC_PREMULT_COLORS;
    return destPtr;
}


#ifdef HAVE_XSHMQUERYEXTENSION

/*
 *---------------------------------------------------------------------------
 *
 * PaintPictureWithXShm --
 *
 *      Tries to paint the picture to the given drawable using the XShm
 *      extension.  Return 1 is successful and 0 otherwise.  The region of
 *      the picture is specified and the coordinates where in the
 *      destination drawable is the image to be displayed.
 *
 * Results:
 *      Returns TRUE is the picture was successfully displayed.  Otherwise
 *      FALSE is returned if the particular combination visual and image
 *      depth is not handled.
 *
 *---------------------------------------------------------------------------
 */
static int
PaintPictureWithXShm(
    Painter *p,
    Drawable drawable,
    Pict *srcPtr,
    int sx, int sy,                     /* Coordinates of region in the
                                         * picture. */
    int w, int h,                       /* Dimension of the source region.
                                         * Area cannot extend beyond the
                                         * end of the picture. */
    int dx, int dy)                     /* Coordinates of destination
                                         * region in the drawable.  */
{
    int code;
    Tk_ErrorHandler handler;
    XImage *imgPtr;
    XShmSegmentInfo xssi;

#ifdef notdef
    fprintf(stderr,
            "PaintPictureWithXShm: drawable=%x x=%d,y=%d,w=%d,h=%d,dx=%d,dy=%d flags=%x\n",
            drawable, sx, sy, w, h, dx, dy, srcPtr->flags);
#endif
    if (p->flags & PAINTER_DONT_USE_SHM) {
        return FALSE;
    }
    if (!XShmQueryExtension(p->display)) {
        p->flags |= PAINTER_DONT_USE_SHM;
        return FALSE;
    }
    code = TCL_OK;
    /* for the XShmPixmap */
    xssi.shmid = -1;
    xssi.shmaddr = (char *)-1;
    xssi.readOnly = False;
    
    imgPtr = XShmCreateImage(p->display, p->visualPtr, p->depth, ZPixmap,
                             (char *)NULL, &xssi, w, h);
    assert(imgPtr);
    xssi.shmid = shmget(IPC_PRIVATE,
                        imgPtr->bytes_per_line * imgPtr->height,
                        IPC_CREAT | 0777);
    if (xssi.shmid == -1) {
        Blt_Warn("shmget: %s\n", strerror(errno));
        return FALSE;
    }

    xssi.shmaddr = imgPtr->data = shmat(xssi.shmid, NULL, 0);
    if ((xssi.shmaddr == (void *)-1) ||  (xssi.shmaddr == NULL)) {
        Blt_Warn("shmat: %s\n", strerror(errno));
        return FALSE;
    }
    handler = Tk_CreateErrorHandler(p->display, -1, -1, X_ShmAttach, 
                                    PainterErrorProc, &code);
    XShmAttach(p->display, &xssi);
    XSync(p->display, False);
    Tk_DeleteErrorHandler(handler);
    if (code != TCL_OK) {
        shmdt(xssi.shmaddr);
        shmctl (xssi.shmid, IPC_RMID, 0);
        p->flags |= PAINTER_DONT_USE_SHM;
        return FALSE;
    }
    PictureToXImage(p, srcPtr, sx, sy, w, h, imgPtr);
    XShmPutImage(p->display, drawable, p->gc, imgPtr, 0, 0, dx, dy, w, h,
                 False /*send_event*/);
    XShmDetach(p->display, &xssi);
    shmdt(xssi.shmaddr);
    shmctl (xssi.shmid, IPC_RMID, 0);
    XSync(p->display, False);
    XDestroyImage(imgPtr);
    return TRUE;
}

#endif  /* HAVE_XSHMQUERYEXTENSION */

/*
 *---------------------------------------------------------------------------
 *
 * PaintPicture --
 *
 *      Paints the picture to the given drawable. The region of the picture
 *      is specified and the coordinates where in the destination drawable
 *      is the image to be displayed.
 *
 *      First tries to paint the picture using the XShmPutImage, if that
 *      fails, try XPutImage.
 *      The image may be dithered depending upon the bit set in the flags
 *      parameter: 0 no dithering, 1 for dithering.
 * 
 * Results:
 *      Returns TRUE is the picture was successfully displayed.  Otherwise
 *      FALSE is returned if the particular combination visual and image
 *      depth is not handled.
 *
 *---------------------------------------------------------------------------
 */
static int
PaintPicture(
    Painter *p,
    Drawable drawable,
    Pict *srcPtr,
    int sx, int sy,                     /* Coordinates of region in the
                                         * picture. */
    int w, int h,                       /* Dimension of the source region.
                                         * Area cannot extend beyond the
                                         * end of the picture. */
    int dx, int dy,                     /* Coordinates of destination
                                         * region in the drawable.  */
    unsigned int flags)
{
    Pict *ditherPtr;
    XImage *imgPtr;
    
#ifdef notdef
    fprintf(stderr,
            "PaintPicture: drawable=%x x=%d,y=%d,w=%d,h=%d,dx=%d,dy=%d flags=%x\n",
            drawable, sx, sy, w, h, dx, dy, srcPtr->flags);
#endif
    ditherPtr = NULL;
    if (flags & BLT_PAINTER_DITHER) {
        ditherPtr = Blt_DitherPicture(srcPtr, p->palette);
        if (ditherPtr != NULL) {
            srcPtr = ditherPtr;
        }
    }
#ifdef HAVE_XSHMQUERYEXTENSION
    if (PaintPictureWithXShm(p, drawable, srcPtr, sx, sy, w, h, dx, dy)) {
        if (ditherPtr != NULL) {
            Blt_FreePicture(ditherPtr);
        }
        return TRUE;
    }
#endif
    imgPtr = XCreateImage(p->display, p->visualPtr, p->depth, ZPixmap, 0, 
                          (char *)NULL, w, h, 32, 0);
    assert(imgPtr);
    imgPtr->data = Blt_AssertMalloc(sizeof(Blt_Pixel) * w * h);

    PictureToXImage(p, srcPtr, sx, sy, w, h, imgPtr);
    PaintXImage(p, drawable, imgPtr, 0, 0, w, h, dx, dy);
    XDestroyImage(imgPtr);
    if (ditherPtr != NULL) {
        Blt_FreePicture(ditherPtr);
    }
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * CompositePictureWithXRender --
 *
 *      Blends and paints the picture in the given drawable using
 *      XRenderComposite. This has the advantage of not requiring the
 *      background to be snapped from the drawable on the server,
 *      composited on the client, and then redisplayed on the server.  We
 *      also using a shared memory pixmap.  If XRender or XShm and not
 *      available or fail for some reason, return FALSE so that the
 *      calling routine can fall back to another compositing method.
 * 
 * Results:
 *      Returns TRUE is the picture was successfully displayed.  Otherwise
 *      FALSE is returned.  
 *
 *---------------------------------------------------------------------------
 */
static int
CompositePictureWithXRender(
    Painter *p,
    Drawable drawable,
    Pict *srcPtr,
    int sx, int sy,                     /* Coordinates of source region in
                                         * the picture. */
    int w, int h,                       /* Dimension of the source region.
                                         * Region cannot extend beyond the
                                         * end of the picture. */
    int dx, int dy)                     /* Coordinates of destination
                                         * region in the drawable.  */
{
   int code;
    Tk_ErrorHandler handler;
#ifdef HAVE_LIBXRENDER
#define FMT_ARGB32      (PictFormatType | PictFormatDepth | \
                        PictFormatRedMask | PictFormatRed | \
                        PictFormatGreenMask | PictFormatGreen | \
                        PictFormatBlueMask | PictFormatBlue | \
                        PictFormatAlphaMask | PictFormatAlpha)
#ifdef WORD_BIGENDIAN
    static int nativeByteOrder = MSBFirst;
#else
    static int nativeByteOrder = LSBFirst;
#endif  /* WORD_BIGENDIAN */
    XRenderPictureAttributes pa;
    Picture srcPict, dstPict;
    Blt_Pixel *srcRowPtr;
    Pixmap pixmap;
    XImage *imgPtr;
    int y;
    unsigned char *destRowPtr;
#ifdef HAVE_XSHMQUERYEXTENSION
    XShmSegmentInfo xssi;
#endif  /* HAVE_XSHMQUERYEXTENSION */
    XRenderPictFormat *pfPtr;
    Visual *visualPtr;
    int majorNum, minorNum;

#ifdef notdef
    fprintf(stderr, "CompositePictureWithXRender: "
            "drawable=%x x=%d,y=%d,w=%d,h=%d,dx=%d,dy=%d\n",
            drawable, sx, sy, w, h, dx, dy);
#endif
    code = TCL_OK;
    if (p->flags & (PAINTER_DONT_USE_SHM|PAINTER_NO_32BIT_VISUAL)) {
        return FALSE;
    }
    if (!XRenderQueryVersion(p->display, &majorNum, &minorNum)) {
        return FALSE;
    }
    if (!Blt_Picture_IsPremultiplied(srcPtr)) {
        Blt_PremultiplyColors(srcPtr);
    }
    pfPtr = XRenderFindStandardFormat(p->display, PictStandardARGB32);
    if (pfPtr == NULL) {
        XRenderPictFormat pf;

        fprintf(stderr, "Can't find standard format for ARGB32\n");
        
        /* lookup another ARGB32 picture format */
        pf.type = PictTypeDirect;
        pf.depth = 32;
        pf.direct.alphaMask = 0xff;
        pf.direct.redMask = 0xff;
        pf.direct.greenMask = 0xff;
        pf.direct.blueMask = 0xff;
        pf.direct.alpha = 24;
        pf.direct.red = 16;
        pf.direct.green = 8;
        pf.direct.blue = 0;
        pfPtr = XRenderFindFormat(p->display, FMT_ARGB32, &pf, 0);
    }
    visualPtr = NULL;
    if (pfPtr == NULL) {
        fprintf(stderr, "Can't find 32 bit picture format\n");
        return FALSE;
    } else {
        XVisualInfo *visuals, template;
        int numVisuals;
        int i;
        
        /* Try to lookup a RGB visual with depth 32. */
        numVisuals = 0;
        template.screen = DefaultScreen(p->display);
        template.depth = 32;
        template.bits_per_rgb = 8;
        
        visuals = XGetVisualInfo(p->display,
               (VisualScreenMask | VisualDepthMask | VisualBitsPerRGBMask),
                                 &template, &numVisuals);

        if (visuals == NULL) {
            fprintf(stderr, "No visual matching criteria\n");
            p->flags |= PAINTER_NO_32BIT_VISUAL;
            return FALSE;
        }

        for (i = 0; i < numVisuals; i++) {
            XRenderPictFormat *fmtPtr;
            
            fmtPtr = XRenderFindVisualFormat(p->display, visuals[i].visual);
            if ((fmtPtr != NULL) && (fmtPtr->id == pfPtr->id)) {
                visualPtr = visuals[i].visual;
                break;
            }
        }
        XFree(visuals);
    }                   /* pict_format_alpha != NULL */
    if (visualPtr == NULL) {
        fprintf(stderr, "Can't find matching visual.\n");
        p->flags |= PAINTER_NO_32BIT_VISUAL;
        return FALSE;
    }
#ifdef HAVE_XSHMQUERYEXTENSION
    if (!XShmQueryExtension(p->display)) {
        p->flags |= PAINTER_DONT_USE_SHM;
        return FALSE;
    }
                                        
    /* for the XShmPixmap */
    xssi.shmid = -1;
    xssi.shmaddr = (char *)-1;
    xssi.readOnly = False;
    
    imgPtr = XShmCreateImage(p->display, visualPtr, 32, ZPixmap,
                             (char *)NULL, &xssi, w, h);
    assert(imgPtr);
    xssi.shmid = shmget(IPC_PRIVATE, imgPtr->bytes_per_line * imgPtr->height,
                        IPC_CREAT | 0777);
    if (xssi.shmid == -1) {
        Blt_Warn("shmget: %s\n", strerror(errno));
        return FALSE;
    }

    xssi.shmaddr = imgPtr->data = shmat(xssi.shmid, NULL, 0);
    if ((xssi.shmaddr == (void *)-1) ||  (xssi.shmaddr == NULL)) {
        Blt_Warn("shmat: %s\n", strerror(errno));
        return FALSE;
    }
    handler = Tk_CreateErrorHandler(p->display, -1, -1, X_ShmAttach, 
                                    PainterErrorProc, &code);
    XShmAttach(p->display, &xssi);
    XSync(p->display, False);
    Tk_DeleteErrorHandler(handler);
    if (code != TCL_OK) {
        shmdt(xssi.shmaddr);
        shmctl (xssi.shmid, IPC_RMID, 0);
        p->flags |= PAINTER_DONT_USE_SHM;
        return FALSE;
    }
#else
    return FALSE;
#endif  /* HAVE_XSHMQUERYEXTENSION */
    imgPtr->byte_order = nativeByteOrder;
    srcRowPtr = srcPtr->bits + ((sy * srcPtr->pixelsPerRow) + sx);
    destRowPtr = (unsigned char *)imgPtr->data;
    
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *send;
        unsigned int *dp;
        
        dp = (unsigned int *)destRowPtr;
        for (sp = srcRowPtr, send = sp + w; sp < send; sp++) {
            unsigned int a, r, g, b;
            /* Format is ARGB */
            a = sp->Alpha << 24;
            r = p->igammaTable[sp->Red] << 16;
            g = p->igammaTable[sp->Green] << 8;
            b = p->igammaTable[sp->Blue];
            *dp = r | g | b | a;
            dp++;
        }
        destRowPtr += imgPtr->bytes_per_line;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    pixmap = XShmCreatePixmap(p->display, drawable, xssi.shmaddr, &xssi,
                              w, h, 32);
    pa.component_alpha = True;
    srcPict = XRenderCreatePicture(p->display, pixmap, pfPtr,
                                   CPComponentAlpha, &pa);
    pa.component_alpha = False;
    dstPict = XRenderCreatePicture(p->display, drawable,
                   XRenderFindStandardFormat(p->display, PictStandardRGB24),
                                   CPComponentAlpha, &pa);
    XRenderComposite(p->display, PictOpOver, srcPict, None, dstPict, 0, 0, 0, 0,
                     dx, dy, w, h);
    XShmDetach(p->display, &xssi);
    shmdt(xssi.shmaddr);
    XSync(p->display, False);
    XRenderFreePicture(p->display, srcPict);
    XRenderFreePicture(p->display, dstPict);
    XDestroyImage(imgPtr);
    return TRUE;
#else 
    return FALSE;
#endif /* HAVE_LIBXRENDER */
}

/*
 *---------------------------------------------------------------------------
 *
 * CompositePicture --
 *
 *      Blends and paints the picture in the given drawable. The region of
 *      the picture is specified and the coordinates where in the
 *      destination drawable is the image to be displayed.
 *
 *      The background is snapped from the drawable and converted into a
 *      picture.  This picture is then blended with the current picture
 *      (the background always assumed to be 100% opaque).
 * 
 * Results:
 *      Returns TRUE is the picture was successfully displayed.  Otherwise
 *      FALSE is returned.  This may happen if the background can not be
 *      obtained from the drawable.
 *
 *---------------------------------------------------------------------------
 */
static int
CompositePicture(
    Painter *p,
    Drawable drawable,
    Blt_Picture fg,
    int x, int y,                       /* Coordinates of source region in
                                         * the picture. */
    int w, int h,                       /* Dimension of the source region.
                                         * Region cannot extend beyond the
                                         * end of the picture. */
    int dx, int dy,                     /* Coordinates of destination
                                         * region in the drawable.  */
    unsigned int flags)
{
    int dw, dh;
#ifdef notdef
    fprintf(stderr, "CompositePicture: drawable=%x, x=%d,y=%d,w=%d,h=%d,dx=%d,dy=%d\n",
            drawable, x, y, w, h, dx, dy);
#endif
    if (dx < 0) {
        w += dx;                        /* Shrink the width. */
        x += -dx;                       /* Change the left of the source
                                         * region. */
        dx = 0;                         /* Start at the left of the
                                         * destination. */
    } 
    if (dy < 0) {
        h -= -dy;                       /* Shrink the height. */
        y += -dy;                       /* Change the top of the source
                                         * region. */
        dy = 0;                         /* Start at the top of the
                                         * destination. */
    }
    if ((w < 0) || (h < 0)) {
        return FALSE;
    }
    dw = MIN(w, Blt_Picture_Width(fg));
    dh = MIN(h, Blt_Picture_Height(fg));
    if (!CompositePictureWithXRender(p, drawable, fg, x, y, dw, dh, dx, dy)) {
        Pict *bgPtr;

#ifdef notdef
        fprintf(stderr, "CompositePictureWithXRender failed\n");
#endif
        bgPtr = DrawableToPicture(p, drawable, x, y, dw, dh);
        if (bgPtr == NULL) {
            return FALSE;
        }
        /* Dimension of source region may be adjusted by the actual size of the
         * drawable.  This is reflected in the size of the background
         * picture. */
        Blt_CompositeRegion(bgPtr, fg, x, y, bgPtr->width, bgPtr->height, 0, 0);
        PaintPicture(p, drawable, bgPtr, 0, 0, bgPtr->width, bgPtr->height,
                     dx, dy, flags);
        Blt_FreePicture(bgPtr);
    }
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DrawableToPicture --
 *
 *      Takes a snapshot of an X drawable (pixmap or window) and converts
 *      it to a picture.
 *
 * Results:
 *      Returns a picture of the drawable.  If an error occurred, NULL is
 *      returned.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture
Blt_DrawableToPicture(
    Tk_Window tkwin,
    Drawable drawable,
    int x, int y,                       /* Offset of image from the
                                         * drawable's origin. */
    int w, int h,                       /* Dimension of the image.  Image
                                         * must be completely contained by
                                         * the drawable. */
    float gamma)
{
    Blt_Painter painter;
    Blt_Picture picture;

    painter = Blt_GetPainter(tkwin, gamma);
    picture =  DrawableToPicture(painter, drawable, x, y, w, h);
    Blt_FreePainter(painter);
    return picture;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_WindowToPicture --
 *
 *      Takes a snapshot of an X drawable (pixmap or window) and converts
 *      it to a picture.
 *
 *      This routine is used to snap foreign (non-Tk) windows. For pixmaps
 *      and Tk windows, Blt_DrawableToPicture is preferred.
 *
 * Results:
 *      Returns a picture of the drawable.  If an error occurred, NULL is
 *      returned.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture
Blt_WindowToPicture(
    Display *display,
    Drawable drawable,
    int x, int y,                       /* Offset of image from the
                                         * drawable's origin. */
    int w, int h,                       /* Dimension of the image.  Image
                                         * must be completely contained by
                                         * the drawable. */
    float gamma)
{
    Blt_Painter painter;
    Blt_Picture picture;

    painter = Blt_GetPainterFromDrawable(display, drawable, gamma);
    picture =  DrawableToPicture(painter, drawable, x, y, w, h);
    Blt_FreePainter(painter);
    return picture;
}

int
Blt_PaintPicture(
    Blt_Painter painter,
    Drawable drawable,
    Blt_Picture picture,
    int x, int y,                       /* Starting coordinates of
                                         * subregion in the picture to be
                                         * painted. */
    int w, int h,                       /* Dimension of the subregion.  */
    int dx, int dy,                     /* Coordinates of region in the
                                         * drawable.  */
    unsigned int flags)
{
    int x1, y1, x2, y2;

#ifdef notdef
    fprintf(stderr, "Blt_PaintPicture: drawable=%x, picture=%x, x=%d,y=%d,w=%d,h=%d,dx=%d,dy=%d picture flags=%x, opaque=%x\n",
            drawable, picture, x, y, w, h, dx, dy, Blt_Picture_Flags(picture),
            Blt_Picture_IsOpaque(picture));
#endif
    /* 
     * Nothing to draw. The selected region is outside of the picture.
     *
     *   0,0
     *    +---------+
     *    |         |
     *    | Picture |
     *    |         |
     *    +---------+
     *              x,y
     *               +-------+
     *               |       |
     *               |       | h
     *               +-------+
     *                   w
     */
    x1 = x, y1 = y, x2 = x + w, y2 = y1 + h;
    if ((picture == NULL) || 
        (x1 >= Blt_Picture_Width(picture))  || (x2 <= 0) ||
        (y1 >= Blt_Picture_Height(picture)) || (y2 <= 0)) {
        return TRUE;    
    }
    if (dx < 0) {                       
        x1 -= dx;                       /* Add offset */
        dx = 0;
    } 
    if (dy < 0) {
        y1 -= dy;                       /* Add offset */
        dy = 0;
    }
    /* 
     * Correct the dimensions if the origin starts before the picture
     * (i.e. coordinate is negative).  Reset the coordinate to 0.
     *
     * x,y                     
     *   +---------+               0,0                 
     *   |  +------|--------+       +------+--------+
     * h |  |0,0   |        |       |      |        |
     *   |  |      |        |       |      |        |
     *   +--|------+        |       +------+        |
     *    w |               |       |               |
     *      |               |       |               |
     *      +---------------+       +---------------+ 
     *
     */
    if (x1 < 0) {               
        x2 += x1;
        x1 = 0;
    }
    if (y1 < 0) {
        y2 += y1;
        y1 = 0;
    }
    /* 
     * Check that the given area does not extend beyond the end of the
     * picture.
     * 
     *   0,0                        0,0
     *    +-----------------+        +-----------------+                
     *    |                 |        |                 |
     *    |        x,y      |        |        x,y      |                
     *    |         +---------+      |         +-------+
     *    |         |       | |      |         |       |
     *    |         |       | | w    |         |       |
     *    +---------|-------+ |      +---------+-------+
     *              +---------+                    
     *                   h
     *                                                    
     * Clip the end of the area if it's too big.
     */
    if ((x2 - x1) > Blt_Picture_Width(picture)) {
        x2 = x1 + Blt_Picture_Width(picture);
    }
    if ((y2 - y1) > Blt_Picture_Height(picture)) {
        y2 = y1 + Blt_Picture_Height(picture);
    }
    /* Check that there's still something to paint. */
    if (((x2 - x1) <= 0) || ((y2 - y1) <= 0)) {
        return TRUE;
    }
    if (Blt_Picture_IsOpaque(picture)) {
        return PaintPicture(painter, drawable, picture, x1, y1, x2 - x1, 
                            y2 - y1, dx, dy, flags);
    } else {
        return CompositePicture(painter, drawable, picture, x1, y1, x2 - x1,
                            y2 - y1, dx, dy, flags);
    }
}

int
Blt_PaintPictureWithBlend(
    Blt_Painter painter,
    Drawable drawable,
    Blt_Picture picture,
    int x, int y,                       /* Coordinates of region in the
                                         * picture. */
    int w, int h,                       /* Dimension of the region.  Area
                                         * cannot extend beyond the end of
                                         * the picture. */
    int dx, int dy,                     /* Coordinates of region in the
                                         * drawable.  */
    unsigned int flags)                 /* Indicates whether to dither the
                                         * picture before displaying. */
{
    int x1, y1, x2, y2;

    /* 
     * Nothing to draw. The selected region is outside of the picture.
     *
     *   0,0
     *    +---------+
     *    |         |
     *    | Picture |
     *    |         |
     *    +---------+
     *              x,y
     *               +-------+
     *               |       |
     *               |       | h
     *               +-------+
     *                   w
     */
    x1 = x, y1 = y, x2 = x + w, y2 = y1 + h;
    if ((picture == NULL) || 
        (x1 >= Blt_Picture_Width(picture))  || (x2 <= 0) ||
        (y1 >= Blt_Picture_Height(picture)) || (y2 <= 0)) {
        return TRUE;    
    }
    if (dx < 0) {                       
        x1 -= dx;
        dx = 0;                         /* Add offset */
    } 
    if (dy < 0) {
        y1 -= dy;                       /* Add offset */
        dy = 0;
    }
    /* 
     * Correct the dimensions if the origin starts before the picture
     * (i.e. coordinate is negative).  Reset the coordinate the 0.
     *
     * x,y                     
     *   +---------+               0,0                 
     *   |  +------|--------+       +------+--------+
     * h |  |0,0   |        |       |      |        |
     *   |  |      |        |       |      |        |
     *   +--|------+        |       +------+        |
     *    w |               |       |               |
     *      |               |       |               |
     *      +---------------+       +---------------+ 
     *
     */
    if (x1 < 0) {               
        x2 += x1;
        x1 = 0;
    }
    if (y1 < 0) {
        y2 += y2;
        y1 = 0;
    }
    /* 
     * Check that the given area does not extend beyond the end of the
     * picture.
     * 
     *   0,0                        0,0
     *    +-----------------+        +-----------------+                
     *    |                 |        |                 |
     *    |        x,y      |        |        x,y      |                
     *    |         +---------+      |         +-------+
     *    |         |       | |      |         |       |
     *    |         |       | | w    |         |       |
     *    +---------|-------+ |      +---------+-------+
     *              +---------+                    
     *                   h
     *                                                    
     * Clip the end of the area if it's too big.
     */
    if ((x2 - x1) > Blt_Picture_Width(picture)) {
        x2 = x1 + Blt_Picture_Width(picture);
    }
    if ((y2 - y1) > Blt_Picture_Height(picture)) {
        y2 = y1 + Blt_Picture_Height(picture);
    }
    /* Check that there's still something to paint. */
    if (((x2 - x1) <= 0) || ((y2 - y1) <= 0)) {
        return TRUE;
    }
    return CompositePicture(painter, drawable, picture, x1, y1, x2 - x1, 
                            y2 - y1, dx, dy, flags);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPainterFromDrawable --
 *
 *      Gets a painter for a particular combination of display, colormap,
 *      visual, depth, and gamma value.  This information is retrieved from
 *      the drawable which is assumed to be a window.
 *
 * Results:
 *      A pointer to the new painter is returned.
 *
 *---------------------------------------------------------------------------
 */
Painter *
Blt_GetPainterFromDrawable(Display *display, Drawable drawable, float gamma)
{
    XGCValues gcValues;
    unsigned long gcMask;
    Painter *p;

    {
        Blt_DrawableAttributes *attrPtr;

        attrPtr = Blt_GetDrawableAttributes(display, drawable);
        if ((attrPtr != NULL) && (attrPtr->visual != NULL)) {
            p = GetPainter(display, attrPtr->colormap, attrPtr->visual, 
                attrPtr->depth, gamma);
        } else {
            XWindowAttributes a;

            XGetWindowAttributes(display, drawable, &a);
            p = GetPainter(display, a.colormap, a.visual, a.depth, gamma);
        }
    }

    /*
     * Make a GC with background = black and foreground = white.
     */
    gcMask = GCGraphicsExposures;
    gcValues.graphics_exposures = False;
            
    p->gc = XCreateGC(display, drawable, gcMask, &gcValues);
    p->flags |= GC_PRIVATE;
    return p;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPainter --
 *
 *      Gets a painter for a particular combination of display, colormap,
 *      visual, depth, and gamma value.  This information (except for the
 *      monitor's gamma value) is retrieved from the given Tk window.
 *
 * Results:
 *      A pointer to the new painter is returned.
 *
 *---------------------------------------------------------------------------
 */
Painter *
Blt_GetPainter(Tk_Window tkwin, float gamma)
{
    Painter *p;
    XGCValues gcValues;
    unsigned long gcMask;

    p = GetPainter(Tk_Display(tkwin), Tk_Colormap(tkwin), 
        Tk_Visual(tkwin), Tk_Depth(tkwin), gamma);

    /*
     * Make a GC with background = black and foreground = white.
     */
    gcMask = GCGraphicsExposures;
    gcValues.graphics_exposures = False;
    p->gc = Tk_GetGC(tkwin, gcMask, &gcValues);
    p->flags &= ~GC_PRIVATE;
    return p;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FreePainter --
 *
 *      Frees the painter. Painters are reference counted. Only when no
 *      clients are using the painter (the count is zero) is the painter
 *      actually freed.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_FreePainter(Painter *p)
{
    p->refCount--;
    if (p->refCount <= 0) {
        Tcl_EventuallyFree(p, FreePainter);
    }
}

GC 
Blt_PainterGC(Painter *p)
{
    return p->gc;
}

int 
Blt_PainterDepth(Painter *p)
{
    return p->depth;
}

void
Blt_SetPainterClipRegion(Painter *p, TkRegion rgn)
{
    TkSetRegion(p->display, p->gc, rgn);
}

void
Blt_UnsetPainterClipRegion(Painter *p)
{
    XSetClipMask(p->display, p->gc, None);
}

#ifdef notdef

typedef struct {
    Pixmap pixmap;
    Display *display;
    int w, h;
    int depth;
#ifdef HAVE_XSHMQUERYEXTENSION
    XShmSegmentInfo xssi;
#endif  /* HAVE_XSHMQUERYEXTENSION */
} Blt_PixmapInfo;

static int pixmapTableInitialized = 0;
static Blt_HashTable pixmapTable;

Blt_PixmapInfo  *
Blt_GetPixmapInfo(Pixmap pixmap)
{
    if (!pixmapTableInitialized) {
        Blt_InitHashTable(&pixmapTable, BLT_ONE_WORD_KEYS);
        pixmapTableInitialized = TRUE;
    }
    hPtr = Blt_FindHashEntry(&pixmapTable, pixmap);
    if (hPtr == NULL) {
        return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

static void
AddPixmapInfoEntry(Display *display, Pixmap pixmap, int w, int h, int depth)
{
    int isNew;
    Blt_HashEntry *hPtr;
    Blt_PixmapInfo *pmiPtr;

    if (!pixmapTableInitialized) {
        Blt_InitHashTable(&pixmapTable, BLT_ONE_WORD_KEYS);
        pixmapTableInitialized = TRUE;
    }
    hPtr = Blt_CreateHashEntry(&pixmapTable, pixmap, &isNew);
    assert(isNew);
    pmiPtr = Blt_AssertMalloc(sizeof(Blt_PixmapInfo));
    pmiPtr->display = display;
    pmiPtr->pixmap = pixmap;
    pmiPtr->hashPtr = hPtr;
    pmiPtr->w = w;
    pmiPtr->h = h;
    pmiPtr->depth = depth;
#ifdef HAVE_XSHMQUERYEXTENSION
#endif  /* HAVE_XSHMQUERYEXTENSION */
    Blt_SetHashValue(hPtr, pmiPtr);
}

static void
DeletePixmapInfoEntry(Pixmap pixmap)
{
    Blt_HashEntry *hPtr;

    if (!pixmapTableInitialized) {
        Blt_InitHashTable(&pixmapTable, BLT_ONE_WORD_KEYS);
        pixmapTableInitialized = TRUE;
    }
    hPtr = Blt_FindHashEntry(&pixmapTable, pixmap);
    if (hPtr != NULL) {
        Blt_PixmapInfo *pmiPtr;

        pmiPtr = Blt_GetHashValue(hPtr);
        Blt_DeleteHashEntry(&pixmapTable, pmiPtr->hashPtr);
#ifdef HAVE_XSHMQUERYEXTENSION
#endif  /* HAVE_XSHMQUERYEXTENSION */
        Blt_Free(pmiPtr);
    }
}

Pixmap 
Blt_GetPixmapAbortOnError(Display *display, Drawable drawable, int w, int h, 
                          int depth, int lineNum, const char *fileName)
{
    if (w <= 0) {
        Blt_Warn("line %d of %s: width is %d\n", lineNum, fileName, w);
        abort();
    }
    if (h <= 0) {
        Blt_Warn("line %d of %s: height is %d\n", lineNum, fileName, h);
        abort();
    }
    pixmap = Tk_GetPixmap(display, drawable, w, h, depth);
#ifdef HAVE_XSHMQUERYEXTENSION
#endif  /* HAVE_XSHMQUERYEXTENSION */
    AddPixmapInfoEntry(display, pixmap, w, h, depth);
    return pixmap;
}

void
Blt_FreePixmap(Display *display, Pixmap pixmap) 
{
    DeletePixmapInfoEntry(pixmap);
#ifdef HAVE_XSHMQUERYEXTENSION
#endif  /* HAVE_XSHMQUERYEXTENSION */
    Tk_FreePixmap(display, pixmap);
}
#endif

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPainter.c --
 *
 * This module implements generic painting procedures for pictures in
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
#include "bltHash.h"
#include "bltPicture.h"
#include "bltPainterInt.h"

typedef struct _Blt_Picture Picture;

#define CFRAC(i, n)     ((i) * 65535 / (n))
/* As for CFRAC, but apply exponent of g. */
#define CGFRAC(i, n, g) ((int)(65535 * pow((double)(i) / (n), (g))))

#define MAXIMAGESIZE(dpy)       (XMaxRequestSize(dpy) << 2) - 24

#define CLAMP(c)        ((((c) < 0.0) ? 0.0 : ((c) > 255.0) ? 255.0 : (c)))


static Blt_HashTable painterTable;
static int initialized = 0;

#define COLOR_WINDOW            (1<<0)
#define BLACK_AND_WHITE         (1<<1)
#define MAP_COLORS              (1<<2)

/*
 * PainterKey --
 *
 * This structure represents the key used to uniquely identify painters.  A
 * painter is specified by a combination of display, visual, colormap, depth,
 * and monitor gamma value.
 */
typedef struct {
    Display *display;                   /* Display of painter. Used to free
                                         * colors allocated. */
    Visual *visualPtr;                  /* Visual information for the class of
                                         * windows displaying the image. */
    Colormap colormap;                  /* Colormap used.  This may be the
                                         * default colormap, or an allocated
                                         * private map. */
    int depth;                          /* Pixel depth of the display. */
    float gamma;                        /* Gamma correction value of
                                         * monitor. */
} PainterKey;


#define GC_PRIVATE      1               /* Indicates if the GC in the painter
                                         * was shared (allocated by Tk_GetGC)
                                         * or private (by XCreateGC). */

static Tcl_FreeProc FreePainter;

/*
 *---------------------------------------------------------------------------
 *
 * FindShift --
 *
 *      Returns the position of the least significant (low) bit in the given
 *      mask.
 *
 *      For TrueColor and DirectColor visuals, a pixel value is formed by
 *      OR-ing the red, green, and blue colormap indices into a single 32-bit
 *      word.  The visual's color masks tell you where in the word the indices
 *      are supposed to be.  The masks contain bits only where the index is
 *      found.  By counting the leading zeros in the mask, we know how many
 *      bits to shift to the individual red, green, and blue values to form a
 *      pixel.
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
 *      The *gammaTable* and *igammaTable* arrays are filled out to
 *      contain the mapped values.
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
 * Blt_QueryPalette --
 *
 *      Queries the X display server for the colors currently used in the
 *      colormap.  These values will then be used to map screen pixels back to
 *      RGB values (see Blt_DrawableToPicture). The queried non-linear color
 *      intensities are reverse mapped back to to linear RGB values.
 *      
 * Results:
 *      The *palette* array is filled in with the RGB color values of the
 *      colors allocated.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_QueryPalette(Painter *p, Blt_Pixel *palette)
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
            dp->Red =   (unsigned char)(cp->red * a + 0.5);
            dp->Green = (unsigned char)(cp->green * a + 0.5);
            dp->Blue =  (unsigned char)(cp->blue * a + 0.5);
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
            dp->Red =   p->gammaTable[(int)(cp->red * a + 0.5)];
            dp->Green = p->gammaTable[(int)(cp->green * a + 0.5)];
            dp->Blue =  p->gammaTable[(int)(cp->blue * a + 0.5)];
            cp++, dp++;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorRamp --
 *
 *      Computes a smooth color ramp based upon the number of colors available
 *      for each color component.  It returns an array of the desired colors
 *      (XColor structures).  The screen gamma is factored into the desired
 *      colors.
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
    double igamma;
    int i;

    numColors = 0;                      /* Suppress compiler warning. */

    /*
     * Calculate the RGB coordinates of the colors we want to allocate and
     * store them in *colors.
     */
    igamma = 1.0 / (double)p->gamma;

    rScale = 255.0 / (p->numRed - 1);
    gScale = 255.0 / (p->numGreen - 1);
    bScale = 255.0 / (p->numBlue - 1);

    switch (p->visualPtr->class) {
    case TrueColor:
    case DirectColor:
        
        nColors = MAX3(p->numRed, p->numGreen, p->numBlue);
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

        nColors = (p->numRed * p->numGreen * p->numBlue);
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

            scale = 255.0 / (nColors - 1);

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
 *      Individually allocates each of the desired colors (as specified by the
 *      *colors* array).  If a color can't be allocated the desired colors
 *      allocated to that point as released, the number of component
 *      intensities is reduced, and 0 is returned.
 *
 *      For TrueColor visuals, we don't need to allocate colors at all, since
 *      we can compute them directly.
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

            /* Shift each color into the proper location of the pixel index. */
            r = (r << p->rShift) & p->rMask;
            g = (g << p->gShift) & p->gMask;
            b = (b << p->bShift) & p->bMask;
            cp->pixel = (r | g | b);
        }
        p->nPixels = 0;                 /* This will indicate that we didn't
                                         * use XAllocColor to obtain pixel
                                         * values. */
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
        p->nPixels = i;                 /* # of pixels in array */
        if (i == numColors) {
            return TRUE;                /* Success. */
        }
    }
    /*
     * If we didn't get all of the colors, free the current palette, reduce
     * the palette RGB component sizes.
     */
#ifdef notdef
    Blt_Warn("can't allocate %d/%d/%d colors\n", p->numRed, p->numGreen, p->numBlue);
#endif
    XFreeColors(p->display, p->colormap, p->pixels, p->nPixels, 0);

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
 *         value will be translated to the actual pixel used by the display.
 *
 *      2) Map 8-bit RGB values to the actual color values used.  The
 *         color ramp generated may be only a subset of the possible
 *         color values.  The resulting palette is used in dithering the
 *         image, using the error between the desired picture RGB value
 *         and the actual value used.
 *
 * Results:
 *      Color palette and pixel maps are filled in.
 *
 *---------------------------------------------------------------------------
 */
static void
FillPalette(Painter *p, XColor *colors, int numColors) 
{
    p->nColors = numColors;
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
            p->palette[i].Red = (unsigned char)(r * rScale + 0.5);
            p->palette[i].Green = (unsigned char)(g * gScale + 0.5);
            p->palette[i].Blue = (unsigned char)(b * bScale + 0.5);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * AllocatePalette --
 *
 *      This procedure allocates the colors required by a color table, and
 *      sets up the fields in the color table data structure which are used in
 *      dithering.
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
 *      Colors are allocated from the X server.  The color palette and pixel
 *      indices are updated.
 *
 *---------------------------------------------------------------------------
 */
static void
AllocatePalette(Painter *p)            /* Pointer to the color table requiring
                                        * colors to be allocated. */
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
     * Each time around this loop, we reduce the number of colors we're trying
     * to allocate until we succeed in allocating all of the colors we need.
     */
    for (;;) {
        /*
         * If we are using 1 bit/pixel, we don't need to allocate any colors
         * (we just use the foreground and background colors in the GC).
         */
        if ((p->isMonochrome) && (p->numRed <= 2)) {
            p->flags |= BLACK_AND_WHITE;
            /* return; */
        }
        /*
         * Calculate the RGB values of a color ramp, given the some number of
         * red, green, blue intensities available.
         */
        nColors = ColorRamp(p, colors);

        /* Now try to allocate the colors we've calculated. */

        if (AllocateColors(p, colors, numColors)) {
            break;                      /* Success. */
        }
        if (!p->isMonochrome) {
            if ((p->numRed == 2) && (p->numGreen == 2) && (p->numBlue == 2)) {
                break;
                /* Fall back to 1-bit monochrome display. */
                /* p->mono = TRUE; */
            } else {
                /*
                 * Reduce the number of shades of each primary to about 3/4 of
                 * the previous value.  This will reduce the total number of
                 * colors required to less than half (27/64) the previous
                 * value for PseudoColor displays.
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
 *      Creates a new painter to be used to paint pictures. Painters are keyed
 *      by the combination of display, colormap, visual, depth, and gamma
 *      value used.
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

/*
 *---------------------------------------------------------------------------
 *
 * FreePainter --
 *
 *      Called when the TCL interpreter is idle, this routine frees the
 *      painter. Painters are reference counted. Only when no clients are
 *      using the painter (the count is zero) is the painter actually freed.
 *      By deferring its deletion, this allows client code to call
 *      Blt_GetPainter after Blt_FreePainter without incurring a performance
 *      penalty.
 *
 *---------------------------------------------------------------------------
 */
static void
FreePainter(DestroyData data)
{
    Painter *p = (Painter *)data;

    if (p->refCount <= 0) {
        if (p->nColors > 0) {
            XFreeColors(p->display, p->colormap,
                p->pixels, p->nPixels, 0);
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
 *      If no current painter exists, a new painter is added to the hash table
 *      of painters.  Otherwise, the current painter's reference count is
 *      incremented indicated how many clients are using the painter.
 *
 *---------------------------------------------------------------------------
 */
static Painter *
GetPainter(
    Display *display, 
    Colormap colormap, 
    Visual *visualPtr,
    int depth,
    float gamma)
{
    Painter *p;
    PainterKey key;
    int isNew;
    Blt_HashEntry *hPtr;

    if (!initialized) {
        Blt_InitHashTable(&painterTable, sizeof(PainterKey) / sizeof(int));
        initialized = TRUE;
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
    Visual *visual;
    Colormap colormap;
    int depth;


    Blt_GetDrawableInfo(display, drawable, &visual, &colormap, &depth);
    p = GetPainter(display, colormap, visual, depth, gamma);

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


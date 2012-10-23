
/*
 * bltMacOSXImage.c --
 *
 * This module implements MacOSX-specific image processing procedures
 * for the BLT toolkit.
 *
 *	Copyright 1997-2004 George A Howlett.
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
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"
#include "bltImage.h"
/* #include <X11/Xutil.h> */
#include "tkDisplay.h"

typedef struct _Blt_Picture _Picture;

#define Cursor MacOSX_Cursor
#define Picture MacOSX_Picture
#define TextStyle MacOSX_TextStyle
#include <Carbon/Carbon.h>
#undef Picture
#undef Cursor
#undef TextStyle

struct TkWindow;

struct _MacDrawable {
    TkWindow *winPtr;     	/* Ptr to tk window or NULL if Pixmap */
    CGrafPtr  grafPtr;
    ControlRef rootControl;
    int xOffset;		/* X offset from toplevel window. */
    int yOffset;	       	/* Y offset from toplevel window. */
    RgnHandle clipRgn;		/* Visable region of window. */
    RgnHandle aboveClipRgn;	/* Visable region of window and its
				 * children. */
    int referenceCount;		/* Don't delete toplevel until
				 * children are gone. */

    /* Pointer to the toplevel datastruct. */
    struct _MacDrawable *toplevel;	
    int flags;			/* Various state see defines below. */
};

typedef struct _MacDrawable *MacDrawable;


#define COLOR_WINDOW		(1<<0)
#define BLACK_AND_WHITE		(1<<1)
#define MAP_COLORS		(1<<2)

static Blt_HashTable painterTable;
static int initialized = 0;

/*
 * PainterKey --
 *
 * This structure represents the key used to uniquely identify
 * painters.  A painter is specified by a combination of display,
 * visual, colormap, depth, and monitor gamma value.
 */
typedef struct {
    Display *display;		/* Display of painter. Used to free
				 * colors allocated. */

    Visual *visualPtr;		/* Visual information for the class of
				 * windows displaying the image. */

    Colormap colormap;		/* Colormap used.  This may be the
				 * default colormap, or an allocated
				 * private map. */

    int depth;			/* Pixel depth of the display. */

    double gamma;		/* Gamma correction value of monitor. */

} PainterKey;


/*
 * Painter --
 *
 * This structure represents a painter used to display picture images.
 * A painter is specified by a combination of display, visual,
 * colormap, depth, and monitor gamma value.  Painters contain
 * information necessary to display a picture.  This includes both an
 * RGB to pixel map, and a RGB to allocated color map.
 *
 * Painters may be shared by more than one client and are reference
 * counted.  When no clients are using the painter, it is freed.
 */
typedef struct _Blt_Painter {
    Display *display;		/* Display of painter. Used to free
				 * colors allocated. */

    Visual *visualPtr;		/* Visual information for the class of
				 * windows displaying the image. */

    Colormap colormap;		/* Colormap used.  This may be the default
				 * colormap, or an allocated private map. */

    int depth;			/* Pixel depth of the display. */

    double gamma;		/* Gamma correction value of monitor. */

    unsigned int flags;		/* Flags listed below. */

    int refCount;		/* # of clients using this painter. If
				 * zero, the painter is freed. */

    Blt_HashEntry *hashPtr;	/* Used to delete the painter entry
				 * from the hash table of painters. */

    int numColors;		/* # of colors allocated.  */
    int numRed, numGreen, numBlue;	/* # of intensities for each RGB component. */

    unsigned long pixels[256];	/* Array of pixel values. Needed to
				 * deallocate the color palette. Also
				 * contains the mapping between linear
				 * pixel values (rBits, gBits, bBits)
				 * and the actual pixel for
				 * PsuedoColor, StaticColor,
				 * Greyscale, and StaticGrey visuals.
				 */

    int numPixels;		/* # of pixels allocated in above array. */


    GC gc;			/* GC used to draw the image. */

    /* 
     * The following arrays are used for DirectColor, PsuedoColor,
     * StaticColor, Greyscale, and StaticGrey visuals to convert RGB
     * triplets to a parts of a pixel index.
     */
    unsigned int rBits[256], gBits[256], bBits[256];

    /* 
     * This following as used for TrueColor and DirectColor visuals
     * only.  They are used to directly compute of pixel values from
     * picture RGB components. 
     */
    unsigned int rAdjust, gAdjust, bAdjust;
    unsigned int rShift, gShift, bShift;
    unsigned int rMask, gMask, bMask;

    unsigned char gammaTable[256]; /* Input gamma lookup table. Used to
				 * map non-linear monitor values back
				 * to RGB values. This is used
				 * whenever we take a snapshot of the
				 * screen (e.g. alpha blending). 
				 * Computes the power mapping.  
				 * D = I^gamma. */

    unsigned char igammaTable[256]; /* Output gamma lookup table. Used to
				 * map RGB values to non-linear
				 * monitor values. Computes the
				 * inverse power mapping. 
				 * I~ = D^1/gamma. */

    int isMonochrome;		/* Indicates if the display uses a single
				 * color component (e.g. 4-bit grayscale). */

    Blt_Pixel palette[256];	/* Maps the picture's 8-bit RGB values to the
				 * RGB values of the colors actually
				 * allocated. This is used for dithering the
				 * picture. */

} Painter;

#define GC_PRIVATE	1	/* Indicates if the GC in the painter
				 * was shared (allocated by Tk_GetGC)
				 * or private (by XCreateGC). */

static Tcl_FreeProc FreePainter;

typedef struct _Blt_Picture Picture;

#define GetBit(x, y) \
   srcBits[(srcBytesPerRow * (srcHeight - y - 1)) + (x>>3)] & (0x80 >> (x&7))
#define SetBit(x, y) \
   destBits[(destBytesPerRow * (destHeight - y - 1)) + (x>>3)] |= (0x80 >>(x&7))

/*
 *---------------------------------------------------------------------------
 *
 * FindShift --
 *
 *	Returns the position of the least significant (low) bit in
 *	the given mask.
 *
 *	For TrueColor and DirectColor visuals, a pixel value is
 *	formed by OR-ing the red, green, and blue colormap indices
 *	into a single 32-bit word.  The visual's color masks tell
 *	you where in the word the indices are supposed to be.  The
 *	masks contain bits only where the index is found.  By counting
 *	the leading zeros in the mask, we know how many bits to shift
 *	to the individual red, green, and blue values to form a pixel.
 *
 * Results:
 *      The number of the least significant bit.
 *
 *---------------------------------------------------------------------------
 */
static int
FindShift(unsigned int mask)	/* 32-bit word */
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
 *	Returns the number of bits set in the given 32-bit mask.
 *
 *	    Reference: Graphics Gems Volume II.
 *	
 * Results:
 *      The number of bits to set in the mask.
 *
 *
 *---------------------------------------------------------------------------
 */
static int
CountBits(unsigned long mask)	/* 32  1-bit tallies */
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
 *	Initializes both the power and inverse power tables for the
 *	painter with a given gamma value.  These tables are used
 *	to/from map linear RGB values to/from non-linear monitor
 *	intensities.
 *	
 * Results:
 *      The *gammaTable* and *igammaTable* arrays are filled out to
 *      contain the mapped values.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeGammaTables(Painter *painterPtr)
{
    int i;
    double igamma, gamma;
    
    gamma = painterPtr->gamma;
    igamma = 1.0 / gamma;
    for (i = 0; i < 256; i++) {
	double x, y;

	y = i / 255.0;
	x = pow(y, gamma) * 255.0 + 0.5;
	painterPtr->gammaTable[i] = (unsigned char)CLAMP(x);
	x = pow(y, igamma) * 255.0 + 0.5;
	painterPtr->igammaTable[i] = (unsigned char)CLAMP(x);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * QueryPalette --
 *
 *	Queries the X display server for the colors currently used in
 *	the colormap.  These values will then be used to map screen
 *	pixels back to RGB values (see Blt_DrawableToPicture). The
 *	queried non-linear color intensities are reverse mapped back
 *	to to linear RGB values.
 *	
 * Results:
 *      The *palette* array is filled in with the RGB color values 
 *      of the colors allocated.
 *
 *---------------------------------------------------------------------------
 */
static void
QueryPalette(Painter *painterPtr, Blt_Pixel *palette)
{
    Visual *visualPtr;
    XColor colors[256];

    visualPtr = painterPtr->visualPtr;
    assert(visualPtr->map_entries <= 256);

    if ((visualPtr->class == DirectColor) || (visualPtr->class == TrueColor)) {
	XColor *cp, *cend;
	unsigned int numRed, numGreen, numBlue;
	unsigned int  r, g, b;
	
	r = g = b = 0;
	nRed   = (painterPtr->rMask >> painterPtr->rShift) + 1;
	nGreen = (painterPtr->gMask >> painterPtr->gShift) + 1;
	nBlue  = (painterPtr->bMask >> painterPtr->bShift) + 1;

	for (cp = colors, cend = cp + visualPtr->map_entries; cp < cend; 
	     cp++) {
	    cp->pixel = ((r << painterPtr->rShift) | 
			 (g << painterPtr->gShift) |
			 (b << painterPtr->bShift));
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

#ifdef notdef
    /* FIXME */
    XQueryColors(painterPtr->display, painterPtr->colormap, colors, 
		 visualPtr->map_entries);
#endif    
    /* Scale to convert XColor component value (0..65535) to unsigned
     * char (0..255). */
    if (painterPtr->gamma == 1.0) {
	Blt_Pixel *dp, *dend;
	XColor *cp;
	double a;
	
	a = 1.0 / 257.0;
	cp = colors;
	for (dp = palette, dend = dp + visualPtr->map_entries; dp < dend; 
	     dp++) {
	    dp->Red =   (unsigned char)(cp->red * a + 0.5);
	    dp->Green = (unsigned char)(cp->green * a + 0.5);
	    dp->Blue =  (unsigned char)(cp->blue * a + 0.5);
	    cp++;
	}
    } else {
	Blt_Pixel *dp, *dend;
	XColor *cp;
	double a;

	a = 1.0 / 257.0;
	cp = colors;
	for (dp = palette, dend = dp + visualPtr->map_entries; dp < dend; 
	     dp++) {
	    dp->Red =   painterPtr->gammaTable[(int)(cp->red * a + 0.5)];
	    dp->Green = painterPtr->gammaTable[(int)(cp->green * a + 0.5)];
	    dp->Blue =  painterPtr->gammaTable[(int)(cp->blue * a + 0.5)];
	    cp++;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorRamp --
 *
 *	Computes a color ramp based upon the number of colors
 *	available for each color component.  It returns an array of
 *	the desired colors (XColor structures).  The screen gamma is
 *	factored into the desired colors.
 *	
 * Results:
 *      Returns the number of colors desired.  The *colors* array
 *	is filled out to contain the component values.
 *
 *---------------------------------------------------------------------------
 */
static int
ColorRamp(Painter *painterPtr, XColor *colors)
{
    int numColors;
    XColor *cp;
    double rScale, gScale, bScale;
    double igamma;
    int i;

    numColors = 0;		/* Suppress compiler warning. */

    /*
     * Calculate the RGB coordinates of the colors we want to allocate
     * and store them in *colors.
     */
    igamma = 1.0 / painterPtr->gamma;

    rScale = 255.0 / (painterPtr->nRed - 1);
    gScale = 255.0 / (painterPtr->nGreen - 1);
    bScale = 255.0 / (painterPtr->nBlue - 1);

    switch (painterPtr->visualPtr->class) {
    case TrueColor:
    case DirectColor:
	
	nColors = MAX3(painterPtr->nRed, painterPtr->nGreen,painterPtr->nBlue);
	if (painterPtr->isMonochrome) {
	    numColors = painterPtr->nBlue = painterPtr->nGreen = painterPtr->nRed;
	} 

	/* Compute the 16-bit RGB values from each possible 8-bit
	 * value. */
	cp = colors;
	for (i = 0; i < numColors; i++) {
	    int r, g, b;
	    
	    r = (int)(i * rScale + 0.5);
	    g = (int)(i * gScale + 0.5);
	    b = (int)(i * bScale + 0.5);

	    r = painterPtr->igammaTable[r];
	    g = painterPtr->igammaTable[g];
	    b = painterPtr->igammaTable[b];

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

	nColors = (painterPtr->nRed * painterPtr->nGreen * painterPtr->nBlue);
	if (painterPtr->isMonochrome) {
	    numColors = painterPtr->nRed;
	} 
	if (!painterPtr->isMonochrome) {
	    XColor *cp;
	    int i;
	    
	    cp = colors;
	    for (i = 0; i < painterPtr->nRed; i++) {
		int j;
		unsigned char r;
		
		r = (unsigned char)(i * rScale + 0.5);
		r = painterPtr->igammaTable[r];
		for (j = 0; j < painterPtr->nGreen; j++) {
		    int k;
		    unsigned int g;

		    g = (unsigned char)(j * gScale + 0.5);
		    g = painterPtr->igammaTable[g];
		    for (k = 0; k < painterPtr->nBlue; k++) {
			unsigned int b;

			b = (unsigned char)(k * bScale + 0.5);
			b = painterPtr->igammaTable[b];
			cp->red = (r << 8) | r;
			cp->green = (g << 8) | g;
			cp->blue = (b << 8) | b;
			cp++;
		    } 
		}
	    }
	}
	break;

    default: /* Monochrome */
	{
	    XColor *cp;
	    double scale;
	    int i;

	    scale = 255.0 / (nColors - 1);

	    cp = colors;
	    for (i = 0; i < numColors; ++i) {
		int c;

		c = (int)(i * scale + 0.5);
		c = painterPtr->igammaTable[c];
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
 *	Individually allocates each of the desired colors (as
 *	specified by the *colors* array).  If a color can't be
 *	allocated the desired colors allocated to that point as
 *	released, the number of component intensities is reduced,
 *	and 0 is returned.
 *
 *	For TrueColor visuals, we don't need to allocate colors
 *	at all, since we can compute them directly.  
 *	
 * Results:
 *      Returns 1 if all desired colors were allocated successfully.
 *	If unsuccessful, returns 0.  All colors allocated up to that
 *	point are freed and a smaller color palette size is computed
 *	and reset in the painter structure.
 *
 *---------------------------------------------------------------------------
 */
static int 
AllocateColors(Painter *painterPtr, XColor *colors, int numColors)
{
    if (painterPtr->visualPtr->class == TrueColor) {
	XColor *cp, *cend;

	/* 
	 * For TrueColor visuals, don't call XAllocColor, compute the
	 * pixel value directly.
	 */
	for (cp = colors, cend = cp + numColors; cp < cend; cp++) {
	    unsigned int r, g, b;

	    r = ((cp->red >> 8) >> painterPtr->rAdjust);
	    g = ((cp->green >> 8) >> painterPtr->gAdjust);
	    b = ((cp->blue >> 8) >> painterPtr->bAdjust);

	    /* Shift each color into the proper location of the pixel index. */
	    r = (r << painterPtr->rShift) & painterPtr->rMask;
	    g = (g << painterPtr->gShift) & painterPtr->gMask;
	    b = (b << painterPtr->bShift) & painterPtr->bMask;
	    cp->pixel = (r | g | b);
	}
	painterPtr->nPixels = 0; /* This will indicate that we didn't
				  * use XAllocColor to obtain pixel
				  * values. */
	return TRUE;
    } else {
	int i;
	XColor *cp;

	cp = colors;
	for (i = 0; i < numColors; i++) {
	    if (!XAllocColor(painterPtr->display, painterPtr->colormap, cp)){
		fprintf(stderr, "can't allocate color #%d: r=%x g=%x b=%x\n", 
			i, cp->red, cp->green, cp->blue);
		break;
	    }
#ifdef notdef
	    fprintf(stderr, "picture: allocated r=%x g=%x b=%x\n",
		colors[i].red, colors[i].green, colors[i].blue);
#endif
	    painterPtr->pixels[i] = cp->pixel;
	    cp++;
	}
	painterPtr->nPixels = i; /* # of pixels in array */
	if (i == numColors) {
	    fprintf(stderr, "painter palette %d/%d/%d colors okay\n", 
		    painterPtr->nRed, painterPtr->nGreen, painterPtr->nBlue);
	    return TRUE;		/* Success. */
	}
    }
    /*
     * If we didn't get all of the colors, free the current palette,
     * reduce the palette RGB component sizes.
     */
    fprintf(stderr, "can't allocate %d/%d/%d colors\n", painterPtr->nRed,
	    painterPtr->nGreen, painterPtr->nBlue);
    XFreeColors(painterPtr->display, painterPtr->colormap, painterPtr->pixels, 
	painterPtr->nPixels, 0);

    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * FillPalette --
 *
 *	Base upon the colors allocated, generate two mappings from
 *	the picture's 8-bit RGB components.
 *
 *	1) Map 8-bit RGB values to the bits of the pixel.  Each component
 *	   contains a portion of the pixel value.  For mapped visuals
 *	   (pseudocolor, staticcolor, grayscale, and staticgray) this 
 *	   pixel value will be translated to the actual pixel used by
 *	   the display.
 *
 *	2) Map 8-bit RGB values to the actual color values used.  The
 *	   color ramp generated may be only a subset of the possible
 *	   color values.  The resulting palette is used in dithering the
 *	   image, using the error between the desired picture RGB value
 *	   and the actual value used.
 *
 * Results:
 *	Color palette and pixel maps are filled in.
 *
 *---------------------------------------------------------------------------
 */
static void
FillPalette(Painter *painterPtr, XColor *colors, int numColors) 
{
    painterPtr->nColors = numColors;
    if (!painterPtr->isMonochrome) {
	painterPtr->flags |= PAINER_COLOR_WINDOW;
	
	if ((painterPtr->visualPtr->class != DirectColor) && 
	    (painterPtr->visualPtr->class != TrueColor)) {
	    painterPtr->flags |= PAINTER_MAP_COLORS;
	}
    }
    if (painterPtr->isMonochrome) {
	int i;
	
	for (i = 0; i < 256; i++) {
	    int c;
	    
	    c = (i + 127) / 255;
	    painterPtr->rBits[i] = colors[c].pixel;
	    painterPtr->palette[i].Blue = painterPtr->palette[i].Green = 
		painterPtr->palette[i].Red = (unsigned char)(c * 255 + 0.5);
	} 
    } else {
	int i, rMult;
	double rScale, gScale, bScale;
	
	rMult = painterPtr->nGreen * painterPtr->nBlue;
	
	rScale = 255.0 / (painterPtr->nRed - 1);
	gScale = 255.0 / (painterPtr->nGreen - 1);
	bScale = 255.0 / (painterPtr->nBlue - 1);
	
	for (i = 0; i < 256; i++) {
	    int r, g, b;
	    
	    r = (i * (painterPtr->nRed - 1) + 127) / 255;
	    g = (i * (painterPtr->nGreen - 1) + 127) / 255;
	    b = (i * (painterPtr->nBlue - 1) + 127) / 255;
	    
	    if ((painterPtr->visualPtr->class == DirectColor) || 
		(painterPtr->visualPtr->class == TrueColor)) {
		painterPtr->rBits[i] = colors[r].pixel & painterPtr->rMask;
		painterPtr->gBits[i] = colors[g].pixel & painterPtr->gMask;
		painterPtr->bBits[i] = colors[b].pixel & painterPtr->bMask;
	    } else {
		painterPtr->rBits[i] = r * rMult;
		painterPtr->gBits[i] = g * painterPtr->nBlue;
		painterPtr->bBits[i] = b;
	    }
	    painterPtr->palette[i].Red = (unsigned char)(r * rScale + 0.5);
	    painterPtr->palette[i].Green = (unsigned char)(g * gScale + 0.5);
	    painterPtr->palette[i].Blue = (unsigned char)(b * bScale + 0.5);
#ifdef notdef
	    fprintf(stderr, "picture: %d color=%x %x %x\n",
		    i, 
		    painterPtr->palette[i].Red, 
		    painterPtr->palette[i].Green, 
		    painterPtr->palette[i].Blue);
#endif
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * AllocatePalette --
 *
 *	This procedure allocates the colors required by a color table,
 *	and sets up the fields in the color table data structure which
 *	are used in dithering.
 *
 *	This routine essentially mimics what is done in tkImgPhoto.c.
 *	It's purpose is to allocate exactly the same color ramp as the
 *	photo image. That way both image types can co-exist without
 *	fighting over available colors.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Colors are allocated from the X server.  The color palette
 *	and pixel indices are updated.
 *
 *---------------------------------------------------------------------------
 */
static void
AllocatePalette(
    Painter *painterPtr)	/* Pointer to the color table requiring
				 * colors to be allocated. */
{
    XColor colors[256];
    int numColors;
    static int stdPalettes[13][3] = {
	/* numRed, numGreen, numBlue */
	{ 2,  2,  2  },		/* 3 bits, 8 colors */
	{ 2,  3,  2  },		/* 4 bits, 12 colors */
	{ 3,  4,  2  },		/* 5 bits, 24 colors */
	{ 4,  5,  3  },		/* 6 bits, 60 colors */
	{ 5,  6,  4  },		/* 7 bits, 120 colors */ 
	{ 7,  7,  4  },		/* 8 bits, 198 colors */
	{ 8,  10, 6  },		/* 9 bits, 480 colors */
	{ 10, 12, 8  },		/* 10 bits, 960 colors */
	{ 14, 15, 9  },		/* 11 bits, 1890 colors */
	{ 16, 20, 12 },		/* 12 bits, 3840 colors */
	{ 20, 24, 16 },		/* 13 bits, 7680 colors */
	{ 26, 30, 20 },		/* 14 bits, 15600 colors */
	{ 32, 32, 30 },		/* 15 bits, 30720 colors */
    };

    painterPtr->isMonochrome = FALSE; 
    switch (painterPtr->visualPtr->class) {
    case TrueColor:
    case DirectColor:
	painterPtr->nRed =   1 << CountBits(painterPtr->rMask);
	painterPtr->nGreen = 1 << CountBits(painterPtr->gMask);
	painterPtr->nBlue =  1 << CountBits(painterPtr->bMask);
	break;

    case GrayScale:
    case StaticGray:
    case PseudoColor:
    case StaticColor:
	if (painterPtr->depth > 15) {
	    painterPtr->nRed = painterPtr->nGreen = painterPtr->nBlue = 32;
	} else if (painterPtr->depth >= 3) {
	    int *ip = stdPalettes[painterPtr->depth - 3];
	    painterPtr->nRed = ip[0];
	    painterPtr->nGreen = ip[1];
	    painterPtr->nBlue = ip[2];
	}
	break;

    default:
	painterPtr->nGreen = painterPtr->nBlue = 0;
	painterPtr->nRed = 1 << painterPtr->depth;
	painterPtr->isMonochrome = TRUE;
	break;
    }

    /*
     * Each time around this loop, we reduce the number of colors
     * we're trying to allocate until we succeed in allocating all of
     * the colors we need.
     */
    for (;;) {
	/*
	 * If we are using 1 bit/pixel, we don't need to allocate any
	 * colors (we just use the foreground and background colors in
	 * the GC).
	 */
	if ((painterPtr->isMonochrome) && (painterPtr->nRed <= 2)) {
	    painterPtr->flags |= BLACK_AND_WHITE;
	    /* return; */
	}
	/*
	 * Calculate the RGB values of a color ramp, given the some
	 * number of red, green, blue intensities available.
	 */
	nColors = ColorRamp(painterPtr, colors);

	/* Now try to allocate the colors we've calculated. */

	if (AllocateColors(painterPtr, colors, numColors)) {
	    break;		/* Success. */
	}
	if (!painterPtr->isMonochrome) {
	    if ((painterPtr->nRed == 2) && (painterPtr->nGreen == 2) && 
		(painterPtr->nBlue == 2)) {
		break;
		/* Fall back to 1-bit monochrome display. */
		/* painterPtr->mono = TRUE; */
	    } else {
		/*
		 * Reduce the number of shades of each primary to
		 * about 3/4 of the previous value.  This will reduce
		 * the total number of colors required to less than
		 * half (27/64) the previous value for PseudoColor
		 * displays.
		 */
		painterPtr->nRed = (painterPtr->nRed * 3 + 2) / 4;
		painterPtr->nGreen = (painterPtr->nGreen * 3 + 2) / 4;
		painterPtr->nBlue = (painterPtr->nBlue * 3 + 2) / 4;
	    }
	} else {
	    painterPtr->nRed /= 2;
	}
    }
    FillPalette(painterPtr, colors, numColors);
}


/*
 *---------------------------------------------------------------------------
 *
 * NewPainter --
 *
 *	Creates a new painter to be used to paint pictures. Painters
 *	are keyed by the combination of display, colormap, visual,
 *	depth, and gamma value used.  
 *
 * Results:
 *      A pointer to the new painter is returned.
 *
 * Side Effects:
 *	A color ramp is allocated (not true for TrueColor visuals).
 *	Gamma tables are computed and filled.
 *
 *---------------------------------------------------------------------------
 */
static Painter *
NewPainter(PainterKey *keyPtr)
{
    Painter *painterPtr;
    
    painterPtr = Blt_AssertCalloc(1, sizeof(Painter));

    painterPtr->colormap = keyPtr->colormap;
    painterPtr->depth = keyPtr->depth;
    painterPtr->display = keyPtr->display;
    painterPtr->gamma = keyPtr->gamma;
    painterPtr->visualPtr = keyPtr->visualPtr;
    
    painterPtr->refCount = 0;
    painterPtr->rMask = (unsigned int)painterPtr->visualPtr->red_mask;
    painterPtr->gMask = (unsigned int)painterPtr->visualPtr->green_mask;
    painterPtr->bMask = (unsigned int)painterPtr->visualPtr->blue_mask;

    painterPtr->rShift = FindShift(painterPtr->rMask);
    painterPtr->gShift = FindShift(painterPtr->gMask);
    painterPtr->bShift = FindShift(painterPtr->bMask);

    painterPtr->rAdjust = painterPtr->gAdjust = painterPtr->bAdjust = 0;

    {
	int numRedBits, numGreenBits, numBlueBits;

	nRedBits = CountBits(painterPtr->rMask);
	nGreenBits = CountBits(painterPtr->gMask);
	nBlueBits = CountBits(painterPtr->bMask);
	if (nRedBits < 8) {
	    painterPtr->rAdjust = 8 - numRedBits;
	}
	if (nGreenBits < 8) {
	    painterPtr->gAdjust = 8 - numGreenBits;
	}
	if (nBlueBits < 8) {
	    painterPtr->bAdjust = 8 - numBlueBits;
	}
    }
    ComputeGammaTables(painterPtr);
    AllocatePalette(painterPtr);
    return painterPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * FreePainter --
 *
 *	Called when the TCL interpreter is idle, this routine frees the
 *	painter. Painters are reference counted. Only when no clients
 *	are using the painter (the count is zero) is the painter
 *	actually freed.  By deferring its deletion, this allows client
 *	code to call Blt_GetPainter after Blt_FreePainter without
 *	incurring a performance penalty.
 *
 *---------------------------------------------------------------------------
 */
static void
FreePainter(DestroyData data)
{
    Painter *painterPtr = (Painter *)data;

    if (painterPtr->refCount <= 0) {
	if (painterPtr->nColors > 0) {
	    XFreeColors(painterPtr->display, painterPtr->colormap,
		painterPtr->pixels, painterPtr->nPixels, 0);
	}
	Blt_DeleteHashEntry(&painterTable, painterPtr->hashPtr);
	if (painterPtr->gc != NULL) {
	    if (painterPtr->flags & GC_PRIVATE) {
		XFreeGC(painterPtr->display, painterPtr->gc);
	    } else {
		Tk_FreeGC(painterPtr->display, painterPtr->gc);
	    }
	    painterPtr->gc = NULL;
	}
	Blt_Free(painterPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetPainter --
 *
 *	Attempts to retrieve a painter for a particular combination of
 *	display, colormap, visual, depth, and gamma value.  If no
 *	specific painter exists, then one is created.
 *
 * Results:
 *      A pointer to the new painter is returned.
 *
 * Side Effects:
 *	If no current painter exists, a new painter is added to the
 *	hash table of painters.  Otherwise, the current painter's
 *	reference count is incremented indicated how many clients
 *	are using the painter.
 *
 *---------------------------------------------------------------------------
 */
static Painter *
GetPainter(
    Display *display, 
    Colormap colormap, 
    Visual *visualPtr,
    int depth,
    double gamma)
{
    Painter *painterPtr;
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
	painterPtr = NewPainter(&key);
	painterPtr->hashPtr = hPtr;
	Blt_SetHashValue(hPtr, painterPtr);
    } else {
	painterPtr = Blt_GetHashValue(hPtr);
    }
    painterPtr->refCount++;
    return painterPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPainterFromDrawable --
 *
 *	Gets a painter for a particular combination of display,
 *	colormap, visual, depth, and gamma value.  This information is
 *	retrieved from the drawable which is assumed to be a window.
 *
 * Results:
 *      A pointer to the new painter is returned.
 *
 *---------------------------------------------------------------------------
 */
Painter *
Blt_GetPainterFromDrawable(
    Display *display, 
    Drawable drawable, 
    double gamma)
{
    XGCValues gcValues;
    unsigned long gcMask;
    Painter *painterPtr;
    Colormap colormap;
    int screenNum;
    Visual *visual;
    int depth;
    
    screenNum = 0;
    /* Need colormap, visual, depth. */
    colormap = DefaultColormap(display, DefaultScreen(display));
    visual = DefaultVisual(display, screenNum);
    depth  = DefaultDepth(display, screenNum);
    painterPtr = GetPainter(display, colormap, visual, depth, gamma);

    /*
     * Make a GC with background = black and foreground = white.
     */
    gcMask = GCGraphicsExposures;
    gcValues.graphics_exposures = False;
	    
    painterPtr->gc = XCreateGC(display, drawable, gcMask, &gcValues);
    painterPtr->flags |= GC_PRIVATE;
    return painterPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPainter --
 *
 *	Gets a painter for a particular combination of display,
 *	colormap, visual, depth, and gamma value.  This information
 *	(except for the monitor's gamma value) is retrieved from the
 *	given Tk window.
 *
 * Results:
 *      A pointer to the new painter is returned.
 *
 *---------------------------------------------------------------------------
 */
Painter *
Blt_GetPainter(Tk_Window tkwin, double gamma)
{
    Painter *painterPtr;
    XGCValues gcValues;
    unsigned long gcMask;

    painterPtr = GetPainter(Tk_Display(tkwin), Tk_Colormap(tkwin), 
	Tk_Visual(tkwin), Tk_Depth(tkwin), gamma);

    /*
     * Make a GC with background = black and foreground = white.
     */
    gcMask = GCGraphicsExposures;
    gcValues.graphics_exposures = False;
    painterPtr->gc = Tk_GetGC(tkwin, gcMask, &gcValues);
    painterPtr->flags &= ~GC_PRIVATE;
    return painterPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FreePainter --
 *
 *	Frees the painter. Painters are reference counted. Only when
 *	no clients are using the painter (the count is zero) is the
 *	painter actually freed.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_FreePainter(Painter *painterPtr)
{
    painterPtr->refCount--;
    if (painterPtr->refCount <= 0) {
	Tcl_EventuallyFree(painterPtr, FreePainter);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * DrawableToPicture --
 *
 *      Takes a snapshot of an X drawable (pixmap or window) and
 *	converts it to a picture.
 *
 * Results:
 *      Returns a picture of the drawable.  If an error occurred,
 *	NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Picture
DrawableToPicture(
    Painter *painterPtr,
    Drawable drawable,
    int x, int y,
    int width, int height)	/* Dimension of the drawable. */
{
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr srcPort, destPort;
    Picture *destPtr;

    srcPort = TkMacOSXGetDrawablePort(drawable);
    destPtr = Blt_CreatePicture(width, height);
    {
	Rect srcRect, destRect;
        MacDrawable dstDraw = (MacDrawable)drawable;
        PixMap pm;
      
	SetRect(&srcRect, x, y, x + width, y + height);
        SetRect(&destRect, 0, 0, width, height);

	GetGWorld(&saveWorld, &saveDevice);
	SetGWorld(srcPort, NULL);

	TkMacOSXSetUpClippingRgn(drawable);

	pm.bounds.left = pm.bounds.top = 0;
	pm.bounds.right = (short)width;
	pm.bounds.bottom = (short)height;

	pm.pixelType = RGBDirect;
	pm.pmVersion = baseAddr32; /* 32bit clean */

	pm.packType = pm.packSize = 0;
	pm.hRes = pm.vRes = 0x00480000; /* 72 dpi */

	pm.pixelSize = sizeof(Blt_Pixel) * 8; /* Bits per pixel. */
	pm.cmpCount = 3;	/* 3 components for direct. */
	pm.cmpSize = 8;		/* 8 bits per component. */

	pm.pixelFormat = k32ARGBPixelFormat;
	pm.pmTable = NULL;
	pm.pmExt = 0;

	pm.baseAddr = (Ptr)destPtr->bits;
	pm.rowBytes = destPtr->pixelsPerRow * sizeof(Blt_Pixel);

	pm.rowBytes |= 0x8000;	   /* Indicates structure a PixMap,
				    * not a BitMap.  */
	
	CopyBits(GetPortBitMapForCopyBits(destPort), 
		 (BitMap *)&pm, &srcRect, &destRect, srcCopy, NULL);
    }
    SetGWorld(saveWorld, saveDevice);
    return destPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_WindowToPicture --
 *
 *      Takes a snapshot of an X drawable (pixmap or window) and
 *	converts it to a picture.
 *
 *	This routine is used to snap foreign (non-Tk) windows. For
 *	pixmaps and Tk windows, Blt_DrawableToPicture is preferred.
 *
 * Results:
 *      Returns a picture of the drawable.  If an error occurred,
 *	NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
Picture *
Blt_WindowToPicture(
    Display *display,
    Drawable drawable,
    int x, int y,		/* Offset of image from the drawable's
				 * origin. */
    int width, int height,	/* Dimension of the image.  Image must
				 * be completely contained by the
				 * drawable. */
    double gamma)
{
    Blt_Painter painter;
    Blt_Picture picture;

    painter = Blt_GetPainterFromDrawable(display, drawable, gamma);
    picture =  DrawableToPicture(painter, drawable, x, y, width, height);
    Blt_FreePainter(painter);
    return picture;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DrawableToPicture --
 *
 *      Takes a snapshot of an X drawable (pixmap or window) and
 *	converts it to a picture.
 *
 * Results:
 *      Returns a picture of the drawable.  If an error occurred,
 *	NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
Picture *
Blt_DrawableToPicture(
    Tk_Window tkwin,
    Drawable drawable,
    int x, int y,		/* Offset of image from the drawable's
				 * origin. */
    int width, int height,	/* Dimension of the image.  Image must
				 * be completely contained by the
				 * drawable. */
    double gamma)
{
    Blt_Painter painter;
    Blt_Picture picture;

    painter = Blt_GetPainter(tkwin, gamma);
    picture =  DrawableToPicture(painter, drawable, x, y, width, height);
    Blt_FreePainter(painter);
    return picture;
}


Pixmap
Blt_PhotoImageMask(
    Tk_Window tkwin,
    Tk_PhotoImageBlock src)
{
    TkWinBitmap *twdPtr;
    int offset, count;
    int x, y;
    unsigned char *srcPtr;
    int destBytesPerRow;
    int destHeight;
    unsigned char *destBits;

    destBytesPerRow = ((src.width + 31) & ~31) / 8;
    destBits = Blt_AssertCalloc(src.height, destBytesPerRow);
    destHeight = src.height;

    offset = count = 0;

    /* FIXME: figure out why this is so! */
    for (y = src.height - 1; y >= 0; y--) {
	srcPtr = src.pixelPtr + offset;
	for (x = 0; x < src.width; x++) {
	    if (srcPtr[src.offset[3]] == 0x00) {
		SetBit(x, y);
		count++;
	    }
	    srcPtr += src.pixelSize;
	}
	offset += src.pitch;
    }
    if (count > 0) {
	HBITMAP hBitmap;
	BITMAP bm;

	bm.bmType = 0;
	bm.bmWidth = src.width;
	bm.bmHeight = src.height;
	bm.bmWidthBytes = destBytesPerRow;
	bm.bmPlanes = 1;
	bm.bmBitsPixel = 1;
	bm.bmBits = destBits;
	hBitmap = CreateBitmapIndirect(&bm);

	twdPtr = Blt_AssertMalloc(sizeof(TkWinBitmap));
	twdPtr->type = TWD_BITMAP;
	twdPtr->handle = hBitmap;
	twdPtr->depth = 1;
	if (Tk_WindowId(tkwin) == None) {
	    twdPtr->colormap = DefaultColormap(Tk_Display(tkwin), 
			 DefaultScreen(Tk_Display(tkwin)));
	} else {
	    twdPtr->colormap = Tk_Colormap(tkwin);
	}
    } else {
	twdPtr = NULL;
    }
    if (destBits != NULL) {
	Blt_Free(destBits);
    }
    return (Pixmap)twdPtr;
}

Pixmap
Blt_PictureMask(
    Tk_Window tkwin,
    Blt_Picture pict)
{
    TkWinBitmap *twdPtr;
    int count;
    int x, y;
    Blt_Pixel *sp;
    int destBytesPerRow;
    int destWidth, destHeight;
    unsigned char *destBits;

    destWidth = Blt_PictureWidth(pict);
    destHeight = Blt_PictureHeight(pict);
    destBytesPerRow = ((destWidth + 31) & ~31) / 8;
    destBits = Blt_AssertCalloc(destHeight, destBytesPerRow);
    count = 0;
    sp = Blt_PictureBits(pict);
    for (y = 0; y < destHeight; y++) {
	for (x = 0; x < destWidth; x++) {
	    if (sp->Alpha == 0x00) {
		SetBit(x, y);
		count++;
	    }
	    sp++;
	}
    }
    if (count > 0) {
	HBITMAP hBitmap;
	BITMAP bm;

	bm.bmType = 0;
	bm.bmWidth = Blt_PictureWidth(pict);
	bm.bmHeight = Blt_PictureHeight(pict);
	bm.bmWidthBytes = destBytesPerRow;
	bm.bmPlanes = 1;
	bm.bmBitsPixel = 1;
	bm.bmBits = destBits;
	hBitmap = CreateBitmapIndirect(&bm);

	twdPtr = Blt_AssertMalloc(sizeof(TkWinBitmap));
	twdPtr->type = TWD_BITMAP;
	twdPtr->handle = hBitmap;
	twdPtr->depth = 1;
	if (Tk_WindowId(tkwin) == None) {
	    twdPtr->colormap = DefaultColormap(Tk_Display(tkwin), 
			 DefaultScreen(Tk_Display(tkwin)));
	} else {
	    twdPtr->colormap = Tk_Colormap(tkwin);
	}
    } else {
	twdPtr = NULL;
    }
    if (destBits != NULL) {
	Blt_Free(destBits);
    }
    return (Pixmap)twdPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_RotateBitmap --
 *
 *	Creates a new bitmap containing the rotated image of the given
 *	bitmap.  We also need a special GC of depth 1, so that we do
 *	not need to rotate more than one plane of the bitmap.
 *
 *	Note that under Windows, monochrome bitmaps are stored
 *	bottom-to-top.  This is why the right angle rotations 0/180
 *	and 90/270 look reversed.
 *
 * Results:
 *	Returns a new bitmap containing the rotated image.
 *
 *---------------------------------------------------------------------------
 */
Pixmap
Blt_RotateBitmap(
    Tk_Window tkwin,
    Pixmap srcBitmap,		/* Source bitmap to be rotated */
    int srcWidth, 
    int srcHeight,		/* Width and height of the source bitmap */
    float angle,		/* Right angle rotation to perform */
    int *destWidthPtr, 
    int *destHeightPtr)
{
    Display *display;		/* X display */
    Window root;		/* Root window drawable */
    Pixmap destBitmap;
    double rotWidth, rotHeight;
    HDC hDC;
    TkWinDCState state;
    int x, y;			/* Destination bitmap coordinates */
    int sx, sy;			/* Source bitmap coordinates */
    unsigned long pixel;
    HBITMAP hBitmap;
    int result;
    struct MonoBitmap {
	BITMAPINFOHEADER bi;
	RGBQUAD colors[2];
    } mb;
    int srcBytesPerRow, destBytesPerRow;
    int destWidth, destHeight;
    unsigned char *srcBits, *destBits;

    display = Tk_Display(tkwin);
    root = Tk_RootWindow(tkwin);
    Blt_GetBoundingBox(srcWidth, srcHeight, angle, &rotWidth, &rotHeight,
	(Point2d *)NULL);

    destWidth = (int)ceil(rotWidth);
    destHeight = (int)ceil(rotHeight);
    destBitmap = Blt_GetPixmap(display, root, destWidth, destHeight, 1);
    if (destBitmap == None) {
	return None;		/* Can't allocate pixmap. */
    }
    srcBits = Blt_GetBitmapData(display, srcBitmap, srcWidth, srcHeight,
	&srcBytesPerRow);
    if (srcBits == NULL) {
	OutputDebugString("Blt_GetBitmapData failed");
	return None;
    }
    destBytesPerRow = ((destWidth + 31) & ~31) / 8;
    destBits = Blt_AssertCalloc(destHeight, destBytesPerRow);

    angle = FMOD(angle, 360.0);
    if (FMOD(angle, (double)90.0) == 0.0) {
	int quadrant;

	/* Handle right-angle rotations specially. */

	quadrant = (int)(angle / 90.0);
	switch (quadrant) {
	case ROTATE_270:	/* 270 degrees */
	    for (y = 0; y < destHeight; y++) {
		sx = y;
		for (x = 0; x < destWidth; x++) {
		    sy = destWidth - x - 1;
		    pixel = GetBit(sx, sy);
		    if (pixel) {
			SetBit(x, y);
		    }
		}
	    }
	    break;

	case ROTATE_180:		/* 180 degrees */
	    for (y = 0; y < destHeight; y++) {
		sy = destHeight - y - 1;
		for (x = 0; x < destWidth; x++) {
		    sx = destWidth - x - 1;
		    pixel = GetBit(sx, sy);
		    if (pixel) {
			SetBit(x, y);
		    }
		}
	    }
	    break;

	case ROTATE_90:		/* 90 degrees */
	    for (y = 0; y < destHeight; y++) {
		sx = destHeight - y - 1;
		for (x = 0; x < destWidth; x++) {
		    sy = x;
		    pixel = GetBit(sx, sy);
		    if (pixel) {
			SetBit(x, y);
		    }
		}
	    }
	    break;

	case ROTATE_0:		/* 0 degrees */
	    for (y = 0; y < destHeight; y++) {
		for (x = 0; x < destWidth; x++) {
		    pixel = GetBit(x, y);
		    if (pixel) {
			SetBit(x, y);
		    }
		}
	    }
	    break;

	default:
	    /* The calling routine should never let this happen. */
	    break;
	}
    } else {
	double radians, sinTheta, cosTheta;
	double srcCX, srcCY;	/* Center of source rectangle */
	double destCX, destCY;	/* Center of destination rectangle */
	double tx, ty;
	double rx, ry;		/* Angle of rotation for x and y coordinates */

	radians = angle * DEG2RAD;
	sinTheta = sin(radians), cosTheta = cos(radians);

	/*
	 * Coordinates of the centers of the source and destination rectangles
	 */
	srcCX = srcWidth * 0.5;
	srcCY = srcHeight * 0.5;
	destCX = destWidth * 0.5;
	destCY = destHeight * 0.5;

	/* Rotate each pixel of dest image, placing results in source image */

	for (y = 0; y < destHeight; y++) {
	    ty = y - destCY;
	    for (x = 0; x < destWidth; x++) {

		/* Translate origin to center of destination image */
		tx = x - destCX;

		/* Rotate the coordinates about the origin */
		rx = (tx * cosTheta) - (ty * sinTheta);
		ry = (tx * sinTheta) + (ty * cosTheta);

		/* Translate back to the center of the source image */
		rx += srcCX;
		ry += srcCY;

		sx = ROUND(rx);
		sy = ROUND(ry);

		/*
		 * Verify the coordinates, since the destination image can be
		 * bigger than the source
		 */

		if ((sx >= srcWidth) || (sx < 0) || (sy >= srcHeight) ||
		    (sy < 0)) {
		    continue;
		}
		pixel = GetBit(sx, sy);
		if (pixel) {
		    SetBit(x, y);
		}
	    }
	}
    }
    hBitmap = ((TkWinDrawable *)destBitmap)->bitmap.handle;
    ZeroMemory(&mb, sizeof(mb));
    mb.bi.biSize = sizeof(BITMAPINFOHEADER);
    mb.bi.biPlanes = 1;
    mb.bi.biBitCount = 1;
    mb.bi.biCompression = BI_RGB;
    mb.bi.biWidth = destWidth;
    mb.bi.biHeight = destHeight;
    mb.bi.biSizeImage = destBytesPerRow * destHeight;
    mb.colors[0].rgbBlue = mb.colors[0].rgbRed = mb.colors[0].rgbGreen = 0x0;
    mb.colors[1].rgbBlue = mb.colors[1].rgbRed = mb.colors[1].rgbGreen = 0xFF;
    hDC = TkWinGetDrawableDC(display, destBitmap, &state);
    result = SetDIBits(hDC, hBitmap, 0, destHeight, (LPVOID)destBits, 
	(BITMAPINFO *)&mb, DIB_RGB_COLORS);
    TkWinReleaseDrawableDC(destBitmap, hDC, &state);
    if (!result) {
#if WINDEBUG
	PurifyPrintf("can't setDIBits: %s\n", Blt_LastError());
#endif
	destBitmap = None;
    }
    if (destBits != NULL) {
         Blt_Free(destBits);
    }
    if (srcBits != NULL) {
         Blt_Free(srcBits);
    }

    *destWidthPtr = destWidth;
    *destHeightPtr = destHeight;
    return destBitmap;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ScaleBitmap --
 *
 *	Creates a new scaled bitmap from another bitmap. 
 *
 * Results:
 *	The new scaled bitmap is returned.
 *
 * Side Effects:
 *	A new pixmap is allocated. The caller must release this.
 *
 *---------------------------------------------------------------------------
 */
Pixmap
Blt_ScaleBitmap(
    Tk_Window tkwin,
    Pixmap srcBitmap,
    int srcWidth, 
    int srcHeight, 
    int destWidth, 
    int destHeight)
{
    TkWinDCState srcState, destState;
    HDC src, dest;
    Pixmap destBitmap;
    Window root;
    Display *display;

    /* Create a new bitmap the size of the region and clear it */

    display = Tk_Display(tkwin);
    root = Tk_RootWindow(tkwin);
    destBitmap = Blt_GetPixmap(display, root, destWidth, destHeight, 1);
    if (destBitmap == None) {
	return None;
    }
    src = TkWinGetDrawableDC(display, srcBitmap, &srcState);
    dest = TkWinGetDrawableDC(display, destBitmap, &destState);

    StretchBlt(dest, 0, 0, destWidth, destHeight, src, 0, 0,
	srcWidth, srcHeight, SRCCOPY);

    TkWinReleaseDrawableDC(srcBitmap, src, &srcState);
    TkWinReleaseDrawableDC(destBitmap, dest, &destState);
    return destBitmap;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ScaleRotateBitmapRegion --
 *
 *	Creates a scaled and rotated bitmap from a given bitmap.  The
 *	caller also provides (offsets and dimensions) the region of
 *	interest in the destination bitmap.  This saves having to
 *	process the entire destination bitmap is only part of it is
 *	showing in the viewport.
 *
 *	This uses a simple rotation/scaling of each pixel in the 
 *	destination image.  For each pixel, the corresponding 
 *	pixel in the source bitmap is used.  This means that 
 *	destination coordinates are first scaled to the size of 
 *	the rotated source bitmap.  These coordinates are then
 *	rotated back to their original orientation in the source.
 *
 * Results:
 *	The new rotated and scaled bitmap is returned.
 *
 * Side Effects:
 *	A new pixmap is allocated. The caller must release this.
 *
 *---------------------------------------------------------------------------
 */
Pixmap
Blt_ScaleRotateBitmapRegion(
    Tk_Window tkwin,
    Pixmap srcBitmap,		/* Source bitmap. */
    unsigned int srcWidth, 
    unsigned int srcHeight,	/* Size of source bitmap */
    int regionX, 
    int regionY,		/* Offset of region in virtual
				 * destination bitmap. */
    unsigned int regionWidth, 
    unsigned int regionHeight,	/* Desire size of bitmap region. */
    unsigned int virtWidth,		
    unsigned int virtHeight,	/* Virtual size of destination bitmap. */
    float angle)		/* Angle to rotate bitmap.  */
{
    Display *display;		/* X display */
    Pixmap destBitmap;
    Window root;		/* Root window drawable */
    double rWidth, rHeight;
    double xScale, yScale;
    int srcBytesPerRow, destBytesPerRow;
    int destHeight;
    int result;
    unsigned char *srcBits, *destBits;

    display = Tk_Display(tkwin);
    root = Tk_RootWindow(tkwin);

    /* Create a bitmap and image big enough to contain the rotated text */
    destBitmap = Blt_GetPixmap(display, root, regionWidth, regionHeight, 1);
    if (destBitmap == None) {
	return None;		/* Can't allocate pixmap. */
    }
    srcBits = Blt_GetBitmapData(display, srcBitmap, srcWidth, srcHeight,
	&srcBytesPerRow);
    if (srcBits == NULL) {
	OutputDebugString("Blt_GetBitmapData failed");
	return None;
    }
    destBytesPerRow = ((regionWidth + 31) & ~31) / 8;
    destBits = Blt_AssertCalloc(regionHeight, destBytesPerRow);
    destHeight = regionHeight;

    angle = FMOD(angle, 360.0);
    Blt_GetBoundingBox(srcWidth, srcHeight, angle, &rWidth, &rHeight,
	       (Point2d *)NULL);
    xScale = rWidth / (double)virtWidth;
    yScale = rHeight / (double)virtHeight;

    if (FMOD(angle, (double)90.0) == 0.0) {
	int quadrant;
	int y;

	/* Handle right-angle rotations specifically */

	quadrant = (int)(angle / 90.0);
	switch (quadrant) {
	case ROTATE_270:	/* 270 degrees */
	    for (y = 0; y < (int)regionHeight; y++) {
		int sx, x;

		sx = (int)(yScale * (double)(y+regionY));
		for (x = 0; x < (int)regionWidth; x++) {
		    unsigned long pixel;
		    int sy;

		    sy = (int)(xScale *(double)(virtWidth - (x+regionX) - 1));
		    pixel = GetBit(sx, sy);
		    if (pixel) {
			SetBit(x, y);
		    }
		}
	    }
	    break;

	case ROTATE_180:	/* 180 degrees */
	    for (y = 0; y < (int)regionHeight; y++) {
		int sy, x;

		sy = (int)(yScale * (double)(virtHeight - (y + regionY) - 1));
		for (x = 0; x < (int)regionWidth; x++) {
		    unsigned long pixel;
		    int sx;

		    sx = (int)(xScale *(double)(virtWidth - (x+regionX) - 1));
		    pixel = GetBit(sx, sy);
		    if (pixel) {
			SetBit(x, y);
		    }
		}
	    }
	    break;

	case ROTATE_90:		/* 90 degrees */
	    for (y = 0; y < (int)regionHeight; y++) {
		int sx, x;

		sx = (int)(yScale * (double)(virtHeight - (y + regionY) - 1));
		for (x = 0; x < (int)regionWidth; x++) {
		    int sy;
		    unsigned long pixel;

		    sy = (int)(xScale * (double)(x + regionX));
		    pixel = GetBit(sx, sy);
		    if (pixel) {
			SetBit(x, y);
		    }
		}
	    }
	    break;

	case ROTATE_0:		/* 0 degrees */
	    for (y = 0; y < (int)regionHeight; y++) {
		int sy, x;

		sy = (int)(yScale * (double)(y + regionY));
		for (x = 0; x < (int)regionWidth; x++) {
		    int sx;
		    unsigned long pixel;

		    sx = (int)(xScale * (double)(x + regionX));
		    pixel = GetBit(sx, sy);
		    if (pixel) {
			SetBit(x, y);
		    }
		}
	    }
	    break;

	default:
	    /* The calling routine should never let this happen. */
	    break;
	}
    } else {
	double radians, sinTheta, cosTheta;
	double scx, scy; 	/* Offset from the center of the
				 * source rectangle. */
	double rcx, rcy; 	/* Offset to the center of the
				 * rotated rectangle. */
	int y;

	radians = angle * DEG2RAD;
	sinTheta = sin(radians), cosTheta = cos(radians);

	/*
	 * Coordinates of the centers of the source and destination rectangles
	 */
	scx = srcWidth * 0.5;
	scy = srcHeight * 0.5;
	rcx = rWidth * 0.5;
	rcy = rHeight * 0.5;

	/* For each pixel of the destination image, transform back to the
	 * associated pixel in the source image. */

	for (y = 0; y < (int)regionHeight; y++) {
	    int x;
	    double ty;		/* Translated coordinates from center */

	    ty = (yScale * (double)(y + regionY)) - rcy;
	    for (x = 0; x < (int)regionWidth; x++) {
		double rx, ry;	/* Angle of rotation for x and y coordinates */
		double tx;	/* Translated coordinates from center */
		int sx, sy;
		unsigned long pixel;

		/* Translate origin to center of destination image. */
		tx = (xScale * (double)(x + regionX)) - rcx;

		/* Rotate the coordinates about the origin. */
		rx = (tx * cosTheta) - (ty * sinTheta);
		ry = (tx * sinTheta) + (ty * cosTheta);

		/* Translate back to the center of the source image. */
		rx += scx;
		ry += scy;

		sx = ROUND(rx);
		sy = ROUND(ry);

		/*
		 * Verify the coordinates, since the destination image can be
		 * bigger than the source.
		 */

		if ((sx >= (int)srcWidth) || (sx < 0) || 
		    (sy >= (int)srcHeight) || (sy < 0)) {
		    continue;
		}
		pixel = GetBit(sx, sy);
		if (pixel) {
		    SetBit(x, y);
		}
	    }
	}
    }
    {
	HBITMAP hBitmap;
	HDC hDC;
	TkWinDCState state;
	struct MonoBitmap {
	    BITMAPINFOHEADER bmiHeader;
	    RGBQUAD colors[2];
	} mb;
	
	/* Write the rotated image into the destination bitmap. */
	hBitmap = ((TkWinDrawable *)destBitmap)->bitmap.handle;
	ZeroMemory(&mb, sizeof(mb));
	mb.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	mb.bmiHeader.biPlanes = 1;
	mb.bmiHeader.biBitCount = 1;
	mb.bmiHeader.biCompression = BI_RGB;
	mb.bmiHeader.biWidth = regionWidth;
	mb.bmiHeader.biHeight = regionHeight;
	mb.bmiHeader.biSizeImage = destBytesPerRow * regionHeight;
	mb.colors[0].rgbBlue = mb.colors[0].rgbRed = mb.colors[0].rgbGreen = 
	    0x0;
	mb.colors[1].rgbBlue = mb.colors[1].rgbRed = mb.colors[1].rgbGreen = 
	    0xFF;
	hDC = TkWinGetDrawableDC(display, destBitmap, &state);
	result = SetDIBits(hDC, hBitmap, 0, regionHeight, (LPVOID)destBits, 
		(BITMAPINFO *)&mb, DIB_RGB_COLORS);
	TkWinReleaseDrawableDC(destBitmap, hDC, &state);
    }
    if (!result) {
#if WINDEBUG
	PurifyPrintf("can't setDIBits: %s\n", Blt_LastError());
#endif
	destBitmap = None;
    }
    if (destBits != NULL) {
         Blt_Free(destBits);
    }
    if (srcBits != NULL) {
         Blt_Free(srcBits);
    }
    return destBitmap;
}

#ifdef HAVE_IJL_H

#include <ijl.h>

Blt_Picture
Blt_JPEGToPicture(interp, fileName)
    Tcl_Interp *interp;
    char *fileName;
{
    JPEG_CORE_PROPERTIES jpgProps;
    Blt_Picture pict;

    ZeroMemory(&jpgProps, sizeof(JPEG_CORE_PROPERTIES));
    if(ijlInit(&jpgProps) != IJL_OK) {
	Tcl_AppendResult(interp, "can't initialize Intel JPEG library",
			 (char *)NULL);
	return NULL;
    }
    jpgProps.JPGFile = fileName;
    if (ijlRead(&jpgProps, IJL_JFILE_READPARAMS) != IJL_OK) {
	Tcl_AppendResult(interp, "can't read JPEG file header from \"",
			 fileName, "\" file.", (char *)NULL);
	goto error;
    }

    // !dudnik: to fix bug case 584680, [OT:287A305B]
    // Set the JPG color space ... this will always be
    // somewhat of an educated guess at best because JPEG
    // is "color blind" (i.e., nothing in the bit stream
    // tells you what color space the data was encoded from).
    // However, in this example we assume that we are
    // reading JFIF files which means that 3 channel images
    // are in the YCbCr color space and 1 channel images are
    // in the Y color space.
    switch(jpgProps.JPGChannels) {
    case 1:
	jpgProps.JPGColor = IJL_G;
	jpgProps.DIBChannels = 4;
	jpgProps.DIBColor = IJL_RGBA_FPX;
	break;
	
    case 3:
	jpgProps.JPGColor = IJL_YCBCR;
	jpgProps.DIBChannels = 4;
	jpgProps.DIBColor = IJL_RGBA_FPX;
	break;

    case 4:
	jpgProps.JPGColor = IJL_YCBCRA_FPX;
	jpgProps.DIBChannels = 4;
	jpgProps.DIBColor = IJL_RGBA_FPX;
	break;

    default:
	/* This catches everything else, but no color twist will be
           performed by the IJL. */
	jpgProps.DIBColor = (IJL_COLOR)IJL_OTHER;
 	jpgProps.JPGColor = (IJL_COLOR)IJL_OTHER;
	jpgProps.DIBChannels = jpgProps.JPGChannels;
	break;
    }

    jpgProps.DIBWidth    = jpgProps.JPGWidth;
    jpgProps.DIBHeight   = jpgProps.JPGHeight;
    jpgProps.DIBPadBytes = IJL_DIB_PAD_BYTES(jpgProps.DIBWidth, 
					     jpgProps.DIBChannels);

    pict = Blt_CreatePicture(jpgProps.JPGWidth, jpgProps.JPGHeight);

    jpgProps.DIBBytes = (BYTE *)Blt_PictureBits(pict);
    if (ijlRead(&jpgProps, IJL_JFILE_READWHOLEIMAGE) != IJL_OK) {
	Tcl_AppendResult(interp, "can't read image data from \"", fileName,
		 "\"", (char *)NULL);
	goto error;
    }
    if (ijlFree(&jpgProps) != IJL_OK) {
	Tcl_AppendResult(interp, "can't free Intel(R) JPEG library.", 
		(char *)NULL);
    }
    return pict;

 error:
    ijlFree(&jpgProps);
    if (pict != NULL) {
	Blt_FreePicture(pict);
    }
    ijlFree(&jpgProps);
    return NULL;
} 

#else 

#ifdef HAVE_JPEGLIB_H

#undef HAVE_STDLIB_H
#ifdef WIN32
#define XMD_H	1
#endif
#include "jpeglib.h"
#include <setjmp.h>

typedef struct {
    struct jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf jmpBuf;
    Tcl_DString ds;
} ReaderHandler;

static void ErrorProc(j_common_ptr jpegInfo);
static void MessageProc(j_common_ptr jpegInfo);

/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void
ErrorProc(jpgPtr)
    j_common_ptr jpgPtr;
{
    ReaderHandler *handlerPtr = (ReaderHandler *)jpgPtr->err;

    (*handlerPtr->pub.output_message) (jpgPtr);
    longjmp(handlerPtr->jmpBuf, 1);
}

static void
MessageProc(jpgPtr)
    j_common_ptr jpgPtr;
{
    ReaderHandler *handlerPtr = (ReaderHandler *)jpgPtr->err;
    char buffer[JMSG_LENGTH_MAX];

    /* Create the message and append it into the dynamic string. */
    (*handlerPtr->pub.format_message) (jpgPtr, buffer);
    Tcl_DStringAppend(&handlerPtr->ds, " ", -1);
    Tcl_DStringAppend(&handlerPtr->ds, buffer, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_JPEGToPicture --
 *
 *      Reads a JPEG file and converts it into a picture.
 *
 * Results:
 *      The picture is returned.  If an error occured, such
 *	as the designated file could not be opened, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture
Blt_JPEGToPicture(interp, fileName)
    Tcl_Interp *interp;
    char *fileName;
{
    struct jpeg_decompress_struct jpg;
    Blt_Picture pict;
    unsigned int pictWidth, pictHeight;
    Blt_Pixel *dp;
    ReaderHandler handler;
    FILE *f;
    JSAMPLE **readBuffer;
    int row_stride;
    int i;
    JSAMPLE *bufPtr;

    f = Blt_OpenFile(interp, fileName, "rb");
    if (f == NULL) {
	return NULL;
    }
    pict = NULL;

    /* Step 1: allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    jpg.dct_method = JDCT_IFAST;
    jpg.err = jpeg_std_error(&handler.pub);
    handler.pub.error_exit = ErrorProc;
    handler.pub.output_message = MessageProc;

    Tcl_DStringInit(&handler.ds);
    Tcl_DStringAppend(&handler.ds, "error reading \"", -1);
    Tcl_DStringAppend(&handler.ds, fileName, -1);
    Tcl_DStringAppend(&handler.ds, "\": ", -1);

    if (setjmp(handler.jmpBuf)) {
	jpeg_destroy_decompress(&jpg);
	fclose(f);
	Tcl_DStringResult(interp, &handler.ds);
	return NULL;
    }
    jpeg_create_decompress(&jpg);
    jpeg_stdio_src(&jpg, f);

    jpeg_read_header(&jpg, TRUE);	/* Step 3: read file parameters */

    jpeg_start_decompress(&jpg);	/* Step 5: Start decompressor */
    pictWidth = jpg.output_width;
    pictHeight = jpg.output_height;
    if ((pictWidth < 1) || (pictHeight < 1)) {
	Tcl_AppendResult(interp, "bad JPEG image size", (char *)NULL);
	fclose(f);
	return NULL;
    }
    /* JSAMPLEs per row in output buffer */
    row_stride = pictWidth * jpg.output_components;

    /* Make a one-row-high sample array that will go away when done
     * with image */
    readBuffer = (*jpg.mem->alloc_sarray) ((j_common_ptr)&jpg, JPOOL_IMAGE, 
	row_stride, 1);
    pict = Blt_CreatePicture(pictWidth, pictHeight);
    dp = Blt_PictureBits(pict);

    if (jpg.output_components == 1) {
	while (jpg.output_scanline < pictHeight) {
	    jpeg_read_scanlines(&jpg, readBuffer, 1);
	    bufPtr = readBuffer[0];
	    for (i = 0; i < (int)pictWidth; i++) {
		dp->Red = dp->Green = dp->Blue = *bufPtr++;
		dp->Alpha = ALPHA_OPAQUE;
		dp++;
	    }
	}
    } else {
	while (jpg.output_scanline < pictHeight) {
	    jpeg_read_scanlines(&jpg, readBuffer, 1);
	    bufPtr = readBuffer[0];
	    for (i = 0; i < (int)pictWidth; i++) {
		dp->Red = *bufPtr++;
		dp->Green = *bufPtr++;
		dp->Blue = *bufPtr++;
		dp->Alpha = ALPHA_OPAQUE;
		dp++;
	    }
	}
    }
    jpeg_finish_decompress(&jpg);	/* We can ignore the return value
					 * since suspension is not
					 * possible with the stdio data
					 * source.  */
    jpeg_destroy_decompress(&jpg);


    /*  
     * After finish_decompress, we can close the input file.  Here we
     * postpone it until after no more JPEG errors are possible, so as
     * to simplify the setjmp error logic above.  (Actually, I don't
     * think that jpeg_destroy can do an error exit, but why assume
     * anything...)  
     */
    fclose(f);

    /* 
     * At this point you may want to check to see whether any corrupt-data
     * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
     */
    if (handler.pub.num_warnings > 0) {
	Tcl_SetErrorCode(interp, "IMAGE", "JPEG", 
		 Tcl_DStringValue(&handler.ds), (char *)NULL);
    } else {
	Tcl_SetErrorCode(interp, "NONE", (char *)NULL);
    }
    /*
     * We're ready to call the Tk_Photo routines. They'll take the RGB
     * array we've processed to build the Tk image of the JPEG.
     */
    Tcl_DStringFree(&handler.ds);
    return pict;
}

#endif /* HAVE_JPEGLIB_H */
#endif /* HAVE_IJL_H */

/*
 *---------------------------------------------------------------------------
 *
 * PaintPicture --
 *
 *	Paints the picture to the given drawable. The region of
 *	the picture is specified and the coordinates where in the 
 *	destination drawable is the image to be displayed.
 *
 *	The image may be dithered depending upon the bit set in
 *	the flags parameter: 0 no dithering, 1 for dithering.
 * 
 * Results:
 *	Returns TRUE is the picture was successfully display,
 *	Otherwise FALSE is returned if the particular combination
 *	visual and image depth is not handled.
 *
 *---------------------------------------------------------------------------
 */
static int
PaintPicture(
    Painter *painterPtr,
    Drawable drawable,
    Picture *srcPtr,
    int srcX, int srcY,		/* Coordinates of region in the
				 * picture. */
    int width, int height,	/* Dimension of the region.  Region
				 * cannot extend beyond the end of the
				 * picture. */
    int destX, int destY,	/* Coordinates of region in the
				 * drawable.  */
    unsigned int flags)
{
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    Picture *ditherPtr;

    ditherPtr = NULL;
    if (flags & BLT_PAINTER_DITHER) {
	ditherPtr = Blt_DitherPicture(srcPtr, painterPtr->palette);
	if (ditherPtr != NULL) {
	    srcPtr = ditherPtr;
	}
    }

    destPort = TkMacOSXGetDrawablePort(drawable);
    {
	Rect srcRect, destRect;
        MacDrawable dstDraw = (MacDrawable)drawable;
        PixMap pm;
      
	SetRect(&srcRect, srcX, srcY, srcX + width, srcY + height);
        SetRect(&destRect, 
	    destX + dstDraw->xOffset, 
	    destY + dstDraw->yOffset, 
            destX + width + dstDraw->xOffset, 
	    destY + height + dstDraw->yOffset);

	GetGWorld(&saveWorld, &saveDevice);
	SetGWorld(destPort, NULL);

	TkMacOSXSetUpClippingRgn(drawable);

	pm.bounds.left = pm.bounds.top = 0;
	pm.bounds.right = (short)width;
	pm.bounds.bottom = (short)height;

	pm.pixelType = RGBDirect;
	pm.pmVersion = baseAddr32; /* 32bit clean */

	pm.packType = pm.packSize = 0;
	pm.hRes = pm.vRes = 0x00480000; /* 72 dpi */

	pm.pixelSize = sizeof(Blt_Pixel) * 8; /* Bits per pixel. */
	pm.cmpCount = 3;	/* 3 components for direct. */
	pm.cmpSize = 8;		/* 8 bits per component. */

	pm.pixelFormat = k32ARGBPixelFormat;
	pm.pmTable = NULL;
	pm.pmExt = 0;

	pm.baseAddr = (Ptr)srcPtr->bits;
	pm.rowBytes = srcPtr->pixelsPerRow * sizeof(Blt_Pixel);

	pm.rowBytes |= 0x8000;	   /* Indicates structure a PixMap,
				    * not a BitMap.  */
	
	CopyBits((BitMap *)&pm, 
		 GetPortBitMapForCopyBits(destPort), 
		 &srcRect, &destRect, srcCopy, NULL);
    }
    if (ditherPtr != NULL) {
        Blt_FreePicture(ditherPtr);
    }
    SetGWorld(saveWorld, saveDevice);
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * PaintPictureWithBlend --
 *
 *	Blends and paints the picture with the given drawable. The
 *	region of the picture is specified and the coordinates where
 *	in the destination drawable is the image to be displayed.
 *
 *	The background is snapped from the drawable and converted into
 *	a picture.  This picture is then blended with the current
 *	picture (the background always assumed to be 100% opaque).
 * 
 * Results:
 *	Returns TRUE is the picture was successfully display,
 *	Otherwise FALSE is returned.  This may happen if the
 *	background can not be obtained from the drawable.
 *
 *---------------------------------------------------------------------------
 */
static int
PaintPictureWithBlend(
    Painter *painterPtr,
    Drawable drawable,
    Blt_Picture fg,
    int x, int y,		/* Coordinates of region in the
				 * picture. */
    int width, int height,	/* Dimension of the region.  Region
				 * cannot extend beyond the end of the
				 * picture. */
    int destX, int destY,	/* Coordinates of region in the
				 * drawable.  */
    unsigned int flags)
{
    Blt_Picture bg;

    if (destX < 0) {
	width += destX;
	destX = 0;
    } 
    if (destY < 0) {
	height += destY;
	destY = 0;
    }
    if ((width < 0) || (height < 0)) {
	return FALSE;
    }
    bg = DrawableToPicture(painterPtr, drawable, destX, destY, width, height);
    if (bg == NULL) {
	return FALSE;
    }
    Blt_BlendPictures(bg, fg, x, y, bg->width, bg->height, 0, 0);
    PaintPicture(painterPtr, drawable, bg, 0, 0, bg->width, bg->height, destX, 
	destY, flags);
    Blt_FreePicture(bg);
    return TRUE;
}


int
Blt_PaintPicture(
    Blt_Painter painter,
    Drawable drawable,
    Blt_Picture picture,
    int x, int y,		/* Coordinates of region in the
				 * picture. */
    int width, int height,	/* Dimension of the region.  Region
				 * cannot extend beyond the end of the
				 * picture. */
    int destX, int destY,	/* Coordinates of region in the
				 * drawable.  */
    unsigned int flags)
{
    if ((picture == NULL) || (x >= Blt_PictureWidth(picture)) || 
	(y >= Blt_PictureHeight(picture))) {
	/* Nothing to draw. The region offset starts beyond the end of
	 * the picture. */
	return TRUE;	
    }
    if ((width + x) > Blt_PictureWidth(picture)) {
	width = Blt_PictureWidth(picture) - x;
    }
    if ((height + y) > Blt_PictureHeight(picture)) {
	height = Blt_PictureHeight(picture) - y;
    }
    if ((width <= 0) || (height <= 0)) {
	return TRUE;
    }
    if (Blt_IsOpaquePicture(picture)) {
	return PaintPicture(painter, drawable, picture, x, y, width, height, 
		destX, destY, flags);
    } else {
	return PaintPictureWithBlend(painter, drawable, picture, x, y, 
		width, height, destX, destY, flags);
    }
}


int
Blt_PaintPictureWithBlend(
    Blt_Painter painter,
    Drawable drawable,
    Blt_Picture picture,
    int x, int y,		/* Coordinates of region in the
				 * picture. */
    int width, int height,	/* Dimension of the region.  Region
				 * cannot extend beyond the end of the
				 * picture. */
    int destX, int destY,	/* Coordinates of region in the
				 * drawable.  */
    unsigned int flags)		/* Indicates whether to dither the
				 * picture before displaying. */
{
    assert((x >= 0) && (y >= 0));
    /* assert((destX >= 0) && (destY >= 0)); */
    assert((width >= 0) && (height >= 0));

    if ((x >= Blt_PictureWidth(picture)) || (y >= Blt_PictureHeight(picture))){
	/* Nothing to draw. The region offset starts beyond the end of
	 * the picture. */	
	return TRUE;
    }
    /* 
     * Check that the region defined does not extend beyond the end of
     * the picture.
     *
     * Clip the end of the region if it is too big.
     */
    if ((width + x) > Blt_PictureWidth(picture)) {
	width = Blt_PictureWidth(picture) - x;
    }
    if ((height + y) > Blt_PictureHeight(picture)) {
	height = Blt_PictureHeight(picture) - y;
    }
    return PaintPictureWithBlend(painter, drawable, picture, x, y, width,
	height, destX, destY, flags);
}


GC 
Blt_PainterGC(Painter *painterPtr)
{
    return painterPtr->gc;
}

int 
Blt_PainterDepth(Painter *painterPtr)
{
    return painterPtr->depth;
}

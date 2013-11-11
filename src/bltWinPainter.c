/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltWinPainter.c --
 *
 * This module implements Win32-specific image processing procedures for the
 * BLT toolkit.
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

#include <X11/Xutil.h>

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltPicture.h"
#include "bltBg.h"
#include "bltPainter.h"
#include "bltWinPainter.h"
#include "tkDisplay.h"

#define CLAMP(c)	((((c) < 0.0) ? 0.0 : ((c) > 255.0) ? 255.0 : (c)))


#define CFRAC(i, n)	((i) * 65535 / (n))
/* As for CFRAC, but apply exponent of g. */
#define CGFRAC(i, n, g)	((int)(65535 * pow((double)(i) / (n), (g))))

typedef struct _Blt_Picture Pict;

/*
 * The following structure is used to encapsulate palette information.
 */

typedef struct {
    HPALETTE palette;		/* Palette handle used when drawing. */
    UINT size;			/* Number of entries in the palette. */
    int stale;			/* 1 if palette needs to be realized,
				 * otherwise 0.  If the palette is stale,
				 * then an idle handler is scheduled to
				 * realize the palette. */
    Tcl_HashTable refCounts;	/* Hash table of palette entry reference counts
				 * indexed by pixel value. */
} TkWinColormap;

/*
 * PainterKey --
 *
 * This structure represents the key used to uniquely identify painters.  A
 * painter is specified by a combination of display, visual, colormap, depth,
 * and monitor gamma value.
 */
typedef struct {
    Display *display;		/* Display of painter. Used to free colors
				 * allocated. */

    Colormap colormap;		/* Colormap used.  This may be the default
				 * colormap, or an allocated private map. */

    int depth;			/* Pixel depth of the display. */

    float gamma;		/* Gamma correction value of monitor. */

} PainterKey;


#define GC_PRIVATE	1	/* Indicates if the GC in the painter was
				 * shared (allocated by Tk_GetGC) or private
				 * (by XCreateGC). */

static Tcl_FreeProc FreePainter;

static Blt_HashTable painterTable;
static int initialized = 0;

static void
GetPaletteColors(HDC dc, Painter *p, Blt_Pixel *colors)
{
    DWORD flags;

    flags = GetDeviceCaps(dc, RASTERCAPS);
    if (flags & RC_PALETTE) {
	LOGPALETTE *logPalPtr;
	PALETTEENTRY *pePtr;
	Blt_Pixel *dp, *dend;
	TkWinColormap *cmap;

	logPalPtr = (LOGPALETTE *) GlobalAlloc(GPTR, 
		sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));
	logPalPtr->palVersion = 0x300;
	cmap = (TkWinColormap *)p->colormap;
	logPalPtr->palNumEntries = GetPaletteEntries(cmap->palette, 0, 256, 
		logPalPtr->palPalEntry);
	pePtr = logPalPtr->palPalEntry;
	for (dp = colors, dend = dp + logPalPtr->palNumEntries; dp < dend; 
		dp++, pePtr++) {
#ifdef notdef
	    int r, g, b;
	    r = p->igammaTable[r];
	    g = p->igammaTable[g];
	    b = p->igammaTable[b];
#endif
	    dp->Red = pePtr->peRed;
	    dp->Green = pePtr->peGreen;
	    dp->Blue = pePtr->peBlue;
	    dp->Alpha = 0xFF;
	}
	GlobalFree(logPalPtr);
    } else {
	Blt_Pixel *dp, *dend;
	double rScale, gScale, bScale;
	int i;
	int numRed, numGreen, numBlue;

	/*
	 * Calculate the RGB coordinates of the colors we want to allocate and
	 * store them in *colors.
	 */
	numRed = numGreen = 8, numBlue = 4;
	rScale = 255.0 / (numRed - 1);
	gScale = 255.0 / (numGreen - 1);
	bScale = 255.0 / (numBlue - 1);
	
	for (i = 0, dp = colors, dend = dp + 256; dp < dend; dp++, i++) {
	    int r, g, b;
	    
	    r = (int)(i * rScale + 0.5);
	    g = (int)(i * gScale + 0.5);
	    b = (int)(i * bScale + 0.5);

	    r = p->igammaTable[r];
	    g = p->igammaTable[g];
	    b = p->igammaTable[b];

	    dp->Red = (r << 8) + r;
	    dp->Green = (g << 8) + g;
	    dp->Blue = (b << 8) + b;
	    dp->Alpha = 0xFF;
	}
    }
    ReleaseDC(NULL, dc);
}


/*
 *---------------------------------------------------------------------------
 *
 * ComputeGammaTables --
 *
 *	Initializes both the power and inverse power tables for the painter
 *	with a given gamma value.  These tables are used to/from map linear
 *	RGB values to/from non-linear monitor intensities.
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
 * NewPainter --
 *
 *	Creates a new painter to be used to paint pictures. Painters are keyed
 *	by the combination of display, colormap, visual, depth, and gamma
 *	value used.
 *
 * Results:
 *      A pointer to the new painter is returned.
 *
 * Side Effects:
 *	A color ramp is allocated (not true for TrueColor visuals).  Gamma
 *	tables are computed and filled.
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
    p->refCount = 0;

    ComputeGammaTables(p);
    return p;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreePainter --
 *
 *	Called when the TCL interpreter is idle, this routine frees
 *	the painter. Painters are reference counted.  Only when no
 *	clients are using the painter (the count is zero) is the
 *	painter actually freed.  By deferring its deletion, this
 *	allows client code to call Blt_GetPainter after
 *	Blt_FreePainter without incurring a performance penalty.
 *
 *---------------------------------------------------------------------------
 */
static void
FreePainter(DestroyData data)
{
    Painter *p = (Painter *)data;

    if (p->refCount <= 0) {
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
 *	Attempts to retrieve a painter for a particular combination of
 *	display, colormap, visual, depth, and gamma value.  If no specific
 *	painter exists, then one is created.
 *
 * Results:
 *      A pointer to the new painter is returned.
 *
 * Side Effects:
 *	If no current painter exists, a new painter is added to the hash table
 *	of painters.  Otherwise, the current painter's reference count is
 *	incremented indicated how many clients are using the painter.
 *
 *---------------------------------------------------------------------------
 */
static Painter *
GetPainter(Display *display, Colormap colormap, int depth, float gamma)
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
 * DrawableToPicture --
 *
 *      Takes a snapshot of a DC and converts it to a picture.
 *
 * Results:
 *      Returns a picture of the drawable.  If an error occurred, NULL is
 *      returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Picture
DrawableToPicture(
    Painter *painterPtr,
    Drawable drawable,
    int x, int y, int w, int h)	/* Dimension of the drawable. */
{
    BITMAPINFO bmi;
    DIBSECTION ds;
    HBITMAP hBitmap;
    HDC dc, memdc;
    Pict *destPtr;
    TkWinDCState state;
    void *data;
    HPALETTE prevPalette;
    int resetPalette;

    dc = TkWinGetDrawableDC(painterPtr->display, drawable, &state);

    /* Create the intermediate drawing surface at window resolution. */
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hBitmap = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &data, NULL, 0);
    memdc = CreateCompatibleDC(dc);
    (void)SelectBitmap(memdc, hBitmap);
    if (GetDeviceCaps(dc, RASTERCAPS) & RC_PALETTE) {
	TkWinColormap *mapPtr;

	mapPtr = (TkWinColormap *)painterPtr->colormap;
	prevPalette = SelectPalette(dc, mapPtr->palette, FALSE);
	RealizePalette(dc);
	SelectPalette(memdc, mapPtr->palette, FALSE);
	RealizePalette(memdc);
	resetPalette = TRUE;
    } else {
	resetPalette = FALSE;
    }
    destPtr = NULL;

    /* Copy the window contents to the memory surface. */
    if (!BitBlt(memdc, 0, 0, w, h, dc, x, y, SRCCOPY)) {
#ifdef notdef
	PurifyPrintf("can't blit: %s\n", Blt_LastError());
#endif
	goto done;
    }
    if (GetObject(hBitmap, sizeof(DIBSECTION), &ds) == 0) {
#ifdef notdef
	PurifyPrintf("can't get object: %s\n", Blt_LastError());
#endif
    } else {
	Blt_Pixel *destRowPtr;
	unsigned char *bits, *sp;

	bits = (unsigned char *)ds.dsBm.bmBits;
	destPtr = Blt_CreatePicture(w, h);
	destRowPtr = destPtr->bits;
	
	/* 
	 * Copy the DIB RGB data into the picture. The DIB origin is the
	 * bottom-left corner, so the scanlines are stored in reverse order
	 * from that of a picture.  
	 */
	destRowPtr = destPtr->bits + ((h - 1) * destPtr->pixelsPerRow);
	sp = bits;
	for (y = 0; y < h; y++) {
	    Blt_Pixel *dp, *dend;
	    
	    for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
		dp->Blue  = painterPtr->gammaTable[sp[0]];
		dp->Green = painterPtr->gammaTable[sp[1]];
		dp->Red   = painterPtr->gammaTable[sp[2]];
		dp->Alpha = ALPHA_OPAQUE; 
		sp += 4;
	    }
	    destRowPtr -= destPtr->pixelsPerRow;
	}
    }
  done:
    DeleteBitmap(hBitmap);
    if (resetPalette) {
	SelectPalette(dc, prevPalette, FALSE);
    }
    DeleteDC(memdc);
    TkWinReleaseDrawableDC(drawable, dc, &state);
    return destPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * PaintPicture --
 *
 *	Paints the picture to the given drawable. The region of the picture is
 *	specified and the coordinates where in the destination drawable is the
 *	image to be displayed.
 *
 *	The image may be dithered depending upon the bit set in the flags
 *	parameter: 0 no dithering, 1 for dithering.
 * 
 * Results:
 *	Returns TRUE is the picture was successfully display, Otherwise FALSE
 *	is returned if the particular combination visual and image depth is
 *	not handled.
 *
 *---------------------------------------------------------------------------
 */
static int
PaintPicture(
    Painter *painterPtr,
    Drawable drawable,
    Pict *srcPtr,
    int x, int y,		/* Coordinates of region in the picture. */
    int w, int h,		/* Dimension of the region.  Area cannot
				 * extend beyond the end of the picture. */
    int dx, int dy,		/* Coordinates of region in the drawable.  */
    unsigned int flags)
{
    HDC dc;
    Pict *ditherPtr;
    TkWinDCState state;
    int resetPalette;
    HPALETTE prevPalette;

    ditherPtr = NULL;
    dc = TkWinGetDrawableDC(painterPtr->display, drawable, &state);
    if (GetDeviceCaps(dc, RASTERCAPS) & RC_PALETTE) {
	TkWinColormap *mapPtr;

	mapPtr = (TkWinColormap *)painterPtr->colormap;
	prevPalette = SelectPalette(dc, mapPtr->palette, FALSE);
	RealizePalette(dc);
	resetPalette = TRUE;
    } else {
	resetPalette = FALSE;
    }
    if (flags & BLT_PAINTER_DITHER) {
	Blt_Pixel colors[256];

	GetPaletteColors(dc, painterPtr, colors);
	ditherPtr = Blt_DitherPicture(srcPtr, colors);
	if (ditherPtr != NULL) {
	    srcPtr = ditherPtr;
	}
    }
    assert((x + w) <= srcPtr->width);
    assert((y + h) <= srcPtr->height);
    {
	BITMAPINFO bmi;
	Blt_Pixel *srcRowPtr;
	int sy;
	unsigned char *dp, *bits;

	bits = Blt_AssertMalloc(w * h * sizeof(Blt_Pixel));

	/* 
	 * Copy the DIB RGB data into the picture. The DIB scanlines are
	 * stored bottom-to-top and the order of the color components is BGRA.
	 */
	srcRowPtr = srcPtr->bits + ((y + h - 1) * srcPtr->pixelsPerRow) + x;
	dp = bits;
	for (sy = 0; sy < h; sy++) {
	    Blt_Pixel *sp, *send;
	    
	    for (sp = srcRowPtr, send = sp + w; sp < send; sp++) {
		dp[0] = sp->Blue;
		dp[1] = sp->Green;
		dp[2] = sp->Red;
		dp[3] = ALPHA_OPAQUE;
		dp += 4;
	    }
	    srcRowPtr -= srcPtr->pixelsPerRow;
	}
	
	/* Create the intermediate drawing surface at window resolution. */
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	
	SetDIBitsToDevice(dc, dx, dy, w, h, 0, 0, 0, h, bits, &bmi, 
		DIB_RGB_COLORS);
	Blt_Free(bits);
    }
    if (resetPalette) {
	SelectPalette(dc, prevPalette, FALSE);
    }
    TkWinReleaseDrawableDC(drawable, dc, &state);
    if (ditherPtr != NULL) {
	Blt_FreePicture(ditherPtr);
    }
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * PaintPictureWithBlend --
 *
 *	Blends and paints the picture in the given drawable. The region of the
 *	picture is specified and the coordinates where in the destination
 *	drawable is the image to be displayed.
 *
 *	The background is snapped from the drawable and converted into a
 *	picture.  This picture is then blended with the current picture (the
 *	background always assumed to be 100% opaque).
 * 
 * Results:
 *	Returns TRUE is the picture was successfully display, Otherwise FALSE
 *	is returned.  This may happen if the background can not be obtained
 *	from the drawable.
 *
 *---------------------------------------------------------------------------
 */
static int
PaintPictureWithBlend(
    Painter *p,
    Drawable drawable,
    Blt_Picture fg,
    int x, int y,		/* Coordinates of source region in the
				 * picture. */
    int w, int h,		/* Dimension of the source region.  Region
				 * cannot extend beyond the end of the
				 * picture. */
    int dx, int dy,		/* Coordinates of destination region in the
				 * drawable.  */
    unsigned int flags)
{
    Blt_Picture bg;

#ifdef notdef
    fprintf(stderr, "PaintPictureWithBlend: x=%d,y=%d,w=%d,h=%d,dx=%d,dy=%d\n",
	    x, y, w, h, dx, dy);
#endif
    if (dx < 0) {
	w += dx;
	x -= dx;
	dx = 0;
    } 
    if (dy < 0) {
	h += dy;
	y -= dy;
	dy = 0;
    }
    if (dx < 0) {
	dx = 0;
    } 
    if (dy < 0) {
	dy = 0;
    }
    if ((w < 0) || (h < 0)) {
	return FALSE;
    }
    bg = DrawableToPicture(p, drawable, dx, dy, w, h);
    if (bg == NULL) {
	return FALSE;
    }
    Blt_BlendRegion(bg, fg, x, y, w, h, 0, 0);
    PaintPicture(p, drawable, bg, 0, 0, w, h, dx, dy, flags);
    Blt_FreePicture(bg);
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPainterFromDrawable --
 *
 *	Gets a painter for a particular combination of display, colormap,
 *	visual, depth, and gamma value.  This information is retrieved from
 *	the drawable which is assumed to be a window.
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
    TkWinBitmap *bmPtr;

    bmPtr = (TkWinBitmap *)drawable;
    assert(bmPtr->type != TWD_BITMAP);
    p = GetPainter(display, bmPtr->colormap, bmPtr->depth, gamma);

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
 *	Gets a painter for a particular combination of display, colormap,
 *	visual, depth, and gamma value.  This information (except for the
 *	monitor's gamma value) is retrieved from the given Tk window.
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

    p = GetPainter(Tk_Display(tkwin), Tk_Colormap(tkwin), Tk_Depth(tkwin), 
	gamma);

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
 *	Frees the painter. Painters are reference counted. Only when no
 *	clients are using the painter (the count is zero) is the painter
 *	actually freed.
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
Blt_Picture 
Blt_WindowToPicture(
    Display *display,
    Drawable drawable,
    int x, int y,		/* Offset of image from the drawable's
				 * origin. */
    int w, int h,		/* Dimension of the image.  Image must
				 * be completely contained by the
				 * drawable. */
    float gamma)
{
    Blt_Painter painter;
    Blt_Picture dump;		/* Picture containing dump of window. */

    painter = Blt_GetPainterFromDrawable(display, drawable, gamma);
    dump = DrawableToPicture(painter, drawable, x, y, w, h);
    Blt_FreePainter(painter);
    return dump;
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
Blt_Picture 
Blt_DrawableToPicture(
    Tk_Window tkwin,
    Drawable drawable,
    int x, int y,		/* Offset of image from the drawable's
				 * origin. */
    int w, int h,		/* Dimension of the image.  Image must
				 * be completely contained by the
				 * drawable. */
    float gamma)
{
    Blt_Painter painter;
    Blt_Picture dump;		/* Picture containing dump of drawable. */

    painter = Blt_GetPainter(tkwin, gamma);
    dump =  DrawableToPicture(painter, drawable, x, y, w, h);
    Blt_FreePainter(painter);
    return dump;
}



int
Blt_PaintPicture(
    Blt_Painter painter,
    Drawable drawable,
    Blt_Picture picture,
    int ax, int ay,		/* Starting coordinates of subregion in the
				 * picture to be painted. */
    int aw, int ah,		/* Dimension of the subregion.  */
    int x, int y,		/* Coordinates of region in the drawable.  */
    unsigned int flags)
{
    /* 
     * Nothing to draw. The region offset starts beyond the end of the
     * picture.
     *
     *  +---------------+    	
     *  |               |    	
     *  |               |    	
     *  |               | ax,ay   
     *  |               |   +---------+
     *  |               |   |         |
     *  +---------------+   |         |
     *			    |         |
     *                      +---------+
     */			        
    if ((picture == NULL) || 
	(ax >= Blt_Picture_Width(picture)) || 
	(ay >= Blt_Picture_Height(picture))) {
	return TRUE;	
    }
    /* 
     * Correct the dimensions if the origin starts before the picture
     * (i.e. coordinate is negative).  Reset the coordinate the 0.
     *
     * ax,ay		       
     *   +---------+ 	          ax,ay		       
     *   |  +------|--------+       +------+--------+
     *   |  |      |        |       |      |        |
     *   |  |      |        |       |      |        |
     *   +--|------+        |       +------+        |
     *      |               |       |               |
     *      |               |       |               |
     *      +---------------+       +---------------+ 
     *
     */
    if (ax < 0) {		
        aw += ax;
        ax = 0;
    }
    if (ay < 0) {
        ah += ay;
        ay = 0;
    }
    /* 
     * Check that the given area does not extend beyond the end of the
     * picture.
     * 
     *    +-----------------+	     +-----------------+	  	
     *    |                 |	     |                 |	  	
     *    |      ax,ay      |	     |      ax,ay      |	  	
     *    |         +---------+      |         +-------+
     *    |         |       | |      |         |       |
     *    |         |       | |      |         |       |
     *    +---------|-------+ |      +---------+-------+
     * 	            +---------+   	           
     *                                                    
     * Clip the end of the area if it's too big.
     */
    if ((aw + ax) > Blt_Picture_Width(picture)) {
	aw = Blt_Picture_Width(picture) - ax;
    }
    if ((ah + ay) > Blt_Picture_Height(picture)) {
	ah = Blt_Picture_Height(picture) - ay;
    }
    /* Check that there's still something to paint. */
    if ((aw <= 0) || (ah <= 0)) {
	return TRUE;
    }
#ifdef notdef
    if (x < 0) {
	x = 0;
    } 
    if (y < 0) {
	y = 0;
    }
#endif
    if (Blt_Picture_IsOpaque(picture)) {
	return PaintPicture(painter, drawable, picture, ax, ay, 
		aw, ah, x, y, flags);
    } else {
	return PaintPictureWithBlend(painter, drawable, picture, ax, ay, 
		aw, ah, x, y, flags);
    }
}

int
Blt_PaintPictureWithBlend(
    Blt_Painter painter,
    Drawable drawable,
    Blt_Picture picture,
    int x, int y,		/* Coordinates of region in the picture. */
    int w, int h,		/* Dimension of the region.  Area cannot
				 * extend beyond the end of the picture. */
    int dx, int dy,		/* Coordinates of region in the drawable.  */
    unsigned int flags)		/* Indicates whether to dither the picture
				 * before displaying. */
{
    /* 
     * Nothing to draw. The selected region is outside of the picture.
     *
     *   0,0
     *    +---------+
     *    |         |
     *    | Picture |
     *	  |         |
     *    +---------+
     *              x,y
     *               +-------+
     *		     |       |
     *               |       | h
     *		     +-------+
     *			 w
     */
    if ((picture == NULL) || 
	(x >= Blt_Picture_Width(picture)) || 
	(y >= Blt_Picture_Height(picture)) ||
	((x + w) <= 0) || ((y + h) <= 0)) {
	return TRUE;	
    }
    /* 
     * Correct the dimensions if the origin starts before the picture
     * (i.e. coordinate is negative).  Reset the coordinate the 0.
     *
     * x,y		       
     *   +---------+ 	          x,y = 0,0		       
     *   |  +------|--------+       +------+--------+
     * h |  |0,0   |        |       |      |        |
     *   |  |      |        |       |      |        |
     *   +--|------+        |       +------+        |
     *    w |               |       |               |
     *      |               |       |               |
     *      +---------------+       +---------------+ 
     *
     */
    if (x < 0) {		
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }

    /* 
     * Check that the given area does not extend beyond the end of the
     * picture.
     * 
     *   0,0                        0,0
     *    +-----------------+	     +-----------------+	  	
     *    |		    |        |                 |
     *    |        x,y      |	     |        x,y      |	  	
     *    |         +---------+      |         +-------+
     *    |         |       | |      |         |       |
     *    |         |       | | w    |         |       |
     *    +---------|-------+ |      +---------+-------+
     * 	            +---------+   	           
     *                   h
     *                                                    
     * Clip the end of the area if it's too big.
     */
    if ((x + w) > Blt_Picture_Width(picture)) {
	w = Blt_Picture_Width(picture) - x;
    }
    if ((y + h) > Blt_Picture_Height(picture)) {
	h = Blt_Picture_Height(picture) - y;
    }
    if (dx < 0) {
	dx = 0;
    } 
    if (dy < 0) {
	dy = 0;
    }
    /* Check that there's still something to paint. */
    if ((w <= 0) || (h <= 0)) {
	return TRUE;
    }
    return PaintPictureWithBlend(painter, drawable, picture, x, y, w, h, dx, dy,
	flags);
}


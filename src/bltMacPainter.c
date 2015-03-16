/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltMacPainter.c --
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
#include "bltPicture.h"
#include "bltPainter.h"
#include <X11/Xutil.h>
#include "tkDisplay.h"

typedef struct _Blt_Picture Picture;

#include <Carbon/Carbon.h>

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
    GWorldPtr srcPort;
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

    destWidth = Blt_Picture_Width(pict);
    destHeight = Blt_Picture_Height(pict);
    destBytesPerRow = ((destWidth + 31) & ~31) / 8;
    destBits = Blt_AssertCalloc(destHeight, destBytesPerRow);
    count = 0;
    sp = Blt_Picture_Bits(pict);
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
	bm.bmWidth = Blt_Picture_Width(pict);
	bm.bmHeight = Blt_Picture_Height(pict);
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
 * Blt_ScaleRotateBitmapArea --
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
Blt_ScaleRotateBitmapArea(
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

    jpgProps.DIBBytes = (BYTE *)Blt_Picture_Bits(pict);
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
    dp = Blt_Picture_Bits(pict);

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

void
DrawCGImage(
    Drawable d,
    GC gc,
    CGContextRef context,
    CGImageRef image,
    unsigned long imageForeground,
    unsigned long imageBackground,
    CGRect imageBounds,
    CGRect srcBounds,
    CGRect dstBounds)
{
    MacDrawable *macDraw = (MacDrawable *) d;

    if (macDraw && context && image) {
	CGImageRef subImage = NULL;

	if (!CGRectEqualToRect(imageBounds, srcBounds)) {
	    if (!CGRectContainsRect(imageBounds, srcBounds)) {
		TkMacOSXDbgMsg("Mismatch of sub CGImage bounds");


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
 *	Returns TRUE if the picture was successfully display, Otherwise FALSE
 *	is returned if the particular combination visual and image depth is
 *	not handled.
 *
 *---------------------------------------------------------------------------
 */
static int
PaintPicture(
    Painter *p,
    Drawable drawable,
    Picture *srcPtr,
    int sx, int sy,			/* Coordinates of region in the
					 * picture. */
    int w, int h,			/* Dimension of the region.  Area
					 * cannot extend beyond the end of the
					 * picture. */
    int dx, int dy,			/* Coordinates of region in the
					 * drawable.  */
    unsigned int flags)

{   
    Picture *ditherPtr;
    TkMacOSXDrawingContext dc;

    if (!TkMacOSXSetupDrawingContext(drawable, p->gc, 1, &dc)) {
	return;
    }
    ditherPtr = NULL;
    if (flags & BLT_PAINTER_DITHER) {
	ditherPtr = Blt_DitherPicture(srcPtr, p->palette);
	if (ditherPtr != NULL) {
	    srcPtr = ditherPtr;
	}
    }
    {
	CFImageRef image;
	CGColorSpaceRef cs;
	size_t numBytes, bytesPerRow;
	size_t numBytes;
	unsigned int flags;
	char *bits;

	numBytes = srcPtr->pixelsPerRow * srcPtr->height;
	flags = kCGImageAlphaNoneSkipFirst;
#ifdef WORDS_BIGENDIAN
	flags |= kCGBitmapByteOrder32Big;
#else
	flags |= kCGBitmapByteOrder32Little;
#endif
	bits = Blt_AssetMalloc(numBytes);
	memcpy(data, srcPtr->bits, numBytes);
	provider = CGDataProviderCreateWithData(bits, bits, numBytes, 
		releaseData);
	if (provider == NULL) {
	    return;
	}
	cs = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	if (cs == NULL) {
	    return;
	}
	bytesPerRow = srcPtr->pixelsPerRow * sizeof(Blt_Pixel);
	image = CGImageCreate(srcPtr->width, srcPtr->height, 8, 32, bytesPerRow,
		cs, flags, provider, decode, 0, kCGRenderingIntentDefault);
	CFRelease(cs);
	{ 
	    GCRect imgRect, srcRect, dstRect;

	    imgRect = CGRectMake(0, 0, srcPtr->width, srcPtr->height);
	    srcRect = CGRectMake(sx, sy, w, h);
	    dstRect = CGRectMake(dx, dy, w, h);
	    DrawCGImage(drawable, p->gc, dc.context, img, 
			p->gc->foreground, p->gc->background,
			imgRect, srcRect, dstRect);
	}
	CFRelease(img);
    }
    TkMacOSXRestoreDrawingContext(&dc);
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
    int width, int height,	/* Dimension of the region.  Area
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
    Blt_BlendRegion(bg, fg, x, y, bg->width, bg->height, 0, 0);
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
    int width, int height,	/* Dimension of the region.  Area
				 * cannot extend beyond the end of the
				 * picture. */
    int destX, int destY,	/* Coordinates of region in the
				 * drawable.  */
    unsigned int flags)
{
    if ((picture == NULL) || (x >= Blt_Picture_Width(picture)) || 
	(y >= Blt_Picture_Height(picture))) {
	/* Nothing to draw. The region offset starts beyond the end of
	 * the picture. */
	return TRUE;	
    }
    if ((width + x) > Blt_Picture_Width(picture)) {
	width = Blt_Picture_Width(picture) - x;
    }
    if ((height + y) > Blt_Picture_Height(picture)) {
	height = Blt_Picture_Height(picture) - y;
    }
    if ((width <= 0) || (height <= 0)) {
	return TRUE;
    }
    if (Blt_Picture_IsOpaque(picture)) {
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
    int width, int height,	/* Dimension of the region.  Area
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

    if ((x >= Blt_Picture_Width(picture)) || (y >= Blt_Picture_Height(picture))){
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
    if ((width + x) > Blt_Picture_Width(picture)) {
	width = Blt_Picture_Width(picture) - x;
    }
    if ((height + y) > Blt_Picture_Height(picture)) {
	height = Blt_Picture_Height(picture) - y;
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

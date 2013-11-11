/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltWinBitmap.c --
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
#include "bltMath.h"
#include "bltAlloc.h"
#include "bltBitmap.h"
#include "bltPicture.h"
#include "bltPictInt.h"
#include "bltBg.h"
#include "bltPainter.h"
#include "bltWinPainter.h"
#include "tkDisplay.h"

#define WINDEBUG 0

#define GetBit(x, y) \
   srcBits[(srcBytesPerRow * (srcHeight - y - 1)) + (x>>3)] & (0x80 >> (x&7))
#define SetBit(x, y) \
   destBits[(destBytesPerRow * (destHeight - y - 1)) + (x>>3)] |= (0x80 >>(x&7))

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

#ifdef notdef
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
#endif

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

    angle = FMOD(angle, 360.0f);
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
	double theta, sinTheta, cosTheta;
	double srcCX, srcCY;	/* Center of source rectangle */
	double destCX, destCY;	/* Center of destination rectangle */
	double tx, ty;
	double rx, ry;		/* Angle of rotation for x and y coordinates */

	theta = angle * DEG2RAD;
	sinTheta = sin(theta), cosTheta = cos(theta);

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

    angle = FMOD(angle, 360.0f);
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
	double theta, sinTheta, cosTheta;
	double scx, scy; 	/* Offset from the center of the
				 * source rectangle. */
	double rcx, rcy; 	/* Offset to the center of the
				 * rotated rectangle. */
	int y;

	theta = angle * DEG2RAD;
	sinTheta = sin(theta), cosTheta = cos(theta);

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



/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltUnixBitmap.c --
 *
 * This module implements X11-specific bitmap processing procedures for the
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
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include "bltAlloc.h"
#include "bltMath.h"
#include "bltHash.h"
#include "bltBitmap.h"
#include "bltPicture.h"
#include "bltPictInt.h"

typedef struct BitmapMaster {
    Tk_ImageMaster tkMaster;	/* Tk's token for image master. NULL means the
				 * image is being deleted. */
    Tcl_Interp *interp;		/* Interpreter for application that is using
				 * image. */
    Tcl_Command imageCmd;	/* Token for image command (used to delete it
				 * when the image goes away). NULL means the
				 * image command has already been deleted. */
    int width, height;		/* Dimensions of image. */
    char *data;			/* Data comprising bitmap (suitable for input
				 * to XCreateBitmapFromData). May be NULL if
				 * no data. Malloc'ed. */
    char *maskData;		/* Data for bitmap's mask (suitable for input
				 * to XCreateBitmapFromData). Malloc'ed. */
    Tk_Uid fgUid;		/* Value of -foreground option (malloc'ed). */
    Tk_Uid bgUid;		/* Value of -background option (malloc'ed). */
    char *fileString;		/* Value of -file option (malloc'ed). */
    char *dataString;		/* Value of -data option (malloc'ed). */
    char *maskFileString;	/* Value of -maskfile option (malloc'ed). */
    char *maskDataString;	/* Value of -maskdata option (malloc'ed). */
    struct BitmapInstance *instancePtr;
				/* First in list of all instances associated
				 * with this master. */
} BitmapMaster;


#define ROTATE_0        0
#define ROTATE_90       1
#define ROTATE_180      2
#define ROTATE_270      3

Pixmap
Blt_PhotoImageMask(Tk_Window tkwin, Tk_PhotoImageBlock src)
{
    Pixmap bitmap;
    int arraySize, bytes_per_line;
    int offset, count;
    int y;
    unsigned char *bits;
    unsigned char *dp;

    bytes_per_line = (src.width + 7) / 8;
    arraySize = src.height * bytes_per_line;
    bits = Blt_AssertMalloc(sizeof(unsigned char) * arraySize);
    dp = bits;
    offset = count = 0;
    for (y = 0; y < src.height; y++) {
        int value, bitMask;
        int x;
        unsigned char *sp;

        value = 0, bitMask = 1;
        sp = src.pixelPtr + offset;
        for (x = 0; x < src.width; /*empty*/ ) {
            unsigned long pixel;

            pixel = (sp[src.offset[3]] != 0x00);
            if (pixel) {
                value |= bitMask;
            } else {
                count++;        /* Count the number of transparent pixels. */
            }
            bitMask <<= 1;
            x++;
            if (!(x & 7)) {
                *dp++ = (unsigned char)value;
                value = 0, bitMask = 1;
            }
            sp += src.pixelSize;
        }
        if (x & 7) {
            *dp++ = (unsigned char)value;
        }
        offset += src.pitch;
    }
    if (count > 0) {
        Tk_MakeWindowExist(tkwin);
        bitmap = XCreateBitmapFromData(Tk_Display(tkwin), Tk_WindowId(tkwin),
            (char *)bits, (unsigned int)src.width, (unsigned int)src.height);
    } else {
        bitmap = None;          /* Image is opaque. */
    }
    Blt_Free(bits);
    return bitmap;
}

#ifdef notdef
Pixmap
Blt_PictureMask(Tk_Window tkwin, Picture *srcPtr)
{
    Blt_Pixel *srcRowPtr;
    Pixmap bitmap;
    int bytesPerLine;
    int count;
    int x, y;
    unsigned char *bits;
    unsigned char *destRowPtr;

    bytesPerLine = (srcPtr->width + 7) / 8;
    bits = Blt_AssertMalloc(sizeof(unsigned char)*srcPtr->height*bytesPerLine);
    count = 0;
    srcRowPtr = srcPtr->bits;
    destRowPtr = bits;
    for (y = 0; y < srcPtr->height; y++) {
        int value, bitMask;
        Blt_Pixel *sp;
        unsigned char *dp;

        sp = srcRowPtr, dp = destRowPtr;
        value = 0, bitMask = 1;
        for (x = 0; x < srcPtr->width; /*empty*/ ) {
            unsigned long pixel;

            pixel = (sp->Alpha != ALPHA_TRANSPARENT);
            if (pixel) {
                value |= bitMask;
            } else {
                count++;        /* Count the number of transparent pixels. */
            }
            bitMask <<= 1;
            x++;
            if (!(x & 7)) {
                *dp++ = (unsigned char)value;
                value = 0, bitMask = 1;
            }
            sp++;
        }
        if (x & 7) {
            *dp++ = (unsigned char)value;
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += bytesPerLine;
    }
    if (count > 0) {
        Tk_MakeWindowExist(tkwin);
        bitmap = XCreateBitmapFromData(Tk_Display(tkwin), Tk_WindowId(tkwin),
                (char *)bits, (unsigned int)srcPtr->width, 
                (unsigned int)srcPtr->height);
    } else {
        bitmap = None;          /* Image is opaque. */
    }
    Blt_Free(bits);
    return bitmap;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * Blt_RotateBitmap --
 *
 *      Creates a new bitmap containing the rotated image of the given bitmap.
 *      We also need a special GC of depth 1, so that we do not need to rotate
 *      more than one plane of the bitmap.
 *
 * Results:
 *      Returns a new bitmap containing the rotated image.
 *
 *---------------------------------------------------------------------------
 */
Pixmap
Blt_RotateBitmap(
    Tk_Window tkwin,
    Pixmap srcBitmap,           /* Source bitmap to be rotated */
    int srcWidth, int srcHeight, /* Width and height of the source bitmap */
    float angle,                /* # of degrees to rotate the bitmap. */
    int *destWidthPtr, 
    int *destHeightPtr)
{
    Display *display;           /* X display */
    GC bitmapGC;
    Pixmap destBitmap;
    Window root;                /* Root window drawable */
    XImage *srcImgPtr, *destImgPtr;
    double rotWidth, rotHeight;
    int destWidth, destHeight;

    display = Tk_Display(tkwin);
    root = Tk_RootWindow(tkwin);

    /* Create a bitmap and image big enough to contain the rotated text */
    Blt_GetBoundingBox(srcWidth, srcHeight, angle, &rotWidth, &rotHeight,
        (Point2d *)NULL);
    destWidth = ROUND(rotWidth);
    destHeight = ROUND(rotHeight);
    destBitmap = Blt_GetPixmap(display, root, destWidth, destHeight, 1);
    bitmapGC = Blt_GetBitmapGC(tkwin);
    XSetForeground(display, bitmapGC, 0x0);
    XFillRectangle(display, destBitmap, bitmapGC, 0, 0, destWidth, destHeight);

    srcImgPtr = XGetImage(display, srcBitmap, 0, 0, srcWidth, srcHeight, 1, 
        ZPixmap);
    destImgPtr = XGetImage(display, destBitmap, 0, 0, destWidth, destHeight, 
        1, ZPixmap);
    angle = FMOD(angle, 360.0);
    if (FMOD(angle, (double)90.0) == 0.0) {
        int quadrant;
        int y;

        /* Handle right-angle rotations specifically */

        quadrant = (int)(angle / 90.0);
        switch (quadrant) {
        case ROTATE_270:        /* 270 degrees */
            for (y = 0; y < destHeight; y++) {
                int x, sx;

                sx = y;
                for (x = 0; x < destWidth; x++) {
                    int sy;
                    unsigned long pixel;
                    
                    sy = destWidth - x - 1;
                    pixel = XGetPixel(srcImgPtr, sx, sy);
                    if (pixel) {
                        XPutPixel(destImgPtr, x, y, pixel);
                    }
                }
            }
            break;

        case ROTATE_180:        /* 180 degrees */
            for (y = 0; y < destHeight; y++) {
                int x, sy;

                sy = destHeight - y - 1;
                for (x = 0; x < destWidth; x++) {
                    int sx;
                    unsigned long pixel;

                    sx = destWidth - x - 1, 
                    pixel = XGetPixel(srcImgPtr, sx, sy);
                    if (pixel) {
                        XPutPixel(destImgPtr, x, y, pixel);
                    }
                }
            }
            break;

        case ROTATE_90:         /* 90 degrees */
            for (y = 0; y < destHeight; y++) {
                int x, sx;

                sx = destHeight - y - 1;
                for (x = 0; x < destWidth; x++) {
                    int sy;
                    unsigned long pixel;

                    sy = x;
                    pixel = XGetPixel(srcImgPtr, sx, sy);
                    if (pixel) {
                        XPutPixel(destImgPtr, x, y, pixel);
                    }
                }
            }
            break;

        case ROTATE_0:          /* 0 degrees */
            for (y = 0; y < destHeight; y++) {
                int x;

                for (x = 0; x < destWidth; x++) {
                    unsigned long pixel;

                    pixel = XGetPixel(srcImgPtr, x, y);
                    if (pixel) {
                        XPutPixel(destImgPtr, x, y, pixel);
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
        double sox, soy;        /* Offset from the center of the source
                                 * rectangle. */
        double destCX, destCY;  /* Offset to the center of the destination
                                 * rectangle. */
        int y;

        theta = angle * DEG2RAD;
        sinTheta = sin(theta), cosTheta = cos(theta);

        /* Coordinates of the centers of source and destination rectangles */
        sox = srcWidth * 0.5;
        soy = srcHeight * 0.5;
        destCX = destWidth * 0.5;
        destCY = destHeight * 0.5;

        /* For each pixel of the destination image, transform back to the
         * associated pixel in the source image. */

        for (y = 0; y < destHeight; y++) {
            double ty;
            int x;

            ty = y - destCY;
            for (x = 0; x < destWidth; x++) {
                double tx, rx, ry, sx, sy;
                unsigned long pixel;

                /* Translate origin to center of destination image. */
                tx = x - destCX;

                /* Rotate the coordinates about the origin. */
                rx = (tx * cosTheta) - (ty * sinTheta);
                ry = (tx * sinTheta) + (ty * cosTheta);

                /* Translate back to the center of the source image. */
                rx += sox;
                ry += soy;

                sx = ROUND(rx);
                sy = ROUND(ry);

                /*
                 * Verify the coordinates, since the destination image can be
                 * bigger than the source.
                 */

                if ((sx >= srcWidth) || (sx < 0) || (sy >= srcHeight) ||
                    (sy < 0)) {
                    continue;
                }
                pixel = XGetPixel(srcImgPtr, sx, sy);
                if (pixel) {
                    XPutPixel(destImgPtr, x, y, pixel);
                }
            }
        }
    }
    /* Write the rotated image into the destination bitmap. */
    XPutImage(display, destBitmap, bitmapGC, destImgPtr, 0, 0, 0, 0, 
        destWidth, destHeight);

    /* Clean up the temporary resources used. */
    XDestroyImage(srcImgPtr), XDestroyImage(destImgPtr);
    *destWidthPtr = destWidth;
    *destHeightPtr = destHeight;
    return destBitmap;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ScaleBitmap --
 *
 *      Creates a new scaled bitmap from another bitmap. The new bitmap is
 *      bounded by a specified region. Only this portion of the bitmap is
 *      scaled from the original bitmap.
 *
 *      By bounding scaling to a region we can generate a new bitmap which is
 *      no bigger than the specified viewport.
 *
 * Results:
 *      The new scaled bitmap is returned.
 *
 * Side Effects:
 *      A new pixmap is allocated. The caller must release this.
 *
 *---------------------------------------------------------------------------
 */
Pixmap
Blt_ScaleBitmap(
    Tk_Window tkwin,
    Pixmap srcBitmap,
    int srcWidth, int srcHeight, 
    int destWidth, int destHeight)
{
    Display *display;
    GC bitmapGC;
    Pixmap destBitmap;
    Window root;
    XImage *srcImgPtr, *destImgPtr;
    double xScale, yScale;
    int y;              /* Destination bitmap coordinates */

    /* Create a new bitmap the size of the region and clear it */

    display = Tk_Display(tkwin);

    root = Tk_RootWindow(tkwin);
    destBitmap = Blt_GetPixmap(display, root, destWidth, destHeight, 1);
    bitmapGC = Blt_GetBitmapGC(tkwin);
    XSetForeground(display, bitmapGC, 0x0);
    XFillRectangle(display, destBitmap, bitmapGC, 0, 0, destWidth, destHeight);

    srcImgPtr = XGetImage(display, srcBitmap, 0, 0, srcWidth, srcHeight, 1, 
        ZPixmap);
    destImgPtr = XGetImage(display, destBitmap, 0, 0, destWidth, destHeight, 
        1, ZPixmap);

    /*
     * Scale each pixel of destination image from results of source
     * image. Verify the coordinates, since the destination image can be
     * bigger than the source
     */
    xScale = (double)srcWidth / (double)destWidth;
    yScale = (double)srcHeight / (double)destHeight;

    /* Map each pixel in the destination image back to the source. */
    for (y = 0; y < destHeight; y++) {
        int x, sy;

        sy = (int)(yScale * (double)y);
        for (x = 0; x < destWidth; x++) {
            int sx;
            unsigned long pixel;

            sx = (int)(xScale * (double)x);
            pixel = XGetPixel(srcImgPtr, sx, sy);
            if (pixel) {
                XPutPixel(destImgPtr, x, y, pixel);
            }
        }
    }
    /* Write the scaled image into the destination bitmap */

    XPutImage(display, destBitmap, bitmapGC, destImgPtr, 0, 0, 0, 0, 
        destWidth, destHeight);
    XDestroyImage(srcImgPtr), XDestroyImage(destImgPtr);
    return destBitmap;
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_RotateScaleBitmapArea --
 *
 *      Creates a scaled and rotated bitmap from a given bitmap.  The caller
 *      also provides (offsets and dimensions) the region of interest in the
 *      destination bitmap.  This saves having to process the entire
 *      destination bitmap is only part of it is showing in the viewport.
 *
 *      This uses a simple rotation/scaling of each pixel in the destination
 *      image.  For each pixel, the corresponding pixel in the source bitmap
 *      is used.  This means that destination coordinates are first scaled to
 *      the size of the rotated source bitmap.  These coordinates are then
 *      rotated back to their original orientation in the source.
 *
 * Results:
 *      The new rotated and scaled bitmap is returned.
 *
 * Side Effects:
 *      A new pixmap is allocated. The caller must release this.
 *
 *---------------------------------------------------------------------------
 */
Pixmap
Blt_ScaleRotateBitmapArea(
    Tk_Window tkwin,
    Pixmap srcBitmap,           /* Source bitmap. */
    unsigned int srcWidth, 
    unsigned int srcHeight,     /* Size of source bitmap */
    int regionX, int regionY,   /* Offset of region in virtual destination
                                 * bitmap. */
    unsigned int regionWidth, 
    unsigned int regionHeight,  /* Desire size of bitmap region. */
    unsigned int destWidth,             
    unsigned int destHeight,    /* Virtual size of destination bitmap. */
    float angle)                /* Angle to rotate bitmap.  */
{
    Display *display;           /* X display */
    Window root;                /* Root window drawable */
    Pixmap destBitmap;
    XImage *srcImgPtr, *destImgPtr;
    double xScale, yScale;
    double rotWidth, rotHeight;
    GC bitmapGC;

    display = Tk_Display(tkwin);
    root = Tk_RootWindow(tkwin);

    /* Create a bitmap and image big enough to contain the rotated text */
    bitmapGC = Blt_GetBitmapGC(tkwin);
    destBitmap = Blt_GetPixmap(display, root, regionWidth, regionHeight, 1);
    XSetForeground(display, bitmapGC, 0x0);
    XFillRectangle(display, destBitmap, bitmapGC, 0, 0, regionWidth, 
        regionHeight);

    srcImgPtr = XGetImage(display, srcBitmap, 0, 0, srcWidth, srcHeight, 1, 
        ZPixmap);
    destImgPtr = XGetImage(display, destBitmap, 0, 0, regionWidth, 
        regionHeight, 1, ZPixmap);
    angle = FMOD(angle, 360.0);

    Blt_GetBoundingBox(srcWidth, srcHeight, angle, &rotWidth, &rotHeight,
        (Point2d *)NULL);

    xScale = rotWidth / (double)destWidth;
    yScale = rotHeight / (double)destHeight;

    if (FMOD(angle, (double)90.0) == 0.0) {
        int quadrant;
        int x, y;

        /* Handle right-angle rotations specifically */

        quadrant = (int)(angle / 90.0);
        switch (quadrant) {
        case ROTATE_270:        /* 270 degrees */
            for (y = 0; y < regionHeight; y++) {
                int sx; 

                sx = (int)(yScale * (double)(y + regionY));
                for (x = 0; x < regionWidth; x++) {
                    int sy;     
                    unsigned long pixel;

                    sy = (int)(xScale *(double)(destWidth - (x + regionX) - 1));
                    pixel = XGetPixel(srcImgPtr, sx, sy);
                    if (pixel) {
                        XPutPixel(destImgPtr, x, y, pixel);
                    }
                }
            }
            break;

        case ROTATE_180:        /* 180 degrees */
            for (y = 0; y < regionHeight; y++) {
                int sy; 

                sy = (int)(yScale * (double)(destHeight - (y + regionY) - 1));
                for (x = 0; x < regionWidth; x++) {
                    int sx;     
                    unsigned long pixel;

                    sx = (int)(xScale *(double)(destWidth - (x + regionX) - 1));
                    pixel = XGetPixel(srcImgPtr, sx, sy);
                    if (pixel) {
                        XPutPixel(destImgPtr, x, y, pixel);
                    }
                }
            }
            break;

        case ROTATE_90:         /* 90 degrees */
            for (y = 0; y < regionHeight; y++) {
                int sx; 

                sx = (int)(yScale * (double)(destHeight - (y + regionY) - 1));
                for (x = 0; x < regionWidth; x++) {
                    int sy;     
                    unsigned long pixel;

                    sy = (int)(xScale * (double)(x + regionX));
                    pixel = XGetPixel(srcImgPtr, sx, sy);
                    if (pixel) {
                        XPutPixel(destImgPtr, x, y, pixel);
                    }
                }
            }
            break;

        case ROTATE_0:          /* 0 degrees */
            for (y = 0; y < regionHeight; y++) {
                int sy;

                sy = (int)(yScale * (double)(y + regionY));
                for (x = 0; x < regionWidth; x++) {
                    int sx;     
                    unsigned long pixel;

                    sx = (int)(xScale * (double)(x + regionX));
                    pixel = XGetPixel(srcImgPtr, sx, sy);
                    if (pixel) {
                        XPutPixel(destImgPtr, x, y, pixel);
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
        double sox, soy;        /* Offset from the center of the source
                                 * rectangle. */
        double rox, roy;        /* Offset to the center of the rotated
                                 * rectangle. */
        int x, y;

        theta = angle * DEG2RAD;
        sinTheta = sin(theta), cosTheta = cos(theta);

        /*
         * Coordinates of the centers of the source and destination rectangles
         */
        sox = srcWidth * 0.5;
        soy = srcHeight * 0.5;
        rox = rotWidth * 0.5;
        roy = rotHeight * 0.5;

        /* For each pixel of the destination image, transform back to the
         * associated pixel in the source image. */

        for (y = 0; y < regionHeight; y++) {
            double ty;

            ty = (yScale * (double)(y + regionY)) - roy;
            for (x = 0; x < regionWidth; x++) {
                double tx, rx, ry;
                int sx, sy;     
                unsigned long pixel;

                /* Translate origin to center of destination image. */
                tx = (xScale * (double)(x + regionX)) - rox;

                /* Rotate the coordinates about the origin. */
                rx = (tx * cosTheta) - (ty * sinTheta);
                ry = (tx * sinTheta) + (ty * cosTheta);

                /* Translate back to the center of the source image. */
                rx += sox;
                ry += soy;

                sx = ROUND(rx);
                sy = ROUND(ry);

                /*
                 * Verify the coordinates, since the destination image can be
                 * bigger than the source.
                 */

                if ((sx >= srcWidth) || (sx < 0) || (sy >= srcHeight) ||
                    (sy < 0)) {
                    continue;
                }
                pixel = XGetPixel(srcImgPtr, sx, sy);
                if (pixel) {
                    XPutPixel(destImgPtr, x, y, pixel);
                }
            }
        }
    }
    /* Write the rotated image into the destination bitmap. */
    XPutImage(display, destBitmap, bitmapGC, destImgPtr, 0, 0, 0, 0, 
        regionWidth, regionHeight);

    /* Clean up the temporary resources used. */
    XDestroyImage(srcImgPtr), XDestroyImage(destImgPtr);
    return destBitmap;
}

Blt_Picture 
Blt_BitmapImageToPicture(Tk_Image tkImage)
{
    BitmapMaster *masterPtr;
    Blt_Picture picture;
    int bytesPerRow;
    
    masterPtr = Blt_Image_GetMasterData(tkImage);
    /* Process data and mask. */
    bytesPerRow = (masterPtr->width + 7) / 8;
    picture = Blt_CreatePicture(masterPtr->width, masterPtr->height);
    if (masterPtr->data != NULL) {
        int y;
        Blt_Pixel *destRowPtr;
        unsigned char *srcRowPtr;
        
        destRowPtr = Blt_Picture_Bits(picture);
        srcRowPtr = (unsigned char *)masterPtr->data;
        for (y = 0; y < masterPtr->height; y++) {
            int x;
            Blt_Pixel *dp;
            unsigned char *sp, *send;
            
            dp = destRowPtr;
            x = 0;
            for (sp = srcRowPtr, send = sp + bytesPerRow; sp < send; sp++) {
                int i;
                
                for (i = 0; (x < masterPtr->width) && (i < 8); i++, x++) {
                    dp->u32 = (*sp & (1<<i)) ? 0xFF000000 : 0xFFFFFFFF;
                    dp++;
                }
            }
            destRowPtr += Blt_Picture_Stride(picture);
            srcRowPtr += bytesPerRow;
        }
    }
    if (masterPtr->maskData != NULL) {
        int y;
        Blt_Pixel *destRowPtr;
        unsigned char *srcRowPtr;
        
        destRowPtr = Blt_Picture_Bits(picture);
        srcRowPtr = (unsigned char *)masterPtr->maskData;
        for (y = 0; y < masterPtr->height; y++) {
            int x;
            Blt_Pixel *dp;
            unsigned char *sp, *send;
            
            dp = destRowPtr;
            x = 0;
            for (sp = srcRowPtr, send = sp + bytesPerRow; sp < send; sp++) {
                int i;
                
                for (i = 0; (x < masterPtr->width) && (i < 8); i++, x++) {
                    dp->Alpha = (*sp & (1<<i)) ? 0xFF : 0x00;
                    dp++;
                }
            }
            destRowPtr += Blt_Picture_Stride(picture);
            srcRowPtr += bytesPerRow;
        }
    }
    return picture;
}

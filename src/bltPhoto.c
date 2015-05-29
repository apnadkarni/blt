/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPhoto.c --
 *
 * This module implements photo-to-picture conversion routines for the BLT
 * toolkit.
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
 */


#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include "bltPicture.h"
#include "bltImage.h"
#include "bltPictInt.h"
#include "bltPhoto.h"

#define imul8x8(a,b,t)  ((t) = (a)*(b)+128,(((t)+((t)>>8))>>8))

#if (_TK_VERSION >= _VERSION(8,5,0)) 
/*
 *---------------------------------------------------------------------------
 *
 * Blt_PictureToPhoto --
 *
 *      Translates a picture into a Tk photo.
 *
 * Results:
 *      The photo is re-written with the new picture.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_PictureToPhoto(Pict *srcPtr, Tk_PhotoHandle photo)
{
    Tk_PhotoImageBlock dest;            /* Destination image block. */
    int result, flags;

    Tk_PhotoGetImage(photo, &dest);
    dest.pixelSize = sizeof(Blt_Pixel);
    dest.pitch = sizeof(Blt_Pixel) * srcPtr->pixelsPerRow;
    dest.width = srcPtr->width;
    dest.height = srcPtr->height;
    dest.offset[0] = Blt_Offset(Blt_Pixel, Red);
    dest.offset[1] = Blt_Offset(Blt_Pixel, Green);
    dest.offset[2] = Blt_Offset(Blt_Pixel, Blue);
    dest.offset[3] = Blt_Offset(Blt_Pixel, Alpha); 
    flags = TK_PHOTO_COMPOSITE_SET;
    result = Tk_PhotoSetSize(NULL, photo, srcPtr->width, srcPtr->height);
    if (srcPtr->flags & BLT_PIC_ASSOCIATED_COLORS) {
        Pict *tmpPtr;
        
        /* Divide out the alphas from picture's pre-multipled RGB values. */
        tmpPtr = Blt_ClonePicture(srcPtr);
        Blt_UnassociateColors(tmpPtr);
        dest.pixelPtr = (unsigned char *)tmpPtr->bits;
        if (result != TCL_OK) {
            result = Tk_PhotoSetSize(NULL, photo, tmpPtr->width, 
                                     tmpPtr->height);
        }
        if (result != TCL_OK) {
            result = Tk_PhotoPutBlock(NULL, photo, &dest, 0, 0, 
                                      tmpPtr->width, tmpPtr->height, flags);
        }
        Blt_FreePicture(tmpPtr);
    } else {
        dest.pixelPtr = (unsigned char *)srcPtr->bits;
        if (result != TCL_OK) {
            result = Tk_PhotoPutBlock(NULL, photo, &dest, 0, 0, 
                                      srcPtr->width, srcPtr->height, flags);
        }
    }
}
#else
/*
 *---------------------------------------------------------------------------
 *
 * Blt_PictureToPhoto --
 *
 *      Translates a picture into a Tk photo.
 *
 * Results:
 *      The photo is re-written with the new picture.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_PictureToPhoto(Pict *srcPtr, Tk_PhotoHandle photo)
{
    Tk_PhotoImageBlock dest;            /* Destination image block. */

    Tk_PhotoGetImage(photo, &dest);
    dest.pixelSize = sizeof(Blt_Pixel);
    dest.pitch = sizeof(Blt_Pixel) * srcPtr->pixelsPerRow;
    dest.width = srcPtr->width;
    dest.height = srcPtr->height;
    dest.offset[0] = Blt_Offset(Blt_Pixel, Red);
    dest.offset[1] = Blt_Offset(Blt_Pixel, Green);
    dest.offset[2] = Blt_Offset(Blt_Pixel, Blue);
    dest.offset[3] = Blt_Offset(Blt_Pixel, Alpha); 
    Tk_PhotoSetSize(photo, srcPtr->width, srcPtr->height);
    if (srcPtr->flags & BLT_PIC_ASSOCIATED_COLORS) {
        Pict *tmpPtr;

        /* Divide out the alphas from picture's pre-multipled RGB values. */
        tmpPtr = Blt_ClonePicture(srcPtr);
        Blt_UnassociateColors(tmpPtr);
        dest.pixelPtr = (unsigned char *)tmpPtr->bits;
        Tk_PhotoSetSize(photo, tmpPtr->width, tmpPtr->height);
        Tk_PhotoPutBlock(photo, &dest, 0, 0, tmpPtr->width, tmpPtr->height);
        Blt_FreePicture(tmpPtr);
    } else {
        dest.pixelPtr = (unsigned char *)srcPtr->bits;
        Tk_PhotoPutBlock(photo, &dest, 0, 0, srcPtr->width, srcPtr->height);
    }
}
#endif  /* < 8.5 */

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PhotoAreaToPicture --
 *
 *      Create a picture from a region in a photo image.
 *
 * Results:
 *      The new picture is returned.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture
Blt_PhotoAreaToPicture(Tk_PhotoHandle photo, int x, int y, int w, int h)
{
    Tk_PhotoImageBlock src;             /* Source image block. */
    Pict *destPtr;
    int offset;
    int ir, ib, ig, ia;

    if (x < 0) {
        x = 0;
    } 
    if (y < 0) {
        y = 0;
    }
    Tk_PhotoGetImage(photo, &src);
    if (w < 0) {
        w = src.width;
    }
    if (h < 0) {
        h = src.height;
    }
    if ((x + w) > src.width) {
        w = src.width - x;
    }
    if ((h + y) > src.height) {
        h = src.width - y;
    }
    offset = (x * src.pixelSize) + (y * src.pitch);

    destPtr = Blt_CreatePicture(w, h);
    ir = src.offset[0];
    ig = src.offset[1];
    ib = src.offset[2];
    ia = src.offset[3];

    if (src.pixelSize == 4) {
        Blt_Pixel *destRowPtr;
        int x, y;

        destRowPtr = destPtr->bits;
        for (y = 0; y < h; y++) {
            Blt_Pixel *dp;
            unsigned char *bits;

            dp = destRowPtr;
            bits = src.pixelPtr + offset;
            for (x = 0; x < w; x++) {

                dp->Alpha = bits[ia];

                if (dp->Alpha == 0xFF) {
                    dp->Red = bits[ir];
                    dp->Green = bits[ig];
                    dp->Blue = bits[ib];
                } else if (dp->Alpha == 0x00) {
                    dp->Red = bits[ir];
                    dp->Green = bits[ig];
                    dp->Blue = bits[ib];
                    destPtr->flags |= BLT_PIC_MASK;
                } else {
                    int t;

                    /* 
                     * Premultiple the alpha into each component. 
                     * (0..255 * 0..255) / 255.0 
                     */
                    dp->Red = imul8x8(dp->Alpha, bits[ir], t);
                    dp->Green = imul8x8(dp->Alpha, bits[ig], t);
                    dp->Blue = imul8x8(dp->Alpha, bits[ib], t);
                    destPtr->flags |= 
                        (BLT_PIC_BLEND | BLT_PIC_ASSOCIATED_COLORS);
                }
                bits += src.pixelSize;
                dp++;
            }
            offset += src.pitch;
            destRowPtr += destPtr->pixelsPerRow;
        }
    } else if (src.pixelSize == 3) {
        Blt_Pixel *destRowPtr;
        int x, y;

        destRowPtr = destPtr->bits;
        for (y = 0; y < h; y++) {
            Blt_Pixel *dp;
            unsigned char *bits;

            dp = destRowPtr;
            bits = src.pixelPtr + offset;
            for (x = 0; x < w; x++) {
                dp->Red = bits[ir];
                dp->Green = bits[ig];
                dp->Blue = bits[ib];
                dp->Alpha = ALPHA_OPAQUE;
                bits += src.pixelSize;
                dp++;
            }
            offset += src.pitch;
            destRowPtr += destPtr->pixelsPerRow;
        }
    } else {
        Blt_Pixel *destRowPtr;
        int x, y;

        destRowPtr = destPtr->bits;
        for (y = 0; y < h; y++) {
            Blt_Pixel *dp;
            unsigned char *bits;

            dp = destRowPtr;
            bits = src.pixelPtr + offset;
            for (x = 0; x < w; x++) {
                dp->Red = dp->Green = dp->Blue = bits[ir];
                dp->Alpha = ALPHA_OPAQUE;
                bits += src.pixelSize;
                dp++;
            }
            offset += src.pitch;
            destRowPtr += destPtr->pixelsPerRow;
        }
    } 
    return destPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PhotoToPicture --
 *
 *      Create a picture from a photo image.
 *
 * Results:
 *      The new picture is returned.
 *
 *---------------------------------------------------------------------------
 */
Blt_Picture
Blt_PhotoToPicture(Tk_PhotoHandle photo) /* Source photo to convert. */
{
    Pict *destPtr;
    Tk_PhotoImageBlock src;             /* Source image block. */
    int bytesPerRow;
    int sw, sh;
    int ir, ib, ig, ia;

    Tk_PhotoGetImage(photo, &src);
    sw = src.width;
    sh = src.height;
    bytesPerRow = src.pixelSize * sw;
    ir = src.offset[0];
    ig = src.offset[1];
    ib = src.offset[2];
    ia = src.offset[3];

    destPtr = Blt_CreatePicture(sw, sh);
    if (src.pixelSize == 4) {
        Blt_Pixel *destRowPtr;
        int y;
        unsigned char *srcRowPtr;
        
        srcRowPtr = src.pixelPtr;
        destRowPtr = destPtr->bits;
        for (y = 0; y < sh; y++) {
            Blt_Pixel *dp;
            unsigned char *bits, *bend;

            dp = destRowPtr;
            for (bits = srcRowPtr, bend = bits + bytesPerRow; bits < bend; 
                bits += src.pixelSize) {
                dp->Alpha = bits[ia];
                if (dp->Alpha == 0xFF) {
                    dp->Red = bits[ir];
                    dp->Green = bits[ig];
                    dp->Blue = bits[ib];
                } else if (dp->Alpha == 0x00) {
                    dp->Red = bits[ir];
                    dp->Green = bits[ig];
                    dp->Blue = bits[ib];
                    destPtr->flags |= BLT_PIC_MASK;
                } else {
                    dp->Red = bits[ir];
                    dp->Green = bits[ig];
                    dp->Blue = bits[ib];
                    destPtr->flags |= BLT_PIC_BLEND;
                }
                dp++;
            }
            srcRowPtr += src.pitch;
            destRowPtr += destPtr->pixelsPerRow;
        }
    } else if (src.pixelSize == 3) {
        Blt_Pixel *destRowPtr;
        int y;
        unsigned char *srcRowPtr;

        srcRowPtr = src.pixelPtr;
        destRowPtr = destPtr->bits;
        for (y = 0; y < sh; y++) {
            Blt_Pixel *dp;
            unsigned char *bits, *bend;

            dp = destRowPtr;
            for (bits = srcRowPtr, bend = bits + bytesPerRow; bits < bend; 
                bits += src.pixelSize) {
                dp->Red = bits[ir];
                dp->Green = bits[ig];
                dp->Blue = bits[ib];
                dp->Alpha = ALPHA_OPAQUE;
                dp++;
            }
            srcRowPtr += src.pitch;
            destRowPtr += destPtr->pixelsPerRow;
        }
    } else {
        Blt_Pixel *destRowPtr;
        int y;
        unsigned char *srcRowPtr;

        srcRowPtr = src.pixelPtr;
        destRowPtr = destPtr->bits;
        for (y = 0; y < sh; y++) {
            Blt_Pixel *dp;
            unsigned char *bits, *bend;

            dp = destRowPtr;
            for (bits = srcRowPtr, bend = bits + bytesPerRow; bits < bend; 
                bits += src.pixelSize) {
                dp->Red = dp->Green = dp->Blue = bits[ir];
                dp->Alpha = ALPHA_OPAQUE;
                dp++;
            }
            srcRowPtr += src.pitch;
            destRowPtr += destPtr->pixelsPerRow;
        }
    } 
    destPtr->flags &= ~BLT_PIC_UNINITIALIZED;
    return destPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_SnapPhoto --
 *
 *      Takes a snapshot of an X drawable (pixmap or window) and writes it to
 *      an existing Tk photo image.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      The named Tk photo is updated with the snapshot.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_SnapPhoto(
    Tcl_Interp *interp,                 /* Interpreter to report errors back
                                         * to */
    Tk_Window tkwin,
    Drawable drawable,                  /* Window or pixmap to be snapped */
    int x, int y,                       /* Offset of image from drawable
                                         * origin. */
    int width, int height,              /* Dimension of the drawable */
    int dw, int dh,                     /* Desired size of the destination Tk
                                         * photo. */
    const char *photoName,              /* Name of a current Tk photo image. */
    float gamma)
{
    Tk_PhotoHandle photo;               /* The photo image to write into. */
    Blt_Picture pict;

    photo = Tk_FindPhoto(interp, photoName);
    if (photo == NULL) {
        Tcl_AppendResult(interp, "can't find photo \"", photoName, "\"", 
                (char *)NULL);
        return TCL_ERROR;
    }
    pict = Blt_DrawableToPicture(tkwin, drawable, x, y, width, height, gamma);
    if (pict == NULL) {
        Tcl_AppendResult(interp,
            "can't grab window or pixmap (possibly obscured?)", (char *)NULL);
        return TCL_ERROR;               /* Can't grab window image */
    }
    if ((dw != width) || (dh != height)) {
        Blt_Picture dest;

        /*
         * The requested size for the destination image is different than that
         * of the source snapshot.  Resample the image as necessary.  We'll
         * use a cheap box filter. I'm assuming that the destination image
         * will typically be smaller than the original.
         */
        dest = Blt_CreatePicture(dw, dh);
        Blt_ResamplePicture(dest, pict, bltBoxFilter, bltBoxFilter);
        Blt_FreePicture(pict);
        pict = dest;
    }
    Blt_PictureToPhoto(pict, photo);
    Blt_FreePicture(pict);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_SnapPicture --
 *
 *      Takes a snapshot of an X drawable (pixmap or window) and writes it to
 *      an existing Tk photo image.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      The named Tk photo is updated with the snapshot.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_SnapPicture(
    Tcl_Interp *interp,                 /* Interpreter to return results. */
    Tk_Window tkwin,
    Drawable drawable,                  /* Window or pixmap to be snapped */
    int x, int y,                       /* Offset of image in drawable
                                         * origin. */
    int w, int h,                       /* Dimension of the drawable. */
    int destWidth, int destHeight,      /* Desired size of the destination
                                         * picture. */
    const char *imageName,              /* Name of a picture image. */
    float gamma)
{
    Blt_Picture dest;

    dest = Blt_DrawableToPicture(tkwin, drawable, x, y, w, h, gamma);
    if (dest == NULL) {
        Tcl_AppendResult(interp,
            "can't grab window or pixmap (possibly obscured?)", (char *)NULL);
        return TCL_ERROR;               /* Can't grab window image */
    }
    if ((destWidth != w) || (destHeight != h)) {
        Blt_Picture newPict;

        /*
         * The requested size for the destination image is different than that
         * of the source snapshot.  Resample the image as necessary.  We'll
         * use a cheap box filter. I'm assuming that the destination image
         * will typically be smaller than the original.
         */
        newPict = Blt_CreatePicture(destWidth, destHeight);
        Blt_ResamplePicture(newPict, dest, bltBoxFilter, bltBoxFilter);
        Blt_FreePicture(dest);
        dest = newPict;
    }
    if (Blt_ResetPicture(interp, imageName, dest) == TCL_OK) {
        return TCL_OK;
    }
    Blt_FreePicture(dest);
    return TCL_ERROR;
}


Blt_Picture
Blt_GetPictureFromPhotoImage(Tcl_Interp *interp, Tk_Image tkImage)
{
    const char *name;
    Tk_PhotoHandle photo;

    name = Blt_Image_Name(tkImage);
    photo = Tk_FindPhoto(interp, name);
    if (photo == NULL) {
        return NULL;
    }
    return Blt_PhotoToPicture(photo);
}

Blt_Picture
Blt_GetPictureFromImage(Tcl_Interp *interp, Tk_Image tkImage, int *isPhotoPtr)
{
    const char *type;
    Blt_Picture picture;
    int isPhoto;

    type = Blt_Image_NameOfType(tkImage);
    if (strcmp(type, "picture") == 0) {
        picture = Blt_GetPictureFromPictureImage(interp, tkImage);
        isPhoto = FALSE;
    } else if (strcmp(type, "photo") == 0) {
        picture = Blt_GetPictureFromPhotoImage(interp, tkImage);
        isPhoto = TRUE;
    } else {
        isPhoto = FALSE;
        if (interp != NULL) {
            Tcl_AppendResult(interp, "image is not a photo or picture",
                (char *)NULL);
        }
        return NULL;
    }
    if (isPhotoPtr != NULL) {
        *isPhotoPtr = isPhoto;
    }
    return picture;
}


/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltBitmap.c --
 *
 * This module implements TCL bitmaps for the Tk toolkit.
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
 * Much of the code is taken from XRdBitF.c and XWrBitF.c from the MIT
 * X11R5 distribution.
 *
 * Copyright, 1987, Massachusetts Institute of Technology 
 * 
 *   Permission to use, copy, modify, distribute, and sell this software
 *   and its documentation for any purpose is hereby granted without fee,
 *   provided that the above copyright notice appear in all copies and that
 *   both that copyright notice and this permission notice appear in
 *   supporting documentation, and that the name of M.I.T. not be used in
 *   advertising or publicity pertaining to distribution of the software
 *   without specific, written prior permission.  M.I.T. makes no
 *   representations about the suitability of this software for any
 *   purpose.  It is provided "as is" without express or implied warranty.
 */

/*
  Predefined table holds bitmap info (source width, height)
  Name table holds bitmap names  
  Id table hold bitmap ids
  Both id and name tables get you the actual bitmap.
 */
#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_BITMAP

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include <X11/Xutil.h>

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltHash.h"
#include "bltFont.h"
#include "bltText.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define BITMAP_THREAD_KEY       "BLT Bitmap Data"

/* 
 * BitmapInterpData --
 *
 *      Tk's routine to create a bitmap, Tk_DefineBitmap, assumes that the
 *      source (bit array) is always statically allocated.  This isn't true
 *      here (we dynamically allocate the arrays), so we have to save them
 *      in a hashtable and cleanup after the interpreter is deleted.
 */
typedef struct {
    Blt_HashTable bitmapTable;          /* Hash table of bitmap data keyed
                                         * by the name of the bitmap. */
    Tcl_Interp *interp;
    Display *display;                   /* Display of interpreter. */
    Tk_Window tkMain;                   /* Main window of interpreter. */
} BitmapInterpData;

#define MAX_SIZE 255

/* 
 * BitmapInfo --
 */
typedef struct {
    double angle;                       /* Rotation of text string */
    double scale;                       /* Scaling factor */
    Blt_Font font;                      /* Font pointer */
    Tk_Justify justify;                 /* Justify text */
    Blt_Pad padX, padY;                 /* Padding around the text */
} BitmapInfo;

/* 
 * BitmapData --
 */
typedef struct {
    int width, height;                  /* Dimension of image */
    unsigned char *bits;                /* Data array for bitmap image */
} BitmapData;

#define DEF_BITMAP_FONT         STD_FONT
#define DEF_BITMAP_PAD          "4"
#define DEF_BITMAP_ANGLE        "0.0"
#define DEF_BITMAP_SCALE        "1.0"
#define DEF_BITMAP_JUSTIFY      "center"

#define ROTATE_0        0
#define ROTATE_90       1
#define ROTATE_180      2
#define ROTATE_270      3


static Blt_ConfigSpec composeConfigSpecs[] =
{
    {BLT_CONFIG_FONT, "-font", (char *)NULL, (char *)NULL,
        DEF_BITMAP_FONT, Blt_Offset(BitmapInfo, font), 0},
    {BLT_CONFIG_JUSTIFY, "-justify", (char *)NULL, (char *)NULL,
        DEF_BITMAP_JUSTIFY, Blt_Offset(BitmapInfo, justify),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-padx", (char *)NULL, (char *)NULL,
        DEF_BITMAP_PAD, Blt_Offset(BitmapInfo, padX),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-pady", (char *)NULL, (char *)NULL,
        DEF_BITMAP_PAD, Blt_Offset(BitmapInfo, padY),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DOUBLE, "-rotate", (char *)NULL, (char *)NULL,
        DEF_BITMAP_ANGLE, Blt_Offset(BitmapInfo, angle),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DOUBLE, "-scale", (char *)NULL, (char *)NULL,
        DEF_BITMAP_SCALE, Blt_Offset(BitmapInfo, scale),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
};

static Blt_ConfigSpec defineConfigSpecs[] =
{
    {BLT_CONFIG_DOUBLE, "-rotate", (char *)NULL, (char *)NULL,
        DEF_BITMAP_ANGLE, Blt_Offset(BitmapInfo, angle),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DOUBLE, "-scale", (char *)NULL, (char *)NULL,
        DEF_BITMAP_SCALE, Blt_Offset(BitmapInfo, scale),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
};

/* Shared data for the image read/parse logic */
static unsigned char hexTable[256];     /* Conversion value */
static int initialized = 0;             /* Easier to fill in at run time */

#define blt_width 40
#define blt_height 40
static unsigned char blt_bits[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0x03, 0x00, 0x04,
    0x00, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x02, 0x00, 0xe4, 0x33, 0x3f,
    0x01, 0x00, 0x64, 0x36, 0x0c, 0x01, 0x00, 0x64, 0x36, 0x8c, 0x00, 0x00,
    0xe4, 0x33, 0x8c, 0x00, 0x00, 0x64, 0x36, 0x8c, 0x00, 0x00, 0x64, 0x36,
    0x0c, 0x01, 0x00, 0xe4, 0xf3, 0x0d, 0x01, 0x00, 0x04, 0x00, 0x00, 0x02,
    0x00, 0x04, 0x00, 0x00, 0x02, 0x00, 0xfc, 0xff, 0xff, 0x03, 0x00, 0x0c,
    0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x0c, 0xf8, 0xff,
    0x03, 0x80, 0xed, 0x07, 0x00, 0x04, 0xe0, 0x0c, 0x00, 0x20, 0x09, 0x10,
    0x0c, 0x00, 0x00, 0x12, 0x10, 0x0c, 0x00, 0x00, 0x10, 0x30, 0x00, 0x00,
    0x00, 0x19, 0xd0, 0x03, 0x00, 0x00, 0x14, 0xb0, 0xfe, 0xff, 0xff, 0x1b,
    0x50, 0x55, 0x55, 0x55, 0x0d, 0xe8, 0xaa, 0xaa, 0xaa, 0x16, 0xe4, 0xff,
    0xff, 0xff, 0x2f, 0xf4, 0xff, 0xff, 0xff, 0x27, 0xd8, 0xae, 0xaa, 0xbd,
    0x2d, 0x6c, 0x5f, 0xd5, 0x67, 0x1b, 0xbc, 0xf3, 0x7f, 0xd0, 0x36, 0xf8,
    0x01, 0x10, 0xcc, 0x1f, 0xe0, 0x45, 0x8e, 0x92, 0x0f, 0xb0, 0x32, 0x41,
    0x43, 0x0b, 0xd0, 0xcf, 0x3c, 0x7c, 0x0d, 0xb0, 0xaa, 0xc2, 0xab, 0x0a,
    0x60, 0x55, 0x55, 0x55, 0x05, 0xc0, 0xff, 0xab, 0xaa, 0x03, 0x00, 0x00,
    0xfe, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define bigblt_width 64
#define bigblt_height 64
static unsigned char bigblt_bits[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0x3f, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00,
    0x00, 0x00, 0xe2, 0x0f, 0xc7, 0xff, 0x10, 0x00, 0x00, 0x00, 0xe2, 0x1f,
    0xc7, 0xff, 0x10, 0x00, 0x00, 0x00, 0xe2, 0x38, 0x07, 0x1c, 0x08, 0x00,
    0x00, 0x00, 0xe2, 0x38, 0x07, 0x1c, 0x08, 0x00, 0x00, 0x00, 0xe2, 0x38,
    0x07, 0x1c, 0x08, 0x00, 0x00, 0x00, 0xe2, 0x1f, 0x07, 0x1c, 0x04, 0x00,
    0x00, 0x00, 0xe2, 0x1f, 0x07, 0x1c, 0x04, 0x00, 0x00, 0x00, 0xe2, 0x38,
    0x07, 0x1c, 0x08, 0x00, 0x00, 0x00, 0xe2, 0x38, 0x07, 0x1c, 0x08, 0x00,
    0x00, 0x00, 0xe2, 0x38, 0x07, 0x1c, 0x08, 0x00, 0x00, 0x00, 0xe2, 0x1f,
    0xff, 0x1c, 0x10, 0x00, 0x00, 0x00, 0xe2, 0x0f, 0xff, 0x1c, 0x10, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00,
    0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0x3f, 0x00, 0x00, 0x00, 0x06, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xc0, 0xff, 0xff, 0x07, 0x00,
    0x00, 0xe0, 0xf6, 0x3f, 0x00, 0x00, 0x38, 0x00, 0x00, 0x1c, 0x06, 0x00,
    0x00, 0x00, 0xc0, 0x00, 0x80, 0x03, 0x06, 0x00, 0x00, 0xc0, 0x08, 0x03,
    0x40, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x04, 0x40, 0x00, 0x06, 0x00,
    0x00, 0x00, 0x40, 0x04, 0x40, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x40, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x04, 0xc0, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0c, 0x06, 0x40, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0xc0, 0xfe, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x06, 0x40, 0x55, 0xff, 0xff,
    0xff, 0xff, 0x7f, 0x05, 0x80, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x06,
    0x80, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x03, 0x40, 0xab, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0x01, 0x70, 0x57, 0x55, 0x55, 0x55, 0x55, 0xd5, 0x04,
    0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0b, 0xd8, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x14, 0xd0, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0x13,
    0xf0, 0xda, 0xbf, 0xaa, 0xba, 0xfd, 0xd6, 0x0b, 0x70, 0xed, 0x77, 0x55,
    0x57, 0xe5, 0xad, 0x07, 0xb8, 0xf7, 0xab, 0xaa, 0xaa, 0xd2, 0x5b, 0x0f,
    0xf8, 0xfb, 0x54, 0x55, 0x75, 0x94, 0xf7, 0x1e, 0xf0, 0x7b, 0xfa, 0xff,
    0x9f, 0xa9, 0xef, 0x1f, 0xc0, 0xbf, 0x00, 0x20, 0x40, 0x54, 0xfe, 0x0f,
    0x00, 0x1f, 0x92, 0x00, 0x04, 0xa9, 0xfc, 0x01, 0xc0, 0x5f, 0x41, 0xf9,
    0x04, 0x21, 0xfd, 0x00, 0xc0, 0x9b, 0x28, 0x04, 0xd8, 0x0a, 0x9a, 0x03,
    0x40, 0x5d, 0x08, 0x40, 0x44, 0x44, 0x62, 0x03, 0xc0, 0xaa, 0x67, 0xe2,
    0x03, 0x64, 0xba, 0x02, 0x40, 0x55, 0xd5, 0x55, 0xfd, 0xdb, 0x55, 0x03,
    0x80, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x01, 0x00, 0x57, 0x55, 0x55,
    0x55, 0x55, 0xd5, 0x00, 0x00, 0xac, 0xaa, 0xaa, 0xaa, 0xaa, 0x2a, 0x00,
    0x00, 0xf0, 0xff, 0x57, 0x55, 0x55, 0x1d, 0x00, 0x00, 0x00, 0x00, 0xf8,
    0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static Tcl_ObjCmdProc BitmapCmdProc;
static Tcl_InterpDeleteProc BitmapInterpDeleteProc;

/*
 *---------------------------------------------------------------------------
 *
 * GetHexValue --
 *
 *      Converts the hexadecimal string into an unsigned integer value.
 *      The hexadecimal string need not have a leading "0x".
 *
 * Results:
 *      Returns a standard TCL result. If the conversion was successful,
 *      TCL_OK is returned, otherwise TCL_ERROR.
 *
 * Side Effects:
 *      If the conversion fails, interp->result is filled with an error
 *      message.
 *
 *---------------------------------------------------------------------------
 */
static int
GetHexValue(Tcl_Interp *interp, const char *string, int *valuePtr)
{
    const char *s;
    int value;

    s = string;
    if ((s[0] == '0') && ((s[1] == 'x') || (s[1] == 'X'))) {
        s += 2;
    }
    if (s[0] == '\0') {
        Tcl_AppendResult(interp, "expecting hex value: got \"", string, "\"",
            (char *)NULL);
        return TCL_ERROR;               /* Only found "0x"  */
    }
    value = 0;
    for ( /*empty*/ ; *s != '\0'; s++) {
        unsigned char byte;

        /* Trim high bits, check type and accumulate */
        byte = hexTable[(int)*s];
        if (byte == 0xFF) {
            Tcl_AppendResult(interp, "expecting hex value: got \"", string,
                "\"", (char *)NULL);
            return TCL_ERROR;           /* Not a hexadecimal number */
        }
        value = (value << 4) | byte;
    }
    *valuePtr = value;
    return TCL_OK;
}

#ifdef WIN32
/*
 *---------------------------------------------------------------------------
 *
 * BitmapToData --
 *
 *      Converts a bitmap into an data array.
 *
 * Results:
 *      Returns the number of bytes in an data array representing the
 *      bitmap.
 *
 * Side Effects:
 *      Memory is allocated for the data array. Caller must free array
 *      later.
 *
 *---------------------------------------------------------------------------
 */
static int
BitmapToData(
    Tk_Window tkwin,                    /* Main window of interpreter */
    Pixmap bitmap,                      /* Bitmap to be queried */
    int width, int height,              /* Dimensions of the bitmap */
    unsigned char **bitsPtr)            /* Pointer to converted array of
                                         * data */
{
    int y;
    int count;
    int numBytes, bytes_per_line;
    unsigned char *bits;
    unsigned char *srcBits;
    int bytesPerRow;

    *bitsPtr = NULL;
    srcBits = Blt_GetBitmapData(Tk_Display(tkwin), bitmap, width, height,
        &bytesPerRow);
    if (srcBits == NULL) {
        OutputDebugString("BitmapToData: Can't get bitmap data");
        return 0;
    }
    bytes_per_line = (width + 7) / 8;
    numBytes = height * bytes_per_line;
    bits = Blt_AssertMalloc(sizeof(unsigned char) * numBytes);
    count = 0;
    for (y = height - 1; y >= 0; y--) {
        unsigned char *srcPtr;
        int value, bitMask;
        int x;

        srcPtr = srcBits + (bytesPerRow * y);
        value = 0, bitMask = 1;
        for (x = 0; x < width; /* empty */ ) {
            unsigned long pixel;

            pixel = (*srcPtr & (0x80 >> (x & 7)));
            if (pixel) {
                value |= bitMask;
            }
            bitMask <<= 1;
            x++;
            if (!(x & 7)) {
                bits[count++] = (unsigned char)value;
                value = 0, bitMask = 1;
                srcPtr++;
            }
        }
        if (x & 7) {
            bits[count++] = (unsigned char)value;
        }
    }
    *bitsPtr = bits;
    return count;
}

#else

/*
 *---------------------------------------------------------------------------
 *
 * BitmapToData --
 *
 *      Converts a bitmap into an data array.
 *
 * Results:
 *      Returns the number of bytes in an data array representing the
 *      bitmap.
 *
 * Side Effects:
 *      Memory is allocated for the data array. Caller must free
 *      array later.
 *
 *---------------------------------------------------------------------------
 */
static int
BitmapToData(
    Tk_Window tkwin,                    /* Main window of interpreter */
    Pixmap bitmap,                      /* Bitmap to be queried */
    int width, int height,              /* Dimensions of the bitmap */
    unsigned char **bitsPtr)            /* Pointer to converted array of
                                         * data */
{
    int y;
    int count;
    int numBytes, bytes_per_line;
    Display *display;
    XImage *imagePtr;
    unsigned char *bits;

    display = Tk_Display(tkwin);
    /* Convert the bitmap to an X image */
    imagePtr = XGetImage(display, bitmap, 0, 0, width, height, 1L, XYPixmap);
    /*
     * The slow but robust brute force method of converting an X image:
     */
    bytes_per_line = (width + 7) / 8;
    numBytes = height * bytes_per_line;
    bits = Blt_AssertMalloc(sizeof(unsigned char) * numBytes);
    count = 0;
    for (y = 0; y < height; y++) {
        int value, bitMask;
        int x;

        value = 0, bitMask = 1;
        for (x = 0; x < width; /*empty*/ ) {
            unsigned long pixel;

            pixel = XGetPixel(imagePtr, x, y);
            if (pixel) {
                value |= bitMask;
            }
            bitMask <<= 1;
            x++;
            if (!(x & 7)) {
                bits[count++] = (unsigned char)value;
                value = 0, bitMask = 1;
            }
        }
        if (x & 7) {
            bits[count++] = (unsigned char)value;
        }
    }
    XDestroyImage(imagePtr);
    *bitsPtr = bits;
    return count;
}

#endif

/*
 *---------------------------------------------------------------------------
 *
 * AsciiToData --
 *
 *      Converts a TCL list of ASCII values into a data array.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      If an error occurs while processing the data, interp->result
 *      is filled with a corresponding error message.
 *
 *---------------------------------------------------------------------------
 */
static int
AsciiToData(
    Tcl_Interp *interp,                 /* Interpreter to report results
                                         * to */
    const char *elemList,               /* List of of hex numbers
                                         * representing bitmap data */
    int width, int height,              /* Dimension of bitmap. */
    unsigned char **bitsPtr)            /* data array (output) */
{
    int numBytes;                       /* Number of bytes of data */
    int value;                          /* from an input line */
    int padding;                        /* to handle alignment */
    int bytesPerLine;                   /* per scanline of data */
    unsigned char *bits;
    int count;
    enum Formats {
        V10, V11
    } format;
    int i;              /*  */
    const char **argv;
    int argc;

    /* First time through initialize the ascii->hex translation table */
    if (!initialized) {
        Blt_InitHexTable(hexTable);
        initialized = 1;
    }
    if (Tcl_SplitList(interp, elemList, &argc, &argv) != TCL_OK) {
        return TCL_ERROR;
    }
    bytesPerLine = (width + 7) / 8;
    numBytes = bytesPerLine * height;
    if (argc == numBytes) {
        format = V11;
    } else if (argc == (numBytes / 2)) {
        format = V10;
    } else {
        Tcl_AppendResult(interp, "bitmap has wrong # of data values",
            (char *)NULL);
        goto error;
    }
    padding = 0;
    if (format == V10) {
        padding = ((width % 16) && ((width % 16) < 9));
        if (padding) {
            bytesPerLine = (width + 7) / 8 + padding;
            numBytes = bytesPerLine * height;
        }
    }
    bits = Blt_Calloc(numBytes, sizeof(unsigned char));
    if (bits == NULL) {
        Tcl_AppendResult(interp, "can't allocate memory for bitmap",
            (char *)NULL);
        goto error;
    }
    count = 0;
    for (i = 0; i < argc; i++) {
        if (GetHexValue(interp, argv[i], &value) != TCL_OK) {
            Blt_Free(bits);
            goto error;
        }
        bits[count++] = (unsigned char)value;
        if (format == V10) {
            if ((!padding) || (((i * 2) + 2) % bytesPerLine)) {
                bits[count++] = value >> 8;
            }
        }
    }
    Tcl_Free((char *)argv);
    *bitsPtr = bits;
    return TCL_OK;
  error:
    Tcl_Free((char *)argv);
    return TCL_ERROR;
}

static int
ParseListData(Tcl_Interp *interp, Tcl_Obj *objPtr, int *widthPtr,
              int *heightPtr, unsigned char **bitsPtr)
{
    char *p;
    int width, height;
    const char *string;
    int objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 2) {
        Tcl_Obj **dims;
        int numDims;
        
        if (Tcl_ListObjGetElements(interp, objv[0], &numDims, &dims) != TCL_OK){
            return TCL_ERROR;
        }
        if (numDims != 2) {
            Tcl_AppendResult(interp, "wrong # of bitmap dimensions: ",
                             "should be \"width height\"", (char *)NULL);
            return TCL_ERROR;
        } 
        if ((Tcl_GetIntFromObj(interp, dims[0], &width) != TCL_OK) ||
            (Tcl_GetIntFromObj(interp, dims[1], &height) != TCL_OK)) {
            return TCL_ERROR;
        }
        string = Tcl_GetString(objv[1]);
    } else if (objc == 3) {
        if ((Tcl_GetIntFromObj(interp, objv[0], &width) != TCL_OK) ||
            (Tcl_GetIntFromObj(interp, objv[1], &height) != TCL_OK)) {
            return TCL_ERROR;
        }
        string = Tcl_GetString(objv[2]);
    } else {
        Tcl_AppendResult(interp, "wrong # of bitmap data components: ",
                         "should be \"dimensions sourceData\"", (char *)NULL);
        return TCL_ERROR;;
    }
    if ((width < 1) || (height < 1)) {
        Tcl_AppendResult(interp, "bad bitmap dimensions", (char *)NULL);
        return TCL_ERROR;
    }
    /* Convert commas to blank spaces */
    string = Blt_AssertStrdup(string);
    for (p = (char *)string; *p != '\0'; p++) {
        if (*p == ',') {
            *p = ' ';
        }
    }
    if (AsciiToData(interp, string, width, height, bitsPtr) != TCL_OK) {
        Blt_Free(string);
        return TCL_ERROR;
    }
    *widthPtr = width;
    *heightPtr = height;
    return TCL_OK;
}

/*
 * Parse the lines that define the dimensions of the bitmap, plus the first
 * line that defines the bitmap data (it declares the name of a data
 * variable but doesn't include any actual data).  These lines look
 * something like the following:
 *
 *              #define foo_width 16
 *              #define foo_height 16
 *              #define foo_x_hot 3
 *              #define foo_y_hot 3
 *              static char foo_bits[] = {
 *
 * The x_hot and y_hot lines may or may not be present.  It's important to
 * check for "char" in the last line, in order to reject old X10-style
 * bitmaps that used shorts.
 */

static int
ParseStructData(Tcl_Interp *interp, Tcl_Obj *objPtr, int *widthPtr, 
                int *heightPtr, unsigned char **bitsPtr)
{
    int width, height;
    int hotX, hotY;
    char *line, *nextline;
    char *data;
    const char *string;

    width = height = 0;
    hotX = hotY = -1;
    data = NULL;
    {
        char *p;

        /* Skip leading spaces. */
        for (p = Tcl_GetString(objPtr); isspace(UCHAR(*p)); p++) {
            /*empty*/
        }
        string = Blt_AssertStrdup(p);
    }
    nextline = (char *)string;
    for (line = (char *)string; nextline != NULL; line = nextline + 1) {
        Tcl_RegExp re;

        nextline = strchr(line, '\n');
        if ((nextline == NULL) || (line == nextline)) {
            continue;           /* Empty line */
        }
        *nextline = '\0';
        re = Tcl_RegExpCompile(interp, " *# *define +");
        if (Tcl_RegExpExec(interp, re, line, line)) {
            const char *start, *end;
            const char *name, *value;
            size_t len;

            Tcl_RegExpRange(re, 0, &start, &end);
            name = strtok((char *)end, " \t"); 
            value = strtok(NULL, " \t");
            if ((name == NULL) || (value == NULL)) {
                Tcl_AppendResult(interp, "what's the error?", (char *)NULL);
                goto error;
            }
            len = strlen(name);
            if ((len >= 6) && (name[len-6] == '_') && 
                (strcmp(name+len-6, "_width") == 0)) {
                if (Tcl_GetInt(interp, value, &width) != TCL_OK) {
                    goto error;
                }
            } else if ((len >= 7) && (name[len-7] == '_') && 
                       (strcmp(name+len-7, "_height") == 0)) {
                if (Tcl_GetInt(interp, value, &height) != TCL_OK) {
                    goto error;
                }
            } else if ((len >= 6) && (name[len-6] == '_') && 
                       (strcmp(name+len-6, "_x_hot") == 0)) {
                if (Tcl_GetInt(interp, value, &hotX) != TCL_OK) {
                    goto error;
                }
            } else if ((len >= 6) && (name[len-6] == '_') && 
                       (strcmp(name+len-6, "_y_hot") == 0)) {
                if (Tcl_GetInt(interp, value, &hotY) != TCL_OK) {
                    goto error;
                }
            } 
        } else {
            re = Tcl_RegExpCompile(interp, " *static +.*char +");
            if (Tcl_RegExpExec(interp, re, line, line)) {
                char *p;

                /* Find the { */
                /* Repair the string so we can search the entire string. */
                *nextline = ' ';   
                p = strchr(line, '{');
                if (p == NULL) {
                    goto error;
                }
                data = p + 1;
                break;
            } else {
                Tcl_AppendResult(interp, "unknown bitmap format \"", line, 
                     "\": obsolete X10 bitmap file?", (char *)NULL);
                goto error;
            }
        }
    }
    /*
     * Now we've read everything but the data.  Allocate an array and read
     * in the data.
     */
    if ((width <= 0) || (height <= 0)) {
        Tcl_AppendResult(interp, "invalid bitmap dimensions \"", (char *)NULL);
        Tcl_AppendResult(interp, Blt_Itoa(width), " x ", (char *)NULL);
        Tcl_AppendResult(interp, Blt_Itoa(height), "\"", (char *)NULL);
        goto error;
    }
    {
        char *p;

        for (p = data; *p != '\0'; p++) {
            if ((*p == ',') || (*p == ';') || (*p == '}')) {
                *p = ' ';
            }
        }
    }
    if (AsciiToData(interp, data, width, height, bitsPtr) != TCL_OK) {
        goto error;
    }
    *widthPtr = width;
    *heightPtr = height;
    Blt_Free(string);
    return TCL_OK;
 error:
    Blt_Free(string);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * ScaleRotateData --
 *
 *      Creates a new data array of the rotated and scaled bitmap.
 *
 * Results:
 *      A standard TCL result. If the bitmap data is rotated successfully,
 *      TCL_OK is returned.  But if memory could not be allocated for the
 *      new data array, TCL_ERROR is returned and an error message is left
 *      in interp->result.
 *
 * Side Effects:
 *      Memory is allocated for rotated, scaled data array. Caller must
 *      free array later.
 *
 *---------------------------------------------------------------------------
 */
static int
ScaleRotateData(
    Tcl_Interp *interp,                 /* Interpreter to report results
                                         * to */
    BitmapData *srcPtr,                 /* Source bitmap to transform. */
    double angle,                       /* Number of degrees to rotate the
                                         * bitmap. */
    double scale,                       /* Factor to scale the bitmap. */
    BitmapData *destPtr)                /* Destination bitmap. */
{
    int x, y;
    double srcX, srcY, destX, destY;    /* Origins of source and
                                         * destination bitmaps */
    double sinTheta, cosTheta;
    double rw, rh;
    double radians;
    unsigned char *bits;
    int numBytes;
    int srcBytesPerLine, destBytesPerLine;

    srcBytesPerLine = (srcPtr->width + 7) / 8;
    Blt_GetBoundingBox((double)srcPtr->width, (double)srcPtr->height, angle, 
        &rw, &rh, (Point2d *)NULL);
    destPtr->width = (int)(rw * scale + 0.5) ;
    destPtr->height = (int)(rh * scale + 0.5);

    destBytesPerLine = (destPtr->width + 7) / 8;
    numBytes = destPtr->height * destBytesPerLine;
    bits = Blt_Calloc(numBytes, sizeof(unsigned char));
    if (bits == NULL) {
        Tcl_AppendResult(interp, "can't allocate bitmap data array",
            (char *)NULL);
        return TCL_ERROR;
    }
    scale = 1.0 / scale;
    destPtr->bits = bits;

    radians = angle * DEG2RAD;
    sinTheta = sin(radians);
    cosTheta = cos(radians);

    /*
     * Coordinates of the centers of the source and destination rectangles
     */
    srcX = srcPtr->width * 0.5;
    srcY = srcPtr->height * 0.5;
    destX = rw * 0.5;
    destY = rh * 0.5;

    /*
     * Rotate each pixel of dest image, placing results in source X image
     */
    for (y = 0; y < destPtr->height; y++) {
        for (x = 0; x < destPtr->width; x++) {
            double sxd, syd;    
            int sx, sy;
            int pixel, ipixel;
            
            sxd = scale * (double)x;
            syd = scale * (double)y;
            if (angle == 270.0) {
                sx = (int)syd, sy = (int)(rw - sxd) - 1;
            } else if (angle == 180.0) {
                sx = (int)(rw - sxd) - 1, sy = (int)(rh - syd) - 1;
            } else if (angle == 90.0) {
                sx = (int)(rh - syd) - 1, sy = (int)sxd;
            } else if (angle == 0.0) {
                sx = (int)sxd, sy = (int)syd;
            } else {
                double tx, ty, rx, ry;
                /* Translate origin to center of destination X image */

                tx = sxd - destX;
                ty = syd - destY;

                /* Rotate the coordinates about the origin */

                rx = (tx * cosTheta) - (ty * sinTheta);
                ry = (tx * sinTheta) + (ty * cosTheta);

                /* Translate back to the center of the source X image */
                rx += srcX;
                ry += srcY;

                sx = ROUND(rx);
                sy = ROUND(ry);

                /*
                 * Verify the coordinates, since the destination X image
                 * can be bigger than the source.
                 */

                if ((sx >= srcPtr->width) || (sx < 0) ||
                    (sy >= srcPtr->height) || (sy < 0)) {
                    continue;
                }
            }
            ipixel = (srcBytesPerLine * sy) + (sx / 8);
            pixel = srcPtr->bits[ipixel] & (1 << (sx % 8));
            if (pixel) {
                ipixel = (destBytesPerLine * y) + (x / 8);
                bits[ipixel] |= (1 << (x % 8));
            }
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BitmapDataToString --
 *
 *      Returns a list of hex values corresponding to the data bits of the
 *      bitmap given.
 *
 *      Converts the unsigned character value into a two character
 *      hexadecimal string.  A separator is also added, which may either a
 *      newline or space according the the number of bytes already output.
 *
 * Results:
 *      Returns TCL_ERROR if a data array can't be generated from the
 *      bitmap (memory allocation failure), otherwise TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
static void
BitmapDataToString(
    Tk_Window tkwin,                    /* Main window of interpreter */
    Pixmap bitmap,                      /* Bitmap to be queried */
    Tcl_DString *resultPtr)             /* Dynamic string to output results
                                         * to */
{
    unsigned char *bits;
    int numBytes;
    int i;
    int width, height;

    /* Get the dimensions of the bitmap */
    Tk_SizeOfBitmap(Tk_Display(tkwin), bitmap, &width, &height);
    numBytes = BitmapToData(tkwin, bitmap, width, height, &bits);
#define BYTES_PER_OUTPUT_LINE 24
    for (i = 0; i < numBytes; i++) {
        const char *separator;
        char string[200];

        separator = (i % BYTES_PER_OUTPUT_LINE) ? " " : "\n    ";
        Blt_FormatString(string, 200, "%s%02x", separator, bits[i]);
        Tcl_DStringAppend(resultPtr, string, -1);
    }
    if (bits != NULL) {
        Blt_Free(bits);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComposeOp --
 *
 *      Converts the text string into an internal bitmap.
 *
 *      There's a lot of extra (read unnecessary) work going on here, but I
 *      don't (right now) think that it matters much.  The rotated bitmap
 *      (formerly an X image) is converted back to an image just so we can
 *      convert it to a data array for Tk_DefineBitmap.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      If an error occurs while processing the data, interp->result is
 *      filled with a corresponding error message.
 *
 *---------------------------------------------------------------------------
 */
static int
ComposeOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    BitmapInfo bi;                      /* Text rotation and font
                                         * information */
    BitmapInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Pixmap bitmap;                      /* Text bitmap */
    TextLayout *textPtr;
    TextStyle ts;
    char *string;
    double angle;
    int numBytes;
    int isNew;
    int result;
    int width, height;                  /* Dimensions of bitmap */
    unsigned char *bits;                /* Data array derived from text
                                         * bitmap */
    bitmap = Tk_AllocBitmapFromObj((Tcl_Interp *)NULL, dataPtr->tkMain, 
        objv[2]);
    if (bitmap != None) {
        Tk_FreeBitmap(dataPtr->display, bitmap);
        return TCL_OK;
    }
    /* Initialize info and process flags */
    bi.justify = TK_JUSTIFY_CENTER;
    bi.angle = 0.0f;                    /* No rotation or scaling by
                                         * default */
    bi.scale = 1.0f;
    bi.padLeft = bi.padRight = 0;
    bi.padTop = bi.padBottom = 0;
    bi.font = (Blt_Font)NULL;           /* Initialized by
                                         * Blt_ConfigureWidget */
    if (Blt_ConfigureWidgetFromObj(interp, dataPtr->tkMain, composeConfigSpecs,
            objc - 4, objv + 4, (char *)&bi, 0) != TCL_OK) {
        return TCL_ERROR;
    }
    angle = FMOD(bi.angle, 360.0);
    if (angle < 0.0) {
        angle += 360.0;
    }
    Blt_Ts_InitStyle(ts);
    Blt_Ts_SetFont(ts, bi.font);
    Blt_Ts_SetJustify(ts, bi.justify);
    Blt_Ts_SetPadding(ts, bi.padX.side1, bi.padX.side2, bi.padY.side1, 
        bi.padY.side2);

    string = Tcl_GetStringFromObj(objv[3], &numBytes);
    textPtr = Blt_Ts_CreateLayout(string, numBytes, &ts);
    bitmap = Blt_Ts_Bitmap(dataPtr->tkMain, textPtr, &ts, &width, &height);
    Blt_Free(textPtr);
    if (bitmap == None) {
        Tcl_AppendResult(interp, "can't create bitmap", (char *)NULL);
        return TCL_ERROR;
    }
    /* Free the font structure, since we don't need it anymore */
    Blt_FreeOptions(composeConfigSpecs, (char *)&bi, dataPtr->display, 0);

    /* Convert bitmap back to a data array */
    numBytes = BitmapToData(dataPtr->tkMain, bitmap, width, height, &bits);
    Tk_FreePixmap(dataPtr->display, bitmap);
    if (numBytes == 0) {
        Tcl_AppendResult(interp, "can't get bitmap data", (char *)NULL);
        return TCL_ERROR;
    }
    /* If bitmap is to be rotated or scaled, do it here */
    if ((angle != 0.0) || (bi.scale != 1.0)) {
        BitmapData srcData, destData;

        srcData.bits = bits;
        srcData.width = width;
        srcData.height = height;

        result = ScaleRotateData(interp, &srcData, angle, bi.scale, &destData);
        Blt_Free(bits);                 /* Free the un-transformed data
                                         * array. */
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
        bits = destData.bits;
        width = destData.width;
        height = destData.height;
    }
    /* Create the bitmap again, this time using Tk's bitmap facilities */
    string = Tcl_GetString(objv[2]);
    result = Tk_DefineBitmap(interp, Tk_GetUid(string), (char *)bits,
        width, height);
    if (result != TCL_OK) {
        Blt_Free(bits);
    }
    hPtr = Blt_CreateHashEntry(&dataPtr->bitmapTable, string, &isNew);
    Blt_SetHashValue(hPtr, bits);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * DefineOp --
 *
 *      Converts the dataList into an internal bitmap.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      If an error occurs while processing the data, interp->result is
 *      filled with a corresponding error message.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
DefineOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    BitmapInterpData *dataPtr = clientData;
    int width, height;                  /* Dimensions of bitmap */
    unsigned char *bits;                /* working variable */
    char *p;
    BitmapInfo bi;
    int result;
    double angle;
    Pixmap bitmap;
    Blt_HashEntry *hPtr;
    int isNew;
    char *string;

    bitmap = Tk_AllocBitmapFromObj((Tcl_Interp *)NULL, dataPtr->tkMain,objv[2]);
    if (bitmap != None) {
        Tk_FreeBitmap(dataPtr->display, bitmap);
        return TCL_OK;
    }
    /* Initialize info and then process flags */
    bi.angle = 0.0;                     /* No rotation by default */
    bi.scale = 1.0;                     /* No scaling by default */
    if (Blt_ConfigureWidgetFromObj(interp, dataPtr->tkMain, defineConfigSpecs,
            objc - 4, objv + 4, (char *)&bi, 0) != TCL_OK) {
        return TCL_ERROR;
    }
    bits = NULL;
    /* Skip leading spaces. */
    for (p = Tcl_GetString(objv[3]); isspace(UCHAR(*p)); p++) {
        /*empty*/
    }
    width = height = 0;                 /* Suppress compiler warning. */
    if (*p == '#') {
        result = ParseStructData(interp, objv[3], &width, &height, &bits);
    } else {
        result = ParseListData(interp, objv[3], &width, &height, &bits);
    }
    if (result != TCL_OK) {
        return TCL_ERROR;
    }
    angle = FMOD(bi.angle, 360.0);
    if (angle < 0.0) {
        angle += 360.0;
    }
    /* If bitmap is to be rotated or scale, do it here */
    if ((angle != 0.0) || (bi.scale != 1.0)) { 
        BitmapData srcData, destData;

        srcData.bits = bits;
        srcData.width = width;
        srcData.height = height;

        result = ScaleRotateData(interp, &srcData, angle, bi.scale, &destData);
        Blt_Free(bits);                 /* Free the array of un-transformed
                                         * data. */
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
        bits = destData.bits;
        width = destData.width;
        height = destData.height;
    }
    string = Tcl_GetString(objv[2]);
    result = Tk_DefineBitmap(interp, Tk_GetUid(string), (char *)bits, width, 
        height);
    if (result != TCL_OK) {
        Blt_Free(bits);
    }
    hPtr = Blt_CreateHashEntry(&dataPtr->bitmapTable, string, &isNew);
    Blt_SetHashValue(hPtr, bits);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExistOp --
 *
 *      Indicates if the named bitmap exists.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    BitmapInterpData *dataPtr = clientData;
    Pixmap bitmap;
    
    bitmap = Tk_AllocBitmapFromObj((Tcl_Interp *)NULL, dataPtr->tkMain,objv[2]);
    if (bitmap != None) {
        Tk_FreeBitmap(dataPtr->display, bitmap);
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (bitmap != None));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HeightOp --
 *
 *      Returns the height of the named bitmap.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HeightOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    BitmapInterpData *dataPtr = clientData;
    int width, height;
    Pixmap bitmap;
    
    bitmap = Tk_AllocBitmapFromObj(interp, dataPtr->tkMain, objv[2]);
    if (bitmap == None) {
        return TCL_ERROR;
    }
    Tk_SizeOfBitmap(dataPtr->display, bitmap, &width, &height);
    Tk_FreeBitmap(dataPtr->display, bitmap);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), height);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * WidthOp --
 *
 *      Returns the width of the named bitmap.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
WidthOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    BitmapInterpData *dataPtr = clientData;
    int width, height;
    Pixmap bitmap;

    bitmap = Tk_AllocBitmapFromObj(interp, dataPtr->tkMain, objv[2]);
    if (bitmap == None) {
        return TCL_ERROR;
    }
    Tk_SizeOfBitmap(dataPtr->display, bitmap, &width, &height);
    Tk_FreeBitmap(dataPtr->display, bitmap);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), width);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SourceOp --
 *
 *      Returns the data array (excluding width and height) of the named
 *      bitmap.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SourceOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    BitmapInterpData *dataPtr = clientData;
    Pixmap bitmap;
    Tcl_DString ds;

    bitmap = Tk_AllocBitmapFromObj(interp, dataPtr->tkMain, objv[2]);
    if (bitmap == None) {
        return TCL_ERROR;
    }
    Tcl_DStringInit(&ds);
    BitmapDataToString(dataPtr->tkMain, bitmap, &ds);
    Tk_FreeBitmap(dataPtr->display, bitmap);
    Tcl_DStringResult(interp, &ds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DataOp --
 *
 *      Returns the data array, including width and height, of the named
 *      bitmap.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DataOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    BitmapInterpData *dataPtr = clientData;
    Pixmap bitmap;
    int width, height;
    Tcl_DString ds;

    bitmap = Tk_AllocBitmapFromObj(interp, dataPtr->tkMain, objv[2]);
    if (bitmap == None) {
        return TCL_ERROR;
    }
    Tk_SizeOfBitmap(dataPtr->display, bitmap, &width, &height);
    Tcl_DStringInit(&ds);
    Tcl_DStringAppendElement(&ds, Blt_Itoa(width));
    Tcl_DStringAppendElement(&ds, Blt_Itoa(height));
    Tcl_DStringStartSublist(&ds);
    BitmapDataToString(dataPtr->tkMain, bitmap, &ds);
    Tcl_DStringEndSublist(&ds);
    Tk_FreeBitmap(dataPtr->display, bitmap);
    Tcl_DStringResult(interp, &ds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BLT Sub-command specification:
 *
 *      - Name of the sub-command.
 *      - Minimum number of characters needed to unambiguously
 *        recognize the sub-command.
 *      - Pointer to the function to be called for the sub-command.
 *      - Minimum number of arguments accepted.
 *      - Maximum number of arguments accepted.
 *      - String to be displayed for usage.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec bitmapOps[] =
{
    {"compose", 1, ComposeOp, 4, 0, "bitmapName text ?flags?",},
    {"data",    2, DataOp,    3, 3, "bitmapName",},
    {"define",  2, DefineOp,  4, 0, "bitmapName data ?flags?",},
    {"exists",  1, ExistsOp,  3, 3, "bitmapName",},
    {"height",  1, HeightOp,  3, 3, "bitmapName",},
    {"source",  1, SourceOp,  3, 3, "bitmapName",},
    {"width",   1, WidthOp,   3, 3, "bitmapName",},
};
static int numBitmapOps = sizeof(bitmapOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * BitmapCmdProc --
 *
 *      This procedure is invoked to process the TCL command that
 *      corresponds to bitmaps managed by this module.  See the user
 *      documentation for details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BitmapCmdProc(
    ClientData clientData,              /* Thread-specific data for
                                         * bitmaps. */
    Tcl_Interp *interp,                 /* Interpreter to report results
                                         * to */
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numBitmapOps, bitmapOps, BLT_OP_ARG1, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * BitmapInterpDeleteProc --
 *
 *      This is called when the interpreter is deleted.  All the bitmaps
 *      specific to that interpreter are destroyed.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Destroys the bitmap table.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
BitmapInterpDeleteProc(
    ClientData clientData,      /* Thread-specific data. */
    Tcl_Interp *interp)
{
    BitmapInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    
    for (hPtr = Blt_FirstHashEntry(&dataPtr->bitmapTable, &iter);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        unsigned char *bits;

        bits = Blt_GetHashValue(hPtr);
        Blt_Free(bits);
    }
    Blt_DeleteHashTable(&dataPtr->bitmapTable);
    Tcl_DeleteAssocData(interp, BITMAP_THREAD_KEY);
    Blt_Free(dataPtr);
}

static BitmapInterpData *
GetBitmapInterpData(Tcl_Interp *interp)
{
    BitmapInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (BitmapInterpData *)
        Tcl_GetAssocData(interp, BITMAP_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
        dataPtr = Blt_AssertMalloc(sizeof(BitmapInterpData));
        dataPtr->interp = interp;
        dataPtr->tkMain = Tk_MainWindow(interp);
        dataPtr->display = Tk_Display(dataPtr->tkMain);
        Tcl_SetAssocData(interp, BITMAP_THREAD_KEY, BitmapInterpDeleteProc, 
                 dataPtr);
        Blt_InitHashTable(&dataPtr->bitmapTable, BLT_STRING_KEYS);
    }
    return dataPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_BitmapCmdInitProc --
 *
 *      This procedure is invoked to initialize the bitmap command.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Adds the command to the interpreter and sets an array variable
 *      which its version number.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_BitmapCmdInitProc(Tcl_Interp *interp)
{
    BitmapInterpData *dataPtr;
    static Blt_CmdSpec cmdSpec = {"bitmap", BitmapCmdProc };

    /* Define the BLT logo bitmaps */

    dataPtr = GetBitmapInterpData(interp);
    cmdSpec.clientData = dataPtr;
    Tk_DefineBitmap(interp, Tk_GetUid("bigBLT"), (char *)bigblt_bits,
        bigblt_width, bigblt_height);
    Tk_DefineBitmap(interp, Tk_GetUid("BLT"), (char *)blt_bits,
        blt_width, blt_height);
    Tcl_ResetResult(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

#endif /* NO_BITMAP */

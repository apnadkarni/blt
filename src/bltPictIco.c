/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictIco.c --
 *
 * This module implements ICO file format conversion routines for the
 * picture image type in the BLT toolkit.
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

#include "bltInt.h"

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef _MSC_VER
  #define vsnprintf               _vsnprintf
#endif

#include <setjmp.h>

#undef assert
#ifdef __STDC__
  #define assert(EX) (void)((EX) || (IcoAssert(#EX, __FILE__, __LINE__), 0))
#else
  #define assert(EX) (void)((EX) || (IcoAssert("EX", __FILE__, __LINE__), 0))
#endif /* __STDC__ */

typedef struct _Blt_Picture Picture;

#define TRUE    1
#define FALSE   0
#define TYPE_UNKNOWN    0
#define TYPE_ICO        1
#define TYPE_CUR        2

#define OFF_TYPE                0
#define OFF_FILE_SIZE           2
#define OFF_RESERVED1           6
#define OFF_RESERVED2           8
#define OFF_OFFBITS             10

#define OFF_SIZE                0       /* 14 */
#define OFF_WIDTH               4       /* 18 */
#define OFF_HEIGHT              8       /* 22 */
#define OFF_PLANES              12      /* 26 */
#define OFF_BIT_COUNT           14      /* 28 */
#define OFF_COMPRESSION         16      /* 30 */
#define OFF_SIZE_IMAGE          20      /* 34 */
#define OFF_X_PELS_PER_METER    24      /* 38 */
#define OFF_Y_PELS_PER_METER    28      /* 42 */
#define OFF_CLR_USED            32      /* 46 */
#define OFF_CLR_IMPORTANT       36      /* 50 */
#define OFF_RED_MASK            40      /* 54 */
#define OFF_GREEN_MASK          44      /* 58 */
#define OFF_BLUE_MASK           48      /* 62 */
#define OFF_ALPHA_MASK          52      /* 66 */
#define OFF_CS_TYPE             56      /* 70 */
#define OFF_END_POINTS          60      /* 74 */
#define OFF_GAMMA_RED           110 
#define OFF_GAMMA_GREEN         114
#define OFF_GAMMA_BLUE          118  
#define OFF_INTENT              122 
#define OFF_PROFILE_DATA        126
#define OFF_PROFILE_SIZE        130
#define OFF_RESERVED3           134 

#define OFF_OSV1_SIZE           14
#define OFF_OSV1_WIDTH          18
#define OFF_OSV1_HEIGHT         20
#define OFF_OSV1_PLANES         22
#define OFF_OSV1_BIT_COUNT      24

#define SIZEOF_BITMAPOS2V1HEADER 12
#define SIZEOF_BITMAPOS2V2HEADER 64
#define SIZEOF_BITMAPV3HEADER    40
#define SIZEOF_BITMAPV4HEADER    108
#define SIZEOF_BITMAPV5HEADER    124
#define SIZEOF_ICODIRENTRY       16
#define SIZEOF_ICOHEADER         6


typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
} IcoImportSwitches;

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    int flags;                  /* Flag. */
    Blt_Pixel bg;
    int index;
} IcoExportSwitches;

#define EXPORT_ALPHA    (1<<0)

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data",  "data", (char *)NULL,
        Blt_Offset(IcoImportSwitches, dataObjPtr), 0},
    {BLT_SWITCH_OBJ, "-file",  "fileName", (char *)NULL,
        Blt_Offset(IcoImportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchParseProc ColorSwitchProc;
static Blt_SwitchCustom colorSwitch = {
    ColorSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_BITMASK, "-alpha", "", (char *)NULL,
        Blt_Offset(IcoExportSwitches, flags),   0, EXPORT_ALPHA},
    {BLT_SWITCH_CUSTOM, "-background", "color", (char *)NULL,
        Blt_Offset(IcoExportSwitches, bg),         0, 0, &colorSwitch},
    {BLT_SWITCH_OBJ, "-data",  "varName", (char *)NULL,
        Blt_Offset(IcoExportSwitches, dataObjPtr), 0},
    {BLT_SWITCH_OBJ, "-file",  "fileName", (char *)NULL,
        Blt_Offset(IcoExportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_INT_NNEG, "-index", "int", (char *)NULL,
        Blt_Offset(IcoExportSwitches, index), 0},
    {BLT_SWITCH_END}
};

typedef struct {
    jmp_buf jmpbuf;
    Tcl_DString errors;
    Tcl_DString warnings;
    int numWarnings, numErrors;
} IcoMessage;

static IcoMessage *icoMessagePtr;

static const char *compression_types[] = {
    "RGB", "RLE8", "RLE4", "BITFIELDS", "JPEG", "PNG"
};

#ifndef WIN32
enum CompressionTypes {
    BI_RGB,                             /* No compression. */
    BI_RLE8,                            /* RLE 8-bits/pixel. Only used with
                                         * 8-bit/pixel bitmaps */  
    BI_RLE4,                            /* RLE 4-bits/pixel. Can be used only
                                         * with 4-bit/pixel bitmaps */
    BI_BITFIELDS,                       /* Bit fields. Can be used only with
                                         * 16 and 32-bit/pixel bitmaps. */
    BI_JPEG,                            /* The bitmap contains a JPEG
                                         * image. */
    BI_PNG,                             /* The bitmap contains a PNG image. */
};
#endif

typedef struct {
    unsigned short bfType;
    unsigned int   bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int   bfOffBits;
} BitmapFileHeader;

typedef struct {
    int  RedX;                          /* X coordinate of red endpoint */
    int  RedY;                          /* Y coordinate of red endpoint */
    int  RedZ;                          /* Z coordinate of red endpoint */
    int  GreenX;                        /* X coordinate of green endpoint */
    int  GreenY;                        /* Y coordinate of green endpoint */
    int  GreenZ;                        /* Z coordinate of green endpoint */
    int  BlueX;                         /* X coordinate of blue endpoint */
    int  BlueY;                         /* Y coordinate of blue endpoint */
    int  BlueZ;                         /* Z coordinate of blue endpoint */
} CieXyzTriple;

typedef struct {
    unsigned int   biSize;              /* Size of this structure. This
                                         * determines what version of the
                                         * header is * used. */
    int            biWidth; 
    int            biHeight; 
    unsigned short biPlanes; 
    unsigned short biBitCount; 
    unsigned int   biCompression; 
    unsigned int   biSizeImage; 
    int            biXPelsPerMeter; 
    int            biYPelsPerMeter; 
    unsigned int   biClrUsed; 
    unsigned int   biClrImportant; 

    /* Fields related to Version 4 of header. */
    unsigned int   biRedMask;
    unsigned int   biGreenMask;
    unsigned int   biBlueMask;
    unsigned int   biAlphaMask;
    unsigned int   biCSType;
    CieXyzTriple   biEndpoints;
    unsigned int   biGammaRed;
    unsigned int   biGammaGreen;
    unsigned int   biGammaBlue;

    /* Fields related to Version 5 of header. */
    unsigned int   biIntent; 
    unsigned int   biProfileData; 
    unsigned int   biProfileSize; 
    unsigned int   biReserved; 
} BitmapInfoHeader;

#define MAXCOLORS       256

typedef struct {
    int type;                           /* Type of file: either TYPE_ICO or
                                         * TYPE_CUR */
    int numImages;                      /* Number of images in file. */
    Blt_PictFormat *fmtPtr;             /* If non-NULL, pointer to PNG
                                         * format.  */
    BitmapInfoHeader bmih;
    Blt_Pixel colorTable[MAXCOLORS];
    const char *name;
} Ico;

DLLEXPORT extern Tcl_AppInitProc Blt_PictureIcoInit;
DLLEXPORT extern Tcl_AppInitProc Blt_PictureIcoSafeInit;

/*
 *---------------------------------------------------------------------------
 *
 * ColorSwitchProc --
 *
 *      Convert a Tcl_Obj representing a Blt_Pixel color.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColorSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results. */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Blt_Pixel *pixelPtr = (Blt_Pixel *)(record + offset);
    const char *string;

    string = Tcl_GetString(objPtr);
    if (string[0] == '\0') {
        pixelPtr->u32 = 0x00;
        return TCL_OK;
    }
    if (Blt_GetPixelFromObj(interp, objPtr, pixelPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*ARGSUSED*/
static void
IcoError(const char *fmt, ...)
{
    char string[BUFSIZ+4];
    int length;
    va_list args;

    va_start(args, fmt);
    length = vsnprintf(string, BUFSIZ, fmt, args);
    if (length > BUFSIZ) {
        strcat(string, "...");
    }
    Tcl_DStringAppend(&icoMessagePtr->errors, string, -1);
    va_end(args);
    longjmp(icoMessagePtr->jmpbuf, 0);
}

/*ARGSUSED*/
static void
IcoWarning(const char *fmt, ...)
{
    char string[BUFSIZ+4];
    int length;
    va_list args;

    va_start(args, fmt);
    length = vsnprintf(string, BUFSIZ, fmt, args);
    if (length > BUFSIZ) {
        strcat(string, "...");
    }
    Tcl_DStringAppend(&icoMessagePtr->warnings, string, -1);
    va_end(args);
    icoMessagePtr->numWarnings++;
}

static void
IcoAssert(const char *testExpr, const char *fileName, int lineNumber)
{
    IcoError("line %d of %s: Assert \"%s\" failed\n", lineNumber, fileName, 
             testExpr);
}

static INLINE unsigned int
IcoGetLong(unsigned char *buf)
{
#ifdef WORDS_BIGENDIAN
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
#else
    return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
#endif
}

static INLINE unsigned short
IcoGetShort(unsigned char *buf)
{
#ifdef WORDS_BIGENDIAN
    return (buf[0] << 8) | buf[1];
#else
    return buf[0] | (buf[1] << 8);
#endif
}

static INLINE unsigned char *
IcoSetLong(unsigned char *buf, unsigned long value)
{
#ifdef WORDS_BIGENDIAN
    buf[0] = (value >> 24) & 0xFF;
    buf[1] = (value >> 16) & 0xFF;
    buf[2] = (value >> 8)  & 0xFF;
    buf[3] = (value)       & 0xFF;
#else
    buf[0] = (value)       & 0xFF;
    buf[1] = (value >> 8)  & 0xFF;
    buf[2] = (value >> 16) & 0xFF;
    buf[3] = (value >> 24) & 0xFF;
#endif
    return buf + 4;
}

static INLINE unsigned char *
IcoSetShort(unsigned char *buf, unsigned long value)
{
#ifdef WORDS_BIGENDIAN
    buf[0] = (value >> 8)  & 0xFF;
    buf[1] = (value)       & 0xFF;
#else
    buf[0] = (value)       & 0xFF;
    buf[1] = (value >> 8)  & 0xFF;
#endif
    return buf + 2;
}

/*
 * Version 2            Field Name              Size 
 *   | 0| 1| 2| 3|      biSize                  4 Bytes 
 *   | 4| 5|            biWidth                 2 Bytes 
 *   | 6| 7|            biHeight                2 Bytes 
 *   | 8| 9|            biPlanes                2 Bytes 
 *   |10|11|            biBitCount              2 Bytes 

 * Version 3            
 *   | 0| 1| 2| 3|      biSize                  4 Bytes 
 *   | 4| 5| 6| 7|      biWidth                 4 Bytes 
 *   | 8| 9|10|11|      biHeight                4 Bytes 
 *   |12|13|            biPlanes                2 Bytes 
 *   |14|15|            biBitCount              2 Bytes 
 *   |16|17|18|19|      biCompression           4 Bytes 
 *   |20|21|22|23|      biSizeImage             4 Bytes 
 *   |24|25|26|27|      biXPelsPerMeter         4 Bytes 
 *   |28|29|30|31|      biYPelsPerMeter         4 Bytes 
 *   |32|33|34|35|      biClrUsed               4 Bytes 
 *   |36|37|38|39|      biClrImportant          4 Bytes 
 * Version 4
 *   |40|41|42|43|      biRedMask               4 Bytes 
 *   |44|45|46|47|      biGreenMask             4 Bytes 
 *   |48|49|50|51|      biBlueMask              4 Bytes 
 *   |52|53|54|55|      biAlphaMask             4 Bytes 
 *   |56|57|58|59|      biCSType                4 Bytes 
 *   |60--95|           biEndpoints             36 Bytes 
 *   |96|97|98|99|      biGammaRed              4 Bytes 
 *   |00|01|02|03|      biGammaGreen            4 Bytes 
 *   |04|05|06|07|      biGammaBlue             4 Bytes 
 * Version 5
 *   |08|09|10|11|      bV5Intent               4 Bytes 
 *   |12|13|14|15|      bV5ProfileData          4 Bytes 
 *   |16|17|18|19|      bV5ProfileSize          4 Bytes 
 *   |20|21|22|23|      bV5Reserved             4 Bytes 
 */

static int
IcoImageHeader(Blt_DBuffer dbuffer, Ico *icoPtr)
{
    unsigned char *bp;

    bp = Blt_DBuffer_Pointer(dbuffer);
    icoPtr->bmih.biSize = IcoGetLong(bp + OFF_SIZE);

    /* Verify header size. */
    switch (icoPtr->bmih.biSize) {
    case SIZEOF_BITMAPOS2V1HEADER:
    case SIZEOF_BITMAPOS2V2HEADER:
    case SIZEOF_BITMAPV3HEADER:
    case SIZEOF_BITMAPV4HEADER:
    case SIZEOF_BITMAPV5HEADER:
        break;
    default:
        IcoError("unknown ICO bitmap header (size=%lu).", icoPtr->bmih.biSize);
    }
    if (icoPtr->bmih.biSize == SIZEOF_BITMAPOS2V1HEADER) {
        icoPtr->bmih.biWidth         = (int)IcoGetShort(bp + OFF_OSV1_WIDTH);
        icoPtr->bmih.biHeight        = (int)IcoGetShort(bp + OFF_OSV1_HEIGHT);
        icoPtr->bmih.biPlanes        = IcoGetShort(bp + OFF_OSV1_PLANES);
        icoPtr->bmih.biBitCount      = IcoGetShort(bp + OFF_OSV1_BIT_COUNT);
        icoPtr->bmih.biCompression   = BI_RGB;
    } else {
        icoPtr->bmih.biWidth         = (int)IcoGetLong(bp + OFF_WIDTH);
        icoPtr->bmih.biHeight        = (int)IcoGetLong(bp + OFF_HEIGHT);
        icoPtr->bmih.biPlanes        = IcoGetShort(bp + OFF_PLANES);
        icoPtr->bmih.biBitCount      = IcoGetShort(bp + OFF_BIT_COUNT);
        icoPtr->bmih.biCompression   = IcoGetLong(bp + OFF_COMPRESSION);
        icoPtr->bmih.biSizeImage     = IcoGetLong(bp + OFF_SIZE_IMAGE);
        icoPtr->bmih.biXPelsPerMeter = (int)IcoGetLong(bp+OFF_X_PELS_PER_METER);
        icoPtr->bmih.biYPelsPerMeter = (int)IcoGetLong(bp+OFF_Y_PELS_PER_METER);
        icoPtr->bmih.biClrUsed       = IcoGetLong(bp + OFF_CLR_USED);
        icoPtr->bmih.biClrImportant  = IcoGetLong(bp + OFF_CLR_IMPORTANT);
    }   

#ifdef notdef
    fprintf(stderr, "fileName=%s\n", icoPtr->name);
    fprintf(stderr, "  biSize=%d\n", icoPtr->bmih.biSize);
    fprintf(stderr, "  biWidth=%d\n", icoPtr->bmih.biWidth);
    fprintf(stderr, "  biHeight=%d\n", icoPtr->bmih.biHeight);
    fprintf(stderr, "  biPlanes=%d\n", icoPtr->bmih.biPlanes);
    fprintf(stderr, "  biBitCount=%d\n", icoPtr->bmih.biBitCount);
    fprintf(stderr, "  biCompression=%d\n", icoPtr->bmih.biCompression);
    fprintf(stderr, "  biSizeImage=%d\n", icoPtr->bmih.biSizeImage);
    fprintf(stderr, "  biClrUsed=%d\n", icoPtr->bmih.biClrUsed);
#endif

    if (Blt_DBuffer_BytesLeft(dbuffer) < icoPtr->bmih.biSize) {
        IcoError("bad ICO header, short file");
    }
    if (icoPtr->bmih.biWidth <= 0) {
        IcoError("invalid image width %d.", icoPtr->bmih.biWidth);
    }
    if (icoPtr->bmih.biHeight == 0) {
        IcoError("invalid image height %d.", icoPtr->bmih.biHeight);
    }
    /* According to the MicroSoft documentation, if the image height is
     * negative, the image data is in top-down order. Since virtually no one
     * does this, read the image data bottom-up. The user can always flip the
     * resulting image. */
    if (icoPtr->bmih.biHeight < 0) {
        icoPtr->bmih.biHeight = -icoPtr->bmih.biHeight;
    }
#ifdef notdef
    if (Blt_DBuffer_Length(dbuffer) < icoPtr->bmfh.bfSize) {
        int old;

        old = Blt_DBuffer_Length(dbuffer);
        Blt_DBuffer_Resize(dbuffer, icoPtr->bmfh.bfSize);
        memset(bp + old, 0, icoPtr->bmfh.bfSize - old);
        Blt_DBuffer_SetLength(dbuffer, icoPtr->bmfh.bfSize);
    }
#endif
    if (icoPtr->bmih.biSize >= SIZEOF_BITMAPV4HEADER) {
        icoPtr->bmih.biRedMask     = IcoGetLong(bp + OFF_RED_MASK);
        icoPtr->bmih.biGreenMask   = IcoGetLong(bp + OFF_GREEN_MASK);
        icoPtr->bmih.biBlueMask    = IcoGetLong(bp + OFF_BLUE_MASK);
        icoPtr->bmih.biAlphaMask   = IcoGetLong(bp + OFF_ALPHA_MASK);
        icoPtr->bmih.biCSType      = IcoGetLong(bp + OFF_CS_TYPE);
        /* Skip CIEXYZ endpoints */
        icoPtr->bmih.biGammaRed    = IcoGetLong(bp + OFF_GAMMA_RED);
        icoPtr->bmih.biGammaGreen  = IcoGetLong(bp + OFF_GAMMA_GREEN);
        icoPtr->bmih.biGammaBlue   = IcoGetLong(bp + OFF_GAMMA_BLUE);
    } 
    if (icoPtr->bmih.biSize >= SIZEOF_BITMAPV5HEADER) {
        icoPtr->bmih.biIntent      = IcoGetLong(bp + OFF_INTENT);
        icoPtr->bmih.biProfileData = IcoGetLong(bp + OFF_PROFILE_DATA);
        icoPtr->bmih.biProfileSize = IcoGetLong(bp + OFF_PROFILE_SIZE);
        icoPtr->bmih.biReserved    = IcoGetLong(bp + OFF_RESERVED3);
    }

#ifdef notdef
    if (icoPtr->bmih.biCSType != 0) {
    fprintf(stderr, "fileName=%s\n", icoPtr->name);
    fprintf(stderr, "  biRedMask=%x\n", icoPtr->bmih.biRedMask);
    fprintf(stderr, "  biGreenMask=%x\n", icoPtr->bmih.biGreenMask);
    fprintf(stderr, "  biBlueMask=%x\n", icoPtr->bmih.biBlueMask);
    fprintf(stderr, "  biAlphaMask=%x\n", icoPtr->bmih.biAlphaMask);
    fprintf(stderr, "  biCSType=%d\n", icoPtr->bmih.biCSType);
    fprintf(stderr, "  biGammaRed=%d\n", icoPtr->bmih.biGammaRed);
    fprintf(stderr, "  biGammaGreen=%d\n", icoPtr->bmih.biGammaGreen);
    fprintf(stderr, "  biGammaBlue=%d\n", icoPtr->bmih.biGammaBlue);
    fprintf(stderr, "  biIntent=%d\n", icoPtr->bmih.biIntent);
    fprintf(stderr, "  biProfileData=%d\n", icoPtr->bmih.biProfileData);
    fprintf(stderr, "  biProfileSize=%d\n", icoPtr->bmih.biProfileSize);
    fprintf(stderr, "  biReserved=%d\n", icoPtr->bmih.biReserved);
    }
#endif

    /* Verify bits per pixel count and colors used. */
    switch (icoPtr->bmih.biBitCount) {
    case 1:                             /* 2-bits, Monochrome */
        if (icoPtr->bmih.biClrUsed > 2) {
            IcoError("wrong # colors (%d), expecting <= 2 colors.", 
                     icoPtr->bmih.biClrUsed);
        }
        if (icoPtr->bmih.biClrUsed == 0) {
            icoPtr->bmih.biClrUsed = 2;
        }
        break;
    case 4:                             /* 4-bits, 16 colors. */
        if (icoPtr->bmih.biClrUsed > 16) {
            IcoError("wrong # colors (%d), expecting <= 16 colors.",
                     icoPtr->bmih.biClrUsed);
        }
        if (icoPtr->bmih.biClrUsed == 0) {
            icoPtr->bmih.biClrUsed = 16;
        }
        break;
    case 8:                             /* 8-bits, 256 colors */
        if (icoPtr->bmih.biClrUsed > 256) {
            IcoError("wrong # colors (%d), expecting <= 256 colors.",
                     icoPtr->bmih.biClrUsed);
        }
        if (icoPtr->bmih.biClrUsed == 0) {
            icoPtr->bmih.biClrUsed = 256;
        }
        break;
    case 16:
    case 24:
    case 32:                            /* True color. */
        if (icoPtr->bmih.biClrUsed != 0) {
            IcoWarning("# colors is %d, expecting 0 colors in %d-bit image.",
                       icoPtr->bmih.biClrUsed, icoPtr->bmih.biBitCount);
            icoPtr->bmih.biClrUsed = 0;
        }
        break;
    default:
        IcoError("invalid # bits per pixel (%d)", icoPtr->bmih.biBitCount);
        break;
    }   

    /* Verify compression type. */
    switch (icoPtr->bmih.biCompression) {
    case BI_RGB:
        break;
    case BI_BITFIELDS:
        if ((icoPtr->bmih.biBitCount != 16) &&
            (icoPtr->bmih.biBitCount != 32)) {
            IcoError("wrong # bits per pixel (%d) for BITFIELD compression",
                     icoPtr->bmih.biBitCount);
        }
        break;
    case BI_RLE4:
    case BI_RLE8:
    case BI_PNG:
    case BI_JPEG:
        IcoError("compression type \"%s\" not implemented",
                 compression_types[icoPtr->bmih.biCompression]);
        break;
    default:
        IcoError("unknown compression type (%d)", icoPtr->bmih.biCompression);
    }      
    Blt_DBuffer_IncrCursor(dbuffer, icoPtr->bmih.biSize);
    return TRUE;
}

static int
IcoImagePalette(Blt_DBuffer dbuffer, Ico *icoPtr)
{
    unsigned char *bp;

    bp = Blt_DBuffer_Pointer(dbuffer);
    if (icoPtr->bmih.biClrUsed == 0) {
        if (icoPtr->bmih.biSize == SIZEOF_BITMAPV3HEADER) {
            icoPtr->bmih.biRedMask = IcoGetLong(bp);
            icoPtr->bmih.biGreenMask = IcoGetLong(bp + 4);
            icoPtr->bmih.biBlueMask = IcoGetLong(bp + 8);
            icoPtr->bmih.biAlphaMask = 0;
        }
    } else {
        int sizeElem;
        int i;

        assert(icoPtr->bmih.biClrUsed <= 256);
        sizeElem = (icoPtr->bmih.biSize == SIZEOF_BITMAPOS2V1HEADER) ? 3 : 4;
        if (Blt_DBuffer_BytesLeft(dbuffer) < 
            (icoPtr->bmih.biClrUsed * sizeElem)) {
            IcoError("short file");
        }
        for (i = 0; i < icoPtr->bmih.biClrUsed; i++, bp += sizeElem) {
            /* Colormap components are ordered BGBA. */
            icoPtr->colorTable[i].Blue  = bp[0];
            icoPtr->colorTable[i].Green = bp[1];
            icoPtr->colorTable[i].Red   = bp[2];
            icoPtr->colorTable[i].Alpha = ALPHA_OPAQUE;
        }
    }
    Blt_DBuffer_SetPointer(dbuffer, bp);
    return TRUE;
}

static Blt_Picture
IcoRgbImageData(Blt_DBuffer dbuffer, Ico *icoPtr)
{
    Blt_Pixel *destRowPtr;
    Picture *destPtr;
    unsigned char *srcBits, *srcRowPtr;
    unsigned int bytesPerRow, wordsPerRow, maskBytesPerRow, maskWordsPerRow;
    int w, h, h2;
    int y;

    w = icoPtr->bmih.biWidth;
    h = icoPtr->bmih.biHeight;
    h2 = h / 2;
    wordsPerRow = (w * icoPtr->bmih.biBitCount + 31) / 32;
    bytesPerRow = wordsPerRow * 4;

    maskWordsPerRow = (w + 31) / 32;
    maskBytesPerRow = maskWordsPerRow * 4;

    srcBits = Blt_DBuffer_Pointer(dbuffer);
    if (Blt_DBuffer_BytesLeft(dbuffer) < 
        (bytesPerRow * h2 + maskBytesPerRow * h2)) {
        IcoError("image size is %d, need %u bytes", 
                Blt_DBuffer_BytesLeft(dbuffer), 
                 bytesPerRow * h2 + maskBytesPerRow * h2);
    }
    destPtr = Blt_CreatePicture(w, h2);
    /* Start from the bottom of the destination picture. */
    destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * h2);
    srcRowPtr = srcBits;
    switch (icoPtr->bmih.biBitCount) {
    case 1:
        for (y = 0; y < h2; y++) {
            Blt_Pixel *dp, *dend;
            
            destRowPtr -= destPtr->pixelsPerRow;
            for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
                int x;
                unsigned char byte;
                x = dp - destRowPtr;
                
                byte = srcRowPtr[x>>3];
                dp->u32 = (byte & (0x80 >> (x & 7))) ? 
                    icoPtr->colorTable[1].u32 : icoPtr->colorTable[0].u32;
                dp->Alpha = ALPHA_OPAQUE;
            }
            srcRowPtr += bytesPerRow;
        }
        break;
    case 4:
        for (y = 0; y < h2; y++) {
            Blt_Pixel *dp, *dend;
            unsigned char *sp;
            
            destRowPtr -= destPtr->pixelsPerRow;
            sp = srcRowPtr;
            for (dp = destRowPtr, dend = dp + w; dp < dend; /*empty*/) {
                unsigned int pixel;
                
                pixel = ((sp[0] >> 4) & 0x0F);
                dp->u32 = icoPtr->colorTable[pixel].u32;
                dp++;
                pixel = (sp[0] & 0x0F);
                dp->u32 = icoPtr->colorTable[pixel].u32;
                dp++;
                sp++;
            }
            srcRowPtr += bytesPerRow;
        }
        break;
    case 8:
        for (y = 0; y < h2; y++) {
            Blt_Pixel *dp, *dend;
            unsigned char *sp;
            
            destRowPtr -= destPtr->pixelsPerRow;
            sp = srcRowPtr;
            for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
                dp->u32 = icoPtr->colorTable[*sp].u32;
                sp++;
            }
            srcRowPtr += bytesPerRow;
        }
        break;
    case 16:
        for (y = 0; y < h2; y++) {
            Blt_Pixel *dp, *dend;
            unsigned char *sp;
            
            destRowPtr -= destPtr->pixelsPerRow;
            sp = srcRowPtr;
            for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
                unsigned int pixel;
                
                pixel = IcoGetShort(sp);
                dp->Blue  = (pixel & 0x001f) << 3;
                dp->Green = (pixel & 0x03e0) >> 2;
                dp->Red   = (pixel & 0x7c00) >> 7;
                dp->Alpha = ALPHA_OPAQUE;
                sp += 2;
            }
            srcRowPtr += bytesPerRow;
        }
        break;
    case 24:
        for (y = 0; y < h2; y++) {
            Blt_Pixel *dp, *dend;
            unsigned char *sp;
            
            destRowPtr -= destPtr->pixelsPerRow;
            sp = srcRowPtr;
            for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
                dp->Blue  = sp[0];
                dp->Green = sp[1];
                dp->Red   = sp[2];
                dp->Alpha = ALPHA_OPAQUE;
                sp += 3;
            }
            srcRowPtr += bytesPerRow;
        }
        break;
    case 32:
        for (y = 0; y < h2; y++) {
            Blt_Pixel *dp, *dend;
            unsigned char *sp;
            
            destRowPtr -= destPtr->pixelsPerRow;
            sp = srcRowPtr;
            for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
                dp->Blue  = sp[0];
                dp->Green = sp[1];
                dp->Red   = sp[2];
                dp->Alpha = sp[3];
                sp += 4;
            }
            srcRowPtr += bytesPerRow;
        }
    }
    /* Combine the monochrome mask 0=opaque, 1=transparent. */

    /* Start from the bottom of the destination picture. */
    destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * h2);
    for (y = 0; y < h2; y++) {
        Blt_Pixel *dp, *dend;
        
        destRowPtr -= destPtr->pixelsPerRow;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
            int x;
            unsigned char byte;
            x = dp - destRowPtr;
            
            byte = srcRowPtr[x>>3];
            if (byte & (0x80 >> (x & 7))) {
                dp->Alpha = ALPHA_TRANSPARENT;
            }
        }
        srcRowPtr += maskBytesPerRow;
    }
    Blt_ClassifyPicture(destPtr);
    destPtr->flags &= ~BLT_PIC_UNINITIALIZED;
    return destPtr;
}

/*-------------------------------------------------------------------------------
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
 * FindShift --
 *
 *      Returns the position of the least significant (low) bit in the given
 *      mask.
 *
 * Results:
 *      The number of the least significant bit.
 *
 *---------------------------------------------------------------------------
 */
static int
FindShift(unsigned int mask)
{
    int bit;

    for (bit = 0; bit < 32; bit++) {
        if (mask & (1 << bit)) {
            break;
        }
    }
    return bit;
}

static int
GetAdjust(unsigned int mask)
{
    int n;
    int adjust = 0;

    n = CountBits(mask);
    if (n < 8) {
        adjust = 8 - n;
    }
    return adjust;
}

static Blt_Picture
IcoBitfieldImageData(Blt_DBuffer dbuffer, Ico *icoPtr)
{
    Picture *destPtr;
    unsigned char *srcBits, *srcRowPtr;
    unsigned int bytesPerRow, wordsPerRow, maskBytesPerRow, maskWordsPerRow;
    int y, w, h, h2;
    unsigned int rShift, gShift, bShift, aShift;
    unsigned int rMask, gMask, bMask, aMask;
    unsigned int rAdjust, gAdjust, bAdjust, aAdjust;
    Blt_Pixel *destRowPtr;
        

    fprintf(stderr, "IcoBitfieldImageData\n");
    rMask = icoPtr->bmih.biRedMask;
    gMask = icoPtr->bmih.biGreenMask;
    bMask = icoPtr->bmih.biBlueMask;
    aMask = icoPtr->bmih.biAlphaMask;

    rShift = FindShift(rMask);
    gShift = FindShift(gMask);
    bShift = FindShift(bMask);
    aShift = FindShift(aMask);

    rAdjust = GetAdjust(rMask);
    gAdjust = GetAdjust(gMask);
    bAdjust = GetAdjust(bMask);
    aAdjust = GetAdjust(aMask);

    w = icoPtr->bmih.biWidth;
    h = icoPtr->bmih.biHeight;
    h2 = h / 2;
    wordsPerRow = (w * icoPtr->bmih.biBitCount + 31) / 32;
    bytesPerRow = wordsPerRow * 4;

    maskWordsPerRow = (w + 31) / 32;
    maskBytesPerRow = maskWordsPerRow * 4;

    srcBits = Blt_DBuffer_Pointer(dbuffer);
    destPtr = Blt_CreatePicture(w, h2);
    srcRowPtr = srcBits;
    if (icoPtr->bmih.biBitCount == 32) {
        destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * h2);
        for (y = 0; y < h2; y++) {
            Blt_Pixel *dp, *dend;
            unsigned char *sp;
            
            destRowPtr -= destPtr->pixelsPerRow;
            sp = srcRowPtr;
            for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
                unsigned int pixel;
                
                pixel = IcoGetLong(sp);
                dp->Red   = ((pixel & rMask) >> rShift) << rAdjust;
                dp->Green = ((pixel & gMask) >> gShift) << gAdjust;
                dp->Blue  = ((pixel & bMask) >> bShift) << bAdjust;
                dp->Alpha = ((pixel & aMask) >> aShift) << aAdjust;
                sp += 4;
            }
            srcRowPtr += bytesPerRow;
        }
    } else if (icoPtr->bmih.biBitCount == 16) {
        destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * h2);
        for (y = 0; y < h2; y++) {
            Blt_Pixel *dp, *dend;
            unsigned char *sp;
            
            destRowPtr -= destPtr->pixelsPerRow;
            sp = srcRowPtr;
            for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
                unsigned int pixel;
                
                pixel = IcoGetShort(sp);
                dp->Red   = ((pixel & rMask) >> rShift) << rAdjust;
                dp->Green = ((pixel & gMask) >> gShift) << gAdjust;
                dp->Blue  = ((pixel & bMask) >> bShift) << bAdjust;
                dp->Alpha = ((pixel & aMask) >> aShift) << aAdjust;
                sp += 2;
            }
            srcRowPtr += bytesPerRow;
        }
    } else {
        return NULL;
    }

    /* Combine the 1-bit mask */

    /* Start from the bottom of the destination picture. */
    destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * h2);
    for (y = 0; y < h2; y++) {
        Blt_Pixel *dp, *dend;
        
        destRowPtr -= destPtr->pixelsPerRow;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
            int x;
            unsigned char byte;
            x = dp - destRowPtr;
            
            byte = srcRowPtr[x>>3];
            if (byte & (0x80 >> (x & 7))) {
                dp->Alpha = ALPHA_TRANSPARENT;
            }
        }
        srcRowPtr += maskBytesPerRow;
    }
    Blt_ClassifyPicture(destPtr);
    destPtr->flags &= ~BLT_PIC_UNINITIALIZED;
    return destPtr;
}

static Blt_Picture
IcoImageData(Blt_DBuffer dbuffer, Ico *icoPtr)
{
    Blt_Picture picture;

    picture = NULL;
    if (Blt_DBuffer_BytesLeft(dbuffer) < (icoPtr->bmih.biSizeImage)) {
        IcoError("short file: not enough bytes for image data");
    }
    switch(icoPtr->bmih.biCompression) {
    case BI_RGB:
        picture = IcoRgbImageData(dbuffer, icoPtr);
        break;
    case BI_BITFIELDS:
        picture = IcoBitfieldImageData(dbuffer, icoPtr);
        break;
    case BI_RLE4:
    case BI_RLE8:
    case BI_PNG:
    case BI_JPEG:
        IcoError("compression type \"%s\" not implemented",
                 compression_types[icoPtr->bmih.biCompression]);
        break;
    }
    return picture;
}

/*
 *     0  2     Reserved
 *     2  2     Type 
 *     4  2     Number of Images
 */
static int
IcoHeader(Blt_DBuffer dbuffer, Ico *icoPtr)
{
    unsigned char *bp;
    int reserved;

    Blt_DBuffer_Rewind(dbuffer);
    if (Blt_DBuffer_BytesLeft(dbuffer) < 6) {
        return FALSE;
    }
    bp = Blt_DBuffer_Pointer(dbuffer);
    reserved = IcoGetShort(bp);
    if (reserved != 0) {
        return FALSE;
    }
    icoPtr->type = IcoGetShort(bp + 2);
    if ((icoPtr->type != TYPE_ICO) && (icoPtr->type != TYPE_CUR)) {
        return FALSE;                   /* Not a CUR or ICO image type. */
    }
    icoPtr->numImages = IcoGetShort(bp + 4);
    if (icoPtr->numImages == 0) {
        return FALSE;                   /* No images in directory. */
    }
    Blt_DBuffer_IncrCursor(dbuffer, 6);
    return TRUE;
}

/*
 *     0  1     Image Width
 *     1  1     Image Height 
 *     2  1     Number of colors in palette.
 *     3  1     Reserved
 *     4  2     Pixel Aspect Ratio
 */
static Blt_Picture
IcoNextImage(Tcl_Interp *interp, Blt_DBuffer dbuffer, Ico *icoPtr)
{
    unsigned char *bp;
    int w, h;
    int imgSize, offset, biSize;
    Blt_Picture picture;
    int last;

    if (Blt_DBuffer_BytesLeft(dbuffer) <  16) {
        IcoError("short file: expected 16 bytes for ICO directory entry");
    }
    bp = Blt_DBuffer_Pointer(dbuffer);
    /* Logical Screen Descriptor. */
    w = bp[0];
    if (w == 0) {
        w = 256;
    }
    h = bp[1];
    if (h == 0) {
        h = 256;
    }
    if (bp[3] != 0) {
        IcoError("entry reserved bit should be zero");
    }
    imgSize = IcoGetLong(bp + 8);
    offset  = IcoGetLong(bp + 12);

    if ((offset + imgSize) > Blt_DBuffer_Length(dbuffer)) {
        IcoError("short file: image offset and size exceeds file size");
    }
    Blt_DBuffer_IncrCursor(dbuffer, SIZEOF_ICODIRENTRY);
    last = Blt_DBuffer_Cursor(dbuffer);
    picture = NULL;
    Blt_DBuffer_SetCursor(dbuffer, offset);
    bp = Blt_DBuffer_Pointer(dbuffer);
    biSize = IcoGetLong(bp);
    if (biSize == SIZEOF_BITMAPV3HEADER) {
        if (!IcoImageHeader(dbuffer, icoPtr)) {
            IcoError("bad ICO logical screen descriptor");
        }
        if (!IcoImagePalette(dbuffer, icoPtr)) {
            IcoError("bad ICO color table");
        }
        picture = IcoImageData(dbuffer, icoPtr);
    } else if (icoPtr->fmtPtr != NULL) {
        Blt_DBuffer tmp;

        tmp = Blt_DBuffer_Create();
        Blt_DBuffer_AppendData(tmp, bp, imgSize);
        if ((*icoPtr->fmtPtr->isFmtProc)(tmp)) {
            Blt_Chain chain;

            chain = (*icoPtr->fmtPtr->readProc)(interp, "-data", tmp);
            picture = Blt_Chain_FirstValue(chain);
        }
        Blt_DBuffer_Destroy(tmp);
    }
    if (picture == NULL) {
        IcoError("unknown type of image biSize=%d", biSize);
    }
    Blt_DBuffer_SetCursor(dbuffer, last);
    return picture;
}

/*
 *---------------------------------------------------------------------------
 *
 * IcoToPictures --
 *
 *      Reads a ICO file, converts it into a picture, and appends it
 *      to the list of images.  We only handle only single ICO images.
 *
 * Results:
 *      The picture is returned.  If an error occured, such
 *      as the designated file could not be opened, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain 
IcoToPictures(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer,
              IcoImportSwitches *switchesPtr)
{
    Blt_Chain chain;
    Ico ico;
    IcoMessage message;
    int i;

    icoMessagePtr = &message;
    memset(&ico, 0, sizeof(ico));       /* Clear the structure. */
    message.numWarnings = 0;
    ico.name = fileName;

    Tcl_DStringInit(&message.errors);
    Tcl_DStringInit(&message.warnings);
    Tcl_DStringAppend(&message.errors, "error reading \"", -1);
    Tcl_DStringAppend(&message.errors, fileName, -1);
    Tcl_DStringAppend(&message.errors, "\": ", -1);

    Tcl_DStringAppend(&message.warnings, "\"", -1);
    Tcl_DStringAppend(&message.warnings, fileName, -1);
    Tcl_DStringAppend(&message.warnings, "\": ", -1);

    chain = NULL;
    if (setjmp(message.jmpbuf)) {
        Tcl_DStringResult(interp, &message.errors);
        Tcl_DStringFree(&message.warnings);
        if (chain != NULL) {
            /* Run through the chain freeing pictures. */
        }
        return NULL;
    }
    ico.fmtPtr = Blt_FindPictureFormat(interp, "png");
    ico.name = fileName;
    if (!IcoHeader(dbuffer, &ico)) {
        IcoError("bad ICO header");
    }
    chain = Blt_Chain_Create();
    for (i = 0; i < ico.numImages; i++) {
        Blt_Picture picture;

        picture = IcoNextImage(interp, dbuffer, &ico);
        Blt_Chain_Append(chain, picture);
    }   
    if (message.numWarnings > 0) {
        Tcl_SetErrorCode(interp, "PICTURE", "ICO_READ_WARNINGS", 
                Tcl_DStringValue(&message.warnings), (char *)NULL);
    } else {
        Tcl_SetErrorCode(interp, "NONE", (char *)NULL);
    }
    Tcl_DStringFree(&message.warnings);
    Tcl_DStringFree(&message.errors);
    return chain;
}

/*
 *---------------------------------------------------------------------------
 *
 * IcoWriteImageData --
 *
 *      Reads an picture into ICO file format.  May contain one or more
 *      images.
 *
 * Results:
 *      The picture is returned.  If an error occured, such as the designated
 *      file could not be opened, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
IcoWriteImageData(Tcl_Interp *interp, Blt_Picture original, Blt_DBuffer dbuffer,
                  IcoExportSwitches *switchesPtr)
{
    int numColors;
    int format;
    int bitsPerPixel;
    Picture *srcPtr;
    Blt_HashTable colorTable;
    unsigned int wordsPerRow, bytesPerRow, maskWordsPerRow, maskBytesPerRow;
    unsigned int imageSize, dataSize, infoHeaderSize, offsetToData;
    unsigned char *bp, *destBits;

    /* Write the ICO image as either 32-bit RGB or ARGB image or PNG
     * image.  It's not worth it to create bit-masked versions because
     * it's increasing unlikely the icon will be 1-bit masked. */
    srcPtr = original;
    if (switchesPtr->flags & EXPORT_ALPHA) {
        bitsPerPixel = 32;
        format = BI_BITFIELDS;
        numColors = 0;
        infoHeaderSize = SIZEOF_BITMAPV4HEADER;
    } 

    numColors = Blt_QueryColors(srcPtr, (Blt_HashTable *)NULL);
    format = BI_RGB;
    infoHeaderSize = SIZEOF_BITMAPV3HEADER;
    if (Blt_Picture_IsOpaque(srcPtr)) {
        if (numColors <= 256) {
            bitsPerPixel = 8;
            Blt_InitHashTable(&colorTable, BLT_ONE_WORD_KEYS);
            numColors = Blt_QueryColors(srcPtr, &colorTable);
        } else {
            bitsPerPixel = 24;
            numColors = 0;
        }
    } else {
        bitsPerPixel = 32;
        numColors = 0;
    }
    wordsPerRow = (srcPtr->width * bitsPerPixel + 31) / 32;
    bytesPerRow = wordsPerRow * 4;
    imageSize = bytesPerRow * srcPtr->height;

    maskWordsPerRow = (srcPtr->width + 31) / 32;
    maskBytesPerRow = maskWordsPerRow * 4;
    imageSize += maskBytesPerRow * srcPtr->height;

    /*
     * Compute the size of the structure.
     *
     * header + infoheader + colortable + imagedata
     *
     * 108 + 0-256 * 4  + bytesPerRow * height;
     */
    dataSize = imageSize + (numColors * 4) + infoHeaderSize;
    
    offsetToData = infoHeaderSize + (numColors * 4);

    bp = Blt_DBuffer_Extend(dbuffer, dataSize);
    memset(bp, 0, dataSize);

    /* Image header. */
    IcoSetLong (bp + OFF_SIZE,             dataSize);
    IcoSetLong (bp + OFF_WIDTH,            srcPtr->width);
    IcoSetLong (bp + OFF_HEIGHT,           srcPtr->height * 2);
    IcoSetShort(bp + OFF_PLANES,           1);  
    IcoSetShort(bp + OFF_BIT_COUNT,        bitsPerPixel);       
    IcoSetLong (bp + OFF_COMPRESSION,      format); 
    IcoSetLong (bp + OFF_SIZE_IMAGE,       bytesPerRow * srcPtr->height);
    IcoSetLong (bp + OFF_X_PELS_PER_METER, 0);  
    IcoSetLong (bp + OFF_Y_PELS_PER_METER, 0);  
    IcoSetLong (bp + OFF_CLR_USED,         numColors);          
    IcoSetLong (bp + OFF_CLR_IMPORTANT,    0);  

    /* Color table. */
    if (bitsPerPixel == 8) {
        unsigned long i;
        Blt_HashEntry *hPtr;
        Blt_HashSearch cursor;

        i = 0;
        bp += infoHeaderSize;
        for (hPtr = Blt_FirstHashEntry(&colorTable, &cursor); hPtr != NULL;
             hPtr = Blt_NextHashEntry(&cursor)) {
            Blt_Pixel pixel;
            unsigned long key;

            Blt_SetHashValue(hPtr, i);
            key = (unsigned long)Blt_GetHashKey(&colorTable, hPtr);
            pixel.u32 = (unsigned int)key;

            /* Colormap components are ordered BGBA. */
            bp[0] = pixel.Blue;
            bp[1] = pixel.Green;
            bp[2] = pixel.Red;
            bp[3] = pixel.Alpha;
            bp += 4;
            i++;
        }
        assert(i == numColors);
    }

    destBits = Blt_DBuffer_Pointer(dbuffer) + offsetToData;

    /* Image data. */
    switch (bitsPerPixel) {
    case 32:
        {
            Blt_Pixel *srcRowPtr;
            int y;
            unsigned char *destRowPtr;

            destRowPtr = destBits;
            srcRowPtr = srcPtr->bits+((srcPtr->height-1)*srcPtr->pixelsPerRow);
            for (y = 0; y < srcPtr->height; y++) {
                Blt_Pixel *sp, *send;
                unsigned char *dp;

                dp = destRowPtr;
                for (sp = srcRowPtr, send = sp+srcPtr->width; sp < send; sp++) {
                    IcoSetLong(dp, sp->u32);
                    dp += 4;
                }
                destRowPtr += bytesPerRow;
                srcRowPtr  -= srcPtr->pixelsPerRow;
            }
            assert((destRowPtr - Blt_DBuffer_Pointer(dbuffer)) == dataSize);
        }
        break;

    case 24:
        {
            Blt_Pixel *srcRowPtr;
            int y;
            unsigned char *destRowPtr;
            
            destRowPtr = destBits;
            srcRowPtr = srcPtr->bits+((srcPtr->height-1)*srcPtr->pixelsPerRow);
            for (y = 0; y < srcPtr->height; y++) {
                Blt_Pixel *sp, *send;
                unsigned char *dp;

                dp = destRowPtr;
                for (sp = srcRowPtr, send = sp+srcPtr->width; sp < send; sp++) {
                    dp[0] = sp->Blue;
                    dp[1] = sp->Green;
                    dp[2] = sp->Red;
                    dp += 3;
                }
                destRowPtr += bytesPerRow;
                srcRowPtr  -= srcPtr->pixelsPerRow;
            }
            assert((destRowPtr - Blt_DBuffer_Pointer(dbuffer)) == dataSize);
        }
        break;

    case 8:
        {
            Blt_Pixel *srcRowPtr;
            int y;
            unsigned char *destRowPtr;

            destRowPtr = destBits;
            srcRowPtr = srcPtr->bits+((srcPtr->height-1)*srcPtr->pixelsPerRow);
            for (y = 0; y < srcPtr->height; y++) {
                Blt_Pixel *sp, *send;
                unsigned char *dp;
                
                dp = destRowPtr;
                for (sp = srcRowPtr, send = sp+srcPtr->width; sp < send; sp++) {
                    Blt_HashEntry *hPtr;
                    unsigned long index;
                    union {
                        Blt_Pixel color;
                        char *key;
                    } value;

                    value.color.u32 = sp->u32;
                    value.color.Alpha = 0xFF;
                    hPtr = Blt_FindHashEntry(&colorTable, value.key);
                    if (hPtr == NULL) {
#ifdef notdef
                        Blt_Warn("can't find %x\n", sp->u32);
#endif
                        continue;
                    }
                    index = (unsigned long)Blt_GetHashValue(hPtr);
                    *dp = (unsigned char)(index & 0xFF);
                    dp++;
                }
                destRowPtr += bytesPerRow;
                srcRowPtr  -= srcPtr->pixelsPerRow;
            }
            assert((destRowPtr - Blt_DBuffer_Pointer(dbuffer)) == dataSize);
        }
        break;
    }
    if (bitsPerPixel == 8) {
        Blt_DeleteHashTable(&colorTable);
    }
    if (srcPtr != original) {
        Blt_FreePicture(srcPtr);
    }
    return dataSize;
}

/*
 *---------------------------------------------------------------------------
 *
 * PictureToIco --
 *
 *      Converts a single picture into ICO file format.  
 *
 * Results:
 *      A standard TCL result.  If an error occured, an error message will be
 *      left in the interpreter.
 *
 *---------------------------------------------------------------------------
 */
static int
PictureToIco(Tcl_Interp *interp, Blt_Picture original, Blt_DBuffer dbuffer,
              IcoExportSwitches *switchesPtr)
{
    unsigned int offset;
    unsigned char *bp;
    Blt_DBuffer tmp;
    size_t size;
    size_t numColors, bpp;
    Picture *srcPtr;

    srcPtr = original;
    if ((srcPtr->width > 256) || (srcPtr->height > 256)) {
        Tcl_AppendResult(interp, 
                         "picture contains image too big for ICO format",
                         (char *)NULL);
        return TCL_ERROR;
    }
    if ((!Blt_Picture_IsOpaque(srcPtr)) && 
        ((switchesPtr->flags & EXPORT_ALPHA) == 0)) {
        Blt_Picture background;
            
        /* Blend picture with solid color background. */
        background = Blt_CreatePicture(srcPtr->width, srcPtr->height);
        Blt_BlankPicture(background, switchesPtr->bg.u32); 
        Blt_CompositePictures(background, srcPtr);
        if (srcPtr != original) {
            Blt_FreePicture(srcPtr);
        }
        srcPtr = background;
    }

    /* Write the 6-byte ICO header. */
    /*
     *     0  2 Reserved
     *     2  2     Type 
     *     4  2     Number of Images
     */
    Blt_DBuffer_Extend(dbuffer, 6);
    bp = Blt_DBuffer_Pointer(dbuffer);
    IcoSetShort(bp, 0);
    IcoSetShort(bp + 2, TYPE_ICO);
    IcoSetLong (bp + 4, 1);             /* Single image. */
    bp += SIZEOF_ICOHEADER;

    /* Write the image directory. Accumulate image data in a second dynamic
     * buffer.  We'll append that to the end of the directory listing. */
    offset = SIZEOF_ICOHEADER + SIZEOF_ICODIRENTRY;
    tmp = Blt_DBuffer_Create();         /* Buffer to hold image data. */
    
    /* Create room for the image directory. */
    Blt_DBuffer_Extend(dbuffer, SIZEOF_ICODIRENTRY);  
    bp = Blt_DBuffer_Pointer(dbuffer);
    
    numColors = Blt_QueryColors(srcPtr, (Blt_HashTable *)NULL);
    size = IcoWriteImageData(interp, srcPtr, tmp, switchesPtr);
    bp[0] = (srcPtr->width <= 255) ? srcPtr->width : 0;
    bp[1] = (srcPtr->height <= 255) ? srcPtr->height : 0;
    bp[2] = (numColors > 255) ? 0 : numColors;
    bp[3] = 0;                          /* Reserved. */
    IcoSetShort(bp + 4, 0);             /* # color planes (ICO) or x-coordinate
                                         * hotspot (CUR). */
    bpp = 32;
    IcoSetShort(bp + 6, bpp);           /* Bits per pixel (ICO) or
                                         * y-coordinate hotspot (CUR). */
    IcoSetLong (bp + 8, size);          /* Image size. */
    IcoSetLong (bp + 12, offset);       /* Offset. */
    Blt_DBuffer_Concat(dbuffer, tmp);
    Blt_DBuffer_Destroy(tmp);
    if (srcPtr != original) {
        Blt_FreePicture(srcPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PicturesToIco --
 *
 *      Converts a list of images into ICO file format.  
 *
 * Results:
 *      A standard TCL result.  If an error occured, an error message will be
 *      left in the interpreter.
 *
 *---------------------------------------------------------------------------
 */
static int
PicturesToIco(Tcl_Interp *interp, Blt_Chain chain, Blt_DBuffer dbuffer,
              IcoExportSwitches *switchesPtr)
{
    unsigned int offset;
    unsigned char *bp;
    int numImages;
    Blt_DBuffer tmp;
    Blt_ChainLink link;

    /* Verify that all the images dimensions are less than 257 pixels. */
    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        Picture *srcPtr;

        srcPtr = Blt_Chain_GetValue(link);
        if ((srcPtr->width > 256) || (srcPtr->height > 256)) {
            Tcl_AppendResult(interp, 
                "picture contains image too big for ICO format",
                (char *)NULL);
            return TCL_ERROR;
        }
    }
    /* Write the 6-byte ICO header. */
    /*
     *     0  2 Reserved
     *     2  2     Type 
     *     4  2     Number of Images
     */
    Blt_DBuffer_Extend(dbuffer, 6);
    numImages = Blt_Chain_GetLength(chain);
    bp = Blt_DBuffer_Pointer(dbuffer);
    IcoSetShort(bp, 0);
    IcoSetShort(bp + 2, TYPE_ICO);
    IcoSetLong (bp + 4, numImages);
    bp += SIZEOF_ICOHEADER;

    /* Write the image directory. Accumulate image data in a second dynamic
     * buffer.  We'll append that to the end of the directory listing. */
    offset = SIZEOF_ICOHEADER + (numImages * SIZEOF_ICODIRENTRY);
    tmp = Blt_DBuffer_Create();         /* Buffer to hold image data. */

    /* Create room for the image directory. */
    Blt_DBuffer_Extend(dbuffer, SIZEOF_ICODIRENTRY * numImages);  
    bp = Blt_DBuffer_Pointer(dbuffer);
    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        Picture *srcPtr, *original;
        size_t size, bpp;
        size_t numColors, numPlanes;

        srcPtr = original = Blt_Chain_GetValue(link);
        if ((!Blt_Picture_IsOpaque(srcPtr)) && 
            ((switchesPtr->flags & EXPORT_ALPHA) == 0)) {
            Blt_Picture background;
            
            /* Blend picture with solid color background. */
            background = Blt_CreatePicture(srcPtr->width, srcPtr->height);
            Blt_BlankPicture(background, switchesPtr->bg.u32); 
            Blt_CompositePictures(background, srcPtr);
            if (srcPtr != original) {
                Blt_FreePicture(srcPtr);
            }
            srcPtr = background;
        }
        numColors = Blt_QueryColors(srcPtr, (Blt_HashTable *)NULL);
        size = IcoWriteImageData(interp, srcPtr, tmp, switchesPtr);
        bp[0] = (srcPtr->width <= 255) ? srcPtr->width : 0;
        bp[1] = (srcPtr->height <= 255) ? srcPtr->height : 0;
        bp[2] = (numColors > 255) ? 0 : numColors;
        bp[3] = 0;                      /* Reserved. */
        numPlanes = 1;
        IcoSetShort(bp + 4, numPlanes); /* # color planes (ICO) or
                                         * # x-coordinate * hotspot (CUR). */
        bpp = 32;               
        IcoSetShort(bp + 6, bpp);       /* Bits per pixel (ICO) or
                                         * y-coordinate hotspot (CUR). */
        IcoSetLong (bp + 8, size);      /* Image size. */
        IcoSetLong (bp + 12, offset);   /* Offset. */
        offset += size;
        bp += SIZEOF_ICODIRENTRY;
        if (srcPtr != original) {
            Blt_FreePicture(srcPtr);
        }
    }
    Blt_DBuffer_Concat(dbuffer, tmp);
    Blt_DBuffer_Destroy(tmp);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsIco --
 *
 *      Attempts to parse a ICO file header.
 *
 * Results:
 *      Returns 1 is the header is ICO and 0 otherwise.  Note that
 *      the validity of the header contents is not checked here.  That's
 *      done in IcoToPictures.
 *
 *---------------------------------------------------------------------------
 */
static int
IsIco(Blt_DBuffer dbuffer)
{
    Ico ico;

    return IcoHeader(dbuffer, &ico);
}

static Blt_Chain
ReadIco(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer)
{
    IcoImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    return IcoToPictures(interp, fileName, dbuffer, &switches);
}

static Tcl_Obj *
WriteIco(Tcl_Interp *interp, Blt_Picture picture)
{
    Blt_DBuffer dbuffer;
    IcoExportSwitches switches;
    Tcl_Obj *objPtr;

    /* Default export switch settings. */
    memset(&switches, 0, sizeof(switches));
    switches.bg.u32 = 0xFFFFFFFF;       /* white */

    dbuffer = Blt_DBuffer_Create();
    objPtr = NULL;
    if (PictureToIco(interp, picture, dbuffer, &switches) == TCL_OK) {
        objPtr = Blt_DBuffer_Base64EncodeToObj(dbuffer);
    }
    return objPtr;
}

static Blt_Chain
ImportIco(
    Tcl_Interp *interp, 
    int objc,
    Tcl_Obj *const *objv,
    const char **fileNamePtr)
{
    Blt_DBuffer dbuffer;
    Blt_Chain chain;
    const char *string;
    IcoImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3, objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
        return NULL;
    }
    if ((switches.dataObjPtr != NULL) && (switches.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "more than one import source: ",
                "use only one -file or -data flag.", (char *)NULL);
        Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
        return NULL;
    }
    dbuffer = Blt_DBuffer_Create();
    chain = NULL;
    if (switches.dataObjPtr != NULL) {
        unsigned char *bytes;
        int numBytes;

        bytes = Tcl_GetByteArrayFromObj(switches.dataObjPtr, &numBytes);
        string = (const char *)bytes;
        if (Blt_IsBase64(string, numBytes)) {
            if (Blt_DBuffer_Base64Decode(interp, string, numBytes, dbuffer) 
                != TCL_OK) {
                goto error;
            }
        } else {
            Blt_DBuffer_AppendData(dbuffer, bytes, numBytes);
        } 
        string = "data buffer";
        *fileNamePtr = NULL;
    } else {
        string = Tcl_GetString(switches.fileObjPtr);
        *fileNamePtr = string;
        if (Blt_DBuffer_LoadFile(interp, string, dbuffer) != TCL_OK) {
            goto error;
        }
    }
    chain = IcoToPictures(interp, string, dbuffer, &switches);
    if (chain == NULL) {
        return NULL;
    }
 error:
    Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
    Blt_DBuffer_Destroy(dbuffer);
    return chain;
}

static int
ExportIco(Tcl_Interp *interp, int index, Blt_Chain chain, int objc,
          Tcl_Obj *const *objv)
{
    Blt_DBuffer dbuffer;
    Blt_Picture picture;
    IcoExportSwitches switches;
    int result;

    memset(&switches, 0, sizeof(switches));
    switches.bg.u32 = 0xFFFFFFFF;       /* Default bgcolor is white. */
    switches.index = index;
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 3, objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if ((switches.dataObjPtr != NULL) && (switches.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "more than one export destination: ",
                "use only one -file or -data switch.", (char *)NULL);
        return TCL_ERROR;
    }
    picture = Blt_GetNthPicture(chain, switches.index);
    if (picture == NULL) {
        Tcl_AppendResult(interp, "bad picture index.", (char *)NULL);
        return TCL_ERROR;
    }
    dbuffer = Blt_DBuffer_Create();
    result = PicturesToIco(interp, chain, dbuffer, &switches);
    if (result != TCL_OK) {
        Tcl_AppendResult(interp, "can't convert \"", 
                Tcl_GetString(objv[2]), "\"", (char *)NULL);
        goto error;
    }
    if (switches.fileObjPtr != NULL) {
        const char *fileName;

        /* Write the image into the designated file. */
        fileName = Tcl_GetString(switches.fileObjPtr);
        result = Blt_DBuffer_SaveFile(interp, fileName, dbuffer);
    } else if (switches.dataObjPtr != NULL) {
        Tcl_Obj *objPtr;

        /* Write the image into the designated TCL variable. */
        objPtr = Tcl_ObjSetVar2(interp, switches.dataObjPtr, NULL, 
                Blt_DBuffer_ByteArrayObj(dbuffer), 0);
        result = (objPtr == NULL) ? TCL_ERROR : TCL_OK;
    } else {
        Tcl_Obj *objPtr;

        /* Return the image as a base64 string in the interpreter result. */
        result = TCL_ERROR;
        objPtr = Blt_DBuffer_Base64EncodeToObj(dbuffer);
        if (objPtr != NULL) {
            Tcl_SetObjResult(interp, objPtr);
            result = TCL_OK;
        }
    }
 error:
    Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
    Blt_DBuffer_Destroy(dbuffer);
    return result;
}

int 
Blt_PictureIcoInit(Tcl_Interp *interp)
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
        return TCL_ERROR;
    };
#endif
#ifdef USE_BLT_STUBS
    if (Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    };
    if (Blt_InitTkStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    };
#endif    
    if (Tcl_PkgRequire(interp, "blt_tk", BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, "blt_picture_ico", BLT_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }
    return Blt_PictureRegisterFormat(interp, 
        "ico",                  /* Name of format. */
        IsIco,                  /* Discovery routine. */
        ReadIco,                /* Read format procedure. */
        WriteIco,               /* Write format procedure. */
        ImportIco,              /* Import format procedure. */
        ExportIco);             /* Export format switches. */
}

int 
Blt_PictureIcoSafeInit(Tcl_Interp *interp) 
{
    return Blt_PictureIcoInit(interp);
}

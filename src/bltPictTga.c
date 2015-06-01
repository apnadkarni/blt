/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictTga.c --
 *
 * This module implements Targa file format conversion routines for the
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
#include "config.h"
#include <tcl.h>

#include <time.h>
#include <setjmp.h>
#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include "bltPicture.h"
#include "bltPictFmts.h"

#ifdef _MSC_VER
  #define vsnprintf               _vsnprintf
#endif  /* _MSC_VER */

#undef MIN
#define MIN(a,b)        (((a)<(b))?(a):(b))

/* 
 * TODO: 
 *
 * 1. Read postage stamp file.  
 * 2. Read comments, etc and display.
 */
typedef struct _Blt_Picture Pict;

#define TRUE    1
#define FALSE   0

#define TGA_AUTHOR_SIZE         40
#define TGA_SW_SIZE             40
#define TGA_JOBNAME_SIZE        40
#define TGA_COMMENT_SIZE        80

/* Field offsets for Tga header.  */
#define OFF_ID_LENGTH           0
#define OFF_CM_TYPE             1
#define OFF_IMG_TYPE            2
#define OFF_CM_OFFSET           3
#define OFF_CM_NUMENTRIES       5
#define OFF_CM_BITSPERPIXEL     7
#define OFF_X_ORIGIN            8
#define OFF_Y_ORIGIN            10
#define OFF_WIDTH               12
#define OFF_HEIGHT              14
#define OFF_BITSPERPIXEL        16
#define OFF_ATTRIBUTES          17
#define TGA_HEADER_SIZE         18

/* Field offsets for Tga footer.  */
#define OFF_EXT_OFFSET          0
#define OFF_DEV_OFFSET          4
#define OFF_TRUEVISION          8
#define TGA_FOOTER_SIZE         26

/* Field offsets for Tga extension.  */
#define OFF_EXTSIZE             0
#define OFF_AUTHOR              2
#define OFF_COMMENT1            43
#define OFF_COMMENT2            43+81
#define OFF_COMMENT3            43+81+81
#define OFF_COMMENT4            43+81+81+81
#define OFF_TIME_MONTH          367
#define OFF_TIME_DAY            369
#define OFF_TIME_YEAR           371
#define OFF_TIME_HOUR           373
#define OFF_TIME_MIN            375
#define OFF_TIME_SEC            377
#define OFF_JOB_NAME            379
#define OFF_JOB_HOUR            420
#define OFF_JOB_MIN             422
#define OFF_JOB_SEC             424
#define OFF_SW_ID               426
#define OFF_SW_VERS_NUM         467
#define OFF_SW_VERS_LET         469
#define OFF_KEYCOLOR_A          470
#define OFF_KEYCOLOR_R          471
#define OFF_KEYCOLOR_G          472
#define OFF_KEYCOLOR_B          473
#define OFF_PIXEL_RATIO_NUM     474
#define OFF_PIXEL_RATIO_DEM     476
#define OFF_GAMMA_NUM           478
#define OFF_GAMMA_DEM           480 
#define OFF_COLOR_OFFSET        482
#define OFF_POSTSTAMP_OFFSET    486
#define OFF_SCANLINES_OFFSET    490
#define OFF_ATTRIBUTE_TYPE      494
#define TGA_EXT_SIZE            495

/* Definitions for image types. */
#define TGA_TYPE_UNKNOWN        0       /* No image data included. */
#define TGA_TYPE_PSEUDOCOLOR    1       /* Color-mapped image */
#define TGA_TYPE_TRUECOLOR      2       /* RGB image */
#define TGA_TYPE_GREYSCALE      3       /* Monochrome image. */
#define TGA_TYPE_COMP           32      /* Compressed color-mapped data,
                                         * using Huffman, Delta, and
                                         * runlength encoding. */
#define TGA_TYPE_COMP4          33      /* Compressed color-mapped data,
                                         * using Huffman, Delta, and RLE.
                                         * 4-pass quadtree- type
                                         * process. */
#define TGA_RLE                 8       /* If bit is set in image type,
                                         * indicates image is run length
                                         * encoded. */

#define PKT_RLE                 0x80    /* Indicates this packet is run
                                         * length encoded. */

#define NO_INTERLEAVE     0 
#define INTERLEAVE        1
#define INTERLEAVE4       2

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    Tcl_Obj *infoVarObjPtr;             /* Name of TCL variable to hold
                                         * information related to the
                                         * importing of the image. */
} TgaReader;

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data",  "data", (char *)NULL,
        Blt_Offset(TgaReader, dataObjPtr), 0},
    {BLT_SWITCH_OBJ, "-file",  "fileName", (char *)NULL,
        Blt_Offset(TgaReader, fileObjPtr), 0},
    {BLT_SWITCH_OBJ, "-info",  "varName", (char *)NULL,
        Blt_Offset(TgaReader, infoVarObjPtr), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchParseProc ColorSwitchProc;
static Blt_SwitchCustom colorSwitch = {
    ColorSwitchProc, NULL, NULL, (ClientData)0
};

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    int flags;                          /* Flag. */
    Blt_Pixel bg;
    int index;
    const char *jobName;                /* Label to be displayed as the id
                                         * of the image. */
    const char *comments;               /* Comments to be displayed as the
                                         * extension of the image. */
    const char *author;                 /* Author of the image. */
    const char *software;               /* Software used to create
                                         * image. */
} TgaWriter;

#define EXPORT_ALPHA    (1<<0)          /* Export the alpha channel. */
#define EXPORT_RLE      (1<<1)          /* Run length encode the image. */

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_BITMASK, "-alpha", "", (char *)NULL,
        Blt_Offset(TgaWriter, flags),   0, EXPORT_ALPHA},
    {BLT_SWITCH_STRING, "-author", "string", (char *)NULL,
        Blt_Offset(TgaWriter, author),   0},
    {BLT_SWITCH_CUSTOM, "-background", "color", (char *)NULL,
        Blt_Offset(TgaWriter, bg),         0, 0, &colorSwitch},
    {BLT_SWITCH_STRING, "-comments", "string", (char *)NULL,
        Blt_Offset(TgaWriter, comments),   0},
    {BLT_SWITCH_OBJ, "-data",  "varName", (char *)NULL,
        Blt_Offset(TgaWriter, dataObjPtr), 0},
    {BLT_SWITCH_OBJ, "-file",  "fileName", (char *)NULL,
        Blt_Offset(TgaWriter, fileObjPtr), 0},
    {BLT_SWITCH_INT_NNEG, "-index", "int", (char *)NULL,
        Blt_Offset(TgaWriter, index), 0},
    {BLT_SWITCH_STRING, "-job",  "string", (char *)NULL,
        Blt_Offset(TgaWriter, jobName), 0},
    {BLT_SWITCH_BITMASK, "-rle", "", (char *)NULL,
        Blt_Offset(TgaWriter, flags),   0, EXPORT_RLE},
    {BLT_SWITCH_STRING, "-software", "string", (char *)NULL,
        Blt_Offset(TgaWriter, software),   0},
    {BLT_SWITCH_END}
};

#define MAXCOLORS       256

typedef struct _Tga Tga;

typedef unsigned int TgaGetPixelProc(Tga *tgaPtr);
typedef void TgaPutPixelProc(Tga *tgaPtr, Blt_Pixel *color);

typedef struct {
} TgaPostageStamp;

typedef struct {
} TgaScanLine;

typedef struct {
} TgaColorCorrect;

typedef struct {
    unsigned short int size;
    char author[TGA_AUTHOR_SIZE+1];
    char comment1[TGA_COMMENT_SIZE+1];
    char comment2[TGA_COMMENT_SIZE+1];
    char comment3[TGA_COMMENT_SIZE+1];
    char comment4[TGA_COMMENT_SIZE+1];
    unsigned short int mon, day, year;
    unsigned short int hour, min, sec;
    char jobName[TGA_JOBNAME_SIZE+1];
    unsigned short int jobHour, jobMin, jobSec;
    char swId[TGA_SW_SIZE+1];
    unsigned short int versionNum;
    unsigned char versionLetter;
    Blt_Pixel keyColor;
    double pixelAspectRatio;
    double gamma;
    unsigned int colorCorrOffset;
    unsigned int postageStampOffset;
    unsigned int scanLineOffset;
    char attributesType;
} TgaExtension;

struct _Tga {
    const char *name;                   /* Name of file. */
    int version;                        /* Either 1 or 2. */
    unsigned int numBytesId;            /* Size of image ID below */
    int isRle;                          /* Indicates if the image is
                                         * runlength encoded. */
    int colorMapExists;                 /* Color map type: 0 = no color
                                         * map.  1 = 256 entry palette. */
    int imageType;                      /* Image type code */
    int cmOffset;                       /* Index of first color map
                                           entry. */
    int cmNumEntries;                   /* # of color map entries. */
    int cmBitsPerPixel;                 /* Bits per pixel of color map
                                         * entries. */
    int xOrigin;                        /* X coordinate of the lower left
                                         * corner of the image. */
    int yOrigin;                        /* Y coordinate of the lower left
                                         * corner of the image. */
    int width;                          /* Width of the image in pixels. */
    int height;                         /* Height of the image in
                                         * pixels. */
    int bitsPerPixel;                   /* Bytes per pixel. */
    unsigned int attributes;            /* Flags:
                                         * 0-3 : Number of attribute bits
                                         *   4 : reserved
                                         *   5 : Screen origin in upper 
                                         *       left corner
                                         * 6-7 : Data storage interleave
                                         *      00 - no interleave
                                         *      01 - even/odd interleave
                                         *      10 - four way interleave
                                         *      11 - reserved  The byte 
                                         *           should be set. 
                                         */
    int flags;
    int numAlphaBits;
    int originBits;
    int interleaveBits;
    int extOffset;                      /* Offset of extension from
                                         * beginning of the file. */
    int devOffset;                      /* Offset of developer area from
                                         * beginning of the file. */
    Blt_DBuffer dbuffer;
    Blt_Pixel palette[MAXCOLORS];
    char *id;

    int pktIsRle;                       /* Indicates if the current packet
                                         * is raw or run length encoded. */
    int pktCount;                       /* Current count of raw or run
                                         * length encoded packets. */
    Blt_Pixel pktRep;                   /* Current pixel being replicated
                                         * for run length encoded
                                         * packets. */
    unsigned char *bp;                  /* Current pointer. */
    int bytesLeft;                      /* Bytes left in image stream. */
    TgaGetPixelProc *getProc;
    TgaPutPixelProc *putProc;
    TgaExtension ext;
    Blt_HashTable colorTable;
    unsigned int initialized;
    jmp_buf jmpbuf;
    Tcl_DString errors;
    Tcl_DString warnings;
    int numWarnings, numErrors;
};

DLLEXPORT extern Tcl_AppInitProc Blt_PictureTgaInit;
DLLEXPORT extern Tcl_AppInitProc Blt_PictureTgaSafeInit;

static TgaGetPixelProc TgaGet15BitTrueColorPixelProc;
static TgaGetPixelProc TgaGet16BitTrueColorPixelProc;
static TgaGetPixelProc TgaGet24BitTrueColorPixelProc;
static TgaGetPixelProc TgaGet32BitTrueColorPixelProc;
static TgaGetPixelProc TgaGet8BitGreyscalePixelProc;
static TgaGetPixelProc TgaGet8BitPseudoColorPixelProc;

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

#ifdef WIN32
static struct tm *
localtime_r(const time_t *timePtr, struct tm *resultPtr)
{
    if (resultPtr != NULL) {
        struct tm *tmPtr;

        tmPtr = localtime(timePtr);
        if (tmPtr == NULL) {
            return NULL;
        }
        memcpy(resultPtr, tmPtr, sizeof(struct tm));
    }
    return resultPtr;
}
#endif  /* WIN32 */

static void
TgaFree(Tga *tgaPtr)
{
    if (tgaPtr->initialized) {
        Blt_DeleteHashTable(&tgaPtr->colorTable);
    }
    Blt_Free(tgaPtr->id);
}

/*ARGSUSED*/
static void
TgaError(Tga *tgaPtr, const char *fmt, ...)
{
    char string[BUFSIZ+4];
    int length;
    va_list args;

    va_start(args, fmt);
    length = vsnprintf(string, BUFSIZ, fmt, args);
    if (length > BUFSIZ) {
        strcat(string, "...");
    }
    Tcl_DStringAppend(&tgaPtr->errors, string, -1);
    va_end(args);
    longjmp(tgaPtr->jmpbuf, 0);
}

#ifdef notdef
/*ARGSUSED*/
static void
TgaWarning(Tga *tagPtr, const char *fmt, ...)
{
    char string[BUFSIZ+4];
    int length;
    va_list args;

    va_start(args, fmt);
    length = vsnprintf(string, BUFSIZ, fmt, args);
    if (length > BUFSIZ) {
        strcat(string, "...");
    }
    Tcl_DStringAppend(&tgaPtr->warnings, string, -1);
    va_end(args);
    tgaPtr->numWarnings++;
}
#endif

static INLINE unsigned int
TgaGetLong(unsigned char *buf)
{
#ifdef WORDS_BIGENDIAN
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
#else
    return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
#endif
}

static INLINE unsigned short
TgaGetShort(unsigned char *buf)
{
#ifdef WORDS_BIGENDIAN
    return (buf[0] << 8) | buf[1];
#else
    return buf[0] | (buf[1] << 8);
#endif
}

static INLINE unsigned char *
TgaSetLong(unsigned char *buf, unsigned long value)
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
TgaSetShort(unsigned char *buf, unsigned long value)
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

static const char *
TgaNameOfType(Tga *tgaPtr) 
{
    const char *string;
    switch (tgaPtr->imageType & 0x07) {
    case TGA_TYPE_PSEUDOCOLOR:
        string = "pseudocolor"; break;
    case TGA_TYPE_TRUECOLOR:
        string = "truecolor"; break;
    case TGA_TYPE_GREYSCALE:
        string = "greyscale"; break;
    default:
        string = "???";
    }
    return string;
}

static void
TgaInitPacket(Tga *tgaPtr)
{
    tgaPtr->pktCount = 0;
    tgaPtr->pktIsRle = 0;
    tgaPtr->bp = Blt_DBuffer_Pointer(tgaPtr->dbuffer);
    tgaPtr->bytesLeft = Blt_DBuffer_BytesLeft(tgaPtr->dbuffer);
}

static unsigned short
TgaNextWord(Tga *tgaPtr)
{
    unsigned char buf[2];

    buf[0] = Blt_DBuffer_NextByte(tgaPtr->dbuffer);
    buf[1] = Blt_DBuffer_NextByte(tgaPtr->dbuffer);
#ifdef WORDS_BIGENDIAN
    return (buf[0] << 8) | buf[1];
#else
    return buf[0] | (buf[1] << 8);
#endif
}

static unsigned int
TgaGet15BitTrueColorPixelProc(Tga *tgaPtr)
{
    Blt_Pixel color;
    unsigned int pixel;

    pixel = TgaNextWord(tgaPtr);
    color.Blue  = (pixel & 0x001f) << 3;
    color.Green = (pixel & 0x03e0) >> 2;
    color.Red   = (pixel & 0x7c00) >> 7;
    color.Alpha = ALPHA_OPAQUE;
    return color.u32;
}

static unsigned int
TgaGet16BitTrueColorPixelProc(Tga *tgaPtr)
{
    Blt_Pixel color;
    unsigned int pixel;

    pixel = TgaNextWord(tgaPtr);
    color.Blue  = (pixel & 0x001f) << 3;
    color.Green = (pixel & 0x03e0) >> 2;
    color.Red   = (pixel & 0x7c00) >> 7;
    if (tgaPtr->numAlphaBits > 0) {
        if (tgaPtr->numAlphaBits != 1) {
            TgaError(tgaPtr, 
                     "number of alpha bits must be 1, not %d for 16-bit image", 
                     tgaPtr->numAlphaBits);
        }
        color.Alpha = (pixel & 0x8000) ? ALPHA_TRANSPARENT : ALPHA_OPAQUE;
    } else {
        color.Alpha = ALPHA_OPAQUE;
    }
    return color.u32;
}

static unsigned int
TgaGet24BitTrueColorPixelProc(Tga *tgaPtr)
{
    Blt_Pixel color;

    color.Blue  = Blt_DBuffer_NextByte(tgaPtr->dbuffer);
    color.Green = Blt_DBuffer_NextByte(tgaPtr->dbuffer);
    color.Red   = Blt_DBuffer_NextByte(tgaPtr->dbuffer);
    color.Alpha = ALPHA_OPAQUE;
    return color.u32;
}

static unsigned int
TgaGet32BitTrueColorPixelProc(Tga *tgaPtr)
{
    Blt_Pixel color;

    color.Blue  = Blt_DBuffer_NextByte(tgaPtr->dbuffer);
    color.Green = Blt_DBuffer_NextByte(tgaPtr->dbuffer);
    color.Red   = Blt_DBuffer_NextByte(tgaPtr->dbuffer);
    color.Alpha = Blt_DBuffer_NextByte(tgaPtr->dbuffer);
    if (tgaPtr->numAlphaBits > 0) {
        color.Alpha = color.Alpha & ((1 << tgaPtr->numAlphaBits) - 1);
    } else {
        color.Alpha = ALPHA_OPAQUE;
    }
    return color.u32;
}

static unsigned int
TgaGet8BitGreyscalePixelProc(Tga *tgaPtr)
{
    Blt_Pixel color;
    
    color.Red = color.Green = color.Blue = 
        Blt_DBuffer_NextByte(tgaPtr->dbuffer);
    color.Alpha = ALPHA_OPAQUE;
    return color.u32;
}

static unsigned int
TgaGet8BitPseudoColorPixelProc(Tga *tgaPtr)
{
    unsigned int index;

    index = Blt_DBuffer_NextByte(tgaPtr->dbuffer);
    if (index >= tgaPtr->cmNumEntries) {
        TgaError(tgaPtr, "invalid color index %d (> %d)", index, 
                 tgaPtr->cmNumEntries);
    }
    return tgaPtr->palette[index].u32;
}

static unsigned int
TgaGetRLEPixel(Tga *tgaPtr) 
{
    if (tgaPtr->pktCount == 0) {

        /* Next byte is the start of a packet. Determine the type of packet
         * and get the replicant pixel if it's a RLE packet. */

        tgaPtr->pktCount = Blt_DBuffer_NextByte(tgaPtr->dbuffer);
        tgaPtr->pktIsRle = (tgaPtr->pktCount & PKT_RLE);
 
        if (tgaPtr->pktIsRle) {
            /* Run length encoded packet. Count is the replication factor of
             * the following pixel. */
            tgaPtr->pktCount &= 0x7F;   
            tgaPtr->pktRep.u32 = (*tgaPtr->getProc)(tgaPtr);
        }
        tgaPtr->pktCount++;             /* The count is the lower 7 bits + 1. */
        if (tgaPtr->pktCount < 0 || tgaPtr->pktCount > 128) {
            TgaError(tgaPtr, "invalid packet count %d, must be 0..128",
                     tgaPtr->pktCount);
        }
    }
    tgaPtr->pktCount--;
    if (tgaPtr->pktIsRle) {
        return tgaPtr->pktRep.u32;
    }
    return (*tgaPtr->getProc)(tgaPtr);
}

static int
TgaGetHeader(Blt_DBuffer dbuffer, Tga *tgaPtr)
{
    unsigned char *bp;
    size_t fileSize;

    Blt_DBuffer_Rewind(dbuffer);
    fileSize = Blt_DBuffer_BytesLeft(dbuffer);
    if (fileSize < TGA_HEADER_SIZE) {
        return FALSE;
    }
    bp = Blt_DBuffer_Pointer(dbuffer);
    tgaPtr->numBytesId     = bp[OFF_ID_LENGTH];
    tgaPtr->colorMapExists = bp[OFF_CM_TYPE];
    if (tgaPtr->colorMapExists > 1) {
        return FALSE;                   /* Likely not a TGA file. */
    }
    tgaPtr->dbuffer = dbuffer;
    tgaPtr->imageType      = bp[OFF_IMG_TYPE] & 0x7;
    tgaPtr->isRle          = bp[OFF_IMG_TYPE] & TGA_RLE;
    tgaPtr->cmOffset       = TgaGetShort(bp+OFF_CM_OFFSET);
    tgaPtr->cmNumEntries   = TgaGetShort(bp+OFF_CM_NUMENTRIES);
    tgaPtr->cmBitsPerPixel = bp[OFF_CM_BITSPERPIXEL];
    tgaPtr->xOrigin        = TgaGetShort(bp+OFF_X_ORIGIN);
    tgaPtr->yOrigin        = TgaGetShort(bp+OFF_Y_ORIGIN);
    tgaPtr->width          = TgaGetShort(bp+OFF_WIDTH);
    tgaPtr->height         = TgaGetShort(bp+OFF_HEIGHT);
    tgaPtr->bitsPerPixel   = bp[OFF_BITSPERPIXEL];

    tgaPtr->attributes     = bp[OFF_ATTRIBUTES];
    tgaPtr->numAlphaBits   = tgaPtr->attributes & 0x0F;        /* 0-3 */
    tgaPtr->originBits     = (tgaPtr->attributes & 0x30) >> 4; /* 4-5 */
    tgaPtr->interleaveBits = (tgaPtr->attributes & 0xC0) >> 6; /* 6-7 */
    tgaPtr->version = 1;

    switch (tgaPtr->imageType) {
    case TGA_TYPE_PSEUDOCOLOR:
    case TGA_TYPE_TRUECOLOR:
    case TGA_TYPE_GREYSCALE:
        break;
    default: 
        return FALSE;
    }
    switch (tgaPtr->bitsPerPixel) {
    case 8:                             /* 8-bit greyscale. */
    case 15:                            /* 5-5-5 RGB color */
    case 16:                            /* 1-5-5-5 RGBA color */
    case 24:                            /* 8-8-8 RGB color. */
    case 32:                            /* 8-8-8-8 RGBA color. */
        break;
    default:                            /* Unknown pixel format. */
        return FALSE;
    }
    /* Go to the end of the buffer to check for the footer. */
    bp += Blt_DBuffer_Length(dbuffer) - TGA_FOOTER_SIZE;
    tgaPtr->extOffset = TgaGetLong(bp + OFF_EXT_OFFSET);
    tgaPtr->devOffset = TgaGetLong(bp + OFF_DEV_OFFSET);
    if (strncmp("TRUEVISION-XFILE.", (const char *)bp + OFF_TRUEVISION, 
        17) == 0) {
        tgaPtr->version = 2;
    }
    Blt_DBuffer_IncrCursor(dbuffer, TGA_HEADER_SIZE); /* Skip header. */
    return TRUE;
}

static void
TgaGetId(Tga *tgaPtr)
{
    if (tgaPtr->numBytesId > 0) {
        unsigned char *bp;

        tgaPtr->id = Blt_Malloc(tgaPtr->numBytesId + 1);
        if (tgaPtr->id == NULL) {
            TgaError(tgaPtr, "can't allocate id of %d bytes", 
                     tgaPtr->numBytesId + 1);
        }
        bp = Blt_DBuffer_Pointer(tgaPtr->dbuffer);
        strncpy((char *)tgaPtr->id, (char *)bp, tgaPtr->numBytesId);
        tgaPtr->id[tgaPtr->numBytesId] = '\0';
        Blt_DBuffer_IncrCursor(tgaPtr->dbuffer, tgaPtr->numBytesId);
    }
}

static void
TgaAddString(Tcl_Interp *interp, Tcl_Obj *listObjPtr, const char *name, 
             const char *value)
{
    Tcl_Obj *objPtr;

    objPtr = Tcl_NewStringObj(name, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewStringObj(value, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
}    

static void
TgaAddInt(Tcl_Interp *interp, Tcl_Obj *listObjPtr, const char *name, int value)
{
    Tcl_Obj *objPtr;

    objPtr = Tcl_NewStringObj(name, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(value);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
}    

static void
TgaAddBoolean(Tcl_Interp *interp, Tcl_Obj *listObjPtr, const char *name, 
              int value)
{
    Tcl_Obj *objPtr;

    objPtr = Tcl_NewStringObj(name, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewBooleanObj(value);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
}    

static void
TgaAddDouble(Tcl_Interp *interp, Tcl_Obj *listObjPtr, const char *name, 
             double value)
{
    Tcl_Obj *objPtr;

    objPtr = Tcl_NewStringObj(name, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewDoubleObj(value);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
}    

static void
TgaSetInfo(Tcl_Interp *interp, Tga *tgaPtr, Tcl_Obj *varNameObjPtr)
{
    Tcl_Obj *listObjPtr;
    TgaExtension *extPtr = &tgaPtr->ext;
    char string[200];

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    /* Filename */
    if (tgaPtr->name != NULL) {
        TgaAddString(interp, listObjPtr, "fileName", tgaPtr->name);
    }
    TgaAddString(interp, listObjPtr, "imageType", TgaNameOfType(tgaPtr));
    TgaAddBoolean(interp, listObjPtr, "rle", tgaPtr->isRle);
    TgaAddInt(interp, listObjPtr, "width", tgaPtr->width);
    TgaAddInt(interp, listObjPtr, "height", tgaPtr->height);
    TgaAddInt(interp, listObjPtr, "bitsPerPixel", tgaPtr->bitsPerPixel);
    TgaAddInt(interp, listObjPtr, "xOrigin", tgaPtr->xOrigin);
    TgaAddInt(interp, listObjPtr, "yOrigin", tgaPtr->yOrigin);
    TgaAddBoolean(interp, listObjPtr, "colormap", tgaPtr->colorMapExists);
    TgaAddInt(interp, listObjPtr, "cmNumEntries", tgaPtr->cmNumEntries);
    TgaAddInt(interp, listObjPtr, "cmBitsPerPixel", tgaPtr->cmBitsPerPixel);
    TgaAddInt(interp, listObjPtr, "numAlphaBits", tgaPtr->numAlphaBits);
    TgaAddInt(interp, listObjPtr, "originBits", tgaPtr->originBits);
    TgaAddInt(interp, listObjPtr, "interleave", tgaPtr->interleaveBits);
    TgaAddInt(interp, listObjPtr, "version", tgaPtr->version);
    if (tgaPtr->ext.size > 0) {
        TgaAddString(interp, listObjPtr, "author", tgaPtr->ext.author);
        if (extPtr->comment1[0] != '\0') {
            TgaAddString(interp, listObjPtr, "comment1", extPtr->comment1);
        }
        if (extPtr->comment2[0] != '\0') {
            TgaAddString(interp, listObjPtr, "comment2", extPtr->comment2);
        }
        if (extPtr->comment3[0] != '\0') {
            TgaAddString(interp, listObjPtr, "comment3", extPtr->comment3);
        }
        if (extPtr->comment4[0] != '\0') {
            TgaAddString(interp, listObjPtr, "comment4", extPtr->comment4);
        }
        if ((extPtr->mon == 0) && (extPtr->day == 0) && (extPtr->year == 0)) {
            TgaAddInt(interp, listObjPtr, "date", 0);
        } else {
            sprintf(string, "%d/%d/%d", extPtr->mon, extPtr->day,extPtr->year);
            TgaAddString(interp, listObjPtr, "date", string);
        }
        if ((extPtr->sec == 0) && (extPtr->min == 0) && (extPtr->hour == 0)) {
            TgaAddInt(interp, listObjPtr, "time", 0);
        } else {
            sprintf(string, "%02d:%02d:%02d", extPtr->hour, extPtr->min, 
                extPtr->sec);
            TgaAddString(interp, listObjPtr, "time", string);
        }
        TgaAddString(interp, listObjPtr, "jobName", extPtr->jobName);
        /* JobTime */
        if ((extPtr->jobHour == 0) && (extPtr->jobMin == 0) && 
            (extPtr->jobSec == 0)) {
            TgaAddInt(interp, listObjPtr, "jobTime", 0);
        } else {
            sprintf(string, "%02d:%02d:%02d", extPtr->jobHour, extPtr->jobMin, 
                    extPtr->jobSec);
            TgaAddString(interp, listObjPtr, "jobTime", string);
        }
        TgaAddString(interp, listObjPtr, "softwareId", extPtr->swId);
        TgaAddInt(interp, listObjPtr, "softwareVersion", extPtr->versionNum);
        TgaAddDouble(interp, listObjPtr, "pixelAspectRatio", 
                     extPtr->pixelAspectRatio);
        TgaAddDouble(interp, listObjPtr, "gamma", extPtr->gamma);
        TgaAddBoolean(interp, listObjPtr, "thumbnail", 
                      extPtr->postageStampOffset);
        TgaAddBoolean(interp, listObjPtr, "colorCorrection", 
                      extPtr->colorCorrOffset);
        TgaAddBoolean(interp, listObjPtr, "scanLines", 
                      extPtr->scanLineOffset);
    }
    Tcl_ObjSetVar2(interp, varNameObjPtr, NULL, listObjPtr, TCL_LEAVE_ERR_MSG);
}

static void
TgaGetColorTable(Tga *tgaPtr) 
{
    Blt_Pixel *dp, *dend;
    size_t numBytes;
    int bytesPerPixel;
    TgaGetPixelProc *proc;

    if (tgaPtr->cmNumEntries == 0) {
        return;
    }
    if (tgaPtr->cmNumEntries > 256) {
        TgaError(tgaPtr, "too many (%d) color table entries", 
                 tgaPtr->cmNumEntries);
    }       
    switch (tgaPtr->cmBitsPerPixel) {
    case 8:                             /* 8-bit greyscale. */
        proc = TgaGet8BitGreyscalePixelProc;
        bytesPerPixel = 1; 
        break;
    case 15:                            /* 5-5-5 RGB color */
        proc = TgaGet15BitTrueColorPixelProc;
        bytesPerPixel = 2; 
        break;
    case 16:                            /* 1-5-5-5 RGBA color */
        proc = TgaGet16BitTrueColorPixelProc;
        bytesPerPixel = 2; 
        break;
    case 24:                            /* 8-8-8 RGB color. */
        proc = TgaGet24BitTrueColorPixelProc;
        bytesPerPixel = 3; 
        break;
    case 32:                            /* 8-8-8-8 RGBA color. */
        proc = TgaGet32BitTrueColorPixelProc;
        bytesPerPixel = 4; 
        break;
    default:                            /* Unknown pixel format. */
        TgaError(tgaPtr, "unknown colormap pixel size (%d).", 
                 tgaPtr->cmBitsPerPixel);
        return;                         /* NOTREACHED */
    }
    if (tgaPtr->cmOffset > 0) {
        Blt_DBuffer_IncrCursor(tgaPtr->dbuffer, 
                               tgaPtr->cmOffset * bytesPerPixel);
    }
    numBytes = bytesPerPixel * tgaPtr->cmNumEntries;
    if (numBytes > Blt_DBuffer_BytesLeft(tgaPtr->dbuffer)) {
        TgaError(tgaPtr, "GetColorTable: short file.");
    }
    TgaInitPacket(tgaPtr);
    for (dp = tgaPtr->palette, dend = dp + tgaPtr->cmNumEntries; 
         dp < dend; dp++) {
        dp->u32 = (*proc)(tgaPtr);
    }
}

static Blt_Picture
TgaGetImageData(Tga *tgaPtr)
{
    Pict *destPtr;
    TgaGetPixelProc *proc;
    int y;
    Blt_Pixel *destRowPtr;

    /* Set up the correct pixel procedure depending whether the data is run
     * length encoded image or not */
    proc = (tgaPtr->isRle) ? TgaGetRLEPixel : tgaPtr->getProc;

    destPtr = Blt_CreatePicture(tgaPtr->width, tgaPtr->height);
    TgaInitPacket(tgaPtr);
    switch (tgaPtr->originBits) {
    case 0:                             /* Origin is bottom-left corner. */
        destRowPtr = destPtr->bits + 
            ((tgaPtr->height - 1) * destPtr->pixelsPerRow);
        for (y = 0; y < tgaPtr->height; y++) {
            Blt_Pixel *dp, *dend;
            
            for (dp = destRowPtr, dend = dp + tgaPtr->width; dp < dend; dp++) {
                dp->u32 = (*proc)(tgaPtr);
                if (dp->Alpha == 0x00) {
                    destPtr->flags |= BLT_PIC_MASK;
                } else if (dp->Alpha != 0xFF) {
                    destPtr->flags |= BLT_PIC_ALPHAS;
                }
            }
            destRowPtr -= destPtr->pixelsPerRow;
        }
        break;
    case 1:                             /* Origin is bottom-right corner. */
        destRowPtr = destPtr->bits + 
            ((tgaPtr->height - 1) * destPtr->pixelsPerRow);
        for (y = 0; y < tgaPtr->height; y++) {
            Blt_Pixel *dp, *dend;
            
            for (dend = destRowPtr, dp = dend + (tgaPtr->width - 1);
                 dp >= dend; dp--) {
                dp->u32 = (*proc)(tgaPtr);
                if (dp->Alpha == 0x00) {
                    destPtr->flags |= BLT_PIC_MASK;
                } else if (dp->Alpha != 0xFF) {
                    destPtr->flags |= BLT_PIC_ALPHAS;
                }
            }
            destRowPtr -= destPtr->pixelsPerRow;
        }
        break;
    case 2:                             /* Origin is top-left corner. */
        destRowPtr = destPtr->bits;
        for (y = 0; y < tgaPtr->height; y++) {
            Blt_Pixel *dp, *dend;
            
            for (dp = destRowPtr, dend = dp + tgaPtr->width; dp < dend; dp++) {
                dp->u32 = (*proc)(tgaPtr);
                if (dp->Alpha == 0x00) {
                    destPtr->flags |= BLT_PIC_MASK;
                } else if (dp->Alpha != 0xFF) {
                    destPtr->flags |= BLT_PIC_ALPHAS;
                }
            }
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
    case 3:                             /* Origin is top-right corner. */
        destRowPtr = destPtr->bits;
        for (y = 0; y < tgaPtr->height; y++) {
            Blt_Pixel *dp, *dend;
            
            for (dend = destRowPtr, dp = dend + (tgaPtr->width - 1);
                 dp >= dend; dp--) {
                dp->u32 = (*proc)(tgaPtr);
                if (dp->Alpha == 0x00) {
                    destPtr->flags |= BLT_PIC_MASK;
                } else if (dp->Alpha != 0xFF) {
                    destPtr->flags |= BLT_PIC_ALPHAS;
                }
            }
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
    }
    destPtr->flags &= ~BLT_PIC_UNINITIALIZED;
    return destPtr;
}

static void
TgaGetExtension(Tga *tgaPtr)
{
    unsigned char *bp, *old;
    TgaExtension *extPtr = &tgaPtr->ext;
    double x, y;

    memset(extPtr, 0, sizeof(TgaExtension));
    if (tgaPtr->extOffset > Blt_DBuffer_Size(tgaPtr->dbuffer)) {
        return;
        TgaError(tgaPtr, "Extension: short file (size is %d).", 
                 tgaPtr->extOffset);
    }
    /* Save the old buffer pointer. */
    old = Blt_DBuffer_Pointer(tgaPtr->dbuffer);
    Blt_DBuffer_SetCursor(tgaPtr->dbuffer, tgaPtr->extOffset);
    bp = Blt_DBuffer_Pointer(tgaPtr->dbuffer);
    extPtr->size = TgaGetShort(bp + OFF_EXTSIZE);
    strncpy(extPtr->author,   (char *)bp + OFF_AUTHOR,   TGA_AUTHOR_SIZE+1);
    strncpy(extPtr->comment1, (char *)bp + OFF_COMMENT1, TGA_COMMENT_SIZE+1);
    strncpy(extPtr->comment2, (char *)bp + OFF_COMMENT2, TGA_COMMENT_SIZE+1);
    strncpy(extPtr->comment3, (char *)bp + OFF_COMMENT3, TGA_COMMENT_SIZE+1);
    strncpy(extPtr->comment4, (char *)bp + OFF_COMMENT4, TGA_COMMENT_SIZE+1);
    extPtr->mon  = TgaGetShort(bp + OFF_TIME_MONTH);
    extPtr->day  = TgaGetShort(bp + OFF_TIME_DAY);
    extPtr->year = TgaGetShort(bp + OFF_TIME_YEAR);
    extPtr->hour = TgaGetShort(bp + OFF_TIME_HOUR);
    extPtr->min  = TgaGetShort(bp + OFF_TIME_MIN);
    extPtr->sec  = TgaGetShort(bp + OFF_TIME_SEC);
    strncpy(extPtr->jobName, (char *)bp + OFF_JOB_NAME, TGA_JOBNAME_SIZE+1);
    extPtr->jobHour = TgaGetShort(bp + OFF_JOB_HOUR);
    extPtr->jobMin  = TgaGetShort(bp + OFF_JOB_MIN);
    extPtr->jobSec  = TgaGetShort(bp + OFF_JOB_SEC);
    strncpy(extPtr->swId, (char *)bp + OFF_SW_ID, TGA_SW_SIZE+1);
    extPtr->versionNum  = TgaGetShort(bp + OFF_SW_VERS_NUM);
    extPtr->versionLetter  = bp[OFF_SW_VERS_LET];
    extPtr->keyColor.Alpha = bp[OFF_KEYCOLOR_A];
    extPtr->keyColor.Red   = bp[OFF_KEYCOLOR_R];
    extPtr->keyColor.Green = bp[OFF_KEYCOLOR_G];
    extPtr->keyColor.Blue  = bp[OFF_KEYCOLOR_B];
    x = (double)TgaGetShort(bp + OFF_PIXEL_RATIO_NUM);
    y = (double)TgaGetShort(bp + OFF_PIXEL_RATIO_DEM);
    if (y == 0) {
        extPtr->pixelAspectRatio = y;
    } else {
        extPtr->pixelAspectRatio = x / y;
    }
    x = TgaGetShort(bp + OFF_GAMMA_NUM);
    y = TgaGetShort(bp + OFF_GAMMA_DEM);
    if (y == 0) {
        extPtr->gamma = y;
    } else {
        extPtr->gamma = x / y;
    }
    extPtr->colorCorrOffset = TgaGetLong(bp + OFF_COLOR_OFFSET);
    extPtr->postageStampOffset = TgaGetLong(bp + OFF_POSTSTAMP_OFFSET);
    extPtr->scanLineOffset = TgaGetLong(bp + OFF_SCANLINES_OFFSET);
    extPtr->attributesType = bp[OFF_ATTRIBUTE_TYPE];
    /* Reset the pointer. */
    Blt_DBuffer_SetPointer(tgaPtr->dbuffer, old);
}

/*
 *---------------------------------------------------------------------------
 *
 * TgaToPicture --
 *
 *      Reads a TGA file, converts it into a picture, and appends it
 *      to the list of images.  We only handle only single TGA images.
 *
 * Results:
 *      The picture is returned.  If an error occured, such
 *      as the designated file could not be opened, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain 
TgaToPicture(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer,
             TgaReader *readerPtr)
{
    Blt_Chain chain;
    Blt_Picture picture;
    Tga tga;

    memset(&tga, 0, sizeof(tga)); /* Clear the structure. */
    tga.numWarnings = 0;
    tga.name = fileName;

    Tcl_DStringInit(&tga.errors);
    Tcl_DStringInit(&tga.warnings);
    Tcl_DStringAppend(&tga.errors, "error reading \"", -1);
    Tcl_DStringAppend(&tga.errors, fileName, -1);
    Tcl_DStringAppend(&tga.errors, "\": ", -1);

    Tcl_DStringAppend(&tga.warnings, "\"", -1);
    Tcl_DStringAppend(&tga.warnings, fileName, -1);
    Tcl_DStringAppend(&tga.warnings, "\": ", -1);

    if (setjmp(tga.jmpbuf)) {
        Tcl_DStringResult(interp, &tga.errors);
        Tcl_DStringFree(&tga.warnings);
        TgaFree(&tga);
        return NULL;
    }
    chain = NULL;
    if (!TgaGetHeader(dbuffer, &tga)) {
        TgaError(&tga, "bad TGA header.");
    }
    if (tga.extOffset > 0) {
        TgaGetExtension(&tga);
    }
    if (tga.numBytesId > 0) {
        TgaGetId(&tga);
    }
    if (tga.colorMapExists) {
        TgaGetColorTable(&tga);
    }
    switch (tga.imageType) {
    case TGA_TYPE_PSEUDOCOLOR:
        switch (tga.bitsPerPixel) {
        case 8:
            tga.getProc = TgaGet8BitPseudoColorPixelProc; break;
        default:
            TgaError(&tga, "unknown pseudocolor pixel size \"%d\".", 
                     tga.bitsPerPixel);
            break;
        }
        break;

    case TGA_TYPE_TRUECOLOR:
        switch (tga.bitsPerPixel) {
        case 15:
            tga.getProc = TgaGet15BitTrueColorPixelProc;        break;
        case 16:
            tga.getProc = TgaGet16BitTrueColorPixelProc;        break;
        case 24:
            tga.getProc = TgaGet24BitTrueColorPixelProc;        break;
        case 32:
            tga.getProc = TgaGet32BitTrueColorPixelProc;        break;
        default:
            TgaError(&tga, "unknown truecolor pixel size %d", 
                     tga.bitsPerPixel);
            break;
        }
        break;

    case TGA_TYPE_GREYSCALE:
        switch (tga.bitsPerPixel) {
        case 8:
            tga.getProc = TgaGet8BitGreyscalePixelProc; break;
        default:
            TgaError(&tga, "unknown greyscale colormap pixel size %d", 
                     tga.bitsPerPixel);
            break;
        }
        break;

    default: 
        TgaError(&tga, "can't handle compressed format %d", tga.imageType);
    }
    if (readerPtr->infoVarObjPtr != NULL) {
        TgaSetInfo(interp, &tga, readerPtr->infoVarObjPtr);
    }
    picture = NULL;
    if ((tga.width > 0) && (tga.height > 0)) {
        picture = TgaGetImageData(&tga);
        if ((tga.bitsPerPixel == 16) || (tga.bitsPerPixel == 32)) {
            Blt_AssociateColors(picture);
        }
        /* Set associated colors flag.  */
        picture->flags |= BLT_PIC_ASSOCIATED_COLORS;
    }

    if (tga.numWarnings > 0) {
        Tcl_SetErrorCode(interp, "PICTURE", "TGA_READ_WARNINGS", 
                Tcl_DStringValue(&tga.warnings), (char *)NULL);
    } else {
        Tcl_SetErrorCode(interp, "NONE", (char *)NULL);
    }
    Tcl_DStringFree(&tga.warnings);
    Tcl_DStringFree(&tga.errors);
    if (picture != NULL) {
        chain = Blt_Chain_Create();
        Blt_Chain_Append(chain, picture);
    }
    TgaFree(&tga);
    return chain;
}

static void
TgaPutHeader(Tga *tgaPtr, TgaWriter *writerPtr)
{
    unsigned char *bp;

    bp = Blt_DBuffer_Extend(tgaPtr->dbuffer, TGA_HEADER_SIZE);
    /* Image header. */
    bp[OFF_ID_LENGTH] = tgaPtr->numBytesId; 
    bp[OFF_CM_TYPE]   = tgaPtr->colorMapExists;
    bp[OFF_IMG_TYPE]  = tgaPtr->imageType | tgaPtr->isRle; 
    TgaSetShort(bp + OFF_CM_OFFSET, 0);         
    TgaSetShort(bp + OFF_CM_NUMENTRIES, tgaPtr->cmNumEntries);  
    bp[OFF_CM_BITSPERPIXEL] = tgaPtr->cmBitsPerPixel;
    TgaSetShort(bp + OFF_X_ORIGIN, tgaPtr->xOrigin);
    TgaSetShort(bp + OFF_Y_ORIGIN, tgaPtr->yOrigin);
    TgaSetShort(bp + OFF_WIDTH,    tgaPtr->width);
    TgaSetShort(bp + OFF_HEIGHT,   tgaPtr->height);
    bp[OFF_BITSPERPIXEL] = tgaPtr->bitsPerPixel;
    bp[OFF_ATTRIBUTES] = (tgaPtr->numAlphaBits) | (tgaPtr->originBits << 4);
    /* Id */
    if (writerPtr->jobName != NULL) {
        bp = Blt_DBuffer_Extend(tgaPtr->dbuffer, tgaPtr->numBytesId + 1);
        strcpy((char *)bp, writerPtr->jobName);
    }
}

static void
TgaPut15BitTrueColorPixelProc(Tga *tgaPtr, Blt_Pixel *sp)
{
    unsigned int pixel;

    pixel = (sp->Blue >> 3) | ((sp->Green >> 3) << 5) | 
        ((sp->Red >> 3) << 10);
    Blt_DBuffer_AppendShort(tgaPtr->dbuffer, pixel);
}

static void
TgaPut16BitTrueColorPixelProc(Tga *tgaPtr, Blt_Pixel *sp)
{
    unsigned int pixel;

    pixel = (sp->Blue >> 3) | ((sp->Green >> 3) << 5) | 
        ((sp->Red >> 3) << 10);
    if (sp->Alpha == 0) {
        pixel |= 0x8000;
    }
    Blt_DBuffer_AppendShort(tgaPtr->dbuffer, pixel);
}

static void
TgaPut24BitTrueColorPixelProc(Tga *tgaPtr, Blt_Pixel *sp)
{
    Blt_DBuffer_AppendByte(tgaPtr->dbuffer, sp->Blue);
    Blt_DBuffer_AppendByte(tgaPtr->dbuffer, sp->Green);
    Blt_DBuffer_AppendByte(tgaPtr->dbuffer, sp->Red);
}

static void
TgaPut32BitTrueColorPixelProc(Tga *tgaPtr, Blt_Pixel *sp)
{
    Blt_DBuffer_AppendByte(tgaPtr->dbuffer, sp->Blue);
    Blt_DBuffer_AppendByte(tgaPtr->dbuffer, sp->Green);
    Blt_DBuffer_AppendByte(tgaPtr->dbuffer, sp->Red);
    Blt_DBuffer_AppendByte(tgaPtr->dbuffer, sp->Alpha);
}

static void
TgaPut8BitGreyscalePixelProc(Tga *tgaPtr, Blt_Pixel *sp)
{
    Blt_DBuffer_AppendByte(tgaPtr->dbuffer, sp->Red);
}

static void
TgaPut8BitPseudoColorPixelProc(Tga *tgaPtr, Blt_Pixel *sp)
{
    unsigned long pixel;
    unsigned long index;
    Blt_HashEntry *hPtr;

    pixel = (unsigned long)sp->u32;
    hPtr = Blt_FindHashEntry(&tgaPtr->colorTable, (char *)pixel);
    if (hPtr == NULL) {
        TgaError(tgaPtr, "can't find 8-bit pixel %lu in colortable", pixel);
    }
    index = (unsigned long)Blt_GetHashValue(hPtr);
    if (index >= tgaPtr->cmNumEntries) {
        TgaError(tgaPtr, "invalid index %d for 8-bit pixel %lu", index, pixel);
    }
    Blt_DBuffer_AppendByte(tgaPtr->dbuffer, index);
}

static void
TgaPutColorTable(Tga *tgaPtr) 
{
    Blt_Pixel *sp, *send;
    unsigned char *dp;

    switch (tgaPtr->cmBitsPerPixel) {
    case 24:                            /* 8-8-8 RGB color. */
        dp = Blt_DBuffer_Extend(tgaPtr->dbuffer, tgaPtr->cmNumEntries * 3);
        for (sp = tgaPtr->palette, send = sp + tgaPtr->cmNumEntries; sp < send;
             sp++) {
            dp[0] = sp->Blue;
            dp[1] = sp->Green;
            dp[2] = sp->Red;
            dp += 3;
        }
        break;

    case 32:                            /* 8-8-8-8 RGBA color. */
        dp = Blt_DBuffer_Extend(tgaPtr->dbuffer, tgaPtr->cmNumEntries * 4);
        for (sp = tgaPtr->palette, send = sp + tgaPtr->cmNumEntries; sp < send;
             sp++) {
            dp[0] = sp->Blue;
            dp[1] = sp->Green;
            dp[2] = sp->Red;
            dp[3] = sp->Alpha;
            dp += 4;
        }
        break;

    case 8:                             /* 8-bit pseudocolor. */
    case 15:                            /* 5-5-5 RGB color */
    case 16:                            /* 1-5-5-5 RGBA color */
        TgaError(tgaPtr, "internal error: colormap pixel size (%d)", 
                 tgaPtr->cmBitsPerPixel);
        return;                         /* NOTREACHED */
    default:                            /* Unknown pixel format. */
        TgaError(tgaPtr, "unknown colormap pixel size (%d)", 
                 tgaPtr->cmBitsPerPixel);
        return;                         /* NOTREACHED */
    }
    Blt_DBuffer_SetPointer(tgaPtr->dbuffer, dp);
    return;
}

static void
TgaPutImageData(Tga *tgaPtr, Pict *srcPtr) 
{
    TgaPutPixelProc *proc;

    proc = NULL;                        /* Suppress compiler warning. */
    switch (tgaPtr->bitsPerPixel) {
    case 8:                             /* 8-bit greyscale or pseudocolor. */
        proc = (tgaPtr->imageType == TGA_TYPE_GREYSCALE) ?
            TgaPut8BitGreyscalePixelProc : TgaPut8BitPseudoColorPixelProc;
        break;
    case 24:                            /* 8-8-8 RGB color. */
        proc = TgaPut24BitTrueColorPixelProc;  break;
    case 32:                            /* 8-8-8-8 RGBA color. */
        proc = TgaPut32BitTrueColorPixelProc;  break;
    case 15:                            /* 5-5-5 RGB color */
        proc = TgaPut15BitTrueColorPixelProc;  break;
    case 16:                            /* 1-5-5-5 RGBA color */
        proc = TgaPut16BitTrueColorPixelProc;  break;
    default:                            /* Unknown pixel format. */
        Blt_Panic("unknown pixel size (%d)", tgaPtr->bitsPerPixel);
    }
    if (tgaPtr->isRle) {
        Blt_Pixel *srcRowPtr;
        int y;
        
        srcRowPtr = srcPtr->bits + (srcPtr->height - 1) * srcPtr->pixelsPerRow;
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *send;
            
            for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send;
                 /*empty*/) {
                 Blt_Pixel *lastp;

                /* Duplicate consecutive pixels. Find the length of the
                 * run. */
                lastp = sp;
                if (sp->u32 == lastp->u32) {
                    int count;

                    count = 0;
                    sp++;
                    while((lastp->u32 == sp->u32) && (sp < send) && 
                          (count < 127)) {
                        count++;
                        sp++;
                    }
                    Blt_DBuffer_AppendByte(tgaPtr->dbuffer, count | PKT_RLE);
                    (*proc)(tgaPtr, lastp);
                } else {
                    int count;

                    /* Determine length of non-duplicated pixels. */
                    count = 0;
                    sp++;
                    while ((lastp->u32 != sp->u32) && (sp < send) && 
                          (count < 127)) {
                        count++;
                        sp++;
                    }
                    Blt_DBuffer_AppendByte(tgaPtr->dbuffer, count);
                    while (lastp->u32 != sp->u32) {
                        (*proc)(tgaPtr, lastp);
                        lastp++;
                    }
                }
            }
            srcRowPtr -= srcPtr->pixelsPerRow;
        }
    } else {
        Blt_Pixel *srcRowPtr;
        int y;
        
        srcRowPtr = srcPtr->bits + (srcPtr->height - 1) * srcPtr->pixelsPerRow;
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *send;
            
            for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
                (*proc)(tgaPtr, sp);
            }
            srcRowPtr -= srcPtr->pixelsPerRow;
        }
    }
    Blt_DBuffer_SetCursor(tgaPtr->dbuffer, Blt_DBuffer_Length(tgaPtr->dbuffer));
}

static void
TgaPutFooter(Tga *tgaPtr) 
{
    unsigned char *dp;

    dp = Blt_DBuffer_Extend(tgaPtr->dbuffer, TGA_FOOTER_SIZE);
    TgaSetLong(dp, tgaPtr->extOffset + OFF_EXT_OFFSET);
    TgaSetLong(dp + OFF_DEV_OFFSET, tgaPtr->devOffset);
    strcpy((char *)dp + OFF_TRUEVISION, "TRUEVISION-XFILE.");
    Blt_DBuffer_SetPointer(tgaPtr->dbuffer, dp + TGA_FOOTER_SIZE);
}

static void
TgaPutExtension(Tga *tgaPtr, TgaWriter *writerPtr) 
{
    unsigned char *bp;
    time_t ticks;
    struct tm tm;

    tgaPtr->extOffset = Blt_DBuffer_Cursor(tgaPtr->dbuffer);
    bp = Blt_DBuffer_Extend(tgaPtr->dbuffer, TGA_EXT_SIZE);
    memset(bp, 0, TGA_EXT_SIZE);        /* Clear the extension area with zero
                                         * bytes.  */
    /* Compute size of extension */
    TgaSetShort(bp + OFF_EXT_OFFSET, TGA_EXT_SIZE);
    if (writerPtr->author != NULL) {
        strncpy((char *)bp + OFF_AUTHOR, writerPtr->author, TGA_AUTHOR_SIZE);
    }
    if (writerPtr->comments != NULL) {
        const char *p, *start;
        int line;
        line = 0;

        for (start = p = writerPtr->comments; *p != '\0'; p++) {
            if (*p == '\n') {
                int offset, length;
                
                offset = line * (TGA_COMMENT_SIZE+1);
                length = MIN(p - start, TGA_COMMENT_SIZE);
                strncpy((char *)bp + OFF_COMMENT1 + offset, start, length);
                line++;
                start = p + 1;
                if (line == 4) {
                    break;
                }
            } 
        }
        if ((p != start) && (line < 4)) {
            size_t offset, length;
            
            offset = line * (TGA_COMMENT_SIZE+1);
            length = MIN(p - start, TGA_COMMENT_SIZE);
            strncpy((char *)bp + OFF_COMMENT1 + offset, start, length);
        }
    }
    ticks = time((time_t *)NULL);
    localtime_r(&ticks, &tm);
    TgaSetShort(bp + OFF_TIME_MONTH, tm.tm_mon + 1);
    TgaSetShort(bp + OFF_TIME_DAY,   tm.tm_mday);
    TgaSetShort(bp + OFF_TIME_YEAR,  tm.tm_year + 1900);
    TgaSetShort(bp + OFF_TIME_HOUR,  tm.tm_hour);
    TgaSetShort(bp + OFF_TIME_MIN,   tm.tm_min);
    TgaSetShort(bp + OFF_TIME_SEC,   tm.tm_sec);

    if (writerPtr->jobName != NULL) {
        strncpy((char *)bp + OFF_JOB_NAME, writerPtr->jobName,
                TGA_JOBNAME_SIZE);
    }
    if (writerPtr->software != NULL) {
        strncpy((char *)bp + OFF_SW_ID, writerPtr->software, TGA_SW_SIZE);
    }
    TgaSetShort(bp + OFF_SW_VERS_NUM, 0); 
    bp[OFF_SW_VERS_LET] = ' ';                  
}

/*
 *---------------------------------------------------------------------------
 *
 * PictureToTga --
 *
 *      Writes a TGA file from a picture.
 *
 * Results:
 *      A standard TCL result.  If an error occured, the interpreter result
 *      contain an error message and TCL_ERROR is returned.
 *
 * Side Effects:
 *      The dynamic buffer will contain to contains of the Tga image.
 *
 *---------------------------------------------------------------------------
 */
static int
PictureToTga(Tcl_Interp *interp, Blt_Picture original, Blt_DBuffer dbuffer,
             TgaWriter *writerPtr)
{
    Tga tga;
    int numColors;
    Pict *srcPtr;

    srcPtr = original;
    memset(&tga, 0, sizeof(Tga));
    tga.version = 2;                    /* Alway write TGA version 2 files. */
    tga.interleaveBits = 0;             /* Never interleave. */
    tga.originBits = 0;                 /* Always order scanlines bottom to
                                         * top. */
    tga.dbuffer = dbuffer;
    tga.width = srcPtr->width;
    tga.height = srcPtr->height;

    Blt_InitHashTable(&tga.colorTable, BLT_ONE_WORD_KEYS);
    tga.initialized = TRUE;
    tga.isRle = (writerPtr->flags & EXPORT_RLE) ? TGA_RLE : 0;
    numColors = Blt_QueryColors(srcPtr, &tga.colorTable); 
    if ((writerPtr->flags & EXPORT_ALPHA) && 
        (srcPtr->flags & (BLT_PIC_ALPHAS|BLT_PIC_MASK))) {
            fprintf(stderr, "using transparency with %d colors\n", numColors);
        /* Want transparency and have transparency. */
        if (numColors > 256) {
            tga.bitsPerPixel = 32;
            tga.imageType = TGA_TYPE_TRUECOLOR;
            tga.numAlphaBits = 8;
            tga.colorMapExists = FALSE;
            tga.cmNumEntries = 0;
            tga.cmBitsPerPixel = 0;
        } else {
            Blt_HashEntry *hPtr;
            Blt_HashSearch iter;
            unsigned long i;

            tga.bitsPerPixel = 8;
            tga.imageType = TGA_TYPE_PSEUDOCOLOR;
            for (i = 0, hPtr = Blt_FirstHashEntry(&tga.colorTable, &iter); 
                 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter), i++) {
                unsigned int pixel;

                Blt_SetHashValue(hPtr, i);
                pixel = (unsigned long)Blt_GetHashKey(&tga.colorTable, hPtr);
                tga.palette[i].u32 = (unsigned int)pixel;
            } 
            tga.colorMapExists = TRUE;
            tga.cmNumEntries = i;
            tga.cmBitsPerPixel = 32;
            tga.numAlphaBits = 8;
        }
    } else {
        tga.numAlphaBits = 0;
        /* Blend the picture with a background if there is transparency. */
        if (!Blt_Picture_IsOpaque(srcPtr)) {    
            Blt_Picture background;

            /* Blend picture with solid color background. */
            background = Blt_CreatePicture(srcPtr->width, srcPtr->height);
            Blt_BlankPicture(background, writerPtr->bg.u32); 
            Blt_CompositePictures(background, srcPtr);
            if (srcPtr != original) {
                Blt_FreePicture(srcPtr);
            }
            srcPtr = background;
            /* Query the colors again after blending. */
            Blt_DeleteHashTable(&tga.colorTable);
            Blt_InitHashTable(&tga.colorTable, BLT_ONE_WORD_KEYS);
            numColors = Blt_QueryColors(srcPtr, &tga.colorTable); 
        }
        if (numColors > 256) {
            tga.imageType = TGA_TYPE_TRUECOLOR;
            tga.bitsPerPixel = 24;
            tga.colorMapExists = FALSE;
            tga.cmBitsPerPixel = 0;
        } else {
            tga.bitsPerPixel = 8;
            if (srcPtr->flags & BLT_PIC_COLOR) {
                int i;
                Blt_HashEntry *hPtr;
                Blt_HashSearch iter;
                
                /* Add indices to the colortable.  */
                for (i = 0, hPtr = Blt_FirstHashEntry(&tga.colorTable, &iter); 
                     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter), i++) {
                    unsigned long key;
                    
                    Blt_SetHashValue(hPtr, (unsigned long)i);
                    key = (unsigned long)Blt_GetHashKey(&tga.colorTable, hPtr);
                    tga.palette[i].u32 = (unsigned int)key;
                } 
                tga.imageType = TGA_TYPE_PSEUDOCOLOR;
                tga.colorMapExists = TRUE;
                tga.cmNumEntries = i;
                tga.cmBitsPerPixel = 24;
            } else {
                tga.imageType = TGA_TYPE_GREYSCALE;
                tga.colorMapExists = FALSE;
                tga.cmNumEntries = 0;
                tga.cmBitsPerPixel = 0;
            }
        }
    }
    TgaPutHeader(&tga, writerPtr);
    if (tga.colorMapExists) {
        TgaPutColorTable(&tga);
    }
    TgaPutImageData(&tga, srcPtr);
    TgaPutExtension(&tga, writerPtr);
    /* Thumbnail. */
    TgaPutFooter(&tga);
    TgaFree(&tga);
    if (srcPtr != original) {
        Blt_FreePicture(srcPtr);
    }
    return TCL_OK;
}

/*
 * ---------------------------------------------------------------------------
 *
 * IsTga --
 *
 *      Attempts to parse a TGA file header.
 *
 * Results:
 *      Returns 1 is the header is TGA and 0 otherwise.  
 *
 * ---------------------------------------------------------------------------
 */
static int
IsTga(Blt_DBuffer dbuffer)
{
    Tga tga;

    return TgaGetHeader(dbuffer, &tga);
}

static Blt_Chain
ReadTga(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer)
{
    TgaReader reader;

    memset(&reader, 0, sizeof(reader));
    return TgaToPicture(interp, fileName, dbuffer, &reader);
}

static Tcl_Obj *
WriteTga(Tcl_Interp *interp, Blt_Picture picture)
{
    Blt_DBuffer dbuffer;
    TgaWriter writer;
    Tcl_Obj *objPtr;

    /* Default export switch settings. */
    memset(&writer, 0, sizeof(writer));
    writer.bg.u32 = 0xFFFFFFFF;         /* Default background is white. */

    dbuffer = Blt_DBuffer_Create();
    objPtr = NULL;
    if (PictureToTga(interp, picture, dbuffer, &writer) == TCL_OK) {
        objPtr = Blt_DBuffer_Base64EncodeToObj(dbuffer);
    }
    return objPtr;
}

static Blt_Chain
ImportTga(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, 
          const char **fileNamePtr)
{
    Blt_DBuffer dbuffer;
    Blt_Chain chain;
    const char *string;
    TgaReader reader;

    memset(&reader, 0, sizeof(reader));
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3, objv + 3, 
        &reader, BLT_SWITCH_DEFAULTS) < 0) {
        Blt_FreeSwitches(importSwitches, (char *)&reader, 0);
        return NULL;
    }
    if ((reader.dataObjPtr != NULL) && (reader.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "more than one import source: ",
                "use only one -file or -data flag.", (char *)NULL);
        Blt_FreeSwitches(importSwitches, (char *)&reader, 0);
        return NULL;
    }
    dbuffer = NULL;                     /* Suppress compiler warning. */
    chain = NULL;
    if (reader.dataObjPtr != NULL) {
        unsigned char *bytes;
        int numBytes;

        bytes = Tcl_GetByteArrayFromObj(reader.dataObjPtr, &numBytes);
        string = (const char *)bytes;
        if (Blt_IsBase64(string, numBytes)) {
            dbuffer = Blt_Base64_DecodeToBuffer(interp, string, numBytes);
            if (dbuffer == NULL) {
                goto error;
            }
        } else {
            dbuffer = Blt_DBuffer_Create();
            Blt_DBuffer_AppendData(dbuffer, bytes, numBytes);
        } 
        string = "data buffer";
        *fileNamePtr = NULL;
    } else {
        string = Tcl_GetString(reader.fileObjPtr);
        *fileNamePtr = string;
        dbuffer = Blt_DBuffer_Create();
        if (Blt_DBuffer_LoadFile(interp, string, dbuffer) != TCL_OK) {
            goto error;
        }
    }
    chain = TgaToPicture(interp, string, dbuffer, &reader);
    if (chain == NULL) {
        return NULL;
    }
 error:
    Blt_FreeSwitches(importSwitches, (char *)&reader, 0);
    Blt_DBuffer_Destroy(dbuffer);
    return chain;
}

static int
ExportTga(Tcl_Interp *interp, int index, Blt_Chain chain, int objc,
          Tcl_Obj *const *objv)
{
    Blt_DBuffer dbuffer;
    Blt_Picture picture;
    TgaWriter writer;
    int result;

    memset(&writer, 0, sizeof(writer));
    writer.bg.u32 = 0xFFFFFFFF; /* Default bgcolor is white. */
    writer.index = index;
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 3, objv + 3, 
        &writer, BLT_SWITCH_DEFAULTS) < 0) {
        Blt_FreeSwitches(exportSwitches, (char *)&writer, 0);
        return TCL_ERROR;
    }
    if ((writer.dataObjPtr != NULL) && (writer.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "more than one export destination: ",
                "use only one -file or -data switch.", (char *)NULL);
        Blt_FreeSwitches(exportSwitches, (char *)&writer, 0);
        return TCL_ERROR;
    }
    picture = Blt_GetNthPicture(chain, writer.index);
    if (picture == NULL) {
        Tcl_AppendResult(interp, "bad picture index.", (char *)NULL);
        Blt_FreeSwitches(exportSwitches, (char *)&writer, 0);
        return TCL_ERROR;
    }
    dbuffer = Blt_DBuffer_Create();
    result = PictureToTga(interp, picture, dbuffer, &writer);
    if (result != TCL_OK) {
        Tcl_AppendResult(interp, "can't convert \"", 
                Tcl_GetString(objv[2]), "\"", (char *)NULL);
        goto error;
    }
    if (writer.fileObjPtr != NULL) {
        const char *fileName;

        /* Write the image into the designated file. */
        fileName = Tcl_GetString(writer.fileObjPtr);
        result = Blt_DBuffer_SaveFile(interp, fileName, dbuffer);
    } else if (writer.dataObjPtr != NULL) {
        Tcl_Obj *objPtr;

        /* Write the image into the designated TCL variable. */
        objPtr = Tcl_ObjSetVar2(interp, writer.dataObjPtr, NULL, 
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
    Blt_FreeSwitches(exportSwitches, (char *)&writer, 0);
    Blt_DBuffer_Destroy(dbuffer);
    return result;
}

int 
Blt_PictureTgaInit(Tcl_Interp *interp)
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
#else 
    if (Tcl_PkgRequire(interp, "blt_tcl", BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_PkgRequire(interp, "blt_tk", BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    }
#endif    
    if (Tcl_PkgProvide(interp, "blt_picture_tga", BLT_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }
    return Blt_PictureRegisterFormat(interp, 
        "tga",                  /* Name of format. */
        IsTga,                  /* Discovery routine. */
        ReadTga,                /* Read format procedure. */
        WriteTga,               /* Write format procedure. */
        ImportTga,              /* Import format procedure. */
        ExportTga);             /* Export format procedure. */
}

int 
Blt_PictureTgaSafeInit(Tcl_Interp *interp) 
{
    return Blt_PictureTgaInit(interp);
}

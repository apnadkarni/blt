/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltPictBmp.c --
 *
 * This module implements BMP file format conversion routines for the picture
 * image type in the BLT toolkit.
 *
 *	Copyright 2003-2005 George A Howlett.
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
 *
 */

#include "bltInt.h"

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /*HAVE_STRING_H*/

#ifdef _MSC_VER
#define vsnprintf		_vsnprintf
#endif

#include <setjmp.h>

typedef struct _Blt_Picture Picture;

#define TRUE 	1
#define FALSE 	0


#define OFF_TYPE		0
#define OFF_FILE_SIZE		2
#define OFF_RESERVED1		6
#define OFF_RESERVED2		8
#define OFF_OFFBITS		10

#define OFF_SIZE		14
#define OFF_WIDTH		18
#define OFF_HEIGHT		22
#define OFF_PLANES		26
#define OFF_BIT_COUNT		28
#define OFF_COMPRESSION		30
#define OFF_SIZE_IMAGE		34
#define OFF_X_PELS_PER_METER	38
#define OFF_Y_PELS_PER_METER	42
#define OFF_CLR_USED		46
#define OFF_CLR_IMPORTANT	50
#define OFF_RED_MASK		54
#define OFF_GREEN_MASK		58
#define OFF_BLUE_MASK		62
#define OFF_ALPHA_MASK		66
#define OFF_CS_TYPE		70
#define OFF_END_POINTS		74
#define OFF_GAMMA_RED		110 
#define OFF_GAMMA_GREEN		114
#define OFF_GAMMA_BLUE		118  
#define OFF_INTENT		122 
#define OFF_PROFILE_DATA	126
#define OFF_PROFILE_SIZE	130
#define OFF_RESERVED3		134 

#define OFF_OSV1_SIZE		14
#define OFF_OSV1_WIDTH		18
#define OFF_OSV1_HEIGHT		20
#define OFF_OSV1_PLANES		22
#define OFF_OSV1_BIT_COUNT	24

#define SIZEOF_BITMAPOS2V1HEADER 12
#define SIZEOF_BITMAPOS2V2HEADER 64
#define SIZEOF_BITMAPV3HEADER    40
#define SIZEOF_BITMAPV4HEADER	 108
#define SIZEOF_BITMAPV5HEADER	 124


typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
} BmpImportSwitches;

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    int flags;			/* Flag. */
    Blt_Pixel bg;
    int index;
} BmpExportSwitches;

#define EXPORT_ALPHA	(1<<0)

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data",  "data", (char *)NULL,
	Blt_Offset(BmpImportSwitches, dataObjPtr), 0},
    {BLT_SWITCH_OBJ, "-file",  "fileName", (char *)NULL,
	Blt_Offset(BmpImportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchParseProc ColorSwitchProc;
static Blt_SwitchCustom colorSwitch = {
    ColorSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data",  "data", (char *)NULL,
	Blt_Offset(BmpExportSwitches, dataObjPtr), 0},
    {BLT_SWITCH_OBJ, "-file",  "fileName", (char *)NULL,
	Blt_Offset(BmpExportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_CUSTOM, "-bg", "color", (char *)NULL,
	Blt_Offset(BmpExportSwitches, bg),         0, 0, &colorSwitch},
    {BLT_SWITCH_BITMASK, "-alpha", "", (char *)NULL,
	Blt_Offset(BmpExportSwitches, flags),   0, EXPORT_ALPHA},
    {BLT_SWITCH_INT_NNEG, "-index", "int", (char *)NULL,
	Blt_Offset(BmpExportSwitches, index), 0},
    {BLT_SWITCH_END}
};

typedef struct {
    jmp_buf jmpbuf;
    Tcl_DString errors;
    Tcl_DString warnings;
    int numWarnings, numErrors;
} BmpMessage;

static BmpMessage *bmpMessagePtr;

static const char *compression_types[] = {
    "RGB", "RLE8", "RLE4", "BITFIELDS", "JPEG", "PNG"
};

#ifndef WIN32
enum CompressionTypes {
    BI_RGB,				/* No compression. */
    BI_RLE8,				/* RLE 8-bits/pixel. Only used with
					 * 8-bit/pixel bitmaps */  
    BI_RLE4,				/* RLE 4-bits/pixel. Can be used only
					 * with 4-bit/pixel bitmaps */
    BI_BITFIELDS,			/* Bit fields. Can be used only with
					 * 16 and 32-bit/pixel bitmaps. */
    BI_JPEG,				/* The bitmap contains a JPEG
					 * image. */
    BI_PNG,				/* The bitmap contains a PNG image. */
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
    int  RedX;				/* X coordinate of red endpoint */
    int  RedY;				/* Y coordinate of red endpoint */
    int  RedZ;				/* Z coordinate of red endpoint */
    int  GreenX;			/* X coordinate of green endpoint */
    int  GreenY;			/* Y coordinate of green endpoint */
    int  GreenZ;			/* Z coordinate of green endpoint */
    int  BlueX;				/* X coordinate of blue endpoint */
    int  BlueY;				/* Y coordinate of blue endpoint */
    int  BlueZ;				/* Z coordinate of blue endpoint */
} CieXyzTriple;

typedef struct {
    unsigned int   biSize;		/* Size of this structure. This
					 * determines what version of the
					 * header is * used. */
    int		   biWidth; 
    int		   biHeight; 
    unsigned short biPlanes; 
    unsigned short biBitCount; 
    unsigned int   biCompression; 
    unsigned int   biSizeImage; 
    int		   biXPelsPerMeter; 
    int		   biYPelsPerMeter; 
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
    BitmapFileHeader bmfh;
    BitmapInfoHeader bmih;
    Blt_Pixel colorTable[MAXCOLORS];
    const char *name;
} Bmp;

DLLEXPORT extern Tcl_AppInitProc Blt_PictureBmpInit;
DLLEXPORT extern Tcl_AppInitProc Blt_PictureBmpSafeInit;

/*
 *---------------------------------------------------------------------------
 *
 * ColorSwitchProc --
 *
 *	Convert a Tcl_Obj representing a Blt_Pixel color.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColorSwitchProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
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
BmpError(const char *fmt, ...)
{
    char string[BUFSIZ+4];
    int length;
    va_list args;

    va_start(args, fmt);
    length = vsnprintf(string, BUFSIZ, fmt, args);
    if (length > BUFSIZ) {
	strcat(string, "...");
    }
    Tcl_DStringAppend(&bmpMessagePtr->errors, string, -1);
    va_end(args);
    longjmp(bmpMessagePtr->jmpbuf, 0);
}

/*ARGSUSED*/
static void
BmpWarning(const char *fmt, ...)
{
    char string[BUFSIZ+4];
    int length;
    va_list args;

    va_start(args, fmt);
    length = vsnprintf(string, BUFSIZ, fmt, args);
    if (length > BUFSIZ) {
	strcat(string, "...");
    }
    Tcl_DStringAppend(&bmpMessagePtr->warnings, string, -1);
    va_end(args);
    bmpMessagePtr->numWarnings++;
}

static INLINE unsigned int
BmpGetLong(unsigned char *buf)
{
#ifdef WORDS_BIGENDIAN
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
#else
    return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
#endif
}

static INLINE unsigned short
BmpGetShort(unsigned char *buf)
{
#ifdef WORDS_BIGENDIAN
    return (buf[0] << 8) | buf[1];
#else
    return buf[0] | (buf[1] << 8);
#endif
}

static INLINE unsigned char *
BmpSetLong(unsigned char *buf, unsigned long value)
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
BmpSetShort(unsigned char *buf, unsigned long value)
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
 *			       Field Name                    Type 
 *   | 0| 1|			bfType                      2 Bytes 
 *   | 2| 3| 4| 5|		bfSize                      4 Bytes 
 *   | 6| 7|			bfReserved1                 4 Bytes 
 *   | 8| 9|			bfReserved2                 4 Bytes 
 *   |10|11|12|13|		bfOffBits                   4 Bytes 
 *   
 */
static int
BmpHeader(Blt_DBuffer dbuffer, Bmp *bmpPtr)
{
    unsigned char *bp;
    size_t fileSize;

    Blt_DBuffer_Rewind(dbuffer);
    fileSize = Blt_DBuffer_BytesLeft(dbuffer);
    if (fileSize < 14) {
	return FALSE;
    }
    bp = Blt_DBuffer_Pointer(dbuffer);
    if ((bp[0] != 'B') || (bp[1] != 'M')) {
	return FALSE;
    }
    bmpPtr->bmfh.bfSize      = BmpGetLong (bp + OFF_FILE_SIZE);
    bmpPtr->bmfh.bfReserved1 = BmpGetShort(bp + OFF_RESERVED1);
    bmpPtr->bmfh.bfReserved2 = BmpGetShort(bp + OFF_RESERVED2);
    bmpPtr->bmfh.bfOffBits   = BmpGetLong (bp + OFF_OFFBITS);
    return TRUE;
}

/*
 * Version 2		Field Name		Size 
 *   | 0| 1| 2| 3|	biSize			4 Bytes 
 *   | 4| 5|		biWidth			2 Bytes 
 *   | 6| 7|		biHeight		2 Bytes 
 *   | 8| 9|		biPlanes		2 Bytes 
 *   |10|11|		biBitCount		2 Bytes 

 * Version 3		
 *   | 0| 1| 2| 3|	biSize			4 Bytes 
 *   | 4| 5| 6| 7|	biWidth			4 Bytes 
 *   | 8| 9|10|11|	biHeight		4 Bytes 
 *   |12|13|		biPlanes		2 Bytes 
 *   |14|15|		biBitCount		2 Bytes 
 *   |16|17|18|19|	biCompression		4 Bytes 
 *   |20|21|22|23|	biSizeImage		4 Bytes 
 *   |24|25|26|27|	biXPelsPerMeter		4 Bytes 
 *   |28|29|30|31|	biYPelsPerMeter		4 Bytes 
 *   |32|33|34|35|	biClrUsed		4 Bytes 
 *   |36|37|38|39|	biClrImportant		4 Bytes 
 * Version 4
 *   |40|41|42|43|	biRedMask		4 Bytes 
 *   |44|45|46|47|	biGreenMask		4 Bytes 
 *   |48|49|50|51|	biBlueMask		4 Bytes 
 *   |52|53|54|55|	biAlphaMask		4 Bytes 
 *   |56|57|58|59|	biCSType		4 Bytes 
 *   |60--95|		biEndpoints		36 Bytes 
 *   |96|97|98|99|	biGammaRed		4 Bytes 
 *   |00|01|02|03|	biGammaGreen		4 Bytes 
 *   |04|05|06|07|	biGammaBlue		4 Bytes 
 * Version 5
 *   |08|09|10|11|	bV5Intent		4 Bytes 
 *   |12|13|14|15|	bV5ProfileData		4 Bytes 
 *   |16|17|18|19|	bV5ProfileSize		4 Bytes 
 *   |20|21|22|23|	bV5Reserved		4 Bytes 
 */

static int
BmpHeaderInfo(Blt_DBuffer dbuffer, Bmp *bmpPtr)
{
    unsigned char *bp;

    bp = Blt_DBuffer_Pointer(dbuffer);
    bmpPtr->bmih.biSize = BmpGetLong(bp + OFF_SIZE);

    /* Verify header size. */
    switch (bmpPtr->bmih.biSize) {
    case SIZEOF_BITMAPOS2V1HEADER:
    case SIZEOF_BITMAPOS2V2HEADER:
    case SIZEOF_BITMAPV3HEADER:
    case SIZEOF_BITMAPV4HEADER:
    case SIZEOF_BITMAPV5HEADER:
	break;
    default:
	BmpError("unknown BMP bitmap header (size=%d).", bmpPtr->bmih.biSize);
    }
    if (bmpPtr->bmih.biSize == SIZEOF_BITMAPOS2V1HEADER) {
	bmpPtr->bmih.biWidth         = (int)BmpGetShort(bp + OFF_OSV1_WIDTH);
	bmpPtr->bmih.biHeight        = (int)BmpGetShort(bp + OFF_OSV1_HEIGHT);
	bmpPtr->bmih.biPlanes        = BmpGetShort(bp + OFF_OSV1_PLANES);
	bmpPtr->bmih.biBitCount      = BmpGetShort(bp + OFF_OSV1_BIT_COUNT);
	bmpPtr->bmih.biCompression   = BI_RGB;
    } else {
	bmpPtr->bmih.biWidth         = (int)BmpGetLong(bp + OFF_WIDTH);
	bmpPtr->bmih.biHeight        = (int)BmpGetLong(bp + OFF_HEIGHT);
	bmpPtr->bmih.biPlanes        = BmpGetShort(bp + OFF_PLANES);
	bmpPtr->bmih.biBitCount      = BmpGetShort(bp + OFF_BIT_COUNT);
	bmpPtr->bmih.biCompression   = BmpGetLong(bp + OFF_COMPRESSION);
	bmpPtr->bmih.biSizeImage     = BmpGetLong(bp + OFF_SIZE_IMAGE);
	bmpPtr->bmih.biXPelsPerMeter = (int)BmpGetLong(bp+OFF_X_PELS_PER_METER);
	bmpPtr->bmih.biYPelsPerMeter = (int)BmpGetLong(bp+OFF_Y_PELS_PER_METER);
	bmpPtr->bmih.biClrUsed       = BmpGetLong(bp + OFF_CLR_USED);
	bmpPtr->bmih.biClrImportant  = BmpGetLong(bp + OFF_CLR_IMPORTANT);
    }	

#ifdef notdef
    fprintf(stderr, "fileName=%s\n", bmpPtr->name);
    fprintf(stderr, "  biSize=%d\n", bmpPtr->bmih.biSize);
    fprintf(stderr, "  biWidth=%d\n", bmpPtr->bmih.biWidth);
    fprintf(stderr, "  biHeight=%d\n", bmpPtr->bmih.biHeight);
    fprintf(stderr, "  biPlanes=%d\n", bmpPtr->bmih.biPlanes);
    fprintf(stderr, "  biBitCount=%d\n", bmpPtr->bmih.biBitCount);
    fprintf(stderr, "  biCompression=%d\n", bmpPtr->bmih.biCompression);
    fprintf(stderr, "  biSizeImage=%d\n", bmpPtr->bmih.biSizeImage);
    fprintf(stderr, "  biClrUsed=%d\n", bmpPtr->bmih.biClrUsed);
#endif

    if (Blt_DBuffer_BytesLeft(dbuffer) < bmpPtr->bmih.biSize) {
	BmpError("bad BMP header, short file");
    }
    if (bmpPtr->bmih.biWidth <= 0) {
	BmpError("invalid image width %d.", bmpPtr->bmih.biWidth);
    }
    if (bmpPtr->bmih.biHeight == 0) {
	BmpError("invalid image height %d.", bmpPtr->bmih.biHeight);
    }
    /* According to the MicroSoft documentation, if the image height is
     * negative, the image data is in top-down order. Since virtually no one
     * does this, read the image data bottom-up. The user can always flip the
     * resulting image. */
    if (bmpPtr->bmih.biHeight < 0) {
	bmpPtr->bmih.biHeight = -bmpPtr->bmih.biHeight;
    }
    if (Blt_DBuffer_Length(dbuffer) < bmpPtr->bmfh.bfSize) {
	int old;

	old = Blt_DBuffer_Length(dbuffer);
	Blt_DBuffer_Resize(dbuffer, bmpPtr->bmfh.bfSize);
	memset(bp + old, 0, bmpPtr->bmfh.bfSize - old);
	Blt_DBuffer_SetLength(dbuffer, bmpPtr->bmfh.bfSize);
    }
    if (bmpPtr->bmih.biSize >= SIZEOF_BITMAPV4HEADER) {
	bmpPtr->bmih.biRedMask     = BmpGetLong(bp + OFF_RED_MASK);
	bmpPtr->bmih.biGreenMask   = BmpGetLong(bp + OFF_GREEN_MASK);
	bmpPtr->bmih.biBlueMask    = BmpGetLong(bp + OFF_BLUE_MASK);
	bmpPtr->bmih.biAlphaMask   = BmpGetLong(bp + OFF_ALPHA_MASK);
	bmpPtr->bmih.biCSType      = BmpGetLong(bp + OFF_CS_TYPE);
	/* Skip CIEXYZ endpoints */
	bmpPtr->bmih.biGammaRed    = BmpGetLong(bp + OFF_GAMMA_RED);
	bmpPtr->bmih.biGammaGreen  = BmpGetLong(bp + OFF_GAMMA_GREEN);
	bmpPtr->bmih.biGammaBlue   = BmpGetLong(bp + OFF_GAMMA_BLUE);
    } 
    if (bmpPtr->bmih.biSize >= SIZEOF_BITMAPV5HEADER) {
	bmpPtr->bmih.biIntent      = BmpGetLong(bp + OFF_INTENT);
	bmpPtr->bmih.biProfileData = BmpGetLong(bp + OFF_PROFILE_DATA);
	bmpPtr->bmih.biProfileSize = BmpGetLong(bp + OFF_PROFILE_SIZE);
	bmpPtr->bmih.biReserved    = BmpGetLong(bp + OFF_RESERVED3);
    }

#ifdef notdef
    if (bmpPtr->bmih.biCSType != 0) {
    fprintf(stderr, "fileName=%s\n", bmpPtr->name);
    fprintf(stderr, "  biRedMask=%x\n", bmpPtr->bmih.biRedMask);
    fprintf(stderr, "  biGreenMask=%x\n", bmpPtr->bmih.biGreenMask);
    fprintf(stderr, "  biBlueMask=%x\n", bmpPtr->bmih.biBlueMask);
    fprintf(stderr, "  biAlphaMask=%x\n", bmpPtr->bmih.biAlphaMask);
    fprintf(stderr, "  biCSType=%d\n", bmpPtr->bmih.biCSType);
    fprintf(stderr, "  biGammaRed=%d\n", bmpPtr->bmih.biGammaRed);
    fprintf(stderr, "  biGammaGreen=%d\n", bmpPtr->bmih.biGammaGreen);
    fprintf(stderr, "  biGammaBlue=%d\n", bmpPtr->bmih.biGammaBlue);
    fprintf(stderr, "  biIntent=%d\n", bmpPtr->bmih.biIntent);
    fprintf(stderr, "  biProfileData=%d\n", bmpPtr->bmih.biProfileData);
    fprintf(stderr, "  biProfileSize=%d\n", bmpPtr->bmih.biProfileSize);
    fprintf(stderr, "  biReserved=%d\n", bmpPtr->bmih.biReserved);
    }
#endif

    /* Verify bits per pixel count and colors used. */
    switch (bmpPtr->bmih.biBitCount) {
    case 1:				/* 2-bits, Monochrome */
	if (bmpPtr->bmih.biClrUsed > 2) {
	    BmpError("wrong # colors (%d), expecting <= 2 colors.", 
		     bmpPtr->bmih.biClrUsed);
	}
	if (bmpPtr->bmih.biClrUsed == 0) {
	    bmpPtr->bmih.biClrUsed = 2;
	}
	break;
    case 4:				/* 4-bits, 16 colors. */
	if (bmpPtr->bmih.biClrUsed > 16) {
	    BmpError("wrong # colors (%d), expecting <= 16 colors.",
		     bmpPtr->bmih.biClrUsed);
	}
	if (bmpPtr->bmih.biClrUsed == 0) {
	    bmpPtr->bmih.biClrUsed = 16;
	}
	break;
    case 8:				/* 8-bits, 256 colors */
	if (bmpPtr->bmih.biClrUsed > 256) {
	    BmpError("wrong # colors (%d), expecting <= 256 colors.",
		     bmpPtr->bmih.biClrUsed);
	}
	if (bmpPtr->bmih.biClrUsed == 0) {
	    bmpPtr->bmih.biClrUsed = 256;
	}
	break;
    case 16:
    case 24:
    case 32:				/* True color. */
	if (bmpPtr->bmih.biClrUsed != 0) {
	    BmpWarning("# colors is %d, expecting 0 colors in %d-bit image.",
		       bmpPtr->bmih.biClrUsed, bmpPtr->bmih.biBitCount);
	    bmpPtr->bmih.biClrUsed = 0;
	}
	break;
    default:
	BmpError("invalid # bits per pixel (%d)", bmpPtr->bmih.biBitCount);
	break;
    }	

    /* Verify compression type. */
    switch (bmpPtr->bmih.biCompression) {
    case BI_RGB:
	break;
    case BI_RLE4:
	if (bmpPtr->bmih.biBitCount != 4) {
	    BmpError("wrong # bits per pixel (%d) for RLE4 compression",
		     bmpPtr->bmih.biBitCount);
	}
	break;
    case BI_RLE8:
	if (bmpPtr->bmih.biBitCount != 8) {
	    BmpError("wrong # bits per pixel (%d) for RLE8 compression",
		     bmpPtr->bmih.biBitCount);
	}
	break;
    case BI_BITFIELDS:
	if ((bmpPtr->bmih.biBitCount != 16)&&(bmpPtr->bmih.biBitCount != 32)) {
	    BmpError("wrong # bits per pixel (%d) for BITFIELD compression",
		     bmpPtr->bmih.biBitCount);
	}
	break;
    case BI_PNG:
    case BI_JPEG:
	BmpError("compression type \"%s\" not implemented",
		 compression_types[bmpPtr->bmih.biCompression]);
	break;
    default:
	BmpError("unknown compression type (%d)", bmpPtr->bmih.biCompression);
    }      
    Blt_DBuffer_SetPointer(dbuffer, bp + bmpPtr->bmih.biSize + 14);
    return TRUE;
}

static int
BmpPalette(Blt_DBuffer dbuffer, Bmp *bmpPtr)
{
    unsigned char *bp;

    bp = Blt_DBuffer_Pointer(dbuffer);
    if (bmpPtr->bmih.biClrUsed == 0) {
	if (bmpPtr->bmih.biSize == SIZEOF_BITMAPV3HEADER) {
	    bmpPtr->bmih.biRedMask = BmpGetLong(bp);
	    bmpPtr->bmih.biGreenMask = BmpGetLong(bp + 4);
	    bmpPtr->bmih.biBlueMask = BmpGetLong(bp + 8);
	    bmpPtr->bmih.biAlphaMask = 0;
	}
    } else {
	int sizeElem;
	int i;

	assert(bmpPtr->bmih.biClrUsed <= 256);
	sizeElem = (bmpPtr->bmih.biSize == SIZEOF_BITMAPOS2V1HEADER) ? 3 : 4;
	if (Blt_DBuffer_BytesLeft(dbuffer) < 
	    (bmpPtr->bmih.biClrUsed * sizeElem)) {
	    BmpError("short file");
	}
	for (i = 0; i < bmpPtr->bmih.biClrUsed; i++, bp += sizeElem) {
	    /* Colormap components are ordered BGBA. */
	    bmpPtr->colorTable[i].Blue  = bp[0];
	    bmpPtr->colorTable[i].Green = bp[1];
	    bmpPtr->colorTable[i].Red   = bp[2];
	    bmpPtr->colorTable[i].Alpha = ALPHA_OPAQUE;
	}
    }
    Blt_DBuffer_SetPointer(dbuffer, bp);
    return TRUE;
}

static Blt_Picture
BmpRgbImageData(Blt_DBuffer dbuffer, Bmp *bmpPtr)
{
    Blt_Pixel *destRowPtr;
    Picture *destPtr;
    unsigned char *srcBits;
    unsigned int bytesPerRow, wordsPerRow;
    int w, h;

    w = bmpPtr->bmih.biWidth;
    h = bmpPtr->bmih.biHeight;

    wordsPerRow = (w * bmpPtr->bmih.biBitCount + 31) / 32;
    bytesPerRow = wordsPerRow * 4;

    srcBits = Blt_DBuffer_Pointer(dbuffer);
    if (Blt_DBuffer_BytesLeft(dbuffer) < (bytesPerRow * h)) {
	BmpError("image size is %d, need %u bytes", 
		Blt_DBuffer_BytesLeft(dbuffer), bytesPerRow * (unsigned int)h);
    }
    destPtr = Blt_CreatePicture(w, h);
    destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * h);
    switch (bmpPtr->bmih.biBitCount) {
    case 1:
	{
	    int y;
	    unsigned char *srcRowPtr;

	    srcRowPtr = srcBits;
	    for (y = 0; y < h; y++) {
		Blt_Pixel *dp, *dend;

		destRowPtr -= destPtr->pixelsPerRow;
		for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
		    int x;
		    unsigned char byte;
		    x = dp - destRowPtr;

		    byte = srcRowPtr[x>>3];
		    dp->u32 = (byte & (0x80 >> (x & 7))) ? 
			bmpPtr->colorTable[1].u32 : 
			bmpPtr->colorTable[0].u32;
		    dp->Alpha = ALPHA_OPAQUE;
		}
		srcRowPtr += bytesPerRow;
	    }
	}
	break;
    case 4:
	{
	    int y;
	    unsigned char *srcRowPtr;

	    srcRowPtr = srcBits;
	    for (y = 0; y < h; y++) {
		Blt_Pixel *dp, *dend;
		unsigned char *sp;

		destRowPtr -= destPtr->pixelsPerRow;
		sp = srcRowPtr;
		for (dp = destRowPtr, dend = dp + w; dp < dend; /*empty*/) {
		    unsigned int pixel;
		    
		    pixel = ((sp[0] >> 4) & 0x0F);
		    dp->u32 = bmpPtr->colorTable[pixel].u32;
		    dp++;
		    pixel = (sp[0] & 0x0F);
		    dp->u32 = bmpPtr->colorTable[pixel].u32;
		    dp++;
		    sp++;
		}
		srcRowPtr += bytesPerRow;
	    }
	}
	break;
    case 8:
	{
	    int y;
	    unsigned char *srcRowPtr;

	    srcRowPtr = srcBits;
	    for (y = 0; y < h; y++) {
		Blt_Pixel *dp, *dend;
		unsigned char *sp;

		destRowPtr -= destPtr->pixelsPerRow;
		sp = srcRowPtr;
		for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
		    dp->u32 = bmpPtr->colorTable[*sp].u32;
		    sp++;
		}
		srcRowPtr += bytesPerRow;
	    }
	    break;
	}
    case 16:
	{
	    int y;
	    unsigned char *srcRowPtr;

	    srcRowPtr = srcBits;
	    for (y = 0; y < h; y++) {
		Blt_Pixel *dp, *dend;
		unsigned char *sp;

		destRowPtr -= destPtr->pixelsPerRow;
		sp = srcRowPtr;
		for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
		    unsigned int pixel;
		    
		    pixel = BmpGetShort(sp);
		    dp->Blue  = (pixel & 0x001f) << 3;
		    dp->Green = (pixel & 0x03e0) >> 2;
		    dp->Red   = (pixel & 0x7c00) >> 7;
		    dp->Alpha = ALPHA_OPAQUE;
		    sp += 2;
		}
		srcRowPtr += bytesPerRow;
	    }
	}
	break;
    case 24:
	{
	    int y;
	    unsigned char *srcRowPtr;

	    srcRowPtr = srcBits;
	    for (y = 0; y < h; y++) {
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
	}
	break;
    case 32:
	{
	    int y;
	    unsigned char *srcRowPtr;

	    srcRowPtr = srcBits;
	    for (y = 0; y < h; y++) {
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
	break;
    }
    return destPtr;
}

/*-------------------------------------------------------------------------------
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
CountBits(unsigned long mask)		/* 32  1-bit tallies */
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
 *	Returns the position of the least significant (low) bit in the given
 *	mask.
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
BmpBitfieldImageData(Blt_DBuffer dbuffer, Bmp *bmpPtr)
{
    Picture *destPtr;
    unsigned char *srcBits;
    unsigned int bytesPerRow, wordsPerRow;
    int w, h;
    unsigned int rShift, gShift, bShift, aShift;
    unsigned int rMask, gMask, bMask, aMask;
    unsigned int rAdjust, gAdjust, bAdjust, aAdjust;

    rMask = bmpPtr->bmih.biRedMask;
    gMask = bmpPtr->bmih.biGreenMask;
    bMask = bmpPtr->bmih.biBlueMask;
    aMask = bmpPtr->bmih.biAlphaMask;

    rShift = FindShift(rMask);
    gShift = FindShift(gMask);
    bShift = FindShift(bMask);
    aShift = FindShift(aMask);

    rAdjust = GetAdjust(rMask);
    gAdjust = GetAdjust(gMask);
    bAdjust = GetAdjust(bMask);
    aAdjust = GetAdjust(aMask);

    w = bmpPtr->bmih.biWidth;
    h = bmpPtr->bmih.biHeight;
    wordsPerRow = (w * bmpPtr->bmih.biBitCount + 31) / 32;
    bytesPerRow = wordsPerRow * 4;

    srcBits = Blt_DBuffer_Pointer(dbuffer);
    destPtr = Blt_CreatePicture(w, h);
    if (bmpPtr->bmih.biBitCount == 32) {
	Blt_Pixel *destRowPtr;
	int y;
	unsigned char *srcRowPtr;
	
	destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * h);
	srcRowPtr = srcBits;
	for (y = 0; y < h; y++) {
	    Blt_Pixel *dp, *dend;
	    unsigned char *sp;
	    
	    destRowPtr -= destPtr->pixelsPerRow;
	    sp = srcRowPtr;
	    for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
		unsigned int pixel;
		
		pixel = BmpGetLong(sp);
		dp->Red   = ((pixel & rMask) >> rShift) << rAdjust;
		dp->Green = ((pixel & gMask) >> gShift) << gAdjust;
		dp->Blue  = ((pixel & bMask) >> bShift) << bAdjust;
		dp->Alpha = ((pixel & aMask) >> aShift) << aAdjust;
		sp += 4;
	    }
	    srcRowPtr += bytesPerRow;
	}
    } else if (bmpPtr->bmih.biBitCount == 16) {
	Blt_Pixel *destRowPtr;
	int y;
	unsigned char *srcRowPtr;
	
	destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * h);
	srcRowPtr = srcBits;
	for (y = 0; y < h; y++) {
	    Blt_Pixel *dp, *dend;
	    unsigned char *sp;
	    
	    destRowPtr -= destPtr->pixelsPerRow;
	    sp = srcRowPtr;
	    for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
		unsigned int pixel;
		
		pixel = BmpGetShort(sp);
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
    if (aMask != 0) {
	/* The image may or may not be transparent. Check */
	Blt_ClassifyPicture(destPtr);
    }
    return destPtr;
}

static Blt_Picture
BmpRleImageData(Blt_DBuffer dbuffer, Bmp *bmpPtr)
{
    Picture *destPtr;
    unsigned char *srcBits, *sp;
    int w, h;

    w = bmpPtr->bmih.biWidth;
    h = bmpPtr->bmih.biHeight;

    srcBits = Blt_DBuffer_Pointer(dbuffer);
    destPtr = Blt_CreatePicture(w, h);
    Blt_BlankPicture(destPtr, bmpPtr->colorTable[0].u32);
    sp = srcBits;
    if (bmpPtr->bmih.biBitCount == 8) {
	int x, y;

	x = 0, y = h - 1;
	for (;;) {
	    unsigned int index, count;
	    
	    count = sp[0];
	    index = sp[1];
	    sp += 2;
	    if (count == 0) {
		switch (index) {
		case 0:			/* End-of-line */
		    x = 0;
		    y--;
		    if (y < 0) {
			goto done;
		    }
		    break;

		case 1:			/* End-of-bitmap */
		    goto done;
		    
		case 2:			/* Delta */
		    x += sp[2];
		    y -= sp[3];
		    if (y < 0) {
			goto done;
		    }
		    sp += 2;
		    break;

		default:	     /* Absolute mode. index is # of bytes. */
		    {
			int i;
			Blt_Pixel *dp;

			count = index;

			if ((x + count) > w) {
			    Blt_FreePicture(destPtr);
			    BmpError("invalid image data: abs run of %d pixels will overrun row (%d,%d) %d index=%d", count, x, y, w, index);
			}
			/* The run is always padded to an even number of bytes
			 * (16-bit boundary). This loop relies on the fact
			 * that picture data is also padded. */
			dp = Blt_PicturePixel(destPtr, x, y);
			for (i = 0; i < count; i += 2) {
			    dp->u32 = bmpPtr->colorTable[sp[0]].u32;
			    dp++;
			    dp->u32 = bmpPtr->colorTable[sp[1]].u32;
			    dp++, sp += 2;
			}
			x += count;
		    }
		    break;
		} 
	    } else {
		int i;
		Blt_Pixel *dp;

		dp = Blt_PicturePixel(destPtr, x, y);
		for (i = 0; (x < w) && (i < count); i++, x++) {
		    dp->u32 = bmpPtr->colorTable[index].u32;
		    dp++;
		}
	    }
	}
    } else if (bmpPtr->bmih.biBitCount == 4) {
	int x, y;

	x = 0, y = h - 1;
	for (;;) {
	    unsigned int index, count;

	    count = sp[0];
	    index = sp[1];
	    sp += 2;
	    if (count == 0) {
		switch (index) {
		case 0:			/* End-of-line */
		    x = 0;
		    y--;
		    if (y < 0) {
			goto done;
		    }
		    break;

		case 1:			/* End-of-bitmap */
		    goto done;
		    
		case 2:			/* Delta */
		    x += sp[2];
		    y -= sp[3];
		    if (y < 0) {
			goto done;
		    }
		    sp += 2;
		    break;

		default:	       /* Absolute mode. index is # pixels. */
		    {
			int i;
			Blt_Pixel *dp;
			unsigned char *send;

			count = index;
			dp = Blt_PicturePixel(destPtr, x, y);
			/* The run may be padded up to 12 bits. */
			send = sp + ((count + 3) / 4) * 2;

			if ((x + count) > w) {
			    Blt_FreePicture(destPtr);
			    BmpError("invalid image data: abs run of %d pixels will overrun row (%d,%d) %d index=%d", count, x, y, w, index);
			}

			for (i = 0; i < count; i += 2) {
			    int i1, i2;
			    i1 = (sp[0] >> 4) & 0x0F;
			    i2 = sp[0] & 0x0F;
			    sp++;
			    dp->u32 = bmpPtr->colorTable[i1].u32;
			    dp++;
			    dp->u32 = bmpPtr->colorTable[i2].u32;
			    dp++;
			}
			x += count;
			sp = send;
		    }
		    break;
		} 
	    } else {
		/* Encoded mode */
		int i;
		Blt_Pixel *dp;
		unsigned int c1, c2;

		c1 = bmpPtr->colorTable[(index >> 4) & 0x0F].u32;
		c2 =  bmpPtr->colorTable[index & 0x0F].u32;
		dp = Blt_PicturePixel(destPtr, x, y);
		for (i = 0; (x < w) && (i < count); i++, x++) {
		    dp->u32 = (i & 0x1) ? c2 : c1;
		    dp++;
		}
	    }
	}
    }
 done:
    return destPtr;
}


static Blt_Picture
BmpImageData(Blt_DBuffer dbuffer, Bmp *bmpPtr)
{
    Blt_Picture picture;

    picture = NULL;
    Blt_DBuffer_SetCursor(dbuffer, bmpPtr->bmfh.bfOffBits);
    if (Blt_DBuffer_BytesLeft(dbuffer) < (bmpPtr->bmih.biSizeImage)) {
	BmpError("short file: not enough bytes for image data");
    }
    switch(bmpPtr->bmih.biCompression) {
    case BI_RGB:
	picture = BmpRgbImageData(dbuffer, bmpPtr);
	break;
    case BI_BITFIELDS:
	picture = BmpBitfieldImageData(dbuffer, bmpPtr);
	break;
    case BI_RLE4:
    case BI_RLE8:
	picture = BmpRleImageData(dbuffer, bmpPtr);
	break;
    case BI_PNG:
    case BI_JPEG:
	break;
    }
    return picture;
}

/*
 *---------------------------------------------------------------------------
 *
 * BmpToPicture --
 *
 *      Reads a BMP file, converts it into a picture, and appends it
 *	to the list of images.  We only handle only single BMP images.
 *
 * Results:
 *      The picture is returned.  If an error occured, such
 *	as the designated file could not be opened, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain 
BmpToPicture(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer,
	     BmpImportSwitches *switchesPtr)
{
    Blt_Chain chain;
    Blt_Picture picture;
    Bmp bmp;
    BmpMessage message;

    bmpMessagePtr = &message;
    memset(&bmp, 0, sizeof(bmp));	/* Clear the structure. */
    message.numWarnings = 0;
    bmp.name = fileName;

    Tcl_DStringInit(&message.errors);
    Tcl_DStringInit(&message.warnings);
    Tcl_DStringAppend(&message.errors, "error reading \"", -1);
    Tcl_DStringAppend(&message.errors, fileName, -1);
    Tcl_DStringAppend(&message.errors, "\": ", -1);

    Tcl_DStringAppend(&message.warnings, "\"", -1);
    Tcl_DStringAppend(&message.warnings, fileName, -1);
    Tcl_DStringAppend(&message.warnings, "\": ", -1);

    if (setjmp(message.jmpbuf)) {
	Tcl_DStringResult(interp, &message.errors);
	Tcl_DStringFree(&message.warnings);
	return NULL;
    }
    chain = NULL;
    if (!BmpHeader(dbuffer, &bmp)) {
	BmpError("bad BMP header");
    }
    if (!BmpHeaderInfo(dbuffer, &bmp)) {
	BmpError("bad BMP logical screen descriptor");
    }
    if (!BmpPalette(dbuffer, &bmp)) {
	BmpError("bad BMP color table");
    }
    picture = BmpImageData(dbuffer, &bmp);
    if (message.numWarnings > 0) {
	Tcl_SetErrorCode(interp, "PICTURE", "BMP_READ_WARNINGS", 
		Tcl_DStringValue(&message.warnings), (char *)NULL);
    } else {
	Tcl_SetErrorCode(interp, "NONE", (char *)NULL);
    }
    Tcl_DStringFree(&message.warnings);
    Tcl_DStringFree(&message.errors);
    if (picture != NULL) {
	chain = Blt_Chain_Create();
	Blt_Chain_Append(chain, picture);
    }
    return chain;
}

/*
 *---------------------------------------------------------------------------
 *
 * PictureToBmp --
 *
 *      Reads an BMP file and converts it into a picture.
 *
 * Results:
 *      The picture is returned.  If an error occured, such as the designated
 *      file could not be opened, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
PictureToBmp(Tcl_Interp *interp, Blt_Picture original, Blt_DBuffer dbuffer,
	     BmpExportSwitches *switchesPtr)
{
    int numColors;
    int format;
    int bitsPerPixel;
    Picture *srcPtr;
    Blt_HashTable colorTable;
    unsigned int wordsPerRow, bytesPerRow;
    unsigned int imageSize, fileSize, infoHeaderSize, offsetToData;
    unsigned char *bp, *destBits;

    srcPtr = original;
    if (switchesPtr->flags & EXPORT_ALPHA) {
	bitsPerPixel = 32;
	format = BI_BITFIELDS;
	numColors = 0;
	infoHeaderSize = SIZEOF_BITMAPV4HEADER;
    } else {
	if (!Blt_PictureIsOpaque(srcPtr)) {	
	    Blt_Picture background;

	    /* Blend picture with solid color background. */
	    background = Blt_CreatePicture(srcPtr->width, srcPtr->height);
	    Blt_BlankPicture(background, switchesPtr->bg.u32); 
	    Blt_BlendRegion(background, srcPtr, 0, 0, srcPtr->width, 
		srcPtr->height, 0, 0);
	    if (srcPtr != original) {
		Blt_FreePicture(srcPtr);
	    }
	    srcPtr = background;
	}
	numColors = Blt_QueryColors(srcPtr, (Blt_HashTable *)NULL);
	format = BI_RGB;
	infoHeaderSize = SIZEOF_BITMAPV3HEADER;
	if (numColors <= 256) {
	    bitsPerPixel = 8;
	    Blt_InitHashTable(&colorTable, BLT_ONE_WORD_KEYS);
	    numColors = Blt_QueryColors(srcPtr, &colorTable);
	} else {
	    bitsPerPixel = 24;
	    numColors = 0;
	}
    }
    wordsPerRow = (srcPtr->width * bitsPerPixel + 31) / 32;
    bytesPerRow = wordsPerRow * 4;
    imageSize = bytesPerRow * srcPtr->height;

    /*
     * Compute the size of the structure.
     *
     * header + infoheader + colortable + imagedata
     *
     * 14 + 108 + 0-256 * 4  + bytesPerRow * height;
     */
    fileSize = imageSize + (numColors * 4) + infoHeaderSize + 14;
    offsetToData = 14 + infoHeaderSize + (numColors * 4);

    bp = Blt_DBuffer_Extend(dbuffer, fileSize);
    memset(bp, 0, fileSize);

    /* File header. */
    bp[0] = 'B', bp[1] = 'M';
    BmpSetLong(bp + OFF_FILE_SIZE,	   fileSize); 
    BmpSetLong(bp + OFF_OFFBITS,	   offsetToData); 

    /* Image header. */
    BmpSetLong (bp + OFF_SIZE,		   infoHeaderSize);
    BmpSetLong (bp + OFF_WIDTH,            srcPtr->width);
    BmpSetLong (bp + OFF_HEIGHT,	   srcPtr->height);
    BmpSetShort(bp + OFF_PLANES,	   1);	
    BmpSetShort(bp + OFF_BIT_COUNT,	   bitsPerPixel);	
    BmpSetLong (bp + OFF_COMPRESSION,	   format); 
    BmpSetLong (bp + OFF_SIZE_IMAGE,       bytesPerRow * srcPtr->height);
    BmpSetLong (bp + OFF_X_PELS_PER_METER, 0);	
    BmpSetLong (bp + OFF_Y_PELS_PER_METER, 0);	
    BmpSetLong (bp + OFF_CLR_USED,	   numColors);		
    BmpSetLong (bp + OFF_CLR_IMPORTANT,    0);	

    if (bitsPerPixel == 32) {
	BmpSetLong(bp + OFF_RED_MASK,   0x00FF0000);  /* biRedMask */
	BmpSetLong(bp + OFF_GREEN_MASK, 0x0000FF00);  /* biGreenMask */
	BmpSetLong(bp + OFF_BLUE_MASK,  0x000000FF);  /* biBlueMask */
	BmpSetLong(bp + OFF_ALPHA_MASK, 0xFF000000);  /* biAlphaMask */
    } 

    /* Color table. */
    if (bitsPerPixel == 8) {
	unsigned long i;
	Blt_HashEntry *hPtr;
	Blt_HashSearch cursor;

	i = 0;
	bp += 14 + infoHeaderSize;
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
		    BmpSetLong(dp, sp->u32);
		    dp += 4;
		}
		destRowPtr += bytesPerRow;
		srcRowPtr  -= srcPtr->pixelsPerRow;
	    }
	    assert((destRowPtr - Blt_DBuffer_Pointer(dbuffer)) == fileSize);
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
	    assert((destRowPtr - Blt_DBuffer_Pointer(dbuffer)) == fileSize);
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
	    assert((destRowPtr - Blt_DBuffer_Pointer(dbuffer)) == fileSize);
	}
	break;
    }
    if (bitsPerPixel == 8) {
	Blt_DeleteHashTable(&colorTable);
    }
    if (srcPtr != original) {
	Blt_FreePicture(srcPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsBmp --
 *
 *      Attempts to parse a BMP file header.
 *
 * Results:
 *      Returns 1 is the header is BMP and 0 otherwise.  Note that
 *      the validity of the header contents is not checked here.  That's
 *	done in BmpToPicture.
 *
 *---------------------------------------------------------------------------
 */
static int
IsBmp(Blt_DBuffer dbuffer)
{
    Bmp bmp;

    return BmpHeader(dbuffer, &bmp);
}

static Blt_Chain
ReadBmp(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer)
{
    BmpImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    return BmpToPicture(interp, fileName, dbuffer, &switches);
}

static Tcl_Obj *
WriteBmp(Tcl_Interp *interp, Blt_Picture picture)
{
    Blt_DBuffer dbuffer;
    BmpExportSwitches switches;
    Tcl_Obj *objPtr;

    /* Default export switch settings. */
    memset(&switches, 0, sizeof(switches));
    switches.bg.u32 = 0xFFFFFFFF; /* white */

    dbuffer = Blt_DBuffer_Create();
    objPtr = NULL;
    if (PictureToBmp(interp, picture, dbuffer, &switches) == TCL_OK) {
	objPtr = Blt_DBuffer_Base64EncodeToObj(interp, dbuffer);
    }
    return objPtr;
}

static Blt_Chain
ImportBmp(
    Tcl_Interp *interp, 
    int objc,
    Tcl_Obj *const *objv,
    const char **fileNamePtr)
{
    Blt_DBuffer dbuffer;
    Blt_Chain chain;
    const char *string;
    BmpImportSwitches switches;

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
    chain = BmpToPicture(interp, string, dbuffer, &switches);
    if (chain == NULL) {
	return NULL;
    }
 error:
    Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
    Blt_DBuffer_Destroy(dbuffer);
    return chain;
}

static int
ExportBmp(Tcl_Interp *interp, unsigned int index, Blt_Chain chain, 
	  int objc, Tcl_Obj *const *objv)
{
    Blt_DBuffer dbuffer;
    Blt_Picture picture;
    BmpExportSwitches switches;
    int result;

    memset(&switches, 0, sizeof(switches));
    switches.bg.u32 = 0xFFFFFFFF;	/* Default bgcolor is white. */
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
    result = PictureToBmp(interp, picture, dbuffer, &switches);
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
	objPtr = Blt_DBuffer_Base64EncodeToObj(interp, dbuffer);
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
Blt_PictureBmpInit(Tcl_Interp *interp)
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
	return TCL_ERROR;
    };
#else
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
	return TCL_ERROR;
    }
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
    if (Tcl_PkgProvide(interp, "blt_picture_bmp", BLT_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }
    return Blt_PictureRegisterFormat(interp, 
	"bmp",			/* Name of format. */
	IsBmp,			/* Discovery routine. */
	ReadBmp,		/* Read format procedure. */
	WriteBmp,		/* Write format procedure. */
	ImportBmp,		/* Import format procedure. */
	ExportBmp);		/* Export format switches. */
}

int 
Blt_PictureBmpSafeInit(Tcl_Interp *interp) 
{
    return Blt_PictureBmpInit(interp);
}

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictPng.c --
 *
 * This module implements PNG file format conversion routines for the
 * picture image type in the BLT toolkit.
 *
 *	Copyright 2003-2005 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use, copy,
 *	modify, merge, publish, distribute, sublicense, and/or sell copies
 *	of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
 *
 */

#include "bltInt.h"

#include "config.h"
#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */
#ifdef HAVE_LIBPNG
#include <tcl.h>
#include <bltSwitch.h>
#include "bltPicture.h"
#include "bltPictFmts.h"

#define TRUE 	1
#define FALSE 	0

typedef struct _Blt_Picture Picture;

#ifdef WIN32
#if SIZEOF_VOID_P == 8
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64
#else 
#define _LARGEFILE64_SOURCE 0
#define _FILE_OFFSET_BITS 32
#endif /* SIZEOF_VOID_P == 8 */
#endif /*WIN32*/
#include <zlib.h>
#include <png.h>

#ifndef png_jmpbuf
#define png_jmpbuf(p)	(p)->jmpbuf
#endif

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    char **comments;		/* Comment pairs */
    int index;
} PngExportSwitches;

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
} PngImportSwitches;

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data", "data", (char *)NULL,
	Blt_Offset(PngExportSwitches, dataObjPtr), 0},
    {BLT_SWITCH_OBJ, "-file", "fileName", (char *)NULL,
	Blt_Offset(PngExportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_LIST, "-comments", "{key string...}", (char *)NULL,
	Blt_Offset(PngExportSwitches, comments), 0},
    {BLT_SWITCH_INT_NNEG, "-index", "int", (char *)NULL,
	Blt_Offset(PngExportSwitches, index), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data", "data", (char *)NULL,
	Blt_Offset(PngImportSwitches, dataObjPtr), 0},
    {BLT_SWITCH_OBJ, "-file", "fileName", (char *)NULL,
	Blt_Offset(PngImportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_END}
};

typedef struct {
    Tcl_DString errors;
    Tcl_DString warnings;
    int numErrors, numWarnings;
} PngMessage;


DLLEXPORT extern Tcl_AppInitProc Blt_PicturePngInit;
DLLEXPORT extern Tcl_AppInitProc Blt_PicturePngSafeInit;

static void
PngError(png_struct *pngPtr, const char *mesg)
{
    PngMessage *mesgPtr;

    mesgPtr = png_get_error_ptr(pngPtr);
    Tcl_DStringAppend(&mesgPtr->errors, mesg, -1);
    mesgPtr->numErrors++;
}

static void
PngWarning(png_struct *pngPtr, const char *mesg)
{
    PngMessage *mesgPtr;

    mesgPtr = png_get_error_ptr(pngPtr);
    Tcl_DStringAppend(&mesgPtr->warnings, mesg, -1);
    mesgPtr->numWarnings++;
}

static void
PngReadFromBuffer(png_structp png, png_byte *out, png_size_t numWanted)
{
    Blt_DBuffer dbuffer;
    
    dbuffer = png_get_io_ptr(png);
    if (Blt_DBuffer_BytesLeft(dbuffer) < numWanted) {
	numWanted = Blt_DBuffer_BytesLeft(dbuffer);
    }
    if (numWanted > 0) {
	memcpy(out, Blt_DBuffer_Pointer(dbuffer), numWanted);
	Blt_DBuffer_IncrCursor(dbuffer, numWanted);
    }
}

static void
PngWriteToBuffer(png_struct *pngPtr, png_byte *out, png_size_t numBytes)
{
    Blt_DBuffer dbuffer;

    dbuffer = png_get_io_ptr(pngPtr);
    if (!Blt_DBuffer_AppendData(dbuffer, out, numBytes)) {
	return;
    }
}

static void
PngFlush(png_struct *pngPtr)
{
    /* Do nothing */
}

#ifdef PNG_TEXT_SUPPORTED
static void
PngAddText(Tcl_Interp *interp, png_struct *pngPtr, png_info *infoPtr, 
	   char **comments)
{
    png_text *text, *textPtr;
    char **p, **pend;
    int count;

    count = 0;
    for (p = comments; *p != NULL; p++) {
	count++;
    }
    if (count == 0) {
	return;
    }
    text = Blt_AssertMalloc(sizeof(png_text) * count);
    textPtr = text;
    for (p = comments, pend = p + count; p < pend; p += 2) {
	textPtr->compression = PNG_TEXT_COMPRESSION_NONE;
	textPtr->key = *p;
	if (*(p+1) == NULL) {
	    continue;
	}
	textPtr->text = *(p+1);
	textPtr++;
    }
    if (count > 0) {
	png_set_text(pngPtr, infoPtr, text, textPtr - text);
    }
    Blt_Free(text);
}
#endif	/* PNG_TEXT_SUPPORTED */

/*
 *---------------------------------------------------------------------------
 *
 * IsPng --
 *
 *      Attempts to parse a PNG file header.
 *
 * Results:
 *      Returns 1 is the header is PNG and 0 otherwise.  Note that only the
 *      signature is compared and the validity of the image contents is not
 *      checked here.  That's done in PngToPicture.
 *
 *---------------------------------------------------------------------------
 */
static int 
IsPng(Blt_DBuffer dbuffer)
{
#define PNG_BYTES_TO_CHECK 4
    if (Blt_DBuffer_Length(dbuffer) > PNG_BYTES_TO_CHECK) {
	unsigned char *bp;
	int bool;

	bp = Blt_DBuffer_Pointer(dbuffer);
	bool = (png_sig_cmp(bp, (png_size_t)0, PNG_BYTES_TO_CHECK) == 0);
	return bool;
    }
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * PngToPicture --
 *
 *      Reads a PNG file and converts it into a picture.
 *
 * Results:
 *      The picture is returned.  If an error occured, such as the
 *      designated file could not be opened, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain
PngToPicture(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer,
	     PngImportSwitches *switchesPtr)
{
    Picture *destPtr;
    PngMessage message;
    int transform;
    png_infop info;
    png_structp png;
    unsigned int colorType;
    unsigned int numChannels;
    unsigned int width, height;
    
    Tcl_DStringInit(&message.errors);
    Tcl_DStringInit(&message.warnings);
    message.numErrors = message.numWarnings = 0;

    Tcl_DStringAppend(&message.errors, "error reading \"", -1);
    Tcl_DStringAppend(&message.errors, fileName, -1);
    Tcl_DStringAppend(&message.errors, "\": ", -1);
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, &message, PngError, 
	PngWarning);
    if (png == NULL) {
	return NULL;
    }

    destPtr = NULL;		/* Mark to indicate failure. */

    info = png_create_info_struct(png);
    if (info == NULL) {
	goto bad;
    }
    if (setjmp(png_jmpbuf(png))) {
	goto bad;
    }
    png_set_read_fn(png, dbuffer, PngReadFromBuffer);

    transform = (PNG_TRANSFORM_EXPAND | /* Expand 1,2 and 4 bit to 8 bits. */
		 PNG_TRANSFORM_STRIP_16); /* Convert 16-bit components to 8. */

    png_read_png(png, info, transform, (void *)NULL);

#ifdef notdef
    bitDepth =  png_get_bit_depth(png, info);
    filter =    png_get_filter_type(png, info);
    interlace = png_get_interlace_type(png, info);
#endif
    colorType = png_get_color_type(png, info);
    height =    png_get_image_height(png, info);
    numChannels = png_get_channels(png, info);
    width =     png_get_image_width(png, info);
    
#ifdef notdef
    fprintf(stderr, "%s: %dx%d, bit_depth=%d, channels=%d, interlace=%d\n",
	    fileName, width, height, bitDepth, numChannels, interlace);
    fprintf(stderr, "colortype= %s %s %s \n",
	    (colorType & 1) ? "palette" : "",
	    (colorType & 2) ? "color"   : "",
	    (colorType & 4) ? "alpha"   : "");
#endif

    destPtr = Blt_CreatePicture(width, height);
    if (colorType & PNG_COLOR_MASK_ALPHA) {
	assert((numChannels == 4) || (numChannels == 2));
	destPtr->flags |= BLT_PIC_BLEND;
    }
    if ((numChannels == 4) || (numChannels == 3)) {
	destPtr->flags |= BLT_PIC_COLOR;
    }
    /*  Set associated colors flag.  Fixed, if necessary, below. */
    destPtr->flags |= BLT_PIC_ASSOCIATED_COLORS;

    {
	unsigned int y;
	Blt_Pixel *destRowPtr;
	png_byte **row_pointers;

	destRowPtr = destPtr->bits;
	row_pointers = png_get_rows(png, info);
	switch (numChannels) {
	case 4:			/* 1 red, 1 green, 1 blue, 1 alpha */
	    for (y = 0; y < height; y++) {
		Blt_Pixel *dp;
		png_byte *sp;
		unsigned int x;
		
		dp = destRowPtr;
		sp = row_pointers[y];
		for (x = 0; x < width; x++) {
		    dp->Red = sp[0];
		    dp->Green = sp[1];
		    dp->Blue = sp[2];
		    dp->Alpha = sp[3];
		    dp++, sp += 4;
		}
		destRowPtr += destPtr->pixelsPerRow;
	    }
	    break;
	case 3:			/* 1 red, 1 green, 1 blue */
	    for (y = 0; y < height; y++) {
		Blt_Pixel *dp;
		unsigned char *sp;
		unsigned int x;
		
		dp = destRowPtr;
		sp = row_pointers[y];
		for (x = 0; x < width; x++) {
		    dp->Red = sp[0];
		    dp->Green = sp[1];
		    dp->Blue = sp[2];
		    dp->Alpha = ALPHA_OPAQUE;
		    dp++, sp += 3;
		}
		destRowPtr += destPtr->pixelsPerRow;
	    }
	    break;
	case 2:			/* 1 greyscale, 1 alpha */
	    for (y = 0; y < height; y++) {
		Blt_Pixel *dp;
		unsigned char *sp;
		unsigned int x;
		
		dp = destRowPtr;
		sp = row_pointers[y];
		for (x = 0; x < width; x++) {
		    dp->Red = dp->Green = dp->Blue = sp[0];
		    dp->Alpha = sp[1];
		    dp++, sp += 2;
		}
		destRowPtr += destPtr->pixelsPerRow;
	    }
	    break;
	case 1:			/* 1 greyscale */
	    for (y = 0; y < height; y++) {
		Blt_Pixel *dp;
		unsigned char *sp;
		unsigned int x;
		
		dp = destRowPtr;
		sp = row_pointers[y];
		for (x = 0; x < width; x++) {
		    dp->Red = dp->Green = dp->Blue = *sp++;
		    dp->Alpha = ALPHA_OPAQUE;
		    dp++;
		}
		destRowPtr += destPtr->pixelsPerRow;
	    }
	    break;
	}
    }
    if (colorType & PNG_COLOR_MASK_ALPHA) {
	Blt_AssociateColors(destPtr);
    }

 bad:
    png_destroy_read_struct(&png, &info, (png_infop *)NULL);
    if (message.numWarnings > 0) {
	Tcl_SetErrorCode(interp, "PICTURE", "PNG_READ_WARNINGS", 
		Tcl_DStringValue(&message.errors), (char *)NULL);
    } else {
	Tcl_SetErrorCode(interp, "NONE", (char *)NULL);
    }
    Tcl_DStringFree(&message.warnings);
    if (message.numErrors > 0) {
	Tcl_DStringResult(interp, &message.errors);
    }
    Tcl_DStringFree(&message.errors);
    if (destPtr != NULL) {
	Blt_Chain chain;

	chain = Blt_Chain_Create();
	Blt_Chain_Append(chain, destPtr);
	return chain;
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * PictureToPng --
 *
 *      Writes a PNG format image to the provided data buffer.
 *
 * Results:
 *      A standard TCL result.  If an error occured, TCL_ERROR is returned
 *      and an error message will be place in the interpreter
 *      result. Otherwise, the data buffer will contain the binary output
 *      of the image.
 *
 * Side Effects:
 *	Memory is allocated for the data buffer.
 *
 *---------------------------------------------------------------------------
 */
static int
PictureToPng(Tcl_Interp *interp, Blt_Picture picture, Blt_DBuffer dbuffer,
	     PngExportSwitches *switchesPtr)
{
    PngMessage message;
    png_infop info;
    png_structp png;
    unsigned int colorType, numChannels, bitsPerPixel;

    Tcl_DStringInit(&message.errors);
    Tcl_DStringInit(&message.warnings);
    message.numErrors = message.numWarnings = 0;

    Tcl_DStringAppend(&message.errors, "error writing PNG output: ", -1);
    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, &message, PngError, 
	PngWarning);
    if (png == NULL) {
	return TCL_ERROR;
    }
    info = png_create_info_struct(png);
    if (info == NULL) {
	png_destroy_write_struct(&png, (png_infop *)NULL);
	return TCL_ERROR;
    }
    if (setjmp(png_jmpbuf(png))) {
	goto bad;
    }
    png_set_write_fn(png, (void *)dbuffer, PngWriteToBuffer, PngFlush);

    png_set_compression_level(png, Z_BEST_COMPRESSION);
    Blt_QueryColors(picture, (Blt_HashTable *)NULL);
    if (Blt_Picture_IsColor(picture)) {
	numChannels = 3;
	colorType = PNG_COLOR_TYPE_RGB;
    } else {
	numChannels = 1;
	colorType = PNG_COLOR_TYPE_GRAY;
    }
    if (!Blt_Picture_IsOpaque(picture)) {
	numChannels++;
	colorType |= PNG_COLOR_MASK_ALPHA;
    }
    bitsPerPixel = 8;

    png_set_IHDR(png, info, 
	Blt_Picture_Width(picture),    /* Width */
	Blt_Picture_Height(picture),   /* Height */
	bitsPerPixel, 		      /* Bits per pixel. */
	colorType, 		      /* Color type: RGB or GRAY */
	PNG_INTERLACE_NONE, 	      /* Interlace */
	PNG_COMPRESSION_TYPE_DEFAULT, /* Compression */
	PNG_FILTER_TYPE_DEFAULT);     /* Filter */

#ifdef PNG_TEXT_SUPPORTED
    if (switchesPtr->comments != NULL) {
	PngAddText(interp, png, info, switchesPtr->comments);
    }
#endif
    png_write_info(png, info);
    png_set_packing(png);

    {
	Picture *srcPtr;
	Blt_Pixel *srcRowPtr;
	int bytesPerRow;
	int y;
	unsigned char **rowArray;
	unsigned char *destBuffer, *destRowPtr;
	
	srcPtr = picture;
	bytesPerRow = numChannels * srcPtr->width;
	destBuffer = Blt_Malloc(bytesPerRow * srcPtr->height);
	if (destBuffer == NULL) {
	    goto bad;
	}
	rowArray = Blt_Malloc(sizeof(unsigned char *) * srcPtr->height);
	if (rowArray == NULL) {
	    Blt_Free(destBuffer);
	    goto bad;
	}
	destRowPtr = destBuffer;
	srcRowPtr = srcPtr->bits;
	switch (numChannels) {
	case 4:			/* RGBA */
	    for (y = 0; y < srcPtr->height; y++) {
		Blt_Pixel *sp, *send;
		unsigned char *dp;
		
		dp = destRowPtr;
		for (sp = srcRowPtr, send = sp + srcPtr->width; sp<send; sp++) {
		    dp[0] = sp->Red;
		    dp[1] = sp->Green;
		    dp[2] = sp->Blue;
		    dp[3] = sp->Alpha;
		    dp += 4;
		}
		rowArray[y] = destRowPtr;
		destRowPtr += bytesPerRow;
		srcRowPtr += srcPtr->pixelsPerRow;
	    }
	    break;
	    
        case 3:                         /* RGB, 100% opaque image.  */
	    for (y = 0; y < srcPtr->height; y++) {
		Blt_Pixel *sp, *send;
		unsigned char *dp;
		
		dp = destRowPtr;
		for (sp = srcRowPtr, send = sp + srcPtr->width; sp<send; sp++) {
		    dp[0] = sp->Red;
		    dp[1] = sp->Green;
		    dp[2] = sp->Blue;
		    dp += 3;
		}
		rowArray[y] = destRowPtr;
		destRowPtr += bytesPerRow;
		srcRowPtr += srcPtr->pixelsPerRow;
	    }
	    break;
	    
	case 2:                         /* Greyscale with alpha
                                         * component.  */
             for (y = 0; y < srcPtr->height; y++) {
                 Blt_Pixel *sp, *send;
                 unsigned char *dp;

                 dp = destRowPtr;
                 for (sp = srcRowPtr, send = sp + srcPtr->width; sp<send; sp++) {
                     dp[0] = sp->Red;
                     dp[1] = sp->Alpha;
                     dp += 2;
                 }
                 rowArray[y] = destRowPtr;
                 destRowPtr += bytesPerRow;
                 srcRowPtr += srcPtr->pixelsPerRow;
             }
             break;

         case 1:			/* Greyscale, 100% opaque
                                         * image.  */
             for (y = 0; y < srcPtr->height; y++) {
                 Blt_Pixel *sp, *send;
                 unsigned char *dp;

                 dp = destRowPtr;
                 for (sp = srcRowPtr, send = sp + srcPtr->width; sp<send; sp++) {
                     *dp++ = sp->Red;
                 }
                 rowArray[y] = destRowPtr;
                 destRowPtr += bytesPerRow;
                 srcRowPtr += srcPtr->pixelsPerRow;
             }
             break;
         }
         png_write_image(png, rowArray);
         png_write_end(png, NULL);

         Blt_Free(destBuffer);
         Blt_Free(rowArray);
     }
  bad:
     png_destroy_write_struct(&png, &info);
     if (message.numWarnings > 0) {
         Tcl_SetErrorCode(interp, "PICTURE", "PNG_WRITE_WARNINGS", 
                 Tcl_DStringValue(&message.errors), (char *)NULL);
     } else {
         Tcl_SetErrorCode(interp, "NONE", (char *)NULL);
     }
     Tcl_DStringFree(&message.warnings);
     if (message.numErrors > 0) {
         Tcl_DStringResult(interp, &message.errors);
         return TCL_ERROR;
     } 
     Tcl_DStringFree(&message.errors);
     return TCL_OK;
 }

 static Blt_Chain
 ReadPng(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer)
 {
     PngImportSwitches switches;

     memset(&switches, 0, sizeof(switches));
     return PngToPicture(interp, fileName, dbuffer, &switches);
 }

 static Tcl_Obj *
 WritePng(Tcl_Interp *interp, Blt_Picture picture)
 {
     Blt_DBuffer dbuffer;
     PngExportSwitches switches;
     Tcl_Obj *objPtr;

     /* Default export switch settings. */
     memset(&switches, 0, sizeof(switches));

     objPtr = NULL;
     dbuffer = Blt_DBuffer_Create();
     if (PictureToPng(interp, picture, dbuffer, &switches) == TCL_OK) {
         objPtr = Blt_DBuffer_Base64EncodeToObj(interp, dbuffer);
     }
     Blt_DBuffer_Destroy(dbuffer);
     return objPtr;
 }

 static Blt_Chain
 ImportPng(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, 
           const char **fileNamePtr)
 {
     Blt_Chain chain;
     Blt_DBuffer dbuffer;
     PngImportSwitches switches;
     const char *string;

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
     chain = PngToPicture(interp, string, dbuffer, &switches);
  error:
     Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
     Blt_DBuffer_Destroy(dbuffer);
     return chain;
 }

 static int
 ExportPng(Tcl_Interp *interp, unsigned int index, Blt_Chain chain, int objc, 
           Tcl_Obj *const *objv)
 {
     PngExportSwitches switches;
     Blt_DBuffer dbuffer;
     int result;
     Blt_Picture picture;

     /* Default export switch settings. */
     memset(&switches, 0, sizeof(switches));
     switches.index = index;
     if (Blt_ParseSwitches(interp, exportSwitches, objc - 3, objv + 3, 
         &switches, BLT_SWITCH_DEFAULTS) < 0) {
         Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
         return TCL_ERROR;
     }
     if ((switches.dataObjPtr != NULL) && (switches.fileObjPtr != NULL)) {
         Tcl_AppendResult(interp, "more than one export destination: ",
                 "use only one -file or -data flag.", (char *)NULL);
         Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
         return TCL_ERROR;
     }
     picture = Blt_GetNthPicture(chain, switches.index);
     if (picture == NULL) {
         Tcl_AppendResult(interp, "no picture at index ", 
                 Blt_Itoa(switches.index), (char *)NULL);
         Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
         return TCL_ERROR;
     }
     dbuffer = Blt_DBuffer_Create();
     result = PictureToPng(interp, picture, dbuffer, &switches);
     if (result != TCL_OK) {
         Tcl_AppendResult(interp, "can't convert \"", 
                 Tcl_GetString(objv[2]), "\"", (char *)NULL);
         goto error;
     }

     /* Write the PNG data to file or convert it to a base64 string. */
     if (switches.fileObjPtr != NULL) {
         char *fileName;

         fileName = Tcl_GetString(switches.fileObjPtr);
         result = Blt_DBuffer_SaveFile(interp, fileName, dbuffer);
     } else if (switches.dataObjPtr != NULL) {
         Tcl_Obj *objPtr;

         objPtr = Tcl_ObjSetVar2(interp, switches.dataObjPtr, NULL, 
                 Blt_DBuffer_ByteArrayObj(dbuffer), 0);
         result = (objPtr == NULL) ? TCL_ERROR : TCL_OK;
     } else {
         Tcl_Obj *objPtr;

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
 Blt_PicturePngInit(Tcl_Interp *interp)
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
     if (Tcl_PkgRequire(interp, "blt_tcl", BLT_VERSION, PKG_EXACT) == NULL) {
         return TCL_ERROR;
     }
     if (Tcl_PkgRequire(interp, "blt_tk", BLT_VERSION, PKG_EXACT) == NULL) {
         return TCL_ERROR;
     }
     if (Tcl_PkgProvide(interp, "blt_picture_png", BLT_VERSION) != TCL_OK) {
         return TCL_ERROR;
     }
    return Blt_PictureRegisterFormat(interp,
        "png",			/* Name of format. */
	IsPng,			/* Format discovery procedure. */
	ReadPng,		/* Read format procedure. */
	WritePng,		/* Write format procedure. */
	ImportPng,		/* Import format procedure. */
	ExportPng);		/* Export format procedure. */
}

int 
Blt_PicturePngSafeInit(Tcl_Interp *interp) 
{
    return Blt_PicturePngInit(interp);
}

#endif /* HAVE_LIBPNG */


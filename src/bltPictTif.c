/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictTif.c --
 *
 * This module implements TIF file format conversion routines for the
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
#ifdef HAVE_LIBTIF
#include <tcl.h>
#include "bltPicture.h"
#include "bltPictFmts.h"

#ifdef HAVE_MEMORY_H
  #include <memory.h>
#endif /* HAVE_MEMORY_H */

#if defined(WIN32) || defined(MACOSX)
  #include <setjmp.h>
#endif

typedef struct _Blt_Picture Picture;

#include <tiffio.h>

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    int compress;
    int index;
} TifExportSwitches;

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    Tcl_Obj *varObjPtr;
} TifImportSwitches;

static Blt_SwitchParseProc CompressSwitch;
static Blt_SwitchCustom compressSwitch = {
    CompressSwitch, NULL, NULL, (ClientData)0,
};

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-compress", "type", (char *)NULL,
        Blt_Offset(TifExportSwitches, compress), 0, 0, &compressSwitch},
    {BLT_SWITCH_OBJ, "-data", "varName", (char *)NULL,
        Blt_Offset(TifExportSwitches, dataObjPtr), 0},
    {BLT_SWITCH_OBJ, "-file", "fileName", (char *)NULL,
        Blt_Offset(TifExportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_INT_NNEG, "-index", "int", (char *)NULL,
        Blt_Offset(TifExportSwitches, index), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data", "data", (char *)NULL,
        Blt_Offset(TifImportSwitches, dataObjPtr), 0},
    {BLT_SWITCH_OBJ, "-file", "fileName", (char *)NULL,
        Blt_Offset(TifImportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_OBJ, "-tags", "varName", (char *)NULL,
        Blt_Offset(TifImportSwitches, varObjPtr), 0},
    {BLT_SWITCH_END}
};

DLLEXPORT extern Tcl_AppInitProc Blt_PictureTifInit;
DLLEXPORT extern Tcl_AppInitProc Blt_PictureTifSafeInit;

/*
 *---------------------------------------------------------------------------
 *
 * CompressSwitch --
 *
 *      Convert a Tcl_Obj representing a TIFF compression name.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static const char *compressNames[] =  {
    "lzw",                              /* Lempel-Ziv & Welch */
    "ojpeg",                            /* !6.0 JPEG */
    "jpeg",                             /* %JPEG DCT compression */
    "next",                             /* NeXT 2-bit RLE */
    "packbits",                         /* Macintosh RLE */
    "thunderscan",                      /* ThunderScan RLE */
    "pixarfilm",                        /* Pixar companded 10bit LZW */
    "pixarlog",                         /* Pixar companded 11bit ZIP */
    "deflate",                          /* Deflate compression */
    "adobe_deflate",                    /* Adobe's deflate */
    "dcs",                              /* Kodak DCS encoding */
    "sgilog",                           /* SGI Log Luminance RLE */
    "sgilog24",                         /* SGI Log 24-bit packed */
    NULL
};

static int numCompressNames = sizeof(compressNames) / sizeof(char *);

/*ARGSUSED*/
static int
CompressSwitch(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to return results. */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    int *compressPtr = (int *)(record + offset);
    int i;
    char *string;
    char c;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if (c == '\0') {
        *compressPtr = 0;
        return TCL_OK;
    } 
    for (i = 0; i < numCompressNames; i++) {
        if ((c == compressNames[i][0]) && 
            (strcasecmp(string, compressNames[i]) == 0)) {
            *compressPtr = i + 1;
            return TCL_OK;
        }
    }
    Tcl_AppendResult(interp, "unknown TIFF compression mode \"", string, "\"",
                     (char *) NULL);
    return TCL_ERROR;
}


static tsize_t
TifRead(thandle_t handle, tdata_t out, tsize_t numWanted)
{
    Blt_DBuffer dbuffer = (Blt_DBuffer)handle;

    if (Blt_DBuffer_BytesLeft(dbuffer) < numWanted) {
        numWanted = Blt_DBuffer_BytesLeft(dbuffer);
    }
    if (numWanted > 0) {
        memcpy(out, Blt_DBuffer_Pointer(dbuffer), numWanted);
        Blt_DBuffer_IncrCursor(dbuffer, numWanted);
    }
    return numWanted;
}

static toff_t
TifSeek(thandle_t handle, toff_t offset, int whence)
{
    Blt_DBuffer dbuffer = (Blt_DBuffer)handle;

    if (whence == SEEK_CUR) {
        offset += Blt_DBuffer_Cursor(dbuffer);
    } else if (whence == SEEK_END) {
        offset += Blt_DBuffer_Length(dbuffer);
    }
    if (offset > Blt_DBuffer_Size(dbuffer)) {
        /* Attempting to seek past the end of the current buffer. Resize
         * the buffer */
        Blt_DBuffer_Resize(dbuffer, offset);
    }
    Blt_DBuffer_SetCursor(dbuffer, offset);
    return (toff_t) Blt_DBuffer_Cursor(dbuffer);
}

static toff_t
TifSize(thandle_t handle)
{
    Blt_DBuffer dbuffer = (Blt_DBuffer)handle;

    return Blt_DBuffer_Length(dbuffer);
}

static int
TifMapFile(thandle_t handle, tdata_t *data, toff_t *offsetPtr)
{
    return 0;
}

static void
TifUnmapFile(thandle_t handle, tdata_t data, toff_t offset)
{
}

static int
TifClose(thandle_t handle)
{
    return 0;
}

static tsize_t
TifWrite(thandle_t handle, tdata_t out, tsize_t numBytes)
{
    Blt_DBuffer dbuffer = (Blt_DBuffer)handle;
    unsigned int needed;

    needed = Blt_DBuffer_Cursor(dbuffer) + numBytes;
    if (needed > Blt_DBuffer_Size(dbuffer)) {
        if (!Blt_DBuffer_Resize(dbuffer, needed)) {
            return -1;
        }
    }
    memcpy(Blt_DBuffer_Pointer(dbuffer), out, numBytes);
    Blt_DBuffer_IncrCursor(dbuffer, numBytes);
    if (Blt_DBuffer_Cursor(dbuffer) > Blt_DBuffer_Length(dbuffer)) {
        Blt_DBuffer_SetLength(dbuffer, Blt_DBuffer_Cursor(dbuffer));
    }
    return numBytes;
}

typedef struct {
    Tcl_DString errors;
    Tcl_DString warnings;
    int numWarnings, numErrors;
} TifMessage;

static TifMessage *tifMessagePtr;

static void 
TifError(const char *routine, const char *fmt, va_list ap)
{
    char string[BUFSIZ+4];
    int length;

    length = vsnprintf(string, BUFSIZ, fmt, ap);
    if (length > BUFSIZ) {
        strcat(string, "...");
    }
    Tcl_DStringAppend(&tifMessagePtr->errors, string, -1);
    tifMessagePtr->numErrors++;
}

/* Warnings are not processed in TCL */
static void
TifWarning(const char *routine, const char *fmt, va_list ap)
{
    char string[BUFSIZ+4];
    int length;

    length = vsnprintf(string, BUFSIZ, fmt, ap);
    if (length > BUFSIZ) {
        strcat(string, "...");
    }
    Tcl_DStringAppend(&tifMessagePtr->warnings, string, -1);
    Tcl_DStringAppend(&tifMessagePtr->warnings, "\n", -1);
    tifMessagePtr->numWarnings++;
}

static int
TifReadImage(Tcl_Interp *interp, TIFF *tifPtr, Blt_Chain chain)
{
    int w, h, numPixels;
    uint32 *srcBits, *sp;
    Picture *destPtr;
    Blt_Pixel *destRowPtr;
    int y;

    TIFFGetField(tifPtr, TIFFTAG_IMAGEWIDTH, &w);
    TIFFGetField(tifPtr, TIFFTAG_IMAGELENGTH, &h);
    numPixels = w * h;
    srcBits = _TIFFmalloc(sizeof(uint32) * numPixels);
    if (srcBits == NULL) {
        Tcl_AppendResult(interp, "can't allocate ", Blt_Itoa(numPixels), 
                " buffer for TIF image", (char *)NULL);
        return TCL_ERROR;
    }
    if (!TIFFReadRGBAImage(tifPtr, w, h, srcBits, /*stopOnError*/0)) {
        Tcl_AppendResult(interp, "can't read image in directory", 
                (char *)NULL);
        _TIFFfree(srcBits);
        return TCL_ERROR;
    }   
    destPtr = Blt_CreatePicture(w, h);
    destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * (h - 1));
    sp = srcBits;
    for (y = h - 1; y >= 0; y--) {
        Blt_Pixel *dp, *dend;

        for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
            dp->Red   = TIFFGetR(*sp);
            dp->Green = TIFFGetG(*sp);
            dp->Blue  = TIFFGetB(*sp);
            dp->Alpha = TIFFGetA(*sp);
            sp++;
        }
        destRowPtr -= destPtr->pixelsPerRow;
    }
    _TIFFfree(srcBits);
    Blt_Chain_Append(chain, destPtr);
    destPtr->flags &= ~BLT_PIC_UNINITIALIZED;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsTif --
 *
 *      Attempts to parse a TIF file header.
 *
 * Results:
 *      Returns 1 is the header is TIF and 0 otherwise.  Note that the
 *      validity of the header contents is not checked here.  That's done in
 *      TifToPicture.
 *
 *---------------------------------------------------------------------------
 */
static int
IsTif(Blt_DBuffer dbuffer)
{
    unsigned char *bp;
    int bool;

    Blt_DBuffer_Rewind(dbuffer);
    bp = Blt_DBuffer_Pointer(dbuffer);
    bool = (((bp[0] == 'M') && (bp[1] == 'M')) || 
            ((bp[0] == 'I') && (bp[1] == 'I')));
    return bool;
}

/*
 *---------------------------------------------------------------------------
 *
 * TifToPicture --
 *
 *      Reads a TIFF file and converts it into a picture.
 *
 * Results:
 *      The picture is returned.  If an error occured, such as the
 *      designated file could not be opened, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain
TifToPicture(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer,
             TifImportSwitches *switchesPtr)
{
    Blt_Chain chain;
    TIFF *tifPtr;
    TIFFErrorHandler oldErrorHandler, oldWarningHandler;
    TifMessage message;

    message.numWarnings = message.numErrors = 0;
    Tcl_DStringInit(&message.errors);
    Tcl_DStringInit(&message.warnings);
    Tcl_DStringAppend(&message.errors, "error reading \"", -1);
    Tcl_DStringAppend(&message.errors, fileName, -1);
    Tcl_DStringAppend(&message.errors, "\": ", -1);
    tifMessagePtr = &message;

    oldErrorHandler = TIFFSetErrorHandler(TifError);
    oldWarningHandler = TIFFSetWarningHandler(TifWarning);

    chain = NULL;
    tifPtr = TIFFClientOpen(fileName, "rC", (thandle_t)dbuffer,
        TifRead,                /* TIFFReadProc */
        TifWrite,               /* TIFFWriteProc */
        TifSeek,                /* TIFFSeekProc */
        TifClose,               /* TIFFCloseProc */
        TifSize,                /* TIFFSizeProc */
        TifMapFile,             /* TIFFMapFileProc */
        TifUnmapFile);          /* TIFFUnmapFileProc */
    if (tifPtr == NULL) {
        goto bad;
    }
    if (switchesPtr->varObjPtr != NULL) {
        const char *varName;
        
#ifdef notdef
        TIFFPrintDirectory(tifPtr, stdout, 0);
#endif
        varName = Tcl_GetString(switchesPtr->varObjPtr);
        if (Blt_ParseTifTags(interp, varName, Blt_DBuffer_Bytes(dbuffer), 0,
                             Blt_DBuffer_Length(dbuffer)) == TCL_ERROR) {
            goto bad;
        }
    }
    chain = Blt_Chain_Create();
    do {
        if (TifReadImage(interp, tifPtr, chain) != TCL_OK) {
            goto bad;
        }
    } while (TIFFReadDirectory(tifPtr));

bad:
    if (tifPtr != NULL) {
        TIFFClose(tifPtr);
    }
    TIFFSetErrorHandler(oldErrorHandler);
    TIFFSetWarningHandler(oldWarningHandler);
    if (message.numWarnings > 0) {
        Tcl_SetErrorCode(interp, "PICTURE", "TIF_READ_WARNINGS", 
                Tcl_DStringValue(&message.warnings), (char *)NULL);
    } else {
        Tcl_SetErrorCode(interp, "NONE", (char *)NULL);
    }
    Tcl_DStringFree(&message.warnings);
    if (message.numErrors > 0) {
        Tcl_AppendResult(interp, Tcl_DStringValue(&message.errors), 
                         (char *)NULL); 
    }
    Tcl_DStringFree(&message.errors);
    return chain;
}

static int tifCompressionSchemes[] = {
    COMPRESSION_NONE,

#ifdef LZW_SUPPORT
    COMPRESSION_LZW,            /* Lempel-Ziv & Welch */
#else 
    COMPRESSION_NONE,
#endif

#ifdef OJPEG_SUPPORT
    COMPRESSION_OJPEG,          /* !6.0 JPEG */
#else 
    COMPRESSION_NONE,
#endif /* OJPEG_SUPPORT */

#ifdef JPEG_SUPPORT
    COMPRESSION_JPEG,           /* %JPEG DCT compression */
#else 
    COMPRESSION_NONE,
#endif /* JPEG_SUPPORT */

#ifdef NEXT_SUPPORT
    COMPRESSION_NEXT,           /* NeXT 2-bit RLE */
#else 
    COMPRESSION_NONE,
#endif /* NEXT_SUPPORT */

#ifdef PACKBITS_SUPPORT
    COMPRESSION_PACKBITS,       /* Macintosh RLE */
#else 
    COMPRESSION_NONE,
#endif /* PACKBITS_SUPPORT */

#ifdef THUNDER_SUPPORT
    COMPRESSION_THUNDERSCAN,    /* ThunderScan RLE */
#else 
    COMPRESSION_NONE,
#endif /* THUNDER_SUPPORT */

    COMPRESSION_PIXARFILM,      /* Pixar 10-bit LZW */

#ifdef PIXARLOG_SUPPORT
    COMPRESSION_PIXARLOG,       /* Pixar 11-bit ZIP */
#else 
    COMPRESSION_NONE,
#endif /* PIXARLOG_SUPPORT */

#ifdef ZIP_SUPPORT
    COMPRESSION_DEFLATE,        /* Deflate compression */
#else 
    COMPRESSION_NONE,
#endif /* ZIP_SUPPORT */

#ifdef ADOBE_SUPPORT
    COMPRESSION_ADOBE_DEFLATE,  /* Adobe's deflate */
#else
    COMPRESSION_NONE,
#endif /* ADOBE_SUPPORT */
    COMPRESSION_DCS,            /* Kodak DCS encoding */

#ifdef SGILOG_SUPPORT
    COMPRESSION_SGILOG,         /* SGI Log Luminance RLE */
    COMPRESSION_SGILOG24,       /* SGI Log 24-bit packed */
#else 
    COMPRESSION_NONE,
    COMPRESSION_NONE,
#endif /* SGILOG_SUPPORT */

};

/*
 *---------------------------------------------------------------------------
 *
 * PictureToTif --
 *
 *      Writes a TIFF format image to the provided data buffer.
 *
 * Results:
 *      A standard TCL result.  If an error occured, TCL_ERROR is
 *      returned and an error message will be place in the interpreter
 *      result. Otherwise, the data sink will contain the binary
 *      output of the image.
 *
 * Side Effects:
 *      Memory is allocated for the data sink.
 *
 *---------------------------------------------------------------------------
 */
static int 
PictureToTif(Tcl_Interp *interp, Blt_Picture picture, Blt_DBuffer dbuffer,
             TifExportSwitches *switchesPtr)
{
    TIFF *tifPtr;
    TIFFErrorHandler oldErrorHandler, oldWarningHandler;
    TifMessage message;
    int photometric, samplesPerPixel;
    int compress, result;
    Picture *srcPtr;

    compress = tifCompressionSchemes[switchesPtr->compress];
    if (compress == COMPRESSION_NONE) {
#ifdef notdef
        Blt_Warn("not compressing TIFF output\n");
#endif
    }
#ifdef notdef
    if (!TIFFIsCODECConfigured((unsigned short int)compress)) {
        compress = COMPRESSION_NONE;
    }   
#endif
    srcPtr = picture;

    Tcl_DStringInit(&message.errors);
    Tcl_DStringInit(&message.warnings);
    Tcl_DStringAppend(&message.errors, "error writing TIF output: ", -1);
    tifMessagePtr = &message;
    message.numErrors = message.numWarnings = 0;

    oldErrorHandler = TIFFSetErrorHandler(TifError);
    oldWarningHandler = TIFFSetWarningHandler(TifWarning);

    tifPtr = TIFFClientOpen("data buffer", "w", (thandle_t)dbuffer,
        TifRead,                /* TIFFReadWriteProc */
        TifWrite,               /* TIFFReadWriteProc */
        TifSeek,                /* TIFFSeekProc */
        TifClose,               /* TIFFCloseProc */
        TifSize,                /* TIFFSizeProc */
        TifMapFile,             /* TIFFMapFileProc */
        TifUnmapFile);          /* TIFFUnmapFileProc */
    if (tifPtr == NULL) {
        Tcl_AppendResult(interp, "can't register TIF procs: ", (char *)NULL);
        return TCL_ERROR;
    }
    Blt_ClassifyPicture(srcPtr);
    if (Blt_Picture_IsColor(srcPtr)) {
        samplesPerPixel = (Blt_Picture_IsOpaque(srcPtr)) ? 3 : 4;
        photometric = PHOTOMETRIC_RGB;
    } else {
        if (!Blt_Picture_IsOpaque(srcPtr)) {
            Blt_Picture background;
            /* Blend picture with solid color background. */
            background = Blt_CreatePicture(srcPtr->width, srcPtr->height);
            Blt_BlankPicture(background, 0xFFFFFFFF); /* White background. */
            Blt_CompositePictures(background, srcPtr);
            srcPtr = background;
        }
        samplesPerPixel = 1;
        photometric = PHOTOMETRIC_MINISBLACK;
    }
    TIFFSetField(tifPtr, TIFFTAG_BITSPERSAMPLE,    8);
    TIFFSetField(tifPtr, TIFFTAG_COMPRESSION, (unsigned short int)compress);
    TIFFSetField(tifPtr, TIFFTAG_IMAGELENGTH,      srcPtr->height);
    TIFFSetField(tifPtr, TIFFTAG_IMAGEWIDTH,       srcPtr->width);
    TIFFSetField(tifPtr, TIFFTAG_ORIENTATION,      ORIENTATION_TOPLEFT);
    TIFFSetField(tifPtr, TIFFTAG_PHOTOMETRIC,      photometric);
    TIFFSetField(tifPtr, TIFFTAG_PLANARCONFIG,     PLANARCONFIG_CONTIG);
    TIFFSetField(tifPtr, TIFFTAG_RESOLUTIONUNIT,   2);
    TIFFSetField(tifPtr, TIFFTAG_ROWSPERSTRIP,     srcPtr->height);
    TIFFSetField(tifPtr, TIFFTAG_SAMPLESPERPIXEL,  samplesPerPixel);
    TIFFSetField(tifPtr, TIFFTAG_SOFTWARE,         TIFFGetVersion());
    TIFFSetField(tifPtr, TIFFTAG_XRESOLUTION,      300.0f);
    TIFFSetField(tifPtr, TIFFTAG_YRESOLUTION,      300.0f);
#ifdef WORD_BIGENDIAN
    TIFFSetField(tifPtr, TIFFTAG_FILLORDER,        FILLORDER_MSB2LSB);
#else
    TIFFSetField(tifPtr, TIFFTAG_FILLORDER,        FILLORDER_LSB2MSB);
#endif
    result = -1;
    {
        Blt_Pixel *srcRowPtr;
        int destBitsSize;
        int y;
        unsigned char *destBits;
        unsigned char *dp;

        destBitsSize = srcPtr->width * srcPtr->height * sizeof(uint32);
        destBits = (unsigned char *)_TIFFmalloc(destBitsSize);

        if (destBits == NULL) {
            TIFFError("tiff", "can't allocate space for TIF buffer");
            TIFFClose(tifPtr);
            return TCL_ERROR;
        }
        dp = destBits;
        srcRowPtr = srcPtr->bits;
        switch (samplesPerPixel) {
        case 4:
            for (y = 0; y < srcPtr->height; y++) {
                Blt_Pixel *sp;
                int x;
                
                sp = srcRowPtr;
                for (x = 0; x < srcPtr->width; x++) {
                    dp[0] = sp->Red;
                    dp[1] = sp->Green;
                    dp[2] = sp->Blue;
                    dp[3] = sp->Alpha;
                    dp += 4, sp++;
                }
                srcRowPtr += srcPtr->pixelsPerRow;
            }
            break;

        case 3:                         /* RGB, 100% opaque image. */
            for (y = 0; y < srcPtr->height; y++) {
                Blt_Pixel *sp;
                int x;
                
                sp = srcRowPtr;
                for (x = 0; x < srcPtr->width; x++) {
                    dp[0] = sp->Red;
                    dp[1] = sp->Green;
                    dp[2] = sp->Blue;
                    dp += 3, sp++;
                }
                srcRowPtr += srcPtr->pixelsPerRow;
            }
            break;

        case 1:
            for (y = 0; y < srcPtr->height; y++) {
                Blt_Pixel *sp;
                int x;
                
                sp = srcRowPtr;
                for (x = 0; x < srcPtr->width; x++) {
                    *dp++ = sp->Red;
                    sp++;
                }
                srcRowPtr += srcPtr->pixelsPerRow;
            }
            break;

        }
        result = TIFFWriteEncodedStrip(tifPtr, 0, destBits, destBitsSize);
        if (result < 0) {
            Tcl_AppendResult(interp, "error writing TIFF encoded strip",
                             (char *)NULL);
        }
        _TIFFfree(destBits);
    }
    TIFFClose(tifPtr);
    if (result == -1) {
        Blt_DBuffer_Free(dbuffer);
    }
    TIFFSetErrorHandler(oldErrorHandler);
    TIFFSetWarningHandler(oldWarningHandler);
    if (message.numWarnings > 0) {
        Tcl_SetErrorCode(interp, "PICTURE", "TIF_WRITE_WARNINGS", 
                Tcl_DStringValue(&message.warnings), (char *)NULL);
    } else {
        Tcl_SetErrorCode(interp, "NONE", (char *)NULL);
    }
    Tcl_DStringFree(&message.warnings);
    if (message.numErrors > 0) {
        Tcl_DStringResult(interp, &message.errors);
    }
    Tcl_DStringFree(&message.errors);
    if (srcPtr != picture) {
        Blt_FreePicture(srcPtr);
    }
    return (result == -1) ? TCL_ERROR : TCL_OK;
}


static Blt_Chain
ReadTif(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer)
{
    TifImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    return TifToPicture(interp, fileName, dbuffer, &switches);
}

static Tcl_Obj *
WriteTif(Tcl_Interp *interp, Blt_Picture picture)
{
    Tcl_Obj *objPtr;
    Blt_DBuffer dbuffer;
    TifExportSwitches switches;

    /* Default export switch settings. */
    memset(&switches, 0, sizeof(switches));
    dbuffer = Blt_DBuffer_Create();
    objPtr = NULL;
    if (PictureToTif(interp, picture, dbuffer, &switches) == TCL_OK) {
        objPtr = Blt_DBuffer_Base64EncodeToObj(dbuffer);
    }
    Blt_DBuffer_Destroy(dbuffer);
    return objPtr;
}

static Blt_Chain
ImportTif(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, 
          const char **fileNamePtr)
{
    Blt_Chain chain;
    Blt_DBuffer dbuffer;
    TifImportSwitches switches;
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
    chain = TifToPicture(interp, string, dbuffer, &switches);
 error:
    Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
    Blt_DBuffer_Destroy(dbuffer);
    return chain;
}

static int
ExportTif(Tcl_Interp *interp, int index, Blt_Chain chain, int objc, 
          Tcl_Obj *const *objv)
{
    Blt_DBuffer dbuffer;
    Blt_Picture picture;
    TifExportSwitches switches;
    int result;

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
    result = PictureToTif(interp, picture, dbuffer, &switches);
    if (result != TCL_OK) {
        Tcl_AppendResult(interp, "can't convert \"", 
                Tcl_GetString(objv[2]), "\"", (char *)NULL);
        goto error;
    }

    /* Write the TIF data to file or convert it to a base64 string. */
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
Blt_PictureTifInit(Tcl_Interp *interp)
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
    if (Tcl_PkgProvide(interp, "blt_picture_tif", BLT_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }
    return Blt_PictureRegisterFormat(interp,
        "tif",                  /* Name of format. */
        IsTif,                  /* Discovery routine. */
        ReadTif,                /* Read format procedure. */
        WriteTif,               /* Write format procedure. */
        ImportTif,              /* Import format procedure. */
        ExportTif);             /* Export format procedure. */
}

int 
Blt_PictureTifSafeInit(Tcl_Interp *interp) 
{
    return Blt_PictureTifInit(interp);
}

#endif /* HAVE_LIBTIF */


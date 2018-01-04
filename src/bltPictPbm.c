/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictPbm.c --
 *
 * This module implements PBM file format conversion routines for the
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
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <bltAlloc.h>
#include <bltSwitch.h>
#include <bltDBuffer.h>
#include <bltHash.h>
#include "bltPicture.h"
#include "bltPictFmts.h"

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef _MSC_VER
  #define vsnprintf               _vsnprintf
#endif

#include <setjmp.h>

typedef struct _Blt_Picture Picture;

#define TRUE    1
#define FALSE   0
#define div257(t)       (((t)+((t)>>8))>>8)
#define SetBit(x)       destRowPtr[(x>>3)] |= (0x80 >>(x&7))
#define GetBit(x)        srcRowPtr[(x>>3)] &  (0x80 >> (x&7))

typedef struct {
    jmp_buf jmpbuf;
    Tcl_DString errors;
    Tcl_DString warnings;
    int numWarnings, numErrors;
    int numLines;
} PbmMessage;

typedef struct {
    unsigned int version;               /* Version of PBM file */
    unsigned int maxval;                /* Maximum intensity allowed. */
    unsigned int width, height;         /* Dimensions of the image. */
    unsigned int bitsPerPixel;          /* # bits per pixel. */
    unsigned int isRaw;                 /* Indicates if the image format is
                                         * raw or plain. */
    Blt_DBuffer dbuffer;                /* */
    unsigned char *data;                /* Start of raw data */
    unsigned int bytesPerRow;
    Blt_Picture picture;
} Pbm;

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    int imageIndex;
} PbmImportSwitches;

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    Blt_Pixel bg;
    int index;
    int flags;
} PbmExportSwitches;

#define MAXCOLORS       256
#define PLAIN           (1<<0)

enum PbmVersions {
    PBM_UNKNOWN,                        
    PBM_PLAIN,                          /* Monochrome: 1-bit per pixel */
    PGM_PLAIN,                          /* 8-bits per pixel */
    PPM_PLAIN,                          /* 24-bits per pixel */
    PBM_RAW,                            /* 1-bit per pixel */
    PGM_RAW,                            /* 8/16-bits per pixel */
    PPM_RAW                             /* 24/48 bits per pixel */
};
    
static const char *pbmFormat[] = {
    "???", 
    "pbmplain",
    "pgmplain",
    "ppmplain",
    "pbmraw",
    "pgmraw",
    "ppmraw",
};

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_OBJ,   "-data",  "data", (char *)NULL,
        Blt_Offset(PbmImportSwitches, dataObjPtr), 0},
    {BLT_SWITCH_OBJ,   "-file",  "fileName", (char *)NULL,
        Blt_Offset(PbmImportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_INT, "-index", "int", (char *)NULL,
        Blt_Offset(PbmImportSwitches, imageIndex), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchParseProc ColorSwitchProc;
static Blt_SwitchCustom colorSwitch = {
    ColorSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,  "-background",    "color", (char *)NULL,
        Blt_Offset(PbmExportSwitches, bg), 0, 0, &colorSwitch},
    {BLT_SWITCH_OBJ,     "-data",  "varName", (char *)NULL,
        Blt_Offset(PbmExportSwitches, dataObjPtr), 0},
    {BLT_SWITCH_OBJ,     "-file",  "fileName", (char *)NULL,
        Blt_Offset(PbmExportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_INT_NNEG, "-index", "numPixture", (char *)NULL,
        Blt_Offset(PbmExportSwitches, index), 0},
    {BLT_SWITCH_BITS_NOARG, "-plain", "", (char *)NULL,
        Blt_Offset(PbmExportSwitches, flags), 0, PLAIN},
    {BLT_SWITCH_END}
};

static PbmMessage *pbmMessagePtr;

DLLEXPORT extern Tcl_AppInitProc Blt_PicturePbmInit;
DLLEXPORT extern Tcl_AppInitProc Blt_PicturePbmSafeInit;

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
PbmError(const char *fmt, ...)
{
    char string[BUFSIZ+4];
    int length;
    va_list args;

    va_start(args, fmt);
    length = vsnprintf(string, BUFSIZ, fmt, args);
    if (length > BUFSIZ) {
        strcat(string, "...");
    }
    Tcl_DStringAppend(&pbmMessagePtr->errors, Blt_Itoa(pbmMessagePtr->numLines),
        -1);
    Tcl_DStringAppend(&pbmMessagePtr->errors, ": ", 2);
    Tcl_DStringAppend(&pbmMessagePtr->errors, string, -1);
    va_end(args);
    longjmp(pbmMessagePtr->jmpbuf, 0);
}

/*ARGSUSED*/
static void
PbmWarning(const char *fmt, ...)
{
    char string[BUFSIZ+4];
    int length;
    va_list args;

    va_start(args, fmt);
    length = vsnprintf(string, BUFSIZ, fmt, args);
    if (length > BUFSIZ) {
        strcat(string, "...");
    }
    Tcl_DStringAppend(&pbmMessagePtr->warnings, string, -1);
    va_end(args);
    pbmMessagePtr->numWarnings++;
}

static char *
PbmComment(Pbm *pbmPtr, char *bp) 
{
    char *p;

    if (pbmPtr->isRaw) {
        return bp;
    }
    p = bp;
    if (*p == '#') {
        /* Find end of comment line. */
        while((*p != '\n') && (*p != '\0')) {
            p++;
        }
        if (*p == '\n') {
            pbmMessagePtr->numLines++;
        }
        PbmWarning("comment: %.*s\n", p-bp, bp);
    }
    return p;
}

static unsigned int
PbmNextValue(Pbm *pbmPtr)
{
    char *p, *endp;
    unsigned int value;

    p = (char *)Blt_DBuffer_Pointer(pbmPtr->dbuffer);
    while(isspace(*p)) {
        if (*p == '\n') {
            pbmMessagePtr->numLines++;
        }
        p++;
    }
    if (*p == '#') {
        p = PbmComment(pbmPtr, p);
    }
    while(isspace(*p)) {
        if (*p == '\n') {
            pbmMessagePtr->numLines++;
        }
        p++;
    }
    if (*p == '\0') {
        PbmError("unexpected EOF in image data");
    }
    value = strtoul(p, &endp, 10);
    if (endp == p) {
        PbmError("bad value \"%.10s\" in %s image data", 
                 p, pbmFormat[pbmPtr->version]);
    }
    if (value > pbmPtr->maxval) {
        PbmError("value (%d) greater than %s image max value %d", value, 
                pbmFormat[pbmPtr->version], pbmPtr->maxval);
    }
    p = endp;
    while (isspace(*p)) {
        if (*p == '\n') {
            pbmMessagePtr->numLines++;
        }
        p++;
    }
    Blt_DBuffer_SetPointer(pbmPtr->dbuffer, (unsigned char*)p);
    return value;
}    


static INLINE int 
PbmGetShort(unsigned char *bp) {
    return (bp[0] << 8) + bp[1];
}

static Picture *
PbmPlainData(Pbm *pbmPtr)
{
    Picture *destPtr;

    pbmPtr->picture = destPtr = 
        Blt_CreatePicture(pbmPtr->width, pbmPtr->height);
    switch (pbmPtr->bitsPerPixel) {
    case 1:                     /* Monochrome */
    case 8:                     /* Greyscale */
        {
            Blt_Pixel *destRowPtr;
            unsigned int y;

            destRowPtr = destPtr->bits;
            for (y = 0; y < pbmPtr->height; y++) {
                Blt_Pixel *dp, *dend;

                for (dp = destRowPtr, dend = dp + destPtr->width; dp < dend;
                         dp++) {
                    dp->Red = dp->Green = dp->Blue = PbmNextValue(pbmPtr);
                    dp->Alpha = ALPHA_OPAQUE;
                }
                destRowPtr += destPtr->pixelsPerRow;
            }
        }
        break;
    case 16:                    /* Greyscale (2 bytes)  */
        {
            Blt_Pixel *destRowPtr;
            unsigned int y;

            destRowPtr = destPtr->bits;
            for (y = 0; y < pbmPtr->height; y++) {
                Blt_Pixel *dp, *dend;

                for (dp = destRowPtr, dend = dp + destPtr->width; dp < dend;
                     dp++) {
                    unsigned int value;

                    value = PbmNextValue(pbmPtr);
                    dp->Red = dp->Green = dp->Blue = div257(value);
                    dp->Alpha = ALPHA_OPAQUE;
                }
                destRowPtr += destPtr->pixelsPerRow;
            }
        }
        break;
    case 24:                    /* Color (1 byte per color component) */
        {
            Blt_Pixel *destRowPtr;
            unsigned int y;

            destRowPtr = destPtr->bits;
            for (y = 0; y < pbmPtr->height; y++) {
                Blt_Pixel *dp, *dend;

                for (dp = destRowPtr, dend = dp + destPtr->width; dp < dend;
                         dp++) {
                    dp->Red   = PbmNextValue(pbmPtr);
                    dp->Green = PbmNextValue(pbmPtr);
                    dp->Blue  = PbmNextValue(pbmPtr);
                    dp->Alpha = ALPHA_OPAQUE;
                }
                destRowPtr += destPtr->pixelsPerRow;
            }
        }
        break;
    case 48:                    /* Color (2 bytes per color component) */
        {
            Blt_Pixel *destRowPtr;
            unsigned int y;

            destRowPtr = destPtr->bits;
            for (y = 0; y < pbmPtr->height; y++) {
                Blt_Pixel *dp, *dend;

                for (dp = destRowPtr, dend = dp + destPtr->width; dp < dend;
                         dp++) {
                    int r, g, b;

                    r = PbmNextValue(pbmPtr);
                    g = PbmNextValue(pbmPtr);
                    b = PbmNextValue(pbmPtr);
                    dp->Red   = div257(r);
                    dp->Green = div257(g);
                    dp->Blue  = div257(b);
                    dp->Alpha = ALPHA_OPAQUE;
                }
                destRowPtr += destPtr->pixelsPerRow;
            }
        }
        break;
    }
    return destPtr;
}

static Picture *
PbmRawData(Pbm *pbmPtr)
{
    Picture *destPtr;

    pbmPtr->picture = destPtr = 
        Blt_CreatePicture(pbmPtr->width, pbmPtr->height);
    switch (pbmPtr->bitsPerPixel) {
    case 1: 
        {
            /* Monochrome */
            Blt_Pixel *destRowPtr;
            unsigned int y;
            unsigned char *srcRowPtr;
            
            srcRowPtr = pbmPtr->data;
            destRowPtr = destPtr->bits;
            for (y = 0; y < pbmPtr->height; y++) {
                unsigned int x;
                Blt_Pixel *dp;
                
                dp = destRowPtr;
                for (x = 0; x < pbmPtr->width; x++) {
                    dp->Red = dp->Green = dp->Blue = (GetBit(x)) ? 0xFF : 0;
                    dp->Alpha = ALPHA_OPAQUE;
                    dp++;
                }
                srcRowPtr += pbmPtr->bytesPerRow;
                destRowPtr += destPtr->pixelsPerRow;
            }
            break;
        }
    case 8: 
        {
            /* Greyscale (1 byte)  */
            Blt_Pixel *destRowPtr;
            unsigned int y;
            unsigned char *srcRowPtr;
            
            srcRowPtr = pbmPtr->data;
            destRowPtr = destPtr->bits;
            for (y = 0; y < pbmPtr->height; y++) {
                unsigned char *sp;
                Blt_Pixel *dp, *dend;
                
                sp = srcRowPtr;
                for (dp = destRowPtr, dend = dp + destPtr->width; dp < dend;
                     sp++, dp++) {
                    dp->Red = dp->Green = dp->Blue = *sp;
                    dp->Alpha = ALPHA_OPAQUE;
                }
                srcRowPtr += pbmPtr->bytesPerRow;
                destRowPtr += destPtr->pixelsPerRow;
            }
            break;
        }
    case 16: 
        { 
            /* Greyscale (2 bytes)  */
            Blt_Pixel *destRowPtr;
            unsigned int y;
            unsigned char *srcRowPtr;
            
            srcRowPtr = pbmPtr->data;
            destRowPtr = destPtr->bits;
            for (y = 0; y < pbmPtr->height; y++) {
                unsigned char *sp;
                Blt_Pixel *dp, *dend;
                
                sp = srcRowPtr;
                for (dp = destRowPtr, dend = dp + destPtr->width; dp < dend;
                     sp += 2, dp++) {
                    unsigned int value;
                    
                    value = PbmGetShort(sp);
                    dp->Red = dp->Green = dp->Blue = div257(value);
                    dp->Alpha = ALPHA_OPAQUE;
                }
                srcRowPtr += pbmPtr->bytesPerRow;
                destRowPtr += destPtr->pixelsPerRow;
            }
            break;
        }
    case 24: 
        { 
            /* Color (1 byte per color component) */
            Blt_Pixel *destRowPtr;
            unsigned int y;
            unsigned char *srcRowPtr;
            
            srcRowPtr = pbmPtr->data;
            destRowPtr = destPtr->bits;
            for (y = 0; y < pbmPtr->height; y++) {
                unsigned char *sp;
                Blt_Pixel *dp, *dend;
                
                sp = srcRowPtr;
                for (dp = destRowPtr, dend = dp + destPtr->width; dp < dend;
                     sp += 3, dp++) {
                    dp->Red   = sp[0];
                    dp->Green = sp[1];
                    dp->Blue  = sp[2];
                    dp->Alpha = ALPHA_OPAQUE;
                }
                srcRowPtr += pbmPtr->bytesPerRow;
                destRowPtr += destPtr->pixelsPerRow;
            }
            break;
        }
    case 48: 
        {
            /* Color (2 bytes per color component) */
            Blt_Pixel *destRowPtr;
            unsigned int y;
            unsigned char *srcRowPtr;
            
            srcRowPtr = pbmPtr->data;
            destRowPtr = destPtr->bits;
            for (y = 0; y < pbmPtr->height; y++) {
                unsigned char *sp;
                Blt_Pixel *dp, *dend;
                
                sp = srcRowPtr;
                for (dp = destRowPtr, dend = dp + destPtr->width; dp < dend;
                     sp += 6, dp++) {
                    unsigned int r, g, b;
                    
                    r = PbmGetShort(sp);
                    g = PbmGetShort(sp+2);
                    b = PbmGetShort(sp+4);
                    dp->Red   = div257(r);
                    dp->Green = div257(g);
                    dp->Blue  = div257(b);
                    dp->Alpha = ALPHA_OPAQUE;
                }
                srcRowPtr += pbmPtr->bytesPerRow;
                destRowPtr += destPtr->pixelsPerRow;
            }
            break;
        }
    }
    return destPtr;
}

static Blt_Picture
PbmImage(Pbm *pbmPtr)
{
    char *bp, *p;
    const char *type;
    size_t size, want;
    unsigned char *start;
    Picture *destPtr;

    size = Blt_DBuffer_BytesLeft(pbmPtr->dbuffer);
    start = Blt_DBuffer_Pointer(pbmPtr->dbuffer);
    if (size < 14) {
        PbmError("can't read PBM image: short file %d bytes", size);
    }
    bp = (char *)start;
    if ((bp[0] != 'P') || (bp[1] < '1') || (bp[1] > '6')) {
        PbmError("unknown PBM image header (%c%c) (%d).", bp[0], bp[1],
                 Blt_DBuffer_Cursor(pbmPtr->dbuffer));
    }
    pbmPtr->version = bp[1] - '0';
    pbmPtr->isRaw = (pbmPtr->version > 3);
    switch(pbmPtr->version) {   
    case PBM_PLAIN:             /* P2 */
    case PBM_RAW:               /* P5 */
        pbmPtr->bitsPerPixel = 8;
        break;
    case PGM_PLAIN:             /* P1 */
    case PGM_RAW:               /* P4 */
        pbmPtr->bitsPerPixel = 1;
        break;
    case PPM_PLAIN:             /* P3 */
    case PPM_RAW:               /* P6 */
        pbmPtr->bitsPerPixel = 24;
        break;
    }
    type = pbmFormat[pbmPtr->version];
    if (!isspace(bp[2])) {
        PbmError("no white space after version in %s header.", type);
    }
    if (bp[2] == '\n') {
        pbmMessagePtr->numLines++;
    }
    p = bp + 3;
    if (*p == '#') {
        p = PbmComment(pbmPtr, p);
    }
    pbmPtr->width = strtoul(p, &p, 10);
    if (pbmPtr->width == 0) {
        PbmError("bad %s width specification %s", type, bp+3);
    }
    if (!isspace(*p)) {
        PbmError("no white space after width in %s header.", type);
    }
    if (*p == '\n') {
        pbmMessagePtr->numLines++;
    }
    p++;
    if (*p == '#') {
        p = PbmComment(pbmPtr, p);
    }
    pbmPtr->height = strtoul(p, &p, 10);
    if (pbmPtr->height == 0) {
        PbmError("bad %s height specification", type);
    }
    if (!isspace(*p)) {
        PbmError("no white space after height in %s header.", type);
    }
    if (*p == '\n') {
        pbmMessagePtr->numLines++;
    }
    p++;
    if (*p == '#') {
        p = PbmComment(pbmPtr, p);
    }
    if (pbmPtr->bitsPerPixel != 1) {
        pbmPtr->maxval = strtoul(p, &p, 10);
        if (pbmPtr->maxval == 0) {
            PbmError("bad %s maxval specification", type);
        }
        if (!isspace(*p)) {
            PbmError("no white space after maxval in %s header.", type);
        }
        if (*p == '\n') {
            pbmMessagePtr->numLines++;
        }
        p++;
        if (*p == '#') {
            p = PbmComment(pbmPtr, p);
        }
        if (pbmPtr->maxval >= USHRT_MAX) {
            PbmError("invalid %s maxval specification", type);
        }
        if (pbmPtr->maxval > 255) {
            pbmPtr->bitsPerPixel <<= 1;  /* 16-bit greyscale or 48 bit color. */
        }
    }
    pbmPtr->data = (unsigned char *)p;
    pbmPtr->bytesPerRow = ((pbmPtr->bitsPerPixel * pbmPtr->width) + 7) / 8;
    want = (pbmPtr->data - start) + pbmPtr->height * pbmPtr->bytesPerRow;
    if ((pbmPtr->isRaw) && (want > Blt_DBuffer_BytesLeft(pbmPtr->dbuffer))) {
        PbmError("short %s file: expected %d bytes, got %d", type, want,
                 Blt_DBuffer_BytesLeft(pbmPtr->dbuffer));
    }       
    if (pbmPtr->isRaw) {
        destPtr = PbmRawData(pbmPtr);
        Blt_DBuffer_SetPointer(pbmPtr->dbuffer, pbmPtr->data + 
                               (pbmPtr->height * pbmPtr->bytesPerRow));
    } else {
        Blt_DBuffer_SetPointer(pbmPtr->dbuffer, pbmPtr->data);
        destPtr = PbmPlainData(pbmPtr);
    }
    destPtr->flags &= ~BLT_PIC_UNINITIALIZED;
    return destPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * IsPbm --
 *
 *      Attempts to parse a PBM file header.
 *
 * Results:
 *      Returns 1 is the header is PBM and 0 otherwise.  Note that
 *      the validity of the header contents is not checked here.  That's
 *      done in PbmToPictures.
 *
 *---------------------------------------------------------------------------
 */
static int
IsPbm(Blt_DBuffer dbuffer)
{
    unsigned char *bp;

    Blt_DBuffer_Rewind(dbuffer);
    if (Blt_DBuffer_BytesLeft(dbuffer) < 2) {
        return FALSE;
    }
    bp = Blt_DBuffer_Pointer(dbuffer);
    if ((bp[0] != 'P') || (bp[1] < '1') || (bp[1] > '6')) {
        return FALSE;
    }
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * PbmToPictures --
 *
 *      Reads a PBM file and converts it into a picture.
 *
 * Results:
 *      The picture is returned.  If an error occured, such
 *      as the designated file could not be opened, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain 
PbmToPictures(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer,
              PbmImportSwitches *switchesPtr)
{
    Blt_Chain chain;
    Pbm pbm;
    PbmMessage message;

    pbmMessagePtr = &message;
    message.numWarnings = 0;
    message.numLines = 0;
    memset(&pbm, 0, sizeof(pbm)); /* Clear the structure. */
    pbm.dbuffer = dbuffer;

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
        if (pbm.picture != NULL) {
            Blt_FreePicture(pbm.picture);
        }
        return NULL;
    }
    chain = NULL;
    if (!IsPbm(pbm.dbuffer)) {
        PbmError("bad PBM header");
    }
    Blt_DBuffer_Rewind(pbm.dbuffer);
    chain = Blt_Chain_Create();
    while (Blt_DBuffer_BytesLeft(pbm.dbuffer) > 0) {
        Blt_Picture picture;

        picture = PbmImage(&pbm);
        Blt_Chain_Append(chain, picture);
    }
    if (message.numWarnings > 0) {
        Tcl_SetErrorCode(interp, "PICTURE", "PBM_READ_WARNINGS", 
                Tcl_DStringValue(&message.warnings), (char *)NULL);
    } else {
        Tcl_SetErrorCode(interp, "NONE", (char *)NULL);
    }
    Tcl_DStringFree(&message.warnings);
    Tcl_DStringFree(&message.errors);
    return chain;
}

static void
WritePlainGreyscale(Picture *srcPtr, Blt_DBuffer dbuffer)
{
    Blt_Pixel *srcRowPtr;
    int y, count;
    
    Blt_DBuffer_Format(dbuffer, "P%d\n%d\n%d\n", PGM_PLAIN, 
                       srcPtr->width, srcPtr->height);
    srcRowPtr = srcPtr->bits;
    count = 0;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;
        
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            char *bp;
            
            bp = (char *)Blt_DBuffer_Extend(dbuffer, 4);
            sprintf(bp, "%3d ", (sp->Red) ? 1 : 0);
            count++;
            if ((count % 15) == 0) {
                Blt_DBuffer_AppendString(dbuffer, "\n", 1);
            }
        }
        Blt_DBuffer_AppendString(dbuffer, "\n", 1);
        count = 0;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    Blt_DBuffer_AppendString(dbuffer, "\n", 1);
}

static void
WritePlainColor(Picture *srcPtr, Blt_DBuffer dbuffer)
{
    Blt_Pixel *srcRowPtr;
    int y, count;
    
    Blt_DBuffer_Format(dbuffer, "P%d\n%d\n%d\n255\n", PPM_PLAIN, 
        srcPtr->width, srcPtr->height);
    srcRowPtr = srcPtr->bits;
    count = 0;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;
        
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            char *bp;
            
            bp = (char *)Blt_DBuffer_Extend(dbuffer, 13);
            sprintf(bp, "%3d %3d %3d  ", sp->Red, sp->Green, sp->Blue);
            count++;
            if ((count % 5) == 0) {
                Blt_DBuffer_AppendString(dbuffer, "\n", 1);
            }
        }
        Blt_DBuffer_AppendString(dbuffer, "\n", 1);
        count = 0;
        srcRowPtr += srcPtr->pixelsPerRow;
    }
    Blt_DBuffer_AppendString(dbuffer, "\n", 1);
}

static void
WriteBinaryGreyscale(Picture *srcPtr, Blt_DBuffer dbuffer)
{
    Blt_Pixel *srcRowPtr;
    int bytesPerRow;
    int y;
    size_t length;
    unsigned char *destRowPtr;
    
    Blt_DBuffer_Format(dbuffer, "P%d\n%d\n%d\n255\n", PGM_RAW, 
        srcPtr->width, srcPtr->height);
    bytesPerRow = srcPtr->width;
    length = Blt_DBuffer_Length(dbuffer);
    Blt_DBuffer_Extend(dbuffer, srcPtr->height * bytesPerRow);
    destRowPtr = Blt_DBuffer_Bytes(dbuffer) + length;
    length += srcPtr->height * bytesPerRow;
    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;
        unsigned char *dp;
        
        dp = destRowPtr;
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            dp[0] = sp->Red;
            dp++;
        }
        destRowPtr += bytesPerRow;
        srcRowPtr  += srcPtr->pixelsPerRow;
    }
    Blt_DBuffer_SetLength(dbuffer, length);
}

static void
WriteBinaryColor(Picture *srcPtr, Blt_DBuffer dbuffer)
{
    size_t length;
    Blt_Pixel *srcRowPtr;
    int bytesPerRow;
    int y;
    unsigned char *destRowPtr;
    
    Blt_DBuffer_Format(dbuffer, "P%d\n%d\n%d\n255\n", PPM_RAW, 
                       srcPtr->width, srcPtr->height);
    bytesPerRow = srcPtr->width * 3;
    length = Blt_DBuffer_Length(dbuffer);
    Blt_DBuffer_Extend(dbuffer, srcPtr->height * bytesPerRow);
    destRowPtr = Blt_DBuffer_Bytes(dbuffer) + length;
    length += srcPtr->height * bytesPerRow;
    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;
        unsigned char *dp;
        
        dp = destRowPtr;
        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            dp[0] = sp->Red;
            dp[1] = sp->Green;
            dp[2] = sp->Blue;
            dp += 3;
        }
        destRowPtr += bytesPerRow;
        srcRowPtr  += srcPtr->pixelsPerRow;
    }
    Blt_DBuffer_SetLength(dbuffer, length);
}

/*
 *---------------------------------------------------------------------------
 *
 * PicturesToPbm --
 *
 *      Reads an PBM file and converts it into a picture.
 *
 * Results:
 *      The picture is returned.  If an error occured, such as the designated
 *      file could not be opened, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
PictureToPbm(Tcl_Interp *interp, Blt_Picture original, Blt_DBuffer dbuffer,
             PbmExportSwitches *switchesPtr)
{
    Picture *srcPtr;

    srcPtr = original;
    Blt_ClassifyPicture(srcPtr);
    if (!Blt_Picture_IsOpaque(srcPtr)) { 
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
    if (srcPtr->flags & BLT_PIC_PREMULT_COLORS) {
        Blt_Picture unassoc;
        /* 
         * The picture has alphas burned into its color components.
         * Create a temporary copy removing pre-multiplied alphas.
         */ 
        unassoc = Blt_ClonePicture(srcPtr);
        Blt_UnmultiplyColors(unassoc);
        if (srcPtr != original) {
            Blt_FreePicture(srcPtr);
        }
        srcPtr = unassoc;
    }
    if (srcPtr->flags & BLT_PIC_GREYSCALE) {  /* Greyscale */
        if (switchesPtr->flags & PLAIN) {
            WritePlainGreyscale(srcPtr, dbuffer);
        } else {
            WriteBinaryGreyscale(srcPtr, dbuffer);
        }
    } else {                    /* Color PPM */
        if (switchesPtr->flags & PLAIN) {
            WritePlainColor(srcPtr, dbuffer);
        } else {
            WriteBinaryColor(srcPtr, dbuffer);
        }
    }
    if (srcPtr != original) {
        Blt_FreePicture(srcPtr);
    }
    return TCL_OK;
}


static Blt_Chain
ReadPbm(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer)
{
    PbmImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    switches.imageIndex = 1;
    return PbmToPictures(interp, fileName, dbuffer, &switches);
}

static Tcl_Obj *
WritePbm(Tcl_Interp *interp, Blt_Picture picture)
{
    Blt_DBuffer dbuffer;
    PbmExportSwitches switches;
    Tcl_Obj *objPtr;

    /* Default export switch settings. */
    memset(&switches, 0, sizeof(switches));
    switches.bg.u32 = 0xFFFFFFFF; /* white */

    dbuffer = Blt_DBuffer_Create();
    objPtr = NULL;
    if (PictureToPbm(interp, picture, dbuffer, &switches) == TCL_OK) {
        objPtr = Blt_DBuffer_Base64EncodeToObj(dbuffer);
    }
    Blt_DBuffer_Destroy(dbuffer);
    return objPtr;
}

static Blt_Chain
ImportPbm(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, 
          const char **fileNamePtr)
{
    Blt_DBuffer dbuffer;
    Blt_Chain chain;
    PbmImportSwitches switches;
    const char *string;

    memset(&switches, 0, sizeof(switches));
    switches.imageIndex = 1;
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
    chain = PbmToPictures(interp, string, dbuffer, &switches);
 error:
    Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
    Blt_DBuffer_Destroy(dbuffer);
    return chain;
}

static int
ExportPbm(Tcl_Interp *interp, int index, Blt_Chain chain, int objc, 
          Tcl_Obj *const *objv)
{
    Blt_DBuffer dbuffer;
    PbmExportSwitches switches;
    int result;

    memset(&switches, 0, sizeof(switches));
    switches.index = index;
    switches.bg.u32 = 0xFFFFFFFF;       /* Default bgcolor is white. */
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 3, objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if ((switches.dataObjPtr != NULL) && (switches.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "more than one export destination: ",
                "use only one -file or -data switch.", (char *)NULL);
        return TCL_ERROR;
    }
    result = TCL_ERROR;             /* Suppress compiler warning. */
    dbuffer = Blt_DBuffer_Create();
    if (switches.index < 0) {
        Blt_ChainLink link;
        
        for (link = Blt_Chain_FirstLink(chain); link != NULL;
             link = Blt_Chain_NextLink(link)) {
            Blt_Picture picture;

            picture = Blt_Chain_GetValue(link);
            if (PictureToPbm(interp, picture, dbuffer, &switches) != TCL_OK) {
                Tcl_AppendResult(interp, "can't convert \"", 
                                 Tcl_GetString(objv[2]), "\"", (char *)NULL);
                goto error;
            }
        }
    } else {
        Blt_Picture picture;

        picture = Blt_GetNthPicture(chain, switches.index);
        if (picture == NULL) {
            Tcl_AppendResult(interp, "bad picture index.", (char *)NULL);
            goto error;
        }
        if (PictureToPbm(interp, picture, dbuffer, &switches) != TCL_OK) {
            Tcl_AppendResult(interp, "can't convert \"", 
                             Tcl_GetString(objv[2]), "\"", (char *)NULL);
            goto error;
        }
    }
    /* Write the PBM data to file or convert it to a base64 string. */
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
Blt_PicturePbmInit(Tcl_Interp *interp)
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
    if (Tcl_PkgProvide(interp, "blt_picture_pbm", BLT_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }
    return Blt_PictureRegisterFormat(interp, 
        "pbm",                  /* Name of format. */
        IsPbm,                  /* Discovery routine. */
        ReadPbm,                /* Read format procedure. */
        WritePbm,               /* Write format procedure. */
        ImportPbm,              /* Import format procedure. */
        ExportPbm);             /* Export format switches. */
}

int 
Blt_PicturePbmSafeInit(Tcl_Interp *interp) 
{
    return Blt_PicturePbmInit(interp);
}

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictJpg.c --
 *
 * This module implements JPEG file format conversion routines for the
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
 * The JPEG reader/writer is adapted from jdatasrc.c and jdatadst.c in the
 * Independent JPEG Group (version 6b) library distribution.
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.  All Rights Reserved except as
 * specified below.
 * 
 *   Permission is hereby granted to use, copy, modify, and distribute this
 *   software (or portions thereof) for any purpose, without fee, subject
 *   to these conditions: (1) If any part of the source code for this
 *   software is distributed, then this README file must be included, with
 *   this copyright and no-warranty notice unaltered; and any additions,
 *   deletions, or changes to the original files must be clearly indicated
 *   in accompanying documentation.  (2) If only executable code is
 *   distributed, then the accompanying documentation must state that "this
 *   software is based in part on the work of the Independent JPEG Group".
 *   (3) Permission for use of this software is granted only if the user
 *   accepts full responsibility for any undesirable consequences; the
 *   authors accept NO LIABILITY for damages of any kind.
 *
 *   The authors make NO WARRANTY or representation, either express or
 *   implied, with respect to this software, its quality, accuracy,
 *   merchantability, or fitness for a particular purpose.  This software
 *   is provided "AS IS", and you, its user, assume the entire risk as to
 *   its quality and accuracy.
 *
 */

#include "bltInt.h"

#include "config.h"
#ifdef HAVE_LIBJPG

#include <tcl.h>

#include "bltPicture.h"
#include "bltPictFmts.h"

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_MEMORY_H
  #include <memory.h>
#endif /* HAVE_MEMORY_H */

typedef struct _Blt_Picture Picture;

#undef HAVE_STDLIB_H

#ifdef WIN32
  #define XMD_H   1
#endif  /* WIN32 */

#undef EXTERN
#undef FAR
#include "jpeglib.h"
#include "jerror.h"
#include <setjmp.h>

#define PIC_PROGRESSIVE (1<<0)
#define PIC_NOQUANTIZE  (1<<1)

#define PIC_FMT_ISASCII (1<<3)

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    int quality;                        /* Value 0..100 */
    int smoothing;                      /* Value 0..100 */
    int compress;                       /* Value 0..N */
    int flags;                          /* Flag. */
    Blt_Pixel bg;
    int index;
} JpgExportSwitches;

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    Tcl_Obj *varObjPtr;                 /* Name of variable storing the
                                         * tags.*/
    int method;                         /* DCT method. */
} JpgImportSwitches;

static Blt_SwitchParseProc ColorSwitchProc;

static Blt_SwitchCustom colorSwitch = {
    ColorSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc DctSwitchProc;

static Blt_SwitchCustom dctSwitch = {
    DctSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc PercentSwitchProc;

static Blt_SwitchCustom percentSwitch = {
    PercentSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,    "-background", "color", (char *)NULL,
        Blt_Offset(JpgExportSwitches, bg), 0, 0, &colorSwitch},
    {BLT_SWITCH_OBJ,       "-data",        "varName", (char *)NULL,
        Blt_Offset(JpgExportSwitches, dataObjPtr),0},
    {BLT_SWITCH_OBJ,       "-file",        "fileName", (char *)NULL,
        Blt_Offset(JpgExportSwitches, fileObjPtr),0},
    {BLT_SWITCH_INT_NNEG, "-index", "int", (char *)NULL,
        Blt_Offset(JpgExportSwitches, index), 0},
    {BLT_SWITCH_CUSTOM,  "-quality",     "percent", (char *)NULL,
        Blt_Offset(JpgExportSwitches, quality), 0, 0, &percentSwitch},
    {BLT_SWITCH_INT_NNEG,  "-smooth",      "percent", (char *)NULL,
        Blt_Offset(JpgExportSwitches, smoothing), 0, 0, &percentSwitch},
    {BLT_SWITCH_BITMASK,   "-progressive", "", (char *)NULL,
        Blt_Offset(JpgExportSwitches, flags), 0, PIC_PROGRESSIVE},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data", "data", (char *)NULL,
        Blt_Offset(JpgImportSwitches, dataObjPtr), 0},
    {BLT_SWITCH_CUSTOM, "-dct", "method", (char *)NULL,
       Blt_Offset(JpgImportSwitches, method), 0, 0, &dctSwitch},
    {BLT_SWITCH_OBJ, "-file", "fileName", (char *)NULL,
        Blt_Offset(JpgImportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_OBJ, "-tags", "varName", (char *)NULL,
        Blt_Offset(JpgImportSwitches, varObjPtr), 0},
    {BLT_SWITCH_END}
};

#define JPG_BUF_SIZE  4096             /* Choose an efficiently fwrite'able
                                        * size */

typedef struct {
    struct jpeg_source_mgr pub;         /* Public fields */

    Blt_DBuffer dBuffer;                /* Collects the converted data. */
    Tcl_Interp *interp;
    const char *varName;
} JpgReader;

typedef struct {
    struct jpeg_destination_mgr pub;    /* Public fields */
    Blt_DBuffer dBuffer;                /* Target stream */
    JOCTET *bytes;                      /* Start of buffer */
} JpgWriter;

typedef struct {
    struct jpeg_error_mgr pub;          /* "Public" fields */
    jmp_buf jmpbuf;
    Tcl_DString ds;
} JpgErrorHandler;

DLLEXPORT extern Tcl_AppInitProc Blt_PictureJpgInit;
DLLEXPORT extern Tcl_AppInitProc Blt_PictureJpgSafeInit;

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

/*
 *---------------------------------------------------------------------------
 *
 * DctSwitchProc --
 *
 *      Convert a Tcl_Obj representing a DCT method.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DctSwitchProc(ClientData clientData, Tcl_Interp *interp, const char *switchName,
              Tcl_Obj *objPtr, char *record, int offset, int flags)     
{
    int *methodPtr = (int *)(record + offset);
    const char *string;
    char c;
    
    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'f') && (strcmp(string, "fast") == 0)) {
        *methodPtr = JDCT_IFAST;
    } else if ((c == 'f') && (strcmp(string, "slow") == 0)) {
        *methodPtr = JDCT_ISLOW;
    } else if ((c == 'f') && (strcmp(string, "float") == 0)) {
        *methodPtr = JDCT_FLOAT;
    } else {
        Tcl_AppendResult(interp, "bad DCT method \"", string, "\" should be ",
                         " fast, slow, or float.", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PercentSwitchProc --
 *
 *      Convert a Tcl_Obj representing an integer percentage 0 to 100.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PercentSwitchProc(ClientData clientData, Tcl_Interp *interp,
                  const char *switchName, Tcl_Obj *objPtr, char *record,
                  int offset, int flags)        
{
    int *percentPtr = (int *)(record + offset);
    double value;
    
    if (Tcl_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((value < 0.0) || (value > 100.0)) {
        Tcl_AppendResult(interp, "bad percent value \"", Tcl_GetString(objPtr),
                         "\" should be between 0 and 100.", (char *)NULL);
        return TCL_ERROR;
    }
    *percentPtr = (int)(value + 0.5);
    return TCL_OK;
}

/*
 * Marker processor for COM and interesting APPn markers.
 * This replaces the library's built-in processor, which just skips the marker.
 * We want to print out the marker as text, to the extent possible.
 * Note this code relies on a non-suspending data source.
 */
static int
jpeg_getc(j_decompress_ptr cinfo)
/* Read next byte */
{
    struct jpeg_source_mgr * datasrc = cinfo->src;
    
    if (datasrc->bytes_in_buffer == 0) {
        if (! (*datasrc->fill_input_buffer) (cinfo))
            ERREXIT(cinfo, JERR_CANT_SUSPEND);
    }
    datasrc->bytes_in_buffer--;
    return GETJOCTET(*datasrc->next_input_byte++);
}

static int
JpgApp1Proc(j_decompress_ptr commPtr)
{
    int length;
    struct jpeg_source_mgr *datasrc;
    JpgReader *readerPtr;
    
    /* Get length of segment. */
    length = jpeg_getc(commPtr) << 8;
    length += jpeg_getc(commPtr);
    length -= 2;			/* discount the length word itself */
    
    datasrc = commPtr->src;
    if (datasrc->bytes_in_buffer < length) {
        if (! (*datasrc->fill_input_buffer) (commPtr)) {
            ERREXIT(commPtr, JERR_CANT_SUSPEND);
        }
    }
    readerPtr = (JpgReader *)commPtr->src;
    if (memcmp(datasrc->next_input_byte, "Exif\0\0", 6) != 0) {
        return TRUE;                    /* Not a EXIF directory. */
    }
    if (Blt_ParseTifTags(readerPtr->interp, readerPtr->varName,
                datasrc->next_input_byte, 6, length) == TCL_ERROR) {
        return FALSE;
    }
    datasrc->bytes_in_buffer -= length;
    datasrc->next_input_byte += length;
    return TRUE;
}

static void
JpgErrorProc(j_common_ptr commPtr)
{
    JpgErrorHandler *errorPtr = (JpgErrorHandler *)commPtr->err;

    (*errorPtr->pub.output_message) (commPtr);
    longjmp(errorPtr->jmpbuf, 1);
}

static void
JpgMessageProc(j_common_ptr commPtr)
{
    JpgErrorHandler *errorPtr = (JpgErrorHandler *)commPtr->err;
    char mesg[JMSG_LENGTH_MAX];

    /* Create the message and append it into the dynamic string. */
    (*errorPtr->pub.format_message) (commPtr, mesg);
    Tcl_DStringAppend(&errorPtr->ds, " ", -1);
    Tcl_DStringAppend(&errorPtr->ds, mesg, -1);
}

static void
JpgInitSource(j_decompress_ptr commPtr)
{
    JpgReader *readerPtr = (JpgReader *)commPtr->src;

    readerPtr->pub.next_input_byte = Blt_DBuffer_Bytes(readerPtr->dBuffer);
    readerPtr->pub.bytes_in_buffer = Blt_DBuffer_Length(readerPtr->dBuffer);
}

static boolean
JpgFillInputBuffer(j_decompress_ptr commPtr)
{
    JpgReader *readerPtr = (JpgReader *)commPtr->src; 
    static unsigned char eoi[2] = { 0xFF, JPEG_EOI };

    /* Insert a fake EOI marker */
    readerPtr->pub.next_input_byte = eoi;
    readerPtr->pub.bytes_in_buffer = 2;
    return TRUE;
}

static void
JpgSkipInputData(j_decompress_ptr commPtr, long numBytes)
{
    if (numBytes > 0) {
        JpgReader *readerPtr = (JpgReader *)commPtr->src; 

        if ((readerPtr->pub.next_input_byte + numBytes) >= 
            (Blt_DBuffer_Bytes(readerPtr->dBuffer) + 
             Blt_DBuffer_Length(readerPtr->dBuffer))) {
            char mesg[200];
            JpgErrorHandler *errorPtr = (JpgErrorHandler *)commPtr->err;
            
            sprintf(mesg, "short buffer: wanted %lu bytes, bytes left is %lu",
                    (unsigned long)numBytes,
                    (unsigned long)Blt_DBuffer_Length(readerPtr->dBuffer));
            Tcl_DStringAppend(&errorPtr->ds, " ", -1);
            Tcl_DStringAppend(&errorPtr->ds, mesg, -1);
            ERREXIT(commPtr, 10);
        }
        readerPtr->pub.next_input_byte += (size_t)numBytes;
        readerPtr->pub.bytes_in_buffer -= (size_t)numBytes;
    }
}

static void
JpgTermSource (j_decompress_ptr commPtr)
{
    /* Nothing to do. */
}

static void
JpgSetSourceFromBuffer(j_decompress_ptr commPtr, Blt_DBuffer buffer)
{
    JpgReader *readerPtr;
    
    /* The source object is made permanent so that a series of JPEG images
     * can be read from the same file by calling jpeg_stdio_src only before
     * the first one.  (If we discarded the buffer at the end of one image,
     * we'd likely lose the start of the next one.)  This makes it unsafe
     * to use this manager and a different source manager serially with the
     * same JPEG object.  Caveat programmer.
     */
    if (commPtr->src == NULL) {      /* First time for this JPEG object? */
        commPtr->src = (struct jpeg_source_mgr *)
            (*commPtr->mem->alloc_small) ((j_common_ptr)commPtr, 
                JPOOL_PERMANENT, sizeof(JpgReader));
        readerPtr = (JpgReader *)commPtr->src;
    }
    readerPtr = (JpgReader *)commPtr->src;
    readerPtr->dBuffer = buffer;
    readerPtr->pub.init_source = JpgInitSource;
    readerPtr->pub.fill_input_buffer = JpgFillInputBuffer;
    readerPtr->pub.skip_input_data = JpgSkipInputData;
    /* use default method */
    readerPtr->pub.resync_to_restart = jpeg_resync_to_restart; 
    readerPtr->pub.term_source = JpgTermSource;
}

static void
JpgInitDestination (j_compress_ptr commPtr)
{
    JpgWriter *writerPtr = (JpgWriter *)commPtr->dest;

    writerPtr->bytes = (JOCTET *)(*commPtr->mem->alloc_small) 
        ((j_common_ptr) commPtr, JPOOL_IMAGE, JPG_BUF_SIZE * sizeof(JOCTET));
    writerPtr->pub.next_output_byte = writerPtr->bytes;
    writerPtr->pub.free_in_buffer = JPG_BUF_SIZE;
}

static boolean
JpgEmptyOutputBuffer(j_compress_ptr commPtr)
{
    JpgWriter *writerPtr = (JpgWriter *)commPtr->dest;

    if (!Blt_DBuffer_AppendData(writerPtr->dBuffer, writerPtr->bytes, 
        JPG_BUF_SIZE)) {
        ERREXIT(commPtr, 10);
    }
    writerPtr->pub.next_output_byte = writerPtr->bytes;
    writerPtr->pub.free_in_buffer = JPG_BUF_SIZE;
    return TRUE;
}

static void
JpgTermDestination (j_compress_ptr commPtr)
{
    JpgWriter *writerPtr = (JpgWriter *)commPtr->dest;
    size_t numBytes = JPG_BUF_SIZE - writerPtr->pub.free_in_buffer;
    
    /* Write any data remaining in the buffer */
    if (numBytes > 0) {
        if (!Blt_DBuffer_AppendData(writerPtr->dBuffer, writerPtr->bytes, 
                numBytes)) {
            ERREXIT(commPtr, 10);
        }
    }
}

static void
JpgSetDestinationToBuffer(j_compress_ptr commPtr, Blt_DBuffer buffer)
{
    JpgWriter *writerPtr;
    
    /* The destination object is made permanent so that multiple JPEG
     * images can be written to the same file without re-executing
     * jpeg_stdio_dest.  This makes it dangerous to use this manager and a
     * different destination manager serially with the same JPEG object,
     * because their private object sizes may be different.  Caveat
     * programmer.
     */
    if (commPtr->dest == NULL) {        /* first time for this JPEG object? */
        commPtr->dest = (struct jpeg_destination_mgr *)
            (*commPtr->mem->alloc_small) ((j_common_ptr)commPtr, 
                        JPOOL_PERMANENT, sizeof(JpgWriter));
    }
    writerPtr = (JpgWriter *)commPtr->dest;
    writerPtr->pub.init_destination = JpgInitDestination;
    writerPtr->pub.empty_output_buffer = JpgEmptyOutputBuffer;
    writerPtr->pub.term_destination = JpgTermDestination;
    writerPtr->dBuffer = buffer;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsJpg --
 *
 *      Attempts to parse a JPG file header.
 *
 * Results:
 *      Returns 1 is the header is JPG and 0 otherwise.  Note that the
 *      validity of the header contents is not checked here.  That's done
 *      in JpgToPicture.
 *
 *---------------------------------------------------------------------------
 */
static int
IsJpg(Blt_DBuffer buffer)
{
    JpgErrorHandler error;
    struct jpeg_decompress_struct cinfo;
    int bool;

    /* Step 1: allocate and initialize JPEG decompression object */
    
    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.dct_method = JDCT_IFAST;
    cinfo.err = jpeg_std_error(&error.pub);
    error.pub.error_exit = JpgErrorProc;
    error.pub.output_message = JpgMessageProc;
    
    /* Initialize possible error message */
    bool = FALSE;
    Tcl_DStringInit(&error.ds);
    if (setjmp(error.jmpbuf)) {
        goto done;
    }
    jpeg_create_decompress(&cinfo);
    JpgSetSourceFromBuffer(&cinfo, buffer);
    bool = (jpeg_read_header(&cinfo, TRUE) == JPEG_HEADER_OK);
 done:
    jpeg_destroy_decompress(&cinfo);
    Tcl_DStringFree(&error.ds);
    return bool;
}

/*
 *---------------------------------------------------------------------------
 *
 * JpgToPicture --
 *
 *      Reads a JPEG data buffer and converts it into a picture.
 *
 * Results:
 *      The picture is returned.  If an error occured, such as the
 *      designated file could not be opened, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain
JpgToPicture(
    Tcl_Interp *interp,         /* Interpreter to report errors back to. */
    const char *fileName,       /* Name of file used to fill the dynamic
                                 * buffer.  */
    Blt_DBuffer buffer,         /* Contents of the above file. */
    JpgImportSwitches *switchesPtr)
{
    Blt_Chain chain;
    JSAMPLE **rows;
    JpgErrorHandler error;
    Picture *destPtr;
    int samplesPerRow;
    struct jpeg_decompress_struct cinfo;
    unsigned int width, height;
    Blt_Pixel *destRowPtr;
    JpgReader *readerPtr;
    
    destPtr = NULL;

    /* Step 1: allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.dct_method = switchesPtr->method;
    cinfo.err = jpeg_std_error(&error.pub);
    error.pub.error_exit = JpgErrorProc;
    error.pub.output_message = JpgMessageProc;

    /* Initialize possible error message */
    Tcl_DStringInit(&error.ds);
    Tcl_DStringAppend(&error.ds, "error reading \"", -1);
    Tcl_DStringAppend(&error.ds, fileName, -1);
    Tcl_DStringAppend(&error.ds, "\": ", -1);

    if (setjmp(error.jmpbuf)) {
    error:
        jpeg_destroy_decompress(&cinfo);
        Tcl_DStringResult(interp, &error.ds);
        return NULL;
    }

    jpeg_create_decompress(&cinfo);
    JpgSetSourceFromBuffer(&cinfo, buffer);

    readerPtr = (JpgReader *)cinfo.src;
    readerPtr->interp = interp;
    if (switchesPtr->varObjPtr != NULL) {
        readerPtr->varName = Tcl_GetString(switchesPtr->varObjPtr);
        jpeg_set_marker_processor(&cinfo, JPEG_APP0+1, JpgApp1Proc);
    }
    jpeg_read_header(&cinfo, TRUE);     /* Step 3: read file parameters */

    jpeg_start_decompress(&cinfo);      /* Step 5: Start decompressor */
    width = cinfo.output_width;
    height = cinfo.output_height;
    if ((width < 1) || (height < 1)) {
        Tcl_AppendResult(interp, "error reading \"", fileName, 
                "\": bad JPEG image size", (char *)NULL);
        return NULL;
    }
    /* JSAMPLEs per row in output buffer */
    samplesPerRow = width * cinfo.output_components;

    /* Make a one-row-high sample array that will go away when done with
     * image */
    rows = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, 
        samplesPerRow, 1);
    destPtr = Blt_CreatePicture(width, height);
    destRowPtr = destPtr->bits;
    switch (cinfo.output_components) {
    case 1:
        while (cinfo.output_scanline < height) {
            JSAMPLE *bp;
            Blt_Pixel *dp;
            int i;

            dp = destRowPtr;
            jpeg_read_scanlines(&cinfo, rows, 1);
            bp = rows[0];
            for (i = 0; i < (int)width; i++) {
                dp->Red = dp->Green = dp->Blue = *bp++;
                dp->Alpha = ALPHA_OPAQUE;
                dp++;
            }
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
    case 3:
        while (cinfo.output_scanline < height) {
            JSAMPLE *bp;
            Blt_Pixel *dp;
            int i;
            
            dp = destRowPtr;
            jpeg_read_scanlines(&cinfo, rows, 1);
            bp = rows[0];
            for (i = 0; i < (int)width; i++) {
                dp->Red = *bp++;
                dp->Green = *bp++;
                dp->Blue = *bp++;
                dp->Alpha = ALPHA_OPAQUE;
                dp++;
            }
            destRowPtr += destPtr->pixelsPerRow;
        }
        destPtr->flags |= BLT_PIC_COLOR;
        break;
    case 4:
        while (cinfo.output_scanline < height) {
            JSAMPLE *bp;
            Blt_Pixel *dp;
            int i;
            
            dp = destRowPtr;
            jpeg_read_scanlines(&cinfo, rows, 1);
            bp = rows[0];
            for (i = 0; i < (int)width; i++) {
                dp->Red = *bp++;
                dp->Green = *bp++;
                dp->Blue = *bp++;
                dp->Alpha = *bp++;
                dp++;
            }
            destRowPtr += destPtr->pixelsPerRow;
        }
        destPtr->flags |= BLT_PIC_COLOR | BLT_PIC_COMPOSITE;
        break;
    default:
        Tcl_AppendResult(interp, "\"", fileName, "\": ",
                         "don't know how to handle JPEG image with ", 
                        Blt_Itoa(cinfo.output_components), 
                        " output components.", (char *)NULL);
        Blt_FreePicture(destPtr);
        goto error;
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    if (error.pub.num_warnings > 0) {
        Tcl_SetErrorCode(interp, "PICTURE", "JPG_READ_WARNINGS", 
                Tcl_DStringValue(&error.ds), (char *)NULL);
    } else {
        Tcl_SetErrorCode(interp, "NONE", (char *)NULL);
    }
    Tcl_DStringFree(&error.ds);
    chain = Blt_Chain_Create();
    /* Opaque image, set associate colors flag.  */
    destPtr->flags |= BLT_PIC_PREMULTIPLED_COLORS;
    Blt_Chain_Append(chain, destPtr);
    destPtr->flags &= ~BLT_PIC_UNINITIALIZED;
    return chain;
}

/*
 *---------------------------------------------------------------------------
 *
 * PictureToJpg --
 *
 *      Writes a JPEG format image to the provided dynamic buffer.
 *
 * Results:
 *      A standard TCL result.  If an error occured, TCL_ERROR is returned
 *      and an error message will be place in the interpreter
 *      result. Otherwise, the dynamic buffer will contain the binary
 *      output of the image.
 *
 * Side Effects:
 *      Memory is allocated for the dynamic buffer.
 *
 *---------------------------------------------------------------------------
 */
static int
PictureToJpg(
    Tcl_Interp *interp,         /* Interpreter to report errors back to. */
    Blt_Picture original,       /* Picture source. */
    Blt_DBuffer buffer,         /* Destination buffer to contain the JPEG
                                 * image.  */
    JpgExportSwitches *switchesPtr)
{
    JpgErrorHandler error;
    int result;
    struct jpeg_compress_struct cinfo;
    Picture *srcPtr;

    result = TCL_ERROR;
    srcPtr = original;

    /* Step 1: allocate and initialize JPEG compression object */
    cinfo.err = jpeg_std_error(&error.pub);
    error.pub.error_exit = JpgErrorProc;
    error.pub.output_message = JpgMessageProc;

    /* Initialize possible error message */
    Tcl_DStringInit(&error.ds);
    Tcl_DStringAppend(&error.ds, "error writing jpg: ", -1);

    if (setjmp(error.jmpbuf)) {
        /* Transfer the error message to the interpreter result. */
        Tcl_DStringResult(interp, &error.ds);
        goto bad;
    }

    /* Now we can initialize the JPEG compression object. */
    jpeg_create_compress(&cinfo);
    
    /* Step 2: specify data destination (eg, a file) */
    
    JpgSetDestinationToBuffer(&cinfo, buffer);
    
    /* Step 3: set parameters for compression */
    
    /* First we supply a description of the input image.  Four fields of the
     * cinfo struct must be filled in:
     */
    cinfo.image_width = srcPtr->width;
    cinfo.image_height = srcPtr->height;

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
    if (srcPtr->flags & BLT_PIC_PREMULTIPLED_COLORS) {
        Blt_Picture unassoc;
        /* 
         * The picture has an alpha burned into the components.  Create a
         * temporary copy removing pre-multiplied alphas.
         */ 
        unassoc = Blt_ClonePicture(srcPtr);
        Blt_UnmultiplyColors(unassoc);
        if (srcPtr != original) {
            Blt_FreePicture(srcPtr);
        }
        srcPtr = unassoc;
    }
    Blt_QueryColors(srcPtr, (Blt_HashTable *)NULL);
    if (Blt_Picture_IsColor(srcPtr)) {
        cinfo.input_components = 3;   /* # of color components per pixel */
        cinfo.in_color_space = JCS_RGB; /* Colorspace of input image */
    } else {
        cinfo.input_components = 1;   /* # of color components per pixel */
        cinfo.in_color_space = JCS_GRAYSCALE; /* Colorspace of input image */
    }   
    jpeg_set_defaults(&cinfo);

    /* 
     * Now you can set any non-default parameters you wish to.  Here we
     * just illustrate the use of quality (quantization table) scaling:
     */

    /* limit to baseline-JPEG values */
    jpeg_set_quality(&cinfo, switchesPtr->quality, TRUE);
    if (switchesPtr->flags & PIC_PROGRESSIVE) {
        jpeg_simple_progression(&cinfo);
    }
    if (switchesPtr->smoothing > 0) {
        cinfo.smoothing_factor = switchesPtr->smoothing;
    }
    /* Step 4: Start compressor */

    jpeg_start_compress(&cinfo, TRUE);

    /* Step 5: while (scan lines remain to be written) */
    /*           jpeg_write_scanlines(...); */
    {
        int y;
        int row_stride;
        JSAMPLE *destRow;
        Blt_Pixel *srcRowPtr;
        JSAMPROW row_pointer[1];        /* pointer to JSAMPLE row[s] */

        /* JSAMPLEs per row in image_buffer */
        row_stride = srcPtr->width * cinfo.input_components;
        destRow = Blt_AssertMalloc(sizeof(JSAMPLE) * row_stride);
        srcRowPtr = srcPtr->bits;
        if (cinfo.input_components == 3) {
            for (y = 0; y < srcPtr->height; y++) {
                Blt_Pixel *sp, *send;
                JSAMPLE *dp;
                
                dp = destRow;
                for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; 
                     sp++) {
                    dp[0] = sp->Red;
                    dp[1] = sp->Green;
                    dp[2] = sp->Blue;
                    dp += 3;
                }
                row_pointer[0] = destRow;
                jpeg_write_scanlines(&cinfo, row_pointer, 1);
                srcRowPtr += srcPtr->pixelsPerRow;
            }
        } else {
            for (y = 0; y < srcPtr->height; y++) {
                Blt_Pixel *sp, *send;
                JSAMPLE *dp;
                
                dp = destRow;
                for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; 
                     sp++) {
                    *dp++ = sp->Red;
                }
                row_pointer[0] = destRow;
                jpeg_write_scanlines(&cinfo, row_pointer, 1);
                srcRowPtr += srcPtr->pixelsPerRow;
            }
        }
        Blt_Free(destRow);
    }
    /* Step 6: Finish compression */
    jpeg_finish_compress(&cinfo);
    result = TCL_OK;
 bad:
    /* Step 7: release JPEG compression object */
    jpeg_destroy_compress(&cinfo);

    if (error.pub.num_warnings > 0) {
        Tcl_SetErrorCode(interp, "PICTURE", "JPG_WRITE_WARNINGS", 
                Tcl_DStringValue(&error.ds), (char *)NULL);
    } else {
        Tcl_SetErrorCode(interp, "NONE", (char *)NULL);
    }
    Tcl_DStringFree(&error.ds);
    if (srcPtr != original) {
        Blt_FreePicture(srcPtr);
    }
    return result;
}

static Blt_Chain
ReadJpg(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer)
{
    JpgImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    return JpgToPicture(interp, fileName, dbuffer, &switches);
}

static Tcl_Obj *
WriteJpg(Tcl_Interp *interp, Blt_Picture picture)
{
    Tcl_Obj *objPtr;
    Blt_DBuffer dbuffer;
    JpgExportSwitches switches;

    /* Default export switch settings. */
    memset(&switches, 0, sizeof(switches));
    switches.quality = 100;
    switches.smoothing = 0;
    switches.flags = 0;                /* No progressive or compression. */
    switches.bg.u32 = 0xFFFFFFFF;      /* White */

    objPtr = NULL;
    dbuffer = Blt_DBuffer_Create();
    if (PictureToJpg(interp, picture, dbuffer, &switches) == TCL_OK) {
        objPtr = Blt_DBuffer_Base64EncodeToObj(dbuffer);
    }
    Blt_DBuffer_Destroy(dbuffer);
    return objPtr;
}

static Blt_Chain
ImportJpg(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, 
          const char **fileNamePtr)
{
    Blt_DBuffer dbuffer;
    Blt_Chain chain;
    const char *string;
    JpgImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    switches.method = JDCT_ISLOW;       /* Default method. */
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
    chain = JpgToPicture(interp, string, dbuffer, &switches);
 error:
    Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
    Blt_DBuffer_Destroy(dbuffer);
    return chain;
}

static int
ExportJpg(Tcl_Interp *interp, int index, Blt_Chain chain, int objc,
          Tcl_Obj *const *objv)
{
    Blt_DBuffer dbuffer;
    Blt_Picture picture;
    JpgExportSwitches switches;
    int result;

    /* Default export switch settings. */
    memset(&switches, 0, sizeof(switches));
    switches.quality = 100;
    switches.smoothing = 0;
    switches.flags = 0;                /* No progressive or compression. */
    switches.bg.u32 = 0xFFFFFFFF;      /* White */
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
    if (switches.quality == 0) {
        switches.quality = 100;         /* Default quality setting. */
    } else if (switches.quality > 100) {
        switches.quality = 100;         /* Maximum quality setting. */
    }
    if (switches.smoothing > 100) {
        switches.smoothing = 100;       /* Maximum smoothing setting. */
    }

    dbuffer = Blt_DBuffer_Create();
    result = PictureToJpg(interp, picture, dbuffer, &switches);
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
Blt_PictureJpgInit(Tcl_Interp *interp)
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
    if (Tcl_PkgProvide(interp, "blt_picture_jpg", BLT_VERSION) != TCL_OK) { 
        return TCL_ERROR;
    }
    return Blt_PictureRegisterFormat(interp,
        "jpg",                          /* Name of format. */
        IsJpg,                          /* Format discovery procedure. */
        ReadJpg,                        /* Read format procedure. */
        WriteJpg,                       /* Write format procedure. */
        ImportJpg,                      /* Import format procedure. */
        ExportJpg);                     /* Export format procedure. */
}

int 
Blt_PictureJpgSafeInit(Tcl_Interp *interp) 
{
    return Blt_PictureJpgInit(interp);
}

#endif /* HAVE_LIBJPG */


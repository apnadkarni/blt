/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictPdf.c --
 *
 * This module implements Portable Document Format (PDF) file conversion
 * routines for the picture image type in the BLT toolkit.
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

/* TODO:
 *      o fix margin padding, default should be 0.  (translate)
 *      o handle -maxpect flag. (Scale)
 *      x fix date.
 *      x only write one image on one page.
 *      x test greyscale images.
 *      x add softmask for alpha channel.
 */
#include "bltInt.h"
#include <unistd.h>

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_TIME_H
  #include <time.h>
#endif /* HAVE_TIME_H */

#ifdef HAVE_SYS_TIME_H
  #include <sys/time.h>
#endif

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif /*HAVE_UNISTD_H*/

#ifdef HAVE_UNISTD_H
  #include <stdlib.h>
#endif /*HAVE_UNISTD_H*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <bltSwitch.h>
#include <bltChain.h>
#include "bltPictFmts.h"
#include "bltPs.h"
#include "bltWait.h"

extern Tcl_AppInitProc Blt_PicturePdfInit;
extern Tcl_AppInitProc Blt_PicturePdfInit;

#define UCHAR(c)        ((unsigned char) (c))
#define ISASCII(c)      (UCHAR(c)<=0177)
#define MIN(a,b)        (((a)<(b))?(a):(b))

#define div257(t)       (((t)+((t)>>8))>>8)
#define GetBit(x)        srcRowPtr[(x>>3)] &  (0x80 >> (x&7))

#define MAXCOLORS       256
#define BUFFER_SIZE (1<<16)

enum PbmVersions {
    PBM_UNKNOWN,
    PBM_PLAIN,                          /* Monochrome: 1-bit per pixel */
    PGM_PLAIN,                          /* 8-bits per pixel */
    PPM_PLAIN,                          /* 24-bits per pixel */
    PBM_RAW,                            /* 1-bit per pixel */
    PGM_RAW,                            /* 8/16-bits per pixel */
    PPM_RAW                             /* 24/48 bits per pixel */
};
    
typedef struct {
    unsigned int *xref;                 /* Array of object numbers. */
    int numObjects;                     /* # of slots in above array. */
    Blt_DBuffer dbuffer;                /* Holds output (PDF file). */
} Pdf;
    
#ifdef notdef
static const char *pbmFormat[] = {
    "???", 
    "pbmplain",
    "pgmplain",
    "ppmplain",
    "pbmraw",
    "pgmraw",
    "ppmraw",
};
#endif

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    int flags;                          /* Flag. */
    Blt_Pixel bg;
    PageSetup setup;
    int index;
} PdfExportSwitches;

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    int dpi;                            /* Dots per inch. */
    const char *paperSize;              /* Papersize. */
    int crop;
} PdfImportSwitches;

#define PDF_CROP                (1<<0)
#define EXPORT_ALPHA            (1<<1)

typedef struct {
    unsigned int width, height;         /* Dimensions of the image. */
    unsigned int bitsPerPixel;          /* # bits per pixel. */
    unsigned char *data;                /* Start of raw data */
    unsigned int bytesPerRow;
    Blt_DBuffer dbuffer;
} Pbm;


static Blt_SwitchParseProc ColorSwitchProc;
static Blt_SwitchCustom colorSwitch = {
    ColorSwitchProc, NULL, NULL, (ClientData)0
};

#ifdef notdef
static Blt_SwitchParseProc PicaSwitchProc;
static Blt_SwitchCustom picaSwitch = {
    PicaSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc PadSwitchProc;
static Blt_SwitchCustom padSwitch = {
    PadSwitchProc, NULL, NULL, (ClientData)0
};
#endif

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_BITS_NOARG, "-alpha", "", (char *)NULL,
        Blt_Offset(PdfExportSwitches, flags),   0, EXPORT_ALPHA},
    {BLT_SWITCH_CUSTOM,  "-background",         "color", (char *)NULL,
        Blt_Offset(PdfExportSwitches, bg),         0, 0, &colorSwitch},
    {BLT_SWITCH_OBJ,     "-data",       "varName", (char *)NULL,
        Blt_Offset(PdfExportSwitches, dataObjPtr),  0},
    {BLT_SWITCH_OBJ,     "-file",       "fileName", (char *)NULL,
        Blt_Offset(PdfExportSwitches, fileObjPtr),  0},
    {BLT_SWITCH_LISTOBJ, "-comments", "{key value...}", (char *)NULL,
        Blt_Offset(PdfExportSwitches, setup.cmtsObjPtr),
        BLT_SWITCH_NULL_OK},
    {BLT_SWITCH_INT_NNEG, "-index", "int", (char *)NULL,
        Blt_Offset(PdfExportSwitches, index), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec importSwitches[] =
{
    {BLT_SWITCH_OBJ,     "-data",       "data", (char *)NULL,
        Blt_Offset(PdfImportSwitches, dataObjPtr),  0},
    {BLT_SWITCH_INT,      "-dpi",       "number", (char *)NULL,
        Blt_Offset(PdfImportSwitches, dpi), 0},
    {BLT_SWITCH_OBJ,     "-file",       "fileName", (char *)NULL,
        Blt_Offset(PdfImportSwitches, fileObjPtr),  0},
    {BLT_SWITCH_BITS_NOARG,  "-nocrop",    "", (char *)NULL,
        Blt_Offset(PdfImportSwitches, crop), 0, FALSE},
    {BLT_SWITCH_STRING,  "-papersize",  "string", (char *)NULL,
        Blt_Offset(PdfImportSwitches, paperSize),   0},
    {BLT_SWITCH_END}
};

DLLEXPORT extern Tcl_AppInitProc Blt_PicturePdfInit;
DLLEXPORT extern Tcl_AppInitProc Blt_PicturePdfSafeInit;

#ifdef WIN32

typedef HANDLE Pipe;

typedef struct {
    DWORD pid;
    HANDLE hProcess;
} ProcessId;

#else
typedef int Pipe;
typedef pid_t ProcessId;
#endif

#ifdef WIN32
  #define kill                    KillProcess
  #define waitpid                 WaitProcess
#endif  /* WIN32 */

#define TRUE    1
#define FALSE   0

typedef struct _Blt_Picture Picture;

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

/*#ifdef WIN32*/
extern char *strptime(const char *buf, const char *fmt, struct tm *tm);
/*#endif*/

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
ColorSwitchProc(ClientData clientData, Tcl_Interp *interp,
                const char *switchName, Tcl_Obj *objPtr, char *record,
                int offset, int flags)  
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

#ifdef TIME_WITH_SYS_TIME
  #include <sys/time.h>
  #include <time.h>
#else
  #ifdef HAVE_SYS_TIME_H
    #include <sys/time.h>
  #else
    #include <time.h>
  #endif /* HAVE_SYS_TIME_H */
#endif /* TIME_WITH_SYS_TIME */

#ifdef notdef

/*
 *--------------------------------------------------------------------------
 *
 * PicaSwitchProc --
 *
 *      Convert a Tcl_Obj list of 2 or 4 numbers into representing a bounding
 *      box structure.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *--------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PicaSwitchProc(ClientData clientData, Tcl_Interp *interp,
               const char *switchName, Tcl_Obj *objPtr, char *record,
               int offset, int flags)  
{
    int *picaPtr = (int *)(record + offset);
    
    return Blt_Ps_GetPicaFromObj(interp, objPtr, picaPtr);
}

/*
 *--------------------------------------------------------------------------
 *
 * PadSwitchProc --
 *
 *      Convert a Tcl_Obj list of 2 or 4 numbers into representing a
 *      bounding box structure.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *--------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PadSwitchProc(ClientData clientData, Tcl_Interp *interp, const char *switchName,
              Tcl_Obj *objPtr, char *record, int offset, int flags)  
{
    Blt_Pad *padPtr = (Blt_Pad *)(record + offset);
    
    return Blt_Ps_GetPadFromObj(interp, objPtr, padPtr);
}
#endif

static char *
PbmComment(char *bp) 
{
    char *p;

    p = bp;
    if (*p == '#') {
        /* Find end of comment line. */
        while((*p != '\n') && (*p != '\0')) {
            p++;
        }
    }
    return p;
}

static INLINE int 
PbmGetShort(unsigned char *bp) {
    return (bp[0] << 8) + bp[1];
}

static Picture *
PbmRawData(Pbm *pbmPtr)
{
    Picture *destPtr;

    destPtr = Blt_CreatePicture(pbmPtr->width, pbmPtr->height);
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
    Blt_DBuffer_SetPointer(pbmPtr->dbuffer, pbmPtr->data + 
                           (pbmPtr->height * pbmPtr->bytesPerRow));
    destPtr->flags &= ~BLT_PIC_UNINITIALIZED;
    return destPtr;
}

static Blt_Picture
PbmToPicture(Tcl_Interp *interp, Blt_DBuffer dbuffer)
{
    char *bp, *p;
    int isRaw;
    int version;
    size_t size, want;
    unsigned char *start;
    Pbm pbm;

    size = Blt_DBuffer_BytesLeft(dbuffer);
    start = Blt_DBuffer_Pointer(dbuffer);
    if (size < 14) {
        Tcl_AppendResult(interp, "can't read PBM bitmap: short file", 
                         (char *)NULL);
        return NULL;
    }
    bp = (char *)start;
    if ((bp[0] != 'P') || (bp[1] < '1') || (bp[1] > '6')) {
        Tcl_AppendResult(interp, "unknown PBM bitmap header", (char *)NULL);
        return NULL;
    }
    version = bp[1] - '0';
    isRaw = (version > 2);
    switch(version) {   
    case PBM_PLAIN:                     /* P2 */
    case PBM_RAW:                       /* P5 */
        pbm.bitsPerPixel = 8;
        break;
    case PGM_PLAIN:                     /* P1 */
    case PGM_RAW:                       /* P4 */
        pbm.bitsPerPixel = 1;
        break;
    case PPM_PLAIN:                     /* P3 */
    case PPM_RAW:                       /* P6 */
        pbm.bitsPerPixel = 24;
        break;
    default:
        Tcl_AppendResult(interp, "unknown PBM version \"", Blt_Itoa(version),
                         "\"", (char *)NULL);
        return NULL;
    }
    if (!isspace(bp[2])) {
        Tcl_AppendResult(interp, "no white space after version in pbm header.", 
                         (char *)NULL);
        return NULL;
    }
    p = bp + 3;
    if (*p == '#') {
        p = PbmComment(p);
    }
    pbm.width = strtoul(p, &p, 10);
    if (pbm.width == 0) {
        Tcl_AppendResult(interp, "bad width specification ", bp+3, ".", 
                         (char *)NULL);
        return NULL;
    }
    if (!isspace(*p)) {
        Tcl_AppendResult(interp, "no white space after width in pbm header.", 
                (char *)NULL);
        return NULL;
    }
    p++;
    if (*p == '#') {
        p = PbmComment(p);
    }
    pbm.height = strtoul(p, &p, 10);
    if (pbm.height == 0) {
        Tcl_AppendResult(interp, "bad height specification", (char *)NULL);
        return NULL;
    }
    if (!isspace(*p)) {
        Tcl_AppendResult(interp, "no white space after height in header.", 
                         (char *)NULL);
        return NULL;
    }
    p++;
    if (*p == '#') {
        p = PbmComment(p);
    }
    if (pbm.bitsPerPixel != 1) {
        unsigned int maxval;            /* Maximum intensity allowed. */

        maxval = strtoul(p, &p, 10);
        if (maxval == 0) {
            Tcl_AppendResult(interp, "bad maxval specification", (char *)NULL);
            return NULL;
        }
        if (!isspace(*p)) {
            Tcl_AppendResult(interp, "no white space after maxval pbm header.", 
                             (char *)NULL);
            return NULL;
        }
        p++;
        if (*p == '#') {
            p = PbmComment(p);
        }
        if (maxval >= USHRT_MAX) {
            Tcl_AppendResult(interp, "invalid maxval specification", 
                             (char *)NULL);
            return NULL;
        }
        if (maxval > 255) {
            pbm.bitsPerPixel <<= 1;     /* 16-bit greyscale or 48 bit
                                         * color. */
        }
    }
    pbm.data = (unsigned char *)p;
    pbm.dbuffer = dbuffer;
    pbm.bytesPerRow = ((pbm.bitsPerPixel * pbm.width) + 7) / 8;
    want = (pbm.data - start) + pbm.height * pbm.bytesPerRow;
    if ((isRaw) && (want > Blt_DBuffer_BytesLeft(dbuffer))) {
        Tcl_AppendResult(interp, "short pbm file", (char *)NULL);
        return NULL;
    }       
    if (!isRaw) {
        Tcl_AppendResult(interp, "expected raw pbm file", (char *)NULL);
        return NULL;
    }
    return PbmRawData(&pbm);
}

#ifdef WIN32

typedef struct {
    HANDLE hFile;
    Blt_DBuffer dbuffer;
    int lastError;
} PdfWriter;

/*
 *---------------------------------------------------------------------------
 *
 * WriteBufferProc --
 *
 *      This function runs in a separate thread and write the data buffer
 *      as input to the ghostscript process.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Writes the buffer (PDF file) to the ghostscript standard input file
 *      descriptor.
 *
 *---------------------------------------------------------------------------
 */
static DWORD WINAPI
WriteBufferProc(void *clientData)
{
    PdfWriter *writerPtr = clientData;
    const unsigned char *bp;
    int bytesLeft;
    HANDLE hFile;
    DWORD count;

    hFile = writerPtr->hFile;
    bp = Blt_DBuffer_Bytes(writerPtr->dbuffer);
    for (bytesLeft = Blt_DBuffer_Length(writerPtr->dbuffer); bytesLeft > 0;
         bytesLeft -= count) {
        if (!WriteFile(hFile, bp, bytesLeft, &count, NULL)) {
            writerPtr->lastError = GetLastError();
            break;
        }
        bp += count;
    }
    Blt_DBuffer_Destroy(writerPtr->dbuffer);
    CloseHandle(hFile);
    if (bytesLeft > 0) {
        ExitThread(1);
    }
    ExitThread(0);
    /* NOTREACHED */
    return 0;
}

static PdfWriter writer;

static HANDLE
WriteToGhostscript(Tcl_Interp *interp, void *filePtr, Blt_DBuffer dbuffer)
{
    HANDLE hThread;
    ClientData clientData;
    DWORD id;
    
    writer.dbuffer = Blt_DBuffer_Create();
    Blt_DBuffer_Init(writer.dbuffer);
    /* Copy the input to a new buffer. */
    Blt_DBuffer_Concat(writer.dbuffer, dbuffer);
    writer.hFile = filePtr;
    writer.lastError = 0;
    clientData = &writer;
    hThread = CreateThread(
        NULL,                           /* Security attributes */
        8000,                           /* Initial stack size. */
        WriteBufferProc,                /* Address of thread routine */
        clientData,                     /* One-word of data passed to
                                         * routine. */
        0,                              /* Creation flags */
        &id);                           /* (out) Will contain Id of new
                                         * thread. */
    return hThread;
}


static int
ReadFromGhostscript(Tcl_Interp *interp, Pipe pipe, Blt_DBuffer dbuffer)
{
    DWORD numBytes;
    HANDLE hFile = pipe;
    int result;

    Blt_DBuffer_Free(dbuffer);
    numBytes = 0;
    for (;;) {
        DWORD numRead;
        unsigned char *bp;

        bp = Blt_DBuffer_Extend(dbuffer, BUFFER_SIZE);
        if (!ReadFile(hFile, (char *)bp, BUFFER_SIZE, &numRead, NULL)) {
            DWORD err;

            err = GetLastError();
            if ((err != ERROR_BROKEN_PIPE) && (err != ERROR_HANDLE_EOF)) {
                Tcl_AppendResult(interp, "error reading from ghostscript: ",
                                 Blt_LastError(), (char *)NULL);
                result = TCL_ERROR;
                break;
            }
        }
        if (numRead == 0) {
            result = TCL_OK;
            break;                      /* EOF */
        }
        numBytes += numRead;
        Blt_DBuffer_SetLength(dbuffer, numBytes);
    }
    Blt_DBuffer_SetLength(dbuffer, numBytes);
    CloseHandle(hFile);
    return result;
}

#else  /* WIN32 */

static pid_t
WriteToGhostscript(Tcl_Interp *interp, Pipe pipe, Blt_DBuffer dbuffer)
{
    pid_t child;
    int fd = pipe;
    
    child = fork();
    if (child == -1) {
        Tcl_AppendResult(interp, "can't fork process: ", Tcl_PosixError(interp),
                         (char *)NULL);
        return 0;
    } else if (child > 0) {
        close(fd);
        return child;
    } else {
        const unsigned char *bytes;
        size_t numBytes;
        ssize_t numWritten;

        bytes = Blt_DBuffer_Bytes(dbuffer);
        numBytes = Blt_DBuffer_Length(dbuffer);
        numWritten = write(fd, bytes, numBytes);
        close(fd);
        if (numWritten != numBytes) {
            exit(1);
        }
        exit(0);
    }
}

static int
ReadFromGhostscript(Tcl_Interp *interp, Pipe pipe, Blt_DBuffer dbuffer)
{
    int numBytes;
    int fd = pipe;
    
    Blt_DBuffer_Free(dbuffer);
    numBytes = 0;
    for (;;) {
        char *bp;
        int numRead;

        bp = (char *)Blt_DBuffer_Extend(dbuffer, BUFFER_SIZE);
        numRead = read(fd, bp, BUFFER_SIZE);
        if (numRead == 0) {
            break;              /* EOF */
        } else if (numRead < 0) {
            Tcl_AppendResult(interp, "error reading from ghostscript: ",
                             Tcl_PosixError(interp), (char *)NULL);
            return TCL_ERROR;
        }

        numBytes += numRead;
        Blt_DBuffer_SetLength(dbuffer, numBytes);
    }
    Blt_DBuffer_SetLength(dbuffer, numBytes);
    close(fd);
    return TCL_OK;
}

#endif  /* WIN32 */

static int
PdfToPbm(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer,
         PdfImportSwitches *switchesPtr)
{
    Pipe inPipe, outPipe;               /* File descriptors for ghostscript
                                         * subprocess. */
    char string1[200];
    char string2[200];
    int numPids;
    Blt_Pid *pids;
    int result;
    const char *args[] = {
        "gs",                           /* Ghostscript command */
        "-dEPSCrop",                    /* (optional) crop page to bbox  */
        "-dSAFER",                      /*  */
        "-q",                           /* Quiet mode.  No GS messages.  */
        "-sDEVICE=ppmraw",              /* Format is PPM raw */
        "-dBATCH",                      /* Batch mode. No "quit" necessary. */
        "-sPAPERSIZE=letter",           /* (optional) Specify paper size. */
        "-r100x100",                    /* (optional) Specify DPI of screen */
        "-dNOPAUSE",                    /*  */
        "-sOutputFile=-",               /* Output file is stdout. */
        "-",
        NULL
    };

    args[1] = (switchesPtr->crop) ? "-dEPSCrop" : "-dSAFER";
    {
        Tk_Window tkwin;
        int xdpi, ydpi;

        tkwin = Tk_MainWindow(interp);
        if (switchesPtr->dpi > 0) {
            xdpi = ydpi = switchesPtr->dpi;
        } else {
            Blt_ScreenDPI(tkwin, &xdpi, &ydpi);
        }
        sprintf(string1, "-r%dx%d", xdpi, ydpi);
        args[7] = string1;
    }
    if (switchesPtr->paperSize != NULL) {
        sprintf(string2, "-sPAPERSIZE=%s", switchesPtr->paperSize);
        args[6] = string2;
    }
    {
        int i;
        Tcl_Obj *objv[11];
        int objc = 11;
        const char **p;

        for (i = 0, p = args; *p != NULL; p++, i++) {
            objv[i] = Tcl_NewStringObj(*p, -1);
            Tcl_IncrRefCount(objv[i]);
        }
        numPids = Blt_CreatePipeline(interp, objc, objv, &pids, &inPipe,
                &outPipe, (int *)NULL, NULL);
        for (i = 0; i < objc; i++) {
            Tcl_DecrRefCount(objv[i]);
        }
    }
    if (numPids < 0) {
        return TCL_ERROR;
    }
    Blt_DetachPids(numPids, pids);
    Blt_Free(pids);
#ifdef WIN32
    {
        HANDLE hThread;
        
        hThread = WriteToGhostscript(interp, inPipe, dbuffer);
        if (hThread == NULL) {
            return TCL_ERROR;
        }
        result = ReadFromGhostscript(interp, outPipe, dbuffer);
        CloseHandle(hThread);
    }
#else
    {
        pid_t child;

        child = WriteToGhostscript(interp, inPipe, dbuffer);
        if (child == 0) {
            return TCL_ERROR;
        }
        result = ReadFromGhostscript(interp, outPipe, dbuffer);
    }
#endif
    Tcl_ReapDetachedProcs();
#ifdef notdef
    Blt_DBuffer_SaveFile(interp, "junk.ppm", dbuffer);
#endif
    if (result != TCL_OK) {
        Blt_DBuffer_Free(dbuffer);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static Blt_Chain
PdfToPicture(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer,
             PdfImportSwitches *switchesPtr)
{
    Blt_Chain chain;

    if (PdfToPbm(interp, fileName, dbuffer, switchesPtr) != TCL_OK) {
        return NULL;
    }
    /* Can be more than one image in buffer. Save each picture in a list. */
    chain = Blt_Chain_Create();
    while (Blt_DBuffer_BytesLeft(dbuffer) > 0) {
        Blt_Picture picture;

        picture = PbmToPicture(interp, dbuffer);
        if (picture == NULL) {
            Blt_Chain_Destroy(chain);
            return NULL;
        }
        Blt_Chain_Append(chain, picture);
    }
    return chain;
}

enum PdfObjects {
    OBJ_NULL,    
    OBJ_CATALOG,
    OBJ_INFO,
    OBJ_PAGES,
    OBJ_PAGE,
    OBJ_RESOURCES,
    OBJ_CONTENT,
    OBJ_CONTENT_LENGTH,
    OBJ_IMAGE,
    OBJ_IMAGE_LENGTH,
    OBJ_SOFTMASK,
    OBJ_SOFTMASK_LENGTH,
};

#define MAX_NUM_OBJS    OBJ_SOFTMASK_LENGTH

static Pdf *
NewPdf()
{
    Pdf *pdfPtr;

    pdfPtr = Blt_AssertCalloc(1, sizeof(Pdf));
    pdfPtr->xref = Blt_AssertCalloc(MAX_NUM_OBJS, sizeof(int));
    pdfPtr->numObjects = OBJ_IMAGE_LENGTH;
    pdfPtr->dbuffer = Blt_DBuffer_Create();
    return pdfPtr;
}

static void
FreePdf(Pdf *pdfPtr)
{
    Blt_Free(pdfPtr->xref);
    Blt_DBuffer_Destroy(pdfPtr->dbuffer);
    Blt_Free(pdfPtr);
}

static void
AddComments(Pdf *pdfPtr, Tcl_Obj *objPtr)
{
    Tcl_Obj **objv;
    int objc;
    int i;
    
    Tcl_ListObjGetElements(NULL, objPtr, &objc, &objv);
    for (i = 0; i < objc; i += 2) {
        if ((i+1) == objc) {
            break;
        }
        Blt_DBuffer_Format(pdfPtr->dbuffer, "%% %s: %s\n",
                Tcl_GetString(objv[i]), Tcl_GetString(objv[i+1]));
    }
}

/*
 * image object 0
 * LENGTH stream object 1
 * PAGE object 2
 * TRANS object 3 /Contents
 * PAGES        numObjects - 2
 * CATALOG      numObjects - 1
 * INFO         numObjects 
 * XREF         0
 */

#define SetXRefOffset(p,id) \
    ((p)->xref[(id) - 1] = Blt_DBuffer_Length((p)->dbuffer));

static int 
PictureToPdf(Tcl_Interp *interp, Blt_Picture original, Pdf *pdfPtr, 
             PdfExportSwitches *exportPtr)
{
    PageSetup *setupPtr = &exportPtr->setup;
    Picture *srcPtr;
    char date[200];
    const char *colorSpace, *version;
    int i, numComponents;
    size_t length, offset;
    struct tm *tmPtr;
    time_t ticks;
    unsigned char *imgData;

    srcPtr = original;
    Blt_DBuffer_VarAppend(pdfPtr->dbuffer, 
                          "%PDF-1.4\n",
                          (char *)NULL);

    if (setupPtr->cmtsObjPtr != NULL) {
        AddComments(pdfPtr, setupPtr->cmtsObjPtr);
    }

    version = Tcl_GetVar(interp, "blt_version", TCL_GLOBAL_ONLY);
    if (version == NULL) {
        version = "???";
    }
    ticks = time((time_t *) NULL);
    tmPtr = gmtime(&ticks);
    strftime(date, 200, "D:%Y%m%d%H%M%SZ", tmPtr);

    /* Catalog object */
    SetXRefOffset(pdfPtr, OBJ_CATALOG);
    Blt_DBuffer_Format(pdfPtr->dbuffer, 
                       "%d 0 obj\n"
                       "  <<\n"
                       "    /Type /Catalog\n"
                       "    /Pages %d 0 R\n"
                       "  >>\n"
                       "endobj\n", 
                       OBJ_CATALOG, OBJ_PAGES);

    /* Information object */
    SetXRefOffset(pdfPtr, OBJ_INFO);
    Blt_DBuffer_Format(pdfPtr->dbuffer,
                       "%d 0 obj\n"
                       "  <<\n"
                       "    /CreationDate (%s)\n"
                       "    /Producer (BLT %s Picture)\n"
                       "  >>\n"
                       "endobj\n", 
                       OBJ_INFO, 
                       date, 
                       version);

    /* Pages object */
    SetXRefOffset(pdfPtr, OBJ_PAGES);
    Blt_DBuffer_Format(pdfPtr->dbuffer, 
                       "%d 0 obj\n"
                       "  <<\n"
                       "    /Type /Pages\n"
                       "    /Kids [ %d 0 R ]\n"
                       "    /Count 1\n"
                       "  >>\n"
                       "endobj\n", 
                       OBJ_PAGES,
                       OBJ_PAGE); 

    Blt_Ps_ComputeBoundingBox(setupPtr, srcPtr->width, srcPtr->height);

    SetXRefOffset(pdfPtr, OBJ_PAGE);
    /* Page object. */
    Blt_DBuffer_Format(pdfPtr->dbuffer, 
                       "%d 0 obj\n" 
                       "  <<\n" 
                       "    /Type /Page\n"
                       "    /Parent %d 0 R\n"
                       "    /Resources %d 0 R\n"
                       "    /MediaBox [ 0 0 %d %d ]\n"
                       "    /Contents %d 0 R\n"
                       "  >>\n"
                       "endobj\n", 
                       OBJ_PAGE, 
                       OBJ_PAGES,
                       OBJ_RESOURCES,
                       setupPtr->right - setupPtr->left, 
                       setupPtr->top - setupPtr->bottom,
                       OBJ_CONTENT);

    SetXRefOffset(pdfPtr, OBJ_RESOURCES);
    /* Resource dictionary */
    Blt_DBuffer_Format(pdfPtr->dbuffer, 
                       "%d 0 obj\n" 
                       "  <<\n" 
                       "    /ProcSet [ /PDF /ImageC /ImageB ]\n"
                       "    /XObject << /Im1 %d 0 R >>\n"
                       "  >>\n"
                       "endobj\n", 
                       OBJ_RESOURCES,
                       OBJ_IMAGE);
    
    /* Content object.  */
    SetXRefOffset(pdfPtr, OBJ_CONTENT);
    Blt_DBuffer_Format(pdfPtr->dbuffer, 
                       "%d 0 obj\n"
                       "  <<\n"
                       "    /Length %d 0 R\n"
                       "  >>\n"
                       "stream\n",
                       OBJ_CONTENT,
                       OBJ_CONTENT_LENGTH);
    length = Blt_DBuffer_Format(pdfPtr->dbuffer, 
                                " q\n" 
                                "   1 0 0 1 0 0 cm\n"  /* Translate */
                                "   %d 0 0 %d 0 0 cm\n"  /* Scale */
                                "   /Im1 Do\n"
                                " Q\n", 
                                setupPtr->right - setupPtr->left, 
                                setupPtr->top - setupPtr->bottom);
    Blt_DBuffer_VarAppend(pdfPtr->dbuffer, 
                          "endstream\n"
                          "endobj\n", (char *)NULL);

    /* Length of content stream. */
    SetXRefOffset(pdfPtr, OBJ_CONTENT_LENGTH);
    Blt_DBuffer_Format(pdfPtr->dbuffer, 
                       "%d 0 obj\n" 
                       "  %ld\n"
                       "endobj\n", 
                       OBJ_CONTENT_LENGTH,
                       length);

    Blt_ClassifyPicture(srcPtr);
    if ((!Blt_Picture_IsOpaque(srcPtr)) &&
        ((exportPtr->flags & EXPORT_ALPHA) == 0)) {
        Blt_Picture background;
        
        background = Blt_CreatePicture(srcPtr->width, srcPtr->height);
        Blt_BlankPicture(background, exportPtr->bg.u32);
        Blt_CompositePictures(background, srcPtr);
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
    if (Blt_Picture_IsGreyscale(srcPtr)) {
        colorSpace = "DeviceGray";
        numComponents = 1;
    } else {
        colorSpace = "DeviceRGB";
        numComponents = 3;
    }

    /* Transfer image data to buffer. */
    imgData = Blt_AssertMalloc(srcPtr->width * srcPtr->height * numComponents);
    if (Blt_Picture_IsGreyscale(srcPtr)) {
        Blt_Pixel *srcRowPtr;
        int y;
        unsigned char *dp;

        srcRowPtr = srcPtr->bits;
        dp = imgData;
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *send;

            for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
                *dp++ = sp->Red;
            }
            srcRowPtr += srcPtr->pixelsPerRow;
        }
    } else {
        Blt_Pixel *srcRowPtr;
        unsigned char *dp;
        int y;

        srcRowPtr = srcPtr->bits;
        dp = imgData;
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *send;

            for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
                dp[0] = sp->Red;
                dp[1] = sp->Green;
                dp[2] = sp->Blue;
                dp += 3;
            }
            srcRowPtr += srcPtr->pixelsPerRow;
        }
    }

    /* Image object */
    SetXRefOffset(pdfPtr, OBJ_IMAGE);
    Blt_DBuffer_Format(pdfPtr->dbuffer, 
                       "%d 0 obj\n"
                       "  <<\n"
                       "    /Type /XObject\n"
                       "    /Subtype /Image\n"
                       "    /Width %d\n"
                       "    /Height %d\n"
                       "    /ColorSpace /%s\n"
                       "    /BitsPerComponent 8\n"
                       "    /Filter /ASCII85Decode\n",
                       OBJ_IMAGE, 
                       srcPtr->width, srcPtr->height, colorSpace);
    
    if (!Blt_Picture_IsOpaque(srcPtr)) {
        Blt_DBuffer_Format(pdfPtr->dbuffer, 
                           "    /SMask %d 0 R\n",
                           OBJ_SOFTMASK);
    }
    Blt_DBuffer_Format(pdfPtr->dbuffer, 
                       "    /Name /Im1\n"
                       "    /Length %d 0 R\n" 
                       "  >>\n",
                       OBJ_IMAGE_LENGTH);
    
    Blt_DBuffer_VarAppend(pdfPtr->dbuffer, "stream\n", (char *)NULL);
    Blt_DBuffer_AppendBase85(pdfPtr->dbuffer, imgData, 
        srcPtr->width * srcPtr->height * numComponents);
    length = Blt_DBuffer_Length(pdfPtr->dbuffer);
    Blt_DBuffer_VarAppend(pdfPtr->dbuffer, 
                          "\n"
                          "endstream\n", 
                          "endobj\n", (char *)NULL);

    /* Length of image stream object. */
    SetXRefOffset(pdfPtr, OBJ_IMAGE_LENGTH);
    Blt_DBuffer_Format(pdfPtr->dbuffer, 
                       "%d 0 obj\n" 
                       "  %ld\n"
                       "endobj\n", 
                       OBJ_IMAGE_LENGTH,
                       length);
    Blt_Free(imgData);

    if ((!Blt_Picture_IsOpaque(srcPtr)) && (exportPtr->flags & EXPORT_ALPHA)) {
        unsigned char *maskData, *dp;
        Blt_Pixel *srcRowPtr;
        int y;

        maskData = Blt_AssertMalloc(srcPtr->width * srcPtr->height);
        srcRowPtr = srcPtr->bits;
        dp = maskData;
        for (y = 0; y < srcPtr->height; y++) {
            Blt_Pixel *sp, *send;

            for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
                *dp++ = sp->Alpha;
            }
            srcRowPtr += srcPtr->pixelsPerRow;
        }
        /* Softmask object */
        SetXRefOffset(pdfPtr, OBJ_SOFTMASK);
        Blt_DBuffer_Format(pdfPtr->dbuffer, 
                           "%d 0 obj\n"
                           "  <<\n"
                           "    /Type /XObject\n"
                           "    /Subtype /Image\n"
                           "    /Width %d\n"
                           "    /Height %d\n"
                           "    /ColorSpace /DeviceGray\n"
                           "    /BitsPerComponent 8\n"
                           "    /Filter /ASCII85Decode\n"
                           "    /Length %d 0 R\n" 
                           "  >>\n",
                           OBJ_SOFTMASK, 
                           srcPtr->width, srcPtr->height, 
                           OBJ_SOFTMASK_LENGTH);
        Blt_DBuffer_VarAppend(pdfPtr->dbuffer, "stream\n", (char *)NULL);
        Blt_DBuffer_AppendBase85(pdfPtr->dbuffer, maskData,
                srcPtr->width * srcPtr->height);
        length = Blt_DBuffer_Length(pdfPtr->dbuffer);
        Blt_DBuffer_VarAppend(pdfPtr->dbuffer, 
                              "\n"
                              "endstream\n", 
                              "endobj\n", (char *)NULL);

        /* Length of softmask stream object. */
        SetXRefOffset(pdfPtr, OBJ_SOFTMASK_LENGTH);
        Blt_DBuffer_Format(pdfPtr->dbuffer, 
                           "%d 0 obj\n" 
                           "  %ld\n"
                           "endobj\n", 
                           OBJ_SOFTMASK_LENGTH,
                           length);
        Blt_Free(maskData);
        pdfPtr->numObjects = OBJ_SOFTMASK_LENGTH;
    }

    /* Xref */
    offset = Blt_DBuffer_Length(pdfPtr->dbuffer);
    Blt_DBuffer_Format(pdfPtr->dbuffer,
                       "xref\n"
                       "0 %d\n"
                       "0000000000 65535 f \n",
                       pdfPtr->numObjects + 1);
    for (i = 0; i < pdfPtr->numObjects; i++) {
        Blt_DBuffer_Format(pdfPtr->dbuffer,
                           "%010ld 00000 n \n", 
                           pdfPtr->xref[i]);
    }

    /* Trailer */
    Blt_DBuffer_Format(pdfPtr->dbuffer,
                       "trailer\n"
                       "  <<\n"
                       "    /Size %d\n"
                       "    /Root %d 0 R\n"
                       "    /Info %d 0 R\n"
                       "  >>\n"
                       "startxref\n"
                       "%ld\n"
                       "%%%%EOF\n", 
                       pdfPtr->numObjects + 1,
                       OBJ_CATALOG, 
                       OBJ_INFO,
                       offset);

    if (srcPtr != original) {
        Blt_Free(srcPtr);
    }
    return TCL_OK;
}

static int 
IsPdf(Blt_DBuffer dbuffer)
{
    unsigned char *bp;

    Blt_DBuffer_Rewind(dbuffer);
    if (Blt_DBuffer_BytesLeft(dbuffer) < 4) {
        return FALSE;
    }
    bp = Blt_DBuffer_Pointer(dbuffer);
    return (strncmp("%PDF", (char *)bp, 4) == 0);
}

static Blt_Chain
ReadPdf(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer)
{
    Blt_Chain chain;
    PdfImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    switches.crop = TRUE;
    chain = PdfToPicture(interp, fileName, dbuffer, &switches);
    Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
    return chain;
}

static Tcl_Obj *
WritePdf(Tcl_Interp *interp, Blt_Picture picture)
{
    Pdf *pdfPtr;
    PdfExportSwitches switches;
    Tcl_Obj *objPtr;

    /* Default export switch settings. */
    memset(&switches, 0, sizeof(switches));
    switches.bg.u32 = 0xFFFFFFFF; /* Default bgcolor is white. */

    objPtr = NULL;
    pdfPtr = NewPdf();
    if (PictureToPdf(interp, picture, pdfPtr, &switches) == TCL_OK) {
        objPtr = Blt_DBuffer_Base64EncodeToObj(pdfPtr->dbuffer);
    }
    FreePdf(pdfPtr);
    return objPtr;
}

static Blt_Chain
ImportPdf(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, 
          const char **fileNamePtr)
{
    Blt_Chain chain;
    Blt_DBuffer dbuffer;
    PdfImportSwitches switches;
    const char *string;

    memset(&switches, 0, sizeof(switches));
    switches.crop = TRUE;
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3, objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return NULL;
    }
    if ((switches.dataObjPtr != NULL) && (switches.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "more than one import source: ",
                "use only one -file or -data flag.", (char *)NULL);
        return NULL;
    }
    chain = NULL;
    dbuffer = Blt_DBuffer_Create();
    if (switches.dataObjPtr != NULL) {
        int numBytes;

        string = Tcl_GetStringFromObj(switches.dataObjPtr, &numBytes);
        Blt_DBuffer_AppendData(dbuffer, (unsigned char *)string, numBytes);
        string = "data buffer";
        *fileNamePtr = NULL;
    } else {
        string = Tcl_GetString(switches.fileObjPtr);
        if (Blt_DBuffer_LoadFile(interp, string, dbuffer) != TCL_OK) {
            Blt_DBuffer_Destroy(dbuffer);
            return NULL;
        }
        *fileNamePtr = string;
    }
    chain = PdfToPicture(interp, string, dbuffer, &switches);
    Blt_DBuffer_Destroy(dbuffer);
    Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
    return chain;
}

static int
ExportPdf(Tcl_Interp *interp, int index, Blt_Chain chain, int objc,
          Tcl_Obj *const *objv)
{
    PdfExportSwitches switches;
    Pdf *pdfPtr;
    int result;
    Blt_Picture picture;

    result = TCL_ERROR;
    memset(&switches, 0, sizeof(switches));
    switches.bg.u32 = 0xFFFFFFFF; /* Default bgcolor is white. */
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 3, objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
        return TCL_ERROR;
    }
    if ((switches.dataObjPtr != NULL) && (switches.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "more than one export destination: ",
                "use only one -file or -data switch.", (char *)NULL);
        Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
        return TCL_ERROR;
    }
    picture = Blt_GetNthPicture(chain, switches.index);
    pdfPtr = NewPdf();
    if (PictureToPdf(interp, picture, pdfPtr, &switches) != TCL_OK) {
        Tcl_AppendResult(interp, "can't convert \"", 
                Tcl_GetString(objv[2]), "\"", (char *)NULL);
        goto error;
    }
    if (switches.fileObjPtr != NULL) {
        char *fileName;

        fileName = Tcl_GetString(switches.fileObjPtr);
        fprintf(stderr, "writing out %s\n", fileName);
        result = Blt_DBuffer_SaveFile(interp, fileName, pdfPtr->dbuffer);
    } else if (switches.dataObjPtr != NULL) {
        Tcl_Obj *objPtr;

        objPtr = Blt_DBuffer_ByteArrayObj(pdfPtr->dbuffer);
        result = (objPtr == NULL) ? TCL_ERROR : TCL_OK;
        if (objPtr != NULL) {
            Tcl_SetObjResult(interp, objPtr);
        }
    } else {
        Tcl_Obj *objPtr;

        objPtr = Blt_DBuffer_Base64EncodeToObj(pdfPtr->dbuffer);
        result = (objPtr == NULL) ? TCL_ERROR : TCL_OK;
        if (objPtr != NULL) {
            Tcl_SetObjResult(interp, objPtr);
        }
    }  
 error:
    Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
    FreePdf(pdfPtr);
    return result;
}

int 
Blt_PicturePdfInit(Tcl_Interp *interp)
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
    if (Tcl_PkgProvide(interp, "blt_picture_pdf", BLT_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }
    return Blt_PictureRegisterFormat(interp,
        "pdf",                          /* Name of format. */
        IsPdf,                          /* Discovery routine. */
        ReadPdf,                        /* Read procedure. */
        WritePdf,                       /* Write procedure. */
        ImportPdf,                      /* Import procedure. */
        ExportPdf);                     /* Export procedure. */
}

int 
Blt_PicturePdfSafeInit(Tcl_Interp *interp) 
{
    return Blt_PicturePdfInit(interp);
}

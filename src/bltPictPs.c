/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictPs.c --
 *
 * This module implements Encapsulated PostScript file format conversion
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

#include "bltInt.h"
#include "config.h"
#include <stdlib.h>
#include <limits.h>
#include <tcl.h>

#include <limits.h>
#include <stdlib.h>

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifndef __WIN32__
  #if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__BORLANDC__)
    #define __WIN32__
    #ifndef WIN32
      #define WIN32
    #endif
  #endif
#endif  /*__WIN32__*/

#ifdef _MSC_VER
  #define _CRT_SECURE_NO_DEPRECATE
  #define _CRT_NONSTDC_NO_DEPRECATE
#endif  /*MSC_VER*/

#ifdef WIN32
  #define STRICT
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #undef STRICT
  #undef WIN32_LEAN_AND_MEAN
  #include <windowsx.h>
#endif  /*WIN32*/

#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif /*HAVE_UNISTD_H*/

#include <tk.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "bltTkInt.h"

#include <bltSwitch.h>
#include "bltPicture.h"
#include "bltPictFmts.h"
#include "bltPs.h"
#include "bltWait.h"

#ifdef WIN32
  #include "bltWin.h"
#endif /*WIN32*/

#define UCHAR(c)        ((unsigned char) (c))
#define ISASCII(c)      (UCHAR(c)<=0177)
#define MIN(a,b)        (((a)<(b))?(a):(b))

#define TRUE    1
#define FALSE   0
#define div257(t)       (((t)+((t)>>8))>>8)
#define SetBit(x)       destRowPtr[(x>>3)] |= (0x80 >>(x&7))
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
} PsExportSwitches;

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    int dpi;                            /* Dots per inch. */
    const char *paperSize;              /* Papersize. */
    int crop;
} PsImportSwitches;

#define PS_CROP         (1<<0)

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

static Blt_SwitchParseProc PicaSwitchProc;
static Blt_SwitchCustom picaSwitch = {
    PicaSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc PadSwitchProc;
static Blt_SwitchCustom padSwitch = {
    PadSwitchProc, NULL, NULL, (ClientData)0,
};

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,  "-background", "color", (char *)NULL,
        Blt_Offset(PsExportSwitches, bg),          0, 0, &colorSwitch},
    {BLT_SWITCH_OBJ,     "-data",       "data", (char *)NULL,
        Blt_Offset(PsExportSwitches, dataObjPtr),  0},
    {BLT_SWITCH_OBJ,     "-file",       "fileName", (char *)NULL,
        Blt_Offset(PsExportSwitches, fileObjPtr),  0},
    {BLT_SWITCH_BITMASK, "-center",     "", (char *)NULL,
        Blt_Offset(PsExportSwitches, setup.flags), 0, PS_CENTER},
    {BLT_SWITCH_BITMASK, "-greyscale",  "", (char *)NULL,
        Blt_Offset(PsExportSwitches, setup.flags), 0, PS_GREYSCALE},
    {BLT_SWITCH_BITMASK, "-landscape",  "", (char *)NULL,
        Blt_Offset(PsExportSwitches, setup.flags), 0, PS_LANDSCAPE},
    {BLT_SWITCH_INT_POS, "-level",      "pslevel", (char *)NULL,
        Blt_Offset(PsExportSwitches, setup.level), 0},
    {BLT_SWITCH_BITMASK, "-maxpect",    "", (char *)NULL,
        Blt_Offset(PsExportSwitches, setup.flags), 0, PS_MAXPECT},
    {BLT_SWITCH_CUSTOM,  "-padx",       "pad", (char *)NULL,
        Blt_Offset(PsExportSwitches, setup.xPad),  0, 0, &padSwitch},
    {BLT_SWITCH_CUSTOM,  "-pady",       "pad", (char *)NULL,
        Blt_Offset(PsExportSwitches, setup.yPad),  0, 0, &padSwitch},
    {BLT_SWITCH_CUSTOM,  "-paperheight","pica", (char *)NULL,
        Blt_Offset(PsExportSwitches, setup.reqPaperHeight), 0, 0, &picaSwitch},
    {BLT_SWITCH_CUSTOM,  "-paperwidth", "pica", (char *)NULL,
        Blt_Offset(PsExportSwitches, setup.reqPaperWidth), 0, 0, &picaSwitch},
    {BLT_SWITCH_LISTOBJ,  "-comments", "{key value...}", (char *)NULL,
        Blt_Offset(PsExportSwitches, setup.cmtsObjPtr), BLT_SWITCH_NULL_OK},
    {BLT_SWITCH_INT_NNEG, "-index", "int", (char *)NULL,
        Blt_Offset(PsExportSwitches, index), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec importSwitches[] =
{
    {BLT_SWITCH_OBJ,     "-data",       "data", (char *)NULL,
        Blt_Offset(PsImportSwitches, dataObjPtr),  0},
    {BLT_SWITCH_INT,      "-dpi",       "number", (char *)NULL,
        Blt_Offset(PsImportSwitches, dpi), 0},
    {BLT_SWITCH_OBJ,     "-file",       "fileName", (char *)NULL,
        Blt_Offset(PsImportSwitches, fileObjPtr),  0},
    {BLT_SWITCH_BITMASK,  "-nocrop",    "", (char *)NULL,
        Blt_Offset(PsImportSwitches, crop), 0, FALSE},
    {BLT_SWITCH_STRING,  "-papersize",  "string", (char *)NULL,
        Blt_Offset(PsImportSwitches, paperSize),   0},
    {BLT_SWITCH_END}
};

DLLEXPORT extern Tcl_AppInitProc Blt_PicturePsInit;
DLLEXPORT extern Tcl_AppInitProc Blt_PicturePsSafeInit;

#ifdef WIN32
  #define close(fd)               CloseHandle((HANDLE)fd)
  #define kill                    KillProcess
  #define waitpid                 WaitProcess
#endif  /* WIN32 */

#define TRUE    1
#define FALSE   0

typedef struct _Blt_Picture Picture;

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

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

static void
AddComments(Blt_Ps ps, Tcl_Obj *objPtr)
{
    Tcl_Obj **objv;
    int objc;
    int i;
    
    Tcl_ListObjGetElements(NULL, objPtr, &objc, &objv);
    for (i = 0; i < objc; i += 2) {
        if ((i+1) == objc) {
            break;
        }
        Blt_Ps_Format(ps, "%% %s: %s\n", Tcl_GetString(objv[i]),
                      Tcl_GetString(objv[i+1]));
    }
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

/*
 *--------------------------------------------------------------------------
 *
 * PicaSwitchProc --
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
PicaSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)  
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
PadSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Blt_Pad *padPtr = (Blt_Pad *)(record + offset);
    
    return Blt_Ps_GetPadFromObj(interp, objPtr, padPtr);
}

/*
 * --------------------------------------------------------------------------
 *
 * PostScriptPreamble --
 *
 *      The PostScript preamble calculates the needed translation and
 *      scaling to make image coordinates compatible with PostScript.
 *
 * --------------------------------------------------------------------------
 */
static int
PostScriptPreamble(Tcl_Interp *interp, Picture *srcPtr, 
                   PsExportSwitches *switchesPtr, Blt_Ps ps)
{
    PageSetup *setupPtr = &switchesPtr->setup;
    time_t ticks;
    char date[200];                     /* Holds the date string from
                                         * ctime */
    const char *version;
    char *newline;

    Blt_Ps_Append(ps, "%!PS-Adobe-3.0 EPSF-3.0\n");

    /* The "BoundingBox" comment is required for EPS files. */
    Blt_Ps_Format(ps, "%%%%BoundingBox: %d %d %d %d\n",
        setupPtr->left, setupPtr->paperHeight - setupPtr->top,
        setupPtr->right, setupPtr->paperHeight - setupPtr->bottom);
    Blt_Ps_Append(ps, "%%Pages: 0\n");

    version = Tcl_GetVar(interp, "blt_version", TCL_GLOBAL_ONLY);
    if (version == NULL) {
        version = "???";
    }
    Blt_Ps_Format(ps, "%%%%Creator: (BLT %s Picture)\n", version);

    ticks = time((time_t *) NULL);
    strcpy(date, ctime(&ticks));
    newline = date + strlen(date) - 1;
    if (*newline == '\n') {
        *newline = '\0';
    }
    Blt_Ps_Format(ps, "%%%%CreationDate: (%s)\n", date);
    Blt_Ps_Append(ps, "%%DocumentData: Clean7Bit\n");
    if (setupPtr->flags & PS_LANDSCAPE) {
        Blt_Ps_Append(ps, "%%Orientation: Landscape\n");
    } else {
        Blt_Ps_Append(ps, "%%Orientation: Portrait\n");
    }
    if (setupPtr->cmtsObjPtr != NULL) {
        AddComments(ps, setupPtr->cmtsObjPtr);
    }
    Blt_Ps_Append(ps, "%%BeginProlog\n");
    Blt_Ps_Append(ps, "%%EndProlog\n");
    Blt_Ps_Append(ps, "%%BeginSetup\n");
    Blt_Ps_Append(ps, "gsave\n");
    /*
     * Set the conversion from PostScript to X11 coordinates.  Scale pica
     * to pixels and flip the y-axis (the origin is the upperleft corner).
     */
    Blt_Ps_VarAppend(ps,
        "% Transform coordinate system to use X11 coordinates\n"
        "% 1. Flip y-axis over by reversing the scale,\n", (char *)NULL);
    Blt_Ps_Append(ps, "1 -1 scale\n");
    Blt_Ps_VarAppend(ps, 
        "% 2. Translate the origin to the other side of the page,\n"
        "%    making the origin the upper left corner\n", (char *)NULL);
    Blt_Ps_Format(ps, "0 %d translate\n\n", -setupPtr->paperHeight);
    Blt_Ps_VarAppend(ps, "% User defined page layout\n\n",
        "% Set color level\n", (char *)NULL);
    Blt_Ps_Format(ps, "%% Set origin\n%d %d translate\n\n",
                  setupPtr->left, setupPtr->bottom);
    if (setupPtr->flags & PS_LANDSCAPE) {
        Blt_Ps_Format(ps,
            "%% Landscape orientation\n0 %g translate\n-90 rotate\n",
            ((double)srcPtr->width * setupPtr->scale));
    }
    if (setupPtr->scale != 1.0f) {
        Blt_Ps_Append(ps, "\n% Setting picture scale factor\n");
        Blt_Ps_Format(ps, " %g %g scale\n", setupPtr->scale, setupPtr->scale);
    }
    Blt_Ps_Append(ps, "\n%%EndSetup\n\n");
    return TCL_OK;
}


static char *
PbmComment(char *bp) 
{
    char *p;

    p = bp;
    if (*p == '#') {
        /* Comment: file end of line */
        while((*p != '\n') && (p != '\0')) {
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
        Tcl_AppendResult(interp, "unknown ppm format", (char *)NULL);
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
            pbm.bitsPerPixel <<= 1;     /* 16-bit greyscale or 48-bit color. */
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
    int fd;
    Blt_DBuffer dbuffer;
    int lastError;
} PsWriter;

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
 *      Writes the buffer (PostScript file) to the ghostscript standard
 *      input file descriptor.
 *
 *---------------------------------------------------------------------------
 */
static DWORD WINAPI
WriteBufferProc(void *clientData)
{
    PsWriter *writerPtr = clientData;
    const unsigned char *bp;
    int bytesLeft;
    HANDLE hFile;
    DWORD count;

    hFile = (HANDLE)writerPtr->fd;
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

static PsWriter writer;

static int
WriteToGhostscript(Tcl_Interp *interp, int fd, Blt_DBuffer dbuffer)
{
    HANDLE hThread;
    ClientData clientData;
    DWORD id;

    writer.dbuffer = Blt_DBuffer_Create();
    Blt_DBuffer_Init(writer.dbuffer);
    /* Copy the input to a new buffer. */
    Blt_DBuffer_Concat(writer.dbuffer, dbuffer);
    writer.fd = fd;
    writer.lastError = 0;
    clientData = &writer;
    hThread = CreateThread(
        NULL,                           /* Security attributes */
        8000,                           /* Initial stack size. */
        WriteBufferProc,                /* Address of thread routine */
        clientData,                     /* One-word of data passed to the
                                         * routine. */
        0,                              /* Creation flags */
        &id);                           /* (out) Will contain Id of new
                                         * thread. */
    return (int)hThread;
}


static int
ReadFromGhostscript(Tcl_Interp *interp, int fd, Blt_DBuffer dbuffer)
{
    DWORD numBytes;
    HANDLE hFile;
    int result;

    Blt_DBuffer_Free(dbuffer);
    hFile = (HANDLE)fd;
    numBytes = 0;
    for (;;) {
        DWORD numRead;
        unsigned char *bp;

        bp = Blt_DBuffer_Extend(dbuffer, BUFFER_SIZE);
        if (!ReadFile(hFile, bp, BUFFER_SIZE, &numRead, NULL)) {
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

static int
WriteToGhostscript(Tcl_Interp *interp, int fd, Blt_DBuffer dbuffer)
{
    pid_t child;

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
ReadFromGhostscript(Tcl_Interp *interp, int fd, Blt_DBuffer dbuffer)
{
    int numBytes;
    
    Blt_DBuffer_Free(dbuffer);
    numBytes = 0;
    for (;;) {
        char *bp;
        int numRead;

        bp = (char *)Blt_DBuffer_Extend(dbuffer, BUFFER_SIZE);
        numRead = read(fd, bp, BUFFER_SIZE);
        if (numRead == 0) {
            break;                      /* EOF */
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
PsToPbm(
    Tcl_Interp *interp, 
    const char *fileName,
    Blt_DBuffer dbuffer,
    PsImportSwitches *switchesPtr)
{
    int in, out;                        /* File descriptors for ghostscript
                                         * subprocess. */
    char string1[200];
    char string2[200];
    int numPids;
    Blt_Pid *pidPtr;
    int result;
    pid_t child;
    const char **p;
    const char *args[] = {
        "gs",                           /* Ghostscript command */
        "-dEPSCrop",                    /* (optional) crop page to bbox  */
        "-dSAFER",                      /*  */
        "-q",                           /* Quiet mode.  No GS messages.  */
        "-sDEVICE=ppmraw",              /* Format is PPM raw */
        "-dBATCH",                      /* Batch mode. No "quit" necessary. */
        "-sPAPERSIZE=letter",           /* (optional) Specify paper size. */
        "-r100x100",                    /* (optional) Specify DPI of X
                                         * screen */
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

        for (i = 0, p = args; *p != NULL; p++, i++) {
            objv[i] = Tcl_NewStringObj(*p, -1);
            Tcl_IncrRefCount(objv[i]);
        }
        numPids = Blt_CreatePipeline(interp, objc, objv, &pidPtr, &in, &out,
                (int *)NULL);
        for (i = 0; i < objc; i++) {
            Tcl_DecrRefCount(objv[i]);
        }
    }
    if (numPids < 0) {
        return TCL_ERROR;
    }
    Tcl_DetachPids(numPids, (Tcl_Pid *)pidPtr);
    child = WriteToGhostscript(interp, in, dbuffer);
    if (child == 0) {
        return TCL_ERROR;
    }
    result = ReadFromGhostscript(interp, out, dbuffer);
#ifdef WIN32
    CloseHandle((HANDLE)child);
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
PsToPicture(
    Tcl_Interp *interp, 
    const char *fileName,
    Blt_DBuffer dbuffer,
    PsImportSwitches *switchesPtr)
{
    Blt_Chain chain;

    if (PsToPbm(interp, fileName, dbuffer, switchesPtr) != TCL_OK) {
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

static int
PictureToPs(Tcl_Interp *interp, Blt_Picture original, Blt_Ps ps,
            PsExportSwitches *switchesPtr)
{
    Picture *srcPtr;
    int w, h;

    srcPtr = original;
    w = srcPtr->width, h = srcPtr->height;
    Blt_Ps_ComputeBoundingBox(&switchesPtr->setup, w, h);
    if (PostScriptPreamble(interp, srcPtr, switchesPtr, ps) != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_ClassifyPicture(srcPtr); 
    if (!Blt_Picture_IsOpaque(srcPtr)) {
        Blt_Picture background;

        background = Blt_CreatePicture(srcPtr->width, srcPtr->height);
        Blt_BlankPicture(background, switchesPtr->bg.u32);
        Blt_CompositePictures(background, srcPtr);
        srcPtr = background;
    }
    if (srcPtr->flags & BLT_PIC_ASSOCIATED_COLORS) {
        Blt_UnassociateColors(srcPtr);
    }
    Blt_Ps_Rectangle(ps, 0, 0, srcPtr->width, srcPtr->height);
    Blt_Ps_Append(ps, "gsave clip\n\n");
    Blt_Ps_DrawPicture(ps, srcPtr, 0, 0);
    Blt_Ps_VarAppend(ps, "\n",
        "% Unset clipping\n",
        "grestore\n\n", (char *)NULL);
    Blt_Ps_VarAppend(ps,
        "showpage\n",
        "%Trailer\n",
        "grestore\n",
        "end\n",
        "%EOF\n", (char *)NULL);
    if (srcPtr != original) {
        Blt_Free(srcPtr);
    }
    return TCL_OK;
}

static int 
IsPs(Blt_DBuffer dbuffer)
{
    unsigned char *bp;

    Blt_DBuffer_Rewind(dbuffer);
    if (Blt_DBuffer_BytesLeft(dbuffer) < 4) {
        return FALSE;
    }
    bp = Blt_DBuffer_Pointer(dbuffer);
    return (strncmp("%!PS", (char *)bp, 4) == 0);
}

static Blt_Chain
ReadPs(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer)
{
    Blt_Chain chain;
    PsImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    switches.crop = TRUE;
    chain = PsToPicture(interp, fileName, dbuffer, &switches);
    Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
    return chain;
}

static Tcl_Obj *
WritePs(Tcl_Interp *interp, Blt_Picture picture)
{
    Blt_Ps ps;
    PsExportSwitches switches;
    Tcl_Obj *objPtr;
    int result;

    /* Default export switch settings. */
    memset(&switches, 0, sizeof(switches));
    switches.bg.u32 = 0xFFFFFFFF;       /* Default bgcolor is white. */
    switches.setup.reqPaperHeight = 792; /* 11 inches */
    switches.setup.reqPaperWidth = 612; /* 8.5 inches */
    switches.setup.level = 1;
    switches.setup.xPad.side1 = 72;
    switches.setup.xPad.side2 = 72;
    switches.setup.yPad.side1 = 72;
    switches.setup.yPad.side2 = 72;
    switches.setup.flags = 0;
    ps = Blt_Ps_Create(interp, &switches.setup);
    result = PictureToPs(interp, picture, ps, &switches);
    objPtr = NULL;
    if (result == TCL_OK) {
        const char *string;
        int length;

        string = Blt_Ps_GetValue(ps, &length);
        objPtr = Tcl_NewStringObj(string, length);
    }
    Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
    Blt_Ps_Free(ps);
    return objPtr;
}

static Blt_Chain
ImportPs(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, 
         const char **fileNamePtr)
{
    Blt_Chain chain;
    Blt_DBuffer dbuffer;
    PsImportSwitches switches;
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
    chain = PsToPicture(interp, string, dbuffer, &switches);
    Blt_DBuffer_Destroy(dbuffer);
    Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
    return chain;
}

static int
ExportPs(Tcl_Interp *interp, int index, Blt_Chain chain, int objc, 
         Tcl_Obj *const *objv)
{
    PsExportSwitches switches;
    Blt_Ps ps;
    int result;
    Blt_Picture picture;

    memset(&switches, 0, sizeof(switches));
    switches.bg.u32 = 0xFFFFFFFF;       /* Default bgcolor is white. */
    switches.setup.reqPaperHeight = 792; /* 11 inches */
    switches.setup.reqPaperWidth = 612; /* 8.5 inches */
    switches.setup.level = 1;
    switches.setup.xPad.side1 = 72;
    switches.setup.xPad.side2 = 72;
    switches.setup.yPad.side1 = 72;
    switches.setup.yPad.side2 = 72;
    switches.setup.flags = 0;
    switches.index = index;
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
    if (picture == NULL) {
        Tcl_AppendResult(interp, "no picture at index ", 
                Blt_Itoa(switches.index), (char *)NULL);
        Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
        return TCL_ERROR;
    }
    ps = Blt_Ps_Create(interp, &switches.setup);
    result = PictureToPs(interp, picture, ps, &switches);
    if (result != TCL_OK) {
        Tcl_AppendResult(interp, "can't convert \"", 
                Tcl_GetString(objv[2]), "\"", (char *)NULL);
        goto error;
    }
    if (switches.fileObjPtr != NULL) {
        char *fileName;

        fileName = Tcl_GetString(switches.fileObjPtr);
        result = Blt_Ps_SaveFile(interp, ps, fileName);
    } else if (switches.dataObjPtr != NULL) {
        Tcl_Obj *objPtr;
        int length;
        const char *string;

        string = Blt_Ps_GetValue(ps, &length);
        objPtr = Tcl_NewStringObj(string, length);
        objPtr = Tcl_ObjSetVar2(interp, switches.dataObjPtr, NULL, objPtr, 0);
        result = (objPtr == NULL) ? TCL_ERROR : TCL_OK;
    } else {
        const char *string;
        int length;

        string = Blt_Ps_GetValue(ps, &length);
        Tcl_SetStringObj(Tcl_GetObjResult(interp), string, length);
    }  
 error:
    Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
    Blt_Ps_Free(ps);
    return result;
}

int 
Blt_PicturePsInit(Tcl_Interp *interp)
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
    if (Tcl_PkgProvide(interp, "blt_picture_ps", BLT_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }
    return Blt_PictureRegisterFormat(interp,
        "ps",                           /* Name of format. */
        IsPs,                           /* Discovery routine. */
        ReadPs,                         /* Import format procedure. */
        WritePs,                        /* Export format procedure. */
        ImportPs,                       /* Import format procedure. */
        ExportPs);                      /* Export format procedure. */
}

int 
Blt_PicturePsSafeInit(Tcl_Interp *interp) 
{
    return Blt_PicturePsInit(interp);
}

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPs.c --
 *
 * This module implements general PostScript conversion routines.
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

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_STDARG_H
  #include <stdarg.h>
#endif  /* HAVE_STDARG_H */


#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltDBuffer.h"
#include "bltPicture.h"
#include "bltPsInt.h"
#include "bltAfm.h"
#include "tkIntBorder.h"
#include "tkDisplay.h"
#include "tkFont.h"
#define PS_MAXPATH      1500            /* Maximum number of components in
                                         * a PostScript (level 1) path. */

#define PICA_MM         2.83464566929
#define PICA_INCH       72.0
#define PICA_CM         28.3464566929

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ps_GetPica --
 *
 *      Given a string, returns the number of pica corresponding to that
 *      string.
 *
 * Results:
 *      The return value is a standard TCL return result.  If TCL_OK is
 *      returned, then everything went well and the pixel distance is
 *      stored at *doublePtr; otherwise TCL_ERROR is returned and an error
 *      message is left in interp->result.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Ps_GetPicaFromObj(
    Tcl_Interp *interp,                 /* Use this for error reporting. */
    Tcl_Obj *objPtr,                    /* String describing a number of
                                         * pixels. */
    int *picaPtr)                       /* Place to store converted
                                         * result. */
{
    char *p;
    double pica;
    const char *string;
    
    string = Tcl_GetString(objPtr);
    pica = strtod((char *)string, &p);
    if (p == string) {
        goto error;
    }
    if (pica < 0.0) {
        goto error;
    }
    while ((*p != '\0') && isspace(UCHAR(*p))) {
        p++;
    }
    switch (*p) {
        case '\0':                           break;
        case 'c': pica *= PICA_CM;   p++;    break;
        case 'i': pica *= PICA_INCH; p++;    break;
        case 'm': pica *= PICA_MM;   p++;    break;
        case 'p': p++;                       break;
        default:  goto error;
    }
    while ((*p != '\0') && isspace(UCHAR(*p))) {
        p++;
    }
    if (*p == '\0') {
        *picaPtr = ROUND(pica);
        return TCL_OK;
    }
 error:
    Tcl_AppendResult(interp, "bad screen distance \"", string, "\"", 
        (char *) NULL);
    return TCL_ERROR;
}

int
Blt_Ps_GetPadFromObj(
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tcl_Obj *objPtr,                    /* Pixel value string */
    Blt_Pad *padPtr)
{
    int side1, side2;
    int objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((objc < 1) || (objc > 2)) {
        Tcl_AppendResult(interp, "wrong # elements in padding list",
            (char *)NULL);
        return TCL_ERROR;
    }
    if (Blt_Ps_GetPicaFromObj(interp, objv[0], &side1) != TCL_OK) {
        return TCL_ERROR;
    }
    side2 = side1;
    if ((objc > 1) && 
        (Blt_Ps_GetPicaFromObj(interp, objv[1], &side2) != TCL_OK)) {
        return TCL_ERROR;
    }
    /* Don't update the pad structure until we know both values are okay. */
    padPtr->side1 = side1;
    padPtr->side2 = side2;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ps_ComputeBoundingBox --
 *
 *      Computes the bounding box for the PostScript plot.  First get the
 *      size of the plot (by default, it's the size of graph's X window).
 *      If the plot plus the page border is bigger than the designated
 *      paper size, or if the "-maxpect" option is turned on, scale the
 *      plot to the page.
 *
 *      Note: All coordinates/sizes are in screen coordinates, not
 *            PostScript coordinates.  This includes the computed bounding
 *            box and paper size.  They will be scaled to printer points
 *            later.
 *
 * Results:
 *      Returns the height of the paper in screen coordinates.
 *
 * Side Effects:
 *      The graph dimensions (width and height) are changed to the
 *      requested PostScript plot size.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Ps_ComputeBoundingBox(PageSetup *setupPtr, int width, int height)
{
    int paperWidth, paperHeight;
    int x, y, hSize, vSize, hBorder, vBorder;
    float hScale, vScale, scale;

    x = setupPtr->padLeft;
    y = setupPtr->padTop;

    hBorder = PADDING(setupPtr->padX);
    vBorder = PADDING(setupPtr->padY);

    if (setupPtr->flags & PS_LANDSCAPE) {
        hSize = height;
        vSize = width;
    } else {
        hSize = width;
        vSize = height;
    }
    /*
     * If the paper size wasn't specified, set it to the graph size plus
     * the paper border.
     */
    paperWidth = (setupPtr->reqPaperWidth > 0) ? setupPtr->reqPaperWidth :
        hSize + hBorder;
    paperHeight = (setupPtr->reqPaperHeight > 0) ? setupPtr->reqPaperHeight :
        vSize + vBorder;

    /*
     * Scale the plot size (the graph itself doesn't change size) if it's
     * bigger than the paper or if -maxpect was set.
     */
    hScale = vScale = 1.0f;
    if ((setupPtr->flags & PS_MAXPECT) || ((hSize + hBorder) > paperWidth)) {
        hScale = (float)(paperWidth - hBorder) / (float)hSize;
    }
    if ((setupPtr->flags & PS_MAXPECT) || ((vSize + vBorder) > paperHeight)) {
        vScale = (float)(paperHeight - vBorder) / (float)vSize;
    }
    scale = MIN(hScale, vScale);
    if (scale != 1.0f) {
        hSize = (int)((hSize * scale) + 0.5f);
        vSize = (int)((vSize * scale) + 0.5f);
    }
    setupPtr->scale = scale;
    if (setupPtr->flags & PS_CENTER) {
        if (paperWidth > hSize) {
            x = (paperWidth - hSize) / 2;
        }
        if (paperHeight > vSize) {
            y = (paperHeight - vSize) / 2;
        }
    }
    setupPtr->left = x;
    setupPtr->bottom = y;
    setupPtr->right = x + hSize - 1;
    setupPtr->top = y + vSize - 1;
    setupPtr->paperHeight = paperHeight;
    setupPtr->paperWidth = paperWidth;
    return paperHeight;
}

PostScript *
Blt_Ps_Create(Tcl_Interp *interp, PageSetup *setupPtr)
{
    PostScript *psPtr;

    psPtr = Blt_AssertMalloc(sizeof(PostScript));
    psPtr->setupPtr = setupPtr;
    psPtr->interp = interp;
    psPtr->dbuffer = Blt_DBuffer_Create();
    return psPtr;
}

void 
Blt_Ps_SetPrinting(PostScript *psPtr, int state)
{
    Blt_Afm_SetPrinting(psPtr->interp, state);
}

void
Blt_Ps_Free(PostScript *psPtr)
{
    Blt_DBuffer_Destroy(psPtr->dbuffer);
    Blt_Free(psPtr);
}

const char *
Blt_Ps_GetValue(PostScript *psPtr, int *lengthPtr)
{
    *lengthPtr = Blt_DBuffer_Length(psPtr->dbuffer);
    return (const char *)Blt_DBuffer_Bytes(psPtr->dbuffer);
}

const char *
Blt_Ps_GetString(PostScript *psPtr)
{
    return Blt_DBuffer_String(psPtr->dbuffer);
}

void
Blt_Ps_SetInterp(PostScript *psPtr, Tcl_Interp *interp)
{
    Tcl_SetObjResult(interp, Blt_DBuffer_StringObj(psPtr->dbuffer));
}

char *
Blt_Ps_GetScratchBuffer(PostScript *psPtr)
{
    return psPtr->scratchArr;
}

Tcl_Interp *
Blt_Ps_GetInterp(PostScript *psPtr)
{
    return psPtr->interp;
}

Blt_DBuffer
Blt_Ps_GetDBuffer(PostScript *psPtr)
{
    return psPtr->dbuffer;
}

int 
Blt_Ps_SaveFile(Tcl_Interp *interp, PostScript *psPtr, const char *fileName)
{
    return Blt_DBuffer_SaveFile(interp, fileName, psPtr->dbuffer);
}

void
Blt_Ps_VarAppend(PostScript *psPtr, ...)
{
    va_list args;

    va_start(args, psPtr);
    for (;;) {
        const char *string;

        string = va_arg(args, char *);
        if (string == NULL) {
            break;
        }
        Blt_Ps_AppendBytes(psPtr, string, -1);
    }
    va_end(args);
}

void
Blt_Ps_AppendBytes(PostScript *psPtr, const char *bytes, int length)
{
    Blt_DBuffer_AppendString(psPtr->dbuffer, bytes, length);
}

void
Blt_Ps_Append(PostScript *psPtr, const char *string)
{
    Blt_Ps_AppendBytes(psPtr, string, -1);
}

void
Blt_Ps_Format(PostScript *psPtr, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vsnprintf(psPtr->scratchArr, POSTSCRIPT_BUFSIZ, fmt, args);
    va_end(args);
    Blt_Ps_AppendBytes(psPtr, psPtr->scratchArr, -1);
}

int
Blt_Ps_IncludeFile(Tcl_Interp *interp, Blt_Ps ps, const char *fileName)
{
    Tcl_Channel channel;
    Tcl_DString ds;
    char *buf;
    char *libDir;
    int numBytes;

    buf = Blt_Ps_GetScratchBuffer(ps);

    /*
     * Read in a standard prolog file from file and append it to the
     * PostScript output stored in the Tcl_DString in psPtr.
     */

    libDir = (char *)Tcl_GetVar(interp, "blt_library", TCL_GLOBAL_ONLY);
    if (libDir == NULL) {
        Tcl_AppendResult(interp, "couldn't find BLT script library:",
            "global variable \"blt_library\" doesn't exist", (char *)NULL);
        return TCL_ERROR;
    }
    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, libDir, -1);
    Tcl_DStringAppend(&ds, "/", -1);
    Tcl_DStringAppend(&ds, fileName, -1);
    fileName = Tcl_DStringValue(&ds);
    Blt_Ps_VarAppend(ps, "\n% including file \"", fileName, "\"\n\n", 
        (char *)NULL);
    channel = Tcl_OpenFileChannel(interp, fileName, "r", 0);
    if (channel == NULL) {
        Tcl_AppendResult(interp, "couldn't open prologue file \"", fileName,
                 "\": ", Tcl_PosixError(interp), (char *)NULL);
        return TCL_ERROR;
    }
    for(;;) {
        numBytes = Tcl_Read(channel, buf, POSTSCRIPT_BUFSIZ);
        if (numBytes < 0) {
            Tcl_AppendResult(interp, "error reading prologue file \"", 
                     fileName, "\": ", Tcl_PosixError(interp), 
                     (char *)NULL);
            Tcl_Close(interp, channel);
            Tcl_DStringFree(&ds);
            return TCL_ERROR;
        }
        if (numBytes == 0) {
            break;
        }
        buf[numBytes] = '\0';
        Blt_Ps_Append(ps, buf);
    }
    Tcl_DStringFree(&ds);
    Tcl_Close(interp, channel);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * XColorToPostScript --
 *
 *      Convert the a XColor (from its RGB values) to a PostScript command.
 *      If a Tk color map variable exists, it will be consulted for a
 *      PostScript translation based upon the color name.
 *
 *      Maps an X color intensity (0 to 2^16-1) to a floating point value
 *      [0..1].  Many versions of Tk don't properly handle the the lower 8
 *      bits of the color intensity, so we can only consider the upper 8
 *      bits.
 *
 * Results:
 *      The string representing the color mode is returned.
 *
 *---------------------------------------------------------------------------
 */
static void
XColorToPostScript(Blt_Ps ps, XColor *colorPtr)
{
    /* 
     * Shift off the lower byte before dividing because some versions of Tk
     * don't fill the lower byte correctly.
     */
    Blt_Ps_Format(ps, "%g %g %g",
        ((double)(colorPtr->red >> 8) / 255.0),
        ((double)(colorPtr->green >> 8) / 255.0),
        ((double)(colorPtr->blue >> 8) / 255.0));
}

void
Blt_Ps_XSetBackground(PostScript *psPtr, XColor *colorPtr)
{
    /* If the color name exists in TCL array variable, use that
     * translation */
    if ((psPtr->setupPtr != NULL) && (psPtr->setupPtr->colorVarName != NULL)) {
        const char *psColor;

        psColor = Tcl_GetVar2(psPtr->interp, psPtr->setupPtr->colorVarName,
            Tk_NameOfColor(colorPtr), 0);
        if (psColor != NULL) {
            Blt_Ps_VarAppend(psPtr, " ", psColor, "\n", (char *)NULL);
            return;
        }
    }
    XColorToPostScript(psPtr, colorPtr);
    Blt_Ps_Append(psPtr, " setrgbcolor\n");
    if (psPtr->setupPtr->flags & PS_GREYSCALE) {
        Blt_Ps_Append(psPtr, " currentgray setgray\n");
    }
}

void
Blt_Ps_XSetForeground(PostScript *psPtr, XColor *colorPtr)
{
    /* If the color name exists in TCL array variable, use that
     * translation */
    if ((psPtr->setupPtr != NULL) && (psPtr->setupPtr->colorVarName != NULL)) {
        const char *psColor;

        psColor = Tcl_GetVar2(psPtr->interp, psPtr->setupPtr->colorVarName,
            Tk_NameOfColor(colorPtr), 0);
        if (psColor != NULL) {
            Blt_Ps_VarAppend(psPtr, " ", psColor, "\n", (char *)NULL);
            return;
        }
    }
    XColorToPostScript(psPtr, colorPtr);
    Blt_Ps_Append(psPtr, " setrgbcolor\n");
    if (psPtr->setupPtr->flags & PS_GREYSCALE) {
        Blt_Ps_Append(psPtr, " currentgray setgray\n");
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ReverseBits --
 *
 *      Convert a byte from a X image into PostScript image order.  This
 *      requires not only the nybbles to be reversed but also their bit
 *      values.
 *
 * Results:
 *      The converted byte is returned.
 *
 *---------------------------------------------------------------------------
 */
INLINE static unsigned char
ReverseBits(unsigned char byte)
{
    byte = ((byte >> 1) & 0x55) | ((byte << 1) & 0xaa);
    byte = ((byte >> 2) & 0x33) | ((byte << 2) & 0xcc);
    byte = ((byte >> 4) & 0x0f) | ((byte << 4) & 0xf0);
    return byte;
}

/*
 *---------------------------------------------------------------------------
 *
 * ByteToHex --
 *
 *      Convert a byte to its ASCII hexidecimal equivalent.
 *
 * Results:
 *      The converted 2 ASCII character string is returned.
 *
 *---------------------------------------------------------------------------
 */
INLINE static void
ByteToHex(unsigned char byte, char *string)
{
    static char hexDigits[] = "0123456789ABCDEF";

    string[0] = hexDigits[byte >> 4];
    string[1] = hexDigits[byte & 0x0F];
}

#ifdef WIN32
/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ps_XSetBitmapData --
 *
 *      Output a PostScript image string of the given bitmap image.  It is
 *      assumed the image is one bit deep and a zero value indicates an
 *      off-pixel.  To convert to PostScript, the bits need to be reversed
 *      from the X11 image order.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The PostScript image string is appended.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Ps_XSetBitmapData(
    PostScript *psPtr,
    Display *display,
    Pixmap bitmap,
    int width, int height)
{
    int x, y, bitPos;
    unsigned long pixel;
    int byteCount;
    char string[10];
    unsigned char *srcBits;
    int bytesPerRow;

    srcBits = Blt_GetBitmapData(display, bitmap, width, height, &bytesPerRow);
    if (srcBits == NULL) {
        OutputDebugString("Can't get bitmap data");
        return;
    }
    Blt_Ps_Append(psPtr, "\t<");
    byteCount = bitPos = 0;             /* Suppress compiler warning */
    for (y = height - 1; y >= 0; y--) {
        unsigned char byte;
        unsigned char *srcPtr;

        srcPtr = srcBits + (bytesPerRow * y);
        byte = 0;
        for (x = 0; x < width; x++) {
            bitPos = x % 8;
            pixel = (*srcPtr & (0x80 >> bitPos));
            if (pixel) {
                byte |= (unsigned char)(1 << bitPos);
            }
            if (bitPos == 7) {
                byte = ReverseBits(byte);
                ByteToHex(byte, string);
                string[2] = '\0';
                byteCount++;
                srcPtr++;
                byte = 0;
                if (byteCount >= 30) {
                    string[2] = '\n';
                    string[3] = '\t';
                    string[4] = '\0';
                    byteCount = 0;
                }
                Blt_Ps_Append(psPtr, string);
            }
        }                       /* x */
        if (bitPos != 7) {
            byte = ReverseBits(byte);
            ByteToHex(byte, string);
            string[2] = '\0';
            Blt_Ps_Append(psPtr, string);
            byteCount++;
        }
    }                           /* y */
    Blt_Free(srcBits);
    Blt_Ps_Append(psPtr, ">\n");
}

#else

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ps_XSetBitmapData --
 *
 *      Output a PostScript image string of the given bitmap image.  It is
 *      assumed the image is one bit deep and a zero value indicates an
 *      off-pixel.  To convert to PostScript, the bits need to be reversed
 *      from the X11 image order.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The PostScript image string is appended to interp->result.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Ps_XSetBitmapData(Blt_Ps ps, Display *display, Pixmap bitmap, int w, int h)
{
    XImage *imagePtr;
    int byteCount;
    int y, bitPos;

    imagePtr = XGetImage(display, bitmap, 0, 0, w, h, 1, ZPixmap);
    Blt_Ps_Append(ps, "       <");
    byteCount = bitPos = 0;     /* Suppress compiler warning */
    for (y = 0; y < h; y++) {
        unsigned char byte;
        char string[10];
        int x;

        byte = 0;
        for (x = 0; x < w; x++) {
            unsigned long pixel;

            pixel = XGetPixel(imagePtr, x, y);
            bitPos = x % 8;
            byte |= (unsigned char)(pixel << bitPos);
            if (bitPos == 7) {
                byte = ReverseBits(byte);
                ByteToHex(byte, string);
                string[2] = '\0';
                byteCount++;
                byte = 0;
                if (byteCount >= 30) {
                    string[2] = '\n';
                    string[3] = '\t';
                    string[4] = '\0';
                    byteCount = 0;
                }
                Blt_Ps_Append(ps, string);
            }
        }                       /* x */
        if (bitPos != 7) {
            byte = ReverseBits(byte);
            ByteToHex(byte, string);
            byteCount++;
            string[2] = '\0';
            if (byteCount >= 30) {
                string[2] = '\n';
                string[3] = '\t';
                string[4] = '\0';
                byteCount = 0;
            }
            Blt_Ps_Append(ps, string);
        }
    }                           /* y */
    Blt_Ps_Append(ps, ">\n");
    XDestroyImage(imagePtr);
}

#endif /* WIN32 */


/*
 *---------------------------------------------------------------------------
 * Routines to convert X drawing functions to PostScript commands.
 *---------------------------------------------------------------------------
 */
void
Blt_Ps_SetClearBackground(Blt_Ps ps)
{
    Blt_Ps_Append(ps, "1 1 1 setrgbcolor\n");
}

void
Blt_Ps_XSetCapStyle(Blt_Ps ps, int capStyle)
{
    /*
     * X11:not last = 0, butt = 1, round = 2, projecting = 3
     * PS: butt = 0, round = 1, projecting = 2
     */
    if (capStyle > 0) {
        capStyle--;
    }
    Blt_Ps_Format(ps, "%d setlinecap\n", capStyle);
}

void
Blt_Ps_XSetJoinStyle(Blt_Ps ps, int joinStyle)
{
    /*
     * miter = 0, round = 1, bevel = 2
     */
    Blt_Ps_Format(ps, "%d setlinejoin\n", joinStyle);
}

void
Blt_Ps_XSetLineWidth(Blt_Ps ps, int lineWidth)
{
    if (lineWidth < 1) {
        lineWidth = 1;
    }
    Blt_Ps_Format(ps, "%d setlinewidth\n", lineWidth);
}

void
Blt_Ps_XSetDashes(Blt_Ps ps, Blt_Dashes *dashesPtr)
{

    Blt_Ps_Append(ps, "[ ");
    if (dashesPtr != NULL) {
        unsigned char *p;

        for (p = dashesPtr->values; *p != 0; p++) {
            unsigned int value;

            value = (unsigned int)*p;
            fprintf(stderr, "value=%d\n", value);
            if (value == 0) {
                break;
            }
            Blt_Ps_Format(ps, " %d", value);
        }
    }
    Blt_Ps_Append(ps, "] 0 setdash\n");
}

void
Blt_Ps_XSetLineAttributes(
    Blt_Ps ps,
    XColor *colorPtr,
    int lineWidth,
    Blt_Dashes *dashesPtr,
    int capStyle, 
    int joinStyle)
{
    Blt_Ps_XSetJoinStyle(ps, joinStyle);
    Blt_Ps_XSetCapStyle(ps, capStyle);
    Blt_Ps_XSetForeground(ps, colorPtr);
    Blt_Ps_XSetLineWidth(ps, lineWidth);
    Blt_Ps_XSetDashes(ps, dashesPtr);
    Blt_Ps_Append(ps, "/DashesProc {} def\n");
}

void
Blt_Ps_Rectangle(Blt_Ps ps, int x, int y, int width, int height)
{
    Blt_Ps_Append(ps, "newpath\n");
    Blt_Ps_Format(ps, "  %d %d moveto\n", x, y);
    Blt_Ps_Format(ps, "  %d %d rlineto\n", width, 0);
    Blt_Ps_Format(ps, "  %d %d rlineto\n", 0, height);
    Blt_Ps_Format(ps, "  %d %d rlineto\n", -width, 0);
    Blt_Ps_Append(ps, "closepath\n");
}

void
Blt_Ps_Rectangle2(Blt_Ps ps, double x1, double y1, double x2, double y2)
{
    Blt_Ps_Append(ps, "newpath\n");
    Blt_Ps_Format(ps, "  %g %g moveto\n", x1, y1);
    Blt_Ps_Format(ps, "  %g %g lineto\n", x1, y2);
    Blt_Ps_Format(ps, "  %g %g lineto\n", x2, y2);
    Blt_Ps_Format(ps, "  %g %g lineto\n", x2, y1);
    Blt_Ps_Append(ps, "closepath\n");
}

void
Blt_Ps_XFillRectangle(Blt_Ps ps, double x, double y, int width, int height)
{
    Blt_Ps_Rectangle(ps, (int)x, (int)y, width, height);
    Blt_Ps_Append(ps, "fill\n");
}

void
Blt_Ps_PolylineFromXPoints(Blt_Ps ps, int n, XPoint *points)
{
    int i;

    Blt_Ps_Append(ps, "newpath\n");
    Blt_Ps_Format(ps, "  %d %d moveto\n", points[0].x, points[0].y);
    for (i = 1; i < n; i++) {
        Blt_Ps_Format(ps, "  %d %d lineto\n", points[i].x, points[i].y);
    }
}

void
Blt_Ps_Polyline(Blt_Ps ps, int numScreenPts, Point2d *screenPts)
{
    int i;

    Blt_Ps_Append(ps, "newpath\n");
    Blt_Ps_Format(ps, "  %g %g moveto\n", screenPts[0].x, screenPts[0].y);
    for (i = 1; i < numScreenPts; i++) {
        Blt_Ps_Format(ps, "  %g %g lineto\n", screenPts[i].x, screenPts[i].y);
    }
}

void
Blt_Ps_Polygon(Blt_Ps ps, Point2d *screenPts, int numScreenPts)
{
    int i;

    Blt_Ps_Append(ps, "newpath\n");
    Blt_Ps_Format(ps, "  %g %g moveto\n", screenPts[0].x, screenPts[0].y);
    for (i = 1; i < numScreenPts; i++) {
        Blt_Ps_Format(ps, "  %g %g lineto\n", screenPts[i].x, screenPts[i].y);
    }
    /* Close the path. */
    Blt_Ps_Format(ps, "  %g %g lineto\n", screenPts[0].x, screenPts[0].y);
    Blt_Ps_Append(ps, "closepath\n");
}

void
Blt_Ps_XFillPolygon(Blt_Ps ps, int numScreenPts, Point2d *screenPts)
{
    Blt_Ps_Polygon(ps, screenPts, numScreenPts);
    Blt_Ps_Append(ps, "fill\n");
}

void
Blt_Ps_XDrawSegments(Blt_Ps ps, int numSegments, XSegment *segments)
{
    XSegment *sp, *send;

    for (sp = segments, send = sp + numSegments; sp < send; sp++) {
        Blt_Ps_Format(ps, "%d %d moveto %d %d lineto\n", sp->x1, sp->y1, 
                      sp->x2, sp->y2);
        Blt_Ps_Append(ps, "DashesProc stroke\n");
    }
}


void
Blt_Ps_XFillRectangles(Blt_Ps ps, int numRectangles, XRectangle *rectangles)
{
    XRectangle *rp, *rend;

    for (rp = rectangles, rend = rp + numRectangles; rp < rend; rp++) {
        Blt_Ps_XFillRectangle(ps, (double)rp->x, (double)rp->y, 
                (int)rp->width, (int)rp->height);
    }
}

#ifndef TK_RELIEF_SOLID
#define TK_RELIEF_SOLID         -1      /* Set the an impossible value. */
#endif /* TK_RELIEF_SOLID */

void
Blt_Ps_Draw3DRectangle(
    Blt_Ps ps,
    Tk_3DBorder border,         /* Token for border to draw. */
    double x, double y,         /* Coordinates of rectangle */
    int width, int height,      /* Region to be drawn. */
    int borderWidth,            /* Desired width for border, in pixels. */
    int relief)                 /* Should be either TK_RELIEF_RAISED or
                                 * TK_RELIEF_SUNKEN; indicates position of
                                 * interior of window relative to
                                 * exterior. */
{
    Point2d points[7];
    TkBorder *borderPtr = (TkBorder *) border;
    XColor *lightPtr, *darkPtr;
    XColor *topPtr, *bottomPtr;
    XColor light, dark;
    int twiceWidth = (borderWidth * 2);

    if ((width < twiceWidth) || (height < twiceWidth)) {
        return;
    }
    if ((relief == TK_RELIEF_SOLID) ||
        (borderPtr->lightColor == NULL) || (borderPtr->darkColor == NULL)) {
        if (relief == TK_RELIEF_SOLID) {
            dark.red = dark.blue = dark.green = 0x00;
            light.red = light.blue = light.green = 0x00;
            relief = TK_RELIEF_SUNKEN;
        } else {
            light = *borderPtr->bgColor;
            dark.red = dark.blue = dark.green = 0xFF;
        }
        lightPtr = &light;
        darkPtr = &dark;
    } else {
        lightPtr = borderPtr->lightColor;
        darkPtr = borderPtr->darkColor;
    }


    /* Handle grooves and ridges with recursive calls. */

    if ((relief == TK_RELIEF_GROOVE) || (relief == TK_RELIEF_RIDGE)) {
        int halfWidth, insideOffset;

        halfWidth = borderWidth / 2;
        insideOffset = borderWidth - halfWidth;
        Blt_Ps_Draw3DRectangle(ps, border, (double)x, (double)y,
            width, height, halfWidth, 
            (relief == TK_RELIEF_GROOVE) ? TK_RELIEF_SUNKEN : TK_RELIEF_RAISED);
        Blt_Ps_Draw3DRectangle(ps, border, 
            (double)(x + insideOffset), (double)(y + insideOffset), 
            width - insideOffset * 2, height - insideOffset * 2, halfWidth,
            (relief == TK_RELIEF_GROOVE) ? TK_RELIEF_RAISED : TK_RELIEF_SUNKEN);
        return;
    }
    if (relief == TK_RELIEF_RAISED) {
        topPtr = lightPtr;
        bottomPtr = darkPtr;
    } else if (relief == TK_RELIEF_SUNKEN) {
        topPtr = darkPtr;
        bottomPtr = lightPtr;
    } else {
        topPtr = bottomPtr = borderPtr->bgColor;
    }
    Blt_Ps_XSetBackground(ps, bottomPtr);
    Blt_Ps_XFillRectangle(ps, x, y + height - borderWidth, width, borderWidth);
    Blt_Ps_XFillRectangle(ps, x + width - borderWidth, y, borderWidth, height);
    points[0].x = points[1].x = points[6].x = x;
    points[0].y = points[6].y = y + height;
    points[1].y = points[2].y = y;
    points[2].x = x + width;
    points[3].x = x + width - borderWidth;
    points[3].y = points[4].y = y + borderWidth;
    points[4].x = points[5].x = x + borderWidth;
    points[5].y = y + height - borderWidth;
    if (relief != TK_RELIEF_FLAT) {
        Blt_Ps_XSetBackground(ps, topPtr);
    }
    Blt_Ps_XFillPolygon(ps, 7, points);
}

void
Blt_Ps_Fill3DRectangle(
    Blt_Ps ps,
    Tk_3DBorder border,         /* Token for border to draw. */
    double x, double y,         /* Coordinates of top-left of border area */
    int width, int height,      /* Dimension of border to be drawn. */
    int borderWidth,            /* Desired width for border, in pixels. */
    int relief)                 /* Should be either TK_RELIEF_RAISED or
                                 * TK_RELIEF_SUNKEN; indicates position of
                                 * interior of window relative to
                                 * exterior. */
{
    TkBorder *borderPtr = (TkBorder *) border;

    Blt_Ps_XSetBackground(ps, borderPtr->bgColor);
    Blt_Ps_XFillRectangle(ps, x, y, width, height);
    Blt_Ps_Draw3DRectangle(ps, border, x, y, width, height, borderWidth,
        relief);
}

void
Blt_Ps_XSetStipple(Blt_Ps ps, Display *display, Pixmap bitmap)
{
    int width, height;

    Tk_SizeOfBitmap(display, bitmap, &width, &height);
    Blt_Ps_Format(ps, 
        "gsave\n"
        "  clip\n"
        "  %d %d\n", 
        width, height);
    Blt_Ps_XSetBitmapData(ps, display, bitmap, width, height);
    Blt_Ps_VarAppend(ps, 
        "  StippleFill\n"
        "grestore\n", (char *)NULL);
}

static void
Base85Encode(PostScript *psPtr, Blt_DBuffer dBuffer)
{
    unsigned char *dp; 
    int count;
    int extra, numBytes, oldLength;
    int n, remainder;
    unsigned char *sp, *send;

    oldLength = Blt_DBuffer_Length(psPtr->dbuffer);
    numBytes = Blt_DBuffer_Length(dBuffer);

    /*
     * Compute worst-case length of buffer needed for encoding.  That is 5
     * characters per 4 bytes.  There are newlines every 65 characters. The
     * actual size can be smaller, depending upon the number of 0 tuples
     * ('z' bytes).
     */
    extra = ((numBytes + 3)/4) * 5;     /* 5 characters per 4 bytes. */
    remainder = extra % 4;
    extra += (numBytes + 64) / 65;     /* # of newlines. */
    Blt_DBuffer_SetLength(psPtr->dbuffer, extra + oldLength);

    n = oldLength;
    count = 0;
    dp = Blt_DBuffer_Bytes(psPtr->dbuffer) + oldLength;
    for (sp = Blt_DBuffer_Bytes(dBuffer), send = sp + (numBytes - remainder);
         sp < send; sp += 4) {
        unsigned int tuple;
        
        tuple = 0;
#ifdef WORDS_BIGENDIAN
        tuple |= (sp[3] << 24);
        tuple |= (sp[2] << 16); 
        tuple |= (sp[1] <<  8);
        tuple |= sp[0];
#else 
        tuple |= (sp[0] << 24);
        tuple |= (sp[1] << 16); 
        tuple |= (sp[2] <<  8);
        tuple |= sp[3];
#endif
        if (tuple == 0) {
            *dp++ = 'z';
            count++;
            n++;
        } else {
            dp[4] = '!' + (tuple % 85);
            tuple /= 85;
            dp[3] = '!' + (tuple % 85);
            tuple /= 85;
            dp[2] = '!' + (tuple % 85);
            tuple /= 85;
            dp[1] = '!' + (tuple % 85);
            tuple /= 85;
            dp[0] = '!' + (tuple % 85);
            dp += 5;
            n += 5;
            count += 5;
        }
        if (count > 64) {
            *dp++ = '\n';
            n++;
            count = 0;
        }
    }
    
    {
        unsigned int tuple;

        /* Handle remaining bytes (0-3). */
        numBytes = (numBytes & 0x3);
        sp -= numBytes;
        tuple = 0;
        switch (numBytes) {
#ifdef WORDS_BIGENDIAN
        case 3:
            tuple |= (sp[2] <<  8);
        case 2:
            tuple |= (sp[1] << 16); 
        case 1:
            tuple |= (sp[0] << 24);
#else
        case 3:
            tuple |= (sp[2] << 24);
        case 2:
            tuple |= (sp[1] << 16); 
        case 1:
            tuple |= (sp[0] <<  8);
#endif
        default:
            break;
        }
        if (numBytes > 0) {
            tuple /= 85;        
            if (numBytes > 2) {
                dp[3] = '!' + (tuple % 85);
                n++;
            }
            tuple /= 85;        
            if (numBytes > 1) { 
                dp[2] = '!' + (tuple % 85);
                n++;
            }
            tuple /= 85;        
            dp[1] = '!' + (tuple % 85);
            tuple /= 85;        
            dp[0] = '!' + (tuple % 85);
            *dp++ = '\n';
            n += 3;
        }
        Blt_DBuffer_SetLength(psPtr->dbuffer, n);
    }
}

static void
AsciiHexEncode(PostScript *psPtr, Blt_DBuffer dBuffer)
{
    static char hexDigits[] = "0123456789ABCDEF";
    int length, oldLength;
    int numBytes;

    /*
     * Compute the length of the buffer needed for the encoding.  That is 2
     * characters per byte.  There are newlines every 64 characters.
     */
    length = oldLength = Blt_DBuffer_Length(psPtr->dbuffer);
    numBytes =  Blt_DBuffer_Length(dBuffer) * 2;  /* Two characters per byte */
    length += numBytes;         
    length += (numBytes+63)/64; /* Number of newlines required. */
    Blt_DBuffer_SetLength(psPtr->dbuffer, length);
    {
        unsigned char *sp, *send;
        unsigned char *dp; 
        int count, n;

        count = n = 0;
        dp = Blt_DBuffer_Bytes(psPtr->dbuffer) + oldLength;
        for (sp = Blt_DBuffer_Bytes(dBuffer), 
                 send = sp + Blt_DBuffer_Length(dBuffer); 
             sp < send; sp++) {
            dp[0] = hexDigits[*sp >> 4];
            dp[1] = hexDigits[*sp & 0x0F];
            dp += 2;
            n += 2;
            count++;
            if ((count & 0x1F) == 0) {
                *dp++ = '\n';
                n++;
            }
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ps_DrawPicture --
 *
 *      Translates a picture into 3 component RGB PostScript output.  Uses
 *      PS Language Level 2 operator "colorimage".
 *
 * Results:
 *      The dynamic string will contain the PostScript output.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Ps_DrawPicture(PostScript *psPtr, Blt_Picture picture, double x, double y)
{
    PageSetup *setupPtr = psPtr->setupPtr;
    int w, h;
    Blt_DBuffer dBuffer;

    w = Blt_Picture_Width(picture);
    h = Blt_Picture_Height(picture);
    Blt_Ps_Format(psPtr, 
        "gsave\n"
        "/DeviceRGB setcolorspace\n"
        "%g %g translate\n"
        "%d %d scale\n", x, y, w, h);
    if ((setupPtr->flags & PS_GREYSCALE) || (setupPtr->level == 1)) {
        int strSize;

        strSize = (setupPtr->flags & PS_GREYSCALE) ? w : w * 3;
        Blt_Ps_Format(psPtr, 
            "/picstr %d string def\n"
            "%d %d 8\n" 
            "[%d 0 0 %d 0 %d]\n"
            "{\n"
            "  currentfile picstr readhexstring pop\n"
            "}\n", strSize, w, h, w, -h, h);
        if (setupPtr->flags & PS_GREYSCALE) {
            Blt_Picture greyscale;
            
            Blt_Ps_Append(psPtr, "image\n");
            greyscale = Blt_GreyscalePicture(picture);
            dBuffer = Blt_PictureToDBuffer(picture, 1);
            Blt_FreePicture(greyscale);
        } else {
            Blt_Ps_Append(psPtr, "false 3 colorimage\n");
            dBuffer = Blt_PictureToDBuffer(picture, 3);
        }
        AsciiHexEncode(psPtr, dBuffer);
        Blt_DBuffer_Free(dBuffer);
    } else {
        Blt_Ps_Format(psPtr, 
            "<<\n"
            "/ImageType 1\n"
            "/Width %d\n"
            "/Height %d\n"
            "/BitsPerComponent 8\n"
            "/Decode [0 1 0 1 0 1]\n"
            "/ImageMatrix [%d 0 0 %d 0 %d]\n"
            "/Interpolate true\n"
            "/DataSource  currentfile /ASCII85Decode filter\n"
            ">>\n"
            "image\n", w, h, w, -h, h);
        dBuffer = Blt_PictureToDBuffer(picture, 3);
        Base85Encode(psPtr, dBuffer);
        Blt_DBuffer_Free(dBuffer);
    } 
    Blt_Ps_Append(psPtr, "\ngrestore\n\n");
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ps_XDrawWindow --
 *
 *      Converts a Tk window to PostScript.  If the window could not be
 *      "snapped", then a grey rectangle is drawn in its place.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Ps_XDrawWindow(Blt_Ps ps, Tk_Window tkwin, double x, double y)
{
    Blt_Picture picture;

    picture = Blt_DrawableToPicture(tkwin, Tk_WindowId(tkwin), 0, 0, 
        Tk_Width(tkwin), Tk_Height(tkwin), GAMMA);
    if (picture == NULL) {
        /* Can't grab window image so paint the window area grey */
        Blt_Ps_VarAppend(ps, "% Can't grab window \"", Tk_PathName(tkwin),
                "\"\n", (char *)NULL);
        Blt_Ps_Append(ps, "0.5 0.5 0.5 setrgbcolor\n");
        Blt_Ps_XFillRectangle(ps, x, y, Tk_Width(tkwin), Tk_Height(tkwin));
        return;
    }
    Blt_Ps_DrawPicture(ps, picture, x, y);
    Blt_FreePicture(picture);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ps_DrawPhoto --
 *
 *      Output a PostScript image string of the given photo image.  The
 *      photo is first converted into a picture and then translated into
 *      PostScript.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The PostScript output representing the photo is appended to the
 *      psPtr's dynamic string.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Ps_DrawPhoto(Blt_Ps ps, Tk_PhotoHandle photo, double x, double y)
{
    Blt_Picture picture;

    picture = Blt_PhotoToPicture(photo);
    Blt_Ps_DrawPicture(ps, picture, x, y);
    Blt_FreePicture(picture);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ps_XSetFont --
 *
 *      Map the Tk font to a PostScript font and point size.
 *
 *      If a TCL array variable was specified, each element should be
 *      indexed by the X11 font name and contain a list of 1-2 elements;
 *      the PostScript font name and the desired point size.  The point
 *      size may be omitted and the X font point size will be used.
 *
 *      Otherwise, if the foundry is "Adobe", we try to do a plausible
 *      mapping looking at the full name of the font and building a string
 *      in the form of "Family-TypeFace".
 *
 * Returns:
 *      None.
 *
 * Side Effects:
 *      PostScript commands are output to change the type and the point
 *      size of the current font.
 *
 *---------------------------------------------------------------------------
 */

void
Blt_Ps_XSetFont(PostScript *psPtr, Blt_Font font) 
{
    Tcl_Interp *interp = psPtr->interp;
    const char *family;

    /* Use the font variable information if it exists. */
    if ((psPtr->setupPtr != NULL) && (psPtr->setupPtr->fontVarName != NULL)) {
        Tcl_Obj *valueObjPtr;

        valueObjPtr = Tcl_GetVar2Ex(interp, psPtr->setupPtr->fontVarName, 
                                    Blt_Font_Name(font), 0);
        if (valueObjPtr != NULL) {
            Tcl_Obj **objv = NULL;
            int objc;
            int newSize;
            double pointSize;
            const char *fontName;

            if (Tcl_ListObjGetElements(NULL, valueObjPtr, &objc, &objv)
                != TCL_OK) {
                return;
            }
            fontName = Tcl_GetString(objv[0]);
            if ((objc != 2) || 
                (Tcl_GetIntFromObj(interp, objv[1], &newSize) != TCL_OK)) {
                return;
            }
            pointSize = (double)newSize;
            Blt_Ps_Format(psPtr, "%g /%s SetFont\n", pointSize, fontName);
            return;
        }
        /*FallThru*/
    }

    /*
     * Check to see if it's a PostScript font. Tk_PostScriptFontName silently
     * generates a bogus PostScript font name, so we have to check to see if
     * this is really a PostScript font first before we call it.
     */
    family = Blt_Afm_GetPostscriptFamily(Blt_Font_Family(font));
    if (family != NULL) {
        Tcl_DString ds;
        double pointSize;
        
        Tcl_DStringInit(&ds);
        pointSize = (double)Blt_Font_PostscriptName(font, &ds);
        pointSize = Blt_Font_PointSize(font);
        Blt_Ps_Format(psPtr, "%g /%s SetFont\n", pointSize, 
                Tcl_DStringValue(&ds));
        Tcl_DStringFree(&ds);
        return;
    }
    Blt_Ps_Format(psPtr, "%g /Helvetica-Bold SetFont\n", 
                  Blt_Font_PointSize(font));
}

void
Blt_Ps_TextString(Blt_Ps ps, const char *string, int numBytes) 
{
    char *bp, *dst;
    int count;                  /* Counts the # of bytes written to the
                                 * intermediate scratch buffer. */
    const char *src, *end;
    unsigned char c;
#if HAVE_UTF
    Tcl_UniChar ch;
#endif
    int limit;

    limit = POSTSCRIPT_BUFSIZ - 4; /* High water mark for scratch buffer. */

    Blt_Ps_Append(ps, "(");
    count = 0;
    dst = Blt_Ps_GetScratchBuffer(ps);
    src = string;
    end = string + numBytes;
    while (src < end) {
        if (count > limit) {
            /* Don't let the scatch buffer overflow */
            dst = Blt_Ps_GetScratchBuffer(ps);
            dst[count] = '\0';
            Blt_Ps_Append(ps, dst);
            count = 0;
        }
#if HAVE_UTF
        /*
         * INTL: For now we just treat the characters as binary data and
         * display the lower byte.  Eventually this should be revised to
         * handle international postscript fonts.
         */
        src += Tcl_UtfToUniChar(src, &ch);
        c = (unsigned char)(ch & 0xff);
#else 
        c = *src++;
#endif
        
        if ((c == '\\') || (c == '(') || (c == ')')) {
            /*
             * If special PostScript characters characters "\", "(", and
             * ")" are contained in the text string, prepend backslashes
             * to them.
             */
            *dst++ = '\\';
            *dst++ = c;
            count += 2;
        } else if ((c < ' ') || (c > '~')) {
            /* Convert non-printable characters into octal. */
            Blt_FmtString(dst, 5, "\\%03o", c);
            dst += 4;
            count += 4;
        } else {
            *dst++ = c;
            count++;
        }
    }
    bp = Blt_Ps_GetScratchBuffer(ps);
    bp[count] = '\0';
    Blt_Ps_Append(ps, bp);
    Blt_Ps_Append(ps, ")");
}    

void
Blt_Ps_TextLayout(Blt_Ps ps, int x, int y, TextLayout *textPtr)
{
    TextFragment *fragPtr;
    int i;

    fragPtr = textPtr->fragments;
    for (i = 0; i < textPtr->numFragments; i++, fragPtr++) {
        if (fragPtr->numBytes > 0) {
            Blt_Ps_TextString(ps, fragPtr->text, fragPtr->numBytes);
            Blt_Ps_Format(ps, " %d %d %d DrawAdjText\n",
                          fragPtr->w, x + fragPtr->x, y + fragPtr->y);
        }
    }
}


static void
TextLayoutToPostScript(Blt_Ps ps, int x, int y, TextLayout *textPtr)
{
    char *bp, *dst;
    int count;                  /* Counts the # of bytes written to the
                                 * intermediate scratch buffer. */
    const char *src, *end;
    TextFragment *fragPtr;
    int i;
    unsigned char c;
#if HAVE_UTF
    Tcl_UniChar ch;
#endif
    int limit;

    limit = POSTSCRIPT_BUFSIZ - 4; /* High water mark for scratch buffer. */
    fragPtr = textPtr->fragments;
    for (i = 0; i < textPtr->numFragments; i++, fragPtr++) {
        if (fragPtr->numBytes < 1) {
            continue;
        }
        Blt_Ps_Append(ps, "(");
        count = 0;
        dst = Blt_Ps_GetScratchBuffer(ps);
        src = fragPtr->text;
        end = fragPtr->text + fragPtr->numBytes;
        while (src < end) {
            if (count > limit) {
                /* Don't let the scatch buffer overflow */
                dst = Blt_Ps_GetScratchBuffer(ps);
                dst[count] = '\0';
                Blt_Ps_Append(ps, dst);
                count = 0;
            }
#if HAVE_UTF
            /*
             * INTL: For now we just treat the characters as binary data and
             * display the lower byte.  Eventually this should be revised to
             * handle international postscript fonts.
             */
            src += Tcl_UtfToUniChar(src, &ch);
            c = (unsigned char)(ch & 0xff);
#else 
            c = *src++;
#endif

            if ((c == '\\') || (c == '(') || (c == ')')) {
                /*
                 * If special PostScript characters characters "\", "(", and
                 * ")" are contained in the text string, prepend backslashes
                 * to them.
                 */
                *dst++ = '\\';
                *dst++ = c;
                count += 2;
            } else if ((c < ' ') || (c > '~')) {
                /* Convert non-printable characters into octal. */
                Blt_FmtString(dst, 5, "\\%03o", c);
                dst += 4;
                count += 4;
            } else {
                *dst++ = c;
                count++;
            }
        }
        bp = Blt_Ps_GetScratchBuffer(ps);
        bp[count] = '\0';
        Blt_Ps_Append(ps, bp);
        Blt_Ps_Format(ps, ") %d %d %d DrawAdjText\n",
            fragPtr->w, x + fragPtr->x, y + fragPtr->y);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ps_DrawText --
 *
 *      Output PostScript commands to print a text string. The string may be
 *      rotated at any arbitrary angle, and placed according the anchor type
 *      given. The anchor indicates how to interpret the window coordinates as
 *      an anchor for the text bounding box.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Text string is drawn using the given font and GC on the graph window
 *      at the given coordinates, anchor, and rotation
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Ps_DrawText(
    Blt_Ps ps,
    const char *string,         /* String to convert to PostScript */
    TextStyle *tsPtr,           /* Text attribute information */
    double x, double y)         /* Window coordinates where to print text */
{
    TextLayout *textPtr;
    Point2d t;

    if ((string == NULL) || (*string == '\0')) { /* Empty string, do nothing */
        return;
    }
    textPtr = Blt_Ts_CreateLayout(string, -1, tsPtr);
    {
        float angle;
        double rw, rh;
        
        angle = FMOD(tsPtr->angle, (double)360.0);
        Blt_GetBoundingBox((double)textPtr->width, (double)textPtr->height, 
                angle, &rw, &rh, (Point2d *)NULL);
        /*
         * Find the center of the bounding box
         */
        t = Blt_AnchorPoint(x, y, rw, rh, tsPtr->anchor); 
        t.x += rw * 0.5;
        t.y += rh * 0.5;
    }

    /* Initialize text (sets translation and rotation) */
    Blt_Ps_Format(ps, "%d %d %g %g %g BeginText\n", textPtr->width, 
        textPtr->height, tsPtr->angle, t.x, t.y);

    Blt_Ps_XSetFont(ps, tsPtr->font);

    Blt_Ps_XSetForeground(ps, tsPtr->color);
    TextLayoutToPostScript(ps, 0, 0, textPtr);
    Blt_Free(textPtr);
    Blt_Ps_Append(ps, "EndText\n");
}

void
Blt_Ps_XDrawLines(Blt_Ps ps, int n, XPoint *points)
{
    int numLeft;

    if (n <= 0) {
        return;
    }
    for (numLeft = n; numLeft > 0; numLeft -= PS_MAXPATH) {
        int length;

        length = MIN(PS_MAXPATH, numLeft);
        Blt_Ps_PolylineFromXPoints(ps, length, points);
        Blt_Ps_Append(ps, "DashesProc stroke\n");
        points += length;
    }
}

void
Blt_Ps_DrawPolyline(Blt_Ps ps, int numPoints, Point2d *points)
{
    int numLeft;

    if (numPoints <= 0) {
        return;
    }
    for (numLeft = numPoints; numLeft > 0; numLeft -= PS_MAXPATH) {
        int length;

        length = MIN(PS_MAXPATH, numLeft);
        Blt_Ps_Polyline(ps, length, points);
        Blt_Ps_Append(ps, "DashesProc stroke\n");
        points += length;
    }
}

void
Blt_Ps_DrawBitmap(
    Blt_Ps ps,
    Display *display,
    Pixmap bitmap,              /* Bitmap to be converted to PostScript */
    double xScale, double yScale)
{
    int width, height;
    double sw, sh;

    Tk_SizeOfBitmap(display, bitmap, &width, &height);
    sw = (double)width * xScale;
    sh = (double)height * yScale;
    Blt_Ps_Append(ps, "  gsave\n");
    Blt_Ps_Format(ps, "    %g %g translate\n", sw * -0.5, sh * 0.5);
    Blt_Ps_Format(ps, "    %g %g scale\n", sw, -sh);
    Blt_Ps_Format(ps, "    %d %d true [%d 0 0 %d 0 %d] {", 
        width, height, width, -height, height);
    Blt_Ps_XSetBitmapData(ps, display, bitmap, width, height);
    Blt_Ps_Append(ps, "    } imagemask\n  grestore\n");
}

void
Blt_Ps_DrawSegments2d(Blt_Ps ps, int numSegments, Segment2d *segments)
{
    Segment2d *sp, *send;

    Blt_Ps_Append(ps, "newpath\n");
    for (sp = segments, send = sp + numSegments; sp < send; sp++) {
        Blt_Ps_Format(ps, "  %g %g moveto %g %g lineto\n", 
                sp->p.x, sp->p.y, sp->q.x, sp->q.y);
        Blt_Ps_Append(ps, "DashesProc stroke\n");
    }
}

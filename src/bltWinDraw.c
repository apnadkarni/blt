/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltWinDraw.c --
 *
 * This module contains WIN32 routines not included in the Tcl/Tk
 * libraries.
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
#include <bltInt.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include "bltAlloc.h"
#include "bltMath.h"
#include <bltFont.h>
#include <bltText.h>
#include "tkDisplay.h"
#include "tkFont.h"
#include "tkIntBorder.h"

#define EXT_GC  ('\xFF')

/*
 * Data structure for setting graphics context.
 */
typedef struct {
    int function;                       /* Logical operation. */
    unsigned long plane_mask;           /* Plane mask. */
    unsigned long foreground;           /* Foreground pixel. */
    unsigned long background;           /* Background pixel. */
    int line_width;                     /* Line width. */
    int line_style;                     /* LineSolid, LineOnOffDash,
                                         * LineDoubleDash. */
    int cap_style;                      /* CapNotLast, CapButt, CapRound,
                                         * CapProjecting. */
    int join_style;                     /* JoinMiter, JoinRound, JoinBevel. */
    int fill_style;                     /* FillSolid, FillTiled, FillStippled,
                                         * FillOpaeueStippled. */
    int fill_rule;                      /* EvenOddRule, WindingRule. */
    int arc_mode;                       /* ArcChord, ArcPieSlice. */
    Pixmap tile;                        /* Pixmap for tiling operations. */
    Pixmap stipple;                     /* Stipple 1 plane pixmap for
                                         * stipping. */
    int ts_x_origin, ts_y_origin;       /* Offset for tile or stipple
                                         * operations. */
    Font font;                          /* Default text font for text
                                         * operations */
    int subwindow_mode;                 /* ClipByChildren, IncludeInferiors */
    Bool graphics_exposures;            /* Boolean, should exposures be
                                         * generated */
    int clip_x_origin, clip_y_origin;   /* origin for clipping */
    Pixmap clip_mask;                   /* Bitmap clipping; other calls for
                                         * rects */
    int dash_offset;                    /* Patterned/dashed line information. */
    char dashes;
    int numDashValues;
    char dashValues[12];
} XGCValuesEx;

static int tkpWinRopModes[] =
{
    R2_BLACK,                           /* GXclear */
    R2_MASKPEN,                         /* GXand */
    R2_MASKPENNOT,                      /* GXandReverse */
    R2_COPYPEN,                         /* GXcopy */
    R2_MASKNOTPEN,                      /* GXandInverted */
    R2_NOT,                             /* GXnoop */
    R2_XORPEN,                          /* GXxor */
    R2_MERGEPEN,                        /* GXor */
    R2_NOTMERGEPEN,                     /* GXnor */
    R2_NOTXORPEN,                       /* GXequiv */
    R2_NOT,                             /* GXinvert */
    R2_MERGEPENNOT,                     /* GXorReverse */
    R2_NOTCOPYPEN,                      /* GXcopyInverted */
    R2_MERGENOTPEN,                     /* GXorInverted */
    R2_NOTMASKPEN,                      /* GXnand */
    R2_WHITE                            /* GXset */
};

#define MASKPAT         0x00E20746 /* dest = (src & pat) | (!src & dst) */
#define COPYFG          0x00CA0749 /* dest = (pat & src) | (!pat & dst) */
#define COPYBG          0x00AC0744 /* dest = (!pat & src) | (pat & dst) */
/*
 * Translation table between X gc functions and Win32 BitBlt op modes.  Some
 * of the operations defined in X don't have names, so we have to construct
 * new opcodes for those functions.  This is arcane and probably not all that
 * useful, but at least it's accurate.
 */

#define NOTSRCAND       (DWORD)0x00220326 /* dest = (NOT src) AND dest */
#define NOTSRCINVERT    (DWORD)0x00990066 /* dest = (NOT src) XOR dest */
#define SRCORREVERSE    (DWORD)0x00DD0228 /* dest = src OR (NOT dest) */
#define SRCNAND         (DWORD)0x007700E6 /* dest = NOT (src AND dest) */

static int bltModes[] =
{
    BLACKNESS,                          /* GXclear */
    SRCAND,                             /* GXand */
    SRCERASE,                           /* GXandReverse */
    SRCCOPY,                            /* GXcopy */
    NOTSRCAND,                          /* GXandInverted */
    PATCOPY,                            /* GXnoop */
    SRCINVERT,                          /* GXxor */
    SRCPAINT,                           /* GXor */
    NOTSRCERASE,                        /* GXnor */
    NOTSRCINVERT,                       /* GXequiv */
    DSTINVERT,                          /* GXinvert */
    SRCORREVERSE,                       /* GXorReverse */
    NOTSRCCOPY,                         /* GXcopyInverted */
    MERGEPAINT,                         /* GXorInverted */
    SRCNAND,                            /* GXnand */
    WHITENESS                           /* GXset */
};

#if (_TCL_VERSION <  _VERSION(8,1,0)) 
typedef void *Tcl_Encoding;            /* Make up dummy type for encoding.  */
#endif

HPALETTE
Blt_GetSystemPalette(void)
{
    HDC hDC;
    HPALETTE hPalette;
    DWORD flags;

    hPalette = NULL;
    hDC = GetDC(NULL);                  /* Get the desktop device context */
    flags = GetDeviceCaps(hDC, RASTERCAPS);
    if (flags & RC_PALETTE) {
        LOGPALETTE *palettePtr;

        palettePtr = (LOGPALETTE *)
            GlobalAlloc(GPTR, sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));
        palettePtr->palVersion = 0x300;
        palettePtr->palNumEntries = 256;
        GetSystemPaletteEntries(hDC, 0, 256, palettePtr->palPalEntry);
        hPalette = CreatePalette(palettePtr);
        GlobalFree(palettePtr);
    }
    ReleaseDC(NULL, hDC);
    return hPalette;
}

typedef struct _DCInfo {
    TkWinDCState state;
    TkpClipMask *clipPtr;
    GC gc;
    Drawable drawable;
    HDC dc;
} DCState;

static HDC
GetDCState(Display *display, Drawable drawable, GC gc, DCState *statePtr)
{
    statePtr->clipPtr = (TkpClipMask *)gc->clip_mask;
    statePtr->dc = TkWinGetDrawableDC(display, drawable, &statePtr->state);
    statePtr->drawable = drawable;
    if ((statePtr->clipPtr != NULL) && 
        (statePtr->clipPtr->type==TKP_CLIP_REGION)) {
        SelectClipRgn(statePtr->dc, (HRGN)statePtr->clipPtr->value.region);
        OffsetClipRgn(statePtr->dc, gc->clip_x_origin, gc->clip_y_origin);
    }
    SetROP2(statePtr->dc, tkpWinRopModes[gc->function]);
    return statePtr->dc;
}

static void
FreeDCState(DCState *statePtr)
{
    if ((statePtr->clipPtr != NULL) && 
        (statePtr->clipPtr->type==TKP_CLIP_REGION)) {
        SelectClipRgn(statePtr->dc, NULL);
    }
    TkWinReleaseDrawableDC(statePtr->drawable, statePtr->dc, &statePtr->state);
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * CreateRotatedFont --
 *
 *      Creates a rotated copy of the given font.  This only works 
 *      for TrueType fonts.
 *
 * Results:
 *      Returns the newly create font or NULL if the font could not
 *      be created.
 *
 *---------------------------------------------------------------------------
 */
static HFONT
CreateRotatedFont(
    unsigned long fontId,       /* Font identifier (actually a Tk_Font) */
    float angle)
{                               /* Number of degrees to rotate font */
    TkFontAttributes *faPtr;    /* Set of attributes to match. */
    TkFont *fontPtr;
    HFONT hFont;
    LOGFONTW lf;

    fontPtr = (TkFont *) fontId;
    faPtr = &fontPtr->fa;
    ZeroMemory(&lf, sizeof(LOGFONT));
    lf.lfHeight = -faPtr->size;
    if (lf.lfHeight < 0) {
        HDC dc;

        dc = GetDC(NULL);
        lf.lfHeight = -MulDiv(faPtr->size, GetDeviceCaps(dc, LOGPIXELSY), 72);
        ReleaseDC(NULL, dc);
    }
    lf.lfWidth = 0;
    lf.lfEscapement = lf.lfOrientation = ROUND(angle * 10.0);
#define TK_FW_NORMAL    0
    lf.lfWeight = (faPtr->weight == TK_FW_NORMAL) ? FW_NORMAL : FW_BOLD;
    lf.lfItalic = faPtr->slant;
    lf.lfUnderline = faPtr->underline;
    lf.lfStrikeOut = faPtr->overstrike;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

    hFont = NULL;
    if (faPtr->family == NULL) {
        lf.lfFaceName[0] = '\0';
    } else {
#if (_TCL_VERSION >= _VERSION(8,1,0)) 
        Tcl_DString ds;

        Tcl_UtfToExternalDString(systemEncoding, faPtr->family, -1, &ds);

        if (Blt_GetPlatformId() == VER_PLATFORM_WIN32_NT) {
            Tcl_UniChar *src, *dst;
            
            /*
             * We can only store up to LF_FACESIZE wide characters
             */
            if (Tcl_DStringLength(&ds) >= (LF_FACESIZE * sizeof(WCHAR))) {
                Tcl_DStringSetLength(&ds, LF_FACESIZE);
            }
            src = (Tcl_UniChar *)Tcl_DStringValue(&ds);
            dst = (Tcl_UniChar *)lf.lfFaceName;
            while (*src != '\0') {
                *dst++ = *src++;
            }
            *dst = '\0';
            hFont = CreateFontIndirectW((LOGFONTW *)&lf);
        } else {
            /*
             * We can only store up to LF_FACESIZE characters
             */
            if (Tcl_DStringLength(&ds) >= LF_FACESIZE) {
                Tcl_DStringSetLength(&ds, LF_FACESIZE);
            }
            strcpy((char *)lf.lfFaceName, Tcl_DStringValue(&ds));
            hFont = CreateFontIndirectA((LOGFONTA *)&lf);
        }
        Tcl_DStringFree(&ds);
#else
        strncpy((char *)lf.lfFaceName, faPtr->family, LF_FACESIZE - 1);
        lf.lfFaceName[LF_FACESIZE] = '\0';
#endif /* _TCL_VERSION >= 8.1.0 */
    }

    if (hFont == NULL) {

    } else { 
        HFONT oldFont;
        TEXTMETRIC tm;
        HDC hRefDC;
        int result;

        /* Check if the rotated font is really a TrueType font. */

        hRefDC = GetDC(NULL);           /* Get the desktop device context */
        oldFont = SelectFont(hRefDC, hFont);
        result = ((GetTextMetrics(hRefDC, &tm)) && 
                  (tm.tmPitchAndFamily & TMPF_TRUETYPE));
        SelectFont(hRefDC, oldFont);
        ReleaseDC(NULL, hRefDC);
        if (!result) {
            DeleteFont(hFont);
            return NULL;
        }
    }
    return hFont;
}
#endif 
/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetBitmapData --
 *
 *      Returns the DIB bits from a bitmap.
 *
 * Results:
 *      Returns a byte array of bitmap data or NULL if an error
 *      occurred.  The parameter pitchPtr returns the number
 *      of bytes per row.
 *
 *---------------------------------------------------------------------------
 */
unsigned char *
Blt_GetBitmapData(
    Display *display,           /* Display of bitmap */
    Pixmap bitmap,              /* Bitmap to query */
    int w, int h,               /* Dimensions of bitmap */
    int *pitchPtr)              /* (out) Number of bytes per row */
{                       
    TkWinDCState state;
    HDC hDC;
    int result;
    unsigned char *bits;
    unsigned int size;
    HBITMAP hBitmap;
    BITMAPINFOHEADER *bmihPtr;
    HANDLE hMem, hMem2;
    int bytesPerRow, imageSize;

    size = sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD);
    hMem = GlobalAlloc(GHND, size);
    bmihPtr = (BITMAPINFOHEADER *)GlobalLock(hMem);
    bmihPtr->biSize = sizeof(BITMAPINFOHEADER);
    bmihPtr->biPlanes = 1;
    bmihPtr->biBitCount = 1;
    bmihPtr->biCompression = BI_RGB;
    bmihPtr->biWidth = w;
    bmihPtr->biHeight = h;

    hBitmap = ((TkWinDrawable *)bitmap)->bitmap.handle;
    hDC = TkWinGetDrawableDC(display, bitmap, &state);
    result = GetDIBits(hDC, hBitmap, 0, h, (LPVOID)NULL, (BITMAPINFO *)bmihPtr, 
        DIB_RGB_COLORS);
    TkWinReleaseDrawableDC(bitmap, hDC, &state);
    if (!result) {
        GlobalUnlock(hMem);
        GlobalFree(hMem);
        return NULL;
    }
    imageSize = bmihPtr->biSizeImage;
    GlobalUnlock(hMem);
    bytesPerRow = ((w + 31) & ~31) / 8;
    if (imageSize == 0) {
         imageSize = bytesPerRow * h;
    }   
    hMem2 = GlobalReAlloc(hMem, size + imageSize, 0);
    if (hMem2 == NULL) {
        GlobalFree(hMem);
        return NULL;
    }
    hMem = hMem2;
    bmihPtr = (BITMAPINFOHEADER *)GlobalLock(hMem);
    hDC = TkWinGetDrawableDC(display, bitmap, &state);
    result = GetDIBits(hDC, hBitmap, 0, h, (unsigned char *)bmihPtr + size,
        (BITMAPINFO *)bmihPtr, DIB_RGB_COLORS);
    TkWinReleaseDrawableDC(bitmap, hDC, &state);
    bits = NULL;
    if (!result) {
        OutputDebugString("GetDIBits failed\n");
    } else {
        bits = Blt_Malloc(imageSize);
        if (bits != NULL) {
            memcpy (bits, (unsigned char *)bmihPtr + size, imageSize);
        }
    }
    GlobalUnlock(hMem);
    GlobalFree(hMem);
    *pitchPtr = bytesPerRow;
    return bits;
}

/*
 *---------------------------------------------------------------------------
 *
 * XFree --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXFree(void *ptr)
{
    Blt_Free(ptr);
}

/*
 *---------------------------------------------------------------------------
 *
 * XMaxRequestSize --
 *
 *---------------------------------------------------------------------------
 */
long
Blt_EmulateXMaxRequestSize(Display *display)
{
    return (SHRT_MAX / 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * XLowerWindow --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXLowerWindow(
    Display *display, 
    Window window)
{
    HWND hWnd;

    hWnd = Tk_GetHWND(window);
    display->request++;
    SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

/*
 *---------------------------------------------------------------------------
 *
 * XRaiseWindow --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXRaiseWindow(
    Display *display, 
    Window window)
{
    HWND hWnd;

    hWnd = Tk_GetHWND(window);
    display->request++;
    SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

/*
 *---------------------------------------------------------------------------
 *
 * XUnmapWindow --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXUnmapWindow(
    Display *display, 
    Window window)
{
    HWND hWnd;

    hWnd = Tk_GetHWND(window);
    display->request++;
    ShowWindow(hWnd, SW_HIDE);
    /* SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); */
}

/*
 *---------------------------------------------------------------------------
 *
 * XWarpPointer --
 *
 *      If destWindow is None, moves the pointer by the offsets (destX,
 *      destY) relative to the current position of the pointer.
 *      If destWindow is a window, moves the pointer to the offsets
 *      (destX, destY) relative to the origin of destWindow.  However,
 *      if srcWindow is a window, the move only takes place if the window
 *      srcWindow contains the pointer and if the specified rectangle of
 *      srcWindow contains the pointer.
 *
 *      The srcX and srcY coordinates are relative to the origin of
 *      srcWindow.  If srcHeight is zero, it is replaced with the current
 *      height of srcWindow minus srcY.  If srcWidth is zero, it is
 *      replaced with the current width of srcWindow minus srcX.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXWarpPointer(
    Display *display,
    Window srcWindow,
    Window destWindow,
    int srcX,
    int srcY,
    unsigned int srcWidth,
    unsigned int srcHeight,
    int destX,
    int destY)
{
    HWND hWnd;
    POINT point;

    hWnd = Tk_GetHWND(destWindow);
    point.x = destX, point.y = destY;
    if (ClientToScreen(hWnd, &point)) {
        SetCursorPos(point.x, point.y);
    }
}

#ifdef notdef
static Blt_HashTable gcTable;
static int gcInitialized = FALSE;
#endif

typedef struct {
    HDC dc;
    int count;
    COLORREF color;
    int offset, numBits;
} DashInfo;

void
Blt_SetDashes(Display *display, GC gc, Blt_Dashes *dashesPtr)
{
    XGCValuesEx *gcPtr = (XGCValuesEx *)gc;
    int i;

    /* This must be used only with a privately created GC */
    assert(gcPtr->dashes == EXT_GC);
    for (i = 0; i < 12; i++) {
        if (dashesPtr->values[i] == 0) {
            break;                      /* End of dash list. */
        }
        gcPtr->dashValues[i] = dashesPtr->values[i];
    }
    gcPtr->numDashValues = i;
    gcPtr->dash_offset = dashesPtr->offset;
}

static int
GetDashInfo(HDC dc, GC gc, DashInfo *infoPtr)
{
    int dashOffset, dashValue;

    dashValue = 0;
    dashOffset = gc->dash_offset;
    if (gc->dashes == EXT_GC) {
        XGCValuesEx *gcPtr = (XGCValuesEx *)gc;

        /* This is an extended GC. */
        if (gcPtr->numDashValues == 1) {
            dashValue = gcPtr->dashValues[0];
        }
    } else if (gc->dashes > 0) {
        dashValue = (int)gc->dashes;
    }
    if (dashValue == 0) {
        return FALSE;
    }
    infoPtr->dc = dc;
    infoPtr->numBits = dashValue;
    infoPtr->offset = dashOffset;
    infoPtr->count = 0;
    infoPtr->color = gc->foreground;
    return TRUE;
}

#ifdef notdef
void
Blt_SetROP2(HDC dc, int function)
{
    SetROP2(dc, tkpWinRopModes[function]);
}
#endif

static XGCValuesEx *
CreateGC()
{
    XGCValuesEx *gcPtr;

    gcPtr = Blt_Malloc(sizeof(XGCValuesEx));
    if (gcPtr == NULL) {
        return NULL;
    }
    gcPtr->arc_mode = ArcPieSlice;
    gcPtr->background = 0xffffff;
    gcPtr->cap_style = CapNotLast;
    gcPtr->clip_mask = None;
    gcPtr->clip_x_origin = gcPtr->clip_y_origin = 0;
    gcPtr->dash_offset  = 0;
    gcPtr->fill_rule = WindingRule;
    gcPtr->fill_style = FillSolid;
    gcPtr->font = None;
    gcPtr->foreground = 0;
    gcPtr->function = GXcopy;
    gcPtr->graphics_exposures = True;
    gcPtr->join_style = JoinMiter;
    gcPtr->line_style = LineSolid;
    gcPtr->line_width = 0;
    gcPtr->plane_mask = ~0;
    gcPtr->stipple = None;
    gcPtr->subwindow_mode = ClipByChildren;
    gcPtr->tile = None;
    gcPtr->ts_x_origin = gcPtr->ts_y_origin = 0;

    gcPtr->dashes = EXT_GC;             /* Mark that this an extended GC */
    gcPtr->numDashValues = 0;
    memset(gcPtr->dashValues, 0, 12);
    return gcPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXCreateGC --
 *
 *      Allocate a new extended GC, and initialize the specified fields.
 *
 * Results:
 *      Returns a newly allocated GC.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
GC
Blt_EmulateXCreateGC(
    Display *display,
    Drawable drawable,
    unsigned long mask,
    XGCValues *srcPtr)
{
    XGCValuesEx *destPtr;

    destPtr = CreateGC();
    if (destPtr == NULL) {
        return None;
    }
    if (mask & GCFunction) {
        destPtr->function = srcPtr->function;
    }
    if (mask & GCPlaneMask) {
        destPtr->plane_mask = srcPtr->plane_mask;
    }
    if (mask & GCForeground) {
        destPtr->foreground = srcPtr->foreground;
    }
    if (mask & GCBackground) {
        destPtr->background = srcPtr->background;
    }
    if (mask & GCLineWidth) {
        destPtr->line_width = srcPtr->line_width;
    }
    if (mask & GCLineStyle) {
        destPtr->line_style = srcPtr->line_style;
    }
    if (mask & GCCapStyle) {
        destPtr->cap_style = srcPtr->cap_style;
    }
    if (mask & GCJoinStyle) {
        destPtr->join_style = srcPtr->join_style;
    }
    if (mask & GCFillStyle) {
        destPtr->fill_style = srcPtr->fill_style;
    }
    if (mask & GCFillRule) {
        destPtr->fill_rule = srcPtr->fill_rule;
    }
    if (mask & GCArcMode) {
        destPtr->arc_mode = srcPtr->arc_mode;
    }
    if (mask & GCTile) {
        destPtr->tile = srcPtr->tile;
    }
    if (mask & GCStipple) {
        destPtr->stipple = srcPtr->stipple;
    }
    if (mask & GCTileStipXOrigin) {
        destPtr->ts_x_origin = srcPtr->ts_x_origin;
    }
    if (mask & GCTileStipXOrigin) {
        destPtr->ts_y_origin = srcPtr->ts_y_origin;
    }
    if (mask & GCFont) {
        destPtr->font = srcPtr->font;
    }
    if (mask & GCSubwindowMode) {
        destPtr->subwindow_mode = srcPtr->subwindow_mode;
    }
    if (mask & GCGraphicsExposures) {
        destPtr->graphics_exposures = srcPtr->graphics_exposures;
    }
    if (mask & GCClipXOrigin) {
        destPtr->clip_x_origin = srcPtr->clip_x_origin;
    }
    if (mask & GCClipYOrigin) {
        destPtr->clip_y_origin = srcPtr->clip_y_origin;
    }
    if (mask & GCDashOffset) {
        destPtr->dash_offset = srcPtr->dash_offset;
    }
    if (mask & GCDashList) {
        destPtr->dashes = srcPtr->dashes;
    }
    if (mask & GCClipMask) {
        TkpClipMask *clipPtr;

        clipPtr = Blt_AssertCalloc(1, sizeof(TkpClipMask));
        clipPtr->type = TKP_CLIP_PIXMAP;
        clipPtr->value.pixmap = srcPtr->clip_mask;
        destPtr->clip_mask = (Pixmap) clipPtr;
    }
    return (GC)destPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GCToPen --
 *
 *      Set up the graphics port from the given GC.
 *
 *      Geometric and cosmetic pens available under both 95 and NT.  
 *      Geometric pens differ from cosmetic pens in that they can 
 *        1. Draw in world units (can have thick lines: line width > 1).
 *        2. Under NT, allow arbitrary line style.
 *        3. Can have caps and join (needed for thick lines).
 *        4. Draw very, very slowly.
 *
 *      Cosmetic pens are single line width only.
 *
 *                       95      98      NT
 *        PS_SOLID      c,g     c,g     c,g
 *        PS_DASH               c,g     c,g     c,g
 *        PS_DOT                  c       c     c,g
 *        PS_DASHDOT      c       -     c,g
 *        PS_DASHDOTDOT   c       -     c,g
 *        PS_USERSTYLE    -       -     c,g
 *        PS_ALTERNATE    -       -       c
 *
 *      Geometric only for 95/98
 *
 *        PS_ENDCAP_ROUND
 *        PS_ENDCAP_SQUARE
 *        PS_ENDCAP_FLAT
 *        PS_JOIN_BEVEL
 *        PS_JOIN_ROUND
 *        PS_JOIN_MITER
 * 
 * Results:
 *      None.
 *
 * Side effects:
 *      The current port is adjusted.
 *
 *---------------------------------------------------------------------------
 */
HPEN
Blt_GCToPen(HDC dc, GC gc)
{
    DWORD lineAttrs, lineStyle;
    DWORD dashArr[12];
    DWORD *dashPtr;
    int numDashValues, lineWidth;
    LOGBRUSH lBrush;
    HPEN pen;

    numDashValues = 0;
    lineWidth = (gc->line_width < 1) ? 1 : gc->line_width;
    if ((gc->line_style == LineOnOffDash) ||
        (gc->line_style == LineDoubleDash)) {
        if (gc->dashes == EXT_GC) {
            XGCValuesEx *gcPtr = (XGCValuesEx *)gc;
            int i;

            /* This is an extended GC. */
            for (i = 0; i < gcPtr->numDashValues; i++) {
                dashArr[i] = (DWORD)gcPtr->dashValues[i];
            }
            numDashValues = gcPtr->numDashValues;
            if (numDashValues == 1) {
                dashArr[1] = dashArr[0];
                numDashValues = 2;
            }
        } else {
            dashArr[1] = dashArr[0] = (DWORD)gc->dashes;
            numDashValues = 2;
        }
    }

    switch (numDashValues) {
    case 0:
        lineStyle = PS_SOLID;
        break;
    case 3:
        lineStyle = PS_DASHDOT;
        break;
    case 4:
        lineStyle = PS_DASHDOTDOT;
        break;
    case 2:
    default:
        /* PS_DASH style dash length is too long. */
        lineStyle = PS_DOT;
        break;
    }

    lBrush.lbStyle = BS_SOLID;
    lBrush.lbColor = gc->foreground;
    lBrush.lbHatch = 0;         /* Value is ignored when style is BS_SOLID. */

    lineAttrs = 0;
    switch (gc->cap_style) {
    case CapNotLast:
    case CapButt:
        lineAttrs |= PS_ENDCAP_FLAT;
        break;
    case CapRound:
        lineAttrs |= PS_ENDCAP_ROUND;
        break;
    default:
        lineAttrs |= PS_ENDCAP_SQUARE;
        break;
    }
    switch (gc->join_style) {
    case JoinMiter:
        lineAttrs |= PS_JOIN_MITER;
        break;
    case JoinBevel:
        lineAttrs |= PS_JOIN_BEVEL;
        break;
    case JoinRound:
    default:
        lineAttrs |= PS_JOIN_ROUND;
        break;
    }
    SetBkMode(dc, TRANSPARENT);

    if (Blt_GetPlatformId() == VER_PLATFORM_WIN32_NT) {
        /* Windows NT/2000/XP. */
        if (numDashValues > 0) {
            lineStyle = PS_USERSTYLE;
            dashPtr = dashArr;
        } else {
            dashPtr = NULL;
        }
        if (lineWidth > 1) {
            /* Limit the use of geometric pens to thick lines. */
            pen = ExtCreatePen(PS_GEOMETRIC | lineAttrs | lineStyle, lineWidth,
                       &lBrush, numDashValues, dashPtr);
        } else {
            /* Cosmetic pens are much faster. */
            pen = ExtCreatePen(PS_COSMETIC | lineAttrs | lineStyle, 1, &lBrush,
                       numDashValues, dashPtr);
        }           
    } else {
        /* Windows 95/98. */
        if ((lineStyle == PS_SOLID) && (lineWidth > 1)) {
            /* Use geometric pens with solid, thick lines only. */
            pen = ExtCreatePen(PS_GEOMETRIC | lineAttrs | lineStyle, lineWidth,
                       &lBrush, 0, NULL);
        } else {
            /* Otherwise sacrifice thick lines for dashes. */
            pen = ExtCreatePen(PS_COSMETIC | lineStyle, 1, &lBrush, 0, NULL);
        }
    } 
    assert(pen != NULL);
    return pen;
}

/*
 *---------------------------------------------------------------------------
 *
 * XDrawRectangles --
 *
 *       Draws the outlines of the specified rectangles as if a
 *       five-point PolyLine protocol request were specified for each
 *       rectangle:
 *
 *             [x,y] [x+width,y] [x+width,y+height] [x,y+height]
 *             [x,y]
 *
 *      For the specified rectangles, these functions do not draw a
 *      pixel more than once.  XDrawRectangles draws the rectangles in
 *      the order listed in the array.  If rectangles intersect, the
 *      intersecting pixels are drawn multiple times.  Draws a
 *      rectangle.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Draws rectangles on the specified drawable.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXDrawRectangles(Display *display, Drawable drawable, GC gc,
                           XRectangle *rectangles, int numRectangles)
{
    HBRUSH hBrush, oldBrush;
    HDC hDC;
    HPEN hPen, oldPen;
    DCState state;
    XRectangle *rp, *rend;

    if (drawable == None) {
        return;
    }
    hDC = GetDCState(display, drawable, gc, &state);

    hPen = Blt_GCToPen(hDC, gc);
    oldPen = SelectPen(hDC, hPen);

    hBrush = GetStockObject(NULL_BRUSH);
    oldBrush = SelectBrush(hDC, hBrush);

    SetROP2(hDC, tkpWinRopModes[gc->function]);
    for (rp = rectangles, rend = rp + numRectangles; rp < rend; rp++) {
        Rectangle(hDC, (int)rp->x, (int)rp->y, (int)(rp->x + rp->width + 1),
            (int)(rp->y + rp->height + 1));
    }
    (void)SelectPen(hDC, oldPen), DeletePen(hPen);
    (void)SelectBrush(hDC, oldBrush), DeleteBrush(hBrush);
    FreeDCState(&state);
}

#ifdef notdef
/*
 * Implements the "pixeling" of small arcs, because GDI-performance
 * for this is awful
 * was made especially for BLT, graph4 demo now runs 4x faster
 *
 */
/* O-outer , I-inner, B-both */
#define NEITHER_ 0
#define OUTLINE 1
#define FILL 2
#define BOTH (OUTLINE|FILL)
#define MINIARCS 5
static int arcus0[1] =
{
    BOTH
};
static int arcus1[4] =
{
    BOTH, BOTH,
    BOTH, BOTH
};

static int arcus2[9] =
{
    NEITHER, OUTLINE, NEITHER,
    OUTLINE, FILL, OUTLINE,
    NEITHER, OUTLINE, NEITHER
};

static int arcus3[16] =
{
    NEITHER, OUTLINE, OUTLINE, NEITHER,
    OUTLINE, FILL, FILL, OUTLINE,
    OUTLINE, FILL, FILL, OUTLINE,
    NEITHER, OUTLINE, OUTLINE, NEITHER
};

static int arcus4[25] =
{
    NEITHER, OUTLINE, OUTLINE, OUTLINE, NEITHER,
    OUTLINE, FILL, FILL, FILL, OUTLINE,
    OUTLINE, FILL, FILL, FILL, OUTLINE,
    OUTLINE, FILL, FILL, FILL, OUTLINE,
    NEITHER, OUTLINE, OUTLINE, OUTLINE, NEITHER
};

static int *arcis[MINIARCS] =
{
    arcus0, arcus1, arcus2, arcus3, arcus4
};

static void
DrawMiniArc(
    HDC dc,
    int width,
    int x,
    int y,
    int mask,
    COLORREF inner,
    COLORREF outer)
{
    int *arc;
    int i, j;

    if (width > MINIARCS) {
        return;
    }
    arc = arcis[width];
    for (i = 0; i <= width; i++) {
        for (j = 0; j <= width; j++) {
            bit = (mask & *arc);
            if (bit & OUTLINE) {
                SetPixelV(dc, x + i, y + j, outer);
            } else if (bit & FILL) {
                SetPixelV(dc, x + i, y + j, inner);
            }
            arc++;
        }
    }
}

#endif

/*
 *---------------------------------------------------------------------------
 *
 * DrawArc --
 *
 *      This procedure handles the rendering of drawn or filled
 *      arcs and chords.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Renders the requested arcs.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawArc(
    HDC dc,
    int arcMode,                /* Mode: either ArcChord or ArcPieSlice */
    XArc *arcPtr,
    HPEN pen,
    HBRUSH brush)
{
    int start, extent, clockwise;
    int xstart, ystart, xend, yend;
    double radian_start, radian_end, xr, yr;
    double dx, dy;

    if ((arcPtr->angle1 == 0) && (arcPtr->angle2 == 23040)) {
        /* Handle special case of circle or ellipse */
        Ellipse(dc, arcPtr->x, arcPtr->y, arcPtr->x + arcPtr->width + 1,
            arcPtr->y + arcPtr->height + 1);
        return;
    }
    start = arcPtr->angle1, extent = arcPtr->angle2;
    clockwise = (extent < 0);   /* Non-zero if clockwise */

    /*
     * Compute the absolute starting and ending angles in normalized radians.
     * Swap the start and end if drawing clockwise.
     */
    start = start % (64 * 360);
    if (start < 0) {
        start += (64 * 360);
    }
    extent = (start + extent) % (64 * 360);
    if (extent < 0) {
        extent += (64 * 360);
    }
    if (clockwise) {
        int tmp = start;
        start = extent;
        extent = tmp;
    }
#define XAngleToRadians(a) ((double)(a) / 64 * M_PI / 180);
    radian_start = XAngleToRadians(start);
    radian_end = XAngleToRadians(extent);

    /*
     * Now compute points on the radial lines that define the starting and
     * ending angles.  Be sure to take into account that the y-coordinate
     * system is inverted.
     */
    dx = arcPtr->width * 0.5;
    dy = arcPtr->height * 0.5;

    xr = arcPtr->x + dx;
    yr = arcPtr->y + dy;
    xstart = (int)((xr + cos(radian_start) * dx) + 0.5);
    ystart = (int)((yr + sin(-radian_start) * dy) + 0.5);
    xend = (int)((xr + cos(radian_end) * dx) + 0.5);
    yend = (int)((yr + sin(-radian_end) * dy) + 0.5);

    /*
     * Now draw a filled or open figure.  Note that we have to
     * increase the size of the bounding box by one to account for the
     * difference in pixel definitions between X and Windows.
     */

    if (brush == 0) {
        /*
         * Note that this call will leave a gap of one pixel at the
         * end of the arc for thin arcs.  We can't use ArcTo because
         * it's only supported under Windows NT.
         */
        Arc(dc, arcPtr->x, arcPtr->y, arcPtr->x + arcPtr->width + 1,
            arcPtr->y + arcPtr->height + 1, xstart, ystart, xend, yend);
        /* FIXME: */
    } else {
        if (arcMode == ArcChord) {
            Chord(dc, arcPtr->x, arcPtr->y, arcPtr->x + arcPtr->width + 1,
                arcPtr->y + arcPtr->height + 1, xstart, ystart, xend, yend);
        } else if (arcMode == ArcPieSlice) {
            Pie(dc, arcPtr->x, arcPtr->y, arcPtr->x + arcPtr->width + 1,
                arcPtr->y + arcPtr->height + 1, xstart, ystart, xend, yend);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * XDrawArcs --
 *
 *      Draws multiple circular or elliptical arcs.  Each arc is specified by
 *      a rectangle and two angles.  The center of the circle or ellipse is
 *      the center of the rect- angle, and the major and minor axes are
 *      specified by the width and height.  Positive angles indicate
 *      counterclock- wise motion, and negative angles indicate clockwise
 *      motion.  If the magnitude of angle2 is greater than 360 degrees,
 *      XDrawArcs truncates it to 360 degrees.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Draws an arc for each array element on the specified drawable.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXDrawArcs(
    Display *display,
    Drawable drawable,
    GC gc,
    XArc *arcArr,
    int numArcs)
{
    DCState state;
    HBRUSH brush, oldBrush;
    HDC dc;
    HPEN pen, oldPen;
    XArc *ap, *aend;
    
    display->request++;
    if (drawable == None) {
        return;
    }
    dc = GetDCState(display, drawable, gc, &state);
    pen = Blt_GCToPen(dc, gc);
    oldPen = SelectPen(dc, pen);
    brush = GetStockBrush(NULL_BRUSH);
    oldBrush = SelectBrush(dc, brush);
    for (ap = arcArr, aend = ap + numArcs; ap < aend; ap++) {
        DrawArc(dc, gc->arc_mode, ap, pen, 0);
    }
    DeleteBrush(SelectBrush(dc, oldBrush));
    DeletePen(SelectPen(dc, oldPen));
    FreeDCState(&state);
}

/*
 *---------------------------------------------------------------------------
 *
 * XFillArcs --
 *
 *      Draw a filled arc.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Draws a filled arc for each array element on the specified drawable.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXFillArcs(
    Display *display,
    Drawable drawable,
    GC gc,
    XArc *arcArr,
    int numArcs)
{
    HBRUSH brush, oldBrush;
    HPEN pen, oldPen;
    HDC dc;
    DCState state;

    display->request++;
    if (drawable == None) {
        return;
    }
    dc = GetDCState(display, drawable, gc, &state);
    pen = Blt_GCToPen(dc, gc);
    oldPen = SelectPen(dc, pen);
    brush = CreateSolidBrush(gc->foreground);
    oldBrush = SelectBrush(dc, brush);
    {
        XArc *ap, *aend;

        for (ap = arcArr, aend = ap + numArcs; ap < aend; ap++) {
            DrawArc(dc, gc->arc_mode, ap, pen, brush);
        }
    }
    DeleteBrush(SelectBrush(dc, oldBrush));
    DeletePen(SelectPen(dc, oldPen));
    FreeDCState(&state);
}

/*
 *---------------------------------------------------------------------------
 *
 * XDrawLines --
 *
 *      Draw connected lines.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Renders a series of connected lines.
 *
 *---------------------------------------------------------------------------
 */

static void CALLBACK
DrawDot(
    int x, int y,               /* Coordinates of point */
    LPARAM clientData)
{                               /* Line information */
    DashInfo *infoPtr = (DashInfo *) clientData;
    int count;

    infoPtr->count++;
    count = (infoPtr->count + infoPtr->offset) / infoPtr->numBits;
    if (count & 0x1) {
        SetPixelV(infoPtr->dc, x, y, infoPtr->color);
    }
}


void
Blt_EmulateXDrawLine(Display *display, Drawable drawable, GC gc, 
                     int x1, int y1, int x2, int y2)
{
    DCState state;
    HDC dc;

    if (drawable == None) {
        return;
    }
    dc = GetDCState(display, drawable, gc, &state);
    if (gc->line_style != LineSolid) {
        /* Handle dotted lines specially */
        DashInfo info;
        int count;

        if (!GetDashInfo(dc, gc, &info)) {
            goto solidLine;
        }
        count = info.offset / info.numBits;
        if (x1 == x2) {         /* Vertical line */
            int y;

            for (y = y1; y <= y2; y += 2) {
                SetPixelV(dc, x1, y + count, info.color);
            }
        } else if (y1 == y2) {  /* Horizontal line */
            int x;

            for (x = x1; x <= x2; x += 2) {
                SetPixelV(dc, x + count, y1, info.color);
            }
        } else {
            LineDDA(x1, y1, x2, y2, DrawDot, (LPARAM)&info);
        }
    } else {
        HPEN pen, oldPen;
        HBRUSH brush, oldBrush;

      solidLine:
        pen = Blt_GCToPen(dc, gc);
        oldPen = SelectPen(dc, pen);
        brush = CreateSolidBrush(gc->foreground);
        oldBrush = SelectBrush(dc, brush);
        MoveToEx(dc, x1, y1, (LPPOINT)NULL);
        LineTo(dc, x2, y2);
        DeletePen(SelectPen(dc, oldPen));
        DeleteBrush(SelectBrush(dc, oldBrush));
    }
    FreeDCState(&state);
}

static void
DrawLine(Display *display, Drawable drawable, GC gc, POINT *points, int numPoints)
{
    DCState state;
    HDC dc;
    int i, n;
    int start, extra, size;
    HPEN pen, oldPen;
    HBRUSH brush, oldBrush;

    if (drawable == None) {
        return;
    }
    dc = GetDCState(display, drawable, gc, &state);
    pen = Blt_GCToPen(dc, gc);
    oldPen = SelectPen(dc, pen);
    brush = CreateSolidBrush(gc->foreground);
    oldBrush = SelectBrush(dc, brush);
    
    start = extra = 0;
    /*  
     * Depending if the line is wide (> 1 pixel), arbitrarily break the line
     * in sections of 100 points.  This bit of weirdness has to do with wide
     * geometric pens.  The longer the polyline, the slower it draws.  The
     * trade off is that we lose dash and cap uniformity for unbearably slow
     * polyline draws.
     */
    if (gc->line_width > 1) {
        size = 100;
    } else {
        size = numPoints;
    }
    for (i = numPoints; i > 0; i -= size) {
        n = MIN(i, size);
        Polyline(dc, points + start, n + extra);
        start += (n - 1);
        extra = 1;
    }
    DeletePen(SelectPen(dc, oldPen));
    DeleteBrush(SelectBrush(dc, oldBrush));
    FreeDCState(&state);
}

void
Blt_EmulateXDrawLines(Display *display, Drawable drawable, GC gc, 
                      XPoint *pointArr, int numPoints, int mode)
{
    if (drawable == None) {
        return;
    }
    if (gc->line_style != LineSolid) {  /* Handle dotted lines specially */
        DashInfo info;
        DCState state;
        HDC dc;
        int result;

        dc = GetDCState(display, drawable, gc, &state);
        result = GetDashInfo(dc, gc, &info);
        if (result) {
            XPoint *p1, *p2;
            int i;

            p1 = pointArr;
            p2 = p1 + 1;
            for (i = 1; i < numPoints; i++, p1++, p2++) {
                LineDDA(p1->x, p1->y, p2->x, p2->y, DrawDot, (LPARAM)&info);
            }
            result = TCL_OK;
        }
        FreeDCState(&state);
        if (result) {
            return;
        }
    } else {
        POINT *points;

        points = Blt_Malloc(sizeof(POINT) * numPoints);
        if (points == NULL) {
            return;
        }
        if (mode == CoordModeOrigin) {
            POINT *destPtr;
            XPoint *sp, *send;

            destPtr = points;
            for (sp = pointArr, send = sp + numPoints; sp < send; sp++) {
                destPtr->x = (int)sp->x;
                destPtr->y = (int)sp->y;
                destPtr++;
            }
        } else {
            XPoint *sp, *send;
            POINT *destPtr, *lastPtr;
            
            sp = pointArr;
            destPtr = points;
            destPtr->x = (int)sp->x;
            destPtr->y = (int)sp->y;
            lastPtr = destPtr;
            sp++, destPtr++;
            for (send = pointArr + numPoints; sp < send; sp++) {
                destPtr->x = lastPtr->x + (int)sp->x;
                destPtr->y = lastPtr->y + (int)sp->y;
                lastPtr = destPtr;
                destPtr++;
            }
        }
        DrawLine(display, drawable, gc, points, numPoints);
        Blt_Free(points);
    }
}



/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmultateXDrawSegments --
 *
 *      Draws multiple, unconnected lines. For each segment, draws a line
 *      between (x1, y1) and (x2, y2).  It draws the lines in the order listed
 *      in the array of XSegment structures and does not perform joining at
 *      coincident endpoints.  For any given line, does not draw a pixel more
 *      than once. If lines intersect, the intersecting pixels are drawn
 *      multiple times.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Draws unconnected line segments on the specified drawable.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXDrawSegments(Display *display, Drawable drawable, GC gc,
                         XSegment *segArr, int numSegments)
{
    HDC dc;
    DCState state;

    display->request++;
    if (drawable == None) {
        return;
    }
    dc = GetDCState(display, drawable, gc, &state);
    if (gc->line_style != LineSolid) {
        XSegment *sp, *send;

        /* Handle dotted lines specially */
        DashInfo info;

        if (!GetDashInfo(dc, gc, &info)) {
            goto solidLine;
        }
        for (sp = segArr, send = sp + numSegments; sp < send; sp++) {
            info.count = 0; /* Reset dash counter after every segment. */
            LineDDA(sp->x1, sp->y1, sp->x2, sp->y2, DrawDot, (LPARAM)&info);
        }
    } else {
        XSegment *sp, *send;
        HPEN pen, oldPen;

      solidLine:
        pen = Blt_GCToPen(dc, gc);
        oldPen = SelectPen(dc, pen);
        for (sp = segArr, send = sp + numSegments; sp < send; sp++) {
            MoveToEx(dc, sp->x1, sp->y1, (LPPOINT)NULL);
            LineTo(dc, sp->x2, sp->y2);
        }
        DeletePen(SelectPen(dc, oldPen));
    }
    FreeDCState(&state);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmultateXDrawRectangle --
 *
 *       Draws the outlines of the specified rectangle as if a five-point
 *       PolyLine protocol request were specified for each rectangle:
 *
 *             [x,y] [x+width,y] [x+width,y+height] [x,y+height]
 *             [x,y]
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Draws a rectangle on the specified drawable.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXDrawRectangle(Display *display, Drawable drawable, GC gc, int x, 
                          int y, unsigned int w, unsigned int h)
{
    DCState state;
    HPEN pen, oldPen;
    HBRUSH brush, oldBrush;
    HDC dc;

    if (drawable == None) {
        return;
    }
    
    dc = GetDCState(display, drawable, gc, &state);
    pen = Blt_GCToPen(dc, gc);
    brush = GetStockObject(NULL_BRUSH);
    oldPen = SelectPen(dc, pen);
    oldBrush = SelectBrush(dc, brush);
    if (gc->line_style != LineSolid) {
        /* Handle dotted lines specially */
        int x2, y2;
        DashInfo info;

        if (!GetDashInfo(dc, gc, &info)) {
            goto solidLine;
        }
        x2 = x + w;
        y2 = y + h;
        LineDDA(x, y, x2, y, DrawDot, (LPARAM)&info);
        LineDDA(x2, y, x2, y2, DrawDot, (LPARAM)&info);
        LineDDA(x2, y2, x, y2, DrawDot, (LPARAM)&info);
        LineDDA(x, y2, x, y, DrawDot, (LPARAM)&info);
    } else {
      solidLine:
        Rectangle(dc, x, y, x + w + 1, y + h + 1);
    }
    DeletePen(SelectPen(dc, oldPen));
    DeleteBrush(SelectBrush(dc, oldBrush));
    FreeDCState(&state);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXDrawPoints --
 *
 *      Uses the foreground pixel and function components of the GC to
 *      draw a multiple points into the specified drawable.
 *      CoordModeOrigin treats all coordinates as relative to the
 *      origin, and CoordModePrevious treats all coordinates after
 *      the first as relative to the previous point.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Draws points on the specified drawable.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXDrawPoints(Display *display, Drawable drawable, GC gc, 
                       XPoint *points, int numPoints, int mode)
/* Ignored. CoordModeOrigin is assumed. */
{                                   
    HDC dc;
    XPoint *pp, *pend;
    DCState state;

    display->request++;
    if (drawable == None) {
        return;
    }
    dc = GetDCState(display, drawable, gc, &state);
    for (pp = points, pend = pp + numPoints; pp < pend; pp++) {
        SetPixelV(dc, pp->x, pp->y, gc->foreground);
    }
    FreeDCState(&state);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmultateXReparentWindow --
 *
 *      If the specified window is mapped, automatically performs an
 *      UnmapWindow request on it, removes it from its current
 *      position in the hierarchy, and inserts it as the child of the
 *      specified parent.  The window is placed in the stacking order
 *      on top with respect to sibling windows.
 *
 *      Note: In WIN32 you can't reparent to/from another application.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Reparents the specified window.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXReparentWindow(Display *display, Window window, Window parent,
                           int x, int y)
{
    HWND child, newParent;

    child = Tk_GetHWND(window);
    newParent = Tk_GetHWND(parent);
    SetParent(child, newParent);
    SetWindowLong(child, GWL_STYLE, WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS);
    XMoveWindow(display, window, x, y);
    XRaiseWindow(display, window);
    XMapWindow(display, window);
}

void
Blt_EmulateXSetDashes(Display *display, GC gc, int dashOffset, 
                      _Xconst char *dashList, int n)
{
    assert(n >= 0);
    gc->dashes = n;
    gc->dash_offset = (int)dashList;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmultateXDrawString --
 *
 *      Draw a single string in the current font.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Renders the specified string in the drawable.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXDrawString(Display *display, Drawable drawable, GC gc,
                       int x, int y, _Xconst char *string, int length)
{
    if (drawable == None) {
        return;
    }
    Tk_DrawChars(display, drawable, gc, (Tk_Font)gc->font, string, length, 
        x, y);
}

static void
TileArea(HDC destDC, HDC srcDC, int tileOriginX, int tileOriginY, 
        int tileWidth,  int tileHeight, int x, int y, int width, int height)
{
    int destX, destY;
    int destWidth, destHeight;
    int srcX, srcY;
    int xOrigin, yOrigin;
    int delta;
    int left, top, right, bottom;

    xOrigin = x, yOrigin = y;
    if (x < tileOriginX) {
        delta = (tileOriginX - x) % tileWidth;
        if (delta > 0) {
            xOrigin -= (tileWidth - delta);
        }
    } else if (x > tileOriginX) {
        delta = (x - tileOriginX) % tileWidth;
        if (delta > 0) {
            xOrigin -= delta;
        }
    }
    if (y < tileOriginY) {
        delta = (tileOriginY - y) % tileHeight;
        if (delta > 0) {
            yOrigin -= (tileHeight - delta);
        }
    } else if (y >= tileOriginY) {
        delta = (y - tileOriginY) % tileHeight;
        if (delta > 0) {
            yOrigin -= delta;
        }
    }
    left = x;
    right = x + width;
    top = y;
    bottom = y + height;
    for (y = yOrigin; y < bottom; y += tileHeight) {
        srcY = 0;
        destY = y;
        destHeight = tileHeight;
        if (y < top) {
            srcY = (top - y);
            destHeight = tileHeight - srcY;
            destY = top;
        } 
        if ((destY + destHeight) > bottom) {
            destHeight = (bottom - destY);
        }
        for (x = xOrigin; x < right; x += tileWidth) {
            srcX = 0;
            destX = x;
            destWidth = tileWidth;
            if (x < left) {
                srcX = (left - x);
                destWidth = tileWidth - srcX;
                destX = left;
            } 
            if ((destX + destWidth) > right) {
                destWidth = (right - destX);
            }
            BitBlt(destDC, destX, destY, destWidth, destHeight, 
                srcDC, srcX, srcY, SRCCOPY);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmultateXFillRectangles --
 *
 *      Fill multiple rectangular areas in the given drawable.
 *      Handles tiling.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Draws onto the specified drawable.
 *
 *---------------------------------------------------------------------------
 */

void
Blt_EmulateXFillRectangles(Display *display, Drawable drawable, GC gc,
                           XRectangle *rectangles, int numRectangles)
{
    HBRUSH oldBrush, hFgBrush, hBgBrush, hBrush;
    HDC hDC;
    HDC memDC;
    DCState state;
    TkWinDrawable *twdPtr;
    XRectangle *rp, *rend;
    
    if (drawable == None) {
        return;
    }
    hDC = GetDCState(display, drawable, gc, &state);
    switch(gc->fill_style) {
    case FillTiled:
        if (gc->tile == None) {
            goto fillSolid;
        }
#ifdef notdef
        if ((GetDeviceCaps(hDC, RASTERCAPS) & RC_BITBLT) == 0) {
            goto fillSolid;
        }
#endif
        twdPtr = (TkWinDrawable *)gc->tile;
        {
            HBITMAP oldBitmap;
            BITMAP bm;

            GetObject(twdPtr->bitmap.handle, sizeof(BITMAP), &bm);
            memDC = CreateCompatibleDC(hDC);
            oldBitmap = SelectBitmap(memDC, twdPtr->bitmap.handle);
            for (rp = rectangles, rend = rp + numRectangles; rp < rend; rp++) {
                TileArea(hDC, memDC, gc->ts_x_origin, gc->ts_y_origin, 
                         bm.bmWidth, bm.bmHeight, 
                         (int)rp->x, (int)rp->y, (int)rp->width, 
                         (int)rp->height);
            }
            (void)SelectBitmap(memDC, oldBitmap);
            DeleteDC(memDC);
        }
        break; 

    case FillOpaqueStippled:
    case FillStippled:
        if (gc->stipple == None) {
            goto fillSolid;
        }
        twdPtr = (TkWinDrawable *)gc->stipple;
        if (twdPtr->type != TWD_BITMAP) {
            panic("unexpected drawable type in stipple");
        }
        hBrush = CreatePatternBrush(twdPtr->bitmap.handle);
        SetBrushOrgEx(hDC, gc->ts_x_origin, gc->ts_y_origin, NULL);
        oldBrush = SelectBrush(hDC, hBrush);
        memDC = CreateCompatibleDC(hDC);

        /*
         * For each rectangle, create a drawing surface which is the size of
         * the rectangle and fill it with the background color.  Then merge the
         * result with the stipple pattern.
         */
        hFgBrush = CreateSolidBrush(gc->foreground);
        hBgBrush = CreateSolidBrush(gc->background);
        for (rp = rectangles, rend = rp + numRectangles; rp < rend; rp++) {
            RECT rect;
            HBITMAP oldBitmap, hBitmap;

            hBitmap = CreateCompatibleBitmap(hDC, rp->width, rp->height);
            oldBitmap = SelectObject(memDC, hBitmap);
            rect.left = rect.top = 0;
            rect.right = rp->width;
            rect.bottom = rp->height;
            FillRect(memDC, &rect, hFgBrush);
            BitBlt(hDC, rp->x, rp->y, rp->width, rp->height, memDC, 0, 0, 
                   COPYBG);
            if (gc->fill_style == FillOpaqueStippled) {
                FillRect(memDC, &rect, hBgBrush);
                BitBlt(hDC, rp->x, rp->y, rp->width, rp->height, memDC, 0, 0, 
                       COPYFG);
            }
            SelectObject(memDC, oldBitmap);
            DeleteObject(hBitmap);
        }
        DeleteBrush(hFgBrush);
        DeleteBrush(hBgBrush);
        DeleteDC(memDC);
        (void)SelectBrush(hDC, oldBrush);
        DeleteBrush(hBrush);
        break;

    case FillSolid:
        fillSolid:
#ifdef notdef
        memDC = CreateCompatibleDC(hDC);
#endif
        hFgBrush = CreateSolidBrush(gc->foreground);
        for (rp = rectangles, rend = rp + numRectangles; rp < rend; rp++) {
            RECT rect;
#ifdef notdef
            HBITMAP oldBitmap, hBitmap;

            hBitmap = CreateCompatibleBitmap(hDC, rp->width, rp->height);
            oldBitmap = SelectObject(memDC, hBitmap);
#endif
            /* Note that width and height is already assumed to be
             * subtracted by one. */
            rect.left = rp->x, rect.top = rp->y;
            rect.right = rp->x + rp->width;
            rect.bottom = rp->y + rp->height;
            FillRect(hDC, &rect, hFgBrush);
#ifdef notdef
            BitBlt(hDC, rp->x, rp->y, rp->width, rp->height, memDC, 0, 0, 
                SRCCOPY);
            SelectObject(memDC, oldBitmap);
            DeleteObject(hBitmap);
#endif
        }
        DeleteBrush(hFgBrush);
#ifdef notdef
        DeleteDC(memDC);
#endif
        break;
    }
    FreeDCState(&state);
}

void
Blt_EmulateXFillRectangle(Display *display, Drawable drawable, GC gc,
                          int x, int y, unsigned int w, unsigned int h)
{
    HDC hDC;
    RECT rect;
    DCState state;

    if (drawable == None) {
        return;
    }
    hDC = GetDCState(display, drawable, gc, &state);
    rect.left = rect.top = 0;
    rect.right = w, rect.bottom = h;

    switch(gc->fill_style) {
    case FillTiled:
        {
            TkWinDrawable *twdPtr;
            HBITMAP oldBitmap;
            HDC memDC;
            BITMAP bm;

            if (gc->tile == None) { 
                goto fillSolid;
            }
#ifdef notdef
            if ((GetDeviceCaps(hDC, RASTERCAPS) & RC_BITBLT) == 0) {
                goto fillSolid;
            }
#endif
            twdPtr = (TkWinDrawable *)gc->tile;
            /* The tiling routine needs to know the size of the bitmap */
            GetObject(twdPtr->bitmap.handle, sizeof(BITMAP), &bm);

            memDC = CreateCompatibleDC(hDC);
            oldBitmap = SelectBitmap(memDC, twdPtr->bitmap.handle);
            TileArea(hDC, memDC, gc->ts_x_origin, gc->ts_y_origin, bm.bmWidth, 
                     bm.bmHeight, x, y, w, h);
            (void)SelectBitmap(memDC, oldBitmap);
            DeleteDC(memDC);
        }
        break; 
            
    case FillOpaqueStippled:
    case FillStippled:
        {
            TkWinDrawable *twdPtr;
            HBRUSH oldBrush, hBrush;
            HBRUSH hBrushFg, hBrushBg;
            HBITMAP oldBitmap, hBitmap;
            HDC memDC;

            if (gc->stipple == None) {
                goto fillSolid;
            }
            twdPtr = (TkWinDrawable *)gc->stipple;
            if (twdPtr->type != TWD_BITMAP) {
                panic("unexpected drawable type in stipple");
            }
            hBrush = CreatePatternBrush(twdPtr->bitmap.handle);
            SetBrushOrgEx(hDC, gc->ts_x_origin, gc->ts_y_origin, NULL);
            oldBrush = SelectBrush(hDC, hBrush);
            memDC = CreateCompatibleDC(hDC);
            
            hBrushFg = CreateSolidBrush(gc->foreground);
            hBrushBg = CreateSolidBrush(gc->background);
            hBitmap = CreateCompatibleBitmap(hDC, w, h);
            oldBitmap = SelectObject(memDC, hBitmap);
            FillRect(memDC, &rect, hBrushFg);
            SetBkMode(hDC, TRANSPARENT);
            BitBlt(hDC, x, y, w, h, memDC, 0, 0, COPYFG);
            if (gc->fill_style == FillOpaqueStippled) {
                FillRect(memDC, &rect, hBrushBg);
                BitBlt(hDC, x, y, w, h, memDC, 0, 0, COPYBG);
            }
            (void)SelectBrush(hDC, oldBrush);
            (void)SelectBitmap(memDC, oldBitmap);
            DeleteBrush(hBrushFg);
            DeleteBrush(hBrushBg);
            DeleteBrush(hBrush);
            DeleteBitmap(hBitmap);
            DeleteDC(memDC);
        }
        break;

    case FillSolid:
        {
            HBRUSH hBrush;
            HBITMAP oldBitmap, hBitmap;
            HDC memDC;

        fillSolid:
            /* TkWinFillRect(hDC, x, y, w, h, gc->foreground);  */
            memDC = CreateCompatibleDC(hDC);
            hBrush = CreateSolidBrush(gc->foreground);
            hBitmap = CreateCompatibleBitmap(hDC, w, h);
            oldBitmap = SelectBitmap(memDC, hBitmap);
            rect.left = rect.top = 0;
            rect.right = w, rect.bottom = h;
            FillRect(memDC, &rect, hBrush);
            BitBlt(hDC, x, y, w, h, memDC, 0, 0, SRCCOPY);
            SelectObject(memDC, oldBitmap);
            DeleteBitmap(hBitmap);
            DeleteBrush(hBrush);
            DeleteDC(memDC);
        }
        break;
    }
    FreeDCState(&state);
}

#ifdef notdef
#if (_TCL_VERSION >= _VERSION(8,1,0)) 
static BOOL
DrawChars(HDC dc, int x, int y, char *string, int length)
{
    BOOL result;

    if (systemEncoding == NULL) {
        result = TextOutA(dc, x, y, string, length);
    } else {
        Tcl_DString ds;
        const unsigned short *ws;
        int wlength;

        Tcl_DStringInit(&ds);
        Tcl_UtfToExternalDString(systemEncoding, string, length, &ds);
        wlength = Tcl_DStringLength(&ds) >> 1;
        ws = (const unsigned short *)Tcl_DStringValue(&ds);
        result = TextOutW(dc, x, y, ws, wlength);
        Tcl_DStringFree(&ds);
    }
    return result;
}
#else
static BOOL
DrawChars(HDC dc, int x, int y, char *string, int length)
{
    return TextOutA(dc, x, y, string, length);
}
#endif /* _TCL_VERSION >= _VERSION(8,1,0) */
#endif

#ifdef notef
int
Blt_DrawTextWithRotatedFont(
    Tk_Window tkwin,
    Drawable drawable,
    int depth,
    float angle,
    TextStyle *tsPtr,
    TextLayout *textPtr,
    int x, int y,
    int xMax)
{
    Display *display;
    HFONT hFont, oldFont;
    TkWinDCState state;
    HDC hDC;
    int isActive;
    int bbWidth, bbHeight;
    double rotWidth, rotHeight;
    double sinTheta, cosTheta;
    double radians;
    Point2d p, q, center;
    TextFragment *fp, *fend;

#if (_TCL_VERSION >=  _VERSION(8,1,0)) 
    static int initialized = 0;

    if (!initialized) {
        if (Blt_GetPlatformId() == VER_PLATFORM_WIN32_NT) {
            /*
             * If running NT, then we will be calling some Unicode
             * functions explictly.  So, even if the TCL system
             * encoding isn't Unicode, make sure we convert to/from
             * the Unicode char set.
             */
            systemEncoding = Tcl_GetEncoding(NULL, "unicode");
        } 
        initialized = 1;
    }
#endif
    display = Tk_Display(tkwin);
    hFont = CreateRotatedFont(tsPtr->gc->font, angle);
    if (hFont == NULL) {
        return FALSE;
    }
    Blt_RotateStartingTextPositions(layoutPtr, angle);
    Blt_GetBoundingBox((double)layoutPtr->width, (double)layoutPtr->height, 
                angle, &rotWidth, &rotHeight, (Point2d *)NULL);
    Blt_TranslateAnchor(x, y, ROUND(rotWidth), ROUND(rotHeight), tsPtr->anchor, 
                &x, &y);

    isActive = (tsPtr->state & STATE_ACTIVE);
    hDC = TkWinGetDrawableDC(display, drawable, &state);
    SetROP2(hDC, tsPtr->gc->function);
    oldFont = SelectFont(hDC, hFont);

    SetBkMode(hDC, TRANSPARENT);
    SetTextAlign(hDC, TA_LEFT | TA_BASELINE);

    if (tsPtr->state & (STATE_DISABLED | STATE_EMPHASIS)) {
        TkBorder *borderPtr = (TkBorder *) tsPtr->border;
        XColor *color1, *color2;
        
        color1 = borderPtr->lightColor, color2 = borderPtr->darkColor;
        if (tsPtr->state & STATE_EMPHASIS) {
            XColor *hold;
            
            hold = color1, color1 = color2, color2 = hold;
        }
        if (color1 != NULL) {
            SetTextColor(hDC, color1->pixel);
            for (fp = textPtr->fragments, fend = fp + textPtr->nFrags; 
                 fp < fend; fp++) {
                DrawChars(hDC, fp->sx, fp->sy, fp->text, fp->count); 
            }
        }
        if (color2 != NULL) {
            SetTextColor(hDC, color2->pixel);
            for (fp = textPtr->fragments, fend = fp + textPtr->nFrags; 
                 fp < fend; fp++) {
                DrawChars(hDC, fp->sx + 1, fp->sy + 1, fp->text, fp->count);
            }
        }
        goto done;              /* Done */
    }
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, tsPtr->color->pixel);
    for (fp = textPtr->fragments, fend = fp + textPtr->nFrags; fp < fend; 
         fp++) {
        DrawChars(hDC, fp->sx, fp->sy, fp->text, fp->count);
    }
 done:
    SelectFont(hDC, oldFont);
    DeleteFont(hFont);
    TkWinReleaseDrawableDC(drawable, hDC, &state);
    return TRUE;
}
#endif

#ifdef notdef
static void
DrawPixel(HDC hDC, int x, int y, COLORREF color)
{
    HDC memDC;
    HBRUSH hBrushFg;
    HBITMAP oldBitmap, hBitmap;
    RECT rect;
    int size;
 
    size = 1;
    memDC = CreateCompatibleDC(hDC);
    hBrushFg = CreateSolidBrush(color);
    hBitmap = CreateCompatibleBitmap(hDC, size, size);
    oldBitmap = SelectObject(memDC, hBitmap);
    rect.left = rect.top = 0;
    rect.right = rect.bottom = size;
    FillRect(memDC, &rect, hBrushFg);
    BitBlt(hDC, x, y, size, size, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteBrush(hBrushFg);
    DeleteDC(memDC);
}

/*
 *---------------------------------------------------------------------------
 *
 * PixelateBitmap --
 *
 *      Draws a masked bitmap in given device (should be printer)
 *      context.  Bit operations on print devices usually fail because
 *      there's no way to read back from the device surface to get the
 *      previous pixel values, rendering BitBlt useless. The bandaid
 *      solution here is to draw 1x1 pixel rectangles at each
 *      coordinate as directed by the the mask and source bitmaps.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Draws onto the specified drawable.
 *
 *---------------------------------------------------------------------------
 */
static void
PixelateBitmap(
    Display *display,
    Drawable drawable,
    Pixmap srcBitmap,
    Pixmap maskBitmap,
    int width,
    int height,
    GC gc,
    int destX,
    int destY)
{
    int x, y;
    int dx, dy;
    int pixel;
    unsigned char *srcBits;
    unsigned char *srcPtr;
    int bitPos, bytesPerRow;
    HDC hDC;
    TkWinDCState state;

    srcBits = Blt_GetBitmapData(display, srcBitmap, width, height, 
        &bytesPerRow);
    if (srcBits == NULL) {
        return;
    }
    hDC = TkWinGetDrawableDC(display, drawable, &state);
    if (maskBitmap != None) {
        unsigned char *maskPtr;
        unsigned char *maskBits;
        maskBits = Blt_GetBitmapData(display, maskBitmap, width, height,
            &bytesPerRow);
        bytesPerRow = ((width + 31) & ~31) / 8;
        for (dy = destY, y = height - 1; y >= 0; y--, dy++) {
            maskPtr = maskBits + (bytesPerRow * y);
            srcPtr = srcBits + (bytesPerRow * y);
            for (dx = destX, x = 0; x < width; x++, dx++) {
                bitPos = x % 8;
                pixel = (*maskPtr & (0x80 >> bitPos));
                if (pixel) {
                    pixel = (*srcPtr & (0x80 >> bitPos));
                    DrawPixel(hDC, dx, dy, 
                        (pixel) ? gc->foreground : gc->background);
                }
                if (bitPos == 7) {
                    srcPtr++, maskPtr++;
                }
            }                   /* x */
        }
        Blt_Free(maskBits);
    } else {
        bytesPerRow = ((width + 31) & ~31) / 8;
        for (dy = destY, y = height - 1; y >= 0; y--, dy++) {
            srcPtr = srcBits + (bytesPerRow * y);
            for (dx = destX, x = 0; x < width; x++, dx++) {
                bitPos = x % 8;
                pixel = (*srcPtr & (0x80 >> bitPos));
                DrawPixel(hDC, dx, dy, 
                        (pixel) ? gc->foreground : gc->background);
                if (bitPos == 7) {
                    srcPtr++;
                }
            }
        }
    }
    TkWinReleaseDrawableDC(drawable, hDC, &state);
    Blt_Free(srcBits);
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXCopyPlane --
 *
 *      Simplified version of XCopyPlane.  Right now it ignores
 *              function, 
 *              clip_x_origin, 
 *              clip_y_origin
 *
 *      The plane argument must always be 1.
 *
 *      This routine differs from the Tk version in how it handles 
 *      transparency.  It uses a different method of drawing transparent
 *      bitmaps that doesn't copy the background or use brushes.  The
 *      second change is to call a special routine when the destDC is
 *      a printer.   Stippling is done by a very slow brute-force
 *      method of drawing 1x1 rectangles for each pixel (bleech).  
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Changes the destination drawable.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXCopyPlane(
    Display *display,
    Drawable src, Drawable dest,
    GC gc,
    int srcX, int srcY,
    unsigned int width, unsigned int height,
    int destX, int destY,
    unsigned long plane)
{
    HDC srcDC, destDC;
    TkWinDCState srcState, destState;
    TkpClipMask *clipPtr = (TkpClipMask *) gc->clip_mask;

    display->request++;

    if (plane != 1) {
        panic("Unexpected plane specified for XCopyPlane");
    }
    srcDC = TkWinGetDrawableDC(display, src, &srcState);

    if (src != dest) {
        destDC = TkWinGetDrawableDC(display, dest, &destState);
    } else {
        destDC = srcDC;
    }
    if ((clipPtr == NULL) || (clipPtr->type == TKP_CLIP_REGION)) {
        /*
         * Case 1: opaque bitmaps.  Windows handles the conversion
         * from one bit to multiple bits by setting 0 to the
         * foreground color, and 1 to the background color (seems
         * backwards, but there you are).
         */
        if ((clipPtr != NULL) && (clipPtr->type == TKP_CLIP_REGION)) {
            SelectClipRgn(destDC, (HRGN)clipPtr->value.region);
            OffsetClipRgn(destDC, gc->clip_x_origin, gc->clip_y_origin);
        }
        SetBkMode(destDC, OPAQUE);
        SetBkColor(destDC, gc->foreground);
        SetTextColor(destDC, gc->background);
        BitBlt(destDC, destX, destY, width, height, srcDC, srcX, srcY,
            SRCCOPY);
        if ((clipPtr != NULL) && (clipPtr->type == TKP_CLIP_REGION)) {
            SelectClipRgn(destDC, NULL);
        }
    } else if (clipPtr->type == TKP_CLIP_PIXMAP) {
        Drawable mask;
        /*
         * Case 2: transparent bitmaps are handled by setting the
         * destination to the foreground color whenever the source
         * pixel is set.
         */
        /*
         * Case 3: two arbitrary bitmaps.  Copy the source rectangle
         * into a color pixmap.  Use the result as a brush when
         * copying the clip mask into the destination.
         */
        mask = clipPtr->value.pixmap;

        {
            HDC maskDC;
            TkWinDCState maskState;

            if (mask != src) {
                maskDC = TkWinGetDrawableDC(display, mask, &maskState);
            } else {
                maskDC = srcDC;
            }
            SetBkMode(destDC, OPAQUE);
            SetTextColor(destDC, gc->background);
            SetBkColor(destDC, gc->foreground);
            BitBlt(destDC, destX, destY, width, height, srcDC, srcX, srcY, 
                   SRCINVERT);
            /* 
             * Make sure we treat the mask as a monochrome bitmap. 
             * We can get alpha-blending with non-black/white fg/bg 
             * color selections.
             */
            SetTextColor(destDC, RGB(255,255,255));
            SetBkColor(destDC, RGB(0,0,0));

            /* FIXME: Handle gc->clip_?_origin's */ 
            BitBlt(destDC, destX, destY, width, height, maskDC, 0, 0, SRCAND);

            SetTextColor(destDC, gc->background);
            SetBkColor(destDC, gc->foreground);
            BitBlt(destDC, destX, destY, width, height, srcDC, srcX, srcY, 
                   SRCINVERT);
            if (mask != src) {
                TkWinReleaseDrawableDC(mask, maskDC, &maskState);
            }
        }
    }
    if (src != dest) {
        TkWinReleaseDrawableDC(dest, destDC, &destState);
    }
    TkWinReleaseDrawableDC(src, srcDC, &srcState);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXCopyArea --
 *
 *      Copies data from one drawable to another using block transfer
 *      routines.  The small enhancement over the version in Tk is
 *      that it doesn't assume that the source and destination devices
 *      have the same resolution. This isn't true when the destination
 *      device is a printer.
 *
 *      FIXME: not true anymore.  delete this routine.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Data is moved from a window or bitmap to a second window,
 *      bitmap, or printer.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXCopyArea(
    Display *display,
    Drawable src,
    Drawable dest,
    GC gc,
    int srcX,                   /* Source X-coordinate */
    int srcY,                   /* Source Y-coordinate. */
    unsigned int width,         /* Width of area. */
    unsigned int height,        /* Height of area. */
    int destX,                  /* Destination X-coordinate (in screen 
                                 * coordinates). */
    int destY)                  /* Destination Y-coordinate (in screen
                                 * coordinates). */
{
    HDC srcDC, destDC;
    TkWinDCState srcState, destState;
    TkpClipMask *clipPtr;

    srcDC = TkWinGetDrawableDC(display, src, &srcState);
    if (src != dest) {
        destDC = TkWinGetDrawableDC(display, dest, &destState);
    } else {
        destDC = srcDC;
    }
    clipPtr = (TkpClipMask *)gc->clip_mask;
    if ((clipPtr != NULL) && (clipPtr->type == TKP_CLIP_REGION)) {
        SelectClipRgn(destDC, (HRGN)clipPtr->value.region);
        OffsetClipRgn(destDC, gc->clip_x_origin, gc->clip_y_origin);
    }

    BitBlt(destDC, destX, destY, width, height, srcDC, srcX, srcY, 
                bltModes[gc->function]);
    if ((clipPtr != NULL) && (clipPtr->type == TKP_CLIP_REGION)) {
        SelectClipRgn(destDC, NULL);
    }
    if (src != dest) {
        TkWinReleaseDrawableDC(dest, destDC, &destState);
    }
    TkWinReleaseDrawableDC(src, srcDC, &srcState);
}

static void
StippleArea(
    Display *display,
    HDC hDC,                    /* Device context: For polygons, clip
                                 * region will be installed. */
    GC gc,
    int x, int y, int w, int h)
{
    BITMAP bm;
    HBITMAP oldBitmap;
    HDC maskDC, memDC;
    Pixmap mask;
    TkWinDCState maskState;
    TkWinDrawable *twdPtr;
    int dx, dy;
    int left, top, right, bottom;
    int startX, startY;         /* Starting upper left corner of region. */
    
    twdPtr = (TkWinDrawable *)gc->stipple;
    GetObject(twdPtr->bitmap.handle, sizeof(BITMAP), &bm);

    startX = x;
    if (x < gc->ts_x_origin) {
        dx = (gc->ts_x_origin - x) % bm.bmWidth;
        if (dx > 0) {
            startX -= (bm.bmWidth - dx);
        }
    } else if (x > gc->ts_x_origin) {
        dx = (x - gc->ts_x_origin) % bm.bmWidth;
        if (dx > 0) {
            startX -= dx;
        }
    }
    startY = y;
    if (y < gc->ts_y_origin) {
        dy = (gc->ts_y_origin - y) % bm.bmHeight;
        if (dy > 0) {
            startY -= (bm.bmHeight - dy);
        }
    } else if (y >= gc->ts_y_origin) {
        dy = (y - gc->ts_y_origin) % bm.bmHeight;
        if (dy > 0) {
            startY -= dy;
        }
    }
    left = x;
    right = x + w;
    top = y;
    bottom = y + h;

    maskDC = memDC = CreateCompatibleDC(hDC);
    oldBitmap = SelectBitmap(memDC, twdPtr->bitmap.handle);
    mask = gc->stipple;
    if (gc->fill_style == FillStippled) { /* With transparency. */
        if (gc->clip_mask != None) {
            TkpClipMask *clipPtr;
            
            mask = gc->stipple;
            clipPtr = (TkpClipMask *)gc->clip_mask;
            if  (clipPtr->type == TKP_CLIP_PIXMAP) {
                mask = clipPtr->value.pixmap;
            }
        }
        if (mask != gc->stipple) {
            maskDC = TkWinGetDrawableDC(display, mask, &maskState);
        }
    }

    for (y = startY; y < bottom; y += bm.bmHeight) {
        int srcX, srcY;
        int destX, destY, destWidth, destHeight;

        srcY = 0;
        destY = y;
        destHeight = bm.bmHeight;
        if (y < top) {
            srcY = (top - y);
            destHeight = bm.bmHeight - srcY;
            destY = top;
        } 
        if ((destY + destHeight) > bottom) {
            destHeight = (bottom - destY);
        }
        for (x = startX; x < right; x += bm.bmWidth) {
            srcX = 0;
            destX = x;
            destWidth = bm.bmWidth;
            if (x < left) {
                srcX = (left - x);
                destWidth = bm.bmWidth - srcX;
                destX = left;
            } 
            if ((destX + destWidth) > right) {
                destWidth = (right - destX);
            }
            if (gc->fill_style == FillStippled) { /* With transparency. */
                SetBkMode(hDC, OPAQUE);
                SetTextColor(hDC, gc->background);
                SetBkColor(hDC, gc->foreground);
                BitBlt(hDC, destX, destY, destWidth, destHeight, memDC, 
                       srcX, srcY, SRCINVERT);
                SetTextColor(hDC, RGB(255,255,255));
                SetBkColor(hDC, RGB(0,0,0));
                BitBlt(hDC, destX, destY, destWidth, destHeight, maskDC, 
                       srcX, srcY, SRCAND);
                SetTextColor(hDC, gc->background);
                SetBkColor(hDC, gc->foreground);
                BitBlt(hDC, destX, destY, destWidth, destHeight, memDC, 
                       srcX, srcY, SRCINVERT);
            } else if (gc->fill_style == FillOpaqueStippled) { /* Opaque. */
                SetBkColor(hDC, gc->foreground);
                SetTextColor(hDC, gc->background);
                BitBlt(hDC, destX, destY, destWidth, destHeight, memDC, 
                        srcX, srcY, SRCCOPY);
            }
        }
    }
    (void)SelectBitmap(memDC, oldBitmap);
    if (maskDC != memDC) {
        TkWinReleaseDrawableDC(mask, maskDC, &maskState);
    }
    DeleteDC(memDC);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXFillPolygon --
 *
 *      This differs from Tk's XFillPolygon in that it works around
 *      deficencies in Windows 95/98: 
 *              1. Stippling bitmap is limited to 8x8.
 *              2. No tiling (with or without mask).
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXFillPolygon(
    Display *display, 
    Drawable drawable, 
    GC gc,
    XPoint *points, 
    int numPoints, 
    int shape, 
    int mode) 
{
    HDC hDC;
    int left, right, top, bottom;
    TkWinDCState state;
    int fillMode;
    POINT *winPts;

    if (drawable == None) {
        return;
    }
    /* Allocate array of POINTS to create the polygon's path. */
    winPts = Blt_Malloc(sizeof(POINT) * numPoints);
    if (winPts == NULL) {
        return;
    }
    {
        POINT *wp;
        XPoint *p, *pend;

        /* Determine the bounding box of the polygon. */
        left = right = points[0].x;
        top = bottom = points[0].y;

        wp = winPts;
        for (p = points, pend = p + numPoints; p < pend; p++) {
            if (p->x < left) {
                left = p->x;
            } 
            if (p->x > right) {
                right = p->x;
            }
            if (p->y < top) {
                top = p->y;
            } 
            if (p->y > bottom) {
                bottom = p->y;
            }
            wp->x = p->x;
            wp->y = p->y;
            wp++;
        }
    }

    hDC = TkWinGetDrawableDC(display, drawable, &state);
    SetROP2(hDC, tkpWinRopModes[gc->function]);
    fillMode = (gc->fill_rule == EvenOddRule) ? ALTERNATE : WINDING;

    if ((gc->fill_style == FillStippled) || 
        (gc->fill_style == FillOpaqueStippled)) {
#ifndef notdef
        POINT *wp, *wend;
        HRGN hRgn;

        /* Points are offsets within the bounding box. */
        for (wp = winPts, wend = wp + numPoints; wp < wend; wp++) {
            wp->x -= left;
            wp->y -= top;
        }
        /* Use the polygon as a clip path. */
        LPtoDP(hDC, winPts, numPoints);
        hRgn = CreatePolygonRgn(winPts, numPoints, fillMode);
        SelectClipRgn(hDC, hRgn);
        OffsetClipRgn(hDC, left, top);
        
        /* Tile the bounding box. */
        StippleArea(display, hDC, gc, left, top, right - left + 1, bottom - top + 1);
        SelectClipRgn(hDC, NULL), DeleteRgn(hRgn);
#else 
        TkWinDrawable *twdPtr;
        HBITMAP hBitmap;
        HBRUSH hBrush, oldBrush;
        HPEN hPen, oldPen;
        HDC hMem;
        HBITMAP oldBitmap;

        twdPtr = (TkWinDrawable *)gc->stipple;
        if (twdPtr->type != TWD_BITMAP) {
            panic("unexpected drawable type in stipple");
        }
        {
            BITMAP bm;
            int bytesPerRow;
            int y;
            unsigned char *srcRowPtr;
            unsigned char *bits;
            
            GetObject(twdPtr->bitmap.handle, sizeof(BITMAP), &bm);
            bits = Blt_GetBitmapData(display, gc->stipple, bm.bmWidth, bm.bmHeight,
                                     &bytesPerRow);
            if (bits == NULL) {
                panic("help me");
            }
            srcRowPtr = bits;
            for (y = 0; y < bm.bmHeight; y++) {
                unsigned char *bp, *bend;
                for (bp = srcRowPtr, bend = bp + bytesPerRow; bp < bend; bp++) {
                    *bp = ~*bp;
                }
                srcRowPtr += bytesPerRow;
            }
            bm.bmBits = bits;
            bm.bmType = 0;
            bm.bmPlanes = 1;
            bm.bmBitsPixel = 1;
            hBitmap = CreateBitmapIndirect(&bm);
            Blt_Free(bits);
        }
        /* hBrush = CreatePatternBrush(twdPtr->bitmap.handle); */
        hBrush = CreateHatchBrush(HS_VERTICAL, gc->foreground);
        SetBrushOrgEx(hDC, gc->ts_x_origin, gc->ts_y_origin, NULL);
        hPen = GetStockObject(NULL_PEN);
        SetTextColor(hDC, gc->foreground);
        SetBkMode(hDC, TRANSPARENT);
        oldPen = SelectPen(hDC, hPen);
        oldBrush = SelectBrush(hDC, hBrush);
        if (gc->fill_style == FillOpaqueStippled) {
            SetBkColor(hDC, gc->background);
        } else {
            SetBkMode(hDC, TRANSPARENT);
        }
        SetPolyFillMode(hDC, fillMode);
        Polygon(hDC, winPts, numPoints);
        SelectPen(hDC, oldPen), DeletePen(hPen);
        SelectBrush(hDC, oldBrush), DeleteBrush(hBrush);
#endif
    } else {
        HPEN hPen, oldPen;
        HBRUSH hBrush, oldBrush;

        /* 
         * FIXME: Right now, we're assuming that it's solid or
         * stippled and ignoring tiling. I'll merge the bits from
         * Blt_TilePolygon later. 
         */
        hPen = GetStockObject(NULL_PEN);
        oldPen = SelectPen(hDC, hPen);
        hBrush = CreateSolidBrush(gc->foreground);
        oldBrush = SelectBrush(hDC, hBrush);
        SetPolyFillMode(hDC, fillMode);
        Polygon(hDC, winPts, numPoints);
        (void)SelectPen(hDC, oldPen), DeletePen(hPen);
        (void)SelectBrush(hDC, oldBrush), DeleteBrush(hBrush);
    }
    Blt_Free(winPts);
    TkWinReleaseDrawableDC(drawable, hDC, &state);
}


static void
DrawTextOut(
    HDC hDC,                    /* HDC to draw into. */
    HFONT hFont,                /* Contains set of fonts to use when drawing
                                 * following string. */
    const char *text,           /* Potentially multilingual UTF-8 string. */
    size_t length,              /* Length of string in bytes. */
    int x, int y)               /* Coordinates at which to place origin *
                                 * of string when drawing. */
{
    HFONT oldFont;
    Tcl_Encoding encoding;
    TEXTMETRIC tm;

    oldFont = SelectFont(hDC, hFont);
    encoding = Tcl_GetEncoding(NULL, "unicode");
    GetTextMetrics(hDC, &tm);
    x -= tm.tmOverhang /2;
    if (encoding == NULL) {
        TextOutA(hDC, x, y, text, length);
    } else {
        Tcl_DString ds;
        const unsigned short *wideText;
        int wideLength;

        Tcl_DStringInit(&ds);
        Tcl_UtfToExternalDString(encoding, text, length, &ds);
        wideLength = Tcl_DStringLength(&ds) >> 1;
        wideText = (const unsigned short *)Tcl_DStringValue(&ds);
        TextOutW(hDC, x, y, wideText, wideLength);
        Tcl_DStringFree(&ds);
    }
    (void)SelectFont(hDC, oldFont);
}

void
Blt_TextOut(
    HDC hDC,
    GC gc,                      /* Graphics context for drawing characters. */
    HFONT hFont,                /* Font in which characters will be drawn;
                                 * must be the same as font used in GC. */
    const char *text,           /* UTF-8 string to be displayed.  Need not be
                                 * '\0' terminated.  All Tk meta-characters
                                 * (tabs, control characters, and newlines)
                                 * should be stripped out of the string that
                                 * is passed to this function.  If they are
                                 * not stripped out, they will be displayed as
                                 * regular printing characters. */
    int numBytes,                       /* Number of bytes in string. */
    int x, int y)               /* Coordinates at which to place origin of
                                 * string when drawing. */
{
    SetROP2(hDC, bltModes[gc->function]);
    if ((gc->clip_mask != None) && 
        ((TkpClipMask*)gc->clip_mask)->type == TKP_CLIP_REGION) {
        SelectClipRgn(hDC, (HRGN)((TkpClipMask*)gc->clip_mask)->value.region);
    }
    if (((gc->fill_style == FillStippled) || 
         (gc->fill_style == FillOpaqueStippled)) && 
        (gc->stipple != None)) {
        TkWinDrawable *twdPtr = (TkWinDrawable *)gc->stipple;
        HBRUSH oldBrush, hBrush;
        HBITMAP oldBitmap, hBitmap;
        HDC memDC;
        TEXTMETRIC tm;
        SIZE size;

        if (twdPtr->type != TWD_BITMAP) {
            Tcl_Panic("unexpected drawable type in stipple");
        }

        /*
         * Select stipple pattern into destination dc.
         */
        memDC = CreateCompatibleDC(hDC);
        hBrush = CreatePatternBrush(twdPtr->bitmap.handle);
        SetBrushOrgEx(hDC, gc->ts_x_origin, gc->ts_y_origin, NULL);
        oldBrush = SelectBrush(hDC, hBrush);

        SetTextAlign(memDC, TA_LEFT | TA_BASELINE);
        SetTextColor(memDC, gc->foreground);
        SetBkMode(memDC, TRANSPARENT);
        SetBkColor(memDC, RGB(0, 0, 0));

        /*
         * Compute the bounding box and create a compatible bitmap.
         */
        GetTextExtentPoint(memDC, text, numBytes, &size);
        GetTextMetrics(memDC, &tm);
        size.cx -= tm.tmOverhang;
        hBitmap = CreateCompatibleBitmap(hDC, size.cx, size.cy);
        oldBitmap = SelectBitmap(memDC, hBitmap);
        /*
         * The following code is tricky because fonts are rendered in multiple
         * colors.  First we draw onto a black background and copy the white
         * bits.  Then we draw onto a white background and copy the black bits.
         * Both the foreground and background bits of the font are ANDed with
         * the stipple pattern as they are copied.
         */
        PatBlt(memDC, 0, 0, size.cx, size.cy, BLACKNESS);
        DrawTextOut(hDC, hFont, text, numBytes, x, y);
        BitBlt(hDC, x, y-tm.tmAscent, size.cx, size.cy, memDC, 0, 0, 0xEA02E9);
        PatBlt(memDC, 0, 0, size.cx, size.cy, WHITENESS);
        DrawTextOut(hDC, hFont, text, numBytes, x, y);
        BitBlt(hDC, x, y-tm.tmAscent, size.cx, size.cy, memDC, 0, 0, 0x8A0E06);

        /*
         * Destroy the temporary bitmap and restore the device context.
         */
        (void)SelectBrush(memDC, oldBitmap);
        DeleteBitmap(hBitmap);
        DeleteDC(memDC);
        (void)SelectBrush(hDC, oldBrush);
        DeleteBrush(hBrush);
    } else if (gc->function == GXcopy) {
        SetTextAlign(hDC, TA_LEFT | TA_BASELINE);
        SetTextColor(hDC, gc->foreground);
        SetBkMode(hDC, TRANSPARENT);
        DrawTextOut(hDC, hFont, text, numBytes, x, y);
    } else {
        HBITMAP oldBitmap, hBitmap;
        HDC memDC;
        TEXTMETRIC tm;
        SIZE size;

        memDC = CreateCompatibleDC(hDC);
        SetTextAlign(memDC, TA_LEFT | TA_BASELINE);
        SetTextColor(memDC, gc->foreground);
        SetBkMode(memDC, TRANSPARENT);
        SetBkColor(memDC, RGB(0, 0, 0));

        /*
         * Compute the bounding box and create a compatible bitmap.
         */
        GetTextExtentPoint(memDC, text, numBytes, &size);
        GetTextMetrics(memDC, &tm);
        size.cx -= tm.tmOverhang;
        hBitmap = CreateCompatibleBitmap(hDC, size.cx, size.cy);
        oldBitmap = SelectObject(memDC, hBitmap);

        DrawTextOut(memDC, hFont, text, numBytes, 0, tm.tmAscent);
        BitBlt(hDC, x, y - tm.tmAscent, size.cx, size.cy, memDC, 0, 0, 
                bltModes[gc->function]);
        (void)SelectBitmap(memDC, oldBitmap);
        DeleteBitmap(hBitmap);
        DeleteDC(memDC);
    }
}

Region
Blt_EmulateXPolygonRegion(XPoint xPointArr[], int numPoints, int rule)
{
    HRGN hRgn; 
    POINT *points;
    POINT staticPoints[5];
    int i;

    if (numPoints > 5) {
        points = Blt_Malloc(sizeof(POINT) * numPoints);
        if (points == NULL) {
            return NULL;
        }
    } else {
        points = staticPoints;
    }
    for (i = 0; i < numPoints; i++) {
        points[i].x = (int)xPointArr[i].x;
        points[i].y = (int)xPointArr[i].y;
    }
    if (rule == EvenOddRule) {
        hRgn = CreatePolygonRgn(points, numPoints, ALTERNATE);
    } else if (rule == WindingRule) {
        hRgn = CreatePolygonRgn(points, numPoints, WINDING);
    } else {
        hRgn = NULL;
    }
    if (hRgn == NULL) {
    }
    if (points != staticPoints) {
        Blt_Free(points);
    }
    return (Region)hRgn;
}

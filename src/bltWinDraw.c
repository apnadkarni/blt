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

typedef struct _DCState {
    TkWinDCState state;
    TkpClipMask *clipPtr;
    GC gc;
    Drawable drawable;
    HDC dc;
    HRGN hOldRegion;
} DCState;

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

#if (_TCL_VERSION <  _VERSION(8,1,0)) 
typedef void *Tcl_Encoding;            /* Make up dummy type for encoding.  */
#endif

HPALETTE
Blt_GetSystemPalette(void)
{
    DWORD flags;
    HDC hDC;
    HPALETTE hPalette;

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

static HDC
GetDCAndState(Display *display, Drawable drawable, GC gc, DCState *statePtr)
{
    statePtr->clipPtr = (TkpClipMask *)gc->clip_mask;
    statePtr->dc = TkWinGetDrawableDC(display, drawable, &statePtr->state);
    statePtr->drawable = drawable;
    statePtr->hOldRegion = NULL;
    if ((statePtr->clipPtr != NULL) && 
        (statePtr->clipPtr->type == TKP_CLIP_REGION)) {
        statePtr->hOldRegion = SelectClipRgn(statePtr->dc,
               (HRGN)statePtr->clipPtr->value.region);
        OffsetClipRgn(statePtr->dc, gc->clip_x_origin, gc->clip_y_origin);
    }
    SetROP2(statePtr->dc, tkpWinRopModes[gc->function]);
    return statePtr->dc;
}

static void
ReleaseDCAndState(DCState *statePtr)
{
    if ((statePtr->clipPtr != NULL) && 
        (statePtr->clipPtr->type==TKP_CLIP_REGION)) {
        SelectClipRgn(statePtr->dc, statePtr->hOldRegion);
    }
    TkWinReleaseDrawableDC(statePtr->drawable, statePtr->dc, &statePtr->state);
}

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
    BITMAPINFOHEADER *bmihPtr;
    HANDLE hMem, hMem2;
    HBITMAP hBitmap;
    HDC hDC;
    TkWinDCState state;
    int bytesPerRow, imageSize;
    int result;
    unsigned char *bits;
    unsigned int size;

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
 * Blt_EmulateXFree --
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
 * Blt_EmulateXMaxRequestSize --
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
 * Blt_EmulateXLowerWindow --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXLowerWindow(Display *display, Window window)
{
    HWND hWnd;

    hWnd = Tk_GetHWND(window);
    display->request++;
    SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXRaiseWindow --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXRaiseWindow(Display *display, Window window)
{
    HWND hWnd;

    hWnd = Tk_GetHWND(window);
    display->request++;
    SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXUnmapWindow --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXUnmapWindow(Display *display, Window window)
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
 * Blt_EmulateXWarpPointer --
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
Blt_EmulateXWarpPointer(Display *display, Window src, Window dest, int sx,
                        int sy, unsigned int srcWidth, unsigned int srcHeight,
                        int dx, int dy)
{
    if (src != None) {
        int x, y, w, h;
        POINT p;
    
        GetCursorPos(&p);
        if (Blt_GetWindowExtents(display, src, &x, &y, &w, &h) != TCL_OK) {
            return;                     /* Can't get window extents. */
        }
        if ((p.y < y) || (p.y >= (y + h)) || (p.x < x) || (p.x >= (x + w))) {
            return;                     /* Point is outside of src window. */
        }
    }
    if (dest == None) {
        POINT p;
    
        GetCursorPos(&p);
        SetCursorPos(p.x + dx, p.y + dy);
    } else {
        int x, y, w, h;

        if (Blt_GetWindowExtents(display, dest, &x, &y, &w, &h) != TCL_OK) {
            return;                     /* Can't get window extents. */
        }
        SetCursorPos(x + dx, y + dy);
    }
}

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
GetDashInfo(HDC hDC, GC gc, DashInfo *infoPtr)
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
    infoPtr->dc = hDC;
    infoPtr->numBits = dashValue;
    infoPtr->offset = dashOffset;
    infoPtr->count = 0;
    infoPtr->color = gc->foreground;
    return TRUE;
}

#ifdef notdef
void
Blt_SetROP2(HDC hDC, int function)
{
    SetROP2(hDC, tkpWinRopModes[function]);
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
Blt_EmulateXCreateGC(Display *display, Drawable drawable, unsigned long mask,
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
 *        PS_DASH       c,g     c,g     c,g
 *        PS_DOT          c       c     c,g
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
Blt_GCToPen(HDC hDC, GC gc)
{
    DWORD *dashPtr;
    DWORD dashArr[12];
    DWORD lineAttrs, lineStyle;
    HPEN hPen;
    LOGBRUSH logBrush;
    int numDashValues, lineWidth;

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

    logBrush.lbStyle = BS_SOLID;
    logBrush.lbColor = gc->foreground;
    logBrush.lbHatch = 0;         /* Value is ignored when style is BS_SOLID. */

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
    SetBkMode(hDC, TRANSPARENT);

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
            hPen = ExtCreatePen(PS_GEOMETRIC | lineAttrs | lineStyle, lineWidth,
                       &logBrush, numDashValues, dashPtr);
        } else {
            /* Cosmetic pens are much faster. */
            hPen = ExtCreatePen(PS_COSMETIC | lineAttrs | lineStyle, 1, 
                        &logBrush, numDashValues, dashPtr);
        }           
    } else {
        /* Windows 95/98. */
        if ((lineStyle == PS_SOLID) && (lineWidth > 1)) {
            /* Use geometric pens with solid, thick lines only. */
            hPen = ExtCreatePen(PS_GEOMETRIC | lineAttrs | lineStyle, lineWidth,
                       &logBrush, 0, NULL);
        } else {
            /* Otherwise sacrifice thick lines for dashes. */
            hPen = ExtCreatePen(PS_COSMETIC | lineStyle, 1, &logBrush, 0, NULL);
        }
    } 
    assert(hPen != NULL);
    return hPen;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXDrawRectangles --
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
                           XRectangle *xRectangleArr, int numRectangles)
{
    HBRUSH hBrush, hOldBrush;
    HDC hDC;
    HPEN hPen, hOldPen;
    DCState state;
    int i;

    if (drawable == None) {
        return;
    }
    hDC = GetDCAndState(display, drawable, gc, &state);

    hPen = Blt_GCToPen(hDC, gc);
    hOldPen = SelectPen(hDC, hPen);

    hBrush = GetStockObject(NULL_BRUSH);
    hOldBrush = SelectBrush(hDC, hBrush);

    SetROP2(hDC, tkpWinRopModes[gc->function]);
    for (i = 0; i < numRectangles; i++) {
        int x1, y1, x2, y2;

        x1 = xRectangleArr[i].x;
        y1 = xRectangleArr[i].y;
        /* FIXME: Shouldn't this be -1. (Copied from Tk). */
        x2 = xRectangleArr[i].x + xRectangleArr[i].width + 1;
        y2 = xRectangleArr[i].y + xRectangleArr[i].height + 1;
        Rectangle(hDC, x1, y1, x2, y2);
    }
    SelectPen(hDC, hOldPen),     DeletePen(hPen);
    SelectBrush(hDC, hOldBrush), DeleteBrush(hBrush);
    ReleaseDCAndState(&state);
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
DrawMiniArc(HDC hDC,int width, int x, int y, int mask, COLORREF inner,
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
                SetPixelV(hDC, x + i, y + j, outer);
            } else if (bit & FILL) {
                SetPixelV(hDC, x + i, y + j, inner);
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
    HDC hDC,
    int arcMode,                /* Mode: either ArcChord or ArcPieSlice */
    XArc *arcPtr,
    HPEN hPen,
    HBRUSH hBrush)
{
    int start, extent, clockwise;
    int xstart, ystart, xend, yend;
    double radian_start, radian_end, xr, yr;
    double dx, dy;

    if ((arcPtr->angle1 == 0) && (arcPtr->angle2 == 23040)) {
        /* Handle special case of circle or ellipse */
        Ellipse(hDC, arcPtr->x, arcPtr->y, arcPtr->x + arcPtr->width + 1,
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

    if (hBrush == 0) {
        /*
         * Note that this call will leave a gap of one pixel at the
         * end of the arc for thin arcs.  We can't use ArcTo because
         * it's only supported under Windows NT.
         */
        Arc(hDC, arcPtr->x, arcPtr->y, arcPtr->x + arcPtr->width + 1,
            arcPtr->y + arcPtr->height + 1, xstart, ystart, xend, yend);
        /* FIXME: */
    } else {
        if (arcMode == ArcChord) {
            Chord(hDC, arcPtr->x, arcPtr->y, arcPtr->x + arcPtr->width + 1,
                arcPtr->y + arcPtr->height + 1, xstart, ystart, xend, yend);
        } else if (arcMode == ArcPieSlice) {
            Pie(hDC, arcPtr->x, arcPtr->y, arcPtr->x + arcPtr->width + 1,
                arcPtr->y + arcPtr->height + 1, xstart, ystart, xend, yend);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXDrawArcs --
 *
 *      Draws multiple circular or elliptical arcs.  Each arc is specified
 *      by a rectangle and two angles.  The center of the circle or ellipse
 *      is the center of the rect- angle, and the major and minor axes are
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
Blt_EmulateXDrawArcs(Display *display, Drawable drawable, GC gc, XArc *xArcArr,
                     int numArcs)
{
    DCState state;
    HBRUSH hBrush, hOldBrush;
    HDC hDC;
    HPEN hPen, hOldPen;
    int i;

    display->request++;
    if (drawable == None) {
        return;
    }
    hDC = GetDCAndState(display, drawable, gc, &state);
    hPen = Blt_GCToPen(hDC, gc);
    hOldPen = SelectPen(hDC, hPen);
    hBrush = GetStockBrush(NULL_BRUSH);
    hOldBrush = SelectBrush(hDC, hBrush);
    for (i = 0; i < numArcs; i++) {
        DrawArc(hDC, gc->arc_mode, xArcArr + i, hPen, 0);
    }
    DeleteBrush(SelectBrush(hDC, hOldBrush));
    DeletePen(SelectPen(hDC, hOldPen));
    ReleaseDCAndState(&state);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXFillArcs --
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
Blt_EmulateXFillArcs(Display *display, Drawable drawable, GC gc, XArc *xArcArr,
                     int numArcs)
{
    HBRUSH hBrush, hOldBrush;
    HPEN hPen, hOldPen;
    HDC hDC;
    DCState state;
    int i;

    display->request++;
    if (drawable == None) {
        return;
    }
    hDC = GetDCAndState(display, drawable, gc, &state);
    hPen = Blt_GCToPen(hDC, gc);
    hOldPen = SelectPen(hDC, hPen);
    hBrush = CreateSolidBrush(gc->foreground);
    hOldBrush = SelectBrush(hDC, hBrush);
    for (i = 0; i < numArcs; i++) {
        DrawArc(hDC, gc->arc_mode, xArcArr + i, hPen, hBrush);
    }
    DeleteBrush(SelectBrush(hDC, hOldBrush));
    DeletePen(SelectPen(hDC, hOldPen));
    ReleaseDCAndState(&state);
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


/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXDrawLine --
 *
 *      Draw line segment.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

void
Blt_EmulateXDrawLine(Display *display, Drawable drawable, GC gc, 
                     int x1, int y1, int x2, int y2)
{
    DCState state;
    HDC hDC;

    if (drawable == None) {
        return;
    }
    hDC = GetDCAndState(display, drawable, gc, &state);
    if (gc->line_style != LineSolid) {
        /* Handle dotted lines specially */
        DashInfo info;
        int count;

        if (!GetDashInfo(hDC, gc, &info)) {
            goto solidLine;
        }
        count = info.offset / info.numBits;
        if (x1 == x2) {         /* Vertical line */
            int y;

            for (y = y1; y <= y2; y += 2) {
                SetPixelV(hDC, x1, y + count, info.color);
            }
        } else if (y1 == y2) {  /* Horizontal line */
            int x;

            for (x = x1; x <= x2; x += 2) {
                SetPixelV(hDC, x + count, y1, info.color);
            }
        } else {
            LineDDA(x1, y1, x2, y2, DrawDot, (LPARAM)&info);
        }
    } else {
        HPEN hPen, hOldPen;
        HBRUSH hBrush, hOldBrush;

      solidLine:
        hPen = Blt_GCToPen(hDC, gc);
        hOldPen = SelectPen(hDC, hPen);
        hBrush = CreateSolidBrush(gc->foreground);
        hOldBrush = SelectBrush(hDC, hBrush);
        MoveToEx(hDC, x1, y1, (LPPOINT)NULL);
        LineTo(hDC, x2, y2);
        DeletePen(SelectPen(hDC, hOldPen));
        DeleteBrush(SelectBrush(hDC, hOldBrush));
    }
    ReleaseDCAndState(&state);
}

static void
DrawLine(Display *display, Drawable drawable, GC gc, POINT *winPointArr, 
         int numPoints)
{
    DCState state;
    HDC hDC;
    int i, n;
    int start, extra, size;
    HPEN hPen, hOldPen;
    HBRUSH hBrush, hOldBrush;

    if (drawable == None) {
        return;
    }
    hDC = GetDCAndState(display, drawable, gc, &state);
    hPen = Blt_GCToPen(hDC, gc);
    hOldPen = SelectPen(hDC, hPen);
    hBrush = CreateSolidBrush(gc->foreground);
    hOldBrush = SelectBrush(hDC, hBrush);
    
    start = extra = 0;

    /*  
     * Depending if the line is wide (> 1 pixel), arbitrarily break the
     * line in sections of 100 points.  This bit of weirdness has to do
     * with wide geometric pens.  The longer the polyline, the slower it
     * draws.  The trade off is that we lose dash and cap uniformity for
     * unbearably slow polyline draws.
     */
    if (gc->line_width > 1) {
        size = 100;
    } else {
        size = numPoints;
    }
    for (i = numPoints; i > 0; i -= size) {
        n = MIN(i, size);
        Polyline(hDC, winPointArr + start, n + extra);
        start += (n - 1);
        extra = 1;
    }
    DeletePen(SelectPen(hDC, hOldPen));
    DeleteBrush(SelectBrush(hDC, hOldBrush));
    ReleaseDCAndState(&state);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXDrawLines --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXDrawLines(Display *display, Drawable drawable, GC gc, 
                      XPoint *xPointArr, int numPoints, int mode)
{
    if (drawable == None) {
        return;
    }
    if (gc->line_style != LineSolid) {  /* Handle dotted lines specially */
        DashInfo info;
        DCState state;
        HDC hDC;
        int result;

        hDC = GetDCAndState(display, drawable, gc, &state);
        result = GetDashInfo(hDC, gc, &info);
        if (result) {
            XPoint *p1, *p2;
            int i;

            p1 = xPointArr;
            p2 = p1 + 1;
            for (i = 1; i < numPoints; i++, p1++, p2++) {
                LineDDA(p1->x, p1->y, p2->x, p2->y, DrawDot, (LPARAM)&info);
            }
            result = TCL_OK;
        }
        ReleaseDCAndState(&state);
        if (result) {
            return;
        }
    } else {
        POINT *winPointArr;

        winPointArr = Blt_Malloc(sizeof(POINT) * numPoints);
        if (winPointArr == NULL) {
            return;
        }
        if (mode == CoordModeOrigin) {
            int i;

            for (i = 0; i < numPoints; i++) {
                winPointArr[i].x = (int)xPointArr[i].x;
                winPointArr[i].y = (int)xPointArr[i].y;
            }
        } else {
            int i, j;
            
            winPointArr[0].x = (int)xPointArr[0].x;
            winPointArr[0].y = (int)xPointArr[0].y;
            for (i = 1, j = 0; i < numPoints; i++, j++) {
                winPointArr[i].x = winPointArr[j].x + (int)xPointArr[i].x;
                winPointArr[i].y = winPointArr[j].y + (int)xPointArr[i].y;
            }
        }
        DrawLine(display, drawable, gc, winPointArr, numPoints);
        Blt_Free(winPointArr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXDrawSegments --
 *
 *      Draws multiple, unconnected lines. For each segment, draws a line
 *      between (x1, y1) and (x2, y2).  It draws the lines in the order
 *      listed in the array of XSegment structures and does not perform
 *      joining at coincident endpoints.  For any given line, does not draw
 *      a pixel more than once. If lines intersect, the intersecting pixels
 *      are drawn multiple times.
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
                         XSegment *xSegmentArr, int numSegments)
{
    HDC hDC;
    DCState state;

    display->request++;
    if (drawable == None) {
        return;
    }
    hDC = GetDCAndState(display, drawable, gc, &state);
    if (gc->line_style != LineSolid) {
        /* Handle dotted lines specially */
        DashInfo info;
        int i;

        if (!GetDashInfo(hDC, gc, &info)) {
            goto solidLine;
        }
        for (i = 0; i < numSegments; i++) {
            info.count = 0; /* Reset dash counter after every segment. */
            LineDDA(xSegmentArr[i].x1, xSegmentArr[i].y1, xSegmentArr[i].x2, 
                    xSegmentArr[i].y2, DrawDot, (LPARAM)&info);
        }
    } else {
        HPEN hPen, hOldPen;
        int i;

      solidLine:
        hPen = Blt_GCToPen(hDC, gc);
        hOldPen = SelectPen(hDC, hPen);
        for (i = 0; i < numSegments; i++) {
            MoveToEx(hDC, xSegmentArr[i].x1, xSegmentArr[i].y1, (LPPOINT)NULL);
            LineTo(hDC, xSegmentArr[i].x2, xSegmentArr[i].y2);
        }
        DeletePen(SelectPen(hDC, hOldPen));
    }
    ReleaseDCAndState(&state);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXDrawRectangle --
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
    HPEN hPen, hOldPen;
    HBRUSH hBrush, hOldBrush;
    HDC hDC;

    if (drawable == None) {
        return;
    }
    
    hDC = GetDCAndState(display, drawable, gc, &state);
    hPen = Blt_GCToPen(hDC, gc);
    hBrush = GetStockObject(NULL_BRUSH);
    hOldPen = SelectPen(hDC, hPen);
    hOldBrush = SelectBrush(hDC, hBrush);
    if (gc->line_style != LineSolid) {
        /* Handle dotted lines specially */
        int x2, y2;
        DashInfo info;

        if (!GetDashInfo(hDC, gc, &info)) {
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
        Rectangle(hDC, x, y, x + w + 1, y + h + 1);
    }
    DeletePen(SelectPen(hDC, hOldPen));
    DeleteBrush(SelectBrush(hDC, hOldBrush));
    ReleaseDCAndState(&state);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXDrawPoints --
 *
 *      Uses the foreground pixel and function components of the GC to draw
 *      a multiple points into the specified drawable.  CoordModeOrigin
 *      treats all coordinates as relative to the origin, and
 *      CoordModePrevious treats all coordinates after the first as
 *      relative to the previous point.
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
                       XPoint *xPointArr, int numPoints, int mode)
/* Ignored. CoordModeOrigin is assumed. */
{                                   
    DCState state;
    HDC hDC;
    int i;

    display->request++;
    if (drawable == None) {
        return;
    }
    hDC = GetDCAndState(display, drawable, gc, &state);
    for (i = 0; i < numPoints; i++) {
        SetPixelV(hDC, xPointArr[i].x, xPointArr[i].y, gc->foreground);
    }
    ReleaseDCAndState(&state);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXReparentWindow --
 *
 *      If the specified window is mapped, automatically performs an
 *      UnmapWindow request on it, removes it from its current position in
 *      the hierarchy, and inserts it as the child of the specified parent.
 *      The window is placed in the stacking order on top with respect to
 *      sibling windows.
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
    HWND hChild, hNewParent;

    hChild = Tk_GetHWND(window);
    hNewParent = Tk_GetHWND(parent);
    SetParent(hChild, hNewParent);
    SetWindowLong(hChild, GWL_STYLE, WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS);
    XMoveWindow(display, window, x, y);
    XRaiseWindow(display, window);
    XMapWindow(display, window);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXSetDashes --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EmulateXSetDashes(Display *display, GC gc, int dashOffset, 
                      _Xconst char *dashList, int n)
{
    assert(n >= 0);
    gc->dashes = n;
    gc->dash_offset = (uintptr_t)dashList;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXDrawString --
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

/*
 *---------------------------------------------------------------------------
 *
 * TileArea --
 *
 *      Helper routine to tile rectangular areas.  Used to tile rectangles
 *      and polygons (clip mask needed for polygons).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Paints the specified src DC across the destination DC.
 *
 *---------------------------------------------------------------------------
 */
static void
TileArea(HDC hDestDC, HDC hSrcDC, int tileOriginX, int tileOriginY, 
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
            BitBlt(hDestDC, destX, destY, destWidth, destHeight, 
                hSrcDC, srcX, srcY, SRCCOPY);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXFillRectangles --
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
                           XRectangle *xRectangleArr, int numRectangles)
{
    HBRUSH hOldBrush, hFgBrush, hBgBrush, hBrush;
    HDC hDC;
    HDC hMemDC;
    DCState state;
    TkWinDrawable *twdPtr;
    int i;
    
    if (drawable == None) {
        return;
    }
    hDC = GetDCAndState(display, drawable, gc, &state);
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
            HBITMAP hOldBitmap;
            BITMAP bm;
            int i;

            GetObject(twdPtr->bitmap.handle, sizeof(BITMAP), &bm);
            hMemDC = CreateCompatibleDC(hDC);
            hOldBitmap = SelectBitmap(hMemDC, twdPtr->bitmap.handle);
            for (i = 0; i < numRectangles; i++) {
                int x, y, w, h;
                
                x = xRectangleArr[i].x;
                y = xRectangleArr[i].y;
                w = xRectangleArr[i].width;
                h = xRectangleArr[i].height;
                TileArea(hDC, hMemDC, gc->ts_x_origin, gc->ts_y_origin, 
                         bm.bmWidth, bm.bmHeight, x, y, w, h);
            }
            SelectBitmap(hMemDC, hOldBitmap);
            DeleteDC(hMemDC);
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
        hOldBrush = SelectBrush(hDC, hBrush);
        hMemDC = CreateCompatibleDC(hDC);

        /*
         * For each rectangle, create a drawing surface which is the size of
         * the rectangle and fill it with the background color.  Then merge the
         * result with the stipple pattern.
         */
        hFgBrush = CreateSolidBrush(gc->foreground);
        hBgBrush = CreateSolidBrush(gc->background);
        for (i = 0; i < numRectangles; i++) {
            RECT r;
            int x, y, w, h;
            HBITMAP hOldBitmap, hBitmap;
                
            x = xRectangleArr[i].x;
            y = xRectangleArr[i].y;
            w = xRectangleArr[i].width;
            h = xRectangleArr[i].height;

            hBitmap = CreateCompatibleBitmap(hDC, w, h);
            hOldBitmap = SelectObject(hMemDC, hBitmap);
            r.left = r.top = 0;
            r.right = w;
            r.bottom = h;
            FillRect(hMemDC, &r, hFgBrush);
            BitBlt(hDC, x, y, w, h, hMemDC, 0, 0, COPYBG);
            if (gc->fill_style == FillOpaqueStippled) {
                FillRect(hMemDC, &r, hBgBrush);
                BitBlt(hDC, x, y, w, h, hMemDC, 0, 0, COPYFG);
            }
            SelectObject(hMemDC, hOldBitmap);
            DeleteObject(hBitmap);
        }
        DeleteBrush(hFgBrush);
        DeleteBrush(hBgBrush);
        DeleteDC(hMemDC);
        (void)SelectBrush(hDC, hOldBrush);
        DeleteBrush(hBrush);
        break;

    case FillSolid:
        fillSolid:
#ifdef notdef
        hMemDC = CreateCompatibleDC(hDC);
#endif
        hFgBrush = CreateSolidBrush(gc->foreground);
        for (i = 0; i < numRectangles; i++) {
            RECT r;
            int x, y, w, h;
                
            x = xRectangleArr[i].x;
            y = xRectangleArr[i].y;
            w = xRectangleArr[i].width;
            h = xRectangleArr[i].height;

            /* Note that width and height is already assumed to be
             * subtracted by one. */
            r.left = x, r.top = y;
            r.right = x + w;
            r.bottom = y + h;
            FillRect(hDC, &r, hFgBrush);
#ifdef notdef
            BitBlt(hDC, rp->x, rp->y, rp->width, rp->height, hMemDC, 0, 0, 
                SRCCOPY);
            SelectObject(hMemDC, hOldBitmap);
            DeleteObject(hBitmap);
#endif
        }
        DeleteBrush(hFgBrush);
#ifdef notdef
        DeleteDC(hMemDC);
#endif
        break;
    }
    ReleaseDCAndState(&state);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXFillRectangle --
 *
 *      Fill rectangular area in the given drawable.  Handles tiling.
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
Blt_EmulateXFillRectangle(Display *display, Drawable drawable, GC gc,
                          int x, int y, unsigned int w, unsigned int h)
{
    HDC hDC;
    RECT r;
    DCState state;

    if (drawable == None) {
        return;
    }
    hDC = GetDCAndState(display, drawable, gc, &state);
    r.left = r.top = 0;
    r.right = w, r.bottom = h;

    switch(gc->fill_style) {
    case FillTiled:
        {
            TkWinDrawable *twdPtr;
            HBITMAP hOldBitmap;
            HDC hMemDC;
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

            hMemDC = CreateCompatibleDC(hDC);
            hOldBitmap = SelectBitmap(hMemDC, twdPtr->bitmap.handle);
            TileArea(hDC, hMemDC, gc->ts_x_origin, gc->ts_y_origin,
                     bm.bmWidth, bm.bmHeight, x, y, w, h);
            (void)SelectBitmap(hMemDC, hOldBitmap);
            DeleteDC(hMemDC);
        }
        break; 
            
    case FillOpaqueStippled:
    case FillStippled:
        {
            TkWinDrawable *twdPtr;
            HBRUSH hOldBrush, hBrush;
            HBRUSH hFgBrush, hBgBrush;
            HBITMAP hOldBitmap, hBitmap;
            HDC hMemDC;

            if (gc->stipple == None) {
                goto fillSolid;
            }
            twdPtr = (TkWinDrawable *)gc->stipple;
            if (twdPtr->type != TWD_BITMAP) {
                panic("unexpected drawable type in stipple");
            }
            hBrush = CreatePatternBrush(twdPtr->bitmap.handle);
            SetBrushOrgEx(hDC, gc->ts_x_origin, gc->ts_y_origin, NULL);
            hOldBrush = SelectBrush(hDC, hBrush);
            hMemDC = CreateCompatibleDC(hDC);
            
            hFgBrush = CreateSolidBrush(gc->foreground);
            hBgBrush = CreateSolidBrush(gc->background);
            hBitmap = CreateCompatibleBitmap(hDC, w, h);
            hOldBitmap = SelectObject(hMemDC, hBitmap);
            FillRect(hMemDC, &r, hFgBrush);
            SetBkMode(hDC, TRANSPARENT);
            BitBlt(hDC, x, y, w, h, hMemDC, 0, 0, COPYFG);
            if (gc->fill_style == FillOpaqueStippled) {
                FillRect(hMemDC, &r, hBgBrush);
                BitBlt(hDC, x, y, w, h, hMemDC, 0, 0, COPYBG);
            }
            (void)SelectBrush(hDC, hOldBrush);
            (void)SelectBitmap(hMemDC, hOldBitmap);
            DeleteBrush(hFgBrush);
            DeleteBrush(hBgBrush);
            DeleteBrush(hBrush);
            DeleteBitmap(hBitmap);
            DeleteDC(hMemDC);
        }
        break;

    case FillSolid:
        {
            HBRUSH hBrush;
            HBITMAP hOldBitmap, hBitmap;
            HDC hMemDC;

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
    ReleaseDCAndState(&state);
}

#ifdef notdef
#if (_TCL_VERSION >= _VERSION(8,1,0)) 
static BOOL
DrawChars(HDC hDC, int x, int y, char *string, int length)
{
    BOOL result;

    if (systemEncoding == NULL) {
        result = TextOutA(hDC, x, y, string, length);
    } else {
        Tcl_DString ds;
        const unsigned short *ws;
        int wlength;

        Tcl_DStringInit(&ds);
        Tcl_UtfToExternalDString(systemEncoding, string, length, &ds);
        wlength = Tcl_DStringLength(&ds) >> 1;
        ws = (const unsigned short *)Tcl_DStringValue(&ds);
        result = TextOutW(hDC, x, y, ws, wlength);
        Tcl_DStringFree(&ds);
    }
    return result;
}
#else
static BOOL
DrawChars(HDC hDC, int x, int y, char *string, int length)
{
    return TextOutA(hDC, x, y, string, length);
}
#endif /* _TCL_VERSION >= _VERSION(8,1,0) */
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
 *      bitmaps that doesn't copy the background or use brushes. 
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
    HDC hSrcDC, hDestDC;
    TkWinDCState srcState, destState;
    TkpClipMask *clipPtr = (TkpClipMask *) gc->clip_mask;

    display->request++;

    if (plane != 1) {
        panic("Unexpected plane specified for XCopyPlane");
    }
    hSrcDC = TkWinGetDrawableDC(display, src, &srcState);

    if (src != dest) {
        hDestDC = TkWinGetDrawableDC(display, dest, &destState);
    } else {
        hDestDC = hSrcDC;
    }
    if ((clipPtr == NULL) || (clipPtr->type == TKP_CLIP_REGION)) {
        HRGN hOldRegion;

        /*
         * Case 1: Opaque bitmaps.  Windows handles the conversion from one
         *         bit to multiple bits by setting 0 to the foreground
         *         color, and 1 to the background color (seems backwards,
         *         but there you are).
         */
        if ((clipPtr != NULL) && (clipPtr->type == TKP_CLIP_REGION)) {
            hOldRegion = SelectClipRgn(hDestDC, (HRGN)clipPtr->value.region);
            OffsetClipRgn(hDestDC, gc->clip_x_origin, gc->clip_y_origin);
        }
        SetBkMode(hDestDC, OPAQUE);
        SetBkColor(hDestDC, gc->foreground);
        SetTextColor(hDestDC, gc->background);
        BitBlt(hDestDC, destX, destY, width, height, hSrcDC, srcX, srcY,
            SRCCOPY);
        if ((clipPtr != NULL) && (clipPtr->type == TKP_CLIP_REGION)) {
            SelectClipRgn(hDestDC, hOldRegion);
        }
    } else if (clipPtr->type == TKP_CLIP_PIXMAP) {
        Drawable mask;
        /*
         * Case 2: Transparent bitmaps are handled by setting the
         *         destination to the foreground color whenever the source
         *         pixel is set.
         */
        /*
         * Case 3: Two arbitrary bitmaps.  Copy the source rectangle into a
         *         color pixmap.  Use the result as a brush when copying
         *         the clip mask into the destination.
         */
        mask = clipPtr->value.pixmap;

        {
            HDC hMaskDC;
            TkWinDCState maskState;

            if (mask != src) {
                hMaskDC = TkWinGetDrawableDC(display, mask, &maskState);
            } else {
                hMaskDC = hSrcDC;
            }
            SetBkMode(hDestDC, OPAQUE);
            SetTextColor(hDestDC, gc->background);
            SetBkColor(hDestDC, gc->foreground);
            BitBlt(hDestDC, destX, destY, width, height, hSrcDC, srcX, srcY, 
                   SRCINVERT);
            /* 
             * Make sure we treat the mask as a monochrome bitmap.  We can
             * get alpha-blending with non-black/white fg/bg color
             * selections.
             */
            SetTextColor(hDestDC, RGB(255,255,255));
            SetBkColor(hDestDC, RGB(0,0,0));

            /* FIXME: Handle gc->clip_?_origin's */ 
            BitBlt(hDestDC, destX, destY, width, height, hMaskDC, 0, 0, SRCAND);

            SetTextColor(hDestDC, gc->background);
            SetBkColor(hDestDC, gc->foreground);
            BitBlt(hDestDC, destX, destY, width, height, hSrcDC, srcX, srcY, 
                   SRCINVERT);
            if (mask != src) {
                TkWinReleaseDrawableDC(mask, hMaskDC, &maskState);
            }
        }
    }
    if (src != dest) {
        TkWinReleaseDrawableDC(dest, hDestDC, &destState);
    }
    TkWinReleaseDrawableDC(src, hSrcDC, &srcState);
}

static void
StippleArea(Display *display,
    HDC hDC,                    /* Device context: For polygons, clip
                                 * region will be installed. */
    GC gc,
    int x, int y, int w, int h)
{
    BITMAP bm;
    HBITMAP hOldBitmap;
    HDC hMaskDC, hMemDC;
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

    hMaskDC = hMemDC = CreateCompatibleDC(hDC);
    hOldBitmap = SelectBitmap(hMemDC, twdPtr->bitmap.handle);
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
            hMaskDC = TkWinGetDrawableDC(display, mask, &maskState);
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
                BitBlt(hDC, destX, destY, destWidth, destHeight, hMemDC, 
                       srcX, srcY, SRCINVERT);
                SetTextColor(hDC, RGB(255,255,255));
                SetBkColor(hDC, RGB(0,0,0));
                BitBlt(hDC, destX, destY, destWidth, destHeight, hMaskDC, 
                       srcX, srcY, SRCAND);
                SetTextColor(hDC, gc->background);
                SetBkColor(hDC, gc->foreground);
                BitBlt(hDC, destX, destY, destWidth, destHeight, hMemDC, 
                       srcX, srcY, SRCINVERT);
            } else if (gc->fill_style == FillOpaqueStippled) { /* Opaque. */
                SetBkColor(hDC, gc->foreground);
                SetTextColor(hDC, gc->background);
                BitBlt(hDC, destX, destY, destWidth, destHeight, hMemDC, 
                        srcX, srcY, SRCCOPY);
            }
        }
    }
    (void)SelectBitmap(hMemDC, hOldBitmap);
    if (hMaskDC != hMemDC) {
        TkWinReleaseDrawableDC(mask, hMaskDC, &maskState);
    }
    DeleteDC(hMemDC);
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
Blt_EmulateXFillPolygon(Display *display, Drawable drawable, GC gc,
                        XPoint *xPointArr, int numPoints, int shape, int mode) 
{
    HDC hDC;
    POINT *winPointArr;
    POINT staticPoints[64];
    TkWinDCState state;
    int fillMode;
    int i;
    int x1, x2, y1, y2, w, h;

    if (drawable == None) {
        return;
    }
    if (numPoints > 64) {
        winPointArr = Blt_Malloc(sizeof(POINT) * numPoints);
        if (winPointArr == NULL) {
            return;
        }
    } else {
        winPointArr = staticPoints;
    }

    /* Determine the bounding box of the polygon. */
    x1 = x2 = xPointArr[0].x;
    y1 = y2 = xPointArr[0].y;
    for (i = 0 ; i < numPoints; i++) {
        if (xPointArr[i].x < x1) {
            x1 = xPointArr[i].x;
        } 
        if (xPointArr[i].x > x2) {
            x2 = xPointArr[i].x;
        }
        if (xPointArr[i].y < y1) {
            y1 = xPointArr[i].y;
        } 
        if (xPointArr[i].y > y2) {
            y2 = xPointArr[i].y;
        }
        winPointArr[i].x = xPointArr[i].x;
        winPointArr[i].y = xPointArr[i].y;
    }
    w = x2 - x1 + 1;
    h = y2 - y1 + 1;
    
    hDC = TkWinGetDrawableDC(display, drawable, &state);
    SetROP2(hDC, tkpWinRopModes[gc->function]);
    fillMode = (gc->fill_rule == EvenOddRule) ? ALTERNATE : WINDING;

    switch(gc->fill_style) {
    case FillStippled:
    case FillOpaqueStippled:
        {
            int i;
            HRGN hRgn, hOldRegion;
            
            /* Points are offsets within the bounding box. */
            for (i = 0; i <  numPoints; i++) {
                winPointArr[i].x -= x1;
                winPointArr[i].y -= y1;
            }
            /* Use the polygon as a clip path. */
            LPtoDP(hDC, winPointArr, numPoints);
            /* FIXME: Really need to combine with current region. */
            hRgn = CreatePolygonRgn(winPointArr, numPoints, fillMode);
            hOldRegion = SelectClipRgn(hDC, hRgn);
            OffsetClipRgn(hDC, x1, y1);
            
            /* Stipple the bounding box. */
            StippleArea(display, hDC, gc, x1, y1, w, h);
            SelectClipRgn(hDC, hOldRegion), DeleteRgn(hRgn);
        }
        break;
        
    case FillTiled:
        {
            BITMAP bm;
            HBITMAP hOldBitmap;
            HDC hMemDC;
            TkWinDrawable *twdPtr;
            HRGN hRgn, hOldRegion;
            
            if (gc->tile == None) { 
                goto fillSolid;
            }
#ifdef notdef
            if ((GetDeviceCaps(hDC, RASTERCAPS) & RC_BITBLT) == 0) {
                goto fillSolid;
            }
#endif 
            /* Use the polygon as a clip path. */
            LPtoDP(hDC, winPointArr, numPoints);
            /* FIXME: Really need to combine regions. */
            hRgn = CreatePolygonRgn(winPointArr, numPoints, fillMode);
            hOldRegion = SelectClipRgn(hDC, hRgn);
            OffsetClipRgn(hDC, x1, y1);
            
            twdPtr = (TkWinDrawable *)gc->tile;
            /* The tiling routine needs to know the size of the bitmap */
            GetObject(twdPtr->bitmap.handle, sizeof(BITMAP), &bm);
            hMemDC = CreateCompatibleDC(hDC);
            hOldBitmap = SelectBitmap(hMemDC, twdPtr->bitmap.handle);
            TileArea(hDC, hMemDC, gc->ts_x_origin, gc->ts_y_origin, bm.bmWidth, 
                     bm.bmHeight, x1, y1, w, h);
            (void)SelectBitmap(hMemDC, hOldBitmap);
            SelectClipRgn(hDC, hOldRegion), DeleteRgn(hRgn);
            DeleteDC(hMemDC);
        }
        break; 

    case FillSolid:
        {
            HPEN hPen, hOldPen;
            HBRUSH hBrush, hOldBrush;
            
        fillSolid:
            hPen = GetStockObject(NULL_PEN);
            hOldPen = SelectPen(hDC, hPen);
            hBrush = CreateSolidBrush(gc->foreground);
            hOldBrush = SelectBrush(hDC, hBrush);
            SetPolyFillMode(hDC, fillMode);
            Polygon(hDC, winPointArr, numPoints);
            (void)SelectPen(hDC, hOldPen), DeletePen(hPen);
            (void)SelectBrush(hDC, hOldBrush), DeleteBrush(hBrush);
        }
        break;
    }
    if (winPointArr != staticPoints) {
        Blt_Free(winPointArr);
    }
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
    HFONT hOldFont;
    Tcl_Encoding encoding;
    TEXTMETRIC tm;

    hOldFont = SelectFont(hDC, hFont);
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
    (void)SelectFont(hDC, hOldFont);
}

void
Blt_TextOut(
    HDC hDC,
    GC gc,                      /* Graphics context for drawing
                                 * characters. */
    HFONT hFont,                /* Font in which characters will be drawn;
                                 * must be the same as font used in GC. */
    const char *text,           /* UTF-8 string to be displayed.  Need not
                                 * be '\0' terminated.  All Tk
                                 * meta-characters (tabs, control
                                 * characters, and newlines) should be
                                 * stripped out of the string that is
                                 * passed to this function.  If they are
                                 * not stripped out, they will be displayed
                                 * as regular printing characters. */
    int numBytes,                /* Number of bytes in string. */
    int x, int y)                /* Coordinates at which to place origin of
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
        HBRUSH hOldBrush, hBrush;
        HBITMAP hOldBitmap, hBitmap;
        HDC hMemDC;
        TEXTMETRIC tm;
        SIZE size;

        if (twdPtr->type != TWD_BITMAP) {
            Tcl_Panic("unexpected drawable type in stipple");
        }

        /*
         * Select stipple pattern into destination dc.
         */
        hMemDC = CreateCompatibleDC(hDC);
        hBrush = CreatePatternBrush(twdPtr->bitmap.handle);
        SetBrushOrgEx(hDC, gc->ts_x_origin, gc->ts_y_origin, NULL);
        hOldBrush = SelectBrush(hDC, hBrush);

        SetTextAlign(hMemDC, TA_LEFT | TA_BASELINE);
        SetTextColor(hMemDC, gc->foreground);
        SetBkMode(hMemDC, TRANSPARENT);
        SetBkColor(hMemDC, RGB(0, 0, 0));

        /*
         * Compute the bounding box and create a compatible bitmap.
         */
        GetTextExtentPoint(hMemDC, text, numBytes, &size);
        GetTextMetrics(hMemDC, &tm);
        size.cx -= tm.tmOverhang;
        hBitmap = CreateCompatibleBitmap(hDC, size.cx, size.cy);
        hOldBitmap = SelectBitmap(hMemDC, hBitmap);
        /*
         * The following code is tricky because fonts are rendered in
         * multiple colors.  First we draw onto a black background and copy
         * the white bits.  Then we draw onto a white background and copy
         * the black bits.  Both the foreground and background bits of the
         * font are ANDed with the stipple pattern as they are copied.
         */
        PatBlt(hMemDC, 0, 0, size.cx, size.cy, BLACKNESS);
        DrawTextOut(hDC, hFont, text, numBytes, x, y);
        BitBlt(hDC, x, y-tm.tmAscent, size.cx, size.cy, hMemDC, 0, 0, 0xEA02E9);
        PatBlt(hMemDC, 0, 0, size.cx, size.cy, WHITENESS);
        DrawTextOut(hDC, hFont, text, numBytes, x, y);
        BitBlt(hDC, x, y-tm.tmAscent, size.cx, size.cy, hMemDC, 0, 0, 0x8A0E06);

        /*
         * Destroy the temporary bitmap and restore the device context.
         */
        (void)SelectBrush(hMemDC, hOldBitmap);
        DeleteBitmap(hBitmap);
        DeleteDC(hMemDC);
        (void)SelectBrush(hDC, hOldBrush);
        DeleteBrush(hBrush);
    } else if (gc->function == GXcopy) {
        SetTextAlign(hDC, TA_LEFT | TA_BASELINE);
        SetTextColor(hDC, gc->foreground);
        SetBkMode(hDC, TRANSPARENT);
        DrawTextOut(hDC, hFont, text, numBytes, x, y);
    } else {
        HBITMAP hOldBitmap, hBitmap;
        HDC hMemDC;
        TEXTMETRIC tm;
        SIZE size;

        hMemDC = CreateCompatibleDC(hDC);
        SetTextAlign(hMemDC, TA_LEFT | TA_BASELINE);
        SetTextColor(hMemDC, gc->foreground);
        SetBkMode(hMemDC, TRANSPARENT);
        SetBkColor(hMemDC, RGB(0, 0, 0));

        /*
         * Compute the bounding box and create a compatible bitmap.
         */
        GetTextExtentPoint(hMemDC, text, numBytes, &size);
        GetTextMetrics(hMemDC, &tm);
        size.cx -= tm.tmOverhang;
        hBitmap = CreateCompatibleBitmap(hDC, size.cx, size.cy);
        hOldBitmap = SelectObject(hMemDC, hBitmap);

        DrawTextOut(hMemDC, hFont, text, numBytes, 0, tm.tmAscent);
        BitBlt(hDC, x, y - tm.tmAscent, size.cx, size.cy, hMemDC, 0, 0, 
                bltModes[gc->function]);
        (void)SelectBitmap(hMemDC, hOldBitmap);
        DeleteBitmap(hBitmap);
        DeleteDC(hMemDC);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EmulateXPolygonRegion --
 *
 *---------------------------------------------------------------------------
 */
Region
Blt_EmulateXPolygonRegion(XPoint xPointArr[], int numPoints, int rule)
{
    HRGN hRgn; 
    POINT *winPointArr;
    POINT staticPoints[64];
    int i;

    if (numPoints > 64) {
        winPointArr = Blt_Malloc(sizeof(POINT) * numPoints);
        if (winPointArr == NULL) {
            return NULL;
        }
    } else {
        winPointArr = staticPoints;
    }
    for (i = 0; i < numPoints; i++) {
        winPointArr[i].x = (int)xPointArr[i].x;
        winPointArr[i].y = (int)xPointArr[i].y;
    }
    if (rule == EvenOddRule) {
        hRgn = CreatePolygonRgn(winPointArr, numPoints, ALTERNATE);
    } else if (rule == WindingRule) {
        hRgn = CreatePolygonRgn(winPointArr, numPoints, WINDING);
    } else {
        hRgn = NULL;
    }
    if (hRgn == NULL) {
    }
    if (winPointArr != staticPoints) {
        Blt_Free(winPointArr);
    }
    return (Region)hRgn;
}



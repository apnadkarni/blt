/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTkInt.h --
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

#ifndef _BLT_TK_INT_H
#define _BLT_TK_INT_H

#define USE_COMPOSITELESS_PHOTO_PUT_BLOCK 
#include <tk.h>

#define RGB_ANTIQUEWHITE1       "#ffefdb"
#define RGB_BISQUE1             "#ffe4c4"
#define RGB_BISQUE2             "#eed5b7"
#define RGB_BISQUE3             "#cdb79e"
#define RGB_BLACK               "#000000"
#define RGB_BLUE                "#0000ff"
#define RGB_GREEN               "#00ff00"
#define RGB_GREY                "#b0b0b0"
#define RGB_GREY15              "#262626"
#define RGB_GREY20              "#333333"
#define RGB_GREY25              "#404040"
#define RGB_GREY30              "#4d4d4d"
#define RGB_GREY35              "#595959"
#define RGB_GREY40              "#666666"
#define RGB_GREY50              "#7f7f7f"
#define RGB_GREY64              "#a3a3a3"
#define RGB_GREY70              "#b3b3b3"
#define RGB_GREY75              "#bfbfbf"
#define RGB_GREY77              "#c3c3c3"
#define RGB_GREY82              "#d1d1d1"
#define RGB_GREY85              "#d9d9d9"
#define RGB_GREY90              "#e5e5e5"
#define RGB_GREY93              "#ececec"
#define RGB_GREY95              "#f2f2f2"
#define RGB_GREY97              "#f7f7f7"
#define RGB_LIGHTBLUE0          "#e4f7ff"
#define RGB_LIGHTBLUE00         "#D9F5FF"
#define RGB_LIGHTBLUE1          "#bfefff"
#define RGB_LIGHTBLUE2          "#b2dfee"
#define RGB_LIGHTSKYBLUE1       "#b0e2ff"
#define RGB_MAROON              "#b03060"
#define RGB_NAVYBLUE            "#000080"
#define RGB_PINK                "#ffc0cb"
#define RGB_BISQUE1             "#ffe4c4"
#define RGB_RED                 "#ff0000"
#define RGB_RED3                "#cd0000"
#define RGB_WHITE               "#ffffff"
#define RGB_YELLOW              "#ffff00"
#define RGB_SKYBLUE4            "#4a708b"

#ifdef OLD_TK_COLORS
#define STD_NORMAL_BACKGROUND   RGB_BISQUE1
#define STD_ACTIVE_BACKGROUND   RGB_BISQUE2
#define STD_SELECT_BACKGROUND   RGB_LIGHTBLUE1
#define STD_SELECT_FOREGROUND   RGB_BLACK
#else
#define STD_NORMAL_BACKGROUND   RGB_GREY85
#define STD_ACTIVE_BACKGROUND   RGB_GREY90
#define STD_SELECT_BACKGROUND   RGB_SKYBLUE4
#define STD_SELECT_FOREGROUND   RGB_WHITE
#endif /* OLD_TK_COLORS */

#define STD_ACTIVE_BG_MONO      RGB_BLACK
#define STD_ACTIVE_FOREGROUND   RGB_BLACK
#define STD_ACTIVE_FG_MONO      RGB_WHITE
#define STD_BORDERWIDTH         "2"
#define STD_FONT_HUGE           "{Sans Serif} 18"
#define STD_FONT_LARGE          "{Sans Serif} 14"
#define STD_FONT_MEDIUM         "{Sans Serif} 11"
#define STD_FONT_NORMAL         "{Sans Serif} 10"
#define STD_FONT_SMALL          "{Sans Serif} 9"
#define STD_FONT_NUMBERS        "Math 8"
#define STD_FONT                STD_FONT_NORMAL
#define STD_INDICATOR_COLOR     RGB_RED3
#define STD_NORMAL_BG_MONO      RGB_WHITE
#define STD_NORMAL_FOREGROUND   RGB_BLACK
#define STD_NORMAL_FG_MONO      RGB_BLACK
#define STD_SELECT_BG_MONO      RGB_BLACK
#define STD_SELECT_BORDERWIDTH  "2"
#define STD_SELECT_FG_MONO      RGB_WHITE
#define STD_SHADOW_MONO         RGB_BLACK
#define STD_SELECT_FONT_HUGE    "{Sans Serif} 18 Bold"
#define STD_SELECT_FONT_LARGE   "{Sans Serif} 14 Bold"
#define STD_SELECT_FONT_MEDIUM  "{Sans Serif} 11 Bold"
#define STD_SELECT_FONT_NORMAL  "{Sans Serif} 10 Bold"
#define STD_SELECT_FONT_SMALL   "{Sans Serif} 9 Bold"
#define STD_SELECT_FONT         STD_SELECT_FONT_NORMAL
#define STD_DISABLED_FOREGROUND RGB_GREY70
#define STD_DISABLED_BACKGROUND RGB_GREY90

#define PIXELS_NNEG             0
#define PIXELS_POS              1
#define PIXELS_ANY              2

BLT_EXTERN void Blt_Draw3DRectangle(Tk_Window tkwin, Drawable drawable,
        Tk_3DBorder border, int x, int y, int width, int height, 
        int borderWidth, int relief);
BLT_EXTERN void Blt_Fill3DRectangle(Tk_Window tkwin, Drawable drawable,
        Tk_3DBorder border, int x, int y, int width, int height, 
        int borderWidth, int relief);

#define BLT_SCROLL_MODE_CANVAS  (1<<0)
#define BLT_SCROLL_MODE_LISTBOX (1<<1)
#define BLT_SCROLL_MODE_HIERBOX (1<<2)

BLT_EXTERN int Blt_AdjustViewport (int offset, int worldSize, int windowSize, 
        int scrollUnits, int scrollMode);

BLT_EXTERN int Blt_GetScrollInfoFromObj (Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv, int *offsetPtr, int worldSize, int windowSize,
        int scrollUnits, int scrollMode);

BLT_EXTERN void Blt_UpdateScrollbar(Tcl_Interp *interp, 
        Tcl_Obj *scrollCmdObjPtr, int first, int last, int width);

#ifndef TK_RELIEF_SOLID
#  define TK_RELIEF_SOLID               TK_RELIEF_FLAT
#endif


/*--------------------------------------------------------------------------
 *
 * ColorPair --
 *
 *      Holds a pair of foreground, background colors.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    XColor *fgColor, *bgColor;
} ColorPair;

#define COLOR_NONE              (XColor *)0
#define COLOR_DEFAULT           (XColor *)1
#define COLOR_ALLOW_DEFAULTS    1

BLT_EXTERN void Blt_FreeColorPair (ColorPair *pairPtr);

#define ARROW_LEFT              (0)
#define ARROW_UP                (1)
#define ARROW_RIGHT             (2)
#define ARROW_DOWN              (3)
#define ARROW_OFFSET            4
#define STD_ARROW_HEIGHT        3
#define STD_ARROW_WIDTH         ((2 * (ARROW_OFFSET - 1)) + 1)

/*
 *---------------------------------------------------------------------------
 *
 *      X11/Xosdefs.h requires XNOSTDHDRS be set for some systems.  This is
 *      a guess.  If I can't find STDC headers or unistd.h, assume that
 *      this is non-POSIX and non-STDC environment.  (needed for Encore
 *      Umax 3.4 ?)
 *
 *---------------------------------------------------------------------------
 */
#if !defined(STDC_HEADERS) && !defined(HAVE_UNISTD_H)
#  define XNOSTDHDRS    1
#endif

BLT_EXTERN int Blt_LineRectClip(Region2d *regionPtr, Point2d *p, Point2d *q);

#define CLIP_OUTSIDE    0
#define CLIP_INSIDE     (1<<0)
#define CLIP_LEFT       (1<<1)
#define CLIP_RIGHT      (1<<2)

BLT_EXTERN GC Blt_GetPrivateGC(Tk_Window tkwin, unsigned long gcMask,
        XGCValues *valuePtr);

BLT_EXTERN GC Blt_GetPrivateGCFromDrawable(Display *display, Drawable drawable, 
        unsigned long gcMask, XGCValues *valuePtr);

BLT_EXTERN void Blt_FreePrivateGC(Display *display, GC gc);

BLT_EXTERN int Blt_GetWindowFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        Window *windowPtr);

BLT_EXTERN const char *Blt_GetWindowName(Display *display, Window window);

BLT_EXTERN Blt_Chain Blt_GetChildrenFromWindow(Display *display, Window window);

BLT_EXTERN Window Blt_GetParentWindow(Display *display, Window window);

BLT_EXTERN Tk_Window Blt_FindChild(Tk_Window parent, char *name);

BLT_EXTERN Tk_Window Blt_FirstChild(Tk_Window parent);

BLT_EXTERN Tk_Window Blt_NextChild(Tk_Window tkwin);

BLT_EXTERN void Blt_RelinkWindow (Tk_Window tkwin, Tk_Window newParent, int x, 
        int y);

BLT_EXTERN Tk_Window Blt_Toplevel(Tk_Window tkwin);

BLT_EXTERN int Blt_GetPixels(Tcl_Interp *interp, Tk_Window tkwin, 
        const char *string, int check, int *valuePtr);

BLT_EXTERN const char *Blt_NameOfFill(int fill);

BLT_EXTERN const char *Blt_NameOfResize(int resize);

BLT_EXTERN int Blt_GetXY (Tcl_Interp *interp, Tk_Window tkwin, 
        const char *string, int *xPtr, int *yPtr);

BLT_EXTERN Point2d Blt_GetProjection(int x, int y, Point2d *p, Point2d *q);
BLT_EXTERN Point2d Blt_GetProjection2(int x, int y, double x1, double y1,
                                      double x2, double y2);

BLT_EXTERN void Blt_DrawArrowOld(Display *display, Drawable drawable, GC gc, 
        int x, int y, int w, int h, int borderWidth, int orientation);

BLT_EXTERN void Blt_DrawArrow (Display *display, Drawable drawable, 
        XColor *color, int x, int y, int w, int h, 
        int borderWidth, int orientation);

BLT_EXTERN void Blt_MakeTransparentWindowExist (Tk_Window tkwin, Window parent, 
        int isBusy);

BLT_EXTERN void Blt_TranslateAnchor (int x, int y, int width, int height, 
        Tk_Anchor anchor, int *transXPtr, int *transYPtr);

BLT_EXTERN Point2d Blt_AnchorPoint (double x, double y, double width, 
        double height, Tk_Anchor anchor);

BLT_EXTERN long Blt_MaxRequestSize (Display *display, size_t elemSize);

BLT_EXTERN Window Blt_GetWindowId (Tk_Window tkwin);

BLT_EXTERN void Blt_InitXRandrConfig(Tcl_Interp *interp);
BLT_EXTERN void Blt_SizeOfScreen(Tk_Window tkwin, int *widthPtr,int *heightPtr);

BLT_EXTERN int Blt_RootX (Tk_Window tkwin);

BLT_EXTERN int Blt_RootY (Tk_Window tkwin);

BLT_EXTERN void Blt_RootCoordinates (Tk_Window tkwin, int x, int y, 
        int *rootXPtr, int *rootYPtr);

BLT_EXTERN void Blt_MapToplevelWindow(Tk_Window tkwin);

BLT_EXTERN void Blt_UnmapToplevelWindow(Tk_Window tkwin);

BLT_EXTERN void Blt_RaiseToplevelWindow(Tk_Window tkwin);

BLT_EXTERN void Blt_LowerToplevelWindow(Tk_Window tkwin);

BLT_EXTERN void Blt_ResizeToplevelWindow(Tk_Window tkwin, int w, int h);

BLT_EXTERN void Blt_MoveToplevelWindow(Tk_Window tkwin, int x, int y);

BLT_EXTERN void Blt_MoveResizeToplevelWindow(Tk_Window tkwin, int x, int y, 
        int w, int h);

BLT_EXTERN int Blt_GetWindowRegion(Display *display, Window window, int *xPtr,
        int *yPtr, int *widthPtr, int *heightPtr);

BLT_EXTERN ClientData Blt_GetWindowInstanceData (Tk_Window tkwin);

BLT_EXTERN void Blt_SetWindowInstanceData (Tk_Window tkwin, 
        ClientData instanceData);

BLT_EXTERN void Blt_DeleteWindowInstanceData (Tk_Window tkwin);

BLT_EXTERN int Blt_ReparentWindow (Display *display, Window window, 
        Window newParent, int x, int y);

extern void Blt_RegisterPictureImageType(Tcl_Interp *interp);
extern void Blt_RegisterEpsCanvasItem(void);


typedef struct {
    Drawable id;
    unsigned short int width, height;
    int depth;
    Colormap colormap;
    Visual *visual;
} Blt_DrawableAttributes;

BLT_EXTERN Blt_DrawableAttributes *Blt_GetDrawableAttribs(Display *display,
        Drawable drawable);

BLT_EXTERN void Blt_SetDrawableAttribs(Display *display, Drawable drawable,
        int width, int height, int depth, Colormap colormap, Visual *visual);

BLT_EXTERN void Blt_SetDrawableAttribsFromWindow(Tk_Window tkwin, 
        Drawable drawable);

BLT_EXTERN void Blt_FreeDrawableAttribs(Display *display, Drawable drawable);

BLT_EXTERN GC Blt_GetBitmapGC(Tk_Window tkwin);

#define Tk_RootWindow(tkwin)    \
        RootWindow(Tk_Display(tkwin),Tk_ScreenNumber(tkwin))

typedef struct _TkDisplay TkDisplay;    /* Opaque type */

BLT_EXTERN Pixmap Blt_GetPixmapAbortOnError(Display *dpy, Drawable draw, 
        int w, int h, int depth, int lineNum, const char *fileName);

#undef Blt_GetPixmap
#define Blt_GetPixmap(dpy, draw, w, h, depth) \
    Blt_GetPixmapAbortOnError(dpy, draw, w, h, depth, __LINE__, __FILE__)
#define Blt_FreePixmap(dpy, pix)        Tk_FreePixmap(dpy, pix)

#if defined(HAVE_LIBXRANDR) && defined(HAVE_X11_EXTENSIONS_RANDR_H) && defined(HAVE_X11_EXTENSIONS_XRANDR_H) 
#define HAVE_RANDR 1
#endif

BLT_EXTERN void Blt_ScreenDPI(Tk_Window tkwin, int *xPtr, int *yPtr);

#if defined (WIN32) || defined(MAC_TCL) || defined(MAC_OSX_TCL)
typedef struct _TkRegion *TkRegion;     /* Opaque type */

/* 114 */
extern TkRegion TkCreateRegion(void);
/* 115 */
extern void TkDestroyRegion (TkRegion rgn);
/* 116 */
extern void TkIntersectRegion (TkRegion sra, TkRegion srcb, TkRegion dr_return);
/* 117 */
extern int TkRectInRegion(TkRegion rgn, int x, int y, unsigned int width, 
                          unsigned int height);
/* 118 */
extern void TkSetRegion(Display* display, GC gc, TkRegion rgn);
/* 119 */
extern void TkUnionRectWithRegion(XRectangle* rect, TkRegion src, 
        TkRegion dr_return);
#else
typedef struct _TkRegion *TkRegion;     /* Opaque type */
#define TkClipBox(rgn, rect) XClipBox((Region) rgn, rect)
#define TkCreateRegion() (TkRegion) XCreateRegion()
#define TkDestroyRegion(rgn) XDestroyRegion((Region) rgn)
#define TkIntersectRegion(a, b, r) XIntersectRegion((Region) a, \
        (Region) b, (Region) r)
#define TkRectInRegion(r, x, y, w, h) XRectInRegion((Region) r, x, y, w, h)
#define TkSetRegion(d, gc, rgn) XSetRegion(d, gc, (Region) rgn)
#define TkSubtractRegion(a, b, r) XSubtractRegion((Region) a, \
        (Region) b, (Region) r)
#define TkUnionRectWithRegion(rect, src, ret) XUnionRectWithRegion(rect, \
        (Region) src, (Region) ret)
#endif 

BLT_EXTERN int Blt_OldConfigModified(Tk_ConfigSpec *specs, ...);

BLT_EXTERN void Blt_GetLineExtents(size_t numPoints, Point2d *points, 
        Region2d *r);

BLT_EXTERN void Blt_GetBoundingBox (int width, int height, float angle, 
        double *widthPtr, double *heightPtr, Point2d *points);

BLT_EXTERN int Blt_ParseTifTags(Tcl_Interp *interp, const char *varName,
        const unsigned char *bytes, off_t offset, size_t numBytes);


#include "bltConfig.h"

#endif /*BLT_TK_INT*/

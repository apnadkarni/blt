/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "bltTkInt.h"
#include "bltFont.h"
#include "bltPaintBrush.h"
#include "bltBg.h"
#include "bltBind.h"
#include "bltAfm.h"
#include "bltConfig.h"
#include "bltImage.h"
#include "bltPainter.h"
#include "bltPs.h"
#include "bltText.h"

/* !BEGIN!: Do not edit below this line. */

/*
 * Exported function declarations:
 */

/* Slot 0 is reserved */
#ifndef Blt_Draw3DRectangle_DECLARED
#define Blt_Draw3DRectangle_DECLARED
/* 1 */
BLT_EXTERN void		Blt_Draw3DRectangle(Tk_Window tkwin,
				Drawable drawable, Tk_3DBorder border, int x,
				int y, int width, int height,
				int borderWidth, int relief);
#endif
#ifndef Blt_Fill3DRectangle_DECLARED
#define Blt_Fill3DRectangle_DECLARED
/* 2 */
BLT_EXTERN void		Blt_Fill3DRectangle(Tk_Window tkwin,
				Drawable drawable, Tk_3DBorder border, int x,
				int y, int width, int height,
				int borderWidth, int relief);
#endif
#ifndef Blt_AdjustViewport_DECLARED
#define Blt_AdjustViewport_DECLARED
/* 3 */
BLT_EXTERN int		Blt_AdjustViewport(int offset, int worldSize,
				int windowSize, int scrollUnits,
				int scrollMode);
#endif
#ifndef Blt_GetScrollInfoFromObj_DECLARED
#define Blt_GetScrollInfoFromObj_DECLARED
/* 4 */
BLT_EXTERN int		Blt_GetScrollInfoFromObj(Tcl_Interp *interp,
				int objc, Tcl_Obj *const *objv,
				int *offsetPtr, int worldSize,
				int windowSize, int scrollUnits,
				int scrollMode);
#endif
#ifndef Blt_UpdateScrollbar_DECLARED
#define Blt_UpdateScrollbar_DECLARED
/* 5 */
BLT_EXTERN void		Blt_UpdateScrollbar(Tcl_Interp *interp,
				Tcl_Obj *scrollCmdObjPtr, int first,
				int last, int width);
#endif
#ifndef Blt_FreeColorPair_DECLARED
#define Blt_FreeColorPair_DECLARED
/* 6 */
BLT_EXTERN void		Blt_FreeColorPair(ColorPair *pairPtr);
#endif
#ifndef Blt_LineRectClip_DECLARED
#define Blt_LineRectClip_DECLARED
/* 7 */
BLT_EXTERN int		Blt_LineRectClip(Region2d *regionPtr, Point2d *p,
				Point2d *q);
#endif
#ifndef Blt_GetPrivateGC_DECLARED
#define Blt_GetPrivateGC_DECLARED
/* 8 */
BLT_EXTERN GC		Blt_GetPrivateGC(Tk_Window tkwin,
				unsigned long gcMask, XGCValues *valuePtr);
#endif
#ifndef Blt_GetPrivateGCFromDrawable_DECLARED
#define Blt_GetPrivateGCFromDrawable_DECLARED
/* 9 */
BLT_EXTERN GC		Blt_GetPrivateGCFromDrawable(Display *display,
				Drawable drawable, unsigned long gcMask,
				XGCValues *valuePtr);
#endif
#ifndef Blt_FreePrivateGC_DECLARED
#define Blt_FreePrivateGC_DECLARED
/* 10 */
BLT_EXTERN void		Blt_FreePrivateGC(Display *display, GC gc);
#endif
#ifndef Blt_GetWindowFromObj_DECLARED
#define Blt_GetWindowFromObj_DECLARED
/* 11 */
BLT_EXTERN int		Blt_GetWindowFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Window *windowPtr);
#endif
#ifndef Blt_GetWindowName_DECLARED
#define Blt_GetWindowName_DECLARED
/* 12 */
BLT_EXTERN const char *	 Blt_GetWindowName(Display *display, Window window);
#endif
#ifndef Blt_GetChildrenFromWindow_DECLARED
#define Blt_GetChildrenFromWindow_DECLARED
/* 13 */
BLT_EXTERN Blt_Chain	Blt_GetChildrenFromWindow(Display *display,
				Window window);
#endif
#ifndef Blt_GetParentWindow_DECLARED
#define Blt_GetParentWindow_DECLARED
/* 14 */
BLT_EXTERN Window	Blt_GetParentWindow(Display *display, Window window);
#endif
#ifndef Blt_FindChild_DECLARED
#define Blt_FindChild_DECLARED
/* 15 */
BLT_EXTERN Tk_Window	Blt_FindChild(Tk_Window parent, char *name);
#endif
#ifndef Blt_FirstChild_DECLARED
#define Blt_FirstChild_DECLARED
/* 16 */
BLT_EXTERN Tk_Window	Blt_FirstChild(Tk_Window parent);
#endif
#ifndef Blt_NextChild_DECLARED
#define Blt_NextChild_DECLARED
/* 17 */
BLT_EXTERN Tk_Window	Blt_NextChild(Tk_Window tkwin);
#endif
#ifndef Blt_RelinkWindow_DECLARED
#define Blt_RelinkWindow_DECLARED
/* 18 */
BLT_EXTERN void		Blt_RelinkWindow(Tk_Window tkwin,
				Tk_Window newParent, int x, int y);
#endif
#ifndef Blt_Toplevel_DECLARED
#define Blt_Toplevel_DECLARED
/* 19 */
BLT_EXTERN Tk_Window	Blt_Toplevel(Tk_Window tkwin);
#endif
#ifndef Blt_GetPixels_DECLARED
#define Blt_GetPixels_DECLARED
/* 20 */
BLT_EXTERN int		Blt_GetPixels(Tcl_Interp *interp, Tk_Window tkwin,
				const char *string, int check, int *valuePtr);
#endif
#ifndef Blt_NameOfFill_DECLARED
#define Blt_NameOfFill_DECLARED
/* 21 */
BLT_EXTERN const char *	 Blt_NameOfFill(int fill);
#endif
#ifndef Blt_NameOfResize_DECLARED
#define Blt_NameOfResize_DECLARED
/* 22 */
BLT_EXTERN const char *	 Blt_NameOfResize(int resize);
#endif
#ifndef Blt_GetXY_DECLARED
#define Blt_GetXY_DECLARED
/* 23 */
BLT_EXTERN int		Blt_GetXY(Tcl_Interp *interp, Tk_Window tkwin,
				const char *string, int *xPtr, int *yPtr);
#endif
#ifndef Blt_GetProjection_DECLARED
#define Blt_GetProjection_DECLARED
/* 24 */
BLT_EXTERN Point2d	Blt_GetProjection(int x, int y, Point2d *p,
				Point2d *q);
#endif
#ifndef Blt_GetProjection2_DECLARED
#define Blt_GetProjection2_DECLARED
/* 25 */
BLT_EXTERN Point2d	Blt_GetProjection2(int x, int y, double x1,
				double y1, double x2, double y2);
#endif
#ifndef Blt_DrawArrowOld_DECLARED
#define Blt_DrawArrowOld_DECLARED
/* 26 */
BLT_EXTERN void		Blt_DrawArrowOld(Display *display, Drawable drawable,
				GC gc, int x, int y, int w, int h,
				int borderWidth, int orientation);
#endif
#ifndef Blt_DrawArrow_DECLARED
#define Blt_DrawArrow_DECLARED
/* 27 */
BLT_EXTERN void		Blt_DrawArrow(Display *display, Drawable drawable,
				XColor *color, int x, int y, int w, int h,
				int borderWidth, int orientation);
#endif
#ifndef Blt_MakeTransparentWindowExist_DECLARED
#define Blt_MakeTransparentWindowExist_DECLARED
/* 28 */
BLT_EXTERN void		Blt_MakeTransparentWindowExist(Tk_Window tkwin,
				Window parent, int isBusy);
#endif
#ifndef Blt_TranslateAnchor_DECLARED
#define Blt_TranslateAnchor_DECLARED
/* 29 */
BLT_EXTERN void		Blt_TranslateAnchor(int x, int y, int width,
				int height, Tk_Anchor anchor, int *transXPtr,
				int *transYPtr);
#endif
#ifndef Blt_AnchorPoint_DECLARED
#define Blt_AnchorPoint_DECLARED
/* 30 */
BLT_EXTERN Point2d	Blt_AnchorPoint(double x, double y, double width,
				double height, Tk_Anchor anchor);
#endif
#ifndef Blt_MaxRequestSize_DECLARED
#define Blt_MaxRequestSize_DECLARED
/* 31 */
BLT_EXTERN long		Blt_MaxRequestSize(Display *display, size_t elemSize);
#endif
#ifndef Blt_GetWindowId_DECLARED
#define Blt_GetWindowId_DECLARED
/* 32 */
BLT_EXTERN Window	Blt_GetWindowId(Tk_Window tkwin);
#endif
#ifndef Blt_InitXRandrConfig_DECLARED
#define Blt_InitXRandrConfig_DECLARED
/* 33 */
BLT_EXTERN void		Blt_InitXRandrConfig(Tcl_Interp *interp);
#endif
#ifndef Blt_SizeOfScreen_DECLARED
#define Blt_SizeOfScreen_DECLARED
/* 34 */
BLT_EXTERN void		Blt_SizeOfScreen(Tk_Window tkwin, int *widthPtr,
				int *heightPtr);
#endif
#ifndef Blt_RootX_DECLARED
#define Blt_RootX_DECLARED
/* 35 */
BLT_EXTERN int		Blt_RootX(Tk_Window tkwin);
#endif
#ifndef Blt_RootY_DECLARED
#define Blt_RootY_DECLARED
/* 36 */
BLT_EXTERN int		Blt_RootY(Tk_Window tkwin);
#endif
#ifndef Blt_RootCoordinates_DECLARED
#define Blt_RootCoordinates_DECLARED
/* 37 */
BLT_EXTERN void		Blt_RootCoordinates(Tk_Window tkwin, int x, int y,
				int *rootXPtr, int *rootYPtr);
#endif
#ifndef Blt_MapToplevelWindow_DECLARED
#define Blt_MapToplevelWindow_DECLARED
/* 38 */
BLT_EXTERN void		Blt_MapToplevelWindow(Tk_Window tkwin);
#endif
#ifndef Blt_UnmapToplevelWindow_DECLARED
#define Blt_UnmapToplevelWindow_DECLARED
/* 39 */
BLT_EXTERN void		Blt_UnmapToplevelWindow(Tk_Window tkwin);
#endif
#ifndef Blt_RaiseToplevelWindow_DECLARED
#define Blt_RaiseToplevelWindow_DECLARED
/* 40 */
BLT_EXTERN void		Blt_RaiseToplevelWindow(Tk_Window tkwin);
#endif
#ifndef Blt_LowerToplevelWindow_DECLARED
#define Blt_LowerToplevelWindow_DECLARED
/* 41 */
BLT_EXTERN void		Blt_LowerToplevelWindow(Tk_Window tkwin);
#endif
#ifndef Blt_ResizeToplevelWindow_DECLARED
#define Blt_ResizeToplevelWindow_DECLARED
/* 42 */
BLT_EXTERN void		Blt_ResizeToplevelWindow(Tk_Window tkwin, int w,
				int h);
#endif
#ifndef Blt_MoveToplevelWindow_DECLARED
#define Blt_MoveToplevelWindow_DECLARED
/* 43 */
BLT_EXTERN void		Blt_MoveToplevelWindow(Tk_Window tkwin, int x, int y);
#endif
#ifndef Blt_MoveResizeToplevelWindow_DECLARED
#define Blt_MoveResizeToplevelWindow_DECLARED
/* 44 */
BLT_EXTERN void		Blt_MoveResizeToplevelWindow(Tk_Window tkwin, int x,
				int y, int w, int h);
#endif
#ifndef Blt_GetWindowRegion_DECLARED
#define Blt_GetWindowRegion_DECLARED
/* 45 */
BLT_EXTERN int		Blt_GetWindowRegion(Display *display, Window window,
				int *xPtr, int *yPtr, int *widthPtr,
				int *heightPtr);
#endif
#ifndef Blt_GetRootWindowSize_DECLARED
#define Blt_GetRootWindowSize_DECLARED
/* 46 */
BLT_EXTERN int		Blt_GetRootWindowSize(Display *display,
				Window window, unsigned int *widthPtr,
				unsigned int *heightPtr);
#endif
#ifndef Blt_GetWindowInstanceData_DECLARED
#define Blt_GetWindowInstanceData_DECLARED
/* 47 */
BLT_EXTERN ClientData	Blt_GetWindowInstanceData(Tk_Window tkwin);
#endif
#ifndef Blt_SetWindowInstanceData_DECLARED
#define Blt_SetWindowInstanceData_DECLARED
/* 48 */
BLT_EXTERN void		Blt_SetWindowInstanceData(Tk_Window tkwin,
				ClientData instanceData);
#endif
#ifndef Blt_DeleteWindowInstanceData_DECLARED
#define Blt_DeleteWindowInstanceData_DECLARED
/* 49 */
BLT_EXTERN void		Blt_DeleteWindowInstanceData(Tk_Window tkwin);
#endif
#ifndef Blt_ReparentWindow_DECLARED
#define Blt_ReparentWindow_DECLARED
/* 50 */
BLT_EXTERN int		Blt_ReparentWindow(Display *display, Window window,
				Window newParent, int x, int y);
#endif
#ifndef Blt_GetDrawableAttributes_DECLARED
#define Blt_GetDrawableAttributes_DECLARED
/* 51 */
BLT_EXTERN Blt_DrawableAttributes * Blt_GetDrawableAttributes(
				Display *display, Drawable drawable);
#endif
#ifndef Blt_SetDrawableAttributes_DECLARED
#define Blt_SetDrawableAttributes_DECLARED
/* 52 */
BLT_EXTERN void		Blt_SetDrawableAttributes(Display *display,
				Drawable drawable, int width, int height,
				int depth, Colormap colormap, Visual *visual);
#endif
#ifndef Blt_SetDrawableAttributesFromWindow_DECLARED
#define Blt_SetDrawableAttributesFromWindow_DECLARED
/* 53 */
BLT_EXTERN void		Blt_SetDrawableAttributesFromWindow(Tk_Window tkwin,
				Drawable drawable);
#endif
#ifndef Blt_FreeDrawableAttributes_DECLARED
#define Blt_FreeDrawableAttributes_DECLARED
/* 54 */
BLT_EXTERN void		Blt_FreeDrawableAttributes(Display *display,
				Drawable drawable);
#endif
#ifndef Blt_GetBitmapGC_DECLARED
#define Blt_GetBitmapGC_DECLARED
/* 55 */
BLT_EXTERN GC		Blt_GetBitmapGC(Tk_Window tkwin);
#endif
#ifndef Blt_GetPixmapAbortOnError_DECLARED
#define Blt_GetPixmapAbortOnError_DECLARED
/* 56 */
BLT_EXTERN Pixmap	Blt_GetPixmapAbortOnError(Display *dpy,
				Drawable draw, int w, int h, int depth,
				int lineNum, const char *fileName);
#endif
#ifndef Blt_ScreenDPI_DECLARED
#define Blt_ScreenDPI_DECLARED
/* 57 */
BLT_EXTERN void		Blt_ScreenDPI(Tk_Window tkwin, int *xPtr, int *yPtr);
#endif
#ifndef Blt_OldConfigModified_DECLARED
#define Blt_OldConfigModified_DECLARED
/* 58 */
BLT_EXTERN int		Blt_OldConfigModified(Tk_ConfigSpec *specs, ...);
#endif
#ifndef Blt_GetLineExtents_DECLARED
#define Blt_GetLineExtents_DECLARED
/* 59 */
BLT_EXTERN void		Blt_GetLineExtents(size_t numPoints, Point2d *points,
				Region2d *r);
#endif
#ifndef Blt_GetBoundingBox_DECLARED
#define Blt_GetBoundingBox_DECLARED
/* 60 */
BLT_EXTERN void		Blt_GetBoundingBox(int width, int height,
				float angle, double *widthPtr,
				double *heightPtr, Point2d *points);
#endif
#ifndef Blt_ParseTifTags_DECLARED
#define Blt_ParseTifTags_DECLARED
/* 61 */
BLT_EXTERN int		Blt_ParseTifTags(Tcl_Interp *interp,
				const char *varName,
				const unsigned char *bytes, off_t offset,
				size_t numBytes);
#endif
#ifndef Blt_GetFont_DECLARED
#define Blt_GetFont_DECLARED
/* 62 */
BLT_EXTERN Blt_Font	Blt_GetFont(Tcl_Interp *interp, Tk_Window tkwin,
				const char *string);
#endif
#ifndef Blt_AllocFontFromObj_DECLARED
#define Blt_AllocFontFromObj_DECLARED
/* 63 */
BLT_EXTERN Blt_Font	Blt_AllocFontFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Tcl_Obj *objPtr);
#endif
#ifndef Blt_DrawWithEllipsis_DECLARED
#define Blt_DrawWithEllipsis_DECLARED
/* 64 */
BLT_EXTERN void		Blt_DrawWithEllipsis(Tk_Window tkwin,
				Drawable drawable, GC gc, Blt_Font font,
				int depth, float angle, const char *string,
				int numBytes, int x, int y, int maxLength);
#endif
#ifndef Blt_GetFontFromObj_DECLARED
#define Blt_GetFontFromObj_DECLARED
/* 65 */
BLT_EXTERN Blt_Font	Blt_GetFontFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Tcl_Obj *objPtr);
#endif
#ifndef Blt_Font_GetMetrics_DECLARED
#define Blt_Font_GetMetrics_DECLARED
/* 66 */
BLT_EXTERN void		Blt_Font_GetMetrics(Blt_Font font,
				Blt_FontMetrics *fmPtr);
#endif
#ifndef Blt_TextWidth_DECLARED
#define Blt_TextWidth_DECLARED
/* 67 */
BLT_EXTERN int		Blt_TextWidth(Blt_Font font, const char *string,
				int length);
#endif
#ifndef Blt_Font_GetInterp_DECLARED
#define Blt_Font_GetInterp_DECLARED
/* 68 */
BLT_EXTERN Tcl_Interp *	 Blt_Font_GetInterp(Blt_Font font);
#endif
#ifndef Blt_Font_GetFile_DECLARED
#define Blt_Font_GetFile_DECLARED
/* 69 */
BLT_EXTERN Tcl_Obj *	Blt_Font_GetFile(Tcl_Interp *interp, Tcl_Obj *objPtr,
				double *sizePtr);
#endif
#ifndef Blt_NewTileBrush_DECLARED
#define Blt_NewTileBrush_DECLARED
/* 70 */
BLT_EXTERN Blt_PaintBrush Blt_NewTileBrush(void );
#endif
#ifndef Blt_NewLinearGradientBrush_DECLARED
#define Blt_NewLinearGradientBrush_DECLARED
/* 71 */
BLT_EXTERN Blt_PaintBrush Blt_NewLinearGradientBrush(void );
#endif
#ifndef Blt_NewStripesBrush_DECLARED
#define Blt_NewStripesBrush_DECLARED
/* 72 */
BLT_EXTERN Blt_PaintBrush Blt_NewStripesBrush(void );
#endif
#ifndef Blt_NewCheckersBrush_DECLARED
#define Blt_NewCheckersBrush_DECLARED
/* 73 */
BLT_EXTERN Blt_PaintBrush Blt_NewCheckersBrush(void );
#endif
#ifndef Blt_NewRadialGradientBrush_DECLARED
#define Blt_NewRadialGradientBrush_DECLARED
/* 74 */
BLT_EXTERN Blt_PaintBrush Blt_NewRadialGradientBrush(void );
#endif
#ifndef Blt_NewConicalGradientBrush_DECLARED
#define Blt_NewConicalGradientBrush_DECLARED
/* 75 */
BLT_EXTERN Blt_PaintBrush Blt_NewConicalGradientBrush(void );
#endif
#ifndef Blt_NewColorBrush_DECLARED
#define Blt_NewColorBrush_DECLARED
/* 76 */
BLT_EXTERN Blt_PaintBrush Blt_NewColorBrush(unsigned int color);
#endif
#ifndef Blt_GetBrushType_DECLARED
#define Blt_GetBrushType_DECLARED
/* 77 */
BLT_EXTERN const char *	 Blt_GetBrushType(Blt_PaintBrush brush);
#endif
#ifndef Blt_GetBrushName_DECLARED
#define Blt_GetBrushName_DECLARED
/* 78 */
BLT_EXTERN const char *	 Blt_GetBrushName(Blt_PaintBrush brush);
#endif
#ifndef Blt_GetBrushColor_DECLARED
#define Blt_GetBrushColor_DECLARED
/* 79 */
BLT_EXTERN const char *	 Blt_GetBrushColor(Blt_PaintBrush brush);
#endif
#ifndef Blt_ConfigurePaintBrush_DECLARED
#define Blt_ConfigurePaintBrush_DECLARED
/* 80 */
BLT_EXTERN int		Blt_ConfigurePaintBrush(Tcl_Interp *interp,
				Blt_PaintBrush brush);
#endif
#ifndef Blt_GetBrushTypeFromObj_DECLARED
#define Blt_GetBrushTypeFromObj_DECLARED
/* 81 */
BLT_EXTERN int		Blt_GetBrushTypeFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_PaintBrushType *typePtr);
#endif
#ifndef Blt_FreeBrush_DECLARED
#define Blt_FreeBrush_DECLARED
/* 82 */
BLT_EXTERN void		Blt_FreeBrush(Blt_PaintBrush brush);
#endif
#ifndef Blt_GetPaintBrushFromObj_DECLARED
#define Blt_GetPaintBrushFromObj_DECLARED
/* 83 */
BLT_EXTERN int		Blt_GetPaintBrushFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_PaintBrush *brushPtr);
#endif
#ifndef Blt_GetPaintBrush_DECLARED
#define Blt_GetPaintBrush_DECLARED
/* 84 */
BLT_EXTERN int		Blt_GetPaintBrush(Tcl_Interp *interp,
				const char *string, Blt_PaintBrush *brushPtr);
#endif
#ifndef Blt_SetLinearGradientBrushPalette_DECLARED
#define Blt_SetLinearGradientBrushPalette_DECLARED
/* 85 */
BLT_EXTERN void		Blt_SetLinearGradientBrushPalette(
				Blt_PaintBrush brush, Blt_Palette palette);
#endif
#ifndef Blt_SetLinearGradientBrushCalcProc_DECLARED
#define Blt_SetLinearGradientBrushCalcProc_DECLARED
/* 86 */
BLT_EXTERN void		Blt_SetLinearGradientBrushCalcProc(
				Blt_PaintBrush brush,
				Blt_PaintBrushCalcProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_SetLinearGradientBrushColors_DECLARED
#define Blt_SetLinearGradientBrushColors_DECLARED
/* 87 */
BLT_EXTERN void		Blt_SetLinearGradientBrushColors(
				Blt_PaintBrush brush, Blt_Pixel *lowPtr,
				Blt_Pixel *highPtr);
#endif
#ifndef Blt_SetTileBrushPicture_DECLARED
#define Blt_SetTileBrushPicture_DECLARED
/* 88 */
BLT_EXTERN void		Blt_SetTileBrushPicture(Blt_PaintBrush brush,
				Blt_Picture tile);
#endif
#ifndef Blt_SetColorBrushColor_DECLARED
#define Blt_SetColorBrushColor_DECLARED
/* 89 */
BLT_EXTERN void		Blt_SetColorBrushColor(Blt_PaintBrush brush,
				unsigned int value);
#endif
#ifndef Blt_SetBrushOrigin_DECLARED
#define Blt_SetBrushOrigin_DECLARED
/* 90 */
BLT_EXTERN void		Blt_SetBrushOrigin(Blt_PaintBrush brush, int x,
				int y);
#endif
#ifndef Blt_SetBrushOpacity_DECLARED
#define Blt_SetBrushOpacity_DECLARED
/* 91 */
BLT_EXTERN void		Blt_SetBrushOpacity(Blt_PaintBrush brush,
				double percent);
#endif
#ifndef Blt_SetBrushRegion_DECLARED
#define Blt_SetBrushRegion_DECLARED
/* 92 */
BLT_EXTERN void		Blt_SetBrushRegion(Blt_PaintBrush brush, int x,
				int y, int w, int h);
#endif
#ifndef Blt_GetBrushAlpha_DECLARED
#define Blt_GetBrushAlpha_DECLARED
/* 93 */
BLT_EXTERN int		Blt_GetBrushAlpha(Blt_PaintBrush brush);
#endif
#ifndef Blt_GetBrushOrigin_DECLARED
#define Blt_GetBrushOrigin_DECLARED
/* 94 */
BLT_EXTERN void		Blt_GetBrushOrigin(Blt_PaintBrush brush, int *xPtr,
				int *yPtr);
#endif
#ifndef Blt_GetAssociatedColorFromBrush_DECLARED
#define Blt_GetAssociatedColorFromBrush_DECLARED
/* 95 */
BLT_EXTERN int		Blt_GetAssociatedColorFromBrush(Blt_PaintBrush brush,
				int x, int y);
#endif
#ifndef Blt_IsVerticalLinearBrush_DECLARED
#define Blt_IsVerticalLinearBrush_DECLARED
/* 96 */
BLT_EXTERN int		Blt_IsVerticalLinearBrush(Blt_PaintBrush brush);
#endif
#ifndef Blt_PaintRectangle_DECLARED
#define Blt_PaintRectangle_DECLARED
/* 97 */
BLT_EXTERN void		Blt_PaintRectangle(Blt_Picture picture, int x, int y,
				int w, int h, int dx, int dy,
				Blt_PaintBrush brush, int composite);
#endif
#ifndef Blt_PaintPolygon_DECLARED
#define Blt_PaintPolygon_DECLARED
/* 98 */
BLT_EXTERN void		Blt_PaintPolygon(Blt_Picture picture, int n,
				Point2f *vertices, Blt_PaintBrush brush);
#endif
#ifndef Blt_CreateBrushNotifier_DECLARED
#define Blt_CreateBrushNotifier_DECLARED
/* 99 */
BLT_EXTERN void		Blt_CreateBrushNotifier(Blt_PaintBrush brush,
				Blt_BrushChangedProc *notifyProc,
				ClientData clientData);
#endif
#ifndef Blt_DeleteBrushNotifier_DECLARED
#define Blt_DeleteBrushNotifier_DECLARED
/* 100 */
BLT_EXTERN void		Blt_DeleteBrushNotifier(Blt_PaintBrush brush,
				Blt_BrushChangedProc *notifyProc,
				ClientData clientData);
#endif
#ifndef Blt_GetBg_DECLARED
#define Blt_GetBg_DECLARED
/* 101 */
BLT_EXTERN int		Blt_GetBg(Tcl_Interp *interp, Tk_Window tkwin,
				const char *bgName, Blt_Bg *bgPtr);
#endif
#ifndef Blt_GetBgFromObj_DECLARED
#define Blt_GetBgFromObj_DECLARED
/* 102 */
BLT_EXTERN int		Blt_GetBgFromObj(Tcl_Interp *interp, Tk_Window tkwin,
				Tcl_Obj *objPtr, Blt_Bg *bgPtr);
#endif
#ifndef Blt_Bg_BorderColor_DECLARED
#define Blt_Bg_BorderColor_DECLARED
/* 103 */
BLT_EXTERN XColor *	Blt_Bg_BorderColor(Blt_Bg bg);
#endif
#ifndef Blt_Bg_Border_DECLARED
#define Blt_Bg_Border_DECLARED
/* 104 */
BLT_EXTERN Tk_3DBorder	Blt_Bg_Border(Blt_Bg bg);
#endif
#ifndef Blt_Bg_PaintBrush_DECLARED
#define Blt_Bg_PaintBrush_DECLARED
/* 105 */
BLT_EXTERN Blt_PaintBrush Blt_Bg_PaintBrush(Blt_Bg bg);
#endif
#ifndef Blt_Bg_Name_DECLARED
#define Blt_Bg_Name_DECLARED
/* 106 */
BLT_EXTERN const char *	 Blt_Bg_Name(Blt_Bg bg);
#endif
#ifndef Blt_Bg_Free_DECLARED
#define Blt_Bg_Free_DECLARED
/* 107 */
BLT_EXTERN void		Blt_Bg_Free(Blt_Bg bg);
#endif
#ifndef Blt_Bg_DrawRectangle_DECLARED
#define Blt_Bg_DrawRectangle_DECLARED
/* 108 */
BLT_EXTERN void		Blt_Bg_DrawRectangle(Tk_Window tkwin,
				Drawable drawable, Blt_Bg bg, int x, int y,
				int width, int height, int borderWidth,
				int relief);
#endif
#ifndef Blt_Bg_FillRectangle_DECLARED
#define Blt_Bg_FillRectangle_DECLARED
/* 109 */
BLT_EXTERN void		Blt_Bg_FillRectangle(Tk_Window tkwin,
				Drawable drawable, Blt_Bg bg, int x, int y,
				int width, int height, int borderWidth,
				int relief);
#endif
#ifndef Blt_Bg_DrawPolygon_DECLARED
#define Blt_Bg_DrawPolygon_DECLARED
/* 110 */
BLT_EXTERN void		Blt_Bg_DrawPolygon(Tk_Window tkwin,
				Drawable drawable, Blt_Bg bg, XPoint *points,
				int numPoints, int borderWidth,
				int leftRelief);
#endif
#ifndef Blt_Bg_FillPolygon_DECLARED
#define Blt_Bg_FillPolygon_DECLARED
/* 111 */
BLT_EXTERN void		Blt_Bg_FillPolygon(Tk_Window tkwin,
				Drawable drawable, Blt_Bg bg, XPoint *points,
				int numPoints, int borderWidth,
				int leftRelief);
#endif
#ifndef Blt_Bg_GetOrigin_DECLARED
#define Blt_Bg_GetOrigin_DECLARED
/* 112 */
BLT_EXTERN void		Blt_Bg_GetOrigin(Blt_Bg bg, int *xPtr, int *yPtr);
#endif
#ifndef Blt_Bg_SetChangedProc_DECLARED
#define Blt_Bg_SetChangedProc_DECLARED
/* 113 */
BLT_EXTERN void		Blt_Bg_SetChangedProc(Blt_Bg bg,
				Blt_BackgroundChangedProc *notifyProc,
				ClientData clientData);
#endif
#ifndef Blt_Bg_SetOrigin_DECLARED
#define Blt_Bg_SetOrigin_DECLARED
/* 114 */
BLT_EXTERN void		Blt_Bg_SetOrigin(Tk_Window tkwin, Blt_Bg bg, int x,
				int y);
#endif
#ifndef Blt_Bg_DrawFocus_DECLARED
#define Blt_Bg_DrawFocus_DECLARED
/* 115 */
BLT_EXTERN void		Blt_Bg_DrawFocus(Tk_Window tkwin, Blt_Bg bg,
				int highlightWidth, Drawable drawable);
#endif
#ifndef Blt_Bg_BorderGC_DECLARED
#define Blt_Bg_BorderGC_DECLARED
/* 116 */
BLT_EXTERN GC		Blt_Bg_BorderGC(Tk_Window tkwin, Blt_Bg bg,
				int which);
#endif
#ifndef Blt_Bg_SetFromBackground_DECLARED
#define Blt_Bg_SetFromBackground_DECLARED
/* 117 */
BLT_EXTERN void		Blt_Bg_SetFromBackground(Tk_Window tkwin, Blt_Bg bg);
#endif
#ifndef Blt_Bg_UnsetClipRegion_DECLARED
#define Blt_Bg_UnsetClipRegion_DECLARED
/* 118 */
BLT_EXTERN void		Blt_Bg_UnsetClipRegion(Tk_Window tkwin, Blt_Bg bg);
#endif
#ifndef Blt_Bg_SetClipRegion_DECLARED
#define Blt_Bg_SetClipRegion_DECLARED
/* 119 */
BLT_EXTERN void		Blt_Bg_SetClipRegion(Tk_Window tkwin, Blt_Bg bg,
				TkRegion rgn);
#endif
#ifndef Blt_Bg_GetColor_DECLARED
#define Blt_Bg_GetColor_DECLARED
/* 120 */
BLT_EXTERN unsigned int	 Blt_Bg_GetColor(Blt_Bg bg);
#endif
#ifndef Blt_3DBorder_SetClipRegion_DECLARED
#define Blt_3DBorder_SetClipRegion_DECLARED
/* 121 */
BLT_EXTERN void		Blt_3DBorder_SetClipRegion(Tk_Window tkwin,
				Tk_3DBorder border, TkRegion rgn);
#endif
#ifndef Blt_3DBorder_UnsetClipRegion_DECLARED
#define Blt_3DBorder_UnsetClipRegion_DECLARED
/* 122 */
BLT_EXTERN void		Blt_3DBorder_UnsetClipRegion(Tk_Window tkwin,
				Tk_3DBorder border);
#endif
#ifndef Blt_DestroyBindingTable_DECLARED
#define Blt_DestroyBindingTable_DECLARED
/* 123 */
BLT_EXTERN void		Blt_DestroyBindingTable(Blt_BindTable table);
#endif
#ifndef Blt_CreateBindingTable_DECLARED
#define Blt_CreateBindingTable_DECLARED
/* 124 */
BLT_EXTERN Blt_BindTable Blt_CreateBindingTable(Tcl_Interp *interp,
				Tk_Window tkwin, ClientData clientData,
				Blt_BindPickProc *pickProc,
				Blt_BindAppendTagsProc *tagProc);
#endif
#ifndef Blt_ConfigureBindings_DECLARED
#define Blt_ConfigureBindings_DECLARED
/* 125 */
BLT_EXTERN int		Blt_ConfigureBindings(Tcl_Interp *interp,
				Blt_BindTable table, ClientData item,
				int argc, const char **argv);
#endif
#ifndef Blt_ConfigureBindingsFromObj_DECLARED
#define Blt_ConfigureBindingsFromObj_DECLARED
/* 126 */
BLT_EXTERN int		Blt_ConfigureBindingsFromObj(Tcl_Interp *interp,
				Blt_BindTable table, ClientData item,
				int objc, Tcl_Obj *const *objv);
#endif
#ifndef Blt_PickCurrentItem_DECLARED
#define Blt_PickCurrentItem_DECLARED
/* 127 */
BLT_EXTERN void		Blt_PickCurrentItem(Blt_BindTable table);
#endif
#ifndef Blt_DeleteBindings_DECLARED
#define Blt_DeleteBindings_DECLARED
/* 128 */
BLT_EXTERN void		Blt_DeleteBindings(Blt_BindTable table,
				ClientData object);
#endif
#ifndef Blt_MoveBindingTable_DECLARED
#define Blt_MoveBindingTable_DECLARED
/* 129 */
BLT_EXTERN void		Blt_MoveBindingTable(Blt_BindTable table,
				Tk_Window tkwin);
#endif
#ifndef Blt_Afm_SetPrinting_DECLARED
#define Blt_Afm_SetPrinting_DECLARED
/* 130 */
BLT_EXTERN void		Blt_Afm_SetPrinting(Tcl_Interp *interp, int value);
#endif
#ifndef Blt_Afm_IsPrinting_DECLARED
#define Blt_Afm_IsPrinting_DECLARED
/* 131 */
BLT_EXTERN int		Blt_Afm_IsPrinting(void );
#endif
#ifndef Blt_Afm_TextWidth_DECLARED
#define Blt_Afm_TextWidth_DECLARED
/* 132 */
BLT_EXTERN int		Blt_Afm_TextWidth(Blt_Font font, const char *s,
				int numBytes);
#endif
#ifndef Blt_Afm_GetMetrics_DECLARED
#define Blt_Afm_GetMetrics_DECLARED
/* 133 */
BLT_EXTERN int		Blt_Afm_GetMetrics(Blt_Font font,
				Blt_FontMetrics *fmPtr);
#endif
#ifndef Blt_Afm_GetPostscriptFamily_DECLARED
#define Blt_Afm_GetPostscriptFamily_DECLARED
/* 134 */
BLT_EXTERN const char *	 Blt_Afm_GetPostscriptFamily(const char *family);
#endif
#ifndef Blt_Afm_GetPostscriptName_DECLARED
#define Blt_Afm_GetPostscriptName_DECLARED
/* 135 */
BLT_EXTERN void		Blt_Afm_GetPostscriptName(const char *family,
				int flags, Tcl_DString *resultPtr);
#endif
#ifndef Blt_SetDashes_DECLARED
#define Blt_SetDashes_DECLARED
/* 136 */
BLT_EXTERN void		Blt_SetDashes(Display *display, GC gc,
				Blt_Dashes *dashesPtr);
#endif
#ifndef Blt_ResetLimits_DECLARED
#define Blt_ResetLimits_DECLARED
/* 137 */
BLT_EXTERN void		Blt_ResetLimits(Blt_Limits *limitsPtr);
#endif
#ifndef Blt_GetLimitsFromObj_DECLARED
#define Blt_GetLimitsFromObj_DECLARED
/* 138 */
BLT_EXTERN int		Blt_GetLimitsFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Tcl_Obj *objPtr,
				Blt_Limits *limitsPtr);
#endif
#ifndef Blt_ConfigureInfoFromObj_DECLARED
#define Blt_ConfigureInfoFromObj_DECLARED
/* 139 */
BLT_EXTERN int		Blt_ConfigureInfoFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Blt_ConfigSpec *specs,
				char *widgRec, Tcl_Obj *objPtr, int flags);
#endif
#ifndef Blt_ConfigureValueFromObj_DECLARED
#define Blt_ConfigureValueFromObj_DECLARED
/* 140 */
BLT_EXTERN int		Blt_ConfigureValueFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Blt_ConfigSpec *specs,
				char *widgRec, Tcl_Obj *objPtr, int flags);
#endif
#ifndef Blt_ConfigureWidgetFromObj_DECLARED
#define Blt_ConfigureWidgetFromObj_DECLARED
/* 141 */
BLT_EXTERN int		Blt_ConfigureWidgetFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Blt_ConfigSpec *specs,
				int objc, Tcl_Obj *const *objv,
				char *widgRec, int flags);
#endif
#ifndef Blt_ConfigureComponentFromObj_DECLARED
#define Blt_ConfigureComponentFromObj_DECLARED
/* 142 */
BLT_EXTERN int		Blt_ConfigureComponentFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, const char *name,
				const char *className, Blt_ConfigSpec *specs,
				int objc, Tcl_Obj *const *objv,
				char *widgRec, int flags);
#endif
#ifndef Blt_ConfigModified_DECLARED
#define Blt_ConfigModified_DECLARED
/* 143 */
BLT_EXTERN int		Blt_ConfigModified(Blt_ConfigSpec *specs, ...);
#endif
#ifndef Blt_NameOfState_DECLARED
#define Blt_NameOfState_DECLARED
/* 144 */
BLT_EXTERN const char *	 Blt_NameOfState(int state);
#endif
#ifndef Blt_FreeOptions_DECLARED
#define Blt_FreeOptions_DECLARED
/* 145 */
BLT_EXTERN void		Blt_FreeOptions(Blt_ConfigSpec *specs, char *widgRec,
				Display *display, int needFlags);
#endif
#ifndef Blt_ObjIsOption_DECLARED
#define Blt_ObjIsOption_DECLARED
/* 146 */
BLT_EXTERN int		Blt_ObjIsOption(Blt_ConfigSpec *specs,
				Tcl_Obj *objPtr, int flags);
#endif
#ifndef Blt_GetPixelsFromObj_DECLARED
#define Blt_GetPixelsFromObj_DECLARED
/* 147 */
BLT_EXTERN int		Blt_GetPixelsFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Tcl_Obj *objPtr, int flags,
				int *valuePtr);
#endif
#ifndef Blt_GetPadFromObj_DECLARED
#define Blt_GetPadFromObj_DECLARED
/* 148 */
BLT_EXTERN int		Blt_GetPadFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Tcl_Obj *objPtr,
				Blt_Pad *padPtr);
#endif
#ifndef Blt_GetStateFromObj_DECLARED
#define Blt_GetStateFromObj_DECLARED
/* 149 */
BLT_EXTERN int		Blt_GetStateFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *statePtr);
#endif
#ifndef Blt_GetFillFromObj_DECLARED
#define Blt_GetFillFromObj_DECLARED
/* 150 */
BLT_EXTERN int		Blt_GetFillFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *fillPtr);
#endif
#ifndef Blt_GetResizeFromObj_DECLARED
#define Blt_GetResizeFromObj_DECLARED
/* 151 */
BLT_EXTERN int		Blt_GetResizeFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *fillPtr);
#endif
#ifndef Blt_GetDashesFromObj_DECLARED
#define Blt_GetDashesFromObj_DECLARED
/* 152 */
BLT_EXTERN int		Blt_GetDashesFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_Dashes *dashesPtr);
#endif
#ifndef Blt_Image_IsDeleted_DECLARED
#define Blt_Image_IsDeleted_DECLARED
/* 153 */
BLT_EXTERN int		Blt_Image_IsDeleted(Tk_Image tkImage);
#endif
#ifndef Blt_Image_GetMaster_DECLARED
#define Blt_Image_GetMaster_DECLARED
/* 154 */
BLT_EXTERN Tk_ImageMaster Blt_Image_GetMaster(Tk_Image tkImage);
#endif
#ifndef Blt_Image_GetType_DECLARED
#define Blt_Image_GetType_DECLARED
/* 155 */
BLT_EXTERN Tk_ImageType * Blt_Image_GetType(Tk_Image tkImage);
#endif
#ifndef Blt_Image_GetInstanceData_DECLARED
#define Blt_Image_GetInstanceData_DECLARED
/* 156 */
BLT_EXTERN ClientData	Blt_Image_GetInstanceData(Tk_Image tkImage);
#endif
#ifndef Blt_Image_GetMasterData_DECLARED
#define Blt_Image_GetMasterData_DECLARED
/* 157 */
BLT_EXTERN ClientData	Blt_Image_GetMasterData(Tk_Image tkImage);
#endif
#ifndef Blt_Image_Name_DECLARED
#define Blt_Image_Name_DECLARED
/* 158 */
BLT_EXTERN const char *	 Blt_Image_Name(Tk_Image tkImage);
#endif
#ifndef Blt_Image_NameOfType_DECLARED
#define Blt_Image_NameOfType_DECLARED
/* 159 */
BLT_EXTERN const char *	 Blt_Image_NameOfType(Tk_Image tkImage);
#endif
#ifndef Blt_FreePainter_DECLARED
#define Blt_FreePainter_DECLARED
/* 160 */
BLT_EXTERN void		Blt_FreePainter(Blt_Painter painter);
#endif
#ifndef Blt_GetPainter_DECLARED
#define Blt_GetPainter_DECLARED
/* 161 */
BLT_EXTERN Blt_Painter	Blt_GetPainter(Tk_Window tkwin, float gamma);
#endif
#ifndef Blt_GetPainterFromDrawable_DECLARED
#define Blt_GetPainterFromDrawable_DECLARED
/* 162 */
BLT_EXTERN Blt_Painter	Blt_GetPainterFromDrawable(Display *display,
				Drawable drawable, float gamma);
#endif
#ifndef Blt_PainterGC_DECLARED
#define Blt_PainterGC_DECLARED
/* 163 */
BLT_EXTERN GC		Blt_PainterGC(Blt_Painter painter);
#endif
#ifndef Blt_PainterDepth_DECLARED
#define Blt_PainterDepth_DECLARED
/* 164 */
BLT_EXTERN int		Blt_PainterDepth(Blt_Painter painter);
#endif
#ifndef Blt_SetPainterClipRegion_DECLARED
#define Blt_SetPainterClipRegion_DECLARED
/* 165 */
BLT_EXTERN void		Blt_SetPainterClipRegion(Blt_Painter painter,
				TkRegion rgn);
#endif
#ifndef Blt_UnsetPainterClipRegion_DECLARED
#define Blt_UnsetPainterClipRegion_DECLARED
/* 166 */
BLT_EXTERN void		Blt_UnsetPainterClipRegion(Blt_Painter painter);
#endif
#ifndef Blt_PaintPicture_DECLARED
#define Blt_PaintPicture_DECLARED
/* 167 */
BLT_EXTERN int		Blt_PaintPicture(Blt_Painter painter,
				Drawable drawable, Blt_Picture src, int srcX,
				int srcY, int width, int height, int destX,
				int destY, unsigned int flags);
#endif
#ifndef Blt_PaintPictureWithBlend_DECLARED
#define Blt_PaintPictureWithBlend_DECLARED
/* 168 */
BLT_EXTERN int		Blt_PaintPictureWithBlend(Blt_Painter painter,
				Drawable drawable, Blt_Picture src, int srcX,
				int srcY, int width, int height, int destX,
				int destY, unsigned int flags);
#endif
#ifndef Blt_PaintCheckbox_DECLARED
#define Blt_PaintCheckbox_DECLARED
/* 169 */
BLT_EXTERN Blt_Picture	Blt_PaintCheckbox(int width, int height,
				XColor *fillColor, XColor *outlineColor,
				XColor *checkColor, int isOn);
#endif
#ifndef Blt_PaintRadioButton_DECLARED
#define Blt_PaintRadioButton_DECLARED
/* 170 */
BLT_EXTERN Blt_Picture	Blt_PaintRadioButton(int width, int height,
				Blt_Bg bg, XColor *fill, XColor *outline,
				int isOn);
#endif
#ifndef Blt_PaintRadioButtonOld_DECLARED
#define Blt_PaintRadioButtonOld_DECLARED
/* 171 */
BLT_EXTERN Blt_Picture	Blt_PaintRadioButtonOld(int width, int height,
				XColor *bg, XColor *fill, XColor *outline,
				XColor *check, int isOn);
#endif
#ifndef Blt_PaintDelete_DECLARED
#define Blt_PaintDelete_DECLARED
/* 172 */
BLT_EXTERN Blt_Picture	Blt_PaintDelete(int width, int height,
				XColor *bgColor, XColor *fillColor,
				XColor *symColor, int isActive);
#endif
#ifndef Blt_Ps_Create_DECLARED
#define Blt_Ps_Create_DECLARED
/* 173 */
BLT_EXTERN Blt_Ps	Blt_Ps_Create(Tcl_Interp *interp,
				PageSetup *setupPtr);
#endif
#ifndef Blt_Ps_Free_DECLARED
#define Blt_Ps_Free_DECLARED
/* 174 */
BLT_EXTERN void		Blt_Ps_Free(Blt_Ps ps);
#endif
#ifndef Blt_Ps_GetValue_DECLARED
#define Blt_Ps_GetValue_DECLARED
/* 175 */
BLT_EXTERN const char *	 Blt_Ps_GetValue(Blt_Ps ps, int *lengthPtr);
#endif
#ifndef Blt_Ps_GetDBuffer_DECLARED
#define Blt_Ps_GetDBuffer_DECLARED
/* 176 */
BLT_EXTERN Blt_DBuffer	Blt_Ps_GetDBuffer(Blt_Ps ps);
#endif
#ifndef Blt_Ps_GetInterp_DECLARED
#define Blt_Ps_GetInterp_DECLARED
/* 177 */
BLT_EXTERN Tcl_Interp *	 Blt_Ps_GetInterp(Blt_Ps ps);
#endif
#ifndef Blt_Ps_GetScratchBuffer_DECLARED
#define Blt_Ps_GetScratchBuffer_DECLARED
/* 178 */
BLT_EXTERN char *	Blt_Ps_GetScratchBuffer(Blt_Ps ps);
#endif
#ifndef Blt_Ps_SetInterp_DECLARED
#define Blt_Ps_SetInterp_DECLARED
/* 179 */
BLT_EXTERN void		Blt_Ps_SetInterp(Blt_Ps ps, Tcl_Interp *interp);
#endif
#ifndef Blt_Ps_Append_DECLARED
#define Blt_Ps_Append_DECLARED
/* 180 */
BLT_EXTERN void		Blt_Ps_Append(Blt_Ps ps, const char *string);
#endif
#ifndef Blt_Ps_AppendBytes_DECLARED
#define Blt_Ps_AppendBytes_DECLARED
/* 181 */
BLT_EXTERN void		Blt_Ps_AppendBytes(Blt_Ps ps, const char *string,
				int numBytes);
#endif
#ifndef Blt_Ps_VarAppend_DECLARED
#define Blt_Ps_VarAppend_DECLARED
/* 182 */
BLT_EXTERN void		Blt_Ps_VarAppend(Blt_Ps ps, ...);
#endif
#ifndef Blt_Ps_Format_DECLARED
#define Blt_Ps_Format_DECLARED
/* 183 */
BLT_EXTERN void		Blt_Ps_Format(Blt_Ps ps, const char *fmt, ...);
#endif
#ifndef Blt_Ps_SetClearBackground_DECLARED
#define Blt_Ps_SetClearBackground_DECLARED
/* 184 */
BLT_EXTERN void		Blt_Ps_SetClearBackground(Blt_Ps ps);
#endif
#ifndef Blt_Ps_IncludeFile_DECLARED
#define Blt_Ps_IncludeFile_DECLARED
/* 185 */
BLT_EXTERN int		Blt_Ps_IncludeFile(Tcl_Interp *interp, Blt_Ps ps,
				const char *fileName);
#endif
#ifndef Blt_Ps_GetPicaFromObj_DECLARED
#define Blt_Ps_GetPicaFromObj_DECLARED
/* 186 */
BLT_EXTERN int		Blt_Ps_GetPicaFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *picaPtr);
#endif
#ifndef Blt_Ps_GetPadFromObj_DECLARED
#define Blt_Ps_GetPadFromObj_DECLARED
/* 187 */
BLT_EXTERN int		Blt_Ps_GetPadFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_Pad *padPtr);
#endif
#ifndef Blt_Ps_ComputeBoundingBox_DECLARED
#define Blt_Ps_ComputeBoundingBox_DECLARED
/* 188 */
BLT_EXTERN int		Blt_Ps_ComputeBoundingBox(PageSetup *setupPtr, int w,
				int h);
#endif
#ifndef Blt_Ps_DrawPicture_DECLARED
#define Blt_Ps_DrawPicture_DECLARED
/* 189 */
BLT_EXTERN void		Blt_Ps_DrawPicture(Blt_Ps ps, Blt_Picture picture,
				double x, double y);
#endif
#ifndef Blt_Ps_Rectangle_DECLARED
#define Blt_Ps_Rectangle_DECLARED
/* 190 */
BLT_EXTERN void		Blt_Ps_Rectangle(Blt_Ps ps, int x, int y, int w,
				int h);
#endif
#ifndef Blt_Ps_Rectangle2_DECLARED
#define Blt_Ps_Rectangle2_DECLARED
/* 191 */
BLT_EXTERN void		Blt_Ps_Rectangle2(Blt_Ps ps, double x1, double y1,
				double x2, double y2);
#endif
#ifndef Blt_Ps_SaveFile_DECLARED
#define Blt_Ps_SaveFile_DECLARED
/* 192 */
BLT_EXTERN int		Blt_Ps_SaveFile(Tcl_Interp *interp, Blt_Ps ps,
				const char *fileName);
#endif
#ifndef Blt_Ps_XSetLineWidth_DECLARED
#define Blt_Ps_XSetLineWidth_DECLARED
/* 193 */
BLT_EXTERN void		Blt_Ps_XSetLineWidth(Blt_Ps ps, int lineWidth);
#endif
#ifndef Blt_Ps_XSetBackground_DECLARED
#define Blt_Ps_XSetBackground_DECLARED
/* 194 */
BLT_EXTERN void		Blt_Ps_XSetBackground(Blt_Ps ps, XColor *colorPtr);
#endif
#ifndef Blt_Ps_XSetBitmapData_DECLARED
#define Blt_Ps_XSetBitmapData_DECLARED
/* 195 */
BLT_EXTERN void		Blt_Ps_XSetBitmapData(Blt_Ps ps, Display *display,
				Pixmap bitmap, int width, int height);
#endif
#ifndef Blt_Ps_XSetForeground_DECLARED
#define Blt_Ps_XSetForeground_DECLARED
/* 196 */
BLT_EXTERN void		Blt_Ps_XSetForeground(Blt_Ps ps, XColor *colorPtr);
#endif
#ifndef Blt_Ps_XSetFont_DECLARED
#define Blt_Ps_XSetFont_DECLARED
/* 197 */
BLT_EXTERN void		Blt_Ps_XSetFont(Blt_Ps ps, Blt_Font font);
#endif
#ifndef Blt_Ps_XSetDashes_DECLARED
#define Blt_Ps_XSetDashes_DECLARED
/* 198 */
BLT_EXTERN void		Blt_Ps_XSetDashes(Blt_Ps ps, Blt_Dashes *dashesPtr);
#endif
#ifndef Blt_Ps_XSetLineAttributes_DECLARED
#define Blt_Ps_XSetLineAttributes_DECLARED
/* 199 */
BLT_EXTERN void		Blt_Ps_XSetLineAttributes(Blt_Ps ps,
				XColor *colorPtr, int lineWidth,
				Blt_Dashes *dashesPtr, int capStyle,
				int joinStyle);
#endif
#ifndef Blt_Ps_XSetStipple_DECLARED
#define Blt_Ps_XSetStipple_DECLARED
/* 200 */
BLT_EXTERN void		Blt_Ps_XSetStipple(Blt_Ps ps, Display *display,
				Pixmap bitmap);
#endif
#ifndef Blt_Ps_Polyline_DECLARED
#define Blt_Ps_Polyline_DECLARED
/* 201 */
BLT_EXTERN void		Blt_Ps_Polyline(Blt_Ps ps, int n, Point2d *points);
#endif
#ifndef Blt_Ps_XDrawLines_DECLARED
#define Blt_Ps_XDrawLines_DECLARED
/* 202 */
BLT_EXTERN void		Blt_Ps_XDrawLines(Blt_Ps ps, int n, XPoint *points);
#endif
#ifndef Blt_Ps_XDrawSegments_DECLARED
#define Blt_Ps_XDrawSegments_DECLARED
/* 203 */
BLT_EXTERN void		Blt_Ps_XDrawSegments(Blt_Ps ps, int n,
				XSegment *segments);
#endif
#ifndef Blt_Ps_DrawPolyline_DECLARED
#define Blt_Ps_DrawPolyline_DECLARED
/* 204 */
BLT_EXTERN void		Blt_Ps_DrawPolyline(Blt_Ps ps, int n,
				Point2d *points);
#endif
#ifndef Blt_Ps_DrawSegments2d_DECLARED
#define Blt_Ps_DrawSegments2d_DECLARED
/* 205 */
BLT_EXTERN void		Blt_Ps_DrawSegments2d(Blt_Ps ps, int n,
				Segment2d *segments);
#endif
#ifndef Blt_Ps_Draw3DRectangle_DECLARED
#define Blt_Ps_Draw3DRectangle_DECLARED
/* 206 */
BLT_EXTERN void		Blt_Ps_Draw3DRectangle(Blt_Ps ps, Tk_3DBorder border,
				double x, double y, int width, int height,
				int borderWidth, int relief);
#endif
#ifndef Blt_Ps_Fill3DRectangle_DECLARED
#define Blt_Ps_Fill3DRectangle_DECLARED
/* 207 */
BLT_EXTERN void		Blt_Ps_Fill3DRectangle(Blt_Ps ps, Tk_3DBorder border,
				double x, double y, int width, int height,
				int borderWidth, int relief);
#endif
#ifndef Blt_Ps_XFillRectangle_DECLARED
#define Blt_Ps_XFillRectangle_DECLARED
/* 208 */
BLT_EXTERN void		Blt_Ps_XFillRectangle(Blt_Ps ps, double x, double y,
				int width, int height);
#endif
#ifndef Blt_Ps_XFillRectangles_DECLARED
#define Blt_Ps_XFillRectangles_DECLARED
/* 209 */
BLT_EXTERN void		Blt_Ps_XFillRectangles(Blt_Ps ps, int n,
				XRectangle *rects);
#endif
#ifndef Blt_Ps_XFillPolygon_DECLARED
#define Blt_Ps_XFillPolygon_DECLARED
/* 210 */
BLT_EXTERN void		Blt_Ps_XFillPolygon(Blt_Ps ps, int n,
				Point2d *points);
#endif
#ifndef Blt_Ps_DrawPhoto_DECLARED
#define Blt_Ps_DrawPhoto_DECLARED
/* 211 */
BLT_EXTERN void		Blt_Ps_DrawPhoto(Blt_Ps ps,
				Tk_PhotoHandle photoToken, double x,
				double y);
#endif
#ifndef Blt_Ps_XDrawWindow_DECLARED
#define Blt_Ps_XDrawWindow_DECLARED
/* 212 */
BLT_EXTERN void		Blt_Ps_XDrawWindow(Blt_Ps ps, Tk_Window tkwin,
				double x, double y);
#endif
#ifndef Blt_Ps_DrawText_DECLARED
#define Blt_Ps_DrawText_DECLARED
/* 213 */
BLT_EXTERN void		Blt_Ps_DrawText(Blt_Ps ps, const char *string,
				TextStyle *attrPtr, double x, double y);
#endif
#ifndef Blt_Ps_DrawBitmap_DECLARED
#define Blt_Ps_DrawBitmap_DECLARED
/* 214 */
BLT_EXTERN void		Blt_Ps_DrawBitmap(Blt_Ps ps, Display *display,
				Pixmap bitmap, double scaleX, double scaleY);
#endif
#ifndef Blt_Ps_XSetCapStyle_DECLARED
#define Blt_Ps_XSetCapStyle_DECLARED
/* 215 */
BLT_EXTERN void		Blt_Ps_XSetCapStyle(Blt_Ps ps, int capStyle);
#endif
#ifndef Blt_Ps_XSetJoinStyle_DECLARED
#define Blt_Ps_XSetJoinStyle_DECLARED
/* 216 */
BLT_EXTERN void		Blt_Ps_XSetJoinStyle(Blt_Ps ps, int joinStyle);
#endif
#ifndef Blt_Ps_PolylineFromXPoints_DECLARED
#define Blt_Ps_PolylineFromXPoints_DECLARED
/* 217 */
BLT_EXTERN void		Blt_Ps_PolylineFromXPoints(Blt_Ps ps, int n,
				XPoint *points);
#endif
#ifndef Blt_Ps_Polygon_DECLARED
#define Blt_Ps_Polygon_DECLARED
/* 218 */
BLT_EXTERN void		Blt_Ps_Polygon(Blt_Ps ps, Point2d *points,
				int numPoints);
#endif
#ifndef Blt_Ps_SetPrinting_DECLARED
#define Blt_Ps_SetPrinting_DECLARED
/* 219 */
BLT_EXTERN void		Blt_Ps_SetPrinting(Blt_Ps ps, int value);
#endif
#ifndef Blt_Ts_CreateLayout_DECLARED
#define Blt_Ts_CreateLayout_DECLARED
/* 220 */
BLT_EXTERN TextLayout *	 Blt_Ts_CreateLayout(const char *string, int length,
				TextStyle *tsPtr);
#endif
#ifndef Blt_Ts_TitleLayout_DECLARED
#define Blt_Ts_TitleLayout_DECLARED
/* 221 */
BLT_EXTERN TextLayout *	 Blt_Ts_TitleLayout(const char *string, int length,
				TextStyle *tsPtr);
#endif
#ifndef Blt_Ts_DrawLayout_DECLARED
#define Blt_Ts_DrawLayout_DECLARED
/* 222 */
BLT_EXTERN void		Blt_Ts_DrawLayout(Tk_Window tkwin, Drawable drawable,
				TextLayout *textPtr, TextStyle *tsPtr, int x,
				int y);
#endif
#ifndef Blt_Ts_GetExtents_DECLARED
#define Blt_Ts_GetExtents_DECLARED
/* 223 */
BLT_EXTERN void		Blt_Ts_GetExtents(TextStyle *tsPtr, const char *text,
				unsigned int *widthPtr,
				unsigned int *heightPtr);
#endif
#ifndef Blt_Ts_ResetStyle_DECLARED
#define Blt_Ts_ResetStyle_DECLARED
/* 224 */
BLT_EXTERN void		Blt_Ts_ResetStyle(Tk_Window tkwin, TextStyle *tsPtr);
#endif
#ifndef Blt_Ts_FreeStyle_DECLARED
#define Blt_Ts_FreeStyle_DECLARED
/* 225 */
BLT_EXTERN void		Blt_Ts_FreeStyle(Display *display, TextStyle *tsPtr);
#endif
#ifndef Blt_Ts_SetDrawStyle_DECLARED
#define Blt_Ts_SetDrawStyle_DECLARED
/* 226 */
BLT_EXTERN void		Blt_Ts_SetDrawStyle(TextStyle *tsPtr, Blt_Font font,
				GC gc, XColor *fgColor, float angle,
				Tk_Anchor anchor, Tk_Justify justify,
				int leader);
#endif
#ifndef Blt_DrawText_DECLARED
#define Blt_DrawText_DECLARED
/* 227 */
BLT_EXTERN void		Blt_DrawText(Tk_Window tkwin, Drawable drawable,
				const char *string, TextStyle *tsPtr, int x,
				int y);
#endif
#ifndef Blt_DrawText2_DECLARED
#define Blt_DrawText2_DECLARED
/* 228 */
BLT_EXTERN void		Blt_DrawText2(Tk_Window tkwin, Drawable drawable,
				const char *string, TextStyle *tsPtr, int x,
				int y, Dim2d *dimPtr);
#endif
#ifndef Blt_Ts_Bitmap_DECLARED
#define Blt_Ts_Bitmap_DECLARED
/* 229 */
BLT_EXTERN Pixmap	Blt_Ts_Bitmap(Tk_Window tkwin, TextLayout *textPtr,
				TextStyle *tsPtr, int *widthPtr,
				int *heightPtr);
#endif
#ifndef Blt_DrawTextWithRotatedFont_DECLARED
#define Blt_DrawTextWithRotatedFont_DECLARED
/* 230 */
BLT_EXTERN int		Blt_DrawTextWithRotatedFont(Tk_Window tkwin,
				Drawable drawable, float angle,
				TextStyle *tsPtr, TextLayout *textPtr, int x,
				int y);
#endif
#ifndef Blt_DrawLayout_DECLARED
#define Blt_DrawLayout_DECLARED
/* 231 */
BLT_EXTERN void		Blt_DrawLayout(Tk_Window tkwin, Drawable drawable,
				GC gc, Blt_Font font, int depth, float angle,
				int x, int y, TextLayout *layoutPtr,
				int maxLength);
#endif
#ifndef Blt_GetTextExtents_DECLARED
#define Blt_GetTextExtents_DECLARED
/* 232 */
BLT_EXTERN void		Blt_GetTextExtents(Blt_Font font, int leader,
				const char *text, int textLen,
				unsigned int *widthPtr,
				unsigned int *heightPtr);
#endif
#ifndef Blt_RotateStartingTextPositions_DECLARED
#define Blt_RotateStartingTextPositions_DECLARED
/* 233 */
BLT_EXTERN void		Blt_RotateStartingTextPositions(TextLayout *textPtr,
				float angle);
#endif
#ifndef Blt_ComputeTextLayout_DECLARED
#define Blt_ComputeTextLayout_DECLARED
/* 234 */
BLT_EXTERN Tk_TextLayout Blt_ComputeTextLayout(Blt_Font font,
				const char *string, int numChars,
				int wrapLength, Tk_Justify justify,
				int flags, int *widthPtr, int *heightPtr);
#endif
#ifndef Blt_DrawTextLayout_DECLARED
#define Blt_DrawTextLayout_DECLARED
/* 235 */
BLT_EXTERN void		Blt_DrawTextLayout(Display *display,
				Drawable drawable, GC gc,
				Tk_TextLayout layout, int x, int y,
				int firstChar, int lastChar);
#endif
#ifndef Blt_CharBbox_DECLARED
#define Blt_CharBbox_DECLARED
/* 236 */
BLT_EXTERN int		Blt_CharBbox(Tk_TextLayout layout, int index,
				int *xPtr, int *yPtr, int *widthPtr,
				int *heightPtr);
#endif
#ifndef Blt_UnderlineTextLayout_DECLARED
#define Blt_UnderlineTextLayout_DECLARED
/* 237 */
BLT_EXTERN void		Blt_UnderlineTextLayout(Display *display,
				Drawable drawable, GC gc,
				Tk_TextLayout layout, int x, int y,
				int underline);
#endif
#ifndef Blt_Ts_UnderlineLayout_DECLARED
#define Blt_Ts_UnderlineLayout_DECLARED
/* 238 */
BLT_EXTERN void		Blt_Ts_UnderlineLayout(Tk_Window tkwin,
				Drawable drawable, TextLayout *layoutPtr,
				TextStyle *tsPtr, int x, int y);
#endif
#ifndef Blt_Ts_DrawText_DECLARED
#define Blt_Ts_DrawText_DECLARED
/* 239 */
BLT_EXTERN void		Blt_Ts_DrawText(Tk_Window tkwin, Drawable drawable,
				const char *text, int textLen,
				TextStyle *tsPtr, int x, int y);
#endif
#ifndef Blt_MeasureText_DECLARED
#define Blt_MeasureText_DECLARED
/* 240 */
BLT_EXTERN int		Blt_MeasureText(Blt_Font font, const char *text,
				int textLen, int maxLength, int *nBytesPtr);
#endif
#ifndef Blt_FreeTextLayout_DECLARED
#define Blt_FreeTextLayout_DECLARED
/* 241 */
BLT_EXTERN void		Blt_FreeTextLayout(Tk_TextLayout layout);
#endif

typedef struct BltTkIntProcs {
    int magic;
    struct BltTkIntStubHooks *hooks;

    void *reserved0;
    void (*blt_Draw3DRectangle) (Tk_Window tkwin, Drawable drawable, Tk_3DBorder border, int x, int y, int width, int height, int borderWidth, int relief); /* 1 */
    void (*blt_Fill3DRectangle) (Tk_Window tkwin, Drawable drawable, Tk_3DBorder border, int x, int y, int width, int height, int borderWidth, int relief); /* 2 */
    int (*blt_AdjustViewport) (int offset, int worldSize, int windowSize, int scrollUnits, int scrollMode); /* 3 */
    int (*blt_GetScrollInfoFromObj) (Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, int *offsetPtr, int worldSize, int windowSize, int scrollUnits, int scrollMode); /* 4 */
    void (*blt_UpdateScrollbar) (Tcl_Interp *interp, Tcl_Obj *scrollCmdObjPtr, int first, int last, int width); /* 5 */
    void (*blt_FreeColorPair) (ColorPair *pairPtr); /* 6 */
    int (*blt_LineRectClip) (Region2d *regionPtr, Point2d *p, Point2d *q); /* 7 */
    GC (*blt_GetPrivateGC) (Tk_Window tkwin, unsigned long gcMask, XGCValues *valuePtr); /* 8 */
    GC (*blt_GetPrivateGCFromDrawable) (Display *display, Drawable drawable, unsigned long gcMask, XGCValues *valuePtr); /* 9 */
    void (*blt_FreePrivateGC) (Display *display, GC gc); /* 10 */
    int (*blt_GetWindowFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Window *windowPtr); /* 11 */
    const char * (*blt_GetWindowName) (Display *display, Window window); /* 12 */
    Blt_Chain (*blt_GetChildrenFromWindow) (Display *display, Window window); /* 13 */
    Window (*blt_GetParentWindow) (Display *display, Window window); /* 14 */
    Tk_Window (*blt_FindChild) (Tk_Window parent, char *name); /* 15 */
    Tk_Window (*blt_FirstChild) (Tk_Window parent); /* 16 */
    Tk_Window (*blt_NextChild) (Tk_Window tkwin); /* 17 */
    void (*blt_RelinkWindow) (Tk_Window tkwin, Tk_Window newParent, int x, int y); /* 18 */
    Tk_Window (*blt_Toplevel) (Tk_Window tkwin); /* 19 */
    int (*blt_GetPixels) (Tcl_Interp *interp, Tk_Window tkwin, const char *string, int check, int *valuePtr); /* 20 */
    const char * (*blt_NameOfFill) (int fill); /* 21 */
    const char * (*blt_NameOfResize) (int resize); /* 22 */
    int (*blt_GetXY) (Tcl_Interp *interp, Tk_Window tkwin, const char *string, int *xPtr, int *yPtr); /* 23 */
    Point2d (*blt_GetProjection) (int x, int y, Point2d *p, Point2d *q); /* 24 */
    Point2d (*blt_GetProjection2) (int x, int y, double x1, double y1, double x2, double y2); /* 25 */
    void (*blt_DrawArrowOld) (Display *display, Drawable drawable, GC gc, int x, int y, int w, int h, int borderWidth, int orientation); /* 26 */
    void (*blt_DrawArrow) (Display *display, Drawable drawable, XColor *color, int x, int y, int w, int h, int borderWidth, int orientation); /* 27 */
    void (*blt_MakeTransparentWindowExist) (Tk_Window tkwin, Window parent, int isBusy); /* 28 */
    void (*blt_TranslateAnchor) (int x, int y, int width, int height, Tk_Anchor anchor, int *transXPtr, int *transYPtr); /* 29 */
    Point2d (*blt_AnchorPoint) (double x, double y, double width, double height, Tk_Anchor anchor); /* 30 */
    long (*blt_MaxRequestSize) (Display *display, size_t elemSize); /* 31 */
    Window (*blt_GetWindowId) (Tk_Window tkwin); /* 32 */
    void (*blt_InitXRandrConfig) (Tcl_Interp *interp); /* 33 */
    void (*blt_SizeOfScreen) (Tk_Window tkwin, int *widthPtr, int *heightPtr); /* 34 */
    int (*blt_RootX) (Tk_Window tkwin); /* 35 */
    int (*blt_RootY) (Tk_Window tkwin); /* 36 */
    void (*blt_RootCoordinates) (Tk_Window tkwin, int x, int y, int *rootXPtr, int *rootYPtr); /* 37 */
    void (*blt_MapToplevelWindow) (Tk_Window tkwin); /* 38 */
    void (*blt_UnmapToplevelWindow) (Tk_Window tkwin); /* 39 */
    void (*blt_RaiseToplevelWindow) (Tk_Window tkwin); /* 40 */
    void (*blt_LowerToplevelWindow) (Tk_Window tkwin); /* 41 */
    void (*blt_ResizeToplevelWindow) (Tk_Window tkwin, int w, int h); /* 42 */
    void (*blt_MoveToplevelWindow) (Tk_Window tkwin, int x, int y); /* 43 */
    void (*blt_MoveResizeToplevelWindow) (Tk_Window tkwin, int x, int y, int w, int h); /* 44 */
    int (*blt_GetWindowRegion) (Display *display, Window window, int *xPtr, int *yPtr, int *widthPtr, int *heightPtr); /* 45 */
    int (*blt_GetRootWindowSize) (Display *display, Window window, unsigned int *widthPtr, unsigned int *heightPtr); /* 46 */
    ClientData (*blt_GetWindowInstanceData) (Tk_Window tkwin); /* 47 */
    void (*blt_SetWindowInstanceData) (Tk_Window tkwin, ClientData instanceData); /* 48 */
    void (*blt_DeleteWindowInstanceData) (Tk_Window tkwin); /* 49 */
    int (*blt_ReparentWindow) (Display *display, Window window, Window newParent, int x, int y); /* 50 */
    Blt_DrawableAttributes * (*blt_GetDrawableAttributes) (Display *display, Drawable drawable); /* 51 */
    void (*blt_SetDrawableAttributes) (Display *display, Drawable drawable, int width, int height, int depth, Colormap colormap, Visual *visual); /* 52 */
    void (*blt_SetDrawableAttributesFromWindow) (Tk_Window tkwin, Drawable drawable); /* 53 */
    void (*blt_FreeDrawableAttributes) (Display *display, Drawable drawable); /* 54 */
    GC (*blt_GetBitmapGC) (Tk_Window tkwin); /* 55 */
    Pixmap (*blt_GetPixmapAbortOnError) (Display *dpy, Drawable draw, int w, int h, int depth, int lineNum, const char *fileName); /* 56 */
    void (*blt_ScreenDPI) (Tk_Window tkwin, int *xPtr, int *yPtr); /* 57 */
    int (*blt_OldConfigModified) (Tk_ConfigSpec *specs, ...); /* 58 */
    void (*blt_GetLineExtents) (size_t numPoints, Point2d *points, Region2d *r); /* 59 */
    void (*blt_GetBoundingBox) (int width, int height, float angle, double *widthPtr, double *heightPtr, Point2d *points); /* 60 */
    int (*blt_ParseTifTags) (Tcl_Interp *interp, const char *varName, const unsigned char *bytes, off_t offset, size_t numBytes); /* 61 */
    Blt_Font (*blt_GetFont) (Tcl_Interp *interp, Tk_Window tkwin, const char *string); /* 62 */
    Blt_Font (*blt_AllocFontFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr); /* 63 */
    void (*blt_DrawWithEllipsis) (Tk_Window tkwin, Drawable drawable, GC gc, Blt_Font font, int depth, float angle, const char *string, int numBytes, int x, int y, int maxLength); /* 64 */
    Blt_Font (*blt_GetFontFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr); /* 65 */
    void (*blt_Font_GetMetrics) (Blt_Font font, Blt_FontMetrics *fmPtr); /* 66 */
    int (*blt_TextWidth) (Blt_Font font, const char *string, int length); /* 67 */
    Tcl_Interp * (*blt_Font_GetInterp) (Blt_Font font); /* 68 */
    Tcl_Obj * (*blt_Font_GetFile) (Tcl_Interp *interp, Tcl_Obj *objPtr, double *sizePtr); /* 69 */
    Blt_PaintBrush (*blt_NewTileBrush) (void); /* 70 */
    Blt_PaintBrush (*blt_NewLinearGradientBrush) (void); /* 71 */
    Blt_PaintBrush (*blt_NewStripesBrush) (void); /* 72 */
    Blt_PaintBrush (*blt_NewCheckersBrush) (void); /* 73 */
    Blt_PaintBrush (*blt_NewRadialGradientBrush) (void); /* 74 */
    Blt_PaintBrush (*blt_NewConicalGradientBrush) (void); /* 75 */
    Blt_PaintBrush (*blt_NewColorBrush) (unsigned int color); /* 76 */
    const char * (*blt_GetBrushType) (Blt_PaintBrush brush); /* 77 */
    const char * (*blt_GetBrushName) (Blt_PaintBrush brush); /* 78 */
    const char * (*blt_GetBrushColor) (Blt_PaintBrush brush); /* 79 */
    int (*blt_ConfigurePaintBrush) (Tcl_Interp *interp, Blt_PaintBrush brush); /* 80 */
    int (*blt_GetBrushTypeFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_PaintBrushType *typePtr); /* 81 */
    void (*blt_FreeBrush) (Blt_PaintBrush brush); /* 82 */
    int (*blt_GetPaintBrushFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_PaintBrush *brushPtr); /* 83 */
    int (*blt_GetPaintBrush) (Tcl_Interp *interp, const char *string, Blt_PaintBrush *brushPtr); /* 84 */
    void (*blt_SetLinearGradientBrushPalette) (Blt_PaintBrush brush, Blt_Palette palette); /* 85 */
    void (*blt_SetLinearGradientBrushCalcProc) (Blt_PaintBrush brush, Blt_PaintBrushCalcProc *proc, ClientData clientData); /* 86 */
    void (*blt_SetLinearGradientBrushColors) (Blt_PaintBrush brush, Blt_Pixel *lowPtr, Blt_Pixel *highPtr); /* 87 */
    void (*blt_SetTileBrushPicture) (Blt_PaintBrush brush, Blt_Picture tile); /* 88 */
    void (*blt_SetColorBrushColor) (Blt_PaintBrush brush, unsigned int value); /* 89 */
    void (*blt_SetBrushOrigin) (Blt_PaintBrush brush, int x, int y); /* 90 */
    void (*blt_SetBrushOpacity) (Blt_PaintBrush brush, double percent); /* 91 */
    void (*blt_SetBrushRegion) (Blt_PaintBrush brush, int x, int y, int w, int h); /* 92 */
    int (*blt_GetBrushAlpha) (Blt_PaintBrush brush); /* 93 */
    void (*blt_GetBrushOrigin) (Blt_PaintBrush brush, int *xPtr, int *yPtr); /* 94 */
    int (*blt_GetAssociatedColorFromBrush) (Blt_PaintBrush brush, int x, int y); /* 95 */
    int (*blt_IsVerticalLinearBrush) (Blt_PaintBrush brush); /* 96 */
    void (*blt_PaintRectangle) (Blt_Picture picture, int x, int y, int w, int h, int dx, int dy, Blt_PaintBrush brush, int composite); /* 97 */
    void (*blt_PaintPolygon) (Blt_Picture picture, int n, Point2f *vertices, Blt_PaintBrush brush); /* 98 */
    void (*blt_CreateBrushNotifier) (Blt_PaintBrush brush, Blt_BrushChangedProc *notifyProc, ClientData clientData); /* 99 */
    void (*blt_DeleteBrushNotifier) (Blt_PaintBrush brush, Blt_BrushChangedProc *notifyProc, ClientData clientData); /* 100 */
    int (*blt_GetBg) (Tcl_Interp *interp, Tk_Window tkwin, const char *bgName, Blt_Bg *bgPtr); /* 101 */
    int (*blt_GetBgFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr, Blt_Bg *bgPtr); /* 102 */
    XColor * (*blt_Bg_BorderColor) (Blt_Bg bg); /* 103 */
    Tk_3DBorder (*blt_Bg_Border) (Blt_Bg bg); /* 104 */
    Blt_PaintBrush (*blt_Bg_PaintBrush) (Blt_Bg bg); /* 105 */
    const char * (*blt_Bg_Name) (Blt_Bg bg); /* 106 */
    void (*blt_Bg_Free) (Blt_Bg bg); /* 107 */
    void (*blt_Bg_DrawRectangle) (Tk_Window tkwin, Drawable drawable, Blt_Bg bg, int x, int y, int width, int height, int borderWidth, int relief); /* 108 */
    void (*blt_Bg_FillRectangle) (Tk_Window tkwin, Drawable drawable, Blt_Bg bg, int x, int y, int width, int height, int borderWidth, int relief); /* 109 */
    void (*blt_Bg_DrawPolygon) (Tk_Window tkwin, Drawable drawable, Blt_Bg bg, XPoint *points, int numPoints, int borderWidth, int leftRelief); /* 110 */
    void (*blt_Bg_FillPolygon) (Tk_Window tkwin, Drawable drawable, Blt_Bg bg, XPoint *points, int numPoints, int borderWidth, int leftRelief); /* 111 */
    void (*blt_Bg_GetOrigin) (Blt_Bg bg, int *xPtr, int *yPtr); /* 112 */
    void (*blt_Bg_SetChangedProc) (Blt_Bg bg, Blt_BackgroundChangedProc *notifyProc, ClientData clientData); /* 113 */
    void (*blt_Bg_SetOrigin) (Tk_Window tkwin, Blt_Bg bg, int x, int y); /* 114 */
    void (*blt_Bg_DrawFocus) (Tk_Window tkwin, Blt_Bg bg, int highlightWidth, Drawable drawable); /* 115 */
    GC (*blt_Bg_BorderGC) (Tk_Window tkwin, Blt_Bg bg, int which); /* 116 */
    void (*blt_Bg_SetFromBackground) (Tk_Window tkwin, Blt_Bg bg); /* 117 */
    void (*blt_Bg_UnsetClipRegion) (Tk_Window tkwin, Blt_Bg bg); /* 118 */
    void (*blt_Bg_SetClipRegion) (Tk_Window tkwin, Blt_Bg bg, TkRegion rgn); /* 119 */
    unsigned int (*blt_Bg_GetColor) (Blt_Bg bg); /* 120 */
    void (*blt_3DBorder_SetClipRegion) (Tk_Window tkwin, Tk_3DBorder border, TkRegion rgn); /* 121 */
    void (*blt_3DBorder_UnsetClipRegion) (Tk_Window tkwin, Tk_3DBorder border); /* 122 */
    void (*blt_DestroyBindingTable) (Blt_BindTable table); /* 123 */
    Blt_BindTable (*blt_CreateBindingTable) (Tcl_Interp *interp, Tk_Window tkwin, ClientData clientData, Blt_BindPickProc *pickProc, Blt_BindAppendTagsProc *tagProc); /* 124 */
    int (*blt_ConfigureBindings) (Tcl_Interp *interp, Blt_BindTable table, ClientData item, int argc, const char **argv); /* 125 */
    int (*blt_ConfigureBindingsFromObj) (Tcl_Interp *interp, Blt_BindTable table, ClientData item, int objc, Tcl_Obj *const *objv); /* 126 */
    void (*blt_PickCurrentItem) (Blt_BindTable table); /* 127 */
    void (*blt_DeleteBindings) (Blt_BindTable table, ClientData object); /* 128 */
    void (*blt_MoveBindingTable) (Blt_BindTable table, Tk_Window tkwin); /* 129 */
    void (*blt_Afm_SetPrinting) (Tcl_Interp *interp, int value); /* 130 */
    int (*blt_Afm_IsPrinting) (void); /* 131 */
    int (*blt_Afm_TextWidth) (Blt_Font font, const char *s, int numBytes); /* 132 */
    int (*blt_Afm_GetMetrics) (Blt_Font font, Blt_FontMetrics *fmPtr); /* 133 */
    const char * (*blt_Afm_GetPostscriptFamily) (const char *family); /* 134 */
    void (*blt_Afm_GetPostscriptName) (const char *family, int flags, Tcl_DString *resultPtr); /* 135 */
    void (*blt_SetDashes) (Display *display, GC gc, Blt_Dashes *dashesPtr); /* 136 */
    void (*blt_ResetLimits) (Blt_Limits *limitsPtr); /* 137 */
    int (*blt_GetLimitsFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr, Blt_Limits *limitsPtr); /* 138 */
    int (*blt_ConfigureInfoFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Blt_ConfigSpec *specs, char *widgRec, Tcl_Obj *objPtr, int flags); /* 139 */
    int (*blt_ConfigureValueFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Blt_ConfigSpec *specs, char *widgRec, Tcl_Obj *objPtr, int flags); /* 140 */
    int (*blt_ConfigureWidgetFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Blt_ConfigSpec *specs, int objc, Tcl_Obj *const *objv, char *widgRec, int flags); /* 141 */
    int (*blt_ConfigureComponentFromObj) (Tcl_Interp *interp, Tk_Window tkwin, const char *name, const char *className, Blt_ConfigSpec *specs, int objc, Tcl_Obj *const *objv, char *widgRec, int flags); /* 142 */
    int (*blt_ConfigModified) (Blt_ConfigSpec *specs, ...); /* 143 */
    const char * (*blt_NameOfState) (int state); /* 144 */
    void (*blt_FreeOptions) (Blt_ConfigSpec *specs, char *widgRec, Display *display, int needFlags); /* 145 */
    int (*blt_ObjIsOption) (Blt_ConfigSpec *specs, Tcl_Obj *objPtr, int flags); /* 146 */
    int (*blt_GetPixelsFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr, int flags, int *valuePtr); /* 147 */
    int (*blt_GetPadFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr, Blt_Pad *padPtr); /* 148 */
    int (*blt_GetStateFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *statePtr); /* 149 */
    int (*blt_GetFillFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *fillPtr); /* 150 */
    int (*blt_GetResizeFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *fillPtr); /* 151 */
    int (*blt_GetDashesFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Dashes *dashesPtr); /* 152 */
    int (*blt_Image_IsDeleted) (Tk_Image tkImage); /* 153 */
    Tk_ImageMaster (*blt_Image_GetMaster) (Tk_Image tkImage); /* 154 */
    Tk_ImageType * (*blt_Image_GetType) (Tk_Image tkImage); /* 155 */
    ClientData (*blt_Image_GetInstanceData) (Tk_Image tkImage); /* 156 */
    ClientData (*blt_Image_GetMasterData) (Tk_Image tkImage); /* 157 */
    const char * (*blt_Image_Name) (Tk_Image tkImage); /* 158 */
    const char * (*blt_Image_NameOfType) (Tk_Image tkImage); /* 159 */
    void (*blt_FreePainter) (Blt_Painter painter); /* 160 */
    Blt_Painter (*blt_GetPainter) (Tk_Window tkwin, float gamma); /* 161 */
    Blt_Painter (*blt_GetPainterFromDrawable) (Display *display, Drawable drawable, float gamma); /* 162 */
    GC (*blt_PainterGC) (Blt_Painter painter); /* 163 */
    int (*blt_PainterDepth) (Blt_Painter painter); /* 164 */
    void (*blt_SetPainterClipRegion) (Blt_Painter painter, TkRegion rgn); /* 165 */
    void (*blt_UnsetPainterClipRegion) (Blt_Painter painter); /* 166 */
    int (*blt_PaintPicture) (Blt_Painter painter, Drawable drawable, Blt_Picture src, int srcX, int srcY, int width, int height, int destX, int destY, unsigned int flags); /* 167 */
    int (*blt_PaintPictureWithBlend) (Blt_Painter painter, Drawable drawable, Blt_Picture src, int srcX, int srcY, int width, int height, int destX, int destY, unsigned int flags); /* 168 */
    Blt_Picture (*blt_PaintCheckbox) (int width, int height, XColor *fillColor, XColor *outlineColor, XColor *checkColor, int isOn); /* 169 */
    Blt_Picture (*blt_PaintRadioButton) (int width, int height, Blt_Bg bg, XColor *fill, XColor *outline, int isOn); /* 170 */
    Blt_Picture (*blt_PaintRadioButtonOld) (int width, int height, XColor *bg, XColor *fill, XColor *outline, XColor *check, int isOn); /* 171 */
    Blt_Picture (*blt_PaintDelete) (int width, int height, XColor *bgColor, XColor *fillColor, XColor *symColor, int isActive); /* 172 */
    Blt_Ps (*blt_Ps_Create) (Tcl_Interp *interp, PageSetup *setupPtr); /* 173 */
    void (*blt_Ps_Free) (Blt_Ps ps); /* 174 */
    const char * (*blt_Ps_GetValue) (Blt_Ps ps, int *lengthPtr); /* 175 */
    Blt_DBuffer (*blt_Ps_GetDBuffer) (Blt_Ps ps); /* 176 */
    Tcl_Interp * (*blt_Ps_GetInterp) (Blt_Ps ps); /* 177 */
    char * (*blt_Ps_GetScratchBuffer) (Blt_Ps ps); /* 178 */
    void (*blt_Ps_SetInterp) (Blt_Ps ps, Tcl_Interp *interp); /* 179 */
    void (*blt_Ps_Append) (Blt_Ps ps, const char *string); /* 180 */
    void (*blt_Ps_AppendBytes) (Blt_Ps ps, const char *string, int numBytes); /* 181 */
    void (*blt_Ps_VarAppend) (Blt_Ps ps, ...); /* 182 */
    void (*blt_Ps_Format) (Blt_Ps ps, const char *fmt, ...); /* 183 */
    void (*blt_Ps_SetClearBackground) (Blt_Ps ps); /* 184 */
    int (*blt_Ps_IncludeFile) (Tcl_Interp *interp, Blt_Ps ps, const char *fileName); /* 185 */
    int (*blt_Ps_GetPicaFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *picaPtr); /* 186 */
    int (*blt_Ps_GetPadFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Pad *padPtr); /* 187 */
    int (*blt_Ps_ComputeBoundingBox) (PageSetup *setupPtr, int w, int h); /* 188 */
    void (*blt_Ps_DrawPicture) (Blt_Ps ps, Blt_Picture picture, double x, double y); /* 189 */
    void (*blt_Ps_Rectangle) (Blt_Ps ps, int x, int y, int w, int h); /* 190 */
    void (*blt_Ps_Rectangle2) (Blt_Ps ps, double x1, double y1, double x2, double y2); /* 191 */
    int (*blt_Ps_SaveFile) (Tcl_Interp *interp, Blt_Ps ps, const char *fileName); /* 192 */
    void (*blt_Ps_XSetLineWidth) (Blt_Ps ps, int lineWidth); /* 193 */
    void (*blt_Ps_XSetBackground) (Blt_Ps ps, XColor *colorPtr); /* 194 */
    void (*blt_Ps_XSetBitmapData) (Blt_Ps ps, Display *display, Pixmap bitmap, int width, int height); /* 195 */
    void (*blt_Ps_XSetForeground) (Blt_Ps ps, XColor *colorPtr); /* 196 */
    void (*blt_Ps_XSetFont) (Blt_Ps ps, Blt_Font font); /* 197 */
    void (*blt_Ps_XSetDashes) (Blt_Ps ps, Blt_Dashes *dashesPtr); /* 198 */
    void (*blt_Ps_XSetLineAttributes) (Blt_Ps ps, XColor *colorPtr, int lineWidth, Blt_Dashes *dashesPtr, int capStyle, int joinStyle); /* 199 */
    void (*blt_Ps_XSetStipple) (Blt_Ps ps, Display *display, Pixmap bitmap); /* 200 */
    void (*blt_Ps_Polyline) (Blt_Ps ps, int n, Point2d *points); /* 201 */
    void (*blt_Ps_XDrawLines) (Blt_Ps ps, int n, XPoint *points); /* 202 */
    void (*blt_Ps_XDrawSegments) (Blt_Ps ps, int n, XSegment *segments); /* 203 */
    void (*blt_Ps_DrawPolyline) (Blt_Ps ps, int n, Point2d *points); /* 204 */
    void (*blt_Ps_DrawSegments2d) (Blt_Ps ps, int n, Segment2d *segments); /* 205 */
    void (*blt_Ps_Draw3DRectangle) (Blt_Ps ps, Tk_3DBorder border, double x, double y, int width, int height, int borderWidth, int relief); /* 206 */
    void (*blt_Ps_Fill3DRectangle) (Blt_Ps ps, Tk_3DBorder border, double x, double y, int width, int height, int borderWidth, int relief); /* 207 */
    void (*blt_Ps_XFillRectangle) (Blt_Ps ps, double x, double y, int width, int height); /* 208 */
    void (*blt_Ps_XFillRectangles) (Blt_Ps ps, int n, XRectangle *rects); /* 209 */
    void (*blt_Ps_XFillPolygon) (Blt_Ps ps, int n, Point2d *points); /* 210 */
    void (*blt_Ps_DrawPhoto) (Blt_Ps ps, Tk_PhotoHandle photoToken, double x, double y); /* 211 */
    void (*blt_Ps_XDrawWindow) (Blt_Ps ps, Tk_Window tkwin, double x, double y); /* 212 */
    void (*blt_Ps_DrawText) (Blt_Ps ps, const char *string, TextStyle *attrPtr, double x, double y); /* 213 */
    void (*blt_Ps_DrawBitmap) (Blt_Ps ps, Display *display, Pixmap bitmap, double scaleX, double scaleY); /* 214 */
    void (*blt_Ps_XSetCapStyle) (Blt_Ps ps, int capStyle); /* 215 */
    void (*blt_Ps_XSetJoinStyle) (Blt_Ps ps, int joinStyle); /* 216 */
    void (*blt_Ps_PolylineFromXPoints) (Blt_Ps ps, int n, XPoint *points); /* 217 */
    void (*blt_Ps_Polygon) (Blt_Ps ps, Point2d *points, int numPoints); /* 218 */
    void (*blt_Ps_SetPrinting) (Blt_Ps ps, int value); /* 219 */
    TextLayout * (*blt_Ts_CreateLayout) (const char *string, int length, TextStyle *tsPtr); /* 220 */
    TextLayout * (*blt_Ts_TitleLayout) (const char *string, int length, TextStyle *tsPtr); /* 221 */
    void (*blt_Ts_DrawLayout) (Tk_Window tkwin, Drawable drawable, TextLayout *textPtr, TextStyle *tsPtr, int x, int y); /* 222 */
    void (*blt_Ts_GetExtents) (TextStyle *tsPtr, const char *text, unsigned int *widthPtr, unsigned int *heightPtr); /* 223 */
    void (*blt_Ts_ResetStyle) (Tk_Window tkwin, TextStyle *tsPtr); /* 224 */
    void (*blt_Ts_FreeStyle) (Display *display, TextStyle *tsPtr); /* 225 */
    void (*blt_Ts_SetDrawStyle) (TextStyle *tsPtr, Blt_Font font, GC gc, XColor *fgColor, float angle, Tk_Anchor anchor, Tk_Justify justify, int leader); /* 226 */
    void (*blt_DrawText) (Tk_Window tkwin, Drawable drawable, const char *string, TextStyle *tsPtr, int x, int y); /* 227 */
    void (*blt_DrawText2) (Tk_Window tkwin, Drawable drawable, const char *string, TextStyle *tsPtr, int x, int y, Dim2d *dimPtr); /* 228 */
    Pixmap (*blt_Ts_Bitmap) (Tk_Window tkwin, TextLayout *textPtr, TextStyle *tsPtr, int *widthPtr, int *heightPtr); /* 229 */
    int (*blt_DrawTextWithRotatedFont) (Tk_Window tkwin, Drawable drawable, float angle, TextStyle *tsPtr, TextLayout *textPtr, int x, int y); /* 230 */
    void (*blt_DrawLayout) (Tk_Window tkwin, Drawable drawable, GC gc, Blt_Font font, int depth, float angle, int x, int y, TextLayout *layoutPtr, int maxLength); /* 231 */
    void (*blt_GetTextExtents) (Blt_Font font, int leader, const char *text, int textLen, unsigned int *widthPtr, unsigned int *heightPtr); /* 232 */
    void (*blt_RotateStartingTextPositions) (TextLayout *textPtr, float angle); /* 233 */
    Tk_TextLayout (*blt_ComputeTextLayout) (Blt_Font font, const char *string, int numChars, int wrapLength, Tk_Justify justify, int flags, int *widthPtr, int *heightPtr); /* 234 */
    void (*blt_DrawTextLayout) (Display *display, Drawable drawable, GC gc, Tk_TextLayout layout, int x, int y, int firstChar, int lastChar); /* 235 */
    int (*blt_CharBbox) (Tk_TextLayout layout, int index, int *xPtr, int *yPtr, int *widthPtr, int *heightPtr); /* 236 */
    void (*blt_UnderlineTextLayout) (Display *display, Drawable drawable, GC gc, Tk_TextLayout layout, int x, int y, int underline); /* 237 */
    void (*blt_Ts_UnderlineLayout) (Tk_Window tkwin, Drawable drawable, TextLayout *layoutPtr, TextStyle *tsPtr, int x, int y); /* 238 */
    void (*blt_Ts_DrawText) (Tk_Window tkwin, Drawable drawable, const char *text, int textLen, TextStyle *tsPtr, int x, int y); /* 239 */
    int (*blt_MeasureText) (Blt_Font font, const char *text, int textLen, int maxLength, int *nBytesPtr); /* 240 */
    void (*blt_FreeTextLayout) (Tk_TextLayout layout); /* 241 */
} BltTkIntProcs;

#ifdef __cplusplus
extern "C" {
#endif
extern BltTkIntProcs *bltTkIntProcsPtr;
#ifdef __cplusplus
}
#endif

#if defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TK_PROCS)

/*
 * Inline function declarations:
 */

/* Slot 0 is reserved */
#ifndef Blt_Draw3DRectangle
#define Blt_Draw3DRectangle \
	(bltTkIntProcsPtr->blt_Draw3DRectangle) /* 1 */
#endif
#ifndef Blt_Fill3DRectangle
#define Blt_Fill3DRectangle \
	(bltTkIntProcsPtr->blt_Fill3DRectangle) /* 2 */
#endif
#ifndef Blt_AdjustViewport
#define Blt_AdjustViewport \
	(bltTkIntProcsPtr->blt_AdjustViewport) /* 3 */
#endif
#ifndef Blt_GetScrollInfoFromObj
#define Blt_GetScrollInfoFromObj \
	(bltTkIntProcsPtr->blt_GetScrollInfoFromObj) /* 4 */
#endif
#ifndef Blt_UpdateScrollbar
#define Blt_UpdateScrollbar \
	(bltTkIntProcsPtr->blt_UpdateScrollbar) /* 5 */
#endif
#ifndef Blt_FreeColorPair
#define Blt_FreeColorPair \
	(bltTkIntProcsPtr->blt_FreeColorPair) /* 6 */
#endif
#ifndef Blt_LineRectClip
#define Blt_LineRectClip \
	(bltTkIntProcsPtr->blt_LineRectClip) /* 7 */
#endif
#ifndef Blt_GetPrivateGC
#define Blt_GetPrivateGC \
	(bltTkIntProcsPtr->blt_GetPrivateGC) /* 8 */
#endif
#ifndef Blt_GetPrivateGCFromDrawable
#define Blt_GetPrivateGCFromDrawable \
	(bltTkIntProcsPtr->blt_GetPrivateGCFromDrawable) /* 9 */
#endif
#ifndef Blt_FreePrivateGC
#define Blt_FreePrivateGC \
	(bltTkIntProcsPtr->blt_FreePrivateGC) /* 10 */
#endif
#ifndef Blt_GetWindowFromObj
#define Blt_GetWindowFromObj \
	(bltTkIntProcsPtr->blt_GetWindowFromObj) /* 11 */
#endif
#ifndef Blt_GetWindowName
#define Blt_GetWindowName \
	(bltTkIntProcsPtr->blt_GetWindowName) /* 12 */
#endif
#ifndef Blt_GetChildrenFromWindow
#define Blt_GetChildrenFromWindow \
	(bltTkIntProcsPtr->blt_GetChildrenFromWindow) /* 13 */
#endif
#ifndef Blt_GetParentWindow
#define Blt_GetParentWindow \
	(bltTkIntProcsPtr->blt_GetParentWindow) /* 14 */
#endif
#ifndef Blt_FindChild
#define Blt_FindChild \
	(bltTkIntProcsPtr->blt_FindChild) /* 15 */
#endif
#ifndef Blt_FirstChild
#define Blt_FirstChild \
	(bltTkIntProcsPtr->blt_FirstChild) /* 16 */
#endif
#ifndef Blt_NextChild
#define Blt_NextChild \
	(bltTkIntProcsPtr->blt_NextChild) /* 17 */
#endif
#ifndef Blt_RelinkWindow
#define Blt_RelinkWindow \
	(bltTkIntProcsPtr->blt_RelinkWindow) /* 18 */
#endif
#ifndef Blt_Toplevel
#define Blt_Toplevel \
	(bltTkIntProcsPtr->blt_Toplevel) /* 19 */
#endif
#ifndef Blt_GetPixels
#define Blt_GetPixels \
	(bltTkIntProcsPtr->blt_GetPixels) /* 20 */
#endif
#ifndef Blt_NameOfFill
#define Blt_NameOfFill \
	(bltTkIntProcsPtr->blt_NameOfFill) /* 21 */
#endif
#ifndef Blt_NameOfResize
#define Blt_NameOfResize \
	(bltTkIntProcsPtr->blt_NameOfResize) /* 22 */
#endif
#ifndef Blt_GetXY
#define Blt_GetXY \
	(bltTkIntProcsPtr->blt_GetXY) /* 23 */
#endif
#ifndef Blt_GetProjection
#define Blt_GetProjection \
	(bltTkIntProcsPtr->blt_GetProjection) /* 24 */
#endif
#ifndef Blt_GetProjection2
#define Blt_GetProjection2 \
	(bltTkIntProcsPtr->blt_GetProjection2) /* 25 */
#endif
#ifndef Blt_DrawArrowOld
#define Blt_DrawArrowOld \
	(bltTkIntProcsPtr->blt_DrawArrowOld) /* 26 */
#endif
#ifndef Blt_DrawArrow
#define Blt_DrawArrow \
	(bltTkIntProcsPtr->blt_DrawArrow) /* 27 */
#endif
#ifndef Blt_MakeTransparentWindowExist
#define Blt_MakeTransparentWindowExist \
	(bltTkIntProcsPtr->blt_MakeTransparentWindowExist) /* 28 */
#endif
#ifndef Blt_TranslateAnchor
#define Blt_TranslateAnchor \
	(bltTkIntProcsPtr->blt_TranslateAnchor) /* 29 */
#endif
#ifndef Blt_AnchorPoint
#define Blt_AnchorPoint \
	(bltTkIntProcsPtr->blt_AnchorPoint) /* 30 */
#endif
#ifndef Blt_MaxRequestSize
#define Blt_MaxRequestSize \
	(bltTkIntProcsPtr->blt_MaxRequestSize) /* 31 */
#endif
#ifndef Blt_GetWindowId
#define Blt_GetWindowId \
	(bltTkIntProcsPtr->blt_GetWindowId) /* 32 */
#endif
#ifndef Blt_InitXRandrConfig
#define Blt_InitXRandrConfig \
	(bltTkIntProcsPtr->blt_InitXRandrConfig) /* 33 */
#endif
#ifndef Blt_SizeOfScreen
#define Blt_SizeOfScreen \
	(bltTkIntProcsPtr->blt_SizeOfScreen) /* 34 */
#endif
#ifndef Blt_RootX
#define Blt_RootX \
	(bltTkIntProcsPtr->blt_RootX) /* 35 */
#endif
#ifndef Blt_RootY
#define Blt_RootY \
	(bltTkIntProcsPtr->blt_RootY) /* 36 */
#endif
#ifndef Blt_RootCoordinates
#define Blt_RootCoordinates \
	(bltTkIntProcsPtr->blt_RootCoordinates) /* 37 */
#endif
#ifndef Blt_MapToplevelWindow
#define Blt_MapToplevelWindow \
	(bltTkIntProcsPtr->blt_MapToplevelWindow) /* 38 */
#endif
#ifndef Blt_UnmapToplevelWindow
#define Blt_UnmapToplevelWindow \
	(bltTkIntProcsPtr->blt_UnmapToplevelWindow) /* 39 */
#endif
#ifndef Blt_RaiseToplevelWindow
#define Blt_RaiseToplevelWindow \
	(bltTkIntProcsPtr->blt_RaiseToplevelWindow) /* 40 */
#endif
#ifndef Blt_LowerToplevelWindow
#define Blt_LowerToplevelWindow \
	(bltTkIntProcsPtr->blt_LowerToplevelWindow) /* 41 */
#endif
#ifndef Blt_ResizeToplevelWindow
#define Blt_ResizeToplevelWindow \
	(bltTkIntProcsPtr->blt_ResizeToplevelWindow) /* 42 */
#endif
#ifndef Blt_MoveToplevelWindow
#define Blt_MoveToplevelWindow \
	(bltTkIntProcsPtr->blt_MoveToplevelWindow) /* 43 */
#endif
#ifndef Blt_MoveResizeToplevelWindow
#define Blt_MoveResizeToplevelWindow \
	(bltTkIntProcsPtr->blt_MoveResizeToplevelWindow) /* 44 */
#endif
#ifndef Blt_GetWindowRegion
#define Blt_GetWindowRegion \
	(bltTkIntProcsPtr->blt_GetWindowRegion) /* 45 */
#endif
#ifndef Blt_GetRootWindowSize
#define Blt_GetRootWindowSize \
	(bltTkIntProcsPtr->blt_GetRootWindowSize) /* 46 */
#endif
#ifndef Blt_GetWindowInstanceData
#define Blt_GetWindowInstanceData \
	(bltTkIntProcsPtr->blt_GetWindowInstanceData) /* 47 */
#endif
#ifndef Blt_SetWindowInstanceData
#define Blt_SetWindowInstanceData \
	(bltTkIntProcsPtr->blt_SetWindowInstanceData) /* 48 */
#endif
#ifndef Blt_DeleteWindowInstanceData
#define Blt_DeleteWindowInstanceData \
	(bltTkIntProcsPtr->blt_DeleteWindowInstanceData) /* 49 */
#endif
#ifndef Blt_ReparentWindow
#define Blt_ReparentWindow \
	(bltTkIntProcsPtr->blt_ReparentWindow) /* 50 */
#endif
#ifndef Blt_GetDrawableAttributes
#define Blt_GetDrawableAttributes \
	(bltTkIntProcsPtr->blt_GetDrawableAttributes) /* 51 */
#endif
#ifndef Blt_SetDrawableAttributes
#define Blt_SetDrawableAttributes \
	(bltTkIntProcsPtr->blt_SetDrawableAttributes) /* 52 */
#endif
#ifndef Blt_SetDrawableAttributesFromWindow
#define Blt_SetDrawableAttributesFromWindow \
	(bltTkIntProcsPtr->blt_SetDrawableAttributesFromWindow) /* 53 */
#endif
#ifndef Blt_FreeDrawableAttributes
#define Blt_FreeDrawableAttributes \
	(bltTkIntProcsPtr->blt_FreeDrawableAttributes) /* 54 */
#endif
#ifndef Blt_GetBitmapGC
#define Blt_GetBitmapGC \
	(bltTkIntProcsPtr->blt_GetBitmapGC) /* 55 */
#endif
#ifndef Blt_GetPixmapAbortOnError
#define Blt_GetPixmapAbortOnError \
	(bltTkIntProcsPtr->blt_GetPixmapAbortOnError) /* 56 */
#endif
#ifndef Blt_ScreenDPI
#define Blt_ScreenDPI \
	(bltTkIntProcsPtr->blt_ScreenDPI) /* 57 */
#endif
#ifndef Blt_OldConfigModified
#define Blt_OldConfigModified \
	(bltTkIntProcsPtr->blt_OldConfigModified) /* 58 */
#endif
#ifndef Blt_GetLineExtents
#define Blt_GetLineExtents \
	(bltTkIntProcsPtr->blt_GetLineExtents) /* 59 */
#endif
#ifndef Blt_GetBoundingBox
#define Blt_GetBoundingBox \
	(bltTkIntProcsPtr->blt_GetBoundingBox) /* 60 */
#endif
#ifndef Blt_ParseTifTags
#define Blt_ParseTifTags \
	(bltTkIntProcsPtr->blt_ParseTifTags) /* 61 */
#endif
#ifndef Blt_GetFont
#define Blt_GetFont \
	(bltTkIntProcsPtr->blt_GetFont) /* 62 */
#endif
#ifndef Blt_AllocFontFromObj
#define Blt_AllocFontFromObj \
	(bltTkIntProcsPtr->blt_AllocFontFromObj) /* 63 */
#endif
#ifndef Blt_DrawWithEllipsis
#define Blt_DrawWithEllipsis \
	(bltTkIntProcsPtr->blt_DrawWithEllipsis) /* 64 */
#endif
#ifndef Blt_GetFontFromObj
#define Blt_GetFontFromObj \
	(bltTkIntProcsPtr->blt_GetFontFromObj) /* 65 */
#endif
#ifndef Blt_Font_GetMetrics
#define Blt_Font_GetMetrics \
	(bltTkIntProcsPtr->blt_Font_GetMetrics) /* 66 */
#endif
#ifndef Blt_TextWidth
#define Blt_TextWidth \
	(bltTkIntProcsPtr->blt_TextWidth) /* 67 */
#endif
#ifndef Blt_Font_GetInterp
#define Blt_Font_GetInterp \
	(bltTkIntProcsPtr->blt_Font_GetInterp) /* 68 */
#endif
#ifndef Blt_Font_GetFile
#define Blt_Font_GetFile \
	(bltTkIntProcsPtr->blt_Font_GetFile) /* 69 */
#endif
#ifndef Blt_NewTileBrush
#define Blt_NewTileBrush \
	(bltTkIntProcsPtr->blt_NewTileBrush) /* 70 */
#endif
#ifndef Blt_NewLinearGradientBrush
#define Blt_NewLinearGradientBrush \
	(bltTkIntProcsPtr->blt_NewLinearGradientBrush) /* 71 */
#endif
#ifndef Blt_NewStripesBrush
#define Blt_NewStripesBrush \
	(bltTkIntProcsPtr->blt_NewStripesBrush) /* 72 */
#endif
#ifndef Blt_NewCheckersBrush
#define Blt_NewCheckersBrush \
	(bltTkIntProcsPtr->blt_NewCheckersBrush) /* 73 */
#endif
#ifndef Blt_NewRadialGradientBrush
#define Blt_NewRadialGradientBrush \
	(bltTkIntProcsPtr->blt_NewRadialGradientBrush) /* 74 */
#endif
#ifndef Blt_NewConicalGradientBrush
#define Blt_NewConicalGradientBrush \
	(bltTkIntProcsPtr->blt_NewConicalGradientBrush) /* 75 */
#endif
#ifndef Blt_NewColorBrush
#define Blt_NewColorBrush \
	(bltTkIntProcsPtr->blt_NewColorBrush) /* 76 */
#endif
#ifndef Blt_GetBrushType
#define Blt_GetBrushType \
	(bltTkIntProcsPtr->blt_GetBrushType) /* 77 */
#endif
#ifndef Blt_GetBrushName
#define Blt_GetBrushName \
	(bltTkIntProcsPtr->blt_GetBrushName) /* 78 */
#endif
#ifndef Blt_GetBrushColor
#define Blt_GetBrushColor \
	(bltTkIntProcsPtr->blt_GetBrushColor) /* 79 */
#endif
#ifndef Blt_ConfigurePaintBrush
#define Blt_ConfigurePaintBrush \
	(bltTkIntProcsPtr->blt_ConfigurePaintBrush) /* 80 */
#endif
#ifndef Blt_GetBrushTypeFromObj
#define Blt_GetBrushTypeFromObj \
	(bltTkIntProcsPtr->blt_GetBrushTypeFromObj) /* 81 */
#endif
#ifndef Blt_FreeBrush
#define Blt_FreeBrush \
	(bltTkIntProcsPtr->blt_FreeBrush) /* 82 */
#endif
#ifndef Blt_GetPaintBrushFromObj
#define Blt_GetPaintBrushFromObj \
	(bltTkIntProcsPtr->blt_GetPaintBrushFromObj) /* 83 */
#endif
#ifndef Blt_GetPaintBrush
#define Blt_GetPaintBrush \
	(bltTkIntProcsPtr->blt_GetPaintBrush) /* 84 */
#endif
#ifndef Blt_SetLinearGradientBrushPalette
#define Blt_SetLinearGradientBrushPalette \
	(bltTkIntProcsPtr->blt_SetLinearGradientBrushPalette) /* 85 */
#endif
#ifndef Blt_SetLinearGradientBrushCalcProc
#define Blt_SetLinearGradientBrushCalcProc \
	(bltTkIntProcsPtr->blt_SetLinearGradientBrushCalcProc) /* 86 */
#endif
#ifndef Blt_SetLinearGradientBrushColors
#define Blt_SetLinearGradientBrushColors \
	(bltTkIntProcsPtr->blt_SetLinearGradientBrushColors) /* 87 */
#endif
#ifndef Blt_SetTileBrushPicture
#define Blt_SetTileBrushPicture \
	(bltTkIntProcsPtr->blt_SetTileBrushPicture) /* 88 */
#endif
#ifndef Blt_SetColorBrushColor
#define Blt_SetColorBrushColor \
	(bltTkIntProcsPtr->blt_SetColorBrushColor) /* 89 */
#endif
#ifndef Blt_SetBrushOrigin
#define Blt_SetBrushOrigin \
	(bltTkIntProcsPtr->blt_SetBrushOrigin) /* 90 */
#endif
#ifndef Blt_SetBrushOpacity
#define Blt_SetBrushOpacity \
	(bltTkIntProcsPtr->blt_SetBrushOpacity) /* 91 */
#endif
#ifndef Blt_SetBrushRegion
#define Blt_SetBrushRegion \
	(bltTkIntProcsPtr->blt_SetBrushRegion) /* 92 */
#endif
#ifndef Blt_GetBrushAlpha
#define Blt_GetBrushAlpha \
	(bltTkIntProcsPtr->blt_GetBrushAlpha) /* 93 */
#endif
#ifndef Blt_GetBrushOrigin
#define Blt_GetBrushOrigin \
	(bltTkIntProcsPtr->blt_GetBrushOrigin) /* 94 */
#endif
#ifndef Blt_GetAssociatedColorFromBrush
#define Blt_GetAssociatedColorFromBrush \
	(bltTkIntProcsPtr->blt_GetAssociatedColorFromBrush) /* 95 */
#endif
#ifndef Blt_IsVerticalLinearBrush
#define Blt_IsVerticalLinearBrush \
	(bltTkIntProcsPtr->blt_IsVerticalLinearBrush) /* 96 */
#endif
#ifndef Blt_PaintRectangle
#define Blt_PaintRectangle \
	(bltTkIntProcsPtr->blt_PaintRectangle) /* 97 */
#endif
#ifndef Blt_PaintPolygon
#define Blt_PaintPolygon \
	(bltTkIntProcsPtr->blt_PaintPolygon) /* 98 */
#endif
#ifndef Blt_CreateBrushNotifier
#define Blt_CreateBrushNotifier \
	(bltTkIntProcsPtr->blt_CreateBrushNotifier) /* 99 */
#endif
#ifndef Blt_DeleteBrushNotifier
#define Blt_DeleteBrushNotifier \
	(bltTkIntProcsPtr->blt_DeleteBrushNotifier) /* 100 */
#endif
#ifndef Blt_GetBg
#define Blt_GetBg \
	(bltTkIntProcsPtr->blt_GetBg) /* 101 */
#endif
#ifndef Blt_GetBgFromObj
#define Blt_GetBgFromObj \
	(bltTkIntProcsPtr->blt_GetBgFromObj) /* 102 */
#endif
#ifndef Blt_Bg_BorderColor
#define Blt_Bg_BorderColor \
	(bltTkIntProcsPtr->blt_Bg_BorderColor) /* 103 */
#endif
#ifndef Blt_Bg_Border
#define Blt_Bg_Border \
	(bltTkIntProcsPtr->blt_Bg_Border) /* 104 */
#endif
#ifndef Blt_Bg_PaintBrush
#define Blt_Bg_PaintBrush \
	(bltTkIntProcsPtr->blt_Bg_PaintBrush) /* 105 */
#endif
#ifndef Blt_Bg_Name
#define Blt_Bg_Name \
	(bltTkIntProcsPtr->blt_Bg_Name) /* 106 */
#endif
#ifndef Blt_Bg_Free
#define Blt_Bg_Free \
	(bltTkIntProcsPtr->blt_Bg_Free) /* 107 */
#endif
#ifndef Blt_Bg_DrawRectangle
#define Blt_Bg_DrawRectangle \
	(bltTkIntProcsPtr->blt_Bg_DrawRectangle) /* 108 */
#endif
#ifndef Blt_Bg_FillRectangle
#define Blt_Bg_FillRectangle \
	(bltTkIntProcsPtr->blt_Bg_FillRectangle) /* 109 */
#endif
#ifndef Blt_Bg_DrawPolygon
#define Blt_Bg_DrawPolygon \
	(bltTkIntProcsPtr->blt_Bg_DrawPolygon) /* 110 */
#endif
#ifndef Blt_Bg_FillPolygon
#define Blt_Bg_FillPolygon \
	(bltTkIntProcsPtr->blt_Bg_FillPolygon) /* 111 */
#endif
#ifndef Blt_Bg_GetOrigin
#define Blt_Bg_GetOrigin \
	(bltTkIntProcsPtr->blt_Bg_GetOrigin) /* 112 */
#endif
#ifndef Blt_Bg_SetChangedProc
#define Blt_Bg_SetChangedProc \
	(bltTkIntProcsPtr->blt_Bg_SetChangedProc) /* 113 */
#endif
#ifndef Blt_Bg_SetOrigin
#define Blt_Bg_SetOrigin \
	(bltTkIntProcsPtr->blt_Bg_SetOrigin) /* 114 */
#endif
#ifndef Blt_Bg_DrawFocus
#define Blt_Bg_DrawFocus \
	(bltTkIntProcsPtr->blt_Bg_DrawFocus) /* 115 */
#endif
#ifndef Blt_Bg_BorderGC
#define Blt_Bg_BorderGC \
	(bltTkIntProcsPtr->blt_Bg_BorderGC) /* 116 */
#endif
#ifndef Blt_Bg_SetFromBackground
#define Blt_Bg_SetFromBackground \
	(bltTkIntProcsPtr->blt_Bg_SetFromBackground) /* 117 */
#endif
#ifndef Blt_Bg_UnsetClipRegion
#define Blt_Bg_UnsetClipRegion \
	(bltTkIntProcsPtr->blt_Bg_UnsetClipRegion) /* 118 */
#endif
#ifndef Blt_Bg_SetClipRegion
#define Blt_Bg_SetClipRegion \
	(bltTkIntProcsPtr->blt_Bg_SetClipRegion) /* 119 */
#endif
#ifndef Blt_Bg_GetColor
#define Blt_Bg_GetColor \
	(bltTkIntProcsPtr->blt_Bg_GetColor) /* 120 */
#endif
#ifndef Blt_3DBorder_SetClipRegion
#define Blt_3DBorder_SetClipRegion \
	(bltTkIntProcsPtr->blt_3DBorder_SetClipRegion) /* 121 */
#endif
#ifndef Blt_3DBorder_UnsetClipRegion
#define Blt_3DBorder_UnsetClipRegion \
	(bltTkIntProcsPtr->blt_3DBorder_UnsetClipRegion) /* 122 */
#endif
#ifndef Blt_DestroyBindingTable
#define Blt_DestroyBindingTable \
	(bltTkIntProcsPtr->blt_DestroyBindingTable) /* 123 */
#endif
#ifndef Blt_CreateBindingTable
#define Blt_CreateBindingTable \
	(bltTkIntProcsPtr->blt_CreateBindingTable) /* 124 */
#endif
#ifndef Blt_ConfigureBindings
#define Blt_ConfigureBindings \
	(bltTkIntProcsPtr->blt_ConfigureBindings) /* 125 */
#endif
#ifndef Blt_ConfigureBindingsFromObj
#define Blt_ConfigureBindingsFromObj \
	(bltTkIntProcsPtr->blt_ConfigureBindingsFromObj) /* 126 */
#endif
#ifndef Blt_PickCurrentItem
#define Blt_PickCurrentItem \
	(bltTkIntProcsPtr->blt_PickCurrentItem) /* 127 */
#endif
#ifndef Blt_DeleteBindings
#define Blt_DeleteBindings \
	(bltTkIntProcsPtr->blt_DeleteBindings) /* 128 */
#endif
#ifndef Blt_MoveBindingTable
#define Blt_MoveBindingTable \
	(bltTkIntProcsPtr->blt_MoveBindingTable) /* 129 */
#endif
#ifndef Blt_Afm_SetPrinting
#define Blt_Afm_SetPrinting \
	(bltTkIntProcsPtr->blt_Afm_SetPrinting) /* 130 */
#endif
#ifndef Blt_Afm_IsPrinting
#define Blt_Afm_IsPrinting \
	(bltTkIntProcsPtr->blt_Afm_IsPrinting) /* 131 */
#endif
#ifndef Blt_Afm_TextWidth
#define Blt_Afm_TextWidth \
	(bltTkIntProcsPtr->blt_Afm_TextWidth) /* 132 */
#endif
#ifndef Blt_Afm_GetMetrics
#define Blt_Afm_GetMetrics \
	(bltTkIntProcsPtr->blt_Afm_GetMetrics) /* 133 */
#endif
#ifndef Blt_Afm_GetPostscriptFamily
#define Blt_Afm_GetPostscriptFamily \
	(bltTkIntProcsPtr->blt_Afm_GetPostscriptFamily) /* 134 */
#endif
#ifndef Blt_Afm_GetPostscriptName
#define Blt_Afm_GetPostscriptName \
	(bltTkIntProcsPtr->blt_Afm_GetPostscriptName) /* 135 */
#endif
#ifndef Blt_SetDashes
#define Blt_SetDashes \
	(bltTkIntProcsPtr->blt_SetDashes) /* 136 */
#endif
#ifndef Blt_ResetLimits
#define Blt_ResetLimits \
	(bltTkIntProcsPtr->blt_ResetLimits) /* 137 */
#endif
#ifndef Blt_GetLimitsFromObj
#define Blt_GetLimitsFromObj \
	(bltTkIntProcsPtr->blt_GetLimitsFromObj) /* 138 */
#endif
#ifndef Blt_ConfigureInfoFromObj
#define Blt_ConfigureInfoFromObj \
	(bltTkIntProcsPtr->blt_ConfigureInfoFromObj) /* 139 */
#endif
#ifndef Blt_ConfigureValueFromObj
#define Blt_ConfigureValueFromObj \
	(bltTkIntProcsPtr->blt_ConfigureValueFromObj) /* 140 */
#endif
#ifndef Blt_ConfigureWidgetFromObj
#define Blt_ConfigureWidgetFromObj \
	(bltTkIntProcsPtr->blt_ConfigureWidgetFromObj) /* 141 */
#endif
#ifndef Blt_ConfigureComponentFromObj
#define Blt_ConfigureComponentFromObj \
	(bltTkIntProcsPtr->blt_ConfigureComponentFromObj) /* 142 */
#endif
#ifndef Blt_ConfigModified
#define Blt_ConfigModified \
	(bltTkIntProcsPtr->blt_ConfigModified) /* 143 */
#endif
#ifndef Blt_NameOfState
#define Blt_NameOfState \
	(bltTkIntProcsPtr->blt_NameOfState) /* 144 */
#endif
#ifndef Blt_FreeOptions
#define Blt_FreeOptions \
	(bltTkIntProcsPtr->blt_FreeOptions) /* 145 */
#endif
#ifndef Blt_ObjIsOption
#define Blt_ObjIsOption \
	(bltTkIntProcsPtr->blt_ObjIsOption) /* 146 */
#endif
#ifndef Blt_GetPixelsFromObj
#define Blt_GetPixelsFromObj \
	(bltTkIntProcsPtr->blt_GetPixelsFromObj) /* 147 */
#endif
#ifndef Blt_GetPadFromObj
#define Blt_GetPadFromObj \
	(bltTkIntProcsPtr->blt_GetPadFromObj) /* 148 */
#endif
#ifndef Blt_GetStateFromObj
#define Blt_GetStateFromObj \
	(bltTkIntProcsPtr->blt_GetStateFromObj) /* 149 */
#endif
#ifndef Blt_GetFillFromObj
#define Blt_GetFillFromObj \
	(bltTkIntProcsPtr->blt_GetFillFromObj) /* 150 */
#endif
#ifndef Blt_GetResizeFromObj
#define Blt_GetResizeFromObj \
	(bltTkIntProcsPtr->blt_GetResizeFromObj) /* 151 */
#endif
#ifndef Blt_GetDashesFromObj
#define Blt_GetDashesFromObj \
	(bltTkIntProcsPtr->blt_GetDashesFromObj) /* 152 */
#endif
#ifndef Blt_Image_IsDeleted
#define Blt_Image_IsDeleted \
	(bltTkIntProcsPtr->blt_Image_IsDeleted) /* 153 */
#endif
#ifndef Blt_Image_GetMaster
#define Blt_Image_GetMaster \
	(bltTkIntProcsPtr->blt_Image_GetMaster) /* 154 */
#endif
#ifndef Blt_Image_GetType
#define Blt_Image_GetType \
	(bltTkIntProcsPtr->blt_Image_GetType) /* 155 */
#endif
#ifndef Blt_Image_GetInstanceData
#define Blt_Image_GetInstanceData \
	(bltTkIntProcsPtr->blt_Image_GetInstanceData) /* 156 */
#endif
#ifndef Blt_Image_GetMasterData
#define Blt_Image_GetMasterData \
	(bltTkIntProcsPtr->blt_Image_GetMasterData) /* 157 */
#endif
#ifndef Blt_Image_Name
#define Blt_Image_Name \
	(bltTkIntProcsPtr->blt_Image_Name) /* 158 */
#endif
#ifndef Blt_Image_NameOfType
#define Blt_Image_NameOfType \
	(bltTkIntProcsPtr->blt_Image_NameOfType) /* 159 */
#endif
#ifndef Blt_FreePainter
#define Blt_FreePainter \
	(bltTkIntProcsPtr->blt_FreePainter) /* 160 */
#endif
#ifndef Blt_GetPainter
#define Blt_GetPainter \
	(bltTkIntProcsPtr->blt_GetPainter) /* 161 */
#endif
#ifndef Blt_GetPainterFromDrawable
#define Blt_GetPainterFromDrawable \
	(bltTkIntProcsPtr->blt_GetPainterFromDrawable) /* 162 */
#endif
#ifndef Blt_PainterGC
#define Blt_PainterGC \
	(bltTkIntProcsPtr->blt_PainterGC) /* 163 */
#endif
#ifndef Blt_PainterDepth
#define Blt_PainterDepth \
	(bltTkIntProcsPtr->blt_PainterDepth) /* 164 */
#endif
#ifndef Blt_SetPainterClipRegion
#define Blt_SetPainterClipRegion \
	(bltTkIntProcsPtr->blt_SetPainterClipRegion) /* 165 */
#endif
#ifndef Blt_UnsetPainterClipRegion
#define Blt_UnsetPainterClipRegion \
	(bltTkIntProcsPtr->blt_UnsetPainterClipRegion) /* 166 */
#endif
#ifndef Blt_PaintPicture
#define Blt_PaintPicture \
	(bltTkIntProcsPtr->blt_PaintPicture) /* 167 */
#endif
#ifndef Blt_PaintPictureWithBlend
#define Blt_PaintPictureWithBlend \
	(bltTkIntProcsPtr->blt_PaintPictureWithBlend) /* 168 */
#endif
#ifndef Blt_PaintCheckbox
#define Blt_PaintCheckbox \
	(bltTkIntProcsPtr->blt_PaintCheckbox) /* 169 */
#endif
#ifndef Blt_PaintRadioButton
#define Blt_PaintRadioButton \
	(bltTkIntProcsPtr->blt_PaintRadioButton) /* 170 */
#endif
#ifndef Blt_PaintRadioButtonOld
#define Blt_PaintRadioButtonOld \
	(bltTkIntProcsPtr->blt_PaintRadioButtonOld) /* 171 */
#endif
#ifndef Blt_PaintDelete
#define Blt_PaintDelete \
	(bltTkIntProcsPtr->blt_PaintDelete) /* 172 */
#endif
#ifndef Blt_Ps_Create
#define Blt_Ps_Create \
	(bltTkIntProcsPtr->blt_Ps_Create) /* 173 */
#endif
#ifndef Blt_Ps_Free
#define Blt_Ps_Free \
	(bltTkIntProcsPtr->blt_Ps_Free) /* 174 */
#endif
#ifndef Blt_Ps_GetValue
#define Blt_Ps_GetValue \
	(bltTkIntProcsPtr->blt_Ps_GetValue) /* 175 */
#endif
#ifndef Blt_Ps_GetDBuffer
#define Blt_Ps_GetDBuffer \
	(bltTkIntProcsPtr->blt_Ps_GetDBuffer) /* 176 */
#endif
#ifndef Blt_Ps_GetInterp
#define Blt_Ps_GetInterp \
	(bltTkIntProcsPtr->blt_Ps_GetInterp) /* 177 */
#endif
#ifndef Blt_Ps_GetScratchBuffer
#define Blt_Ps_GetScratchBuffer \
	(bltTkIntProcsPtr->blt_Ps_GetScratchBuffer) /* 178 */
#endif
#ifndef Blt_Ps_SetInterp
#define Blt_Ps_SetInterp \
	(bltTkIntProcsPtr->blt_Ps_SetInterp) /* 179 */
#endif
#ifndef Blt_Ps_Append
#define Blt_Ps_Append \
	(bltTkIntProcsPtr->blt_Ps_Append) /* 180 */
#endif
#ifndef Blt_Ps_AppendBytes
#define Blt_Ps_AppendBytes \
	(bltTkIntProcsPtr->blt_Ps_AppendBytes) /* 181 */
#endif
#ifndef Blt_Ps_VarAppend
#define Blt_Ps_VarAppend \
	(bltTkIntProcsPtr->blt_Ps_VarAppend) /* 182 */
#endif
#ifndef Blt_Ps_Format
#define Blt_Ps_Format \
	(bltTkIntProcsPtr->blt_Ps_Format) /* 183 */
#endif
#ifndef Blt_Ps_SetClearBackground
#define Blt_Ps_SetClearBackground \
	(bltTkIntProcsPtr->blt_Ps_SetClearBackground) /* 184 */
#endif
#ifndef Blt_Ps_IncludeFile
#define Blt_Ps_IncludeFile \
	(bltTkIntProcsPtr->blt_Ps_IncludeFile) /* 185 */
#endif
#ifndef Blt_Ps_GetPicaFromObj
#define Blt_Ps_GetPicaFromObj \
	(bltTkIntProcsPtr->blt_Ps_GetPicaFromObj) /* 186 */
#endif
#ifndef Blt_Ps_GetPadFromObj
#define Blt_Ps_GetPadFromObj \
	(bltTkIntProcsPtr->blt_Ps_GetPadFromObj) /* 187 */
#endif
#ifndef Blt_Ps_ComputeBoundingBox
#define Blt_Ps_ComputeBoundingBox \
	(bltTkIntProcsPtr->blt_Ps_ComputeBoundingBox) /* 188 */
#endif
#ifndef Blt_Ps_DrawPicture
#define Blt_Ps_DrawPicture \
	(bltTkIntProcsPtr->blt_Ps_DrawPicture) /* 189 */
#endif
#ifndef Blt_Ps_Rectangle
#define Blt_Ps_Rectangle \
	(bltTkIntProcsPtr->blt_Ps_Rectangle) /* 190 */
#endif
#ifndef Blt_Ps_Rectangle2
#define Blt_Ps_Rectangle2 \
	(bltTkIntProcsPtr->blt_Ps_Rectangle2) /* 191 */
#endif
#ifndef Blt_Ps_SaveFile
#define Blt_Ps_SaveFile \
	(bltTkIntProcsPtr->blt_Ps_SaveFile) /* 192 */
#endif
#ifndef Blt_Ps_XSetLineWidth
#define Blt_Ps_XSetLineWidth \
	(bltTkIntProcsPtr->blt_Ps_XSetLineWidth) /* 193 */
#endif
#ifndef Blt_Ps_XSetBackground
#define Blt_Ps_XSetBackground \
	(bltTkIntProcsPtr->blt_Ps_XSetBackground) /* 194 */
#endif
#ifndef Blt_Ps_XSetBitmapData
#define Blt_Ps_XSetBitmapData \
	(bltTkIntProcsPtr->blt_Ps_XSetBitmapData) /* 195 */
#endif
#ifndef Blt_Ps_XSetForeground
#define Blt_Ps_XSetForeground \
	(bltTkIntProcsPtr->blt_Ps_XSetForeground) /* 196 */
#endif
#ifndef Blt_Ps_XSetFont
#define Blt_Ps_XSetFont \
	(bltTkIntProcsPtr->blt_Ps_XSetFont) /* 197 */
#endif
#ifndef Blt_Ps_XSetDashes
#define Blt_Ps_XSetDashes \
	(bltTkIntProcsPtr->blt_Ps_XSetDashes) /* 198 */
#endif
#ifndef Blt_Ps_XSetLineAttributes
#define Blt_Ps_XSetLineAttributes \
	(bltTkIntProcsPtr->blt_Ps_XSetLineAttributes) /* 199 */
#endif
#ifndef Blt_Ps_XSetStipple
#define Blt_Ps_XSetStipple \
	(bltTkIntProcsPtr->blt_Ps_XSetStipple) /* 200 */
#endif
#ifndef Blt_Ps_Polyline
#define Blt_Ps_Polyline \
	(bltTkIntProcsPtr->blt_Ps_Polyline) /* 201 */
#endif
#ifndef Blt_Ps_XDrawLines
#define Blt_Ps_XDrawLines \
	(bltTkIntProcsPtr->blt_Ps_XDrawLines) /* 202 */
#endif
#ifndef Blt_Ps_XDrawSegments
#define Blt_Ps_XDrawSegments \
	(bltTkIntProcsPtr->blt_Ps_XDrawSegments) /* 203 */
#endif
#ifndef Blt_Ps_DrawPolyline
#define Blt_Ps_DrawPolyline \
	(bltTkIntProcsPtr->blt_Ps_DrawPolyline) /* 204 */
#endif
#ifndef Blt_Ps_DrawSegments2d
#define Blt_Ps_DrawSegments2d \
	(bltTkIntProcsPtr->blt_Ps_DrawSegments2d) /* 205 */
#endif
#ifndef Blt_Ps_Draw3DRectangle
#define Blt_Ps_Draw3DRectangle \
	(bltTkIntProcsPtr->blt_Ps_Draw3DRectangle) /* 206 */
#endif
#ifndef Blt_Ps_Fill3DRectangle
#define Blt_Ps_Fill3DRectangle \
	(bltTkIntProcsPtr->blt_Ps_Fill3DRectangle) /* 207 */
#endif
#ifndef Blt_Ps_XFillRectangle
#define Blt_Ps_XFillRectangle \
	(bltTkIntProcsPtr->blt_Ps_XFillRectangle) /* 208 */
#endif
#ifndef Blt_Ps_XFillRectangles
#define Blt_Ps_XFillRectangles \
	(bltTkIntProcsPtr->blt_Ps_XFillRectangles) /* 209 */
#endif
#ifndef Blt_Ps_XFillPolygon
#define Blt_Ps_XFillPolygon \
	(bltTkIntProcsPtr->blt_Ps_XFillPolygon) /* 210 */
#endif
#ifndef Blt_Ps_DrawPhoto
#define Blt_Ps_DrawPhoto \
	(bltTkIntProcsPtr->blt_Ps_DrawPhoto) /* 211 */
#endif
#ifndef Blt_Ps_XDrawWindow
#define Blt_Ps_XDrawWindow \
	(bltTkIntProcsPtr->blt_Ps_XDrawWindow) /* 212 */
#endif
#ifndef Blt_Ps_DrawText
#define Blt_Ps_DrawText \
	(bltTkIntProcsPtr->blt_Ps_DrawText) /* 213 */
#endif
#ifndef Blt_Ps_DrawBitmap
#define Blt_Ps_DrawBitmap \
	(bltTkIntProcsPtr->blt_Ps_DrawBitmap) /* 214 */
#endif
#ifndef Blt_Ps_XSetCapStyle
#define Blt_Ps_XSetCapStyle \
	(bltTkIntProcsPtr->blt_Ps_XSetCapStyle) /* 215 */
#endif
#ifndef Blt_Ps_XSetJoinStyle
#define Blt_Ps_XSetJoinStyle \
	(bltTkIntProcsPtr->blt_Ps_XSetJoinStyle) /* 216 */
#endif
#ifndef Blt_Ps_PolylineFromXPoints
#define Blt_Ps_PolylineFromXPoints \
	(bltTkIntProcsPtr->blt_Ps_PolylineFromXPoints) /* 217 */
#endif
#ifndef Blt_Ps_Polygon
#define Blt_Ps_Polygon \
	(bltTkIntProcsPtr->blt_Ps_Polygon) /* 218 */
#endif
#ifndef Blt_Ps_SetPrinting
#define Blt_Ps_SetPrinting \
	(bltTkIntProcsPtr->blt_Ps_SetPrinting) /* 219 */
#endif
#ifndef Blt_Ts_CreateLayout
#define Blt_Ts_CreateLayout \
	(bltTkIntProcsPtr->blt_Ts_CreateLayout) /* 220 */
#endif
#ifndef Blt_Ts_TitleLayout
#define Blt_Ts_TitleLayout \
	(bltTkIntProcsPtr->blt_Ts_TitleLayout) /* 221 */
#endif
#ifndef Blt_Ts_DrawLayout
#define Blt_Ts_DrawLayout \
	(bltTkIntProcsPtr->blt_Ts_DrawLayout) /* 222 */
#endif
#ifndef Blt_Ts_GetExtents
#define Blt_Ts_GetExtents \
	(bltTkIntProcsPtr->blt_Ts_GetExtents) /* 223 */
#endif
#ifndef Blt_Ts_ResetStyle
#define Blt_Ts_ResetStyle \
	(bltTkIntProcsPtr->blt_Ts_ResetStyle) /* 224 */
#endif
#ifndef Blt_Ts_FreeStyle
#define Blt_Ts_FreeStyle \
	(bltTkIntProcsPtr->blt_Ts_FreeStyle) /* 225 */
#endif
#ifndef Blt_Ts_SetDrawStyle
#define Blt_Ts_SetDrawStyle \
	(bltTkIntProcsPtr->blt_Ts_SetDrawStyle) /* 226 */
#endif
#ifndef Blt_DrawText
#define Blt_DrawText \
	(bltTkIntProcsPtr->blt_DrawText) /* 227 */
#endif
#ifndef Blt_DrawText2
#define Blt_DrawText2 \
	(bltTkIntProcsPtr->blt_DrawText2) /* 228 */
#endif
#ifndef Blt_Ts_Bitmap
#define Blt_Ts_Bitmap \
	(bltTkIntProcsPtr->blt_Ts_Bitmap) /* 229 */
#endif
#ifndef Blt_DrawTextWithRotatedFont
#define Blt_DrawTextWithRotatedFont \
	(bltTkIntProcsPtr->blt_DrawTextWithRotatedFont) /* 230 */
#endif
#ifndef Blt_DrawLayout
#define Blt_DrawLayout \
	(bltTkIntProcsPtr->blt_DrawLayout) /* 231 */
#endif
#ifndef Blt_GetTextExtents
#define Blt_GetTextExtents \
	(bltTkIntProcsPtr->blt_GetTextExtents) /* 232 */
#endif
#ifndef Blt_RotateStartingTextPositions
#define Blt_RotateStartingTextPositions \
	(bltTkIntProcsPtr->blt_RotateStartingTextPositions) /* 233 */
#endif
#ifndef Blt_ComputeTextLayout
#define Blt_ComputeTextLayout \
	(bltTkIntProcsPtr->blt_ComputeTextLayout) /* 234 */
#endif
#ifndef Blt_DrawTextLayout
#define Blt_DrawTextLayout \
	(bltTkIntProcsPtr->blt_DrawTextLayout) /* 235 */
#endif
#ifndef Blt_CharBbox
#define Blt_CharBbox \
	(bltTkIntProcsPtr->blt_CharBbox) /* 236 */
#endif
#ifndef Blt_UnderlineTextLayout
#define Blt_UnderlineTextLayout \
	(bltTkIntProcsPtr->blt_UnderlineTextLayout) /* 237 */
#endif
#ifndef Blt_Ts_UnderlineLayout
#define Blt_Ts_UnderlineLayout \
	(bltTkIntProcsPtr->blt_Ts_UnderlineLayout) /* 238 */
#endif
#ifndef Blt_Ts_DrawText
#define Blt_Ts_DrawText \
	(bltTkIntProcsPtr->blt_Ts_DrawText) /* 239 */
#endif
#ifndef Blt_MeasureText
#define Blt_MeasureText \
	(bltTkIntProcsPtr->blt_MeasureText) /* 240 */
#endif
#ifndef Blt_FreeTextLayout
#define Blt_FreeTextLayout \
	(bltTkIntProcsPtr->blt_FreeTextLayout) /* 241 */
#endif

#endif /* defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TK_PROCS) */

/* !END!: Do not edit above this line. */

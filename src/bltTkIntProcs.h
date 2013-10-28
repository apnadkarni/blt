/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "bltTkInt.h"
#include "bltFont.h"
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
#ifndef Blt_GetParentWindow_DECLARED
#define Blt_GetParentWindow_DECLARED
/* 12 */
BLT_EXTERN Window	Blt_GetParentWindow(Display *display, Window window);
#endif
#ifndef Blt_FindChild_DECLARED
#define Blt_FindChild_DECLARED
/* 13 */
BLT_EXTERN Tk_Window	Blt_FindChild(Tk_Window parent, char *name);
#endif
#ifndef Blt_FirstChild_DECLARED
#define Blt_FirstChild_DECLARED
/* 14 */
BLT_EXTERN Tk_Window	Blt_FirstChild(Tk_Window parent);
#endif
#ifndef Blt_NextChild_DECLARED
#define Blt_NextChild_DECLARED
/* 15 */
BLT_EXTERN Tk_Window	Blt_NextChild(Tk_Window tkwin);
#endif
#ifndef Blt_RelinkWindow_DECLARED
#define Blt_RelinkWindow_DECLARED
/* 16 */
BLT_EXTERN void		Blt_RelinkWindow(Tk_Window tkwin,
				Tk_Window newParent, int x, int y);
#endif
#ifndef Blt_Toplevel_DECLARED
#define Blt_Toplevel_DECLARED
/* 17 */
BLT_EXTERN Tk_Window	Blt_Toplevel(Tk_Window tkwin);
#endif
#ifndef Blt_GetPixels_DECLARED
#define Blt_GetPixels_DECLARED
/* 18 */
BLT_EXTERN int		Blt_GetPixels(Tcl_Interp *interp, Tk_Window tkwin,
				const char *string, int check, int *valuePtr);
#endif
#ifndef Blt_NameOfFill_DECLARED
#define Blt_NameOfFill_DECLARED
/* 19 */
BLT_EXTERN const char *	 Blt_NameOfFill(int fill);
#endif
#ifndef Blt_NameOfResize_DECLARED
#define Blt_NameOfResize_DECLARED
/* 20 */
BLT_EXTERN const char *	 Blt_NameOfResize(int resize);
#endif
#ifndef Blt_GetXY_DECLARED
#define Blt_GetXY_DECLARED
/* 21 */
BLT_EXTERN int		Blt_GetXY(Tcl_Interp *interp, Tk_Window tkwin,
				const char *string, int *xPtr, int *yPtr);
#endif
#ifndef Blt_GetProjection_DECLARED
#define Blt_GetProjection_DECLARED
/* 22 */
BLT_EXTERN Point2d	Blt_GetProjection(int x, int y, Point2d *p,
				Point2d *q);
#endif
#ifndef Blt_DrawArrowOld_DECLARED
#define Blt_DrawArrowOld_DECLARED
/* 23 */
BLT_EXTERN void		Blt_DrawArrowOld(Display *display, Drawable drawable,
				GC gc, int x, int y, int w, int h,
				int borderWidth, int orientation);
#endif
#ifndef Blt_DrawArrow_DECLARED
#define Blt_DrawArrow_DECLARED
/* 24 */
BLT_EXTERN void		Blt_DrawArrow(Display *display, Drawable drawable,
				XColor *color, int x, int y, int w, int h,
				int borderWidth, int orientation);
#endif
#ifndef Blt_MakeTransparentWindowExist_DECLARED
#define Blt_MakeTransparentWindowExist_DECLARED
/* 25 */
BLT_EXTERN void		Blt_MakeTransparentWindowExist(Tk_Window tkwin,
				Window parent, int isBusy);
#endif
#ifndef Blt_TranslateAnchor_DECLARED
#define Blt_TranslateAnchor_DECLARED
/* 26 */
BLT_EXTERN void		Blt_TranslateAnchor(int x, int y, int width,
				int height, Tk_Anchor anchor, int *transXPtr,
				int *transYPtr);
#endif
#ifndef Blt_AnchorPoint_DECLARED
#define Blt_AnchorPoint_DECLARED
/* 27 */
BLT_EXTERN Point2d	Blt_AnchorPoint(double x, double y, double width,
				double height, Tk_Anchor anchor);
#endif
#ifndef Blt_MaxRequestSize_DECLARED
#define Blt_MaxRequestSize_DECLARED
/* 28 */
BLT_EXTERN long		Blt_MaxRequestSize(Display *display, size_t elemSize);
#endif
#ifndef Blt_GetWindowId_DECLARED
#define Blt_GetWindowId_DECLARED
/* 29 */
BLT_EXTERN Window	Blt_GetWindowId(Tk_Window tkwin);
#endif
#ifndef Blt_InitXRandrConfig_DECLARED
#define Blt_InitXRandrConfig_DECLARED
/* 30 */
BLT_EXTERN void		Blt_InitXRandrConfig(Tcl_Interp *interp);
#endif
#ifndef Blt_SizeOfScreen_DECLARED
#define Blt_SizeOfScreen_DECLARED
/* 31 */
BLT_EXTERN void		Blt_SizeOfScreen(Tk_Window tkwin, int *widthPtr,
				int *heightPtr);
#endif
#ifndef Blt_RootX_DECLARED
#define Blt_RootX_DECLARED
/* 32 */
BLT_EXTERN int		Blt_RootX(Tk_Window tkwin);
#endif
#ifndef Blt_RootY_DECLARED
#define Blt_RootY_DECLARED
/* 33 */
BLT_EXTERN int		Blt_RootY(Tk_Window tkwin);
#endif
#ifndef Blt_RootCoordinates_DECLARED
#define Blt_RootCoordinates_DECLARED
/* 34 */
BLT_EXTERN void		Blt_RootCoordinates(Tk_Window tkwin, int x, int y,
				int *rootXPtr, int *rootYPtr);
#endif
#ifndef Blt_MapToplevelWindow_DECLARED
#define Blt_MapToplevelWindow_DECLARED
/* 35 */
BLT_EXTERN void		Blt_MapToplevelWindow(Tk_Window tkwin);
#endif
#ifndef Blt_UnmapToplevelWindow_DECLARED
#define Blt_UnmapToplevelWindow_DECLARED
/* 36 */
BLT_EXTERN void		Blt_UnmapToplevelWindow(Tk_Window tkwin);
#endif
#ifndef Blt_RaiseToplevelWindow_DECLARED
#define Blt_RaiseToplevelWindow_DECLARED
/* 37 */
BLT_EXTERN void		Blt_RaiseToplevelWindow(Tk_Window tkwin);
#endif
#ifndef Blt_LowerToplevelWindow_DECLARED
#define Blt_LowerToplevelWindow_DECLARED
/* 38 */
BLT_EXTERN void		Blt_LowerToplevelWindow(Tk_Window tkwin);
#endif
#ifndef Blt_ResizeToplevelWindow_DECLARED
#define Blt_ResizeToplevelWindow_DECLARED
/* 39 */
BLT_EXTERN void		Blt_ResizeToplevelWindow(Tk_Window tkwin, int w,
				int h);
#endif
#ifndef Blt_MoveToplevelWindow_DECLARED
#define Blt_MoveToplevelWindow_DECLARED
/* 40 */
BLT_EXTERN void		Blt_MoveToplevelWindow(Tk_Window tkwin, int x, int y);
#endif
#ifndef Blt_MoveResizeToplevelWindow_DECLARED
#define Blt_MoveResizeToplevelWindow_DECLARED
/* 41 */
BLT_EXTERN void		Blt_MoveResizeToplevelWindow(Tk_Window tkwin, int x,
				int y, int w, int h);
#endif
#ifndef Blt_GetWindowRegion_DECLARED
#define Blt_GetWindowRegion_DECLARED
/* 42 */
BLT_EXTERN int		Blt_GetWindowRegion(Display *display, Window window,
				int *xPtr, int *yPtr, int *widthPtr,
				int *heightPtr);
#endif
#ifndef Blt_GetWindowInstanceData_DECLARED
#define Blt_GetWindowInstanceData_DECLARED
/* 43 */
BLT_EXTERN ClientData	Blt_GetWindowInstanceData(Tk_Window tkwin);
#endif
#ifndef Blt_SetWindowInstanceData_DECLARED
#define Blt_SetWindowInstanceData_DECLARED
/* 44 */
BLT_EXTERN void		Blt_SetWindowInstanceData(Tk_Window tkwin,
				ClientData instanceData);
#endif
#ifndef Blt_DeleteWindowInstanceData_DECLARED
#define Blt_DeleteWindowInstanceData_DECLARED
/* 45 */
BLT_EXTERN void		Blt_DeleteWindowInstanceData(Tk_Window tkwin);
#endif
#ifndef Blt_ReparentWindow_DECLARED
#define Blt_ReparentWindow_DECLARED
/* 46 */
BLT_EXTERN int		Blt_ReparentWindow(Display *display, Window window,
				Window newParent, int x, int y);
#endif
#ifndef Blt_GetDrawableAttribs_DECLARED
#define Blt_GetDrawableAttribs_DECLARED
/* 47 */
BLT_EXTERN Blt_DrawableAttributes * Blt_GetDrawableAttribs(Display *display,
				Drawable drawable);
#endif
#ifndef Blt_SetDrawableAttribs_DECLARED
#define Blt_SetDrawableAttribs_DECLARED
/* 48 */
BLT_EXTERN void		Blt_SetDrawableAttribs(Display *display,
				Drawable drawable, int width, int height,
				int depth, Colormap colormap, Visual *visual);
#endif
#ifndef Blt_SetDrawableAttribsFromWindow_DECLARED
#define Blt_SetDrawableAttribsFromWindow_DECLARED
/* 49 */
BLT_EXTERN void		Blt_SetDrawableAttribsFromWindow(Tk_Window tkwin,
				Drawable drawable);
#endif
#ifndef Blt_FreeDrawableAttribs_DECLARED
#define Blt_FreeDrawableAttribs_DECLARED
/* 50 */
BLT_EXTERN void		Blt_FreeDrawableAttribs(Display *display,
				Drawable drawable);
#endif
#ifndef Blt_GetBitmapGC_DECLARED
#define Blt_GetBitmapGC_DECLARED
/* 51 */
BLT_EXTERN GC		Blt_GetBitmapGC(Tk_Window tkwin);
#endif
#ifndef Blt_GetPixmapAbortOnError_DECLARED
#define Blt_GetPixmapAbortOnError_DECLARED
/* 52 */
BLT_EXTERN Pixmap	Blt_GetPixmapAbortOnError(Display *dpy,
				Drawable draw, int w, int h, int depth,
				int lineNum, const char *fileName);
#endif
#ifndef Blt_ScreenDPI_DECLARED
#define Blt_ScreenDPI_DECLARED
/* 53 */
BLT_EXTERN void		Blt_ScreenDPI(Tk_Window tkwin, int *xPtr, int *yPtr);
#endif
#ifndef Blt_OldConfigModified_DECLARED
#define Blt_OldConfigModified_DECLARED
/* 54 */
BLT_EXTERN int		Blt_OldConfigModified(Tk_ConfigSpec *specs, ...);
#endif
#ifndef Blt_GetLineExtents_DECLARED
#define Blt_GetLineExtents_DECLARED
/* 55 */
BLT_EXTERN void		Blt_GetLineExtents(size_t numPoints, Point2d *points,
				Region2d *r);
#endif
#ifndef Blt_GetBoundingBox_DECLARED
#define Blt_GetBoundingBox_DECLARED
/* 56 */
BLT_EXTERN void		Blt_GetBoundingBox(int width, int height,
				float angle, double *widthPtr,
				double *heightPtr, Point2d *points);
#endif
#ifndef Blt_GetFont_DECLARED
#define Blt_GetFont_DECLARED
/* 57 */
BLT_EXTERN Blt_Font	Blt_GetFont(Tcl_Interp *interp, Tk_Window tkwin,
				const char *string);
#endif
#ifndef Blt_AllocFontFromObj_DECLARED
#define Blt_AllocFontFromObj_DECLARED
/* 58 */
BLT_EXTERN Blt_Font	Blt_AllocFontFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Tcl_Obj *objPtr);
#endif
#ifndef Blt_DrawWithEllipsis_DECLARED
#define Blt_DrawWithEllipsis_DECLARED
/* 59 */
BLT_EXTERN void		Blt_DrawWithEllipsis(Tk_Window tkwin,
				Drawable drawable, GC gc, Blt_Font font,
				int depth, float angle, const char *string,
				int numBytes, int x, int y, int maxLength);
#endif
#ifndef Blt_GetFontFromObj_DECLARED
#define Blt_GetFontFromObj_DECLARED
/* 60 */
BLT_EXTERN Blt_Font	Blt_GetFontFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Tcl_Obj *objPtr);
#endif
#ifndef Blt_Font_GetMetrics_DECLARED
#define Blt_Font_GetMetrics_DECLARED
/* 61 */
BLT_EXTERN void		Blt_Font_GetMetrics(Blt_Font font,
				Blt_FontMetrics *fmPtr);
#endif
#ifndef Blt_TextWidth_DECLARED
#define Blt_TextWidth_DECLARED
/* 62 */
BLT_EXTERN int		Blt_TextWidth(Blt_Font font, const char *string,
				int length);
#endif
#ifndef Blt_Font_GetInterp_DECLARED
#define Blt_Font_GetInterp_DECLARED
/* 63 */
BLT_EXTERN Tcl_Interp *	 Blt_Font_GetInterp(Blt_Font font);
#endif
#ifndef Blt_Font_GetFile_DECLARED
#define Blt_Font_GetFile_DECLARED
/* 64 */
BLT_EXTERN Tcl_Obj *	Blt_Font_GetFile(Tcl_Interp *interp, Tcl_Obj *objPtr,
				double *sizePtr);
#endif
#ifndef Blt_GetBg_DECLARED
#define Blt_GetBg_DECLARED
/* 65 */
BLT_EXTERN Blt_Bg	Blt_GetBg(Tcl_Interp *interp, Tk_Window tkwin,
				const char *styleName);
#endif
#ifndef Blt_GetBgFromObj_DECLARED
#define Blt_GetBgFromObj_DECLARED
/* 66 */
BLT_EXTERN Blt_Bg	Blt_GetBgFromObj(Tcl_Interp *interp, Tk_Window tkwin,
				Tcl_Obj *objPtr);
#endif
#ifndef Blt_Bg_BorderColor_DECLARED
#define Blt_Bg_BorderColor_DECLARED
/* 67 */
BLT_EXTERN XColor *	Blt_Bg_BorderColor(Blt_Bg bg);
#endif
#ifndef Blt_Bg_Border_DECLARED
#define Blt_Bg_Border_DECLARED
/* 68 */
BLT_EXTERN Tk_3DBorder	Blt_Bg_Border(Blt_Bg bg);
#endif
#ifndef Blt_Bg_Name_DECLARED
#define Blt_Bg_Name_DECLARED
/* 69 */
BLT_EXTERN const char *	 Blt_Bg_Name(Blt_Bg bg);
#endif
#ifndef Blt_FreeBg_DECLARED
#define Blt_FreeBg_DECLARED
/* 70 */
BLT_EXTERN void		Blt_FreeBg(Blt_Bg bg);
#endif
#ifndef Blt_Bg_DrawRectangle_DECLARED
#define Blt_Bg_DrawRectangle_DECLARED
/* 71 */
BLT_EXTERN void		Blt_Bg_DrawRectangle(Tk_Window tkwin,
				Drawable drawable, Blt_Bg bg, int x, int y,
				int width, int height, int borderWidth,
				int relief);
#endif
#ifndef Blt_Bg_FillRectangle_DECLARED
#define Blt_Bg_FillRectangle_DECLARED
/* 72 */
BLT_EXTERN void		Blt_Bg_FillRectangle(Tk_Window tkwin,
				Drawable drawable, Blt_Bg bg, int x, int y,
				int width, int height, int borderWidth,
				int relief);
#endif
#ifndef Blt_Bg_DrawPolygon_DECLARED
#define Blt_Bg_DrawPolygon_DECLARED
/* 73 */
BLT_EXTERN void		Blt_Bg_DrawPolygon(Tk_Window tkwin,
				Drawable drawable, Blt_Bg bg, XPoint *points,
				int numPoints, int borderWidth,
				int leftRelief);
#endif
#ifndef Blt_Bg_FillPolygon_DECLARED
#define Blt_Bg_FillPolygon_DECLARED
/* 74 */
BLT_EXTERN void		Blt_Bg_FillPolygon(Tk_Window tkwin,
				Drawable drawable, Blt_Bg bg, XPoint *points,
				int numPoints, int borderWidth,
				int leftRelief);
#endif
#ifndef Blt_Bg_GetOrigin_DECLARED
#define Blt_Bg_GetOrigin_DECLARED
/* 75 */
BLT_EXTERN void		Blt_Bg_GetOrigin(Blt_Bg bg, int *xPtr, int *yPtr);
#endif
#ifndef Blt_Bg_SetChangedProc_DECLARED
#define Blt_Bg_SetChangedProc_DECLARED
/* 76 */
BLT_EXTERN void		Blt_Bg_SetChangedProc(Blt_Bg bg,
				Blt_Bg_ChangedProc *notifyProc,
				ClientData clientData);
#endif
#ifndef Blt_Bg_SetOrigin_DECLARED
#define Blt_Bg_SetOrigin_DECLARED
/* 77 */
BLT_EXTERN void		Blt_Bg_SetOrigin(Tk_Window tkwin, Blt_Bg bg, int x,
				int y);
#endif
#ifndef Blt_Bg_DrawFocus_DECLARED
#define Blt_Bg_DrawFocus_DECLARED
/* 78 */
BLT_EXTERN void		Blt_Bg_DrawFocus(Tk_Window tkwin, Blt_Bg bg,
				int highlightWidth, Drawable drawable);
#endif
#ifndef Blt_Bg_BorderGC_DECLARED
#define Blt_Bg_BorderGC_DECLARED
/* 79 */
BLT_EXTERN GC		Blt_Bg_BorderGC(Tk_Window tkwin, Blt_Bg bg,
				int which);
#endif
#ifndef Blt_Bg_SetFromBackground_DECLARED
#define Blt_Bg_SetFromBackground_DECLARED
/* 80 */
BLT_EXTERN void		Blt_Bg_SetFromBackground(Tk_Window tkwin, Blt_Bg bg);
#endif
#ifndef Blt_Bg_UnsetClipRegion_DECLARED
#define Blt_Bg_UnsetClipRegion_DECLARED
/* 81 */
BLT_EXTERN void		Blt_Bg_UnsetClipRegion(Tk_Window tkwin, Blt_Bg bg);
#endif
#ifndef Blt_Bg_SetClipRegion_DECLARED
#define Blt_Bg_SetClipRegion_DECLARED
/* 82 */
BLT_EXTERN void		Blt_Bg_SetClipRegion(Tk_Window tkwin, Blt_Bg bg,
				TkRegion rgn);
#endif
#ifndef Blt_DestroyBindingTable_DECLARED
#define Blt_DestroyBindingTable_DECLARED
/* 83 */
BLT_EXTERN void		Blt_DestroyBindingTable(Blt_BindTable table);
#endif
#ifndef Blt_CreateBindingTable_DECLARED
#define Blt_CreateBindingTable_DECLARED
/* 84 */
BLT_EXTERN Blt_BindTable Blt_CreateBindingTable(Tcl_Interp *interp,
				Tk_Window tkwin, ClientData clientData,
				Blt_BindPickProc *pickProc,
				Blt_BindAppendTagsProc *tagProc);
#endif
#ifndef Blt_ConfigureBindings_DECLARED
#define Blt_ConfigureBindings_DECLARED
/* 85 */
BLT_EXTERN int		Blt_ConfigureBindings(Tcl_Interp *interp,
				Blt_BindTable table, ClientData item,
				int argc, const char **argv);
#endif
#ifndef Blt_ConfigureBindingsFromObj_DECLARED
#define Blt_ConfigureBindingsFromObj_DECLARED
/* 86 */
BLT_EXTERN int		Blt_ConfigureBindingsFromObj(Tcl_Interp *interp,
				Blt_BindTable table, ClientData item,
				int objc, Tcl_Obj *const *objv);
#endif
#ifndef Blt_PickCurrentItem_DECLARED
#define Blt_PickCurrentItem_DECLARED
/* 87 */
BLT_EXTERN void		Blt_PickCurrentItem(Blt_BindTable table);
#endif
#ifndef Blt_DeleteBindings_DECLARED
#define Blt_DeleteBindings_DECLARED
/* 88 */
BLT_EXTERN void		Blt_DeleteBindings(Blt_BindTable table,
				ClientData object);
#endif
#ifndef Blt_MoveBindingTable_DECLARED
#define Blt_MoveBindingTable_DECLARED
/* 89 */
BLT_EXTERN void		Blt_MoveBindingTable(Blt_BindTable table,
				Tk_Window tkwin);
#endif
#ifndef Blt_Afm_SetPrinting_DECLARED
#define Blt_Afm_SetPrinting_DECLARED
/* 90 */
BLT_EXTERN void		Blt_Afm_SetPrinting(Tcl_Interp *interp, int value);
#endif
#ifndef Blt_Afm_IsPrinting_DECLARED
#define Blt_Afm_IsPrinting_DECLARED
/* 91 */
BLT_EXTERN int		Blt_Afm_IsPrinting(void );
#endif
#ifndef Blt_Afm_TextWidth_DECLARED
#define Blt_Afm_TextWidth_DECLARED
/* 92 */
BLT_EXTERN int		Blt_Afm_TextWidth(Blt_Font font, const char *s,
				int numBytes);
#endif
#ifndef Blt_Afm_GetMetrics_DECLARED
#define Blt_Afm_GetMetrics_DECLARED
/* 93 */
BLT_EXTERN int		Blt_Afm_GetMetrics(Blt_Font font,
				Blt_FontMetrics *fmPtr);
#endif
#ifndef Blt_Afm_GetPostscriptFamily_DECLARED
#define Blt_Afm_GetPostscriptFamily_DECLARED
/* 94 */
BLT_EXTERN const char *	 Blt_Afm_GetPostscriptFamily(const char *family);
#endif
#ifndef Blt_Afm_GetPostscriptName_DECLARED
#define Blt_Afm_GetPostscriptName_DECLARED
/* 95 */
BLT_EXTERN void		Blt_Afm_GetPostscriptName(const char *family,
				int flags, Tcl_DString *resultPtr);
#endif
#ifndef Blt_SetDashes_DECLARED
#define Blt_SetDashes_DECLARED
/* 96 */
BLT_EXTERN void		Blt_SetDashes(Display *display, GC gc,
				Blt_Dashes *dashesPtr);
#endif
#ifndef Blt_ResetLimits_DECLARED
#define Blt_ResetLimits_DECLARED
/* 97 */
BLT_EXTERN void		Blt_ResetLimits(Blt_Limits *limitsPtr);
#endif
#ifndef Blt_GetLimitsFromObj_DECLARED
#define Blt_GetLimitsFromObj_DECLARED
/* 98 */
BLT_EXTERN int		Blt_GetLimitsFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Tcl_Obj *objPtr,
				Blt_Limits *limitsPtr);
#endif
#ifndef Blt_ConfigureInfoFromObj_DECLARED
#define Blt_ConfigureInfoFromObj_DECLARED
/* 99 */
BLT_EXTERN int		Blt_ConfigureInfoFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Blt_ConfigSpec *specs,
				char *widgRec, Tcl_Obj *objPtr, int flags);
#endif
#ifndef Blt_ConfigureValueFromObj_DECLARED
#define Blt_ConfigureValueFromObj_DECLARED
/* 100 */
BLT_EXTERN int		Blt_ConfigureValueFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Blt_ConfigSpec *specs,
				char *widgRec, Tcl_Obj *objPtr, int flags);
#endif
#ifndef Blt_ConfigureWidgetFromObj_DECLARED
#define Blt_ConfigureWidgetFromObj_DECLARED
/* 101 */
BLT_EXTERN int		Blt_ConfigureWidgetFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Blt_ConfigSpec *specs,
				int objc, Tcl_Obj *const *objv,
				char *widgRec, int flags);
#endif
#ifndef Blt_ConfigureComponentFromObj_DECLARED
#define Blt_ConfigureComponentFromObj_DECLARED
/* 102 */
BLT_EXTERN int		Blt_ConfigureComponentFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, const char *name,
				const char *className, Blt_ConfigSpec *specs,
				int objc, Tcl_Obj *const *objv,
				char *widgRec, int flags);
#endif
#ifndef Blt_ConfigModified_DECLARED
#define Blt_ConfigModified_DECLARED
/* 103 */
BLT_EXTERN int		Blt_ConfigModified(Blt_ConfigSpec *specs, ...);
#endif
#ifndef Blt_NameOfState_DECLARED
#define Blt_NameOfState_DECLARED
/* 104 */
BLT_EXTERN const char *	 Blt_NameOfState(int state);
#endif
#ifndef Blt_FreeOptions_DECLARED
#define Blt_FreeOptions_DECLARED
/* 105 */
BLT_EXTERN void		Blt_FreeOptions(Blt_ConfigSpec *specs, char *widgRec,
				Display *display, int needFlags);
#endif
#ifndef Blt_ObjIsOption_DECLARED
#define Blt_ObjIsOption_DECLARED
/* 106 */
BLT_EXTERN int		Blt_ObjIsOption(Blt_ConfigSpec *specs,
				Tcl_Obj *objPtr, int flags);
#endif
#ifndef Blt_GetPixelsFromObj_DECLARED
#define Blt_GetPixelsFromObj_DECLARED
/* 107 */
BLT_EXTERN int		Blt_GetPixelsFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Tcl_Obj *objPtr, int flags,
				int *valuePtr);
#endif
#ifndef Blt_GetPadFromObj_DECLARED
#define Blt_GetPadFromObj_DECLARED
/* 108 */
BLT_EXTERN int		Blt_GetPadFromObj(Tcl_Interp *interp,
				Tk_Window tkwin, Tcl_Obj *objPtr,
				Blt_Pad *padPtr);
#endif
#ifndef Blt_GetStateFromObj_DECLARED
#define Blt_GetStateFromObj_DECLARED
/* 109 */
BLT_EXTERN int		Blt_GetStateFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *statePtr);
#endif
#ifndef Blt_GetFillFromObj_DECLARED
#define Blt_GetFillFromObj_DECLARED
/* 110 */
BLT_EXTERN int		Blt_GetFillFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *fillPtr);
#endif
#ifndef Blt_GetResizeFromObj_DECLARED
#define Blt_GetResizeFromObj_DECLARED
/* 111 */
BLT_EXTERN int		Blt_GetResizeFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *fillPtr);
#endif
#ifndef Blt_GetDashesFromObj_DECLARED
#define Blt_GetDashesFromObj_DECLARED
/* 112 */
BLT_EXTERN int		Blt_GetDashesFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_Dashes *dashesPtr);
#endif
#ifndef Blt_Image_IsDeleted_DECLARED
#define Blt_Image_IsDeleted_DECLARED
/* 113 */
BLT_EXTERN int		Blt_Image_IsDeleted(Tk_Image tkImage);
#endif
#ifndef Blt_Image_GetMaster_DECLARED
#define Blt_Image_GetMaster_DECLARED
/* 114 */
BLT_EXTERN Tk_ImageMaster Blt_Image_GetMaster(Tk_Image tkImage);
#endif
#ifndef Blt_Image_GetType_DECLARED
#define Blt_Image_GetType_DECLARED
/* 115 */
BLT_EXTERN Tk_ImageType * Blt_Image_GetType(Tk_Image tkImage);
#endif
#ifndef Blt_Image_GetInstanceData_DECLARED
#define Blt_Image_GetInstanceData_DECLARED
/* 116 */
BLT_EXTERN ClientData	Blt_Image_GetInstanceData(Tk_Image tkImage);
#endif
#ifndef Blt_Image_Name_DECLARED
#define Blt_Image_Name_DECLARED
/* 117 */
BLT_EXTERN const char *	 Blt_Image_Name(Tk_Image tkImage);
#endif
#ifndef Blt_Image_NameOfType_DECLARED
#define Blt_Image_NameOfType_DECLARED
/* 118 */
BLT_EXTERN const char *	 Blt_Image_NameOfType(Tk_Image tkImage);
#endif
#ifndef Blt_FreePainter_DECLARED
#define Blt_FreePainter_DECLARED
/* 119 */
BLT_EXTERN void		Blt_FreePainter(Blt_Painter painter);
#endif
#ifndef Blt_GetPainter_DECLARED
#define Blt_GetPainter_DECLARED
/* 120 */
BLT_EXTERN Blt_Painter	Blt_GetPainter(Tk_Window tkwin, float gamma);
#endif
#ifndef Blt_GetPainterFromDrawable_DECLARED
#define Blt_GetPainterFromDrawable_DECLARED
/* 121 */
BLT_EXTERN Blt_Painter	Blt_GetPainterFromDrawable(Display *display,
				Drawable drawable, float gamma);
#endif
#ifndef Blt_PainterGC_DECLARED
#define Blt_PainterGC_DECLARED
/* 122 */
BLT_EXTERN GC		Blt_PainterGC(Blt_Painter painter);
#endif
#ifndef Blt_PainterDepth_DECLARED
#define Blt_PainterDepth_DECLARED
/* 123 */
BLT_EXTERN int		Blt_PainterDepth(Blt_Painter painter);
#endif
#ifndef Blt_PaintPicture_DECLARED
#define Blt_PaintPicture_DECLARED
/* 124 */
BLT_EXTERN int		Blt_PaintPicture(Blt_Painter painter,
				Drawable drawable, Blt_Picture src, int srcX,
				int srcY, int width, int height, int destX,
				int destY, unsigned int flags);
#endif
#ifndef Blt_PaintPictureWithBlend_DECLARED
#define Blt_PaintPictureWithBlend_DECLARED
/* 125 */
BLT_EXTERN int		Blt_PaintPictureWithBlend(Blt_Painter painter,
				Drawable drawable, Blt_Picture src, int srcX,
				int srcY, int width, int height, int destX,
				int destY, unsigned int flags);
#endif
#ifndef Blt_PaintCheckbox_DECLARED
#define Blt_PaintCheckbox_DECLARED
/* 126 */
BLT_EXTERN Blt_Picture	Blt_PaintCheckbox(int width, int height,
				XColor *fillColor, XColor *outlineColor,
				XColor *checkColor, int isOn);
#endif
#ifndef Blt_PaintRadioButton_DECLARED
#define Blt_PaintRadioButton_DECLARED
/* 127 */
BLT_EXTERN Blt_Picture	Blt_PaintRadioButton(int width, int height,
				Blt_Bg bg, XColor *fill, XColor *outline,
				int isOn);
#endif
#ifndef Blt_PaintRadioButtonOld_DECLARED
#define Blt_PaintRadioButtonOld_DECLARED
/* 128 */
BLT_EXTERN Blt_Picture	Blt_PaintRadioButtonOld(int width, int height,
				XColor *bg, XColor *fill, XColor *outline,
				XColor *check, int isOn);
#endif
#ifndef Blt_PaintDelete_DECLARED
#define Blt_PaintDelete_DECLARED
/* 129 */
BLT_EXTERN Blt_Picture	Blt_PaintDelete(int width, int height,
				XColor *bgColor, XColor *fillColor,
				XColor *symColor, int isActive);
#endif
#ifndef Blt_Ps_Create_DECLARED
#define Blt_Ps_Create_DECLARED
/* 130 */
BLT_EXTERN Blt_Ps	Blt_Ps_Create(Tcl_Interp *interp,
				PageSetup *setupPtr);
#endif
#ifndef Blt_Ps_Free_DECLARED
#define Blt_Ps_Free_DECLARED
/* 131 */
BLT_EXTERN void		Blt_Ps_Free(Blt_Ps ps);
#endif
#ifndef Blt_Ps_GetValue_DECLARED
#define Blt_Ps_GetValue_DECLARED
/* 132 */
BLT_EXTERN const char *	 Blt_Ps_GetValue(Blt_Ps ps, int *lengthPtr);
#endif
#ifndef Blt_Ps_GetInterp_DECLARED
#define Blt_Ps_GetInterp_DECLARED
/* 133 */
BLT_EXTERN Tcl_Interp *	 Blt_Ps_GetInterp(Blt_Ps ps);
#endif
#ifndef Blt_Ps_GetDString_DECLARED
#define Blt_Ps_GetDString_DECLARED
/* 134 */
BLT_EXTERN Tcl_DString * Blt_Ps_GetDString(Blt_Ps ps);
#endif
#ifndef Blt_Ps_GetScratchBuffer_DECLARED
#define Blt_Ps_GetScratchBuffer_DECLARED
/* 135 */
BLT_EXTERN char *	Blt_Ps_GetScratchBuffer(Blt_Ps ps);
#endif
#ifndef Blt_Ps_SetInterp_DECLARED
#define Blt_Ps_SetInterp_DECLARED
/* 136 */
BLT_EXTERN void		Blt_Ps_SetInterp(Blt_Ps ps, Tcl_Interp *interp);
#endif
#ifndef Blt_Ps_Append_DECLARED
#define Blt_Ps_Append_DECLARED
/* 137 */
BLT_EXTERN void		Blt_Ps_Append(Blt_Ps ps, const char *string);
#endif
#ifndef Blt_Ps_AppendBytes_DECLARED
#define Blt_Ps_AppendBytes_DECLARED
/* 138 */
BLT_EXTERN void		Blt_Ps_AppendBytes(Blt_Ps ps, const char *string,
				int numBytes);
#endif
#ifndef Blt_Ps_VarAppend_DECLARED
#define Blt_Ps_VarAppend_DECLARED
/* 139 */
BLT_EXTERN void		Blt_Ps_VarAppend(Blt_Ps ps, ...);
#endif
#ifndef Blt_Ps_Format_DECLARED
#define Blt_Ps_Format_DECLARED
/* 140 */
BLT_EXTERN void		Blt_Ps_Format(Blt_Ps ps, const char *fmt, ...);
#endif
#ifndef Blt_Ps_SetClearBackground_DECLARED
#define Blt_Ps_SetClearBackground_DECLARED
/* 141 */
BLT_EXTERN void		Blt_Ps_SetClearBackground(Blt_Ps ps);
#endif
#ifndef Blt_Ps_IncludeFile_DECLARED
#define Blt_Ps_IncludeFile_DECLARED
/* 142 */
BLT_EXTERN int		Blt_Ps_IncludeFile(Tcl_Interp *interp, Blt_Ps ps,
				const char *fileName);
#endif
#ifndef Blt_Ps_GetPicaFromObj_DECLARED
#define Blt_Ps_GetPicaFromObj_DECLARED
/* 143 */
BLT_EXTERN int		Blt_Ps_GetPicaFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *picaPtr);
#endif
#ifndef Blt_Ps_GetPadFromObj_DECLARED
#define Blt_Ps_GetPadFromObj_DECLARED
/* 144 */
BLT_EXTERN int		Blt_Ps_GetPadFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_Pad *padPtr);
#endif
#ifndef Blt_Ps_ComputeBoundingBox_DECLARED
#define Blt_Ps_ComputeBoundingBox_DECLARED
/* 145 */
BLT_EXTERN int		Blt_Ps_ComputeBoundingBox(PageSetup *setupPtr, int w,
				int h);
#endif
#ifndef Blt_Ps_DrawPicture_DECLARED
#define Blt_Ps_DrawPicture_DECLARED
/* 146 */
BLT_EXTERN void		Blt_Ps_DrawPicture(Blt_Ps ps, Blt_Picture picture,
				double x, double y);
#endif
#ifndef Blt_Ps_Rectangle_DECLARED
#define Blt_Ps_Rectangle_DECLARED
/* 147 */
BLT_EXTERN void		Blt_Ps_Rectangle(Blt_Ps ps, int x, int y, int w,
				int h);
#endif
#ifndef Blt_Ps_SaveFile_DECLARED
#define Blt_Ps_SaveFile_DECLARED
/* 148 */
BLT_EXTERN int		Blt_Ps_SaveFile(Tcl_Interp *interp, Blt_Ps ps,
				const char *fileName);
#endif
#ifndef Blt_Ps_XSetLineWidth_DECLARED
#define Blt_Ps_XSetLineWidth_DECLARED
/* 149 */
BLT_EXTERN void		Blt_Ps_XSetLineWidth(Blt_Ps ps, int lineWidth);
#endif
#ifndef Blt_Ps_XSetBackground_DECLARED
#define Blt_Ps_XSetBackground_DECLARED
/* 150 */
BLT_EXTERN void		Blt_Ps_XSetBackground(Blt_Ps ps, XColor *colorPtr);
#endif
#ifndef Blt_Ps_XSetBitmapData_DECLARED
#define Blt_Ps_XSetBitmapData_DECLARED
/* 151 */
BLT_EXTERN void		Blt_Ps_XSetBitmapData(Blt_Ps ps, Display *display,
				Pixmap bitmap, int width, int height);
#endif
#ifndef Blt_Ps_XSetForeground_DECLARED
#define Blt_Ps_XSetForeground_DECLARED
/* 152 */
BLT_EXTERN void		Blt_Ps_XSetForeground(Blt_Ps ps, XColor *colorPtr);
#endif
#ifndef Blt_Ps_XSetFont_DECLARED
#define Blt_Ps_XSetFont_DECLARED
/* 153 */
BLT_EXTERN void		Blt_Ps_XSetFont(Blt_Ps ps, Blt_Font font);
#endif
#ifndef Blt_Ps_XSetDashes_DECLARED
#define Blt_Ps_XSetDashes_DECLARED
/* 154 */
BLT_EXTERN void		Blt_Ps_XSetDashes(Blt_Ps ps, Blt_Dashes *dashesPtr);
#endif
#ifndef Blt_Ps_XSetLineAttributes_DECLARED
#define Blt_Ps_XSetLineAttributes_DECLARED
/* 155 */
BLT_EXTERN void		Blt_Ps_XSetLineAttributes(Blt_Ps ps,
				XColor *colorPtr, int lineWidth,
				Blt_Dashes *dashesPtr, int capStyle,
				int joinStyle);
#endif
#ifndef Blt_Ps_XSetStipple_DECLARED
#define Blt_Ps_XSetStipple_DECLARED
/* 156 */
BLT_EXTERN void		Blt_Ps_XSetStipple(Blt_Ps ps, Display *display,
				Pixmap bitmap);
#endif
#ifndef Blt_Ps_Polyline_DECLARED
#define Blt_Ps_Polyline_DECLARED
/* 157 */
BLT_EXTERN void		Blt_Ps_Polyline(Blt_Ps ps, int n, Point2d *points);
#endif
#ifndef Blt_Ps_XDrawLines_DECLARED
#define Blt_Ps_XDrawLines_DECLARED
/* 158 */
BLT_EXTERN void		Blt_Ps_XDrawLines(Blt_Ps ps, int n, XPoint *points);
#endif
#ifndef Blt_Ps_XDrawSegments_DECLARED
#define Blt_Ps_XDrawSegments_DECLARED
/* 159 */
BLT_EXTERN void		Blt_Ps_XDrawSegments(Blt_Ps ps, int n,
				XSegment *segments);
#endif
#ifndef Blt_Ps_DrawPolyline_DECLARED
#define Blt_Ps_DrawPolyline_DECLARED
/* 160 */
BLT_EXTERN void		Blt_Ps_DrawPolyline(Blt_Ps ps, int n,
				Point2d *points);
#endif
#ifndef Blt_Ps_DrawSegments2d_DECLARED
#define Blt_Ps_DrawSegments2d_DECLARED
/* 161 */
BLT_EXTERN void		Blt_Ps_DrawSegments2d(Blt_Ps ps, int n,
				Segment2d *segments);
#endif
#ifndef Blt_Ps_Draw3DRectangle_DECLARED
#define Blt_Ps_Draw3DRectangle_DECLARED
/* 162 */
BLT_EXTERN void		Blt_Ps_Draw3DRectangle(Blt_Ps ps, Tk_3DBorder border,
				double x, double y, int width, int height,
				int borderWidth, int relief);
#endif
#ifndef Blt_Ps_Fill3DRectangle_DECLARED
#define Blt_Ps_Fill3DRectangle_DECLARED
/* 163 */
BLT_EXTERN void		Blt_Ps_Fill3DRectangle(Blt_Ps ps, Tk_3DBorder border,
				double x, double y, int width, int height,
				int borderWidth, int relief);
#endif
#ifndef Blt_Ps_XFillRectangle_DECLARED
#define Blt_Ps_XFillRectangle_DECLARED
/* 164 */
BLT_EXTERN void		Blt_Ps_XFillRectangle(Blt_Ps ps, double x, double y,
				int width, int height);
#endif
#ifndef Blt_Ps_XFillRectangles_DECLARED
#define Blt_Ps_XFillRectangles_DECLARED
/* 165 */
BLT_EXTERN void		Blt_Ps_XFillRectangles(Blt_Ps ps, int n,
				XRectangle *rects);
#endif
#ifndef Blt_Ps_XFillPolygon_DECLARED
#define Blt_Ps_XFillPolygon_DECLARED
/* 166 */
BLT_EXTERN void		Blt_Ps_XFillPolygon(Blt_Ps ps, int n,
				Point2d *points);
#endif
#ifndef Blt_Ps_DrawPhoto_DECLARED
#define Blt_Ps_DrawPhoto_DECLARED
/* 167 */
BLT_EXTERN void		Blt_Ps_DrawPhoto(Blt_Ps ps,
				Tk_PhotoHandle photoToken, double x,
				double y);
#endif
#ifndef Blt_Ps_XDrawWindow_DECLARED
#define Blt_Ps_XDrawWindow_DECLARED
/* 168 */
BLT_EXTERN void		Blt_Ps_XDrawWindow(Blt_Ps ps, Tk_Window tkwin,
				double x, double y);
#endif
#ifndef Blt_Ps_DrawText_DECLARED
#define Blt_Ps_DrawText_DECLARED
/* 169 */
BLT_EXTERN void		Blt_Ps_DrawText(Blt_Ps ps, const char *string,
				TextStyle *attrPtr, double x, double y);
#endif
#ifndef Blt_Ps_DrawBitmap_DECLARED
#define Blt_Ps_DrawBitmap_DECLARED
/* 170 */
BLT_EXTERN void		Blt_Ps_DrawBitmap(Blt_Ps ps, Display *display,
				Pixmap bitmap, double scaleX, double scaleY);
#endif
#ifndef Blt_Ps_XSetCapStyle_DECLARED
#define Blt_Ps_XSetCapStyle_DECLARED
/* 171 */
BLT_EXTERN void		Blt_Ps_XSetCapStyle(Blt_Ps ps, int capStyle);
#endif
#ifndef Blt_Ps_XSetJoinStyle_DECLARED
#define Blt_Ps_XSetJoinStyle_DECLARED
/* 172 */
BLT_EXTERN void		Blt_Ps_XSetJoinStyle(Blt_Ps ps, int joinStyle);
#endif
#ifndef Blt_Ps_PolylineFromXPoints_DECLARED
#define Blt_Ps_PolylineFromXPoints_DECLARED
/* 173 */
BLT_EXTERN void		Blt_Ps_PolylineFromXPoints(Blt_Ps ps, int n,
				XPoint *points);
#endif
#ifndef Blt_Ps_Polygon_DECLARED
#define Blt_Ps_Polygon_DECLARED
/* 174 */
BLT_EXTERN void		Blt_Ps_Polygon(Blt_Ps ps, Point2d *points,
				int numPoints);
#endif
#ifndef Blt_Ps_SetPrinting_DECLARED
#define Blt_Ps_SetPrinting_DECLARED
/* 175 */
BLT_EXTERN void		Blt_Ps_SetPrinting(Blt_Ps ps, int value);
#endif
#ifndef Blt_Ts_CreateLayout_DECLARED
#define Blt_Ts_CreateLayout_DECLARED
/* 176 */
BLT_EXTERN TextLayout *	 Blt_Ts_CreateLayout(const char *string, int length,
				TextStyle *tsPtr);
#endif
#ifndef Blt_Ts_TitleLayout_DECLARED
#define Blt_Ts_TitleLayout_DECLARED
/* 177 */
BLT_EXTERN TextLayout *	 Blt_Ts_TitleLayout(const char *string, int length,
				TextStyle *tsPtr);
#endif
#ifndef Blt_Ts_DrawLayout_DECLARED
#define Blt_Ts_DrawLayout_DECLARED
/* 178 */
BLT_EXTERN void		Blt_Ts_DrawLayout(Tk_Window tkwin, Drawable drawable,
				TextLayout *textPtr, TextStyle *tsPtr, int x,
				int y);
#endif
#ifndef Blt_Ts_GetExtents_DECLARED
#define Blt_Ts_GetExtents_DECLARED
/* 179 */
BLT_EXTERN void		Blt_Ts_GetExtents(TextStyle *tsPtr, const char *text,
				unsigned int *widthPtr,
				unsigned int *heightPtr);
#endif
#ifndef Blt_Ts_ResetStyle_DECLARED
#define Blt_Ts_ResetStyle_DECLARED
/* 180 */
BLT_EXTERN void		Blt_Ts_ResetStyle(Tk_Window tkwin, TextStyle *tsPtr);
#endif
#ifndef Blt_Ts_FreeStyle_DECLARED
#define Blt_Ts_FreeStyle_DECLARED
/* 181 */
BLT_EXTERN void		Blt_Ts_FreeStyle(Display *display, TextStyle *tsPtr);
#endif
#ifndef Blt_Ts_SetDrawStyle_DECLARED
#define Blt_Ts_SetDrawStyle_DECLARED
/* 182 */
BLT_EXTERN void		Blt_Ts_SetDrawStyle(TextStyle *tsPtr, Blt_Font font,
				GC gc, XColor *fgColor, float angle,
				Tk_Anchor anchor, Tk_Justify justify,
				int leader);
#endif
#ifndef Blt_DrawText_DECLARED
#define Blt_DrawText_DECLARED
/* 183 */
BLT_EXTERN void		Blt_DrawText(Tk_Window tkwin, Drawable drawable,
				const char *string, TextStyle *tsPtr, int x,
				int y);
#endif
#ifndef Blt_DrawText2_DECLARED
#define Blt_DrawText2_DECLARED
/* 184 */
BLT_EXTERN void		Blt_DrawText2(Tk_Window tkwin, Drawable drawable,
				const char *string, TextStyle *tsPtr, int x,
				int y, Dim2d *dimPtr);
#endif
#ifndef Blt_Ts_Bitmap_DECLARED
#define Blt_Ts_Bitmap_DECLARED
/* 185 */
BLT_EXTERN Pixmap	Blt_Ts_Bitmap(Tk_Window tkwin, TextLayout *textPtr,
				TextStyle *tsPtr, int *widthPtr,
				int *heightPtr);
#endif
#ifndef Blt_DrawTextWithRotatedFont_DECLARED
#define Blt_DrawTextWithRotatedFont_DECLARED
/* 186 */
BLT_EXTERN int		Blt_DrawTextWithRotatedFont(Tk_Window tkwin,
				Drawable drawable, float angle,
				TextStyle *tsPtr, TextLayout *textPtr, int x,
				int y);
#endif
#ifndef Blt_DrawLayout_DECLARED
#define Blt_DrawLayout_DECLARED
/* 187 */
BLT_EXTERN void		Blt_DrawLayout(Tk_Window tkwin, Drawable drawable,
				GC gc, Blt_Font font, int depth, float angle,
				int x, int y, TextLayout *layoutPtr,
				int maxLength);
#endif
#ifndef Blt_GetTextExtents_DECLARED
#define Blt_GetTextExtents_DECLARED
/* 188 */
BLT_EXTERN void		Blt_GetTextExtents(Blt_Font font, int leader,
				const char *text, int textLen,
				unsigned int *widthPtr,
				unsigned int *heightPtr);
#endif
#ifndef Blt_RotateStartingTextPositions_DECLARED
#define Blt_RotateStartingTextPositions_DECLARED
/* 189 */
BLT_EXTERN void		Blt_RotateStartingTextPositions(TextLayout *textPtr,
				float angle);
#endif
#ifndef Blt_ComputeTextLayout_DECLARED
#define Blt_ComputeTextLayout_DECLARED
/* 190 */
BLT_EXTERN Tk_TextLayout Blt_ComputeTextLayout(Blt_Font font,
				const char *string, int numChars,
				int wrapLength, Tk_Justify justify,
				int flags, int *widthPtr, int *heightPtr);
#endif
#ifndef Blt_DrawTextLayout_DECLARED
#define Blt_DrawTextLayout_DECLARED
/* 191 */
BLT_EXTERN void		Blt_DrawTextLayout(Display *display,
				Drawable drawable, GC gc,
				Tk_TextLayout layout, int x, int y,
				int firstChar, int lastChar);
#endif
#ifndef Blt_CharBbox_DECLARED
#define Blt_CharBbox_DECLARED
/* 192 */
BLT_EXTERN int		Blt_CharBbox(Tk_TextLayout layout, int index,
				int *xPtr, int *yPtr, int *widthPtr,
				int *heightPtr);
#endif
#ifndef Blt_UnderlineTextLayout_DECLARED
#define Blt_UnderlineTextLayout_DECLARED
/* 193 */
BLT_EXTERN void		Blt_UnderlineTextLayout(Display *display,
				Drawable drawable, GC gc,
				Tk_TextLayout layout, int x, int y,
				int underline);
#endif
#ifndef Blt_Ts_UnderlineLayout_DECLARED
#define Blt_Ts_UnderlineLayout_DECLARED
/* 194 */
BLT_EXTERN void		Blt_Ts_UnderlineLayout(Tk_Window tkwin,
				Drawable drawable, TextLayout *layoutPtr,
				TextStyle *tsPtr, int x, int y);
#endif
#ifndef Blt_Ts_DrawText_DECLARED
#define Blt_Ts_DrawText_DECLARED
/* 195 */
BLT_EXTERN void		Blt_Ts_DrawText(Tk_Window tkwin, Drawable drawable,
				const char *text, int textLen,
				TextStyle *tsPtr, int x, int y);
#endif
#ifndef Blt_MeasureText_DECLARED
#define Blt_MeasureText_DECLARED
/* 196 */
BLT_EXTERN int		Blt_MeasureText(Blt_Font font, const char *text,
				int textLen, int maxLength, int *nBytesPtr);
#endif
#ifndef Blt_FreeTextLayout_DECLARED
#define Blt_FreeTextLayout_DECLARED
/* 197 */
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
    Window (*blt_GetParentWindow) (Display *display, Window window); /* 12 */
    Tk_Window (*blt_FindChild) (Tk_Window parent, char *name); /* 13 */
    Tk_Window (*blt_FirstChild) (Tk_Window parent); /* 14 */
    Tk_Window (*blt_NextChild) (Tk_Window tkwin); /* 15 */
    void (*blt_RelinkWindow) (Tk_Window tkwin, Tk_Window newParent, int x, int y); /* 16 */
    Tk_Window (*blt_Toplevel) (Tk_Window tkwin); /* 17 */
    int (*blt_GetPixels) (Tcl_Interp *interp, Tk_Window tkwin, const char *string, int check, int *valuePtr); /* 18 */
    const char * (*blt_NameOfFill) (int fill); /* 19 */
    const char * (*blt_NameOfResize) (int resize); /* 20 */
    int (*blt_GetXY) (Tcl_Interp *interp, Tk_Window tkwin, const char *string, int *xPtr, int *yPtr); /* 21 */
    Point2d (*blt_GetProjection) (int x, int y, Point2d *p, Point2d *q); /* 22 */
    void (*blt_DrawArrowOld) (Display *display, Drawable drawable, GC gc, int x, int y, int w, int h, int borderWidth, int orientation); /* 23 */
    void (*blt_DrawArrow) (Display *display, Drawable drawable, XColor *color, int x, int y, int w, int h, int borderWidth, int orientation); /* 24 */
    void (*blt_MakeTransparentWindowExist) (Tk_Window tkwin, Window parent, int isBusy); /* 25 */
    void (*blt_TranslateAnchor) (int x, int y, int width, int height, Tk_Anchor anchor, int *transXPtr, int *transYPtr); /* 26 */
    Point2d (*blt_AnchorPoint) (double x, double y, double width, double height, Tk_Anchor anchor); /* 27 */
    long (*blt_MaxRequestSize) (Display *display, size_t elemSize); /* 28 */
    Window (*blt_GetWindowId) (Tk_Window tkwin); /* 29 */
    void (*blt_InitXRandrConfig) (Tcl_Interp *interp); /* 30 */
    void (*blt_SizeOfScreen) (Tk_Window tkwin, int *widthPtr, int *heightPtr); /* 31 */
    int (*blt_RootX) (Tk_Window tkwin); /* 32 */
    int (*blt_RootY) (Tk_Window tkwin); /* 33 */
    void (*blt_RootCoordinates) (Tk_Window tkwin, int x, int y, int *rootXPtr, int *rootYPtr); /* 34 */
    void (*blt_MapToplevelWindow) (Tk_Window tkwin); /* 35 */
    void (*blt_UnmapToplevelWindow) (Tk_Window tkwin); /* 36 */
    void (*blt_RaiseToplevelWindow) (Tk_Window tkwin); /* 37 */
    void (*blt_LowerToplevelWindow) (Tk_Window tkwin); /* 38 */
    void (*blt_ResizeToplevelWindow) (Tk_Window tkwin, int w, int h); /* 39 */
    void (*blt_MoveToplevelWindow) (Tk_Window tkwin, int x, int y); /* 40 */
    void (*blt_MoveResizeToplevelWindow) (Tk_Window tkwin, int x, int y, int w, int h); /* 41 */
    int (*blt_GetWindowRegion) (Display *display, Window window, int *xPtr, int *yPtr, int *widthPtr, int *heightPtr); /* 42 */
    ClientData (*blt_GetWindowInstanceData) (Tk_Window tkwin); /* 43 */
    void (*blt_SetWindowInstanceData) (Tk_Window tkwin, ClientData instanceData); /* 44 */
    void (*blt_DeleteWindowInstanceData) (Tk_Window tkwin); /* 45 */
    int (*blt_ReparentWindow) (Display *display, Window window, Window newParent, int x, int y); /* 46 */
    Blt_DrawableAttributes * (*blt_GetDrawableAttribs) (Display *display, Drawable drawable); /* 47 */
    void (*blt_SetDrawableAttribs) (Display *display, Drawable drawable, int width, int height, int depth, Colormap colormap, Visual *visual); /* 48 */
    void (*blt_SetDrawableAttribsFromWindow) (Tk_Window tkwin, Drawable drawable); /* 49 */
    void (*blt_FreeDrawableAttribs) (Display *display, Drawable drawable); /* 50 */
    GC (*blt_GetBitmapGC) (Tk_Window tkwin); /* 51 */
    Pixmap (*blt_GetPixmapAbortOnError) (Display *dpy, Drawable draw, int w, int h, int depth, int lineNum, const char *fileName); /* 52 */
    void (*blt_ScreenDPI) (Tk_Window tkwin, int *xPtr, int *yPtr); /* 53 */
    int (*blt_OldConfigModified) (Tk_ConfigSpec *specs, ...); /* 54 */
    void (*blt_GetLineExtents) (size_t numPoints, Point2d *points, Region2d *r); /* 55 */
    void (*blt_GetBoundingBox) (int width, int height, float angle, double *widthPtr, double *heightPtr, Point2d *points); /* 56 */
    Blt_Font (*blt_GetFont) (Tcl_Interp *interp, Tk_Window tkwin, const char *string); /* 57 */
    Blt_Font (*blt_AllocFontFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr); /* 58 */
    void (*blt_DrawWithEllipsis) (Tk_Window tkwin, Drawable drawable, GC gc, Blt_Font font, int depth, float angle, const char *string, int numBytes, int x, int y, int maxLength); /* 59 */
    Blt_Font (*blt_GetFontFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr); /* 60 */
    void (*blt_Font_GetMetrics) (Blt_Font font, Blt_FontMetrics *fmPtr); /* 61 */
    int (*blt_TextWidth) (Blt_Font font, const char *string, int length); /* 62 */
    Tcl_Interp * (*blt_Font_GetInterp) (Blt_Font font); /* 63 */
    Tcl_Obj * (*blt_Font_GetFile) (Tcl_Interp *interp, Tcl_Obj *objPtr, double *sizePtr); /* 64 */
    Blt_Bg (*blt_GetBg) (Tcl_Interp *interp, Tk_Window tkwin, const char *styleName); /* 65 */
    Blt_Bg (*blt_GetBgFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr); /* 66 */
    XColor * (*blt_Bg_BorderColor) (Blt_Bg bg); /* 67 */
    Tk_3DBorder (*blt_Bg_Border) (Blt_Bg bg); /* 68 */
    const char * (*blt_Bg_Name) (Blt_Bg bg); /* 69 */
    void (*blt_FreeBg) (Blt_Bg bg); /* 70 */
    void (*blt_Bg_DrawRectangle) (Tk_Window tkwin, Drawable drawable, Blt_Bg bg, int x, int y, int width, int height, int borderWidth, int relief); /* 71 */
    void (*blt_Bg_FillRectangle) (Tk_Window tkwin, Drawable drawable, Blt_Bg bg, int x, int y, int width, int height, int borderWidth, int relief); /* 72 */
    void (*blt_Bg_DrawPolygon) (Tk_Window tkwin, Drawable drawable, Blt_Bg bg, XPoint *points, int numPoints, int borderWidth, int leftRelief); /* 73 */
    void (*blt_Bg_FillPolygon) (Tk_Window tkwin, Drawable drawable, Blt_Bg bg, XPoint *points, int numPoints, int borderWidth, int leftRelief); /* 74 */
    void (*blt_Bg_GetOrigin) (Blt_Bg bg, int *xPtr, int *yPtr); /* 75 */
    void (*blt_Bg_SetChangedProc) (Blt_Bg bg, Blt_Bg_ChangedProc *notifyProc, ClientData clientData); /* 76 */
    void (*blt_Bg_SetOrigin) (Tk_Window tkwin, Blt_Bg bg, int x, int y); /* 77 */
    void (*blt_Bg_DrawFocus) (Tk_Window tkwin, Blt_Bg bg, int highlightWidth, Drawable drawable); /* 78 */
    GC (*blt_Bg_BorderGC) (Tk_Window tkwin, Blt_Bg bg, int which); /* 79 */
    void (*blt_Bg_SetFromBackground) (Tk_Window tkwin, Blt_Bg bg); /* 80 */
    void (*blt_Bg_UnsetClipRegion) (Tk_Window tkwin, Blt_Bg bg); /* 81 */
    void (*blt_Bg_SetClipRegion) (Tk_Window tkwin, Blt_Bg bg, TkRegion rgn); /* 82 */
    void (*blt_DestroyBindingTable) (Blt_BindTable table); /* 83 */
    Blt_BindTable (*blt_CreateBindingTable) (Tcl_Interp *interp, Tk_Window tkwin, ClientData clientData, Blt_BindPickProc *pickProc, Blt_BindAppendTagsProc *tagProc); /* 84 */
    int (*blt_ConfigureBindings) (Tcl_Interp *interp, Blt_BindTable table, ClientData item, int argc, const char **argv); /* 85 */
    int (*blt_ConfigureBindingsFromObj) (Tcl_Interp *interp, Blt_BindTable table, ClientData item, int objc, Tcl_Obj *const *objv); /* 86 */
    void (*blt_PickCurrentItem) (Blt_BindTable table); /* 87 */
    void (*blt_DeleteBindings) (Blt_BindTable table, ClientData object); /* 88 */
    void (*blt_MoveBindingTable) (Blt_BindTable table, Tk_Window tkwin); /* 89 */
    void (*blt_Afm_SetPrinting) (Tcl_Interp *interp, int value); /* 90 */
    int (*blt_Afm_IsPrinting) (void); /* 91 */
    int (*blt_Afm_TextWidth) (Blt_Font font, const char *s, int numBytes); /* 92 */
    int (*blt_Afm_GetMetrics) (Blt_Font font, Blt_FontMetrics *fmPtr); /* 93 */
    const char * (*blt_Afm_GetPostscriptFamily) (const char *family); /* 94 */
    void (*blt_Afm_GetPostscriptName) (const char *family, int flags, Tcl_DString *resultPtr); /* 95 */
    void (*blt_SetDashes) (Display *display, GC gc, Blt_Dashes *dashesPtr); /* 96 */
    void (*blt_ResetLimits) (Blt_Limits *limitsPtr); /* 97 */
    int (*blt_GetLimitsFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr, Blt_Limits *limitsPtr); /* 98 */
    int (*blt_ConfigureInfoFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Blt_ConfigSpec *specs, char *widgRec, Tcl_Obj *objPtr, int flags); /* 99 */
    int (*blt_ConfigureValueFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Blt_ConfigSpec *specs, char *widgRec, Tcl_Obj *objPtr, int flags); /* 100 */
    int (*blt_ConfigureWidgetFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Blt_ConfigSpec *specs, int objc, Tcl_Obj *const *objv, char *widgRec, int flags); /* 101 */
    int (*blt_ConfigureComponentFromObj) (Tcl_Interp *interp, Tk_Window tkwin, const char *name, const char *className, Blt_ConfigSpec *specs, int objc, Tcl_Obj *const *objv, char *widgRec, int flags); /* 102 */
    int (*blt_ConfigModified) (Blt_ConfigSpec *specs, ...); /* 103 */
    const char * (*blt_NameOfState) (int state); /* 104 */
    void (*blt_FreeOptions) (Blt_ConfigSpec *specs, char *widgRec, Display *display, int needFlags); /* 105 */
    int (*blt_ObjIsOption) (Blt_ConfigSpec *specs, Tcl_Obj *objPtr, int flags); /* 106 */
    int (*blt_GetPixelsFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr, int flags, int *valuePtr); /* 107 */
    int (*blt_GetPadFromObj) (Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr, Blt_Pad *padPtr); /* 108 */
    int (*blt_GetStateFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *statePtr); /* 109 */
    int (*blt_GetFillFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *fillPtr); /* 110 */
    int (*blt_GetResizeFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *fillPtr); /* 111 */
    int (*blt_GetDashesFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Dashes *dashesPtr); /* 112 */
    int (*blt_Image_IsDeleted) (Tk_Image tkImage); /* 113 */
    Tk_ImageMaster (*blt_Image_GetMaster) (Tk_Image tkImage); /* 114 */
    Tk_ImageType * (*blt_Image_GetType) (Tk_Image tkImage); /* 115 */
    ClientData (*blt_Image_GetInstanceData) (Tk_Image tkImage); /* 116 */
    const char * (*blt_Image_Name) (Tk_Image tkImage); /* 117 */
    const char * (*blt_Image_NameOfType) (Tk_Image tkImage); /* 118 */
    void (*blt_FreePainter) (Blt_Painter painter); /* 119 */
    Blt_Painter (*blt_GetPainter) (Tk_Window tkwin, float gamma); /* 120 */
    Blt_Painter (*blt_GetPainterFromDrawable) (Display *display, Drawable drawable, float gamma); /* 121 */
    GC (*blt_PainterGC) (Blt_Painter painter); /* 122 */
    int (*blt_PainterDepth) (Blt_Painter painter); /* 123 */
    int (*blt_PaintPicture) (Blt_Painter painter, Drawable drawable, Blt_Picture src, int srcX, int srcY, int width, int height, int destX, int destY, unsigned int flags); /* 124 */
    int (*blt_PaintPictureWithBlend) (Blt_Painter painter, Drawable drawable, Blt_Picture src, int srcX, int srcY, int width, int height, int destX, int destY, unsigned int flags); /* 125 */
    Blt_Picture (*blt_PaintCheckbox) (int width, int height, XColor *fillColor, XColor *outlineColor, XColor *checkColor, int isOn); /* 126 */
    Blt_Picture (*blt_PaintRadioButton) (int width, int height, Blt_Bg bg, XColor *fill, XColor *outline, int isOn); /* 127 */
    Blt_Picture (*blt_PaintRadioButtonOld) (int width, int height, XColor *bg, XColor *fill, XColor *outline, XColor *check, int isOn); /* 128 */
    Blt_Picture (*blt_PaintDelete) (int width, int height, XColor *bgColor, XColor *fillColor, XColor *symColor, int isActive); /* 129 */
    Blt_Ps (*blt_Ps_Create) (Tcl_Interp *interp, PageSetup *setupPtr); /* 130 */
    void (*blt_Ps_Free) (Blt_Ps ps); /* 131 */
    const char * (*blt_Ps_GetValue) (Blt_Ps ps, int *lengthPtr); /* 132 */
    Tcl_Interp * (*blt_Ps_GetInterp) (Blt_Ps ps); /* 133 */
    Tcl_DString * (*blt_Ps_GetDString) (Blt_Ps ps); /* 134 */
    char * (*blt_Ps_GetScratchBuffer) (Blt_Ps ps); /* 135 */
    void (*blt_Ps_SetInterp) (Blt_Ps ps, Tcl_Interp *interp); /* 136 */
    void (*blt_Ps_Append) (Blt_Ps ps, const char *string); /* 137 */
    void (*blt_Ps_AppendBytes) (Blt_Ps ps, const char *string, int numBytes); /* 138 */
    void (*blt_Ps_VarAppend) (Blt_Ps ps, ...); /* 139 */
    void (*blt_Ps_Format) (Blt_Ps ps, const char *fmt, ...); /* 140 */
    void (*blt_Ps_SetClearBackground) (Blt_Ps ps); /* 141 */
    int (*blt_Ps_IncludeFile) (Tcl_Interp *interp, Blt_Ps ps, const char *fileName); /* 142 */
    int (*blt_Ps_GetPicaFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *picaPtr); /* 143 */
    int (*blt_Ps_GetPadFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Pad *padPtr); /* 144 */
    int (*blt_Ps_ComputeBoundingBox) (PageSetup *setupPtr, int w, int h); /* 145 */
    void (*blt_Ps_DrawPicture) (Blt_Ps ps, Blt_Picture picture, double x, double y); /* 146 */
    void (*blt_Ps_Rectangle) (Blt_Ps ps, int x, int y, int w, int h); /* 147 */
    int (*blt_Ps_SaveFile) (Tcl_Interp *interp, Blt_Ps ps, const char *fileName); /* 148 */
    void (*blt_Ps_XSetLineWidth) (Blt_Ps ps, int lineWidth); /* 149 */
    void (*blt_Ps_XSetBackground) (Blt_Ps ps, XColor *colorPtr); /* 150 */
    void (*blt_Ps_XSetBitmapData) (Blt_Ps ps, Display *display, Pixmap bitmap, int width, int height); /* 151 */
    void (*blt_Ps_XSetForeground) (Blt_Ps ps, XColor *colorPtr); /* 152 */
    void (*blt_Ps_XSetFont) (Blt_Ps ps, Blt_Font font); /* 153 */
    void (*blt_Ps_XSetDashes) (Blt_Ps ps, Blt_Dashes *dashesPtr); /* 154 */
    void (*blt_Ps_XSetLineAttributes) (Blt_Ps ps, XColor *colorPtr, int lineWidth, Blt_Dashes *dashesPtr, int capStyle, int joinStyle); /* 155 */
    void (*blt_Ps_XSetStipple) (Blt_Ps ps, Display *display, Pixmap bitmap); /* 156 */
    void (*blt_Ps_Polyline) (Blt_Ps ps, int n, Point2d *points); /* 157 */
    void (*blt_Ps_XDrawLines) (Blt_Ps ps, int n, XPoint *points); /* 158 */
    void (*blt_Ps_XDrawSegments) (Blt_Ps ps, int n, XSegment *segments); /* 159 */
    void (*blt_Ps_DrawPolyline) (Blt_Ps ps, int n, Point2d *points); /* 160 */
    void (*blt_Ps_DrawSegments2d) (Blt_Ps ps, int n, Segment2d *segments); /* 161 */
    void (*blt_Ps_Draw3DRectangle) (Blt_Ps ps, Tk_3DBorder border, double x, double y, int width, int height, int borderWidth, int relief); /* 162 */
    void (*blt_Ps_Fill3DRectangle) (Blt_Ps ps, Tk_3DBorder border, double x, double y, int width, int height, int borderWidth, int relief); /* 163 */
    void (*blt_Ps_XFillRectangle) (Blt_Ps ps, double x, double y, int width, int height); /* 164 */
    void (*blt_Ps_XFillRectangles) (Blt_Ps ps, int n, XRectangle *rects); /* 165 */
    void (*blt_Ps_XFillPolygon) (Blt_Ps ps, int n, Point2d *points); /* 166 */
    void (*blt_Ps_DrawPhoto) (Blt_Ps ps, Tk_PhotoHandle photoToken, double x, double y); /* 167 */
    void (*blt_Ps_XDrawWindow) (Blt_Ps ps, Tk_Window tkwin, double x, double y); /* 168 */
    void (*blt_Ps_DrawText) (Blt_Ps ps, const char *string, TextStyle *attrPtr, double x, double y); /* 169 */
    void (*blt_Ps_DrawBitmap) (Blt_Ps ps, Display *display, Pixmap bitmap, double scaleX, double scaleY); /* 170 */
    void (*blt_Ps_XSetCapStyle) (Blt_Ps ps, int capStyle); /* 171 */
    void (*blt_Ps_XSetJoinStyle) (Blt_Ps ps, int joinStyle); /* 172 */
    void (*blt_Ps_PolylineFromXPoints) (Blt_Ps ps, int n, XPoint *points); /* 173 */
    void (*blt_Ps_Polygon) (Blt_Ps ps, Point2d *points, int numPoints); /* 174 */
    void (*blt_Ps_SetPrinting) (Blt_Ps ps, int value); /* 175 */
    TextLayout * (*blt_Ts_CreateLayout) (const char *string, int length, TextStyle *tsPtr); /* 176 */
    TextLayout * (*blt_Ts_TitleLayout) (const char *string, int length, TextStyle *tsPtr); /* 177 */
    void (*blt_Ts_DrawLayout) (Tk_Window tkwin, Drawable drawable, TextLayout *textPtr, TextStyle *tsPtr, int x, int y); /* 178 */
    void (*blt_Ts_GetExtents) (TextStyle *tsPtr, const char *text, unsigned int *widthPtr, unsigned int *heightPtr); /* 179 */
    void (*blt_Ts_ResetStyle) (Tk_Window tkwin, TextStyle *tsPtr); /* 180 */
    void (*blt_Ts_FreeStyle) (Display *display, TextStyle *tsPtr); /* 181 */
    void (*blt_Ts_SetDrawStyle) (TextStyle *tsPtr, Blt_Font font, GC gc, XColor *fgColor, float angle, Tk_Anchor anchor, Tk_Justify justify, int leader); /* 182 */
    void (*blt_DrawText) (Tk_Window tkwin, Drawable drawable, const char *string, TextStyle *tsPtr, int x, int y); /* 183 */
    void (*blt_DrawText2) (Tk_Window tkwin, Drawable drawable, const char *string, TextStyle *tsPtr, int x, int y, Dim2d *dimPtr); /* 184 */
    Pixmap (*blt_Ts_Bitmap) (Tk_Window tkwin, TextLayout *textPtr, TextStyle *tsPtr, int *widthPtr, int *heightPtr); /* 185 */
    int (*blt_DrawTextWithRotatedFont) (Tk_Window tkwin, Drawable drawable, float angle, TextStyle *tsPtr, TextLayout *textPtr, int x, int y); /* 186 */
    void (*blt_DrawLayout) (Tk_Window tkwin, Drawable drawable, GC gc, Blt_Font font, int depth, float angle, int x, int y, TextLayout *layoutPtr, int maxLength); /* 187 */
    void (*blt_GetTextExtents) (Blt_Font font, int leader, const char *text, int textLen, unsigned int *widthPtr, unsigned int *heightPtr); /* 188 */
    void (*blt_RotateStartingTextPositions) (TextLayout *textPtr, float angle); /* 189 */
    Tk_TextLayout (*blt_ComputeTextLayout) (Blt_Font font, const char *string, int numChars, int wrapLength, Tk_Justify justify, int flags, int *widthPtr, int *heightPtr); /* 190 */
    void (*blt_DrawTextLayout) (Display *display, Drawable drawable, GC gc, Tk_TextLayout layout, int x, int y, int firstChar, int lastChar); /* 191 */
    int (*blt_CharBbox) (Tk_TextLayout layout, int index, int *xPtr, int *yPtr, int *widthPtr, int *heightPtr); /* 192 */
    void (*blt_UnderlineTextLayout) (Display *display, Drawable drawable, GC gc, Tk_TextLayout layout, int x, int y, int underline); /* 193 */
    void (*blt_Ts_UnderlineLayout) (Tk_Window tkwin, Drawable drawable, TextLayout *layoutPtr, TextStyle *tsPtr, int x, int y); /* 194 */
    void (*blt_Ts_DrawText) (Tk_Window tkwin, Drawable drawable, const char *text, int textLen, TextStyle *tsPtr, int x, int y); /* 195 */
    int (*blt_MeasureText) (Blt_Font font, const char *text, int textLen, int maxLength, int *nBytesPtr); /* 196 */
    void (*blt_FreeTextLayout) (Tk_TextLayout layout); /* 197 */
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
#ifndef Blt_GetParentWindow
#define Blt_GetParentWindow \
	(bltTkIntProcsPtr->blt_GetParentWindow) /* 12 */
#endif
#ifndef Blt_FindChild
#define Blt_FindChild \
	(bltTkIntProcsPtr->blt_FindChild) /* 13 */
#endif
#ifndef Blt_FirstChild
#define Blt_FirstChild \
	(bltTkIntProcsPtr->blt_FirstChild) /* 14 */
#endif
#ifndef Blt_NextChild
#define Blt_NextChild \
	(bltTkIntProcsPtr->blt_NextChild) /* 15 */
#endif
#ifndef Blt_RelinkWindow
#define Blt_RelinkWindow \
	(bltTkIntProcsPtr->blt_RelinkWindow) /* 16 */
#endif
#ifndef Blt_Toplevel
#define Blt_Toplevel \
	(bltTkIntProcsPtr->blt_Toplevel) /* 17 */
#endif
#ifndef Blt_GetPixels
#define Blt_GetPixels \
	(bltTkIntProcsPtr->blt_GetPixels) /* 18 */
#endif
#ifndef Blt_NameOfFill
#define Blt_NameOfFill \
	(bltTkIntProcsPtr->blt_NameOfFill) /* 19 */
#endif
#ifndef Blt_NameOfResize
#define Blt_NameOfResize \
	(bltTkIntProcsPtr->blt_NameOfResize) /* 20 */
#endif
#ifndef Blt_GetXY
#define Blt_GetXY \
	(bltTkIntProcsPtr->blt_GetXY) /* 21 */
#endif
#ifndef Blt_GetProjection
#define Blt_GetProjection \
	(bltTkIntProcsPtr->blt_GetProjection) /* 22 */
#endif
#ifndef Blt_DrawArrowOld
#define Blt_DrawArrowOld \
	(bltTkIntProcsPtr->blt_DrawArrowOld) /* 23 */
#endif
#ifndef Blt_DrawArrow
#define Blt_DrawArrow \
	(bltTkIntProcsPtr->blt_DrawArrow) /* 24 */
#endif
#ifndef Blt_MakeTransparentWindowExist
#define Blt_MakeTransparentWindowExist \
	(bltTkIntProcsPtr->blt_MakeTransparentWindowExist) /* 25 */
#endif
#ifndef Blt_TranslateAnchor
#define Blt_TranslateAnchor \
	(bltTkIntProcsPtr->blt_TranslateAnchor) /* 26 */
#endif
#ifndef Blt_AnchorPoint
#define Blt_AnchorPoint \
	(bltTkIntProcsPtr->blt_AnchorPoint) /* 27 */
#endif
#ifndef Blt_MaxRequestSize
#define Blt_MaxRequestSize \
	(bltTkIntProcsPtr->blt_MaxRequestSize) /* 28 */
#endif
#ifndef Blt_GetWindowId
#define Blt_GetWindowId \
	(bltTkIntProcsPtr->blt_GetWindowId) /* 29 */
#endif
#ifndef Blt_InitXRandrConfig
#define Blt_InitXRandrConfig \
	(bltTkIntProcsPtr->blt_InitXRandrConfig) /* 30 */
#endif
#ifndef Blt_SizeOfScreen
#define Blt_SizeOfScreen \
	(bltTkIntProcsPtr->blt_SizeOfScreen) /* 31 */
#endif
#ifndef Blt_RootX
#define Blt_RootX \
	(bltTkIntProcsPtr->blt_RootX) /* 32 */
#endif
#ifndef Blt_RootY
#define Blt_RootY \
	(bltTkIntProcsPtr->blt_RootY) /* 33 */
#endif
#ifndef Blt_RootCoordinates
#define Blt_RootCoordinates \
	(bltTkIntProcsPtr->blt_RootCoordinates) /* 34 */
#endif
#ifndef Blt_MapToplevelWindow
#define Blt_MapToplevelWindow \
	(bltTkIntProcsPtr->blt_MapToplevelWindow) /* 35 */
#endif
#ifndef Blt_UnmapToplevelWindow
#define Blt_UnmapToplevelWindow \
	(bltTkIntProcsPtr->blt_UnmapToplevelWindow) /* 36 */
#endif
#ifndef Blt_RaiseToplevelWindow
#define Blt_RaiseToplevelWindow \
	(bltTkIntProcsPtr->blt_RaiseToplevelWindow) /* 37 */
#endif
#ifndef Blt_LowerToplevelWindow
#define Blt_LowerToplevelWindow \
	(bltTkIntProcsPtr->blt_LowerToplevelWindow) /* 38 */
#endif
#ifndef Blt_ResizeToplevelWindow
#define Blt_ResizeToplevelWindow \
	(bltTkIntProcsPtr->blt_ResizeToplevelWindow) /* 39 */
#endif
#ifndef Blt_MoveToplevelWindow
#define Blt_MoveToplevelWindow \
	(bltTkIntProcsPtr->blt_MoveToplevelWindow) /* 40 */
#endif
#ifndef Blt_MoveResizeToplevelWindow
#define Blt_MoveResizeToplevelWindow \
	(bltTkIntProcsPtr->blt_MoveResizeToplevelWindow) /* 41 */
#endif
#ifndef Blt_GetWindowRegion
#define Blt_GetWindowRegion \
	(bltTkIntProcsPtr->blt_GetWindowRegion) /* 42 */
#endif
#ifndef Blt_GetWindowInstanceData
#define Blt_GetWindowInstanceData \
	(bltTkIntProcsPtr->blt_GetWindowInstanceData) /* 43 */
#endif
#ifndef Blt_SetWindowInstanceData
#define Blt_SetWindowInstanceData \
	(bltTkIntProcsPtr->blt_SetWindowInstanceData) /* 44 */
#endif
#ifndef Blt_DeleteWindowInstanceData
#define Blt_DeleteWindowInstanceData \
	(bltTkIntProcsPtr->blt_DeleteWindowInstanceData) /* 45 */
#endif
#ifndef Blt_ReparentWindow
#define Blt_ReparentWindow \
	(bltTkIntProcsPtr->blt_ReparentWindow) /* 46 */
#endif
#ifndef Blt_GetDrawableAttribs
#define Blt_GetDrawableAttribs \
	(bltTkIntProcsPtr->blt_GetDrawableAttribs) /* 47 */
#endif
#ifndef Blt_SetDrawableAttribs
#define Blt_SetDrawableAttribs \
	(bltTkIntProcsPtr->blt_SetDrawableAttribs) /* 48 */
#endif
#ifndef Blt_SetDrawableAttribsFromWindow
#define Blt_SetDrawableAttribsFromWindow \
	(bltTkIntProcsPtr->blt_SetDrawableAttribsFromWindow) /* 49 */
#endif
#ifndef Blt_FreeDrawableAttribs
#define Blt_FreeDrawableAttribs \
	(bltTkIntProcsPtr->blt_FreeDrawableAttribs) /* 50 */
#endif
#ifndef Blt_GetBitmapGC
#define Blt_GetBitmapGC \
	(bltTkIntProcsPtr->blt_GetBitmapGC) /* 51 */
#endif
#ifndef Blt_GetPixmapAbortOnError
#define Blt_GetPixmapAbortOnError \
	(bltTkIntProcsPtr->blt_GetPixmapAbortOnError) /* 52 */
#endif
#ifndef Blt_ScreenDPI
#define Blt_ScreenDPI \
	(bltTkIntProcsPtr->blt_ScreenDPI) /* 53 */
#endif
#ifndef Blt_OldConfigModified
#define Blt_OldConfigModified \
	(bltTkIntProcsPtr->blt_OldConfigModified) /* 54 */
#endif
#ifndef Blt_GetLineExtents
#define Blt_GetLineExtents \
	(bltTkIntProcsPtr->blt_GetLineExtents) /* 55 */
#endif
#ifndef Blt_GetBoundingBox
#define Blt_GetBoundingBox \
	(bltTkIntProcsPtr->blt_GetBoundingBox) /* 56 */
#endif
#ifndef Blt_GetFont
#define Blt_GetFont \
	(bltTkIntProcsPtr->blt_GetFont) /* 57 */
#endif
#ifndef Blt_AllocFontFromObj
#define Blt_AllocFontFromObj \
	(bltTkIntProcsPtr->blt_AllocFontFromObj) /* 58 */
#endif
#ifndef Blt_DrawWithEllipsis
#define Blt_DrawWithEllipsis \
	(bltTkIntProcsPtr->blt_DrawWithEllipsis) /* 59 */
#endif
#ifndef Blt_GetFontFromObj
#define Blt_GetFontFromObj \
	(bltTkIntProcsPtr->blt_GetFontFromObj) /* 60 */
#endif
#ifndef Blt_Font_GetMetrics
#define Blt_Font_GetMetrics \
	(bltTkIntProcsPtr->blt_Font_GetMetrics) /* 61 */
#endif
#ifndef Blt_TextWidth
#define Blt_TextWidth \
	(bltTkIntProcsPtr->blt_TextWidth) /* 62 */
#endif
#ifndef Blt_Font_GetInterp
#define Blt_Font_GetInterp \
	(bltTkIntProcsPtr->blt_Font_GetInterp) /* 63 */
#endif
#ifndef Blt_Font_GetFile
#define Blt_Font_GetFile \
	(bltTkIntProcsPtr->blt_Font_GetFile) /* 64 */
#endif
#ifndef Blt_GetBg
#define Blt_GetBg \
	(bltTkIntProcsPtr->blt_GetBg) /* 65 */
#endif
#ifndef Blt_GetBgFromObj
#define Blt_GetBgFromObj \
	(bltTkIntProcsPtr->blt_GetBgFromObj) /* 66 */
#endif
#ifndef Blt_Bg_BorderColor
#define Blt_Bg_BorderColor \
	(bltTkIntProcsPtr->blt_Bg_BorderColor) /* 67 */
#endif
#ifndef Blt_Bg_Border
#define Blt_Bg_Border \
	(bltTkIntProcsPtr->blt_Bg_Border) /* 68 */
#endif
#ifndef Blt_Bg_Name
#define Blt_Bg_Name \
	(bltTkIntProcsPtr->blt_Bg_Name) /* 69 */
#endif
#ifndef Blt_FreeBg
#define Blt_FreeBg \
	(bltTkIntProcsPtr->blt_FreeBg) /* 70 */
#endif
#ifndef Blt_Bg_DrawRectangle
#define Blt_Bg_DrawRectangle \
	(bltTkIntProcsPtr->blt_Bg_DrawRectangle) /* 71 */
#endif
#ifndef Blt_Bg_FillRectangle
#define Blt_Bg_FillRectangle \
	(bltTkIntProcsPtr->blt_Bg_FillRectangle) /* 72 */
#endif
#ifndef Blt_Bg_DrawPolygon
#define Blt_Bg_DrawPolygon \
	(bltTkIntProcsPtr->blt_Bg_DrawPolygon) /* 73 */
#endif
#ifndef Blt_Bg_FillPolygon
#define Blt_Bg_FillPolygon \
	(bltTkIntProcsPtr->blt_Bg_FillPolygon) /* 74 */
#endif
#ifndef Blt_Bg_GetOrigin
#define Blt_Bg_GetOrigin \
	(bltTkIntProcsPtr->blt_Bg_GetOrigin) /* 75 */
#endif
#ifndef Blt_Bg_SetChangedProc
#define Blt_Bg_SetChangedProc \
	(bltTkIntProcsPtr->blt_Bg_SetChangedProc) /* 76 */
#endif
#ifndef Blt_Bg_SetOrigin
#define Blt_Bg_SetOrigin \
	(bltTkIntProcsPtr->blt_Bg_SetOrigin) /* 77 */
#endif
#ifndef Blt_Bg_DrawFocus
#define Blt_Bg_DrawFocus \
	(bltTkIntProcsPtr->blt_Bg_DrawFocus) /* 78 */
#endif
#ifndef Blt_Bg_BorderGC
#define Blt_Bg_BorderGC \
	(bltTkIntProcsPtr->blt_Bg_BorderGC) /* 79 */
#endif
#ifndef Blt_Bg_SetFromBackground
#define Blt_Bg_SetFromBackground \
	(bltTkIntProcsPtr->blt_Bg_SetFromBackground) /* 80 */
#endif
#ifndef Blt_Bg_UnsetClipRegion
#define Blt_Bg_UnsetClipRegion \
	(bltTkIntProcsPtr->blt_Bg_UnsetClipRegion) /* 81 */
#endif
#ifndef Blt_Bg_SetClipRegion
#define Blt_Bg_SetClipRegion \
	(bltTkIntProcsPtr->blt_Bg_SetClipRegion) /* 82 */
#endif
#ifndef Blt_DestroyBindingTable
#define Blt_DestroyBindingTable \
	(bltTkIntProcsPtr->blt_DestroyBindingTable) /* 83 */
#endif
#ifndef Blt_CreateBindingTable
#define Blt_CreateBindingTable \
	(bltTkIntProcsPtr->blt_CreateBindingTable) /* 84 */
#endif
#ifndef Blt_ConfigureBindings
#define Blt_ConfigureBindings \
	(bltTkIntProcsPtr->blt_ConfigureBindings) /* 85 */
#endif
#ifndef Blt_ConfigureBindingsFromObj
#define Blt_ConfigureBindingsFromObj \
	(bltTkIntProcsPtr->blt_ConfigureBindingsFromObj) /* 86 */
#endif
#ifndef Blt_PickCurrentItem
#define Blt_PickCurrentItem \
	(bltTkIntProcsPtr->blt_PickCurrentItem) /* 87 */
#endif
#ifndef Blt_DeleteBindings
#define Blt_DeleteBindings \
	(bltTkIntProcsPtr->blt_DeleteBindings) /* 88 */
#endif
#ifndef Blt_MoveBindingTable
#define Blt_MoveBindingTable \
	(bltTkIntProcsPtr->blt_MoveBindingTable) /* 89 */
#endif
#ifndef Blt_Afm_SetPrinting
#define Blt_Afm_SetPrinting \
	(bltTkIntProcsPtr->blt_Afm_SetPrinting) /* 90 */
#endif
#ifndef Blt_Afm_IsPrinting
#define Blt_Afm_IsPrinting \
	(bltTkIntProcsPtr->blt_Afm_IsPrinting) /* 91 */
#endif
#ifndef Blt_Afm_TextWidth
#define Blt_Afm_TextWidth \
	(bltTkIntProcsPtr->blt_Afm_TextWidth) /* 92 */
#endif
#ifndef Blt_Afm_GetMetrics
#define Blt_Afm_GetMetrics \
	(bltTkIntProcsPtr->blt_Afm_GetMetrics) /* 93 */
#endif
#ifndef Blt_Afm_GetPostscriptFamily
#define Blt_Afm_GetPostscriptFamily \
	(bltTkIntProcsPtr->blt_Afm_GetPostscriptFamily) /* 94 */
#endif
#ifndef Blt_Afm_GetPostscriptName
#define Blt_Afm_GetPostscriptName \
	(bltTkIntProcsPtr->blt_Afm_GetPostscriptName) /* 95 */
#endif
#ifndef Blt_SetDashes
#define Blt_SetDashes \
	(bltTkIntProcsPtr->blt_SetDashes) /* 96 */
#endif
#ifndef Blt_ResetLimits
#define Blt_ResetLimits \
	(bltTkIntProcsPtr->blt_ResetLimits) /* 97 */
#endif
#ifndef Blt_GetLimitsFromObj
#define Blt_GetLimitsFromObj \
	(bltTkIntProcsPtr->blt_GetLimitsFromObj) /* 98 */
#endif
#ifndef Blt_ConfigureInfoFromObj
#define Blt_ConfigureInfoFromObj \
	(bltTkIntProcsPtr->blt_ConfigureInfoFromObj) /* 99 */
#endif
#ifndef Blt_ConfigureValueFromObj
#define Blt_ConfigureValueFromObj \
	(bltTkIntProcsPtr->blt_ConfigureValueFromObj) /* 100 */
#endif
#ifndef Blt_ConfigureWidgetFromObj
#define Blt_ConfigureWidgetFromObj \
	(bltTkIntProcsPtr->blt_ConfigureWidgetFromObj) /* 101 */
#endif
#ifndef Blt_ConfigureComponentFromObj
#define Blt_ConfigureComponentFromObj \
	(bltTkIntProcsPtr->blt_ConfigureComponentFromObj) /* 102 */
#endif
#ifndef Blt_ConfigModified
#define Blt_ConfigModified \
	(bltTkIntProcsPtr->blt_ConfigModified) /* 103 */
#endif
#ifndef Blt_NameOfState
#define Blt_NameOfState \
	(bltTkIntProcsPtr->blt_NameOfState) /* 104 */
#endif
#ifndef Blt_FreeOptions
#define Blt_FreeOptions \
	(bltTkIntProcsPtr->blt_FreeOptions) /* 105 */
#endif
#ifndef Blt_ObjIsOption
#define Blt_ObjIsOption \
	(bltTkIntProcsPtr->blt_ObjIsOption) /* 106 */
#endif
#ifndef Blt_GetPixelsFromObj
#define Blt_GetPixelsFromObj \
	(bltTkIntProcsPtr->blt_GetPixelsFromObj) /* 107 */
#endif
#ifndef Blt_GetPadFromObj
#define Blt_GetPadFromObj \
	(bltTkIntProcsPtr->blt_GetPadFromObj) /* 108 */
#endif
#ifndef Blt_GetStateFromObj
#define Blt_GetStateFromObj \
	(bltTkIntProcsPtr->blt_GetStateFromObj) /* 109 */
#endif
#ifndef Blt_GetFillFromObj
#define Blt_GetFillFromObj \
	(bltTkIntProcsPtr->blt_GetFillFromObj) /* 110 */
#endif
#ifndef Blt_GetResizeFromObj
#define Blt_GetResizeFromObj \
	(bltTkIntProcsPtr->blt_GetResizeFromObj) /* 111 */
#endif
#ifndef Blt_GetDashesFromObj
#define Blt_GetDashesFromObj \
	(bltTkIntProcsPtr->blt_GetDashesFromObj) /* 112 */
#endif
#ifndef Blt_Image_IsDeleted
#define Blt_Image_IsDeleted \
	(bltTkIntProcsPtr->blt_Image_IsDeleted) /* 113 */
#endif
#ifndef Blt_Image_GetMaster
#define Blt_Image_GetMaster \
	(bltTkIntProcsPtr->blt_Image_GetMaster) /* 114 */
#endif
#ifndef Blt_Image_GetType
#define Blt_Image_GetType \
	(bltTkIntProcsPtr->blt_Image_GetType) /* 115 */
#endif
#ifndef Blt_Image_GetInstanceData
#define Blt_Image_GetInstanceData \
	(bltTkIntProcsPtr->blt_Image_GetInstanceData) /* 116 */
#endif
#ifndef Blt_Image_Name
#define Blt_Image_Name \
	(bltTkIntProcsPtr->blt_Image_Name) /* 117 */
#endif
#ifndef Blt_Image_NameOfType
#define Blt_Image_NameOfType \
	(bltTkIntProcsPtr->blt_Image_NameOfType) /* 118 */
#endif
#ifndef Blt_FreePainter
#define Blt_FreePainter \
	(bltTkIntProcsPtr->blt_FreePainter) /* 119 */
#endif
#ifndef Blt_GetPainter
#define Blt_GetPainter \
	(bltTkIntProcsPtr->blt_GetPainter) /* 120 */
#endif
#ifndef Blt_GetPainterFromDrawable
#define Blt_GetPainterFromDrawable \
	(bltTkIntProcsPtr->blt_GetPainterFromDrawable) /* 121 */
#endif
#ifndef Blt_PainterGC
#define Blt_PainterGC \
	(bltTkIntProcsPtr->blt_PainterGC) /* 122 */
#endif
#ifndef Blt_PainterDepth
#define Blt_PainterDepth \
	(bltTkIntProcsPtr->blt_PainterDepth) /* 123 */
#endif
#ifndef Blt_PaintPicture
#define Blt_PaintPicture \
	(bltTkIntProcsPtr->blt_PaintPicture) /* 124 */
#endif
#ifndef Blt_PaintPictureWithBlend
#define Blt_PaintPictureWithBlend \
	(bltTkIntProcsPtr->blt_PaintPictureWithBlend) /* 125 */
#endif
#ifndef Blt_PaintCheckbox
#define Blt_PaintCheckbox \
	(bltTkIntProcsPtr->blt_PaintCheckbox) /* 126 */
#endif
#ifndef Blt_PaintRadioButton
#define Blt_PaintRadioButton \
	(bltTkIntProcsPtr->blt_PaintRadioButton) /* 127 */
#endif
#ifndef Blt_PaintRadioButtonOld
#define Blt_PaintRadioButtonOld \
	(bltTkIntProcsPtr->blt_PaintRadioButtonOld) /* 128 */
#endif
#ifndef Blt_PaintDelete
#define Blt_PaintDelete \
	(bltTkIntProcsPtr->blt_PaintDelete) /* 129 */
#endif
#ifndef Blt_Ps_Create
#define Blt_Ps_Create \
	(bltTkIntProcsPtr->blt_Ps_Create) /* 130 */
#endif
#ifndef Blt_Ps_Free
#define Blt_Ps_Free \
	(bltTkIntProcsPtr->blt_Ps_Free) /* 131 */
#endif
#ifndef Blt_Ps_GetValue
#define Blt_Ps_GetValue \
	(bltTkIntProcsPtr->blt_Ps_GetValue) /* 132 */
#endif
#ifndef Blt_Ps_GetInterp
#define Blt_Ps_GetInterp \
	(bltTkIntProcsPtr->blt_Ps_GetInterp) /* 133 */
#endif
#ifndef Blt_Ps_GetDString
#define Blt_Ps_GetDString \
	(bltTkIntProcsPtr->blt_Ps_GetDString) /* 134 */
#endif
#ifndef Blt_Ps_GetScratchBuffer
#define Blt_Ps_GetScratchBuffer \
	(bltTkIntProcsPtr->blt_Ps_GetScratchBuffer) /* 135 */
#endif
#ifndef Blt_Ps_SetInterp
#define Blt_Ps_SetInterp \
	(bltTkIntProcsPtr->blt_Ps_SetInterp) /* 136 */
#endif
#ifndef Blt_Ps_Append
#define Blt_Ps_Append \
	(bltTkIntProcsPtr->blt_Ps_Append) /* 137 */
#endif
#ifndef Blt_Ps_AppendBytes
#define Blt_Ps_AppendBytes \
	(bltTkIntProcsPtr->blt_Ps_AppendBytes) /* 138 */
#endif
#ifndef Blt_Ps_VarAppend
#define Blt_Ps_VarAppend \
	(bltTkIntProcsPtr->blt_Ps_VarAppend) /* 139 */
#endif
#ifndef Blt_Ps_Format
#define Blt_Ps_Format \
	(bltTkIntProcsPtr->blt_Ps_Format) /* 140 */
#endif
#ifndef Blt_Ps_SetClearBackground
#define Blt_Ps_SetClearBackground \
	(bltTkIntProcsPtr->blt_Ps_SetClearBackground) /* 141 */
#endif
#ifndef Blt_Ps_IncludeFile
#define Blt_Ps_IncludeFile \
	(bltTkIntProcsPtr->blt_Ps_IncludeFile) /* 142 */
#endif
#ifndef Blt_Ps_GetPicaFromObj
#define Blt_Ps_GetPicaFromObj \
	(bltTkIntProcsPtr->blt_Ps_GetPicaFromObj) /* 143 */
#endif
#ifndef Blt_Ps_GetPadFromObj
#define Blt_Ps_GetPadFromObj \
	(bltTkIntProcsPtr->blt_Ps_GetPadFromObj) /* 144 */
#endif
#ifndef Blt_Ps_ComputeBoundingBox
#define Blt_Ps_ComputeBoundingBox \
	(bltTkIntProcsPtr->blt_Ps_ComputeBoundingBox) /* 145 */
#endif
#ifndef Blt_Ps_DrawPicture
#define Blt_Ps_DrawPicture \
	(bltTkIntProcsPtr->blt_Ps_DrawPicture) /* 146 */
#endif
#ifndef Blt_Ps_Rectangle
#define Blt_Ps_Rectangle \
	(bltTkIntProcsPtr->blt_Ps_Rectangle) /* 147 */
#endif
#ifndef Blt_Ps_SaveFile
#define Blt_Ps_SaveFile \
	(bltTkIntProcsPtr->blt_Ps_SaveFile) /* 148 */
#endif
#ifndef Blt_Ps_XSetLineWidth
#define Blt_Ps_XSetLineWidth \
	(bltTkIntProcsPtr->blt_Ps_XSetLineWidth) /* 149 */
#endif
#ifndef Blt_Ps_XSetBackground
#define Blt_Ps_XSetBackground \
	(bltTkIntProcsPtr->blt_Ps_XSetBackground) /* 150 */
#endif
#ifndef Blt_Ps_XSetBitmapData
#define Blt_Ps_XSetBitmapData \
	(bltTkIntProcsPtr->blt_Ps_XSetBitmapData) /* 151 */
#endif
#ifndef Blt_Ps_XSetForeground
#define Blt_Ps_XSetForeground \
	(bltTkIntProcsPtr->blt_Ps_XSetForeground) /* 152 */
#endif
#ifndef Blt_Ps_XSetFont
#define Blt_Ps_XSetFont \
	(bltTkIntProcsPtr->blt_Ps_XSetFont) /* 153 */
#endif
#ifndef Blt_Ps_XSetDashes
#define Blt_Ps_XSetDashes \
	(bltTkIntProcsPtr->blt_Ps_XSetDashes) /* 154 */
#endif
#ifndef Blt_Ps_XSetLineAttributes
#define Blt_Ps_XSetLineAttributes \
	(bltTkIntProcsPtr->blt_Ps_XSetLineAttributes) /* 155 */
#endif
#ifndef Blt_Ps_XSetStipple
#define Blt_Ps_XSetStipple \
	(bltTkIntProcsPtr->blt_Ps_XSetStipple) /* 156 */
#endif
#ifndef Blt_Ps_Polyline
#define Blt_Ps_Polyline \
	(bltTkIntProcsPtr->blt_Ps_Polyline) /* 157 */
#endif
#ifndef Blt_Ps_XDrawLines
#define Blt_Ps_XDrawLines \
	(bltTkIntProcsPtr->blt_Ps_XDrawLines) /* 158 */
#endif
#ifndef Blt_Ps_XDrawSegments
#define Blt_Ps_XDrawSegments \
	(bltTkIntProcsPtr->blt_Ps_XDrawSegments) /* 159 */
#endif
#ifndef Blt_Ps_DrawPolyline
#define Blt_Ps_DrawPolyline \
	(bltTkIntProcsPtr->blt_Ps_DrawPolyline) /* 160 */
#endif
#ifndef Blt_Ps_DrawSegments2d
#define Blt_Ps_DrawSegments2d \
	(bltTkIntProcsPtr->blt_Ps_DrawSegments2d) /* 161 */
#endif
#ifndef Blt_Ps_Draw3DRectangle
#define Blt_Ps_Draw3DRectangle \
	(bltTkIntProcsPtr->blt_Ps_Draw3DRectangle) /* 162 */
#endif
#ifndef Blt_Ps_Fill3DRectangle
#define Blt_Ps_Fill3DRectangle \
	(bltTkIntProcsPtr->blt_Ps_Fill3DRectangle) /* 163 */
#endif
#ifndef Blt_Ps_XFillRectangle
#define Blt_Ps_XFillRectangle \
	(bltTkIntProcsPtr->blt_Ps_XFillRectangle) /* 164 */
#endif
#ifndef Blt_Ps_XFillRectangles
#define Blt_Ps_XFillRectangles \
	(bltTkIntProcsPtr->blt_Ps_XFillRectangles) /* 165 */
#endif
#ifndef Blt_Ps_XFillPolygon
#define Blt_Ps_XFillPolygon \
	(bltTkIntProcsPtr->blt_Ps_XFillPolygon) /* 166 */
#endif
#ifndef Blt_Ps_DrawPhoto
#define Blt_Ps_DrawPhoto \
	(bltTkIntProcsPtr->blt_Ps_DrawPhoto) /* 167 */
#endif
#ifndef Blt_Ps_XDrawWindow
#define Blt_Ps_XDrawWindow \
	(bltTkIntProcsPtr->blt_Ps_XDrawWindow) /* 168 */
#endif
#ifndef Blt_Ps_DrawText
#define Blt_Ps_DrawText \
	(bltTkIntProcsPtr->blt_Ps_DrawText) /* 169 */
#endif
#ifndef Blt_Ps_DrawBitmap
#define Blt_Ps_DrawBitmap \
	(bltTkIntProcsPtr->blt_Ps_DrawBitmap) /* 170 */
#endif
#ifndef Blt_Ps_XSetCapStyle
#define Blt_Ps_XSetCapStyle \
	(bltTkIntProcsPtr->blt_Ps_XSetCapStyle) /* 171 */
#endif
#ifndef Blt_Ps_XSetJoinStyle
#define Blt_Ps_XSetJoinStyle \
	(bltTkIntProcsPtr->blt_Ps_XSetJoinStyle) /* 172 */
#endif
#ifndef Blt_Ps_PolylineFromXPoints
#define Blt_Ps_PolylineFromXPoints \
	(bltTkIntProcsPtr->blt_Ps_PolylineFromXPoints) /* 173 */
#endif
#ifndef Blt_Ps_Polygon
#define Blt_Ps_Polygon \
	(bltTkIntProcsPtr->blt_Ps_Polygon) /* 174 */
#endif
#ifndef Blt_Ps_SetPrinting
#define Blt_Ps_SetPrinting \
	(bltTkIntProcsPtr->blt_Ps_SetPrinting) /* 175 */
#endif
#ifndef Blt_Ts_CreateLayout
#define Blt_Ts_CreateLayout \
	(bltTkIntProcsPtr->blt_Ts_CreateLayout) /* 176 */
#endif
#ifndef Blt_Ts_TitleLayout
#define Blt_Ts_TitleLayout \
	(bltTkIntProcsPtr->blt_Ts_TitleLayout) /* 177 */
#endif
#ifndef Blt_Ts_DrawLayout
#define Blt_Ts_DrawLayout \
	(bltTkIntProcsPtr->blt_Ts_DrawLayout) /* 178 */
#endif
#ifndef Blt_Ts_GetExtents
#define Blt_Ts_GetExtents \
	(bltTkIntProcsPtr->blt_Ts_GetExtents) /* 179 */
#endif
#ifndef Blt_Ts_ResetStyle
#define Blt_Ts_ResetStyle \
	(bltTkIntProcsPtr->blt_Ts_ResetStyle) /* 180 */
#endif
#ifndef Blt_Ts_FreeStyle
#define Blt_Ts_FreeStyle \
	(bltTkIntProcsPtr->blt_Ts_FreeStyle) /* 181 */
#endif
#ifndef Blt_Ts_SetDrawStyle
#define Blt_Ts_SetDrawStyle \
	(bltTkIntProcsPtr->blt_Ts_SetDrawStyle) /* 182 */
#endif
#ifndef Blt_DrawText
#define Blt_DrawText \
	(bltTkIntProcsPtr->blt_DrawText) /* 183 */
#endif
#ifndef Blt_DrawText2
#define Blt_DrawText2 \
	(bltTkIntProcsPtr->blt_DrawText2) /* 184 */
#endif
#ifndef Blt_Ts_Bitmap
#define Blt_Ts_Bitmap \
	(bltTkIntProcsPtr->blt_Ts_Bitmap) /* 185 */
#endif
#ifndef Blt_DrawTextWithRotatedFont
#define Blt_DrawTextWithRotatedFont \
	(bltTkIntProcsPtr->blt_DrawTextWithRotatedFont) /* 186 */
#endif
#ifndef Blt_DrawLayout
#define Blt_DrawLayout \
	(bltTkIntProcsPtr->blt_DrawLayout) /* 187 */
#endif
#ifndef Blt_GetTextExtents
#define Blt_GetTextExtents \
	(bltTkIntProcsPtr->blt_GetTextExtents) /* 188 */
#endif
#ifndef Blt_RotateStartingTextPositions
#define Blt_RotateStartingTextPositions \
	(bltTkIntProcsPtr->blt_RotateStartingTextPositions) /* 189 */
#endif
#ifndef Blt_ComputeTextLayout
#define Blt_ComputeTextLayout \
	(bltTkIntProcsPtr->blt_ComputeTextLayout) /* 190 */
#endif
#ifndef Blt_DrawTextLayout
#define Blt_DrawTextLayout \
	(bltTkIntProcsPtr->blt_DrawTextLayout) /* 191 */
#endif
#ifndef Blt_CharBbox
#define Blt_CharBbox \
	(bltTkIntProcsPtr->blt_CharBbox) /* 192 */
#endif
#ifndef Blt_UnderlineTextLayout
#define Blt_UnderlineTextLayout \
	(bltTkIntProcsPtr->blt_UnderlineTextLayout) /* 193 */
#endif
#ifndef Blt_Ts_UnderlineLayout
#define Blt_Ts_UnderlineLayout \
	(bltTkIntProcsPtr->blt_Ts_UnderlineLayout) /* 194 */
#endif
#ifndef Blt_Ts_DrawText
#define Blt_Ts_DrawText \
	(bltTkIntProcsPtr->blt_Ts_DrawText) /* 195 */
#endif
#ifndef Blt_MeasureText
#define Blt_MeasureText \
	(bltTkIntProcsPtr->blt_MeasureText) /* 196 */
#endif
#ifndef Blt_FreeTextLayout
#define Blt_FreeTextLayout \
	(bltTkIntProcsPtr->blt_FreeTextLayout) /* 197 */
#endif

#endif /* defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TK_PROCS) */

/* !END!: Do not edit above this line. */

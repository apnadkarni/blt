/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "bltPicture.h"
#include "bltPictFmts.h"

/* !BEGIN!: Do not edit below this line. */

/*
 * Exported function declarations:
 */

/* Slot 0 is reserved */
#ifndef Blt_ApplyPictureToPicture_DECLARED
#define Blt_ApplyPictureToPicture_DECLARED
/* 1 */
BLT_EXTERN void		Blt_ApplyPictureToPicture(Blt_Picture dest,
				Blt_Picture src, int x, int y, int w, int h,
				int dx, int dy, Blt_PictureArithOps op);
#endif
#ifndef Blt_ApplyScalarToPicture_DECLARED
#define Blt_ApplyScalarToPicture_DECLARED
/* 2 */
BLT_EXTERN void		Blt_ApplyScalarToPicture(Blt_Picture dest,
				Blt_Pixel *colorPtr, Blt_PictureArithOps op);
#endif
#ifndef Blt_ApplyPictureToPictureWithMask_DECLARED
#define Blt_ApplyPictureToPictureWithMask_DECLARED
/* 3 */
BLT_EXTERN void		Blt_ApplyPictureToPictureWithMask(Blt_Picture dest,
				Blt_Picture src, Blt_Picture mask, int x,
				int y, int w, int h, int dx, int dy,
				int invert, Blt_PictureArithOps op);
#endif
#ifndef Blt_ApplyScalarToPictureWithMask_DECLARED
#define Blt_ApplyScalarToPictureWithMask_DECLARED
/* 4 */
BLT_EXTERN void		Blt_ApplyScalarToPictureWithMask(Blt_Picture dest,
				Blt_Pixel *colorPtr, Blt_Picture mask,
				int invert, Blt_PictureArithOps op);
#endif
#ifndef Blt_MaskPicture_DECLARED
#define Blt_MaskPicture_DECLARED
/* 5 */
BLT_EXTERN void		Blt_MaskPicture(Blt_Picture dest, Blt_Picture mask,
				int x, int y, int w, int h, int dx, int dy,
				Blt_Pixel *colorPtr);
#endif
#ifndef Blt_BlankPicture_DECLARED
#define Blt_BlankPicture_DECLARED
/* 6 */
BLT_EXTERN void		Blt_BlankPicture(Blt_Picture picture,
				unsigned int colorValue);
#endif
#ifndef Blt_BlankRegion_DECLARED
#define Blt_BlankRegion_DECLARED
/* 7 */
BLT_EXTERN void		Blt_BlankRegion(Blt_Picture picture, int x, int y,
				int w, int h, unsigned int colorValue);
#endif
#ifndef Blt_BlurPicture_DECLARED
#define Blt_BlurPicture_DECLARED
/* 8 */
BLT_EXTERN void		Blt_BlurPicture(Blt_Picture dest, Blt_Picture src,
				int radius, int numPasses);
#endif
#ifndef Blt_ResizePicture_DECLARED
#define Blt_ResizePicture_DECLARED
/* 9 */
BLT_EXTERN void		Blt_ResizePicture(Blt_Picture picture, int w, int h);
#endif
#ifndef Blt_AdjustPictureSize_DECLARED
#define Blt_AdjustPictureSize_DECLARED
/* 10 */
BLT_EXTERN void		Blt_AdjustPictureSize(Blt_Picture picture, int w,
				int h);
#endif
#ifndef Blt_ClonePicture_DECLARED
#define Blt_ClonePicture_DECLARED
/* 11 */
BLT_EXTERN Blt_Picture	Blt_ClonePicture(Blt_Picture picture);
#endif
#ifndef Blt_ConvolvePicture_DECLARED
#define Blt_ConvolvePicture_DECLARED
/* 12 */
BLT_EXTERN void		Blt_ConvolvePicture(Blt_Picture dest,
				Blt_Picture src, Blt_ConvolveFilter vFilter,
				Blt_ConvolveFilter hFilter);
#endif
#ifndef Blt_CreatePicture_DECLARED
#define Blt_CreatePicture_DECLARED
/* 13 */
BLT_EXTERN Blt_Picture	Blt_CreatePicture(int w, int h);
#endif
#ifndef Blt_DitherPicture_DECLARED
#define Blt_DitherPicture_DECLARED
/* 14 */
BLT_EXTERN Blt_Picture	Blt_DitherPicture(Blt_Picture picture,
				Blt_Pixel *palette);
#endif
#ifndef Blt_FlipPicture_DECLARED
#define Blt_FlipPicture_DECLARED
/* 15 */
BLT_EXTERN void		Blt_FlipPicture(Blt_Picture picture, int vertically);
#endif
#ifndef Blt_FreePicture_DECLARED
#define Blt_FreePicture_DECLARED
/* 16 */
BLT_EXTERN void		Blt_FreePicture(Blt_Picture picture);
#endif
#ifndef Blt_GreyscalePicture_DECLARED
#define Blt_GreyscalePicture_DECLARED
/* 17 */
BLT_EXTERN Blt_Picture	Blt_GreyscalePicture(Blt_Picture picture);
#endif
#ifndef Blt_QuantizePicture_DECLARED
#define Blt_QuantizePicture_DECLARED
/* 18 */
BLT_EXTERN Blt_Picture	Blt_QuantizePicture(Blt_Picture picture,
				int numColors);
#endif
#ifndef Blt_ResamplePicture_DECLARED
#define Blt_ResamplePicture_DECLARED
/* 19 */
BLT_EXTERN void		Blt_ResamplePicture(Blt_Picture dest,
				Blt_Picture src, Blt_ResampleFilter hFilter,
				Blt_ResampleFilter vFilter);
#endif
#ifndef Blt_ScalePicture_DECLARED
#define Blt_ScalePicture_DECLARED
/* 20 */
BLT_EXTERN Blt_Picture	Blt_ScalePicture(Blt_Picture picture, int x, int y,
				int w, int h, int dw, int dh);
#endif
#ifndef Blt_ScalePictureArea_DECLARED
#define Blt_ScalePictureArea_DECLARED
/* 21 */
BLT_EXTERN Blt_Picture	Blt_ScalePictureArea(Blt_Picture picture, int x,
				int y, int w, int h, int dw, int dh);
#endif
#ifndef Blt_ReflectPicture_DECLARED
#define Blt_ReflectPicture_DECLARED
/* 22 */
BLT_EXTERN Blt_Picture	Blt_ReflectPicture(Blt_Picture picture, int side);
#endif
#ifndef Blt_RotatePictureByShear_DECLARED
#define Blt_RotatePictureByShear_DECLARED
/* 23 */
BLT_EXTERN Blt_Picture	Blt_RotatePictureByShear(Blt_Picture picture,
				float angle);
#endif
#ifndef Blt_RotatePicture_DECLARED
#define Blt_RotatePicture_DECLARED
/* 24 */
BLT_EXTERN Blt_Picture	Blt_RotatePicture(Blt_Picture picture, float angle);
#endif
#ifndef Blt_ProjectPicture_DECLARED
#define Blt_ProjectPicture_DECLARED
/* 25 */
BLT_EXTERN Blt_Picture	Blt_ProjectPicture(Blt_Picture picture,
				float *srcPts, float *destPts, Blt_Pixel *bg);
#endif
#ifndef Blt_TilePicture_DECLARED
#define Blt_TilePicture_DECLARED
/* 26 */
BLT_EXTERN void		Blt_TilePicture(Blt_Picture dest, Blt_Picture src,
				int xOrigin, int yOrigin, int x, int y,
				int w, int h);
#endif
#ifndef Blt_PictureToPsData_DECLARED
#define Blt_PictureToPsData_DECLARED
/* 27 */
BLT_EXTERN int		Blt_PictureToPsData(Blt_Picture picture,
				int numComponents, Tcl_DString *resultPtr,
				const char *prefix);
#endif
#ifndef Blt_SelectPixels_DECLARED
#define Blt_SelectPixels_DECLARED
/* 28 */
BLT_EXTERN void		Blt_SelectPixels(Blt_Picture dest, Blt_Picture src,
				Blt_Pixel *lowerPtr, Blt_Pixel *upperPtr);
#endif
#ifndef Blt_GetPicture_DECLARED
#define Blt_GetPicture_DECLARED
/* 29 */
BLT_EXTERN int		Blt_GetPicture(Tcl_Interp *interp,
				const char *string, Blt_Picture *picturePtr);
#endif
#ifndef Blt_GetPictureFromObj_DECLARED
#define Blt_GetPictureFromObj_DECLARED
/* 30 */
BLT_EXTERN int		Blt_GetPictureFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_Picture *picturePtr);
#endif
#ifndef Blt_GetResampleFilterFromObj_DECLARED
#define Blt_GetResampleFilterFromObj_DECLARED
/* 31 */
BLT_EXTERN int		Blt_GetResampleFilterFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr,
				Blt_ResampleFilter *filterPtr);
#endif
#ifndef Blt_NameOfResampleFilter_DECLARED
#define Blt_NameOfResampleFilter_DECLARED
/* 32 */
BLT_EXTERN const char *	 Blt_NameOfResampleFilter(Blt_ResampleFilter filter);
#endif
#ifndef Blt_PremultiplyColor_DECLARED
#define Blt_PremultiplyColor_DECLARED
/* 33 */
BLT_EXTERN void		Blt_PremultiplyColor(Blt_Pixel *colorPtr);
#endif
#ifndef Blt_UnmultiplyColor_DECLARED
#define Blt_UnmultiplyColor_DECLARED
/* 34 */
BLT_EXTERN void		Blt_UnmultiplyColor(Blt_Pixel *colorPtr);
#endif
#ifndef Blt_PremultiplyColors_DECLARED
#define Blt_PremultiplyColors_DECLARED
/* 35 */
BLT_EXTERN void		Blt_PremultiplyColors(Blt_Picture picture);
#endif
#ifndef Blt_UnmultiplyColors_DECLARED
#define Blt_UnmultiplyColors_DECLARED
/* 36 */
BLT_EXTERN void		Blt_UnmultiplyColors(Blt_Picture picture);
#endif
#ifndef Blt_MultiplyPixels_DECLARED
#define Blt_MultiplyPixels_DECLARED
/* 37 */
BLT_EXTERN void		Blt_MultiplyPixels(Blt_Picture picture, float value);
#endif
#ifndef Blt_GetBBoxFromObjv_DECLARED
#define Blt_GetBBoxFromObjv_DECLARED
/* 38 */
BLT_EXTERN int		Blt_GetBBoxFromObjv(Tcl_Interp *interp, int objc,
				Tcl_Obj *const *objv, PictRegion *regionPtr);
#endif
#ifndef Blt_AdjustRegionToPicture_DECLARED
#define Blt_AdjustRegionToPicture_DECLARED
/* 39 */
BLT_EXTERN int		Blt_AdjustRegionToPicture(Blt_Picture picture,
				PictRegion *regionPtr);
#endif
#ifndef Blt_GetPixelFromObj_DECLARED
#define Blt_GetPixelFromObj_DECLARED
/* 40 */
BLT_EXTERN int		Blt_GetPixelFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_Pixel *pixelPtr);
#endif
#ifndef Blt_GetPixel_DECLARED
#define Blt_GetPixel_DECLARED
/* 41 */
BLT_EXTERN int		Blt_GetPixel(Tcl_Interp *interp, const char *string,
				Blt_Pixel *pixelPtr);
#endif
#ifndef Blt_NameOfPixel_DECLARED
#define Blt_NameOfPixel_DECLARED
/* 42 */
BLT_EXTERN const char *	 Blt_NameOfPixel(Blt_Pixel *pixelPtr);
#endif
#ifndef Blt_NotifyImageChanged_DECLARED
#define Blt_NotifyImageChanged_DECLARED
/* 43 */
BLT_EXTERN void		Blt_NotifyImageChanged(Blt_PictureImage image);
#endif
#ifndef Blt_QueryColors_DECLARED
#define Blt_QueryColors_DECLARED
/* 44 */
BLT_EXTERN int		Blt_QueryColors(Blt_Picture picture,
				Blt_HashTable *tablePtr);
#endif
#ifndef Blt_ClassifyPicture_DECLARED
#define Blt_ClassifyPicture_DECLARED
/* 45 */
BLT_EXTERN void		Blt_ClassifyPicture(Blt_Picture picture);
#endif
#ifndef Blt_TentHorizontally_DECLARED
#define Blt_TentHorizontally_DECLARED
/* 46 */
BLT_EXTERN void		Blt_TentHorizontally(Blt_Picture dest,
				Blt_Picture src);
#endif
#ifndef Blt_TentVertically_DECLARED
#define Blt_TentVertically_DECLARED
/* 47 */
BLT_EXTERN void		Blt_TentVertically(Blt_Picture dest, Blt_Picture src);
#endif
#ifndef Blt_ZoomHorizontally_DECLARED
#define Blt_ZoomHorizontally_DECLARED
/* 48 */
BLT_EXTERN void		Blt_ZoomHorizontally(Blt_Picture dest,
				Blt_Picture src, Blt_ResampleFilter filter);
#endif
#ifndef Blt_ZoomVertically_DECLARED
#define Blt_ZoomVertically_DECLARED
/* 49 */
BLT_EXTERN void		Blt_ZoomVertically(Blt_Picture dest, Blt_Picture src,
				Blt_ResampleFilter filter);
#endif
#ifndef Blt_CompositeRegion_DECLARED
#define Blt_CompositeRegion_DECLARED
/* 50 */
BLT_EXTERN void		Blt_CompositeRegion(Blt_Picture dest, Blt_Picture src,
				int sx, int sy, int w, int h, int dx, int dy);
#endif
#ifndef Blt_ColorBlendPictures_DECLARED
#define Blt_ColorBlendPictures_DECLARED
/* 51 */
BLT_EXTERN void		Blt_ColorBlendPictures(Blt_Picture dest,
				Blt_Picture src, Blt_BlendingMode mode);
#endif
#ifndef Blt_FadePicture_DECLARED
#define Blt_FadePicture_DECLARED
/* 52 */
BLT_EXTERN void		Blt_FadePicture(Blt_Picture picture, int x, int y,
				int w, int h, double factor);
#endif
#ifndef Blt_CopyRegion_DECLARED
#define Blt_CopyRegion_DECLARED
/* 53 */
BLT_EXTERN void		Blt_CopyRegion(Blt_Picture dest,
				Blt_Picture src, int sx, int sy, int w,
				int h, int dx, int dy);
#endif
#ifndef Blt_GammaCorrectPicture_DECLARED
#define Blt_GammaCorrectPicture_DECLARED
/* 54 */
BLT_EXTERN void		Blt_GammaCorrectPicture(Blt_Picture dest,
				Blt_Picture src, float gamma);
#endif
#ifndef Blt_SharpenPicture_DECLARED
#define Blt_SharpenPicture_DECLARED
/* 55 */
BLT_EXTERN void		Blt_SharpenPicture(Blt_Picture dest, Blt_Picture src);
#endif
#ifndef Blt_ApplyColorToPicture_DECLARED
#define Blt_ApplyColorToPicture_DECLARED
/* 56 */
BLT_EXTERN void		Blt_ApplyColorToPicture(Blt_Picture pict,
				Blt_Pixel *colorPtr);
#endif
#ifndef Blt_SizeOfPicture_DECLARED
#define Blt_SizeOfPicture_DECLARED
/* 57 */
BLT_EXTERN void		Blt_SizeOfPicture(Blt_Picture pict, int *wPtr,
				int *hPtr);
#endif
#ifndef Blt_PictureToDBuffer_DECLARED
#define Blt_PictureToDBuffer_DECLARED
/* 58 */
BLT_EXTERN Blt_DBuffer	Blt_PictureToDBuffer(Blt_Picture picture,
				int numComp);
#endif
#ifndef Blt_ResetPicture_DECLARED
#define Blt_ResetPicture_DECLARED
/* 59 */
BLT_EXTERN int		Blt_ResetPicture(Tcl_Interp *interp,
				const char *imageName, Blt_Picture picture);
#endif
#ifndef Blt_MapColors_DECLARED
#define Blt_MapColors_DECLARED
/* 60 */
BLT_EXTERN void		Blt_MapColors(Blt_Picture dest, Blt_Picture src,
				Blt_ColorLookupTable clut);
#endif
#ifndef Blt_GetColorLookupTable_DECLARED
#define Blt_GetColorLookupTable_DECLARED
/* 61 */
BLT_EXTERN Blt_ColorLookupTable Blt_GetColorLookupTable(
				struct _Blt_Chain *chainPtr,
				int numReqColors);
#endif
#ifndef Blt_FadePictureWithGradient_DECLARED
#define Blt_FadePictureWithGradient_DECLARED
/* 62 */
BLT_EXTERN void		Blt_FadePictureWithGradient(Blt_Picture picture,
				int side, double low, double high, int scale,
				Blt_Jitter *jitterPtr);
#endif
#ifndef Blt_ReflectPicture2_DECLARED
#define Blt_ReflectPicture2_DECLARED
/* 63 */
BLT_EXTERN Blt_Picture	Blt_ReflectPicture2(Blt_Picture picture, int side);
#endif
#ifndef Blt_SubtractColor_DECLARED
#define Blt_SubtractColor_DECLARED
/* 64 */
BLT_EXTERN void		Blt_SubtractColor(Blt_Picture picture,
				Blt_Pixel *colorPtr);
#endif
#ifndef Blt_PhotoToPicture_DECLARED
#define Blt_PhotoToPicture_DECLARED
/* 65 */
BLT_EXTERN Blt_Picture	Blt_PhotoToPicture(Tk_PhotoHandle photo);
#endif
#ifndef Blt_PhotoAreaToPicture_DECLARED
#define Blt_PhotoAreaToPicture_DECLARED
/* 66 */
BLT_EXTERN Blt_Picture	Blt_PhotoAreaToPicture(Tk_PhotoHandle photo, int x,
				int y, int w, int h);
#endif
#ifndef Blt_DrawableToPicture_DECLARED
#define Blt_DrawableToPicture_DECLARED
/* 67 */
BLT_EXTERN Blt_Picture	Blt_DrawableToPicture(Tk_Window tkwin,
				Drawable drawable, int x, int y, int w,
				int h, float gamma);
#endif
#ifndef Blt_WindowToPicture_DECLARED
#define Blt_WindowToPicture_DECLARED
/* 68 */
BLT_EXTERN Blt_Picture	Blt_WindowToPicture(Display *display,
				Drawable drawable, int x, int y, int w,
				int h, float gamma);
#endif
#ifndef Blt_PictureToPhoto_DECLARED
#define Blt_PictureToPhoto_DECLARED
/* 69 */
BLT_EXTERN void		Blt_PictureToPhoto(Blt_Picture picture,
				Tk_PhotoHandle photo);
#endif
#ifndef Blt_SnapPhoto_DECLARED
#define Blt_SnapPhoto_DECLARED
/* 70 */
BLT_EXTERN int		Blt_SnapPhoto(Tcl_Interp *interp, Tk_Window tkwin,
				Drawable drawable, int sx, int sy, int w,
				int h, int dw, int dh, const char *photoName,
				float gamma);
#endif
#ifndef Blt_SnapPicture_DECLARED
#define Blt_SnapPicture_DECLARED
/* 71 */
BLT_EXTERN int		Blt_SnapPicture(Tcl_Interp *interp, Tk_Window tkwin,
				Drawable drawable, int sx, int sy, int w,
				int h, int dw, int dh, const char *imageName,
				float gamma);
#endif
#ifndef Blt_XColorToPixel_DECLARED
#define Blt_XColorToPixel_DECLARED
/* 72 */
BLT_EXTERN unsigned int	 Blt_XColorToPixel(XColor *colorPtr);
#endif
#ifndef Blt_IsPicture_DECLARED
#define Blt_IsPicture_DECLARED
/* 73 */
BLT_EXTERN int		Blt_IsPicture(Tk_Image tkImage);
#endif
#ifndef Blt_GetPictureFromImage_DECLARED
#define Blt_GetPictureFromImage_DECLARED
/* 74 */
BLT_EXTERN Blt_Picture	Blt_GetPictureFromImage(Tcl_Interp *interp,
				Tk_Image tkImage, int *isPhotoPtr);
#endif
#ifndef Blt_GetPictureFromPictureImage_DECLARED
#define Blt_GetPictureFromPictureImage_DECLARED
/* 75 */
BLT_EXTERN Blt_Picture	Blt_GetPictureFromPictureImage(Tcl_Interp *interp,
				Tk_Image tkImage);
#endif
#ifndef Blt_GetPicturesFromPictureImage_DECLARED
#define Blt_GetPicturesFromPictureImage_DECLARED
/* 76 */
BLT_EXTERN struct _Blt_Chain * Blt_GetPicturesFromPictureImage(
				Tcl_Interp *interp, Tk_Image tkImage);
#endif
#ifndef Blt_GetPictureFromPhotoImage_DECLARED
#define Blt_GetPictureFromPhotoImage_DECLARED
/* 77 */
BLT_EXTERN Blt_Picture	Blt_GetPictureFromPhotoImage(Tcl_Interp *interp,
				Tk_Image tkImage);
#endif
#ifndef Blt_CanvasToPicture_DECLARED
#define Blt_CanvasToPicture_DECLARED
/* 78 */
BLT_EXTERN Blt_Picture	Blt_CanvasToPicture(Tcl_Interp *interp, Tk_Window tkwin,
				float gamma);
#endif
#ifndef Blt_PictureRegisterProc_DECLARED
#define Blt_PictureRegisterProc_DECLARED
/* 79 */
BLT_EXTERN int		Blt_PictureRegisterProc(Tcl_Interp *interp,
				const char *name, Tcl_ObjCmdProc *proc);
#endif
#ifndef Blt_Shadow_Set_DECLARED
#define Blt_Shadow_Set_DECLARED
/* 80 */
BLT_EXTERN void		Blt_Shadow_Set(Blt_Shadow *sPtr, int width,
				int offset, int color, int alpha);
#endif
#ifndef Blt_EmbossPicture_DECLARED
#define Blt_EmbossPicture_DECLARED
/* 81 */
BLT_EXTERN Blt_Picture	Blt_EmbossPicture(Blt_Picture picture,
				double azimuth, double elevation,
				unsigned short width45);
#endif
#ifndef Blt_FadeColor_DECLARED
#define Blt_FadeColor_DECLARED
/* 82 */
BLT_EXTERN void		Blt_FadeColor(Blt_Pixel *colorPtr,
				unsigned int alpha);
#endif
#ifndef Blt_Dissolve2_DECLARED
#define Blt_Dissolve2_DECLARED
/* 83 */
BLT_EXTERN int		Blt_Dissolve2(Blt_Picture dest, Blt_Picture src,
				long start, int numSteps);
#endif
#ifndef Blt_CrossFade_DECLARED
#define Blt_CrossFade_DECLARED
/* 84 */
BLT_EXTERN void		Blt_CrossFade(Blt_Picture dest, Blt_Picture from,
				Blt_Picture to, double opacity);
#endif
#ifndef Blt_FadeFromColor_DECLARED
#define Blt_FadeFromColor_DECLARED
/* 85 */
BLT_EXTERN void		Blt_FadeFromColor(Blt_Picture dest, Blt_Picture to,
				Blt_Pixel *colorPtr, double opacity);
#endif
#ifndef Blt_FadeToColor_DECLARED
#define Blt_FadeToColor_DECLARED
/* 86 */
BLT_EXTERN void		Blt_FadeToColor(Blt_Picture dest, Blt_Picture from,
				Blt_Pixel *colorPtr, double opacity);
#endif
#ifndef Blt_WipePictures_DECLARED
#define Blt_WipePictures_DECLARED
/* 87 */
BLT_EXTERN void		Blt_WipePictures(Blt_Picture dest, Blt_Picture from,
				Blt_Picture to, int orientation,
				double position);
#endif
#ifndef Blt_PictureRegisterFormat_DECLARED
#define Blt_PictureRegisterFormat_DECLARED
/* 88 */
BLT_EXTERN int		Blt_PictureRegisterFormat(Tcl_Interp *interp,
				const char *name,
				Blt_PictureIsFmtProc *isFmtProc,
				Blt_PictureReadProc *readProc,
				Blt_PictureWriteProc *writeProc,
				Blt_PictureImportProc *importProc,
				Blt_PictureExportProc *exportProc);
#endif
#ifndef Blt_GetNthPicture_DECLARED
#define Blt_GetNthPicture_DECLARED
/* 89 */
BLT_EXTERN Blt_Picture	Blt_GetNthPicture(Blt_Chain chain, size_t index);
#endif
#ifndef Blt_FindPictureFormat_DECLARED
#define Blt_FindPictureFormat_DECLARED
/* 90 */
BLT_EXTERN Blt_PictFormat * Blt_FindPictureFormat(Tcl_Interp *interp,
				const char *ext);
#endif

typedef struct BltTkStubHooks {
    struct BltTkIntProcs *bltTkIntProcs;
} BltTkStubHooks;

typedef struct BltTkProcs {
    int magic;
    struct BltTkStubHooks *hooks;

    void *reserved0;
    void (*blt_ApplyPictureToPicture) (Blt_Picture dest, Blt_Picture src, int x, int y, int w, int h, int dx, int dy, Blt_PictureArithOps op); /* 1 */
    void (*blt_ApplyScalarToPicture) (Blt_Picture dest, Blt_Pixel *colorPtr, Blt_PictureArithOps op); /* 2 */
    void (*blt_ApplyPictureToPictureWithMask) (Blt_Picture dest, Blt_Picture src, Blt_Picture mask, int x, int y, int w, int h, int dx, int dy, int invert, Blt_PictureArithOps op); /* 3 */
    void (*blt_ApplyScalarToPictureWithMask) (Blt_Picture dest, Blt_Pixel *colorPtr, Blt_Picture mask, int invert, Blt_PictureArithOps op); /* 4 */
    void (*blt_MaskPicture) (Blt_Picture dest, Blt_Picture mask, int x, int y, int w, int h, int dx, int dy, Blt_Pixel *colorPtr); /* 5 */
    void (*blt_BlankPicture) (Blt_Picture picture, unsigned int colorValue); /* 6 */
    void (*blt_BlankRegion) (Blt_Picture picture, int x, int y, int w, int h, unsigned int colorValue); /* 7 */
    void (*blt_BlurPicture) (Blt_Picture dest, Blt_Picture src, int radius, int numPasses); /* 8 */
    void (*blt_ResizePicture) (Blt_Picture picture, int w, int h); /* 9 */
    void (*blt_AdjustPictureSize) (Blt_Picture picture, int w, int h); /* 10 */
    Blt_Picture (*blt_ClonePicture) (Blt_Picture picture); /* 11 */
    void (*blt_ConvolvePicture) (Blt_Picture dest, Blt_Picture src, Blt_ConvolveFilter vFilter, Blt_ConvolveFilter hFilter); /* 12 */
    Blt_Picture (*blt_CreatePicture) (int w, int h); /* 13 */
    Blt_Picture (*blt_DitherPicture) (Blt_Picture picture, Blt_Pixel *palette); /* 14 */
    void (*blt_FlipPicture) (Blt_Picture picture, int vertically); /* 15 */
    void (*blt_FreePicture) (Blt_Picture picture); /* 16 */
    Blt_Picture (*blt_GreyscalePicture) (Blt_Picture picture); /* 17 */
    Blt_Picture (*blt_QuantizePicture) (Blt_Picture picture, int numColors); /* 18 */
    void (*blt_ResamplePicture) (Blt_Picture dest, Blt_Picture src, Blt_ResampleFilter hFilter, Blt_ResampleFilter vFilter); /* 19 */
    Blt_Picture (*blt_ScalePicture) (Blt_Picture picture, int x, int y, int w, int h, int dw, int dh); /* 20 */
    Blt_Picture (*blt_ScalePictureArea) (Blt_Picture picture, int x, int y, int w, int h, int dw, int dh); /* 21 */
    Blt_Picture (*blt_ReflectPicture) (Blt_Picture picture, int side); /* 22 */
    Blt_Picture (*blt_RotatePictureByShear) (Blt_Picture picture, float angle); /* 23 */
    Blt_Picture (*blt_RotatePicture) (Blt_Picture picture, float angle); /* 24 */
    Blt_Picture (*blt_ProjectPicture) (Blt_Picture picture, float *srcPts, float *destPts, Blt_Pixel *bg); /* 25 */
    void (*blt_TilePicture) (Blt_Picture dest, Blt_Picture src, int xOrigin, int yOrigin, int x, int y, int w, int h); /* 26 */
    int (*blt_PictureToPsData) (Blt_Picture picture, int numComponents, Tcl_DString *resultPtr, const char *prefix); /* 27 */
    void (*blt_SelectPixels) (Blt_Picture dest, Blt_Picture src, Blt_Pixel *lowerPtr, Blt_Pixel *upperPtr); /* 28 */
    int (*blt_GetPicture) (Tcl_Interp *interp, const char *string, Blt_Picture *picturePtr); /* 29 */
    int (*blt_GetPictureFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Picture *picturePtr); /* 30 */
    int (*blt_GetResampleFilterFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_ResampleFilter *filterPtr); /* 31 */
    const char * (*blt_NameOfResampleFilter) (Blt_ResampleFilter filter); /* 32 */
    void (*blt_AssociateColor) (Blt_Pixel *colorPtr); /* 33 */
    void (*blt_UnassociateColor) (Blt_Pixel *colorPtr); /* 34 */
    void (*blt_AssociateColors) (Blt_Picture picture); /* 35 */
    void (*blt_UnassociateColors) (Blt_Picture picture); /* 36 */
    void (*blt_MultiplyPixels) (Blt_Picture picture, float value); /* 37 */
    int (*blt_GetBBoxFromObjv) (Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, PictRegion *regionPtr); /* 38 */
    int (*blt_AdjustRegionToPicture) (Blt_Picture picture, PictRegion *regionPtr); /* 39 */
    int (*blt_GetPixelFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Pixel *pixelPtr); /* 40 */
    int (*blt_GetPixel) (Tcl_Interp *interp, const char *string, Blt_Pixel *pixelPtr); /* 41 */
    const char * (*blt_NameOfPixel) (Blt_Pixel *pixelPtr); /* 42 */
    void (*blt_NotifyImageChanged) (Blt_PictureImage image); /* 43 */
    int (*blt_QueryColors) (Blt_Picture picture, Blt_HashTable *tablePtr); /* 44 */
    void (*blt_ClassifyPicture) (Blt_Picture picture); /* 45 */
    void (*blt_TentHorizontally) (Blt_Picture dest, Blt_Picture src); /* 46 */
    void (*blt_TentVertically) (Blt_Picture dest, Blt_Picture src); /* 47 */
    void (*blt_ZoomHorizontally) (Blt_Picture dest, Blt_Picture src, Blt_ResampleFilter filter); /* 48 */
    void (*blt_ZoomVertically) (Blt_Picture dest, Blt_Picture src, Blt_ResampleFilter filter); /* 49 */
    void (*blt_CompositeRegion) (Blt_Picture dest, Blt_Picture src, int sx, int sy, int w, int h, int dx, int dy); /* 50 */
    void (*blt_ColorBlendPictures) (Blt_Picture dest, Blt_Picture src, Blt_BlendingMode mode); /* 51 */
    void (*blt_FadePicture) (Blt_Picture picture, int x, int y, int w, int h, double factor); /* 52 */
    void (*blt_CopyRegion) (Blt_Picture dest, Blt_Picture src, int sx, int sy, int w, int h, int dx, int dy); /* 53 */
    void (*blt_GammaCorrectPicture) (Blt_Picture dest, Blt_Picture src, float gamma); /* 54 */
    void (*blt_SharpenPicture) (Blt_Picture dest, Blt_Picture src); /* 55 */
    void (*blt_ApplyColorToPicture) (Blt_Picture pict, Blt_Pixel *colorPtr); /* 56 */
    void (*blt_SizeOfPicture) (Blt_Picture pict, int *wPtr, int *hPtr); /* 57 */
    Blt_DBuffer (*blt_PictureToDBuffer) (Blt_Picture picture, int numComp); /* 58 */
    int (*blt_ResetPicture) (Tcl_Interp *interp, const char *imageName, Blt_Picture picture); /* 59 */
    void (*blt_MapColors) (Blt_Picture dest, Blt_Picture src, Blt_ColorLookupTable clut); /* 60 */
    Blt_ColorLookupTable (*blt_GetColorLookupTable) (struct _Blt_Chain *chainPtr, int numReqColors); /* 61 */
    void (*blt_FadePictureWithGradient) (Blt_Picture picture, int side, double low, double high, int scale, Blt_Jitter *jitterPtr); /* 62 */
    Blt_Picture (*blt_ReflectPicture2) (Blt_Picture picture, int side); /* 63 */
    void (*blt_SubtractColor) (Blt_Picture picture, Blt_Pixel *colorPtr); /* 64 */
    Blt_Picture (*blt_PhotoToPicture) (Tk_PhotoHandle photo); /* 65 */
    Blt_Picture (*blt_PhotoAreaToPicture) (Tk_PhotoHandle photo, int x, int y, int w, int h); /* 66 */
    Blt_Picture (*blt_DrawableToPicture) (Tk_Window tkwin, Drawable drawable, int x, int y, int w, int h, float gamma); /* 67 */
    Blt_Picture (*blt_WindowToPicture) (Display *display, Drawable drawable, int x, int y, int w, int h, float gamma); /* 68 */
    void (*blt_PictureToPhoto) (Blt_Picture picture, Tk_PhotoHandle photo); /* 69 */
    int (*blt_SnapPhoto) (Tcl_Interp *interp, Tk_Window tkwin, Drawable drawable, int sx, int sy, int w, int h, int dw, int dh, const char *photoName, float gamma); /* 70 */
    int (*blt_SnapPicture) (Tcl_Interp *interp, Tk_Window tkwin, Drawable drawable, int sx, int sy, int w, int h, int dw, int dh, const char *imageName, float gamma); /* 71 */
    unsigned int (*blt_XColorToPixel) (XColor *colorPtr); /* 72 */
    int (*blt_IsPicture) (Tk_Image tkImage); /* 73 */
    Blt_Picture (*blt_GetPictureFromImage) (Tcl_Interp *interp, Tk_Image tkImage, int *isPhotoPtr); /* 74 */
    Blt_Picture (*blt_GetPictureFromPictureImage) (Tcl_Interp *interp, Tk_Image tkImage); /* 75 */
    struct _Blt_Chain * (*blt_GetPicturesFromPictureImage) (Tcl_Interp *interp, Tk_Image tkImage); /* 76 */
    Blt_Picture (*blt_GetPictureFromPhotoImage) (Tcl_Interp *interp, Tk_Image tkImage); /* 77 */
    Blt_Picture (*blt_CanvasToPicture) (Tcl_Interp *interp, Tk_Window tkwin, float gamma); /* 78 */
    int (*blt_PictureRegisterProc) (Tcl_Interp *interp, const char *name, Tcl_ObjCmdProc *proc); /* 79 */
    void (*blt_Shadow_Set) (Blt_Shadow *sPtr, int width, int offset, int color, int alpha); /* 80 */
    Blt_Picture (*blt_EmbossPicture) (Blt_Picture picture, double azimuth, double elevation, unsigned short width45); /* 81 */
    void (*blt_FadeColor) (Blt_Pixel *colorPtr, unsigned int alpha); /* 82 */
    int (*blt_Dissolve2) (Blt_Picture dest, Blt_Picture src, long start, int numSteps); /* 83 */
    void (*blt_CrossFade) (Blt_Picture dest, Blt_Picture from, Blt_Picture to, double opacity); /* 84 */
    void (*blt_FadeFromColor) (Blt_Picture dest, Blt_Picture to, Blt_Pixel *colorPtr, double opacity); /* 85 */
    void (*blt_FadeToColor) (Blt_Picture dest, Blt_Picture from, Blt_Pixel *colorPtr, double opacity); /* 86 */
    void (*blt_WipePictures) (Blt_Picture dest, Blt_Picture from, Blt_Picture to, int orientation, double position); /* 87 */
    int (*blt_PictureRegisterFormat) (Tcl_Interp *interp, const char *name, Blt_PictureIsFmtProc *isFmtProc, Blt_PictureReadProc *readProc, Blt_PictureWriteProc *writeProc, Blt_PictureImportProc *importProc, Blt_PictureExportProc *exportProc); /* 88 */
    Blt_Picture (*blt_GetNthPicture) (Blt_Chain chain, size_t index); /* 89 */
    Blt_PictFormat * (*blt_FindPictureFormat) (Tcl_Interp *interp, const char *ext); /* 90 */
} BltTkProcs;

#ifdef __cplusplus
extern "C" {
#endif
extern BltTkProcs *bltTkProcsPtr;
#ifdef __cplusplus
}
#endif

#if defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TK_PROCS)

/*
 * Inline function declarations:
 */

/* Slot 0 is reserved */
#ifndef Blt_ApplyPictureToPicture
#define Blt_ApplyPictureToPicture \
	(bltTkProcsPtr->blt_ApplyPictureToPicture) /* 1 */
#endif
#ifndef Blt_ApplyScalarToPicture
#define Blt_ApplyScalarToPicture \
	(bltTkProcsPtr->blt_ApplyScalarToPicture) /* 2 */
#endif
#ifndef Blt_ApplyPictureToPictureWithMask
#define Blt_ApplyPictureToPictureWithMask \
	(bltTkProcsPtr->blt_ApplyPictureToPictureWithMask) /* 3 */
#endif
#ifndef Blt_ApplyScalarToPictureWithMask
#define Blt_ApplyScalarToPictureWithMask \
	(bltTkProcsPtr->blt_ApplyScalarToPictureWithMask) /* 4 */
#endif
#ifndef Blt_MaskPicture
#define Blt_MaskPicture \
	(bltTkProcsPtr->blt_MaskPicture) /* 5 */
#endif
#ifndef Blt_BlankPicture
#define Blt_BlankPicture \
	(bltTkProcsPtr->blt_BlankPicture) /* 6 */
#endif
#ifndef Blt_BlankRegion
#define Blt_BlankRegion \
	(bltTkProcsPtr->blt_BlankRegion) /* 7 */
#endif
#ifndef Blt_BlurPicture
#define Blt_BlurPicture \
	(bltTkProcsPtr->blt_BlurPicture) /* 8 */
#endif
#ifndef Blt_ResizePicture
#define Blt_ResizePicture \
	(bltTkProcsPtr->blt_ResizePicture) /* 9 */
#endif
#ifndef Blt_AdjustPictureSize
#define Blt_AdjustPictureSize \
	(bltTkProcsPtr->blt_AdjustPictureSize) /* 10 */
#endif
#ifndef Blt_ClonePicture
#define Blt_ClonePicture \
	(bltTkProcsPtr->blt_ClonePicture) /* 11 */
#endif
#ifndef Blt_ConvolvePicture
#define Blt_ConvolvePicture \
	(bltTkProcsPtr->blt_ConvolvePicture) /* 12 */
#endif
#ifndef Blt_CreatePicture
#define Blt_CreatePicture \
	(bltTkProcsPtr->blt_CreatePicture) /* 13 */
#endif
#ifndef Blt_DitherPicture
#define Blt_DitherPicture \
	(bltTkProcsPtr->blt_DitherPicture) /* 14 */
#endif
#ifndef Blt_FlipPicture
#define Blt_FlipPicture \
	(bltTkProcsPtr->blt_FlipPicture) /* 15 */
#endif
#ifndef Blt_FreePicture
#define Blt_FreePicture \
	(bltTkProcsPtr->blt_FreePicture) /* 16 */
#endif
#ifndef Blt_GreyscalePicture
#define Blt_GreyscalePicture \
	(bltTkProcsPtr->blt_GreyscalePicture) /* 17 */
#endif
#ifndef Blt_QuantizePicture
#define Blt_QuantizePicture \
	(bltTkProcsPtr->blt_QuantizePicture) /* 18 */
#endif
#ifndef Blt_ResamplePicture
#define Blt_ResamplePicture \
	(bltTkProcsPtr->blt_ResamplePicture) /* 19 */
#endif
#ifndef Blt_ScalePicture
#define Blt_ScalePicture \
	(bltTkProcsPtr->blt_ScalePicture) /* 20 */
#endif
#ifndef Blt_ScalePictureArea
#define Blt_ScalePictureArea \
	(bltTkProcsPtr->blt_ScalePictureArea) /* 21 */
#endif
#ifndef Blt_ReflectPicture
#define Blt_ReflectPicture \
	(bltTkProcsPtr->blt_ReflectPicture) /* 22 */
#endif
#ifndef Blt_RotatePictureByShear
#define Blt_RotatePictureByShear \
	(bltTkProcsPtr->blt_RotatePictureByShear) /* 23 */
#endif
#ifndef Blt_RotatePicture
#define Blt_RotatePicture \
	(bltTkProcsPtr->blt_RotatePicture) /* 24 */
#endif
#ifndef Blt_ProjectPicture
#define Blt_ProjectPicture \
	(bltTkProcsPtr->blt_ProjectPicture) /* 25 */
#endif
#ifndef Blt_TilePicture
#define Blt_TilePicture \
	(bltTkProcsPtr->blt_TilePicture) /* 26 */
#endif
#ifndef Blt_PictureToPsData
#define Blt_PictureToPsData \
	(bltTkProcsPtr->blt_PictureToPsData) /* 27 */
#endif
#ifndef Blt_SelectPixels
#define Blt_SelectPixels \
	(bltTkProcsPtr->blt_SelectPixels) /* 28 */
#endif
#ifndef Blt_GetPicture
#define Blt_GetPicture \
	(bltTkProcsPtr->blt_GetPicture) /* 29 */
#endif
#ifndef Blt_GetPictureFromObj
#define Blt_GetPictureFromObj \
	(bltTkProcsPtr->blt_GetPictureFromObj) /* 30 */
#endif
#ifndef Blt_GetResampleFilterFromObj
#define Blt_GetResampleFilterFromObj \
	(bltTkProcsPtr->blt_GetResampleFilterFromObj) /* 31 */
#endif
#ifndef Blt_NameOfResampleFilter
#define Blt_NameOfResampleFilter \
	(bltTkProcsPtr->blt_NameOfResampleFilter) /* 32 */
#endif
#ifndef Blt_PremultiplyColor
#define Blt_PremultiplyColor \
	(bltTkProcsPtr->blt_AssociateColor) /* 33 */
#endif
#ifndef Blt_UnmultiplyColor
#define Blt_UnmultiplyColor \
	(bltTkProcsPtr->blt_UnassociateColor) /* 34 */
#endif
#ifndef Blt_PremultiplyColors
#define Blt_PremultiplyColors \
	(bltTkProcsPtr->blt_AssociateColors) /* 35 */
#endif
#ifndef Blt_UnmultiplyColors
#define Blt_UnmultiplyColors \
	(bltTkProcsPtr->blt_UnassociateColors) /* 36 */
#endif
#ifndef Blt_MultiplyPixels
#define Blt_MultiplyPixels \
	(bltTkProcsPtr->blt_MultiplyPixels) /* 37 */
#endif
#ifndef Blt_GetBBoxFromObjv
#define Blt_GetBBoxFromObjv \
	(bltTkProcsPtr->blt_GetBBoxFromObjv) /* 38 */
#endif
#ifndef Blt_AdjustRegionToPicture
#define Blt_AdjustRegionToPicture \
	(bltTkProcsPtr->blt_AdjustRegionToPicture) /* 39 */
#endif
#ifndef Blt_GetPixelFromObj
#define Blt_GetPixelFromObj \
	(bltTkProcsPtr->blt_GetPixelFromObj) /* 40 */
#endif
#ifndef Blt_GetPixel
#define Blt_GetPixel \
	(bltTkProcsPtr->blt_GetPixel) /* 41 */
#endif
#ifndef Blt_NameOfPixel
#define Blt_NameOfPixel \
	(bltTkProcsPtr->blt_NameOfPixel) /* 42 */
#endif
#ifndef Blt_NotifyImageChanged
#define Blt_NotifyImageChanged \
	(bltTkProcsPtr->blt_NotifyImageChanged) /* 43 */
#endif
#ifndef Blt_QueryColors
#define Blt_QueryColors \
	(bltTkProcsPtr->blt_QueryColors) /* 44 */
#endif
#ifndef Blt_ClassifyPicture
#define Blt_ClassifyPicture \
	(bltTkProcsPtr->blt_ClassifyPicture) /* 45 */
#endif
#ifndef Blt_TentHorizontally
#define Blt_TentHorizontally \
	(bltTkProcsPtr->blt_TentHorizontally) /* 46 */
#endif
#ifndef Blt_TentVertically
#define Blt_TentVertically \
	(bltTkProcsPtr->blt_TentVertically) /* 47 */
#endif
#ifndef Blt_ZoomHorizontally
#define Blt_ZoomHorizontally \
	(bltTkProcsPtr->blt_ZoomHorizontally) /* 48 */
#endif
#ifndef Blt_ZoomVertically
#define Blt_ZoomVertically \
	(bltTkProcsPtr->blt_ZoomVertically) /* 49 */
#endif
#ifndef Blt_CompositeRegion
#define Blt_CompositeRegion \
	(bltTkProcsPtr->blt_CompositeRegion) /* 50 */
#endif
#ifndef Blt_ColorBlendPictures
#define Blt_ColorBlendPictures \
	(bltTkProcsPtr->blt_ColorBlendPictures) /* 51 */
#endif
#ifndef Blt_FadePicture
#define Blt_FadePicture \
	(bltTkProcsPtr->blt_FadePicture) /* 52 */
#endif
#ifndef Blt_CopyRegion
#define Blt_CopyRegion \
	(bltTkProcsPtr->blt_CopyRegion) /* 53 */
#endif
#ifndef Blt_GammaCorrectPicture
#define Blt_GammaCorrectPicture \
	(bltTkProcsPtr->blt_GammaCorrectPicture) /* 54 */
#endif
#ifndef Blt_SharpenPicture
#define Blt_SharpenPicture \
	(bltTkProcsPtr->blt_SharpenPicture) /* 55 */
#endif
#ifndef Blt_ApplyColorToPicture
#define Blt_ApplyColorToPicture \
	(bltTkProcsPtr->blt_ApplyColorToPicture) /* 56 */
#endif
#ifndef Blt_SizeOfPicture
#define Blt_SizeOfPicture \
	(bltTkProcsPtr->blt_SizeOfPicture) /* 57 */
#endif
#ifndef Blt_PictureToDBuffer
#define Blt_PictureToDBuffer \
	(bltTkProcsPtr->blt_PictureToDBuffer) /* 58 */
#endif
#ifndef Blt_ResetPicture
#define Blt_ResetPicture \
	(bltTkProcsPtr->blt_ResetPicture) /* 59 */
#endif
#ifndef Blt_MapColors
#define Blt_MapColors \
	(bltTkProcsPtr->blt_MapColors) /* 60 */
#endif
#ifndef Blt_GetColorLookupTable
#define Blt_GetColorLookupTable \
	(bltTkProcsPtr->blt_GetColorLookupTable) /* 61 */
#endif
#ifndef Blt_FadePictureWithGradient
#define Blt_FadePictureWithGradient \
	(bltTkProcsPtr->blt_FadePictureWithGradient) /* 62 */
#endif
#ifndef Blt_ReflectPicture2
#define Blt_ReflectPicture2 \
	(bltTkProcsPtr->blt_ReflectPicture2) /* 63 */
#endif
#ifndef Blt_SubtractColor
#define Blt_SubtractColor \
	(bltTkProcsPtr->blt_SubtractColor) /* 64 */
#endif
#ifndef Blt_PhotoToPicture
#define Blt_PhotoToPicture \
	(bltTkProcsPtr->blt_PhotoToPicture) /* 65 */
#endif
#ifndef Blt_PhotoAreaToPicture
#define Blt_PhotoAreaToPicture \
	(bltTkProcsPtr->blt_PhotoAreaToPicture) /* 66 */
#endif
#ifndef Blt_DrawableToPicture
#define Blt_DrawableToPicture \
	(bltTkProcsPtr->blt_DrawableToPicture) /* 67 */
#endif
#ifndef Blt_WindowToPicture
#define Blt_WindowToPicture \
	(bltTkProcsPtr->blt_WindowToPicture) /* 68 */
#endif
#ifndef Blt_PictureToPhoto
#define Blt_PictureToPhoto \
	(bltTkProcsPtr->blt_PictureToPhoto) /* 69 */
#endif
#ifndef Blt_SnapPhoto
#define Blt_SnapPhoto \
	(bltTkProcsPtr->blt_SnapPhoto) /* 70 */
#endif
#ifndef Blt_SnapPicture
#define Blt_SnapPicture \
	(bltTkProcsPtr->blt_SnapPicture) /* 71 */
#endif
#ifndef Blt_XColorToPixel
#define Blt_XColorToPixel \
	(bltTkProcsPtr->blt_XColorToPixel) /* 72 */
#endif
#ifndef Blt_IsPicture
#define Blt_IsPicture \
	(bltTkProcsPtr->blt_IsPicture) /* 73 */
#endif
#ifndef Blt_GetPictureFromImage
#define Blt_GetPictureFromImage \
	(bltTkProcsPtr->blt_GetPictureFromImage) /* 74 */
#endif
#ifndef Blt_GetPictureFromPictureImage
#define Blt_GetPictureFromPictureImage \
	(bltTkProcsPtr->blt_GetPictureFromPictureImage) /* 75 */
#endif
#ifndef Blt_GetPicturesFromPictureImage
#define Blt_GetPicturesFromPictureImage \
	(bltTkProcsPtr->blt_GetPicturesFromPictureImage) /* 76 */
#endif
#ifndef Blt_GetPictureFromPhotoImage
#define Blt_GetPictureFromPhotoImage \
	(bltTkProcsPtr->blt_GetPictureFromPhotoImage) /* 77 */
#endif
#ifndef Blt_CanvasToPicture
#define Blt_CanvasToPicture \
	(bltTkProcsPtr->blt_CanvasToPicture) /* 78 */
#endif
#ifndef Blt_PictureRegisterProc
#define Blt_PictureRegisterProc \
	(bltTkProcsPtr->blt_PictureRegisterProc) /* 79 */
#endif
#ifndef Blt_Shadow_Set
#define Blt_Shadow_Set \
	(bltTkProcsPtr->blt_Shadow_Set) /* 80 */
#endif
#ifndef Blt_EmbossPicture
#define Blt_EmbossPicture \
	(bltTkProcsPtr->blt_EmbossPicture) /* 81 */
#endif
#ifndef Blt_FadeColor
#define Blt_FadeColor \
	(bltTkProcsPtr->blt_FadeColor) /* 82 */
#endif
#ifndef Blt_Dissolve2
#define Blt_Dissolve2 \
	(bltTkProcsPtr->blt_Dissolve2) /* 83 */
#endif
#ifndef Blt_CrossFade
#define Blt_CrossFade \
	(bltTkProcsPtr->blt_CrossFade) /* 84 */
#endif
#ifndef Blt_FadeFromColor
#define Blt_FadeFromColor \
	(bltTkProcsPtr->blt_FadeFromColor) /* 85 */
#endif
#ifndef Blt_FadeToColor
#define Blt_FadeToColor \
	(bltTkProcsPtr->blt_FadeToColor) /* 86 */
#endif
#ifndef Blt_WipePictures
#define Blt_WipePictures \
	(bltTkProcsPtr->blt_WipePictures) /* 87 */
#endif
#ifndef Blt_PictureRegisterFormat
#define Blt_PictureRegisterFormat \
	(bltTkProcsPtr->blt_PictureRegisterFormat) /* 88 */
#endif
#ifndef Blt_GetNthPicture
#define Blt_GetNthPicture \
	(bltTkProcsPtr->blt_GetNthPicture) /* 89 */
#endif
#ifndef Blt_FindPictureFormat
#define Blt_FindPictureFormat \
	(bltTkProcsPtr->blt_FindPictureFormat) /* 90 */
#endif

#endif /* defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TK_PROCS) */

/* !END!: Do not edit above this line. */

#include "bltPicture.h"
#include "bltPictFmts.h"

/* !BEGIN!: Do not edit below this line. */

/*
 * Exported function declarations:
 */

/* Slot 0 is reserved */
#ifndef Blt_Jitter_Init_DECLARED
#define Blt_Jitter_Init_DECLARED
/* 1 */
BLT_EXTERN void		Blt_Jitter_Init(Blt_Jitter *jitterPtr);
#endif
#ifndef Blt_ApplyPictureToPicture_DECLARED
#define Blt_ApplyPictureToPicture_DECLARED
/* 2 */
BLT_EXTERN void		Blt_ApplyPictureToPicture(Blt_Picture dest,
				Blt_Picture src, int x, int y, int w, int h,
				int dx, int dy, Blt_PictureArithOps op);
#endif
#ifndef Blt_ApplyScalarToPicture_DECLARED
#define Blt_ApplyScalarToPicture_DECLARED
/* 3 */
BLT_EXTERN void		Blt_ApplyScalarToPicture(Blt_Picture dest,
				Blt_Pixel *colorPtr, Blt_PictureArithOps op);
#endif
#ifndef Blt_ApplyPictureToPictureWithMask_DECLARED
#define Blt_ApplyPictureToPictureWithMask_DECLARED
/* 4 */
BLT_EXTERN void		Blt_ApplyPictureToPictureWithMask(Blt_Picture dest,
				Blt_Picture src, Blt_Picture mask, int x,
				int y, int w, int h, int dx, int dy,
				int invert, Blt_PictureArithOps op);
#endif
#ifndef Blt_ApplyScalarToPictureWithMask_DECLARED
#define Blt_ApplyScalarToPictureWithMask_DECLARED
/* 5 */
BLT_EXTERN void		Blt_ApplyScalarToPictureWithMask(Blt_Picture dest,
				Blt_Pixel *colorPtr, Blt_Picture mask,
				int invert, Blt_PictureArithOps op);
#endif
#ifndef Blt_MaskPicture_DECLARED
#define Blt_MaskPicture_DECLARED
/* 6 */
BLT_EXTERN void		Blt_MaskPicture(Blt_Picture dest, Blt_Picture mask,
				int x, int y, int w, int h, int dx, int dy,
				Blt_Pixel *colorPtr);
#endif
#ifndef Blt_BlankPicture_DECLARED
#define Blt_BlankPicture_DECLARED
/* 7 */
BLT_EXTERN void		Blt_BlankPicture(Blt_Picture picture,
				unsigned int colorValue);
#endif
#ifndef Blt_BlankRegion_DECLARED
#define Blt_BlankRegion_DECLARED
/* 8 */
BLT_EXTERN void		Blt_BlankRegion(Blt_Picture picture, int x, int y,
				int w, int h, unsigned int colorValue);
#endif
#ifndef Blt_BlurPicture_DECLARED
#define Blt_BlurPicture_DECLARED
/* 9 */
BLT_EXTERN void		Blt_BlurPicture(Blt_Picture dest, Blt_Picture src,
				int radius, int numPasses);
#endif
#ifndef Blt_ResizePicture_DECLARED
#define Blt_ResizePicture_DECLARED
/* 10 */
BLT_EXTERN void		Blt_ResizePicture(Blt_Picture picture, int w, int h);
#endif
#ifndef Blt_AdjustPictureSize_DECLARED
#define Blt_AdjustPictureSize_DECLARED
/* 11 */
BLT_EXTERN void		Blt_AdjustPictureSize(Blt_Picture picture, int w,
				int h);
#endif
#ifndef Blt_ClonePicture_DECLARED
#define Blt_ClonePicture_DECLARED
/* 12 */
BLT_EXTERN Blt_Picture	Blt_ClonePicture(Blt_Picture picture);
#endif
#ifndef Blt_ConvolvePicture_DECLARED
#define Blt_ConvolvePicture_DECLARED
/* 13 */
BLT_EXTERN void		Blt_ConvolvePicture(Blt_Picture dest,
				Blt_Picture src, Blt_ConvolveFilter vFilter,
				Blt_ConvolveFilter hFilter);
#endif
#ifndef Blt_CreatePicture_DECLARED
#define Blt_CreatePicture_DECLARED
/* 14 */
BLT_EXTERN Blt_Picture	Blt_CreatePicture(int w, int h);
#endif
#ifndef Blt_DitherPicture_DECLARED
#define Blt_DitherPicture_DECLARED
/* 15 */
BLT_EXTERN Blt_Picture	Blt_DitherPicture(Blt_Picture picture,
				Blt_Pixel *palette);
#endif
#ifndef Blt_FlipPicture_DECLARED
#define Blt_FlipPicture_DECLARED
/* 16 */
BLT_EXTERN void		Blt_FlipPicture(Blt_Picture picture, int vertically);
#endif
#ifndef Blt_FreePicture_DECLARED
#define Blt_FreePicture_DECLARED
/* 17 */
BLT_EXTERN void		Blt_FreePicture(Blt_Picture picture);
#endif
#ifndef Blt_GreyscalePicture_DECLARED
#define Blt_GreyscalePicture_DECLARED
/* 18 */
BLT_EXTERN Blt_Picture	Blt_GreyscalePicture(Blt_Picture picture);
#endif
#ifndef Blt_QuantizePicture_DECLARED
#define Blt_QuantizePicture_DECLARED
/* 19 */
BLT_EXTERN Blt_Picture	Blt_QuantizePicture(Blt_Picture picture,
				int numColors);
#endif
#ifndef Blt_ResamplePicture_DECLARED
#define Blt_ResamplePicture_DECLARED
/* 20 */
BLT_EXTERN void		Blt_ResamplePicture(Blt_Picture dest,
				Blt_Picture src, Blt_ResampleFilter hFilter,
				Blt_ResampleFilter vFilter);
#endif
#ifndef Blt_ScalePicture_DECLARED
#define Blt_ScalePicture_DECLARED
/* 21 */
BLT_EXTERN Blt_Picture	Blt_ScalePicture(Blt_Picture picture, int x, int y,
				int w, int h, int dw, int dh);
#endif
#ifndef Blt_ScalePictureArea_DECLARED
#define Blt_ScalePictureArea_DECLARED
/* 22 */
BLT_EXTERN Blt_Picture	Blt_ScalePictureArea(Blt_Picture picture, int x,
				int y, int w, int h, int dw, int dh);
#endif
#ifndef Blt_ReflectPicture_DECLARED
#define Blt_ReflectPicture_DECLARED
/* 23 */
BLT_EXTERN Blt_Picture	Blt_ReflectPicture(Blt_Picture picture, int side);
#endif
#ifndef Blt_RotatePictureByShear_DECLARED
#define Blt_RotatePictureByShear_DECLARED
/* 24 */
BLT_EXTERN Blt_Picture	Blt_RotatePictureByShear(Blt_Picture picture,
				float angle);
#endif
#ifndef Blt_RotatePicture_DECLARED
#define Blt_RotatePicture_DECLARED
/* 25 */
BLT_EXTERN Blt_Picture	Blt_RotatePicture(Blt_Picture picture, float angle);
#endif
#ifndef Blt_ProjectPicture_DECLARED
#define Blt_ProjectPicture_DECLARED
/* 26 */
BLT_EXTERN Blt_Picture	Blt_ProjectPicture(Blt_Picture picture,
				float *srcPts, float *destPts, Blt_Pixel *bg);
#endif
#ifndef Blt_TilePicture_DECLARED
#define Blt_TilePicture_DECLARED
/* 27 */
BLT_EXTERN void		Blt_TilePicture(Blt_Picture dest, Blt_Picture src,
				int xOrigin, int yOrigin, int x, int y,
				int w, int h);
#endif
#ifndef Blt_PictureToPsData_DECLARED
#define Blt_PictureToPsData_DECLARED
/* 28 */
BLT_EXTERN int		Blt_PictureToPsData(Blt_Picture picture,
				int numComponents, Tcl_DString *resultPtr,
				const char *prefix);
#endif
#ifndef Blt_SelectPixels_DECLARED
#define Blt_SelectPixels_DECLARED
/* 29 */
BLT_EXTERN void		Blt_SelectPixels(Blt_Picture dest, Blt_Picture src,
				Blt_Pixel *lowerPtr, Blt_Pixel *upperPtr);
#endif
#ifndef Blt_GetPicture_DECLARED
#define Blt_GetPicture_DECLARED
/* 30 */
BLT_EXTERN int		Blt_GetPicture(Tcl_Interp *interp,
				const char *string, Blt_Picture *picturePtr);
#endif
#ifndef Blt_GetPictureFromObj_DECLARED
#define Blt_GetPictureFromObj_DECLARED
/* 31 */
BLT_EXTERN int		Blt_GetPictureFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_Picture *picturePtr);
#endif
#ifndef Blt_GetResampleFilterFromObj_DECLARED
#define Blt_GetResampleFilterFromObj_DECLARED
/* 32 */
BLT_EXTERN int		Blt_GetResampleFilterFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr,
				Blt_ResampleFilter *filterPtr);
#endif
#ifndef Blt_NameOfResampleFilter_DECLARED
#define Blt_NameOfResampleFilter_DECLARED
/* 33 */
BLT_EXTERN const char *	 Blt_NameOfResampleFilter(Blt_ResampleFilter filter);
#endif
#ifndef Blt_AssociateColors_DECLARED
#define Blt_AssociateColors_DECLARED
/* 34 */
BLT_EXTERN void		Blt_AssociateColors(Blt_Picture picture);
#endif
#ifndef Blt_UnassociateColors_DECLARED
#define Blt_UnassociateColors_DECLARED
/* 35 */
BLT_EXTERN void		Blt_UnassociateColors(Blt_Picture picture);
#endif
#ifndef Blt_MultiplyPixels_DECLARED
#define Blt_MultiplyPixels_DECLARED
/* 36 */
BLT_EXTERN void		Blt_MultiplyPixels(Blt_Picture picture, float value);
#endif
#ifndef Blt_GetBBoxFromObjv_DECLARED
#define Blt_GetBBoxFromObjv_DECLARED
/* 37 */
BLT_EXTERN int		Blt_GetBBoxFromObjv(Tcl_Interp *interp, int objc,
				Tcl_Obj *const *objv, PictRegion *regionPtr);
#endif
#ifndef Blt_AdjustRegionToPicture_DECLARED
#define Blt_AdjustRegionToPicture_DECLARED
/* 38 */
BLT_EXTERN int		Blt_AdjustRegionToPicture(Blt_Picture picture,
				PictRegion *regionPtr);
#endif
#ifndef Blt_GetPixelFromObj_DECLARED
#define Blt_GetPixelFromObj_DECLARED
/* 39 */
BLT_EXTERN int		Blt_GetPixelFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_Pixel *pixelPtr);
#endif
#ifndef Blt_GetPixel_DECLARED
#define Blt_GetPixel_DECLARED
/* 40 */
BLT_EXTERN int		Blt_GetPixel(Tcl_Interp *interp, const char *string,
				Blt_Pixel *pixelPtr);
#endif
#ifndef Blt_NameOfPixel_DECLARED
#define Blt_NameOfPixel_DECLARED
/* 41 */
BLT_EXTERN const char *	 Blt_NameOfPixel(Blt_Pixel *pixelPtr);
#endif
#ifndef Blt_NotifyImageChanged_DECLARED
#define Blt_NotifyImageChanged_DECLARED
/* 42 */
BLT_EXTERN void		Blt_NotifyImageChanged(Blt_PictureImage image);
#endif
#ifndef Blt_QueryColors_DECLARED
#define Blt_QueryColors_DECLARED
/* 43 */
BLT_EXTERN int		Blt_QueryColors(Blt_Picture picture,
				Blt_HashTable *tablePtr);
#endif
#ifndef Blt_ClassifyPicture_DECLARED
#define Blt_ClassifyPicture_DECLARED
/* 44 */
BLT_EXTERN void		Blt_ClassifyPicture(Blt_Picture picture);
#endif
#ifndef Blt_TentHorizontally_DECLARED
#define Blt_TentHorizontally_DECLARED
/* 45 */
BLT_EXTERN void		Blt_TentHorizontally(Blt_Picture dest,
				Blt_Picture src);
#endif
#ifndef Blt_TentVertically_DECLARED
#define Blt_TentVertically_DECLARED
/* 46 */
BLT_EXTERN void		Blt_TentVertically(Blt_Picture dest, Blt_Picture src);
#endif
#ifndef Blt_ZoomHorizontally_DECLARED
#define Blt_ZoomHorizontally_DECLARED
/* 47 */
BLT_EXTERN void		Blt_ZoomHorizontally(Blt_Picture dest,
				Blt_Picture src, Blt_ResampleFilter filter);
#endif
#ifndef Blt_ZoomVertically_DECLARED
#define Blt_ZoomVertically_DECLARED
/* 48 */
BLT_EXTERN void		Blt_ZoomVertically(Blt_Picture dest, Blt_Picture src,
				Blt_ResampleFilter filter);
#endif
#ifndef Blt_BlendPictures_DECLARED
#define Blt_BlendPictures_DECLARED
/* 49 */
BLT_EXTERN void		Blt_BlendPictures(Blt_Picture dest, Blt_Picture src,
				int sx, int sy, int w, int h, int dx, int dy);
#endif
#ifndef Blt_BlendPicturesByMode_DECLARED
#define Blt_BlendPicturesByMode_DECLARED
/* 50 */
BLT_EXTERN void		Blt_BlendPicturesByMode(Blt_Picture dest,
				Blt_Picture src, Blt_BlendingMode mode);
#endif
#ifndef Blt_FadePicture_DECLARED
#define Blt_FadePicture_DECLARED
/* 51 */
BLT_EXTERN void		Blt_FadePicture(Blt_Picture picture, int x, int y,
				int w, int h, int alpha);
#endif
#ifndef Blt_CopyPictureBits_DECLARED
#define Blt_CopyPictureBits_DECLARED
/* 52 */
BLT_EXTERN void		Blt_CopyPictureBits(Blt_Picture dest,
				Blt_Picture src, int sx, int sy, int w,
				int h, int dx, int dy);
#endif
#ifndef Blt_GammaCorrectPicture_DECLARED
#define Blt_GammaCorrectPicture_DECLARED
/* 53 */
BLT_EXTERN void		Blt_GammaCorrectPicture(Blt_Picture dest,
				Blt_Picture src, float gamma);
#endif
#ifndef Blt_SharpenPicture_DECLARED
#define Blt_SharpenPicture_DECLARED
/* 54 */
BLT_EXTERN void		Blt_SharpenPicture(Blt_Picture dest, Blt_Picture src);
#endif
#ifndef Blt_ApplyColorToPicture_DECLARED
#define Blt_ApplyColorToPicture_DECLARED
/* 55 */
BLT_EXTERN void		Blt_ApplyColorToPicture(Blt_Picture pict,
				Blt_Pixel *colorPtr);
#endif
#ifndef Blt_SizeOfPicture_DECLARED
#define Blt_SizeOfPicture_DECLARED
/* 56 */
BLT_EXTERN void		Blt_SizeOfPicture(Blt_Picture pict, int *wPtr,
				int *hPtr);
#endif
#ifndef Blt_PictureToDBuffer_DECLARED
#define Blt_PictureToDBuffer_DECLARED
/* 57 */
BLT_EXTERN Blt_DBuffer	Blt_PictureToDBuffer(Blt_Picture picture,
				int numComp);
#endif
#ifndef Blt_ResetPicture_DECLARED
#define Blt_ResetPicture_DECLARED
/* 58 */
BLT_EXTERN int		Blt_ResetPicture(Tcl_Interp *interp,
				const char *imageName, Blt_Picture picture);
#endif
#ifndef Blt_MapColors_DECLARED
#define Blt_MapColors_DECLARED
/* 59 */
BLT_EXTERN void		Blt_MapColors(Blt_Picture dest, Blt_Picture src,
				Blt_ColorLookupTable clut);
#endif
#ifndef Blt_GetColorLookupTable_DECLARED
#define Blt_GetColorLookupTable_DECLARED
/* 60 */
BLT_EXTERN Blt_ColorLookupTable Blt_GetColorLookupTable(
				struct _Blt_Chain *chainPtr,
				int numReqColors);
#endif
#ifndef Blt_FadePictureWithGradient_DECLARED
#define Blt_FadePictureWithGradient_DECLARED
/* 61 */
BLT_EXTERN void		Blt_FadePictureWithGradient(Blt_Picture picture,
				PictFadeSettings *settingsPtr);
#endif
#ifndef Blt_ReflectPicture2_DECLARED
#define Blt_ReflectPicture2_DECLARED
/* 62 */
BLT_EXTERN Blt_Picture	Blt_ReflectPicture2(Blt_Picture picture, int side);
#endif
#ifndef Blt_SubtractColor_DECLARED
#define Blt_SubtractColor_DECLARED
/* 63 */
BLT_EXTERN void		Blt_SubtractColor(Blt_Picture picture,
				Blt_Pixel *colorPtr);
#endif
#ifndef Blt_PhotoToPicture_DECLARED
#define Blt_PhotoToPicture_DECLARED
/* 64 */
BLT_EXTERN Blt_Picture	Blt_PhotoToPicture(Tk_PhotoHandle photo);
#endif
#ifndef Blt_PhotoAreaToPicture_DECLARED
#define Blt_PhotoAreaToPicture_DECLARED
/* 65 */
BLT_EXTERN Blt_Picture	Blt_PhotoAreaToPicture(Tk_PhotoHandle photo, int x,
				int y, int w, int h);
#endif
#ifndef Blt_DrawableToPicture_DECLARED
#define Blt_DrawableToPicture_DECLARED
/* 66 */
BLT_EXTERN Blt_Picture	Blt_DrawableToPicture(Tk_Window tkwin,
				Drawable drawable, int x, int y, int w,
				int h, float gamma);
#endif
#ifndef Blt_WindowToPicture_DECLARED
#define Blt_WindowToPicture_DECLARED
/* 67 */
BLT_EXTERN Blt_Picture	Blt_WindowToPicture(Display *display,
				Drawable drawable, int x, int y, int w,
				int h, float gamma);
#endif
#ifndef Blt_PictureToPhoto_DECLARED
#define Blt_PictureToPhoto_DECLARED
/* 68 */
BLT_EXTERN void		Blt_PictureToPhoto(Blt_Picture picture,
				Tk_PhotoHandle photo);
#endif
#ifndef Blt_SnapPhoto_DECLARED
#define Blt_SnapPhoto_DECLARED
/* 69 */
BLT_EXTERN int		Blt_SnapPhoto(Tcl_Interp *interp, Tk_Window tkwin,
				Drawable drawable, int sx, int sy, int w,
				int h, int dw, int dh, const char *photoName,
				float gamma);
#endif
#ifndef Blt_SnapPicture_DECLARED
#define Blt_SnapPicture_DECLARED
/* 70 */
BLT_EXTERN int		Blt_SnapPicture(Tcl_Interp *interp, Tk_Window tkwin,
				Drawable drawable, int sx, int sy, int w,
				int h, int dw, int dh, const char *imageName,
				float gamma);
#endif
#ifndef Blt_XColorToPixel_DECLARED
#define Blt_XColorToPixel_DECLARED
/* 71 */
BLT_EXTERN unsigned int	 Blt_XColorToPixel(XColor *colorPtr);
#endif
#ifndef Blt_IsPicture_DECLARED
#define Blt_IsPicture_DECLARED
/* 72 */
BLT_EXTERN int		Blt_IsPicture(Tk_Image tkImage);
#endif
#ifndef Blt_GetPictureFromImage_DECLARED
#define Blt_GetPictureFromImage_DECLARED
/* 73 */
BLT_EXTERN Blt_Picture	Blt_GetPictureFromImage(Tcl_Interp *interp,
				Tk_Image tkImage, int *isPhotoPtr);
#endif
#ifndef Blt_GetPictureFromPictureImage_DECLARED
#define Blt_GetPictureFromPictureImage_DECLARED
/* 74 */
BLT_EXTERN Blt_Picture	Blt_GetPictureFromPictureImage(Tcl_Interp *interp,
				Tk_Image tkImage);
#endif
#ifndef Blt_GetPicturesFromPictureImage_DECLARED
#define Blt_GetPicturesFromPictureImage_DECLARED
/* 75 */
BLT_EXTERN struct _Blt_Chain * Blt_GetPicturesFromPictureImage(
				Tcl_Interp *interp, Tk_Image tkImage);
#endif
#ifndef Blt_GetPictureFromPhotoImage_DECLARED
#define Blt_GetPictureFromPhotoImage_DECLARED
/* 76 */
BLT_EXTERN Blt_Picture	Blt_GetPictureFromPhotoImage(Tcl_Interp *interp,
				Tk_Image tkImage);
#endif
#ifndef Blt_CanvasToPicture_DECLARED
#define Blt_CanvasToPicture_DECLARED
/* 77 */
BLT_EXTERN Blt_Picture	Blt_CanvasToPicture(Tcl_Interp *interp,
				const char *s, float gamma);
#endif
#ifndef Blt_PictureRegisterProc_DECLARED
#define Blt_PictureRegisterProc_DECLARED
/* 78 */
BLT_EXTERN int		Blt_PictureRegisterProc(Tcl_Interp *interp,
				const char *name, Tcl_ObjCmdProc *proc);
#endif
#ifndef Blt_Shadow_Set_DECLARED
#define Blt_Shadow_Set_DECLARED
/* 79 */
BLT_EXTERN void		Blt_Shadow_Set(Blt_Shadow *sPtr, int width,
				int offset, int color, int alpha);
#endif
#ifndef Blt_Palette_GetFromObj_DECLARED
#define Blt_Palette_GetFromObj_DECLARED
/* 80 */
BLT_EXTERN int		Blt_Palette_GetFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_Palette *palPtr);
#endif
#ifndef Blt_Palette_GetFromString_DECLARED
#define Blt_Palette_GetFromString_DECLARED
/* 81 */
BLT_EXTERN int		Blt_Palette_GetFromString(Tcl_Interp *interp,
				const char *string, Blt_Palette *palPtr);
#endif
#ifndef Blt_Palette_GetColorFromAbsoluteValue_DECLARED
#define Blt_Palette_GetColorFromAbsoluteValue_DECLARED
/* 82 */
BLT_EXTERN int		Blt_Palette_GetColorFromAbsoluteValue(
				Blt_Palette palette, double absValue,
				double rangeMin, double rangeMax);
#endif
#ifndef Blt_Palette_SetRange_DECLARED
#define Blt_Palette_SetRange_DECLARED
/* 83 */
BLT_EXTERN void		Blt_Palette_SetRange(Blt_Palette palette, double min,
				double max);
#endif
#ifndef Blt_Palette_GetColor_DECLARED
#define Blt_Palette_GetColor_DECLARED
/* 84 */
BLT_EXTERN int		Blt_Palette_GetColor(Blt_Palette palette,
				double relValue);
#endif
#ifndef Blt_Palette_CreateNotifier_DECLARED
#define Blt_Palette_CreateNotifier_DECLARED
/* 85 */
BLT_EXTERN void		Blt_Palette_CreateNotifier(Blt_Palette palette,
				Blt_Palette_NotifyProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Palette_DeleteNotifier_DECLARED
#define Blt_Palette_DeleteNotifier_DECLARED
/* 86 */
BLT_EXTERN void		Blt_Palette_DeleteNotifier(Blt_Palette palette,
				ClientData clientData);
#endif
#ifndef Blt_Palette_Name_DECLARED
#define Blt_Palette_Name_DECLARED
/* 87 */
BLT_EXTERN const char *	 Blt_Palette_Name(Blt_Palette palette);
#endif
#ifndef Blt_Palette_TwoColorPalette_DECLARED
#define Blt_Palette_TwoColorPalette_DECLARED
/* 88 */
BLT_EXTERN Blt_Palette	Blt_Palette_TwoColorPalette(int low, int high);
#endif
#ifndef Blt_Palette_Free_DECLARED
#define Blt_Palette_Free_DECLARED
/* 89 */
BLT_EXTERN void		Blt_Palette_Free(Blt_Palette palette);
#endif
#ifndef Blt_GradientPicture_DECLARED
#define Blt_GradientPicture_DECLARED
/* 90 */
BLT_EXTERN void		Blt_GradientPicture(Blt_Picture picture,
				Blt_Pixel *highPtr, Blt_Pixel *lowPtr,
				Blt_Gradient *gradientPtr,
				Blt_Jitter *jitterPtr);
#endif
#ifndef Blt_TexturePicture_DECLARED
#define Blt_TexturePicture_DECLARED
/* 91 */
BLT_EXTERN void		Blt_TexturePicture(Blt_Picture picture,
				Blt_Pixel *lowPtr, Blt_Pixel *highPtr,
				Blt_TextureType type);
#endif
#ifndef Blt_Paintbrush_Init_DECLARED
#define Blt_Paintbrush_Init_DECLARED
/* 92 */
BLT_EXTERN void		Blt_Paintbrush_Init(Blt_Paintbrush *brushPtr);
#endif
#ifndef Blt_Paintbrush_Free_DECLARED
#define Blt_Paintbrush_Free_DECLARED
/* 93 */
BLT_EXTERN void		Blt_Paintbrush_Free(Blt_Paintbrush *brushPtr);
#endif
#ifndef Blt_Paintbrush_Get_DECLARED
#define Blt_Paintbrush_Get_DECLARED
/* 94 */
BLT_EXTERN int		Blt_Paintbrush_Get(Tcl_Interp *interp,
				Tcl_Obj *objPtr,
				Blt_Paintbrush **brushPtrPtr);
#endif
#ifndef Blt_Paintbrush_GetFromString_DECLARED
#define Blt_Paintbrush_GetFromString_DECLARED
/* 95 */
BLT_EXTERN int		Blt_Paintbrush_GetFromString(Tcl_Interp *interp,
				const char *string,
				Blt_Paintbrush **brushPtrPtr);
#endif
#ifndef Blt_Paintbrush_SetPalette_DECLARED
#define Blt_Paintbrush_SetPalette_DECLARED
/* 96 */
BLT_EXTERN void		Blt_Paintbrush_SetPalette(Blt_Paintbrush *brushPtr,
				Blt_Palette palette);
#endif
#ifndef Blt_Paintbrush_SetColorProc_DECLARED
#define Blt_Paintbrush_SetColorProc_DECLARED
/* 97 */
BLT_EXTERN void		Blt_Paintbrush_SetColorProc(Blt_Paintbrush *brushPtr,
				Blt_Paintbrush_ColorProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Paintbrush_SetColors_DECLARED
#define Blt_Paintbrush_SetColors_DECLARED
/* 98 */
BLT_EXTERN void		Blt_Paintbrush_SetColors(Blt_Paintbrush *brushPtr,
				Blt_Pixel *lowPtr, Blt_Pixel *highPtr);
#endif
#ifndef Blt_Paintbrush_Region_DECLARED
#define Blt_Paintbrush_Region_DECLARED
/* 99 */
BLT_EXTERN void		Blt_Paintbrush_Region(Blt_Paintbrush *brushPtr,
				int x, int y, int w, int h);
#endif
#ifndef Blt_Paintbrush_SetTile_DECLARED
#define Blt_Paintbrush_SetTile_DECLARED
/* 100 */
BLT_EXTERN void		Blt_Paintbrush_SetTile(Blt_Paintbrush *brushPtr,
				Blt_Picture tile);
#endif
#ifndef Blt_Paintbrush_SetTexture_DECLARED
#define Blt_Paintbrush_SetTexture_DECLARED
/* 101 */
BLT_EXTERN void		Blt_Paintbrush_SetTexture(Blt_Paintbrush *brushPtr);
#endif
#ifndef Blt_Paintbrush_SetGradient_DECLARED
#define Blt_Paintbrush_SetGradient_DECLARED
/* 102 */
BLT_EXTERN void		Blt_Paintbrush_SetGradient(Blt_Paintbrush *brushPtr,
				Blt_GradientType type);
#endif
#ifndef Blt_Paintbrush_SetColor_DECLARED
#define Blt_Paintbrush_SetColor_DECLARED
/* 103 */
BLT_EXTERN void		Blt_Paintbrush_SetColor(Blt_Paintbrush *brushPtr,
				unsigned int value);
#endif
#ifndef Blt_Paintbrush_SetOrigin_DECLARED
#define Blt_Paintbrush_SetOrigin_DECLARED
/* 104 */
BLT_EXTERN void		Blt_Paintbrush_SetOrigin(Blt_Paintbrush *brushPtr,
				int x, int y);
#endif
#ifndef Blt_Paintbrush_GetColor_DECLARED
#define Blt_Paintbrush_GetColor_DECLARED
/* 105 */
BLT_EXTERN int		Blt_Paintbrush_GetColor(Blt_Paintbrush *brushPtr,
				int x, int y);
#endif
#ifndef Blt_PaintRectangle_DECLARED
#define Blt_PaintRectangle_DECLARED
/* 106 */
BLT_EXTERN void		Blt_PaintRectangle(Blt_Picture picture, int x, int y,
				int w, int h, int dx, int dy,
				Blt_Paintbrush *brushPtr);
#endif
#ifndef Blt_PaintPolygon_DECLARED
#define Blt_PaintPolygon_DECLARED
/* 107 */
BLT_EXTERN void		Blt_PaintPolygon(Blt_Picture picture, int n,
				Point2f *vertices, Blt_Paintbrush *brushPtr);
#endif
#ifndef Blt_PictureRegisterFormat_DECLARED
#define Blt_PictureRegisterFormat_DECLARED
/* 108 */
BLT_EXTERN int		Blt_PictureRegisterFormat(Tcl_Interp *interp,
				const char *name,
				Blt_PictureIsFmtProc *isFmtProc,
				Blt_PictureReadDataProc *readProc,
				Blt_PictureWriteDataProc *writeProc,
				Blt_PictureImportProc *importProc,
				Blt_PictureExportProc *exportProc);
#endif
#ifndef Blt_GetNthPicture_DECLARED
#define Blt_GetNthPicture_DECLARED
/* 109 */
BLT_EXTERN Blt_Picture	Blt_GetNthPicture(Blt_Chain chain, size_t index);
#endif
#ifndef Blt_FindPictureFormat_DECLARED
#define Blt_FindPictureFormat_DECLARED
/* 110 */
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
    void (*blt_Jitter_Init) (Blt_Jitter *jitterPtr); /* 1 */
    void (*blt_ApplyPictureToPicture) (Blt_Picture dest, Blt_Picture src, int x, int y, int w, int h, int dx, int dy, Blt_PictureArithOps op); /* 2 */
    void (*blt_ApplyScalarToPicture) (Blt_Picture dest, Blt_Pixel *colorPtr, Blt_PictureArithOps op); /* 3 */
    void (*blt_ApplyPictureToPictureWithMask) (Blt_Picture dest, Blt_Picture src, Blt_Picture mask, int x, int y, int w, int h, int dx, int dy, int invert, Blt_PictureArithOps op); /* 4 */
    void (*blt_ApplyScalarToPictureWithMask) (Blt_Picture dest, Blt_Pixel *colorPtr, Blt_Picture mask, int invert, Blt_PictureArithOps op); /* 5 */
    void (*blt_MaskPicture) (Blt_Picture dest, Blt_Picture mask, int x, int y, int w, int h, int dx, int dy, Blt_Pixel *colorPtr); /* 6 */
    void (*blt_BlankPicture) (Blt_Picture picture, unsigned int colorValue); /* 7 */
    void (*blt_BlankRegion) (Blt_Picture picture, int x, int y, int w, int h, unsigned int colorValue); /* 8 */
    void (*blt_BlurPicture) (Blt_Picture dest, Blt_Picture src, int radius, int numPasses); /* 9 */
    void (*blt_ResizePicture) (Blt_Picture picture, int w, int h); /* 10 */
    void (*blt_AdjustPictureSize) (Blt_Picture picture, int w, int h); /* 11 */
    Blt_Picture (*blt_ClonePicture) (Blt_Picture picture); /* 12 */
    void (*blt_ConvolvePicture) (Blt_Picture dest, Blt_Picture src, Blt_ConvolveFilter vFilter, Blt_ConvolveFilter hFilter); /* 13 */
    Blt_Picture (*blt_CreatePicture) (int w, int h); /* 14 */
    Blt_Picture (*blt_DitherPicture) (Blt_Picture picture, Blt_Pixel *palette); /* 15 */
    void (*blt_FlipPicture) (Blt_Picture picture, int vertically); /* 16 */
    void (*blt_FreePicture) (Blt_Picture picture); /* 17 */
    Blt_Picture (*blt_GreyscalePicture) (Blt_Picture picture); /* 18 */
    Blt_Picture (*blt_QuantizePicture) (Blt_Picture picture, int numColors); /* 19 */
    void (*blt_ResamplePicture) (Blt_Picture dest, Blt_Picture src, Blt_ResampleFilter hFilter, Blt_ResampleFilter vFilter); /* 20 */
    Blt_Picture (*blt_ScalePicture) (Blt_Picture picture, int x, int y, int w, int h, int dw, int dh); /* 21 */
    Blt_Picture (*blt_ScalePictureArea) (Blt_Picture picture, int x, int y, int w, int h, int dw, int dh); /* 22 */
    Blt_Picture (*blt_ReflectPicture) (Blt_Picture picture, int side); /* 23 */
    Blt_Picture (*blt_RotatePictureByShear) (Blt_Picture picture, float angle); /* 24 */
    Blt_Picture (*blt_RotatePicture) (Blt_Picture picture, float angle); /* 25 */
    Blt_Picture (*blt_ProjectPicture) (Blt_Picture picture, float *srcPts, float *destPts, Blt_Pixel *bg); /* 26 */
    void (*blt_TilePicture) (Blt_Picture dest, Blt_Picture src, int xOrigin, int yOrigin, int x, int y, int w, int h); /* 27 */
    int (*blt_PictureToPsData) (Blt_Picture picture, int numComponents, Tcl_DString *resultPtr, const char *prefix); /* 28 */
    void (*blt_SelectPixels) (Blt_Picture dest, Blt_Picture src, Blt_Pixel *lowerPtr, Blt_Pixel *upperPtr); /* 29 */
    int (*blt_GetPicture) (Tcl_Interp *interp, const char *string, Blt_Picture *picturePtr); /* 30 */
    int (*blt_GetPictureFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Picture *picturePtr); /* 31 */
    int (*blt_GetResampleFilterFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_ResampleFilter *filterPtr); /* 32 */
    const char * (*blt_NameOfResampleFilter) (Blt_ResampleFilter filter); /* 33 */
    void (*blt_AssociateColors) (Blt_Picture picture); /* 34 */
    void (*blt_UnassociateColors) (Blt_Picture picture); /* 35 */
    void (*blt_MultiplyPixels) (Blt_Picture picture, float value); /* 36 */
    int (*blt_GetBBoxFromObjv) (Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, PictRegion *regionPtr); /* 37 */
    int (*blt_AdjustRegionToPicture) (Blt_Picture picture, PictRegion *regionPtr); /* 38 */
    int (*blt_GetPixelFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Pixel *pixelPtr); /* 39 */
    int (*blt_GetPixel) (Tcl_Interp *interp, const char *string, Blt_Pixel *pixelPtr); /* 40 */
    const char * (*blt_NameOfPixel) (Blt_Pixel *pixelPtr); /* 41 */
    void (*blt_NotifyImageChanged) (Blt_PictureImage image); /* 42 */
    int (*blt_QueryColors) (Blt_Picture picture, Blt_HashTable *tablePtr); /* 43 */
    void (*blt_ClassifyPicture) (Blt_Picture picture); /* 44 */
    void (*blt_TentHorizontally) (Blt_Picture dest, Blt_Picture src); /* 45 */
    void (*blt_TentVertically) (Blt_Picture dest, Blt_Picture src); /* 46 */
    void (*blt_ZoomHorizontally) (Blt_Picture dest, Blt_Picture src, Blt_ResampleFilter filter); /* 47 */
    void (*blt_ZoomVertically) (Blt_Picture dest, Blt_Picture src, Blt_ResampleFilter filter); /* 48 */
    void (*blt_BlendPictures) (Blt_Picture dest, Blt_Picture src, int sx, int sy, int w, int h, int dx, int dy); /* 49 */
    void (*blt_BlendPicturesByMode) (Blt_Picture dest, Blt_Picture src, Blt_BlendingMode mode); /* 50 */
    void (*blt_FadePicture) (Blt_Picture picture, int x, int y, int w, int h, int alpha); /* 51 */
    void (*blt_CopyPictureBits) (Blt_Picture dest, Blt_Picture src, int sx, int sy, int w, int h, int dx, int dy); /* 52 */
    void (*blt_GammaCorrectPicture) (Blt_Picture dest, Blt_Picture src, float gamma); /* 53 */
    void (*blt_SharpenPicture) (Blt_Picture dest, Blt_Picture src); /* 54 */
    void (*blt_ApplyColorToPicture) (Blt_Picture pict, Blt_Pixel *colorPtr); /* 55 */
    void (*blt_SizeOfPicture) (Blt_Picture pict, int *wPtr, int *hPtr); /* 56 */
    Blt_DBuffer (*blt_PictureToDBuffer) (Blt_Picture picture, int numComp); /* 57 */
    int (*blt_ResetPicture) (Tcl_Interp *interp, const char *imageName, Blt_Picture picture); /* 58 */
    void (*blt_MapColors) (Blt_Picture dest, Blt_Picture src, Blt_ColorLookupTable clut); /* 59 */
    Blt_ColorLookupTable (*blt_GetColorLookupTable) (struct _Blt_Chain *chainPtr, int numReqColors); /* 60 */
    void (*blt_FadePictureWithGradient) (Blt_Picture picture, PictFadeSettings *settingsPtr); /* 61 */
    Blt_Picture (*blt_ReflectPicture2) (Blt_Picture picture, int side); /* 62 */
    void (*blt_SubtractColor) (Blt_Picture picture, Blt_Pixel *colorPtr); /* 63 */
    Blt_Picture (*blt_PhotoToPicture) (Tk_PhotoHandle photo); /* 64 */
    Blt_Picture (*blt_PhotoAreaToPicture) (Tk_PhotoHandle photo, int x, int y, int w, int h); /* 65 */
    Blt_Picture (*blt_DrawableToPicture) (Tk_Window tkwin, Drawable drawable, int x, int y, int w, int h, float gamma); /* 66 */
    Blt_Picture (*blt_WindowToPicture) (Display *display, Drawable drawable, int x, int y, int w, int h, float gamma); /* 67 */
    void (*blt_PictureToPhoto) (Blt_Picture picture, Tk_PhotoHandle photo); /* 68 */
    int (*blt_SnapPhoto) (Tcl_Interp *interp, Tk_Window tkwin, Drawable drawable, int sx, int sy, int w, int h, int dw, int dh, const char *photoName, float gamma); /* 69 */
    int (*blt_SnapPicture) (Tcl_Interp *interp, Tk_Window tkwin, Drawable drawable, int sx, int sy, int w, int h, int dw, int dh, const char *imageName, float gamma); /* 70 */
    unsigned int (*blt_XColorToPixel) (XColor *colorPtr); /* 71 */
    int (*blt_IsPicture) (Tk_Image tkImage); /* 72 */
    Blt_Picture (*blt_GetPictureFromImage) (Tcl_Interp *interp, Tk_Image tkImage, int *isPhotoPtr); /* 73 */
    Blt_Picture (*blt_GetPictureFromPictureImage) (Tcl_Interp *interp, Tk_Image tkImage); /* 74 */
    struct _Blt_Chain * (*blt_GetPicturesFromPictureImage) (Tcl_Interp *interp, Tk_Image tkImage); /* 75 */
    Blt_Picture (*blt_GetPictureFromPhotoImage) (Tcl_Interp *interp, Tk_Image tkImage); /* 76 */
    Blt_Picture (*blt_CanvasToPicture) (Tcl_Interp *interp, const char *s, float gamma); /* 77 */
    int (*blt_PictureRegisterProc) (Tcl_Interp *interp, const char *name, Tcl_ObjCmdProc *proc); /* 78 */
    void (*blt_Shadow_Set) (Blt_Shadow *sPtr, int width, int offset, int color, int alpha); /* 79 */
    int (*blt_Palette_GetFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Palette *palPtr); /* 80 */
    int (*blt_Palette_GetFromString) (Tcl_Interp *interp, const char *string, Blt_Palette *palPtr); /* 81 */
    int (*blt_Palette_GetColorFromAbsoluteValue) (Blt_Palette palette, double absValue, double rangeMin, double rangeMax); /* 82 */
    void (*blt_Palette_SetRange) (Blt_Palette palette, double min, double max); /* 83 */
    int (*blt_Palette_GetColor) (Blt_Palette palette, double relValue); /* 84 */
    void (*blt_Palette_CreateNotifier) (Blt_Palette palette, Blt_Palette_NotifyProc *proc, ClientData clientData); /* 85 */
    void (*blt_Palette_DeleteNotifier) (Blt_Palette palette, ClientData clientData); /* 86 */
    const char * (*blt_Palette_Name) (Blt_Palette palette); /* 87 */
    Blt_Palette (*blt_Palette_TwoColorPalette) (int low, int high); /* 88 */
    void (*blt_Palette_Free) (Blt_Palette palette); /* 89 */
    void (*blt_GradientPicture) (Blt_Picture picture, Blt_Pixel *highPtr, Blt_Pixel *lowPtr, Blt_Gradient *gradientPtr, Blt_Jitter *jitterPtr); /* 90 */
    void (*blt_TexturePicture) (Blt_Picture picture, Blt_Pixel *lowPtr, Blt_Pixel *highPtr, Blt_TextureType type); /* 91 */
    void (*blt_Paintbrush_Init) (Blt_Paintbrush *brushPtr); /* 92 */
    void (*blt_Paintbrush_Free) (Blt_Paintbrush *brushPtr); /* 93 */
    int (*blt_Paintbrush_Get) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Paintbrush **brushPtrPtr); /* 94 */
    int (*blt_Paintbrush_GetFromString) (Tcl_Interp *interp, const char *string, Blt_Paintbrush **brushPtrPtr); /* 95 */
    void (*blt_Paintbrush_SetPalette) (Blt_Paintbrush *brushPtr, Blt_Palette palette); /* 96 */
    void (*blt_Paintbrush_SetColorProc) (Blt_Paintbrush *brushPtr, Blt_Paintbrush_ColorProc *proc, ClientData clientData); /* 97 */
    void (*blt_Paintbrush_SetColors) (Blt_Paintbrush *brushPtr, Blt_Pixel *lowPtr, Blt_Pixel *highPtr); /* 98 */
    void (*blt_Paintbrush_Region) (Blt_Paintbrush *brushPtr, int x, int y, int w, int h); /* 99 */
    void (*blt_Paintbrush_SetTile) (Blt_Paintbrush *brushPtr, Blt_Picture tile); /* 100 */
    void (*blt_Paintbrush_SetTexture) (Blt_Paintbrush *brushPtr); /* 101 */
    void (*blt_Paintbrush_SetGradient) (Blt_Paintbrush *brushPtr, Blt_GradientType type); /* 102 */
    void (*blt_Paintbrush_SetColor) (Blt_Paintbrush *brushPtr, unsigned int value); /* 103 */
    void (*blt_Paintbrush_SetOrigin) (Blt_Paintbrush *brushPtr, int x, int y); /* 104 */
    int (*blt_Paintbrush_GetColor) (Blt_Paintbrush *brushPtr, int x, int y); /* 105 */
    void (*blt_PaintRectangle) (Blt_Picture picture, int x, int y, int w, int h, int dx, int dy, Blt_Paintbrush *brushPtr); /* 106 */
    void (*blt_PaintPolygon) (Blt_Picture picture, int n, Point2f *vertices, Blt_Paintbrush *brushPtr); /* 107 */
    int (*blt_PictureRegisterFormat) (Tcl_Interp *interp, const char *name, Blt_PictureIsFmtProc *isFmtProc, Blt_PictureReadDataProc *readProc, Blt_PictureWriteDataProc *writeProc, Blt_PictureImportProc *importProc, Blt_PictureExportProc *exportProc); /* 108 */
    Blt_Picture (*blt_GetNthPicture) (Blt_Chain chain, size_t index); /* 109 */
    Blt_PictFormat * (*blt_FindPictureFormat) (Tcl_Interp *interp, const char *ext); /* 110 */
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
#ifndef Blt_Jitter_Init
#define Blt_Jitter_Init \
	(bltTkProcsPtr->blt_Jitter_Init) /* 1 */
#endif
#ifndef Blt_ApplyPictureToPicture
#define Blt_ApplyPictureToPicture \
	(bltTkProcsPtr->blt_ApplyPictureToPicture) /* 2 */
#endif
#ifndef Blt_ApplyScalarToPicture
#define Blt_ApplyScalarToPicture \
	(bltTkProcsPtr->blt_ApplyScalarToPicture) /* 3 */
#endif
#ifndef Blt_ApplyPictureToPictureWithMask
#define Blt_ApplyPictureToPictureWithMask \
	(bltTkProcsPtr->blt_ApplyPictureToPictureWithMask) /* 4 */
#endif
#ifndef Blt_ApplyScalarToPictureWithMask
#define Blt_ApplyScalarToPictureWithMask \
	(bltTkProcsPtr->blt_ApplyScalarToPictureWithMask) /* 5 */
#endif
#ifndef Blt_MaskPicture
#define Blt_MaskPicture \
	(bltTkProcsPtr->blt_MaskPicture) /* 6 */
#endif
#ifndef Blt_BlankPicture
#define Blt_BlankPicture \
	(bltTkProcsPtr->blt_BlankPicture) /* 7 */
#endif
#ifndef Blt_BlankRegion
#define Blt_BlankRegion \
	(bltTkProcsPtr->blt_BlankRegion) /* 8 */
#endif
#ifndef Blt_BlurPicture
#define Blt_BlurPicture \
	(bltTkProcsPtr->blt_BlurPicture) /* 9 */
#endif
#ifndef Blt_ResizePicture
#define Blt_ResizePicture \
	(bltTkProcsPtr->blt_ResizePicture) /* 10 */
#endif
#ifndef Blt_AdjustPictureSize
#define Blt_AdjustPictureSize \
	(bltTkProcsPtr->blt_AdjustPictureSize) /* 11 */
#endif
#ifndef Blt_ClonePicture
#define Blt_ClonePicture \
	(bltTkProcsPtr->blt_ClonePicture) /* 12 */
#endif
#ifndef Blt_ConvolvePicture
#define Blt_ConvolvePicture \
	(bltTkProcsPtr->blt_ConvolvePicture) /* 13 */
#endif
#ifndef Blt_CreatePicture
#define Blt_CreatePicture \
	(bltTkProcsPtr->blt_CreatePicture) /* 14 */
#endif
#ifndef Blt_DitherPicture
#define Blt_DitherPicture \
	(bltTkProcsPtr->blt_DitherPicture) /* 15 */
#endif
#ifndef Blt_FlipPicture
#define Blt_FlipPicture \
	(bltTkProcsPtr->blt_FlipPicture) /* 16 */
#endif
#ifndef Blt_FreePicture
#define Blt_FreePicture \
	(bltTkProcsPtr->blt_FreePicture) /* 17 */
#endif
#ifndef Blt_GreyscalePicture
#define Blt_GreyscalePicture \
	(bltTkProcsPtr->blt_GreyscalePicture) /* 18 */
#endif
#ifndef Blt_QuantizePicture
#define Blt_QuantizePicture \
	(bltTkProcsPtr->blt_QuantizePicture) /* 19 */
#endif
#ifndef Blt_ResamplePicture
#define Blt_ResamplePicture \
	(bltTkProcsPtr->blt_ResamplePicture) /* 20 */
#endif
#ifndef Blt_ScalePicture
#define Blt_ScalePicture \
	(bltTkProcsPtr->blt_ScalePicture) /* 21 */
#endif
#ifndef Blt_ScalePictureArea
#define Blt_ScalePictureArea \
	(bltTkProcsPtr->blt_ScalePictureArea) /* 22 */
#endif
#ifndef Blt_ReflectPicture
#define Blt_ReflectPicture \
	(bltTkProcsPtr->blt_ReflectPicture) /* 23 */
#endif
#ifndef Blt_RotatePictureByShear
#define Blt_RotatePictureByShear \
	(bltTkProcsPtr->blt_RotatePictureByShear) /* 24 */
#endif
#ifndef Blt_RotatePicture
#define Blt_RotatePicture \
	(bltTkProcsPtr->blt_RotatePicture) /* 25 */
#endif
#ifndef Blt_ProjectPicture
#define Blt_ProjectPicture \
	(bltTkProcsPtr->blt_ProjectPicture) /* 26 */
#endif
#ifndef Blt_TilePicture
#define Blt_TilePicture \
	(bltTkProcsPtr->blt_TilePicture) /* 27 */
#endif
#ifndef Blt_PictureToPsData
#define Blt_PictureToPsData \
	(bltTkProcsPtr->blt_PictureToPsData) /* 28 */
#endif
#ifndef Blt_SelectPixels
#define Blt_SelectPixels \
	(bltTkProcsPtr->blt_SelectPixels) /* 29 */
#endif
#ifndef Blt_GetPicture
#define Blt_GetPicture \
	(bltTkProcsPtr->blt_GetPicture) /* 30 */
#endif
#ifndef Blt_GetPictureFromObj
#define Blt_GetPictureFromObj \
	(bltTkProcsPtr->blt_GetPictureFromObj) /* 31 */
#endif
#ifndef Blt_GetResampleFilterFromObj
#define Blt_GetResampleFilterFromObj \
	(bltTkProcsPtr->blt_GetResampleFilterFromObj) /* 32 */
#endif
#ifndef Blt_NameOfResampleFilter
#define Blt_NameOfResampleFilter \
	(bltTkProcsPtr->blt_NameOfResampleFilter) /* 33 */
#endif
#ifndef Blt_AssociateColors
#define Blt_AssociateColors \
	(bltTkProcsPtr->blt_AssociateColors) /* 34 */
#endif
#ifndef Blt_UnassociateColors
#define Blt_UnassociateColors \
	(bltTkProcsPtr->blt_UnassociateColors) /* 35 */
#endif
#ifndef Blt_MultiplyPixels
#define Blt_MultiplyPixels \
	(bltTkProcsPtr->blt_MultiplyPixels) /* 36 */
#endif
#ifndef Blt_GetBBoxFromObjv
#define Blt_GetBBoxFromObjv \
	(bltTkProcsPtr->blt_GetBBoxFromObjv) /* 37 */
#endif
#ifndef Blt_AdjustRegionToPicture
#define Blt_AdjustRegionToPicture \
	(bltTkProcsPtr->blt_AdjustRegionToPicture) /* 38 */
#endif
#ifndef Blt_GetPixelFromObj
#define Blt_GetPixelFromObj \
	(bltTkProcsPtr->blt_GetPixelFromObj) /* 39 */
#endif
#ifndef Blt_GetPixel
#define Blt_GetPixel \
	(bltTkProcsPtr->blt_GetPixel) /* 40 */
#endif
#ifndef Blt_NameOfPixel
#define Blt_NameOfPixel \
	(bltTkProcsPtr->blt_NameOfPixel) /* 41 */
#endif
#ifndef Blt_NotifyImageChanged
#define Blt_NotifyImageChanged \
	(bltTkProcsPtr->blt_NotifyImageChanged) /* 42 */
#endif
#ifndef Blt_QueryColors
#define Blt_QueryColors \
	(bltTkProcsPtr->blt_QueryColors) /* 43 */
#endif
#ifndef Blt_ClassifyPicture
#define Blt_ClassifyPicture \
	(bltTkProcsPtr->blt_ClassifyPicture) /* 44 */
#endif
#ifndef Blt_TentHorizontally
#define Blt_TentHorizontally \
	(bltTkProcsPtr->blt_TentHorizontally) /* 45 */
#endif
#ifndef Blt_TentVertically
#define Blt_TentVertically \
	(bltTkProcsPtr->blt_TentVertically) /* 46 */
#endif
#ifndef Blt_ZoomHorizontally
#define Blt_ZoomHorizontally \
	(bltTkProcsPtr->blt_ZoomHorizontally) /* 47 */
#endif
#ifndef Blt_ZoomVertically
#define Blt_ZoomVertically \
	(bltTkProcsPtr->blt_ZoomVertically) /* 48 */
#endif
#ifndef Blt_BlendPictures
#define Blt_BlendPictures \
	(bltTkProcsPtr->blt_BlendPictures) /* 49 */
#endif
#ifndef Blt_BlendPicturesByMode
#define Blt_BlendPicturesByMode \
	(bltTkProcsPtr->blt_BlendPicturesByMode) /* 50 */
#endif
#ifndef Blt_FadePicture
#define Blt_FadePicture \
	(bltTkProcsPtr->blt_FadePicture) /* 51 */
#endif
#ifndef Blt_CopyPictureBits
#define Blt_CopyPictureBits \
	(bltTkProcsPtr->blt_CopyPictureBits) /* 52 */
#endif
#ifndef Blt_GammaCorrectPicture
#define Blt_GammaCorrectPicture \
	(bltTkProcsPtr->blt_GammaCorrectPicture) /* 53 */
#endif
#ifndef Blt_SharpenPicture
#define Blt_SharpenPicture \
	(bltTkProcsPtr->blt_SharpenPicture) /* 54 */
#endif
#ifndef Blt_ApplyColorToPicture
#define Blt_ApplyColorToPicture \
	(bltTkProcsPtr->blt_ApplyColorToPicture) /* 55 */
#endif
#ifndef Blt_SizeOfPicture
#define Blt_SizeOfPicture \
	(bltTkProcsPtr->blt_SizeOfPicture) /* 56 */
#endif
#ifndef Blt_PictureToDBuffer
#define Blt_PictureToDBuffer \
	(bltTkProcsPtr->blt_PictureToDBuffer) /* 57 */
#endif
#ifndef Blt_ResetPicture
#define Blt_ResetPicture \
	(bltTkProcsPtr->blt_ResetPicture) /* 58 */
#endif
#ifndef Blt_MapColors
#define Blt_MapColors \
	(bltTkProcsPtr->blt_MapColors) /* 59 */
#endif
#ifndef Blt_GetColorLookupTable
#define Blt_GetColorLookupTable \
	(bltTkProcsPtr->blt_GetColorLookupTable) /* 60 */
#endif
#ifndef Blt_FadePictureWithGradient
#define Blt_FadePictureWithGradient \
	(bltTkProcsPtr->blt_FadePictureWithGradient) /* 61 */
#endif
#ifndef Blt_ReflectPicture2
#define Blt_ReflectPicture2 \
	(bltTkProcsPtr->blt_ReflectPicture2) /* 62 */
#endif
#ifndef Blt_SubtractColor
#define Blt_SubtractColor \
	(bltTkProcsPtr->blt_SubtractColor) /* 63 */
#endif
#ifndef Blt_PhotoToPicture
#define Blt_PhotoToPicture \
	(bltTkProcsPtr->blt_PhotoToPicture) /* 64 */
#endif
#ifndef Blt_PhotoAreaToPicture
#define Blt_PhotoAreaToPicture \
	(bltTkProcsPtr->blt_PhotoAreaToPicture) /* 65 */
#endif
#ifndef Blt_DrawableToPicture
#define Blt_DrawableToPicture \
	(bltTkProcsPtr->blt_DrawableToPicture) /* 66 */
#endif
#ifndef Blt_WindowToPicture
#define Blt_WindowToPicture \
	(bltTkProcsPtr->blt_WindowToPicture) /* 67 */
#endif
#ifndef Blt_PictureToPhoto
#define Blt_PictureToPhoto \
	(bltTkProcsPtr->blt_PictureToPhoto) /* 68 */
#endif
#ifndef Blt_SnapPhoto
#define Blt_SnapPhoto \
	(bltTkProcsPtr->blt_SnapPhoto) /* 69 */
#endif
#ifndef Blt_SnapPicture
#define Blt_SnapPicture \
	(bltTkProcsPtr->blt_SnapPicture) /* 70 */
#endif
#ifndef Blt_XColorToPixel
#define Blt_XColorToPixel \
	(bltTkProcsPtr->blt_XColorToPixel) /* 71 */
#endif
#ifndef Blt_IsPicture
#define Blt_IsPicture \
	(bltTkProcsPtr->blt_IsPicture) /* 72 */
#endif
#ifndef Blt_GetPictureFromImage
#define Blt_GetPictureFromImage \
	(bltTkProcsPtr->blt_GetPictureFromImage) /* 73 */
#endif
#ifndef Blt_GetPictureFromPictureImage
#define Blt_GetPictureFromPictureImage \
	(bltTkProcsPtr->blt_GetPictureFromPictureImage) /* 74 */
#endif
#ifndef Blt_GetPicturesFromPictureImage
#define Blt_GetPicturesFromPictureImage \
	(bltTkProcsPtr->blt_GetPicturesFromPictureImage) /* 75 */
#endif
#ifndef Blt_GetPictureFromPhotoImage
#define Blt_GetPictureFromPhotoImage \
	(bltTkProcsPtr->blt_GetPictureFromPhotoImage) /* 76 */
#endif
#ifndef Blt_CanvasToPicture
#define Blt_CanvasToPicture \
	(bltTkProcsPtr->blt_CanvasToPicture) /* 77 */
#endif
#ifndef Blt_PictureRegisterProc
#define Blt_PictureRegisterProc \
	(bltTkProcsPtr->blt_PictureRegisterProc) /* 78 */
#endif
#ifndef Blt_Shadow_Set
#define Blt_Shadow_Set \
	(bltTkProcsPtr->blt_Shadow_Set) /* 79 */
#endif
#ifndef Blt_Palette_GetFromObj
#define Blt_Palette_GetFromObj \
	(bltTkProcsPtr->blt_Palette_GetFromObj) /* 80 */
#endif
#ifndef Blt_Palette_GetFromString
#define Blt_Palette_GetFromString \
	(bltTkProcsPtr->blt_Palette_GetFromString) /* 81 */
#endif
#ifndef Blt_Palette_GetColorFromAbsoluteValue
#define Blt_Palette_GetColorFromAbsoluteValue \
	(bltTkProcsPtr->blt_Palette_GetColorFromAbsoluteValue) /* 82 */
#endif
#ifndef Blt_Palette_SetRange
#define Blt_Palette_SetRange \
	(bltTkProcsPtr->blt_Palette_SetRange) /* 83 */
#endif
#ifndef Blt_Palette_GetColor
#define Blt_Palette_GetColor \
	(bltTkProcsPtr->blt_Palette_GetColor) /* 84 */
#endif
#ifndef Blt_Palette_CreateNotifier
#define Blt_Palette_CreateNotifier \
	(bltTkProcsPtr->blt_Palette_CreateNotifier) /* 85 */
#endif
#ifndef Blt_Palette_DeleteNotifier
#define Blt_Palette_DeleteNotifier \
	(bltTkProcsPtr->blt_Palette_DeleteNotifier) /* 86 */
#endif
#ifndef Blt_Palette_Name
#define Blt_Palette_Name \
	(bltTkProcsPtr->blt_Palette_Name) /* 87 */
#endif
#ifndef Blt_Palette_TwoColorPalette
#define Blt_Palette_TwoColorPalette \
	(bltTkProcsPtr->blt_Palette_TwoColorPalette) /* 88 */
#endif
#ifndef Blt_Palette_Free
#define Blt_Palette_Free \
	(bltTkProcsPtr->blt_Palette_Free) /* 89 */
#endif
#ifndef Blt_GradientPicture
#define Blt_GradientPicture \
	(bltTkProcsPtr->blt_GradientPicture) /* 90 */
#endif
#ifndef Blt_TexturePicture
#define Blt_TexturePicture \
	(bltTkProcsPtr->blt_TexturePicture) /* 91 */
#endif
#ifndef Blt_Paintbrush_Init
#define Blt_Paintbrush_Init \
	(bltTkProcsPtr->blt_Paintbrush_Init) /* 92 */
#endif
#ifndef Blt_Paintbrush_Free
#define Blt_Paintbrush_Free \
	(bltTkProcsPtr->blt_Paintbrush_Free) /* 93 */
#endif
#ifndef Blt_Paintbrush_Get
#define Blt_Paintbrush_Get \
	(bltTkProcsPtr->blt_Paintbrush_Get) /* 94 */
#endif
#ifndef Blt_Paintbrush_GetFromString
#define Blt_Paintbrush_GetFromString \
	(bltTkProcsPtr->blt_Paintbrush_GetFromString) /* 95 */
#endif
#ifndef Blt_Paintbrush_SetPalette
#define Blt_Paintbrush_SetPalette \
	(bltTkProcsPtr->blt_Paintbrush_SetPalette) /* 96 */
#endif
#ifndef Blt_Paintbrush_SetColorProc
#define Blt_Paintbrush_SetColorProc \
	(bltTkProcsPtr->blt_Paintbrush_SetColorProc) /* 97 */
#endif
#ifndef Blt_Paintbrush_SetColors
#define Blt_Paintbrush_SetColors \
	(bltTkProcsPtr->blt_Paintbrush_SetColors) /* 98 */
#endif
#ifndef Blt_Paintbrush_Region
#define Blt_Paintbrush_Region \
	(bltTkProcsPtr->blt_Paintbrush_Region) /* 99 */
#endif
#ifndef Blt_Paintbrush_SetTile
#define Blt_Paintbrush_SetTile \
	(bltTkProcsPtr->blt_Paintbrush_SetTile) /* 100 */
#endif
#ifndef Blt_Paintbrush_SetTexture
#define Blt_Paintbrush_SetTexture \
	(bltTkProcsPtr->blt_Paintbrush_SetTexture) /* 101 */
#endif
#ifndef Blt_Paintbrush_SetGradient
#define Blt_Paintbrush_SetGradient \
	(bltTkProcsPtr->blt_Paintbrush_SetGradient) /* 102 */
#endif
#ifndef Blt_Paintbrush_SetColor
#define Blt_Paintbrush_SetColor \
	(bltTkProcsPtr->blt_Paintbrush_SetColor) /* 103 */
#endif
#ifndef Blt_Paintbrush_SetOrigin
#define Blt_Paintbrush_SetOrigin \
	(bltTkProcsPtr->blt_Paintbrush_SetOrigin) /* 104 */
#endif
#ifndef Blt_Paintbrush_GetColor
#define Blt_Paintbrush_GetColor \
	(bltTkProcsPtr->blt_Paintbrush_GetColor) /* 105 */
#endif
#ifndef Blt_PaintRectangle
#define Blt_PaintRectangle \
	(bltTkProcsPtr->blt_PaintRectangle) /* 106 */
#endif
#ifndef Blt_PaintPolygon
#define Blt_PaintPolygon \
	(bltTkProcsPtr->blt_PaintPolygon) /* 107 */
#endif
#ifndef Blt_PictureRegisterFormat
#define Blt_PictureRegisterFormat \
	(bltTkProcsPtr->blt_PictureRegisterFormat) /* 108 */
#endif
#ifndef Blt_GetNthPicture
#define Blt_GetNthPicture \
	(bltTkProcsPtr->blt_GetNthPicture) /* 109 */
#endif
#ifndef Blt_FindPictureFormat
#define Blt_FindPictureFormat \
	(bltTkProcsPtr->blt_FindPictureFormat) /* 110 */
#endif

#endif /* defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TK_PROCS) */

/* !END!: Do not edit above this line. */

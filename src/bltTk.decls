library blt
interface bltTk
hooks bltTkInt
declare 1 generic {
   void Blt_Jitter_Init(Blt_Jitter *jitterPtr)
}
declare 2 generic {
   void Blt_ApplyPictureToPicture(Blt_Picture dest, Blt_Picture src,
	int x, int y, int w, int h, int dx, int dy, Blt_PictureArithOps op)
}
declare 3 generic {
   void Blt_ApplyScalarToPicture(Blt_Picture dest, Blt_Pixel *colorPtr, 
	Blt_PictureArithOps op)
}
declare 4 generic {
   void Blt_ApplyPictureToPictureWithMask(Blt_Picture dest, 
	Blt_Picture src, Blt_Picture mask, int x, int y, int w, int h, 
	int dx, int dy, int invert, Blt_PictureArithOps op)
}
declare 5 generic {
   void Blt_ApplyScalarToPictureWithMask(Blt_Picture dest, 
	Blt_Pixel *colorPtr, Blt_Picture mask, int invert, 
	Blt_PictureArithOps op)
}
declare 6 generic {
   void Blt_MaskPicture(Blt_Picture dest, Blt_Picture mask, 
	int x, int y, int w, int h, int dx, int dy, Blt_Pixel *colorPtr)
}
declare 7 generic {
   void Blt_BlankPicture(Blt_Picture picture, unsigned int colorValue)
}
declare 8 generic {
   void Blt_BlankRegion(Blt_Picture picture, int x, int y, int w, int h,
	unsigned int colorValue)
}
declare 9 generic {
   void Blt_BlurPicture(Blt_Picture dest, Blt_Picture src, int radius, 
	int numPasses)
}
declare 10 generic {
   void Blt_ResizePicture(Blt_Picture picture, int w, int h)
}
declare 11 generic {
   void Blt_AdjustPictureSize(Blt_Picture picture, int w, int h)
}
declare 12 generic {
   Blt_Picture Blt_ClonePicture(Blt_Picture picture)
}
declare 13 generic {
   void Blt_ConvolvePicture(Blt_Picture dest, Blt_Picture src, 
	Blt_ConvolveFilter vFilter, Blt_ConvolveFilter hFilter)
}
declare 14 generic {
   Blt_Picture Blt_CreatePicture(int w, int h)
}
declare 15 generic {
   Blt_Picture Blt_DitherPicture(Blt_Picture picture, 
	Blt_Pixel *palette)
}
declare 16 generic {
   void Blt_FlipPicture(Blt_Picture picture, int vertically)
}
declare 17 generic {
   void Blt_FreePicture(Blt_Picture picture)
}
declare 18 generic {
   Blt_Picture Blt_GreyscalePicture(Blt_Picture picture)
}
declare 19 generic {
   Blt_Picture Blt_QuantizePicture (Blt_Picture picture, int numColors)
}
declare 20 generic {
   void Blt_ResamplePicture (Blt_Picture dest, Blt_Picture src, 
	Blt_ResampleFilter hFilter, Blt_ResampleFilter vFilter)
}
declare 21 generic {
   Blt_Picture Blt_ScalePicture(Blt_Picture picture, int x, int y, 
	int w, int h, int dw, int dh)
}
declare 22 generic {
   Blt_Picture Blt_ScalePictureArea(Blt_Picture picture, int x, int y,
	int w, int h, int dw, int dh)
}
declare 23 generic {
   Blt_Picture Blt_ReflectPicture(Blt_Picture picture, int side)
}
declare 24 generic {
   Blt_Picture Blt_RotatePictureByShear(Blt_Picture picture, 
	float angle)
}
declare 25 generic {
   Blt_Picture Blt_RotatePicture(Blt_Picture picture, float angle)
}
declare 26 generic {
   Blt_Picture Blt_ProjectPicture(Blt_Picture picture, float *srcPts,
	float *destPts, Blt_Pixel *bg)
}
declare 27 generic {
   void Blt_TilePicture(Blt_Picture dest, Blt_Picture src, int xOrigin, 
	int yOrigin, int x, int y, int w, int h)
}
declare 28 generic {
   int Blt_PictureToPsData(Blt_Picture picture, int numComponents, 
	Tcl_DString *resultPtr, const char *prefix)
}
declare 29 generic {
   void Blt_SelectPixels(Blt_Picture dest, Blt_Picture src, 
	Blt_Pixel *lowerPtr, Blt_Pixel *upperPtr)
}
declare 30 generic {
   int Blt_GetPicture(Tcl_Interp *interp, const char *string, 
	Blt_Picture *picturePtr)
}
declare 31 generic {
   int Blt_GetPictureFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	Blt_Picture *picturePtr)
}
declare 32 generic {
   int Blt_GetResampleFilterFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
	Blt_ResampleFilter *filterPtr)
}
declare 33 generic {
   const char *Blt_NameOfResampleFilter(Blt_ResampleFilter filter)
}
declare 34 generic {
   void Blt_AssociateColors(Blt_Picture picture)
}
declare 35 generic {
   void Blt_UnassociateColors(Blt_Picture picture)
}
declare 36 generic {
   void Blt_MultiplyPixels(Blt_Picture picture, float value)
}
declare 37 generic {
   int Blt_GetBBoxFromObjv(Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv, PictRegion *regionPtr)
}
declare 38 generic {
   int Blt_AdjustRegionToPicture(Blt_Picture picture, 
	PictRegion *regionPtr)
}
declare 39 generic {
   int Blt_GetPixelFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	Blt_Pixel *pixelPtr)
}
declare 40 generic {
   int Blt_GetPixel(Tcl_Interp *interp, const char *string, 
	Blt_Pixel *pixelPtr)
}
declare 41 generic {
   const char *Blt_NameOfPixel(Blt_Pixel *pixelPtr)
}
declare 42 generic {
   void Blt_NotifyImageChanged(Blt_PictureImage image)
}
declare 43 generic {
   int Blt_QueryColors(Blt_Picture picture, Blt_HashTable *tablePtr)
}
declare 44 generic {
   void Blt_ClassifyPicture(Blt_Picture picture)
}
declare 45 generic {
   void Blt_TentHorizontally(Blt_Picture dest, Blt_Picture src)
}
declare 46 generic {
   void Blt_TentVertically(Blt_Picture dest, Blt_Picture src)
}
declare 47 generic {
   void Blt_ZoomHorizontally(Blt_Picture dest, Blt_Picture src, 
	Blt_ResampleFilter filter)
}
declare 48 generic {
   void Blt_ZoomVertically(Blt_Picture dest, Blt_Picture src, 
	Blt_ResampleFilter filter)
}
declare 49 generic {
   void Blt_BlendPictures(Blt_Picture dest, Blt_Picture src, 
	int sx, int sy, int w, int h, int dx, int dy)
}
declare 50 generic {
   void Blt_BlendPicturesByMode(Blt_Picture dest, Blt_Picture src, 
	Blt_BlendingMode mode)
}
declare 51 generic {
   void Blt_FadePicture(Blt_Picture picture, int x, int y, int w, int h,
	int alpha)
}
declare 52 generic {
   void Blt_CopyPictureBits(Blt_Picture dest, Blt_Picture src, 
	int sx, int sy, int w, int h, int dx, int dy)
}
declare 53 generic {
   void Blt_GammaCorrectPicture(Blt_Picture dest, Blt_Picture src, 
	float gamma)
}
declare 54 generic {
   void Blt_SharpenPicture(Blt_Picture dest, Blt_Picture src)
}
declare 55 generic {
   void Blt_ApplyColorToPicture(Blt_Picture pict, Blt_Pixel *colorPtr)
}
declare 56 generic {
   void Blt_SizeOfPicture(Blt_Picture pict, int *wPtr, int *hPtr)
}
declare 57 generic {
   Blt_DBuffer Blt_PictureToDBuffer(Blt_Picture picture, int numComp)
}
declare 58 generic {
   int Blt_ResetPicture(Tcl_Interp *interp, const char *imageName, 
	Blt_Picture picture)
}
declare 59 generic {
   void Blt_MapColors(Blt_Picture dest, Blt_Picture src, 
	Blt_ColorLookupTable clut)
}
declare 60 generic {
   Blt_ColorLookupTable Blt_GetColorLookupTable(
	struct _Blt_Chain *chainPtr, int numReqColors)
}
declare 61 generic {
   void Blt_FadePictureWithGradient(Blt_Picture picture, 
	PictFadeSettings *settingsPtr)
}
declare 62 generic {
   Blt_Picture Blt_ReflectPicture2(Blt_Picture picture, int side)
}
declare 63 generic {
   void Blt_SubtractColor(Blt_Picture picture, Blt_Pixel *colorPtr)
}
declare 64 generic {
   Blt_Picture Blt_PhotoToPicture (Tk_PhotoHandle photo)
}
declare 65 generic {
   Blt_Picture Blt_PhotoAreaToPicture (Tk_PhotoHandle photo, 
	int x, int y, int w, int h)
}
declare 66 generic {
   Blt_Picture Blt_DrawableToPicture(Tk_Window tkwin, 
 	Drawable drawable, int x, int y, int w, int h, float gamma)
}
declare 67 generic {
   Blt_Picture Blt_WindowToPicture(Display *display, 
	Drawable drawable, int x, int y, int w, int h, float gamma)
}
declare 68 generic {
   void Blt_PictureToPhoto(Blt_Picture picture, Tk_PhotoHandle photo)
}
declare 69 generic {
   int Blt_SnapPhoto(Tcl_Interp *interp, Tk_Window tkwin, 
	Drawable drawable, int sx, int sy, int w, int h, int dw, int dh, 
	const char *photoName, float gamma)
}
declare 70 generic {
   int Blt_SnapPicture(Tcl_Interp *interp, Tk_Window tkwin, 
	Drawable drawable, int sx, int sy, int w, int h, int dw, int dh, 
	const char *imageName, float gamma)
}
declare 71 generic {
   unsigned int Blt_XColorToPixel(XColor *colorPtr)
}
declare 72 generic {
   int Blt_IsPicture(Tk_Image tkImage)
}
declare 73 generic {
   Blt_Picture Blt_GetPictureFromImage(Tcl_Interp *interp, 
	Tk_Image tkImage, int *isPhotoPtr)
}
declare 74 generic {
   Blt_Picture Blt_GetPictureFromPictureImage(Tcl_Interp *interp,
	Tk_Image tkImage)
}
declare 75 generic {
   struct _Blt_Chain *Blt_GetPicturesFromPictureImage(
	Tcl_Interp *interp, Tk_Image tkImage)
}
declare 76 generic {
   Blt_Picture Blt_GetPictureFromPhotoImage(Tcl_Interp *interp,
	Tk_Image tkImage)
}
declare 77 generic {
   Blt_Picture Blt_CanvasToPicture(Tcl_Interp *interp, const char *s,
	float gamma)
}
declare 78 generic {
   int Blt_PictureRegisterProc(Tcl_Interp *interp, const char *name,
	Tcl_ObjCmdProc *proc)
}
declare 79 generic {
   void Blt_Shadow_Set(Blt_Shadow *sPtr, int width, int offset, 
	int color, int alpha)
}
declare 80 generic {
   int Blt_Palette_GetFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	Blt_Palette *palPtr)
}
declare 81 generic {
   int Blt_Palette_GetFromString(Tcl_Interp *interp, const char *string,
	Blt_Palette *palPtr)
}
declare 82 generic {
   int Blt_Palette_GetColorFromAbsoluteValue(Blt_Palette palette, 
	double absValue, double rangeMin, double rangeMax)
}
declare 83 generic {
   void Blt_Palette_SetRange(Blt_Palette palette, double min, 
	double max)
}
declare 84 generic {
   int Blt_Palette_GetColor(Blt_Palette palette, double relValue)
}
declare 85 generic {
   void Blt_Palette_CreateNotifier(Blt_Palette palette, 
	Blt_Palette_NotifyProc *proc, ClientData clientData)
}
declare 86 generic {
   void Blt_Palette_DeleteNotifier(Blt_Palette palette, 
	ClientData clientData)
}
declare 87 generic {
   const char *Blt_Palette_Name(Blt_Palette palette)
}
declare 88 generic {
   Blt_Palette Blt_Palette_TwoColorPalette(int low, int high)
}
declare 89 generic {
   void Blt_Palette_Free(Blt_Palette palette)
}
declare 90 generic {
   void Blt_GradientPicture(Blt_Picture picture, Blt_Pixel *highPtr, 
	Blt_Pixel *lowPtr, Blt_Gradient *gradientPtr, Blt_Jitter *jitterPtr)
}
declare 91 generic {
   void Blt_TexturePicture(Blt_Picture picture, Blt_Pixel *lowPtr, 
	Blt_Pixel *highPtr, Blt_TextureType type)
}
declare 92 generic {
   void Blt_Paintbrush_Init(Blt_Paintbrush *brushPtr)
}
declare 93 generic {
   void Blt_Paintbrush_Free(Blt_Paintbrush *brushPtr)
}
declare 94 generic {
   int Blt_Paintbrush_Get(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	Blt_Paintbrush **brushPtrPtr)
}
declare 95 generic {
   int Blt_Paintbrush_GetFromString(Tcl_Interp *interp, 
	const char *string, Blt_Paintbrush **brushPtrPtr)
}
declare 96 generic {
   void Blt_Paintbrush_SetPalette(Blt_Paintbrush *brushPtr, 
	Blt_Palette palette)
}
declare 97 generic {
   void Blt_Paintbrush_SetColorProc(Blt_Paintbrush *brushPtr, 
	Blt_Paintbrush_ColorProc *proc, ClientData clientData)
}
declare 98 generic {
   void Blt_Paintbrush_SetColors(Blt_Paintbrush *brushPtr, 
	Blt_Pixel *lowPtr, Blt_Pixel *highPtr)
}
declare 99 generic {
   void Blt_Paintbrush_Region(Blt_Paintbrush *brushPtr, int x, int y, 
	int w, 	int h)
}
declare 100 generic {
   void Blt_Paintbrush_SetTile(Blt_Paintbrush *brushPtr, 
	Blt_Picture tile)
}
declare 101 generic {
   void Blt_Paintbrush_SetTexture(Blt_Paintbrush *brushPtr)
}
declare 102 generic {
   void Blt_Paintbrush_SetGradient(Blt_Paintbrush *brushPtr)
}
declare 103 generic {
   void Blt_Paintbrush_SetColor(Blt_Paintbrush *brushPtr, 
	unsigned int value)
}
declare 104 generic {
   void Blt_Paintbrush_SetOrigin(Blt_Paintbrush *brushPtr, int x,int y)
}
declare 105 generic {
   int Blt_Paintbrush_GetColor(Blt_Paintbrush *brushPtr, int x, int y)
}
declare 106 generic {
   void Blt_PaintRectangle(Blt_Picture picture, int x, int y, int w, 
	int h, int dx, int dy, Blt_Paintbrush *brushPtr)
}
declare 107 generic {
   void Blt_PaintPolygon(Blt_Picture picture, int n, Point2f *vertices,
	Blt_Paintbrush *brushPtr)
}
declare 108 generic {
   int Blt_PictureRegisterFormat(Tcl_Interp *interp, 
	const char *name, 
	Blt_PictureIsFmtProc *isFmtProc,
	Blt_PictureReadProc *readProc, 
	Blt_PictureWriteProc *writeProc,
	Blt_PictureImportProc *importProc, 
	Blt_PictureExportProc *exportProc)
}
declare 109 generic {
   Blt_Picture Blt_GetNthPicture(Blt_Chain chain, size_t index)
}
declare 110 generic {
   Blt_PictFormat *Blt_FindPictureFormat(Tcl_Interp *interp,
	const char *ext)
}

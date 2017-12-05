/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#define BUILD_BLT_TK_PROCS 1
#include <bltInt.h>

extern BltTkIntProcs bltTkIntProcs;

/* !BEGIN!: Do not edit below this line. */

static BltTkStubHooks bltTkStubHooks = {
    &bltTkIntProcs
};

BltTkProcs bltTkProcs = {
    TCL_STUB_MAGIC,
    &bltTkStubHooks,
    NULL, /* 0 */
    Blt_ApplyPictureToPicture, /* 1 */
    Blt_ApplyScalarToPicture, /* 2 */
    Blt_ApplyPictureToPictureWithMask, /* 3 */
    Blt_ApplyScalarToPictureWithMask, /* 4 */
    Blt_MaskPicture, /* 5 */
    Blt_BlankPicture, /* 6 */
    Blt_BlankRegion, /* 7 */
    Blt_BlurPicture, /* 8 */
    Blt_ResizePicture, /* 9 */
    Blt_AdjustPictureSize, /* 10 */
    Blt_ClonePicture, /* 11 */
    Blt_ConvolvePicture, /* 12 */
    Blt_CreatePicture, /* 13 */
    Blt_DitherPicture, /* 14 */
    Blt_FlipPicture, /* 15 */
    Blt_FreePicture, /* 16 */
    Blt_GreyscalePicture, /* 17 */
    Blt_QuantizePicture, /* 18 */
    Blt_ResamplePicture, /* 19 */
    Blt_ResamplePicture2, /* 20 */
    Blt_ScalePicture, /* 21 */
    Blt_ScalePictureArea, /* 22 */
    Blt_ReflectPicture, /* 23 */
    Blt_RotatePictureByShear, /* 24 */
    Blt_RotatePicture, /* 25 */
    Blt_ProjectPicture, /* 26 */
    Blt_TilePicture, /* 27 */
    Blt_PictureToPsData, /* 28 */
    Blt_SelectPixels, /* 29 */
    Blt_GetPicture, /* 30 */
    Blt_GetPictureFromObj, /* 31 */
    Blt_GetResampleFilterFromObj, /* 32 */
    Blt_NameOfResampleFilter, /* 33 */
    Blt_PremultiplyColor, /* 34 */
    Blt_UnmultiplyColor, /* 35 */
    Blt_PremultiplyColors, /* 36 */
    Blt_UnmultiplyColors, /* 37 */
    Blt_MultiplyPixels, /* 38 */
    Blt_GetBBoxFromObjv, /* 39 */
    Blt_AdjustRegionToPicture, /* 40 */
    Blt_GetPixelFromObj, /* 41 */
    Blt_GetPixel, /* 42 */
    Blt_NameOfPixel, /* 43 */
    Blt_NotifyImageChanged, /* 44 */
    Blt_QueryColors, /* 45 */
    Blt_ClassifyPicture, /* 46 */
    Blt_TentHorizontally, /* 47 */
    Blt_TentVertically, /* 48 */
    Blt_ZoomHorizontally, /* 49 */
    Blt_ZoomVertically, /* 50 */
    Blt_CompositeRegion, /* 51 */
    Blt_CompositePictures, /* 52 */
    Blt_ColorBlendPictures, /* 53 */
    Blt_FadePicture, /* 54 */
    Blt_CopyRegion, /* 55 */
    Blt_CopyPictureBits, /* 56 */
    Blt_GammaCorrectPicture, /* 57 */
    Blt_SharpenPicture, /* 58 */
    Blt_ApplyColorToPicture, /* 59 */
    Blt_SizeOfPicture, /* 60 */
    Blt_PictureToDBuffer, /* 61 */
    Blt_ResetPicture, /* 62 */
    Blt_MapColors, /* 63 */
    Blt_GetColorLookupTable, /* 64 */
    Blt_FadePictureWithGradient, /* 65 */
    Blt_ReflectPicture2, /* 66 */
    Blt_SubtractColor, /* 67 */
    Blt_PhotoToPicture, /* 68 */
    Blt_PhotoAreaToPicture, /* 69 */
    Blt_DrawableToPicture, /* 70 */
    Blt_WindowToPicture, /* 71 */
    Blt_PictureToPhoto, /* 72 */
    Blt_SnapPhoto, /* 73 */
    Blt_SnapPicture, /* 74 */
    Blt_XColorToPixel, /* 75 */
    Blt_PixelToXColor, /* 76 */
    Blt_IsPicture, /* 77 */
    Blt_GetPicturesFromPictureImage, /* 78 */
    Blt_GetPictureFromImage, /* 79 */
    Blt_GetPictureFromPictureImage, /* 80 */
    Blt_GetPictureFromPhotoImage, /* 81 */
    Blt_GetPictureFromBitmapImage, /* 82 */
    Blt_CanvasToPicture, /* 83 */
    Blt_GraphToPicture, /* 84 */
    Blt_PictureRegisterProc, /* 85 */
    Blt_Shadow_Set, /* 86 */
    Blt_EmbossPicture, /* 87 */
    Blt_FadeColor, /* 88 */
    Blt_Dissolve2, /* 89 */
    Blt_CrossFadePictures, /* 90 */
    Blt_FadeFromColor, /* 91 */
    Blt_FadeToColor, /* 92 */
    Blt_WipePictures, /* 93 */
    Blt_PictureRegisterFormat, /* 94 */
    Blt_GetNthPicture, /* 95 */
    Blt_FindPictureFormat, /* 96 */
};

/* !END!: Do not edit above this line. */

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
    Blt_ScalePicture, /* 20 */
    Blt_ScalePictureArea, /* 21 */
    Blt_ReflectPicture, /* 22 */
    Blt_RotatePictureByShear, /* 23 */
    Blt_RotatePicture, /* 24 */
    Blt_ProjectPicture, /* 25 */
    Blt_TilePicture, /* 26 */
    Blt_PictureToPsData, /* 27 */
    Blt_SelectPixels, /* 28 */
    Blt_GetPicture, /* 29 */
    Blt_GetPictureFromObj, /* 30 */
    Blt_GetResampleFilterFromObj, /* 31 */
    Blt_NameOfResampleFilter, /* 32 */
    Blt_PremultiplyColor, /* 33 */
    Blt_UnmultiplyColor, /* 34 */
    Blt_PremultiplyColors, /* 35 */
    Blt_UnmultiplyColors, /* 36 */
    Blt_MultiplyPixels, /* 37 */
    Blt_GetBBoxFromObjv, /* 38 */
    Blt_AdjustRegionToPicture, /* 39 */
    Blt_GetPixelFromObj, /* 40 */
    Blt_GetPixel, /* 41 */
    Blt_NameOfPixel, /* 42 */
    Blt_NotifyImageChanged, /* 43 */
    Blt_QueryColors, /* 44 */
    Blt_ClassifyPicture, /* 45 */
    Blt_TentHorizontally, /* 46 */
    Blt_TentVertically, /* 47 */
    Blt_ZoomHorizontally, /* 48 */
    Blt_ZoomVertically, /* 49 */
    Blt_CompositeRegion, /* 50 */
    Blt_CompositePictures, /* 51 */
    Blt_ColorBlendPictures, /* 52 */
    Blt_FadePicture, /* 53 */
    Blt_CopyRegion, /* 54 */
    Blt_CopyPictures, /* 55 */
    Blt_GammaCorrectPicture, /* 56 */
    Blt_SharpenPicture, /* 57 */
    Blt_ApplyColorToPicture, /* 58 */
    Blt_SizeOfPicture, /* 59 */
    Blt_PictureToDBuffer, /* 60 */
    Blt_ResetPicture, /* 61 */
    Blt_MapColors, /* 62 */
    Blt_GetColorLookupTable, /* 63 */
    Blt_FadePictureWithGradient, /* 64 */
    Blt_ReflectPicture2, /* 65 */
    Blt_SubtractColor, /* 66 */
    Blt_PhotoToPicture, /* 67 */
    Blt_PhotoAreaToPicture, /* 68 */
    Blt_DrawableToPicture, /* 69 */
    Blt_WindowToPicture, /* 70 */
    Blt_PictureToPhoto, /* 71 */
    Blt_SnapPhoto, /* 72 */
    Blt_SnapPicture, /* 73 */
    Blt_XColorToPixel, /* 74 */
    Blt_IsPicture, /* 75 */
    Blt_GetPictureFromImage, /* 76 */
    Blt_GetPictureFromPictureImage, /* 77 */
    Blt_GetPicturesFromPictureImage, /* 78 */
    Blt_GetPictureFromPhotoImage, /* 79 */
    Blt_CanvasToPicture, /* 80 */
    Blt_GraphToPicture, /* 81 */
    Blt_PictureRegisterProc, /* 82 */
    Blt_Shadow_Set, /* 83 */
    Blt_EmbossPicture, /* 84 */
    Blt_FadeColor, /* 85 */
    Blt_Dissolve2, /* 86 */
    Blt_CrossFadePictures, /* 87 */
    Blt_FadeFromColor, /* 88 */
    Blt_FadeToColor, /* 89 */
    Blt_WipePictures, /* 90 */
    Blt_PictureRegisterFormat, /* 91 */
    Blt_GetNthPicture, /* 92 */
    Blt_FindPictureFormat, /* 93 */
};

/* !END!: Do not edit above this line. */

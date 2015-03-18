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
    Blt_Jitter_Init, /* 1 */
    Blt_ApplyPictureToPicture, /* 2 */
    Blt_ApplyScalarToPicture, /* 3 */
    Blt_ApplyPictureToPictureWithMask, /* 4 */
    Blt_ApplyScalarToPictureWithMask, /* 5 */
    Blt_MaskPicture, /* 6 */
    Blt_BlankPicture, /* 7 */
    Blt_BlankRegion, /* 8 */
    Blt_BlurPicture, /* 9 */
    Blt_ResizePicture, /* 10 */
    Blt_AdjustPictureSize, /* 11 */
    Blt_ClonePicture, /* 12 */
    Blt_ConvolvePicture, /* 13 */
    Blt_CreatePicture, /* 14 */
    Blt_DitherPicture, /* 15 */
    Blt_FlipPicture, /* 16 */
    Blt_FreePicture, /* 17 */
    Blt_GreyscalePicture, /* 18 */
    Blt_QuantizePicture, /* 19 */
    Blt_ResamplePicture, /* 20 */
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
    Blt_AssociateColor, /* 34 */
    Blt_UnassociateColor, /* 35 */
    Blt_AssociateColors, /* 36 */
    Blt_UnassociateColors, /* 37 */
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
    Blt_BlendRegion, /* 51 */
    Blt_BlendPicturesByMode, /* 52 */
    Blt_FadePicture, /* 53 */
    Blt_CopyPictureBits, /* 54 */
    Blt_GammaCorrectPicture, /* 55 */
    Blt_SharpenPicture, /* 56 */
    Blt_ApplyColorToPicture, /* 57 */
    Blt_SizeOfPicture, /* 58 */
    Blt_PictureToDBuffer, /* 59 */
    Blt_ResetPicture, /* 60 */
    Blt_MapColors, /* 61 */
    Blt_GetColorLookupTable, /* 62 */
    Blt_FadePictureWithGradient, /* 63 */
    Blt_ReflectPicture2, /* 64 */
    Blt_SubtractColor, /* 65 */
    Blt_PhotoToPicture, /* 66 */
    Blt_PhotoAreaToPicture, /* 67 */
    Blt_DrawableToPicture, /* 68 */
    Blt_WindowToPicture, /* 69 */
    Blt_PictureToPhoto, /* 70 */
    Blt_SnapPhoto, /* 71 */
    Blt_SnapPicture, /* 72 */
    Blt_XColorToPixel, /* 73 */
    Blt_IsPicture, /* 74 */
    Blt_GetPictureFromImage, /* 75 */
    Blt_GetPictureFromPictureImage, /* 76 */
    Blt_GetPicturesFromPictureImage, /* 77 */
    Blt_GetPictureFromPhotoImage, /* 78 */
    Blt_CanvasToPicture, /* 79 */
    Blt_PictureRegisterProc, /* 80 */
    Blt_Shadow_Set, /* 81 */
    Blt_GradientPicture, /* 82 */
    Blt_TexturePicture, /* 83 */
    Blt_PaintBrush_Init, /* 84 */
    Blt_PaintBrush_Free, /* 85 */
    Blt_PaintBrush_Get, /* 86 */
    Blt_PaintBrush_GetFromString, /* 87 */
    Blt_PaintBrush_SetPalette, /* 88 */
    Blt_PaintBrush_SetColorProc, /* 89 */
    Blt_PaintBrush_SetColors, /* 90 */
    Blt_PaintBrush_Region, /* 91 */
    Blt_PaintBrush_SetTile, /* 92 */
    Blt_PaintBrush_SetTexture, /* 93 */
    Blt_PaintBrush_SetGradient, /* 94 */
    Blt_PaintBrush_SetColor, /* 95 */
    Blt_PaintBrush_SetOrigin, /* 96 */
    Blt_PaintBrush_GetAssociatedColor, /* 97 */
    Blt_PaintRectangle, /* 98 */
    Blt_PaintPolygon, /* 99 */
    Blt_EmbossPicture, /* 100 */
    Blt_FadeColor, /* 101 */
    Blt_PictureRegisterFormat, /* 102 */
    Blt_GetNthPicture, /* 103 */
    Blt_FindPictureFormat, /* 104 */
};

/* !END!: Do not edit above this line. */

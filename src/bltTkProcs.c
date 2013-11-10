/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#define BUILD_BLT_TK_PROCS 1
#include <bltInt.h>

/* !BEGIN!: Do not edit below this line. */
extern BltTkIntProcs bltTkIntProcs;

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
    Blt_AssociateColors, /* 34 */
    Blt_UnassociateColors, /* 35 */
    Blt_MultiplyPixels, /* 36 */
    Blt_GetBBoxFromObjv, /* 37 */
    Blt_AdjustRegionToPicture, /* 38 */
    Blt_GetPixelFromObj, /* 39 */
    Blt_GetPixel, /* 40 */
    Blt_NameOfPixel, /* 41 */
    Blt_NotifyImageChanged, /* 42 */
    Blt_QueryColors, /* 43 */
    Blt_ClassifyPicture, /* 44 */
    Blt_TentHorizontally, /* 45 */
    Blt_TentVertically, /* 46 */
    Blt_ZoomHorizontally, /* 47 */
    Blt_ZoomVertically, /* 48 */
    Blt_BlendRegion, /* 49 */
    Blt_BlendPicturesByMode, /* 50 */
    Blt_FadePicture, /* 51 */
    Blt_CopyPictureBits, /* 52 */
    Blt_GammaCorrectPicture, /* 53 */
    Blt_SharpenPicture, /* 54 */
    Blt_ApplyColorToPicture, /* 55 */
    Blt_SizeOfPicture, /* 56 */
    Blt_PictureToDBuffer, /* 57 */
    Blt_ResetPicture, /* 58 */
    Blt_MapColors, /* 59 */
    Blt_GetColorLookupTable, /* 60 */
    Blt_FadePictureWithGradient, /* 61 */
    Blt_ReflectPicture2, /* 62 */
    Blt_SubtractColor, /* 63 */
    Blt_PhotoToPicture, /* 64 */
    Blt_PhotoAreaToPicture, /* 65 */
    Blt_DrawableToPicture, /* 66 */
    Blt_WindowToPicture, /* 67 */
    Blt_PictureToPhoto, /* 68 */
    Blt_SnapPhoto, /* 69 */
    Blt_SnapPicture, /* 70 */
    Blt_XColorToPixel, /* 71 */
    Blt_IsPicture, /* 72 */
    Blt_GetPictureFromImage, /* 73 */
    Blt_GetPictureFromPictureImage, /* 74 */
    Blt_GetPicturesFromPictureImage, /* 75 */
    Blt_GetPictureFromPhotoImage, /* 76 */
    Blt_CanvasToPicture, /* 77 */
    Blt_PictureRegisterProc, /* 78 */
    Blt_Shadow_Set, /* 79 */
    Blt_Palette_GetFromObj, /* 80 */
    Blt_Palette_GetFromString, /* 81 */
    Blt_Palette_GetAssociatedColorFromAbsoluteValue, /* 82 */
    Blt_Palette_SetRange, /* 83 */
    Blt_Palette_GetAssociatedColor, /* 84 */
    Blt_Palette_CreateNotifier, /* 85 */
    Blt_Palette_DeleteNotifier, /* 86 */
    Blt_Palette_Name, /* 87 */
    Blt_Palette_TwoColorPalette, /* 88 */
    Blt_Palette_Free, /* 89 */
    Blt_GradientPicture, /* 90 */
    Blt_TexturePicture, /* 91 */
    Blt_PaintBrush_Init, /* 92 */
    Blt_PaintBrush_Free, /* 93 */
    Blt_PaintBrush_Get, /* 94 */
    Blt_PaintBrush_GetFromString, /* 95 */
    Blt_PaintBrush_SetPalette, /* 96 */
    Blt_PaintBrush_SetColorProc, /* 97 */
    Blt_PaintBrush_SetColors, /* 98 */
    Blt_PaintBrush_Region, /* 99 */
    Blt_PaintBrush_SetTile, /* 100 */
    Blt_PaintBrush_SetTexture, /* 101 */
    Blt_PaintBrush_SetGradient, /* 102 */
    Blt_PaintBrush_SetColor, /* 103 */
    Blt_PaintBrush_SetOrigin, /* 104 */
    Blt_PaintBrush_GetAssociatedColor, /* 105 */
    Blt_PaintRectangle, /* 106 */
    Blt_PaintPolygon, /* 107 */
    Blt_PictureRegisterFormat, /* 108 */
    Blt_GetNthPicture, /* 109 */
    Blt_FindPictureFormat, /* 110 */
};

/* !END!: Do not edit above this line. */

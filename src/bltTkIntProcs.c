/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#define BUILD_BLT_TK_PROCS 1
#include <bltInt.h>

/* !BEGIN!: Do not edit below this line. */

BltTkIntProcs bltTkIntProcs = {
    TCL_STUB_MAGIC,
    NULL,
    NULL, /* 0 */
    Blt_Draw3DRectangle, /* 1 */
    Blt_Fill3DRectangle, /* 2 */
    Blt_AdjustViewport, /* 3 */
    Blt_GetScrollInfoFromObj, /* 4 */
    Blt_UpdateScrollbar, /* 5 */
    Blt_FreeColorPair, /* 6 */
    Blt_GetPrivateGC, /* 7 */
    Blt_GetPrivateGCFromDrawable, /* 8 */
    Blt_FreePrivateGC, /* 9 */
    Blt_GetWindowFromObj, /* 10 */
    Blt_GetWindowName, /* 11 */
    Blt_GetChildrenFromWindow, /* 12 */
    Blt_GetParentWindow, /* 13 */
    Blt_FindChild, /* 14 */
    Blt_FirstChild, /* 15 */
    Blt_NextChild, /* 16 */
    Blt_RelinkWindow, /* 17 */
    Blt_Toplevel, /* 18 */
    Blt_GetPixels, /* 19 */
    Blt_GetXY, /* 20 */
    Blt_DrawArrowOld, /* 21 */
    Blt_DrawArrow, /* 22 */
    Blt_MakeTransparentWindowExist, /* 23 */
    Blt_TranslateAnchor, /* 24 */
    Blt_AnchorPoint, /* 25 */
    Blt_MaxRequestSize, /* 26 */
    Blt_GetWindowId, /* 27 */
    Blt_InitXRandrConfig, /* 28 */
    Blt_SizeOfScreen, /* 29 */
    Blt_RootX, /* 30 */
    Blt_RootY, /* 31 */
    Blt_RootCoordinates, /* 32 */
    Blt_MapToplevelWindow, /* 33 */
    Blt_UnmapToplevelWindow, /* 34 */
    Blt_RaiseToplevelWindow, /* 35 */
    Blt_LowerToplevelWindow, /* 36 */
    Blt_ResizeToplevelWindow, /* 37 */
    Blt_MoveToplevelWindow, /* 38 */
    Blt_MoveResizeToplevelWindow, /* 39 */
    Blt_GetWindowExtents, /* 40 */
    Blt_GetWindowInstanceData, /* 41 */
    Blt_SetWindowInstanceData, /* 42 */
    Blt_DeleteWindowInstanceData, /* 43 */
    Blt_ReparentWindow, /* 44 */
    Blt_GetDrawableAttributes, /* 45 */
    Blt_SetDrawableAttributes, /* 46 */
    Blt_SetDrawableAttributesFromWindow, /* 47 */
    Blt_FreeDrawableAttributes, /* 48 */
    Blt_GetBitmapGC, /* 49 */
    Blt_GetPixmapAbortOnError, /* 50 */
    Blt_ScreenDPI, /* 51 */
    Blt_OldConfigModified, /* 52 */
    Blt_GetLineExtents, /* 53 */
    Blt_GetBoundingBox, /* 54 */
    Blt_ParseTifTags, /* 55 */
    Blt_GetFont, /* 56 */
    Blt_AllocFontFromObj, /* 57 */
    Blt_DrawWithEllipsis, /* 58 */
    Blt_GetFontFromObj, /* 59 */
    Blt_Font_GetMetrics, /* 60 */
    Blt_TextWidth, /* 61 */
    Blt_Font_GetInterp, /* 62 */
    Blt_Font_GetFile, /* 63 */
    Blt_NewTileBrush, /* 64 */
    Blt_NewLinearGradientBrush, /* 65 */
    Blt_NewStripesBrush, /* 66 */
    Blt_NewCheckersBrush, /* 67 */
    Blt_NewRadialGradientBrush, /* 68 */
    Blt_NewConicalGradientBrush, /* 69 */
    Blt_NewColorBrush, /* 70 */
    Blt_GetBrushTypeName, /* 71 */
    Blt_GetBrushName, /* 72 */
    Blt_GetBrushColorName, /* 73 */
    Blt_GetBrushPixel, /* 74 */
    Blt_GetBrushType, /* 75 */
    Blt_GetXColorFromBrush, /* 76 */
    Blt_ConfigurePaintBrush, /* 77 */
    Blt_GetBrushTypeFromObj, /* 78 */
    Blt_FreeBrush, /* 79 */
    Blt_GetPaintBrushFromObj, /* 80 */
    Blt_GetPaintBrush, /* 81 */
    Blt_SetLinearGradientBrushPalette, /* 82 */
    Blt_SetLinearGradientBrushCalcProc, /* 83 */
    Blt_SetLinearGradientBrushColors, /* 84 */
    Blt_SetTileBrushPicture, /* 85 */
    Blt_SetColorBrushColor, /* 86 */
    Blt_SetBrushOrigin, /* 87 */
    Blt_SetBrushOpacity, /* 88 */
    Blt_SetBrushArea, /* 89 */
    Blt_GetBrushAlpha, /* 90 */
    Blt_GetBrushOrigin, /* 91 */
    Blt_GetAssociatedColorFromBrush, /* 92 */
    Blt_IsVerticalLinearBrush, /* 93 */
    Blt_IsHorizontalLinearBrush, /* 94 */
    Blt_PaintRectangle, /* 95 */
    Blt_PaintPolygon, /* 96 */
    Blt_CreateBrushNotifier, /* 97 */
    Blt_DeleteBrushNotifier, /* 98 */
    Blt_GetBg, /* 99 */
    Blt_GetBgFromObj, /* 100 */
    Blt_Bg_BorderColor, /* 101 */
    Blt_Bg_Border, /* 102 */
    Blt_Bg_PaintBrush, /* 103 */
    Blt_Bg_Name, /* 104 */
    Blt_Bg_Free, /* 105 */
    Blt_Bg_DrawRectangle, /* 106 */
    Blt_Bg_FillRectangle, /* 107 */
    Blt_Bg_DrawPolygon, /* 108 */
    Blt_Bg_FillPolygon, /* 109 */
    Blt_Bg_GetOrigin, /* 110 */
    Blt_Bg_SetChangedProc, /* 111 */
    Blt_Bg_SetOrigin, /* 112 */
    Blt_Bg_DrawFocus, /* 113 */
    Blt_Bg_BorderGC, /* 114 */
    Blt_Bg_SetFromBackground, /* 115 */
    Blt_Bg_UnsetClipRegion, /* 116 */
    Blt_Bg_SetClipRegion, /* 117 */
    Blt_Bg_GetColor, /* 118 */
    Blt_3DBorder_SetClipRegion, /* 119 */
    Blt_3DBorder_UnsetClipRegion, /* 120 */
    Blt_DestroyBindingTable, /* 121 */
    Blt_CreateBindingTable, /* 122 */
    Blt_ConfigureBindings, /* 123 */
    Blt_ConfigureBindingsFromObj, /* 124 */
    Blt_PickCurrentItem, /* 125 */
    Blt_DeleteBindings, /* 126 */
    Blt_MoveBindingTable, /* 127 */
    Blt_Afm_SetPrinting, /* 128 */
    Blt_Afm_IsPrinting, /* 129 */
    Blt_Afm_TextWidth, /* 130 */
    Blt_Afm_GetMetrics, /* 131 */
    Blt_Afm_GetPostscriptFamily, /* 132 */
    Blt_Afm_GetPostscriptName, /* 133 */
    Blt_SetDashes, /* 134 */
    Blt_ResetLimits, /* 135 */
    Blt_GetLimitsFromObj, /* 136 */
    Blt_ConfigureInfoFromObj, /* 137 */
    Blt_ConfigureValueFromObj, /* 138 */
    Blt_ConfigureWidgetFromObj, /* 139 */
    Blt_ConfigureComponentFromObj, /* 140 */
    Blt_ConfigModified, /* 141 */
    Blt_FreeOptions, /* 142 */
    Blt_ObjIsOption, /* 143 */
    Blt_GetPixelsFromObj, /* 144 */
    Blt_GetPadFromObj, /* 145 */
    Blt_GetDashesFromObj, /* 146 */
    Blt_Image_IsDeleted, /* 147 */
    Blt_Image_GetMaster, /* 148 */
    Blt_Image_GetType, /* 149 */
    Blt_Image_GetInstanceData, /* 150 */
    Blt_Image_GetMasterData, /* 151 */
    Blt_Image_Name, /* 152 */
    Blt_Image_NameOfType, /* 153 */
    Blt_FreePainter, /* 154 */
    Blt_GetPainter, /* 155 */
    Blt_GetPainterFromDrawable, /* 156 */
    Blt_PainterGC, /* 157 */
    Blt_PainterDepth, /* 158 */
    Blt_SetPainterClipRegion, /* 159 */
    Blt_UnsetPainterClipRegion, /* 160 */
    Blt_PaintPicture, /* 161 */
    Blt_PaintPictureWithBlend, /* 162 */
    Blt_PaintCheckbox, /* 163 */
    Blt_PaintRadioButton, /* 164 */
    Blt_PaintRadioButtonOld, /* 165 */
    Blt_PaintDelete, /* 166 */
    Blt_PaintArrowHead, /* 167 */
    Blt_PaintArrowHead2, /* 168 */
    Blt_PaintChevron, /* 169 */
    Blt_PaintArrow, /* 170 */
    Blt_Ps_Create, /* 171 */
    Blt_Ps_Free, /* 172 */
    Blt_Ps_GetValue, /* 173 */
    Blt_Ps_GetDBuffer, /* 174 */
    Blt_Ps_GetInterp, /* 175 */
    Blt_Ps_GetScratchBuffer, /* 176 */
    Blt_Ps_SetInterp, /* 177 */
    Blt_Ps_Append, /* 178 */
    Blt_Ps_AppendBytes, /* 179 */
    Blt_Ps_VarAppend, /* 180 */
    Blt_Ps_Format, /* 181 */
    Blt_Ps_SetClearBackground, /* 182 */
    Blt_Ps_IncludeFile, /* 183 */
    Blt_Ps_GetPicaFromObj, /* 184 */
    Blt_Ps_GetPadFromObj, /* 185 */
    Blt_Ps_ComputeBoundingBox, /* 186 */
    Blt_Ps_DrawPicture, /* 187 */
    Blt_Ps_Rectangle, /* 188 */
    Blt_Ps_Rectangle2, /* 189 */
    Blt_Ps_SaveFile, /* 190 */
    Blt_Ps_XSetLineWidth, /* 191 */
    Blt_Ps_XSetBackground, /* 192 */
    Blt_Ps_XSetBitmapData, /* 193 */
    Blt_Ps_XSetForeground, /* 194 */
    Blt_Ps_XSetFont, /* 195 */
    Blt_Ps_XSetDashes, /* 196 */
    Blt_Ps_XSetLineAttributes, /* 197 */
    Blt_Ps_XSetStipple, /* 198 */
    Blt_Ps_Polyline, /* 199 */
    Blt_Ps_XDrawLines, /* 200 */
    Blt_Ps_XDrawSegments, /* 201 */
    Blt_Ps_DrawPolyline, /* 202 */
    Blt_Ps_DrawSegments2d, /* 203 */
    Blt_Ps_Draw3DRectangle, /* 204 */
    Blt_Ps_Fill3DRectangle, /* 205 */
    Blt_Ps_XFillRectangle, /* 206 */
    Blt_Ps_XFillRectangles, /* 207 */
    Blt_Ps_XFillPolygon, /* 208 */
    Blt_Ps_DrawPhoto, /* 209 */
    Blt_Ps_XDrawWindow, /* 210 */
    Blt_Ps_DrawText, /* 211 */
    Blt_Ps_DrawBitmap, /* 212 */
    Blt_Ps_XSetCapStyle, /* 213 */
    Blt_Ps_XSetJoinStyle, /* 214 */
    Blt_Ps_PolylineFromXPoints, /* 215 */
    Blt_Ps_Polygon, /* 216 */
    Blt_Ps_SetPrinting, /* 217 */
    Blt_Ps_TextLayout, /* 218 */
    Blt_Ps_TextString, /* 219 */
    Blt_Ps_GetString, /* 220 */
    Blt_DrawText, /* 221 */
    Blt_DrawText2, /* 222 */
    Blt_DrawTextWithRotatedFont, /* 223 */
    Blt_DrawLayout, /* 224 */
    Blt_GetTextExtents, /* 225 */
    Blt_MeasureText, /* 226 */
    Blt_RotateStartingTextPositions, /* 227 */
    Blt_TkTextLayout_CharBbox, /* 228 */
    Blt_TkTextLayout_Compute, /* 229 */
    Blt_TkTextLayout_Draw, /* 230 */
    Blt_TkTextLayout_Free, /* 231 */
    Blt_TkTextLayout_UnderlineSingleChar, /* 232 */
    Blt_Ts_Bitmap, /* 233 */
    Blt_Ts_CreateLayout, /* 234 */
    Blt_Ts_DrawLayout, /* 235 */
    Blt_Ts_DrawText, /* 236 */
    Blt_Ts_FreeStyle, /* 237 */
    Blt_Ts_GetExtents, /* 238 */
    Blt_Ts_ResetStyle, /* 239 */
    Blt_Ts_SetDrawStyle, /* 240 */
    Blt_Ts_TitleLayout, /* 241 */
    Blt_Ts_UnderlineChars, /* 242 */
};

/* !END!: Do not edit above this line. */

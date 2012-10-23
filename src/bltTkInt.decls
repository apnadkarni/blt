library blt
interface bltTkInt
declare 1 generic {
   void Blt_Draw3DRectangle(Tk_Window tkwin, Drawable drawable,
	Tk_3DBorder border, int x, int y, int width, int height, 
	int borderWidth, int relief)
}
declare 2 generic {
   void Blt_Fill3DRectangle(Tk_Window tkwin, Drawable drawable,
	Tk_3DBorder border, int x, int y, int width, int height, 
	int borderWidth, int relief)
}
declare 3 generic {
   int Blt_AdjustViewport (int offset, int worldSize, int windowSize, 
	int scrollUnits, int scrollMode)
}
declare 4 generic {
   int Blt_GetScrollInfoFromObj (Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv, int *offsetPtr, int worldSize, int windowSize,
	int scrollUnits, int scrollMode)
}
declare 5 generic {
   void Blt_UpdateScrollbar(Tcl_Interp *interp, 
	Tcl_Obj *scrollCmdObjPtr, int first, int last, int width)
}
declare 6 generic {
   void Blt_FreeColorPair (ColorPair *pairPtr)
}
declare 7 generic {
   int Blt_LineRectClip(Region2d *regionPtr, Point2d *p, Point2d *q)
}
declare 8 generic {
   GC Blt_GetPrivateGC(Tk_Window tkwin, unsigned long gcMask,
	XGCValues *valuePtr)
}
declare 9 generic {
   GC Blt_GetPrivateGCFromDrawable(Display *display, Drawable drawable, 
	unsigned long gcMask, XGCValues *valuePtr)
}
declare 10 generic {
   void Blt_FreePrivateGC(Display *display, GC gc)
}
declare 11 generic {
   int Blt_GetWindowFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	Window *windowPtr)
}
declare 12 generic {
   Window Blt_GetParentWindow(Display *display, Window window)
}
declare 13 generic {
   Tk_Window Blt_FindChild(Tk_Window parent, char *name)
}
declare 14 generic {
   Tk_Window Blt_FirstChild(Tk_Window parent)
}
declare 15 generic {
   Tk_Window Blt_NextChild(Tk_Window tkwin)
}
declare 16 generic {
   void Blt_RelinkWindow (Tk_Window tkwin, Tk_Window newParent, int x, 
	int y)
}
declare 17 generic {
   Tk_Window Blt_Toplevel(Tk_Window tkwin)
}
declare 18 generic {
   int Blt_GetPixels(Tcl_Interp *interp, Tk_Window tkwin, 
	const char *string, int check, int *valuePtr)
}
declare 19 generic {
   const char *Blt_NameOfFill(int fill)
}
declare 20 generic {
   const char *Blt_NameOfResize(int resize)
}
declare 21 generic {
   int Blt_GetXY (Tcl_Interp *interp, Tk_Window tkwin, 
	const char *string, int *xPtr, int *yPtr)
}
declare 22 generic {
   Point2d Blt_GetProjection (int x, int y, Point2d *p, Point2d *q)
}
declare 23 generic {
   void Blt_DrawArrowOld(Display *display, Drawable drawable, GC gc, 
	int x, int y, int w, int h, int borderWidth, int orientation)
}
declare 24 generic {
   void Blt_DrawArrow (Display *display, Drawable drawable, 
	XColor *color, int x, int y, int w, int h, 
	int borderWidth, int orientation)
}
declare 25 generic {
   void Blt_MakeTransparentWindowExist (Tk_Window tkwin, Window parent, 
	int isBusy)
}
declare 26 generic {
   void Blt_TranslateAnchor (int x, int y, int width, int height, 
	Tk_Anchor anchor, int *transXPtr, int *transYPtr)
}
declare 27 generic {
   Point2d Blt_AnchorPoint (double x, double y, double width, 
	double height, Tk_Anchor anchor)
}
declare 28 generic {
   long Blt_MaxRequestSize (Display *display, size_t elemSize)
}
declare 29 generic {
   Window Blt_GetWindowId (Tk_Window tkwin)
}
declare 30 generic {
   void Blt_InitXRandrConfig(Tcl_Interp *interp)
}
declare 31 generic {
   void Blt_SizeOfScreen(Tk_Window tkwin, int *widthPtr,int *heightPtr)
}
declare 32 generic {
   int Blt_RootX (Tk_Window tkwin)
}
declare 33 generic {
   int Blt_RootY (Tk_Window tkwin)
}
declare 34 generic {
   void Blt_RootCoordinates (Tk_Window tkwin, int x, int y, 
	int *rootXPtr, int *rootYPtr)
}
declare 35 generic {
   void Blt_MapToplevelWindow(Tk_Window tkwin)
}
declare 36 generic {
   void Blt_UnmapToplevelWindow(Tk_Window tkwin)
}
declare 37 generic {
   void Blt_RaiseToplevelWindow(Tk_Window tkwin)
}
declare 38 generic {
   void Blt_LowerToplevelWindow(Tk_Window tkwin)
}
declare 39 generic {
   void Blt_ResizeToplevelWindow(Tk_Window tkwin, int w, int h)
}
declare 40 generic {
   void Blt_MoveToplevelWindow(Tk_Window tkwin, int x, int y)
}
declare 41 generic {
   void Blt_MoveResizeToplevelWindow(Tk_Window tkwin, int x, int y, 
	int w, int h)
}
declare 42 generic {
   int Blt_GetWindowRegion(Display *display, Window window, int *xPtr,
	int *yPtr, int *widthPtr, int *heightPtr)
}
declare 43 generic {
   ClientData Blt_GetWindowInstanceData (Tk_Window tkwin)
}
declare 44 generic {
   void Blt_SetWindowInstanceData (Tk_Window tkwin, 
	ClientData instanceData)
}
declare 45 generic {
   void Blt_DeleteWindowInstanceData (Tk_Window tkwin)
}
declare 46 generic {
   int Blt_ReparentWindow (Display *display, Window window, 
	Window newParent, int x, int y)
}
declare 47 generic {
   Blt_DrawableAttributes *Blt_GetDrawableAttribs(Display *display,
	Drawable drawable)
}
declare 48 generic {
   void Blt_SetDrawableAttribs(Display *display, Drawable drawable,
	int width, int height, int depth, Colormap colormap, Visual *visual)
}
declare 49 generic {
   void Blt_SetDrawableAttribsFromWindow(Tk_Window tkwin, 
	Drawable drawable)
}
declare 50 generic {
   void Blt_FreeDrawableAttribs(Display *display, Drawable drawable)
}
declare 51 generic {
   GC Blt_GetBitmapGC(Tk_Window tkwin)
}
declare 52 generic {
   Pixmap Blt_GetPixmapAbortOnError(Display *dpy, Drawable draw, 
	int w, int h, int depth, int lineNum, const char *fileName)
}
declare 53 generic {
   void Blt_ScreenDPI(Tk_Window tkwin, int *xPtr, int *yPtr)
}
declare 54 generic {
   int Blt_OldConfigModified(Tk_ConfigSpec *specs, ...)
}
declare 55 generic {
   void Blt_GetLineExtents(size_t numPoints, Point2d *points, 
	Region2d *r)
}
declare 56 generic {
   void Blt_GetBoundingBox (int width, int height, float angle, 
	double *widthPtr, double *heightPtr, Point2d *points)
}
declare 57 generic {
   Blt_Font Blt_GetFont(Tcl_Interp *interp, Tk_Window tkwin, 
	const char *string)
}
declare 58 generic {
   Blt_Font Blt_AllocFontFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Tcl_Obj *objPtr)
}
declare 59 generic {
   void Blt_DrawWithEllipsis(Tk_Window tkwin, Drawable drawable,
	GC gc, Blt_Font font, int depth, float angle, const char *string, 
	int numBytes, int x, int y, int maxLength)
}
declare 60 generic {
   Blt_Font Blt_GetFontFromObj(Tcl_Interp *interp, Tk_Window tkwin,
	Tcl_Obj *objPtr)
}
declare 61 generic {
   void Blt_Font_GetMetrics(Blt_Font font, Blt_FontMetrics *fmPtr)
}
declare 62 generic {
   int Blt_TextWidth(Blt_Font font, const char *string, int length)
}
declare 63 generic {
   Tcl_Interp *Blt_Font_GetInterp(Blt_Font font)
}
declare 64 generic {
   Tcl_Obj *Blt_Font_GetFile(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	double *sizePtr)
}
declare 65 generic {
   Blt_Bg Blt_GetBg(Tcl_Interp *interp, Tk_Window tkwin,
	const char *styleName)
}
declare 66 generic {
   Blt_Bg Blt_GetBgFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Tcl_Obj *objPtr)
}
declare 67 generic {
   XColor *Blt_Bg_BorderColor(Blt_Bg bg)
}
declare 68 generic {
   Tk_3DBorder Blt_Bg_Border(Blt_Bg bg)
}
declare 69 generic {
   const char *Blt_Bg_Name(Blt_Bg bg)
}
declare 70 generic {
   void Blt_FreeBg(Blt_Bg bg)
}
declare 71 generic {
   void Blt_Bg_DrawRectangle(Tk_Window tkwin, Drawable drawable,
	Blt_Bg bg, int x, int y, int width, int height, int borderWidth,
	int relief)
}
declare 72 generic {
   void Blt_Bg_FillRectangle(Tk_Window tkwin, Drawable drawable,
	Blt_Bg bg, int x, int y, int width, int height, int borderWidth, 
	int relief)
}
declare 73 generic {
   void Blt_Bg_DrawPolygon(Tk_Window tkwin, Drawable drawable, 
	Blt_Bg bg, XPoint *points, int numPoints, int borderWidth, 
	int leftRelief)
}
declare 74 generic {
   void Blt_Bg_FillPolygon(Tk_Window tkwin, Drawable drawable, 
	Blt_Bg bg, XPoint *points, int numPoints, int borderWidth, 
	int leftRelief)
}
declare 75 generic {
   void Blt_Bg_GetOrigin(Blt_Bg bg, int *xPtr, int *yPtr)
}
declare 76 generic {
   void Blt_Bg_SetChangedProc(Blt_Bg bg, Blt_Bg_ChangedProc *notifyProc,
	ClientData clientData)
}
declare 77 generic {
   void Blt_Bg_SetOrigin(Tk_Window tkwin, Blt_Bg bg, int x, int y)
}
declare 78 generic {
   void Blt_Bg_DrawFocus(Tk_Window tkwin, Blt_Bg bg, 
	int highlightWidth, Drawable drawable)
}
declare 79 generic {
   GC Blt_Bg_BorderGC(Tk_Window tkwin, Blt_Bg bg, int which)
}
declare 80 generic {
   void Blt_Bg_SetFromBackground(Tk_Window tkwin, Blt_Bg bg)
}
declare 81 generic {
   void Blt_Bg_UnsetClipRegion(Tk_Window tkwin, Blt_Bg bg)
}
declare 82 generic {
   void Blt_Bg_SetClipRegion(Tk_Window tkwin, Blt_Bg bg, TkRegion rgn)
}
declare 83 generic {
   void Blt_DestroyBindingTable(Blt_BindTable table)
}
declare 84 generic {
   Blt_BindTable Blt_CreateBindingTable(Tcl_Interp *interp, 
	Tk_Window tkwin, ClientData clientData, Blt_BindPickProc *pickProc,
	Blt_BindAppendTagsProc *tagProc)
}
declare 85 generic {
   int Blt_ConfigureBindings(Tcl_Interp *interp, Blt_BindTable table, 
	ClientData item, int argc, const char **argv)
}
declare 86 generic {
   int Blt_ConfigureBindingsFromObj(Tcl_Interp *interp, 
	Blt_BindTable table, ClientData item, int objc, Tcl_Obj *const *objv)
}
declare 87 generic {
   void Blt_PickCurrentItem(Blt_BindTable table)
}
declare 88 generic {
   void Blt_DeleteBindings(Blt_BindTable table, ClientData object)
}
declare 89 generic {
   void Blt_MoveBindingTable(Blt_BindTable table, Tk_Window tkwin)
}
declare 90 generic {
   void Blt_Afm_SetPrinting(Tcl_Interp *interp, int value)
}
declare 91 generic {
   int Blt_Afm_IsPrinting(void)
}
declare 92 generic {
   int Blt_Afm_TextWidth(Blt_Font font, const char *s, int numBytes)
}
declare 93 generic {
   int Blt_Afm_GetMetrics(Blt_Font font, Blt_FontMetrics *fmPtr)
}
declare 94 generic {
   const char *Blt_Afm_GetPostscriptFamily(const char *family)
}
declare 95 generic {
   void Blt_Afm_GetPostscriptName(const char *family, int flags, 
	Tcl_DString *resultPtr)
}
declare 96 generic {
   void Blt_SetDashes (Display *display, GC gc, Blt_Dashes *dashesPtr)
}
declare 97 generic {
   void Blt_ResetLimits(Blt_Limits *limitsPtr)
}
declare 98 generic {
   int Blt_GetLimitsFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Tcl_Obj *objPtr, Blt_Limits *limitsPtr)
}
declare 99 generic {
   int Blt_ConfigureInfoFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Blt_ConfigSpec *specs, char *widgRec, Tcl_Obj *objPtr, int flags)
}
declare 100 generic {
   int Blt_ConfigureValueFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Blt_ConfigSpec *specs, char *widgRec, Tcl_Obj *objPtr, int flags)
}
declare 101 generic {
   int Blt_ConfigureWidgetFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Blt_ConfigSpec *specs, int objc, Tcl_Obj *const *objv, char *widgRec, 
	int flags)
}
declare 102 generic {
   int Blt_ConfigureComponentFromObj(Tcl_Interp *interp, 
	Tk_Window tkwin, const char *name, const char *className, 
	Blt_ConfigSpec *specs, int objc, Tcl_Obj *const *objv, char *widgRec, 
	int flags)
}
declare 103 generic {
   int Blt_ConfigModified(Blt_ConfigSpec *specs, ...)
}
declare 104 generic {
   const char *Blt_NameOfState(int state)
}
declare 105 generic {
   void Blt_FreeOptions(Blt_ConfigSpec *specs, char *widgRec, 
	Display *display, int needFlags)
}
declare 106 generic {
   int Blt_ObjIsOption(Blt_ConfigSpec *specs, Tcl_Obj *objPtr, 
	int flags)
}
declare 107 generic {
   int Blt_GetPixelsFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Tcl_Obj *objPtr, int flags, int *valuePtr)
}
declare 108 generic {
   int Blt_GetPadFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Tcl_Obj *objPtr, Blt_Pad *padPtr)
}
declare 109 generic {
   int Blt_GetStateFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int *statePtr)
}
declare 110 generic {
   int Blt_GetFillFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int *fillPtr)
}
declare 111 generic {
   int Blt_GetResizeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int *fillPtr)
}
declare 112 generic {
   int Blt_GetDashesFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	Blt_Dashes *dashesPtr)
}
declare 113 generic {
   int Blt_Image_IsDeleted(Tk_Image tkImage)
}
declare 114 generic {
   Tk_ImageMaster Blt_Image_GetMaster(Tk_Image tkImage)
}
declare 115 generic {
   Tk_ImageType *Blt_Image_GetType(Tk_Image tkImage)
}
declare 116 generic {
   ClientData Blt_Image_GetInstanceData(Tk_Image tkImage)
}
declare 117 generic {
   const char *Blt_Image_Name(Tk_Image tkImage)
}
declare 118 generic {
   const char *Blt_Image_NameOfType(Tk_Image tkImage)
}
declare 119 generic {
   void Blt_FreePainter(Blt_Painter painter)
}
declare 120 generic {
   Blt_Painter Blt_GetPainter(Tk_Window tkwin, float gamma)
}
declare 121 generic {
   Blt_Painter Blt_GetPainterFromDrawable(Display *display, 
	Drawable drawable, float gamma)
}
declare 122 generic {
   GC Blt_PainterGC(Blt_Painter painter)
}
declare 123 generic {
   int Blt_PainterDepth(Blt_Painter painter)
}
declare 124 generic {
   int Blt_PaintPicture(Blt_Painter painter, Drawable drawable, 
	Blt_Picture src, int srcX, int srcY, int width, int height, 
	int destX, int destY, unsigned int flags)
}
declare 125 generic {
   int Blt_PaintPictureWithBlend(Blt_Painter painter, Drawable drawable, 
	Blt_Picture src, int srcX, int srcY, int width, int height, 
	int destX, int destY, unsigned int flags)
}
declare 126 generic {
   Blt_Picture Blt_PaintCheckbox(int width, int height, 
	XColor *fillColor, XColor *outlineColor, XColor *checkColor, int isOn)
}
declare 127 generic {
   Blt_Picture Blt_PaintRadioButton(int width, int height, Blt_Bg bg, 
	XColor *fill, XColor *outline, int isOn)
}
declare 128 generic {
   Blt_Picture Blt_PaintRadioButtonOld(int width, int height, 
	XColor *bg, XColor *fill, XColor *outline, XColor *check, int isOn)
}
declare 129 generic {
   Blt_Picture Blt_PaintDelete(int width, int height, XColor *bgColor,
	XColor *fillColor, XColor *symColor, int isActive)
}
declare 130 generic {
   Blt_Ps Blt_Ps_Create(Tcl_Interp *interp, PageSetup *setupPtr)
}
declare 131 generic {
   void Blt_Ps_Free(Blt_Ps ps)
}
declare 132 generic {
   const char *Blt_Ps_GetValue(Blt_Ps ps, int *lengthPtr)
}
declare 133 generic {
   Tcl_Interp *Blt_Ps_GetInterp(Blt_Ps ps)
}
declare 134 generic {
   Tcl_DString *Blt_Ps_GetDString(Blt_Ps ps)
}
declare 135 generic {
   char *Blt_Ps_GetScratchBuffer(Blt_Ps ps)
}
declare 136 generic {
   void Blt_Ps_SetInterp(Blt_Ps ps, Tcl_Interp *interp)
}
declare 137 generic {
   void Blt_Ps_Append(Blt_Ps ps, const char *string)
}
declare 138 generic {
   void Blt_Ps_AppendBytes(Blt_Ps ps, const char *string, int numBytes)
}
declare 139 generic {
   void Blt_Ps_VarAppend(Blt_Ps ps, ...)
}
declare 140 generic {
   void Blt_Ps_Format(Blt_Ps ps, const char *fmt, ...)
}
declare 141 generic {
   void Blt_Ps_SetClearBackground(Blt_Ps ps)
}
declare 142 generic {
   int Blt_Ps_IncludeFile(Tcl_Interp *interp, Blt_Ps ps, 
	const char *fileName)
}
declare 143 generic {
   int Blt_Ps_GetPicaFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int *picaPtr)
}
declare 144 generic {
   int Blt_Ps_GetPadFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	Blt_Pad *padPtr)
}
declare 145 generic {
   int Blt_Ps_ComputeBoundingBox(PageSetup *setupPtr, int w, int h)
}
declare 146 generic {
   void Blt_Ps_DrawPicture(Blt_Ps ps, Blt_Picture picture, 
	double x, double y)
}
declare 147 generic {
   void Blt_Ps_Rectangle(Blt_Ps ps, int x, int y, int w, int h)
}
declare 148 generic {
   int Blt_Ps_SaveFile(Tcl_Interp *interp, Blt_Ps ps, 
	const char *fileName)
}
declare 149 generic {
   void Blt_Ps_XSetLineWidth(Blt_Ps ps, int lineWidth)
}
declare 150 generic {
   void Blt_Ps_XSetBackground(Blt_Ps ps, XColor *colorPtr)
}
declare 151 generic {
   void Blt_Ps_XSetBitmapData(Blt_Ps ps, Display *display, 
	Pixmap bitmap, int width, int height)
}
declare 152 generic {
   void Blt_Ps_XSetForeground(Blt_Ps ps, XColor *colorPtr)
}
declare 153 generic {
   void Blt_Ps_XSetFont(Blt_Ps ps, Blt_Font font)
}
declare 154 generic {
   void Blt_Ps_XSetDashes(Blt_Ps ps, Blt_Dashes *dashesPtr)
}
declare 155 generic {
   void Blt_Ps_XSetLineAttributes(Blt_Ps ps, XColor *colorPtr,
	int lineWidth, Blt_Dashes *dashesPtr, int capStyle, int joinStyle)
}
declare 156 generic {
   void Blt_Ps_XSetStipple(Blt_Ps ps, Display *display, Pixmap bitmap)
}
declare 157 generic {
   void Blt_Ps_Polyline(Blt_Ps ps, int n, Point2d *points)
}
declare 158 generic {
   void Blt_Ps_XDrawLines(Blt_Ps ps, int n, XPoint *points)
}
declare 159 generic {
   void Blt_Ps_XDrawSegments(Blt_Ps ps, int n, XSegment *segments)
}
declare 160 generic {
   void Blt_Ps_DrawPolyline(Blt_Ps ps, int n, Point2d *points)
}
declare 161 generic {
   void Blt_Ps_DrawSegments2d(Blt_Ps ps, int n, Segment2d *segments)
}
declare 162 generic {
   void Blt_Ps_Draw3DRectangle(Blt_Ps ps, Tk_3DBorder border, 
	double x, double y, int width, int height, int borderWidth, int relief)
}
declare 163 generic {
   void Blt_Ps_Fill3DRectangle(Blt_Ps ps, Tk_3DBorder border, double x,
	 double y, int width, int height, int borderWidth, int relief)
}
declare 164 generic {
   void Blt_Ps_XFillRectangle(Blt_Ps ps, double x, double y, 
	int width, int height)
}
declare 165 generic {
   void Blt_Ps_XFillRectangles(Blt_Ps ps, int n, XRectangle *rects)
}
declare 166 generic {
   void Blt_Ps_XFillPolygon(Blt_Ps ps, int n, Point2d *points)
}
declare 167 generic {
   void Blt_Ps_DrawPhoto(Blt_Ps ps, Tk_PhotoHandle photoToken,
	double x, double y)
}
declare 168 generic {
   void Blt_Ps_XDrawWindow(Blt_Ps ps, Tk_Window tkwin, 
	double x, double y)
}
declare 169 generic {
   void Blt_Ps_DrawText(Blt_Ps ps, const char *string, 
	TextStyle *attrPtr, double x, double y)
}
declare 170 generic {
   void Blt_Ps_DrawBitmap(Blt_Ps ps, Display *display, Pixmap bitmap, 
	double scaleX, double scaleY)
}
declare 171 generic {
   void Blt_Ps_XSetCapStyle(Blt_Ps ps, int capStyle)
}
declare 172 generic {
   void Blt_Ps_XSetJoinStyle(Blt_Ps ps, int joinStyle)
}
declare 173 generic {
   void Blt_Ps_PolylineFromXPoints(Blt_Ps ps, int n, XPoint *points)
}
declare 174 generic {
   void Blt_Ps_Polygon(Blt_Ps ps, Point2d *points, int numPoints)
}
declare 175 generic {
   void Blt_Ps_SetPrinting(Blt_Ps ps, int value)
}
declare 176 generic {
   TextLayout *Blt_Ts_CreateLayout(const char *string, int length, 
	TextStyle *tsPtr)
}
declare 177 generic {
   TextLayout *Blt_Ts_TitleLayout(const char *string, int length, 
	TextStyle *tsPtr)
}
declare 178 generic {
   void Blt_Ts_DrawLayout(Tk_Window tkwin, Drawable drawable, 
	TextLayout *textPtr, TextStyle *tsPtr, int x, int y)
}
declare 179 generic {
   void Blt_Ts_GetExtents(TextStyle *tsPtr, const char *text, 
	unsigned int *widthPtr, unsigned int *heightPtr)
}
declare 180 generic {
   void Blt_Ts_ResetStyle(Tk_Window tkwin, TextStyle *tsPtr)
}
declare 181 generic {
   void Blt_Ts_FreeStyle(Display *display, TextStyle *tsPtr)
}
declare 182 generic {
   void Blt_Ts_SetDrawStyle (TextStyle *tsPtr, Blt_Font font, GC gc, 
	XColor *fgColor, float angle, Tk_Anchor anchor, Tk_Justify justify, 
	int leader)
}
declare 183 generic {
   void Blt_DrawText(Tk_Window tkwin, Drawable drawable, 
	const char *string, TextStyle *tsPtr, int x, int y)
}
declare 184 generic {
   void Blt_DrawText2(Tk_Window tkwin, Drawable drawable, 
	const char *string, TextStyle *tsPtr, int x, int y, Dim2d * dimPtr)
}
declare 185 generic {
   Pixmap Blt_Ts_Bitmap(Tk_Window tkwin, TextLayout *textPtr, 
	TextStyle *tsPtr, int *widthPtr, int *heightPtr)
}
declare 186 generic {
   int Blt_DrawTextWithRotatedFont(Tk_Window tkwin, Drawable drawable, 
	float angle, TextStyle *tsPtr, TextLayout *textPtr, int x, int y)
}
declare 187 generic {
   void Blt_DrawLayout(Tk_Window tkwin, Drawable drawable, GC gc, 
	Blt_Font font, int depth, float angle, int x, int y, 
	TextLayout *layoutPtr, int maxLength)
}
declare 188 generic {
   void Blt_GetTextExtents(Blt_Font font, int leader, const char *text, 
	int textLen, unsigned int *widthPtr, unsigned int *heightPtr)
}
declare 189 generic {
   void Blt_RotateStartingTextPositions(TextLayout *textPtr,
	float angle)
}
declare 190 generic {
   Tk_TextLayout Blt_ComputeTextLayout(Blt_Font font, 
	const char *string, int numChars, int wrapLength, Tk_Justify justify, 
	int flags, int *widthPtr, int *heightPtr)
}
declare 191 generic {
   void Blt_DrawTextLayout(Display *display, Drawable drawable, GC gc, 
	Tk_TextLayout layout, int x, int y, int firstChar, int lastChar)
}
declare 192 generic {
   int Blt_CharBbox(Tk_TextLayout layout, int index, int *xPtr, 
	int *yPtr, int *widthPtr, int *heightPtr)
}
declare 193 generic {
   void Blt_UnderlineTextLayout(Display *display, Drawable drawable,
	GC gc, Tk_TextLayout layout, int x, int y, int underline)
}
declare 194 generic {
   void Blt_Ts_UnderlineLayout(Tk_Window tkwin, Drawable drawable,
	TextLayout *layoutPtr, TextStyle *tsPtr, int x, int y)
}
declare 195 generic {
   void Blt_Ts_DrawText(Tk_Window tkwin, Drawable drawable, 
	const char *text, int textLen, TextStyle *tsPtr, int x, int y)
}
declare 196 generic {
   int Blt_MeasureText(Blt_Font font, const char *text, int textLen,
	int maxLength, int *nBytesPtr)
}
declare 197 generic {
   void Blt_FreeTextLayout(Tk_TextLayout layout)
}

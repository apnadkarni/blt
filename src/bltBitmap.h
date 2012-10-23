
BLT_EXTERN Pixmap Blt_PhotoImageMask(Tk_Window tkwin, Tk_PhotoImageBlock src);

BLT_EXTERN Pixmap Blt_ScaleBitmap(Tk_Window tkwin, Pixmap srcBitmap, int srcWidth, 
	int srcHeight, int destWidth, int destHeight);

BLT_EXTERN Pixmap Blt_RotateBitmap(Tk_Window tkwin, Pixmap srcBitmap, 
	int srcWidth, int srcHeight, float angle, int *destWidthPtr, 
	int *destHeightPtr);

BLT_EXTERN Pixmap Blt_ScaleRotateBitmapArea(Tk_Window tkwin, Pixmap srcBitmap,
	unsigned int srcWidth, unsigned int srcHeight, int regionX, int regionY,
	unsigned int regionWidth, unsigned int regionHeight, 
	unsigned int destWidth, unsigned int destHeight, float angle);


/*
 * bltPhoto.h --
 *
 * This module implements basic picture processing routines for the BLT
 * toolkit.
 *
 *	Copyright 2011 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

BLT_EXTERN Blt_Picture Blt_PhotoToPicture (Tk_PhotoHandle photo);
BLT_EXTERN Blt_Picture Blt_PhotoAreaToPicture (Tk_PhotoHandle photo, 
	int x, int y, int w, int h);
BLT_EXTERN void Blt_PictureToPhoto(Blt_Picture picture, Tk_PhotoHandle photo);
BLT_EXTERN int Blt_SnapPhoto(Tcl_Interp *interp, Tk_Window tkwin, 
	Drawable drawable, int sx, int sy, int w, int h, int dw, int dh, 
	const char *photoName, float gamma);

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltTkPkgsInit.c --
 *
 * This module initials the BLT toolkit, registering its commands with the
 * Tcl/Tk interpreter.
 *
 *	Copyright 1991-2004 George A Howlett.
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

#define BUILD_BLT_TK_PROCS 1
#include <bltInt.h>
/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */

BLT_EXTERN Tcl_AppInitProc Blt_TkPkgsInit;
BLT_EXTERN Tcl_AppInitProc Blt_TkInit;
BLT_EXTERN Tcl_AppInitProc Blt_TkSafeInit;

/* Picture format packages. */
BLT_EXTERN Tcl_AppInitProc Blt_PictureBmpInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureBmpSafeInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureGifInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureGifSafeInit;
#ifdef HAVE_LIBJPG
BLT_EXTERN Tcl_AppInitProc Blt_PictureJpgInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureJpgSafeInit;
#endif	/* HAVE_LIBJPG */
BLT_EXTERN Tcl_AppInitProc Blt_PicturePbmInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePbmSafeInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePdfInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePdfSafeInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePhotoInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePhotoSafeInit;
#ifdef HAVE_LIBPNG
BLT_EXTERN Tcl_AppInitProc Blt_PicturePngInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePngSafeInit;
#endif	/* HAVE_LIBPNG */
BLT_EXTERN Tcl_AppInitProc Blt_PicturePsInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePsSafeInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureTgaInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureTgaSafeInit;
#ifdef HAVE_LIBTIF
BLT_EXTERN Tcl_AppInitProc Blt_PictureTifInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureTifSafeInit;
#endif	/* HAVE_LIBTIF */
BLT_EXTERN Tcl_AppInitProc Blt_PictureXbmInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureXbmSafeInit;
#ifdef HAVE_LIBXPM
BLT_EXTERN Tcl_AppInitProc Blt_PictureXpmInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureXpmSafeInit;
#endif	/* HAVE_LIBXPM */
#ifdef HAVE_LIBFT2
BLT_EXTERN Tcl_AppInitProc Blt_PictureTextInit;
#endif	/* HAVE_LIBFT2 */

int
Blt_TkPkgsInit(Tcl_Interp *interp)	/* Interpreter for application. */
{
    if (Tcl_PkgRequire(interp, "blt_tk", BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    }
    /* Picture packages. */
    if (Blt_PictureBmpInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_bmp", Blt_PictureBmpInit, 
	Blt_PictureBmpSafeInit);

    if (Blt_PictureGifInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_gif", Blt_PictureGifInit, 
	Blt_PictureGifSafeInit);

#ifdef HAVE_LIBJPG
    if (Blt_PictureJpgInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_jpg", Blt_PictureJpgInit, 
		      Blt_PictureJpgSafeInit);
#endif /*HAVE_LIBJPG*/

    if (Blt_PicturePbmInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_pbm", Blt_PicturePbmInit, 
	Blt_PicturePbmSafeInit);

    if (Blt_PicturePhotoInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_photo", Blt_PicturePhotoInit, 
	Blt_PicturePhotoSafeInit);

#ifdef HAVE_LIBPNG
    if (Blt_PicturePngInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_png", Blt_PicturePngInit, 
	Blt_PicturePngSafeInit);
#endif /*HAVE_LIBPNG*/

    if (Blt_PicturePsInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_ps", Blt_PicturePsInit, 
	Blt_PicturePsSafeInit);

    if (Blt_PicturePdfInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_pdf", Blt_PicturePdfInit, 
	Blt_PicturePdfSafeInit);

    if (Blt_PictureTgaInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_tga", Blt_PictureTgaInit, 
	Blt_PictureTgaSafeInit);

#ifdef HAVE_LIBTIF
    if (Blt_PictureTifInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_tif", Blt_PictureTifInit, 
		      Blt_PictureTifSafeInit);
#endif /*HAVE_LIBTIF*/

    if (Blt_PictureXbmInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_xbm", Blt_PictureXbmInit, 
	Blt_PictureXbmSafeInit);

#ifdef HAVE_LIBXPM
    if (Blt_PictureXpmInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_xpm", Blt_PictureXpmInit, 
	Blt_PictureXpmSafeInit);
#endif /*HAVE_LIBXPM*/
    return TCL_OK;
}


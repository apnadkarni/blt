/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTkPkgsInit.c --
 *
 * This module initials the BLT toolkit, registering its commands with the
 * Tcl/Tk interpreter.
 *
 * Copyright 2015 George A. Howlett. All rights reserved.  
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *   1) Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2) Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the
 *      distribution.
 *   3) Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *   4) Products derived from this software may not be called "BLT" nor may
 *      "BLT" appear in their names without specific prior written
 *      permission from the author.
 *
 *   THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED
 *   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
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
BLT_EXTERN Tcl_AppInitProc Blt_PictureIcoInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureIcoSafeInit;
#ifdef HAVE_LIBJPG
BLT_EXTERN Tcl_AppInitProc Blt_PictureJpgInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureJpgSafeInit;
#endif  /* HAVE_LIBJPG */
BLT_EXTERN Tcl_AppInitProc Blt_PicturePbmInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePbmSafeInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePdfInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePdfSafeInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePhotoInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePhotoSafeInit;
#ifdef HAVE_LIBPNG
BLT_EXTERN Tcl_AppInitProc Blt_PicturePngInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePngSafeInit;
#endif  /* HAVE_LIBPNG */
BLT_EXTERN Tcl_AppInitProc Blt_PicturePsInit;
BLT_EXTERN Tcl_AppInitProc Blt_PicturePsSafeInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureTgaInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureTgaSafeInit;
#ifdef HAVE_LIBTIF
BLT_EXTERN Tcl_AppInitProc Blt_PictureTifInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureTifSafeInit;
#endif  /* HAVE_LIBTIF */
BLT_EXTERN Tcl_AppInitProc Blt_PictureXbmInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureXbmSafeInit;
#ifdef HAVE_LIBXPM
BLT_EXTERN Tcl_AppInitProc Blt_PictureXpmInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureXpmSafeInit;
#endif  /* HAVE_LIBXPM */
#ifdef HAVE_LIBFT2
BLT_EXTERN Tcl_AppInitProc Blt_PictureTextInit;
BLT_EXTERN Tcl_AppInitProc Blt_PictureTextSafeInit;
#endif  /* HAVE_LIBFT2 */

int
Blt_TkPkgsInit(Tcl_Interp *interp)      /* Interpreter for application. */
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

    if (Blt_PictureIcoInit(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_ico", Blt_PictureIcoInit, 
        Blt_PictureIcoSafeInit);

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

#ifdef HAVE_LIBFT2
    if (Blt_PictureTextInit(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_picture_text", Blt_PictureTextInit, 
        Blt_PictureTextSafeInit);
#endif /*HAVE_LIBFT2*/
    return TCL_OK;
}


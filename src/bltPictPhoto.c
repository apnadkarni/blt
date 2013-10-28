/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "bltInt.h"

#include "config.h"
#include <tcl.h>
#include <tk.h>
#include <bltSwitch.h>
#include <bltDBuffer.h>
#include "bltPicture.h"
#include "bltPictFmts.h"

#define FALSE	0
#define TRUE	1

#ifdef HAVE_MEMORY_H
#  include <memory.h>
#endif /* HAVE_MEMORY_H */

typedef struct {
    Tcl_Obj *imageObjPtr;
    int index;
} PhotoExportSwitches;

typedef struct {
    Tcl_Obj *imageObjPtr;
} PhotoImportSwitches;

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-image", "imageName", (char *)NULL,
	Blt_Offset(PhotoExportSwitches, imageObjPtr), 0},
    {BLT_SWITCH_INT_NNEG, "-index", "int", (char *)NULL,
	Blt_Offset(PhotoExportSwitches, index), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-image", "imageName", (char *)NULL,
	Blt_Offset(PhotoImportSwitches, imageObjPtr), 0},
    {BLT_SWITCH_END}
};

DLLEXPORT extern Tcl_AppInitProc Blt_PicturePhotoInit;
DLLEXPORT extern Tcl_AppInitProc Blt_PicturePhotoSafeInit;

#ifdef notdef
void
Blt_SizeOfPhoto(Tk_PhotoHandle photo, int *widthPtr, int *heightPtr)	
{
    Tk_PhotoImageBlock ib;

    Tk_PhotoGetImage(photo, &ib);
    *widthPtr = ib.width;
    *heightPtr = ib.height;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * PhotoToPicture --
 *
 *      Converts a Tk Photo image to a picture.
 *
 * Results:
 *      The picture is returned.  If an error occured, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain
PhotoToPicture(
    Tcl_Interp *interp, 	/* Interpreter to report errors back to. */
    PhotoImportSwitches *importPtr)
{
    Tk_PhotoHandle photo;
    Blt_Picture picture;
    Blt_Chain chain;

    photo = Tk_FindPhoto(interp, Tcl_GetString(importPtr->imageObjPtr));
    if (photo == NULL) {
	Tcl_AppendResult(interp, "can't find photo \"",
		Tcl_GetString(importPtr->imageObjPtr), "\"", (char *)NULL);
	return NULL;
    } 
    picture =  Blt_PhotoToPicture(photo);
    chain = Blt_Chain_Create();
    Blt_Chain_Append(chain, picture);
    return chain;
}

/*
 *---------------------------------------------------------------------------
 *
 * PictureToPhoto --
 *
 *      Writes to an existing Tk Photo to contents of the picture.
 *
 * Results:
 *      A standard TCL result.  If an error occured, TCL_ERROR is
 *	returned and an error message will be place in the interpreter
 *	result. 
 *
 * Side Effects:
 *	Memory is allocated for the photo.
 *
 *---------------------------------------------------------------------------
 */
static int
PictureToPhoto(
    Tcl_Interp *interp, 	/* Interpreter to report errors back to. */
    Blt_Picture picture,	/* Picture source. */
    PhotoExportSwitches *switchesPtr)
{
    Tk_PhotoHandle photo;

    photo = Tk_FindPhoto(interp, Tcl_GetString(switchesPtr->imageObjPtr));
    if (photo == NULL) {
	Tcl_AppendResult(interp, "can't find photo \"",
		Tcl_GetString(switchesPtr->imageObjPtr), "\"", (char *)NULL);
	return TCL_ERROR;
    } 
    Blt_PictureToPhoto(picture, photo);
    return TCL_OK;
}

static int
IsPhoto(Blt_DBuffer buffer)
{
    return FALSE;
}

static Blt_Chain
ImportPhoto(
    Tcl_Interp *interp, 
    int objc,
    Tcl_Obj *const *objv,
    const char **fileNamePtr)
{
    Blt_Chain chain;
    PhotoImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3, objv + 3, 
	&switches, BLT_SWITCH_DEFAULTS) < 0) {
	Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
	return NULL;
    }
    if (switches.imageObjPtr != NULL) {
	Tcl_AppendResult(interp, "no photo specified: use -image switch.",
		(char *)NULL);
	Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
	return NULL;
    }
    chain = PhotoToPicture(interp, &switches);
    Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
    return chain;
}

static int
ExportPhoto(Tcl_Interp *interp, unsigned int index, Blt_Chain chain, int objc, 
	    Tcl_Obj *const *objv)
{
    int result;
    PhotoExportSwitches switches;
    Blt_Picture picture;

    /* Default export switch settings. */
    memset(&switches, 0, sizeof(switches));
    switches.index = index;
    result = TCL_ERROR;
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 3, objv + 3, 
	&switches, BLT_SWITCH_DEFAULTS) < 0) {
	goto error;
    }
    if (switches.imageObjPtr != NULL) {
	Tcl_AppendResult(interp, "no photo specified: use -image switch.",
		(char *)NULL);
	goto error;
    }
    picture = Blt_GetNthPicture(chain, switches.index);
    if (picture == NULL) {
	Tcl_AppendResult(interp, "bad picture index.", (char *)NULL);
	goto error;
    }
    result = PictureToPhoto(interp, picture, &switches);
    if (result != TCL_OK) {
	Tcl_AppendResult(interp, "can't convert \"", Tcl_GetString(objv[2]), 
		"\"", (char *)NULL);
    }
 error:
    Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
    return result;
}

int 
Blt_PicturePhotoInit(Tcl_Interp *interp)
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
	return TCL_ERROR;
    };
    if (Tk_InitStubs(interp, TK_VERSION_COMPILED, PKG_ANY) == NULL) {
	return TCL_ERROR;
    };
#endif
#ifdef USE_BLT_STUBS
    if (Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    };
    if (Blt_InitTkStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    };
#endif    
    if (Tcl_PkgRequire(interp, "blt_tk", BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, "blt_picture_photo", BLT_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }
    return Blt_PictureRegisterFormat(interp,
	"photo",		/* Name of format. */
	IsPhoto,		/* Discovery routine. */
	NULL,			/* Read format procedure. */
	NULL,			/* Write format procedure. */
	ImportPhoto,		/* Import format procedure. */
	ExportPhoto);		/* Export format procedure. */
}

int 
Blt_PicturePhotoSafeInit(Tcl_Interp *interp) 
{
    return Blt_PicturePhotoInit(interp);
}


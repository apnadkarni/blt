/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltImage.c --
 *
 * This module implements image processing procedures for the BLT toolkit.
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
#include "bltInt.h"
#include "bltImage.h"
#include <X11/Xutil.h>

/*
 * Each call to Tk_GetImage returns a pointer to one of the following
 * structures, which is used as a token by clients (widgets) that display
 * images.
 */
typedef struct _TkImage {
    Tk_Window tkwin;                    /* Window passed to Tk_GetImage (needed
                                         * to "re-get" the image later if the *
                                         * manager changes). */
    Display *display;                   /* Display for tkwin.  Needed because
                                         * when the image is eventually freed
                                         * tkwin may not exist anymore. */
    struct _TkImageMaster *masterPtr;   /* Master for this image (identifiers
                                         * image manager, for example). */
    ClientData instanceData;            /* One word argument to pass to image
                                         * manager when dealing with this image
                                         * instance. */
    Tk_ImageChangedProc *changeProc;    /* Code in widget to call when image
                                         * changes in a way that affects
                                         * redisplay. */
    ClientData widgetClientData;        /* Argument to pass to changeProc. */
    struct _TkImage *nextPtr;           /* Next in list of all image instances
                                         * associated with the same name. */
} TkImage;

/*
 * For each image master there is one of the following structures, which
 * represents a name in the image table and all of the images instantiated from
 * it.  Entries in mainPtr->imageTable point to these structures.
 */
typedef struct _TkImageMaster {
    Tk_ImageType *typePtr;              /* Information about image type.  NULL
                                         * means that no image manager owns this
                                         * image: the image was deleted. */
    ClientData masterData;              /* One-word argument to pass to image
                                         * mgr when dealing with the master, as
                                         * opposed to instances. */
    int width, height;                  /* Last known dimensions for image. */
    void *tablePtr;                     /* Pointer to hash table containing
                                         * image (the imageTable field in some
                                         * TkMainInfo structure). */
    void *hPtr;                         /* Hash entry in mainPtr->imageTable for
                                         * this structure (used to delete the
                                         * hash entry). */
    void *instancePtr;                  /* Pointer to first in list of instances
                                         * derived from this name. */
    int deleted;                        /* Flag set when image is being 
                                         * deleted. */
    Tk_Window tkwin;                    /* Main window of interpreter (used to
                                         * detect when the world is falling
                                         * apart.) */
} TkImageMaster;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Image_IsDeleted --
 *
 *      Is there any other way to determine if an image has been deleted?
 *
 * Results:
 *      Returns 1 if the image has been deleted, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
/*LINTLIBRARY*/
int
Blt_Image_IsDeleted(Tk_Image tkImage)   /* Token for image. */
{
    TkImage *imagePtr = (TkImage *) tkImage;

    if (imagePtr->masterPtr == NULL) {
        return TRUE;
    }
    return (imagePtr->masterPtr->typePtr == NULL);
}

/*LINTLIBRARY*/
Tk_ImageMaster
Blt_Image_GetMaster(Tk_Image tkImage)   /* Token for image. */
{
    TkImage *imagePtr = (TkImage *)tkImage;

    return (Tk_ImageMaster)imagePtr->masterPtr;
}

/*LINTLIBRARY*/
ClientData
Blt_Image_GetInstanceData(Tk_Image tkImage) /* Token for image. */
{
    TkImage *imagePtr = (TkImage *)tkImage;

    return imagePtr->instanceData;
}

/*LINTLIBRARY*/
Tk_ImageType *
Blt_Image_GetType(Tk_Image tkImage)     /* Token for image. */
{
    TkImageMaster *masterPtr;

    masterPtr = (TkImageMaster *)Blt_Image_GetMaster(tkImage);
    return masterPtr->typePtr;
}

const char *
Blt_Image_Name(Tk_Image tkImage)
{
    Tk_ImageMaster master;

    master = Blt_Image_GetMaster(tkImage);
    return Tk_NameOfImage(master);
}

const char *
Blt_Image_NameOfType(Tk_Image tkImage)
{
    TkImageMaster *masterPtr;

    masterPtr = (TkImageMaster *)Blt_Image_GetMaster(tkImage);
    return masterPtr->typePtr->name;
}


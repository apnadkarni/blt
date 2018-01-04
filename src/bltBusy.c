/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltBusy.c --
 *
 * This module implements busy windows for the BLT toolkit.
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
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_BUSY

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltChain.h"
#include "bltHash.h"
#include "tkDisplay.h"
#include "bltBg.h"
#include "bltPicture.h"
#include "bltPainter.h"
#include "bltImage.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define BUSYDEBUG 0

#ifndef TK_REPARENTED
#define TK_REPARENTED 0
#endif

#define BUSY_THREAD_KEY "BLT Busy Data"

typedef struct {
    Display *display;                   /* Display of busy window */
    Tcl_Interp *interp;                 /* Interpreter where "busy" command
                                         * was created. It's used to key
                                         * the searches in the window
                                         * hierarchy. See the "windows"
                                         * command. */
    Tk_Window tkBusy;                   /* Busy window: Transparent window
                                         * used to block delivery of events
                                         * to windows underneath it. */
    Tk_Window tkParent;                 /* Parent window of the busy
                                         * window. It may be the reference
                                         * window (if the reference is a
                                         * toplevel) or a mutual ancestor
                                         * of the reference window */
    Tk_Window tkRef;                    /* Reference window of the busy
                                         * window.  It's is used to manage
                                         * the size and position of the
                                         * busy window. */
    int x, y;                           /* Position of the reference
                                           window */
    int width, height;                  /* Size of the reference
                                         * window. Retained to know if the
                                         * reference window has been
                                         * reconfigured to a new size. */
    int menuBar;                        /* Menu bar flag. */
    Tk_Cursor cursor;                   /* Cursor for the busy window. */
    Blt_HashEntry *hashPtr;             /* Used the delete the busy window
                                         * entry out of the global hash
                                         * table. */
    Blt_HashTable *tablePtr;
    const char *text;                   /* Text to be displayed in the
                                         * snapshot window. */
    Blt_Picture original;               /* Original snapshot of reference
                                         * window used as background of
                                         * opaque busy window. */
    Blt_Picture snapshot;               /* Snapshot of reference window
                                         * used as background of opaque
                                         * busy window. This may be a
                                         * pointer to *original* or a
                                         * resampled picture. */
    Blt_Bg bg;                          /* Default background to use if 1)
                                         * busy window is opaque and 2) a
                                         * snapshot of the reference window
                                         * can't be obtained (possibly
                                         * because the reference window was
                                         * obscurred. */
    unsigned int flags;
    int alpha;
    Blt_Picture layer;                  /* Image to be blended/layered at
                                         * the the center of the busy
                                         * window. */
    Tk_Image tkImage;
    Blt_ChainLink link;                 /* Points to current image in
                                         * sequence (chain) of images. */
    Blt_Chain chain;                    /* Chain of images in sequence. */
    int interval;
    Tcl_TimerToken timerToken;
    Blt_Pixel fadeColor;
    GC gc;
} Busy;

#define REDRAW_PENDING  (1<<0)          /* Indicates a DoWhenIdle handler
                                         * has already been queued to
                                         * redraw this window. */
#define RESIZE_PENDING  (1<<1)          /* Indicates a DoWhenIdle handler
                                         * has already been queued to
                                         * redraw this window. */
#define ACTIVE          (1<<2)          /* Indicates whether the busy
                                         * window should be displayed. This
                                         * can be different from what
                                         * Tk_IsMapped says because the a
                                         * sibling reference window may be
                                         * unmapped, forcing the busy
                                         * window to be also hidden. */
#define SNAPSHOT        (1<<3)          /* Indicates whether the busy
                                         * window is a snapshot of the
                                         * parent. */
#define IMAGE_SEQUENCE  (1<<5)          /* Indicates that the image is a
                                         * picture sequence. */

#ifdef WIN32
  #define DEF_CURSOR      "wait"
#else
  #define DEF_CURSOR      "watch"
#endif
#define DEF_BACKGROUND   STD_NORMAL_BACKGROUND
#define DEF_FADE        "black"
#define DEF_DELAY       "0"
#define DEF_OPACITY     "0.0"

static Blt_OptionParseProc ObjToOpacity;
static Blt_OptionPrintProc OpacityToObj;
static Blt_CustomOption opacityOption =
{
    ObjToOpacity, OpacityToObj, NULL, (ClientData)0
};

static Blt_OptionFreeProc FreeImageProc;
static Blt_OptionParseProc ObjToImage;
static Blt_OptionPrintProc ImageToObj;
static Blt_CustomOption pictImageOption =
{
    ObjToImage, ImageToObj, FreeImageProc, (ClientData)0
};

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BACKGROUND, Blt_Offset(Busy, bg), 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
    {BLT_CONFIG_CURSOR, "-cursor", "busyCursor", "BusyCursor",
        DEF_CURSOR, Blt_Offset(Busy, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIX32, "-color", "color", "Color",
        DEF_FADE, Blt_Offset(Busy, fadeColor), 0},
    {BLT_CONFIG_INT_NNEG, "-delay", "delay", "Delay", DEF_DELAY,
        Blt_Offset(Busy, interval), 0},
    {BLT_CONFIG_CUSTOM, "-image", "image", "Image", (char *)NULL, 
        Blt_Offset(Busy, layer), BLT_CONFIG_NULL_OK, &pictImageOption},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "opacity", DEF_OPACITY, 
        Blt_Offset(Busy, alpha), 0, &opacityOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

typedef struct {
    Blt_HashTable busyTable;            /* Hash table of busy window
                                         * structures keyed by the address
                                         * of thereference Tk window */
    Tk_Window tkMain;
    Tcl_Interp *interp;
} BusyInterpData;

static Tk_GeomRequestProc BusyGeometryProc;
static Tk_GeomLostSlaveProc BusyCustodyProc;
static Tk_GeomMgr busyMgrInfo =
{
    (char *)"busy",                     /* Name of geometry manager used by
                                         * winfo */
    BusyGeometryProc,                   /* Procedure to for new geometry
                                         * requests */
    BusyCustodyProc,                    /* Procedure when window is taken
                                         * away */
};

/* Forward declarations */
static Tcl_IdleProc DisplayProc;
static Tcl_FreeProc DestroyBusy;
static Tcl_TimerProc BusyTimerProc;
static Tk_EventProc BusyEventProc;
static Tk_EventProc RefWinEventProc;
static Tcl_ObjCmdProc BusyCmdProc;
static Tcl_InterpDeleteProc BusyInterpDeleteProc;

BLT_EXTERN Blt_Chain Blt_GetPicturesFromPictureImage(Tcl_Interp *interp,
        Tk_Image tkImage);

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *      Tells the Tk dispatcher to call the busy display routine at the
 *      next idle point.  This request is made only if the window is
 *      displayed and no other redraw request is pending.
 *
 * Results: None.
 *
 * Side effects:
 *      The window is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyRedraw(Busy *busyPtr) 
{
    if ((busyPtr->tkBusy != NULL) && 
        ((busyPtr->flags & (REDRAW_PENDING|SNAPSHOT)) == SNAPSHOT)) {
        busyPtr->flags |= REDRAW_PENDING;
        Tcl_DoWhenIdle(DisplayProc, busyPtr);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * ImageChangedProc --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ImageChangedProc(ClientData clientData, int x, int y, int w, int h,
                 int imageWidth, int imageHeight)
{
    Busy *busyPtr = clientData;

    if (busyPtr->layer != NULL) {
        Blt_FreePicture(busyPtr->layer);
        busyPtr->layer = NULL;
    }
    EventuallyRedraw(busyPtr);
    if (Blt_Image_IsDeleted(busyPtr->tkImage)) {
        Tk_FreeImage(busyPtr->tkImage);
        busyPtr->tkImage = NULL;
        return;
    }
    busyPtr->layer = Blt_GetPictureFromImage(busyPtr->interp, busyPtr->tkImage);
}

/*ARGSUSED*/
static void
FreeImageProc(ClientData clientData, Display *display, char *widgRec,
              int offset)
{
    Busy *busyPtr = (Busy *)widgRec;

    if (busyPtr->layer != NULL) {
        Blt_FreePicture(busyPtr->layer);
        busyPtr->layer = NULL;
    }
    if (busyPtr->tkImage != NULL) {
        Tk_FreeImage(busyPtr->tkImage);
    }
    busyPtr->tkImage = NULL;
    busyPtr->layer = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToImageProc --
 *
 *      Given an image name, get the Tk image associated with it.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToImage(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)   
{
    Busy *busyPtr = (Busy *)widgRec;
    Tk_Image tkImage;
    const char *name;

    name = Tcl_GetString(objPtr);
    tkImage = Tk_GetImage(interp, tkwin, name, ImageChangedProc, busyPtr);
    if (tkImage == NULL) {
        return TCL_ERROR;
    }
    if (busyPtr->tkImage != NULL) {
        Tk_FreeImage(busyPtr->tkImage);
    }
    if (busyPtr->layer != NULL) {
        Blt_FreePicture(busyPtr->layer);
    }
    busyPtr->tkImage = tkImage;
    busyPtr->layer = Blt_GetPictureFromImage(interp, tkImage);
    busyPtr->chain = Blt_GetPicturesFromPictureImage(interp, tkImage);
    if (busyPtr->chain == NULL) {
        return TCL_ERROR;
    }
    if (Blt_Chain_GetLength(busyPtr->chain) > 1) {
        busyPtr->flags |= IMAGE_SEQUENCE;
        busyPtr->link = NULL;
    }
    EventuallyRedraw(busyPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageToObj --
 *
 *      Convert the image name into a string Tcl_Obj.
 *
 * Results:
 *      The string representation of the image is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ImageToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)    
{
    Busy *busyPtr = (Busy *)(widgRec);
    
    if (busyPtr->tkImage == NULL) {
        return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(Blt_Image_Name(busyPtr->tkImage), -1);
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToOpacity --
 *
 *      Converts the string representing the percent of opacity to an
 *      alpha value 0..255.
 *
 * Results:
 *      A standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToOpacity(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
             Tcl_Obj *objPtr, char *widgRec, int offset, int flags)     
{
    int *alphaPtr = (int *)(widgRec + offset);
    double opacity;

    if (Tcl_GetDoubleFromObj(interp, objPtr, &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((opacity < 0.0) || (opacity > 100.0)) {
        Tcl_AppendResult(interp, "invalid percent opacity \"", 
                Tcl_GetString(objPtr), "\": number should be between 0 and 100",
                (char *)NULL);
        return TCL_ERROR;
    }
    opacity = (opacity / 100.0) * 255.0;
    *alphaPtr = ROUND(opacity);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OpacityToObj --
 *
 *      Convert the alpha value into a string Tcl_Obj representing a
 *      percentage.
 *
 * Results:
 *      The string representation of the opacity percentage is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
OpacityToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
             char *widgRec, int offset, int flags)  
{
    int *alphaPtr = (int *)(widgRec + offset);
    double opacity;

    opacity = (*alphaPtr / 255.0) * 100.0;
    return Tcl_NewDoubleObj(opacity);
}

static void
GetBusyWindowCoordinates(Busy *busyPtr, int *xPtr, int *yPtr)
{
    int x, y;

    x = y = 0;
    if (busyPtr->tkParent != busyPtr->tkRef) {
        Tk_Window tkwin;
        
        for (tkwin = busyPtr->tkRef; (tkwin != NULL) &&
                 (!Tk_IsTopLevel(tkwin)); tkwin = Tk_Parent(tkwin)) {
            if (tkwin == busyPtr->tkParent) {
                break;
            }
            x += Tk_X(tkwin) + Tk_Changes(tkwin)->border_width;
            y += Tk_Y(tkwin) + Tk_Changes(tkwin)->border_width;
        }
    }
    *xPtr = x;
    *yPtr = y;
}


static Blt_Picture
SnapReferenceWindow(Tk_Window tkwin, int w, int h)
{
    int rootX, rootY;
    int x, y;
    int screenWidth, screenHeight;
    int clipped;
    Blt_Picture picture;
    Tk_Window top;
    
    /* Raise the toplevel window that's associated with the reference
     * window. */
    top = Blt_Toplevel(tkwin);
    Blt_RaiseToplevelWindow(top);
    
    /* Check if any portion of the window is off-screen. */
    x = y = 0;
    Tk_GetRootCoords(tkwin, &rootX, &rootY);
    clipped = FALSE;
    if (rootX < 0) {
        w += rootX;
        x = -rootX;
        rootX = 0;
        clipped++;
    }
    if (rootY < 0) {
        h += rootY;
        y = -rootY;
        rootY = 0;
        clipped++;
    }
    Blt_SizeOfScreen(tkwin, &screenWidth, &screenHeight);
    if ((rootX > w) > screenWidth) {
        w = screenWidth - rootX;
        clipped++;
    }
    if ((rootY > h) > screenHeight) {
        h = screenHeight - rootY;
        clipped++;
    }
    /* Get a snapshot of the portion of the window that's on-screen. */
    picture = Blt_DrawableToPicture(tkwin, Tk_RootWindow(tkwin), rootX, rootY,
                w, h, 1.0);
    if (picture == NULL) {
        Blt_Warn("can't grab window (possibly obscured?)\n");
        return NULL;
    }
    if (clipped) {
        Blt_Picture copy;
        
        /* If we only have a partial snapshot, create a picture are big as
         * the reference window and copy the snapshot region into it.  */
        copy = Blt_CreatePicture(Tk_Width(tkwin), Tk_Height(tkwin));
        Blt_BlankPicture(copy, 0xFFFFFFFF);
        Blt_CopyArea(copy, picture, 0, 0, w, h, x, y);
        Blt_FreePicture(picture);
        picture = copy;
    } 
    return picture;
}

/*
 *---------------------------------------------------------------------------
 *
 * SnapBackground --
 *
 *      Displays the busy window. If the busy window is transparent, then
 *      the window is simply mapped and the cursor is updated.  If the busy
 *      window is transparent, then background is snapped.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
SnapBackground(Busy *busyPtr)
{
    Blt_Picture picture;
    
    if (busyPtr->original != NULL) {
        /* If a snapshot of this window already exists, just resize it. */
        picture = Blt_CreatePicture(busyPtr->width, busyPtr->height);
        Blt_ResamplePicture(picture, busyPtr->original, bltMitchellFilter,
                            bltMitchellFilter);
    } else {
        Blt_Pixel color;

        picture = SnapReferenceWindow(busyPtr->tkRef, busyPtr->width,
                busyPtr->height);
        if (picture == NULL) {
            return;
        }
        busyPtr->original = picture;
        if (busyPtr->alpha != 0x0) {
            Blt_Picture fg;
            int w, h;
            
            w = Blt_Picture_Width(picture);
            h = Blt_Picture_Height(picture);
            /* Fade to black. */
            fg = Blt_CreatePicture(w, h);
            color.u32 = busyPtr->fadeColor.u32;
            color.Alpha = busyPtr->alpha;
            Blt_BlankPicture(fg, color.u32);
            Blt_CompositePictures(picture, fg);
            Blt_FreePicture(fg);
        }           
    }
    if ((busyPtr->snapshot != NULL) && 
        (busyPtr->snapshot != busyPtr->original)) {
        Blt_FreePicture(busyPtr->snapshot);
    }
    busyPtr->snapshot = picture;
    if (busyPtr->timerToken != (Tcl_TimerToken)0) {
        Tcl_DeleteTimerHandler(busyPtr->timerToken);
        busyPtr->timerToken = 0;
    }
    if ((busyPtr->chain != NULL) && (busyPtr->interval > 0)) {
        busyPtr->link = NULL;
        busyPtr->timerToken = Tcl_CreateTimerHandler(busyPtr->interval, 
                BusyTimerProc, busyPtr);
    }
    EventuallyRedraw(busyPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * MapBusy --
 *
 *      Maps the busy window. If the busy window is transparent, then
 *      the window is simply mapped and the cursor is updated.  If the busy
 *      window is semi-opaque, then the current background is snapped
 *      into an image.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
MapBusy(Busy *busyPtr)
{
    busyPtr->flags |= ACTIVE;
    /* 
     * If the busy window is opaque, take a snapshot of the reference
     * window and use that as the contents of the window.
     */
    if (busyPtr->flags & SNAPSHOT) {
        SnapBackground(busyPtr);
    }
    if (busyPtr->tkBusy != NULL) {
        Tk_MapWindow(busyPtr->tkBusy);
        /* 
         * Always raise the busy window just in case new sibling windows
         * have been created in the meantime. Can't use Tk_RestackWindow
         * because it doesn't work under Win32.
         */
        XRaiseWindow(busyPtr->display, Tk_WindowId(busyPtr->tkBusy));
    }
#ifdef WIN32
    {
        POINT point;
        /*
         * In Win32 cursors aren't associated with windows.  Tk fakes this
         * by watching <Motion> events on its windows.  Tk will
         * automatically change the cursor when the pointer enters the Busy
         * window.  But Windows doesn't immediately change the cursor; it
         * waits for the cursor position to change or a system call.  We
         * need to change the cursor before the application starts
         * processing, so set the cursor position redundantly back to the
         * current position.
         */
        GetCursorPos(&point);
        SetCursorPos(point.x, point.y);
    }
#else
    XFlush(busyPtr->display);
#endif /* WIN32 */
}

/*
 *---------------------------------------------------------------------------
 *
 * UnmapBusy --
 *
 *      Unmaps the busy window and the cursor is updated..
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
UnmapBusy(Busy *busyPtr)
{
    busyPtr->flags &= ~ACTIVE;
    if (busyPtr->tkBusy != NULL) {
        Tk_UnmapWindow(busyPtr->tkBusy); 
    }
    if (busyPtr->timerToken != (Tcl_TimerToken)0) {
        Tcl_DeleteTimerHandler(busyPtr->timerToken);
        busyPtr->timerToken = 0;
    }
#ifdef WIN32
    {
        POINT point;
        /*
         * Under Win32, cursors aren't associated with windows.  Tk fakes
         * this by watching Motion events on its windows.  So Tk will
         * automatically change the cursor when the pointer enters the Busy
         * window.  But Windows doesn't immediately change the cursor: it
         * waits for the cursor position to change or a system call.  We
         * need to change the cursor before the application starts
         * processing, so set the cursor position redundantly back to the
         * current position.
         */
        GetCursorPos(&point);
        SetCursorPos(point.x, point.y);
    }
#else
    XFlush(busyPtr->display);
#endif /* WIN32 */
    if ((busyPtr->snapshot != NULL) && 
        (busyPtr->snapshot != busyPtr->original)) {
        Blt_FreePicture(busyPtr->snapshot);
    }
    if (busyPtr->original != NULL) {
        Blt_FreePicture(busyPtr->original);
    }
    busyPtr->snapshot = busyPtr->original = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * BusyEventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for events on the
 *      busy window itself.  We're only concerned with DestroyNotify events
 *      and ConfigureNotify events for opaque busy windows.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      When a busy window is destroyed, all internal structures associated
 *      with it released at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static void
BusyEventProc(
    ClientData clientData,              /* Busy window record */
    XEvent *eventPtr)                   /* Event which triggered call to
                                         * routine */
{
    Busy *busyPtr = clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedraw(busyPtr);
        }
    } else if (eventPtr->type == DestroyNotify) {
        if (busyPtr->flags & REDRAW_PENDING) {
            busyPtr->flags &= ~REDRAW_PENDING;
            Tcl_CancelIdleCall(DisplayProc, busyPtr);
        }
        if (busyPtr->tkBusy != NULL) {
            busyPtr->tkBusy = NULL;
            Tcl_EventuallyFree(busyPtr, DestroyBusy);
        }
    } else if (eventPtr->type == ConfigureNotify) {
        EventuallyRedraw(busyPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * BusyCustodyProc --
 *
 *      This procedure is invoked when the busy window has been stolen by
 *      another geometry manager.  The information and memory associated
 *      with the busy window is released. I don't know why anyone would try
 *      to pack a busy window, but this should keep everything sane, if it
 *      is.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The Busy structure is freed at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
BusyCustodyProc(ClientData clientData, Tk_Window tkwin)
{
    Busy *busyPtr = clientData;

    Tk_DeleteEventHandler(busyPtr->tkBusy, StructureNotifyMask, BusyEventProc, 
        busyPtr);
    UnmapBusy(busyPtr);
    busyPtr->tkBusy = NULL;

    /* Remove the entry to this busy window right now so that busy commands
     * can't reference this window. */
    Blt_DeleteHashEntry(busyPtr->tablePtr, busyPtr->hashPtr);
    busyPtr->hashPtr = NULL;

    if (busyPtr->tkBusy != NULL) {
        busyPtr->tkBusy = NULL;
        Tcl_EventuallyFree(busyPtr, DestroyBusy);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * BusyGeometryProc --
 *
 *      This procedure is invoked by Tk_GeometryRequest for busy windows.
 *      Busy windows never request geometry, so it's unlikely that this
 *      routine will ever be called.  The routine exists simply as a place
 *      holder for the GeomProc in the Geometry Manager structure.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
BusyGeometryProc(ClientData clientData, Tk_Window tkwin)
{
    /* Should never get here */
}

/*
 *---------------------------------------------------------------------------
 *
 * RefWinEventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for the following
 *      events on the reference window.  If the reference and parent
 *      windows are the same, only the first event is important.
 *
 *         1) ConfigureNotify  - The reference window has been resized or
 *                               moved.  Move and resize the busy window
 *                               to be the same size and position of the
 *                               reference window.
 *
 *         2) DestroyNotify    - The reference window was destroyed. Destroy
 *                               the busy window and the free resources
 *                               used.
 *
 *         3) MapNotify        - The reference window was (re)shown. Map the
 *                               busy window again.
 *
 *         4) UnmapNotify      - The reference window was hidden. Unmap the
 *                               busy window.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      When the reference window gets deleted, internal structures get
 *      cleaned up.  When it gets resized, the busy window is resized
 *      accordingly. If it's displayed, the busy window is displayed. And
 *      when it's hidden, the busy window is unmapped.
 *
 *---------------------------------------------------------------------------
 */
static void
RefWinEventProc(
    ClientData clientData,              /* Busy window record */
    XEvent *eventPtr)                   /* Event that triggered the call to
                                         * this routine */
{
    Busy *busyPtr = clientData;

    switch (eventPtr->type) {
    case ReparentNotify:
    case DestroyNotify:

        /* Remove the entry to this busy window right now so that busy
         * commands can't reference this window. */
        Blt_DeleteHashEntry(busyPtr->tablePtr, busyPtr->hashPtr);
        busyPtr->hashPtr = NULL;

        /* Arrange for the busy structure to be removed at a proper
         * time. */
        busyPtr->tkRef = NULL;
        if (busyPtr->tkBusy != NULL) {
            Tk_DestroyWindow(busyPtr->tkBusy);
        }
        break;

    case ConfigureNotify:
        EventuallyRedraw(busyPtr);
        break;

    case MapNotify:
        if ((busyPtr->tkParent != busyPtr->tkRef) &&
            (busyPtr->flags & ACTIVE)) {
            /* Need to resize busy window and possibly re-snap the
             * reference window. */
            MapBusy(busyPtr);
        }
        break;

    case UnmapNotify:
        if (busyPtr->tkParent != busyPtr->tkRef) {
            UnmapBusy(busyPtr);
        }
        break;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * BusyTimerProc --
 *
 *---------------------------------------------------------------------------
 */
static void
BusyTimerProc(ClientData clientData)
{
    Busy *busyPtr = clientData;

    if (busyPtr->timerToken != (Tcl_TimerToken)0) {
        Tcl_DeleteTimerHandler(busyPtr->timerToken);
        busyPtr->timerToken = 0;
    }
    if (busyPtr->interval > 0) {
        busyPtr->timerToken = Tcl_CreateTimerHandler(busyPtr->interval, 
                BusyTimerProc, busyPtr);
    }
    /* Get the next image. */
    if (busyPtr->link != NULL) {
        busyPtr->link = Blt_Chain_NextLink(busyPtr->link);
    }
    if (busyPtr->link == NULL) {
        busyPtr->link = Blt_Chain_FirstLink(busyPtr->chain);
    } 
    busyPtr->layer = Blt_Chain_GetValue(busyPtr->link);
    EventuallyRedraw(busyPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureBusy --
 *
 *      This procedure is called from the Tk event dispatcher. It releases
 *      X resources and memory used by the busy window and updates the
 *      internal hash table.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Memory and resources are released and the Tk event handler is
 *      removed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureBusy(Tcl_Interp *interp, Busy *busyPtr, int objc, Tcl_Obj *const *objv,
              unsigned int flags)
{
    Tk_Cursor oldCursor;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    oldCursor = busyPtr->cursor;
    if (Blt_ConfigureWidgetFromObj(interp, busyPtr->tkRef, configSpecs, 
        objc, objv, (char *)busyPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }

    if (busyPtr->cursor != oldCursor) {
        if (busyPtr->cursor == None) {
            Tk_UndefineCursor(busyPtr->tkBusy);
        } else {
            Tk_DefineCursor(busyPtr->tkBusy, busyPtr->cursor);
        }
    }
    if (busyPtr->alpha == 0) {
        busyPtr->flags &= ~SNAPSHOT;
    } else {
        busyPtr->flags |= SNAPSHOT;
    }
    gcMask = 0;
    newGC = Tk_GetGC(busyPtr->tkRef, gcMask, &gcValues);
    if (busyPtr->gc != NULL) {
        Tk_FreeGC(busyPtr->display, busyPtr->gc);
    }
    busyPtr->gc = newGC;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewBusy --
 *
 *      Creates a child transparent window that obscures its parent window
 *      thereby effectively blocking device events.  The size and position
 *      of the busy window is exactly that of the reference window.
 *
 *      We want to create sibling to the window to be blocked.  If the busy
 *      window is a child of the window to be blocked, Enter/Leave events
 *      can sneak through.  Futhermore under WIN32, messages of transparent
 *      windows are sent directly to the parent.  The only exception to
 *      this are toplevels, since we can't make a sibling.  Fortunately,
 *      toplevel windows rarely receive events that need blocking.
 *
 * Results:
 *      Returns a pointer to the new busy window structure.
 *
 * Side effects:
 *      When the busy window is eventually displayed, it will screen device
 *      events (in the area of the reference window) from reaching its
 *      parent window and its children.  User feed back can be achieved by
 *      changing the cursor.
 *
 *---------------------------------------------------------------------------
 */
static Busy *
NewBusy(Tcl_Interp *interp, Tk_Window tkRef)
{
    Busy *busyPtr;
    Tk_Window tkBusy;
    Tk_Window tkParent;
    char *name;
    const char *fmt;
    int length;

    busyPtr = Blt_AssertCalloc(1, sizeof(Busy));
    if (Tk_IsTopLevel(tkRef)) {
        fmt = "_Busy";          /* Child */
        tkParent = tkRef;
    } else {
        fmt = "%s_Busy";        /* Sibling */
        tkParent = Tk_Parent(tkRef);
    }
    {
        Tk_Window tkwin;

        for (tkwin = Blt_FirstChild(tkParent); tkwin != NULL; 
             tkwin = Blt_NextChild(tkwin)) {
            Tk_MakeWindowExist(tkwin);
        }
    }
    length = strlen(Tk_Name(tkRef));
    name = Blt_AssertMalloc(length + 6);
    Blt_FmtString(name, length + 6, fmt, Tk_Name(tkRef));
    tkBusy = Tk_CreateWindow(interp, tkParent, name, (char *)NULL);
    Blt_Free(name);

    if (tkBusy == NULL) {
        return NULL;
    }
    Tk_MakeWindowExist(tkRef);
    busyPtr->display = Tk_Display(tkRef);
    busyPtr->interp = interp;
    busyPtr->tkRef = tkRef;
    busyPtr->tkParent = tkParent;
    busyPtr->tkBusy = tkBusy;
    busyPtr->width = Tk_Width(tkRef);
    busyPtr->height = Tk_Height(tkRef);
    busyPtr->x = Tk_X(tkRef);
    busyPtr->y = Tk_Y(tkRef);
    Tk_SetClass(tkBusy, "BltBusy");
    Blt_SetWindowInstanceData(tkBusy, busyPtr);
    return busyPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * InitializeBusy --
 *
 *      Creates a child transparent window that obscures its parent window
 *      thereby effectively blocking device events.  The size and position
 *      of the busy window is exactly that of the reference window.
 *
 *      We want to create sibling to the window to be blocked.  If the busy
 *      window is a child of the window to be blocked, Enter/Leave events
 *      can sneak through.  Futhermore under WIN32, messages of transparent
 *      windows are sent directly to the parent.  The only exception to
 *      this are toplevels, since we can't make a sibling.  Fortunately,
 *      toplevel windows rarely receive events that need blocking.
 *
 * Results:
 *      Returns a pointer to the new busy window structure.
 *
 * Side effects:
 *      When the busy window is eventually displayed, it will screen device
 *      events (in the area of the reference window) from reaching its
 *      parent window and its children.  User feed back can be achieved by
 *      changing the cursor.
 *
 *---------------------------------------------------------------------------
 */
static void
InitializeBusy(Busy *busyPtr)
{
    Tk_FakeWin *winPtr;
    Window parent;
    unsigned int mask;
    int x, y;

    winPtr = (Tk_FakeWin *) busyPtr->tkRef;
    if (winPtr->flags & TK_REPARENTED) {
        /*
         * This works around a bug in the implementation of menubars for
         * non-MacIntosh window systems (Win32 and X11).  Tk doesn't reset
         * the pointers to the parent window when the menu is reparented
         * (winPtr->parentPtr points to the wrong window). We get around
         * this by determining the parent via the native API calls.
         */
#ifdef WIN32
        {
            HWND hWnd;
            RECT rect;

            hWnd = Tk_GetHWND(Tk_WindowId(busyPtr->tkRef));
            hWnd = GetParent(hWnd);
            parent = (Window) hWnd;
            if (GetWindowRect(hWnd, &rect)) {
                busyPtr->width = rect.right - rect.left;
                busyPtr->height = rect.bottom - rect.top;
            }
        }
#else
        parent = Blt_GetParentWindow(busyPtr->display, 
                Tk_WindowId(busyPtr->tkRef));
#endif
    } else {
        parent = Tk_WindowId(busyPtr->tkParent);
#ifdef WIN32
        parent = (Window)Tk_GetHWND(parent);
#endif
    }
    mask = StructureNotifyMask;
    if (busyPtr->flags & SNAPSHOT) {
        Tk_MakeWindowExist(busyPtr->tkBusy);
        mask |= ExposureMask;
    } else {
        Blt_MakeTransparentWindowExist(busyPtr->tkBusy, parent, TRUE);
    }

#if BUSYDEBUG
    Blt_Warn("menubar1: width=%d, height=%d\n", busyPtr->width, 
             busyPtr->height);
#endif
    GetBusyWindowCoordinates(busyPtr, &x, &y);
#if BUSYDEBUG
    fprintf(stderr, "InitializeBusy: busy window %s is at %d,%d %dx%d\n", 
                        Tk_PathName(busyPtr->tkBusy), busyPtr->x, busyPtr->y, 
                        busyPtr->width, busyPtr->height);
#endif
    Tk_MoveResizeWindow(busyPtr->tkBusy, x, y, busyPtr->width, busyPtr->height);

    /* Only worry if the busy window is destroyed.  */
    Tk_CreateEventHandler(busyPtr->tkBusy, mask, BusyEventProc, busyPtr);
    /*
     * Indicate that the busy window's geometry is being managed.  This
     * will also notify us if the busy window is ever packed.
     */
    Tk_ManageGeometry(busyPtr->tkBusy, &busyMgrInfo, busyPtr);
    if (busyPtr->cursor != None) {
        Tk_DefineCursor(busyPtr->tkBusy, busyPtr->cursor);
    }
    /* Track the reference window to see if it is resized or destroyed.  */
    Tk_CreateEventHandler(busyPtr->tkRef, StructureNotifyMask, RefWinEventProc,
        busyPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyBusy --
 *
 *      This procedure is called from the Tk event dispatcher. It releases
 *      X resources and memory used by the busy window and updates the
 *      internal hash table.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Memory and resources are released and the Tk event handler is
 *      removed.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyBusy(DestroyData data)           /* Busy window structure record */
{
    Busy *busyPtr = (Busy *)data;

    Blt_FreeOptions(configSpecs, (char *)busyPtr, busyPtr->display, 0);
    if (busyPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(busyPtr->tablePtr, busyPtr->hashPtr);
    }
    if (busyPtr->flags & REDRAW_PENDING) {
        busyPtr->flags &= ~REDRAW_PENDING;
        Tcl_CancelIdleCall(DisplayProc, busyPtr);
    }
    if (busyPtr->timerToken != (Tcl_TimerToken)0) {
        Tcl_DeleteTimerHandler(busyPtr->timerToken);
        busyPtr->timerToken = 0;
    }
    if (busyPtr->tkRef != NULL) {
        Tk_DeleteEventHandler(busyPtr->tkRef, StructureNotifyMask, 
                              RefWinEventProc, busyPtr);
    }
    if (busyPtr->tkBusy != NULL) {
        unsigned int mask;

        mask = StructureNotifyMask;
        if (busyPtr->flags & SNAPSHOT) {
            mask |= ExposureMask;
        }
        Tk_DeleteEventHandler(busyPtr->tkBusy, mask, BusyEventProc, busyPtr);
        Tk_ManageGeometry(busyPtr->tkBusy, NULL, busyPtr);
    }
    if ((busyPtr->snapshot != NULL) && 
        (busyPtr->snapshot != busyPtr->original)) {
        Blt_FreePicture(busyPtr->snapshot);
    }
    if (busyPtr->original != NULL) {
        Blt_FreePicture(busyPtr->original);
    }
    if (busyPtr->gc != NULL) {
        Tk_FreeGC(busyPtr->display, busyPtr->gc);
    }
    Blt_Free(busyPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetBusy --
 *
 *      Returns the busy window structure associated with the reference
 *      window, keyed by its path name.  The clientData argument is the
 *      main window of the interpreter, used to search for the reference
 *      window in its own window hierarchy.
 *
 * Results:
 *      If path name represents a reference window with a busy window, a
 *      pointer to the busy window structure is returned. Otherwise, NULL
 *      is returned and an error message is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
static int
GetBusy(
    BusyInterpData *dataPtr,            /* Interpreter-specific data. */
    Tcl_Interp *interp,                 /* Interpreter to report errors to. If
                                         * NULL, indicates not to generate error
                                         * message. */
    Tcl_Obj *objPtr,
    Busy **busyPtrPtr)                  /* Will contain address of busy window
                                         * if found. */
{
    Blt_HashEntry *hPtr;
    Tk_Window tkwin;
    const char *pathName;               /* Path name of parent window */

    pathName = Tcl_GetString(objPtr);
    tkwin = Tk_NameToWindow(dataPtr->interp, pathName, dataPtr->tkMain);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    hPtr = Blt_FindHashEntry(&dataPtr->busyTable, (const char *)tkwin);
    if (hPtr == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find busy window \"", pathName, 
                "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    *busyPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HoldBusy --
 *
 *      Creates (if necessary) and maps a busy window, thereby preventing
 *      device events from being be received by the parent window and its
 *      children.
 *
 * Results:
 *      Returns a standard TCL result. If path name represents a busy
 *      window, it is unmapped and TCL_OK is returned. Otherwise, TCL_ERROR
 *      is returned and an error message is left in interp->result.
 *
 * Side effects:
 *      The busy window is created and displayed, blocking events from the
 *      parent window and its children.
 *
 *---------------------------------------------------------------------------
 */
static int
HoldBusy(
    BusyInterpData *dataPtr,            /* Interpreter-specific data. */
    Tcl_Interp *interp,                 /* Interpreter to report errors to */
    int objc,
    Tcl_Obj *const *objv)               /* Window name and option pairs */
{
    Tk_Window tkwin;
    Blt_HashEntry *hPtr;
    Busy *busyPtr;
    int isNew;
    int result;

    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[0]), dataPtr->tkMain);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    hPtr = Blt_CreateHashEntry(&dataPtr->busyTable, (char *)tkwin, &isNew);
    if (isNew) {
        busyPtr = NewBusy(interp, tkwin);
        if (busyPtr == NULL) {
            return TCL_ERROR;
        }
        Blt_SetHashValue(hPtr, busyPtr);
        busyPtr->hashPtr = hPtr;
        busyPtr->tablePtr = &dataPtr->busyTable;
        result = ConfigureBusy(interp, busyPtr, objc - 1, objv + 1, 0);
        InitializeBusy(busyPtr);
    } else {
        busyPtr = Blt_GetHashValue(hPtr);
        result = ConfigureBusy(interp, busyPtr, objc - 1, objv + 1, 
                BLT_CONFIG_OBJV_ONLY);
    }
    if ((busyPtr->flags & ACTIVE) == 0) {
        /* 
         * Don't map the busy window unless the reference window is also
         * currently displayed.
         */
        if (Tk_IsMapped(busyPtr->tkRef)) {
            MapBusy(busyPtr);
        } else {
            UnmapBusy(busyPtr);
        }
    }
    return result;
}


/*
 *---------------------------------------------------------------------------
 *
 * BusyInterpDeleteProc --
 *
 *      This is called when the interpreter hosting the "busy" command is
 *      destroyed.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Destroys all the hash table managing the busy windows.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
BusyInterpDeleteProc(ClientData clientData, Tcl_Interp *interp)
{
    BusyInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&dataPtr->busyTable, &iter);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        Busy *busyPtr;

        busyPtr = Blt_GetHashValue(hPtr);
        busyPtr->hashPtr = NULL;
        DestroyBusy((DestroyData)busyPtr);
    }
    Blt_DeleteHashTable(&dataPtr->busyTable);
    Tcl_DeleteAssocData(interp, BUSY_THREAD_KEY);
    Blt_Free(dataPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ActiveOp --
 *
 *      Reports the names of all widgets with busy windows attached to them,
 *      (matching a given pattern) and are active.  If no pattern is given,
 *      all busy widgets are listed.
 *
 * Results:
 *      Returns a TCL list of the names of the widget with busy windows
 *      attached to them, regardless if the widget is currently busy or not.
 *
 *---------------------------------------------------------------------------
 */
static int
ActiveOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    BusyInterpData *dataPtr = clientData;
    Tcl_Obj *listObjPtr;
    const char *pattern;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    pattern = (objc > 2) ? Tcl_GetString(objv[2]) : NULL;
    for (hPtr = Blt_FirstHashEntry(&dataPtr->busyTable, &iter);
        hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        Busy *busyPtr;

        busyPtr = Blt_GetHashValue(hPtr);
        if ((busyPtr->flags & ACTIVE) == 0) {
            continue;
        }
        if ((pattern == NULL) ||
            (Tcl_StringMatch(Tk_PathName(busyPtr->tkRef), pattern))) {
            Tcl_Obj *objPtr;
            objPtr = Tcl_NewStringObj(Tk_PathName(busyPtr->tkRef), -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED*/
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    BusyInterpData *dataPtr = clientData;
    Busy *busyPtr;

    if (GetBusy(dataPtr, interp, objv[2], &busyPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return Blt_ConfigureValueFromObj(interp, busyPtr->tkRef, configSpecs,
        (char *)busyPtr, objv[3], 0);
}
/*
 *---------------------------------------------------------------------------
 *
 * CheckOp --
 *
 *      Checks if the named window is currently busy.  This also includes
 *      windows whose ancestors are currently busy.
 *
 * Results:
 *      Returns 1 a TCL list of the names of the widget with busy windows
 *      attached to them, regardless if the widget is currently busy or not.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CheckOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    BusyInterpData *dataPtr = clientData;
    Tk_Window tkwin;
    const char *pathName;

    pathName = Tcl_GetString(objv[2]);
    tkwin = Tk_NameToWindow(interp, pathName, dataPtr->tkMain);
    do {
        Blt_HashEntry *hPtr;

        hPtr = Blt_FindHashEntry(&dataPtr->busyTable, (const char *)tkwin);
        if (hPtr != NULL) {
            Busy *busyPtr;

            /* Found a busy window, is it on? */
            busyPtr = Blt_GetHashValue(hPtr);
            if (busyPtr->flags & ACTIVE) {
                Tcl_SetBooleanObj(Tcl_GetObjResult(interp), 1);
                return TCL_OK;
            }
        }
        tkwin = Tk_Parent(tkwin);
    } while(tkwin != NULL);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *      This procedure is called to process an objv/objc list in order to
 *      configure (or reconfigure) a busy window.
 *
 * Results:
 *      The return value is a standard TCL result.  If TCL_ERROR is
 *      returned, then interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information get set for busyPtr; old resources get
 *      freed, if there were any.  The busy window destroyed and recreated
 *      in a new parent window.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    BusyInterpData *dataPtr = clientData;
    Busy *busyPtr;
    int result;

    if (GetBusy(dataPtr, interp, objv[2], &busyPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 3) {
        result = Blt_ConfigureInfoFromObj(interp, busyPtr->tkRef, configSpecs,
            (char *)busyPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
        result = Blt_ConfigureInfoFromObj(interp, busyPtr->tkRef, configSpecs,
            (char *)busyPtr, objv[3], 0);
    } else {
        Tcl_Preserve(busyPtr);
        result = ConfigureBusy(interp, busyPtr, objc - 3, objv + 3,
                BLT_CONFIG_OBJV_ONLY);
        Tcl_Release(busyPtr);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ForgetOp --
 *
 *      Destroys the busy window associated with the reference window and
 *      arranges for internal resources to the released when they're not
 *      being used anymore.
 *
 * Results:
 *      Returns a standard TCL result. If path name represents a busy
 *      window, it is destroyed and TCL_OK is returned. Otherwise,
 *      TCL_ERROR is returned and an error message is left in
 *      interp->result.
 *
 * Side effects:
 *      The busy window is removed.  Other related memory and resources are
 *      eventually released by the Tk dispatcher.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ForgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    BusyInterpData *dataPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
        Busy *busyPtr;

        if (GetBusy(dataPtr, (Tcl_Interp *)NULL, objv[i], &busyPtr) == TCL_OK) {
            /* Unmap the window even though it will be soon destroyed */
            UnmapBusy(busyPtr);

            /* Remove the entry to this busy window right now so that busy
             * commands can't reference this window. */
            Blt_DeleteHashEntry(busyPtr->tablePtr, busyPtr->hashPtr);
            busyPtr->hashPtr = NULL;

            /* Destroy the busy window. This in turn frees the busy
             * structure allocated. */
            if (busyPtr->tkBusy != NULL) {
                Tk_DestroyWindow(busyPtr->tkBusy);
            }
        }
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * HoldOp --
 *
 *      Creates (if necessary) and maps a busy window, thereby preventing
 *      device events from being be received by the parent window and its
 *      children. The argument vector may contain option-value pairs of
 *      configuration options to be set.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 * Side effects:
 *      The busy window is created and displayed, blocking events from the
 *      parent window and its children.
 *
 *---------------------------------------------------------------------------
 */
static int
HoldOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    BusyInterpData *dataPtr = clientData;
    int i;
    const char *string;

    string = Tcl_GetString(objv[1]);
    if ((string[0] == 'h') && (strcmp(string, "hold") == 0)) {
        objc--, objv++;                 /* Command used "hold" keyword */
    }
    for (i = 1; i < objc; i++) {
        int count;

        /* Find the end of the option-value pairs for this window. */
        for (count = i + 1; count < objc; count += 2) {
            string = Tcl_GetString(objv[count]);
            if (string[0] != '-') {
                break;
            }
        }
        if (count > objc) {
            count = objc;
        }
        if (HoldBusy(dataPtr, interp, count - i, objv + i) != TCL_OK) {
            return TCL_ERROR;
        }
        i = count;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsBusyOp --
 *
 *      Returns the status of the busy window; whether it's blocking events
 *      or not.
 *
 * Results:
 *      Returns a standard TCL result. If path name represents a busy
 *      window, the status is returned via interp->result and TCL_OK is
 *      returned. Otherwise, TCL_ERROR is returned and an error message is
 *      left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsBusyOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    BusyInterpData *dataPtr = clientData;
    Busy *busyPtr;
    int state;

    state = FALSE;
    if (GetBusy(dataPtr, NULL, objv[2], &busyPtr) == TCL_OK) {
        state = (busyPtr->flags & ACTIVE);
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *      Reports the names of all widgets with busy windows attached to
 *      them, matching a given pattern.  If no pattern is given, all busy
 *      widgets are listed.
 *
 * Results:
 *      Returns a TCL list of the names of the widget with busy windows
 *      attached to them, regardless if the widget is currently busy or
 *      not.
 *
 *
 *      blt::busy names ?pattern ... ?
 *---------------------------------------------------------------------------
 */
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    BusyInterpData *dataPtr = clientData;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&dataPtr->busyTable, &iter);
        hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        Busy *busyPtr;
        int match;
        const char *name;
        int i;

        busyPtr = Blt_GetHashValue(hPtr);
        name = Tk_PathName(busyPtr->tkRef);
        match = (objc == 2);
        for (i = 3; i < objc; i++) {
            char *pattern;

            pattern = Tcl_GetString(objv[i]);
            if (Tcl_StringMatch(name, pattern)) {
                match = TRUE;
                break;
            }
        }
        if (match) {
            Tcl_Obj *objPtr;

            objPtr = Tcl_NewStringObj(name, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ReleaseOp --
 *
 *      Unmaps the busy window, thereby permitting device events to be
 *      received by the parent window and its children.
 *
 * Results:
 *      Returns a standard TCL result. If path name represents a busy
 *      window, it is unmapped and TCL_OK is returned. Otherwise, TCL_ERROR
 *      is returned and an error message is left in interp->result.
 *
 * Side effects:
 *      The busy window is hidden, allowing the parent window and its
 *      children to receive events again.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ReleaseOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    BusyInterpData *dataPtr = clientData;
    Busy *busyPtr;
    int i;

    for (i = 2; i < objc; i++) {
        if (GetBusy(dataPtr, (Tcl_Interp *)NULL, objv[i], &busyPtr) == TCL_OK) {
            UnmapBusy(busyPtr);
        }
    }
    return TCL_OK;
}
/*
 *---------------------------------------------------------------------------
 *
 * StatusOp --
 *
 *      Returns the status of the busy window; whether it's blocking events
 *      or not.
 *
 * Results:
 *      Returns a standard TCL result. If path name represents a busy
 *      window, the status is returned via interp->result and TCL_OK is
 *      returned. Otherwise, TCL_ERROR is returned and an error message is
 *      left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StatusOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    BusyInterpData *dataPtr = clientData;
    Busy *busyPtr;
    const char *string;

    if (GetBusy(dataPtr, NULL, objv[2], &busyPtr) != TCL_OK) {
        string = "???";
    } else if (busyPtr->flags & ACTIVE) {
        string = "active";
    } else {
        string = "inactive";
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), string, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Busy Sub-command specification:
 *
 *      - Name of the sub-command.
 *      - Minimum number of characters needed to unambiguously
 *        recognize the sub-command.
 *      - Pointer to the function to be called for the sub-command.
 *      - Minimum number of arguments accepted.
 *      - Maximum number of arguments accepted.
 *      - String to be displayed for usage (arguments only).
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec busyOps[] =
{
    {"active",    1, ActiveOp,    2, 3, "?pattern?",},
    {"cget",      2, CgetOp,      4, 4, "windowName option",},
    {"check",     2, CheckOp,     3, 3, "windowName",},
    {"configure", 2, ConfigureOp, 3, 0, "windowName ?options?...",},
    {"forget",    1, ForgetOp,    2, 0, "?windowName?...",},
    {"hold",      1, HoldOp,      3, 0, "windowName ?option value ...? ?windowName options value ...?",},
    {"isbusy",    1, IsBusyOp,    3, 3, "windowName",},
    {"names",     1, NamesOp,     2, 0, "?pattern ... ?",},
    {"release",   1, ReleaseOp,   2, 0, "?windowName?...",},
    {"status",    1, StatusOp,    3, 3, "windowName",},
    {"windows",   1, NamesOp,     2, 3, "?pattern?",},
};
static int numBusyOps = sizeof(busyOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * BusyCmdProc --
 *
 *      This procedure is invoked to process the "busy" TCL command.  See
 *      the user documentation for details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static int
BusyCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;
    
    if (objc > 1) {
        const char *string;

        string = Tcl_GetString(objv[1]);
        if (string[0] == '.') {
            return HoldOp(clientData, interp, objc, objv);
        }
    }
    proc = Blt_GetOpFromObj(interp, numBusyOps, busyOps, BLT_OP_ARG1, objc, 
                            objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}

static BusyInterpData *
GetBusyInterpData(Tcl_Interp *interp)
{
    BusyInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (BusyInterpData *)
        Tcl_GetAssocData(interp, BUSY_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
        dataPtr = Blt_AssertMalloc(sizeof(BusyInterpData));
        Tcl_SetAssocData(interp, BUSY_THREAD_KEY, BusyInterpDeleteProc, 
                dataPtr);
        Blt_InitHashTable(&dataPtr->busyTable, BLT_ONE_WORD_KEYS);
        dataPtr->interp = interp;
        dataPtr->tkMain = Tk_MainWindow(interp);
    }
    return dataPtr;
}

int
Blt_BusyCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {"busy", BusyCmdProc, };

    cmdSpec.clientData = GetBusyInterpData(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}


static void
DisplayProc(ClientData clientData)
{
    Busy *busyPtr = clientData;
    Pixmap drawable;
    Tk_Window tkwin;
    Blt_Painter painter;
        
    busyPtr->flags &= ~REDRAW_PENDING;
    if (busyPtr->tkBusy == NULL) {
        return;                         /* Window has been destroyed (we
                                         * should not get here) */
    }
    tkwin = busyPtr->tkBusy;
#ifdef notdef
    fprintf(stderr, "Calling DisplayProc(%s)\n", Tk_PathName(tkwin));
#endif
    if ((Tk_Width(tkwin) <= 1) || (Tk_Height(tkwin) <= 1)) {
        /* Don't bother computing the layout until the size of the window
         * is something reasonable. */
        return;
    }
    busyPtr->width = Tk_Width(tkwin);
    busyPtr->height = Tk_Height(tkwin);

    if (!Tk_IsMapped(tkwin)) {
        /* The busy window isn't displayed, so don't bother drawing
         * anything.  */
        return;
    }

    if ((busyPtr->width != Tk_Width(busyPtr->tkRef)) ||
        (busyPtr->height != Tk_Height(busyPtr->tkRef)) ||
        (busyPtr->x != Tk_X(busyPtr->tkRef)) ||
        (busyPtr->y != Tk_Y(busyPtr->tkRef))) {
        int x, y;
    
        GetBusyWindowCoordinates(busyPtr, &x, &y);
        busyPtr->width = Tk_Width(busyPtr->tkRef);
        busyPtr->height = Tk_Height(busyPtr->tkRef);
        busyPtr->x = Tk_X(busyPtr->tkRef);
        busyPtr->y = Tk_Y(busyPtr->tkRef);

        Tk_MoveResizeWindow(busyPtr->tkBusy, x, y, busyPtr->width, 
                            busyPtr->height);
        if (busyPtr->flags & ACTIVE) {
            /* Need to resize busy window and possibly re-snap the
             * reference window. */
            MapBusy(busyPtr);
        }
    }
    /* Create a pixmap the size of the window for double buffering. */
    drawable = Blt_GetPixmap(busyPtr->display, Tk_WindowId(tkwin), 
        busyPtr->width, busyPtr->height, Tk_Depth(tkwin));
#ifdef WIN32
    assert(drawable != None);
#endif
    painter = Blt_GetPainter(busyPtr->tkBusy, 1.0);
    if (busyPtr->snapshot == NULL) {
        Blt_Bg_FillRectangle(busyPtr->tkBusy, drawable, busyPtr->bg, 
                busyPtr->x, busyPtr->y, busyPtr->width, busyPtr->height, 
                0, TK_RELIEF_FLAT);
        if (busyPtr->layer != NULL) {
            int x, y, w, h;

            w = Blt_Picture_Width(busyPtr->layer);
            h = Blt_Picture_Height(busyPtr->layer);
            x = (busyPtr->width - w) / 2;
            y = (busyPtr->height - h) / 2;
            assert(x >= 0 && y >= 0);
            Blt_PaintPicture(painter, drawable, busyPtr->layer, 0, 0,
                busyPtr->width, busyPtr->height, x, y, 0);
        }
    } else {
        Blt_Picture copy;
            
        copy = busyPtr->snapshot;
        if (busyPtr->layer != NULL) {
            int x, y, w, h;

            w = Blt_Picture_Width(busyPtr->layer);
            h = Blt_Picture_Height(busyPtr->layer);
            x = (busyPtr->width - w) / 2;
            y = (busyPtr->height - h) / 2;
            assert(x >= 0 && y >= 0);
            copy = Blt_ClonePicture(busyPtr->snapshot);
            Blt_CompositeArea(copy, busyPtr->layer, 0, 0, w, h, x, y);
        }
        Blt_PaintPicture(painter, drawable, copy, 0, 0, busyPtr->width, 
                busyPtr->height, 0, 0, 0);
        if (copy != busyPtr->snapshot) {
            Blt_FreePicture(copy);
        }
    }
    XCopyArea(busyPtr->display, drawable, Tk_WindowId(tkwin), busyPtr->gc, 
              0, 0, busyPtr->width, busyPtr->height, 0, 0);
    Tk_FreePixmap(busyPtr->display, drawable);
}

#endif /* NO_BUSY */


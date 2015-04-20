/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictCanv.c --
 *
 * This module implements a Canvas interface for picture images.
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

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltPicture.h"

#define REDRAW_PENDING		1
#define REDRAW_BORDERS		2
#define REPICK_NEEDED		4
#define GOT_FOCUS		8
#define CURSOR_ON		0x10
#define UPDATE_SCROLLBARS	0x20
#define LEFT_GRABBED_ITEM	0x40
#define REPICK_IN_PROGRESS	0x100
#define BBOX_NOT_EMPTY		0x200

/*
 * Flag bits for canvas items (redraw_flags):
 *
 * FORCE_REDRAW -		1 means that the new coordinates of some item
 *				are not yet registered using
 *				Tk_CanvasEventuallyRedraw(). It should still
 *				be done by the general canvas code.
 */

#define FORCE_REDRAW		8

typedef struct _TagSearchExpr TagSearchExpr;

typedef struct TkCanvas {
    Tk_Window tkwin;		/* Window that embodies the canvas. NULL means
				 * that the window has been destroyed but the
				 * data structures haven't yet been cleaned
				 * up.*/
    Display *display;		/* Display containing widget; needed, among
				 * other things, to release resources after
				 * tkwin has already gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with canvas. */
    Tcl_Command widgetCmd;	/* Token for canvas's widget command. */
    Tk_Item *firstItemPtr;	/* First in list of all items in canvas, or
				 * NULL if canvas empty. */
    Tk_Item *lastItemPtr;	/* Last in list of all items in canvas, or
				 * NULL if canvas empty. */

    /*
     * Information used when displaying widget:
     */

    int borderWidth;		/* Width of 3-D border around window. */
    Tk_3DBorder bgBorder;	/* Used for canvas background. */
    int relief;			/* Indicates whether window as a whole is
				 * raised, sunken, or flat. */
    int highlightWidth;		/* Width in pixels of highlight to draw around
				 * widget when it has the focus. <= 0 means
				 * don't draw a highlight. */
    XColor *highlightBgColorPtr;
				/* Color for drawing traversal highlight area
				 * when highlight is off. */
    XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
    int inset;			/* Total width of all borders, including
				 * traversal highlight and 3-D border.
				 * Indicates how much interior stuff must be
				 * offset from outside edges to leave room for
				 * borders. */
    GC pixmapGC;		/* Used to copy bits from a pixmap to the
				 * screen and also to clear the pixmap. */
    int width, height;		/* Dimensions to request for canvas window,
				 * specified in pixels. */
    int redrawX1, redrawY1;	/* Upper left corner of area to redraw, in
				 * pixel coordinates. Border pixels are
				 * included. Only valid if REDRAW_PENDING flag
				 * is set. */
    int redrawX2, redrawY2;	/* Lower right corner of area to redraw, in
				 * integer canvas coordinates. Border pixels
				 * will *not* be redrawn. */
    int confine;		/* Non-zero means constrain view to keep as
				 * much of canvas visible as possible. */

    /*
     * Information used to manage the selection and insertion cursor:
     */

    Tk_CanvasTextInfo textInfo; /* Contains lots of fields; see tk.h for
				 * details. This structure is shared with the
				 * code that implements individual items. */
    int insertOnTime;		/* Number of milliseconds cursor should spend
				 * in "on" state for each blink. */
    int insertOffTime;		/* Number of milliseconds cursor should spend
				 * in "off" state for each blink. */
    Tcl_TimerToken insertBlinkHandler;
				/* Timer handler used to blink cursor on and
				 * off. */

    /*
     * Transformation applied to canvas as a whole: to compute screen
     * coordinates (X,Y) from canvas coordinates (x,y), do the following:
     *
     * X = x - xOrigin;
     * Y = y - yOrigin;
     */

    int xOrigin, yOrigin;	/* Canvas coordinates corresponding to
				 * upper-left corner of window, given in
				 * canvas pixel units. */
    int drawableXOrigin, drawableYOrigin;
				/* During redisplay, these fields give the
				 * canvas coordinates corresponding to the
				 * upper-left corner of the drawable where
				 * items are actually being drawn (typically a
				 * pixmap smaller than the whole window). */

    /*
     * Information used for event bindings associated with items.
     */

    Tk_BindingTable bindingTable;
				/* Table of all bindings currently defined for
				 * this canvas. NULL means that no bindings
				 * exist, so the table hasn't been created.
				 * Each "object" used for this table is either
				 * a Tk_Uid for a tag or the address of an
				 * item named by id. */
    Tk_Item *currentItemPtr;	/* The item currently containing the mouse
				 * pointer, or NULL if none. */
    Tk_Item *newCurrentPtr;	/* The item that is about to become the
				 * current one, or NULL. This field is used to
				 * detect deletions of the new current item
				 * pointer that occur during Leave processing
				 * of the previous current item. */
    double closeEnough;		/* The mouse is assumed to be inside an item
				 * if it is this close to it. */
    XEvent pickEvent;		/* The event upon which the current choice of
				 * currentItem is based. Must be saved so that
				 * if the currentItem is deleted, can pick
				 * another. */
    int state;			/* Last known modifier state. Used to defer
				 * picking a new current object while buttons
				 * are down. */

    /*
     * Information used for managing scrollbars:
     */

    char *xScrollCmd;		/* Command prefix for communicating with
				 * horizontal scrollbar. NULL means no
				 * horizontal scrollbar. Malloc'ed. */
    char *yScrollCmd;		/* Command prefix for communicating with
				 * vertical scrollbar. NULL means no vertical
				 * scrollbar. Malloc'ed. */
    int scrollX1, scrollY1, scrollX2, scrollY2;
				/* These four coordinates define the region
				 * that is the 100% area for scrolling (i.e.
				 * these numbers determine the size and
				 * location of the sliders on scrollbars).
				 * Units are pixels in canvas coords. */
    char *regionString;		/* The option string from which scrollX1 etc.
				 * are derived. Malloc'ed. */
    int xScrollIncrement;	/* If >0, defines a grid for horizontal
				 * scrolling. This is the size of the "unit",
				 * and the left edge of the screen will always
				 * lie on an even unit boundary. */
    int yScrollIncrement;	/* If >0, defines a grid for horizontal
				 * scrolling. This is the size of the "unit",
				 * and the left edge of the screen will always
				 * lie on an even unit boundary. */

    /*
     * Information used for scanning:
     */

    int scanX;			/* X-position at which scan started (e.g.
				 * button was pressed here). */
    int scanXOrigin;		/* Value of xOrigin field when scan started. */
    int scanY;			/* Y-position at which scan started (e.g.
				 * button was pressed here). */
    int scanYOrigin;		/* Value of yOrigin field when scan started. */

    /*
     * Information used to speed up searches by remembering the last item
     * created or found with an item id search.
     */

    Tk_Item *hotPtr;		/* Pointer to "hot" item (one that's been
				 * recently used. NULL means there's no hot
				 * item. */
    Tk_Item *hotPrevPtr;	/* Pointer to predecessor to hotPtr (NULL
				 * means item is first in list). This is only
				 * a hint and may not really be hotPtr's
				 * predecessor. */

    /*
     * Miscellaneous information:
     */

    Tk_Cursor cursor;		/* Current cursor for window, or None. */
    char *takeFocus;		/* Value of -takefocus option; not used in the
				 * C code, but used by keyboard traversal
				 * scripts. Malloc'ed, but may be NULL. */
    double pixelsPerMM;		/* Scale factor between MM and pixels; used
				 * when converting coordinates. */
    int flags;			/* Various flags; see below for
				 * definitions. */
    int nextId;			/* Number to use as id for next item created
				 * in widget. */
    Tk_PostscriptInfo psInfo;	/* Pointer to information used for generating
				 * Postscript for the canvas. NULL means no
				 * Postscript is currently being generated. */
    Tcl_HashTable idTable;	/* Table of integer indices. */

    /*
     * Additional information, added by the 'dash'-patch
     */

    void *reserved1;
    Tk_State canvas_state;	/* State of canvas. */
    void *reserved2;
    void *reserved3;
    Tk_TSOffset tsoffset;
#ifndef USE_OLD_TAG_SEARCH
    TagSearchExpr *bindTagExprs;/* Linked list of tag expressions used in
				 * bindings. */
#endif
} TkCanvas;

/*
 *-----------------------------------------------------------------------------
 *
 * Blt_CanvasToPicture --
 *
 *	Returns a picture of the named canvas. 	Draws each canvas item into
 *	a pixmap and then then converts it into a picture.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *-----------------------------------------------------------------------------
 */
Blt_Picture
Blt_CanvasToPicture(Tcl_Interp *interp, const char *pathName, float gamma)
{
    TkCanvas *canvasPtr;
    Tk_Window tkwin;
    Tk_Item *itemPtr;
    Pixmap pixmap;
    int screenX1, screenX2, screenY1, screenY2, width, height;
    int w, h;
    Blt_Picture picture;
    Tk_Uid classUid;

    tkwin = Tk_NameToWindow(interp, pathName, Tk_MainWindow(interp));
    if (tkwin == NULL) {
	return NULL;
    }
    classUid = Tk_Class(tkwin);
    if (strcmp(classUid, "Canvas") == 0) {
    } else {
	Tcl_AppendResult(interp, "can't grab window of class \"", classUid, 
		"\"", (char *)NULL);
	return NULL;
    }
    if (Tk_WindowId(tkwin) == None) {
	Tk_MakeWindowExist(tkwin);
    }
    canvasPtr = Blt_GetWindowInstanceData(tkwin);
    assert(canvasPtr->tkwin == tkwin);
    if (canvasPtr->tkwin == NULL) {
	Tcl_AppendResult(interp, "can't snap canvas: window was destroyed.",
		(char *)NULL);
	return NULL;
    }
    /*
     * Choose a new current item if that is needed (this could cause event
     * handlers to be invoked).
     */

    w = Tk_Width(canvasPtr->tkwin);
    if (w < 2) {
	w = Tk_ReqWidth(canvasPtr->tkwin);
    }
    h = Tk_Height(canvasPtr->tkwin);
    if (h < 2) {
	h = Tk_ReqHeight(canvasPtr->tkwin);
    }

    /*
     * Compute the intersection between the area that needs redrawing and the
     * area that's visible on the screen.
     */

    /* Always redraw the entire canvas. */
    if ((canvasPtr->redrawX1 < canvasPtr->redrawX2)
	    && (canvasPtr->redrawY1 < canvasPtr->redrawY2)) {
    }
    screenX1 = canvasPtr->xOrigin + canvasPtr->inset;
    screenY1 = canvasPtr->yOrigin + canvasPtr->inset;
    screenX2 = canvasPtr->xOrigin + w - canvasPtr->inset;
    screenY2 = canvasPtr->yOrigin + h - canvasPtr->inset;
    
    width = screenX2 - screenX1;
    height = screenY2 - screenY1;
    
    /*
     * Redrawing is done in a temporary pixmap that is allocated here and
     * freed at the end of the function. All drawing is done to the
     * pixmap, and the pixmap is copied to the screen at the end of the
     * function. The temporary pixmap serves two purposes:
     *
     * 1. It provides a smoother visual effect (no clearing and gradual
     *    redraw will be visible to users).
     * 2. It allows us to redraw only the objects that overlap the redraw
     *    area. Otherwise incorrect results could occur from redrawing
     *    things that stick outside of the redraw area (we'd have to
     *    redraw everything in order to make the overlaps look right).
     *
     * Some tricky points about the pixmap:
     *
     * 1. We only allocate a large enough pixmap to hold the area that has
     *    to be redisplayed. This saves time in in the X server for large
     *    objects that cover much more than the area being redisplayed:
     *    only the area of the pixmap will actually have to be redrawn.
     * 2. Some X servers (e.g. the one for DECstations) have troubles with
     *    with characters that overlap an edge of the pixmap (on the DEC
     *    servers, as of 8/18/92, such characters are drawn one pixel too
     *    far to the right). To handle this problem, make the pixmap a bit
     *    larger than is absolutely needed so that for normal-sized fonts
     *    the characters that overlap the edge of the pixmap will be
     *    outside the area we care about.
     */
    
    canvasPtr->drawableXOrigin = screenX1;
    canvasPtr->drawableYOrigin = screenY1;
    pixmap = Blt_GetPixmap(Tk_Display(tkwin), Tk_WindowId(tkwin),
			  width, height, Tk_Depth(tkwin));
    /*
     * Clear the area to be redrawn.
     */
    XFillRectangle(Tk_Display(tkwin), pixmap, canvasPtr->pixmapGC,
		   0, 0, (unsigned int) width, (unsigned int) height);

    /*
     * Scan through the item list, redrawing those items that need it. An
     * item must be redraw if either (a) it intersects the smaller
     * on-screen area or (b) it intersects the full canvas area and its
     * type requests that it be redrawn always (e.g. so subwindows can be
     * unmapped when they move off-screen).
     */
    
    for (itemPtr = canvasPtr->firstItemPtr; itemPtr != NULL;
	 itemPtr = itemPtr->nextPtr) {
	if (itemPtr->state == TK_STATE_HIDDEN ||
	    (itemPtr->state == TK_STATE_NULL &&
	     canvasPtr->canvas_state == TK_STATE_HIDDEN)) {
	    continue;
	}
	(*itemPtr->typePtr->displayProc)((Tk_Canvas) canvasPtr, itemPtr,
		canvasPtr->display, pixmap, screenX1, screenY1, width, height);
    }
    picture = Blt_DrawableToPicture(tkwin, pixmap, 0, 0, width, height, gamma);
    Tk_FreePixmap(Tk_Display(tkwin), pixmap);
    if (picture == NULL) {
	Tcl_AppendResult(interp, "can't grab pixmap \"", pathName, "\"", 
			 (char *)NULL);
	return NULL;
    }
    return picture;
}

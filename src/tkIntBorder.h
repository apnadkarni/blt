
/*
 * tkIntBorder.h --
 *
 *
 * The Border structure used internally by the Tk_3D* routines.
 * The following is a copy of it from tk3d.c.
 *
 *	Contains copies of internal Tk structures.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _TK_BORDER_INT_H
#define _TK_BORDER_INT_H

typedef struct _TkBorder {
    Screen *screen;		/* Screen on which the border will be used. */
    Visual *visual;		/* Visual for all windows and pixmaps using
				 * the border. */
    int depth;			/* Number of bits per pixel of drawables where
				 * the border will be used. */
    Colormap colormap;		/* Colormap out of which pixels are
				 * allocated. */
    int refCount;		/* Number of different users of
				 * this border.  */
#if (_TK_VERSION >= _VERSION(8,1,0))
    int objRefCount;		/* The number of TCL objects that reference
				 * this structure. */
#endif /* _TK_VERSION >= 8.1.0 */
    XColor *bgColor;		/* Background color (intensity between 
				 * lightColorPtr and darkColorPtr). */
    XColor *darkColor;		/* Color for darker areas (must free when
				 * deleting structure). NULL means shadows
				 * haven't been allocated yet.*/
    XColor *lightColor;		/* Color used for lighter areas of border
				 * (must free this when deleting structure).
				 * NULL means shadows haven't been allocated
				 * yet. */
    Pixmap shadow;		/* Stipple pattern to use for drawing
				 * shadows areas.  Used for displays with
				 * <= 64 colors or where colormap has filled
				 * up. */
    GC bgGC;			/* Used (if necessary) to draw areas in
				 * the background color. */
    GC darkGC;			/* Used to draw darker parts of the
				 * border. None means the shadow colors
				 * haven't been allocated yet.*/
    GC lightGC;			/* Used to draw lighter parts of
				 * the border. None means the shadow colors
				 * haven't been allocated yet. */
    Tcl_HashEntry *hashPtr;	/* Entry in borderTable (needed in
				 * order to delete structure). */
    struct _TkBorder *nextPtr; /* Points to the next TkBorder structure with
				 * the same color name.  Borders with the
				 * same name but different screens or
				 * colormaps are chained together off a
				 * single entry in borderTable. */
} TkBorder;

#endif /* _TK_BORDER_INT_H */

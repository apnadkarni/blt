/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * tkIntBorder.h --
 *
 *
 * The Border structure used internally by the Tk_3D* routines.
 * The following is a copy of it from tk3d.c.
 *
 *      Contains copies of internal Tk structures.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 *
 *   This software is copyrighted by the Regents of the University of
 *   California, Sun Microsystems, Inc., and other parties.  The following
 *   terms apply to all files associated with the software unless
 *   explicitly disclaimed in individual files.
 * 
 *   The authors hereby grant permission to use, copy, modify, distribute,
 *   and license this software and its documentation for any purpose,
 *   provided that existing copyright notices are retained in all copies
 *   and that this notice is included verbatim in any distributions. No
 *   written agreement, license, or royalty fee is required for any of the
 *   authorized uses.  Modifications to this software may be copyrighted by
 *   their authors and need not follow the licensing terms described here,
 *   provided that the new terms are clearly indicated on the first page of
 *   each file where they apply.
 * 
 *   IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 *   FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 *   ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 *   DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 * 
 *   THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 *   NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND
 *   THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 *   MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *   GOVERNMENT USE: If you are acquiring this software on behalf of the
 *   U.S. government, the Government shall have only "Restricted Rights" in
 *   the software and related documentation as defined in the Federal
 *   Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 *   are acquiring the software on behalf of the Department of Defense, the
 *   software shall be classified as "Commercial Computer Software" and the
 *   Government shall have only "Restricted Rights" as defined in Clause
 *   252.227-7013 (b) (3) of DFARs.  Notwithstanding the foregoing, the
 *   authors grant the U.S. Government and others acting in its behalf
 *   permission to use and distribute the software in accordance with the
 *   terms specified in this license.
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

#ifndef _TK_BORDER_INT_H
#define _TK_BORDER_INT_H

typedef struct _TkBorder {
    Screen *screen;             /* Screen on which the border will be used. */
    Visual *visual;             /* Visual for all windows and pixmaps using
                                 * the border. */
    int depth;                  /* Number of bits per pixel of drawables where
                                 * the border will be used. */
    Colormap colormap;          /* Colormap out of which pixels are
                                 * allocated. */
    int refCount;               /* Number of different users of
                                 * this border.  */
#if (_TK_VERSION >= _VERSION(8,1,0))
    int objRefCount;            /* The number of TCL objects that reference
                                 * this structure. */
#endif /* _TK_VERSION >= 8.1.0 */
    XColor *bgColor;            /* Background color (intensity between 
                                 * lightColorPtr and darkColorPtr). */
    XColor *darkColor;          /* Color for darker areas (must free when
                                 * deleting structure). NULL means shadows
                                 * haven't been allocated yet.*/
    XColor *lightColor;         /* Color used for lighter areas of border
                                 * (must free this when deleting structure).
                                 * NULL means shadows haven't been allocated
                                 * yet. */
    Pixmap shadow;              /* Stipple pattern to use for drawing
                                 * shadows areas.  Used for displays with
                                 * <= 64 colors or where colormap has filled
                                 * up. */
    GC bgGC;                    /* Used (if necessary) to draw areas in
                                 * the background color. */
    GC darkGC;                  /* Used to draw darker parts of the
                                 * border. None means the shadow colors
                                 * haven't been allocated yet.*/
    GC lightGC;                 /* Used to draw lighter parts of
                                 * the border. None means the shadow colors
                                 * haven't been allocated yet. */
    Tcl_HashEntry *hashPtr;     /* Entry in borderTable (needed in
                                 * order to delete structure). */
    struct _TkBorder *nextPtr; /* Points to the next TkBorder structure with
                                 * the same color name.  Borders with the
                                 * same name but different screens or
                                 * colormaps are chained together off a
                                 * single entry in borderTable. */
} TkBorder;

#endif /* _TK_BORDER_INT_H */

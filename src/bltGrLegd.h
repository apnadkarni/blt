/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrLegd.h --
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

#ifndef _BLT_GR_LEGEND_H
#define _BLT_GR_LEGEND_H

#define LEGEND_RIGHT    (1<<0)          /* Right margin */
#define LEGEND_LEFT     (1<<1)          /* Left margin */
#define LEGEND_BOTTOM   (1<<2)          /* Bottom margin */
#define LEGEND_TOP      (1<<3)          /* Top margin, below the graph
                                         * title. */
#define LEGEND_PLOT     (1<<4)          /* Plot area */
#define LEGEND_XY       (1<<5)          /* Screen coordinates in the
                                         * plotting area. */
#define LEGEND_WINDOW   (1<<6)          /* External window. */
#define LEGEND_MARGIN_MASK \
        (LEGEND_RIGHT | LEGEND_LEFT | LEGEND_BOTTOM | LEGEND_TOP)
#define LEGEND_PLOTAREA_MASK  (LEGEND_PLOT | LEGEND_XY)

BLT_EXTERN int Blt_CreateLegend(Graph *graphPtr);
BLT_EXTERN void Blt_DestroyLegend(Graph *graphPtr);
BLT_EXTERN void Blt_DrawLegend(Graph *graphPtr, Drawable drawable);
BLT_EXTERN void Blt_MapLegend(Graph *graphPtr, int width, int height);
BLT_EXTERN int Blt_LegendOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv);
BLT_EXTERN int Blt_Legend_Site(Graph *graphPtr);
BLT_EXTERN int Blt_Legend_Width(Graph *graphPtr);
BLT_EXTERN int Blt_Legend_Height(Graph *graphPtr);
BLT_EXTERN int Blt_Legend_IsHidden(Graph *graphPtr);
BLT_EXTERN int Blt_Legend_IsRaised(Graph *graphPtr);
BLT_EXTERN int Blt_Legend_X(Graph *graphPtr);
BLT_EXTERN int Blt_Legend_Y(Graph *graphPtr);
BLT_EXTERN void Blt_Legend_RemoveElement(Graph *graphPtr, Element *elemPtr);
BLT_EXTERN void Blt_Legend_EventuallyRedraw(Graph *graphPtr);

#endif /* BLT_GR_LEGEND_H */

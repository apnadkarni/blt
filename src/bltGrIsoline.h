/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrIsoline.h --
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

#ifndef _BLT_GR_ISOLINE_H
#define _BLT_GR_ISOLINE_H

/* 
 * IsolineSegment --
 * 
 *      Represents an individual line segment of a isoline. 
 */
typedef struct _IsolineSegment {
    struct _IsolineSegment *next;         /* Points to next point in
                                         * trace. */
    float x1, y1, x2, y2;               /* Screen coordinate of the
                                         * point. */
    int index;                          /* Index of this coordinate
                                         * pointing back to the raw world
                                         * values in the individual data
                                         * arrays. This index is replicated
                                         * for generated values. */
    unsigned int flags;                 /* Flags associated with a segment
                                         * are described below. */
} IsolineSegment;


struct _Isoline {
    GraphObj obj;
    Element *elemPtr;                   /* Element that is using this
					 * isoline. */
    unsigned int flags;
    const char *label;                  /* Label to be displayed for
                                         * isoline. */
    double reqValue;                    /* Requested isoline value.  Could
                                         * be either absolute or
                                         * relative. */
    Pen *activePenPtr;
    double reqMin, reqMax;
    Pen *penPtr;
    Blt_ChainLink link;

    /* Fields used by the contour element. */
    Blt_HashEntry *hashPtr;		/* Pointer to entry in contour
					 * element's hash table */
    double value;                       /* Value of the isoline. */
    Blt_Chain traces;                   /* Set of traces that describe the
                                         * polyline(s) that represent the
                                         * isoline. */
    Blt_HashTable pointTable;
    void *segments;                     /* Segments used for isolnes. */
    int numSegments;
    Blt_Pixel paletteColor;
};

#endif /* BLT_GR_ISOLINE_H */

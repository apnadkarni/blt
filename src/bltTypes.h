/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTypes.h --
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

#ifndef _BLT_TYPES_H
#define _BLT_TYPES_H

/*
 * -------------------------------------------------------------------
 *
 * Point2f --
 *
 *      2-D coordinate in floats.
 *
 * -------------------------------------------------------------------
 */
typedef struct {
    float x, y;
} Point2f;

/*
 * -------------------------------------------------------------------
 *
 * Point2d --
 *
 *      2-D coordinate in doubles.
 *
 * -------------------------------------------------------------------
 */
typedef struct {
    double x, y;
} Point2d;

/*
 * -------------------------------------------------------------------
 *
 * Point3d --
 *
 *      3-D coordinate in doubles.
 *
 * -------------------------------------------------------------------
 */
typedef struct {
    double x, y, z;
} Point3d;

/*
 * -------------------------------------------------------------------
 *
 * Segment2d --
 *
 *      2-D line segment.
 *
 * -------------------------------------------------------------------
 */
typedef struct {
    Point2d p, q;               /* The two end points of the segment. */
} Segment2d;

/*
 * -------------------------------------------------------------------
 *
 * Dim2d --
 *
 *      2-D dimension.
 *
 * -------------------------------------------------------------------
 */
typedef struct {
    short int width, height;
} Dim2d;

/*
 * -------------------------------------------------------------------
 *
 * Box2d --
 *
 *      2D Bounding box in integer coordinates.
 *
 * -------------------------------------------------------------------
 */
typedef struct {
    int x1, y1, x2, y2;
} Box2d;

/*
 *----------------------------------------------------------------------
 *
 * Region2f --
 *
 *      2-D region.  Used to copy parts of images.
 *
 *----------------------------------------------------------------------
 */
typedef struct {
    double left, right, top, bottom;
} Region2f;

/*
 *----------------------------------------------------------------------
 *
 * Region2d --
 *
 *      2-D region.  Used to copy parts of images.
 *
 *----------------------------------------------------------------------
 */
typedef struct {
    double left, right, top, bottom;
} Region2d;


typedef struct {
    double left, right, top, bottom, front, back;
} Region3d;

#define RegionWidth(r)          ((r)->right - (r)->left + 1)
#define RegionHeight(r)         ((r)->bottom - (r)->top + 1)

#define PointInRegion(e,x,y) \
        (((x) <= (e)->right) && ((x) >= (e)->left) && \
         ((y) <= (e)->bottom) && ((y) >= (e)->top))

#define PointInRectangle(r,x0,y0) \
        (((x0) <= (int)((r)->x + (r)->width - 1)) && ((x0) >= (int)(r)->x) && \
         ((y0) <= (int)((r)->y + (r)->height - 1)) && ((y0) >= (int)(r)->y))

/*
 *----------------------------------------------------------------------
 *
 * ScaleType --
 *
 *      Type of scaling: linear, logarithmic, atan, time or custom.
 *
 *----------------------------------------------------------------------
 */
typedef enum  ScaleTypes {
    SCALE_LINEAR, SCALE_LOG, SCALE_TIME, SCALE_CUSTOM
} ScaleType;

#endif /*_BLT_TYPES_H*/

/*
 * BinaryEncoder --
 */
typedef struct _BinaryEncoder {
    unsigned int flags;
    int wrapLength;                     /* Maximum length of line of encoded
                                         * character before wrapping. */
    const char *pad;                    /* If non-NULL, padding before each
                                         * line of encoded characters. */
    const char *wrap;                   /* If non-NULL, end of line
                                         * character sequence. By default
                                         * it's "\n". */
    const char *altChars;               /* Alternate character base64
                                         * encodings for 63 and 64
                                         * values. */
    Tcl_Obj *fileObjPtr;                /* Name of file representing the
                                         * channel used as the input
                                         * source. */
    Tcl_Obj *dataObjPtr;                /* If non-NULL, data object to use
                                         * as input source. */
    /* Used internally. */
    unsigned int fill;
} BinaryEncoder;

typedef struct _BinaryDecoder {
    unsigned int flags;
    Tcl_Obj *fileObjPtr;                /* Name of file representing the
                                         * channel used as the input
                                         * source. */
    Tcl_Obj *dataObjPtr;                /* If non-NULL, data object to use
                                         * as input source. */
    const char *altChars;               /* Alternate character base64
                                         * encodings for 63 and 64
                                         * values. */
} BinaryDecoder;

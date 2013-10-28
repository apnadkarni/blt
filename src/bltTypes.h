/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltTypes.h --
 *
 *	Copyright 1993-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _BLT_TYPES_H
#define _BLT_TYPES_H

/*
 * -------------------------------------------------------------------
 *
 * Point2f --
 *
 *	2-D coordinate in floats.
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
 *	2-D coordinate in doubles.
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
 *	3-D coordinate in doubles.
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
 *	2-D line segment.
 *
 * -------------------------------------------------------------------
 */
typedef struct {
    Point2d p, q;		/* The two end points of the segment. */
} Segment2d;

/*
 * -------------------------------------------------------------------
 *
 * Dim2d --
 *
 *	2-D dimension.
 *
 * -------------------------------------------------------------------
 */
typedef struct {
    short int width, height;
} Dim2d;

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

#define RegionWidth(r)		((r)->right - (r)->left + 1)
#define RegionHeight(r)		((r)->bottom - (r)->top + 1)

#define PointInRegion(e,x,y) \
	(((x) <= (e)->right) && ((x) >= (e)->left) && \
	 ((y) <= (e)->bottom) && ((y) >= (e)->top))

#define PointInRectangle(r,x0,y0) \
	(((x0) <= (int)((r)->x + (r)->width - 1)) && ((x0) >= (int)(r)->x) && \
	 ((y0) <= (int)((r)->y + (r)->height - 1)) && ((y0) >= (int)(r)->y))

#endif /*_BLT_TYPES_H*/

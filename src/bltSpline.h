/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltSpline.h --
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
#ifndef _BLT_SPLINE_H
#define _BLT_SPLINE_H

typedef struct _Blt_Spline *Blt_Spline;

BLT_EXTERN Blt_Spline Blt_CreateSpline(Point2d *points, int n, int type);
BLT_EXTERN Point2d Blt_EvaluateSpline(Blt_Spline spline, int index, double x);
BLT_EXTERN void Blt_FreeSpline(Blt_Spline spline);

BLT_EXTERN Blt_Spline Blt_CreateParametricCubicSpline(Point2d *points, int n, 
	int w, int h);
BLT_EXTERN Point2d Blt_EvaluateParametricCubicSpline(Blt_Spline spline, 
	int index, double x);
BLT_EXTERN void Blt_FreeParametricCubicSpline(Blt_Spline spline);

BLT_EXTERN Blt_Spline Blt_CreateCatromSpline(Point2d *points, int n);
BLT_EXTERN Point2d Blt_EvaluateCatromSpline(Blt_Spline spline, int i, double t);
BLT_EXTERN void Blt_FreeCatromSpline(Blt_Spline spline);

BLT_EXTERN int Blt_ComputeNaturalSpline (Point2d *origPts, int numOrigPts, 
	Point2d *intpPts, int numIntpPts);

BLT_EXTERN int Blt_ComputeQuadraticSpline(Point2d *origPts, int numOrigPts, 
	Point2d *intpPts, int numIntpPts);

BLT_EXTERN int Blt_ComputeNaturalParametricSpline (Point2d *origPts, 
	int numOrigPts, Region2d *extsPtr, int isClosed, Point2d *intpPts, 
	int numIntpPts);

BLT_EXTERN int Blt_ComputeCatromParametricSpline (Point2d *origPts, 
	int numOrigPts, Point2d *intpPts, int numIntpPts);

#endif /*_BLT_SPLINE_H*/

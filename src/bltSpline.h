/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltSpline.h --
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

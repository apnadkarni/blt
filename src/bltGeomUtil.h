/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGeomUtil.h --
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
#ifndef _BLT_GEOMUTIL_H
#define _BLT_GEOMUTIL_H

#define CLIP_OUTSIDE    0
#define CLIP_INSIDE     (1<<0)
#define CLIP_LEFT       (1<<1)
#define CLIP_RIGHT      (1<<2)

BLT_EXTERN long Blt_SimplifyLine (Point2d *origPts, long low, long high, 
        double tolerance, long *indices);

BLT_EXTERN int Blt_LineRectClip(Region2d *regionPtr, Point2d *p, Point2d *q);

BLT_EXTERN int Blt_PointInPolygon(Point2d *samplePtr, Point2d *points, 
        int numPoints);

BLT_EXTERN int Blt_PolygonInRegion(Point2d *points, int numPoints, 
            Region2d *extsPtr, int enclosed);

BLT_EXTERN int Blt_PointInSegments(Point2d *samplePtr, Segment2d *segments, 
        int numSegments, double halo);

BLT_EXTERN int Blt_PolyRectClip(Region2d *extsPtr, Point2d *inputPts,
        int numInputPts, Point2d *outputPts);

BLT_EXTERN Point2d Blt_GetProjection(double x, double y, Point2d *p, Point2d *q);
BLT_EXTERN Point2d Blt_GetProjection2(double x, double y, double x1, double y1,
                                      double x2, double y2);

BLT_EXTERN int *Blt_ConvexHull(int numPoints, Point2d *points,
                               int *numHullPtsPtr) ;
#endif /* _BLT_GEOMUTIL_H */

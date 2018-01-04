/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGeomUtil.c --
 *
 * This module implements geometry procedures for the BLT toolkit.
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
 * Convex hull routines.
 *
 * Copyright 2001, softSurfer (www.softsurfer.com) 
 *
 *   This code may be freely used and modified for any purpose providing
 *   that this copyright notice is included with it.  SoftSurfer makes no
 *   warranty for this code, and cannot be held liable for any real or
 *   imagined damage resulting from its use.  Users of this code must
 *   verify correctness for their application.
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"
#define _XOPEN_SOURCE 500      /* See feature_test_macros(7) */
#include <stdio.h>

#ifdef HAVE_STDARG_H
  #include <stdarg.h>
#endif /* HAVE_STDARG_H */

#ifdef HAVE_TIME_H
  #include <time.h>
#endif  /* HAVE_TIME_H */

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_ERRNO_H
  #include <errno.h>
#endif /* HAVE_ERRNO_H */

#include "bltMath.h"
#include "bltString.h"
#include <bltHash.h>
#include <bltDBuffer.h>
#include "bltOp.h"

#define BOUND(x, lo, hi)         \
        (((x) > (hi)) ? (hi) : ((x) < (lo)) ? (lo) : (x))

typedef struct {
    double x, y;
    int index;
} HullVertex;

static double
FindSplit(Point2d *points, long i, long j, long *split)    
{    
    double maxDist2;
    
    maxDist2 = -1.0;
    if ((i + 1) < j) {
        long k;
        double a, b, c; 

        /* 
         * 
         *  dist2 P(k) =  |  1  P(i).x  P(i).y  |
         *                |  1  P(j).x  P(j).y  |
         *                |  1  P(k).x  P(k).y  |
         *       ------------------------------------------
         *       (P(i).x - P(j).x)^2 + (P(i).y - P(j).y)^2
         */

        a = points[i].y - points[j].y;
        b = points[j].x - points[i].x;
        c = (points[i].x * points[j].y) - (points[i].y * points[j].x);
        for (k = (i + 1); k < j; k++) {
            double dist2;

            dist2 = (points[k].x * a) + (points[k].y * b) + c;
            if (dist2 < 0.0) {
                dist2 = -dist2; 
            }
            if (dist2 > maxDist2) {
                maxDist2 = dist2;       /* Track the maximum. */
                *split = k;
            }
        }
        /* Correction for segment length---should be redone if can == 0 */
        maxDist2 *= maxDist2 / (a * a + b * b);
    } 
    return maxDist2;
}

/* Douglas-Peucker line simplification algorithm */
long
Blt_SimplifyLine(Point2d *inputPts, long low, long high, double tolerance,
                 long *indices)
{
#define StackPush(a)    s++, stack[s] = (a)
#define StackPop(a)     (a) = stack[s], s--
#define StackEmpty()    (s < 0)
#define StackTop()      stack[s]
    long *stack;
    long split = -1; 
    double tolerance2;
    long s = -1;                 /* Points to top stack item. */
    long count;

    stack = Blt_AssertMalloc(sizeof(int) * (high - low + 1));
    StackPush(high);
    count = 0;
    indices[count++] = 0;
    tolerance2 = tolerance * tolerance;
    while (!StackEmpty()) {
        double dist2;

        dist2 = FindSplit(inputPts, low, StackTop(), &split);
        if (dist2 > tolerance2) {
            StackPush(split);
        } else {
            indices[count++] = StackTop();
            StackPop(low);
        }
    } 
    Blt_Free(stack);
    return count;
}

int
Blt_PointInSegments(
    Point2d *samplePtr,
    Segment2d *segments,
    int numSegments,
    double halo)
{
    Segment2d *sp, *send;
    double minDist;

    minDist = DBL_MAX;
    for (sp = segments, send = sp + numSegments; sp < send; sp++) {
        double dist;
        double left, right, top, bottom;
        Point2d p, t;

        t = Blt_GetProjection(samplePtr->x, samplePtr->y, &sp->p, &sp->q);
        if (sp->p.x > sp->q.x) {
            right = sp->p.x, left = sp->q.x;
        } else {
            right = sp->q.x, left = sp->p.x;
        }
        if (sp->p.y > sp->q.y) {
            bottom = sp->p.y, top = sp->q.y;
        } else {
            bottom = sp->q.y, top = sp->p.y;
        }
        p.x = BOUND(t.x, left, right);
        p.y = BOUND(t.y, top, bottom);
        dist = hypot(p.x - samplePtr->x, p.y - samplePtr->y);
        if (dist < minDist) {
            minDist = dist;
        }
    }
    return (minDist < halo);
}

int
Blt_PointInPolygon(
    Point2d *s,                         /* Sample point. */
    Point2d *points,                    /* Points representing the
                                         * polygon. */
    int numPoints)                      /* # of points in above array. */
{
    Point2d *p, *q, *qend;
    int count;

    count = 0;
    for (p = points, q = p + 1, qend = p + numPoints; q < qend; p++, q++) {
        if (((p->y <= s->y) && (s->y < q->y)) || 
            ((q->y <= s->y) && (s->y < p->y))) {
            double b;

            b = (q->x - p->x) * (s->y - p->y) / (q->y - p->y) + p->x;
            if (s->x < b) {
                count++;                /* Count the # of intersections. */
            }
        }
    }
    return (count & 0x01);
}

int
Blt_PolygonInRegion(Point2d *points, int numPoints, Region2d *regionPtr,
                    int enclosed)
{
    Point2d *pp, *pend;

    if (enclosed) {
        /*  
         * All points of the polygon must be inside the rectangle.
         */
        for (pp = points, pend = pp + numPoints; pp < pend; pp++) {
            if ((pp->x < regionPtr->left) || (pp->x > regionPtr->right) ||
                (pp->y < regionPtr->top) || (pp->y > regionPtr->bottom)) {
                return FALSE;   /* One point is exterior. */
            }
        }
        return TRUE;
    } else {
        Point2d r;
        /*
         * If any segment of the polygon clips the bounding region, the
         * polygon overlaps the rectangle.
         */
        points[numPoints] = points[0];
        for (pp = points, pend = pp + numPoints; pp < pend; pp++) {
            Point2d p, q;

            p = *pp;
            q = *(pp + 1);
            if (Blt_LineRectClip(regionPtr, &p, &q)) {
                return TRUE;
            }
        }
        /* 
         * Otherwise the polygon and rectangle are either disjoint or
         * enclosed.  Check if one corner of the rectangle is inside the
         * polygon.
         */
        r.x = regionPtr->left;
        r.y = regionPtr->top;
        return Blt_PointInPolygon(&r, points, numPoints);
    }
}

static int 
ClipTest (double ds, double dr, double *t1, double *t2)
{
  double t;

  if (ds < 0.0) {
      t = dr / ds;
      if (t > *t2) {
          return FALSE;
      } 
      if (t > *t1) {
          *t1 = t;
      }
  } else if (ds > 0.0) {
      t = dr / ds;
      if (t < *t1) {
          return FALSE;
      } 
      if (t < *t2) {
          *t2 = t;
      }
  } else {
      /* d = 0, so line is parallel to this clipping edge */
      if (dr < 0.0) {                   /* Line is outside clipping edge */
          return FALSE;
      }
  }
  return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LineRectClip --
 *
 *      Clips the given line segment to a rectangular region.  The
 *      coordinates of the clipped line segment are returned.  The original
 *      coordinates are overwritten.
 *
 *      Reference: 
 *        Liang, Y-D., and B. Barsky, A new concept and method for
 *        Line Clipping, ACM, TOG,3(1), 1984, pp.1-22.
 *
 * Results:
 *      Returns if line segment is visible within the region. The coordinates
 *      of the original line segment are overwritten by the clipped
 *      coordinates.
 *
 *---------------------------------------------------------------------------
 */
int 
Blt_LineRectClip(
    Region2d *regionPtr,                /* Rectangular region to clip. */
    Point2d *p, Point2d *q)             /* (in/out) Coordinates of original
                                         * and clipped line segment. */
{
    double t1, t2;
    double dx;

    t1 = 0.0, t2 = 1.0;
    dx = q->x - p->x;
    if ((ClipTest (-dx, p->x - regionPtr->left, &t1, &t2)) &&
        (ClipTest (dx, regionPtr->right - p->x, &t1, &t2))) {
        double dy;

        dy = q->y - p->y;
        if ((ClipTest (-dy, p->y - regionPtr->top, &t1, &t2)) && 
            (ClipTest (dy, regionPtr->bottom - p->y, &t1, &t2))) {
            int flags;

            flags = CLIP_INSIDE;
            if (t2 < 1.0) {
                q->x = p->x + t2 * dx;
                q->y = p->y + t2 * dy;
                flags |= CLIP_RIGHT;
            }
            if (t1 > 0.0) {
                p->x += t1 * dx;
                p->y += t1 * dy;
                flags |= CLIP_LEFT;
            }
            return flags;
        }
    }
    return CLIP_OUTSIDE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PolyRectClip --
 *
 *      Clips the given polygon to a rectangular region.  The resulting
 *      polygon is returned. Note that the resulting polyon may be complex,
 *      connected by zero width/height segments.  The drawing routine (such
 *      as XFillPolygon) will not draw a connecting segment.
 *
 *      Reference:  
 *        Liang Y. D. and Brian A. Barsky, "Analysis and Algorithm for
 *        Polygon Clipping", Communications of ACM, Vol. 26,
 *        p.868-877, 1983
 *
 * Results:
 *      Returns the number of points in the clipped polygon. The points of
 *      the clipped polygon are stored in *outputPts*.
 *
 *---------------------------------------------------------------------------
 */
#define EPSILON  FLT_EPSILON
#define AddVertex(vx, vy)           r->x=(vx), r->y=(vy), r++, count++ 
#define LastVertex(vx, vy)          r->x=(vx), r->y=(vy), count++ 

int 
Blt_PolyRectClip(
    Region2d *regionPtr,                /* Rectangular region clipping the
                                         * polygon. */
    Point2d *points,                    /* Points of polygon to be
                                         * clipped. */
    int numPoints,                      /* # of points in polygon. */
    Point2d *clipPts)                   /* (out) Points of clipped
                                         * polygon. */
{
    Point2d *p;                         /* First vertex of input polygon
                                         * edge. */
    Point2d *pend;
    Point2d *q;                         /* Last vertex of input polygon
                                         * edge. */
    Point2d *r;
    int count;

    r = clipPts;
    count = 0;                          /* Counts # of vertices in output
                                         * polygon. */

    points[numPoints] = points[0];
    for (p = points, q = p + 1, pend = p + numPoints; p < pend; p++, q++) {
        double dx, dy;
        double tin1, tin2, tinx, tiny;
        double xin, yin, xout, yout;

        dx = q->x - p->x;               /* X-direction */
        dy = q->y - p->y;               /* Y-direction */

        if (FABS(dx) < EPSILON) { 
            dx = (p->x > regionPtr->left) ? -EPSILON : EPSILON ;
        }
        if (FABS(dy) < EPSILON) { 
            dy = (p->y > regionPtr->top) ? -EPSILON : EPSILON ;
        }

        if (dx > 0.0) {         /* Left */
            xin = regionPtr->left;
            xout = regionPtr->right + 1.0;
        } else {                /* Right */
            xin = regionPtr->right + 1.0;
            xout = regionPtr->left;
        }
        if (dy > 0.0) {         /* Top */
            yin = regionPtr->top;
            yout = regionPtr->bottom + 1.0;
        } else {                /* Bottom */
            yin = regionPtr->bottom + 1.0;
            yout = regionPtr->top;
        }
        
        tinx = (xin - p->x) / dx;
        tiny = (yin - p->y) / dy;
        
        if (tinx < tiny) {              /* Hits x first */
            tin1 = tinx;
            tin2 = tiny;
        } else {                        /* Hits y first */
            tin1 = tiny;
            tin2 = tinx;
        }
        
        if (tin1 <= 1.0) {
            if (tin1 > 0.0) {
                AddVertex(xin, yin);
            }
            if (tin2 <= 1.0) {
                double toutx, touty, tout1;

                toutx = (xout - p->x) / dx;
                touty = (yout - p->y) / dy;
                tout1 = MIN(toutx, touty);
                
                if ((tin2 > 0.0) || (tout1 > 0.0)) {
                    if (tin2 <= tout1) {
                        if (tin2 > 0.0) {
                            if (tinx > tiny) {
                                AddVertex(xin, p->y + tinx * dy);
                            } else {
                                AddVertex(p->x + tiny * dx, yin);
                            }
                        }
                        if (tout1 < 1.0) {
                            if (toutx < touty) {
                                AddVertex(xout, p->y + toutx * dy);
                            } else {
                                AddVertex(p->x + touty * dx, yout);
                            }
                        } else {
                            AddVertex(q->x, q->y);
                        }
                    } else {
                        if (tinx > tiny) {
                            AddVertex(xin, yout);
                        } else {
                            AddVertex(xout, yin);
                        }

                    }
                }
            }
        }
    }
    if (count > 0) {
        LastVertex(clipPts[0].x, clipPts[0].y);
    }
    return count;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetProjection --
 *
 *      Computes the projection of a point on a line.  The line (given by
 *      two points), is assumed the be infinite.
 *
 *      Compute the slope (angle) of the line and rotate it 90 degrees.
 *      Using the slope-intercept method (we know the second line from the
 *      sample test point and the computed slope), then find the
 *      intersection of both lines. This will be the projection of the
 *      sample point on the first line.
 *
 * Results:
 *      Returns the coordinates of the projection on the line.
 *
 *---------------------------------------------------------------------------
 */
Point2d
Blt_GetProjection(
    double x, double y,                 /* Screen coordinates of the sample
                                         * point. */
    Point2d *p, Point2d *q)             /* Line segment to project point
                                         * onto */
{
    double dx, dy;
    Point2d t;

    dx = p->x - q->x;
    dy = p->y - q->y;

    /* Test for horizontal and vertical lines */
    if (FABS(dx) < DBL_EPSILON) {
        t.x = p->x, t.y = y;
    } else if (FABS(dy) < DBL_EPSILON) {
        t.x = x, t.y = p->y;
    } else {
        double m1, m2;                  /* Slope of both lines */
        double b1, b2;                  /* y-intercepts */
        double midX, midY;              /* Midpoint of line segment. */
        double ax, ay, bx, by;

        /* Compute the slope and intercept of PQ. */
        m1 = (dy / dx);
        b1 = p->y - (p->x * m1);

        /* 
         * Compute the slope and intercept of a second line segment: one
         * that intersects through sample X-Y coordinate with a slope
         * perpendicular to original line.
         */
        /* Find midpoint of PQ. */
        midX = (p->x + q->x) * 0.5;
        midY = (p->y + q->y) * 0.5;

        /* Rotate the line 90 degrees */
        ax = midX - (0.5 * dy);
        ay = midY - (0.5 * -dx);
        bx = midX + (0.5 * dy);
        by = midY + (0.5 * -dx);

        m2 = (ay - by) / (ax - bx);
        b2 = y - (x * m2);

        /*
         * Given the equations of two lines which contain the same point,
         *
         *    y = m1 * x + b1
         *    y = m2 * x + b2
         *
         * solve for the intersection.
         *
         *    x = (b2 - b1) / (m1 - m2)
         *    y = m1 * x + b1
         *
         */
        t.x = (b2 - b1) / (m1 - m2);
        t.y = m1 * t.x + b1;
    }
    return t;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetProjection2 --
 *
 *      Computes the projection of a point on a line.  The line (given by
 *      x1, y1, x2, y2), is assumed the be infinite.
 *
 *      Compute the slope (angle) of the line and rotate it 90 degrees.
 *      Using the slope-intercept method (we know the second line from the
 *      sample test point and the computed slope), then find the
 *      intersection of both lines. This will be the projection of the
 *      sample point on the first line.
 *
 * Results:
 *      Returns the coordinates of the projection on the line.
 *
 *---------------------------------------------------------------------------
 */
Point2d
Blt_GetProjection2(
    double x, double y,                 /* Screen coordinates of the sample
                                         * point. */
    double x1, double y1,
    double x2, double y2)               /* Line segment to project point
                                         * onto */
{
    double dx, dy;
    Point2d t;

    dx = x1 - x2;
    dy = y1 - y2;

    /* Test for horizontal and vertical lines */
    if (FABS(dx) < DBL_EPSILON) {
        t.x = x1, t.y = y;
    } else if (FABS(dy) < DBL_EPSILON) {
        t.x = x, t.y = y1;
    } else {
        double m1, m2;                  /* Slope of both lines */
        double b1, b2;                  /* y-intercepts */
        double cx, cy;                  /* Midpoint of line segment. */
        double ax, ay, bx, by;

        /* Compute the slope and intercept of PQ. */
        m1 = (dy / dx);
        b1 = y1 - (x1 * m1);

        /* 
         * Compute the slope and intercept of a second line segment: one
         * that intersects through sample X-Y coordinate with a slope
         * perpendicular to original line.
         */
        /* Find midpoint of PQ. */
        cx = (x1 + x2) * 0.5;
        cy = (y1 + y2) * 0.5;

        /* Rotate the line 90 degrees */
        ax = cx - (0.5 * dy);
        ay = cy - (0.5 * -dx);
        bx = cx + (0.5 * dy);
        by = cy + (0.5 * -dx);

        m2 = (ay - by) / (ax - bx);
        b2 = y - (x * m2);

        /*
         * Given the equations of two lines which contain the same point,
         *
         *    y = m1 * x + b1
         *    y = m2 * x + b2
         *
         * solve for the intersection.
         *
         *    x = (b2 - b1) / (m1 - m2)
         *    y = m1 * x + b1
         *
         */
        t.x = (b2 - b1) / (m1 - m2);
        t.y = m1 * t.x + b1;
    }
    return t;
}

static INLINE double 
IsLeft(HullVertex *p0, HullVertex *p1, HullVertex *p2) 
{
    return (((p1->x - p0->x) * (p2->y - p0->y)) - 
            ((p2->x - p0->x) * (p1->y - p0->y)));
}

/*
 *---------------------------------------------------------------------------
 *
 * ChainHull2d --
 *
 *      Computes the convex hull from the vertices of the mesh.  
 *
 *      Notes: 1. The array of vertices is assumed to be sorted.  
 *             2. An array to contain the vertices is assumed.  This is 
 *                allocated by the caller.
 *      
 * Results:
 *      The number of vertices in the convex hull is returned. The
 *      coordinates of the hull will be written in the given point array.
 *
 * Copyright 2001, softSurfer (www.softsurfer.com) 
 *
 *   This code may be freely used and modified for any purpose providing
 *   that this copyright notice is included with it.  SoftSurfer makes no
 *   warranty for this code, and cannot be held liable for any real or
 *   imagined damage resulting from its use.  Users of this code must
 *   verify correctness for their application.
 *
 *---------------------------------------------------------------------------
 */
#define PUSH(i)     (top++, hull[top] = (i))
#define POP()       (top--)

static int 
ChainHull2d(int numPoints, HullVertex *points, int *hull) 
{
    int bot, top, i; 
    int minMin, minMax;
    int maxMin, maxMax;
    double xMin, xMax;

    bot = 0;                            /* Index to bottom of stack. */
    top = -1;                           /* Index to top of stack. */
    minMin = 0;

    /* 
     * Step 1. Get the indices of points with max x-coord and min|max
     *         y-coord .
     */
    xMin = points[0].x;
    for (i = 1; i < numPoints; i++) {
        if (points[i].x != xMin) {
            break;
        }
    }
    minMax = i - 1;
    
    if (minMax == (numPoints - 1)) {    /* Degenerate case: all x-coords ==
                                         * xMin */
        PUSH(minMin);
        if (points[minMax].y != points[minMin].y) {
            PUSH(minMax);               /* A nontrivial segment. */
        }
        PUSH(minMin);
        return top + 1;
    }

    maxMax = numPoints - 1;
    xMax = points[maxMax].x;
    for (i = numPoints - 2; i >= 0; i--) {
        if (points[i].x != xMax) {
            break;
        }
    }
    maxMin = i + 1;

    /* Step 2. Compute the lower hull on the stack. */

    PUSH(minMin);                       /* Push minMin point onto stack. */
    i = minMax;

    while (++i <= maxMin) {
        /* The lower line joins v[minMin] with v[maxMin]. */
        if ((IsLeft(points + minMin, points + maxMin, points + i) >= 0.0) && 
            (i < maxMin)) {
            continue;                   /* Ignore points[i] above or on the
                                         * lower line */
        }
        while (top > 0) {               /* There are at least 2 vertices on
                                         * the stack. */

            /* Test if points[i] is left of the line at the stack top. */
            if (IsLeft(points+hull[top-1], points+hull[top], points+i) > 0.0) {
                break;                  /* points[i] is a new hull vertex. */
            } else {
                POP();                  /* Pop top point off stack */
            }
        }
        PUSH(i);                        /* Push point[i] onto stack. */
    }

    /* Step 3. Compute the upper hull on the stack above the bottom hull. */
    if (maxMax != maxMin)  {            /* if distinct xMax points */
        PUSH(maxMax);                   /* Push maxMax point onto stack. */
    }

    bot = top;                          /* The bottom point of the upper hull
                                           stack. */
    i = maxMin;

    while (--i >= minMax) {
        /* The upper line joins points[maxMax] with points[minMax]. */
        if ((IsLeft(points + maxMax, points + minMax, points + i) >= 0.0) && 
            (i > minMax)) {
            continue;                   /* Ignore points[i] below or on the
                                         * upper line. */
        }
        while (top > bot) {             /* At least 2 points on the upper
                                         * stack. */

            /*  Test if points[i] is left of the line at the stack top. */
            if (IsLeft(points+hull[top-1], points+hull[top], points+i) > 0.0) {
                break;                  /* v[i] is a new hull vertex. */
            } else {
                POP();                  /* Pop top point off stack. */
            }
        }
        PUSH(i);                        /* Push points[i] onto stack. */
    }
    if (minMax != minMin) {
        PUSH(minMin);                  /* Push joining endpoint onto stack. */
    }
    return top + 1;
}


static int
CompareVertices(const void *a, const void *b)
{
    const HullVertex *v1 = a;
    const HullVertex *v2 = b;

    if (v1->y < v2->y) {
        return -1;
    }
    if (v1->y > v2->y) {
        return 1;
    }
    if (v1->x < v2->x) {
        return -1;
    }
    if (v1->x > v2->x) {
        return 1;
    }
    return 0;
}

int *
Blt_ConvexHull(int numPoints, Point2d *points, int *numHullPtsPtr) 
{
    HullVertex *vertices;
    int *hull;
    int i, numVertices;
    
    vertices = Blt_Malloc(numPoints * sizeof(HullVertex));
    for (i = 0; i < numPoints; i++) {
        vertices[i].x = points[i].x;
        vertices[i].y = points[i].y;
        vertices[i].index = i;
    }
    if (vertices == NULL) {
        return NULL;
    }
    qsort(vertices, numPoints, sizeof(HullVertex), CompareVertices);

    /* Allocate worst-case storage initially for the hull. */
    hull = Blt_Malloc(numPoints * sizeof(int));
    if (hull == NULL) {
        Blt_Free(vertices);
        return NULL;
    }

    /* Compute the convex hull. */
    numVertices = ChainHull2d(numPoints, vertices, hull);
    /* Resize the hull array to the actual # of boundary points. */
    if (numVertices < numPoints) {
        hull = Blt_Realloc(hull, numVertices * sizeof(int));
        if (hull == NULL) {
            Blt_Free(vertices);
            return NULL;
        }
    }
    /* Remap the indices back to the unsorted point array. */
    for (i = 0; i < numVertices; i++) {
        hull[i] = vertices[hull[i]].index;
    }
    Blt_Free(vertices);
    *numHullPtsPtr = numVertices;
    return hull;
}

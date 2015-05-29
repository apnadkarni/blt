/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrMisc.c --
 *
 * This module implements miscellaneous routines for the BLT graph widget.
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

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STDARG_H
  #include <stdarg.h>
#endif /* HAVE_STDARG_H */

#include <X11/Xutil.h>

#include "bltAlloc.h"
#include "bltHash.h"
#include "bltChain.h"
#include "bltBind.h"
#include "bltNsUtil.h"
#include "bltOp.h"
#include "bltPs.h"
#include "bltBg.h"
#include "bltMath.h"
#include "bltPicture.h"
#include "bltGraph.h"
#include "bltGrAxis.h"
#include "bltGrLegd.h"

static Blt_OptionParseProc ObjToPoint;
static Blt_OptionPrintProc PointToObj;
Blt_CustomOption bltPointOption =
{
    ObjToPoint, PointToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToLimitsProc;
static Blt_OptionPrintProc LimitsToObjProc;
Blt_CustomOption bltLimitsOption =
{
    ObjToLimitsProc, LimitsToObjProc, NULL, (ClientData)0
};


/*
 *---------------------------------------------------------------------------
 * Custom option parse and print procedures
 *---------------------------------------------------------------------------
 */

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetXY --
 *
 *      Converts a string in the form "@x,y" into an XPoint structure of
 *      the x and y coordinates.
 *
 * Results:
 *      A standard TCL result. If the string represents a valid position
 *      *pointPtr* will contain the converted x and y coordinates and
 *      TCL_OK is returned.  Otherwise, TCL_ERROR is returned and
 *      interp->result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetXY(Tcl_Interp *interp, Tk_Window tkwin, const char *string, 
          int *xPtr, int *yPtr)
{
    char *comma;
    int result;
    int x, y;

    if ((string == NULL) || (*string == '\0')) {
        *xPtr = *yPtr = -SHRT_MAX;
        return TCL_OK;
    }
    if (*string != '@') {
        goto badFormat;
    }
    comma = strchr(string + 1, ',');
    if (comma == NULL) {
        goto badFormat;
    }
    *comma = '\0';
    result = ((Tk_GetPixels(interp, tkwin, string + 1, &x) == TCL_OK) &&
              (Tk_GetPixels(interp, tkwin, comma + 1, &y) == TCL_OK));
    *comma = ',';
    if (!result) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, ": can't parse position \"", string, "\"",
                             (char *)NULL);
        }
        return TCL_ERROR;
    }
    *xPtr = x, *yPtr = y;
    return TCL_OK;

  badFormat:
    if (interp != NULL) {
        Tcl_AppendResult(interp, "bad position \"", string, 
             "\": should be \"@x,y\"", (char *)NULL);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPoint --
 *
 *      Convert the string representation of a legend XY position into
 *      window coordinates.  The form of the string must be "@x,y" or none.
 *
 * Results:
 *      A standard TCL result.  The symbol type is written into the widget
 *      record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPoint(
    ClientData clientData,      /* Not used. */
    Tcl_Interp *interp,         /* Interpreter to send results back to */
    Tk_Window tkwin,            /* Not used. */
    Tcl_Obj *objPtr,            /* New legend position string */
    char *widgRec,              /* Widget record */
    int offset,                 /* Offset to field in structure */
    int flags)                  /* Not used. */
{
    XPoint *pointPtr = (XPoint *)(widgRec + offset);
    int x, y;

    if (Blt_GetXY(interp, tkwin, Tcl_GetString(objPtr), &x, &y) != TCL_OK) {
        return TCL_ERROR;
    }
    pointPtr->x = x, pointPtr->y = y;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PointToObj --
 *
 *      Convert the window coordinates into a string.
 *
 * Results:
 *      The string representing the coordinate position is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
PointToObj(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Not used. */
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    XPoint *pointPtr = (XPoint *)(widgRec + offset);
    Tcl_Obj *objPtr;

    if ((pointPtr->x != -SHRT_MAX) && (pointPtr->y != -SHRT_MAX)) {
        char string[200];

        Blt_FormatString(string, 200, "@%d,%d", pointPtr->x, pointPtr->y);
        objPtr = Tcl_NewStringObj(string, -1);
    } else { 
        objPtr = Tcl_NewStringObj("", -1);
    }
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLimitsProc --
 *
 *      Converts the list of elements into zero or more pixel values which
 *      determine the range of pixel values possible.  An element can be in
 *      any form accepted by Tk_GetPixels. The list has a different meaning
 *      based upon the number of elements.
 *
 *          # of elements:
 *
 *          0 - the limits are reset to the defaults.
 *          1 - the minimum and maximum values are set to this
 *              value, freezing the range at a single value.
 *          2 - first element is the minimum, the second is the
 *              maximum.
 *          3 - first element is the minimum, the second is the
 *              maximum, and the third is the nominal value.
 *
 *      Any element may be the empty string which indicates the default.
 *
 * Results:
 *      The return value is a standard TCL result.  The min and max fields
 *      of the range are set.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToLimitsProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Widget of paneset */
    Tcl_Obj *objPtr,                    /* New width list */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Blt_Limits *limitsPtr = (Blt_Limits *)(widgRec + offset);

    if (Blt_GetLimitsFromObj(interp, tkwin, objPtr, limitsPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * LimitsToObjProc --
 *
 *      Convert the limits of the pixel values allowed into a list.
 *
 * Results:
 *      The string representation of the limits is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
LimitsToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Not used. */
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Row/column structure record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Blt_Limits *limitsPtr = (Blt_Limits *)(widgRec + offset);
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (limitsPtr->flags & LIMITS_MIN_SET) {
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                                 Tcl_NewIntObj(limitsPtr->min));
    } else {
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    if (limitsPtr->flags & LIMITS_MAX_SET) {
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                                 Tcl_NewIntObj(limitsPtr->max));
    } else {
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    if (limitsPtr->flags & LIMITS_NOM_SET) {
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                                 Tcl_NewIntObj(limitsPtr->nom));
    } else {
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    return listObjPtr;
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

        t = Blt_GetProjection((int)samplePtr->x, (int)samplePtr->y, 
                              &sp->p, &sp->q);
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
Blt_RegionInPolygon(
    Region2d *regionPtr,
    Point2d *points,
    int numPoints,
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
    double dx, dy;

    t1 = 0.0, t2 = 1.0;
    dx = q->x - p->x;
    if ((ClipTest (-dx, p->x - regionPtr->left, &t1, &t2)) &&
        (ClipTest (dx, regionPtr->right - p->x, &t1, &t2))) {
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
    int x, int y,                       /* Screen coordinates of the sample
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
        t.x = p->x, t.y = (double)y;
    } else if (FABS(dy) < DBL_EPSILON) {
        t.x = (double)x, t.y = p->y;
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
    int x, int y,                       /* Screen coordinates of the sample
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
        t.x = x1, t.y = (double)y;
    } else if (FABS(dy) < DBL_EPSILON) {
        t.x = (double)x, t.y = y1;
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


typedef struct {
    double hue, sat, val;
} HSV;

#define SetColor(c,r,g,b) ((c)->red = (int)((r) * 65535.0), \
                           (c)->green = (int)((g) * 65535.0), \
                           (c)->blue = (int)((b) * 65535.0))

#ifdef notdef
void
Blt_XColorToHSV(XColor *colorPtr, HSV *hsvPtr)
{
    unsigned short max, min;
    double range;
    unsigned short *colorValues;

    /* Find the minimum and maximum RGB intensities */
    colorValues = (unsigned short *)&colorPtr->red;
    max = MAX3(colorValues[0], colorValues[1], colorValues[2]);
    min = MIN3(colorValues[0], colorValues[1], colorValues[2]);

    hsvPtr->val = (double)max / 65535.0;
    hsvPtr->hue = hsvPtr->sat = 0.0;

    range = (double)(max - min);
    if (max != min) {
        hsvPtr->sat = range / (double)max;
    }
    if (hsvPtr->sat > 0.0) {
        double red, green, blue;

        /* Normalize the RGB values */
        red = (double)(max - colorPtr->red) / range;
        green = (double)(max - colorPtr->green) / range;
        blue = (double)(max - colorPtr->blue) / range;

        if (colorPtr->red == max) {
            hsvPtr->hue = (blue - green);
        } else if (colorPtr->green == max) {
            hsvPtr->hue = 2 + (red - blue);
        } else if (colorPtr->blue == max) {
            hsvPtr->hue = 4 + (green - red);
        }
        hsvPtr->hue *= 60.0;
    } else {
        hsvPtr->sat = 0.5;
    }
    if (hsvPtr->hue < 0.0) {
        hsvPtr->hue += 360.0;
    }
}

void
Blt_HSVToXColor(HSV *hsvPtr, XColor *colorPtr)
{
    double hue, p, q, t;
    double frac;
    int quadrant;

    if (hsvPtr->val < 0.0) {
        hsvPtr->val = 0.0;
    } else if (hsvPtr->val > 1.0) {
        hsvPtr->val = 1.0;
    }
    if (hsvPtr->sat == 0.0) {
        SetColor(colorPtr, hsvPtr->val, hsvPtr->val, hsvPtr->val);
        return;
    }
    hue = FMOD(hsvPtr->hue, 360.0) / 60.0;
    quadrant = (int)floor(hue);
    frac = hsvPtr->hue - quadrant;
    p = hsvPtr->val * (1 - (hsvPtr->sat));
    q = hsvPtr->val * (1 - (hsvPtr->sat * frac));
    t = hsvPtr->val * (1 - (hsvPtr->sat * (1 - frac)));

    switch (quadrant) {
    case 0:
        SetColor(colorPtr, hsvPtr->val, t, p);
        break;
    case 1:
        SetColor(colorPtr, q, hsvPtr->val, p);
        break;
    case 2:
        SetColor(colorPtr, p, hsvPtr->val, t);
        break;
    case 3:
        SetColor(colorPtr, p, q, hsvPtr->val);
        break;
    case 4:
        SetColor(colorPtr, t, p, hsvPtr->val);
        break;
    case 5:
        SetColor(colorPtr, hsvPtr->val, p, q);
        break;
    }
}

static void
RGBToHSV(float *rgb, float *hsv)
{
    double max, min, range;

    /* Find the minimum and maximum RGB intensities */
    max = MAX3(rgb[0], rgb[1], rgb[2]);
    min = MIN3(rgb[0], rgb[1], rgb[2]);

    hsv[2] = max / 65535.0;
    hsv[0] = hsv[1] = 0.0;

    range = (max - min);
    if (max != min) {
        hsv[1] = range / max;
    }
    if (hsv[1] > 0.0) {
        double red, green, blue;

        /* Normalize the RGB values */
        red = (double)(max - rgb[0]) / range;
        green = (double)(max - colorPtr->green) / range;
        blue = (double)(max - colorPtr->blue) / range;

        if (rgb[0] == max) {
            hsv[0] = (rgb[2] - rgb[1]);
        } else if (colorPtr->green == max) {
            hsv[0] = 2 + (rgb[0] - rgb[2]);
        } else if (colorPtr->blue == max) {
            hsv[0] = 4 + (rgb[1] - rgb[0]);
        }
        hsv[0] *= 60.0;
    } else {
        hsv[1] = 0.5;
    }
    if (hsv[0] < 0.0) {
        hsv[0] += 360.0;
    }
}

static void
RGBtoXYZ(float *rgb, float *xyz)
{
    float r, g, b;
                                                                 
    r = rgb[0], g = rgb[1], b = rgb[2];
    if (r <= 0.04045) {
        r = r / 12.;
    } else {
        r = pow((r+0.055) / 1.055, 2.4);
    }
    if (g <= 0.04045) {
        g = g / 12.;
    } else {
        g = pow((g+0.055) / 1.055, 2.4);
    }
    if (b <= 0.04045) {
        b = b / 12.;
    } else {
        b = pow((b+0.055)/1.055, 2.4);
    }
    xyz[0] =  0.436052025 * r + 0.385081593 * g + 0.143087414 * b;
    xyz[1] =  0.222491598 * r + 0.71688606  * g + 0.060621486 * b;
    xyz[2] =  0.013929122 * r + 0.097097002 * g + 0.71418547  * b;
} 

static void
XYZtoRGB(float *xyz, float *rgb)
{
    rgb[0] =  3.240479 * xyz[0] - 1.53715  * xyz[1] - 0.498535 * xyz[2];
    rgb[1] = -0.969256 * xyz[0] + 1.875991 * xyz[1] + 0.041556 * xyz[2];
    rgb[2] =  0.055648 * xyz[0] - 0.204043 * xyz[1] + 1.057311 * xyz[2];
}

static INLINE double
ComputeLAB(float x) 
{ 
    double k, eps;
    double fx;

    k   = 24389.0 / 27.0;
    eps = 216.0 / 24389.0;
    return (x > eps) ? cbrt(x) : ((k * x + 16.0) / 116.0);
}

static void 
XYZtoLAB(float *xyz, float *lab) 
{
    double x, y, z;
    
    x = ComputeLAB(xyz[0] / 0.964221);  /* reference white D50 */
    y = ComputeLAB(xyz[1]);
    z = ComputeLAB(xyz[2] / 0.825211);

    lab[0] = 2.55 * ((116.0 * y) - 16);
    lab[1] = 500.0 * (x-y); 
    lab[2] = 200.0 * (y-z);       
} 

void 
XYZToLUV(float *xyz, float *luv) 
{
    double u, v, uref, vref, yr, L;
#define EPS     (216.0 /24389.0)
#define K       (24389.0/27.0);
#define XREF    0.964221.0              /* Reference white D50 */
#define YREF    1.0
#define ZREF    0.825211
    
    u = 4 * xyz[0] / (xyz[0] + (15.0 * xyz[1]) + (3.0 * xyz[2]));
    v = 9 * xyz[1] / (xyz[0] + (15.0 * xyz[1]) + (3.0 * xyz[2]));
    
    uref = 4 * XREF / (XREF + (15.0 * YREF) + (3.0 * ZREF));
    vref = 9 * YREF / (XREF + (15.0 * YREF) + (3.0 * ZREF));
    
    yr = xyz[1] / YR;
    
    if (yr > EPS) {
        L = (116.0 * cbrt(YREF) - 16);
    } else {
        L = K * YREF;
    }
    luv[0] = 2.55 * L;
    luv[1] = 13 * L * (u - uref);
    luv[2] = 13 * L * (v - vref);
}

static void 
RGBToLAB(float *rgb, float *lab) 
{
    float xyz[3];

    RBGToXYZ(rgb, xyz);
    XYZToLAB(xyz, lab);
} 

static void 
RGBToLUV(float *rgb, float *luv) 
{
    float xyz[3];

    RBGToXYZ(rgb, xyz);
    XYZToLUV(xyz, luv);
} 

void
Blt_PixelToXYZ(Blt_Pixel *pixelPtr, float *xyz)
{
    float rgb[3];

    rgb[0] = pixelPtr->Red / 255.0;             
    rgb[1] = pixelPtr->Green / 255.0;
    rgb[2] = pixelPtr->Blue /255.0;
    RGBtoXYZ(rgb, xyz);
}


void 
rgb2luv(int R, int G, int B, int []luv) 
{
    float rf, gf, bf;
    float r, g, b, X_, Y_, Z_, X, Y, Z, fx, fy, fz, xr, yr, zr;
    float L;
    float eps = 216.f/24389.f;
    float k = 24389.f/27.f;
    
    float Xr = 0.964221f;  // reference white D50
    float Yr = 1.0f;
    float Zr = 0.825211f;
    
    RGBToXYZ(r, g, b, &xyz);
    XYZToLUV(&xyz, luvPtr);
    
    // XYZ to Luv
    
    float u, v, u_, v_, ur_, vr_;
    
    u_ = 4 *X / (X + 15*Y + 3*Z);
    v_ = 9*Y / (X + 15*Y + 3*Z);
    
    ur_ = 4*Xr / (Xr + 15*Yr + 3*Zr);
    vr_ = 9*Yr / (Xr + 15*Yr + 3*Zr);
    
    yr = Y/Yr;
    
    if ( yr > eps )
        L =  (float) (116*Math.pow(yr, 1/3.) - 16);
    else
        L = k * yr;
    
    u = 13*L*(u_ -ur_);
    v = 13*L*(v_ -vr_);
    
    ciePtr->l = (2.55*L + .5);
    ciePtr->u = u; 
    ciePtr->v = v;        
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * Blt_AdjustViewport --
 *
 *      Adjusts the offsets of the viewport according to the scroll mode.
 *      This is to accommodate both "listbox" and "canvas" style scrolling.
 *
 *      "canvas"        The viewport scrolls within the range of world
 *                      coordinates.  This way the viewport always displays
 *                      a full page of the world.  If the world is smaller
 *                      than the viewport, then (bizarrely) the world and
 *                      viewport are inverted so that the world moves up
 *                      and down within the viewport.
 *
 *      "listbox"       The viewport can scroll beyond the range of world
 *                      coordinates.  Every entry can be displayed at the
 *                      top of the viewport.  This also means that the
 *                      scrollbar thumb weirdly shrinks as the last entry
 *                      is scrolled upward.
 *
 * Results:
 *      The corrected offset is returned.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_AdjustViewport(int offset, int worldSize, int windowSize, int scrollUnits, 
                   int scrollMode)
{
    switch (scrollMode) {
    case BLT_SCROLL_MODE_CANVAS:

        /*
         * Canvas-style scrolling allows the world to be scrolled within the
         * window.
         */
        if (worldSize < windowSize) {
            if ((worldSize - offset) > windowSize) {
                offset = worldSize - windowSize;
            }
            if (offset > 0) {
                offset = 0;
            }
        } else {
            if ((offset + windowSize) > worldSize) {
                offset = worldSize - windowSize;
            }
            if (offset < 0) {
                offset = 0;
            }
        }
        break;

    case BLT_SCROLL_MODE_LISTBOX:
        if (offset < 0) {
            offset = 0;
        }
        if (offset >= worldSize) {
            offset = worldSize - scrollUnits;
        }
        break;

    case BLT_SCROLL_MODE_HIERBOX:

        /*
         * Hierbox-style scrolling allows the world to be scrolled within the
         * window.
         */
        if ((offset + windowSize) > worldSize) {
            offset = worldSize - windowSize;
        }
        if (offset < 0) {
            offset = 0;
        }
        break;
    }
    return offset;
}

int
Blt_GetScrollInfoFromObj(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv,
                         int *offsetPtr, int worldSize, int windowSize, 
                         int scrollUnits, int scrollMode)
{
    char c;
    const char *string;
    int length;
    int offset;

    offset = *offsetPtr;
    string = Tcl_GetStringFromObj(objv[0], &length);
    c = string[0];
    if ((c == 's') && (strncmp(string, "scroll", length) == 0)) {
        double fract;
        int count;

        if (objc != 3) {
            return TCL_ERROR;
        }
        /* Scroll number unit/page */
        if (Tcl_GetIntFromObj(interp, objv[1], &count) != TCL_OK) {
            return TCL_ERROR;
        }
        string = Tcl_GetStringFromObj(objv[2], &length);
        c = string[0];
        if ((c == 'u') && (strncmp(string, "units", length) == 0)) {
            fract = (double)count *scrollUnits;
        } else if ((c == 'p') && (strncmp(string, "pages", length) == 0)) {
            /* A page is 90% of the view-able window. */
            fract = (double)count * windowSize * 0.9;
        } else {
            Tcl_AppendResult(interp, "unknown \"scroll\" units \"", 
                     Tcl_GetString(objv[2]), "\"", (char *)NULL);
            return TCL_ERROR;
        }
        offset += (int)fract;
    } else if ((c == 'm') && (strncmp(string, "moveto", length) == 0)) {
        double fract;

        if (objc != 2) {
            return TCL_ERROR;
        }
        /* moveto fraction */
        if (Tcl_GetDoubleFromObj(interp, objv[1], &fract) != TCL_OK) {
            return TCL_ERROR;
        }
        offset = (int)(worldSize * fract);
    } else {
        double fract;
        int count;

        /* Treat like "scroll units" */
        if (Tcl_GetIntFromObj(interp, objv[0], &count) != TCL_OK) {
            return TCL_ERROR;
        }
        fract = (double)count *scrollUnits;
        offset += (int)fract;
    }
    *offsetPtr = Blt_AdjustViewport(offset, worldSize, windowSize, scrollUnits,
        scrollMode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_UpdateScrollbar --
 *
 *      Invoke a TCL command to the scrollbar, defining the new position
 *      and length of the scroll. See the Tk documentation for further
 *      information on the scrollbar.  It is assumed the scrollbar command
 *      prefix is valid.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Scrollbar is commanded to change position and/or size.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_UpdateScrollbar(
    Tcl_Interp *interp,
    Tcl_Obj *scrollCmdObjPtr,           /* Scrollbar command prefix. May be
                                         * several words */
    int first, int last, int width)
{
    Tcl_Obj *cmdObjPtr;
    double firstFract, lastFract;

    firstFract = 0.0, lastFract = 1.0;
    if (width > 0) {
        firstFract = (double)first / (double)width;
        lastFract = (double)last / (double)width;
    }
    cmdObjPtr = Tcl_DuplicateObj(scrollCmdObjPtr);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, Tcl_NewDoubleObj(firstFract));
    Tcl_ListObjAppendElement(interp, cmdObjPtr, Tcl_NewDoubleObj(lastFract));
    Tcl_IncrRefCount(cmdObjPtr);
    if (Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL) != TCL_OK) {
        Tcl_BackgroundError(interp);
    }
    Tcl_DecrRefCount(cmdObjPtr);

}

/* -------------------------------------------------------------------------- */
/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPrivateGCFromDrawable --
 *
 *      Like Tk_GetGC, but doesn't share the GC with any other widget.
 *      This is needed because the certain GC parameters (like dashes) can
 *      not be set via XCreateGC, therefore there is no way for Tk's
 *      hashing mechanism to recognize that two such GCs differ.
 *
 * Results:
 *      A new GC is returned.
 *
 *---------------------------------------------------------------------------
 */
GC
Blt_GetPrivateGCFromDrawable(
    Display *display,
    Drawable drawable,
    unsigned long gcMask,
    XGCValues *valuePtr)
{
    GC newGC;

#ifdef WIN32
    newGC = Blt_EmulateXCreateGC(display, drawable, gcMask, valuePtr);
#else
    newGC = XCreateGC(display, drawable, gcMask, valuePtr);
#endif
    return newGC;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPrivateGC --
 *
 *      Like Tk_GetGC, but doesn't share the GC with any other widget.
 *      This is needed because the certain GC parameters (like dashes) can
 *      not be set via XCreateGC, therefore there is no way for Tk's
 *      hashing mechanism to recognize that two such GCs differ.
 *
 * Results:
 *      A new GC is returned.
 *
 *---------------------------------------------------------------------------
 */
GC
Blt_GetPrivateGC(
    Tk_Window tkwin,
    unsigned long gcMask,
    XGCValues *valuePtr)
{
    GC gc;
    Pixmap pixmap;
    Drawable drawable;
    Display *display;

    pixmap = None;
    drawable = Tk_WindowId(tkwin);
    display = Tk_Display(tkwin);
    if (drawable == None) {
        Drawable root;
        int depth;

        root = Tk_RootWindow(tkwin);
        depth = Tk_Depth(tkwin);

        if (depth == DefaultDepth(display, Tk_ScreenNumber(tkwin))) {
            drawable = root;
        } else {
            pixmap = Blt_GetPixmap(display, root, 1, 1, depth);
            drawable = pixmap;
            Blt_SetDrawableAttribs(display, drawable, 1, 1, depth, 
                Tk_Colormap(tkwin), Tk_Visual(tkwin));
        }
    }
    gc = Blt_GetPrivateGCFromDrawable(display, drawable, gcMask, valuePtr);
    if (pixmap != None) {
        Tk_FreePixmap(display, pixmap);
    }
    return gc;
}

void
Blt_FreePrivateGC(Display *display, GC gc)
{
    Tk_FreeXId(display, (XID) XGContextFromGC(gc));
    XFreeGC(display, gc);
}

#ifndef WIN32
void
Blt_SetDashes(Display *display, GC gc, Blt_Dashes *dashesPtr)
{
    XSetDashes(display, gc, dashesPtr->offset, (const char *)dashesPtr->values,
        (int)strlen((char *)dashesPtr->values));
}
#endif

void
Blt_ScreenDPI(Tk_Window tkwin, int *xPtr, int *yPtr) 
{
    Screen *screen;

#define MM_INCH         25.4
    screen = Tk_Screen(tkwin);
    *xPtr = (int)((WidthOfScreen(screen) * MM_INCH)/WidthMMOfScreen(screen));
    *yPtr = (int)((HeightOfScreen(screen) * MM_INCH)/HeightMMOfScreen(screen));
}

void
Blt_DrawSegments2d(
    Display *display,
    Drawable drawable,
    GC gc,
    Segment2d *segments,
    int numSegments)
{
    XSegment *dp, *xsegments;
    Segment2d *sp, *send;

    xsegments = Blt_Malloc(numSegments * sizeof(XSegment));
    if (xsegments == NULL) {
        return;
    }
    dp = xsegments;
    for (sp = segments, send = sp + numSegments; sp < send; sp++) {
        dp->x1 = (short int)sp->p.x;
        dp->y1 = (short int)sp->p.y;
        dp->x2 = (short int)sp->q.x;
        dp->y2 = (short int)sp->q.y;
        dp++;
    }
    XDrawSegments(display, drawable, gc, xsegments, numSegments);
    Blt_Free(xsegments);
}

void
Blt_DrawArrowOld(Display *display, Drawable drawable, GC gc, int x, int y, int w, 
              int h, int borderWidth, int orientation)
{
    XPoint arrow[4];
    int  s2, s;
    int ax, ay;

#define ARROW_IPAD 1
    w -= 2 * (ARROW_IPAD + borderWidth);
    h -= 2 * (ARROW_IPAD + borderWidth);
    x += ARROW_IPAD + borderWidth;
    y += ARROW_IPAD + borderWidth;

    w |= 0x01;
    h |= 0x01;
    s = MIN(w, h);
    s2 = s / 2;
    ax = x + w / 2;
    ay = y + h / 2;

    switch (orientation) {
    case ARROW_UP:
        ay -= s2/2 + 1;
        arrow[2].x = arrow[0].x = ax;
        arrow[2].y = arrow[0].y = ay;
        arrow[0].x = ax + s2 + 1;
        arrow[1].x = ax - s2;
        arrow[0].y = arrow[1].y = ay + s2 + 1;
        fprintf(stderr, "up arrow %d,%d %d,%d %d,%d\n",
                arrow[0].x, arrow[0].y,
                arrow[1].x, arrow[1].y,
                arrow[2].x, arrow[2].y);
        break;
    case ARROW_DOWN:
        ay -= s2/2;
        arrow[3].x = arrow[0].x = ax;
        arrow[3].y = arrow[0].y = ay + s2 + 1;
        arrow[1].x = ax + s2 + 1;
        arrow[2].x = ax - s2;
        arrow[2].y = arrow[1].y = ay;
        fprintf(stderr, "down arrow %d,%d %d,%d %d,%d\n",
                arrow[0].x, arrow[0].y,
                arrow[1].x, arrow[1].y,
                arrow[2].x, arrow[2].y);
        break;
    case ARROW_LEFT:
        ax -= s2 / 2;
        arrow[3].x = arrow[0].x = ax;
        arrow[3].y = arrow[0].y = ay;
        arrow[1].y = ay - s2;
        arrow[2].y = ay + s2 + 1;
        arrow[2].x = arrow[1].x = ax + s2 + 1;
        break;
    case ARROW_RIGHT:
        ax -= s2 / 2;
        arrow[3].x = arrow[0].x = ax + s2 + 1;
        arrow[3].y = arrow[0].y = ay;
        arrow[1].y = ay - s2;
        arrow[2].y = ay + s2;
        arrow[2].x = arrow[1].x = ax;
        break;
    }
    XFillPolygon(display, drawable, gc, arrow, 3, Convex, CoordModeOrigin);
}

void
Blt_DrawArrow(Display *display, Drawable drawable, XColor *color, int x, int y, 
              int w, int h, int borderWidth, int orientation)
{
    int s;
    int s2;
    int ax, ay;
    int dx, dy;
    GC gc;

#define ARROW_IPAD 1
    w -= 2 * (ARROW_IPAD + borderWidth);
    h -= 2 * (ARROW_IPAD + borderWidth);
    x += ARROW_IPAD + borderWidth;
    y += ARROW_IPAD + borderWidth;
    s = 0;                              /* Suppress compiler warning. */
    switch (orientation) {
    case ARROW_UP:
    case ARROW_DOWN:
        s = w;
        break;
    case ARROW_LEFT:
    case ARROW_RIGHT:
        s = h;
        break;
    }
    s2 = (s / 2) + 1;
    ax = x + w / 2;
    ay = y + h / 2;

    gc = Tk_GCForColor(color, drawable);
    switch (orientation) {
    case ARROW_UP:
        ay -= s2 / 2;
        for (dx = 0; dx < s2; dx++, ay++) {
            XDrawLine(display, drawable, gc, ax - dx, ay, ax + dx, ay);
        }
        break;
    case ARROW_DOWN:
        ay += s2 / 2;
        for (dx = 0; dx < s2; dx++, ay--) {
            XDrawLine(display, drawable, gc, ax - dx, ay, ax + dx, ay);
        }
        break;
    case ARROW_LEFT:
        ax -= s2 / 2;
        for (dy = 0; dy < s2; dy++, ax++) {
            XDrawLine(display, drawable, gc, ax, ay - dy, ax, ay + dy);
        }
        break;
    case ARROW_RIGHT:
        ax += s2 / 2;
        for (dy = 0; dy < s2; dy++, ax--) {
            XDrawLine(display, drawable, gc, ax, ay - dy, ax, ay + dy);
        }
        break;
    }
}

long 
Blt_MaxRequestSize(Display *display, size_t elemSize) 
{
    static long maxSizeBytes = 0L;

    if (maxSizeBytes == 0L) {
        long size;
#ifndef WIN32
        size = XExtendedMaxRequestSize(display);
        if (size == 0) {
            size = XMaxRequestSize(display);
        }
#else
        size = XMaxRequestSize(display);
#endif
        size -= (4 * elemSize);
        /*      maxSizeBytes = (size * 4); */
        maxSizeBytes = size;
    }
    return (maxSizeBytes / elemSize);
}

void
Blt_GetLineExtents(size_t numPoints, Point2d *points, Region2d *r)
{
    Point2d *p, *pend;
    r->top = r->left = DBL_MAX;
    r->bottom = r->right = -DBL_MAX;
    for (p = points, pend = p + numPoints; p < pend; p++) {
        if (r->top > p->y) {
            r->top = p->y;
        }
        if (r->bottom < p->y) {
            r->bottom = p->y;
        }
        if (r->left > p->x) {
            r->left = p->x;
        }
        if (r->right < p->x) {
            r->right = p->x;
        }
    }
}

#undef Blt_Fill3DRectangle
void
Blt_Fill3DRectangle(
    Tk_Window tkwin,            /* Window for which border was allocated. */
    Drawable drawable,          /* X window or pixmap in which to draw. */
    Tk_3DBorder border,         /* Token for border to draw. */
    int x, int y, 
    int width, int  height,     /* Outside area of rectangular region. */
    int borderWidth,            /* Desired width for border, in pixels. Border
                                 * will be *inside* region. */
    int relief)                 /* Indicates 3D effect: TK_RELIEF_FLAT,
                                 * TK_RELIEF_RAISED, or TK_RELIEF_SUNKEN. */
{
#ifndef notdef
    if ((borderWidth > 1) && (width > 2) && (height > 2) &&
        ((relief == TK_RELIEF_SUNKEN) || (relief == TK_RELIEF_RAISED))) {
        GC lightGC, darkGC;
        int x2, y2;

        x2 = x + width - 1;
        y2 = y + height - 1;
#define TK_3D_LIGHT2_GC TK_3D_DARK_GC+1
#define TK_3D_DARK2_GC TK_3D_DARK_GC+2
        if (relief == TK_RELIEF_RAISED) {
            lightGC = Tk_3DBorderGC(tkwin, border, TK_3D_FLAT_GC);
            darkGC = Tk_3DBorderGC(tkwin, border, TK_3D_DARK_GC);
        } else {

            lightGC = Tk_3DBorderGC(tkwin, border, TK_3D_LIGHT_GC);
            darkGC = Tk_3DBorderGC(tkwin, border, TK_3D_FLAT_GC);
        }
        XDrawLine(Tk_Display(tkwin), drawable, lightGC, x, y, x2, y);
        XDrawLine(Tk_Display(tkwin), drawable, darkGC, x2, y2, x2, y);
        XDrawLine(Tk_Display(tkwin), drawable, darkGC, x2, y2, x, y2);
        XDrawLine(Tk_Display(tkwin), drawable, lightGC, x, y, x, y2);
        x++, y++, width -= 2, height -= 2, borderWidth--;
    }
#endif
    Tk_Fill3DRectangle(tkwin, drawable, border, x, y, width, height, 
        borderWidth, relief);
}


#undef Blt_Draw3DRectangle
void
Blt_Draw3DRectangle(
    Tk_Window tkwin,            /* Window for which border was allocated. */
    Drawable drawable,          /* X window or pixmap in which to draw. */
    Tk_3DBorder border,         /* Token for border to draw. */
    int x, int y, 
    int width, int height,      /* Outside area of rectangular region. */
    int borderWidth,            /* Desired width for border, in pixels. Border
                                 * will be *inside* region. */
    int relief)                 /* Indicates 3D effect: TK_RELIEF_FLAT,
                                 * TK_RELIEF_RAISED, or TK_RELIEF_SUNKEN. */
{
#ifndef notdef
    if ((borderWidth > 1) && (width > 2) && (height > 2) &&
        ((relief == TK_RELIEF_SUNKEN) || (relief == TK_RELIEF_RAISED))) {
        GC lightGC, darkGC;
        int x2, y2;

        x2 = x + width - 1;
        y2 = y + height - 1;
        if (relief == TK_RELIEF_RAISED) {
            lightGC = Tk_3DBorderGC(tkwin, border, TK_3D_FLAT_GC);
            darkGC = Tk_3DBorderGC(tkwin, border, TK_3D_DARK_GC);
        } else {
            lightGC = Tk_3DBorderGC(tkwin, border, TK_3D_LIGHT_GC);
            darkGC = Tk_3DBorderGC(tkwin, border, TK_3D_FLAT_GC);
        }
        XDrawLine(Tk_Display(tkwin), drawable, darkGC, x2, y2, x2, y);
        XDrawLine(Tk_Display(tkwin), drawable, lightGC, x, y, x2, y);
        XDrawLine(Tk_Display(tkwin), drawable, darkGC, x2, y2, x, y2);
        XDrawLine(Tk_Display(tkwin), drawable, lightGC, x, y, x, y2);
        x++, y++, width -= 2, height -= 2, borderWidth--;
    }
#endif
    Tk_Draw3DRectangle(tkwin, drawable, border, x, y, width, height, 
        borderWidth, relief);
}

#ifdef notdef
typedef struct {
    Screen *screen;
    Visual *visual;
    Colormap colormap;
    Tk_Uid nameUid;
} BorderKey;

typedef struct {
    Screen *screen;             /* Screen on which the border will be used. */
    Visual *visual;             /* Visual for all windows and pixmaps using
                                 * the border. */
    int depth;                  /* Number of bits per pixel of drawables where
                                 * the border will be used. */
    Colormap colormap;          /* Colormap out of which pixels are
                                 * allocated. */
    int refCount;               /* Number of active uses of this color (each
                                 * active use corresponds to a call to
                                 * Blt_Get3DBorder).  If this count is 0, then
                                 * this structure is no longer valid and it
                                 * isn't present in borderTable: it is being
                                 * kept around only because there are objects
                                 * referring to it.  The structure is freed
                                 * when refCount is 0. */

    XColor *bgColorPtr;         /* Color of face. */
    XColor *shadows[4];

    Pixmap darkStipple;         /* Stipple pattern to use for drawing shadows
                                 * areas.  Used for displays with <= 64 colors
                                 * or where colormap has filled up. */
    Pixmap lightStipple;        /* Stipple pattern to use for drawing shadows
                                 * areas.  Used for displays with <= 64 colors
                                 * or where colormap has filled up. */
    GC bgGC;                    /* Used (if necessary) to draw areas in the
                                 * background color. */
    GC lightGC, darkGC;
    Tcl_HashEntry *hashPtr;     /* Entry in borderTable (needed in order to
                                 * delete structure). */

    struct _Blt_3DBorder *nextPtr;
} Border, *Blt_3DBorder;
    

void
Blt_Draw3DRectangle(tkwin, drawable, border, x, y, width,
        height, borderWidth, relief)
    Tk_Window tkwin;            /* Window for which border was allocated. */
    Drawable drawable;          /* X window or pixmap in which to draw. */
    Blt_3DBorder *borderPtr;    /* Border to draw. */
    int x, y, width, height;    /* Outside area of rectangular region. */
    int borderWidth;            /* Desired width for border, in
                                 * pixels. Border will be *inside* region. */
    int relief;                 /* Indicates 3D effect: TK_RELIEF_FLAT,
                                 * TK_RELIEF_RAISED, or TK_RELIEF_SUNKEN. */
{
    if ((width > (2 * borderWidth)) && (height > (2 * borderWidth))) {
        int x2, y2;
        int i;

        x2 = x + width - 1;
        y2 = y + height - 1;

        XSetForeground(borderPtr->lightGC, borderPtr->shadows[0]);
        XSetForeground(borderPtr->darkGC, borderPtr->shadows[3]);
        XDrawLine(Tk_Display(tkwin), drawable, borderPtr->lightGC, 
                  x, y, x2, y);
        XDrawLine(Tk_Display(tkwin), drawable, borderPtr->lightGC, 
                  x, y, x, y2);
        XDrawLine(Tk_Display(tkwin), drawable, borderPtr->darkGC, 
                  x2, y, x2, y2);
        XDrawLine(Tk_Display(tkwin), drawable, borderPtr->darkGC, 
                  x2, y2, x, y2);
        XSetForeground(borderPtr->lightGC, borderPtr->shadows[1]);
        XSetForeground(borderPtr->darkGC, borderPtr->shadows[2]);
        for (i = 1; i < (borderWidth - 1); i++) {

            /*
             *  +----------
             *  |+-------
             *  ||+-----
             *  |||
             *  |||
             *  ||
             *  |
             */
            x++, y++, x2--, y2--;
            XDrawLine(Tk_Display(tkwin), drawable, borderPtr->lightGC, 
                x, y, x2, y);
            XDrawLine(Tk_Display(tkwin), drawable, borderPtr->lightGC, 
                x, y, x, y2);
            XDrawLine(Tk_Display(tkwin), drawable, borderPtr->darkGC, 
                x2, y, x2, y2);
            XDrawLine(Tk_Display(tkwin), drawable, borderPtr->darkGC, 
                x2, y2, x, y2);
        }
    }
}

void
Blt_Fill3DRectangle(tkwin, drawable, border, x, y, width, height, borderWidth, 
        relief)
    Tk_Window tkwin;            /* Window for which border was allocated. */
    Drawable drawable;          /* X window or pixmap in which to draw. */
    Tk_3DBorder border;         /* Token for border to draw. */
    int x, y, width, height;    /* Outside area of rectangular region. */
    int borderWidth;            /* Desired width for border, in
                                 * pixels. Border will be *inside* region. */
    int relief;                 /* Indicates 3D effect: TK_RELIEF_FLAT,
                                 * TK_RELIEF_RAISED, or TK_RELIEF_SUNKEN. */
{
    Blt_3DBorder *borderPtr;

    XFillRectangle(Tk_Display(tkwin), drawable, borderPtr->bgGC, x, y, width,
           height);
    if ((borderWidth > 0) && (relief != BLT_RELIEF_FLAT)) {
        Blt_Draw3DRectangle(tkwin, drawable, borderPtr, x, y, width, height, 
            borderWidth, relief);
    }
}


void 
FreeBorder(display, borderPtr)
    Display *display;
    Border *borderPtr;
{
    int i;

    if (borderPtr->bgColorPtr != NULL) {
        Tk_FreeColor(display, borderPtr->bgColorPtr);
    }
    for (i = 0; i < 4; i++) {
        Tk_FreeColor(display, borderPtr->shadows[i]);
    }
    if (borderPtr->darkGC != NULL) {
        Blt_FreePrivateGC(display, borderPtr->darkGC);
    }
    if (borderPtr->lightGC != NULL) {
        Blt_FreePrivateGC(tkwin, borderPtr->lightGC);
    }
    if (borderPtr->bgGC != NULL) {
        Blt_FreePrivateGC(tkwin, borderPtr->bgGC);
    }
    Blt_Free(borderPtr);
}

void
Blt_Free3DBorder(display, border)
    Display *display;
    Blt_3DBorder border;
{
    Border *borderPtr = (Border *)border;

    borderPtr->refCount--;
    if (borderPtr->refCount >= 0) {
        /* Search for the border in the bucket. Start at the head. */
        headPtr = Blt_GetHashValue(borderPtr->hashPtr);
        lastPtr = NULL;
        while ((headPtr != borderPtr) && (headPtr != NULL)) {
            lastPtr = headPtr;
            headPtr = headPtr->next;
        }
        if (headPtr == NULL) {
            return;             /* This can't happen. It means that we could
                                 * not find the border. */
        }
        if (lastPtr != NULL) {
            lastPtr->next = borderPtr->next;
        } else {
            Tcl_DeleteHashEntry(borderPtr->hashPtr);
        }
        FreeBorder(display, borderPtr);
    }
}

Blt_3DBorder *
Blt_Get3DBorder(Tcl_Interp *interp, Tk_Window tkwin, const char *borderName)
{
    Blt_3DBorder *borderPtr, *lastBorderPtr;
    Blt_HashEntry *hPtr;
    XColor *bgColorPtr;
    char **argv;
    const char *colorName;
    int argc;
    int isNew;

    lastBorderPtr = NULL;
    hPtr = Tcl_CreateHashEntry(&dataPtr->borderTable, borderName, &isNew);
    if (!isNew) {
        borderPtr = lastBorderPtr = Blt_GetHashValue(hPtr);
        while (borderPtr != NULL) {
            if ((Tk_Screen(tkwin) == borderPtr->screen) && 
                (Tk_Colormap(tkwin) == borderPtr->colormap)) {
                borderPtr->refCount++;
                return borderPtr;
            }
            borderPtr = borderPtr->nextPtr;
        }
    }
    /* Create a new border. */
    argv = NULL;
    bgColorPtr = NULL;

    if (Tcl_SplitList(interp, borderName, &argc, &argv) != TCL_OK) {
        goto error;
    }
    colorName = borderName;
    bgColorPtr = Tk_GetColor(interp, tkwin, colorName);
    if (bgColorPtr == NULL) {
        goto error;
    }

    /* Create a new border */
    borderPtr = Blt_AssertCalloc(1, sizeof(Blt_3DBorder));
    borderPtr->screen = Tk_Screen(tkwin);
    borderPtr->visual = Tk_Visual(tkwin);
    borderPtr->depth = Tk_Depth(tkwin);
    borderPtr->colormap = Tk_Colormap(tkwin);
    borderPtr->refCount = 1;
    borderPtr->bgColorPtr = bgColorPtr;
    borderPtr->darkGC = Blt_GetPrivateGC(tkwin, 0, NULL);
    borderPtr->lightGC = Blt_GetPrivateGC(tkwin, 0, NULL);
    borderPtr->hashPtr = lastBorderPtr->hashPtr;
    lastBorderPtr->nextPtr = lastBorderPtr;
    {
        HSV hsv;
        XColor color;
        double sat, sat0, diff, step, hstep;
        int count;
        
        /* Convert the face (background) color to HSV */
        Blt_XColorToHSV(borderPtr->bgColorPtr, &hsv);
        
        /* Using the color as the baseline intensity, pick a set of colors
         * around the intensity. */
#define UFLOOR(x,u)             (floor((x)*(u))/(u))
        diff = hsv.sat - UFLOOR(hsv.sat, 0.2);
        sat = 0.1 + (diff - 0.1);
        sat0 = hsv.sat;
        count = 0;
        for (sat = 0.1 + (diff - 0.1); sat <= 1.0; sat += 0.2) {
            if (FABS(sat0 - sat) >= 0.1) {
                hsv.sat = sat;
                Blt_HSVToXColor(&hsv, &color);
                borderPtr->shadows[count] = Tk_GetColorByValue(tkwin, &color);
                count++;
            }
        }
    }
    Blt_SetHashValue(hPtr, borderPtr);
    if (argv != NULL) {
        Tcl_Free((char *)argv);
    }
    return TCL_OK;

 error:
    if (argv != NULL) {
        Tcl_Free((char *)argv);
    }
    if (bgColorPtr != NULL) {
        Tk_FreeColor(bgColorPtr);
    }
    if (isNew) {
        Blt_DeleteHashEntry(&borderTable, hPtr);
    }
    return NULL;
}


#endif

Pixmap 
Blt_GetPixmapAbortOnError(Display *display, Drawable drawable, int w, int h, 
                          int depth, int lineNum, const char *fileName)
{
    if (w <= 0) {
        Blt_Warn("line %d of %s: width is %d\n", lineNum, fileName, w);
        abort();
    }
    if (h <= 0) {
        Blt_Warn("line %d of %s: height is %d\n", lineNum, fileName, h);
        abort();
    }
    return Tk_GetPixmap(display, drawable, w, h, depth);
}

#ifndef WIN32
#ifdef DEBUGGC
void
Blt_PrintGC(Display *display, GC gc, const char *string)
{
    XGCValues gcValues;
    unsigned long mask;

    mask = (GCFunction | GCPlaneMask | GCForeground | GCBackground | 
            GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle | 
            GCFillStyle | GCFillRule | GCArcMode | GCTile | GCStipple | 
            GCTileStipXOrigin | GCTileStipYOrigin | GCFont | GCSubwindowMode | 
            GCGraphicsExposures | GCClipXOrigin | GCClipYOrigin | 
            GCDashOffset);
    memset(&gcValues, 0, sizeof(gcValues));
    if (XGetGCValues(display, gc, mask, &gcValues) == 0) {
        Blt_Warn("can't get values from gc\n");
    }
    fprintf(stderr, "GC(%s) = %x\n", string, gc);
    fprintf(stderr, "  arc_mode=%d\n",  gcValues.arc_mode);
    fprintf(stderr, "  background=%d\n", gcValues.background);
    fprintf(stderr, "  cap_style=%d\n",  gcValues.cap_style);
    fprintf(stderr, "  clip_x_origin=%d\n", gcValues.clip_x_origin);
    fprintf(stderr, "  clip_y_origin=%d\n", gcValues.clip_y_origin);
    fprintf(stderr, "  dash_offset=%d\n", gcValues.dash_offset);
    fprintf(stderr, "  fill_rule=%d\n",  gcValues.fill_rule);
    fprintf(stderr, "  fill_style=%d\n",  gcValues.fill_style);
    fprintf(stderr, "  font=%d\n", gcValues.font);
    fprintf(stderr, "  foreground=%d\n", gcValues.foreground);
    fprintf(stderr, "  function=%d\n", gcValues.function);
    fprintf(stderr, "  graphics_exposures=%d\n", gcValues.graphics_exposures);
    fprintf(stderr, "  join_style=%d\n",  gcValues.join_style);
    fprintf(stderr, "  line_style=%d\n", gcValues.line_style);
    fprintf(stderr, "  line_width=%d\n", gcValues.line_width);
    fprintf(stderr, "  plane_mask=%d\n", gcValues.plane_mask);
    fprintf(stderr, "  stipple=%d\n", gcValues.stipple);
    fprintf(stderr, "  subwindow_mode=%d\n", gcValues.subwindow_mode);
    fprintf(stderr, "  tile=%d\n", gcValues.tile);
    fprintf(stderr, "  ts_x_origin=%d\n", gcValues.ts_x_origin);
    fprintf(stderr, "  ts_y_origin=%d\n", gcValues.ts_y_origin);
}
#endif /* DEBUGGC*/
#endif  /*WIN32*/

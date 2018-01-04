/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltArcBall.c --
 *
 * This module implements an arcball controller for the BLT toolkit.
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
 * The quaternion implementation is from 
 *
 *   Shoemake, Ken, Arcball Rotation Control, Graphics Gems IV, p. 175-192, 
 *   code: p. 178-191, arcball.
 *
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltMath.h"
#include <bltHash.h>
#include "bltNsUtil.h"
#include <bltOp.h>
#include "bltInitCmd.h"

typedef struct {
    Blt_HashTable arcballTable;         /* Hash table of arcballs keyed by
                                         * address. */
    Tcl_Interp *interp;
} ArcBallCmdInterpData;

#define ARCBALL_THREAD_KEY "BLT Arcball Command Data"

#define DEF_WIDTH       "100"
#define DEF_HEIGHT      "100"

typedef struct {
    double w, x, y, z;
} Quaternion;

typedef struct {
    double x, y, z;
} EulerAngles;

typedef double HMatrix[3][3];

typedef struct {
    Tcl_Interp *interp;
    Blt_HashTable *tablePtr;
    Blt_HashEntry *hashPtr;
    ArcBallCmdInterpData *dataPtr;
    Tcl_Command cmdToken;
    Point3d click, drag;                
    Quaternion q;
    double xScale, yScale;  
    int width, height;
    Tk_Window tkwin;
} ArcBall;

static Blt_ConfigSpec arcSpecs[] =
{
    {BLT_CONFIG_PIXELS_NNEG, "-height", (char *)NULL, (char *)NULL, 
        DEF_HEIGHT, Blt_Offset(ArcBall, height), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-width", (char *)NULL, (char *)NULL, 
        DEF_WIDTH, Blt_Offset(ArcBall, width), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END}
};

static Tcl_ObjCmdProc ArcBallInstObjCmd;
static Tcl_CmdDeleteProc ArcBallInstDeleteProc;

/*
 * Arcball sphere constants:
 * Diameter is       2.0
 * Radius is         1.0
 * Radius squared is 1.0
 */

static int
GetQuaternionFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Quaternion *q)
{
    Tcl_Obj **objv;
    int objc;
    double x, y, z, w;
    
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc != 4) {
        Tcl_AppendResult(interp, "wrong number of elements in quaternion \"",
                         Tcl_GetString(objPtr), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    if ((Tcl_GetDoubleFromObj(interp, objv[0], &w) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[1], &x) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[2], &y) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[3], &z) != TCL_OK)) {
        return TCL_ERROR;
    }
    q->x = x, q->y = y, q->z = z, q->w = w;
    return TCL_OK;
}

static INLINE void
SetIdentity(Quaternion *q) 
{
    q->x = q->y = q->z = 0.0;
    q->w = 1.0;
}

static INLINE double 
Length(Point3d *p)
{
    return sqrt((p->x * p->x) + (p->y * p->y) + (p->z * p->z));
}

static INLINE double 
DotProduct(Point3d *p1, Point3d *p2)
{
    return (p1->x * p2->x) + (p1->y * p2->y) + (p1->z * p2->z);
}

/**
 * Calculate the cross product of two 3D vectors: c = a x b.
 * "c" must not refer to the same memory location as "a" or "b".
 */
static INLINE void 
CrossProduct(Point3d *a, Point3d *b, Point3d *c)
{
    c->x = (a->y * b->z) - (a->z * b->y);
    c->y = (a->z * b->x) - (a->x * b->z);
    c->z = (a->x * b->y) - (a->y * b->x);
}

/* Return quaternion product qL * qR.  Note: order is important!
 * To combine rotations, use the product Mul(Second, First),
 * which gives the effect of rotating by First then Second. */
static void
CombineRotations(Quaternion *q1, Quaternion *q2, Quaternion *r)
{
    r->w = (q1->w*q2->w) - (q1->x*q2->x) - (q1->y*q2->y) - (q1->z*q2->z);
    r->x = (q1->w*q2->x) + (q1->x*q2->w) + (q1->y*q2->z) - (q1->z*q2->y);
    r->y = (q1->w*q2->y) + (q1->y*q2->w) + (q1->z*q2->x) - (q1->x*q2->z);
    r->z = (q1->w*q2->z) + (q1->z*q2->w) + (q1->x*q2->y) - (q1->y*q2->x);
}

static int
GetMatrixFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, HMatrix A)
{
    int objc;
    Tcl_Obj **objv;
    int i, j, k;
    
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc != 9) {
        Tcl_AppendResult(interp, "wrong # of elements in rotation matrix \"",
                Tcl_GetString(objPtr), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    k = 0;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            double x;

            if (Tcl_GetDoubleFromObj(interp, objv[k], &x) != TCL_OK) {
                return TCL_ERROR;
            }
            A[i][j] = x;
            k++;
        }
    }
    return TCL_OK;
}

static void
EulerToQuaternion(EulerAngles *eulerPtr, Quaternion *q)
{
    double x, y, z, w, c1, c2, c3, s1, s2, s3;
    double a1, a2, a3;

    a1 = eulerPtr->x * DEG2RAD;
    a2 = eulerPtr->y * DEG2RAD;
    a3 = eulerPtr->z * DEG2RAD;

    c1 = cos(a1 * 0.5), s1 = sin(a1 * 0.5);
    c2 = cos(a2 * 0.5), s2 = sin(a2 * 0.5);
    c3 = cos(a3 * 0.5), s3 = sin(a3 * 0.5);
    
    w = c1 * c2 * c3 - s1 * s2 * s3;
    x = c1 * c2 * s3 + s1 * s2 * c3;
    y = s1 * c2 * c3 + c1 * s2 * s3;
    z = c1 * s2 * c3 - s1 * c2 * s3;       

    q->w = w;
    q->x = y;
    q->y = z;
    q->z = x;
}

static void
QuaternionToEuler(Quaternion *q, EulerAngles *eulerPtr)
{
    double test, sqx, sqy, sqz, sqw, unit;
    double qx, qy, qz, qw;
    double a1, a2, a3;
    
    qw = q->w;
    qx = q->z;
    qy = q->x;
    qz = q->y;
    
    sqw = qw*qw;
    sqx = qx*qx;
    sqy = qy*qy;
    sqz = qz*qz;

    /* If normalised is one, otherwise is correction factor. */
    unit = sqx + sqy + sqz + sqw; 
    test = qx*qy + qz*qw;
    if (test > (0.49999 * unit)) {        /* Singularity at north pole. */
        eulerPtr->x = 2 * atan2(qx,qw);
        eulerPtr->y = M_PI_2;
        eulerPtr->z = 0;
        return;
    }
    if (test < (-0.49999 * unit)) {       /* Singularity at south pole. */
        eulerPtr->x = -2 * atan2(qx,qw);
        eulerPtr->y = -M_PI_2;
        eulerPtr->z = 0;
        return;
    }

    a1 = atan2(2*qy*qw-2*qx*qz , sqx - sqy - sqz + sqw);
    a2 = asin(2*test/unit);
    a3 = atan2(2*qx*qw-2*qy*qz , -sqx + sqy - sqz + sqw);

    eulerPtr->x = a1 * RAD2DEG;
    eulerPtr->y = a2 * RAD2DEG;
    eulerPtr->z = a3 * RAD2DEG;
}

#ifdef notdef
/**
 * Sets the value of this matrix to the result of multiplying itself
 * with matrix m1. 
 * @param m1 the other matrix 
 */

/** Multiply the upper left 3x3 parts of A and B to get AB **/
static void 
MultiplyMatrices(HMatrix A, HMatrix B, HMatrix AB)
{
    int i;

    for (i = 0; i < 3; i++) {
        int j;

        for (j = 0; j < 3; j++) {
            AB[i][j] = A[i][0]*B[0][j] + A[i][1]*B[1][j] + A[i][2]*B[2][j];
        }
    }
} 

static double 
Matrix4fSVD(HMatrix A) 
{
    // this is a simple svd.
    // Not complete but fast and reasonable.
    // See comment in Matrix3d.
    return sqrt(((A[0][0]*A[0][0]) + (A[1][0]*A[1][0]) + 
                 (A[2][0]*A[2][0]) + (A[0][1]*A[0][1]) + 
                 (A[1][1]*A[1][1]) + (A[2][1]*A[2][1]) +
                 (A[0][2]*A[0][2]) + (A[1][2]*A[1][2]) + 
                 (A[2][2]*A[2][2])) / 3.0);
}

static void 
SetMatrix4x4FromMatrix3x3(HMatrix m1, HMatrix result)
{
    result[0][0] = m1[0][0]; 
    result[0][1] = m1[0][1]; 
    result[0][2] = m1[0][2];
    result[1][0] = m1[1][0]; 
    result[1][1] = m1[1][1]; 
    result[1][2] = m1[1][2];
    result[2][0] = m1[2][0]; 
    result[2][1] = m1[2][1]; 
    result[2][2] = m1[2][2];
}

static void 
ScaleMatrix(HMatrix A, double scale)
{
    A[0][0] *= scale; 
    A[0][1] *= scale; 
    A[0][2] *= scale;
    A[1][0] *= scale; 
    A[1][1] *= scale; 
    A[1][2] *= scale;
    A[2][0] *= scale; 
    A[2][1] *= scale; 
    A[2][2] *= scale;
}

static void 
Matrix4fSetRotationFromMatrix3f(HMatrix A, HMatrix B)
{
    double scale;
    
    scale = Matrix4fSVD(A);
    SetMatrix4x4FromMatrix3x3(B, A);
    ScaleMatrix(A, scale);
}
#endif

static void 
MatrixToQuaternion(HMatrix A, Quaternion *q)
{
    double trace;

   /* This algorithm avoids near-zero divides by looking for a large
    * component - first w, then x, y, or z.  When the trace is greater than
    * zero, |w| is greater than 1/2, which is as small as a largest
    * component can be.  Otherwise, the largest diagonal entry corresponds
    * to the largest of |x|, |y|, or |z|, one of which must be larger than
    * |w|, and at least 1/2. */

    trace = A[0][0] + A[1][1] + A[2][2]; 
    if (trace >= 0.0) {         
        double s;

        s = 0.5 / sqrt(trace + 1.0);  /* A[3][3] = 1.0 */
        q->w = 0.25 / s;
        q->x = (A[2][1] - A[1][2]) * s;
        q->y = (A[0][2] - A[2][0]) * s;
        q->z = (A[1][0] - A[0][1]) * s;

    } else if ((A[0][0] > A[1][1]) && (A[0][0] > A[2][2])) {
        double s;
        
        s = 2.0 * sqrt(A[0][0] - (A[1][1] + A[2][2]) + 1.0);
        q->w = (A[2][1] - A[1][2]) / s;
        q->x = 0.25 * s;
        q->y = (A[0][1] + A[1][0]) / s;
        q->z = (A[0][2] + A[2][0]) / s;
    } else if (A[1][1] > A[2][2]) {
        double s;
        
        s = 2.0 * sqrt(1.0 + A[1][1] - A[0][0] - A[2][2]);
        q->w = (A[0][2] - A[2][0]) / s;
        q->x = (A[0][1] + A[1][0]) / s;
        q->y = 0.25 * s;
        q->z = (A[1][2] + A[2][1]) / s;
    } else {
        double s;
        
        s = 2.0 * sqrt(1.0 + A[2][2] - A[0][0] - A[1][1]);
        q->w = (A[1][0] - A[0][1]) / s;
        q->x = (A[0][2] + A[2][0]) / s;
        q->y = (A[1][2] + A[2][1]) / s;
        q->z = 0.25 * s;
    }
}

static void 
QuaternionToMatrix(Quaternion* q, HMatrix A)
{
    double n, s;
    double xs, ys, zs;
    double wx, wy, wz;
    double xx, xy, xz;
    double yy, yz, zz;

    n = (q->x * q->x) + (q->y * q->y) + (q->z * q->z) + (q->w * q->w);

    s = (n > 0.0) ? (2.0 / n) : 0.0;
    
    xs = q->x * s;  
    ys = q->y * s;  
    zs = q->z * s;
    wx = q->w * xs; 
    wy = q->w * ys; 
    wz = q->w * zs;
    xx = q->x * xs; 
    xy = q->x * ys; 
    xz = q->x * zs;
    yy = q->y * ys; 
    yz = q->y * zs; 
    zz = q->z * zs;
    
    A[0][0] = 1.0 - (yy + zz); 
    A[0][1] = xy - wz;  
    A[0][2] = xz + wy;
    A[1][0] = xy + wz;  
    A[1][1] = 1.0 - (xx + zz); 
    A[1][2] = yz - wx;
    A[2][0] = xz - wy;  
    A[2][1] = yz + wx;  
    A[2][2] = 1.0 - (xx + yy);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetArcBallFromObj --
 *
 *      Find the arcball command associated with the TCL command.
 *      
 *      We have to do multiple lookups to get this right.  
 *
 *      The first step is to generate a canonical command name.  If an
 *      unqualified command name (i.e. no namespace qualifier) is given, we
 *      should search first the current namespace and then the global one.
 *      Most TCL commands (like Tcl_GetCmdInfo) look only at the global
 *      namespace.
 *
 *      Tcl_GetCommandInfo will get us the objClientData field that should
 *      be a cmdPtr.  We can verify that by searching our hashtable of
 *      cmdPtr addresses.
 *
 * Results:
 *      A pointer to the arcball command.  It's up to the calling routines to
 *      generate an error message.
 *
 *---------------------------------------------------------------------------
 */
static ArcBall *
GetArcBallFromObj(ArcBallCmdInterpData *dataPtr, Tcl_Interp *interp, 
                  Tcl_Obj *objPtr)
{
    Blt_HashEntry *hPtr;
    Blt_ObjectName objName;
    Tcl_CmdInfo cmdInfo;
    Tcl_DString ds;
    const char *ballName;
    const char *string;
    int result;

    /* Pull apart the arcball name and put it back together in a standard
     * format. */
    string = Tcl_GetString(objPtr);
    if (!Blt_ParseObjectName(interp, string, &objName, BLT_NO_ERROR_MSG)) {
        return NULL;                    /* No such parent namespace. */
    }
    /* Rebuild the fully qualified name. */
    ballName = Blt_MakeQualifiedName(&objName, &ds);
    result = Tcl_GetCommandInfo(interp, ballName, &cmdInfo);
    Tcl_DStringFree(&ds);
    if (!result) {
        return NULL;
    }
    hPtr = Blt_FindHashEntry(&dataPtr->arcballTable, 
        (const char *)(cmdInfo.objClientData));
    if (hPtr == NULL) {
        return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

static void 
SetArcBallBounds(ArcBall *arcPtr)
{
    if (arcPtr->width <= 1.0 ) {
        arcPtr->width = 2.0;
    }
    if (arcPtr->height <= 1.0 ) {
        arcPtr->height = 2.0;
    }
    /* Set adjustment factor for width/height */
    arcPtr->xScale = 1.0 / ((arcPtr->width - 1.0) * 0.5);
    arcPtr->yScale = 1.0 / ((arcPtr->height - 1.0) * 0.5);
}

static ArcBall *
NewArcBall(Tcl_Interp *interp, ArcBallCmdInterpData *dataPtr, const char *name)
{
    ArcBall *arcPtr;
    int isNew;
    
    arcPtr = Blt_Calloc(1, sizeof(ArcBall));
    arcPtr->width = arcPtr->height = 100;
    arcPtr->dataPtr = dataPtr;
    arcPtr->interp = interp;
    arcPtr->cmdToken = Tcl_CreateObjCommand(interp, (char *)name, 
        ArcBallInstObjCmd, arcPtr, ArcBallInstDeleteProc);
    arcPtr->tablePtr = &dataPtr->arcballTable;
    arcPtr->hashPtr = Blt_CreateHashEntry(arcPtr->tablePtr,
        (char *)arcPtr, &isNew);
    assert(isNew);
    Blt_SetHashValue(arcPtr->hashPtr, arcPtr);
    arcPtr->tkwin = Tk_MainWindow(interp);
    return arcPtr;
}

static void 
PointOnSphere(ArcBall *arcPtr, double x, double y, Point3d *p)
{
    double sx, sy;
    double d2;

    /* Adjust point coords and scale down to range of [-1 ... 1] */
    sx = (x * arcPtr->xScale)  - 1.0;
    sy = 1.0 - (y * arcPtr->yScale);

    /* Compute the square of the length of the vector to the point from the
     * center. */
    d2 = (sx * sx) + (sy * sy);

    /* If the point is mapped outside of the sphere ... 
     * (length > radius squared)
     */
    if (d2 > 1.0) {
        double scale;

        /* Compute a normalizing factor (radius / sqrt(length)) */
        scale = 1.0 / sqrt(d2);

        /* Return the "normalized" vector, a point on the sphere */
        p->x = sx * scale;
        p->y = sy * scale;
        p->z = 0.0;
    } else {                            /* else it's on the inside */
        /* Return a vector to a point mapped inside the sphere
         * sqrt(radius squared - length) */
        p->x = sx;
        p->y = sy;
        p->z = sqrt(1.0 - d2);
    }
}


/* Mouse down: Supply mouse position in x and y */
static void 
ClickArcBall(ArcBall *arcPtr, double x, double y)
{
    PointOnSphere (arcPtr, x, y, &arcPtr->click);
}


/* Mouse drag, calculate rotation: Supply mouse position in x and y */
static void 
DragArcBall(ArcBall *arcPtr, double x, double y, Quaternion *q)
{
    /* Map the point to the sphere. */
    PointOnSphere(arcPtr, x, y, &arcPtr->drag);

    /* Return the quaternion equivalent to the rotation. */
    if (q != NULL) {
        Point3d perp;

        /* Compute the vector perpendicular to the begin and end vectors. */
        CrossProduct(&arcPtr->click, &arcPtr->drag, &perp);

        /* Compute the length of the perpendicular vector. */
        if (Length(&perp) > DBL_EPSILON) {
            /* If its non-zero, we're ok, so return the perpendicular
             * vector as the transform after all. */
            q->x = perp.x;
            q->y = perp.y;
            q->z = perp.z;
            /* In the quaternion values, w is cosine (theta / 2), where theta
             * is rotation angle. */
            q->w = DotProduct(&arcPtr->click, &arcPtr->drag);
        } else {
            /* If it is zero, the begin and end vectors coincide, so return an
             * identity transform. */
            SetIdentity(q);
        }
    }
}

static int
GetEulerAnglesFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, EulerAngles *e)
{
    Tcl_Obj **objv;
    int objc;
    double x, y, z;
    
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc != 3) {
        Tcl_AppendResult(interp, "wrong number of elements in angle list \"",
                         Tcl_GetString(objPtr), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    if ((Tcl_GetDoubleFromObj(interp, objv[0], &x) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[1], &y) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[2], &z) != TCL_OK)) {
        return TCL_ERROR;
    }
    e->x = x, e->y = y, e->z = z;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      arcballName cget option
 *
 *---------------------------------------------------------------------------
 */
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    ArcBall *arcPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, arcPtr->tkwin, arcSpecs,
                (char *)arcPtr, objv[2], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      arcballName configure ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    ArcBall *arcPtr = clientData;

    if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, arcPtr->tkwin, arcSpecs,
                (char *)arcPtr, (Tcl_Obj *)NULL,  BLT_CONFIG_OBJV_ONLY);
    } else if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, arcPtr->tkwin, arcSpecs,
                (char *)arcPtr, objv[2], BLT_CONFIG_OBJV_ONLY);
    }
    if (Blt_ConfigureWidgetFromObj(interp, arcPtr->tkwin, arcSpecs, objc - 2,
        objv + 2, (char *)arcPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    SetArcBallBounds(arcPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EulerOp --
 *
 *      Sets/gets the current quaternion in terms of euler angles.  Note
 *      that this assumes z is up.  The order in which the angles are
 *      applied is z, y, x;
 *
 * Results:
 *      A standard TCL result.  A list of three numbers representing
 *      the euler angles will be returned.
 *
 *      arcName euler "x y z"
 *      set angles [arcName euler]
 *
 *---------------------------------------------------------------------------
 */
static int
EulerOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    ArcBall *arcPtr = clientData;

    if (objc == 3) {
#ifdef notdef
        EulerAngles euler;
        Quaternion q1, q2, q3, q32;
        double a1, a2, a3;

        if (GetEulerAnglesFromObj(interp, objv[2], &euler) != TCL_OK) {
            return TCL_ERROR;
        }

        a1 = euler.x * DEG2RAD;
        a2 = euler.y * DEG2RAD;
        a3 = euler.z * DEG2RAD;

        /* 3 */
        q3.w = cos(a1 * 0.5);
        q3.x = sin(a1 * 0.5);
        q3.y = 0.0;
        q3.z = 0.0;
        /* 2 */
        q2.w = cos(a2 * 0.5);
        q2.x = 0.0;
        q2.y = sin(a2 * 0.5);
        q2.z = 0.0;
        /* 1 */
        q1.w = cos(a3 * 0.5);
        q1.x = 0.0;
        q1.y = 0.0;
        q1.z = sin(a3 * 0.5);

        CombineRotations(&q3, &q2, &q32);
        CombineRotations(&q32, &q1, &arcPtr->q);
#else
        EulerAngles euler;

        if (GetEulerAnglesFromObj(interp, objv[2], &euler) != TCL_OK) {
            return TCL_ERROR;
        }
        EulerToQuaternion(&euler, &arcPtr->q);
#endif
        
    } else {
        EulerAngles euler;
        Tcl_Obj *objPtr, *listObjPtr;

#ifdef notdef
        HMatrix A;
        double phi, cosPhi;
        double x, y, z;

        QuaternionToMatrix(&arcPtr->q, A);
        phi = -asin(A[0][2]);           /* Calculate Y-axis angle */
        cosPhi = cos(phi);
        y = phi * RAD2DEG;
        
        if (fabs(cosPhi) > 0.005) {     /* Gimball lock? */
            double trx, try;

            trx =  A[2][2] / cosPhi;    /* No, so get X-axis angle */
            try = -A[1][2] / cosPhi;
            x  = atan2(try, trx) * RAD2DEG;
            
            trx =  A[0][0] / cosPhi;    /* Get Z-axis angle */
            try = -A[0][1] / cosPhi;
            z  = atan2(try, trx) * RAD2DEG;
        } else {                        /* Gimball lock has occurred */
            double trx, try;

            x = 0.0;                    /* X-axis angle is zero. */
            
            trx = A[1][1];              /* Compute Z-axis angle */
            try = A[1][0];
            z  = atan2(try, trx) * RAD2DEG;
        }
        euler.x = x;
        euler.y = y;
        euler.z = z;
#else 
        QuaternionToEuler(&arcPtr->q, &euler);
#endif
        /* Clamp all angles to range */
#define CLAMP(x)        (((x) < 0.0) ? 0.0 : ((x) > 360.0) ? 360.0 : (x))
        euler.x = CLAMP(euler.x);
        euler.y = CLAMP(euler.y);
        euler.z = CLAMP(euler.z);

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
        objPtr = Tcl_NewDoubleObj(euler.x);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(euler.y);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(euler.z);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MatrixOp --
 *
 *      Sets/gets the rotation matrix from the current quaternion.  The 3x3
 *      rotation matrix is represented by nine numbers (row-major).
 *
 * Results:
 *      A standard TCL result.  A list representing the rotation matrix is
 *      returned.
 *
 *      arcName matrix matrixList
 *      set matrixList [arcName matrix]
 *
 *---------------------------------------------------------------------------
 */
static int
MatrixOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    ArcBall *arcPtr = clientData;

    if (objc == 3) {
        HMatrix A;

        if (GetMatrixFromObj(interp, objv[2], A) != TCL_OK) {
            return TCL_ERROR;
        }
        MatrixToQuaternion(A, &arcPtr->q);
    } else {
        HMatrix A;
        Tcl_Obj *listObjPtr;
        int i, j;

        QuaternionToMatrix(&arcPtr->q, A);
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 3; j++) {
                Tcl_Obj *objPtr;
                
                objPtr = Tcl_NewDoubleObj(A[i][j]);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
        }
        Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * QuaternionOp --
 *
 *      Sets/gets the current quaternion.
 *
 * Results:
 *      A standard TCL result.  A list representing the quaternion is
 *      returned.
 *
 *      arcName quaternion "w x y z"
 *      set q [arcName quaternion]
 *
 *---------------------------------------------------------------------------
 */
static int
QuaternionOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    ArcBall *arcPtr = clientData;

    if (objc == 3) {
        Quaternion q;

        if (GetQuaternionFromObj(interp, objv[2], &q) != TCL_OK) {
            return TCL_ERROR;
        }
        arcPtr->q = q;
    } else {
        Tcl_Obj *objPtr, *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
        objPtr = Tcl_NewDoubleObj(arcPtr->q.w);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(arcPtr->q.x);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(arcPtr->q.y);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(arcPtr->q.z);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetOp --
 *
 *      Resets the quaternion to identity.  Used also the initialize the
 *      quaternion.
 *
 * Results:
 *      A standard TCL result.  Always returns TCL_OK.
 *
 *      arcName reset
 *
 *---------------------------------------------------------------------------
 */
static int
ResetOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    ArcBall *arcPtr = clientData;

    SetIdentity(&arcPtr->q);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ArcBallResizeOp --
 *
 *      Sets new dimensions for the arcball window.
 *
 * Results:
 *      A standard TCL result.
 *
 *      arcName resize w h
 *
 *---------------------------------------------------------------------------
 */
static int
ResizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    ArcBall *arcPtr = clientData;
    int w, h;

    if ((Tcl_GetIntFromObj(interp, objv[2], &w) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[3], &h) != TCL_OK)) {
        return TCL_ERROR;
    }
    if ((w < 1) || (h < 1)) {
        Tcl_AppendResult(interp, "bad screen size ", 
                         Tcl_GetString(objv[2]), " x ", 
                         Tcl_GetString(objv[3]), (char *)NULL);
        return TCL_ERROR;
    }
    arcPtr->width = w;
    arcPtr->height = h;
    SetArcBallBounds(arcPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RotateOp --
 *
 *      Rotates the arcball based upon the starting end ending coordinates
 *      in the window.
 *
 * Results:
 *      A standard TCL result.  A list representing the new rotated
 *      quaternion is returned.
 *
 *      arcName rotate x1 y1 x2 y2
 *
 *---------------------------------------------------------------------------
 */
static int
RotateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    ArcBall *arcPtr = clientData;
    Quaternion q, p;
    Tcl_Obj *objPtr, *listObjPtr;
    double x1, y1, x2, y2;

    if ((Tcl_GetDoubleFromObj(interp, objv[2], &x1) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[3], &y1) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[4], &x2) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[5], &y2) != TCL_OK)) {
        return TCL_ERROR;
    }
    ClickArcBall(arcPtr, x1, y1);
    DragArcBall(arcPtr, x2, y2, &q);
    p = arcPtr->q;
    CombineRotations(&p, &q, &arcPtr->q);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    objPtr = Tcl_NewDoubleObj(arcPtr->q.w);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewDoubleObj(arcPtr->q.x);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewDoubleObj(arcPtr->q.y);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewDoubleObj(arcPtr->q.z);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ArcBallInstObjCmdOp --
 *
 *      This procedure is invoked to process commands on behalf of
 *      the tree object.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec arcBallOps[] =
{
    {"cget",        2, CgetOp,        3, 3, "option",},
    {"configure",   2, ConfigureOp,   2, 0, "?option value ... ?",},
    {"euler",       1, EulerOp,       2, 3, "?angles?",},
    {"matrix",      1, MatrixOp,      2, 3, "?matrix?",},
    {"quaternion",  1, QuaternionOp,  2, 3, "?quat?",},
    {"reset",       3, ResetOp,       2, 2, "",},
    {"resize",      3, ResizeOp,      4, 4, "w h",},
    {"rotate",      2, RotateOp,      6, 6, "x1 y1 x2 y2",},
};
static int numArcBallOps = sizeof(arcBallOps) / sizeof(Blt_OpSpec);

static int
ArcBallInstObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    ArcBall *arcPtr = clientData;
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numArcBallOps, arcBallOps, BLT_OP_ARG1, 
             objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    Tcl_Preserve(arcPtr);
    result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(arcPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ArcBallInstDeleteProc --
 *
 *      Deletes the command associated with the arcball.  This is called
 *      only when the command associated with the arcball is destroyed.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
ArcBallInstDeleteProc(ClientData clientData)
{
    ArcBall *arcPtr = clientData;

    if (arcPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(arcPtr->tablePtr, arcPtr->hashPtr);
    }
    Blt_Free(arcPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * GenerateName --
 *
 *      Generates an unique arcball command name.  Arcbal names are in the
 *      form "arcballN", where N is a non-negative integer. Check each name
 *      generated to see if it is already a command. We want to recycle
 *      names if possible.
 *      
 * Results:
 *      Returns the unique name.  The string itself is stored in the dynamic
 *      string passed into the routine.
 *
 *---------------------------------------------------------------------------
 */
static const char *
GenerateName(Tcl_Interp *interp, const char *prefix, const char *suffix,
             Tcl_DString *resultPtr)
{

    int i;
    const char *ballName;

    /* 
     * Parse the command and put back so that it's in a consistent format.
     *
     *  t1         <current namespace>::t1
     *  n1::t1     <current namespace>::n1::t1
     *  ::t1       ::t1
     *  ::n1::t1   ::n1::t1
     */
    ballName = NULL;                    /* Suppress compiler warning. */
    for (i = 0; i < INT_MAX; i++) {
        Blt_ObjectName objName;
        Tcl_DString ds;
        char string[200];

        Tcl_DStringInit(&ds);
        Tcl_DStringAppend(&ds, prefix, -1);
        Blt_FmtString(string, 200, "arcball%d", i);
        Tcl_DStringAppend(&ds, string, -1);
        Tcl_DStringAppend(&ds, suffix, -1);
        if (!Blt_ParseObjectName(interp, Tcl_DStringValue(&ds), &objName, 0)) {
            Tcl_DStringFree(&ds);
            return NULL;
        }
        ballName = Blt_MakeQualifiedName(&objName, resultPtr);
        Tcl_DStringFree(&ds);

        if (Blt_CommandExists(interp, ballName)) {
            continue;                   /* A command by this name already
                                         * exists. */
        }
        break;
    }
    return ballName;
}

/*
 *---------------------------------------------------------------------------
 *
 * ArcBallCreateOp --
 *
 *      Creates new instance of an arcball.
 *
 *      blt::arcball create ?arcName? w h 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ArcBallCreateOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    ArcBallCmdInterpData *dataPtr = clientData;
    const char *name;
    Tcl_DString ds;
    ArcBall *arcPtr;
    
    name = NULL;
    if (objc == 3) {
        name = Tcl_GetString(objv[2]);
    }
    Tcl_DStringInit(&ds);
    if (name == NULL) {
        name = GenerateName(interp, "", "", &ds);
    } else {
        char *p;

        p = strstr(name, "#auto");
        if (p != NULL) {
            *p = '\0';
            name = GenerateName(interp, name, p + 5, &ds);
            *p = '#';
        } else {
            Blt_ObjectName objName;

            if (!Blt_ParseObjectName(interp, name, &objName, 0)) {
                return TCL_ERROR;
            }
            name = Blt_MakeQualifiedName(&objName, &ds);
            /* Check if the command already exists. */
            if (Blt_CommandExists(interp, name)) {
                Tcl_AppendResult(interp, "a command \"", name,
                                 "\" already exists", (char *)NULL);
                goto error;
            }
        } 
    } 
    if (name != NULL) {
        arcPtr = NewArcBall(interp, dataPtr, name);
        assert(arcPtr);
        if (Blt_ConfigureWidgetFromObj(interp, arcPtr->tkwin, arcSpecs,
                objc - 2, objv + 2, (char *)arcPtr, 0) != TCL_OK) {
            return TCL_ERROR;
        }
        SetArcBallBounds(arcPtr);
        Tcl_SetStringObj(Tcl_GetObjResult(interp), name, -1);
        Tcl_DStringFree(&ds);
        return TCL_OK;
    }
 error:
    Tcl_DStringFree(&ds);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * ArcBallDestroyOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ArcBallDestroyOp(ClientData clientData, Tcl_Interp *interp, int objc,
                 Tcl_Obj *const *objv)
{
    ArcBallCmdInterpData *dataPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
        ArcBall *arcPtr;

        arcPtr = GetArcBallFromObj(dataPtr, interp, objv[i]);
        if (arcPtr == NULL) {
            Tcl_AppendResult(interp, "can't find an arcball named \"", 
                             Tcl_GetString(objv[i]), "\"", (char *)NULL);
            return TCL_ERROR;
        }
        Tcl_DeleteCommandFromToken(interp, arcPtr->cmdToken);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ArcBallNamesOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ArcBallNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    ArcBallCmdInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    Tcl_Obj *listObjPtr;
    Tcl_DString ds;

    Tcl_DStringInit(&ds);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&dataPtr->arcballTable, &cursor);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        ArcBall *arcPtr;
        Blt_ObjectName objName;
        Tcl_Obj *objPtr;
        const char *qualName;
        
        arcPtr = Blt_GetHashValue(hPtr);
        objName.name = Tcl_GetCommandName(interp, arcPtr->cmdToken);
        objName.nsPtr = Blt_GetCommandNamespace(arcPtr->cmdToken);
        qualName = Blt_MakeQualifiedName(&objName, &ds);
        if (objc == 3) {
            if (!Tcl_StringMatch(qualName, Tcl_GetString(objv[2]))) {
                continue;
            }
        }
        objPtr = Tcl_NewStringObj(qualName, -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    Tcl_DStringFree(&ds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ArcBallCmd --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec arcBallCmdOps[] =
{
    {"create",  1, ArcBallCreateOp,  4, 0, "?name? ?option value ...?",},
    {"destroy", 1, ArcBallDestroyOp, 3, 0, "name...",},
    {"names",   1, ArcBallNamesOp,   2, 3, "?pattern?...",},
};

static int numArcBallCmdOps = sizeof(arcBallCmdOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
ArcBallObjCmd(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numArcBallCmdOps, arcBallCmdOps, 
        BLT_OP_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * ArcballInterpDeleteProc --
 *
 *      This is called when the interpreter hosting the "arcball" command
 *      is deleted.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Removes the hash table managing all arcballs.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ArcBallInterpDeleteProc(ClientData clientData, Tcl_Interp *interp)
{
    ArcBallCmdInterpData *dataPtr = clientData;

    /* All arcball instances should already have been destroyed when their
     * respective TCL commands were deleted. */
    Blt_DeleteHashTable(&dataPtr->arcballTable);
    Tcl_DeleteAssocData(interp, ARCBALL_THREAD_KEY);
    Blt_Free(dataPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * GetArcBallCmdInterpData --
 *
 *---------------------------------------------------------------------------
 */
static ArcBallCmdInterpData *
GetArcBallCmdInterpData(Tcl_Interp *interp)
{
    ArcBallCmdInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (ArcBallCmdInterpData *)
        Tcl_GetAssocData(interp, ARCBALL_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
        dataPtr = Blt_Malloc(sizeof(ArcBallCmdInterpData));
        assert(dataPtr);
        dataPtr->interp = interp;
        Tcl_SetAssocData(interp, ARCBALL_THREAD_KEY, ArcBallInterpDeleteProc,
                 dataPtr);
        Blt_InitHashTable(&dataPtr->arcballTable, BLT_ONE_WORD_KEYS);
    }
    return dataPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ArcBallCmdInitProc --
 *
 *      This procedure is invoked to initialize the "arcball" command.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Creates the new command and adds a new entry into a global Tcl
 *      associative array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ArcBallCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { 
        "arcball", ArcBallObjCmd,
    };

    cmdSpec.clientData = GetArcBallCmdInterpData(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

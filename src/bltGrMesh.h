/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltGrMesh.h --
 *
 * This module implements contour elements for the BLT graph widget.
 *
 *	Copyright 2011 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use, copy,
 *	modify, merge, publish, distribute, sublicense, and/or sell copies
 *	of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
 */

typedef struct {
    int a, b, c;			/* Indices of the vectices that 
					 * form the triangle. */
} MeshTriangle;

typedef struct _DataSource DataSource;
typedef struct _MeshClass MeshClass;
typedef struct _MeshCmdInterpData MeshCmdInterpData;

typedef struct {
    const char *name;			/* Mesh identifier. */
    MeshClass *classPtr;
    MeshCmdInterpData *dataPtr;
    Tcl_Interp *interp;
    unsigned int flags;			/* Indicates if the mesh element is
					 * active or normal */
    Blt_HashEntry *hashPtr;

    DataSource *x, *y;

    /* Resulting mesh is a triangular grid.  */
    Point2d *vertices;
    int numVertices;
    int *hull;				/* Array of indices pointing into
					 * the mesh representing the convex
					 * hull of the mesh. */
    int numHullPts;
    float xMin, yMin, xMax, yMax;
    MeshTriangle *triangles;		/* Array of triangles. */
    MeshTriangle *reqTriangles;         /* User-requested triangles. */
    int numReqTriangles;
    int numTriangles;			/* # of triangles in array. */
    Blt_HashTable notifierTable;
    Blt_HashTable hideTable;
    Blt_HashTable tableTable;
} Mesh;

typedef void (MeshNotifyProc) (Mesh *meshPtr, ClientData clientData, 
			       unsigned int flags);

#define MESH_CHANGE_NOTIFY	(1<<0)
#define MESH_DELETE_NOTIFY	(1<<1)

BLT_EXTERN Tcl_ObjCmdProc Blt_MeshOp;

BLT_EXTERN void Blt_FreeMesh(Mesh *meshPtr);
BLT_EXTERN int Blt_GetMeshFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	Mesh **meshPtrPtr);
BLT_EXTERN int Blt_GetMesh(Tcl_Interp *interp, const char *string, 
	Mesh **meshPtrPtr);
BLT_EXTERN int Blt_Triangulate(Tcl_Interp *interp, size_t numPoints, 
	Point2d *points, int sorted, MeshTriangle *triangles);
BLT_EXTERN void Blt_Mesh_CreateNotifier(Mesh *meshPtr, MeshNotifyProc *proc, 
	ClientData clientData);
BLT_EXTERN void Blt_Mesh_DeleteNotifier(Mesh *meshPtr, ClientData clientData);
BLT_EXTERN const char *Blt_Mesh_Name(Mesh *meshPtr);
BLT_EXTERN const int Blt_Mesh_Type(Mesh *meshPtr);

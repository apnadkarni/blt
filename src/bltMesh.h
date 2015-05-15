/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltMesh.h --
 *
 * This module implements 2D meshes.
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

typedef struct {
    int a, b, c;                        /* Indices of the vectices that 
                                         * form the triangle. */
} MeshTriangle;

typedef struct _DataSource DataSource;
typedef struct _MeshClass MeshClass;
typedef struct _MeshCmdInterpData MeshCmdInterpData;

typedef struct {
    const char *name;                   /* Mesh identifier. */
    MeshClass *classPtr;
    MeshCmdInterpData *dataPtr;
    Tcl_Interp *interp;
    unsigned int flags;                 /* Indicates if the mesh element is
                                         * active or normal */
    Blt_HashEntry *hashPtr;

    DataSource *x, *y;

    /* Resulting mesh is a triangular grid.  */
    Point2d *vertices;
    int numVertices;
    int *hull;                          /* Array of indices pointing into
                                         * the mesh representing the convex
                                         * hull of the mesh. */
    int numHullPts;
    float xMin, yMin, xMax, yMax;
    MeshTriangle *triangles;            /* Array of triangles. */
    MeshTriangle *reqTriangles;         /* User-requested triangles. */
    int numReqTriangles;
    int numTriangles;                   /* # of triangles in array. */
    Blt_HashTable notifierTable;
    Blt_HashTable hideTable;
    Blt_HashTable tableTable;
} Mesh;

typedef void (MeshNotifyProc) (Mesh *meshPtr, ClientData clientData, 
                               unsigned int flags);

#define MESH_CHANGE_NOTIFY      (1<<0)
#define MESH_DELETE_NOTIFY      (1<<1)

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

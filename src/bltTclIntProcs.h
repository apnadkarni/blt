/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "bltArrayObj.h"
#include "bltAssert.h"
#include "bltDBuffer.h"
#include "bltGeomUtil.h"
#include "bltInitCmd.h"
#include "bltInt.h"
#include "bltMath.h"
#include "bltMesh.h"
#include "bltNsUtil.h"
#include "bltOp.h"
#include "bltSpline.h"
#include "bltSwitch.h"
#include "bltTclInt.h"
#include "bltVar.h"

/* !BEGIN!: Do not edit below this line. */

/*
 * Exported function declarations:
 */

/* Slot 0 is reserved */
#ifndef Blt_GetArrayFromObj_DECLARED
#define Blt_GetArrayFromObj_DECLARED
/* 1 */
BLT_EXTERN int		Blt_GetArrayFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_HashTable **tablePtrPtr);
#endif
#ifndef Blt_NewArrayObj_DECLARED
#define Blt_NewArrayObj_DECLARED
/* 2 */
BLT_EXTERN Tcl_Obj *	Blt_NewArrayObj(int objc, Tcl_Obj *objv[]);
#endif
#ifndef Blt_IsArrayObj_DECLARED
#define Blt_IsArrayObj_DECLARED
/* 3 */
BLT_EXTERN int		Blt_IsArrayObj(Tcl_Obj *obj);
#endif
#ifndef Blt_Assert_DECLARED
#define Blt_Assert_DECLARED
/* 4 */
BLT_EXTERN void		Blt_Assert(const char *expr, const char *file,
				int line);
#endif
#ifndef Blt_DBuffer_VarAppend_DECLARED
#define Blt_DBuffer_VarAppend_DECLARED
/* 5 */
BLT_EXTERN void		Blt_DBuffer_VarAppend(Blt_DBuffer buffer, ...);
#endif
#ifndef Blt_DBuffer_Format_DECLARED
#define Blt_DBuffer_Format_DECLARED
/* 6 */
BLT_EXTERN int		Blt_DBuffer_Format(Blt_DBuffer buffer,
				const char *fmt, ...);
#endif
#ifndef Blt_DBuffer_Init_DECLARED
#define Blt_DBuffer_Init_DECLARED
/* 7 */
BLT_EXTERN void		Blt_DBuffer_Init(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_Free_DECLARED
#define Blt_DBuffer_Free_DECLARED
/* 8 */
BLT_EXTERN void		Blt_DBuffer_Free(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_Extend_DECLARED
#define Blt_DBuffer_Extend_DECLARED
/* 9 */
BLT_EXTERN unsigned char * Blt_DBuffer_Extend(Blt_DBuffer buffer,
				size_t extra);
#endif
#ifndef Blt_DBuffer_AppendData_DECLARED
#define Blt_DBuffer_AppendData_DECLARED
/* 10 */
BLT_EXTERN int		Blt_DBuffer_AppendData(Blt_DBuffer buffer,
				const unsigned char *bytes, size_t extra);
#endif
#ifndef Blt_DBuffer_AppendString_DECLARED
#define Blt_DBuffer_AppendString_DECLARED
/* 11 */
BLT_EXTERN int		Blt_DBuffer_AppendString(Blt_DBuffer buffer,
				const char *string, int length);
#endif
#ifndef Blt_DBuffer_DeleteData_DECLARED
#define Blt_DBuffer_DeleteData_DECLARED
/* 12 */
BLT_EXTERN int		Blt_DBuffer_DeleteData(Blt_DBuffer buffer,
				size_t index, size_t numBytes);
#endif
#ifndef Blt_DBuffer_InsertData_DECLARED
#define Blt_DBuffer_InsertData_DECLARED
/* 13 */
BLT_EXTERN int		Blt_DBuffer_InsertData(Blt_DBuffer buffer,
				const unsigned char *bytes, size_t extra,
				size_t index);
#endif
#ifndef Blt_DBuffer_SetFromObj_DECLARED
#define Blt_DBuffer_SetFromObj_DECLARED
/* 14 */
BLT_EXTERN unsigned char * Blt_DBuffer_SetFromObj(Blt_DBuffer buffer,
				Tcl_Obj *objPtr);
#endif
#ifndef Blt_DBuffer_Concat_DECLARED
#define Blt_DBuffer_Concat_DECLARED
/* 15 */
BLT_EXTERN int		Blt_DBuffer_Concat(Blt_DBuffer dest, Blt_DBuffer src);
#endif
#ifndef Blt_DBuffer_Resize_DECLARED
#define Blt_DBuffer_Resize_DECLARED
/* 16 */
BLT_EXTERN int		Blt_DBuffer_Resize(Blt_DBuffer buffer, size_t length);
#endif
#ifndef Blt_DBuffer_SetLength_DECLARED
#define Blt_DBuffer_SetLength_DECLARED
/* 17 */
BLT_EXTERN int		Blt_DBuffer_SetLength(Blt_DBuffer buffer,
				size_t length);
#endif
#ifndef Blt_DBuffer_Create_DECLARED
#define Blt_DBuffer_Create_DECLARED
/* 18 */
BLT_EXTERN Blt_DBuffer	Blt_DBuffer_Create(void );
#endif
#ifndef Blt_DBuffer_Destroy_DECLARED
#define Blt_DBuffer_Destroy_DECLARED
/* 19 */
BLT_EXTERN void		Blt_DBuffer_Destroy(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_LoadFile_DECLARED
#define Blt_DBuffer_LoadFile_DECLARED
/* 20 */
BLT_EXTERN int		Blt_DBuffer_LoadFile(Tcl_Interp *interp,
				const char *fileName, Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_SaveFile_DECLARED
#define Blt_DBuffer_SaveFile_DECLARED
/* 21 */
BLT_EXTERN int		Blt_DBuffer_SaveFile(Tcl_Interp *interp,
				const char *fileName, Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_AppendByte_DECLARED
#define Blt_DBuffer_AppendByte_DECLARED
/* 22 */
BLT_EXTERN void		Blt_DBuffer_AppendByte(Blt_DBuffer buffer,
				unsigned char byte);
#endif
#ifndef Blt_DBuffer_AppendShort_DECLARED
#define Blt_DBuffer_AppendShort_DECLARED
/* 23 */
BLT_EXTERN void		Blt_DBuffer_AppendShort(Blt_DBuffer buffer,
				unsigned short value);
#endif
#ifndef Blt_DBuffer_AppendInt_DECLARED
#define Blt_DBuffer_AppendInt_DECLARED
/* 24 */
BLT_EXTERN void		Blt_DBuffer_AppendInt(Blt_DBuffer buffer,
				unsigned int value);
#endif
#ifndef Blt_DBuffer_ByteArrayObj_DECLARED
#define Blt_DBuffer_ByteArrayObj_DECLARED
/* 25 */
BLT_EXTERN Tcl_Obj *	Blt_DBuffer_ByteArrayObj(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_StringObj_DECLARED
#define Blt_DBuffer_StringObj_DECLARED
/* 26 */
BLT_EXTERN Tcl_Obj *	Blt_DBuffer_StringObj(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_String_DECLARED
#define Blt_DBuffer_String_DECLARED
/* 27 */
BLT_EXTERN const char *	 Blt_DBuffer_String(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_Base64Decode_DECLARED
#define Blt_DBuffer_Base64Decode_DECLARED
/* 28 */
BLT_EXTERN int		Blt_DBuffer_Base64Decode(Tcl_Interp *interp,
				const char *string, size_t length,
				Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_Base64EncodeToObj_DECLARED
#define Blt_DBuffer_Base64EncodeToObj_DECLARED
/* 29 */
BLT_EXTERN Tcl_Obj *	Blt_DBuffer_Base64EncodeToObj(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_AppendBase85_DECLARED
#define Blt_DBuffer_AppendBase85_DECLARED
/* 30 */
BLT_EXTERN int		Blt_DBuffer_AppendBase85(Blt_DBuffer buffer,
				const unsigned char *bytes, size_t numBytes);
#endif
#ifndef Blt_DBuffer_AppendBase64_DECLARED
#define Blt_DBuffer_AppendBase64_DECLARED
/* 31 */
BLT_EXTERN int		Blt_DBuffer_AppendBase64(Blt_DBuffer buffer,
				const unsigned char *bytes, size_t numBytes);
#endif
#ifndef Blt_SimplifyLine_DECLARED
#define Blt_SimplifyLine_DECLARED
/* 32 */
BLT_EXTERN long		Blt_SimplifyLine(Point2d *origPts, long low,
				long high, double tolerance, long *indices);
#endif
#ifndef Blt_LineRectClip_DECLARED
#define Blt_LineRectClip_DECLARED
/* 33 */
BLT_EXTERN int		Blt_LineRectClip(Region2d *regionPtr, Point2d *p,
				Point2d *q);
#endif
#ifndef Blt_PointInPolygon_DECLARED
#define Blt_PointInPolygon_DECLARED
/* 34 */
BLT_EXTERN int		Blt_PointInPolygon(Point2d *samplePtr,
				Point2d *points, int numPoints);
#endif
#ifndef Blt_PolygonInRegion_DECLARED
#define Blt_PolygonInRegion_DECLARED
/* 35 */
BLT_EXTERN int		Blt_PolygonInRegion(Point2d *points, int numPoints,
				Region2d *extsPtr, int enclosed);
#endif
#ifndef Blt_PointInSegments_DECLARED
#define Blt_PointInSegments_DECLARED
/* 36 */
BLT_EXTERN int		Blt_PointInSegments(Point2d *samplePtr,
				Segment2d *segments, int numSegments,
				double halo);
#endif
#ifndef Blt_PolyRectClip_DECLARED
#define Blt_PolyRectClip_DECLARED
/* 37 */
BLT_EXTERN int		Blt_PolyRectClip(Region2d *extsPtr,
				Point2d *inputPts, int numInputPts,
				Point2d *outputPts);
#endif
#ifndef Blt_GetProjection_DECLARED
#define Blt_GetProjection_DECLARED
/* 38 */
BLT_EXTERN Point2d	Blt_GetProjection(double x, double y, Point2d *p,
				Point2d *q);
#endif
#ifndef Blt_GetProjection2_DECLARED
#define Blt_GetProjection2_DECLARED
/* 39 */
BLT_EXTERN Point2d	Blt_GetProjection2(double x, double y, double x1,
				double y1, double x2, double y2);
#endif
#ifndef Blt_ConvexHull_DECLARED
#define Blt_ConvexHull_DECLARED
/* 40 */
BLT_EXTERN int *	Blt_ConvexHull(int numPoints, Point2d *points,
				int *numHullPtsPtr);
#endif
#ifndef Blt_InitCmd_DECLARED
#define Blt_InitCmd_DECLARED
/* 41 */
BLT_EXTERN int		Blt_InitCmd(Tcl_Interp *interp,
				const char *namespace, Blt_CmdSpec *specPtr);
#endif
#ifndef Blt_InitCmds_DECLARED
#define Blt_InitCmds_DECLARED
/* 42 */
BLT_EXTERN int		Blt_InitCmds(Tcl_Interp *interp,
				const char *namespace, Blt_CmdSpec *specPtr,
				int numCmds);
#endif
#ifndef Blt_FreeMesh_DECLARED
#define Blt_FreeMesh_DECLARED
/* 43 */
BLT_EXTERN void		Blt_FreeMesh(Blt_Mesh mesh);
#endif
#ifndef Blt_GetMeshFromObj_DECLARED
#define Blt_GetMeshFromObj_DECLARED
/* 44 */
BLT_EXTERN int		Blt_GetMeshFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_Mesh *meshPtr);
#endif
#ifndef Blt_GetMesh_DECLARED
#define Blt_GetMesh_DECLARED
/* 45 */
BLT_EXTERN int		Blt_GetMesh(Tcl_Interp *interp, const char *string,
				Blt_Mesh *meshPtr);
#endif
#ifndef Blt_Triangulate_DECLARED
#define Blt_Triangulate_DECLARED
/* 46 */
BLT_EXTERN int		Blt_Triangulate(Tcl_Interp *interp, int numPoints,
				Point2d *points, int sorted,
				Blt_MeshTriangle *triangles);
#endif
#ifndef Blt_Mesh_CreateNotifier_DECLARED
#define Blt_Mesh_CreateNotifier_DECLARED
/* 47 */
BLT_EXTERN void		Blt_Mesh_CreateNotifier(Blt_Mesh mesh,
				Blt_MeshChangedProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Mesh_DeleteNotifier_DECLARED
#define Blt_Mesh_DeleteNotifier_DECLARED
/* 48 */
BLT_EXTERN void		Blt_Mesh_DeleteNotifier(Blt_Mesh mesh,
				Blt_MeshChangedProc *proc,
				ClientData clientData);
#endif
#ifndef Blt_Mesh_Name_DECLARED
#define Blt_Mesh_Name_DECLARED
/* 49 */
BLT_EXTERN const char *	 Blt_Mesh_Name(Blt_Mesh mesh);
#endif
#ifndef Blt_Mesh_Type_DECLARED
#define Blt_Mesh_Type_DECLARED
/* 50 */
BLT_EXTERN int		Blt_Mesh_Type(Blt_Mesh mesh);
#endif
#ifndef Blt_Mesh_GetVertices_DECLARED
#define Blt_Mesh_GetVertices_DECLARED
/* 51 */
BLT_EXTERN Point2d *	Blt_Mesh_GetVertices(Blt_Mesh mesh,
				int *numVerticesPtr);
#endif
#ifndef Blt_Mesh_GetHull_DECLARED
#define Blt_Mesh_GetHull_DECLARED
/* 52 */
BLT_EXTERN int *	Blt_Mesh_GetHull(Blt_Mesh mesh, int *numHullPtsPtr);
#endif
#ifndef Blt_Mesh_GetExtents_DECLARED
#define Blt_Mesh_GetExtents_DECLARED
/* 53 */
BLT_EXTERN void		Blt_Mesh_GetExtents(Blt_Mesh mesh, float *x1Ptr,
				float *y1Ptr, float *x2Ptr, float *y2Ptr);
#endif
#ifndef Blt_Mesh_GetTriangles_DECLARED
#define Blt_Mesh_GetTriangles_DECLARED
/* 54 */
BLT_EXTERN Blt_MeshTriangle * Blt_Mesh_GetTriangles(Blt_Mesh mesh,
				int *numTrianglesPtr);
#endif
#ifndef Blt_GetVariableNamespace_DECLARED
#define Blt_GetVariableNamespace_DECLARED
/* 55 */
BLT_EXTERN Tcl_Namespace * Blt_GetVariableNamespace(Tcl_Interp *interp,
				const char *varName);
#endif
#ifndef Blt_GetCommandNamespace_DECLARED
#define Blt_GetCommandNamespace_DECLARED
/* 56 */
BLT_EXTERN Tcl_Namespace * Blt_GetCommandNamespace(Tcl_Command cmdToken);
#endif
#ifndef Blt_EnterNamespace_DECLARED
#define Blt_EnterNamespace_DECLARED
/* 57 */
BLT_EXTERN Tcl_CallFrame * Blt_EnterNamespace(Tcl_Interp *interp,
				Tcl_Namespace *nsPtr);
#endif
#ifndef Blt_LeaveNamespace_DECLARED
#define Blt_LeaveNamespace_DECLARED
/* 58 */
BLT_EXTERN void		Blt_LeaveNamespace(Tcl_Interp *interp,
				Tcl_CallFrame *framePtr);
#endif
#ifndef Blt_ParseObjectName_DECLARED
#define Blt_ParseObjectName_DECLARED
/* 59 */
BLT_EXTERN int		Blt_ParseObjectName(Tcl_Interp *interp,
				const char *name, Blt_ObjectName *objNamePtr,
				unsigned int flags);
#endif
#ifndef Blt_MakeQualifiedName_DECLARED
#define Blt_MakeQualifiedName_DECLARED
/* 60 */
BLT_EXTERN const char *	 Blt_MakeQualifiedName(Blt_ObjectName *objNamePtr,
				Tcl_DString *resultPtr);
#endif
#ifndef Blt_CommandExists_DECLARED
#define Blt_CommandExists_DECLARED
/* 61 */
BLT_EXTERN int		Blt_CommandExists(Tcl_Interp *interp,
				const char *string);
#endif
#ifndef Blt_GetOpFromObj_DECLARED
#define Blt_GetOpFromObj_DECLARED
/* 62 */
BLT_EXTERN void *	Blt_GetOpFromObj(Tcl_Interp *interp, int numSpecs,
				Blt_OpSpec *specs, int operPos, int objc,
				Tcl_Obj *const *objv, int flags);
#endif
#ifndef Blt_CreateSpline_DECLARED
#define Blt_CreateSpline_DECLARED
/* 63 */
BLT_EXTERN Blt_Spline	Blt_CreateSpline(Point2d *points, int n, int type);
#endif
#ifndef Blt_EvaluateSpline_DECLARED
#define Blt_EvaluateSpline_DECLARED
/* 64 */
BLT_EXTERN Point2d	Blt_EvaluateSpline(Blt_Spline spline, int index,
				double x);
#endif
#ifndef Blt_FreeSpline_DECLARED
#define Blt_FreeSpline_DECLARED
/* 65 */
BLT_EXTERN void		Blt_FreeSpline(Blt_Spline spline);
#endif
#ifndef Blt_CreateParametricCubicSpline_DECLARED
#define Blt_CreateParametricCubicSpline_DECLARED
/* 66 */
BLT_EXTERN Blt_Spline	Blt_CreateParametricCubicSpline(Point2d *points,
				int n, int w, int h);
#endif
#ifndef Blt_EvaluateParametricCubicSpline_DECLARED
#define Blt_EvaluateParametricCubicSpline_DECLARED
/* 67 */
BLT_EXTERN Point2d	Blt_EvaluateParametricCubicSpline(Blt_Spline spline,
				int index, double x);
#endif
#ifndef Blt_FreeParametricCubicSpline_DECLARED
#define Blt_FreeParametricCubicSpline_DECLARED
/* 68 */
BLT_EXTERN void		Blt_FreeParametricCubicSpline(Blt_Spline spline);
#endif
#ifndef Blt_CreateCatromSpline_DECLARED
#define Blt_CreateCatromSpline_DECLARED
/* 69 */
BLT_EXTERN Blt_Spline	Blt_CreateCatromSpline(Point2d *points, int n);
#endif
#ifndef Blt_EvaluateCatromSpline_DECLARED
#define Blt_EvaluateCatromSpline_DECLARED
/* 70 */
BLT_EXTERN Point2d	Blt_EvaluateCatromSpline(Blt_Spline spline, int i,
				double t);
#endif
#ifndef Blt_FreeCatromSpline_DECLARED
#define Blt_FreeCatromSpline_DECLARED
/* 71 */
BLT_EXTERN void		Blt_FreeCatromSpline(Blt_Spline spline);
#endif
#ifndef Blt_ComputeNaturalSpline_DECLARED
#define Blt_ComputeNaturalSpline_DECLARED
/* 72 */
BLT_EXTERN int		Blt_ComputeNaturalSpline(Point2d *origPts,
				int numOrigPts, Point2d *intpPts,
				int numIntpPts);
#endif
#ifndef Blt_ComputeQuadraticSpline_DECLARED
#define Blt_ComputeQuadraticSpline_DECLARED
/* 73 */
BLT_EXTERN int		Blt_ComputeQuadraticSpline(Point2d *origPts,
				int numOrigPts, Point2d *intpPts,
				int numIntpPts);
#endif
#ifndef Blt_ComputeNaturalParametricSpline_DECLARED
#define Blt_ComputeNaturalParametricSpline_DECLARED
/* 74 */
BLT_EXTERN int		Blt_ComputeNaturalParametricSpline(Point2d *origPts,
				int numOrigPts, Region2d *extsPtr,
				int isClosed, Point2d *intpPts,
				int numIntpPts);
#endif
#ifndef Blt_ComputeCatromParametricSpline_DECLARED
#define Blt_ComputeCatromParametricSpline_DECLARED
/* 75 */
BLT_EXTERN int		Blt_ComputeCatromParametricSpline(Point2d *origPts,
				int numOrigPts, Point2d *intpPts,
				int numIntpPts);
#endif
#ifndef Blt_ExprDoubleFromObj_DECLARED
#define Blt_ExprDoubleFromObj_DECLARED
/* 76 */
BLT_EXTERN int		Blt_ExprDoubleFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, double *valuePtr);
#endif
#ifndef Blt_ExprIntFromObj_DECLARED
#define Blt_ExprIntFromObj_DECLARED
/* 77 */
BLT_EXTERN int		Blt_ExprIntFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *valuePtr);
#endif
#ifndef Blt_GetStateFromObj_DECLARED
#define Blt_GetStateFromObj_DECLARED
/* 78 */
BLT_EXTERN int		Blt_GetStateFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *statePtr);
#endif
#ifndef Blt_NameOfState_DECLARED
#define Blt_NameOfState_DECLARED
/* 79 */
BLT_EXTERN const char *	 Blt_NameOfState(int state);
#endif
#ifndef Blt_GetFillFromObj_DECLARED
#define Blt_GetFillFromObj_DECLARED
/* 80 */
BLT_EXTERN int		Blt_GetFillFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *fillPtr);
#endif
#ifndef Blt_NameOfFill_DECLARED
#define Blt_NameOfFill_DECLARED
/* 81 */
BLT_EXTERN const char *	 Blt_NameOfFill(int fill);
#endif
#ifndef Blt_GetResizeFromObj_DECLARED
#define Blt_GetResizeFromObj_DECLARED
/* 82 */
BLT_EXTERN int		Blt_GetResizeFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *fillPtr);
#endif
#ifndef Blt_NameOfResize_DECLARED
#define Blt_NameOfResize_DECLARED
/* 83 */
BLT_EXTERN const char *	 Blt_NameOfResize(int resize);
#endif
#ifndef Blt_GetSideFromObj_DECLARED
#define Blt_GetSideFromObj_DECLARED
/* 84 */
BLT_EXTERN int		Blt_GetSideFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *sidePtr);
#endif
#ifndef Blt_NameOfSide_DECLARED
#define Blt_NameOfSide_DECLARED
/* 85 */
BLT_EXTERN const char *	 Blt_NameOfSide(int side);
#endif
#ifndef Blt_GetCount_DECLARED
#define Blt_GetCount_DECLARED
/* 86 */
BLT_EXTERN int		Blt_GetCount(Tcl_Interp *interp, const char *string,
				int check, long *countPtr);
#endif
#ifndef Blt_GetCountFromObj_DECLARED
#define Blt_GetCountFromObj_DECLARED
/* 87 */
BLT_EXTERN int		Blt_GetCountFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int check, long *countPtr);
#endif
#ifndef Blt_ParseSwitches_DECLARED
#define Blt_ParseSwitches_DECLARED
/* 88 */
BLT_EXTERN int		Blt_ParseSwitches(Tcl_Interp *interp,
				Blt_SwitchSpec *specPtr, int objc,
				Tcl_Obj *const *objv, void *rec, int flags);
#endif
#ifndef Blt_FreeSwitches_DECLARED
#define Blt_FreeSwitches_DECLARED
/* 89 */
BLT_EXTERN void		Blt_FreeSwitches(Blt_SwitchSpec *specs, void *rec,
				int flags);
#endif
#ifndef Blt_SwitchChanged_DECLARED
#define Blt_SwitchChanged_DECLARED
/* 90 */
BLT_EXTERN int		Blt_SwitchChanged(Blt_SwitchSpec *specs, ...);
#endif
#ifndef Blt_SwitchInfo_DECLARED
#define Blt_SwitchInfo_DECLARED
/* 91 */
BLT_EXTERN int		Blt_SwitchInfo(Tcl_Interp *interp,
				Blt_SwitchSpec *specs, void *record,
				Tcl_Obj *objPtr, int flags);
#endif
#ifndef Blt_SwitchValue_DECLARED
#define Blt_SwitchValue_DECLARED
/* 92 */
BLT_EXTERN int		Blt_SwitchValue(Tcl_Interp *interp,
				Blt_SwitchSpec *specs, void *record,
				Tcl_Obj *objPtr, int flags);
#endif
#ifndef Blt_Malloc_DECLARED
#define Blt_Malloc_DECLARED
/* 93 */
BLT_EXTERN void *	Blt_Malloc(size_t size);
#endif
#ifndef Blt_Realloc_DECLARED
#define Blt_Realloc_DECLARED
/* 94 */
BLT_EXTERN void *	Blt_Realloc(void *ptr, size_t size);
#endif
#ifndef Blt_Free_DECLARED
#define Blt_Free_DECLARED
/* 95 */
BLT_EXTERN void		Blt_Free(const void *ptr);
#endif
#ifndef Blt_Calloc_DECLARED
#define Blt_Calloc_DECLARED
/* 96 */
BLT_EXTERN void *	Blt_Calloc(size_t numElem, size_t size);
#endif
#ifndef Blt_Strdup_DECLARED
#define Blt_Strdup_DECLARED
/* 97 */
BLT_EXTERN const char *	 Blt_Strdup(const char *string);
#endif
#ifndef Blt_Strndup_DECLARED
#define Blt_Strndup_DECLARED
/* 98 */
BLT_EXTERN const char *	 Blt_Strndup(const char *string, size_t size);
#endif
#ifndef Blt_MallocAbortOnError_DECLARED
#define Blt_MallocAbortOnError_DECLARED
/* 99 */
BLT_EXTERN void *	Blt_MallocAbortOnError(size_t size, const char *file,
				int line);
#endif
#ifndef Blt_CallocAbortOnError_DECLARED
#define Blt_CallocAbortOnError_DECLARED
/* 100 */
BLT_EXTERN void *	Blt_CallocAbortOnError(size_t numElem, size_t size,
				const char *file, int line);
#endif
#ifndef Blt_ReallocAbortOnError_DECLARED
#define Blt_ReallocAbortOnError_DECLARED
/* 101 */
BLT_EXTERN void *	Blt_ReallocAbortOnError(void *ptr, size_t size,
				const char *file, int line);
#endif
#ifndef Blt_StrdupAbortOnError_DECLARED
#define Blt_StrdupAbortOnError_DECLARED
/* 102 */
BLT_EXTERN const char *	 Blt_StrdupAbortOnError(const char *ptr,
				const char *file, int line);
#endif
#ifndef Blt_StrndupAbortOnError_DECLARED
#define Blt_StrndupAbortOnError_DECLARED
/* 103 */
BLT_EXTERN const char *	 Blt_StrndupAbortOnError(const char *ptr,
				size_t size, const char *file, int line);
#endif
#ifndef Blt_DictionaryCompare_DECLARED
#define Blt_DictionaryCompare_DECLARED
/* 104 */
BLT_EXTERN int		Blt_DictionaryCompare(const char *s1, const char *s2);
#endif
#ifndef Blt_GetUid_DECLARED
#define Blt_GetUid_DECLARED
/* 105 */
BLT_EXTERN Blt_Uid	Blt_GetUid(const char *string);
#endif
#ifndef Blt_FreeUid_DECLARED
#define Blt_FreeUid_DECLARED
/* 106 */
BLT_EXTERN void		Blt_FreeUid(Blt_Uid uid);
#endif
#ifndef Blt_FindUid_DECLARED
#define Blt_FindUid_DECLARED
/* 107 */
BLT_EXTERN Blt_Uid	Blt_FindUid(const char *string);
#endif
#ifndef Blt_CreatePipeline_DECLARED
#define Blt_CreatePipeline_DECLARED
/* 108 */
BLT_EXTERN int		Blt_CreatePipeline(Tcl_Interp *interp, int objc,
				Tcl_Obj *const *objv, Blt_Pid **pidArrayPtr,
				void *inPipePtr, void *outPipePtr,
				void *errPipePtr, char *const *env);
#endif
#ifndef Blt_DetachPids_DECLARED
#define Blt_DetachPids_DECLARED
/* 109 */
BLT_EXTERN void		Blt_DetachPids(int numPids, Blt_Pid *pids);
#endif
#ifndef Blt_InitHexTable_DECLARED
#define Blt_InitHexTable_DECLARED
/* 110 */
BLT_EXTERN void		Blt_InitHexTable(unsigned char *table);
#endif
#ifndef Blt_DStringAppendElements_DECLARED
#define Blt_DStringAppendElements_DECLARED
/* 111 */
BLT_EXTERN void		Blt_DStringAppendElements(Tcl_DString *dsPtr, ...);
#endif
#ifndef Blt_LoadLibrary_DECLARED
#define Blt_LoadLibrary_DECLARED
/* 112 */
BLT_EXTERN int		Blt_LoadLibrary(Tcl_Interp *interp,
				const char *libPath,
				const char *initProcName,
				const char *safeProcName);
#endif
#ifndef Blt_Panic_DECLARED
#define Blt_Panic_DECLARED
/* 113 */
BLT_EXTERN void		Blt_Panic(const char *fmt, ...);
#endif
#ifndef Blt_Warn_DECLARED
#define Blt_Warn_DECLARED
/* 114 */
BLT_EXTERN void		Blt_Warn(const char *fmt, ...);
#endif
#ifndef Blt_OpenFile_DECLARED
#define Blt_OpenFile_DECLARED
/* 115 */
BLT_EXTERN FILE *	Blt_OpenFile(Tcl_Interp *interp,
				const char *fileName, const char *mode);
#endif
#ifndef Blt_Itoa_DECLARED
#define Blt_Itoa_DECLARED
/* 116 */
BLT_EXTERN const char *	 Blt_Itoa(int value);
#endif
#ifndef Blt_Ltoa_DECLARED
#define Blt_Ltoa_DECLARED
/* 117 */
BLT_EXTERN const char *	 Blt_Ltoa(long value);
#endif
#ifndef Blt_Utoa_DECLARED
#define Blt_Utoa_DECLARED
/* 118 */
BLT_EXTERN const char *	 Blt_Utoa(unsigned int value);
#endif
#ifndef Blt_Dtoa_DECLARED
#define Blt_Dtoa_DECLARED
/* 119 */
BLT_EXTERN const char *	 Blt_Dtoa(Tcl_Interp *interp, double value);
#endif
#ifndef Blt_DecodeHexadecimal_DECLARED
#define Blt_DecodeHexadecimal_DECLARED
/* 120 */
BLT_EXTERN int		Blt_DecodeHexadecimal(Tcl_Interp *interp,
				const char *src, size_t numChars,
				unsigned char *dest, size_t *numBytesPtr,
				BinaryDecoder *switchesPtr);
#endif
#ifndef Blt_DecodeBase64_DECLARED
#define Blt_DecodeBase64_DECLARED
/* 121 */
BLT_EXTERN int		Blt_DecodeBase64(Tcl_Interp *interp, const char *src,
				size_t numChars, unsigned char *dest,
				size_t *numBytesPtr,
				BinaryDecoder *switchesPtr);
#endif
#ifndef Blt_DecodeBase85_DECLARED
#define Blt_DecodeBase85_DECLARED
/* 122 */
BLT_EXTERN int		Blt_DecodeBase85(Tcl_Interp *interp, const char *src,
				size_t numChars, unsigned char *dest,
				size_t *numBytesPtr,
				BinaryDecoder *switchesPtr);
#endif
#ifndef Blt_DecodeAscii85_DECLARED
#define Blt_DecodeAscii85_DECLARED
/* 123 */
BLT_EXTERN int		Blt_DecodeAscii85(Tcl_Interp *interp,
				const char *src, size_t numChars,
				unsigned char *dest, size_t *numBytesPtr,
				BinaryDecoder *switchesPtr);
#endif
#ifndef Blt_DecodeBase64ToBuffer_DECLARED
#define Blt_DecodeBase64ToBuffer_DECLARED
/* 124 */
BLT_EXTERN Blt_DBuffer	Blt_DecodeBase64ToBuffer(Tcl_Interp *interp,
				const char *src, size_t numChars);
#endif
#ifndef Blt_DecodeHexadecimalToObj_DECLARED
#define Blt_DecodeHexadecimalToObj_DECLARED
/* 125 */
BLT_EXTERN Tcl_Obj *	Blt_DecodeHexadecimalToObj(Tcl_Interp *interp,
				const char *src, size_t numChars);
#endif
#ifndef Blt_DecodeBase64ToObj_DECLARED
#define Blt_DecodeBase64ToObj_DECLARED
/* 126 */
BLT_EXTERN Tcl_Obj *	Blt_DecodeBase64ToObj(Tcl_Interp *interp,
				const char *src, size_t numChars);
#endif
#ifndef Blt_DecodeBase85ToObj_DECLARED
#define Blt_DecodeBase85ToObj_DECLARED
/* 127 */
BLT_EXTERN Tcl_Obj *	Blt_DecodeBase85ToObj(Tcl_Interp *interp,
				const char *src, size_t numChars);
#endif
#ifndef Blt_EncodeHexadecimal_DECLARED
#define Blt_EncodeHexadecimal_DECLARED
/* 128 */
BLT_EXTERN int		Blt_EncodeHexadecimal(const unsigned char *src,
				size_t numBytes, char *dest,
				size_t *numCharsPtr,
				BinaryEncoder *switchesPtr);
#endif
#ifndef Blt_EncodeBase64_DECLARED
#define Blt_EncodeBase64_DECLARED
/* 129 */
BLT_EXTERN int		Blt_EncodeBase64(const unsigned char *src,
				size_t numBytes, char *dest,
				size_t *numCharsPtr,
				BinaryEncoder *switchesPtr);
#endif
#ifndef Blt_EncodeBase85_DECLARED
#define Blt_EncodeBase85_DECLARED
/* 130 */
BLT_EXTERN int		Blt_EncodeBase85(const unsigned char *src,
				size_t numBytes, char *dest,
				size_t *numCharsPtr,
				BinaryEncoder *switchesPtr);
#endif
#ifndef Blt_EncodeAscii85_DECLARED
#define Blt_EncodeAscii85_DECLARED
/* 131 */
BLT_EXTERN int		Blt_EncodeAscii85(const unsigned char *src,
				size_t numBytes, char *dest,
				size_t *numCharsPtr,
				BinaryEncoder *switchesPtr);
#endif
#ifndef Blt_EncodeHexadecimalToObj_DECLARED
#define Blt_EncodeHexadecimalToObj_DECLARED
/* 132 */
BLT_EXTERN Tcl_Obj *	Blt_EncodeHexadecimalToObj(const unsigned char *src,
				size_t numBytes);
#endif
#ifndef Blt_EncodeBase64ToObj_DECLARED
#define Blt_EncodeBase64ToObj_DECLARED
/* 133 */
BLT_EXTERN Tcl_Obj *	Blt_EncodeBase64ToObj(const unsigned char *src,
				size_t numBytes);
#endif
#ifndef Blt_EncodeBase85ToObj_DECLARED
#define Blt_EncodeBase85ToObj_DECLARED
/* 134 */
BLT_EXTERN Tcl_Obj *	Blt_EncodeBase85ToObj(const unsigned char *src,
				size_t numBytes);
#endif
#ifndef Blt_HexadecimalDecodeBufferSize_DECLARED
#define Blt_HexadecimalDecodeBufferSize_DECLARED
/* 135 */
BLT_EXTERN size_t	Blt_HexadecimalDecodeBufferSize(size_t numBytes,
				BinaryDecoder *switchesPtr);
#endif
#ifndef Blt_HexadecimalEncodeBufferSize_DECLARED
#define Blt_HexadecimalEncodeBufferSize_DECLARED
/* 136 */
BLT_EXTERN size_t	Blt_HexadecimalEncodeBufferSize(size_t numChars,
				BinaryEncoder *switchesPtr);
#endif
#ifndef Blt_Base64DecodeBufferSize_DECLARED
#define Blt_Base64DecodeBufferSize_DECLARED
/* 137 */
BLT_EXTERN size_t	Blt_Base64DecodeBufferSize(size_t numBytes,
				BinaryDecoder *switchesPtr);
#endif
#ifndef Blt_Base64EncodeBufferSize_DECLARED
#define Blt_Base64EncodeBufferSize_DECLARED
/* 138 */
BLT_EXTERN size_t	Blt_Base64EncodeBufferSize(size_t numChars,
				BinaryEncoder *switchesPtr);
#endif
#ifndef Blt_Base85DecodeBufferSize_DECLARED
#define Blt_Base85DecodeBufferSize_DECLARED
/* 139 */
BLT_EXTERN size_t	Blt_Base85DecodeBufferSize(size_t numBytes,
				BinaryDecoder *switchesPtr);
#endif
#ifndef Blt_Base85EncodeBufferSize_DECLARED
#define Blt_Base85EncodeBufferSize_DECLARED
/* 140 */
BLT_EXTERN size_t	Blt_Base85EncodeBufferSize(size_t numChars,
				BinaryEncoder *switchesPtr);
#endif
#ifndef Blt_Ascii85DecodeBufferSize_DECLARED
#define Blt_Ascii85DecodeBufferSize_DECLARED
/* 141 */
BLT_EXTERN size_t	Blt_Ascii85DecodeBufferSize(size_t numBytes,
				BinaryDecoder *switchesPtr);
#endif
#ifndef Blt_Ascii85EncodeBufferSize_DECLARED
#define Blt_Ascii85EncodeBufferSize_DECLARED
/* 142 */
BLT_EXTERN size_t	Blt_Ascii85EncodeBufferSize(size_t numChars,
				BinaryEncoder *switchesPtr);
#endif
#ifndef Blt_IsBase64_DECLARED
#define Blt_IsBase64_DECLARED
/* 143 */
BLT_EXTERN int		Blt_IsBase64(const char *buf, size_t length);
#endif
#ifndef Blt_GetTimeFromObj_DECLARED
#define Blt_GetTimeFromObj_DECLARED
/* 144 */
BLT_EXTERN int		Blt_GetTimeFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, double *secondsPtr);
#endif
#ifndef Blt_GetTime_DECLARED
#define Blt_GetTime_DECLARED
/* 145 */
BLT_EXTERN int		Blt_GetTime(Tcl_Interp *interp, const char *string,
				double *secondsPtr);
#endif
#ifndef Blt_SecondsToDate_DECLARED
#define Blt_SecondsToDate_DECLARED
/* 146 */
BLT_EXTERN void		Blt_SecondsToDate(double seconds,
				Blt_DateTime *datePtr);
#endif
#ifndef Blt_DateToSeconds_DECLARED
#define Blt_DateToSeconds_DECLARED
/* 147 */
BLT_EXTERN void		Blt_DateToSeconds(Blt_DateTime *datePtr,
				double *secondsPtr);
#endif
#ifndef Blt_FormatDate_DECLARED
#define Blt_FormatDate_DECLARED
/* 148 */
BLT_EXTERN void		Blt_FormatDate(Blt_DateTime *datePtr,
				const char *format, Tcl_DString *resultPtr);
#endif
#ifndef Blt_GetPositionFromObj_DECLARED
#define Blt_GetPositionFromObj_DECLARED
/* 149 */
BLT_EXTERN int		Blt_GetPositionFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, long *indexPtr);
#endif
#ifndef Blt_ObjIsInteger_DECLARED
#define Blt_ObjIsInteger_DECLARED
/* 150 */
BLT_EXTERN int		Blt_ObjIsInteger(Tcl_Obj *objPtr);
#endif
#ifndef Blt_GetLong_DECLARED
#define Blt_GetLong_DECLARED
/* 151 */
BLT_EXTERN int		Blt_GetLong(Tcl_Interp *interp, const char *s,
				long *valuePtr);
#endif
#ifndef Blt_GetLongFromObj_DECLARED
#define Blt_GetLongFromObj_DECLARED
/* 152 */
BLT_EXTERN int		Blt_GetLongFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, long *valuePtr);
#endif
#ifndef Blt_SetLongObj_DECLARED
#define Blt_SetLongObj_DECLARED
/* 153 */
BLT_EXTERN int		Blt_SetLongObj(Tcl_Obj *objPtr, long value);
#endif
#ifndef Blt_NewLongObj_DECLARED
#define Blt_NewLongObj_DECLARED
/* 154 */
BLT_EXTERN Tcl_Obj *	Blt_NewLongObj(long value);
#endif
#ifndef Blt_IsLongObj_DECLARED
#define Blt_IsLongObj_DECLARED
/* 155 */
BLT_EXTERN int		Blt_IsLongObj(Tcl_Obj *objPtr);
#endif
#ifndef Blt_GetUnsignedLong_DECLARED
#define Blt_GetUnsignedLong_DECLARED
/* 156 */
BLT_EXTERN int		Blt_GetUnsignedLong(Tcl_Interp *interp,
				const char *s, unsigned long *valuePtr);
#endif
#ifndef Blt_GetUnsignedLongFromObj_DECLARED
#define Blt_GetUnsignedLongFromObj_DECLARED
/* 157 */
BLT_EXTERN int		Blt_GetUnsignedLongFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, unsigned long *valuePtr);
#endif
#ifndef Blt_SetUnsignedLongObj_DECLARED
#define Blt_SetUnsignedLongObj_DECLARED
/* 158 */
BLT_EXTERN int		Blt_SetUnsignedLongObj(Tcl_Obj *objPtr,
				unsigned long value);
#endif
#ifndef Blt_NewUnsignedLongObj_DECLARED
#define Blt_NewUnsignedLongObj_DECLARED
/* 159 */
BLT_EXTERN Tcl_Obj *	Blt_NewUnsignedLongObj(unsigned long value);
#endif
#ifndef Blt_IsUnsignedLongObj_DECLARED
#define Blt_IsUnsignedLongObj_DECLARED
/* 160 */
BLT_EXTERN int		Blt_IsUnsignedLongObj(Tcl_Obj *objPtr);
#endif
#ifndef Blt_GetInt64_DECLARED
#define Blt_GetInt64_DECLARED
/* 161 */
BLT_EXTERN int		Blt_GetInt64(Tcl_Interp *interp, const char *s,
				int64_t *valuePtr);
#endif
#ifndef Blt_GetInt64FromObj_DECLARED
#define Blt_GetInt64FromObj_DECLARED
/* 162 */
BLT_EXTERN int		Blt_GetInt64FromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int64_t *valuePtr);
#endif
#ifndef Blt_SetInt64Obj_DECLARED
#define Blt_SetInt64Obj_DECLARED
/* 163 */
BLT_EXTERN int		Blt_SetInt64Obj(Tcl_Obj *objPtr, int64_t value);
#endif
#ifndef Blt_NewInt64Obj_DECLARED
#define Blt_NewInt64Obj_DECLARED
/* 164 */
BLT_EXTERN Tcl_Obj *	Blt_NewInt64Obj(int64_t value);
#endif
#ifndef Blt_IsInt64Obj_DECLARED
#define Blt_IsInt64Obj_DECLARED
/* 165 */
BLT_EXTERN int		Blt_IsInt64Obj(Tcl_Obj *objPtr);
#endif
#ifndef Blt_GetDouble_DECLARED
#define Blt_GetDouble_DECLARED
/* 166 */
BLT_EXTERN int		Blt_GetDouble(Tcl_Interp *interp, const char *s,
				double *valuePtr);
#endif
#ifndef Blt_GetDoubleFromObj_DECLARED
#define Blt_GetDoubleFromObj_DECLARED
/* 167 */
BLT_EXTERN int		Blt_GetDoubleFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, double *valuePtr);
#endif
#ifndef Blt_SetDoubleObj_DECLARED
#define Blt_SetDoubleObj_DECLARED
/* 168 */
BLT_EXTERN int		Blt_SetDoubleObj(Tcl_Obj *objPtr, double value);
#endif
#ifndef Blt_NewDoubleObj_DECLARED
#define Blt_NewDoubleObj_DECLARED
/* 169 */
BLT_EXTERN Tcl_Obj *	Blt_NewDoubleObj(double value);
#endif
#ifndef Blt_IsDoubleObj_DECLARED
#define Blt_IsDoubleObj_DECLARED
/* 170 */
BLT_EXTERN int		Blt_IsDoubleObj(Tcl_Obj *objPtr);
#endif
#ifndef Blt_FmtString_DECLARED
#define Blt_FmtString_DECLARED
/* 171 */
BLT_EXTERN int		Blt_FmtString(char *s, size_t size, const char *fmt, ...);
#endif
#ifndef Blt_LowerCase_DECLARED
#define Blt_LowerCase_DECLARED
/* 172 */
BLT_EXTERN void		Blt_LowerCase(char *s);
#endif
#ifndef Blt_UpperCase_DECLARED
#define Blt_UpperCase_DECLARED
/* 173 */
BLT_EXTERN void		Blt_UpperCase(char *s);
#endif
#ifndef Blt_GetPlatformId_DECLARED
#define Blt_GetPlatformId_DECLARED
/* 174 */
BLT_EXTERN int		Blt_GetPlatformId(void );
#endif
#ifndef Blt_LastError_DECLARED
#define Blt_LastError_DECLARED
/* 175 */
BLT_EXTERN const char *	 Blt_LastError(void );
#endif
#ifndef Blt_NaN_DECLARED
#define Blt_NaN_DECLARED
/* 176 */
BLT_EXTERN double	Blt_NaN(void );
#endif
#ifndef Blt_AlmostEquals_DECLARED
#define Blt_AlmostEquals_DECLARED
/* 177 */
BLT_EXTERN int		Blt_AlmostEquals(double x, double y);
#endif
#ifndef Blt_ConvertListToList_DECLARED
#define Blt_ConvertListToList_DECLARED
/* 178 */
BLT_EXTERN const char ** Blt_ConvertListToList(int argc, const char **argv);
#endif
#ifndef Blt_RegisterObjTypes_DECLARED
#define Blt_RegisterObjTypes_DECLARED
/* 179 */
BLT_EXTERN void		Blt_RegisterObjTypes(void );
#endif
#ifndef Blt_GetCachedVar_DECLARED
#define Blt_GetCachedVar_DECLARED
/* 180 */
BLT_EXTERN Tcl_Var	Blt_GetCachedVar(Blt_HashTable *tablePtr,
				const char *label, Tcl_Obj *objPtr);
#endif
#ifndef Blt_FreeCachedVars_DECLARED
#define Blt_FreeCachedVars_DECLARED
/* 181 */
BLT_EXTERN void		Blt_FreeCachedVars(Blt_HashTable *tablePtr);
#endif

typedef struct BltTclIntProcs {
    int magic;
    struct BltTclIntStubHooks *hooks;

    void *reserved0;
    int (*blt_GetArrayFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_HashTable **tablePtrPtr); /* 1 */
    Tcl_Obj * (*blt_NewArrayObj) (int objc, Tcl_Obj *objv[]); /* 2 */
    int (*blt_IsArrayObj) (Tcl_Obj *obj); /* 3 */
    void (*blt_Assert) (const char *expr, const char *file, int line); /* 4 */
    void (*blt_DBuffer_VarAppend) (Blt_DBuffer buffer, ...); /* 5 */
    int (*blt_DBuffer_Format) (Blt_DBuffer buffer, const char *fmt, ...); /* 6 */
    void (*blt_DBuffer_Init) (Blt_DBuffer buffer); /* 7 */
    void (*blt_DBuffer_Free) (Blt_DBuffer buffer); /* 8 */
    unsigned char * (*blt_DBuffer_Extend) (Blt_DBuffer buffer, size_t extra); /* 9 */
    int (*blt_DBuffer_AppendData) (Blt_DBuffer buffer, const unsigned char *bytes, size_t extra); /* 10 */
    int (*blt_DBuffer_AppendString) (Blt_DBuffer buffer, const char *string, int length); /* 11 */
    int (*blt_DBuffer_DeleteData) (Blt_DBuffer buffer, size_t index, size_t numBytes); /* 12 */
    int (*blt_DBuffer_InsertData) (Blt_DBuffer buffer, const unsigned char *bytes, size_t extra, size_t index); /* 13 */
    unsigned char * (*blt_DBuffer_SetFromObj) (Blt_DBuffer buffer, Tcl_Obj *objPtr); /* 14 */
    int (*blt_DBuffer_Concat) (Blt_DBuffer dest, Blt_DBuffer src); /* 15 */
    int (*blt_DBuffer_Resize) (Blt_DBuffer buffer, size_t length); /* 16 */
    int (*blt_DBuffer_SetLength) (Blt_DBuffer buffer, size_t length); /* 17 */
    Blt_DBuffer (*blt_DBuffer_Create) (void); /* 18 */
    void (*blt_DBuffer_Destroy) (Blt_DBuffer buffer); /* 19 */
    int (*blt_DBuffer_LoadFile) (Tcl_Interp *interp, const char *fileName, Blt_DBuffer buffer); /* 20 */
    int (*blt_DBuffer_SaveFile) (Tcl_Interp *interp, const char *fileName, Blt_DBuffer buffer); /* 21 */
    void (*blt_DBuffer_AppendByte) (Blt_DBuffer buffer, unsigned char byte); /* 22 */
    void (*blt_DBuffer_AppendShort) (Blt_DBuffer buffer, unsigned short value); /* 23 */
    void (*blt_DBuffer_AppendInt) (Blt_DBuffer buffer, unsigned int value); /* 24 */
    Tcl_Obj * (*blt_DBuffer_ByteArrayObj) (Blt_DBuffer buffer); /* 25 */
    Tcl_Obj * (*blt_DBuffer_StringObj) (Blt_DBuffer buffer); /* 26 */
    const char * (*blt_DBuffer_String) (Blt_DBuffer buffer); /* 27 */
    int (*blt_DBuffer_Base64Decode) (Tcl_Interp *interp, const char *string, size_t length, Blt_DBuffer buffer); /* 28 */
    Tcl_Obj * (*blt_DBuffer_Base64EncodeToObj) (Blt_DBuffer buffer); /* 29 */
    int (*blt_DBuffer_AppendBase85) (Blt_DBuffer buffer, const unsigned char *bytes, size_t numBytes); /* 30 */
    int (*blt_DBuffer_AppendBase64) (Blt_DBuffer buffer, const unsigned char *bytes, size_t numBytes); /* 31 */
    long (*blt_SimplifyLine) (Point2d *origPts, long low, long high, double tolerance, long *indices); /* 32 */
    int (*blt_LineRectClip) (Region2d *regionPtr, Point2d *p, Point2d *q); /* 33 */
    int (*blt_PointInPolygon) (Point2d *samplePtr, Point2d *points, int numPoints); /* 34 */
    int (*blt_PolygonInRegion) (Point2d *points, int numPoints, Region2d *extsPtr, int enclosed); /* 35 */
    int (*blt_PointInSegments) (Point2d *samplePtr, Segment2d *segments, int numSegments, double halo); /* 36 */
    int (*blt_PolyRectClip) (Region2d *extsPtr, Point2d *inputPts, int numInputPts, Point2d *outputPts); /* 37 */
    Point2d (*blt_GetProjection) (double x, double y, Point2d *p, Point2d *q); /* 38 */
    Point2d (*blt_GetProjection2) (double x, double y, double x1, double y1, double x2, double y2); /* 39 */
    int * (*blt_ConvexHull) (int numPoints, Point2d *points, int *numHullPtsPtr); /* 40 */
    int (*blt_InitCmd) (Tcl_Interp *interp, const char *namespace, Blt_CmdSpec *specPtr); /* 41 */
    int (*blt_InitCmds) (Tcl_Interp *interp, const char *namespace, Blt_CmdSpec *specPtr, int numCmds); /* 42 */
    void (*blt_FreeMesh) (Blt_Mesh mesh); /* 43 */
    int (*blt_GetMeshFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Mesh *meshPtr); /* 44 */
    int (*blt_GetMesh) (Tcl_Interp *interp, const char *string, Blt_Mesh *meshPtr); /* 45 */
    int (*blt_Triangulate) (Tcl_Interp *interp, int numPoints, Point2d *points, int sorted, Blt_MeshTriangle *triangles); /* 46 */
    void (*blt_Mesh_CreateNotifier) (Blt_Mesh mesh, Blt_MeshChangedProc *proc, ClientData clientData); /* 47 */
    void (*blt_Mesh_DeleteNotifier) (Blt_Mesh mesh, Blt_MeshChangedProc *proc, ClientData clientData); /* 48 */
    const char * (*blt_Mesh_Name) (Blt_Mesh mesh); /* 49 */
    int (*blt_Mesh_Type) (Blt_Mesh mesh); /* 50 */
    Point2d * (*blt_Mesh_GetVertices) (Blt_Mesh mesh, int *numVerticesPtr); /* 51 */
    int * (*blt_Mesh_GetHull) (Blt_Mesh mesh, int *numHullPtsPtr); /* 52 */
    void (*blt_Mesh_GetExtents) (Blt_Mesh mesh, float *x1Ptr, float *y1Ptr, float *x2Ptr, float *y2Ptr); /* 53 */
    Blt_MeshTriangle * (*blt_Mesh_GetTriangles) (Blt_Mesh mesh, int *numTrianglesPtr); /* 54 */
    Tcl_Namespace * (*blt_GetVariableNamespace) (Tcl_Interp *interp, const char *varName); /* 55 */
    Tcl_Namespace * (*blt_GetCommandNamespace) (Tcl_Command cmdToken); /* 56 */
    Tcl_CallFrame * (*blt_EnterNamespace) (Tcl_Interp *interp, Tcl_Namespace *nsPtr); /* 57 */
    void (*blt_LeaveNamespace) (Tcl_Interp *interp, Tcl_CallFrame *framePtr); /* 58 */
    int (*blt_ParseObjectName) (Tcl_Interp *interp, const char *name, Blt_ObjectName *objNamePtr, unsigned int flags); /* 59 */
    const char * (*blt_MakeQualifiedName) (Blt_ObjectName *objNamePtr, Tcl_DString *resultPtr); /* 60 */
    int (*blt_CommandExists) (Tcl_Interp *interp, const char *string); /* 61 */
    VOID * (*blt_GetOpFromObj) (Tcl_Interp *interp, int numSpecs, Blt_OpSpec *specs, int operPos, int objc, Tcl_Obj *const *objv, int flags); /* 62 */
    Blt_Spline (*blt_CreateSpline) (Point2d *points, int n, int type); /* 63 */
    Point2d (*blt_EvaluateSpline) (Blt_Spline spline, int index, double x); /* 64 */
    void (*blt_FreeSpline) (Blt_Spline spline); /* 65 */
    Blt_Spline (*blt_CreateParametricCubicSpline) (Point2d *points, int n, int w, int h); /* 66 */
    Point2d (*blt_EvaluateParametricCubicSpline) (Blt_Spline spline, int index, double x); /* 67 */
    void (*blt_FreeParametricCubicSpline) (Blt_Spline spline); /* 68 */
    Blt_Spline (*blt_CreateCatromSpline) (Point2d *points, int n); /* 69 */
    Point2d (*blt_EvaluateCatromSpline) (Blt_Spline spline, int i, double t); /* 70 */
    void (*blt_FreeCatromSpline) (Blt_Spline spline); /* 71 */
    int (*blt_ComputeNaturalSpline) (Point2d *origPts, int numOrigPts, Point2d *intpPts, int numIntpPts); /* 72 */
    int (*blt_ComputeQuadraticSpline) (Point2d *origPts, int numOrigPts, Point2d *intpPts, int numIntpPts); /* 73 */
    int (*blt_ComputeNaturalParametricSpline) (Point2d *origPts, int numOrigPts, Region2d *extsPtr, int isClosed, Point2d *intpPts, int numIntpPts); /* 74 */
    int (*blt_ComputeCatromParametricSpline) (Point2d *origPts, int numOrigPts, Point2d *intpPts, int numIntpPts); /* 75 */
    int (*blt_ExprDoubleFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, double *valuePtr); /* 76 */
    int (*blt_ExprIntFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *valuePtr); /* 77 */
    int (*blt_GetStateFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *statePtr); /* 78 */
    const char * (*blt_NameOfState) (int state); /* 79 */
    int (*blt_GetFillFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *fillPtr); /* 80 */
    const char * (*blt_NameOfFill) (int fill); /* 81 */
    int (*blt_GetResizeFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *fillPtr); /* 82 */
    const char * (*blt_NameOfResize) (int resize); /* 83 */
    int (*blt_GetSideFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *sidePtr); /* 84 */
    const char * (*blt_NameOfSide) (int side); /* 85 */
    int (*blt_GetCount) (Tcl_Interp *interp, const char *string, int check, long *countPtr); /* 86 */
    int (*blt_GetCountFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int check, long *countPtr); /* 87 */
    int (*blt_ParseSwitches) (Tcl_Interp *interp, Blt_SwitchSpec *specPtr, int objc, Tcl_Obj *const *objv, VOID *rec, int flags); /* 88 */
    void (*blt_FreeSwitches) (Blt_SwitchSpec *specs, VOID *rec, int flags); /* 89 */
    int (*blt_SwitchChanged) (Blt_SwitchSpec *specs, ...); /* 90 */
    int (*blt_SwitchInfo) (Tcl_Interp *interp, Blt_SwitchSpec *specs, VOID *record, Tcl_Obj *objPtr, int flags); /* 91 */
    int (*blt_SwitchValue) (Tcl_Interp *interp, Blt_SwitchSpec *specs, VOID *record, Tcl_Obj *objPtr, int flags); /* 92 */
    VOID * (*blt_Malloc) (size_t size); /* 93 */
    VOID * (*blt_Realloc) (VOID *ptr, size_t size); /* 94 */
    void (*blt_Free) (const VOID *ptr); /* 95 */
    VOID * (*blt_Calloc) (size_t numElem, size_t size); /* 96 */
    const char * (*blt_Strdup) (const char *string); /* 97 */
    const char * (*blt_Strndup) (const char *string, size_t size); /* 98 */
    VOID * (*blt_MallocAbortOnError) (size_t size, const char *file, int line); /* 99 */
    VOID * (*blt_CallocAbortOnError) (size_t numElem, size_t size, const char *file, int line); /* 100 */
    VOID * (*blt_ReallocAbortOnError) (VOID *ptr, size_t size, const char *file, int line); /* 101 */
    const char * (*blt_StrdupAbortOnError) (const char *ptr, const char *file, int line); /* 102 */
    const char * (*blt_StrndupAbortOnError) (const char *ptr, size_t size, const char *file, int line); /* 103 */
    int (*blt_DictionaryCompare) (const char *s1, const char *s2); /* 104 */
    Blt_Uid (*blt_GetUid) (const char *string); /* 105 */
    void (*blt_FreeUid) (Blt_Uid uid); /* 106 */
    Blt_Uid (*blt_FindUid) (const char *string); /* 107 */
    int (*blt_CreatePipeline) (Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, Blt_Pid **pidArrayPtr, VOID *inPipePtr, VOID *outPipePtr, VOID *errPipePtr, char *const *env); /* 108 */
    void (*blt_DetachPids) (int numPids, Blt_Pid *pids); /* 109 */
    void (*blt_InitHexTable) (unsigned char *table); /* 110 */
    void (*blt_DStringAppendElements) (Tcl_DString *dsPtr, ...); /* 111 */
    int (*blt_LoadLibrary) (Tcl_Interp *interp, const char *libPath, const char *initProcName, const char *safeProcName); /* 112 */
    void (*blt_Panic) (const char *fmt, ...); /* 113 */
    void (*blt_Warn) (const char *fmt, ...); /* 114 */
    FILE * (*blt_OpenFile) (Tcl_Interp *interp, const char *fileName, const char *mode); /* 115 */
    const char * (*blt_Itoa) (int value); /* 116 */
    const char * (*blt_Ltoa) (long value); /* 117 */
    const char * (*blt_Utoa) (unsigned int value); /* 118 */
    const char * (*blt_Dtoa) (Tcl_Interp *interp, double value); /* 119 */
    int (*blt_DecodeHexadecimal) (Tcl_Interp *interp, const char *src, size_t numChars, unsigned char *dest, size_t *numBytesPtr, BinaryDecoder *switchesPtr); /* 120 */
    int (*blt_DecodeBase64) (Tcl_Interp *interp, const char *src, size_t numChars, unsigned char *dest, size_t *numBytesPtr, BinaryDecoder *switchesPtr); /* 121 */
    int (*blt_DecodeBase85) (Tcl_Interp *interp, const char *src, size_t numChars, unsigned char *dest, size_t *numBytesPtr, BinaryDecoder *switchesPtr); /* 122 */
    int (*blt_DecodeAscii85) (Tcl_Interp *interp, const char *src, size_t numChars, unsigned char *dest, size_t *numBytesPtr, BinaryDecoder *switchesPtr); /* 123 */
    Blt_DBuffer (*blt_DecodeBase64ToBuffer) (Tcl_Interp *interp, const char *src, size_t numChars); /* 124 */
    Tcl_Obj * (*blt_DecodeHexadecimalToObj) (Tcl_Interp *interp, const char *src, size_t numChars); /* 125 */
    Tcl_Obj * (*blt_DecodeBase64ToObj) (Tcl_Interp *interp, const char *src, size_t numChars); /* 126 */
    Tcl_Obj * (*blt_DecodeBase85ToObj) (Tcl_Interp *interp, const char *src, size_t numChars); /* 127 */
    int (*blt_EncodeHexadecimal) (const unsigned char *src, size_t numBytes, char *dest, size_t *numCharsPtr, BinaryEncoder *switchesPtr); /* 128 */
    int (*blt_EncodeBase64) (const unsigned char *src, size_t numBytes, char *dest, size_t *numCharsPtr, BinaryEncoder *switchesPtr); /* 129 */
    int (*blt_EncodeBase85) (const unsigned char *src, size_t numBytes, char *dest, size_t *numCharsPtr, BinaryEncoder *switchesPtr); /* 130 */
    int (*blt_EncodeAscii85) (const unsigned char *src, size_t numBytes, char *dest, size_t *numCharsPtr, BinaryEncoder *switchesPtr); /* 131 */
    Tcl_Obj * (*blt_EncodeHexadecimalToObj) (const unsigned char *src, size_t numBytes); /* 132 */
    Tcl_Obj * (*blt_EncodeBase64ToObj) (const unsigned char *src, size_t numBytes); /* 133 */
    Tcl_Obj * (*blt_EncodeBase85ToObj) (const unsigned char *src, size_t numBytes); /* 134 */
    size_t (*blt_HexadecimalDecodeBufferSize) (size_t numBytes, BinaryDecoder *switchesPtr); /* 135 */
    size_t (*blt_HexadecimalEncodeBufferSize) (size_t numChars, BinaryEncoder *switchesPtr); /* 136 */
    size_t (*blt_Base64DecodeBufferSize) (size_t numBytes, BinaryDecoder *switchesPtr); /* 137 */
    size_t (*blt_Base64EncodeBufferSize) (size_t numChars, BinaryEncoder *switchesPtr); /* 138 */
    size_t (*blt_Base85DecodeBufferSize) (size_t numBytes, BinaryDecoder *switchesPtr); /* 139 */
    size_t (*blt_Base85EncodeBufferSize) (size_t numChars, BinaryEncoder *switchesPtr); /* 140 */
    size_t (*blt_Ascii85DecodeBufferSize) (size_t numBytes, BinaryDecoder *switchesPtr); /* 141 */
    size_t (*blt_Ascii85EncodeBufferSize) (size_t numChars, BinaryEncoder *switchesPtr); /* 142 */
    int (*blt_IsBase64) (const char *buf, size_t length); /* 143 */
    int (*blt_GetTimeFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, double *secondsPtr); /* 144 */
    int (*blt_GetTime) (Tcl_Interp *interp, const char *string, double *secondsPtr); /* 145 */
    void (*blt_SecondsToDate) (double seconds, Blt_DateTime *datePtr); /* 146 */
    void (*blt_DateToSeconds) (Blt_DateTime *datePtr, double *secondsPtr); /* 147 */
    void (*blt_FormatDate) (Blt_DateTime *datePtr, const char *format, Tcl_DString *resultPtr); /* 148 */
    int (*blt_GetPositionFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, long *indexPtr); /* 149 */
    int (*blt_ObjIsInteger) (Tcl_Obj *objPtr); /* 150 */
    int (*blt_GetLong) (Tcl_Interp *interp, const char *s, long *valuePtr); /* 151 */
    int (*blt_GetLongFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, long *valuePtr); /* 152 */
    int (*blt_SetLongObj) (Tcl_Obj *objPtr, long value); /* 153 */
    Tcl_Obj * (*blt_NewLongObj) (long value); /* 154 */
    int (*blt_IsLongObj) (Tcl_Obj *objPtr); /* 155 */
    int (*blt_GetUnsignedLong) (Tcl_Interp *interp, const char *s, unsigned long *valuePtr); /* 156 */
    int (*blt_GetUnsignedLongFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, unsigned long *valuePtr); /* 157 */
    int (*blt_SetUnsignedLongObj) (Tcl_Obj *objPtr, unsigned long value); /* 158 */
    Tcl_Obj * (*blt_NewUnsignedLongObj) (unsigned long value); /* 159 */
    int (*blt_IsUnsignedLongObj) (Tcl_Obj *objPtr); /* 160 */
    int (*blt_GetInt64) (Tcl_Interp *interp, const char *s, int64_t *valuePtr); /* 161 */
    int (*blt_GetInt64FromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int64_t *valuePtr); /* 162 */
    int (*blt_SetInt64Obj) (Tcl_Obj *objPtr, int64_t value); /* 163 */
    Tcl_Obj * (*blt_NewInt64Obj) (int64_t value); /* 164 */
    int (*blt_IsInt64Obj) (Tcl_Obj *objPtr); /* 165 */
    int (*blt_GetDouble) (Tcl_Interp *interp, const char *s, double *valuePtr); /* 166 */
    int (*blt_GetDoubleFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, double *valuePtr); /* 167 */
    int (*blt_SetDoubleObj) (Tcl_Obj *objPtr, double value); /* 168 */
    Tcl_Obj * (*blt_NewDoubleObj) (double value); /* 169 */
    int (*blt_IsDoubleObj) (Tcl_Obj *objPtr); /* 170 */
    int (*blt_FmtString) (char *s, size_t size, const char *fmt, ...); /* 171 */
    void (*blt_LowerCase) (char *s); /* 172 */
    void (*blt_UpperCase) (char *s); /* 173 */
    int (*blt_GetPlatformId) (void); /* 174 */
    const char * (*blt_LastError) (void); /* 175 */
    double (*blt_NaN) (void); /* 176 */
    int (*blt_AlmostEquals) (double x, double y); /* 177 */
    const char ** (*blt_ConvertListToList) (int argc, const char **argv); /* 178 */
    void (*blt_RegisterObjTypes) (void); /* 179 */
    Tcl_Var (*blt_GetCachedVar) (Blt_HashTable *tablePtr, const char *label, Tcl_Obj *objPtr); /* 180 */
    void (*blt_FreeCachedVars) (Blt_HashTable *tablePtr); /* 181 */
} BltTclIntProcs;

#ifdef __cplusplus
extern "C" {
#endif
extern BltTclIntProcs *bltTclIntProcsPtr;
#ifdef __cplusplus
}
#endif

#if defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TCL_PROCS)

/*
 * Inline function declarations:
 */

/* Slot 0 is reserved */
#ifndef Blt_GetArrayFromObj
#define Blt_GetArrayFromObj \
	(bltTclIntProcsPtr->blt_GetArrayFromObj) /* 1 */
#endif
#ifndef Blt_NewArrayObj
#define Blt_NewArrayObj \
	(bltTclIntProcsPtr->blt_NewArrayObj) /* 2 */
#endif
#ifndef Blt_IsArrayObj
#define Blt_IsArrayObj \
	(bltTclIntProcsPtr->blt_IsArrayObj) /* 3 */
#endif
#ifndef Blt_Assert
#define Blt_Assert \
	(bltTclIntProcsPtr->blt_Assert) /* 4 */
#endif
#ifndef Blt_DBuffer_VarAppend
#define Blt_DBuffer_VarAppend \
	(bltTclIntProcsPtr->blt_DBuffer_VarAppend) /* 5 */
#endif
#ifndef Blt_DBuffer_Format
#define Blt_DBuffer_Format \
	(bltTclIntProcsPtr->blt_DBuffer_Format) /* 6 */
#endif
#ifndef Blt_DBuffer_Init
#define Blt_DBuffer_Init \
	(bltTclIntProcsPtr->blt_DBuffer_Init) /* 7 */
#endif
#ifndef Blt_DBuffer_Free
#define Blt_DBuffer_Free \
	(bltTclIntProcsPtr->blt_DBuffer_Free) /* 8 */
#endif
#ifndef Blt_DBuffer_Extend
#define Blt_DBuffer_Extend \
	(bltTclIntProcsPtr->blt_DBuffer_Extend) /* 9 */
#endif
#ifndef Blt_DBuffer_AppendData
#define Blt_DBuffer_AppendData \
	(bltTclIntProcsPtr->blt_DBuffer_AppendData) /* 10 */
#endif
#ifndef Blt_DBuffer_AppendString
#define Blt_DBuffer_AppendString \
	(bltTclIntProcsPtr->blt_DBuffer_AppendString) /* 11 */
#endif
#ifndef Blt_DBuffer_DeleteData
#define Blt_DBuffer_DeleteData \
	(bltTclIntProcsPtr->blt_DBuffer_DeleteData) /* 12 */
#endif
#ifndef Blt_DBuffer_InsertData
#define Blt_DBuffer_InsertData \
	(bltTclIntProcsPtr->blt_DBuffer_InsertData) /* 13 */
#endif
#ifndef Blt_DBuffer_SetFromObj
#define Blt_DBuffer_SetFromObj \
	(bltTclIntProcsPtr->blt_DBuffer_SetFromObj) /* 14 */
#endif
#ifndef Blt_DBuffer_Concat
#define Blt_DBuffer_Concat \
	(bltTclIntProcsPtr->blt_DBuffer_Concat) /* 15 */
#endif
#ifndef Blt_DBuffer_Resize
#define Blt_DBuffer_Resize \
	(bltTclIntProcsPtr->blt_DBuffer_Resize) /* 16 */
#endif
#ifndef Blt_DBuffer_SetLength
#define Blt_DBuffer_SetLength \
	(bltTclIntProcsPtr->blt_DBuffer_SetLength) /* 17 */
#endif
#ifndef Blt_DBuffer_Create
#define Blt_DBuffer_Create \
	(bltTclIntProcsPtr->blt_DBuffer_Create) /* 18 */
#endif
#ifndef Blt_DBuffer_Destroy
#define Blt_DBuffer_Destroy \
	(bltTclIntProcsPtr->blt_DBuffer_Destroy) /* 19 */
#endif
#ifndef Blt_DBuffer_LoadFile
#define Blt_DBuffer_LoadFile \
	(bltTclIntProcsPtr->blt_DBuffer_LoadFile) /* 20 */
#endif
#ifndef Blt_DBuffer_SaveFile
#define Blt_DBuffer_SaveFile \
	(bltTclIntProcsPtr->blt_DBuffer_SaveFile) /* 21 */
#endif
#ifndef Blt_DBuffer_AppendByte
#define Blt_DBuffer_AppendByte \
	(bltTclIntProcsPtr->blt_DBuffer_AppendByte) /* 22 */
#endif
#ifndef Blt_DBuffer_AppendShort
#define Blt_DBuffer_AppendShort \
	(bltTclIntProcsPtr->blt_DBuffer_AppendShort) /* 23 */
#endif
#ifndef Blt_DBuffer_AppendInt
#define Blt_DBuffer_AppendInt \
	(bltTclIntProcsPtr->blt_DBuffer_AppendInt) /* 24 */
#endif
#ifndef Blt_DBuffer_ByteArrayObj
#define Blt_DBuffer_ByteArrayObj \
	(bltTclIntProcsPtr->blt_DBuffer_ByteArrayObj) /* 25 */
#endif
#ifndef Blt_DBuffer_StringObj
#define Blt_DBuffer_StringObj \
	(bltTclIntProcsPtr->blt_DBuffer_StringObj) /* 26 */
#endif
#ifndef Blt_DBuffer_String
#define Blt_DBuffer_String \
	(bltTclIntProcsPtr->blt_DBuffer_String) /* 27 */
#endif
#ifndef Blt_DBuffer_Base64Decode
#define Blt_DBuffer_Base64Decode \
	(bltTclIntProcsPtr->blt_DBuffer_Base64Decode) /* 28 */
#endif
#ifndef Blt_DBuffer_Base64EncodeToObj
#define Blt_DBuffer_Base64EncodeToObj \
	(bltTclIntProcsPtr->blt_DBuffer_Base64EncodeToObj) /* 29 */
#endif
#ifndef Blt_DBuffer_AppendBase85
#define Blt_DBuffer_AppendBase85 \
	(bltTclIntProcsPtr->blt_DBuffer_AppendBase85) /* 30 */
#endif
#ifndef Blt_DBuffer_AppendBase64
#define Blt_DBuffer_AppendBase64 \
	(bltTclIntProcsPtr->blt_DBuffer_AppendBase64) /* 31 */
#endif
#ifndef Blt_SimplifyLine
#define Blt_SimplifyLine \
	(bltTclIntProcsPtr->blt_SimplifyLine) /* 32 */
#endif
#ifndef Blt_LineRectClip
#define Blt_LineRectClip \
	(bltTclIntProcsPtr->blt_LineRectClip) /* 33 */
#endif
#ifndef Blt_PointInPolygon
#define Blt_PointInPolygon \
	(bltTclIntProcsPtr->blt_PointInPolygon) /* 34 */
#endif
#ifndef Blt_PolygonInRegion
#define Blt_PolygonInRegion \
	(bltTclIntProcsPtr->blt_PolygonInRegion) /* 35 */
#endif
#ifndef Blt_PointInSegments
#define Blt_PointInSegments \
	(bltTclIntProcsPtr->blt_PointInSegments) /* 36 */
#endif
#ifndef Blt_PolyRectClip
#define Blt_PolyRectClip \
	(bltTclIntProcsPtr->blt_PolyRectClip) /* 37 */
#endif
#ifndef Blt_GetProjection
#define Blt_GetProjection \
	(bltTclIntProcsPtr->blt_GetProjection) /* 38 */
#endif
#ifndef Blt_GetProjection2
#define Blt_GetProjection2 \
	(bltTclIntProcsPtr->blt_GetProjection2) /* 39 */
#endif
#ifndef Blt_ConvexHull
#define Blt_ConvexHull \
	(bltTclIntProcsPtr->blt_ConvexHull) /* 40 */
#endif
#ifndef Blt_InitCmd
#define Blt_InitCmd \
	(bltTclIntProcsPtr->blt_InitCmd) /* 41 */
#endif
#ifndef Blt_InitCmds
#define Blt_InitCmds \
	(bltTclIntProcsPtr->blt_InitCmds) /* 42 */
#endif
#ifndef Blt_FreeMesh
#define Blt_FreeMesh \
	(bltTclIntProcsPtr->blt_FreeMesh) /* 43 */
#endif
#ifndef Blt_GetMeshFromObj
#define Blt_GetMeshFromObj \
	(bltTclIntProcsPtr->blt_GetMeshFromObj) /* 44 */
#endif
#ifndef Blt_GetMesh
#define Blt_GetMesh \
	(bltTclIntProcsPtr->blt_GetMesh) /* 45 */
#endif
#ifndef Blt_Triangulate
#define Blt_Triangulate \
	(bltTclIntProcsPtr->blt_Triangulate) /* 46 */
#endif
#ifndef Blt_Mesh_CreateNotifier
#define Blt_Mesh_CreateNotifier \
	(bltTclIntProcsPtr->blt_Mesh_CreateNotifier) /* 47 */
#endif
#ifndef Blt_Mesh_DeleteNotifier
#define Blt_Mesh_DeleteNotifier \
	(bltTclIntProcsPtr->blt_Mesh_DeleteNotifier) /* 48 */
#endif
#ifndef Blt_Mesh_Name
#define Blt_Mesh_Name \
	(bltTclIntProcsPtr->blt_Mesh_Name) /* 49 */
#endif
#ifndef Blt_Mesh_Type
#define Blt_Mesh_Type \
	(bltTclIntProcsPtr->blt_Mesh_Type) /* 50 */
#endif
#ifndef Blt_Mesh_GetVertices
#define Blt_Mesh_GetVertices \
	(bltTclIntProcsPtr->blt_Mesh_GetVertices) /* 51 */
#endif
#ifndef Blt_Mesh_GetHull
#define Blt_Mesh_GetHull \
	(bltTclIntProcsPtr->blt_Mesh_GetHull) /* 52 */
#endif
#ifndef Blt_Mesh_GetExtents
#define Blt_Mesh_GetExtents \
	(bltTclIntProcsPtr->blt_Mesh_GetExtents) /* 53 */
#endif
#ifndef Blt_Mesh_GetTriangles
#define Blt_Mesh_GetTriangles \
	(bltTclIntProcsPtr->blt_Mesh_GetTriangles) /* 54 */
#endif
#ifndef Blt_GetVariableNamespace
#define Blt_GetVariableNamespace \
	(bltTclIntProcsPtr->blt_GetVariableNamespace) /* 55 */
#endif
#ifndef Blt_GetCommandNamespace
#define Blt_GetCommandNamespace \
	(bltTclIntProcsPtr->blt_GetCommandNamespace) /* 56 */
#endif
#ifndef Blt_EnterNamespace
#define Blt_EnterNamespace \
	(bltTclIntProcsPtr->blt_EnterNamespace) /* 57 */
#endif
#ifndef Blt_LeaveNamespace
#define Blt_LeaveNamespace \
	(bltTclIntProcsPtr->blt_LeaveNamespace) /* 58 */
#endif
#ifndef Blt_ParseObjectName
#define Blt_ParseObjectName \
	(bltTclIntProcsPtr->blt_ParseObjectName) /* 59 */
#endif
#ifndef Blt_MakeQualifiedName
#define Blt_MakeQualifiedName \
	(bltTclIntProcsPtr->blt_MakeQualifiedName) /* 60 */
#endif
#ifndef Blt_CommandExists
#define Blt_CommandExists \
	(bltTclIntProcsPtr->blt_CommandExists) /* 61 */
#endif
#ifndef Blt_GetOpFromObj
#define Blt_GetOpFromObj \
	(bltTclIntProcsPtr->blt_GetOpFromObj) /* 62 */
#endif
#ifndef Blt_CreateSpline
#define Blt_CreateSpline \
	(bltTclIntProcsPtr->blt_CreateSpline) /* 63 */
#endif
#ifndef Blt_EvaluateSpline
#define Blt_EvaluateSpline \
	(bltTclIntProcsPtr->blt_EvaluateSpline) /* 64 */
#endif
#ifndef Blt_FreeSpline
#define Blt_FreeSpline \
	(bltTclIntProcsPtr->blt_FreeSpline) /* 65 */
#endif
#ifndef Blt_CreateParametricCubicSpline
#define Blt_CreateParametricCubicSpline \
	(bltTclIntProcsPtr->blt_CreateParametricCubicSpline) /* 66 */
#endif
#ifndef Blt_EvaluateParametricCubicSpline
#define Blt_EvaluateParametricCubicSpline \
	(bltTclIntProcsPtr->blt_EvaluateParametricCubicSpline) /* 67 */
#endif
#ifndef Blt_FreeParametricCubicSpline
#define Blt_FreeParametricCubicSpline \
	(bltTclIntProcsPtr->blt_FreeParametricCubicSpline) /* 68 */
#endif
#ifndef Blt_CreateCatromSpline
#define Blt_CreateCatromSpline \
	(bltTclIntProcsPtr->blt_CreateCatromSpline) /* 69 */
#endif
#ifndef Blt_EvaluateCatromSpline
#define Blt_EvaluateCatromSpline \
	(bltTclIntProcsPtr->blt_EvaluateCatromSpline) /* 70 */
#endif
#ifndef Blt_FreeCatromSpline
#define Blt_FreeCatromSpline \
	(bltTclIntProcsPtr->blt_FreeCatromSpline) /* 71 */
#endif
#ifndef Blt_ComputeNaturalSpline
#define Blt_ComputeNaturalSpline \
	(bltTclIntProcsPtr->blt_ComputeNaturalSpline) /* 72 */
#endif
#ifndef Blt_ComputeQuadraticSpline
#define Blt_ComputeQuadraticSpline \
	(bltTclIntProcsPtr->blt_ComputeQuadraticSpline) /* 73 */
#endif
#ifndef Blt_ComputeNaturalParametricSpline
#define Blt_ComputeNaturalParametricSpline \
	(bltTclIntProcsPtr->blt_ComputeNaturalParametricSpline) /* 74 */
#endif
#ifndef Blt_ComputeCatromParametricSpline
#define Blt_ComputeCatromParametricSpline \
	(bltTclIntProcsPtr->blt_ComputeCatromParametricSpline) /* 75 */
#endif
#ifndef Blt_ExprDoubleFromObj
#define Blt_ExprDoubleFromObj \
	(bltTclIntProcsPtr->blt_ExprDoubleFromObj) /* 76 */
#endif
#ifndef Blt_ExprIntFromObj
#define Blt_ExprIntFromObj \
	(bltTclIntProcsPtr->blt_ExprIntFromObj) /* 77 */
#endif
#ifndef Blt_GetStateFromObj
#define Blt_GetStateFromObj \
	(bltTclIntProcsPtr->blt_GetStateFromObj) /* 78 */
#endif
#ifndef Blt_NameOfState
#define Blt_NameOfState \
	(bltTclIntProcsPtr->blt_NameOfState) /* 79 */
#endif
#ifndef Blt_GetFillFromObj
#define Blt_GetFillFromObj \
	(bltTclIntProcsPtr->blt_GetFillFromObj) /* 80 */
#endif
#ifndef Blt_NameOfFill
#define Blt_NameOfFill \
	(bltTclIntProcsPtr->blt_NameOfFill) /* 81 */
#endif
#ifndef Blt_GetResizeFromObj
#define Blt_GetResizeFromObj \
	(bltTclIntProcsPtr->blt_GetResizeFromObj) /* 82 */
#endif
#ifndef Blt_NameOfResize
#define Blt_NameOfResize \
	(bltTclIntProcsPtr->blt_NameOfResize) /* 83 */
#endif
#ifndef Blt_GetSideFromObj
#define Blt_GetSideFromObj \
	(bltTclIntProcsPtr->blt_GetSideFromObj) /* 84 */
#endif
#ifndef Blt_NameOfSide
#define Blt_NameOfSide \
	(bltTclIntProcsPtr->blt_NameOfSide) /* 85 */
#endif
#ifndef Blt_GetCount
#define Blt_GetCount \
	(bltTclIntProcsPtr->blt_GetCount) /* 86 */
#endif
#ifndef Blt_GetCountFromObj
#define Blt_GetCountFromObj \
	(bltTclIntProcsPtr->blt_GetCountFromObj) /* 87 */
#endif
#ifndef Blt_ParseSwitches
#define Blt_ParseSwitches \
	(bltTclIntProcsPtr->blt_ParseSwitches) /* 88 */
#endif
#ifndef Blt_FreeSwitches
#define Blt_FreeSwitches \
	(bltTclIntProcsPtr->blt_FreeSwitches) /* 89 */
#endif
#ifndef Blt_SwitchChanged
#define Blt_SwitchChanged \
	(bltTclIntProcsPtr->blt_SwitchChanged) /* 90 */
#endif
#ifndef Blt_SwitchInfo
#define Blt_SwitchInfo \
	(bltTclIntProcsPtr->blt_SwitchInfo) /* 91 */
#endif
#ifndef Blt_SwitchValue
#define Blt_SwitchValue \
	(bltTclIntProcsPtr->blt_SwitchValue) /* 92 */
#endif
#ifndef Blt_Malloc
#define Blt_Malloc \
	(bltTclIntProcsPtr->blt_Malloc) /* 93 */
#endif
#ifndef Blt_Realloc
#define Blt_Realloc \
	(bltTclIntProcsPtr->blt_Realloc) /* 94 */
#endif
#ifndef Blt_Free
#define Blt_Free \
	(bltTclIntProcsPtr->blt_Free) /* 95 */
#endif
#ifndef Blt_Calloc
#define Blt_Calloc \
	(bltTclIntProcsPtr->blt_Calloc) /* 96 */
#endif
#ifndef Blt_Strdup
#define Blt_Strdup \
	(bltTclIntProcsPtr->blt_Strdup) /* 97 */
#endif
#ifndef Blt_Strndup
#define Blt_Strndup \
	(bltTclIntProcsPtr->blt_Strndup) /* 98 */
#endif
#ifndef Blt_MallocAbortOnError
#define Blt_MallocAbortOnError \
	(bltTclIntProcsPtr->blt_MallocAbortOnError) /* 99 */
#endif
#ifndef Blt_CallocAbortOnError
#define Blt_CallocAbortOnError \
	(bltTclIntProcsPtr->blt_CallocAbortOnError) /* 100 */
#endif
#ifndef Blt_ReallocAbortOnError
#define Blt_ReallocAbortOnError \
	(bltTclIntProcsPtr->blt_ReallocAbortOnError) /* 101 */
#endif
#ifndef Blt_StrdupAbortOnError
#define Blt_StrdupAbortOnError \
	(bltTclIntProcsPtr->blt_StrdupAbortOnError) /* 102 */
#endif
#ifndef Blt_StrndupAbortOnError
#define Blt_StrndupAbortOnError \
	(bltTclIntProcsPtr->blt_StrndupAbortOnError) /* 103 */
#endif
#ifndef Blt_DictionaryCompare
#define Blt_DictionaryCompare \
	(bltTclIntProcsPtr->blt_DictionaryCompare) /* 104 */
#endif
#ifndef Blt_GetUid
#define Blt_GetUid \
	(bltTclIntProcsPtr->blt_GetUid) /* 105 */
#endif
#ifndef Blt_FreeUid
#define Blt_FreeUid \
	(bltTclIntProcsPtr->blt_FreeUid) /* 106 */
#endif
#ifndef Blt_FindUid
#define Blt_FindUid \
	(bltTclIntProcsPtr->blt_FindUid) /* 107 */
#endif
#ifndef Blt_CreatePipeline
#define Blt_CreatePipeline \
	(bltTclIntProcsPtr->blt_CreatePipeline) /* 108 */
#endif
#ifndef Blt_DetachPids
#define Blt_DetachPids \
	(bltTclIntProcsPtr->blt_DetachPids) /* 109 */
#endif
#ifndef Blt_InitHexTable
#define Blt_InitHexTable \
	(bltTclIntProcsPtr->blt_InitHexTable) /* 110 */
#endif
#ifndef Blt_DStringAppendElements
#define Blt_DStringAppendElements \
	(bltTclIntProcsPtr->blt_DStringAppendElements) /* 111 */
#endif
#ifndef Blt_LoadLibrary
#define Blt_LoadLibrary \
	(bltTclIntProcsPtr->blt_LoadLibrary) /* 112 */
#endif
#ifndef Blt_Panic
#define Blt_Panic \
	(bltTclIntProcsPtr->blt_Panic) /* 113 */
#endif
#ifndef Blt_Warn
#define Blt_Warn \
	(bltTclIntProcsPtr->blt_Warn) /* 114 */
#endif
#ifndef Blt_OpenFile
#define Blt_OpenFile \
	(bltTclIntProcsPtr->blt_OpenFile) /* 115 */
#endif
#ifndef Blt_Itoa
#define Blt_Itoa \
	(bltTclIntProcsPtr->blt_Itoa) /* 116 */
#endif
#ifndef Blt_Ltoa
#define Blt_Ltoa \
	(bltTclIntProcsPtr->blt_Ltoa) /* 117 */
#endif
#ifndef Blt_Utoa
#define Blt_Utoa \
	(bltTclIntProcsPtr->blt_Utoa) /* 118 */
#endif
#ifndef Blt_Dtoa
#define Blt_Dtoa \
	(bltTclIntProcsPtr->blt_Dtoa) /* 119 */
#endif
#ifndef Blt_DecodeHexadecimal
#define Blt_DecodeHexadecimal \
	(bltTclIntProcsPtr->blt_DecodeHexadecimal) /* 120 */
#endif
#ifndef Blt_DecodeBase64
#define Blt_DecodeBase64 \
	(bltTclIntProcsPtr->blt_DecodeBase64) /* 121 */
#endif
#ifndef Blt_DecodeBase85
#define Blt_DecodeBase85 \
	(bltTclIntProcsPtr->blt_DecodeBase85) /* 122 */
#endif
#ifndef Blt_DecodeAscii85
#define Blt_DecodeAscii85 \
	(bltTclIntProcsPtr->blt_DecodeAscii85) /* 123 */
#endif
#ifndef Blt_DecodeBase64ToBuffer
#define Blt_DecodeBase64ToBuffer \
	(bltTclIntProcsPtr->blt_DecodeBase64ToBuffer) /* 124 */
#endif
#ifndef Blt_DecodeHexadecimalToObj
#define Blt_DecodeHexadecimalToObj \
	(bltTclIntProcsPtr->blt_DecodeHexadecimalToObj) /* 125 */
#endif
#ifndef Blt_DecodeBase64ToObj
#define Blt_DecodeBase64ToObj \
	(bltTclIntProcsPtr->blt_DecodeBase64ToObj) /* 126 */
#endif
#ifndef Blt_DecodeBase85ToObj
#define Blt_DecodeBase85ToObj \
	(bltTclIntProcsPtr->blt_DecodeBase85ToObj) /* 127 */
#endif
#ifndef Blt_EncodeHexadecimal
#define Blt_EncodeHexadecimal \
	(bltTclIntProcsPtr->blt_EncodeHexadecimal) /* 128 */
#endif
#ifndef Blt_EncodeBase64
#define Blt_EncodeBase64 \
	(bltTclIntProcsPtr->blt_EncodeBase64) /* 129 */
#endif
#ifndef Blt_EncodeBase85
#define Blt_EncodeBase85 \
	(bltTclIntProcsPtr->blt_EncodeBase85) /* 130 */
#endif
#ifndef Blt_EncodeAscii85
#define Blt_EncodeAscii85 \
	(bltTclIntProcsPtr->blt_EncodeAscii85) /* 131 */
#endif
#ifndef Blt_EncodeHexadecimalToObj
#define Blt_EncodeHexadecimalToObj \
	(bltTclIntProcsPtr->blt_EncodeHexadecimalToObj) /* 132 */
#endif
#ifndef Blt_EncodeBase64ToObj
#define Blt_EncodeBase64ToObj \
	(bltTclIntProcsPtr->blt_EncodeBase64ToObj) /* 133 */
#endif
#ifndef Blt_EncodeBase85ToObj
#define Blt_EncodeBase85ToObj \
	(bltTclIntProcsPtr->blt_EncodeBase85ToObj) /* 134 */
#endif
#ifndef Blt_HexadecimalDecodeBufferSize
#define Blt_HexadecimalDecodeBufferSize \
	(bltTclIntProcsPtr->blt_HexadecimalDecodeBufferSize) /* 135 */
#endif
#ifndef Blt_HexadecimalEncodeBufferSize
#define Blt_HexadecimalEncodeBufferSize \
	(bltTclIntProcsPtr->blt_HexadecimalEncodeBufferSize) /* 136 */
#endif
#ifndef Blt_Base64DecodeBufferSize
#define Blt_Base64DecodeBufferSize \
	(bltTclIntProcsPtr->blt_Base64DecodeBufferSize) /* 137 */
#endif
#ifndef Blt_Base64EncodeBufferSize
#define Blt_Base64EncodeBufferSize \
	(bltTclIntProcsPtr->blt_Base64EncodeBufferSize) /* 138 */
#endif
#ifndef Blt_Base85DecodeBufferSize
#define Blt_Base85DecodeBufferSize \
	(bltTclIntProcsPtr->blt_Base85DecodeBufferSize) /* 139 */
#endif
#ifndef Blt_Base85EncodeBufferSize
#define Blt_Base85EncodeBufferSize \
	(bltTclIntProcsPtr->blt_Base85EncodeBufferSize) /* 140 */
#endif
#ifndef Blt_Ascii85DecodeBufferSize
#define Blt_Ascii85DecodeBufferSize \
	(bltTclIntProcsPtr->blt_Ascii85DecodeBufferSize) /* 141 */
#endif
#ifndef Blt_Ascii85EncodeBufferSize
#define Blt_Ascii85EncodeBufferSize \
	(bltTclIntProcsPtr->blt_Ascii85EncodeBufferSize) /* 142 */
#endif
#ifndef Blt_IsBase64
#define Blt_IsBase64 \
	(bltTclIntProcsPtr->blt_IsBase64) /* 143 */
#endif
#ifndef Blt_GetTimeFromObj
#define Blt_GetTimeFromObj \
	(bltTclIntProcsPtr->blt_GetTimeFromObj) /* 144 */
#endif
#ifndef Blt_GetTime
#define Blt_GetTime \
	(bltTclIntProcsPtr->blt_GetTime) /* 145 */
#endif
#ifndef Blt_SecondsToDate
#define Blt_SecondsToDate \
	(bltTclIntProcsPtr->blt_SecondsToDate) /* 146 */
#endif
#ifndef Blt_DateToSeconds
#define Blt_DateToSeconds \
	(bltTclIntProcsPtr->blt_DateToSeconds) /* 147 */
#endif
#ifndef Blt_FormatDate
#define Blt_FormatDate \
	(bltTclIntProcsPtr->blt_FormatDate) /* 148 */
#endif
#ifndef Blt_GetPositionFromObj
#define Blt_GetPositionFromObj \
	(bltTclIntProcsPtr->blt_GetPositionFromObj) /* 149 */
#endif
#ifndef Blt_ObjIsInteger
#define Blt_ObjIsInteger \
	(bltTclIntProcsPtr->blt_ObjIsInteger) /* 150 */
#endif
#ifndef Blt_GetLong
#define Blt_GetLong \
	(bltTclIntProcsPtr->blt_GetLong) /* 151 */
#endif
#ifndef Blt_GetLongFromObj
#define Blt_GetLongFromObj \
	(bltTclIntProcsPtr->blt_GetLongFromObj) /* 152 */
#endif
#ifndef Blt_SetLongObj
#define Blt_SetLongObj \
	(bltTclIntProcsPtr->blt_SetLongObj) /* 153 */
#endif
#ifndef Blt_NewLongObj
#define Blt_NewLongObj \
	(bltTclIntProcsPtr->blt_NewLongObj) /* 154 */
#endif
#ifndef Blt_IsLongObj
#define Blt_IsLongObj \
	(bltTclIntProcsPtr->blt_IsLongObj) /* 155 */
#endif
#ifndef Blt_GetUnsignedLong
#define Blt_GetUnsignedLong \
	(bltTclIntProcsPtr->blt_GetUnsignedLong) /* 156 */
#endif
#ifndef Blt_GetUnsignedLongFromObj
#define Blt_GetUnsignedLongFromObj \
	(bltTclIntProcsPtr->blt_GetUnsignedLongFromObj) /* 157 */
#endif
#ifndef Blt_SetUnsignedLongObj
#define Blt_SetUnsignedLongObj \
	(bltTclIntProcsPtr->blt_SetUnsignedLongObj) /* 158 */
#endif
#ifndef Blt_NewUnsignedLongObj
#define Blt_NewUnsignedLongObj \
	(bltTclIntProcsPtr->blt_NewUnsignedLongObj) /* 159 */
#endif
#ifndef Blt_IsUnsignedLongObj
#define Blt_IsUnsignedLongObj \
	(bltTclIntProcsPtr->blt_IsUnsignedLongObj) /* 160 */
#endif
#ifndef Blt_GetInt64
#define Blt_GetInt64 \
	(bltTclIntProcsPtr->blt_GetInt64) /* 161 */
#endif
#ifndef Blt_GetInt64FromObj
#define Blt_GetInt64FromObj \
	(bltTclIntProcsPtr->blt_GetInt64FromObj) /* 162 */
#endif
#ifndef Blt_SetInt64Obj
#define Blt_SetInt64Obj \
	(bltTclIntProcsPtr->blt_SetInt64Obj) /* 163 */
#endif
#ifndef Blt_NewInt64Obj
#define Blt_NewInt64Obj \
	(bltTclIntProcsPtr->blt_NewInt64Obj) /* 164 */
#endif
#ifndef Blt_IsInt64Obj
#define Blt_IsInt64Obj \
	(bltTclIntProcsPtr->blt_IsInt64Obj) /* 165 */
#endif
#ifndef Blt_GetDouble
#define Blt_GetDouble \
	(bltTclIntProcsPtr->blt_GetDouble) /* 166 */
#endif
#ifndef Blt_GetDoubleFromObj
#define Blt_GetDoubleFromObj \
	(bltTclIntProcsPtr->blt_GetDoubleFromObj) /* 167 */
#endif
#ifndef Blt_SetDoubleObj
#define Blt_SetDoubleObj \
	(bltTclIntProcsPtr->blt_SetDoubleObj) /* 168 */
#endif
#ifndef Blt_NewDoubleObj
#define Blt_NewDoubleObj \
	(bltTclIntProcsPtr->blt_NewDoubleObj) /* 169 */
#endif
#ifndef Blt_IsDoubleObj
#define Blt_IsDoubleObj \
	(bltTclIntProcsPtr->blt_IsDoubleObj) /* 170 */
#endif
#ifndef Blt_FmtString
#define Blt_FmtString \
	(bltTclIntProcsPtr->blt_FmtString) /* 171 */
#endif
#ifndef Blt_LowerCase
#define Blt_LowerCase \
	(bltTclIntProcsPtr->blt_LowerCase) /* 172 */
#endif
#ifndef Blt_UpperCase
#define Blt_UpperCase \
	(bltTclIntProcsPtr->blt_UpperCase) /* 173 */
#endif
#ifndef Blt_GetPlatformId
#define Blt_GetPlatformId \
	(bltTclIntProcsPtr->blt_GetPlatformId) /* 174 */
#endif
#ifndef Blt_LastError
#define Blt_LastError \
	(bltTclIntProcsPtr->blt_LastError) /* 175 */
#endif
#ifndef Blt_NaN
#define Blt_NaN \
	(bltTclIntProcsPtr->blt_NaN) /* 176 */
#endif
#ifndef Blt_AlmostEquals
#define Blt_AlmostEquals \
	(bltTclIntProcsPtr->blt_AlmostEquals) /* 177 */
#endif
#ifndef Blt_ConvertListToList
#define Blt_ConvertListToList \
	(bltTclIntProcsPtr->blt_ConvertListToList) /* 178 */
#endif
#ifndef Blt_RegisterObjTypes
#define Blt_RegisterObjTypes \
	(bltTclIntProcsPtr->blt_RegisterObjTypes) /* 179 */
#endif
#ifndef Blt_GetCachedVar
#define Blt_GetCachedVar \
	(bltTclIntProcsPtr->blt_GetCachedVar) /* 180 */
#endif
#ifndef Blt_FreeCachedVars
#define Blt_FreeCachedVars \
	(bltTclIntProcsPtr->blt_FreeCachedVars) /* 181 */
#endif

#endif /* defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TCL_PROCS) */

/* !END!: Do not edit above this line. */

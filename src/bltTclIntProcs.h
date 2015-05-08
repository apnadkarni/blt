/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "bltArrayObj.h"
#include "bltAssert.h"
#include "bltDBuffer.h"
#include "bltInitCmd.h"
#include "bltInt.h"
#include "bltMath.h"
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
#ifndef Blt_RegisterArrayObj_DECLARED
#define Blt_RegisterArrayObj_DECLARED
/* 3 */
BLT_EXTERN void		Blt_RegisterArrayObj(void );
#endif
#ifndef Blt_IsArrayObj_DECLARED
#define Blt_IsArrayObj_DECLARED
/* 4 */
BLT_EXTERN int		Blt_IsArrayObj(Tcl_Obj *obj);
#endif
#ifndef Blt_Assert_DECLARED
#define Blt_Assert_DECLARED
/* 5 */
BLT_EXTERN void		Blt_Assert(const char *expr, const char *file,
				int line);
#endif
#ifndef Blt_DBuffer_VarAppend_DECLARED
#define Blt_DBuffer_VarAppend_DECLARED
/* 6 */
BLT_EXTERN void		Blt_DBuffer_VarAppend(Blt_DBuffer buffer, ...);
#endif
#ifndef Blt_DBuffer_Format_DECLARED
#define Blt_DBuffer_Format_DECLARED
/* 7 */
BLT_EXTERN int		Blt_DBuffer_Format(Blt_DBuffer buffer,
				const char *fmt, ...);
#endif
#ifndef Blt_DBuffer_Init_DECLARED
#define Blt_DBuffer_Init_DECLARED
/* 8 */
BLT_EXTERN void		Blt_DBuffer_Init(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_Free_DECLARED
#define Blt_DBuffer_Free_DECLARED
/* 9 */
BLT_EXTERN void		Blt_DBuffer_Free(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_Extend_DECLARED
#define Blt_DBuffer_Extend_DECLARED
/* 10 */
BLT_EXTERN unsigned char * Blt_DBuffer_Extend(Blt_DBuffer buffer,
				size_t extra);
#endif
#ifndef Blt_DBuffer_AppendData_DECLARED
#define Blt_DBuffer_AppendData_DECLARED
/* 11 */
BLT_EXTERN int		Blt_DBuffer_AppendData(Blt_DBuffer buffer,
				const unsigned char *bytes, size_t extra);
#endif
#ifndef Blt_DBuffer_AppendString_DECLARED
#define Blt_DBuffer_AppendString_DECLARED
/* 12 */
BLT_EXTERN int		Blt_DBuffer_AppendString(Blt_DBuffer buffer,
				const char *string, int length);
#endif
#ifndef Blt_DBuffer_DeleteData_DECLARED
#define Blt_DBuffer_DeleteData_DECLARED
/* 13 */
BLT_EXTERN int		Blt_DBuffer_DeleteData(Blt_DBuffer buffer,
				size_t index, size_t numBytes);
#endif
#ifndef Blt_DBuffer_InsertData_DECLARED
#define Blt_DBuffer_InsertData_DECLARED
/* 14 */
BLT_EXTERN int		Blt_DBuffer_InsertData(Blt_DBuffer buffer,
				const unsigned char *bytes, size_t extra,
				size_t index);
#endif
#ifndef Blt_DBuffer_SetFromObj_DECLARED
#define Blt_DBuffer_SetFromObj_DECLARED
/* 15 */
BLT_EXTERN unsigned char * Blt_DBuffer_SetFromObj(Blt_DBuffer buffer,
				Tcl_Obj *objPtr);
#endif
#ifndef Blt_DBuffer_Concat_DECLARED
#define Blt_DBuffer_Concat_DECLARED
/* 16 */
BLT_EXTERN int		Blt_DBuffer_Concat(Blt_DBuffer dest, Blt_DBuffer src);
#endif
#ifndef Blt_DBuffer_Resize_DECLARED
#define Blt_DBuffer_Resize_DECLARED
/* 17 */
BLT_EXTERN int		Blt_DBuffer_Resize(Blt_DBuffer buffer, size_t length);
#endif
#ifndef Blt_DBuffer_SetLength_DECLARED
#define Blt_DBuffer_SetLength_DECLARED
/* 18 */
BLT_EXTERN int		Blt_DBuffer_SetLength(Blt_DBuffer buffer,
				size_t length);
#endif
#ifndef Blt_DBuffer_Create_DECLARED
#define Blt_DBuffer_Create_DECLARED
/* 19 */
BLT_EXTERN Blt_DBuffer	Blt_DBuffer_Create(void );
#endif
#ifndef Blt_DBuffer_Destroy_DECLARED
#define Blt_DBuffer_Destroy_DECLARED
/* 20 */
BLT_EXTERN void		Blt_DBuffer_Destroy(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_LoadFile_DECLARED
#define Blt_DBuffer_LoadFile_DECLARED
/* 21 */
BLT_EXTERN int		Blt_DBuffer_LoadFile(Tcl_Interp *interp,
				const char *fileName, Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_SaveFile_DECLARED
#define Blt_DBuffer_SaveFile_DECLARED
/* 22 */
BLT_EXTERN int		Blt_DBuffer_SaveFile(Tcl_Interp *interp,
				const char *fileName, Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_AppendByte_DECLARED
#define Blt_DBuffer_AppendByte_DECLARED
/* 23 */
BLT_EXTERN void		Blt_DBuffer_AppendByte(Blt_DBuffer buffer,
				unsigned char byte);
#endif
#ifndef Blt_DBuffer_AppendShort_DECLARED
#define Blt_DBuffer_AppendShort_DECLARED
/* 24 */
BLT_EXTERN void		Blt_DBuffer_AppendShort(Blt_DBuffer buffer,
				unsigned short value);
#endif
#ifndef Blt_DBuffer_AppendInt_DECLARED
#define Blt_DBuffer_AppendInt_DECLARED
/* 25 */
BLT_EXTERN void		Blt_DBuffer_AppendInt(Blt_DBuffer buffer,
				unsigned int value);
#endif
#ifndef Blt_DBuffer_ByteArrayObj_DECLARED
#define Blt_DBuffer_ByteArrayObj_DECLARED
/* 26 */
BLT_EXTERN Tcl_Obj *	Blt_DBuffer_ByteArrayObj(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_StringObj_DECLARED
#define Blt_DBuffer_StringObj_DECLARED
/* 27 */
BLT_EXTERN Tcl_Obj *	Blt_DBuffer_StringObj(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_String_DECLARED
#define Blt_DBuffer_String_DECLARED
/* 28 */
BLT_EXTERN const char *	 Blt_DBuffer_String(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_Base64Decode_DECLARED
#define Blt_DBuffer_Base64Decode_DECLARED
/* 29 */
BLT_EXTERN int		Blt_DBuffer_Base64Decode(Tcl_Interp *interp,
				const char *string, size_t length,
				Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_Base64EncodeToObj_DECLARED
#define Blt_DBuffer_Base64EncodeToObj_DECLARED
/* 30 */
BLT_EXTERN Tcl_Obj *	Blt_DBuffer_Base64EncodeToObj(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_AppendBase85_DECLARED
#define Blt_DBuffer_AppendBase85_DECLARED
/* 31 */
BLT_EXTERN int		Blt_DBuffer_AppendBase85(Blt_DBuffer buffer,
				const unsigned char *bytes, size_t numBytes);
#endif
#ifndef Blt_DBuffer_AppendBase64_DECLARED
#define Blt_DBuffer_AppendBase64_DECLARED
/* 32 */
BLT_EXTERN int		Blt_DBuffer_AppendBase64(Blt_DBuffer buffer,
				const unsigned char *bytes, size_t numBytes);
#endif
#ifndef Blt_InitCmd_DECLARED
#define Blt_InitCmd_DECLARED
/* 33 */
BLT_EXTERN int		Blt_InitCmd(Tcl_Interp *interp,
				const char *namespace, Blt_CmdSpec *specPtr);
#endif
#ifndef Blt_InitCmds_DECLARED
#define Blt_InitCmds_DECLARED
/* 34 */
BLT_EXTERN int		Blt_InitCmds(Tcl_Interp *interp,
				const char *namespace, Blt_CmdSpec *specPtr,
				int numCmds);
#endif
#ifndef Blt_GetVariableNamespace_DECLARED
#define Blt_GetVariableNamespace_DECLARED
/* 35 */
BLT_EXTERN Tcl_Namespace * Blt_GetVariableNamespace(Tcl_Interp *interp,
				const char *varName);
#endif
#ifndef Blt_GetCommandNamespace_DECLARED
#define Blt_GetCommandNamespace_DECLARED
/* 36 */
BLT_EXTERN Tcl_Namespace * Blt_GetCommandNamespace(Tcl_Command cmdToken);
#endif
#ifndef Blt_EnterNamespace_DECLARED
#define Blt_EnterNamespace_DECLARED
/* 37 */
BLT_EXTERN Tcl_CallFrame * Blt_EnterNamespace(Tcl_Interp *interp,
				Tcl_Namespace *nsPtr);
#endif
#ifndef Blt_LeaveNamespace_DECLARED
#define Blt_LeaveNamespace_DECLARED
/* 38 */
BLT_EXTERN void		Blt_LeaveNamespace(Tcl_Interp *interp,
				Tcl_CallFrame *framePtr);
#endif
#ifndef Blt_ParseObjectName_DECLARED
#define Blt_ParseObjectName_DECLARED
/* 39 */
BLT_EXTERN int		Blt_ParseObjectName(Tcl_Interp *interp,
				const char *name, Blt_ObjectName *objNamePtr,
				unsigned int flags);
#endif
#ifndef Blt_MakeQualifiedName_DECLARED
#define Blt_MakeQualifiedName_DECLARED
/* 40 */
BLT_EXTERN const char *	 Blt_MakeQualifiedName(Blt_ObjectName *objNamePtr,
				Tcl_DString *resultPtr);
#endif
#ifndef Blt_CommandExists_DECLARED
#define Blt_CommandExists_DECLARED
/* 41 */
BLT_EXTERN int		Blt_CommandExists(Tcl_Interp *interp,
				const char *string);
#endif
#ifndef Blt_GetOpFromObj_DECLARED
#define Blt_GetOpFromObj_DECLARED
/* 42 */
BLT_EXTERN void *	Blt_GetOpFromObj(Tcl_Interp *interp, int numSpecs,
				Blt_OpSpec *specs, int operPos, int objc,
				Tcl_Obj *const *objv, int flags);
#endif
#ifndef Blt_CreateSpline_DECLARED
#define Blt_CreateSpline_DECLARED
/* 43 */
BLT_EXTERN Blt_Spline	Blt_CreateSpline(Point2d *points, int n, int type);
#endif
#ifndef Blt_EvaluateSpline_DECLARED
#define Blt_EvaluateSpline_DECLARED
/* 44 */
BLT_EXTERN Point2d	Blt_EvaluateSpline(Blt_Spline spline, int index,
				double x);
#endif
#ifndef Blt_FreeSpline_DECLARED
#define Blt_FreeSpline_DECLARED
/* 45 */
BLT_EXTERN void		Blt_FreeSpline(Blt_Spline spline);
#endif
#ifndef Blt_CreateParametricCubicSpline_DECLARED
#define Blt_CreateParametricCubicSpline_DECLARED
/* 46 */
BLT_EXTERN Blt_Spline	Blt_CreateParametricCubicSpline(Point2d *points,
				int n, int w, int h);
#endif
#ifndef Blt_EvaluateParametricCubicSpline_DECLARED
#define Blt_EvaluateParametricCubicSpline_DECLARED
/* 47 */
BLT_EXTERN Point2d	Blt_EvaluateParametricCubicSpline(Blt_Spline spline,
				int index, double x);
#endif
#ifndef Blt_FreeParametricCubicSpline_DECLARED
#define Blt_FreeParametricCubicSpline_DECLARED
/* 48 */
BLT_EXTERN void		Blt_FreeParametricCubicSpline(Blt_Spline spline);
#endif
#ifndef Blt_CreateCatromSpline_DECLARED
#define Blt_CreateCatromSpline_DECLARED
/* 49 */
BLT_EXTERN Blt_Spline	Blt_CreateCatromSpline(Point2d *points, int n);
#endif
#ifndef Blt_EvaluateCatromSpline_DECLARED
#define Blt_EvaluateCatromSpline_DECLARED
/* 50 */
BLT_EXTERN Point2d	Blt_EvaluateCatromSpline(Blt_Spline spline, int i,
				double t);
#endif
#ifndef Blt_FreeCatromSpline_DECLARED
#define Blt_FreeCatromSpline_DECLARED
/* 51 */
BLT_EXTERN void		Blt_FreeCatromSpline(Blt_Spline spline);
#endif
#ifndef Blt_ComputeNaturalSpline_DECLARED
#define Blt_ComputeNaturalSpline_DECLARED
/* 52 */
BLT_EXTERN int		Blt_ComputeNaturalSpline(Point2d *origPts,
				int numOrigPts, Point2d *intpPts,
				int numIntpPts);
#endif
#ifndef Blt_ComputeQuadraticSpline_DECLARED
#define Blt_ComputeQuadraticSpline_DECLARED
/* 53 */
BLT_EXTERN int		Blt_ComputeQuadraticSpline(Point2d *origPts,
				int numOrigPts, Point2d *intpPts,
				int numIntpPts);
#endif
#ifndef Blt_ComputeNaturalParametricSpline_DECLARED
#define Blt_ComputeNaturalParametricSpline_DECLARED
/* 54 */
BLT_EXTERN int		Blt_ComputeNaturalParametricSpline(Point2d *origPts,
				int numOrigPts, Region2d *extsPtr,
				int isClosed, Point2d *intpPts,
				int numIntpPts);
#endif
#ifndef Blt_ComputeCatromParametricSpline_DECLARED
#define Blt_ComputeCatromParametricSpline_DECLARED
/* 55 */
BLT_EXTERN int		Blt_ComputeCatromParametricSpline(Point2d *origPts,
				int numOrigPts, Point2d *intpPts,
				int numIntpPts);
#endif
#ifndef Blt_ParseSwitches_DECLARED
#define Blt_ParseSwitches_DECLARED
/* 56 */
BLT_EXTERN int		Blt_ParseSwitches(Tcl_Interp *interp,
				Blt_SwitchSpec *specPtr, int objc,
				Tcl_Obj *const *objv, void *rec, int flags);
#endif
#ifndef Blt_FreeSwitches_DECLARED
#define Blt_FreeSwitches_DECLARED
/* 57 */
BLT_EXTERN void		Blt_FreeSwitches(Blt_SwitchSpec *specs, void *rec,
				int flags);
#endif
#ifndef Blt_SwitchChanged_DECLARED
#define Blt_SwitchChanged_DECLARED
/* 58 */
BLT_EXTERN int		Blt_SwitchChanged(Blt_SwitchSpec *specs, ...);
#endif
#ifndef Blt_SwitchInfo_DECLARED
#define Blt_SwitchInfo_DECLARED
/* 59 */
BLT_EXTERN int		Blt_SwitchInfo(Tcl_Interp *interp,
				Blt_SwitchSpec *specs, void *record,
				Tcl_Obj *objPtr, int flags);
#endif
#ifndef Blt_SwitchValue_DECLARED
#define Blt_SwitchValue_DECLARED
/* 60 */
BLT_EXTERN int		Blt_SwitchValue(Tcl_Interp *interp,
				Blt_SwitchSpec *specs, void *record,
				Tcl_Obj *objPtr, int flags);
#endif
#ifndef Blt_Malloc_DECLARED
#define Blt_Malloc_DECLARED
/* 61 */
BLT_EXTERN void *	Blt_Malloc(size_t size);
#endif
#ifndef Blt_Realloc_DECLARED
#define Blt_Realloc_DECLARED
/* 62 */
BLT_EXTERN void *	Blt_Realloc(void *ptr, size_t size);
#endif
#ifndef Blt_Free_DECLARED
#define Blt_Free_DECLARED
/* 63 */
BLT_EXTERN void		Blt_Free(const void *ptr);
#endif
#ifndef Blt_Calloc_DECLARED
#define Blt_Calloc_DECLARED
/* 64 */
BLT_EXTERN void *	Blt_Calloc(size_t numElem, size_t size);
#endif
#ifndef Blt_Strdup_DECLARED
#define Blt_Strdup_DECLARED
/* 65 */
BLT_EXTERN const char *	 Blt_Strdup(const char *string);
#endif
#ifndef Blt_Strndup_DECLARED
#define Blt_Strndup_DECLARED
/* 66 */
BLT_EXTERN const char *	 Blt_Strndup(const char *string, size_t size);
#endif
#ifndef Blt_MallocAbortOnError_DECLARED
#define Blt_MallocAbortOnError_DECLARED
/* 67 */
BLT_EXTERN void *	Blt_MallocAbortOnError(size_t size, const char *file,
				int line);
#endif
#ifndef Blt_CallocAbortOnError_DECLARED
#define Blt_CallocAbortOnError_DECLARED
/* 68 */
BLT_EXTERN void *	Blt_CallocAbortOnError(size_t numElem, size_t size,
				const char *file, int line);
#endif
#ifndef Blt_ReallocAbortOnError_DECLARED
#define Blt_ReallocAbortOnError_DECLARED
/* 69 */
BLT_EXTERN void *	Blt_ReallocAbortOnError(void *ptr, size_t size,
				const char *file, int line);
#endif
#ifndef Blt_StrdupAbortOnError_DECLARED
#define Blt_StrdupAbortOnError_DECLARED
/* 70 */
BLT_EXTERN const char *	 Blt_StrdupAbortOnError(const char *ptr,
				const char *file, int line);
#endif
#ifndef Blt_StrndupAbortOnError_DECLARED
#define Blt_StrndupAbortOnError_DECLARED
/* 71 */
BLT_EXTERN const char *	 Blt_StrndupAbortOnError(const char *ptr,
				size_t size, const char *file, int line);
#endif
#ifndef Blt_DictionaryCompare_DECLARED
#define Blt_DictionaryCompare_DECLARED
/* 72 */
BLT_EXTERN int		Blt_DictionaryCompare(const char *s1, const char *s2);
#endif
#ifndef Blt_GetUid_DECLARED
#define Blt_GetUid_DECLARED
/* 73 */
BLT_EXTERN Blt_Uid	Blt_GetUid(const char *string);
#endif
#ifndef Blt_FreeUid_DECLARED
#define Blt_FreeUid_DECLARED
/* 74 */
BLT_EXTERN void		Blt_FreeUid(Blt_Uid uid);
#endif
#ifndef Blt_FindUid_DECLARED
#define Blt_FindUid_DECLARED
/* 75 */
BLT_EXTERN Blt_Uid	Blt_FindUid(const char *string);
#endif
#ifndef Blt_CreatePipeline_DECLARED
#define Blt_CreatePipeline_DECLARED
/* 76 */
BLT_EXTERN int		Blt_CreatePipeline(Tcl_Interp *interp, int objc,
				Tcl_Obj *const *objv, Blt_Pid **pidArrayPtr,
				int *stdinPipePtr, int *stdoutPipePtr,
				int *stderrPipePtr);
#endif
#ifndef Blt_InitHexTable_DECLARED
#define Blt_InitHexTable_DECLARED
/* 77 */
BLT_EXTERN void		Blt_InitHexTable(unsigned char *table);
#endif
#ifndef Blt_DStringAppendElements_DECLARED
#define Blt_DStringAppendElements_DECLARED
/* 78 */
BLT_EXTERN void		Blt_DStringAppendElements(Tcl_DString *dsPtr, ...);
#endif
#ifndef Blt_LoadLibrary_DECLARED
#define Blt_LoadLibrary_DECLARED
/* 79 */
BLT_EXTERN int		Blt_LoadLibrary(Tcl_Interp *interp,
				const char *libPath,
				const char *initProcName,
				const char *safeProcName);
#endif
#ifndef Blt_Panic_DECLARED
#define Blt_Panic_DECLARED
/* 80 */
BLT_EXTERN void		Blt_Panic(const char *fmt, ...);
#endif
#ifndef Blt_Warn_DECLARED
#define Blt_Warn_DECLARED
/* 81 */
BLT_EXTERN void		Blt_Warn(const char *fmt, ...);
#endif
#ifndef Blt_GetSideFromObj_DECLARED
#define Blt_GetSideFromObj_DECLARED
/* 82 */
BLT_EXTERN int		Blt_GetSideFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *sidePtr);
#endif
#ifndef Blt_NameOfSide_DECLARED
#define Blt_NameOfSide_DECLARED
/* 83 */
BLT_EXTERN const char *	 Blt_NameOfSide(int side);
#endif
#ifndef Blt_OpenFile_DECLARED
#define Blt_OpenFile_DECLARED
/* 84 */
BLT_EXTERN FILE *	Blt_OpenFile(Tcl_Interp *interp,
				const char *fileName, const char *mode);
#endif
#ifndef Blt_ExprDoubleFromObj_DECLARED
#define Blt_ExprDoubleFromObj_DECLARED
/* 85 */
BLT_EXTERN int		Blt_ExprDoubleFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, double *valuePtr);
#endif
#ifndef Blt_ExprIntFromObj_DECLARED
#define Blt_ExprIntFromObj_DECLARED
/* 86 */
BLT_EXTERN int		Blt_ExprIntFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *valuePtr);
#endif
#ifndef Blt_Itoa_DECLARED
#define Blt_Itoa_DECLARED
/* 87 */
BLT_EXTERN const char *	 Blt_Itoa(int value);
#endif
#ifndef Blt_Ltoa_DECLARED
#define Blt_Ltoa_DECLARED
/* 88 */
BLT_EXTERN const char *	 Blt_Ltoa(long value);
#endif
#ifndef Blt_Utoa_DECLARED
#define Blt_Utoa_DECLARED
/* 89 */
BLT_EXTERN const char *	 Blt_Utoa(unsigned int value);
#endif
#ifndef Blt_Dtoa_DECLARED
#define Blt_Dtoa_DECLARED
/* 90 */
BLT_EXTERN const char *	 Blt_Dtoa(Tcl_Interp *interp, double value);
#endif
#ifndef Blt_Base64_Decode_DECLARED
#define Blt_Base64_Decode_DECLARED
/* 91 */
BLT_EXTERN unsigned char * Blt_Base64_Decode(Tcl_Interp *interp,
				const char *string, size_t *lengthPtr);
#endif
#ifndef Blt_Base64_DecodeToBuffer_DECLARED
#define Blt_Base64_DecodeToBuffer_DECLARED
/* 92 */
BLT_EXTERN Blt_DBuffer	Blt_Base64_DecodeToBuffer(Tcl_Interp *interp,
				const char *string, size_t length);
#endif
#ifndef Blt_Base64_DecodeToObj_DECLARED
#define Blt_Base64_DecodeToObj_DECLARED
/* 93 */
BLT_EXTERN Tcl_Obj *	Blt_Base64_DecodeToObj(Tcl_Interp *interp,
				const char *string, size_t length);
#endif
#ifndef Blt_Base64_EncodeToObj_DECLARED
#define Blt_Base64_EncodeToObj_DECLARED
/* 94 */
BLT_EXTERN Tcl_Obj *	Blt_Base64_EncodeToObj(const unsigned char *buffer,
				size_t bufsize);
#endif
#ifndef Blt_Base64_MaxBufferLength_DECLARED
#define Blt_Base64_MaxBufferLength_DECLARED
/* 95 */
BLT_EXTERN size_t	Blt_Base64_MaxBufferLength(size_t bufsize);
#endif
#ifndef Blt_Base64_Encode_DECLARED
#define Blt_Base64_Encode_DECLARED
/* 96 */
BLT_EXTERN size_t	Blt_Base64_Encode(const unsigned char *buffer,
				size_t bufsize, unsigned char *destBytes);
#endif
#ifndef Blt_Base85_MaxBufferLength_DECLARED
#define Blt_Base85_MaxBufferLength_DECLARED
/* 97 */
BLT_EXTERN size_t	Blt_Base85_MaxBufferLength(size_t bufsize);
#endif
#ifndef Blt_Base85_Encode_DECLARED
#define Blt_Base85_Encode_DECLARED
/* 98 */
BLT_EXTERN size_t	Blt_Base85_Encode(const unsigned char *buffer,
				size_t bufsize, unsigned char *destBytes);
#endif
#ifndef Blt_Base16_Encode_DECLARED
#define Blt_Base16_Encode_DECLARED
/* 99 */
BLT_EXTERN const char *	 Blt_Base16_Encode(const unsigned char *buffer,
				size_t bufsize);
#endif
#ifndef Blt_IsBase64_DECLARED
#define Blt_IsBase64_DECLARED
/* 100 */
BLT_EXTERN int		Blt_IsBase64(const char *buf, size_t length);
#endif
#ifndef Blt_GetDoubleFromString_DECLARED
#define Blt_GetDoubleFromString_DECLARED
/* 101 */
BLT_EXTERN int		Blt_GetDoubleFromString(Tcl_Interp *interp,
				const char *s, double *valuePtr);
#endif
#ifndef Blt_GetDoubleFromObj_DECLARED
#define Blt_GetDoubleFromObj_DECLARED
/* 102 */
BLT_EXTERN int		Blt_GetDoubleFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, double *valuePtr);
#endif
#ifndef Blt_GetTimeFromObj_DECLARED
#define Blt_GetTimeFromObj_DECLARED
/* 103 */
BLT_EXTERN int		Blt_GetTimeFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, double *secondsPtr);
#endif
#ifndef Blt_GetTime_DECLARED
#define Blt_GetTime_DECLARED
/* 104 */
BLT_EXTERN int		Blt_GetTime(Tcl_Interp *interp, const char *string,
				double *secondsPtr);
#endif
#ifndef Blt_SecondsToDate_DECLARED
#define Blt_SecondsToDate_DECLARED
/* 105 */
BLT_EXTERN void		Blt_SecondsToDate(double seconds,
				Blt_DateTime *datePtr);
#endif
#ifndef Blt_DateToSeconds_DECLARED
#define Blt_DateToSeconds_DECLARED
/* 106 */
BLT_EXTERN void		Blt_DateToSeconds(Blt_DateTime *datePtr,
				double *secondsPtr);
#endif
#ifndef Blt_FormatDate_DECLARED
#define Blt_FormatDate_DECLARED
/* 107 */
BLT_EXTERN void		Blt_FormatDate(Blt_DateTime *datePtr,
				const char *format, Tcl_DString *resultPtr);
#endif
#ifndef Blt_GetPositionFromObj_DECLARED
#define Blt_GetPositionFromObj_DECLARED
/* 108 */
BLT_EXTERN int		Blt_GetPositionFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, long *indexPtr);
#endif
#ifndef Blt_GetCountFromObj_DECLARED
#define Blt_GetCountFromObj_DECLARED
/* 109 */
BLT_EXTERN int		Blt_GetCountFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int check, long *valuePtr);
#endif
#ifndef Blt_SimplifyLine_DECLARED
#define Blt_SimplifyLine_DECLARED
/* 110 */
BLT_EXTERN int		Blt_SimplifyLine(Point2d *origPts, int low, int high,
				double tolerance, int *indices);
#endif
#ifndef Blt_GetLong_DECLARED
#define Blt_GetLong_DECLARED
/* 111 */
BLT_EXTERN int		Blt_GetLong(Tcl_Interp *interp, const char *s,
				long *longPtr);
#endif
#ifndef Blt_GetLongFromObj_DECLARED
#define Blt_GetLongFromObj_DECLARED
/* 112 */
BLT_EXTERN int		Blt_GetLongFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, long *longPtr);
#endif
#ifndef Blt_FormatString_DECLARED
#define Blt_FormatString_DECLARED
/* 113 */
BLT_EXTERN int		Blt_FormatString(char *s, size_t size,
				const char *fmt, ...);
#endif
#ifndef Blt_LowerCase_DECLARED
#define Blt_LowerCase_DECLARED
/* 114 */
BLT_EXTERN void		Blt_LowerCase(char *s);
#endif
#ifndef Blt_UpperCase_DECLARED
#define Blt_UpperCase_DECLARED
/* 115 */
BLT_EXTERN void		Blt_UpperCase(char *s);
#endif
#ifndef Blt_GetPlatformId_DECLARED
#define Blt_GetPlatformId_DECLARED
/* 116 */
BLT_EXTERN int		Blt_GetPlatformId(void );
#endif
#ifndef Blt_LastError_DECLARED
#define Blt_LastError_DECLARED
/* 117 */
BLT_EXTERN const char *	 Blt_LastError(void );
#endif
#ifndef Blt_NaN_DECLARED
#define Blt_NaN_DECLARED
/* 118 */
BLT_EXTERN double	Blt_NaN(void );
#endif
#ifndef Blt_AlmostEquals_DECLARED
#define Blt_AlmostEquals_DECLARED
/* 119 */
BLT_EXTERN int		Blt_AlmostEquals(double x, double y);
#endif
#ifndef Blt_GetCachedVar_DECLARED
#define Blt_GetCachedVar_DECLARED
/* 120 */
BLT_EXTERN Tcl_Var	Blt_GetCachedVar(Blt_HashTable *tablePtr,
				const char *label, Tcl_Obj *objPtr);
#endif
#ifndef Blt_FreeCachedVars_DECLARED
#define Blt_FreeCachedVars_DECLARED
/* 121 */
BLT_EXTERN void		Blt_FreeCachedVars(Blt_HashTable *tablePtr);
#endif

typedef struct BltTclIntProcs {
    int magic;
    struct BltTclIntStubHooks *hooks;

    void *reserved0;
    int (*blt_GetArrayFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_HashTable **tablePtrPtr); /* 1 */
    Tcl_Obj * (*blt_NewArrayObj) (int objc, Tcl_Obj *objv[]); /* 2 */
    void (*blt_RegisterArrayObj) (void); /* 3 */
    int (*blt_IsArrayObj) (Tcl_Obj *obj); /* 4 */
    void (*blt_Assert) (const char *expr, const char *file, int line); /* 5 */
    void (*blt_DBuffer_VarAppend) (Blt_DBuffer buffer, ...); /* 6 */
    int (*blt_DBuffer_Format) (Blt_DBuffer buffer, const char *fmt, ...); /* 7 */
    void (*blt_DBuffer_Init) (Blt_DBuffer buffer); /* 8 */
    void (*blt_DBuffer_Free) (Blt_DBuffer buffer); /* 9 */
    unsigned char * (*blt_DBuffer_Extend) (Blt_DBuffer buffer, size_t extra); /* 10 */
    int (*blt_DBuffer_AppendData) (Blt_DBuffer buffer, const unsigned char *bytes, size_t extra); /* 11 */
    int (*blt_DBuffer_AppendString) (Blt_DBuffer buffer, const char *string, int length); /* 12 */
    int (*blt_DBuffer_DeleteData) (Blt_DBuffer buffer, size_t index, size_t numBytes); /* 13 */
    int (*blt_DBuffer_InsertData) (Blt_DBuffer buffer, const unsigned char *bytes, size_t extra, size_t index); /* 14 */
    unsigned char * (*blt_DBuffer_SetFromObj) (Blt_DBuffer buffer, Tcl_Obj *objPtr); /* 15 */
    int (*blt_DBuffer_Concat) (Blt_DBuffer dest, Blt_DBuffer src); /* 16 */
    int (*blt_DBuffer_Resize) (Blt_DBuffer buffer, size_t length); /* 17 */
    int (*blt_DBuffer_SetLength) (Blt_DBuffer buffer, size_t length); /* 18 */
    Blt_DBuffer (*blt_DBuffer_Create) (void); /* 19 */
    void (*blt_DBuffer_Destroy) (Blt_DBuffer buffer); /* 20 */
    int (*blt_DBuffer_LoadFile) (Tcl_Interp *interp, const char *fileName, Blt_DBuffer buffer); /* 21 */
    int (*blt_DBuffer_SaveFile) (Tcl_Interp *interp, const char *fileName, Blt_DBuffer buffer); /* 22 */
    void (*blt_DBuffer_AppendByte) (Blt_DBuffer buffer, unsigned char byte); /* 23 */
    void (*blt_DBuffer_AppendShort) (Blt_DBuffer buffer, unsigned short value); /* 24 */
    void (*blt_DBuffer_AppendInt) (Blt_DBuffer buffer, unsigned int value); /* 25 */
    Tcl_Obj * (*blt_DBuffer_ByteArrayObj) (Blt_DBuffer buffer); /* 26 */
    Tcl_Obj * (*blt_DBuffer_StringObj) (Blt_DBuffer buffer); /* 27 */
    const char * (*blt_DBuffer_String) (Blt_DBuffer buffer); /* 28 */
    int (*blt_DBuffer_Base64Decode) (Tcl_Interp *interp, const char *string, size_t length, Blt_DBuffer buffer); /* 29 */
    Tcl_Obj * (*blt_DBuffer_Base64EncodeToObj) (Blt_DBuffer buffer); /* 30 */
    int (*blt_DBuffer_AppendBase85) (Blt_DBuffer buffer, const unsigned char *bytes, size_t numBytes); /* 31 */
    int (*blt_DBuffer_AppendBase64) (Blt_DBuffer buffer, const unsigned char *bytes, size_t numBytes); /* 32 */
    int (*blt_InitCmd) (Tcl_Interp *interp, const char *namespace, Blt_CmdSpec *specPtr); /* 33 */
    int (*blt_InitCmds) (Tcl_Interp *interp, const char *namespace, Blt_CmdSpec *specPtr, int numCmds); /* 34 */
    Tcl_Namespace * (*blt_GetVariableNamespace) (Tcl_Interp *interp, const char *varName); /* 35 */
    Tcl_Namespace * (*blt_GetCommandNamespace) (Tcl_Command cmdToken); /* 36 */
    Tcl_CallFrame * (*blt_EnterNamespace) (Tcl_Interp *interp, Tcl_Namespace *nsPtr); /* 37 */
    void (*blt_LeaveNamespace) (Tcl_Interp *interp, Tcl_CallFrame *framePtr); /* 38 */
    int (*blt_ParseObjectName) (Tcl_Interp *interp, const char *name, Blt_ObjectName *objNamePtr, unsigned int flags); /* 39 */
    const char * (*blt_MakeQualifiedName) (Blt_ObjectName *objNamePtr, Tcl_DString *resultPtr); /* 40 */
    int (*blt_CommandExists) (Tcl_Interp *interp, const char *string); /* 41 */
    VOID * (*blt_GetOpFromObj) (Tcl_Interp *interp, int numSpecs, Blt_OpSpec *specs, int operPos, int objc, Tcl_Obj *const *objv, int flags); /* 42 */
    Blt_Spline (*blt_CreateSpline) (Point2d *points, int n, int type); /* 43 */
    Point2d (*blt_EvaluateSpline) (Blt_Spline spline, int index, double x); /* 44 */
    void (*blt_FreeSpline) (Blt_Spline spline); /* 45 */
    Blt_Spline (*blt_CreateParametricCubicSpline) (Point2d *points, int n, int w, int h); /* 46 */
    Point2d (*blt_EvaluateParametricCubicSpline) (Blt_Spline spline, int index, double x); /* 47 */
    void (*blt_FreeParametricCubicSpline) (Blt_Spline spline); /* 48 */
    Blt_Spline (*blt_CreateCatromSpline) (Point2d *points, int n); /* 49 */
    Point2d (*blt_EvaluateCatromSpline) (Blt_Spline spline, int i, double t); /* 50 */
    void (*blt_FreeCatromSpline) (Blt_Spline spline); /* 51 */
    int (*blt_ComputeNaturalSpline) (Point2d *origPts, int numOrigPts, Point2d *intpPts, int numIntpPts); /* 52 */
    int (*blt_ComputeQuadraticSpline) (Point2d *origPts, int numOrigPts, Point2d *intpPts, int numIntpPts); /* 53 */
    int (*blt_ComputeNaturalParametricSpline) (Point2d *origPts, int numOrigPts, Region2d *extsPtr, int isClosed, Point2d *intpPts, int numIntpPts); /* 54 */
    int (*blt_ComputeCatromParametricSpline) (Point2d *origPts, int numOrigPts, Point2d *intpPts, int numIntpPts); /* 55 */
    int (*blt_ParseSwitches) (Tcl_Interp *interp, Blt_SwitchSpec *specPtr, int objc, Tcl_Obj *const *objv, VOID *rec, int flags); /* 56 */
    void (*blt_FreeSwitches) (Blt_SwitchSpec *specs, VOID *rec, int flags); /* 57 */
    int (*blt_SwitchChanged) (Blt_SwitchSpec *specs, ...); /* 58 */
    int (*blt_SwitchInfo) (Tcl_Interp *interp, Blt_SwitchSpec *specs, VOID *record, Tcl_Obj *objPtr, int flags); /* 59 */
    int (*blt_SwitchValue) (Tcl_Interp *interp, Blt_SwitchSpec *specs, VOID *record, Tcl_Obj *objPtr, int flags); /* 60 */
    VOID * (*blt_Malloc) (size_t size); /* 61 */
    VOID * (*blt_Realloc) (VOID *ptr, size_t size); /* 62 */
    void (*blt_Free) (const VOID *ptr); /* 63 */
    VOID * (*blt_Calloc) (size_t numElem, size_t size); /* 64 */
    const char * (*blt_Strdup) (const char *string); /* 65 */
    const char * (*blt_Strndup) (const char *string, size_t size); /* 66 */
    VOID * (*blt_MallocAbortOnError) (size_t size, const char *file, int line); /* 67 */
    VOID * (*blt_CallocAbortOnError) (size_t numElem, size_t size, const char *file, int line); /* 68 */
    VOID * (*blt_ReallocAbortOnError) (VOID *ptr, size_t size, const char *file, int line); /* 69 */
    const char * (*blt_StrdupAbortOnError) (const char *ptr, const char *file, int line); /* 70 */
    const char * (*blt_StrndupAbortOnError) (const char *ptr, size_t size, const char *file, int line); /* 71 */
    int (*blt_DictionaryCompare) (const char *s1, const char *s2); /* 72 */
    Blt_Uid (*blt_GetUid) (const char *string); /* 73 */
    void (*blt_FreeUid) (Blt_Uid uid); /* 74 */
    Blt_Uid (*blt_FindUid) (const char *string); /* 75 */
    int (*blt_CreatePipeline) (Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, Blt_Pid **pidArrayPtr, int *stdinPipePtr, int *stdoutPipePtr, int *stderrPipePtr); /* 76 */
    void (*blt_InitHexTable) (unsigned char *table); /* 77 */
    void (*blt_DStringAppendElements) (Tcl_DString *dsPtr, ...); /* 78 */
    int (*blt_LoadLibrary) (Tcl_Interp *interp, const char *libPath, const char *initProcName, const char *safeProcName); /* 79 */
    void (*blt_Panic) (const char *fmt, ...); /* 80 */
    void (*blt_Warn) (const char *fmt, ...); /* 81 */
    int (*blt_GetSideFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *sidePtr); /* 82 */
    const char * (*blt_NameOfSide) (int side); /* 83 */
    FILE * (*blt_OpenFile) (Tcl_Interp *interp, const char *fileName, const char *mode); /* 84 */
    int (*blt_ExprDoubleFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, double *valuePtr); /* 85 */
    int (*blt_ExprIntFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *valuePtr); /* 86 */
    const char * (*blt_Itoa) (int value); /* 87 */
    const char * (*blt_Ltoa) (long value); /* 88 */
    const char * (*blt_Utoa) (unsigned int value); /* 89 */
    const char * (*blt_Dtoa) (Tcl_Interp *interp, double value); /* 90 */
    unsigned char * (*blt_Base64_Decode) (Tcl_Interp *interp, const char *string, size_t *lengthPtr); /* 91 */
    Blt_DBuffer (*blt_Base64_DecodeToBuffer) (Tcl_Interp *interp, const char *string, size_t length); /* 92 */
    Tcl_Obj * (*blt_Base64_DecodeToObj) (Tcl_Interp *interp, const char *string, size_t length); /* 93 */
    Tcl_Obj * (*blt_Base64_EncodeToObj) (const unsigned char *buffer, size_t bufsize); /* 94 */
    size_t (*blt_Base64_MaxBufferLength) (size_t bufsize); /* 95 */
    size_t (*blt_Base64_Encode) (const unsigned char *buffer, size_t bufsize, unsigned char *destBytes); /* 96 */
    size_t (*blt_Base85_MaxBufferLength) (size_t bufsize); /* 97 */
    size_t (*blt_Base85_Encode) (const unsigned char *buffer, size_t bufsize, unsigned char *destBytes); /* 98 */
    const char * (*blt_Base16_Encode) (const unsigned char *buffer, size_t bufsize); /* 99 */
    int (*blt_IsBase64) (const char *buf, size_t length); /* 100 */
    int (*blt_GetDoubleFromString) (Tcl_Interp *interp, const char *s, double *valuePtr); /* 101 */
    int (*blt_GetDoubleFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, double *valuePtr); /* 102 */
    int (*blt_GetTimeFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, double *secondsPtr); /* 103 */
    int (*blt_GetTime) (Tcl_Interp *interp, const char *string, double *secondsPtr); /* 104 */
    void (*blt_SecondsToDate) (double seconds, Blt_DateTime *datePtr); /* 105 */
    void (*blt_DateToSeconds) (Blt_DateTime *datePtr, double *secondsPtr); /* 106 */
    void (*blt_FormatDate) (Blt_DateTime *datePtr, const char *format, Tcl_DString *resultPtr); /* 107 */
    int (*blt_GetPositionFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, long *indexPtr); /* 108 */
    int (*blt_GetCountFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int check, long *valuePtr); /* 109 */
    int (*blt_SimplifyLine) (Point2d *origPts, int low, int high, double tolerance, int *indices); /* 110 */
    int (*blt_GetLong) (Tcl_Interp *interp, const char *s, long *longPtr); /* 111 */
    int (*blt_GetLongFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, long *longPtr); /* 112 */
    int (*blt_FormatString) (char *s, size_t size, const char *fmt, ...); /* 113 */
    void (*blt_LowerCase) (char *s); /* 114 */
    void (*blt_UpperCase) (char *s); /* 115 */
    int (*blt_GetPlatformId) (void); /* 116 */
    const char * (*blt_LastError) (void); /* 117 */
    double (*blt_NaN) (void); /* 118 */
    int (*blt_AlmostEquals) (double x, double y); /* 119 */
    Tcl_Var (*blt_GetCachedVar) (Blt_HashTable *tablePtr, const char *label, Tcl_Obj *objPtr); /* 120 */
    void (*blt_FreeCachedVars) (Blt_HashTable *tablePtr); /* 121 */
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
#ifndef Blt_RegisterArrayObj
#define Blt_RegisterArrayObj \
	(bltTclIntProcsPtr->blt_RegisterArrayObj) /* 3 */
#endif
#ifndef Blt_IsArrayObj
#define Blt_IsArrayObj \
	(bltTclIntProcsPtr->blt_IsArrayObj) /* 4 */
#endif
#ifndef Blt_Assert
#define Blt_Assert \
	(bltTclIntProcsPtr->blt_Assert) /* 5 */
#endif
#ifndef Blt_DBuffer_VarAppend
#define Blt_DBuffer_VarAppend \
	(bltTclIntProcsPtr->blt_DBuffer_VarAppend) /* 6 */
#endif
#ifndef Blt_DBuffer_Format
#define Blt_DBuffer_Format \
	(bltTclIntProcsPtr->blt_DBuffer_Format) /* 7 */
#endif
#ifndef Blt_DBuffer_Init
#define Blt_DBuffer_Init \
	(bltTclIntProcsPtr->blt_DBuffer_Init) /* 8 */
#endif
#ifndef Blt_DBuffer_Free
#define Blt_DBuffer_Free \
	(bltTclIntProcsPtr->blt_DBuffer_Free) /* 9 */
#endif
#ifndef Blt_DBuffer_Extend
#define Blt_DBuffer_Extend \
	(bltTclIntProcsPtr->blt_DBuffer_Extend) /* 10 */
#endif
#ifndef Blt_DBuffer_AppendData
#define Blt_DBuffer_AppendData \
	(bltTclIntProcsPtr->blt_DBuffer_AppendData) /* 11 */
#endif
#ifndef Blt_DBuffer_AppendString
#define Blt_DBuffer_AppendString \
	(bltTclIntProcsPtr->blt_DBuffer_AppendString) /* 12 */
#endif
#ifndef Blt_DBuffer_DeleteData
#define Blt_DBuffer_DeleteData \
	(bltTclIntProcsPtr->blt_DBuffer_DeleteData) /* 13 */
#endif
#ifndef Blt_DBuffer_InsertData
#define Blt_DBuffer_InsertData \
	(bltTclIntProcsPtr->blt_DBuffer_InsertData) /* 14 */
#endif
#ifndef Blt_DBuffer_SetFromObj
#define Blt_DBuffer_SetFromObj \
	(bltTclIntProcsPtr->blt_DBuffer_SetFromObj) /* 15 */
#endif
#ifndef Blt_DBuffer_Concat
#define Blt_DBuffer_Concat \
	(bltTclIntProcsPtr->blt_DBuffer_Concat) /* 16 */
#endif
#ifndef Blt_DBuffer_Resize
#define Blt_DBuffer_Resize \
	(bltTclIntProcsPtr->blt_DBuffer_Resize) /* 17 */
#endif
#ifndef Blt_DBuffer_SetLength
#define Blt_DBuffer_SetLength \
	(bltTclIntProcsPtr->blt_DBuffer_SetLength) /* 18 */
#endif
#ifndef Blt_DBuffer_Create
#define Blt_DBuffer_Create \
	(bltTclIntProcsPtr->blt_DBuffer_Create) /* 19 */
#endif
#ifndef Blt_DBuffer_Destroy
#define Blt_DBuffer_Destroy \
	(bltTclIntProcsPtr->blt_DBuffer_Destroy) /* 20 */
#endif
#ifndef Blt_DBuffer_LoadFile
#define Blt_DBuffer_LoadFile \
	(bltTclIntProcsPtr->blt_DBuffer_LoadFile) /* 21 */
#endif
#ifndef Blt_DBuffer_SaveFile
#define Blt_DBuffer_SaveFile \
	(bltTclIntProcsPtr->blt_DBuffer_SaveFile) /* 22 */
#endif
#ifndef Blt_DBuffer_AppendByte
#define Blt_DBuffer_AppendByte \
	(bltTclIntProcsPtr->blt_DBuffer_AppendByte) /* 23 */
#endif
#ifndef Blt_DBuffer_AppendShort
#define Blt_DBuffer_AppendShort \
	(bltTclIntProcsPtr->blt_DBuffer_AppendShort) /* 24 */
#endif
#ifndef Blt_DBuffer_AppendInt
#define Blt_DBuffer_AppendInt \
	(bltTclIntProcsPtr->blt_DBuffer_AppendInt) /* 25 */
#endif
#ifndef Blt_DBuffer_ByteArrayObj
#define Blt_DBuffer_ByteArrayObj \
	(bltTclIntProcsPtr->blt_DBuffer_ByteArrayObj) /* 26 */
#endif
#ifndef Blt_DBuffer_StringObj
#define Blt_DBuffer_StringObj \
	(bltTclIntProcsPtr->blt_DBuffer_StringObj) /* 27 */
#endif
#ifndef Blt_DBuffer_String
#define Blt_DBuffer_String \
	(bltTclIntProcsPtr->blt_DBuffer_String) /* 28 */
#endif
#ifndef Blt_DBuffer_Base64Decode
#define Blt_DBuffer_Base64Decode \
	(bltTclIntProcsPtr->blt_DBuffer_Base64Decode) /* 29 */
#endif
#ifndef Blt_DBuffer_Base64EncodeToObj
#define Blt_DBuffer_Base64EncodeToObj \
	(bltTclIntProcsPtr->blt_DBuffer_Base64EncodeToObj) /* 30 */
#endif
#ifndef Blt_DBuffer_AppendBase85
#define Blt_DBuffer_AppendBase85 \
	(bltTclIntProcsPtr->blt_DBuffer_AppendBase85) /* 31 */
#endif
#ifndef Blt_DBuffer_AppendBase64
#define Blt_DBuffer_AppendBase64 \
	(bltTclIntProcsPtr->blt_DBuffer_AppendBase64) /* 32 */
#endif
#ifndef Blt_InitCmd
#define Blt_InitCmd \
	(bltTclIntProcsPtr->blt_InitCmd) /* 33 */
#endif
#ifndef Blt_InitCmds
#define Blt_InitCmds \
	(bltTclIntProcsPtr->blt_InitCmds) /* 34 */
#endif
#ifndef Blt_GetVariableNamespace
#define Blt_GetVariableNamespace \
	(bltTclIntProcsPtr->blt_GetVariableNamespace) /* 35 */
#endif
#ifndef Blt_GetCommandNamespace
#define Blt_GetCommandNamespace \
	(bltTclIntProcsPtr->blt_GetCommandNamespace) /* 36 */
#endif
#ifndef Blt_EnterNamespace
#define Blt_EnterNamespace \
	(bltTclIntProcsPtr->blt_EnterNamespace) /* 37 */
#endif
#ifndef Blt_LeaveNamespace
#define Blt_LeaveNamespace \
	(bltTclIntProcsPtr->blt_LeaveNamespace) /* 38 */
#endif
#ifndef Blt_ParseObjectName
#define Blt_ParseObjectName \
	(bltTclIntProcsPtr->blt_ParseObjectName) /* 39 */
#endif
#ifndef Blt_MakeQualifiedName
#define Blt_MakeQualifiedName \
	(bltTclIntProcsPtr->blt_MakeQualifiedName) /* 40 */
#endif
#ifndef Blt_CommandExists
#define Blt_CommandExists \
	(bltTclIntProcsPtr->blt_CommandExists) /* 41 */
#endif
#ifndef Blt_GetOpFromObj
#define Blt_GetOpFromObj \
	(bltTclIntProcsPtr->blt_GetOpFromObj) /* 42 */
#endif
#ifndef Blt_CreateSpline
#define Blt_CreateSpline \
	(bltTclIntProcsPtr->blt_CreateSpline) /* 43 */
#endif
#ifndef Blt_EvaluateSpline
#define Blt_EvaluateSpline \
	(bltTclIntProcsPtr->blt_EvaluateSpline) /* 44 */
#endif
#ifndef Blt_FreeSpline
#define Blt_FreeSpline \
	(bltTclIntProcsPtr->blt_FreeSpline) /* 45 */
#endif
#ifndef Blt_CreateParametricCubicSpline
#define Blt_CreateParametricCubicSpline \
	(bltTclIntProcsPtr->blt_CreateParametricCubicSpline) /* 46 */
#endif
#ifndef Blt_EvaluateParametricCubicSpline
#define Blt_EvaluateParametricCubicSpline \
	(bltTclIntProcsPtr->blt_EvaluateParametricCubicSpline) /* 47 */
#endif
#ifndef Blt_FreeParametricCubicSpline
#define Blt_FreeParametricCubicSpline \
	(bltTclIntProcsPtr->blt_FreeParametricCubicSpline) /* 48 */
#endif
#ifndef Blt_CreateCatromSpline
#define Blt_CreateCatromSpline \
	(bltTclIntProcsPtr->blt_CreateCatromSpline) /* 49 */
#endif
#ifndef Blt_EvaluateCatromSpline
#define Blt_EvaluateCatromSpline \
	(bltTclIntProcsPtr->blt_EvaluateCatromSpline) /* 50 */
#endif
#ifndef Blt_FreeCatromSpline
#define Blt_FreeCatromSpline \
	(bltTclIntProcsPtr->blt_FreeCatromSpline) /* 51 */
#endif
#ifndef Blt_ComputeNaturalSpline
#define Blt_ComputeNaturalSpline \
	(bltTclIntProcsPtr->blt_ComputeNaturalSpline) /* 52 */
#endif
#ifndef Blt_ComputeQuadraticSpline
#define Blt_ComputeQuadraticSpline \
	(bltTclIntProcsPtr->blt_ComputeQuadraticSpline) /* 53 */
#endif
#ifndef Blt_ComputeNaturalParametricSpline
#define Blt_ComputeNaturalParametricSpline \
	(bltTclIntProcsPtr->blt_ComputeNaturalParametricSpline) /* 54 */
#endif
#ifndef Blt_ComputeCatromParametricSpline
#define Blt_ComputeCatromParametricSpline \
	(bltTclIntProcsPtr->blt_ComputeCatromParametricSpline) /* 55 */
#endif
#ifndef Blt_ParseSwitches
#define Blt_ParseSwitches \
	(bltTclIntProcsPtr->blt_ParseSwitches) /* 56 */
#endif
#ifndef Blt_FreeSwitches
#define Blt_FreeSwitches \
	(bltTclIntProcsPtr->blt_FreeSwitches) /* 57 */
#endif
#ifndef Blt_SwitchChanged
#define Blt_SwitchChanged \
	(bltTclIntProcsPtr->blt_SwitchChanged) /* 58 */
#endif
#ifndef Blt_SwitchInfo
#define Blt_SwitchInfo \
	(bltTclIntProcsPtr->blt_SwitchInfo) /* 59 */
#endif
#ifndef Blt_SwitchValue
#define Blt_SwitchValue \
	(bltTclIntProcsPtr->blt_SwitchValue) /* 60 */
#endif
#ifndef Blt_Malloc
#define Blt_Malloc \
	(bltTclIntProcsPtr->blt_Malloc) /* 61 */
#endif
#ifndef Blt_Realloc
#define Blt_Realloc \
	(bltTclIntProcsPtr->blt_Realloc) /* 62 */
#endif
#ifndef Blt_Free
#define Blt_Free \
	(bltTclIntProcsPtr->blt_Free) /* 63 */
#endif
#ifndef Blt_Calloc
#define Blt_Calloc \
	(bltTclIntProcsPtr->blt_Calloc) /* 64 */
#endif
#ifndef Blt_Strdup
#define Blt_Strdup \
	(bltTclIntProcsPtr->blt_Strdup) /* 65 */
#endif
#ifndef Blt_Strndup
#define Blt_Strndup \
	(bltTclIntProcsPtr->blt_Strndup) /* 66 */
#endif
#ifndef Blt_MallocAbortOnError
#define Blt_MallocAbortOnError \
	(bltTclIntProcsPtr->blt_MallocAbortOnError) /* 67 */
#endif
#ifndef Blt_CallocAbortOnError
#define Blt_CallocAbortOnError \
	(bltTclIntProcsPtr->blt_CallocAbortOnError) /* 68 */
#endif
#ifndef Blt_ReallocAbortOnError
#define Blt_ReallocAbortOnError \
	(bltTclIntProcsPtr->blt_ReallocAbortOnError) /* 69 */
#endif
#ifndef Blt_StrdupAbortOnError
#define Blt_StrdupAbortOnError \
	(bltTclIntProcsPtr->blt_StrdupAbortOnError) /* 70 */
#endif
#ifndef Blt_StrndupAbortOnError
#define Blt_StrndupAbortOnError \
	(bltTclIntProcsPtr->blt_StrndupAbortOnError) /* 71 */
#endif
#ifndef Blt_DictionaryCompare
#define Blt_DictionaryCompare \
	(bltTclIntProcsPtr->blt_DictionaryCompare) /* 72 */
#endif
#ifndef Blt_GetUid
#define Blt_GetUid \
	(bltTclIntProcsPtr->blt_GetUid) /* 73 */
#endif
#ifndef Blt_FreeUid
#define Blt_FreeUid \
	(bltTclIntProcsPtr->blt_FreeUid) /* 74 */
#endif
#ifndef Blt_FindUid
#define Blt_FindUid \
	(bltTclIntProcsPtr->blt_FindUid) /* 75 */
#endif
#ifndef Blt_CreatePipeline
#define Blt_CreatePipeline \
	(bltTclIntProcsPtr->blt_CreatePipeline) /* 76 */
#endif
#ifndef Blt_InitHexTable
#define Blt_InitHexTable \
	(bltTclIntProcsPtr->blt_InitHexTable) /* 77 */
#endif
#ifndef Blt_DStringAppendElements
#define Blt_DStringAppendElements \
	(bltTclIntProcsPtr->blt_DStringAppendElements) /* 78 */
#endif
#ifndef Blt_LoadLibrary
#define Blt_LoadLibrary \
	(bltTclIntProcsPtr->blt_LoadLibrary) /* 79 */
#endif
#ifndef Blt_Panic
#define Blt_Panic \
	(bltTclIntProcsPtr->blt_Panic) /* 80 */
#endif
#ifndef Blt_Warn
#define Blt_Warn \
	(bltTclIntProcsPtr->blt_Warn) /* 81 */
#endif
#ifndef Blt_GetSideFromObj
#define Blt_GetSideFromObj \
	(bltTclIntProcsPtr->blt_GetSideFromObj) /* 82 */
#endif
#ifndef Blt_NameOfSide
#define Blt_NameOfSide \
	(bltTclIntProcsPtr->blt_NameOfSide) /* 83 */
#endif
#ifndef Blt_OpenFile
#define Blt_OpenFile \
	(bltTclIntProcsPtr->blt_OpenFile) /* 84 */
#endif
#ifndef Blt_ExprDoubleFromObj
#define Blt_ExprDoubleFromObj \
	(bltTclIntProcsPtr->blt_ExprDoubleFromObj) /* 85 */
#endif
#ifndef Blt_ExprIntFromObj
#define Blt_ExprIntFromObj \
	(bltTclIntProcsPtr->blt_ExprIntFromObj) /* 86 */
#endif
#ifndef Blt_Itoa
#define Blt_Itoa \
	(bltTclIntProcsPtr->blt_Itoa) /* 87 */
#endif
#ifndef Blt_Ltoa
#define Blt_Ltoa \
	(bltTclIntProcsPtr->blt_Ltoa) /* 88 */
#endif
#ifndef Blt_Utoa
#define Blt_Utoa \
	(bltTclIntProcsPtr->blt_Utoa) /* 89 */
#endif
#ifndef Blt_Dtoa
#define Blt_Dtoa \
	(bltTclIntProcsPtr->blt_Dtoa) /* 90 */
#endif
#ifndef Blt_Base64_Decode
#define Blt_Base64_Decode \
	(bltTclIntProcsPtr->blt_Base64_Decode) /* 91 */
#endif
#ifndef Blt_Base64_DecodeToBuffer
#define Blt_Base64_DecodeToBuffer \
	(bltTclIntProcsPtr->blt_Base64_DecodeToBuffer) /* 92 */
#endif
#ifndef Blt_Base64_DecodeToObj
#define Blt_Base64_DecodeToObj \
	(bltTclIntProcsPtr->blt_Base64_DecodeToObj) /* 93 */
#endif
#ifndef Blt_Base64_EncodeToObj
#define Blt_Base64_EncodeToObj \
	(bltTclIntProcsPtr->blt_Base64_EncodeToObj) /* 94 */
#endif
#ifndef Blt_Base64_MaxBufferLength
#define Blt_Base64_MaxBufferLength \
	(bltTclIntProcsPtr->blt_Base64_MaxBufferLength) /* 95 */
#endif
#ifndef Blt_Base64_Encode
#define Blt_Base64_Encode \
	(bltTclIntProcsPtr->blt_Base64_Encode) /* 96 */
#endif
#ifndef Blt_Base85_MaxBufferLength
#define Blt_Base85_MaxBufferLength \
	(bltTclIntProcsPtr->blt_Base85_MaxBufferLength) /* 97 */
#endif
#ifndef Blt_Base85_Encode
#define Blt_Base85_Encode \
	(bltTclIntProcsPtr->blt_Base85_Encode) /* 98 */
#endif
#ifndef Blt_Base16_Encode
#define Blt_Base16_Encode \
	(bltTclIntProcsPtr->blt_Base16_Encode) /* 99 */
#endif
#ifndef Blt_IsBase64
#define Blt_IsBase64 \
	(bltTclIntProcsPtr->blt_IsBase64) /* 100 */
#endif
#ifndef Blt_GetDoubleFromString
#define Blt_GetDoubleFromString \
	(bltTclIntProcsPtr->blt_GetDoubleFromString) /* 101 */
#endif
#ifndef Blt_GetDoubleFromObj
#define Blt_GetDoubleFromObj \
	(bltTclIntProcsPtr->blt_GetDoubleFromObj) /* 102 */
#endif
#ifndef Blt_GetTimeFromObj
#define Blt_GetTimeFromObj \
	(bltTclIntProcsPtr->blt_GetTimeFromObj) /* 103 */
#endif
#ifndef Blt_GetTime
#define Blt_GetTime \
	(bltTclIntProcsPtr->blt_GetTime) /* 104 */
#endif
#ifndef Blt_SecondsToDate
#define Blt_SecondsToDate \
	(bltTclIntProcsPtr->blt_SecondsToDate) /* 105 */
#endif
#ifndef Blt_DateToSeconds
#define Blt_DateToSeconds \
	(bltTclIntProcsPtr->blt_DateToSeconds) /* 106 */
#endif
#ifndef Blt_FormatDate
#define Blt_FormatDate \
	(bltTclIntProcsPtr->blt_FormatDate) /* 107 */
#endif
#ifndef Blt_GetPositionFromObj
#define Blt_GetPositionFromObj \
	(bltTclIntProcsPtr->blt_GetPositionFromObj) /* 108 */
#endif
#ifndef Blt_GetCountFromObj
#define Blt_GetCountFromObj \
	(bltTclIntProcsPtr->blt_GetCountFromObj) /* 109 */
#endif
#ifndef Blt_SimplifyLine
#define Blt_SimplifyLine \
	(bltTclIntProcsPtr->blt_SimplifyLine) /* 110 */
#endif
#ifndef Blt_GetLong
#define Blt_GetLong \
	(bltTclIntProcsPtr->blt_GetLong) /* 111 */
#endif
#ifndef Blt_GetLongFromObj
#define Blt_GetLongFromObj \
	(bltTclIntProcsPtr->blt_GetLongFromObj) /* 112 */
#endif
#ifndef Blt_FormatString
#define Blt_FormatString \
	(bltTclIntProcsPtr->blt_FormatString) /* 113 */
#endif
#ifndef Blt_LowerCase
#define Blt_LowerCase \
	(bltTclIntProcsPtr->blt_LowerCase) /* 114 */
#endif
#ifndef Blt_UpperCase
#define Blt_UpperCase \
	(bltTclIntProcsPtr->blt_UpperCase) /* 115 */
#endif
#ifndef Blt_GetPlatformId
#define Blt_GetPlatformId \
	(bltTclIntProcsPtr->blt_GetPlatformId) /* 116 */
#endif
#ifndef Blt_LastError
#define Blt_LastError \
	(bltTclIntProcsPtr->blt_LastError) /* 117 */
#endif
#ifndef Blt_NaN
#define Blt_NaN \
	(bltTclIntProcsPtr->blt_NaN) /* 118 */
#endif
#ifndef Blt_AlmostEquals
#define Blt_AlmostEquals \
	(bltTclIntProcsPtr->blt_AlmostEquals) /* 119 */
#endif
#ifndef Blt_GetCachedVar
#define Blt_GetCachedVar \
	(bltTclIntProcsPtr->blt_GetCachedVar) /* 120 */
#endif
#ifndef Blt_FreeCachedVars
#define Blt_FreeCachedVars \
	(bltTclIntProcsPtr->blt_FreeCachedVars) /* 121 */
#endif

#endif /* defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TCL_PROCS) */

/* !END!: Do not edit above this line. */

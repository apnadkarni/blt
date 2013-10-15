#include "bltDBuffer.h"
#include "bltTclInt.h"
#include "bltArrayObj.h"
#include "bltAssert.h"
#include "bltInitCmd.h"
#include "bltInt.h"
#include "bltMath.h"
#include "bltOp.h"
#include "bltNsUtil.h"
#include "bltSpline.h"
#include "bltSwitch.h"
#include "bltVar.h"

/* !BEGIN!: Do not edit below this line. */

/*
 * Exported function declarations:
 */

/* Slot 0 is reserved */
#ifndef Blt_DBuffer_VarAppend_DECLARED
#define Blt_DBuffer_VarAppend_DECLARED
/* 1 */
BLT_EXTERN void		Blt_DBuffer_VarAppend(Blt_DBuffer buffer, ...);
#endif
#ifndef Blt_DBuffer_Format_DECLARED
#define Blt_DBuffer_Format_DECLARED
/* 2 */
BLT_EXTERN int		Blt_DBuffer_Format(Blt_DBuffer buffer,
				const char *fmt, ...);
#endif
#ifndef Blt_DBuffer_Init_DECLARED
#define Blt_DBuffer_Init_DECLARED
/* 3 */
BLT_EXTERN void		Blt_DBuffer_Init(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_Free_DECLARED
#define Blt_DBuffer_Free_DECLARED
/* 4 */
BLT_EXTERN void		Blt_DBuffer_Free(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_Extend_DECLARED
#define Blt_DBuffer_Extend_DECLARED
/* 5 */
BLT_EXTERN unsigned char * Blt_DBuffer_Extend(Blt_DBuffer buffer,
				size_t extra);
#endif
#ifndef Blt_DBuffer_AppendData_DECLARED
#define Blt_DBuffer_AppendData_DECLARED
/* 6 */
BLT_EXTERN int		Blt_DBuffer_AppendData(Blt_DBuffer buffer,
				const unsigned char *bytes, size_t extra);
#endif
#ifndef Blt_DBuffer_Concat_DECLARED
#define Blt_DBuffer_Concat_DECLARED
/* 7 */
BLT_EXTERN int		Blt_DBuffer_Concat(Blt_DBuffer dest, Blt_DBuffer src);
#endif
#ifndef Blt_DBuffer_Resize_DECLARED
#define Blt_DBuffer_Resize_DECLARED
/* 8 */
BLT_EXTERN int		Blt_DBuffer_Resize(Blt_DBuffer buffer, size_t length);
#endif
#ifndef Blt_DBuffer_SetLength_DECLARED
#define Blt_DBuffer_SetLength_DECLARED
/* 9 */
BLT_EXTERN int		Blt_DBuffer_SetLength(Blt_DBuffer buffer,
				size_t length);
#endif
#ifndef Blt_DBuffer_Create_DECLARED
#define Blt_DBuffer_Create_DECLARED
/* 10 */
BLT_EXTERN Blt_DBuffer	Blt_DBuffer_Create(void );
#endif
#ifndef Blt_DBuffer_Destroy_DECLARED
#define Blt_DBuffer_Destroy_DECLARED
/* 11 */
BLT_EXTERN void		Blt_DBuffer_Destroy(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_LoadFile_DECLARED
#define Blt_DBuffer_LoadFile_DECLARED
/* 12 */
BLT_EXTERN int		Blt_DBuffer_LoadFile(Tcl_Interp *interp,
				const char *fileName, Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_SaveFile_DECLARED
#define Blt_DBuffer_SaveFile_DECLARED
/* 13 */
BLT_EXTERN int		Blt_DBuffer_SaveFile(Tcl_Interp *interp,
				const char *fileName, Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_AppendByte_DECLARED
#define Blt_DBuffer_AppendByte_DECLARED
/* 14 */
BLT_EXTERN void		Blt_DBuffer_AppendByte(Blt_DBuffer buffer,
				unsigned char byte);
#endif
#ifndef Blt_DBuffer_AppendShort_DECLARED
#define Blt_DBuffer_AppendShort_DECLARED
/* 15 */
BLT_EXTERN void		Blt_DBuffer_AppendShort(Blt_DBuffer buffer,
				unsigned short value);
#endif
#ifndef Blt_DBuffer_AppendInt_DECLARED
#define Blt_DBuffer_AppendInt_DECLARED
/* 16 */
BLT_EXTERN void		Blt_DBuffer_AppendInt(Blt_DBuffer buffer,
				unsigned int value);
#endif
#ifndef Blt_DBuffer_ByteArrayObj_DECLARED
#define Blt_DBuffer_ByteArrayObj_DECLARED
/* 17 */
BLT_EXTERN Tcl_Obj *	Blt_DBuffer_ByteArrayObj(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_StringObj_DECLARED
#define Blt_DBuffer_StringObj_DECLARED
/* 18 */
BLT_EXTERN Tcl_Obj *	Blt_DBuffer_StringObj(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_String_DECLARED
#define Blt_DBuffer_String_DECLARED
/* 19 */
BLT_EXTERN const char *	 Blt_DBuffer_String(Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_Base64Decode_DECLARED
#define Blt_DBuffer_Base64Decode_DECLARED
/* 20 */
BLT_EXTERN int		Blt_DBuffer_Base64Decode(Tcl_Interp *interp,
				const char *string, size_t length,
				Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_Base64Encode_DECLARED
#define Blt_DBuffer_Base64Encode_DECLARED
/* 21 */
BLT_EXTERN const char *	 Blt_DBuffer_Base64Encode(Tcl_Interp *interp,
				Blt_DBuffer buffer);
#endif
#ifndef Blt_DBuffer_Base64EncodeToObj_DECLARED
#define Blt_DBuffer_Base64EncodeToObj_DECLARED
/* 22 */
BLT_EXTERN Tcl_Obj *	Blt_DBuffer_Base64EncodeToObj(Tcl_Interp *interp,
				Blt_DBuffer buffer);
#endif
#ifndef Blt_Malloc_DECLARED
#define Blt_Malloc_DECLARED
/* 23 */
BLT_EXTERN void *	Blt_Malloc(size_t size);
#endif
#ifndef Blt_Realloc_DECLARED
#define Blt_Realloc_DECLARED
/* 24 */
BLT_EXTERN void *	Blt_Realloc(void *ptr, size_t size);
#endif
#ifndef Blt_Free_DECLARED
#define Blt_Free_DECLARED
/* 25 */
BLT_EXTERN void		Blt_Free(const void *ptr);
#endif
#ifndef Blt_Calloc_DECLARED
#define Blt_Calloc_DECLARED
/* 26 */
BLT_EXTERN void *	Blt_Calloc(size_t numElem, size_t size);
#endif
#ifndef Blt_Strdup_DECLARED
#define Blt_Strdup_DECLARED
/* 27 */
BLT_EXTERN char *	Blt_Strdup(const char *string);
#endif
#ifndef Blt_MallocAbortOnError_DECLARED
#define Blt_MallocAbortOnError_DECLARED
/* 28 */
BLT_EXTERN void *	Blt_MallocAbortOnError(size_t size, const char *file,
				int line);
#endif
#ifndef Blt_CallocAbortOnError_DECLARED
#define Blt_CallocAbortOnError_DECLARED
/* 29 */
BLT_EXTERN void *	Blt_CallocAbortOnError(size_t numElem, size_t size,
				const char *file, int line);
#endif
#ifndef Blt_StrdupAbortOnError_DECLARED
#define Blt_StrdupAbortOnError_DECLARED
/* 30 */
BLT_EXTERN char *	Blt_StrdupAbortOnError(const char *ptr,
				const char *file, int line);
#endif
#ifndef Blt_DictionaryCompare_DECLARED
#define Blt_DictionaryCompare_DECLARED
/* 31 */
BLT_EXTERN int		Blt_DictionaryCompare(const char *s1, const char *s2);
#endif
#ifndef Blt_GetUid_DECLARED
#define Blt_GetUid_DECLARED
/* 32 */
BLT_EXTERN Blt_Uid	Blt_GetUid(const char *string);
#endif
#ifndef Blt_FreeUid_DECLARED
#define Blt_FreeUid_DECLARED
/* 33 */
BLT_EXTERN void		Blt_FreeUid(Blt_Uid uid);
#endif
#ifndef Blt_FindUid_DECLARED
#define Blt_FindUid_DECLARED
/* 34 */
BLT_EXTERN Blt_Uid	Blt_FindUid(const char *string);
#endif
#ifndef Blt_CreatePipeline_DECLARED
#define Blt_CreatePipeline_DECLARED
/* 35 */
BLT_EXTERN int		Blt_CreatePipeline(Tcl_Interp *interp, int objc,
				Tcl_Obj *const *objv, Blt_Pid **pidArrayPtr,
				int *stdinPipePtr, int *stdoutPipePtr,
				int *stderrPipePtr);
#endif
#ifndef Blt_InitHexTable_DECLARED
#define Blt_InitHexTable_DECLARED
/* 36 */
BLT_EXTERN void		Blt_InitHexTable(unsigned char *table);
#endif
#ifndef Blt_DStringAppendElements_DECLARED
#define Blt_DStringAppendElements_DECLARED
/* 37 */
BLT_EXTERN void		Blt_DStringAppendElements(Tcl_DString *dsPtr, ...);
#endif
#ifndef Blt_LoadLibrary_DECLARED
#define Blt_LoadLibrary_DECLARED
/* 38 */
BLT_EXTERN int		Blt_LoadLibrary(Tcl_Interp *interp,
				const char *libPath,
				const char *initProcName,
				const char *safeProcName);
#endif
#ifndef Blt_Panic_DECLARED
#define Blt_Panic_DECLARED
/* 39 */
BLT_EXTERN void		Blt_Panic(const char *fmt, ...);
#endif
#ifndef Blt_Warn_DECLARED
#define Blt_Warn_DECLARED
/* 40 */
BLT_EXTERN void		Blt_Warn(const char *fmt, ...);
#endif
#ifndef Blt_GetSideFromObj_DECLARED
#define Blt_GetSideFromObj_DECLARED
/* 41 */
BLT_EXTERN int		Blt_GetSideFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *sidePtr);
#endif
#ifndef Blt_NameOfSide_DECLARED
#define Blt_NameOfSide_DECLARED
/* 42 */
BLT_EXTERN const char *	 Blt_NameOfSide(int side);
#endif
#ifndef Blt_OpenFile_DECLARED
#define Blt_OpenFile_DECLARED
/* 43 */
BLT_EXTERN FILE *	Blt_OpenFile(Tcl_Interp *interp,
				const char *fileName, const char *mode);
#endif
#ifndef Blt_ExprDoubleFromObj_DECLARED
#define Blt_ExprDoubleFromObj_DECLARED
/* 44 */
BLT_EXTERN int		Blt_ExprDoubleFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, double *valuePtr);
#endif
#ifndef Blt_ExprIntFromObj_DECLARED
#define Blt_ExprIntFromObj_DECLARED
/* 45 */
BLT_EXTERN int		Blt_ExprIntFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int *valuePtr);
#endif
#ifndef Blt_Itoa_DECLARED
#define Blt_Itoa_DECLARED
/* 46 */
BLT_EXTERN const char *	 Blt_Itoa(int value);
#endif
#ifndef Blt_Ltoa_DECLARED
#define Blt_Ltoa_DECLARED
/* 47 */
BLT_EXTERN const char *	 Blt_Ltoa(long value);
#endif
#ifndef Blt_Utoa_DECLARED
#define Blt_Utoa_DECLARED
/* 48 */
BLT_EXTERN const char *	 Blt_Utoa(unsigned int value);
#endif
#ifndef Blt_Dtoa_DECLARED
#define Blt_Dtoa_DECLARED
/* 49 */
BLT_EXTERN const char *	 Blt_Dtoa(Tcl_Interp *interp, double value);
#endif
#ifndef Blt_Base64_Decode_DECLARED
#define Blt_Base64_Decode_DECLARED
/* 50 */
BLT_EXTERN unsigned char * Blt_Base64_Decode(Tcl_Interp *interp,
				const char *string, size_t *lengthPtr);
#endif
#ifndef Blt_Base64_DecodeToBuffer_DECLARED
#define Blt_Base64_DecodeToBuffer_DECLARED
/* 51 */
BLT_EXTERN Blt_DBuffer	Blt_Base64_DecodeToBuffer(Tcl_Interp *interp,
				const char *string, size_t length);
#endif
#ifndef Blt_Base64_DecodeToObj_DECLARED
#define Blt_Base64_DecodeToObj_DECLARED
/* 52 */
BLT_EXTERN Tcl_Obj *	Blt_Base64_DecodeToObj(Tcl_Interp *interp,
				const char *string, size_t length);
#endif
#ifndef Blt_Base64_Encode_DECLARED
#define Blt_Base64_Encode_DECLARED
/* 53 */
BLT_EXTERN const char *	 Blt_Base64_Encode(Tcl_Interp *interp,
				const unsigned char *buffer, size_t bufsize);
#endif
#ifndef Blt_Base64_EncodeToObj_DECLARED
#define Blt_Base64_EncodeToObj_DECLARED
/* 54 */
BLT_EXTERN Tcl_Obj *	Blt_Base64_EncodeToObj(Tcl_Interp *interp,
				const unsigned char *buffer, size_t bufsize);
#endif
#ifndef Blt_Base85_Encode_DECLARED
#define Blt_Base85_Encode_DECLARED
/* 55 */
BLT_EXTERN const char *	 Blt_Base85_Encode(Tcl_Interp *interp,
				const unsigned char *buffer, size_t bufsize);
#endif
#ifndef Blt_Base16_Encode_DECLARED
#define Blt_Base16_Encode_DECLARED
/* 56 */
BLT_EXTERN const char *	 Blt_Base16_Encode(Tcl_Interp *interp,
				const unsigned char *buffer, size_t bufsize);
#endif
#ifndef Blt_IsBase64_DECLARED
#define Blt_IsBase64_DECLARED
/* 57 */
BLT_EXTERN int		Blt_IsBase64(const char *buf, size_t length);
#endif
#ifndef Blt_GetDoubleFromString_DECLARED
#define Blt_GetDoubleFromString_DECLARED
/* 58 */
BLT_EXTERN int		Blt_GetDoubleFromString(Tcl_Interp *interp,
				const char *s, double *valuePtr);
#endif
#ifndef Blt_GetDoubleFromObj_DECLARED
#define Blt_GetDoubleFromObj_DECLARED
/* 59 */
BLT_EXTERN int		Blt_GetDoubleFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, double *valuePtr);
#endif
#ifndef Blt_GetTimeFromObj_DECLARED
#define Blt_GetTimeFromObj_DECLARED
/* 60 */
BLT_EXTERN int		Blt_GetTimeFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, double *valuePtr);
#endif
#ifndef Blt_GetTime_DECLARED
#define Blt_GetTime_DECLARED
/* 61 */
BLT_EXTERN int		Blt_GetTime(Tcl_Interp *interp, const char *string,
				double *valuePtr);
#endif
#ifndef Blt_GetDateFromObj_DECLARED
#define Blt_GetDateFromObj_DECLARED
/* 62 */
BLT_EXTERN int		Blt_GetDateFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, double *timePtr);
#endif
#ifndef Blt_GetDate_DECLARED
#define Blt_GetDate_DECLARED
/* 63 */
BLT_EXTERN int		Blt_GetDate(Tcl_Interp *interp, const char *string,
				double *timePtr);
#endif
#ifndef Blt_GetPositionFromObj_DECLARED
#define Blt_GetPositionFromObj_DECLARED
/* 64 */
BLT_EXTERN int		Blt_GetPositionFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, long *indexPtr);
#endif
#ifndef Blt_GetCountFromObj_DECLARED
#define Blt_GetCountFromObj_DECLARED
/* 65 */
BLT_EXTERN int		Blt_GetCountFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, int check, long *valuePtr);
#endif
#ifndef Blt_SimplifyLine_DECLARED
#define Blt_SimplifyLine_DECLARED
/* 66 */
BLT_EXTERN int		Blt_SimplifyLine(Point2d *origPts, int low, int high,
				double tolerance, int *indices);
#endif
#ifndef Blt_GetLong_DECLARED
#define Blt_GetLong_DECLARED
/* 67 */
BLT_EXTERN int		Blt_GetLong(Tcl_Interp *interp, const char *s,
				long *longPtr);
#endif
#ifndef Blt_GetLongFromObj_DECLARED
#define Blt_GetLongFromObj_DECLARED
/* 68 */
BLT_EXTERN int		Blt_GetLongFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, long *longPtr);
#endif
#ifndef Blt_FormatString_DECLARED
#define Blt_FormatString_DECLARED
/* 69 */
BLT_EXTERN int		Blt_FormatString(char *s, size_t size,
				const char *fmt, ...);
#endif
#ifndef Blt_LowerCase_DECLARED
#define Blt_LowerCase_DECLARED
/* 70 */
BLT_EXTERN void		Blt_LowerCase(char *s);
#endif
#ifndef Blt_GetPlatformId_DECLARED
#define Blt_GetPlatformId_DECLARED
/* 71 */
BLT_EXTERN int		Blt_GetPlatformId(void );
#endif
#ifndef Blt_LastError_DECLARED
#define Blt_LastError_DECLARED
/* 72 */
BLT_EXTERN const char *	 Blt_LastError(void );
#endif
#ifndef Blt_NaN_DECLARED
#define Blt_NaN_DECLARED
/* 73 */
BLT_EXTERN double	Blt_NaN(void );
#endif
#ifndef Blt_AlmostEquals_DECLARED
#define Blt_AlmostEquals_DECLARED
/* 74 */
BLT_EXTERN int		Blt_AlmostEquals(double x, double y);
#endif
#ifndef Blt_GetArrayFromObj_DECLARED
#define Blt_GetArrayFromObj_DECLARED
/* 75 */
BLT_EXTERN int		Blt_GetArrayFromObj(Tcl_Interp *interp,
				Tcl_Obj *objPtr, Blt_HashTable **tablePtrPtr);
#endif
#ifndef Blt_NewArrayObj_DECLARED
#define Blt_NewArrayObj_DECLARED
/* 76 */
BLT_EXTERN Tcl_Obj *	Blt_NewArrayObj(int objc, Tcl_Obj *objv[]);
#endif
#ifndef Blt_RegisterArrayObj_DECLARED
#define Blt_RegisterArrayObj_DECLARED
/* 77 */
BLT_EXTERN void		Blt_RegisterArrayObj(void );
#endif
#ifndef Blt_IsArrayObj_DECLARED
#define Blt_IsArrayObj_DECLARED
/* 78 */
BLT_EXTERN int		Blt_IsArrayObj(Tcl_Obj *obj);
#endif
#ifndef Blt_Assert_DECLARED
#define Blt_Assert_DECLARED
/* 79 */
BLT_EXTERN void		Blt_Assert(const char *expr, const char *file,
				int line);
#endif
#ifndef Blt_InitCmd_DECLARED
#define Blt_InitCmd_DECLARED
/* 80 */
BLT_EXTERN int		Blt_InitCmd(Tcl_Interp *interp,
				const char *namespace, Blt_CmdSpec *specPtr);
#endif
#ifndef Blt_InitCmds_DECLARED
#define Blt_InitCmds_DECLARED
/* 81 */
BLT_EXTERN int		Blt_InitCmds(Tcl_Interp *interp,
				const char *namespace, Blt_CmdSpec *specPtr,
				int numCmds);
#endif
#ifndef Blt_GetOpFromObj_DECLARED
#define Blt_GetOpFromObj_DECLARED
/* 82 */
BLT_EXTERN void *	Blt_GetOpFromObj(Tcl_Interp *interp, int numSpecs,
				Blt_OpSpec *specs, int operPos, int objc,
				Tcl_Obj *const *objv, int flags);
#endif
#ifndef Blt_GetVariableNamespace_DECLARED
#define Blt_GetVariableNamespace_DECLARED
/* 83 */
BLT_EXTERN Tcl_Namespace * Blt_GetVariableNamespace(Tcl_Interp *interp,
				const char *varName);
#endif
#ifndef Blt_GetCommandNamespace_DECLARED
#define Blt_GetCommandNamespace_DECLARED
/* 84 */
BLT_EXTERN Tcl_Namespace * Blt_GetCommandNamespace(Tcl_Command cmdToken);
#endif
#ifndef Blt_EnterNamespace_DECLARED
#define Blt_EnterNamespace_DECLARED
/* 85 */
BLT_EXTERN Tcl_CallFrame * Blt_EnterNamespace(Tcl_Interp *interp,
				Tcl_Namespace *nsPtr);
#endif
#ifndef Blt_LeaveNamespace_DECLARED
#define Blt_LeaveNamespace_DECLARED
/* 86 */
BLT_EXTERN void		Blt_LeaveNamespace(Tcl_Interp *interp,
				Tcl_CallFrame *framePtr);
#endif
#ifndef Blt_ParseObjectName_DECLARED
#define Blt_ParseObjectName_DECLARED
/* 87 */
BLT_EXTERN int		Blt_ParseObjectName(Tcl_Interp *interp,
				const char *name, Blt_ObjectName *objNamePtr,
				unsigned int flags);
#endif
#ifndef Blt_MakeQualifiedName_DECLARED
#define Blt_MakeQualifiedName_DECLARED
/* 88 */
BLT_EXTERN const char *	 Blt_MakeQualifiedName(Blt_ObjectName *objNamePtr,
				Tcl_DString *resultPtr);
#endif
#ifndef Blt_CommandExists_DECLARED
#define Blt_CommandExists_DECLARED
/* 89 */
BLT_EXTERN int		Blt_CommandExists(Tcl_Interp *interp,
				const char *string);
#endif
#ifndef Blt_CreateSpline_DECLARED
#define Blt_CreateSpline_DECLARED
/* 90 */
BLT_EXTERN Blt_Spline	Blt_CreateSpline(Point2d *points, int n, int type);
#endif
#ifndef Blt_EvaluateSpline_DECLARED
#define Blt_EvaluateSpline_DECLARED
/* 91 */
BLT_EXTERN Point2d	Blt_EvaluateSpline(Blt_Spline spline, int index,
				double x);
#endif
#ifndef Blt_FreeSpline_DECLARED
#define Blt_FreeSpline_DECLARED
/* 92 */
BLT_EXTERN void		Blt_FreeSpline(Blt_Spline spline);
#endif
#ifndef Blt_CreateParametricCubicSpline_DECLARED
#define Blt_CreateParametricCubicSpline_DECLARED
/* 93 */
BLT_EXTERN Blt_Spline	Blt_CreateParametricCubicSpline(Point2d *points,
				int n, int w, int h);
#endif
#ifndef Blt_EvaluateParametricCubicSpline_DECLARED
#define Blt_EvaluateParametricCubicSpline_DECLARED
/* 94 */
BLT_EXTERN Point2d	Blt_EvaluateParametricCubicSpline(Blt_Spline spline,
				int index, double x);
#endif
#ifndef Blt_FreeParametricCubicSpline_DECLARED
#define Blt_FreeParametricCubicSpline_DECLARED
/* 95 */
BLT_EXTERN void		Blt_FreeParametricCubicSpline(Blt_Spline spline);
#endif
#ifndef Blt_CreateCatromSpline_DECLARED
#define Blt_CreateCatromSpline_DECLARED
/* 96 */
BLT_EXTERN Blt_Spline	Blt_CreateCatromSpline(Point2d *points, int n);
#endif
#ifndef Blt_EvaluateCatromSpline_DECLARED
#define Blt_EvaluateCatromSpline_DECLARED
/* 97 */
BLT_EXTERN Point2d	Blt_EvaluateCatromSpline(Blt_Spline spline, int i,
				double t);
#endif
#ifndef Blt_FreeCatromSpline_DECLARED
#define Blt_FreeCatromSpline_DECLARED
/* 98 */
BLT_EXTERN void		Blt_FreeCatromSpline(Blt_Spline spline);
#endif
#ifndef Blt_ComputeNaturalSpline_DECLARED
#define Blt_ComputeNaturalSpline_DECLARED
/* 99 */
BLT_EXTERN int		Blt_ComputeNaturalSpline(Point2d *origPts,
				int numOrigPts, Point2d *intpPts,
				int numIntpPts);
#endif
#ifndef Blt_ComputeQuadraticSpline_DECLARED
#define Blt_ComputeQuadraticSpline_DECLARED
/* 100 */
BLT_EXTERN int		Blt_ComputeQuadraticSpline(Point2d *origPts,
				int numOrigPts, Point2d *intpPts,
				int numIntpPts);
#endif
#ifndef Blt_ComputeNaturalParametricSpline_DECLARED
#define Blt_ComputeNaturalParametricSpline_DECLARED
/* 101 */
BLT_EXTERN int		Blt_ComputeNaturalParametricSpline(Point2d *origPts,
				int numOrigPts, Region2d *extsPtr,
				int isClosed, Point2d *intpPts,
				int numIntpPts);
#endif
#ifndef Blt_ComputeCatromParametricSpline_DECLARED
#define Blt_ComputeCatromParametricSpline_DECLARED
/* 102 */
BLT_EXTERN int		Blt_ComputeCatromParametricSpline(Point2d *origPts,
				int numOrigPts, Point2d *intpPts,
				int numIntpPts);
#endif
#ifndef Blt_ParseSwitches_DECLARED
#define Blt_ParseSwitches_DECLARED
/* 103 */
BLT_EXTERN int		Blt_ParseSwitches(Tcl_Interp *interp,
				Blt_SwitchSpec *specPtr, int objc,
				Tcl_Obj *const *objv, void *rec, int flags);
#endif
#ifndef Blt_FreeSwitches_DECLARED
#define Blt_FreeSwitches_DECLARED
/* 104 */
BLT_EXTERN void		Blt_FreeSwitches(Blt_SwitchSpec *specs, void *rec,
				int flags);
#endif
#ifndef Blt_SwitchChanged_DECLARED
#define Blt_SwitchChanged_DECLARED
/* 105 */
BLT_EXTERN int		Blt_SwitchChanged(Blt_SwitchSpec *specs, ...);
#endif
#ifndef Blt_SwitchInfo_DECLARED
#define Blt_SwitchInfo_DECLARED
/* 106 */
BLT_EXTERN int		Blt_SwitchInfo(Tcl_Interp *interp,
				Blt_SwitchSpec *specs, void *record,
				Tcl_Obj *objPtr, int flags);
#endif
#ifndef Blt_SwitchValue_DECLARED
#define Blt_SwitchValue_DECLARED
/* 107 */
BLT_EXTERN int		Blt_SwitchValue(Tcl_Interp *interp,
				Blt_SwitchSpec *specs, void *record,
				Tcl_Obj *objPtr, int flags);
#endif
#ifndef Blt_GetCachedVar_DECLARED
#define Blt_GetCachedVar_DECLARED
/* 108 */
BLT_EXTERN Tcl_Var	Blt_GetCachedVar(Blt_HashTable *tablePtr,
				const char *label, Tcl_Obj *objPtr);
#endif
#ifndef Blt_FreeCachedVars_DECLARED
#define Blt_FreeCachedVars_DECLARED
/* 109 */
BLT_EXTERN void		Blt_FreeCachedVars(Blt_HashTable *tablePtr);
#endif

typedef struct BltTclIntProcs {
    int magic;
    struct BltTclIntStubHooks *hooks;

    void *reserved0;
    void (*blt_DBuffer_VarAppend) (Blt_DBuffer buffer, ...); /* 1 */
    int (*blt_DBuffer_Format) (Blt_DBuffer buffer, const char *fmt, ...); /* 2 */
    void (*blt_DBuffer_Init) (Blt_DBuffer buffer); /* 3 */
    void (*blt_DBuffer_Free) (Blt_DBuffer buffer); /* 4 */
    unsigned char * (*blt_DBuffer_Extend) (Blt_DBuffer buffer, size_t extra); /* 5 */
    int (*blt_DBuffer_AppendData) (Blt_DBuffer buffer, const unsigned char *bytes, size_t extra); /* 6 */
    int (*blt_DBuffer_Concat) (Blt_DBuffer dest, Blt_DBuffer src); /* 7 */
    int (*blt_DBuffer_Resize) (Blt_DBuffer buffer, size_t length); /* 8 */
    int (*blt_DBuffer_SetLength) (Blt_DBuffer buffer, size_t length); /* 9 */
    Blt_DBuffer (*blt_DBuffer_Create) (void); /* 10 */
    void (*blt_DBuffer_Destroy) (Blt_DBuffer buffer); /* 11 */
    int (*blt_DBuffer_LoadFile) (Tcl_Interp *interp, const char *fileName, Blt_DBuffer buffer); /* 12 */
    int (*blt_DBuffer_SaveFile) (Tcl_Interp *interp, const char *fileName, Blt_DBuffer buffer); /* 13 */
    void (*blt_DBuffer_AppendByte) (Blt_DBuffer buffer, unsigned char byte); /* 14 */
    void (*blt_DBuffer_AppendShort) (Blt_DBuffer buffer, unsigned short value); /* 15 */
    void (*blt_DBuffer_AppendInt) (Blt_DBuffer buffer, unsigned int value); /* 16 */
    Tcl_Obj * (*blt_DBuffer_ByteArrayObj) (Blt_DBuffer buffer); /* 17 */
    Tcl_Obj * (*blt_DBuffer_StringObj) (Blt_DBuffer buffer); /* 18 */
    const char * (*blt_DBuffer_String) (Blt_DBuffer buffer); /* 19 */
    int (*blt_DBuffer_Base64Decode) (Tcl_Interp *interp, const char *string, size_t length, Blt_DBuffer buffer); /* 20 */
    const char * (*blt_DBuffer_Base64Encode) (Tcl_Interp *interp, Blt_DBuffer buffer); /* 21 */
    Tcl_Obj * (*blt_DBuffer_Base64EncodeToObj) (Tcl_Interp *interp, Blt_DBuffer buffer); /* 22 */
    VOID * (*blt_Malloc) (size_t size); /* 23 */
    VOID * (*blt_Realloc) (VOID *ptr, size_t size); /* 24 */
    void (*blt_Free) (const VOID *ptr); /* 25 */
    VOID * (*blt_Calloc) (size_t numElem, size_t size); /* 26 */
    char * (*blt_Strdup) (const char *string); /* 27 */
    VOID * (*blt_MallocAbortOnError) (size_t size, const char *file, int line); /* 28 */
    VOID * (*blt_CallocAbortOnError) (size_t numElem, size_t size, const char *file, int line); /* 29 */
    char * (*blt_StrdupAbortOnError) (const char *ptr, const char *file, int line); /* 30 */
    int (*blt_DictionaryCompare) (const char *s1, const char *s2); /* 31 */
    Blt_Uid (*blt_GetUid) (const char *string); /* 32 */
    void (*blt_FreeUid) (Blt_Uid uid); /* 33 */
    Blt_Uid (*blt_FindUid) (const char *string); /* 34 */
    int (*blt_CreatePipeline) (Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, Blt_Pid **pidArrayPtr, int *stdinPipePtr, int *stdoutPipePtr, int *stderrPipePtr); /* 35 */
    void (*blt_InitHexTable) (unsigned char *table); /* 36 */
    void (*blt_DStringAppendElements) (Tcl_DString *dsPtr, ...); /* 37 */
    int (*blt_LoadLibrary) (Tcl_Interp *interp, const char *libPath, const char *initProcName, const char *safeProcName); /* 38 */
    void (*blt_Panic) (const char *fmt, ...); /* 39 */
    void (*blt_Warn) (const char *fmt, ...); /* 40 */
    int (*blt_GetSideFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *sidePtr); /* 41 */
    const char * (*blt_NameOfSide) (int side); /* 42 */
    FILE * (*blt_OpenFile) (Tcl_Interp *interp, const char *fileName, const char *mode); /* 43 */
    int (*blt_ExprDoubleFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, double *valuePtr); /* 44 */
    int (*blt_ExprIntFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int *valuePtr); /* 45 */
    const char * (*blt_Itoa) (int value); /* 46 */
    const char * (*blt_Ltoa) (long value); /* 47 */
    const char * (*blt_Utoa) (unsigned int value); /* 48 */
    const char * (*blt_Dtoa) (Tcl_Interp *interp, double value); /* 49 */
    unsigned char * (*blt_Base64_Decode) (Tcl_Interp *interp, const char *string, size_t *lengthPtr); /* 50 */
    Blt_DBuffer (*blt_Base64_DecodeToBuffer) (Tcl_Interp *interp, const char *string, size_t length); /* 51 */
    Tcl_Obj * (*blt_Base64_DecodeToObj) (Tcl_Interp *interp, const char *string, size_t length); /* 52 */
    const char * (*blt_Base64_Encode) (Tcl_Interp *interp, const unsigned char *buffer, size_t bufsize); /* 53 */
    Tcl_Obj * (*blt_Base64_EncodeToObj) (Tcl_Interp *interp, const unsigned char *buffer, size_t bufsize); /* 54 */
    const char * (*blt_Base85_Encode) (Tcl_Interp *interp, const unsigned char *buffer, size_t bufsize); /* 55 */
    const char * (*blt_Base16_Encode) (Tcl_Interp *interp, const unsigned char *buffer, size_t bufsize); /* 56 */
    int (*blt_IsBase64) (const char *buf, size_t length); /* 57 */
    int (*blt_GetDoubleFromString) (Tcl_Interp *interp, const char *s, double *valuePtr); /* 58 */
    int (*blt_GetDoubleFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, double *valuePtr); /* 59 */
    int (*blt_GetTimeFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, double *valuePtr); /* 60 */
    int (*blt_GetTime) (Tcl_Interp *interp, const char *string, double *valuePtr); /* 61 */
    int (*blt_GetDateFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, double *timePtr); /* 62 */
    int (*blt_GetDate) (Tcl_Interp *interp, const char *string, double *timePtr); /* 63 */
    int (*blt_GetPositionFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, long *indexPtr); /* 64 */
    int (*blt_GetCountFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, int check, long *valuePtr); /* 65 */
    int (*blt_SimplifyLine) (Point2d *origPts, int low, int high, double tolerance, int *indices); /* 66 */
    int (*blt_GetLong) (Tcl_Interp *interp, const char *s, long *longPtr); /* 67 */
    int (*blt_GetLongFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, long *longPtr); /* 68 */
    int (*blt_FormatString) (char *s, size_t size, const char *fmt, ...); /* 69 */
    void (*blt_LowerCase) (char *s); /* 70 */
    int (*blt_GetPlatformId) (void); /* 71 */
    const char * (*blt_LastError) (void); /* 72 */
    double (*blt_NaN) (void); /* 73 */
    int (*blt_AlmostEquals) (double x, double y); /* 74 */
    int (*blt_GetArrayFromObj) (Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_HashTable **tablePtrPtr); /* 75 */
    Tcl_Obj * (*blt_NewArrayObj) (int objc, Tcl_Obj *objv[]); /* 76 */
    void (*blt_RegisterArrayObj) (void); /* 77 */
    int (*blt_IsArrayObj) (Tcl_Obj *obj); /* 78 */
    void (*blt_Assert) (const char *expr, const char *file, int line); /* 79 */
    int (*blt_InitCmd) (Tcl_Interp *interp, const char *namespace, Blt_CmdSpec *specPtr); /* 80 */
    int (*blt_InitCmds) (Tcl_Interp *interp, const char *namespace, Blt_CmdSpec *specPtr, int numCmds); /* 81 */
    VOID * (*blt_GetOpFromObj) (Tcl_Interp *interp, int numSpecs, Blt_OpSpec *specs, int operPos, int objc, Tcl_Obj *const *objv, int flags); /* 82 */
    Tcl_Namespace * (*blt_GetVariableNamespace) (Tcl_Interp *interp, const char *varName); /* 83 */
    Tcl_Namespace * (*blt_GetCommandNamespace) (Tcl_Command cmdToken); /* 84 */
    Tcl_CallFrame * (*blt_EnterNamespace) (Tcl_Interp *interp, Tcl_Namespace *nsPtr); /* 85 */
    void (*blt_LeaveNamespace) (Tcl_Interp *interp, Tcl_CallFrame *framePtr); /* 86 */
    int (*blt_ParseObjectName) (Tcl_Interp *interp, const char *name, Blt_ObjectName *objNamePtr, unsigned int flags); /* 87 */
    const char * (*blt_MakeQualifiedName) (Blt_ObjectName *objNamePtr, Tcl_DString *resultPtr); /* 88 */
    int (*blt_CommandExists) (Tcl_Interp *interp, const char *string); /* 89 */
    Blt_Spline (*blt_CreateSpline) (Point2d *points, int n, int type); /* 90 */
    Point2d (*blt_EvaluateSpline) (Blt_Spline spline, int index, double x); /* 91 */
    void (*blt_FreeSpline) (Blt_Spline spline); /* 92 */
    Blt_Spline (*blt_CreateParametricCubicSpline) (Point2d *points, int n, int w, int h); /* 93 */
    Point2d (*blt_EvaluateParametricCubicSpline) (Blt_Spline spline, int index, double x); /* 94 */
    void (*blt_FreeParametricCubicSpline) (Blt_Spline spline); /* 95 */
    Blt_Spline (*blt_CreateCatromSpline) (Point2d *points, int n); /* 96 */
    Point2d (*blt_EvaluateCatromSpline) (Blt_Spline spline, int i, double t); /* 97 */
    void (*blt_FreeCatromSpline) (Blt_Spline spline); /* 98 */
    int (*blt_ComputeNaturalSpline) (Point2d *origPts, int numOrigPts, Point2d *intpPts, int numIntpPts); /* 99 */
    int (*blt_ComputeQuadraticSpline) (Point2d *origPts, int numOrigPts, Point2d *intpPts, int numIntpPts); /* 100 */
    int (*blt_ComputeNaturalParametricSpline) (Point2d *origPts, int numOrigPts, Region2d *extsPtr, int isClosed, Point2d *intpPts, int numIntpPts); /* 101 */
    int (*blt_ComputeCatromParametricSpline) (Point2d *origPts, int numOrigPts, Point2d *intpPts, int numIntpPts); /* 102 */
    int (*blt_ParseSwitches) (Tcl_Interp *interp, Blt_SwitchSpec *specPtr, int objc, Tcl_Obj *const *objv, VOID *rec, int flags); /* 103 */
    void (*blt_FreeSwitches) (Blt_SwitchSpec *specs, VOID *rec, int flags); /* 104 */
    int (*blt_SwitchChanged) (Blt_SwitchSpec *specs, ...); /* 105 */
    int (*blt_SwitchInfo) (Tcl_Interp *interp, Blt_SwitchSpec *specs, VOID *record, Tcl_Obj *objPtr, int flags); /* 106 */
    int (*blt_SwitchValue) (Tcl_Interp *interp, Blt_SwitchSpec *specs, VOID *record, Tcl_Obj *objPtr, int flags); /* 107 */
    Tcl_Var (*blt_GetCachedVar) (Blt_HashTable *tablePtr, const char *label, Tcl_Obj *objPtr); /* 108 */
    void (*blt_FreeCachedVars) (Blt_HashTable *tablePtr); /* 109 */
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
#ifndef Blt_DBuffer_VarAppend
#define Blt_DBuffer_VarAppend \
	(bltTclIntProcsPtr->blt_DBuffer_VarAppend) /* 1 */
#endif
#ifndef Blt_DBuffer_Format
#define Blt_DBuffer_Format \
	(bltTclIntProcsPtr->blt_DBuffer_Format) /* 2 */
#endif
#ifndef Blt_DBuffer_Init
#define Blt_DBuffer_Init \
	(bltTclIntProcsPtr->blt_DBuffer_Init) /* 3 */
#endif
#ifndef Blt_DBuffer_Free
#define Blt_DBuffer_Free \
	(bltTclIntProcsPtr->blt_DBuffer_Free) /* 4 */
#endif
#ifndef Blt_DBuffer_Extend
#define Blt_DBuffer_Extend \
	(bltTclIntProcsPtr->blt_DBuffer_Extend) /* 5 */
#endif
#ifndef Blt_DBuffer_AppendData
#define Blt_DBuffer_AppendData \
	(bltTclIntProcsPtr->blt_DBuffer_AppendData) /* 6 */
#endif
#ifndef Blt_DBuffer_Concat
#define Blt_DBuffer_Concat \
	(bltTclIntProcsPtr->blt_DBuffer_Concat) /* 7 */
#endif
#ifndef Blt_DBuffer_Resize
#define Blt_DBuffer_Resize \
	(bltTclIntProcsPtr->blt_DBuffer_Resize) /* 8 */
#endif
#ifndef Blt_DBuffer_SetLength
#define Blt_DBuffer_SetLength \
	(bltTclIntProcsPtr->blt_DBuffer_SetLength) /* 9 */
#endif
#ifndef Blt_DBuffer_Create
#define Blt_DBuffer_Create \
	(bltTclIntProcsPtr->blt_DBuffer_Create) /* 10 */
#endif
#ifndef Blt_DBuffer_Destroy
#define Blt_DBuffer_Destroy \
	(bltTclIntProcsPtr->blt_DBuffer_Destroy) /* 11 */
#endif
#ifndef Blt_DBuffer_LoadFile
#define Blt_DBuffer_LoadFile \
	(bltTclIntProcsPtr->blt_DBuffer_LoadFile) /* 12 */
#endif
#ifndef Blt_DBuffer_SaveFile
#define Blt_DBuffer_SaveFile \
	(bltTclIntProcsPtr->blt_DBuffer_SaveFile) /* 13 */
#endif
#ifndef Blt_DBuffer_AppendByte
#define Blt_DBuffer_AppendByte \
	(bltTclIntProcsPtr->blt_DBuffer_AppendByte) /* 14 */
#endif
#ifndef Blt_DBuffer_AppendShort
#define Blt_DBuffer_AppendShort \
	(bltTclIntProcsPtr->blt_DBuffer_AppendShort) /* 15 */
#endif
#ifndef Blt_DBuffer_AppendInt
#define Blt_DBuffer_AppendInt \
	(bltTclIntProcsPtr->blt_DBuffer_AppendInt) /* 16 */
#endif
#ifndef Blt_DBuffer_ByteArrayObj
#define Blt_DBuffer_ByteArrayObj \
	(bltTclIntProcsPtr->blt_DBuffer_ByteArrayObj) /* 17 */
#endif
#ifndef Blt_DBuffer_StringObj
#define Blt_DBuffer_StringObj \
	(bltTclIntProcsPtr->blt_DBuffer_StringObj) /* 18 */
#endif
#ifndef Blt_DBuffer_String
#define Blt_DBuffer_String \
	(bltTclIntProcsPtr->blt_DBuffer_String) /* 19 */
#endif
#ifndef Blt_DBuffer_Base64Decode
#define Blt_DBuffer_Base64Decode \
	(bltTclIntProcsPtr->blt_DBuffer_Base64Decode) /* 20 */
#endif
#ifndef Blt_DBuffer_Base64Encode
#define Blt_DBuffer_Base64Encode \
	(bltTclIntProcsPtr->blt_DBuffer_Base64Encode) /* 21 */
#endif
#ifndef Blt_DBuffer_Base64EncodeToObj
#define Blt_DBuffer_Base64EncodeToObj \
	(bltTclIntProcsPtr->blt_DBuffer_Base64EncodeToObj) /* 22 */
#endif
#ifndef Blt_Malloc
#define Blt_Malloc \
	(bltTclIntProcsPtr->blt_Malloc) /* 23 */
#endif
#ifndef Blt_Realloc
#define Blt_Realloc \
	(bltTclIntProcsPtr->blt_Realloc) /* 24 */
#endif
#ifndef Blt_Free
#define Blt_Free \
	(bltTclIntProcsPtr->blt_Free) /* 25 */
#endif
#ifndef Blt_Calloc
#define Blt_Calloc \
	(bltTclIntProcsPtr->blt_Calloc) /* 26 */
#endif
#ifndef Blt_Strdup
#define Blt_Strdup \
	(bltTclIntProcsPtr->blt_Strdup) /* 27 */
#endif
#ifndef Blt_MallocAbortOnError
#define Blt_MallocAbortOnError \
	(bltTclIntProcsPtr->blt_MallocAbortOnError) /* 28 */
#endif
#ifndef Blt_CallocAbortOnError
#define Blt_CallocAbortOnError \
	(bltTclIntProcsPtr->blt_CallocAbortOnError) /* 29 */
#endif
#ifndef Blt_StrdupAbortOnError
#define Blt_StrdupAbortOnError \
	(bltTclIntProcsPtr->blt_StrdupAbortOnError) /* 30 */
#endif
#ifndef Blt_DictionaryCompare
#define Blt_DictionaryCompare \
	(bltTclIntProcsPtr->blt_DictionaryCompare) /* 31 */
#endif
#ifndef Blt_GetUid
#define Blt_GetUid \
	(bltTclIntProcsPtr->blt_GetUid) /* 32 */
#endif
#ifndef Blt_FreeUid
#define Blt_FreeUid \
	(bltTclIntProcsPtr->blt_FreeUid) /* 33 */
#endif
#ifndef Blt_FindUid
#define Blt_FindUid \
	(bltTclIntProcsPtr->blt_FindUid) /* 34 */
#endif
#ifndef Blt_CreatePipeline
#define Blt_CreatePipeline \
	(bltTclIntProcsPtr->blt_CreatePipeline) /* 35 */
#endif
#ifndef Blt_InitHexTable
#define Blt_InitHexTable \
	(bltTclIntProcsPtr->blt_InitHexTable) /* 36 */
#endif
#ifndef Blt_DStringAppendElements
#define Blt_DStringAppendElements \
	(bltTclIntProcsPtr->blt_DStringAppendElements) /* 37 */
#endif
#ifndef Blt_LoadLibrary
#define Blt_LoadLibrary \
	(bltTclIntProcsPtr->blt_LoadLibrary) /* 38 */
#endif
#ifndef Blt_Panic
#define Blt_Panic \
	(bltTclIntProcsPtr->blt_Panic) /* 39 */
#endif
#ifndef Blt_Warn
#define Blt_Warn \
	(bltTclIntProcsPtr->blt_Warn) /* 40 */
#endif
#ifndef Blt_GetSideFromObj
#define Blt_GetSideFromObj \
	(bltTclIntProcsPtr->blt_GetSideFromObj) /* 41 */
#endif
#ifndef Blt_NameOfSide
#define Blt_NameOfSide \
	(bltTclIntProcsPtr->blt_NameOfSide) /* 42 */
#endif
#ifndef Blt_OpenFile
#define Blt_OpenFile \
	(bltTclIntProcsPtr->blt_OpenFile) /* 43 */
#endif
#ifndef Blt_ExprDoubleFromObj
#define Blt_ExprDoubleFromObj \
	(bltTclIntProcsPtr->blt_ExprDoubleFromObj) /* 44 */
#endif
#ifndef Blt_ExprIntFromObj
#define Blt_ExprIntFromObj \
	(bltTclIntProcsPtr->blt_ExprIntFromObj) /* 45 */
#endif
#ifndef Blt_Itoa
#define Blt_Itoa \
	(bltTclIntProcsPtr->blt_Itoa) /* 46 */
#endif
#ifndef Blt_Ltoa
#define Blt_Ltoa \
	(bltTclIntProcsPtr->blt_Ltoa) /* 47 */
#endif
#ifndef Blt_Utoa
#define Blt_Utoa \
	(bltTclIntProcsPtr->blt_Utoa) /* 48 */
#endif
#ifndef Blt_Dtoa
#define Blt_Dtoa \
	(bltTclIntProcsPtr->blt_Dtoa) /* 49 */
#endif
#ifndef Blt_Base64_Decode
#define Blt_Base64_Decode \
	(bltTclIntProcsPtr->blt_Base64_Decode) /* 50 */
#endif
#ifndef Blt_Base64_DecodeToBuffer
#define Blt_Base64_DecodeToBuffer \
	(bltTclIntProcsPtr->blt_Base64_DecodeToBuffer) /* 51 */
#endif
#ifndef Blt_Base64_DecodeToObj
#define Blt_Base64_DecodeToObj \
	(bltTclIntProcsPtr->blt_Base64_DecodeToObj) /* 52 */
#endif
#ifndef Blt_Base64_Encode
#define Blt_Base64_Encode \
	(bltTclIntProcsPtr->blt_Base64_Encode) /* 53 */
#endif
#ifndef Blt_Base64_EncodeToObj
#define Blt_Base64_EncodeToObj \
	(bltTclIntProcsPtr->blt_Base64_EncodeToObj) /* 54 */
#endif
#ifndef Blt_Base85_Encode
#define Blt_Base85_Encode \
	(bltTclIntProcsPtr->blt_Base85_Encode) /* 55 */
#endif
#ifndef Blt_Base16_Encode
#define Blt_Base16_Encode \
	(bltTclIntProcsPtr->blt_Base16_Encode) /* 56 */
#endif
#ifndef Blt_IsBase64
#define Blt_IsBase64 \
	(bltTclIntProcsPtr->blt_IsBase64) /* 57 */
#endif
#ifndef Blt_GetDoubleFromString
#define Blt_GetDoubleFromString \
	(bltTclIntProcsPtr->blt_GetDoubleFromString) /* 58 */
#endif
#ifndef Blt_GetDoubleFromObj
#define Blt_GetDoubleFromObj \
	(bltTclIntProcsPtr->blt_GetDoubleFromObj) /* 59 */
#endif
#ifndef Blt_GetTimeFromObj
#define Blt_GetTimeFromObj \
	(bltTclIntProcsPtr->blt_GetTimeFromObj) /* 60 */
#endif
#ifndef Blt_GetTime
#define Blt_GetTime \
	(bltTclIntProcsPtr->blt_GetTime) /* 61 */
#endif
#ifndef Blt_GetDateFromObj
#define Blt_GetDateFromObj \
	(bltTclIntProcsPtr->blt_GetDateFromObj) /* 62 */
#endif
#ifndef Blt_GetDate
#define Blt_GetDate \
	(bltTclIntProcsPtr->blt_GetDate) /* 63 */
#endif
#ifndef Blt_GetPositionFromObj
#define Blt_GetPositionFromObj \
	(bltTclIntProcsPtr->blt_GetPositionFromObj) /* 64 */
#endif
#ifndef Blt_GetCountFromObj
#define Blt_GetCountFromObj \
	(bltTclIntProcsPtr->blt_GetCountFromObj) /* 65 */
#endif
#ifndef Blt_SimplifyLine
#define Blt_SimplifyLine \
	(bltTclIntProcsPtr->blt_SimplifyLine) /* 66 */
#endif
#ifndef Blt_GetLong
#define Blt_GetLong \
	(bltTclIntProcsPtr->blt_GetLong) /* 67 */
#endif
#ifndef Blt_GetLongFromObj
#define Blt_GetLongFromObj \
	(bltTclIntProcsPtr->blt_GetLongFromObj) /* 68 */
#endif
#ifndef Blt_FormatString
#define Blt_FormatString \
	(bltTclIntProcsPtr->blt_FormatString) /* 69 */
#endif
#ifndef Blt_LowerCase
#define Blt_LowerCase \
	(bltTclIntProcsPtr->blt_LowerCase) /* 70 */
#endif
#ifndef Blt_GetPlatformId
#define Blt_GetPlatformId \
	(bltTclIntProcsPtr->blt_GetPlatformId) /* 71 */
#endif
#ifndef Blt_LastError
#define Blt_LastError \
	(bltTclIntProcsPtr->blt_LastError) /* 72 */
#endif
#ifndef Blt_NaN
#define Blt_NaN \
	(bltTclIntProcsPtr->blt_NaN) /* 73 */
#endif
#ifndef Blt_AlmostEquals
#define Blt_AlmostEquals \
	(bltTclIntProcsPtr->blt_AlmostEquals) /* 74 */
#endif
#ifndef Blt_GetArrayFromObj
#define Blt_GetArrayFromObj \
	(bltTclIntProcsPtr->blt_GetArrayFromObj) /* 75 */
#endif
#ifndef Blt_NewArrayObj
#define Blt_NewArrayObj \
	(bltTclIntProcsPtr->blt_NewArrayObj) /* 76 */
#endif
#ifndef Blt_RegisterArrayObj
#define Blt_RegisterArrayObj \
	(bltTclIntProcsPtr->blt_RegisterArrayObj) /* 77 */
#endif
#ifndef Blt_IsArrayObj
#define Blt_IsArrayObj \
	(bltTclIntProcsPtr->blt_IsArrayObj) /* 78 */
#endif
#ifndef Blt_Assert
#define Blt_Assert \
	(bltTclIntProcsPtr->blt_Assert) /* 79 */
#endif
#ifndef Blt_InitCmd
#define Blt_InitCmd \
	(bltTclIntProcsPtr->blt_InitCmd) /* 80 */
#endif
#ifndef Blt_InitCmds
#define Blt_InitCmds \
	(bltTclIntProcsPtr->blt_InitCmds) /* 81 */
#endif
#ifndef Blt_GetOpFromObj
#define Blt_GetOpFromObj \
	(bltTclIntProcsPtr->blt_GetOpFromObj) /* 82 */
#endif
#ifndef Blt_GetVariableNamespace
#define Blt_GetVariableNamespace \
	(bltTclIntProcsPtr->blt_GetVariableNamespace) /* 83 */
#endif
#ifndef Blt_GetCommandNamespace
#define Blt_GetCommandNamespace \
	(bltTclIntProcsPtr->blt_GetCommandNamespace) /* 84 */
#endif
#ifndef Blt_EnterNamespace
#define Blt_EnterNamespace \
	(bltTclIntProcsPtr->blt_EnterNamespace) /* 85 */
#endif
#ifndef Blt_LeaveNamespace
#define Blt_LeaveNamespace \
	(bltTclIntProcsPtr->blt_LeaveNamespace) /* 86 */
#endif
#ifndef Blt_ParseObjectName
#define Blt_ParseObjectName \
	(bltTclIntProcsPtr->blt_ParseObjectName) /* 87 */
#endif
#ifndef Blt_MakeQualifiedName
#define Blt_MakeQualifiedName \
	(bltTclIntProcsPtr->blt_MakeQualifiedName) /* 88 */
#endif
#ifndef Blt_CommandExists
#define Blt_CommandExists \
	(bltTclIntProcsPtr->blt_CommandExists) /* 89 */
#endif
#ifndef Blt_CreateSpline
#define Blt_CreateSpline \
	(bltTclIntProcsPtr->blt_CreateSpline) /* 90 */
#endif
#ifndef Blt_EvaluateSpline
#define Blt_EvaluateSpline \
	(bltTclIntProcsPtr->blt_EvaluateSpline) /* 91 */
#endif
#ifndef Blt_FreeSpline
#define Blt_FreeSpline \
	(bltTclIntProcsPtr->blt_FreeSpline) /* 92 */
#endif
#ifndef Blt_CreateParametricCubicSpline
#define Blt_CreateParametricCubicSpline \
	(bltTclIntProcsPtr->blt_CreateParametricCubicSpline) /* 93 */
#endif
#ifndef Blt_EvaluateParametricCubicSpline
#define Blt_EvaluateParametricCubicSpline \
	(bltTclIntProcsPtr->blt_EvaluateParametricCubicSpline) /* 94 */
#endif
#ifndef Blt_FreeParametricCubicSpline
#define Blt_FreeParametricCubicSpline \
	(bltTclIntProcsPtr->blt_FreeParametricCubicSpline) /* 95 */
#endif
#ifndef Blt_CreateCatromSpline
#define Blt_CreateCatromSpline \
	(bltTclIntProcsPtr->blt_CreateCatromSpline) /* 96 */
#endif
#ifndef Blt_EvaluateCatromSpline
#define Blt_EvaluateCatromSpline \
	(bltTclIntProcsPtr->blt_EvaluateCatromSpline) /* 97 */
#endif
#ifndef Blt_FreeCatromSpline
#define Blt_FreeCatromSpline \
	(bltTclIntProcsPtr->blt_FreeCatromSpline) /* 98 */
#endif
#ifndef Blt_ComputeNaturalSpline
#define Blt_ComputeNaturalSpline \
	(bltTclIntProcsPtr->blt_ComputeNaturalSpline) /* 99 */
#endif
#ifndef Blt_ComputeQuadraticSpline
#define Blt_ComputeQuadraticSpline \
	(bltTclIntProcsPtr->blt_ComputeQuadraticSpline) /* 100 */
#endif
#ifndef Blt_ComputeNaturalParametricSpline
#define Blt_ComputeNaturalParametricSpline \
	(bltTclIntProcsPtr->blt_ComputeNaturalParametricSpline) /* 101 */
#endif
#ifndef Blt_ComputeCatromParametricSpline
#define Blt_ComputeCatromParametricSpline \
	(bltTclIntProcsPtr->blt_ComputeCatromParametricSpline) /* 102 */
#endif
#ifndef Blt_ParseSwitches
#define Blt_ParseSwitches \
	(bltTclIntProcsPtr->blt_ParseSwitches) /* 103 */
#endif
#ifndef Blt_FreeSwitches
#define Blt_FreeSwitches \
	(bltTclIntProcsPtr->blt_FreeSwitches) /* 104 */
#endif
#ifndef Blt_SwitchChanged
#define Blt_SwitchChanged \
	(bltTclIntProcsPtr->blt_SwitchChanged) /* 105 */
#endif
#ifndef Blt_SwitchInfo
#define Blt_SwitchInfo \
	(bltTclIntProcsPtr->blt_SwitchInfo) /* 106 */
#endif
#ifndef Blt_SwitchValue
#define Blt_SwitchValue \
	(bltTclIntProcsPtr->blt_SwitchValue) /* 107 */
#endif
#ifndef Blt_GetCachedVar
#define Blt_GetCachedVar \
	(bltTclIntProcsPtr->blt_GetCachedVar) /* 108 */
#endif
#ifndef Blt_FreeCachedVars
#define Blt_FreeCachedVars \
	(bltTclIntProcsPtr->blt_FreeCachedVars) /* 109 */
#endif

#endif /* defined(USE_BLT_STUBS) && !defined(BUILD_BLT_TCL_PROCS) */

/* !END!: Do not edit above this line. */

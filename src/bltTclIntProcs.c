/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#define BUILD_BLT_TCL_PROCS 1
#include <bltInt.h>

/* !BEGIN!: Do not edit below this line. */

BltTclIntProcs bltTclIntProcs = {
    TCL_STUB_MAGIC,
    NULL,
    NULL, /* 0 */
    Blt_GetArrayFromObj, /* 1 */
    Blt_NewArrayObj, /* 2 */
    Blt_IsArrayObj, /* 3 */
    Blt_Assert, /* 4 */
    Blt_DBuffer_VarAppend, /* 5 */
    Blt_DBuffer_Format, /* 6 */
    Blt_DBuffer_Init, /* 7 */
    Blt_DBuffer_Free, /* 8 */
    Blt_DBuffer_Extend, /* 9 */
    Blt_DBuffer_AppendData, /* 10 */
    Blt_DBuffer_AppendString, /* 11 */
    Blt_DBuffer_DeleteData, /* 12 */
    Blt_DBuffer_InsertData, /* 13 */
    Blt_DBuffer_SetFromObj, /* 14 */
    Blt_DBuffer_Concat, /* 15 */
    Blt_DBuffer_Resize, /* 16 */
    Blt_DBuffer_SetLength, /* 17 */
    Blt_DBuffer_Create, /* 18 */
    Blt_DBuffer_Destroy, /* 19 */
    Blt_DBuffer_LoadFile, /* 20 */
    Blt_DBuffer_SaveFile, /* 21 */
    Blt_DBuffer_AppendByte, /* 22 */
    Blt_DBuffer_AppendShort, /* 23 */
    Blt_DBuffer_AppendInt, /* 24 */
    Blt_DBuffer_ByteArrayObj, /* 25 */
    Blt_DBuffer_StringObj, /* 26 */
    Blt_DBuffer_String, /* 27 */
    Blt_DBuffer_Base64Decode, /* 28 */
    Blt_DBuffer_Base64EncodeToObj, /* 29 */
    Blt_DBuffer_AppendBase85, /* 30 */
    Blt_DBuffer_AppendBase64, /* 31 */
    Blt_SimplifyLine, /* 32 */
    Blt_LineRectClip, /* 33 */
    Blt_PointInPolygon, /* 34 */
    Blt_PolygonInRegion, /* 35 */
    Blt_PointInSegments, /* 36 */
    Blt_PolyRectClip, /* 37 */
    Blt_GetProjection, /* 38 */
    Blt_GetProjection2, /* 39 */
    Blt_ConvexHull, /* 40 */
    Blt_InitCmd, /* 41 */
    Blt_InitCmds, /* 42 */
    Blt_FreeMesh, /* 43 */
    Blt_GetMeshFromObj, /* 44 */
    Blt_GetMesh, /* 45 */
    Blt_Triangulate, /* 46 */
    Blt_Mesh_CreateNotifier, /* 47 */
    Blt_Mesh_DeleteNotifier, /* 48 */
    Blt_Mesh_Name, /* 49 */
    Blt_Mesh_Type, /* 50 */
    Blt_Mesh_GetVertices, /* 51 */
    Blt_Mesh_GetHull, /* 52 */
    Blt_Mesh_GetExtents, /* 53 */
    Blt_Mesh_GetTriangles, /* 54 */
    Blt_GetVariableNamespace, /* 55 */
    Blt_GetCommandNamespace, /* 56 */
    Blt_EnterNamespace, /* 57 */
    Blt_LeaveNamespace, /* 58 */
    Blt_ParseObjectName, /* 59 */
    Blt_MakeQualifiedName, /* 60 */
    Blt_CommandExists, /* 61 */
    Blt_GetOpFromObj, /* 62 */
    Blt_CreateSpline, /* 63 */
    Blt_EvaluateSpline, /* 64 */
    Blt_FreeSpline, /* 65 */
    Blt_CreateParametricCubicSpline, /* 66 */
    Blt_EvaluateParametricCubicSpline, /* 67 */
    Blt_FreeParametricCubicSpline, /* 68 */
    Blt_CreateCatromSpline, /* 69 */
    Blt_EvaluateCatromSpline, /* 70 */
    Blt_FreeCatromSpline, /* 71 */
    Blt_ComputeNaturalSpline, /* 72 */
    Blt_ComputeQuadraticSpline, /* 73 */
    Blt_ComputeNaturalParametricSpline, /* 74 */
    Blt_ComputeCatromParametricSpline, /* 75 */
    Blt_ExprDoubleFromObj, /* 76 */
    Blt_ExprIntFromObj, /* 77 */
    Blt_GetStateFromObj, /* 78 */
    Blt_NameOfState, /* 79 */
    Blt_GetFillFromObj, /* 80 */
    Blt_NameOfFill, /* 81 */
    Blt_GetResizeFromObj, /* 82 */
    Blt_NameOfResize, /* 83 */
    Blt_GetSideFromObj, /* 84 */
    Blt_NameOfSide, /* 85 */
    Blt_GetCount, /* 86 */
    Blt_GetCountFromObj, /* 87 */
    Blt_ParseSwitches, /* 88 */
    Blt_FreeSwitches, /* 89 */
    Blt_SwitchChanged, /* 90 */
    Blt_SwitchInfo, /* 91 */
    Blt_SwitchValue, /* 92 */
    Blt_Malloc, /* 93 */
    Blt_Realloc, /* 94 */
    Blt_Free, /* 95 */
    Blt_Calloc, /* 96 */
    Blt_Strdup, /* 97 */
    Blt_Strndup, /* 98 */
    Blt_MallocAbortOnError, /* 99 */
    Blt_CallocAbortOnError, /* 100 */
    Blt_ReallocAbortOnError, /* 101 */
    Blt_StrdupAbortOnError, /* 102 */
    Blt_StrndupAbortOnError, /* 103 */
    Blt_DictionaryCompare, /* 104 */
    Blt_GetUid, /* 105 */
    Blt_FreeUid, /* 106 */
    Blt_FindUid, /* 107 */
    Blt_CreatePipeline, /* 108 */
    Blt_DetachPids, /* 109 */
    Blt_InitHexTable, /* 110 */
    Blt_DStringAppendElements, /* 111 */
    Blt_LoadLibrary, /* 112 */
    Blt_Panic, /* 113 */
    Blt_Warn, /* 114 */
    Blt_OpenFile, /* 115 */
    Blt_Itoa, /* 116 */
    Blt_Ltoa, /* 117 */
    Blt_Utoa, /* 118 */
    Blt_Dtoa, /* 119 */
    Blt_DecodeHexadecimal, /* 120 */
    Blt_DecodeBase64, /* 121 */
    Blt_DecodeBase85, /* 122 */
    Blt_DecodeAscii85, /* 123 */
    Blt_DecodeBase64ToBuffer, /* 124 */
    Blt_DecodeHexadecimalToObj, /* 125 */
    Blt_DecodeBase64ToObj, /* 126 */
    Blt_DecodeBase85ToObj, /* 127 */
    Blt_EncodeHexadecimal, /* 128 */
    Blt_EncodeBase64, /* 129 */
    Blt_EncodeBase85, /* 130 */
    Blt_EncodeAscii85, /* 131 */
    Blt_EncodeHexadecimalToObj, /* 132 */
    Blt_EncodeBase64ToObj, /* 133 */
    Blt_EncodeBase85ToObj, /* 134 */
    Blt_HexadecimalDecodeBufferSize, /* 135 */
    Blt_HexadecimalEncodeBufferSize, /* 136 */
    Blt_Base64DecodeBufferSize, /* 137 */
    Blt_Base64EncodeBufferSize, /* 138 */
    Blt_Base85DecodeBufferSize, /* 139 */
    Blt_Base85EncodeBufferSize, /* 140 */
    Blt_Ascii85DecodeBufferSize, /* 141 */
    Blt_Ascii85EncodeBufferSize, /* 142 */
    Blt_IsBase64, /* 143 */
    Blt_GetTimeFromObj, /* 144 */
    Blt_GetTime, /* 145 */
    Blt_SecondsToDate, /* 146 */
    Blt_DateToSeconds, /* 147 */
    Blt_FormatDate, /* 148 */
    Blt_GetPositionFromObj, /* 149 */
    Blt_ObjIsInteger, /* 150 */
    Blt_GetLong, /* 151 */
    Blt_GetLongFromObj, /* 152 */
    Blt_SetLongObj, /* 153 */
    Blt_NewLongObj, /* 154 */
    Blt_IsLongObj, /* 155 */
    Blt_GetUnsignedLong, /* 156 */
    Blt_GetUnsignedLongFromObj, /* 157 */
    Blt_SetUnsignedLongObj, /* 158 */
    Blt_NewUnsignedLongObj, /* 159 */
    Blt_IsUnsignedLongObj, /* 160 */
    Blt_GetInt64, /* 161 */
    Blt_GetInt64FromObj, /* 162 */
    Blt_SetInt64Obj, /* 163 */
    Blt_NewInt64Obj, /* 164 */
    Blt_IsInt64Obj, /* 165 */
    Blt_GetDouble, /* 166 */
    Blt_GetDoubleFromObj, /* 167 */
    Blt_SetDoubleObj, /* 168 */
    Blt_NewDoubleObj, /* 169 */
    Blt_IsDoubleObj, /* 170 */
    Blt_FmtString, /* 171 */
    Blt_LowerCase, /* 172 */
    Blt_UpperCase, /* 173 */
    Blt_GetPlatformId, /* 174 */
    Blt_LastError, /* 175 */
    Blt_NaN, /* 176 */
    Blt_AlmostEquals, /* 177 */
    Blt_ConvertListToList, /* 178 */
    Blt_RegisterObjTypes, /* 179 */
    Blt_GetCachedVar, /* 180 */
    Blt_FreeCachedVars, /* 181 */
};

/* !END!: Do not edit above this line. */

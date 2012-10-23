library blt
interface bltTclInt
declare 1 generic {
   void *Blt_Malloc(size_t size)
}
declare 2 generic {
   void *Blt_Realloc(void *ptr, size_t size)
}
declare 3 generic {
   void Blt_Free(const void *ptr)
}
declare 4 generic {
   void *Blt_Calloc(size_t numElem, size_t size)
}
declare 5 generic {
   char *Blt_Strdup(const char *string)
}
declare 6 generic {
   void *Blt_MallocAbortOnError(size_t size, const char *file,int line)
}
declare 7 generic {
   void *Blt_CallocAbortOnError(size_t numElem, size_t size, 
	const char *file, int line)
}
declare 8 generic {
   char *Blt_StrdupAbortOnError(const char *ptr, const char *file, 
	int line)
}
declare 9 generic {
   int Blt_DictionaryCompare (const char *s1, const char *s2)
}
declare 10 generic {
   Blt_Uid Blt_GetUid(const char *string)
}
declare 11 generic {
   void Blt_FreeUid(Blt_Uid uid)
}
declare 12 generic {
   Blt_Uid Blt_FindUid(const char *string)
}
declare 13 generic {
   int Blt_CreatePipeline(Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv, Blt_Pid **pidArrayPtr, int *stdinPipePtr,
	int *stdoutPipePtr, int *stderrPipePtr)
}
declare 14 generic {
   void Blt_InitHexTable (unsigned char *table)
}
declare 15 generic {
   void Blt_DStringAppendElements(Tcl_DString *dsPtr, ...)
}
declare 16 generic {
   int Blt_LoadLibrary(Tcl_Interp *interp, const char *libPath, 
	const char *initProcName, const char *safeProcName)
}
declare 17 generic {
   void Blt_Panic(const char *fmt, ...)
}
declare 18 generic {
   void Blt_Warn(const char *fmt, ...)
}
declare 19 generic {
   int Blt_GetSideFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int *sidePtr)
}
declare 20 generic {
   const char *Blt_NameOfSide(int side)
}
declare 21 generic {
   FILE *Blt_OpenFile(Tcl_Interp *interp, const char *fileName, 
	const char *mode)
}
declare 22 generic {
   int Blt_ExprDoubleFromObj (Tcl_Interp *interp, Tcl_Obj *objPtr, 
	double *valuePtr)
}
declare 23 generic {
   int Blt_ExprIntFromObj (Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int *valuePtr)
}
declare 24 generic {
   const char *Blt_Itoa(int value)
}
declare 25 generic {
   const char *Blt_Ltoa(long value)
}
declare 26 generic {
   const char *Blt_Utoa(unsigned int value)
}
declare 27 generic {
   const char *Blt_Dtoa(Tcl_Interp *interp, double value)
}
declare 28 generic {
   unsigned char *Blt_Base64_Decode(Tcl_Interp *interp, 
	const char *string, size_t *lengthPtr)
}
declare 29 generic {
   Blt_DBuffer Blt_Base64_DecodeToBuffer(Tcl_Interp *interp, 
	const char *string, size_t length)
}
declare 30 generic {
   Tcl_Obj *Blt_Base64_DecodeToObj(Tcl_Interp *interp, 
	const char *string, size_t length)
}
declare 31 generic {
   const char *Blt_Base64_Encode(Tcl_Interp *interp, 
	const unsigned char *buffer, size_t bufsize)
}
declare 32 generic {
   Tcl_Obj *Blt_Base64_EncodeToObj(Tcl_Interp *interp, 
	const unsigned char *buffer, size_t bufsize)
}
declare 33 generic {
   const char *Blt_Base85_Encode(Tcl_Interp *interp, 
	const unsigned char *buffer, size_t bufsize)
}
declare 34 generic {
   const char *Blt_Base16_Encode(Tcl_Interp *interp, 
	const unsigned char *buffer, size_t bufsize)
}
declare 35 generic {
   int Blt_IsBase64(const char *buf, size_t length)
}
declare 36 generic {
   int Blt_GetDoubleFromString(Tcl_Interp *interp, const char *s, 
	double *valuePtr)
}
declare 37 generic {
   int Blt_GetDoubleFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	double *valuePtr)
}
declare 38 generic {
   int Blt_GetTimeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	double *valuePtr)
}
declare 39 generic {
   int Blt_GetTime(Tcl_Interp *interp, const char *string, 
	double *valuePtr)
}
declare 40 generic {
   int Blt_GetDateFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	double *timePtr)
}
declare 41 generic {
   int Blt_GetDate(Tcl_Interp *interp, const char *string, 
	double *timePtr)
}
declare 42 generic {
   int Blt_GetPositionFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	long *indexPtr)
}
declare 43 generic {
   int Blt_GetCountFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int check, long *valuePtr)
}
declare 44 generic {
   int Blt_SimplifyLine (Point2d *origPts, int low, int high, 
	double tolerance, int *indices)
}
declare 45 generic {
   int Blt_GetLong(Tcl_Interp *interp, const char *s, long *longPtr)
}
declare 46 generic {
   int Blt_GetLongFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	long *longPtr)
}
declare 47 generic {
   int Blt_FormatString(char *s, size_t size, const char *fmt, ...)
}
declare 48 generic {
   void Blt_LowerCase(char *s)
}
declare 49 generic {
   int Blt_GetPlatformId(void)
}
declare 50 generic {
   const char *Blt_LastError(void)
}
declare 51 generic {
   double Blt_NaN(void)
}
declare 52 generic {
   int Blt_AlmostEquals(double x, double y)
}
declare 53 generic {
   int Blt_GetArrayFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	Blt_HashTable **tablePtrPtr)
}
declare 54 generic {
   Tcl_Obj *Blt_NewArrayObj(int objc, Tcl_Obj *objv[])
}
declare 55 generic {
   void Blt_RegisterArrayObj(void)
}
declare 56 generic {
   int Blt_IsArrayObj(Tcl_Obj *obj)
}
declare 57 generic {
   void Blt_Assert(const char *expr, const char *file, int line)
}
declare 58 generic {
   void Blt_DBuffer_VarAppend(Blt_DBuffer buffer, ...)
}
declare 59 generic {
   void Blt_DBuffer_Format(Blt_DBuffer buffer, const char *fmt, ...)
}
declare 60 generic {
   void Blt_DBuffer_Init(Blt_DBuffer buffer)
}
declare 61 generic {
   void Blt_DBuffer_Free(Blt_DBuffer buffer)
}
declare 62 generic {
   unsigned char *Blt_DBuffer_Extend(Blt_DBuffer buffer, size_t extra)
}
declare 63 generic {
   int Blt_DBuffer_AppendData(Blt_DBuffer buffer, 
	const unsigned char *bytes, size_t extra)
}
declare 64 generic {
   int Blt_DBuffer_Concat(Blt_DBuffer dest, Blt_DBuffer src)
}
declare 65 generic {
   int Blt_DBuffer_Resize(Blt_DBuffer buffer, size_t length)
}
declare 66 generic {
   int Blt_DBuffer_SetLength(Blt_DBuffer buffer, size_t length)
}
declare 67 generic {
   Blt_DBuffer Blt_DBuffer_Create(void)
}
declare 68 generic {
   void Blt_DBuffer_Destroy(Blt_DBuffer buffer)
}
declare 69 generic {
   int Blt_DBuffer_LoadFile(Tcl_Interp *interp, const char *fileName, 
	Blt_DBuffer buffer)
}
declare 70 generic {
   int Blt_DBuffer_SaveFile(Tcl_Interp *interp, const char *fileName, 
	Blt_DBuffer buffer)
}
declare 71 generic {
   void Blt_DBuffer_AppendByte(Blt_DBuffer buffer, unsigned char byte)
}
declare 72 generic {
   void Blt_DBuffer_AppendShort(Blt_DBuffer buffer, 
	unsigned short value)
}
declare 73 generic {
   void Blt_DBuffer_AppendLong(Blt_DBuffer buffer, unsigned int value)
}
declare 74 generic {
   Tcl_Obj *Blt_DBuffer_ByteArrayObj(Blt_DBuffer buffer)
}
declare 75 generic {
   Tcl_Obj *Blt_DBuffer_StringObj(Blt_DBuffer buffer)
}
declare 76 generic {
   const char *Blt_DBuffer_String(Blt_DBuffer buffer)
}
declare 77 generic {
   int Blt_DBuffer_Base64Decode(Tcl_Interp *interp, 
	const char *string, size_t length, Blt_DBuffer buffer)
}
declare 78 generic {
   const char *Blt_DBuffer_Base64Encode(Tcl_Interp *interp, 
	Blt_DBuffer buffer)
}
declare 79 generic {
   Tcl_Obj *Blt_DBuffer_Base64EncodeToObj(Tcl_Interp *interp, 
	Blt_DBuffer buffer)
}
declare 80 generic {
   int Blt_InitCmd (Tcl_Interp *interp, const char *namespace, 
	Blt_CmdSpec *specPtr)
}
declare 81 generic {
   int Blt_InitCmds (Tcl_Interp *interp, const char *namespace, 
	Blt_CmdSpec *specPtr, int numCmds)
}
declare 82 generic {
   void *Blt_GetOpFromObj(Tcl_Interp *interp, int numSpecs, 
	Blt_OpSpec *specs, int operPos, int objc, Tcl_Obj *const *objv, 
	int flags)
}
declare 83 generic {
   Tcl_Namespace *Blt_GetVariableNamespace(Tcl_Interp *interp, 
	const char *varName)
}
declare 84 generic {
   Tcl_Namespace *Blt_GetCommandNamespace(Tcl_Command cmdToken)
}
declare 85 generic {
   Tcl_CallFrame *Blt_EnterNamespace(Tcl_Interp *interp, 
	Tcl_Namespace *nsPtr)
}
declare 86 generic {
   void Blt_LeaveNamespace(Tcl_Interp *interp, Tcl_CallFrame *framePtr)
}
declare 87 generic {
   int Blt_ParseObjectName(Tcl_Interp *interp, const char *name, 
	Blt_ObjectName *objNamePtr, unsigned int flags)
}
declare 88 generic {
   const char *Blt_MakeQualifiedName(Blt_ObjectName *objNamePtr, 
	Tcl_DString *resultPtr)
}
declare 89 generic {
   int Blt_CommandExists(Tcl_Interp *interp, const char *string)
}
declare 90 generic {
   Blt_Spline Blt_CreateSpline(Point2d *points, int n, int type)
}
declare 91 generic {
   Point2d Blt_EvaluateSpline(Blt_Spline spline, int index, double x)
}
declare 92 generic {
   void Blt_FreeSpline(Blt_Spline spline)
}
declare 93 generic {
   Blt_Spline Blt_CreateParametricCubicSpline(Point2d *points, int n, 
	int w, int h)
}
declare 94 generic {
   Point2d Blt_EvaluateParametricCubicSpline(Blt_Spline spline, 
	int index, double x)
}
declare 95 generic {
   void Blt_FreeParametricCubicSpline(Blt_Spline spline)
}
declare 96 generic {
   Blt_Spline Blt_CreateCatromSpline(Point2d *points, int n)
}
declare 97 generic {
   Point2d Blt_EvaluateCatromSpline(Blt_Spline spline, int i, double t)
}
declare 98 generic {
   void Blt_FreeCatromSpline(Blt_Spline spline)
}
declare 99 generic {
   int Blt_ComputeNaturalSpline (Point2d *origPts, int numOrigPts, 
	Point2d *intpPts, int numIntpPts)
}
declare 100 generic {
   int Blt_ComputeQuadraticSpline(Point2d *origPts, int numOrigPts, 
	Point2d *intpPts, int numIntpPts)
}
declare 101 generic {
   int Blt_ComputeNaturalParametricSpline (Point2d *origPts, 
	int numOrigPts, Region2d *extsPtr, int isClosed, Point2d *intpPts, 
	int numIntpPts)
}
declare 102 generic {
   int Blt_ComputeCatromParametricSpline (Point2d *origPts, 
	int numOrigPts, Point2d *intpPts, int numIntpPts)
}
declare 103 generic {
   int Blt_ParseSwitches(Tcl_Interp *interp, Blt_SwitchSpec *specPtr, 
	int objc, Tcl_Obj *const *objv, void *rec, int flags)
}
declare 104 generic {
   void Blt_FreeSwitches(Blt_SwitchSpec *specs, void *rec, int flags)
}
declare 105 generic {
   int Blt_SwitchChanged(Blt_SwitchSpec *specs, ...)
}
declare 106 generic {
   int Blt_SwitchInfo(Tcl_Interp *interp, Blt_SwitchSpec *specs,
	void *record, Tcl_Obj *objPtr, int flags)
}
declare 107 generic {
   int Blt_SwitchValue(Tcl_Interp *interp, Blt_SwitchSpec *specs, 
	void *record, Tcl_Obj *objPtr, int flags)
}
declare 108 generic {
   Tcl_Var Blt_GetCachedVar(Blt_HashTable *tablePtr, const char *label,
	Tcl_Obj *objPtr)
}
declare 109 generic {
   void Blt_FreeCachedVars(Blt_HashTable *tablePtr)
}

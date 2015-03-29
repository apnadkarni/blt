/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTclInt.h --
 *
 *	Copyright 1993-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _BLT_TCL_INT_H
#define _BLT_TCL_INT_H

BLT_EXTERN void *Blt_Malloc(size_t size);
BLT_EXTERN void *Blt_Realloc(void *ptr, size_t size);
BLT_EXTERN void Blt_Free(const void *ptr);
BLT_EXTERN void *Blt_Calloc(size_t numElem, size_t size);
BLT_EXTERN const char *Blt_Strdup(const char *string);
BLT_EXTERN const char *Blt_Strndup(const char *string, size_t size);

BLT_EXTERN void *Blt_MallocAbortOnError(size_t size, const char *file,int line);
BLT_EXTERN void *Blt_CallocAbortOnError(size_t numElem, size_t size, 
	const char *file, int line);
BLT_EXTERN void *Blt_ReallocAbortOnError(void *ptr, size_t size, 
	const char *file, int line);
BLT_EXTERN const char *Blt_StrdupAbortOnError(const char *ptr, const char *file,
	int line);
BLT_EXTERN const char *Blt_StrndupAbortOnError(const char *ptr, size_t size,
        const char *file, int line);

#define Blt_AssertCalloc(n,s) (Blt_CallocAbortOnError(n,s,__FILE__, __LINE__))
#define Blt_AssertMalloc(s) (Blt_MallocAbortOnError(s,__FILE__, __LINE__))
#define Blt_AssertRealloc(p,s) (Blt_ReallocAbortOnError(p,s,__FILE__, __LINE__))
#define Blt_AssertStrdup(s) (Blt_StrdupAbortOnError(s,__FILE__, __LINE__))
#define Blt_AssertStrndup(p,s) (Blt_StrdupAbortOnError(p,s,__FILE__, __LINE__))

BLT_EXTERN int Blt_DictionaryCompare (const char *s1, const char *s2);

#define COUNT_NNEG		0
#define COUNT_POS		1
#define COUNT_ANY		2

typedef const char *Blt_Uid;

BLT_EXTERN Blt_Uid Blt_GetUid(const char *string);
BLT_EXTERN void Blt_FreeUid(Blt_Uid uid);
BLT_EXTERN Blt_Uid Blt_FindUid(const char *string);

typedef char *DestroyData;
typedef int (QSortCompareProc) (const void *, const void *);

#ifdef WIN32
typedef struct {
    DWORD pid;
    HANDLE hProcess;
} Blt_Pid;
#else
#include <sys/types.h>
typedef struct _Blt_Pid {
    pid_t pid;
} Blt_Pid;
#endif

#include <time.h>
#include <sys/time.h>

typedef struct {
    int year;                           /* Year 0-9999. */
    int mon;				/* Month 0-11. */
    int week;                           /* Ordinal week. 1-53. */
    int yday;                           /* Day of the year. 0-365. Jan 1st
					 * is 0. */
    int mday;                           /* Day of the month. 1-31. */
    int wday;                           /* Day of week. 0-6. Sunday is
					 * zero. */
    int wyear;                          /* Year of ordinal week. 0-9999. */
    int hour;                           /* Hour 0-23. */
    int min;				/* Minute 0-59 */
    int sec;				/* Second. 0-60. */
    double frac;                        /* Fractional seconds. */
    int tzoffset;			/* Timezone offset. */
    int isdst;
    int isLeapYear;
} Blt_DateTime;

BLT_EXTERN int Blt_CreatePipeline(Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv, Blt_Pid **pidArrayPtr, int *stdinPipePtr,
	int *stdoutPipePtr, int *stderrPipePtr);

BLT_EXTERN void Blt_InitHexTable (unsigned char *table);

BLT_EXTERN void Blt_DStringAppendElements(Tcl_DString *dsPtr, ...);

BLT_EXTERN int Blt_LoadLibrary(Tcl_Interp *interp, const char *libPath, 
	const char *initProcName, const char *safeProcName);

extern int  Blt_CpuFeatures(Tcl_Interp *interp, unsigned long *featuresPtr);

#if (_TCL_VERSION < _VERSION(8,1,0))
BLT_EXTERN const char *Tcl_GetString (Tcl_Obj *objPtr);

BLT_EXTERN int Tcl_EvalObjv (Tcl_Interp *interp, int objc, Tcl_Obj **objv, 
	int flags);

BLT_EXTERN int Tcl_WriteObj (Tcl_Channel channel, Tcl_Obj *objPtr);

BLT_EXTERN char *Tcl_SetVar2Ex (Tcl_Interp *interp, const char *part1, 
	const char *part2, Tcl_Obj *objPtr, int flags);

BLT_EXTERN Tcl_Obj *Tcl_GetVar2Ex (Tcl_Interp *interp, const char *part1, 
	const char *part2, int flags);
#endif /* _TCL_VERSION < 8.1.0 */

#ifndef WIN32
#  define PurifyPrintf  printf
#endif /* WIN32 */

#undef panic
#define panic(mesg)	Blt_Panic("%s:%d %s", __FILE__, __LINE__, (mesg))

BLT_EXTERN void Blt_Panic(const char *fmt, ...);
BLT_EXTERN void Blt_Warn(const char *fmt, ...);

#ifndef USE_TCL_STUBS
extern int TclpHasSockets(Tcl_Interp *interp);
#endif

#define SIDE_LEFT		(1<<0)
#define SIDE_TOP		(1<<1)
#define SIDE_RIGHT		(1<<2)
#define SIDE_BOTTOM		(1<<3)

BLT_EXTERN int Blt_GetSideFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int *sidePtr);

BLT_EXTERN const char *Blt_NameOfSide(int side);

BLT_EXTERN FILE *Blt_OpenFile(Tcl_Interp *interp, const char *fileName, 
	const char *mode);

BLT_EXTERN int Blt_ExprDoubleFromObj (Tcl_Interp *interp, Tcl_Obj *objPtr, 
	double *valuePtr);

BLT_EXTERN int Blt_ExprIntFromObj (Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int *valuePtr);

BLT_EXTERN const char *Blt_Itoa(int value);

BLT_EXTERN const char *Blt_Ltoa(long value);

BLT_EXTERN const char *Blt_Utoa(unsigned int value);

BLT_EXTERN const char *Blt_Dtoa(Tcl_Interp *interp, double value);

BLT_EXTERN unsigned char *Blt_Base64_Decode(Tcl_Interp *interp, 
	const char *string, size_t *lengthPtr);

BLT_EXTERN Blt_DBuffer Blt_Base64_DecodeToBuffer(Tcl_Interp *interp, 
	const char *string, size_t length);

BLT_EXTERN Tcl_Obj *Blt_Base64_DecodeToObj(Tcl_Interp *interp, 
	const char *string, size_t length);

BLT_EXTERN Tcl_Obj *Blt_Base64_EncodeToObj(const unsigned char *buffer, 
	size_t bufsize);

BLT_EXTERN size_t Blt_Base64_MaxBufferLength(size_t bufsize);

BLT_EXTERN size_t Blt_Base64_Encode(const unsigned char *buffer, 
	size_t bufsize, unsigned char *destBytes);

BLT_EXTERN size_t Blt_Base85_MaxBufferLength(size_t bufsize);

BLT_EXTERN size_t Blt_Base85_Encode(const unsigned char *buffer, size_t bufsize,
	unsigned char *destBytes);

BLT_EXTERN const char *Blt_Base16_Encode(const unsigned char *buffer, 
	size_t bufsize);

BLT_EXTERN int Blt_IsBase64(const char *buf, size_t length);

BLT_EXTERN int Blt_GetDoubleFromString(Tcl_Interp *interp, const char *s, 
	double *valuePtr);
BLT_EXTERN int Blt_GetDoubleFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	double *valuePtr);

BLT_EXTERN int Blt_GetTimeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	double *secondsPtr);
BLT_EXTERN int Blt_GetTime(Tcl_Interp *interp, const char *string, 
	double *secondsPtr);

BLT_EXTERN void Blt_SecondsToDate(double seconds, Blt_DateTime *datePtr);
BLT_EXTERN void Blt_DateToSeconds(Blt_DateTime *datePtr, double *secondsPtr);
BLT_EXTERN void Blt_FormatDate(Blt_DateTime *datePtr, const char *format, 
	Tcl_DString *resultPtr);

BLT_EXTERN int Blt_GetPositionFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	long *indexPtr);

BLT_EXTERN int Blt_GetCountFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int check, long *valuePtr);

BLT_EXTERN int Blt_SimplifyLine (Point2d *origPts, int low, int high, 
	double tolerance, int *indices);

BLT_EXTERN int Blt_GetLong(Tcl_Interp *interp, const char *s, long *longPtr);
BLT_EXTERN int Blt_GetLongFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	long *longPtr);

BLT_EXTERN int Blt_FormatString(char *s, size_t size, const char *fmt, ...);
BLT_EXTERN void Blt_LowerCase(char *s);
BLT_EXTERN void Blt_UpperCase(char *s);

BLT_EXTERN int Blt_GetPlatformId(void);
BLT_EXTERN const char *Blt_LastError(void);

BLT_EXTERN double Blt_NaN(void);
BLT_EXTERN int Blt_AlmostEquals(double x, double y);

#endif /*_BLT_TCL_INT_H*/

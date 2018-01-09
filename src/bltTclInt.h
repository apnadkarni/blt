/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTclInt.h --
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

#define COUNT_NNEG              0
#define COUNT_POS               1
#define COUNT_ANY               2

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
    int mon;                            /* Month 0-11. */
    int week;                           /* Ordinal week. 1-53. */
    int yday;                           /* Day of the year. 0-365. Jan 1st
                                         * is 0. */
    int mday;                           /* Day of the month. 1-31. */
    int wday;                           /* Day of week. 0-6. Sunday is
                                         * zero. */
    int wyear;                          /* Year of ordinal week. 0-9999. */
    int hour;                           /* Hour 0-23. */
    int min;                            /* Minute 0-59 */
    int sec;                            /* Second. 0-60. */
    int tzoffset;                       /* Timezone offset. */
    int isdst;
    int isLeapYear;
    double frac;                        /* Fractional seconds. */
} Blt_DateTime;

BLT_EXTERN int Blt_CreatePipeline(Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv, Blt_Pid **pidArrayPtr, void *inPipePtr,
        void *outPipePtr, void *errPipePtr, char *const *env);

BLT_EXTERN void Blt_DetachPids(int numPids, Blt_Pid *pids);

BLT_EXTERN void Blt_InitHexTable (unsigned char *table);

BLT_EXTERN void Blt_DStringAppendElements(Tcl_DString *dsPtr, ...);

BLT_EXTERN int Blt_LoadLibrary(Tcl_Interp *interp, const char *libPath, 
        const char *initProcName, const char *safeProcName);

extern unsigned long Blt_CpuFeatureFlags(Tcl_Interp *interp);

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

#undef panic
#define panic(mesg)     Blt_Panic("%s:%d %s", __FILE__, __LINE__, (mesg))

BLT_EXTERN void Blt_Panic(const char *fmt, ...);
BLT_EXTERN void Blt_Warn(const char *fmt, ...);

#ifndef USE_TCL_STUBS
extern int TclpHasSockets(Tcl_Interp *interp);
#endif

#define SIDE_LEFT               (1<<0)
#define SIDE_TOP                (1<<1)
#define SIDE_RIGHT              (1<<2)
#define SIDE_BOTTOM             (1<<3)

BLT_EXTERN FILE *Blt_OpenFile(Tcl_Interp *interp, const char *fileName, 
        const char *mode);

BLT_EXTERN const char *Blt_Itoa(int value);

BLT_EXTERN const char *Blt_Ltoa(long value);

BLT_EXTERN const char *Blt_Utoa(unsigned int value);

BLT_EXTERN const char *Blt_Dtoa(Tcl_Interp *interp, double value);

BLT_EXTERN int Blt_DecodeHexadecimal(Tcl_Interp *interp, const char *src,
        size_t numChars, unsigned char *dest, size_t *numBytesPtr,
        BinaryDecoder *switchesPtr);
BLT_EXTERN int Blt_DecodeBase64(Tcl_Interp *interp, const char *src,
        size_t numChars, unsigned char *dest, size_t *numBytesPtr,
        BinaryDecoder *switchesPtr);
BLT_EXTERN int Blt_DecodeBase85(Tcl_Interp *interp, const char *src,
        size_t numChars, unsigned char *dest, size_t *numBytesPtr,
        BinaryDecoder *switchesPtr);
BLT_EXTERN int Blt_DecodeAscii85(Tcl_Interp *interp, const char *src,
        size_t numChars, unsigned char *dest, size_t *numBytesPtr,
        BinaryDecoder *switchesPtr);

BLT_EXTERN Blt_DBuffer Blt_DecodeBase64ToBuffer(Tcl_Interp *interp, 
        const char *src, size_t numChars);

BLT_EXTERN Tcl_Obj *Blt_DecodeHexadecimalToObj(Tcl_Interp *interp, 
        const char *src, size_t numChars);
BLT_EXTERN Tcl_Obj *Blt_DecodeBase64ToObj(Tcl_Interp *interp, 
        const char *src, size_t numChars);
BLT_EXTERN Tcl_Obj *Blt_DecodeBase85ToObj(Tcl_Interp *interp, 
        const char *src, size_t numChars);

BLT_EXTERN int Blt_EncodeHexadecimal(const unsigned char *src, size_t numBytes,
        char *dest, size_t *numCharsPtr, BinaryEncoder *switchesPtr);
BLT_EXTERN int Blt_EncodeBase64(const unsigned char *src, size_t numBytes,
        char *dest, size_t *numCharsPtr, BinaryEncoder *switchesPtr);
BLT_EXTERN int Blt_EncodeBase85(const unsigned char *src, size_t numBytes,
        char *dest, size_t *numCharsPtr, BinaryEncoder *switchesPtr);
BLT_EXTERN int Blt_EncodeAscii85(const unsigned char *src, size_t numBytes,
        char *dest, size_t *numCharsPtr, BinaryEncoder *switchesPtr);

BLT_EXTERN Tcl_Obj *Blt_EncodeHexadecimalToObj(const unsigned char *src, 
        size_t numBytes);
BLT_EXTERN Tcl_Obj *Blt_EncodeBase64ToObj(const unsigned char *src, 
        size_t numBytes);
BLT_EXTERN Tcl_Obj *Blt_EncodeBase85ToObj(const unsigned char *src, 
        size_t numBytes);

BLT_EXTERN size_t Blt_HexadecimalDecodeBufferSize(size_t numBytes,
        BinaryDecoder *switchesPtr);
BLT_EXTERN size_t Blt_HexadecimalEncodeBufferSize(size_t numChars,
        BinaryEncoder *switchesPtr);
BLT_EXTERN size_t Blt_Base64DecodeBufferSize(size_t numBytes,
        BinaryDecoder *switchesPtr);
BLT_EXTERN size_t Blt_Base64EncodeBufferSize(size_t numChars,
        BinaryEncoder *switchesPtr);
BLT_EXTERN size_t Blt_Base85DecodeBufferSize(size_t numBytes,
        BinaryDecoder *switchesPtr);
BLT_EXTERN size_t Blt_Base85EncodeBufferSize(size_t numChars,
        BinaryEncoder *switchesPtr);
BLT_EXTERN size_t Blt_Ascii85DecodeBufferSize(size_t numBytes,
        BinaryDecoder *switchesPtr);
BLT_EXTERN size_t Blt_Ascii85EncodeBufferSize(size_t numChars,
        BinaryEncoder *switchesPtr);

BLT_EXTERN int Blt_IsBase64(const char *buf, size_t length);

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

BLT_EXTERN int Blt_ObjIsInteger(Tcl_Obj *objPtr);

BLT_EXTERN int Blt_GetLong(Tcl_Interp *interp, const char *s,
                           long *valuePtr);
BLT_EXTERN int Blt_GetLongFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
                                  long *valuePtr);
BLT_EXTERN int Blt_SetLongObj(Tcl_Obj *objPtr, long value);
BLT_EXTERN Tcl_Obj *Blt_NewLongObj(long value);
BLT_EXTERN int Blt_IsLongObj(Tcl_Obj *objPtr);

BLT_EXTERN int Blt_GetUnsignedLong(Tcl_Interp *interp, const char *s,
                           unsigned long *valuePtr);
BLT_EXTERN int Blt_GetUnsignedLongFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
                                  unsigned long *valuePtr);
BLT_EXTERN int Blt_SetUnsignedLongObj(Tcl_Obj *objPtr, unsigned long value);
BLT_EXTERN Tcl_Obj *Blt_NewUnsignedLongObj(unsigned long value);
BLT_EXTERN int Blt_IsUnsignedLongObj(Tcl_Obj *objPtr);

BLT_EXTERN int Blt_GetInt64(Tcl_Interp *interp, const char *s,
                           int64_t *valuePtr);
BLT_EXTERN int Blt_GetInt64FromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
                                  int64_t *valuePtr);
BLT_EXTERN int Blt_SetInt64Obj(Tcl_Obj *objPtr, int64_t value);
BLT_EXTERN Tcl_Obj *Blt_NewInt64Obj(int64_t value);
BLT_EXTERN int Blt_IsInt64Obj(Tcl_Obj *objPtr);

BLT_EXTERN int Blt_GetDouble(Tcl_Interp *interp, const char *s, 
        double *valuePtr);
BLT_EXTERN int Blt_GetDoubleFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        double *valuePtr);
BLT_EXTERN int Blt_SetDoubleObj(Tcl_Obj *objPtr, double value);
BLT_EXTERN Tcl_Obj *Blt_NewDoubleObj(double value);
BLT_EXTERN int Blt_IsDoubleObj(Tcl_Obj *objPtr);

BLT_EXTERN int Blt_FmtString(char *s, size_t size, const char *fmt, ...);
BLT_EXTERN void Blt_LowerCase(char *s);
BLT_EXTERN void Blt_UpperCase(char *s);

BLT_EXTERN int Blt_GetPlatformId(void);
BLT_EXTERN const char *Blt_LastError(void);

BLT_EXTERN double Blt_NaN(void);
BLT_EXTERN int Blt_AlmostEquals(double x, double y);

BLT_EXTERN const char **Blt_ConvertListToList(int argc, const char **argv);

BLT_EXTERN void Blt_RegisterObjTypes(void);

#endif /*_BLT_TCL_INT_H*/

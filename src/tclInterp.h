
/*
 * tclInterp.h --
 *
 * Excerpts from tclInt.h.  Used to examine interpreter internals.
 * Needed by the former parsing (bltParse.c) functions.
 *
 *	Copyright 2003-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file contains excerpts from tclInt.h of the TCL library distribution.
 *
 *	Copyright (c) 1987-1993 The Regents of the University of
 *	California.
 *
 *	Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *
 */

/*
 *---------------------------------------------------------------------------
 * Data structures related to command parsing. These are used in
 * tclParse.c and its clients.
 *---------------------------------------------------------------------------
 */

/*
 * The following data structure is used by various parsing procedures to hold
 * information about where to store the results of parsing (e.g. the
 * substituted contents of a quoted argument, or the result of a nested
 * command).  At any given time, the space available for output is fixed, but
 * a procedure may be called to expand the space available if the current
 * space runs out.
 */
typedef struct _ParseValue ParseValue;

struct _ParseValue {
    char *buffer;
    char *next;
    char *end;
    void (*expandProc)(ParseValue *pvPtr, int needed);
    ClientData clientData;
};


/*
 * The definitions for the LiteralTable and LiteralEntry structures. Each
 * interpreter contains a LiteralTable. It is used to reduce the storage
 * needed for all the TCL objects that hold the literals of scripts compiled
 * by the interpreter. A literal's object is shared by all the ByteCodes that
 * refer to the literal. Each distinct literal has one LiteralEntry entry in
 * the LiteralTable. A literal table is a specialized hash table that is
 * indexed by the literal's string representation, which may contain null
 * characters.
 *
 * Note that we reduce the space needed for literals by sharing literal
 * objects both within a ByteCode (each ByteCode contains a local
 * LiteralTable) and across all an interpreter's ByteCodes (with the
 * interpreter's global LiteralTable).
 */

typedef struct _LiteralEntry LiteralEntry;

struct _LiteralEntry {
    LiteralEntry *nextPtr;
    Tcl_Obj *objPtr;
    int refCount;
};

typedef struct {
    LiteralEntry **buckets;
    LiteralEntry *staticBuckets[TCL_SMALL_HASH_TABLE];
    int numBuckets;
    int numEntries;
    int rebuildSize;
    int mask;
} LiteralTable;

/*
 * The following structure defines for each TCL interpreter various
 * statistics-related information about the bytecode compiler and
 * interpreter's operation in that interpreter.
 */

#ifdef TCL_COMPILE_STATS
typedef struct {
    long numExecutions;
    long numCompilations;
    long numByteCodesFreed;
    long instructionCount[256];
    double totalSrcBytes;
    double totalByteCodeBytes;
    double currentSrcBytes;
    double currentByteCodeBytes;
    long srcCount[32];
    long byteCodeCount[32];
    long lifetimeCount[32];
    double currentInstBytes;
    double currentLitBytes;
    double currentExceptBytes;
    double currentAuxBytes;
    double currentCmdMapBytes;
    long numLiteralsCreated;
    double totalLitStringBytes;
    double currentLitStringBytes;
    long literalCount[32];
} ByteCodeStats;

#endif /* TCL_COMPILE_STATS */


/*
 *---------------------------------------------------------------------------
 *
 *   Data structures and procedures related to TclHandles, which are a very
 *   lightweight method of preserving enough information to determine if an
 *   arbitrary malloc'd block has been deleted.
 *   
 *---------------------------------------------------------------------------
 */

typedef VOID **TclHandle;

/*
 *   The following fills in dummy types for structure refered to internally by
 *   the TCL interpreter.  Since we don't need the actual size of the
 *   structures (they are only pointer references), we'll simply provide empty
 *   opaque types.
 *
 */

typedef struct ActiveCommandTrace ActiveCommandTrace;
typedef struct ActiveInterpTrace ActiveInterpTrace;
typedef struct _ActiveVarTrace ActiveVarTrace;
typedef struct _CallFrame CallFrame;
typedef struct _ExecEnv ExecEnv;
typedef struct _Namespace Namespace;
typedef struct _Proc Proc;
typedef struct ResolverScheme ResolverScheme;
typedef struct _TclRegexp TclRegexp;
typedef struct _Trace Trace;

/*
 *---------------------------------------------------------------------------
 *
 *  This structure defines an interpreter, which is a collection of commands
 *  plus other state information related to interpreting commands, such as
 *  variable storage. Primary responsibility for this data structure is in
 *  tclBasic.c, but almost every TCL source file uses something in here.
 *  
 *---------------------------------------------------------------------------
 */


typedef struct Interp {
    char *result;
    Tcl_FreeProc *freeProc;
    int errorLine;

#if (_TCL_VERSION >= _VERSION(8,1,0))
    struct TclStubs *stubTable;
    TclHandle handle;
#else 
    Tcl_Obj *objResultPtr;
#endif /* >= 8.1.0 */

    Namespace *globalNsPtr;

#if (_TCL_VERSION >= _VERSION(8,1,0))
    Tcl_HashTable *hiddenCmdTablePtr;
    ClientData interpInfo;
#endif /* >= 8.1.0 */

    Tcl_HashTable mathFuncTable;
    int numLevels;
    int maxNestingDepth;
    CallFrame *framePtr;
    CallFrame *varFramePtr;
    ActiveVarTrace *activeTracePtr;
    int returnCode;
    char *errorInfo;
    char *errorCode;
    char *appendResult;
    int appendAvl;
    int appendUsed;

#if (_TCL_VERSION < _VERSION(8,1,0))
    char *patterns[5];
    int patLengths[5];
    regexp *regexps[5];
#endif /* < 8.1.0 */

    Tcl_HashTable packageTable;
    char *packageUnknown;
    int cmdCount;
    int evalFlags;
    int termOffset;

#if (_TCL_VERSION >= _VERSION(8,1,0))
    LiteralTable literalTable;
#endif

    int compileEpoch;
    Proc *compiledProcPtr;
    ResolverScheme *resolverPtr;

#if (_TCL_VERSION >= _VERSION(8,4,0))
    Tcl_Obj *scriptFile;
#else 
    char *scriptFile;
#endif /* >= 8.4.0 */

    int flags;
    long randSeed;
    Trace *tracePtr;
    Tcl_HashTable *assocData;
    struct ExecEnv *execEnvPtr;
    Tcl_Obj *emptyObjPtr;
    char resultSpace[TCL_RESULT_SIZE+1];

#if (_TCL_VERSION >= _VERSION(8,1,0))
    Tcl_Obj *objResultPtr;
    Tcl_ThreadId threadId;
#endif /* >= 8.1.0 */

#if (_TCL_VERSION >= _VERSION(8,4,0))
    ActiveCommandTrace *activeCmdTracePtr;
    ActiveInterpTrace *activeInterpTracePtr;
    int tracesForbiddingInline;
#endif /* >= 8.4.0 */

#if (_TCL_VERSION >= _VERSION(8,4,1))
#ifdef TCL_COMPILE_STATS
    ByteCodeStats stats;	
#endif /* TCL_COMPILE_STATS */	  
#endif /* >= 8.4.1 */

} Interp;

/*
 * EvalFlag bits for Interp structures:
 *
 * TCL_BRACKET_TERM	1 means that the current script is terminated by
 *			a close bracket rather than the end of the string.
 * TCL_ALLOW_EXCEPTIONS	1 means it's OK for the script to terminate with
 *			a code other than TCL_OK or TCL_ERROR;  0 means
 *			codes other than these should be turned into errors.
 */

#define TCL_BRACKET_TERM	  1
#define TCL_ALLOW_EXCEPTIONS	  4

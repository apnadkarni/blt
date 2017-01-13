/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * tclInterp.h --
 *
 * Excerpts from tclInt.h.  Used to examine interpreter internals.
 * Needed by the former parsing (bltParse.c) functions.
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
 * This file contains excerpts from tclInt.h of the TCL library distribution.
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 *
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
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
typedef struct _LimitHandler LimitHandler;
typedef struct _ContLineLoc ContLineLoc;

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

typedef struct CmdFrame {
    /*
     * General data. Always available.
     */

    int type;			/* Values see below. */
    int level;			/* Number of frames in stack, prevent O(n)
				 * scan of list. */
    int *line;			/* Lines the words of the command start on. */
    int nline;
    CallFrame *framePtr;	/* Procedure activation record, may be
				 * NULL. */
    struct CmdFrame *nextPtr;	/* Link to calling frame. */
    /*
     * Data needed for Eval vs TEBC
     *
     * EXECUTION CONTEXTS and usage of CmdFrame
     *
     * Field	  TEBC		  EvalEx	  EvalObjEx
     * =======	  ====		  ======	  =========
     * level	  yes		  yes		  yes
     * type	  BC/PREBC	  SRC/EVAL	  EVAL_LIST
     * line0	  yes		  yes		  yes
     * framePtr	  yes		  yes		  yes
     * =======	  ====		  ======	  =========
     *
     * =======	  ====		  ======	  ========= union data
     * line1	  -		  yes		  -
     * line3	  -		  yes		  -
     * path	  -		  yes		  -
     * -------	  ----		  ------	  ---------
     * codePtr	  yes		  -		  -
     * pc	  yes		  -		  -
     * =======	  ====		  ======	  =========
     *
     * =======	  ====		  ======	  ========= | union cmd
     * listPtr	  -		  -		  yes	    |
     * -------	  ----		  ------	  --------- |
     * cmd	  yes		  yes		  -	    |
     * cmdlen	  yes		  yes		  -	    |
     * -------	  ----		  ------	  --------- |
     */

    union {
	struct {
	    Tcl_Obj *path;	/* Path of the sourced file the command is
				 * in. */
	} eval;
	struct {
	    const void *codePtr;/* Byte code currently executed... */
	    const char *pc;	/* ... and instruction pointer. */
	} tebc;
    } data;
    union {
	struct {
	    const char *cmd;	/* The executed command, if possible... */
	    int len;		/* ... and its length. */
	} str;
	Tcl_Obj *listPtr;	/* Tcl_EvalObjEx, cmd list. */
    } cmd;
} CmdFrame;

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
    /*
     * Note: the first three fields must match exactly the fields in a
     * Tcl_Interp struct (see tcl.h). If you change one, be sure to change the
     * other.
     *
     * The interpreter's result is held in both the string and the
     * objResultPtr fields. These fields hold, respectively, the result's
     * string or object value. The interpreter's result is always in the
     * result field if that is non-empty, otherwise it is in objResultPtr.
     * The two fields are kept consistent unless some C code sets
     * interp->result directly. Programs should not access result and
     * objResultPtr directly; instead, they should always get and set the
     * result using procedures such as Tcl_SetObjResult, Tcl_GetObjResult, and
     * Tcl_GetStringResult. See the SetResult man page for details.
     */

    char *result;		/* If the last command returned a string
				 * result, this points to it. Should not be
				 * accessed directly; see comment above. */
    Tcl_FreeProc *freeProc;	/* Zero means a string result is statically
				 * allocated. TCL_DYNAMIC means string result
				 * was allocated with ckalloc and should be
				 * freed with ckfree. Other values give
				 * address of procedure to invoke to free the
				 * string result. Tcl_Eval must free it before
				 * executing next command. */
    int errorLine;		/* When TCL_ERROR is returned, this gives the
				 * line number in the command where the error
				 * occurred (1 means first line). */
#if (_TCL_VERSION >= _VERSION(8,1,0))
    struct TclStubs *stubTable;
				/* Pointer to the exported Tcl stub table. On
				 * previous versions of Tcl this is a pointer
				 * to the objResultPtr or a pointer to a
				 * buckets array in a hash table. We therefore
				 * have to do some careful checking before we
				 * can use this. */
    TclHandle handle;		/* Handle used to keep track of when this
				 * interp is deleted. */

#else 
    Tcl_Obj *objResultPtr;
#endif /* >= 8.1.0 */
    Namespace *globalNsPtr;	/* The interpreter's global namespace. */
#if (_TCL_VERSION >= _VERSION(8,1,0))
    Tcl_HashTable *hiddenCmdTablePtr;
				/* Hash table used by tclBasic.c to keep track
				 * of hidden commands on a per-interp
				 * basis. */
    ClientData interpInfo;	/* Information used by tclInterp.c to keep
				 * track of master/slave interps on a
				 * per-interp basis. */
#endif /* >= 8.1.0 */

    Tcl_HashTable mathFuncTable;
                                /* No longer used (was mathFuncTable) */
    int numLevels;		/* Keeps track of how many nested calls to
				 * Tcl_Eval are in progress for this
				 * interpreter. It's used to delay deletion of
				 * the table until all Tcl_Eval invocations
				 * are completed. */
    int maxNestingDepth;	/* If numLevels exceeds this value then Tcl
				 * assumes that infinite recursion has
				 * occurred and it generates an error. */
    CallFrame *framePtr;	/* Points to top-most in stack of all nested
				 * procedure invocations. */
    CallFrame *varFramePtr;	/* Points to the call frame whose variables
				 * are currently in use (same as framePtr
				 * unless an "uplevel" command is
				 * executing). */
    ActiveVarTrace *activeVarTracePtr;
				/* First in list of active traces for interp,
				 * or NULL if no active traces. */
    int returnCode;		/* [return -code] parameter. */
#if (_TCL_VERSION < _VERSION(8,5,0))
    char *errorInfo;
    char *errorCode;
#else
    CallFrame *rootFramePtr;	/* Global frame pointer for this
				 * interpreter. */
    Namespace *lookupNsPtr;	/* Namespace to use ONLY on the next
				 * TCL_EVAL_INVOKE call to Tcl_EvalObjv. */

#endif
    /*
     * Information used by Tcl_AppendResult to keep track of partial results.
     * See Tcl_AppendResult code for details.
     */

    char *appendResult;		/* Storage space for results generated by
				 * Tcl_AppendResult. Ckalloc-ed. NULL means
				 * not yet allocated. */
    int appendAvl;		/* Total amount of space available at
				 * partialResult. */
    int appendUsed;		/* Number of non-null bytes currently stored
				 * at partialResult. */

#if (_TCL_VERSION < _VERSION(8,1,0))
    char *patterns[5];
    int patLengths[5];
    regexp *regexps[5];
#endif /* < 8.1.0 */
    /*
     * Information about packages. Used only in tclPkg.c.
     */

    Tcl_HashTable packageTable;	/* Describes all of the packages loaded in or
				 * available to this interpreter. Keys are
				 * package names, values are (Package *)
				 * pointers. */
    char *packageUnknown;	/* Command to invoke during "package require"
				 * commands for packages that aren't described
				 * in packageTable. Ckalloc'ed, may be
				 * NULL. */
    /*
     * Miscellaneous information:
     */

    int cmdCount;		/* Total number of times a command procedure
				 * has been called for this interpreter. */
    int evalFlags;		/* Flags to control next call to Tcl_Eval.
				 * Normally zero, but may be set before
				 * calling Tcl_Eval. See below for valid
				 * values. */
    int termOffset;

#if (_TCL_VERSION >= _VERSION(8,1,0))
    LiteralTable literalTable;	/* Contains LiteralEntry's describing all Tcl
				 * objects holding literals of scripts
				 * compiled by the interpreter. Indexed by the
				 * string representations of literals. Used to
				 * avoid creating duplicate objects. */
#endif
    int compileEpoch;		/* Holds the current "compilation epoch" for
				 * this interpreter. This is incremented to
				 * invalidate existing ByteCodes when, e.g., a
				 * command with a compile procedure is
				 * redefined. */
    Proc *compiledProcPtr;	/* If a procedure is being compiled, a pointer
				 * to its Proc structure; otherwise, this is
				 * NULL. Set by ObjInterpProc in tclProc.c and
				 * used by tclCompile.c to process local
				 * variables appropriately. */
    ResolverScheme *resolverPtr;
				/* Linked list of name resolution schemes
				 * added to this interpreter. Schemes are
				 * added and removed by calling
				 * Tcl_AddInterpResolvers and
				 * Tcl_RemoveInterpResolver respectively. */
#if (_TCL_VERSION >= _VERSION(8,4,0))
    Tcl_Obj *scriptFile;	/* NULL means there is no nested source
				 * command active; otherwise this points to
				 * pathPtr of the file being sourced. */
#else 
    char *scriptFile;
#endif /* >= 8.4.0 */
    int flags;			/* Various flag bits. See below. */
    long randSeed;		/* Seed used for rand() function. */
    Trace *tracePtr;		/* List of traces for this interpreter. */
    Tcl_HashTable *assocData;	/* Hash table for associating data with this
				 * interpreter. Cleaned up when this
				 * interpreter is deleted. */
    struct ExecEnv *execEnvPtr;	/* Execution environment for Tcl bytecode
				 * execution. Contains a pointer to the Tcl
				 * evaluation stack. */
    Tcl_Obj *emptyObjPtr;	/* Points to an object holding an empty
				 * string. Returned by Tcl_ObjSetVar2 when
				 * variable traces change a variable in a
				 * gross way. */
    char resultSpace[TCL_RESULT_SIZE+1];
				/* Static space holding small results. */
#if (_TCL_VERSION >= _VERSION(8,1,0))
    Tcl_Obj *objResultPtr;	/* If the last command returned an object
				 * result, this points to it. Should not be
				 * accessed directly; see comment above. */
    Tcl_ThreadId threadId;	/* ID of thread that owns the interpreter. */
#endif /* >= 8.1.0 */

#if (_TCL_VERSION >= _VERSION(8,4,0))
    ActiveCommandTrace *activeCmdTracePtr;
				/* First in list of active command traces for
				 * interp, or NULL if no active traces. */
    ActiveInterpTrace *activeInterpTracePtr;
				/* First in list of active traces for interp,
				 * or NULL if no active traces. */

    int tracesForbiddingInline;	/* Count of traces (in the list headed by
				 * tracePtr) that forbid inline bytecode
				 * compilation. */
#endif /* >= 8.4.0 */
#if (_TCL_VERSION >= _VERSION(8,5,0))
    /*
     * Fields used to manage extensible return options (TIP 90).
     */

    Tcl_Obj *returnOpts;	/* A dictionary holding the options to the
				 * last [return] command. */

    Tcl_Obj *errorInfo;		/* errorInfo value (now as a Tcl_Obj). */
    Tcl_Obj *eiVar;		/* cached ref to ::errorInfo variable. */
    Tcl_Obj *errorCode;		/* errorCode value (now as a Tcl_Obj). */
    Tcl_Obj *ecVar;		/* cached ref to ::errorInfo variable. */
    int returnLevel;		/* [return -level] parameter. */

    /*
     * Resource limiting framework support (TIP#143).
     */

    struct {
	int active;		/* Flag values defining which limits have been
				 * set. */
	int granularityTicker;	/* Counter used to determine how often to
				 * check the limits. */
	int exceeded;		/* Which limits have been exceeded, described
				 * as flag values the same as the 'active'
				 * field. */

	int cmdCount;		/* Limit for how many commands to execute in
				 * the interpreter. */
	LimitHandler *cmdHandlers;
				/* Handlers to execute when the limit is
				 * reached. */
	int cmdGranularity;	/* Mod factor used to determine how often to
				 * evaluate the limit check. */

	Tcl_Time time;		/* Time limit for execution within the
				 * interpreter. */
	LimitHandler *timeHandlers;
				/* Handlers to execute when the limit is
				 * reached. */
	int timeGranularity;	/* Mod factor used to determine how often to
				 * evaluate the limit check. */
	Tcl_TimerToken timeEvent;
				/* Handle for a timer callback that will occur
				 * when the time-limit is exceeded. */

	Tcl_HashTable callbacks;/* Mapping from (interp,type) pair to data
				 * used to install a limit handler callback to
				 * run in _this_ interp when the limit is
				 * exceeded. */
    } limit;

    /*
     * Information for improved default error generation from ensembles
     * (TIP#112).
     */

    struct {
	Tcl_Obj *const *sourceObjs;
				/* What arguments were actually input into the
				 * *root* ensemble command? (Nested ensembles
				 * don't rewrite this.) NULL if we're not
				 * processing an ensemble. */
	int numRemovedObjs;	/* How many arguments have been stripped off
				 * because of ensemble processing. */
	int numInsertedObjs;	/* How many of the current arguments were
				 * inserted by an ensemble. */
    } ensembleRewrite;

    /*
     * TIP #219: Global info for the I/O system.
     */

    Tcl_Obj *chanMsg;		/* Error message set by channel drivers, for
				 * the propagation of arbitrary Tcl errors.
				 * This information, if present (chanMsg not
				 * NULL), takes precedence over a POSIX error
				 * code returned by a channel operation. */

    /*
     * Source code origin information (TIP #280).
     */

    CmdFrame *cmdFramePtr;	/* Points to the command frame containing the
				 * location information for the current
				 * command. */
    const CmdFrame *invokeCmdFramePtr;
				/* Points to the command frame which is the
				 * invoking context of the bytecode compiler.
				 * NULL when the byte code compiler is not
				 * active. */
    int invokeWord;		/* Index of the word in the command which
				 * is getting compiled. */
    Tcl_HashTable *linePBodyPtr;/* This table remembers for each statically
				 * defined procedure the location information
				 * for its body. It is keyed by the address of
				 * the Proc structure for a procedure. The
				 * values are "struct CmdFrame*". */
    Tcl_HashTable *lineBCPtr;	/* This table remembers for each ByteCode
				 * object the location information for its
				 * body. It is keyed by the address of the
				 * Proc structure for a procedure. The values
				 * are "struct ExtCmdLoc*". (See
				 * tclCompile.h) */
    Tcl_HashTable *lineLABCPtr;
    Tcl_HashTable *lineLAPtr;	/* This table remembers for each argument of a
				 * command on the execution stack the index of
				 * the argument in the command, and the
				 * location data of the command. It is keyed
				 * by the address of the Tcl_Obj containing
				 * the argument. The values are "struct
				 * CFWord*" (See tclBasic.c). This allows
				 * commands like uplevel, eval, etc. to find
				 * location information for their arguments,
				 * if they are a proper literal argument to an
				 * invoking command. Alt view: An index to the
				 * CmdFrame stack keyed by command argument
				 * holders. */
    ContLineLoc *scriptCLLocPtr;/* This table points to the location data for
				 * invisible continuation lines in the script,
				 * if any. This pointer is set by the function
				 * TclEvalObjEx() in file "tclBasic.c", and
				 * used by function ...() in the same file.
				 * It does for the eval/direct path of script
				 * execution what CompileEnv.clLoc does for
				 * the bytecode compiler.
				 */
    /*
     * TIP #268. The currently active selection mode, i.e. the package require
     * preferences.
     */

    int packagePrefer;		/* Current package selection mode. */

    /*
     * Hashtables for variable traces and searches.
     */

    Tcl_HashTable varTraces;	/* Hashtable holding the start of a variable's
				 * active trace list; varPtr is the key. */
    Tcl_HashTable varSearches;	/* Hashtable holding the start of a variable's
				 * active searches list; varPtr is the key. */
    /*
     * The thread-specific data ekeko: cache pointers or values that
     *  (a) do not change during the thread's lifetime
     *  (b) require access to TSD to determine at runtime
     *  (c) are accessed very often (e.g., at each command call)
     *
     * Note that these are the same for all interps in the same thread. They
     * just have to be initialised for the thread's master interp, slaves
     * inherit the value.
     *
     * They are used by the macros defined below.
     */

    void *allocCache;
    void *pendingObjDataPtr;	/* Pointer to the Cache and PendingObjData
				 * structs for this interp's thread; see
				 * tclObj.c and tclThreadAlloc.c */
    int *asyncReadyPtr;		/* Pointer to the asyncReady indicator for
				 * this interp's thread; see tclAsync.c */
    int *stackBound;		/* Pointer to the limit stack address
				 * allowable for invoking a new command
				 * without "risking" a C-stack overflow; see
				 * TclpCheckStackSpace in the platform's
				 * directory. */

#ifdef TCL_COMPILE_STATS
    ByteCodeStats stats;        
#endif /* TCL_COMPILE_STATS */    
#endif /* >= 8.5.0 */

} Interp;

/*
 * EvalFlag bits for Interp structures:
 *
 * TCL_BRACKET_TERM     1 means that the current script is terminated by
 *                      a close bracket rather than the end of the string.
 * TCL_ALLOW_EXCEPTIONS 1 means it's OK for the script to terminate with
 *                      a code other than TCL_OK or TCL_ERROR;  0 means
 *                      codes other than these should be turned into errors.
 */

#define TCL_BRACKET_TERM          1
#define TCL_ALLOW_EXCEPTIONS      4

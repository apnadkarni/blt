/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltBgexec.c --
 *
 * This module implements a background "exec" command for the BLT toolkit.
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
 */

#define _GNU_SOURCE
#define _XOPEN_SOURCE 500
#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"

#ifndef NO_BGEXEC

#ifdef HAVE_FCNTL_H
  #include <fcntl.h>
#endif  /* HAVE_FCNTL_H */

#ifdef HAVE_SIGNAL_H
  #include <signal.h>
#endif  /* HAVE_SIGNAL_H */

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_SYS_PARAM_H
  #include <sys/param.h>
#endif  /* HAVE_SYS_PARAM_H */

#ifdef HAVE_SYS_TYPES_H
  #include <sys/types.h>
#endif  /* HAVE_SYS_TYPES_H */

#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef MACOSX
  #include <crt_externs.h>
  #include <sys/ioctl.h>
#endif

#ifdef HAVE_STROPTS_H
  #include <stropts.h>
#endif  /* HAVE_STROPTS_H */

#ifdef HAVE_PTY_H
  #include <pty.h>
#endif  /* HAVE_PTY_H */

#ifdef HAVE_TERMIOS_H
  #include <termios.h>
  #include <unistd.h>
#endif  /* HAVE_TERMIOS_H */

#ifdef HAVE_SETSID
  #if defined(HAVE_GETPT) || defined(HAVE_POSIX_OPENPT) || defined(PTYRANGE0) 
    #define HAVE_PTY 1
  #endif
  #define HAVE_SESSION 1
#endif

#include "bltAlloc.h"
#include "bltChain.h"
#include "bltSwitch.h"
#include "bltWait.h"
#include "bltInitCmd.h"

static Tcl_ObjCmdProc BgexecCmdProc;

#define PTY_NAME_SIZE   32

#if (_TCL_VERSION <  _VERSION(8,1,0)) 
typedef void *Tcl_Encoding;             /* Make up dummy type for
                                         * encoding.  */
#endif

#define ENCODING_ASCII          ((Tcl_Encoding)NULL)
#define ENCODING_BINARY         ((Tcl_Encoding)1)

#ifdef WIN32 
  #ifndef __GNUC__
     #ifdef O_NONBLOCK
        #define O_NONBLOCK        1
     #endif
  #endif /* __GNUC__ */
#endif /* WIN32 */

/*
 *  This module creates a stand in for the old Tcl_CreatePipeline call in
 *  the TCL C library.  The prescribed routine is Tcl_OpenCommandChannel.
 *  But it hides the pids of the pipeline away (unless of course you pry
 *  open the undocumented structure PipeStatus as clientData).  The bigger
 *  problem is that I couldn't figure any way to make one side of the pipe
 *  to be non-blocking.
 */

#ifdef WIN32
  #define kill                    KillProcess
  #define waitpid                 WaitProcess
#endif  /* WIN32 */

#define BGEXEC_THREAD_KEY       "BLT Bgexec Data"

#define READ_AGAIN      (0)
#define READ_EOF        (-1)
#define READ_ERROR      (-2)

/* The wait-related definitions are taken from tclUnix.h */

#define TRACE_FLAGS (TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY)

#define BLOCK_SIZE      1024            /* Size of allocation blocks for
                                         * buffer */
#define DEF_BUFFER_SIZE (BLOCK_SIZE * 2)
#define MAX_READS       100             /* Maximum # of successful reads
                                         * before stopping to let TCL catch
                                         * up on events */
#ifdef WIN32
#define SINKOPEN(sinkPtr)  ((sinkPtr)->fileHandle != NULL)
#else 
#define SINKOPEN(sinkPtr)  ((sinkPtr)->fd != -1)
#endif  /* WIN32 */

#ifndef NSIG
#define NSIG            32              /* # of signals available */
#endif /*NSIG*/

#ifndef SIGINT
#define SIGINT          2
#endif /* SIGINT */

#ifndef SIGQUIT
#define SIGQUIT         3
#endif /* SIGQUIT */

#ifndef SIGKILL
#define SIGKILL         9
#endif /* SIGKILL */

#ifndef SIGTERM
#define SIGTERM         14
#endif /* SIGTERM */

typedef struct {
    int number;
    const char *name;
} SignalToken;

static SignalToken signalTable[] =
{
#ifdef SIGABRT
    {SIGABRT, "SIGABRT"},
#endif
#ifdef SIGALRM
    {SIGALRM, "SIGALRM"},
#endif
#ifdef SIGBUS
    {SIGBUS, "SIGBUS"},
#endif
#ifdef SIGCHLD
    {SIGCHLD, "SIGCHLD"},
#endif
#if defined(SIGCLD) && (!defined(SIGCHLD) || (SIGCLD != SIGCHLD))
    {SIGCLD, "SIGCLD"},
#endif
#ifdef SIGCONT
    {SIGCONT, "SIGCONT"},
#endif
#if defined(SIGEMT) && (!defined(SIGXCPU) || (SIGEMT != SIGXCPU))
    {SIGEMT, "SIGEMT"},
#endif
#ifdef SIGFPE
    {SIGFPE, "SIGFPE"},
#endif
#ifdef SIGHUP
    {SIGHUP, "SIGHUP"},
#endif
#ifdef SIGILL
    {SIGILL, "SIGILL"},
#endif
#ifdef SIGINT
    {SIGINT, "SIGINT"},
#endif
#ifdef SIGIO
    {SIGIO, "SIGIO"},
#endif
#if defined(SIGIOT) && (!defined(SIGABRT) || (SIGIOT != SIGABRT))
    {SIGIOT, "SIGIOT"},
#endif
#ifdef SIGKILL
    {SIGKILL, "SIGKILL"},
#endif
#if defined(SIGLOST) && (!defined(SIGIOT) || (SIGLOST != SIGIOT)) && (!defined(SIGURG) || (SIGLOST != SIGURG))
    {SIGLOST, "SIGLOST"},
#endif
#ifdef SIGPIPE
    {SIGPIPE, "SIGPIPE"},
#endif
#if defined(SIGPOLL) && (!defined(SIGIO) || (SIGPOLL != SIGIO))
    {SIGPOLL, "SIGPOLL"},
#endif
#ifdef SIGPROF
    {SIGPROF, "SIGPROF"},
#endif
#if defined(SIGPWR) && (!defined(SIGXFSZ) || (SIGPWR != SIGXFSZ))
    {SIGPWR, "SIGPWR"},
#endif
#ifdef SIGQUIT
    {SIGQUIT, "SIGQUIT"},
#endif
#ifdef SIGSEGV
    {SIGSEGV, "SIGSEGV"},
#endif
#ifdef SIGSTOP
    {SIGSTOP, "SIGSTOP"},
#endif
#ifdef SIGSYS
    {SIGSYS, "SIGSYS"},
#endif
#ifdef SIGTERM
    {SIGTERM, "SIGTERM"},
#endif
#ifdef SIGTRAP
    {SIGTRAP, "SIGTRAP"},
#endif
#ifdef SIGTSTP
    {SIGTSTP, "SIGTSTP"},
#endif
#ifdef SIGTTIN
    {SIGTTIN, "SIGTTIN"},
#endif
#ifdef SIGTTOU
    {SIGTTOU, "SIGTTOU"},
#endif
#if defined(SIGURG) && (!defined(SIGIO) || (SIGURG != SIGIO))
    {SIGURG, "SIGURG"},
#endif
#if defined(SIGUSR1) && (!defined(SIGIO) || (SIGUSR1 != SIGIO))
    {SIGUSR1, "SIGUSR1"},
#endif
#if defined(SIGUSR2) && (!defined(SIGURG) || (SIGUSR2 != SIGURG))
    {SIGUSR2, "SIGUSR2"},
#endif
#ifdef SIGVTALRM
    {SIGVTALRM, "SIGVTALRM"},
#endif
#ifdef SIGWINCH
    {SIGWINCH, "SIGWINCH"},
#endif
#ifdef SIGXCPU
    {SIGXCPU, "SIGXCPU"},
#endif
#ifdef SIGXFSZ
    {SIGXFSZ, "SIGXFSZ"},
#endif
    {-1, "unknown signal"},
};

#ifdef TCL_THREADS
static Tcl_Mutex *mutexPtr = NULL;
#endif /* TCL_THREADS */

static Blt_Chain activePipelines;       /* List of active pipelines and
                                         * their bgexec structures. */

typedef struct _Bgexec Bgexec;

typedef int (ExecutePipelineProc)(Tcl_Interp *interp, Bgexec *bgPtr, int objc,
        Tcl_Obj *const *objv);
typedef void (KillPipelineProc)(Bgexec *bgPtr);
typedef Tcl_Obj *(CheckPipelineProc)(Bgexec *bgPtr);
typedef void (ReportPipelineProc)(Tcl_Interp *interp, Bgexec *bgPtr);

typedef struct _BgexecClass {
    const char *name;
    ExecutePipelineProc *execProc;
    KillPipelineProc *killProc;
    ReportPipelineProc *reportProc;
    CheckPipelineProc *checkProc;
} BgexecClass;

#ifdef HAVE_SESSION

static ExecutePipelineProc ExecutePipelineWithSession;
static KillPipelineProc KillSession;
static ReportPipelineProc ReportSession;
static CheckPipelineProc CheckSession;

static BgexecClass sessionClass = {
    "session",
    ExecutePipelineWithSession,
    KillSession,
    ReportSession,
    CheckSession,
};
#endif

#ifdef HAVE_PTY

static ExecutePipelineProc ExecutePipelineWithPty;

static BgexecClass ptyClass = {
    "pty",
    ExecutePipelineWithPty,
    KillSession,
    ReportSession,
    CheckSession,
};
#endif

static ExecutePipelineProc ExecutePipeline;
static KillPipelineProc KillPipeline;
static ReportPipelineProc ReportPipeline;
static CheckPipelineProc CheckPipeline;

static BgexecClass pipelineClass = {
    "pipeline",
    ExecutePipeline,
    KillPipeline,
    ReportPipeline,
    CheckPipeline,
};

/*
 * Sink buffer:
 *   ____________________
 *  |                    |  "size"      current allocated length of buffer.
 *  |                    |
 *  |--------------------|  "fill"      fill point (# characters in buffer).
 *  |  Raw               |
 *  |--------------------|  "mark"      Marks end of cooked characters.
 *  |                    |
 *  |  Cooked            |
 *  |                    |
 *  |                    |
 *  |--------------------|  "lastMark"  Mark end of processed characters.
 *  |                    |
 *  |                    |
 *  |  Processed         |
 *  |                    |
 *  |____________________| 0
 */
typedef struct {
    Bgexec *bgPtr;
    const char *name;                   /* Name of the sink */
    Tcl_Obj *doneVarObjPtr;             /* Name of a TCL variable set to
                                         * the collected data of the last
                                         * UNIX subprocess. */
    Tcl_Obj *updateVarObjPtr;           /* Name of a TCL variable updated
                                         * as data is read from the
                                         * pipe. */
    Tcl_Obj *cmdObjPtr;                 /* Start of a TCL command executed
                                         * whenever data is read from the
                                         * pipe. */
    unsigned int flags;                  
    int channelNum;                     /* Number of channel to echo data. */
    Tcl_Encoding encoding;              /* Decoding scheme to use when
                                         * translating data. */
#ifdef WIN32
    HANDLE fileHandle;
#else
    int fd;                             /* File descriptor of the pipe. */
#endif  /* WIN32 */

    int status;

    unsigned char *bytes;               /* Stores pipeline output
                                         * (malloc-ed): Initially points to
                                         * static storage */
    size_t size;                        /* Size of dynamically allocated
                                         * buffer. */

    size_t fill;                        /* # of bytes read into the
                                         * buffer. Marks the current fill
                                         * point of the buffer. */

    size_t mark;                        /* # of bytes translated
                                         * (cooked). */
    size_t lastMark;                    /* # of bytes as of the last
                                         * read. This indicates the start
                                         * of the new data in the buffer
                                         * since the last time the "update"
                                         * variable was set. */
    size_t maxSize;
    unsigned char staticSpace[DEF_BUFFER_SIZE]; /* Static space */
} Sink;

#define SINK_ECHO               (1<<2)  /* That data should be echoed to
                                         * the sink's channel (stdout or
                                         * stderr). */

#define SINK_NOTIFY             (1<<3)  /* When new data is available, 
                                         * either a TCL command will be invoked,
                                         * a TCL variable written to with the 
                                         * data, or the data will be echoed
                                         * to stdout or stderr. */
#define SINK_REDIRECT           (1<<4)  /* The user has requested the data
                                         * be redirected to a TCL command,
                                         * a TCL variable, or echoed . */
#define SINK_COLLECT            (1<<5)  /* The incoming data is to be
                                         * collected in the sink. */

struct _Bgexec {
    BgexecClass *classPtr;
    Tcl_Obj *statVarObjPtr;             /* Name of a TCL variable. Will be
                                         * set to the exit status of the
                                         * last process in the pipeline. If
                                         * the user sets this variable, the
                                         * processes in the pipeline
                                         * (i.e. processes that have not
                                         * yet completed) will be signaled
                                         * (default signal is SIGTERM). */
    int signalNum;                      /* If non-zero, indicates the
                                         * signal to send processes of the
                                         * pipeline when cleaning up.*/
    unsigned int flags;                 /* Various bit flags: see below. */
    int interval;                       /* Interval to poll for the exiting
                                         * processes in milliseconds. */
    /* Private */
    Tcl_Interp *interp;                 /* Interpreter containing variables */
    int numPids;                        /* # of processes in pipeline */
    Blt_Pid *pids;                      /* Array of process tokens from
                                         * pipeline.  Under Unix, tokens
                                         * are pid_t, while for Win32
                                         * they're handles. */
    Tcl_TimerToken timerToken;          /* Token for timer handler which
                                         * polls for the exit status of
                                         * each sub-process. If zero,
                                         * there's no timer handler
                                         * queued. */
    int *exitCodePtr;                   /* Pointer to a memory location to
                                         * contain the last process' exit
                                         * code. */
    int *donePtr;
    Sink errSink, outSink;              /* Data sinks for pipeline's stdout
                                         * and stderr channels. */
    Blt_ChainLink link;
    char *const *env;                   /* Environment variables. */
#ifdef HAVE_PTY
    int master;                         /* File descriptor of pty master. */
    int slave;                          /* File descriptor of pty slave.  */
    char masterName[PTY_NAME_SIZE+1];
    char slaveName[PTY_NAME_SIZE+1];
#endif /* HAVE_PTY */
#ifdef HAVE_SESSION
    pid_t sid;                          /* Pid of session leader. */
#endif /* HAVE_SESSION */
};

#define KEEPNEWLINE     (1<<0)          /* Indicates to not trim the
                                         * trailing newline from pipeline
                                         * data as it is deliveried to TCL
                                         * output variables. */
#define LINEBUFFERED    (1<<1)          /* Indicates to deliver data to
                                         * designated update TCL variable
                                         * and/or update TCL proc one line
                                         * at a time. */
#define IGNOREEXITCODE  (1<<2)          /* Don't generate an error if the
                                         * pipeline terminates with a
                                         * non-zero exit code.  */
#define TRACED          (1<<3)          /* Indicates that the status
                                         * variable is currently being
                                         * traced. */
#define FOREGROUND      (1<<4)          /* Indicates that the pipeline is
                                         * running in the foreground. The
                                         * bgexec command will wait for the
                                         * pipeline to complete. */
#define DONTKILL        (1<<6)          /* Indicates that the detached
                                         * pipeline should not be signaled
                                         * on exit. */
#define SESSION         (1<<7)          /* Indicates that a new session
                                         * leader for the pipeline has been
                                         * created.  When the session
                                         * leader is signaled (by setting
                                         * the status TCL variable), all
                                         * the processes and subprocesses
                                         * of the pipeline will be also be
                                         * signaled. */
#define PTY             (1<<8)          /* Indicates that the pipeline
                                         * should use a pseudo terminal and
                                         * session leader. */

static Blt_SwitchParseProc ObjToEnvironSwitchProc;
static Blt_SwitchFreeProc FreeEnvironSwitchProc;
static Blt_SwitchCustom environSwitch =
{
    ObjToEnvironSwitchProc, NULL, FreeEnvironSwitchProc, (ClientData)0,
};

static Blt_SwitchParseProc ObjToSignalSwitchProc;
static Blt_SwitchCustom killSignalSwitch =
{
    ObjToSignalSwitchProc, NULL, NULL, (ClientData)0,
};

static Blt_SwitchParseProc ObjToEncodingSwitchProc;
static Blt_SwitchFreeProc FreeEncodingSwitchProc;
static Blt_SwitchCustom encodingSwitch =
{
    ObjToEncodingSwitchProc, NULL, FreeEncodingSwitchProc, (ClientData)0
};

static Blt_SwitchParseProc ObjToEchoSwitchProc;
static Blt_SwitchCustom echoSwitch =
{
    ObjToEchoSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchSpec switchSpecs[] = 
{
    {BLT_SWITCH_CUSTOM,  "-decodeerror",    "encodingName",     (char *)NULL,
         Blt_Offset(Bgexec, errSink.encoding),  0, 0, &encodingSwitch},
    {BLT_SWITCH_CUSTOM,  "-decodeoutput",   "encodingName",     (char *)NULL,
        Blt_Offset(Bgexec, outSink.encoding),   0, 0, &encodingSwitch}, 
    {BLT_SWITCH_BOOLEAN, "-detach",         "bool",             (char *)NULL,
        Blt_Offset(Bgexec, flags),          0, DONTKILL},
    {BLT_SWITCH_CUSTOM, "-echo",           "echoName",          (char *)NULL,
        0, 0, 0, &echoSwitch},
    {BLT_SWITCH_CUSTOM,  "-environ",       "list",              (char *)NULL,
         Blt_Offset(Bgexec, env), BLT_SWITCH_NULL_OK, 0, &environSwitch},
    {BLT_SWITCH_OBJ,     "-errorvariable",  "varName",          (char *)NULL,
        Blt_Offset(Bgexec, errSink.doneVarObjPtr),    0},
    {BLT_SWITCH_BOOLEAN, "-ignoreexitcode", "bool",             (char *)NULL,
        Blt_Offset(Bgexec, flags),          0, IGNOREEXITCODE},
    {BLT_SWITCH_BOOLEAN, "-keepnewline",    "bool",             (char *)NULL,
        Blt_Offset(Bgexec, flags),          0, KEEPNEWLINE}, 
    {BLT_SWITCH_CUSTOM,  "-killsignal",     "signalName",       (char *)NULL,
        Blt_Offset(Bgexec, signalNum),      0, 0, &killSignalSwitch},
    {BLT_SWITCH_OBJ,    "-lasterrorvariable", "varName",        (char *)NULL,
        Blt_Offset(Bgexec, errSink.updateVarObjPtr),  0},
    {BLT_SWITCH_OBJ,     "-lastoutputvariable",  "varName",     (char *)NULL,
        Blt_Offset(Bgexec, outSink.updateVarObjPtr),  0},
    {BLT_SWITCH_BOOLEAN, "-linebuffered",   "bool",             (char *)NULL,
        Blt_Offset(Bgexec, flags),          0, LINEBUFFERED},
    {BLT_SWITCH_OBJ,     "-onerror",        "cmdString",        (char *)NULL,
        Blt_Offset(Bgexec, errSink.cmdObjPtr),  0},
    {BLT_SWITCH_OBJ,     "-onoutput",       "cmdString",        (char *)NULL,
        Blt_Offset(Bgexec, outSink.cmdObjPtr),  0},
    {BLT_SWITCH_OBJ,     "-outputvariable", "varName",          (char *)NULL,
        Blt_Offset(Bgexec, outSink.doneVarObjPtr),    0},
    {BLT_SWITCH_INT,     "-poll",           "milliseconds",     (char *)NULL,
        Blt_Offset(Bgexec, interval),       0},
    {BLT_SWITCH_BITS_NOARG, "-session",        "",                 (char *)NULL,
        Blt_Offset(Bgexec, flags),          0, SESSION},
    {BLT_SWITCH_BITS_NOARG, "-tty",            "",                 (char *)NULL,
        Blt_Offset(Bgexec, flags),          0, PTY},
    {BLT_SWITCH_OBJ,     "-updatevariable", "varName",          (char *)NULL,
         Blt_Offset(Bgexec, outSink.updateVarObjPtr), 0},
    {BLT_SWITCH_END}
};

/* Foreground processes
 *      stdout not redirected: read stdout and collect.
 *      stdout redirected:     read stdout and collect.
 *      stderr no redirected:  read stderr, echo possibly, don't collect.
 *      stderr redirected:     read stderr and collect.
 * Background processes  
 *      stdout not redirected: read stdout, don't collect
 *      stdout redirected:     read stdout and collect
 *      stderr not redirected: read stderr, echo possibly, don't collect.
 *      stderr redirected:     read stderr and collect.
 */

static Tcl_VarTraceProc VariableProc;
static Tcl_TimerProc TimerProc;
static Tcl_FileProc CollectStdout, CollectStderr;
static Tcl_ExitProc BgexecExitProc;

/*
 *---------------------------------------------------------------------------
 *
 * ExplainError --
 *
 *      Generate and error message in the interpreter given a message
 *      string and the current errno value.
 *
 *---------------------------------------------------------------------------
 */
static void
ExplainError(Tcl_Interp *interp, const char *mesg)
{
    if (mesg != NULL) {
        Tcl_AppendResult(interp, mesg, ": ", Tcl_PosixError(interp),
                         (char *)NULL);
    } else {
        Tcl_AppendResult(interp, Tcl_PosixError(interp), (char *)NULL);
    }
}

#ifndef WIN32

/*
 *---------------------------------------------------------------------------
 *
 * WriteErrorMesgToParent --
 *
 *      Writes the error message to the parent process.  The error message
 *      if the current result in the child's interpreter.  
 *
 * Results:
 *      Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
static int
WriteErrorMesgToParent(Tcl_Interp *interp, int f)
{
    ssize_t numWritten;
    const char *mesg;
    int length;
    
    mesg = Tcl_GetStringFromObj(Tcl_GetObjResult(interp), &length);
    numWritten = write(f, mesg, length);
    assert(numWritten == length);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadErrorMesgFromChild --
 *
 *      Reads back the error message sent by the child process to see if
 *      the pipeline started up properly. The information in the pipe (if
 *      any) is the TCL error message from the child process.
 *
 * Results:
 *      Returns a standard TCL result.  It the pipeline start correctly,
 *      TCL_OK is returned.  Otherwise TCL_ERROR and the interpreter
 *      will contain the error message from the child process.
 *
 *---------------------------------------------------------------------------
 */
static int
ReadErrorMesgFromChild(Tcl_Interp *interp, int f)
{
    char mesg[BUFSIZ+1];
    ssize_t numRead, numBytes;
    
    /*
     * Read back from the error pipe to see if the pipeline started up
     * properly. The info in the pipe (if any) if the TCL error message
     * from the child process.
     */
    numBytes = 0;
    do {
        numRead = read(f, mesg, BUFSIZ);
        if (numRead == -1) {
            return TCL_ERROR;
        }
        mesg[numRead] = '\0';
        Tcl_AppendResult(interp, mesg, (char *)NULL);
        numBytes += numRead;
    } while (numRead > 0);
    close(f);
    return (numBytes > 0) ? TCL_ERROR : TCL_OK;
}
#endif /*!WIN32*/
/*
 *---------------------------------------------------------------------------
 *
 * CreateEnviron --
 *
 *      Creates an environ array from the current enviroment variable and
 *      the override variables stored in the given TCL list.
 *
 * Results:
 *      Returns an new environ array.  
 *
 * Side effects:
 *      The array is malloc-ed. It is the responsibility of the caller to
 *      free the array when it is done with it.
 *
 *---------------------------------------------------------------------------
 */
static int
CreateEnviron(Tcl_Interp *interp, int objc, Tcl_Obj **objv,
              char *const **envPtrPtr)
{
    Blt_HashTable envTable;         /* Temporary hash table for environment
                                     * variables. */
    size_t numBytes;                /* Counter for # bytes in new buffer */
    
    Blt_InitHashTable(&envTable, BLT_STRING_KEYS);
    numBytes = 0;

    /* Step 1: Add the override enviroment variables to the empty table. */
    {
        int i;
        
        for (i = 0; i < objc; i += 2) {
            Blt_HashEntry *hPtr;
            const char *name, *value;
            int isNew, length;
            
            name = Tcl_GetStringFromObj(objv[i], &length);
            numBytes += length;
            hPtr = Blt_CreateHashEntry(&envTable, name, &isNew);
            value = Tcl_GetStringFromObj(objv[i+1], &length);
            Blt_SetHashValue(hPtr, value);
            numBytes += length;
            numBytes += 2;                     /* Include '\0' and '=' */
        }
    }
    /* Step 2:  Add the current environment variables, but don't overwrite
     *          the existing table entries. */
    {
        char *const *varPtr;
#ifdef MACOSX
        char **environ;
        environ = (char **)_NSGetEnviron();
#else
        extern char **environ;
#endif

        for (varPtr = environ; *varPtr != NULL; varPtr++) {
            char *eqsign;
            char *p;
            
            eqsign = NULL;
            /* Search for the end of the string, saving the position of the
             * first '=' found. */
            for (p = (char *)*varPtr; *p != '\0'; p++) {
                if ((*p == '=') && (eqsign == NULL)) {
                    eqsign = p;
                }
            }
            if (*varPtr == p) {
                break;
            }
            if (eqsign != NULL) {
                Blt_HashEntry *hPtr;
                int isNew;
                
                /* Temporarily terminate the variable name at the '=' and
                 * create a hash table entry for the variable. */
                *eqsign = '\0';
                hPtr = Blt_CreateHashEntry(&envTable, *varPtr, &isNew);
                if (isNew) {    /* Ignore the variable if it already exists. */
                    Blt_SetHashValue(hPtr, eqsign + 1);
                    /* Not already in table, add variable as is including the
                     * NUL byte. */
                    numBytes += p - *varPtr + 1;    /* Include NUL byte */
                }
                *eqsign = '=';              /* Restore the '='. */
            }
        }
        numBytes++;                           /* Final NUL byte. */
        assert(numBytes < 100000);
    }
    /* Step 3: Allocate an environment array */
    /* Step 4: Add the name/value pairs from the hashtable to the array as
     *         "name=value". */
    {
#ifdef WIN32
        Blt_HashEntry *hPtr;
        Blt_HashSearch iter;
        char **array;
        char *p;
        
        array = Blt_AssertMalloc(numBytes * sizeof(char));
	p = (char *)array;
        for (hPtr = Blt_FirstHashEntry(&envTable, &iter); hPtr != NULL;
             hPtr = Blt_NextHashEntry(&iter)) {
            size_t numBytes;
            const char *name, *value;
            
            name = Blt_GetHashKey(&envTable, hPtr);
            value = Blt_GetHashValue(hPtr);
            numBytes = sprintf(p, "%s=%s", name, value);
            p += numBytes;
            *p++ = '\0';
        }
        *p++='\0';
        Blt_DeleteHashTable(&envTable);
        *envPtrPtr = (char **)array;
#else
        Blt_HashEntry *hPtr;
        Blt_HashSearch iter;
        char **array;
        char *p;
        int n, arraySize;
        
        arraySize = (envTable.numEntries + 1) * sizeof(char **);
        array = Blt_AssertMalloc(arraySize + (numBytes * sizeof(char)));
        p = (char *)array + arraySize;
        
        n = 0;
        for (hPtr = Blt_FirstHashEntry(&envTable, &iter); hPtr != NULL;
             hPtr = Blt_NextHashEntry(&iter)) {
            size_t numBytes;
            const char *name, *value;
            
            name = Blt_GetHashKey(&envTable, hPtr);
            value = Blt_GetHashValue(hPtr);
            numBytes = sprintf(p, "%s=%s", name, value);
            array[n] = p;
            p += numBytes;
            *p++ = '\0';
            n++;
        }
        array[n] = NULL;                       /* Add final NUL byte. */
        *p++='\0';
        Blt_DeleteHashTable(&envTable);
        *envPtrPtr = (char **)array;
#endif  /* WIN32 */
    }
    return TCL_OK;
}  

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSignalSwitchProc --
 *
 *      Convert a Tcl_Obj representing a signal number into its integer
 *      value.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToSignalSwitchProc(ClientData clientData, Tcl_Interp *interp,
                      const char *switchName, Tcl_Obj *objPtr, char *record,
                      int offset, int flags)
{
    char *string;
    int *signalPtr = (int *)(record + offset);
    int signalNum;

    string = Tcl_GetString(objPtr);
    if (string[0] == '\0') {
        *signalPtr = 0;
        return TCL_OK;
    }
    if (isdigit(UCHAR(string[0]))) {
        if (Tcl_GetIntFromObj(interp, objPtr, &signalNum) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        char *name;
        SignalToken *sp;

        name = string;

        /*  Clip off any "SIG" prefix from the signal name */
        if ((name[0] == 'S') && (name[1] == 'I') && (name[2] == 'G')) {
            name += 3;
        }
        signalNum = -1;
        for (sp = signalTable; sp->number != -1; sp++) {
            if (strcmp(sp->name + 3, name) == 0) {
                signalNum = sp->number;
                break;
            }
        }
        if (signalNum < 0) {
            Tcl_AppendResult(interp, "unknown signal \"", string, "\"",
                (char *)NULL);
            return TCL_ERROR;
        }
    }
    if ((signalNum < 0) || (signalNum > NSIG)) {
        /* Outside range of signals */
        Tcl_AppendResult(interp, "signal number \"", string,
            "\" is out of range", (char *)NULL);
        return TCL_ERROR;
    }
    *signalPtr = signalNum;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToEncodingSwitchProc --
 *
 *      Convert a Tcl_Obj representing a encoding into a Tcl_Encoding.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToEncodingSwitchProc(ClientData clientData, Tcl_Interp *interp,
                        const char *switchName, Tcl_Obj *objPtr, char *record,
                        int offset, int flags)
{
    Tcl_Encoding *encodingPtr = (Tcl_Encoding *)(record + offset);
    Tcl_Encoding encoding;
    const char *name;

    name = Tcl_GetString(objPtr);
    encoding = ENCODING_ASCII;
    if (name != NULL) {
        if (strcmp(name, "binary") == 0) {
            encoding = ENCODING_BINARY;
        } else {
#if (_TCL_VERSION >= _VERSION(8,1,0)) 
            encoding = Tcl_GetEncoding(interp, name);
            if (encoding == NULL) {
                return TCL_ERROR;
            }
#endif
        }
    }
    if ((*encodingPtr != ENCODING_BINARY) && (*encodingPtr != ENCODING_ASCII)) {
        Tcl_FreeEncoding(*encodingPtr);
    }
    *encodingPtr = encoding;
    return TCL_OK;
}

/*ARGSUSED*/
static void
FreeEncodingSwitchProc(ClientData clientData, char *record, int offset,
                       int flags)
{
    Tcl_Encoding *encodingPtr = (Tcl_Encoding *)(record + offset);

    if ((*encodingPtr != ENCODING_BINARY) && (*encodingPtr != ENCODING_ASCII)) {
        Tcl_FreeEncoding(*encodingPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToEnvironSwitchProc --
 *
 *      Convert a Tcl_Obj representing a list of variable names and values
 *      into an environment string.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToEnvironSwitchProc(ClientData clientData, Tcl_Interp *interp,
                       const char *switchName, Tcl_Obj *objPtr, char *record,
                       int offset, int flags)
{
    char *const **envPtrPtr = (char *const **)(record + offset);
    char *const *env;
    Tcl_Obj **objv;
    int objc;
    
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (*envPtrPtr != NULL) {
        Blt_Free(*envPtrPtr);
        *envPtrPtr = NULL;
    }
    if (objc & 0x1) {
        Tcl_AppendResult(interp,
                         "odd number of arguments: should be \"name value\"",
                         (char *)NULL);
        return TCL_ERROR;
    }
    if (objc == 0) {
        return TCL_OK;
    }
    if (CreateEnviron(interp, objc, objv, &env) != TCL_OK) {
        return TCL_ERROR;
    }
    *envPtrPtr = env;
    return TCL_OK;
}

/*ARGSUSED*/
static void
FreeEnvironSwitchProc(ClientData clientData, char *record, int offset,
                      int flags)
{
    char *const **envPtrPtr = (char *const **)(record + offset);

    if (*envPtrPtr != NULL) {
        Blt_Free(*envPtrPtr);
        *envPtrPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToEchoSwitchProc --
 *
 *      Convert a Tcl_Obj representing echo flags.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToEchoSwitchProc(ClientData clientData, Tcl_Interp *interp,
                    const char *switchName, Tcl_Obj *objPtr, char *record,
                    int offset, int flags)
{
    Bgexec *bgPtr = (Bgexec *)(record);
    const char *string;
    char c;
    
    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'b') && (strcmp(string, "both") == 0)) {
        bgPtr->outSink.flags |= SINK_ECHO;
        bgPtr->errSink.flags |= SINK_ECHO;
    } else if ((c == 'e') && (strcmp(string, "error") == 0)) {
        bgPtr->outSink.flags &= ~SINK_ECHO;
        bgPtr->errSink.flags |= SINK_ECHO;
    } else if ((c == 'n') && (strcmp(string, "none") == 0)) {
        bgPtr->outSink.flags &= ~SINK_ECHO;
        bgPtr->errSink.flags &= ~SINK_ECHO;
    } else if ((c == 'o') && (strcmp(string, "output") == 0)) {
        bgPtr->outSink.flags |= SINK_ECHO;
        bgPtr->errSink.flags &= ~SINK_ECHO;
    } else {
        Tcl_AppendResult(interp, "unknown echo value \"", string,
                "\": should be error, output, both, or none.", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetSinkData --
 *
 *      Returns the data currently saved in the buffer
 *
 *---------------------------------------------------------------------------
 */
static void
GetSinkData(Sink *sinkPtr, unsigned char **dataPtr, size_t *lengthPtr)
{
    size_t length;

#ifdef notdef
    sinkPtr->bytes[sinkPtr->mark] = '\0';
#endif
    length = sinkPtr->mark;
    if ((sinkPtr->mark > 0) && (sinkPtr->encoding != ENCODING_BINARY)) {
        unsigned char *last;
        Bgexec *bgPtr;
        
        last = sinkPtr->bytes + (sinkPtr->mark - 1);
        bgPtr = sinkPtr->bgPtr;
        if (((bgPtr->flags & KEEPNEWLINE) == 0) && (*last == '\n')) {
            length--;
        }
    }
    *dataPtr = sinkPtr->bytes;
    *lengthPtr = length;
}

/*
 *---------------------------------------------------------------------------
 *
 * NextBlock --
 *
 *      Returns the next block of data since the last time this routine was
 *      called.
 *
 *---------------------------------------------------------------------------
 */
static unsigned char *
NextBlock(Sink *sinkPtr, int *lengthPtr)
{
    unsigned char *bp;
    int length;

    bp = sinkPtr->bytes + sinkPtr->lastMark;
    length = sinkPtr->mark - sinkPtr->lastMark;
    sinkPtr->lastMark = sinkPtr->mark;
    if (length > 0) {
        Bgexec *bgPtr;
        
        bgPtr = sinkPtr->bgPtr;
        if ((!(bgPtr->flags & KEEPNEWLINE)) && (bp[length-1] == '\n')) {
            length--;
        }
        *lengthPtr = length;
        return bp;
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * NextLine --
 *
 *      Returns the next line of data.
 *
 *---------------------------------------------------------------------------
 */
static unsigned char *
NextLine(Sink *sinkPtr, int *lengthPtr)
{
    if (sinkPtr->mark > sinkPtr->lastMark) {
        unsigned char *string;
        int newBytes;
        int i;

        string = sinkPtr->bytes + sinkPtr->lastMark;
        newBytes = sinkPtr->mark - sinkPtr->lastMark;
        for (i = 0; i < newBytes; i++) {
            if (string[i] == '\n') {
                int length;
                Bgexec *bgPtr;
                
                length = i + 1;
                sinkPtr->lastMark += length;
                bgPtr = sinkPtr->bgPtr;
                if ((bgPtr->flags & KEEPNEWLINE) == 0) {
                    length--;           /* Backup over the newline. */
                }
                *lengthPtr = length;
                return string;
            }
        }
        /* Newline not found.  On errors or EOF, also return a partial line. */
        if (sinkPtr->status < 0) {
            *lengthPtr = newBytes;
            sinkPtr->lastMark = sinkPtr->mark;
            return string;
        }
    }
    return NULL;
}
/*
 *---------------------------------------------------------------------------
 *
 * ResetSink --
 *
 *      Removes the bytes already processed from the buffer, possibly
 *      resetting it to empty.  This used when we don't care about keeping
 *      all the data collected from the channel (no -output flag and the
 *      process is backgrounded).
 *
 *---------------------------------------------------------------------------
 */
static void
ResetSink(Sink *sinkPtr)
{ 
    Bgexec *bgPtr;

    bgPtr = sinkPtr->bgPtr;
    if ((bgPtr->flags & LINEBUFFERED) && (sinkPtr->fill > sinkPtr->lastMark)) {
        size_t i, j;

        /* There may be bytes remaining in the buffer, awaiting another
         * read before we see the next newline.  So move the bytes to the
         * front of the array. */

        for (i = 0, j = sinkPtr->lastMark; j < sinkPtr->fill; i++, j++) {
            sinkPtr->bytes[i] = sinkPtr->bytes[j];
        }
        /* Move back the fill point and processed point. */
        sinkPtr->fill -= sinkPtr->lastMark;
        sinkPtr->mark -= sinkPtr->lastMark;
    } else {
        sinkPtr->mark = sinkPtr->fill = 0;
    }
    sinkPtr->lastMark = 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * InitSink --
 *
 *      Initializes the buffer's storage.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Storage is cleared.
 *
 *---------------------------------------------------------------------------
 */
static void
InitSink(Bgexec *bgPtr, Sink *sinkPtr, const char *name, int channelNum)
{
    sinkPtr->bgPtr = bgPtr;
    sinkPtr->name = name;
#ifdef WIN32
    sinkPtr->fileHandle = NULL;
#else
    sinkPtr->fd = -1;
#endif  /* WIN32 */
    sinkPtr->flags = 0;
    sinkPtr->bytes = sinkPtr->staticSpace;
    sinkPtr->size = DEF_BUFFER_SIZE;
    sinkPtr->encoding = ENCODING_ASCII;
    sinkPtr->channelNum = channelNum;
    ResetSink(sinkPtr);
}

static void
ConfigureSink(Bgexec *bgPtr, Sink *sinkPtr)
{
    if ((sinkPtr->updateVarObjPtr != NULL) || (sinkPtr->cmdObjPtr != NULL) ||
        (sinkPtr->flags & SINK_ECHO)) {
        sinkPtr->flags |= SINK_NOTIFY;
    }
    if ((sinkPtr->updateVarObjPtr != NULL) || (sinkPtr->cmdObjPtr != NULL) ||
        (sinkPtr->doneVarObjPtr != NULL) || (sinkPtr->flags & SINK_ECHO)) {
        sinkPtr->flags |= SINK_REDIRECT;
    }
}

static void
ConfigureSinks(Bgexec *bgPtr)
{
    ConfigureSink(bgPtr, &bgPtr->outSink);
    ConfigureSink(bgPtr, &bgPtr->errSink);
    
    /* Based on whether the pipeline is run in the foreground or background
     * determine whether data will be collected and saved into the sink's
     * buffer. */
    if (bgPtr->flags & FOREGROUND) {
        bgPtr->outSink.flags |= SINK_COLLECT;
        if (bgPtr->errSink.flags & SINK_REDIRECT) {
            bgPtr->errSink.flags |= SINK_COLLECT;
        } 
    } else {
        if (bgPtr->outSink.flags & SINK_REDIRECT) {
            bgPtr->outSink.flags |= SINK_COLLECT;
        }
        if (bgPtr->errSink.flags & SINK_REDIRECT) {
            bgPtr->errSink.flags |= SINK_COLLECT;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeSinkBuffer --
 *
 *      Frees the buffer's storage, freeing any malloc'ed space.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeSinkBuffer(Sink *sinkPtr)
{
    if (sinkPtr->bytes != sinkPtr->staticSpace) {
        Blt_Free(sinkPtr->bytes);
        sinkPtr->bytes = sinkPtr->staticSpace;
    }
#ifdef WIN32
    sinkPtr->fileHandle = NULL;
#else 
    sinkPtr->fd = -1;
#endif  /* WIN32 */
}


/*
 *---------------------------------------------------------------------------
 *
 * ExtendSinkBuffer --
 *
 *      Doubles the size of the current buffer.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static ssize_t
ExtendSinkBuffer(Sink *sinkPtr)
{
    unsigned char *bytes;
    size_t newSize;
    /*
     * Allocate a new array, double the old size
     */
    newSize = sinkPtr->size + sinkPtr->size;
#ifdef notdef
    sinkPtr->bytes[sinkPtr->mark] = '\0';
#endif
    if (sinkPtr->bytes == sinkPtr->staticSpace) {
        bytes = Blt_Malloc(sizeof(unsigned char) * newSize);
    } else {
        bytes = Blt_Realloc(sinkPtr->bytes, sizeof(unsigned char) * newSize);
    }
    if (bytes != NULL) {
        if (sinkPtr->bytes == sinkPtr->staticSpace) {
            unsigned char *sp, *dp, *send;

            dp = bytes;
            for (sp = sinkPtr->bytes, send = sp + sinkPtr->fill; sp < send; 
                 /*empty*/) {
                *dp++ = *sp++;
            }
        }
        sinkPtr->bytes = bytes;
        sinkPtr->size = newSize;
        return (sinkPtr->size - sinkPtr->fill); /* Return bytes left. */
    }
#ifdef notdef
    sinkPtr->bytes[sinkPtr->mark] = '\0';
#endif
    return -1;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadBytes --
 *
 *      Reads and appends any available data from a given file descriptor
 *      to the buffer.
 *
 * Results:
 *      Returns TCL_OK when EOF is found, TCL_RETURN if reading data would
 *      block, and TCL_ERROR if an error occurred.
 *
 *---------------------------------------------------------------------------
 */
static int
ReadBytes(Sink *sinkPtr)
{
    int i;
    ssize_t numBytes;
    Tcl_Interp *interp;

    interp = sinkPtr->bgPtr->interp;
    /*
     *  Worry about indefinite postponement.
     *
     *  Typically we want to stay in the read loop as long as it takes to
     *  collect all the data that's currently available.  But if it's
     *  coming in at a constant high rate, we need to arbitrarily break out
     *  at some point. This allows for both setting the update variable and
     *  the Tk program to handle idle events.
     */
    numBytes = 0;
    for (i = 0; i < MAX_READS; i++) {
        ssize_t bytesLeft;
        char *array;

        /* Allocate a larger buffer when the number of remaining bytes is
         * below the threshold BLOCK_SIZE.  */
        
        bytesLeft = sinkPtr->size - sinkPtr->fill;
        
        if (bytesLeft < BLOCK_SIZE) {
            bytesLeft = ExtendSinkBuffer(sinkPtr);
            if (bytesLeft < 0) {
                errno = ENOMEM;
                sinkPtr->status = READ_ERROR;
                ExplainError(interp, "ExtendSinkBuffer");
                return TCL_ERROR;
            }
        }
        array = (char *)(sinkPtr->bytes + sinkPtr->fill);

        /* Read into a buffer but make sure we leave room for a trailing
         * NUL byte. */
#ifdef WIN32
        numBytes = Blt_AsyncRead(sinkPtr->fileHandle, array, bytesLeft -1);
#else
        numBytes = read(sinkPtr->fd, array, bytesLeft - 1);
#endif /* WIN32 */
        if (numBytes == 0) {            /* EOF: break out of loop. */
            sinkPtr->status = READ_EOF;
            return TCL_BREAK;
        }
        /* This is really weird. Closing the slave generates an I/O error
         * here. */
        if (numBytes < 0) {

#ifdef O_NONBLOCK
  #define BLOCKED         EAGAIN
#else
  #define BLOCKED         EWOULDBLOCK
#endif /*O_NONBLOCK*/

            /* Either an error has occurred or no more data is currently
             * available to read.  */
            if (errno == BLOCKED) {
                sinkPtr->status = READ_AGAIN;
                return TCL_CONTINUE;
            } else if ((errno == EIO) && (sinkPtr->bgPtr->flags & PTY)) {
                sinkPtr->status = READ_EOF;
                return TCL_BREAK;
            } else {
                ExplainError(interp, "read");
                sinkPtr->status = READ_ERROR;
                return TCL_ERROR;
            }
        }
        sinkPtr->fill += numBytes;
#ifndef notdef
        sinkPtr->bytes[sinkPtr->fill] = '\0';
#endif
    }
    sinkPtr->status = numBytes;
    return TCL_OK;
}


static void
CloseSink(Sink *sinkPtr)
{
    if (SINKOPEN(sinkPtr)) {
#ifdef WIN32
        CloseHandle(sinkPtr->fileHandle);
        Blt_DeleteFileHandler(sinkPtr->fileHandle);
        sinkPtr->fileHandle = NULL;
#else
        close(sinkPtr->fd);
        Tcl_DeleteFileHandler(sinkPtr->fd);
        sinkPtr->fd = -1;               /* Mark sink as closed. */
#endif /* WIN32 */
        if (sinkPtr->doneVarObjPtr != NULL) {
            Tcl_Interp *interp;
            unsigned char *data;
            size_t length;

            interp = sinkPtr->bgPtr->interp;
            /* 
             * If data is to be collected, set the "done" variable with the
             * contents of the buffer.
             */
            GetSinkData(sinkPtr, &data, &length);
#if (_TCL_VERSION <  _VERSION(8,1,0)) 
            data[length] = '\0';
            if (Tcl_SetVar(interp, Tcl_GetString(sinkPtr->doneVarObjPtr), data, 
                           TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
                Tcl_BackgroundError(interp);
            }
#else
            if (Tcl_ObjSetVar2(interp, sinkPtr->doneVarObjPtr, NULL /*part2*/, 
                        Tcl_NewByteArrayObj(data, (int)length),
                        TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
                Tcl_BackgroundError(interp);
            }
#endif
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CookSink --
 *
 *      For Windows, translate CR/NL combinations to NL alone.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The size of the byte array may shrink and array contents shifted as
 *      carriage returns are found and removed.
 *
 *---------------------------------------------------------------------------
 */
static int
CookSink(Tcl_Interp *interp, Sink *sinkPtr)
{
    unsigned char *srcPtr, *endPtr;
    int crlf;
    int oldMark;

    oldMark = sinkPtr->mark;
    if (sinkPtr->encoding == ENCODING_BINARY) { /* binary */
        /* No translation needed. */
        sinkPtr->mark = sinkPtr->fill; 
    } else if (sinkPtr->encoding == ENCODING_ASCII) { /* ascii */
#if (_TCL_VERSION <  _VERSION(8,1,0)) 
        /* Convert NUL bytes to question marks. */
        srcPtr = sinkPtr->bytes + sinkPtr->mark;
        endPtr = sinkPtr->bytes + sinkPtr->fill;
        while (srcPtr < endPtr) {
            if (*srcPtr == '\0') {
                *srcPtr = '?';
            }
            srcPtr++;
        }
#endif /* < 8.1.0 */
        /* One-to-one translation. mark == fill. */
        sinkPtr->mark = sinkPtr->fill;
#if (_TCL_VERSION >= _VERSION(8,1,0)) 
    } else { /* Unicode. */
        int numSrcCooked, numCooked;
        int result;
        ssize_t spaceLeft, cookedSize, needed;
        size_t numRaw, numLeftOver;
        unsigned char *destPtr;
        unsigned char *raw, *cooked;
        unsigned char leftover[100];
        
        raw = sinkPtr->bytes + sinkPtr->mark;
        numRaw = sinkPtr->fill - sinkPtr->mark;
        /* Ideally, the cooked buffer size should be smaller */
        cookedSize = numRaw * TCL_UTF_MAX + 1;
        cooked = Blt_AssertMalloc(cookedSize);
        result = Tcl_ExternalToUtf(interp, sinkPtr->encoding, 
                        (char *)raw, numRaw, 0, NULL, (char *)cooked, 
                        cookedSize, &numSrcCooked, &numCooked, NULL);
        numLeftOver = 0;
        if (result == TCL_CONVERT_MULTIBYTE) {
            /* 
             * Last multibyte sequence wasn't completed.  Save the extra
             * characters in a temporary buffer.
             */
            numLeftOver = (numRaw - numSrcCooked);
            srcPtr = sinkPtr->bytes + (sinkPtr->mark + numSrcCooked); 
            endPtr = srcPtr + numLeftOver;
            destPtr = leftover;
            while (srcPtr < endPtr) {
                *destPtr++ = *srcPtr++;
            }
        } 
        /*
         * Create a bigger sink.
         */
        needed = numLeftOver + numCooked;
        spaceLeft = sinkPtr->size - sinkPtr->mark;
        if (spaceLeft >= needed) {
            spaceLeft = ExtendSinkBuffer(sinkPtr);
            if (spaceLeft < 0) {
                errno = ENOMEM;
                sinkPtr->status = READ_ERROR;
                return TCL_ERROR;
            }
        }
        assert(spaceLeft > needed);
        /* 
         * Replace the characters from the mark with the translated
         * characters.
         */
        srcPtr = cooked;
        endPtr = cooked + numCooked;
        destPtr = sinkPtr->bytes + sinkPtr->mark;
        while (srcPtr < endPtr) {
            *destPtr++ = *srcPtr++;
        }
        /* Add the number of newly translated characters to the mark */
        sinkPtr->mark += numCooked;
        
        srcPtr = leftover;
        endPtr = leftover + numLeftOver;
        while (srcPtr < endPtr) {
            *destPtr++ = *srcPtr++;
        }
        sinkPtr->fill = sinkPtr->mark + numLeftOver;
#endif /* >= 8.1.0  */
    }
#ifdef WIN32
    crlf = TRUE;
#else
    crlf = (sinkPtr->bgPtr->flags & PTY);
#endif  /* WIN32 */
    /* 
     * Translate CRLF character sequences to LF characters.  We have to do
     * this after converting the string to UTF from UNICODE.
     */
    if ((crlf) && (sinkPtr->encoding != ENCODING_BINARY)) {
        size_t count;
        unsigned char *destPtr;

        destPtr = srcPtr = sinkPtr->bytes + oldMark;
        endPtr = sinkPtr->bytes + sinkPtr->fill;
        *endPtr = '\0';
        count = 0;
        for (endPtr--; srcPtr < endPtr; srcPtr++) {
            if ((*srcPtr == '\r') && (*(srcPtr + 1) == '\n')) {
                count++;
                continue;               /* Skip the CR in CR/LF sequences. */
            }
            if (srcPtr != destPtr) {
                *destPtr = *srcPtr;     /* Collapse the string, overwriting
                                         * the \r's encountered. */
            }
            destPtr++;
        }
        sinkPtr->mark -= count;
        sinkPtr->fill -= count;
        *destPtr = *srcPtr;             /* Copy the last byte */
        if (*destPtr == '\r') {
            sinkPtr->mark--;
        }
    }
    return TCL_OK;
}

#ifdef WIN32
/*
 *---------------------------------------------------------------------------
 *
 * WaitProcess --
 *
 *      Emulates the waitpid system call under the Win32 API.
 *
 * Results:
 *      Returns 0 if the process is still alive, -1 on an error, or the pid
 *      on a clean close.
 *
 * Side effects:
 *      Unless WNOHANG is set and the wait times out, the process
 *      information record will be deleted and the process handle will be
 *      closed.
 *
 *---------------------------------------------------------------------------
 */
static int
WaitProcess(Blt_Pid child, int *statusPtr, int flags)
{
    DWORD status, exitCode;
    int result;
    int timeout;

    *statusPtr = 0;
    if (child.hProcess == INVALID_HANDLE_VALUE) {
        errno = EINVAL;
        return -1;
    }
    timeout = (flags & WNOHANG) ? 0 : INFINITE;
    status = WaitForSingleObject(child.hProcess, timeout);
                                 
    switch (status) {
    case WAIT_FAILED:
        errno = ECHILD;
        *statusPtr = ECHILD;
        result = -1;
        break;

    case WAIT_TIMEOUT:
        if (timeout == 0) {
            return 0;                   /* Try again */
        }
        result = 0;
        break;

    default:
    case WAIT_ABANDONED:
    case WAIT_OBJECT_0:
        GetExitCodeProcess(child.hProcess, &exitCode);
        *statusPtr = ((exitCode << 8) & 0xff00);
        result = child.pid;
        assert(result != -1);
        break;
    }
    CloseHandle(child.hProcess);
    return result;
}

static BOOL CALLBACK
CloseMessageProc(HWND hWnd, LPARAM lParam)
{
    DWORD pid = 0;
    Blt_Pid *procPtr = (Blt_Pid *)lParam;

    GetWindowThreadProcessId(hWnd, &pid);
    if (pid == procPtr->pid) {
        PostMessage(hWnd, WM_CLOSE, 0, 0);
    }
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * KillProcess --
 *
 *      Emulates the UNIX kill system call under Win32 API.
 *
 * Results:
 *      Returns 0 if the process is killed, -1 on an error.
 *
 * Side effects:
 *      Process is terminated.
 *
 *---------------------------------------------------------------------------
 */
static int
KillProcess(Blt_Pid proc, int signal)
{
    DWORD status;

    if ((proc.hProcess == NULL) || (proc.hProcess == INVALID_HANDLE_VALUE)) {
        errno = EINVAL;
        return -1;
    }
#ifdef notdef
    if (GenerateConsoleCrtlEvent(CTRL_C_EVENT, proc.hProcess)) {
        Blt_Warn("can't send control event to process (handle=%d): %s\n",
                proc.hProcess, Blt_LastError());
        EnumWindows(CloseMessageProc, (LPARAM)&proc);
    }
#else
    EnumWindows(CloseMessageProc, (LPARAM)&proc);
#endif
    /* 
     * Wait on the handle. If it signals, great. If it times out, then call
     * TerminateProcess on it.
     *
     * On Windows 95/98 this also has the added benefit of stopping
     * KERNEL32.dll from dumping.  The two second delay is a guess (one
     * second seems to fail intermittently).
     */
    status = WaitForSingleObject(proc.hProcess, 2000);
    if (status == WAIT_OBJECT_0) {
        return 0;
    }
    if (!TerminateProcess(proc.hProcess, 1)) {
        return -1;
    }
    return 0;
}

#endif /* WIN32 */

#if (_TCL_VERSION < _VERSION(8,1,0)) 

static void
NotifyOnUpdate(Tcl_Interp *interp, Sink *sinkPtr, unsigned char *data, 
               int numBytes)
{
    char save;

    if (data[0] == '\0') {
        return;
    }
    save = data[numBytes];
#ifdef notdef
    data[numBytes] = '\0';
#endif
    if (sinkPtr->flags & SINK_ECHO) {
        Tcl_Channel channel;
        
        channel = Tcl_GetStdChannel(sinkPtr->channelNum);
        if (channel == NULL) {
            Tcl_AppendResult(interp, "can't get ", sinkPtr->name, " channel",
                (char *)NULL);
            Tcl_BackgroundError(interp);
            sinkPtr->flags &= ~SINK_ECHO;
        } else {
            Tcl_Write(channel, data, numBytes);
            if (save == '\n') {
                Tcl_Write(channel, "\n", 1);
            }
            Tcl_Flush(channel);
        }
    }
    objPtr = Tcl_NewByteArrayObj(data, (int)numBytes);
    Tcl_IncrRefCount(objPtr);
    if (sinkPtr->cmdObjPtr != NULL) {
        Tcl_Obj *cmdObjPtr, *objPtr;
        int result;

        cmdObjPtr = Tcl_DuplicateObj(sinkPtr->cmdObjPtr);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        Tcl_IncrRefCount(cmdObjPtr);
        result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(cmdObjPtr);
        if (result != TCL_OK) {
            Tcl_BackgroundError(interp);
        }
    }
    if (sinkPtr->updateVarObjPtr != NULL) {
        if (Tcl_ObjSetVar2(interp, sinkPtr->updateVarObjPtr, NULL /*part2*/,
                objPtr, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
            Tcl_BackgroundError(interp);
        }
    }
    Tcl_DecrRefCount(objPtr);
    data[numBytes] = save;
}

#else 

static void
NotifyOnUpdate(Tcl_Interp *interp, Sink *sinkPtr, unsigned char *data, 
               int numBytes)
{
    Tcl_Obj *objPtr;

    if ((numBytes == 0) || (data[0] == '\0')) {
        return;
    }
    if (sinkPtr->flags & SINK_ECHO) {
        Tcl_Channel channel;
        
        channel = Tcl_GetStdChannel(sinkPtr->channelNum);
        if (channel == NULL) {
            Tcl_AppendResult(interp, "can't get ", sinkPtr->name, " channel",
                (char *)NULL);
            Tcl_BackgroundError(interp);
            sinkPtr->flags &= ~SINK_ECHO;
        } else {
            if (data[numBytes] == '\n') {
                objPtr = Tcl_NewByteArrayObj(data, numBytes + 1);
            } else {
                objPtr = Tcl_NewByteArrayObj(data, numBytes);
            }
            Tcl_WriteObj(channel, objPtr);
            Tcl_Flush(channel);
        }
    }

    objPtr = Tcl_NewByteArrayObj(data, numBytes);
    Tcl_IncrRefCount(objPtr);
    if (sinkPtr->cmdObjPtr != NULL) {
        Tcl_Obj *cmdObjPtr;
        int result;
        
        cmdObjPtr = Tcl_DuplicateObj(sinkPtr->cmdObjPtr);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        Tcl_IncrRefCount(cmdObjPtr);
        result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(cmdObjPtr);
        if (result != TCL_OK) {
            Tcl_BackgroundError(interp);
        }
    }
    if (sinkPtr->updateVarObjPtr != NULL) {
        if (Tcl_ObjSetVar2(interp, sinkPtr->updateVarObjPtr, NULL /*part2*/,
                objPtr, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
            Tcl_BackgroundError(interp);
        }
    }
    Tcl_DecrRefCount(objPtr);
}

#endif /* < 8.1.0 */

static int
CollectData(Sink *sinkPtr)
{
    Bgexec *bgPtr;
    int result;
    
    bgPtr = sinkPtr->bgPtr;
    if (((bgPtr->flags & FOREGROUND) == 0) && (sinkPtr->doneVarObjPtr == NULL)){
        ResetSink(sinkPtr);
    }
    result = ReadBytes(sinkPtr);
    if (result == TCL_ERROR) {
        return TCL_ERROR;               /* Error reading */
    }
    if (CookSink(bgPtr->interp, sinkPtr) != TCL_OK) {
        return TCL_ERROR;               /* Error cooking data. */
    }
    if ((sinkPtr->mark > sinkPtr->lastMark) && (sinkPtr->flags & SINK_NOTIFY)) {
        if (bgPtr->flags & LINEBUFFERED) {
            int length;
            unsigned char *data;

            /* For line-by-line updates, call NotifyOnUpdate for each new
             * complete line.  */
            while ((data = NextLine(sinkPtr, &length)) != NULL) {
                NotifyOnUpdate(bgPtr->interp, sinkPtr, data, length);
            }
        } else {
            int length;
            unsigned char *data;

            length = 0;                 /* Suppress compiler warning. */
            data = NextBlock(sinkPtr, &length);
            NotifyOnUpdate(bgPtr->interp, sinkPtr, data, length);
        }
    }
    if (sinkPtr->status >= 0) {
        return TCL_OK;
    }
    if (sinkPtr->status == READ_ERROR) {
        Tcl_AppendResult(bgPtr->interp, "can't read data from ", sinkPtr->name,
            ": ", Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_RETURN;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateSinkHandler --
 *
 *      Creates a file handler for the given sink.  The file descriptor is
 *      also set for non-blocking I/O.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The memory allocated to the Bgexec structure released.
 *
 *---------------------------------------------------------------------------
 */
static int
CreateSinkHandler(Sink *sinkPtr, Tcl_FileProc *proc)
{
#ifndef WIN32
    int flags;

    flags = fcntl(sinkPtr->fd, F_GETFL);
#ifdef O_NONBLOCK
    flags |= O_NONBLOCK;
#else
    flags |= O_NDELAY;
#endif
    if (fcntl(sinkPtr->fd, F_SETFL, flags) < 0) {
        Tcl_AppendResult(sinkPtr->bgPtr->interp,
                "can't set file descriptor for sink \"", sinkPtr->name,
                "\" to non-blocking: ", Tcl_PosixError(sinkPtr->bgPtr->interp),
                (char *)NULL);
        return TCL_ERROR;
    }
#endif /* !WIN32 */
#ifdef WIN32
    Blt_CreateFileHandler(sinkPtr->fileHandle, TCL_READABLE, proc, sinkPtr);
#else
    Tcl_CreateFileHandler(sinkPtr->fd, TCL_READABLE, proc, sinkPtr);
#endif  /* WIN32 */
    return TCL_OK;
}

static void
DisableTriggers(Bgexec *bgPtr)          /* Background info record. */
{
    if (bgPtr->flags & TRACED) {
        Tcl_UntraceVar(bgPtr->interp, Tcl_GetString(bgPtr->statVarObjPtr),
                TRACE_FLAGS, VariableProc, bgPtr);
        bgPtr->flags &= ~TRACED;
    }
    if (SINKOPEN(&bgPtr->outSink)) {
        CloseSink(&bgPtr->outSink);
    }
    if (SINKOPEN(&bgPtr->errSink)) {
        CloseSink(&bgPtr->errSink);
    }
    if (bgPtr->timerToken != (Tcl_TimerToken) 0) {
        Tcl_DeleteTimerHandler(bgPtr->timerToken);
        bgPtr->timerToken = 0;
    }
    if (bgPtr->donePtr != NULL) {
        *bgPtr->donePtr = TRUE;
    }
}

static int
SetErrorCode(Tcl_Interp *interp, int lastPid, WAIT_STATUS_TYPE lastStatus,
             Tcl_Obj *listObjPtr)
{
    Tcl_Obj *objPtr;
    int code;
    enum PROCESS_STATUS { 
        PROCESS_EXITED, PROCESS_STOPPED, PROCESS_KILLED, PROCESS_UNKNOWN
    } pcode;
    static const char *tokens[] = { 
        "EXITED", "KILLED", "STOPPED", "UNKNOWN"
    };
    const char *mesg;
    char string[200];

    /*
     * All child processes have completed.  Set the status variable with
     * the status of the last process reaped.  The status is a list of an
     * error token, the exit status, and a message.
     */
    code = WEXITSTATUS(lastStatus);
    mesg = NULL;                        /* Suppress compiler warning.  */
    if (WIFEXITED(lastStatus)) {
        pcode = PROCESS_EXITED;
    } else if (WIFSIGNALED(lastStatus)) {
        pcode = PROCESS_KILLED;
        code = -1;
    } else if (WIFSTOPPED(lastStatus)) {
        pcode = PROCESS_STOPPED;
        code = -1;
    } else {
        pcode = PROCESS_UNKNOWN;
    }
    /* Token */
    objPtr = Tcl_NewStringObj(tokens[pcode], -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    /* Pid */
    objPtr = Tcl_NewLongObj(lastPid);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    /* Exit code. */
    objPtr = Tcl_NewIntObj(code);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    switch(pcode) {
    case PROCESS_EXITED:
        mesg = "child completed normally";
        break;
    case PROCESS_KILLED:
        mesg = Tcl_SignalMsg((int)(WTERMSIG(lastStatus)));
        break;
    case PROCESS_STOPPED:
        mesg = Tcl_SignalMsg((int)(WSTOPSIG(lastStatus)));
        break;
    case PROCESS_UNKNOWN:
        Blt_FormatString(string, 200,
                         "child completed with unknown status 0x%x",
                         *((int *)&lastStatus));
        mesg = string;
        break;
    }
    /* Message */
    objPtr = Tcl_NewStringObj(mesg, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    Tcl_SetObjErrorCode(interp, listObjPtr);
    return code;
}

static Tcl_Obj *
CheckPipeline(Bgexec *bgPtr)
{
    Tcl_Obj *listObjPtr;
    Tcl_Interp *interp;
    WAIT_STATUS_TYPE waitStatus, lastStatus;
    int code;
    int i;
    int numPidsLeft;                    /* # of processes still not
                                         * reaped. */
    unsigned int lastPid;

    interp = bgPtr->interp;
    lastPid = (unsigned int)-1;
    *((int *)&waitStatus) = 0;
    *((int *)&lastStatus) = 0;

    numPidsLeft = 0;
    for (i = 0; i < bgPtr->numPids; i++) {
        int pid;

#ifdef WIN32
        pid = WaitProcess(bgPtr->pids[i], (int *)&waitStatus, WNOHANG);
#else
        pid = waitpid(bgPtr->pids[i].pid, (int *)&waitStatus, WNOHANG);
#endif  /* WIN32 */
        if (pid == 0) {                 /* Process has not terminated yet */
            if (numPidsLeft < i) {
                bgPtr->pids[numPidsLeft] = bgPtr->pids[i];
            }
            numPidsLeft++;              /* Count the # of processes left */
        } else if (pid != -1) {
            /*
             * Save the status information associated with the subprocess.
             * We'll use it only if this is the last subprocess to be
             * reaped.
             */
            lastStatus = waitStatus;
            lastPid = (unsigned int)pid;
        }
    }
    bgPtr->numPids = numPidsLeft;
    if (numPidsLeft > 0) {
        return NULL;
    }

    /*
     * All child processes have completed.  Set the error code with the
     * status of the last process reaped.  The status is a list of an error
     * token, the exit status, and a message.
     */
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    code = SetErrorCode(interp, lastPid, lastStatus, listObjPtr);
    if (bgPtr->exitCodePtr != NULL) {
        *bgPtr->exitCodePtr = code;
    }
    return listObjPtr;
}

static void
ReportPipeline(Tcl_Interp *interp, Bgexec *bgPtr)
{
    Tcl_Obj *listObjPtr;
    int i;
    
#ifdef notdef
    fprintf(stderr, "ReportPipeline\n");
#endif
    /* If backgrounded, return a list of the child process ids instead of
     * the output of the pipeline. */
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (i = 0; i < bgPtr->numPids; i++) {
        Tcl_Obj *objPtr;
#ifdef WIN32
        objPtr = Tcl_NewLongObj((unsigned long)bgPtr->pids[i].pid);
#else 
        objPtr = Tcl_NewLongObj(bgPtr->pids[i].pid);
#endif  /* WIN32 */
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
}



static int
ExecutePipeline(Tcl_Interp *interp, Bgexec *bgPtr, int objc,
                Tcl_Obj *const *objv)
{
    void *outFdPtr, *errFdPtr;
    int numPids;
    Blt_Pid *pids;                      /* Array of process Ids. */

    outFdPtr = errFdPtr = NULL;
#ifdef WIN32
    if ((bgPtr->flags & FOREGROUND) ||
        (bgPtr->outSink.doneVarObjPtr != NULL) || 
        (bgPtr->outSink.updateVarObjPtr != NULL) ||
        (bgPtr->outSink.cmdObjPtr != NULL)) {
        outFdPtr = &bgPtr->outSink.fileHandle;
    }
#else
    outFdPtr = &bgPtr->outSink.fd;
#endif  /* WIN32 */
    if ((bgPtr->errSink.doneVarObjPtr != NULL) ||
        (bgPtr->errSink.updateVarObjPtr != NULL) ||
        (bgPtr->errSink.cmdObjPtr != NULL) ||
        (bgPtr->errSink.flags & SINK_ECHO)) {
#ifdef WIN32
        errFdPtr = &bgPtr->errSink.fileHandle;
#else
        errFdPtr = &bgPtr->errSink.fd;
#endif
    }
    numPids = Blt_CreatePipeline(interp, objc, objv, &pids, 
        NULL, outFdPtr, errFdPtr, bgPtr->env);
    if (numPids < 0) {
        return TCL_ERROR;
    }
    bgPtr->pids = pids;
    bgPtr->numPids = numPids;
    if (bgPtr->outSink.fd == -1) {
        /* 
         * If output has been redirected, start polling immediately for the
         * exit status of each process.  Normally, this is done only after
         * stdout has been closed by the last process, but here stdout has
         * been redirected. The default polling interval is every 1 second.
         */
        bgPtr->timerToken = Tcl_CreateTimerHandler(bgPtr->interval, TimerProc,
           bgPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * KillPipeline --
 *
 *      Cleans up background execution processes and memory.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The memory allocated to the Bgexec structure released.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
KillPipeline(Bgexec *bgPtr)            /* Background info record. */
{
    if (bgPtr->pids != NULL) {
        int i;

        for (i = 0; i < bgPtr->numPids; i++) {
            if (bgPtr->signalNum > 0) {
#ifdef WIN32
                kill(bgPtr->pids[i], bgPtr->signalNum);
#else
                kill(bgPtr->pids[i].pid, bgPtr->signalNum);
#endif
            }
        }
        Blt_DetachPids(bgPtr->numPids, bgPtr->pids);
    }
    Tcl_ReapDetachedProcs();
}

#ifdef HAVE_SESSION

static Tcl_Obj *
CheckSession(Bgexec *bgPtr)
{
    Tcl_Obj *listObjPtr;
    Tcl_Interp *interp;
    WAIT_STATUS_TYPE waitStatus;
    int code;
    int pid;

    interp = bgPtr->interp;
    *((int *)&waitStatus) = 0;
    pid = waitpid(bgPtr->sid, (int *)&waitStatus, WNOHANG);
    if (pid == 0) {                     /* Process has not terminated yet */
        return NULL;
    }
    /*
     * All child processes have completed.  Set the error code with the
     * status of the last process reaped.  The status is a list of an error
     * token, the exit status, and a message.
     */
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    code = SetErrorCode(interp, pid, waitStatus, listObjPtr);
    if (bgPtr->exitCodePtr != NULL) {
        *bgPtr->exitCodePtr = code;
    }
    return listObjPtr;
}

static void
ReportSession(Tcl_Interp *interp, Bgexec *bgPtr)
{
    /* If the pipeline is backgrounded, return the process id of the
     * session leader instead of the output of the pipeline. */
    Tcl_SetLongObj(Tcl_GetObjResult(interp), bgPtr->sid);
}

/*
 *---------------------------------------------------------------------------
 *
 * CreatePipeWithCloseOnExec --
 *
 *      Creates a pipe by simply calling the pipe() function.  
 *
 * Results:
 *      Returns 1 on success, 0 on failure.
 *
 * Side effects:
 *      Creates a pipe.
 *
 *---------------------------------------------------------------------------
 */
static int
CreatePipeWithCloseOnExec(Tcl_Interp *interp, int *pipefds)
{
    if (pipe(pipefds) < 0) {
        ExplainError(interp, "can't create pipe");
        return TCL_ERROR;
    }
    if (fcntl(pipefds[0], F_SETFD, FD_CLOEXEC) == -1) {
        ExplainError(interp, "can't change to FD_CLOEXEC");
    }
    if (fcntl(pipefds[1], F_SETFD, FD_CLOEXEC) == -1) {
        ExplainError(interp, "can't change to FD_CLOEXEC");
    }
    return TCL_OK;
}

static int
ExecutePipelineWithSession(Tcl_Interp *interp, Bgexec *bgPtr, int objc,
                           Tcl_Obj *const *objv)
{
    Blt_Pid *pids;                      /* Array of process Ids. */
    WAIT_STATUS_TYPE waitStatus, lastStatus;
    int numPids;
    int stdoutPipe[2], stderrPipe[2], mesgPipe[2];
    pid_t child;

    stdoutPipe[0] = stdoutPipe[1] = stderrPipe[0] = stderrPipe[1] = -1;
    /*
     * Create a pipe that the child can use to return error information if
     * anything goes wrong in creating the pipeline.
     */
    if (pipe(stdoutPipe) == -1) {
        ExplainError(interp, "can't create stdout pipe");
        return TCL_ERROR;
    }
    if (pipe(stderrPipe) == -1) {
        ExplainError(interp, "can't create stderr pipe");
        return TCL_ERROR;
    }
    if (CreatePipeWithCloseOnExec(interp, mesgPipe) != TCL_OK) {
        goto cleanup;
    }
    bgPtr->outSink.fd = stdoutPipe[0];
    bgPtr->errSink.fd = stderrPipe[0];
    child = fork();

    if (child == -1) {
        ExplainError(interp, "fork");
        goto cleanup;
    }
    if (child != 0) {                   /* Parent */
        /*
         * Read back from the error pipe to see if the pipeline started up
         * properly. The info in the pipe (if any) if the TCL error message
         * from the child process.
         */
        close(stdoutPipe[1]);
        close(stderrPipe[1]);
        close(mesgPipe[1]);
        bgPtr->numPids = 1;
        bgPtr->pids = NULL;
        bgPtr->sid = child;

        if (ReadErrorMesgFromChild(interp, mesgPipe[0]) == TCL_OK) {
            return TCL_OK;
        }
    cleanup:
        if (stdoutPipe[0] >= 0) {
            close(stdoutPipe[0]);
        }
        if (stdoutPipe[1] >= 0) {
            close(stdoutPipe[1]);
        }
        if (stderrPipe[0] >= 0) {
            close(stderrPipe[0]);
        }
        if (stderrPipe[1] >= 0) {
            close(stderrPipe[1]);
        }
        return TCL_ERROR;
    }

    /* Child process */
    close(mesgPipe[0]);
    close(stdoutPipe[0]);
    close(stderrPipe[0]);
    if (setsid() == -1) {
        ExplainError(interp, "setsid");
        goto error;
    }
    if (dup2(stdoutPipe[1], STDOUT_FILENO) == -1) {
        ExplainError(interp, "can't dup stdout");
        goto error;
    }
    if (dup2(stderrPipe[1], STDERR_FILENO) == -1) {
        ExplainError(interp, "can't dup stderr");
        goto error;
    }
    /* Always collect characters from the sinks. Display to the screen or
     * not. */
    numPids = Blt_CreatePipeline(interp, objc, objv, &pids, NULL, NULL, NULL,
          bgPtr->env);
    if (numPids <= 0) {
        goto error;
    }
    close(mesgPipe[1]);                 /* This frees the parent to start
                                         * collecting data. */
    *((int *)&waitStatus) = 0;
    *((int *)&lastStatus) = 0;
    while (numPids > 0) {
        int pid;

        pid = waitpid(0, (int *)&waitStatus, 0);
        if (pid < 0) {
            fprintf(stderr, "waitpid: %s\n", Tcl_PosixError(interp));
            continue;
        }
        /*
         * Save the status information associated with the subprocess.
         * We'll use it only if this is the last subprocess to be reaped.
         */
        lastStatus = waitStatus;
        --numPids;
    }
    Blt_Free(pids);
    /*
     * All pipeline processes have completed.  Exit the child replicating the
     * exit code.
     */
    exit(WEXITSTATUS(lastStatus));
    /* notreached */
    return TCL_OK;
 error:
    WriteErrorMesgToParent(interp, mesgPipe[1]);
    exit(1);
    /* notreached */
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * KillSession --
 *
 *      Cleans up background execution processes and memory.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The memory allocated to the Bgexec structure released.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
KillSession(Bgexec *bgPtr)            /* Background info record. */
{
    Blt_Pid pid;

    if (bgPtr->master != -1) {
        close(bgPtr->master);
        bgPtr->master = -1;
    }
    if ((bgPtr->numPids > 0) && (bgPtr->signalNum > 0)) {
        kill(-bgPtr->sid, bgPtr->signalNum);
    }
    pid.pid = bgPtr->sid;
    Blt_DetachPids(1, &pid);
    Tcl_ReapDetachedProcs();
}

#endif /*HAVE_SESSION*/

#ifdef HAVE_PTY
#ifdef HAVE_GETPT

static int 
AcquirePtyMaster(Bgexec *bgPtr)
{
    int master;

    master = getpt();
    if (master < 0) {
        ExplainError(bgPtr->interp, "getpt");
        return TCL_ERROR;
    }
    bgPtr->masterName[0] = '\0';
    bgPtr->master = master;
    return TCL_OK;
}

#elif defined(HAVE_POSIX_OPENPT)

static int 
AcquirePtyMaster(Bgexec *bgPtr)
{
    int master;

    master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0) {
        ExplainError(bgPtr->interp, "openpt");
        return TCL_ERROR;
    }
    bgPtr->masterName[0] = '\0';
    bgPtr->master = master;
    return TCL_OK;
}

#else 

static int 
AcquirePtyMaster(Bgexec *bgPtr)
{
    int master;
    const char *p;
    char ptyName[11];

    strcpy(bgPtr->masterName, "/dev/ptmx");
    master = open(bgPtr->masterName, O_RDWR | O_NOCTTY);
    if (master >= 0) {
        bgPtr->master = master;
        return TCL_OK;
    }
    strcpy(ptyName, "/dev/ptyXY");
    for (p = PTYRANGE0; *p != '\0'; p++) {
        const char *q;

        ptyName[8] = *p;
        for (q = PTYRANGE1; *q != '\0'; q++) {
            int master;

            ptyName[9] = *q;
            master = open(ptyName, O_RDWR);
            if (master >= 0) {
                bgPtr->master = master;
                strcpy(bgPtr->masterName, ptyName);
                return TCL_OK;
            }
            if (errno == E_NOENT) {
                break;
            } 
        }
    }
    Tcl_AppendResult(bgPtr->interp, "can't access any ptys", (char *)NULL);
    return TCL_ERROR;
}
#endif  /* HAVE_POSIX_OPENPT */

static int
InitPtyMaster(Bgexec *bgPtr) 
{
#ifdef HAVE_TCFLUSH
    if (tcflush(bgPtr->master, TCIOFLUSH) < 0) {
        ExplainError(bgPtr->interp, "tcflush");
        return TCL_ERROR;
    }
#else
#ifdef TIOCFLUSH
    if (ioctl(bgPtr->master, TIOCFLUSH, NULL) == -1) {
        ExplainError(bgPtr->interp, "can't set TIOCFLUSH on master");
        return TCL_ERROR;
    }
#endif  /* TIOCFLUSH */
#endif  /* HAVE_TCFLUSH */
    {    
    struct termios tt;

    if (tcgetattr(STDIN_FILENO, &tt) == -1) {
        ExplainError(bgPtr->interp, "master tcgetattr");
        return TCL_ERROR;
    }
    /* tt.c_lflag |= ICANON; */
    tt.c_iflag = ~(INLCR | ICRNL | ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    tt.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    /* tt.c_iflag |= ICRNL;  */
    if (tcsetattr(bgPtr->master, TCSANOW, &tt) == -1) {
        ExplainError(bgPtr->interp, "tcsetattr");
        return TCL_ERROR;
    }
    }
#ifdef TIOCEXCL
    if (ioctl(bgPtr->master, TIOCEXCL, NULL) == -1) {
        ExplainError(bgPtr->interp, "can't get exclusive access to terminal");
        return TCL_ERROR;
    }
#endif  /* TIOCEXCL */
    return TCL_OK;
}

#ifdef HAVE_OPEN_CONTROLLING_PTY
static int
OpenPtyMaster(Tcl_Interp *interp, Bgexec *bgPtr)
{
    char slaveName[PTY_NAME_SIZE+1];
    int master;

    master = open_controlling_pty(slaveName);
    if (master < 0) {
        ExplainError(interp, "can't open controlling pty");
        return TCL_ERROR;
    }
    bgPtr->master = master;
    strcpy(bgPtr->slaveName, slaveName);
    if (InitPtyMaster(bgPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK
}
#endif  /* HAVE_OPEN_CONTROLLING_PTY */

#ifdef HAVE_GRANTPT
static int 
OpenPtyMaster(Tcl_Interp *interp, Bgexec *bgPtr)
{
    const char *slaveName;

    if (AcquirePtyMaster(bgPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (grantpt(bgPtr->master) < 0) {
        ExplainError(interp, "grantpt");
        return TCL_ERROR;
    }
    if (unlockpt(bgPtr->master) < 0) {
        ExplainError(interp, "unlockpt");
        return TCL_ERROR;
    }
    slaveName = ptsname(bgPtr->master);
    if (slaveName == NULL) {
        ExplainError(interp, "ptsname");
        return TCL_ERROR;
    }
    strcpy(bgPtr->slaveName, slaveName);
    if (InitPtyMaster(bgPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}
#else

#ifdef HAVE_OPENPTY
#include <util.h>
static int
OpenPtyMaster(Tcl_Interp *interp, Bgexec *bgPtr)
{
    if (openpty(&bgPtr->master, &bgPtr->slave, NULL, NULL, NULL) < 0) {
        ExplainError(interp, "openpty");
        return TCL_ERROR;
    }
    if (InitPtyMaster(bgPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}
#endif  /* HAVE_OPENPT */
#endif  /* HAVE_GRANTPT */

static int
OpenPtySlave(Tcl_Interp *interp, Bgexec *bgPtr) 
{
    int slave;
    struct termios tt;

    bgPtr->slave = slave = open(bgPtr->slaveName, O_RDWR | O_NOCTTY);
    if (slave == -1) {
        Tcl_AppendResult(interp, "can't open \"", bgPtr->slaveName, 
                "\": ", Tcl_PosixError(interp), (char *)NULL);
        return TCL_ERROR;
    }
#ifdef TIOCSCTTY
    if (ioctl(slave, TIOCSCTTY, slave) == -1) {
        Tcl_AppendResult(interp, "can't make \"", bgPtr->slaveName,
                "\" controlling terminal : ", Tcl_PosixError(interp),
                (char *)NULL);
        return TCL_ERROR;
    }
#endif  /* TIOCSCTTY */
#ifdef I_PUSH
    if (isastream(slave)) {
        if (ioctl(slave, I_PUSH, "p_tem") == -1) {
            ExplainError(interp, "can't set ioctl \"p_tem\"");
            return TCL_ERROR;
        }
        if (ioctl(slave, I_PUSH, "ldterm") == -1) {
            ExplainError(interp, "can't set ioctl \"ldterm\"");
            return TCL_ERROR;
        }
    }
#endif
    if (tcgetattr(0, &tt) == -1) {
        ExplainError(interp, "tcgetattr on slave");
        return TCL_ERROR;
    }
    tt.c_oflag = 0;
    tt.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    if (tcsetattr(slave, TCSANOW, &tt) == -1) {
        ExplainError(interp, "tcsetattr on slave");
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
ExecutePipelineWithPty(Tcl_Interp *interp, Bgexec *bgPtr, int objc,
                       Tcl_Obj *const *objv)
{
    WAIT_STATUS_TYPE waitStatus, lastStatus;
    int numPids;                               /* # of processes. */
    pid_t child;
    int mesgPipe[2], stderrPipe[2];
    Blt_Pid *pids;

    mesgPipe[0] = mesgPipe[1] = stderrPipe[0] = stderrPipe[1] = -1;
    /*
     * Create a pipe that the child can use to return error information if
     * anything goes wrong in creating the pipeline.
     */

    /* Open a pseudo terminal for the pipeline. */
    if (OpenPtyMaster(interp, bgPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (pipe(stderrPipe) == -1) {
        ExplainError(interp, "can't create stderr pipe");
        return TCL_ERROR;
    }
    if (CreatePipeWithCloseOnExec(interp, mesgPipe) != TCL_OK) {
        return TCL_ERROR;
    }
    bgPtr->outSink.fd = bgPtr->master;
    bgPtr->errSink.fd = stderrPipe[0];
    child = fork();

    if (child == -1) {
        ExplainError(interp, "fork");
        return TCL_ERROR;
    }
    if (child != 0) {                   /* Parent */
        close(mesgPipe[1]);
        close(stderrPipe[1]);
        bgPtr->numPids = 1;
        bgPtr->pids = NULL;
        bgPtr->sid = child;
        return ReadErrorMesgFromChild(bgPtr->interp, mesgPipe[0]);
    }

    /* Child process */
    close(mesgPipe[0]);
    close(stderrPipe[0]);
    if (setsid() == -1) {
        ExplainError(interp, "setsid");
        goto error;
    }
    if (OpenPtySlave(interp, bgPtr) != TCL_OK) {
        goto error;
    }
    /* Dup descriptors to slave. */
    if (dup2(bgPtr->slave, STDIN_FILENO) == -1) {
        ExplainError(interp, "can't dup stdin");
        goto error;
    }
    if (dup2(bgPtr->slave, STDOUT_FILENO) == -1) {
        ExplainError(interp, "can't dup stdout");
        goto error;
    }
    if (dup2(stderrPipe[1], STDERR_FILENO) == -1) {
        ExplainError(interp, "can't dup stderr");
        goto error;
    }
    /*
     * Must clear the close-on-exec flag for the target FD, since some
     * systems (e.g. Ultrix) do not clear the CLOEXEC flag on the
     * target FD.
     */
    fcntl(STDIN_FILENO,  F_SETFD, 0);
    fcntl(STDOUT_FILENO, F_SETFD, 0);
    fcntl(STDERR_FILENO, F_SETFD, 0);

    close(bgPtr->master);
    bgPtr->master = -1;

    /* Always collect characters from the sinks. Display to the screen or
     * not. */
    numPids = Blt_CreatePipeline(interp, objc, objv, &pids, NULL, NULL, NULL,
                bgPtr->env);
    if (numPids <= 0) {
        goto error;
    }
    close(mesgPipe[1]);                 /* This frees the parent to start
                                         * collecting data. */
    *((int *)&waitStatus) = 0;
    *((int *)&lastStatus) = 0;
    while (numPids > 0) {
        int pid;

        pid = waitpid(0, (int *)&waitStatus, 0);
        if (pid == -1) {
            fprintf(stderr, "waitpid: %s\n", Tcl_PosixError(interp));
            continue;
        }
        /*
         * Save the status information associated with the subprocess.
         * We'll use it only if this is the last subprocess to be reaped.
         */
        lastStatus = waitStatus;
        --numPids;
    }
    Blt_Free(pids);
    /*
     * All pipeline processes have completed.  Exit the child replicating the
     * exit code.
     */
    close(bgPtr->slave);
    exit(WEXITSTATUS(lastStatus));
    return TCL_OK;
 error:
    WriteErrorMesgToParent(interp, mesgPipe[1]);
    exit(1);
    /* notreached */
    return TCL_OK;
}
#endif /* HAVE_PTY */

/*
 *---------------------------------------------------------------------------
 *
 * FreeBgexec --
 *
 *      Releases the memory allocated for the backgrounded process.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeBgexec(Bgexec *bgPtr)
{
    Blt_FreeSwitches(switchSpecs, (char *)bgPtr, 0);
    if (bgPtr->statVarObjPtr != NULL) {
        Tcl_DecrRefCount(bgPtr->statVarObjPtr);
    }
    if (bgPtr->pids != NULL) {
        Blt_Free(bgPtr->pids);
    }
    if (bgPtr->env != NULL) {
        Blt_Free(bgPtr->env);
    }
    if (bgPtr->link != NULL) {
        Tcl_MutexLock(mutexPtr);
        Blt_Chain_DeleteLink(activePipelines, bgPtr->link);
        Tcl_MutexUnlock(mutexPtr);
    }
    Blt_Free(bgPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * DestroyBgexec --
 *
 *      Cleans up background execution processes and memory.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The memory allocated to the Bgexec structure released.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
DestroyBgexec(Bgexec *bgPtr)            /* Background info record. */
{
#ifdef notdef
    fprintf(stderr, "DestroyBgExec\n");
#endif
    DisableTriggers(bgPtr);
    FreeSinkBuffer(&bgPtr->errSink);
    FreeSinkBuffer(&bgPtr->outSink);
    /* Kill all remaining processes. */
    if (bgPtr->classPtr != NULL) {
        (*bgPtr->classPtr->killProc)(bgPtr);
    }
    FreeBgexec(bgPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * VariableProc --
 *
 *      Kills all currently running pipeline of subprocesses (given the
 *      specified signal). This procedure is called when the user sets the
 *      status variable associated with this pipeline.
 *
 * Results:
 *      Always returns NULL.  Only called from a variable trace.
 *
 * Side effects:
 *      The subprocesses are signaled for termination using the specified
 *      kill signal.  Additionally, any resources allocated to track the
 *      subprocesses is released.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static char *
VariableProc(ClientData clientData, Tcl_Interp *interp, const char *part1,
             const char *part2, int flags)
{
    if (flags & TRACE_FLAGS) {
        Bgexec *bgPtr = clientData;

        DisableTriggers(bgPtr);
        /* Kill all child processes that remain. */
        (*bgPtr->classPtr->killProc)(bgPtr);
        if ((bgPtr->flags & FOREGROUND) == 0) {
            DestroyBgexec(bgPtr);
        }
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * TimerProc --
 *
 *      This is a timer handler procedure which gets called periodically to
 *      reap any of the sub-processes if they have terminated.  After the
 *      last process has terminated, the contents of standard output are
 *      stored in the output variable, which triggers the cleanup proc
 *      (using a variable trace). The status the last process to exit is
 *      written to the status variable.
 *
 * Results:
 *      None.  Called from the TCL event loop.
 *
 * Side effects:
 *      Many. The contents of pids is shifted, leaving only those
 *      sub-processes which have not yet terminated.  If there are still
 *      subprocesses left, this procedure is placed in the timer queue
 *      again. Otherwise the output and possibly the status variables are
 *      updated.  The former triggers the cleanup routine which will
 *      destroy the information and resources associated with these
 *      background processes.
 *
 *---------------------------------------------------------------------------
 */
static void
TimerProc(ClientData clientData)
{
    Bgexec *bgPtr = clientData;
    Tcl_Obj *listObjPtr;

    /* Check to see what processes remain. */
    listObjPtr = (*bgPtr->classPtr->checkProc)(bgPtr);
    if (listObjPtr != NULL) {
        Tcl_Interp *interp;

        DisableTriggers(bgPtr);

        /* It's OK to set the status variable, the variable trace was
         * disabled above. */
        interp = bgPtr->interp;
        if (Tcl_ObjSetVar2(interp, bgPtr->statVarObjPtr, /*part2*/NULL,
                listObjPtr, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
            Tcl_BackgroundError(interp);
        }
        if ((bgPtr->flags & FOREGROUND) == 0) {
            DestroyBgexec(bgPtr);
        }
    } else {
        bgPtr->timerToken = Tcl_CreateTimerHandler(bgPtr->interval, TimerProc,
           bgPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CollectStdout --
 *
 *      This procedure is called when stdout data from the pipeline is
 *      available.  The output is read and saved in a buffer in the Bgexec
 *      structure.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Data is stored in the buffer.  This character array may be
 *      increased as more space is required to contain the output of the
 *      pipeline.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
CollectStdout(ClientData clientData, int mask)
{
    Sink *sinkPtr = clientData;
    Bgexec *bgPtr;
    int result;
    
    result = CollectData(sinkPtr);
    if (result == TCL_OK) {
        return;
    }
    /*
     * Either EOF or an error has occurred.  In either case, close the
     * sink. Note that closing the sink will also remove the file handler,
     * so this routine will not be called again.
     */
    CloseSink(sinkPtr);
    /*
     * If both sinks (stdout and stderr) are closed, this doesn't
     * necessarily mean that the process has terminated.  Set up a timer
     * handler to periodically poll for the exit status of each process.
     * Initially check at the next idle interval.
     */
    bgPtr = sinkPtr->bgPtr;
    if (result == TCL_ERROR) {
        CloseSink(&bgPtr->errSink);
    }
    if (bgPtr->errSink.fd == -1) {
        bgPtr->timerToken = Tcl_CreateTimerHandler(0, TimerProc, bgPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CollectStderr --
 *
 *      This procedure is called when stderr data from the pipeline is
 *      available.  The error is read and saved in a buffer in the Bgexec
 *      structure.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Data is stored in the buffer.  This character array may be
 *      increased as more space is required to contain the stderr of the
 *      pipeline.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
CollectStderr(ClientData clientData, int mask)
{
    Sink *sinkPtr = clientData;
    Bgexec *bgPtr;
    int result;
    
    result = CollectData(sinkPtr);
    if (result == TCL_OK) {
        return;
    }
    /*
     * Either EOF or an error has occurred.  In either case, close the
     * sink. Note that closing the sink will also remove the file handler,
     * so this routine will not be called again.
     */
    CloseSink(sinkPtr);

    /*
     * If both sinks (stdout and stderr) are closed, this doesn't
     * necessarily mean that the process has terminated.  Set up a timer
     * handler to periodically poll for the exit status of each process.
     * Initially check at the next idle interval.
     */
    bgPtr = sinkPtr->bgPtr;
    if (result == TCL_ERROR) {
        CloseSink(&bgPtr->outSink);
    }
    if (bgPtr->outSink.fd == -1) {
        bgPtr->timerToken = Tcl_CreateTimerHandler(0, TimerProc, bgPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * BgexecCmdProc --
 *
 *      This procedure is invoked to process the "bgexec" TCL command.  See
 *      the user documentation for details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
BgexecCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    Bgexec *bgPtr;
    char *lastArg;
    int isBg;
    int i;

    if (objc < 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " varName ?options? command ?arg...?\"",
                (char *)NULL);
        return TCL_ERROR;
    }
    /* Check if the command line is to be run in the background (the last
     * argument is "&") */
    lastArg = Tcl_GetString(objv[objc - 1]);
    isBg = ((lastArg[0] == '&') && (lastArg[1] == '\0'));
    if (isBg) {
        objc--;                         /* Remove the '&' argument */
    }
    bgPtr = Blt_AssertCalloc(1, sizeof(Bgexec));
    /* Initialize the background information record */
    bgPtr->interp = interp;
    bgPtr->signalNum = SIGTERM;
    bgPtr->numPids = -1;
    bgPtr->interval = 1000;             /* Poll every 1 second. */
    bgPtr->flags |= FOREGROUND;
    if (isBg) {
        bgPtr->flags &= ~FOREGROUND;
    }
#ifdef HAVE_PTY
    bgPtr->master = bgPtr->slave = -1;
    bgPtr->sid = -1;
#endif
    bgPtr->statVarObjPtr = objv[1];
    Tcl_IncrRefCount(bgPtr->statVarObjPtr);
    Tcl_MutexLock(mutexPtr);
    bgPtr->link = Blt_Chain_Append(activePipelines, bgPtr);
    Tcl_MutexUnlock(mutexPtr);
    InitSink(bgPtr, &bgPtr->outSink, "stdout", TCL_STDOUT);
    InitSink(bgPtr, &bgPtr->errSink, "stderr", TCL_STDERR);

    /* Try to clean up any detached processes */
    Tcl_ReapDetachedProcs();
    i = Blt_ParseSwitches(interp, switchSpecs, objc - 2, objv + 2, bgPtr, 
                BLT_SWITCH_OBJV_PARTIAL);
    if (i < 0) {
        DestroyBgexec(bgPtr);
        return TCL_ERROR;
    }
    bgPtr->classPtr = &pipelineClass;
#ifdef HAVE_SESSION
    if (bgPtr->flags & SESSION) {
        bgPtr->classPtr = &sessionClass;
    } 
#endif  /* HAVE_SESSION */
#ifdef HAVE_PTY
    if (bgPtr->flags & PTY) {
        bgPtr->classPtr = &ptyClass;
    } 
#endif  /* HAVE_PTY */
    i += 2;
    /* Must be at least one argument left as the command to execute. */
    if (objc <= i) {
        Tcl_AppendResult(interp, "missing command to execute: should be \"",
            Tcl_GetString(objv[0]), " varName ?options? command ?arg...?\"", 
                (char *)NULL);
        DestroyBgexec(bgPtr);
        return TCL_ERROR;
    }

    ConfigureSinks(bgPtr);

    /* Put a trace on the exit status variable.  The will also allow the
     * user to terminate the pipeline by setting the variable.  */
    Tcl_TraceVar(interp, Tcl_GetString(bgPtr->statVarObjPtr), TRACE_FLAGS,
        VariableProc, bgPtr);
    bgPtr->flags |= TRACED;

    if ((*bgPtr->classPtr->execProc)(interp, bgPtr, objc-i, objv+i) != TCL_OK) {
        goto error;
    }
    if (bgPtr->outSink.fd >= 0) {
        if (CreateSinkHandler(&bgPtr->outSink, CollectStdout) != TCL_OK) {
            goto error;
        }
    }
    if (bgPtr->errSink.fd >= 0) {
        if (CreateSinkHandler(&bgPtr->errSink, CollectStderr) != TCL_OK) {
            goto error;
        }
    }
    if ((bgPtr->errSink.fd == -1) && (bgPtr->outSink.fd == -1)) {
        /* We're not reading from either stderr and stdout, so start
         * polling for the pipeline completion.. */
        bgPtr->timerToken = Tcl_CreateTimerHandler(bgPtr->interval, 
                TimerProc, bgPtr);
    }

    if (bgPtr->flags & FOREGROUND) {   
        int exitCode;
        int done;

        bgPtr->exitCodePtr = &exitCode;
        bgPtr->donePtr = &done;

        exitCode = done = 0;
        while (!done) {
            Tcl_DoOneEvent(0);
            if ((bgPtr->outSink.status == READ_ERROR) ||
                (bgPtr->errSink.status == READ_ERROR)) { 
                goto error;
            }
        }
        DisableTriggers(bgPtr);
        if ((bgPtr->flags & IGNOREEXITCODE) || (exitCode == 0)) {
            if (bgPtr->outSink.doneVarObjPtr == NULL) {
                unsigned char *data;
                size_t length;
                
                /* Return the output of the pipeline. */
                GetSinkData(&bgPtr->outSink, &data, &length);
                assert(length <= UINT_MAX);
#if (_TCL_VERSION <  _VERSION(8,1,0)) 
                Tcl_SetObjResult(interp, Tcl_NewStringObj(data, length));
#else
                Tcl_SetObjResult(interp, Tcl_NewByteArrayObj(data, length));
#endif
            }
        } else {
            DestroyBgexec(bgPtr);
            Tcl_AppendResult(interp, "child process exited abnormally",
                (char *)NULL);
            return TCL_ERROR;
        }
        DestroyBgexec(bgPtr);
    } else {
        /* If backgrounded, return a list of the child process ids instead
         * of the output of the pipeline. */
        (*bgPtr->classPtr->reportProc)(interp, bgPtr);
    }
    return TCL_OK;
  error:
    DestroyBgexec(bgPtr);
    return TCL_ERROR;
}

static void
BgexecExitProc(ClientData clientData)
{
    Blt_ChainLink link, next;

    Tcl_MutexLock(mutexPtr);
    for (link = Blt_Chain_FirstLink(activePipelines); link != NULL; 
         link = next) {
        Bgexec *bgPtr;

        next = Blt_Chain_NextLink(link);
        bgPtr = Blt_Chain_GetValue(link);
        bgPtr->link = NULL;             
        if ((bgPtr->flags & DONTKILL) == 0) {
            /* Kill all remaining processes. */
            (*bgPtr->classPtr->killProc)(bgPtr);
        }
    }
    Blt_Chain_Destroy(activePipelines);
    Tcl_MutexUnlock(mutexPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_BgexecCmdInitProc --
 *
 *      This procedure is invoked to initialize the "bgexec" command.  See
 *      the user documentation for details on what it does.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_BgexecCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "bgexec", BgexecCmdProc, };

    if (activePipelines == NULL) {
#ifdef TCL_THREADS
        mutexPtr = Tcl_GetAllocMutex();
#endif
        activePipelines = Blt_Chain_Create();
        Tcl_CreateExitHandler(BgexecExitProc, activePipelines);
    }
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

#endif /* NO_BGEXEC */


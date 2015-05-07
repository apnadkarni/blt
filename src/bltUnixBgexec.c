/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltUnixBgexec.c --
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
 *
 */


#define _GNU_SOURCE
#define _XOPEN_SOURCE
#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"

#ifndef NO_BGEXEC

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif  /* HAVE_STDLIB_H */

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_SIGNAL_H
#  include <signal.h>
#endif  /* HAVE_SIGNAL_H */

#ifdef HAVE_IOCTL_H
#  include <ioctl.h>
#endif  /* HAVE_IOCTL_H */

#ifdef HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
#endif  /* HAVE_SYS_IOCTL_H */

#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif  /* HAVE_FCNTL_H */

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif  /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif  /* HAVE_STROPTS_H */

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#include <unistd.h>
#endif  /* HAVE_TERMIOS_H */

#include "bltAlloc.h"
#include "bltChain.h"
#include "bltWait.h"
#include "bltSwitch.h"
#include "bltInitCmd.h"

static Tcl_ObjCmdProc BgexecCmd;

#if (_TCL_VERSION <  _VERSION(8,1,0)) 
typedef void *Tcl_Encoding;     /* Make up dummy type for encoding.  */
#endif

#define ENCODING_ASCII          ((Tcl_Encoding)NULL)
#define ENCODING_BINARY         ((Tcl_Encoding)1)

#define BGEXEC_THREAD_KEY       "BLT Bgexec Data"

#define READ_AGAIN      (0)
#define READ_EOF        (-1)
#define READ_ERROR      (-2)

#define PTY_NAME_SIZE   32

/* The wait-related definitions are taken from tclUnix.h */

#define TRACE_FLAGS (TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY)

#define BLOCK_SIZE      1024            /* Size of allocation blocks for
                                         * buffer */
#define DEF_BUFFER_SIZE (BLOCK_SIZE * 8)
#define MAX_READS       100             /* Maximum # of successful reads
                                         * before stopping to let TCL catch up
                                         * on events */
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

static SignalToken signalTokens[] =
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
#endif
static Blt_Chain activePipelines;       /* List of active pipelines and
                                         * their ptyexec structures. */

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
    const char *name;                   /* Name of the sink. */
    const char *doneVar;                /* Name of a TCL variable (malloc'ed)
                                         * set to the collected data of the
                                         * last UNIX * subprocess. */
    const char *updateVar;              /* Name of a TCL variable (malloc'ed)
                                         * updated as data is read from the
                                         * pipe. */
    Tcl_Obj *cmdObjPtr;                 /* Start of a TCL command executed
                                         * whenever data is read from the
                                         * pipe. */
    int flags;                  
    Tcl_Encoding encoding;              /* Decoding scheme to use when
                                         * translating data. */
    int fd;                             /* File descriptor of the pipe. */
    int status;
    int echo;                           /* Indicates if the pipeline's stderr
                                         * stream should be echoed */

    unsigned char *bytes;               /* Stores pipeline output (malloc-ed):
                                         * Initially points to static
                                         * storage */
    int size;                           /* Size of dynamically allocated
                                         * buffer. */

    int fill;                           /* # of bytes read into the
                                         * buffer. Marks the current fill
                                         * point of the buffer. */

    int mark;                           /* # of bytes translated (cooked). */
    int lastMark;                       /* # of bytes as of the last read. This
                                         * indicates the start of the new data
                                         * in the buffer since the last time
                                         * the "update" variable was set. */
    unsigned char staticSpace[DEF_BUFFER_SIZE]; /* Static space */

} Sink;

#define SINK_BUFFERED           (1<<0)
#define SINK_KEEP_NL            (1<<1)
#define SINK_NOTIFY             (1<<2)

typedef struct {
    const char *statusVar;              /* Name of a TCL variable set to the
                                         * exit status of the last
                                         * process. Setting this variable
                                         * triggers the termination of all
                                         * subprocesses (regardless whether
                                         * they have already completed) */
    int signalNum;                      /* If non-zero, indicates the signal
                                         * to send subprocesses when cleaning
                                         * up.*/
    unsigned int flags;                 /* Various bit flags: see below. */
    int interval;                       /* Interval to poll for the exiting
                                         * processes */
    /* Private */
    Tcl_Interp *interp;                 /* Interpreter containing variables */
    int numProcs;                       /* # of processes in pipeline */
    Blt_Pid *procIds;                   /* Array of process tokens from
                                         * pipeline.  Under Unix, tokens are
                                         * pid_t, while for Win32 they're
                                         * handles. */
    Tcl_TimerToken timerToken;          /* Token for timer handler which polls
                                         * for the exit status of each
                                         * sub-process. If zero, there's no
                                         * timer handler queued. */
    int *exitCodePtr;                   /* Pointer to a memory location to
                                         * contain the last process' exit
                                         * code. */
    int *donePtr;
    Sink err, out;                      /* Data sinks for pipeline's output
                                         * and error channels. */
    Blt_ChainLink link;

    int master;                         /* File descriptor of master. */
    int slave;                          /* File descriptor of slave.  */
    char masterName[PTY_NAME_SIZE+1];
    char slaveName[PTY_NAME_SIZE+1];
    pid_t sid;                          /* Pid of session leader. */
} Bgexec;

#define KEEPNEWLINE     (1<<0)          /* Indicates to set TCL output
                                         * variables with trailing newlines
                                         * intact */
#define LINEBUFFERED    (1<<1)          /* Indicates to provide data to update
                                         * variable and update proc on a
                                         * line-by-line * basis. */
#define IGNOREEXITCODE  (1<<2)          /* Don't check for 0 exit status of
                                         * the pipeline.  */
#define TRACED          (1<<3)          /* Indicates that the status variable
                                         * is currently being traced. */
#define DETACHED        (1<<4)          /* Indicates that the pipeline is
                                         * detached from standard I/O, running
                                         * in the background. */
#define KILLED          (1<<5)          /* The variable trace has been triggered
                                         * so ignore any errors from a broken
                                         * connection. */

static Blt_SwitchParseProc ObjToSignalProc;
static Blt_SwitchCustom killSignalSwitch =
{
    ObjToSignalProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ObjToEncodingProc;
static Blt_SwitchFreeProc FreeEncodingProc;
static Blt_SwitchCustom encodingSwitch =
{
    ObjToEncodingProc, NULL, FreeEncodingProc, (ClientData)0,
};

static Blt_SwitchSpec switchSpecs[] = 
{
    {BLT_SWITCH_CUSTOM,  "-decodeoutput",  "encoding", (char *)NULL,
        Blt_Offset(Bgexec, out.encoding),  0, 0, &encodingSwitch}, 
    {BLT_SWITCH_CUSTOM,  "-decodeerror",   "encoding", (char *)NULL,
         Blt_Offset(Bgexec, err.encoding),  0, 0, &encodingSwitch},
    {BLT_SWITCH_BOOLEAN, "-echo",           "bool",  (char *)NULL,
         Blt_Offset(Bgexec, err.echo),   0},
    {BLT_SWITCH_STRING,  "-error",          "variable", (char *)NULL,
        Blt_Offset(Bgexec, err.doneVar),   0},
    {BLT_SWITCH_STRING,  "-update",         "variable", (char *)NULL,
         Blt_Offset(Bgexec, out.updateVar), 0},
    {BLT_SWITCH_STRING,  "-output",         "variable", (char *)NULL,
        Blt_Offset(Bgexec, out.doneVar),   0},
    {BLT_SWITCH_STRING,  "-lasterror",      "variable", (char *)NULL,
        Blt_Offset(Bgexec, err.updateVar), 0},
    {BLT_SWITCH_STRING,  "-lastoutput",     "variable", (char *)NULL,
        Blt_Offset(Bgexec, out.updateVar), 0},
    {BLT_SWITCH_OBJ,    "-onerror",        "command", (char *)NULL,
        Blt_Offset(Bgexec, err.cmdObjPtr), 0},
    {BLT_SWITCH_OBJ,    "-onoutput",       "command", (char *)NULL,
        Blt_Offset(Bgexec, out.cmdObjPtr), 0},
    {BLT_SWITCH_BOOLEAN, "-keepnewline",    "bool", (char *)NULL,
        Blt_Offset(Bgexec, flags),         0,   KEEPNEWLINE}, 
    {BLT_SWITCH_INT,     "-check",          "interval", (char *)NULL,
        Blt_Offset(Bgexec, interval),      0},
    {BLT_SWITCH_CUSTOM,  "-killsignal",     "signal", (char *)NULL,
        Blt_Offset(Bgexec, signalNum),     0,   0, &killSignalSwitch},
    {BLT_SWITCH_BOOLEAN, "-linebuffered",   "bool", (char *)NULL,
        Blt_Offset(Bgexec, flags),         0,   LINEBUFFERED},
    {BLT_SWITCH_BOOLEAN, "-ignoreexitcode", "bool", (char *)NULL,
        Blt_Offset(Bgexec, flags),         0,   IGNOREEXITCODE},
    {BLT_SWITCH_STRING,  "-variable",       "variable", (char *)NULL,
        Blt_Offset(Bgexec, statusVar),   0},
    {BLT_SWITCH_END}
};

static Tcl_VarTraceProc VariableProc;
static Tcl_TimerProc TimerProc;
static Tcl_FileProc StdoutProc, StderrProc;
static Tcl_ExitProc BgexecExitProc;

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSignalProc --
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
ObjToSignalProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Intrepreter to return results */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* Value representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
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
        for (sp = signalTokens; sp->number > 0; sp++) {
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
 * ObjToEncodingProc --
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
ObjToEncodingProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Intrepreter to return results */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* Value representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
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
FreeEncodingProc(ClientData clientData, char *record, int offset, int flags)
{
    Tcl_Encoding *encodingPtr = (Tcl_Encoding *)(record + offset);

    if ((*encodingPtr != ENCODING_BINARY) && (*encodingPtr != ENCODING_ASCII)) {
        Tcl_FreeEncoding(*encodingPtr);
    }
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
GetSinkData(Sink *sinkPtr, unsigned char **dataPtr, int *lengthPtr)
{
    int length;

    sinkPtr->bytes[sinkPtr->mark] = '\0';
    length = sinkPtr->mark;
    if ((sinkPtr->mark > 0) && (sinkPtr->encoding != ENCODING_BINARY)) {
        unsigned char *last;

        last = sinkPtr->bytes + (sinkPtr->mark - 1);
        if (((sinkPtr->flags & SINK_KEEP_NL) == 0) && (*last == '\n')) {
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
NextBlock(Sink *sinkPtr, size_t *lengthPtr)
{
    unsigned char *string;
    ptrdiff_t length;

    string = sinkPtr->bytes + sinkPtr->lastMark;
    length = sinkPtr->mark - sinkPtr->lastMark;
    sinkPtr->lastMark = sinkPtr->mark;
    if (length > 0) {
        if ((!(sinkPtr->flags & SINK_KEEP_NL)) && (string[length-1] == '\n')) {
            length--;
        }
        *lengthPtr = length;
        return string;
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
NextLine(Sink *sinkPtr, size_t *lengthPtr)
{
    if (sinkPtr->mark > sinkPtr->lastMark) {
        unsigned char *string;
        size_t newBytes;
        int i;

        string = sinkPtr->bytes + sinkPtr->lastMark;
        newBytes = sinkPtr->mark - sinkPtr->lastMark;
        for (i = 0; i < newBytes; i++) {
            if (string[i] == '\n') {
                int length;
                
                length = i + 1;
                sinkPtr->lastMark += length;
                if (!(sinkPtr->flags & SINK_KEEP_NL)) {
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
 *      resetting it to empty.  This used when we don't care about keeping all
 *      the data collected from the channel (no -output flag and the process
 *      is detached).
 *
 *---------------------------------------------------------------------------
 */
static void
ResetSink(Sink *sinkPtr)
{ 
    if ((sinkPtr->flags & SINK_BUFFERED) && 
        (sinkPtr->fill > sinkPtr->lastMark)) {
        int i, j;

        /* There may be bytes remaining in the buffer, awaiting another read
         * before we see the next newline.  So move the bytes to the front of
         * the array. */

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
InitSink(Bgexec *bgPtr, Sink *sinkPtr, const char *name)
{
    sinkPtr->name = name;
    sinkPtr->echo = FALSE;
    sinkPtr->fd = -1;
    sinkPtr->bytes = sinkPtr->staticSpace;
    sinkPtr->size = DEF_BUFFER_SIZE;

    if (bgPtr->flags & KEEPNEWLINE) {
        sinkPtr->flags |= SINK_KEEP_NL;
    }
    if (bgPtr->flags & LINEBUFFERED) {
        sinkPtr->flags |= SINK_BUFFERED;
    }   
    if ((sinkPtr->cmdObjPtr != NULL) || 
        (sinkPtr->updateVar != NULL) ||
        (sinkPtr->echo)) {
        sinkPtr->flags |= SINK_NOTIFY;
    }
    ResetSink(sinkPtr);
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
    }
    sinkPtr->fd = -1;
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
static int
ExtendSinkBuffer(Sink *sinkPtr)
{
    unsigned char *bytes;
    /*
     * Allocate a new array, double the old size
     */
    sinkPtr->size += sinkPtr->size;
    bytes = Blt_Malloc(sizeof(unsigned char) * sinkPtr->size);
    if (bytes != NULL) {
        unsigned char *sp, *dp, *send;

        dp = bytes;
        for (sp = sinkPtr->bytes, send = sp + sinkPtr->fill; sp < send; 
             /*empty*/) {
            *dp++ = *sp++;
        }
        if (sinkPtr->bytes != sinkPtr->staticSpace) {
            Blt_Free(sinkPtr->bytes);
        }
        sinkPtr->bytes = bytes;
        return (sinkPtr->size - sinkPtr->fill); /* Return bytes left. */
    }
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
static void
ReadBytes(Sink *sinkPtr)
{
    int i;
    int numBytes;

    /*
     *  Worry about indefinite postponement.
     *
     *  Typically we want to stay in the read loop as long as it takes to
     *  collect all the data that's currently available.  But if it's coming
     *  in at a constant high rate, we need to arbitrarily break out at some
     *  point. This allows for both setting the update variable and the Tk
     *  program to handle idle events.
     */
    numBytes = 0;
    for (i = 0; i < MAX_READS; i++) {
        int bytesLeft;
        unsigned char *array;

        /* Allocate a larger buffer when the number of remaining bytes is
         * below the threshold BLOCK_SIZE.  */

        bytesLeft = sinkPtr->size - sinkPtr->fill;

        if (bytesLeft < BLOCK_SIZE) {
            bytesLeft = ExtendSinkBuffer(sinkPtr);
            if (bytesLeft < 0) {
                errno = ENOMEM;
                sinkPtr->status = READ_ERROR;
                return;
            }
        }
        array = sinkPtr->bytes + sinkPtr->fill;

        /* Read into a buffer but make sure we leave room for a trailing NUL
         * byte. */
        numBytes = read(sinkPtr->fd, array, bytesLeft - 1);
        if (numBytes == 0) {    /* EOF: break out of loop. */
            sinkPtr->status = READ_EOF;
            return;
        }
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
                return;
            }
            sinkPtr->bytes[0] = '\0';
            sinkPtr->status = READ_ERROR;
            return;
        }
        sinkPtr->fill += numBytes;
        sinkPtr->bytes[sinkPtr->fill] = '\0';
    }
    sinkPtr->status = numBytes;
}

#define SINKOPEN(sinkPtr)  ((sinkPtr)->fd != -1)

static void
CloseSink(Tcl_Interp *interp, Sink *sinkPtr)
{
    if (SINKOPEN(sinkPtr)) {
        close(sinkPtr->fd);
        Tcl_DeleteFileHandler(sinkPtr->fd);
        sinkPtr->fd = -1;
        if (sinkPtr->doneVar != NULL) {
            unsigned char *data;
            int length;
            /* 
             * If data is to be collected, set the "done" variable with the
             * contents of the buffer.
             */
            GetSinkData(sinkPtr, &data, &length);
#if (_TCL_VERSION <  _VERSION(8,1,0)) 
            data[length] = '\0';
            if (Tcl_SetVar(interp, sinkPtr->doneVar, data, 
                           TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
                Tcl_BackgroundError(interp);
            }
#else
            if (Tcl_SetVar2Ex(interp, sinkPtr->doneVar, NULL, 
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
static void
CookSink(Tcl_Interp *interp, Sink *sinkPtr)
{
    unsigned char *srcPtr, *endPtr;
#ifdef WIN32
    int oldMark;

    oldMark = sinkPtr->mark;
#endif
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
    } else { /* unicode. */
        int numSrcCooked, numCooked;
        int result;
        int cookedSize, spaceLeft, needed;
        int numRaw, numLeftOver;
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
    /* 
     * Translate CRLF character sequences to LF characters.  We have to do
     * this after converting the string to UTF from UNICODE.
     */
    if (sinkPtr->encoding != ENCODING_BINARY) {
        int count;
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
#endif /* WIN32 */
}

#ifdef notdef
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
static void
CookSink(Tcl_Interp *interp, Sink *sinkPtr)
{
    unsigned char *srcPtr, *endPtr;

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
    } else { /* unicode. */
        int numSrcCooked, numCooked;
        int result;
        int cookedSize, spaceLeft, needed;
        int numRaw, numLeftOver;
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
}
#endif

#if (_TCL_VERSION < _VERSION(8,1,0)) 

static void
NotifyOnUpdate(Tcl_Interp *interp, Sink *sinkPtr, unsigned char *data, 
               size_t numBytes)
{
    char save;

    if (data[0] == '\0') {
        return;
    }
    save = data[numBytes];
    data[numBytes] = '\0';
    if (sinkPtr->echo) {
        Tcl_Channel channel;
        
        channel = Tcl_GetStdChannel(TCL_STDERR);
        if (channel == NULL) {
            Tcl_AppendResult(interp, "can't get stderr channel", (char *)NULL);
            Tcl_BackgroundError(interp);
            sinkPtr->echo = FALSE;
        } else {
            Tcl_Write(channel, data, numBytes);
            if (save == '\n') {
                Tcl_Write(channel, "\n", 1);
            }
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
    if (sinkPtr->updateVar != NULL) {
        if (Tcl_SetVar2Ex(interp, sinkPtr->updateVar, NULL, objPtr, 
                TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
            Tcl_BackgroundError(interp);
        }
    }
    Tcl_DecrRefCount(objPtr);
    data[numBytes] = save;
}

#else 

static void
NotifyOnUpdate(Tcl_Interp *interp, Sink *sinkPtr, unsigned char *data, 
               size_t numBytes)
{
    Tcl_Obj *objPtr;

    if ((numBytes == 0) || (data[0] == '\0')) {
        return;
    }
    if (sinkPtr->echo) {
        Tcl_Channel channel;
        
        channel = Tcl_GetStdChannel(TCL_STDERR);
        if (channel == NULL) {
            Tcl_AppendResult(interp, "can't get stderr channel", (char *)NULL);
            Tcl_BackgroundError(interp);
            sinkPtr->echo = FALSE;
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
    if (sinkPtr->updateVar != NULL) {
        if (Tcl_SetVar2Ex(interp, sinkPtr->updateVar, NULL, objPtr, 
                TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
            Tcl_BackgroundError(interp);
        }
    }
    Tcl_DecrRefCount(objPtr);
}

#endif /* < 8.1.0 */

static int
CollectData(Bgexec *bgPtr, Sink *sinkPtr)
{
    if ((bgPtr->flags & DETACHED) && (sinkPtr->doneVar == NULL)) {
        ResetSink(sinkPtr);
    }
    ReadBytes(sinkPtr);
    CookSink(bgPtr->interp, sinkPtr);
    if ((sinkPtr->mark > sinkPtr->lastMark) && (sinkPtr->flags & SINK_NOTIFY)) {
        if (sinkPtr->flags & SINK_BUFFERED) {
            size_t length;
            unsigned char *data;

            /* For line-by-line updates, call NotifyOnUpdate for each new
             * complete line.  */
            while ((data = NextLine(sinkPtr, &length)) != NULL) {
                NotifyOnUpdate(bgPtr->interp, sinkPtr, data, length);
            }
        } else {
            size_t length;
            unsigned char *data;
            int count;
#ifdef notdef
            int i;
#endif
            length = 0;                 /* Suppress compiler warning. */
            data = NextBlock(sinkPtr, &length);
            #ifdef notdef
            count = 0;
            for (i = 0; i < length; i++) {
                if (data[i] == '\r') {
                    continue;
                }
                if (i > count) {
                    data[count] = data[i];
                }
                count++;
            }
            fprintf(stderr, "\n");
            #else 
            count = length;
            #endif
            NotifyOnUpdate(bgPtr->interp, sinkPtr, data, count);
        }
    }
    if (sinkPtr->status >= 0) {
        return TCL_OK;
    }
    if (sinkPtr->status == READ_ERROR) {
        if ((bgPtr->flags & KILLED) == 0) {
            Tcl_AppendResult(bgPtr->interp, "can't read data from ", 
                sinkPtr->name, ": ", Tcl_PosixError(bgPtr->interp), 
                (char *)NULL);
            Tcl_BackgroundError(bgPtr->interp);
        }
        return TCL_RETURN;
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
CreateSinkHandler(Bgexec *bgPtr, Sink *sinkPtr, Tcl_FileProc *proc)
{
    int flags;

    flags = fcntl(sinkPtr->fd, F_GETFL);
#ifdef O_NONBLOCK
    flags |= O_NONBLOCK;
#else
    flags |= O_NDELAY;
#endif
    if (fcntl(sinkPtr->fd, F_SETFL, flags) < 0) {
        Tcl_AppendResult(bgPtr->interp, "can't set file descriptor ",
            Blt_Itoa(sinkPtr->fd), " to non-blocking:",
            Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    Tcl_CreateFileHandler(sinkPtr->fd, TCL_READABLE, proc, bgPtr);
    return TCL_OK;
}

static void
DisableTriggers(Bgexec *bgPtr) /* Background info record. */
{
    if (bgPtr->flags & TRACED) {
        Tcl_UntraceVar(bgPtr->interp, bgPtr->statusVar, TRACE_FLAGS, 
                VariableProc, bgPtr);
        bgPtr->flags &= ~TRACED;
    }
    if (SINKOPEN(&bgPtr->out)) {
        CloseSink(bgPtr->interp, &bgPtr->out);
    }
    if (SINKOPEN(&bgPtr->err)) {
        CloseSink(bgPtr->interp, &bgPtr->err);
    }
    if (bgPtr->timerToken != (Tcl_TimerToken) 0) {
        Tcl_DeleteTimerHandler(bgPtr->timerToken);
        bgPtr->timerToken = 0;
    }
    if (bgPtr->donePtr != NULL) {
        *bgPtr->donePtr = TRUE;
    }
}

#ifdef notdef
   if ((Flags & DILLO_TTY) && tcgetattr(STDIN_FILENO, &tt) == -1)
 if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tt) == -1)
#endif

/*
 *---------------------------------------------------------------------------
 *
 * CreatePipe --
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
CreatePipe(
    Tcl_Interp *interp, 
    int *inPtr,                         /* (out) Descriptor for read side of
                                         * pipe. */
    int *outPtr)                        /* (out) Descriptor for write side of
                                         * pipe. */
{
    int fd[2];

    if (pipe(fd) < 0) {
        Tcl_AppendResult(interp, "can't create pipe: ",
                    Tcl_PosixError(interp), (char *)NULL);
        return TCL_ERROR;
    }
    fcntl(fd[0], F_SETFD, FD_CLOEXEC);
    fcntl(fd[1], F_SETFD, FD_CLOEXEC);

    *inPtr = fd[0];
    *outPtr = fd[1];
    return TCL_OK;
}

#ifdef HAVE_GETPT
static int 
GetMaster(Bgexec *bgPtr)
{
    int f;

    f = getpt();
    if (f < 0) {
        Tcl_AppendResult(bgPtr->interp, "failed getpt: ", 
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    bgPtr->masterName[0] = '\0';
    bgPtr->master = f;
    return TCL_OK;
}

#elif defined(HAVE_POSIX_OPENPT)

static int 
GetMaster(Bgexec *bgPtr)
{
    int f;

    f = posix_openpt(O_RDWR | O_NOCTTY);
    if (f < 0) {
        Tcl_AppendResult(bgPtr->interp, "failed openpt: ", 
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    bgPtr->masterName[0] = '\0';
    bgPtr->master = f;
    return TCL_OK;
}

#else 
static int 
GetMaster(Bgexec *bgPtr)
{
    static const char ptyChars[] = {
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
    };
    int f;
    const char *p;
    char ptyName[11];

    strcpy(bgPtr->masterName, "/dev/ptmx");
    f = open(bgPtr->masterName, O_RDWR | O_NOCTTY);
    if (f >= 0) {
        bgPtr->master = f;
        return TCL_OK;
    }
    strcpy(ptyName, "/dev/ptyXY");
    for (p = ptyChars; *p != '\0'; p++) {
        const char *q;

        ptyName[8] = *p;
        ptyName[9] = *ptyChars;
        if (access(proto, F_OK) == -1) {
            if (errno == E_NOENT) {
                /* If there is no /dev/ptyX0, skip all Xs. */
                continue;
            } 
            Tcl_AppendResult(bgPtr->interp, "can't access \"", ptyName, "\": ",
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
            return TCL_ERROR;
        }
        for (q = ptyChars; *q != '\0'; q++) {
            int f;

            ptyName[9] = *q;
            f = open(ptyName, O_RDWR);
            if (f >= 0) {
                bgPtr->master = f;
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

typedef int Tcl_File;
static int
InitMaster(Bgexec *bgPtr) 
{
#ifdef HAVE_TCFLUSH
    if (tcflush(bgPtr->master, TCIOFLUSH) < 0) {
        Tcl_AppendResult(bgPtr->interp, "tcflush: ", 
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
#else
#ifdef TIOCFLUSH
    if (ioctl(bgPtr->master, TIOCFLUSH, NULL) < 0) {
        Tcl_AppendResult(bgPtr->interp, "can't set TIOCFLUSH on master: ", 
                         Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
#endif  /* TIOCFLUSH */
#endif  /* HAVE_TCFLUSH */
    
#ifdef TIOCEXCL
    if (ioctl(bgPtr->master, TIOCEXCL, NULL) < 0) {
        Tcl_AppendResult(bgPtr->interp, "can't set TIOCEXCL on descriptor: ", 
                         Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
#endif  /* TIOCEXCL */
    return TCL_OK;
}

#ifdef HAVE_OPEN_CONTROLLING_PTY
static int
OpenMaster(Bgexec *bgPtr)
{
    const char *name;
    int f;

    f = open_controlling_pty(name);
    if (f < 0) {
        Tcl_AppendResult(bgPtr->interp, "can't open controlling pty: ",
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    bgPtr->master = f;
    strcpy(bgPtr->slaveName, name);
    if (InitMaster(bgPtr) != TCL_OK) {
        return TCL_ERROR;
    }
}
#endif  /* HAVE_OPEN_CONTROLLING_PTY */

#ifdef HAVE_GRANTPT
static int 
OpenMaster(Bgexec *bgPtr)
{
    const char *name;

    if (GetMaster(bgPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (grantpt(bgPtr->master) < 0) {
        Tcl_AppendResult(bgPtr->interp, "grantpt: ", 
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    if (unlockpt(bgPtr->master) < 0) {
        Tcl_AppendResult(bgPtr->interp, "unlockpt: ", 
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    name = ptsname(bgPtr->master);
    if (name == NULL) {
        Tcl_AppendResult(bgPtr->interp, "ptsname: ", 
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    strcpy(bgPtr->slaveName, name);
    if (InitMaster(bgPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}
#else

#ifdef HAVE_OPENPTY
#include <util.h>
static int
OpenMaster(Bgexec *bgPtr)
{
    if (openpty(&bgPtr->master, &bgPtr->slave, NULL, NULL, NULL) < 0) {
        Tcl_AppendResult(bgPtr->interp, "can't open pty: ", 
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    if (InitMaster(bgPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}
#endif  /* HAVE_OPENPT */
#endif  /* HAVE_GRANTPT */

static int
OpenSlave(Bgexec *bgPtr) 
{
    int f;
    pid_t pid;
    struct termios tt;

    pid = getpid();                     /* Get the pid of the session leader. */
    if (setsid() < 0) {
        Tcl_AppendResult(bgPtr->interp, "setsid: ", 
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    if (fork() > 0) {
        _exit(0); 
    }
    if (setpgid(0, pid) < 0) {
        Tcl_AppendResult(bgPtr->interp, "setpgid: ", 
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    f = open(bgPtr->slaveName, O_RDWR);
    if (f < 0) {
        Tcl_AppendResult(bgPtr->interp, "can't open \"", bgPtr->slaveName, 
                "\": ", Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    bgPtr->slave = f;
#ifdef TIOCSCTTY
    if (ioctl(f, TIOCSCTTY, (char *)TRUE) < 0) {
        Tcl_AppendResult(bgPtr->interp, "can't set ioctl \"TIOCSCTTY\": ",
                         Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
#endif  /* TIOCSCTTY */
#ifdef I_PUSH
    if (isastream(f)) {
        if (ioctl(f, I_PUSH, "p_tem") < 0) {
            Tcl_AppendResult(bgPtr->interp, "can't set ioctl \"p_tem\": ",
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
            return TCL_ERROR;
        }
        if (ioctl(f, I_PUSH, "ldterm") < 0) {
            Tcl_AppendResult(bgPtr->interp, "can't set ioctl \"ldterm\": ",
                             Tcl_PosixError(bgPtr->interp), (char *)NULL);
            return TCL_ERROR;
        }
    }
#endif
    if (tcgetattr(0, &tt) < 0) {
        Tcl_AppendResult(bgPtr->interp, "can't get terminate attributes: ",
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    if (tcsetattr(bgPtr->slave, TCSAFLUSH, &tt) < 0) {
        Tcl_AppendResult(bgPtr->interp, "can't set terminate attributes: ",
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    if (dup2(bgPtr->slave, 0) < 0) {
        Tcl_AppendResult(bgPtr->interp, "can't dup stdin: ",
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    if (dup2(bgPtr->slave, 1) < 0) {
        Tcl_AppendResult(bgPtr->interp, "can't dup stdout: ",
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    if (dup2(bgPtr->slave, 2) < 0) {
        Tcl_AppendResult(bgPtr->interp, "can't dup stderr: ",
                Tcl_PosixError(bgPtr->interp), (char *)NULL);
        return TCL_ERROR;
    }
    /*
     * Must clear the close-on-exec flag for the target FD, since some
     * systems (e.g. Ultrix) do not clear the CLOEXEC flag on the
     * target FD.
     */
    fcntl(0, F_SETFD, 0);
    fcntl(1, F_SETFD, 0);
    fcntl(2, F_SETFD, 0);
    return TCL_OK;
}

static int
ExecutePipeline(Bgexec *bgPtr, int objc, Tcl_Obj *const *objv)
{
    WAIT_STATUS_TYPE waitStatus, lastStatus;
    int numProcs;                               /* # of processes. */
    pid_t child;
    int mesgIn, mesgOut;
    int errIn, errOut;
    Blt_Pid *pids;

    errIn = errOut = -1;
    mesgIn = mesgOut = -1;

    /*
     * Create a pipe that the child can use to return error information if
     * anything goes wrong in creating the pipeline.
     */
    if (CreatePipe(bgPtr->interp, &mesgIn, &mesgOut) != TCL_OK) {
        return TCL_ERROR;
    }
    /* Create a pseudo terminal for the pipeline. */
    if (OpenMaster(bgPtr) != TCL_OK) {
        goto error;
    }
    if (CreatePipe(bgPtr->interp, &errIn, &errOut) != TCL_OK) {
        goto error;
    }
    bgPtr->err.fd = bgPtr->out.fd = bgPtr->master;
#ifdef notdef
    if ((bgPtr->err.doneVar != NULL) || (bgPtr->err.updateVar != NULL) ||
        (bgPtr->err.cmdObjPtr != NULL) || (!bgPtr->err.echo)) {
        /* If we want stderr separately, create a pipe for the error
         * channel.  */
        bgPtr->err.fd = errIn;
    }
#endif
    child = fork();
    if (child > 0) {
        char mesg[BUFSIZ+1];
        ssize_t numRead;
        /*
         * Read back from the error pipe to see if the pipeline started up
         * properly. The info in the pipe (if any) if the TCL error message
         * from the child process.
         */
        close(mesgOut);
        numRead = read(mesgIn, mesg, BUFSIZ);
        if (numRead == 0) {
            bgPtr->sid = child;
            return TCL_OK;
        }
        close(mesgIn);
        mesg[numRead] = '\0';
        Tcl_AppendResult(bgPtr->interp, mesg, (char *)NULL);
    error:
        if (mesgIn >= 0) {
            close(mesgIn);
        }
        if (mesgOut >= 0) {
            close(mesgOut);
        }
        if (errIn >= 0) {
            close(errIn);
        }
        if (errOut >= 0) {
            close(errOut);
        }
        return TCL_ERROR;
    }
    close(mesgIn);
    /* Open the slave side of the pseudo terminal for the pipeline. */
    /* The default descriptor for 0, 1, and 2 are now the pseudo terminal. */
    if (OpenSlave(bgPtr) != TCL_OK) {
        Blt_Warn("OpenSlave: %s\n", Tcl_GetStringResult(bgPtr->interp));
        return TCL_ERROR;
    }    
    /* Always collect characters from the sinks. Display to the screen or
     * not. */
    numProcs = Blt_CreatePipeline(bgPtr->interp, objc, objv, &pids, 
        (int *)NULL, (int *)NULL, (int *)NULL);
    if (numProcs <= 0) {
        ssize_t numWritten;
        const char *mesg;
        int length;

        mesg = Tcl_GetStringFromObj(Tcl_GetObjResult(bgPtr->interp), &length);
        fprintf(stderr, "child: %s\n", mesg);
        numWritten = write(mesgOut, mesg, length);
        assert(numWritten == length);
        _exit(1);
    }
    close(mesgOut);                     /* The pipeline has started. */
    *((int *)&waitStatus) = 0;
    *((int *)&lastStatus) = 0;
    while (numProcs > 0) {
        int pid;

        pid = waitpid(-1, (int *)&waitStatus, 0);
        if (pid < 0) {
            fprintf(stderr, "waitpid: %s\n", Tcl_PosixError(bgPtr->interp));
            continue;
        }
        /*
         * Save the status information associated with the subprocess.
         * We'll use it only if this is the last subprocess to be reaped.
         */
        lastStatus = waitStatus;
        --numProcs;
    }
    /*
     * All pipeline processes have completed.  Exit the child replicating the
     * exit code.
     */
    exit(WEXITSTATUS(lastStatus));
    return TCL_OK;
}

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
KillPipeline(Bgexec *bgPtr)             /* Background info record. */
{
    if (bgPtr->sid > 0) {
        unsigned long pid;
        Tcl_Pid tclPid;

        if (bgPtr->signalNum > 0) {
            kill(-bgPtr->sid, bgPtr->signalNum);
        }
        pid = (long)bgPtr->sid;
        tclPid = (Tcl_Pid)pid;
        Tcl_DetachPids(1, &tclPid);
    } else if (bgPtr->procIds != NULL) {
        int i;

        for (i = 0; i < bgPtr->numProcs; i++) {
            Tcl_Pid tclPid;
            unsigned long pid;

            if (bgPtr->signalNum > 0) {
                kill(bgPtr->procIds[i].pid, bgPtr->signalNum);
            }

            pid = (long)bgPtr->procIds[i].pid;
            tclPid = (Tcl_Pid)pid;
            Tcl_DetachPids(1, &tclPid);
        }
    }
    Tcl_ReapDetachedProcs();
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
    DisableTriggers(bgPtr);
    FreeSinkBuffer(&bgPtr->err);
    FreeSinkBuffer(&bgPtr->out);
    KillPipeline(bgPtr);
    FreeBgexec(bgPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * VariableProc --
 *
 *      Kills all currently running subprocesses (given the specified
 *      signal). This procedure is called when the user sets the status
 *      variable associated with this group of child subprocesses.
 *
 * Results:
 *      Always returns NULL.  Only called from a variable trace.
 *
 * Side effects:
 *      The subprocesses are signaled for termination using the specified kill
 *      signal.  Additionally, any resources allocated to track the
 *      subprocesses is released.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static char *
VariableProc(
    ClientData clientData,      /* File output information. */
    Tcl_Interp *interp,         /* Not used. */
    const char *part1,          /* Not used. */
    const char *part2,          /* Not Used. */
    int flags)
{
    if (flags & TRACE_FLAGS) {
        Bgexec *bgPtr = clientData;

        /* Kill all child processes that remain alive. */
        KillPipeline(bgPtr);
        bgPtr->flags |= KILLED;
    }
    return NULL;
}


static int
CheckPipeline(Bgexec *bgPtr, Tcl_Obj **objPtrPtr)
{
    enum PROCESS_STATUS { 
        PROCESS_EXITED, PROCESS_STOPPED, PROCESS_KILLED, PROCESS_UNKNOWN
    } pcode;
    static const char *tokens[] = { 
        "EXITED", "KILLED", "STOPPED", "UNKNOWN"
    };
    Tcl_Obj *listObjPtr, *objPtr;
    Tcl_Interp *interp;
    WAIT_STATUS_TYPE waitStatus, lastStatus;
    char string[200];
    const char *mesg;
    int code;
    int i;
    int numLeft;                        /* # of processes still not
                                         * reaped */
    unsigned int lastPid;

    mesg = NULL;                        /* Suppress compiler warning. */
    interp = bgPtr->interp;
    lastPid = (unsigned int)-1;
    *((int *)&waitStatus) = 0;
    *((int *)&lastStatus) = 0;

    numLeft = 0;
    for (i = 0; i < bgPtr->numProcs; i++) {
        int pid;

        pid = waitpid(bgPtr->procIds[i].pid, (int *)&waitStatus, WNOHANG);
        if (pid == 0) {                 /* Process has not terminated yet */
            if (numLeft < i) {
                bgPtr->procIds[numLeft] = bgPtr->procIds[i];
            }
            numLeft++;                  /* Count the # of processes left */
        } else if (pid != -1) {
            /*
             * Save the status information associated with the subprocess.
             * We'll use it only if this is the last subprocess to be reaped.
             */
            lastStatus = waitStatus;
            lastPid = (unsigned int)pid;
        }
    }
    bgPtr->numProcs = numLeft;
    if ((numLeft > 0) || (SINKOPEN(&bgPtr->out)) || 
        (SINKOPEN(&bgPtr->err))) {
        /* Keep polling for the status of the children that are left */
        bgPtr->timerToken = Tcl_CreateTimerHandler(bgPtr->interval, 
                TimerProc, bgPtr);
        return -1;
    }

    /*
     * All child processes have completed.  Set the status variable with the
     * status of the last process reaped.  The status is a list of an error
     * token, the exit status, and a message.
     */
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    code = WEXITSTATUS(lastStatus);
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
    objPtr = Tcl_NewStringObj(tokens[pcode], -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewLongObj(lastPid);
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
        Blt_FormatString(string, 200, "child completed with unknown status 0x%x",
            *((int *)&lastStatus));
        mesg = string;
        break;
    }
    objPtr = Tcl_NewStringObj(mesg, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    if (bgPtr->exitCodePtr != NULL) {
        *bgPtr->exitCodePtr = code;
    }
    *objPtrPtr = listObjPtr;
    return code;
}

static int
CheckSession(Bgexec *bgPtr, Tcl_Obj **objPtrPtr)
{
    enum PROCESS_STATUS { 
        PROCESS_EXITED, PROCESS_STOPPED, PROCESS_KILLED, PROCESS_UNKNOWN
    } pcode;
    static const char *tokens[] = { 
        "EXITED", "KILLED", "STOPPED", "UNKNOWN"
    };
    Tcl_Obj *listObjPtr, *objPtr;
    Tcl_Interp *interp;
    WAIT_STATUS_TYPE waitStatus;
    char string[200];
    const char *mesg;
    int code;
    int pid;

    mesg = NULL;                        /* Suppress compiler warning. */
    interp = bgPtr->interp;
    *((int *)&waitStatus) = 0;
    pid = waitpid(bgPtr->sid, (int *)&waitStatus, WNOHANG);
    if (pid == 0) {                     /* Process has not terminated yet */
        /* Keep polling for the status of the children that are left */
        bgPtr->timerToken = Tcl_CreateTimerHandler(bgPtr->interval, 
                TimerProc, bgPtr);
        return -1;
    }
    /*
     * All child processes have completed.  Set the status variable with the
     * status of the last process reaped.  The status is a list of an error
     * token, the exit status, and a message.
     */
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    code = WEXITSTATUS(waitStatus);
    if (WIFEXITED(waitStatus)) {
        pcode = PROCESS_EXITED;
    } else if (WIFSIGNALED(waitStatus)) {
        pcode = PROCESS_KILLED;
        code = -1;
    } else if (WIFSTOPPED(waitStatus)) {
        pcode = PROCESS_STOPPED;
        code = -1;
    } else {
        pcode = PROCESS_UNKNOWN;
    }
    objPtr = Tcl_NewStringObj(tokens[pcode], -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewLongObj(pid);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    switch(pcode) {
    case PROCESS_EXITED:
        mesg = "child completed normally";
        break;
    case PROCESS_KILLED:
        mesg = Tcl_SignalMsg((int)(WTERMSIG(waitStatus)));
        break;
    case PROCESS_STOPPED:
        mesg = Tcl_SignalMsg((int)(WSTOPSIG(waitStatus)));
        break;
    case PROCESS_UNKNOWN:
        Blt_FormatString(string, 200, "child completed with unknown status 0x%x",
            *((int *)&waitStatus));
        mesg = string;
        break;
    }
    objPtr = Tcl_NewStringObj(mesg, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    *objPtrPtr = listObjPtr;
    return code;
}

/*
 *---------------------------------------------------------------------------
 *
 * TimerProc --
 *
 *      This is a timer handler procedure which gets called periodically to
 *      reap any of the sub-processes if they have terminated.  After the last
 *      process has terminated, the contents of standard output are stored in
 *      the output variable, which triggers the cleanup proc (using a variable
 *      trace). The status the last process to exit is written to the status
 *      variable.
 *
 * Results:
 *      None.  Called from the TCL event loop.
 *
 * Side effects:
 *      Many. The contents of procIds is shifted, leaving only those
 *      sub-processes which have not yet terminated.  If there are still
 *      subprocesses left, this procedure is placed in the timer queue
 *      again. Otherwise the output and possibly the status variables are
 *      updated.  The former triggers the cleanup routine which will destroy
 *      the information and resources associated with these background
 *      processes.
 *
 *---------------------------------------------------------------------------
 */
static void
TimerProc(ClientData clientData)
{
    Bgexec *bgPtr = clientData;
    Tcl_Obj *resultObjPtr;
    int code;

    resultObjPtr = NULL;                /* Suppress compiler warning.  */
    if (bgPtr->sid > 0) {
        code = CheckSession(bgPtr, &resultObjPtr);
    } else {
        code = CheckPipeline(bgPtr, &resultObjPtr);
    }
    assert(resultObjPtr != NULL);
    if (code < 0) {
        /* Keep polling for the status of the children that are left */
        return;
    }
    if (bgPtr->exitCodePtr != NULL) {
        *bgPtr->exitCodePtr = code;
    }
    DisableTriggers(bgPtr);
    if (Tcl_SetVar2Ex(bgPtr->interp, bgPtr->statusVar, NULL, resultObjPtr, 
                      TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
        Tcl_BackgroundError(bgPtr->interp);
    }
    if (bgPtr->flags & DETACHED) {
        DestroyBgexec(bgPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Stdoutproc --
 *
 *      This procedure is called when output from the detached pipeline is
 *      available.  The output is read and saved in a buffer in the Bgexec
 *      structure.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Data is stored in the buffer.  This character array may be increased
 *      as more space is required to contain the output of the pipeline.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
StdoutProc(ClientData clientData, int mask)
{
    Bgexec *bgPtr = clientData;

    if (CollectData(bgPtr, &bgPtr->out) == TCL_OK) {
        return;
    }
    /*
     * Either EOF or an error has occurred.  In either case, close the
     * sink. Note that closing the sink will also remove the file handler, so
     * this routine will not be called again.
     */
    CloseSink(bgPtr->interp, &bgPtr->out);

    /*
     * If both sinks (stdout and stderr) are closed, this doesn't necessarily
     * mean that the process has terminated.  Set up a timer handler to
     * periodically poll for the exit status of each process.  Initially check
     * at the next idle interval.
     */
    if (!SINKOPEN(&bgPtr->err)) {
        bgPtr->timerToken = Tcl_CreateTimerHandler(0, TimerProc, clientData);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * StderrProc --
 *
 *      This procedure is called when error from the detached pipeline is
 *      available.  The error is read and saved in a buffer in the Bgexec
 *      structure.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Data is stored in the buffer.  This character array may be increased
 *      as more space is required to contain the stderr of the pipeline.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
StderrProc(ClientData clientData, int mask)
{
    Bgexec *bgPtr = clientData;

    if (CollectData(bgPtr, &bgPtr->err) == TCL_OK) {
        return;
    }
    /*
     * Either EOF or an error has occurred.  In either case, close the
     * sink. Note that closing the sink will also remove the file handler, so
     * this routine will not be called again.
     */
    CloseSink(bgPtr->interp, &bgPtr->err);

    /*
     * If both sinks (stdout and stderr) are closed, this doesn't necessarily
     * mean that the process has terminated.  Set up a timer handler to
     * periodically poll for the exit status of each process.  Initially check
     * at the next idle interval.
     */
    if (!SINKOPEN(&bgPtr->out)) {
        bgPtr->timerToken = Tcl_CreateTimerHandler(0, TimerProc, clientData);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * BgexecCmd --
 *
 *      This procedure is invoked to process the "ptyexec" TCL command.  See
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
BgexecCmd(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Current interpreter. */
    int objc,                           /* # of arguments. */
    Tcl_Obj *const *objv)               /* Argument strings. */
{
    Bgexec *bgPtr;
    char *lastArg;
    int isDetached;
    int i;
#ifdef notdef
    Blt_Pid *pidPtr;
    int *outFdPtr, *errFdPtr;
    int numProcs;
#endif
    if (objc < 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " varName ?options? command ?arg...?\"",
                (char *)NULL);
        return TCL_ERROR;
    }

    /* Check if the command line is to be run detached (the last argument is
     * "&") */
    lastArg = Tcl_GetString(objv[objc - 1]);
    isDetached = ((lastArg[0] == '&') && (lastArg[1] == '\0'));
    if (isDetached) {
        objc--;                         /* Remove the '&' argument */
    }
    bgPtr = Blt_AssertCalloc(1, sizeof(Bgexec));
    /* Initialize the background information record */
    bgPtr->slave = bgPtr->master = -1;
    bgPtr->interp = interp;
    bgPtr->signalNum = SIGHUP;
    bgPtr->numProcs = -1;
    bgPtr->interval = 1000;
    if (isDetached) {
        bgPtr->flags |= DETACHED;
    }
    Tcl_MutexLock(mutexPtr);
    bgPtr->link = Blt_Chain_Append(activePipelines, bgPtr);
    Tcl_MutexUnlock(mutexPtr);
    bgPtr->out.encoding = ENCODING_ASCII;
    bgPtr->err.encoding = ENCODING_ASCII;

    /* Try to clean up any detached processes */
    Tcl_ReapDetachedProcs();
    i = Blt_ParseSwitches(interp, switchSpecs, objc - 1, objv + 1, bgPtr, 
                          BLT_SWITCH_OBJV_PARTIAL);
    if (i < 0) {
        FreeBgexec(bgPtr);
        return TCL_ERROR;
    }
    i += 1;
    /* Must be at least one argument left as the command to execute. */
    if (objc <= i) {
        Tcl_AppendResult(interp, "missing command to execute: should be \"",
            Tcl_GetString(objv[0]), " varName ?options? command ?arg...?\"", 
                         (char *)NULL);
        FreeBgexec(bgPtr);
        return TCL_ERROR;
    }
    InitSink(bgPtr, &bgPtr->out, "stdout");
    InitSink(bgPtr, &bgPtr->err, "stderr");
    if (bgPtr->statusVar == NULL) {
        static int count = 0;
        char string[200];

        count++;
        sprintf(string, "blt::ptyexecstatus%d", count);
        bgPtr->statusVar = Blt_AssertStrdup(string);
    }
    /* Put a trace on the exit status variable.  The will also allow the user
     * to terminate the pipeline by simply setting the variable.  */
    Tcl_TraceVar(interp, bgPtr->statusVar, TRACE_FLAGS, VariableProc,bgPtr);
    bgPtr->flags |= TRACED;

    if (ExecutePipeline(bgPtr, objc - i, objv + i) != TCL_OK) {
        goto error;
    }
    if (CreateSinkHandler(bgPtr, &bgPtr->out, StdoutProc) != TCL_OK) {
        goto error;
    }
    if (bgPtr->err.fd != bgPtr->master) {
        /* Only create a sink for stderr if a separation of stdout and stderr
         * is requested.  */
        if (CreateSinkHandler(bgPtr, &bgPtr->err, StderrProc) != TCL_OK) {
            goto error;
        }
    }
    if (isDetached) {   
        /* If detached, return the process id of the session instead of the
         * output of the pipeline. */
        Tcl_SetLongObj(Tcl_GetObjResult(interp), bgPtr->sid);
    } else {
        int exitCode;
        int done;

        bgPtr->exitCodePtr = &exitCode;
        bgPtr->donePtr = &done;

        exitCode = done = 0;
        while (!done) {
            Tcl_DoOneEvent(0);
        }
        DisableTriggers(bgPtr);
        if ((bgPtr->flags & IGNOREEXITCODE) || (exitCode == 0)) {
            if (bgPtr->out.doneVar == NULL) {
                unsigned char *data;
                int length;
                
                /* Return the output of the pipeline. */
                GetSinkData(&bgPtr->out, &data, &length);
                assert(length <= UINT_MAX);
#if (_TCL_VERSION <  _VERSION(8,1,0)) 
                data[length] = '\0';
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
        next = Blt_Chain_NextLink(link);
        Bgexec *bgPtr;

        bgPtr = Blt_Chain_GetValue(link);
        bgPtr->link = NULL;
        KillPipeline(bgPtr);
    }
    Blt_Chain_Destroy(activePipelines);
    Tcl_MutexUnlock(mutexPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_BgexecCmdInitProc --
 *
 *      This procedure is invoked to initialize the "ptyexec" TCL command.  See
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
Blt_PtyExecCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "ptyexec", BgexecCmd, };

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

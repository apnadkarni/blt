/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltWinPipe.c --
 *
 * This module replaces the former Tcl_CreatePipeline API under Windows.  This
 * file contains the generic portion of the command channel driver as well as
 * various utility routines used in managing subprocesses.
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
 * Parts taken from tclPipe.c and tclWinPipe.c in the TCL distribution.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 *
 *   This software is copyrighted by the Regents of the University of
 *   California, Sun Microsystems, Inc., and other parties.  The following
 *   terms apply to all files associated with the software unless
 *   explicitly disclaimed in individual files.
 * 
 *   The authors hereby grant permission to use, copy, modify, distribute,
 *   and license this software and its documentation for any purpose,
 *   provided that existing copyright notices are retained in all copies
 *   and that this notice is included verbatim in any distributions. No
 *   written agreement, license, or royalty fee is required for any of the
 *   authorized uses.  Modifications to this software may be copyrighted by
 *   their authors and need not follow the licensing terms described here,
 *   provided that the new terms are clearly indicated on the first page of
 *   each file where they apply.
 * 
 *   IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 *   FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 *   ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 *   DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 * 
 *   THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 *   NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND
 *   THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 *   MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *   GOVERNMENT USE: If you are acquiring this software on behalf of the
 *   U.S. government, the Government shall have only "Restricted Rights" in
 *   the software and related documentation as defined in the Federal
 *   Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 *   are acquiring the software on behalf of the Department of Defense, the
 *   software shall be classified as "Commercial Computer Software" and the
 *   Government shall have only "Restricted Rights" as defined in Clause
 *   252.227-7013 (b) (3) of DFARs.  Notwithstanding the foregoing, the
 *   authors grant the U.S. Government and others acting in its behalf
 *   permission to use and distribute the software in accordance with the
 *   terms specified in this license.
 */

/*
 * Todo:
 *      Does terminating bltwish kill child processes?
 *      Handle EOL translation more cleanly.
 */

#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#include "bltAlloc.h"
#include "bltChain.h"

#define PEEK_DEBUG 0
#define QUEUE_DEBUG 0
#define READER_DEBUG 0
#define ASYNC_DEBUG 0
#define KILL_DEBUG 0

/*
 * The following type identifies the various types of applications that
 * run under windows.  There is special case code for the various types.
 */
typedef enum ApplicationTypes {
    APPL_NONE, 
    APPL_DOS, 
    APPL_WIN3X, 
    APPL_WIN32, 
    APPL_INTERP
} ApplicationType;

#ifndef IMAGE_OS2_SIGNATURE
#   define IMAGE_OS2_SIGNATURE    (0x454E)
#endif
#ifndef IMAGE_VXD_SIGNATURE
#   define IMAGE_VXD_SIGNATURE    (0x454C)
#endif

#define PIPE_BUFSIZ     (BUFSIZ*2)      /* Size of pipe read buffer. */

#define PIPE_PENDING    (1<<13)         /* Message is pending in the queue. */
#define PIPE_EOF        (1<<14)         /* Pipe has reached EOF. */
#define PIPE_DELETED    (1<<15)         /* Indicates if the pipe has been
                                         * deleted but its memory hasn't been
                                         * freed yet. */
typedef struct {
    int flags;                          /* State flags, see above for a
                                         * list. */
    HANDLE hPipe;                       /* Pipe handle */
    HANDLE thread;                      /* Thread watching I/O on this
                                         * pipe. */
    HANDLE parent;                      /* Handle of main thread. */
    DWORD parentId;                     /* Main thread ID. */
    HWND hWindow;                       /* Notifier window in main
                                         * thread. Used to goose the TCL
                                         * notifier system indicating that
                                         * an event has occurred that it
                                         * needs to process. */
    HANDLE idleEvent;                   /* Signals that the pipe is idle (no
                                         * one is reading/writing from it). */
    HANDLE readyEvent;                  /* Signals that the pipe is ready for
                                         * the next I/O operation. */
    DWORD lastError;                    /* Error. */
    char *buffer;                       /* Current background output
                                         * buffer. */
    DWORD start, end;                   /* Pointers into the output
                                         * buffer */
    DWORD size;                         /* Size of buffer. */
    Tcl_FileProc *proc;
    ClientData clientData;
} PipeHandler;


typedef struct {
    Tcl_Event header;                   /* Information that is standard for
                                         * all events. */
    PipeHandler *pipePtr;               /* Pointer to pipe handler structure.
                                         * Note that we still have to verify
                                         * that the pipe exists before
                                         * dereferencing this pointer.  */
} PipeEvent;


static int initialized = 0;
static Blt_Chain pipeChain;
static CRITICAL_SECTION pipeCriticalSection;

static DWORD WINAPI PipeWriterThread(void *clientData);
static DWORD WINAPI PipeReaderThread(void *clientData);

static Tcl_FreeProc DestroyPipe;

#ifndef USE_TCL_STUBS
extern HINSTANCE TclWinGetTclInstance(void);
extern void TclWinConvertError(DWORD lastError);
#endif

typedef struct {
    HANDLE hFile;
    HANDLE hCurrFile;
    HANDLE hPipe;
    BOOL redirected;
    BOOL mustClose;
} FileInfo;

static void
InitFileInfo(FileInfo *infoPtr)
{
    infoPtr->hFile = INVALID_HANDLE_VALUE;
    infoPtr->hCurrFile = INVALID_HANDLE_VALUE;
    infoPtr->hPipe = INVALID_HANDLE_VALUE;
    infoPtr->redirected = FALSE;
    infoPtr->mustClose = FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifierWindowProc --
 *
 *      This procedure is called to goose the TCL notifier layer to service
 *      pending events.  The notifier is built upon the Windows message
 *      system.  The Windows event loop may need to be awakened if it's
 *      blocked waiting on messages.  Our psuedo pipes (e.g.  data
 *      available on a pipe) won't do that.  While there may be events
 *      pending in the TCL queue, Windows knows nothing about TCL events
 *      and won't unblock until the next Windows message arrives.
 *
 *      This routine sits in the main thread and is triggered by messages
 *      posted to a notifier window (we set it up earlier) from the
 *      reader/writer pipe threads.  It's purpose is two fold:
 *
 *      1) unblock Windows (posting the message does that) and 
 *      2) call Tcl_ServiceAll from the main thread.
 *
 * Results:
 *      A standard Windows result.
 *
 * Side effects:
 *      Services any pending TCL events.
 *
 *---------------------------------------------------------------------------
 */
static LRESULT CALLBACK
NotifierWindowProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_USER:
    case WM_TIMER:
        break;

    default:
        return DefWindowProc(hWindow, message, wParam, lParam);
    }

    Tcl_ServiceAll();                   /* Process all run-able events. */
    return 0;
}

static void
WakeupNotifier(HWND hWindow)
{
    PostMessage(hWindow, WM_USER, 0, 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetNotifierWindow --
 *
 *      Initializes the platform specific notifier state.
 *
 * Results:
 *      Returns a handle to the notifier state for this thread..
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static HWND
GetNotifierWindow(void)
{
    static HWND hWindow = NULL;

    /*
     * Register Notifier window class if this is the first thread to use this
     * module.
     */
    if (hWindow == NULL) {
        WNDCLASS class;
        HINSTANCE hInstance;

        memset(&class, 0, sizeof(WNDCLASS));
        hInstance = TclWinGetTclInstance();
        class.hInstance = hInstance;
        class.lpszClassName = "PipeNotifier";
        class.lpfnWndProc = NotifierWindowProc;

        if (!RegisterClassA(&class)) {
            panic("Unable to register PipeNotifier window class");
        }
        /*
         * Create a window for communication with the notifier.
         */
        hWindow = CreateWindowA("PipeNotifier", "PipeNotifier", WS_TILED,
            0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    }
    return hWindow;
}

/*
 *---------------------------------------------------------------------------
 *
 * PeekOnPipe --
 *
 *      See if some data is available, the pipe is at EOF, or the reader
 *      thread is currently blocked waiting for data.
 *
 * Results:
 *      Return TRUE if data is available, FALSE if otherwise.  Note that
 *      errors and EOF always return TRUE.  We always make the condition
 *      available until the caller handles it by deleting the pipe event
 *      handler.
 *
 *      On TRUE, the number of bytes returned indicates the following:
 *        0     EOF.
 *        -1    An error has occured or the thread is currently
 *              blocked reading.  In the latter case, errno is set
 *              to EAGAIN.
 *        >0    Number of bytes of data in the buffer.
 *
 *---------------------------------------------------------------------------
 */
static int
PeekOnPipe(PipeHandler *pipePtr, int *numBytesAvailPtr)
{
    int state;
    int numBytesAvail;

    *numBytesAvailPtr = -1;
    state = WaitForSingleObject(pipePtr->readyEvent, 0);
    switch(state) {
    case WAIT_TIMEOUT:
        errno = EAGAIN;
        return FALSE;                   /* Reader thread is currently
                                         * blocked. */
    case WAIT_OBJECT_0:
        if (pipePtr->end < pipePtr->start) {
            fprintf(stderr, "pipe %p (start %d > end %d)\n", pipePtr->hPipe,
                    pipePtr->start, pipePtr->end);
        }
        numBytesAvail = pipePtr->end - pipePtr->start;
        if ((numBytesAvail == 0) && !(pipePtr->flags & PIPE_EOF)) {
            return FALSE;
        }
        *numBytesAvailPtr = numBytesAvail;
        return TRUE;

    default:
    case WAIT_FAILED:
    case WAIT_ABANDONED:
        *numBytesAvailPtr = -1;
        return TRUE;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PipeEventProc --
 *
 *      This function is invoked by Tcl_ServiceEvent when a file event
 *      reaches the front of the event queue.  This procedure calls back
 *      the handler procedure designated for this pipe.
 *
 * Results:
 *      Returns 1 if the event was handled, meaning it should be removed
 *      from the queue.  Returns 0 if the event was not handled, meaning it
 *      should stay on the queue.  The only time the event isn't handled is
 *      if the TCL_FILE_EVENTS flag bit isn't set.
 *
 * Side effects:
 *      Whatever the pipe handler callback does.
 *
 *---------------------------------------------------------------------------
 */
static int
PipeEventProc(Tcl_Event * eventPtr, int flags)
{
    PipeHandler *pipePtr;

    if (!(flags & TCL_FILE_EVENTS)) {
        return 0;
    }
    pipePtr = ((PipeEvent *) eventPtr)->pipePtr;
    if ((pipePtr != NULL) && !(pipePtr->flags & PIPE_DELETED)) {
        Tcl_Preserve(pipePtr);
        if (pipePtr->proc != NULL) {
            (*pipePtr->proc) (pipePtr->clientData, flags);
        }
        /* Allow more events again. */
        pipePtr->flags &= ~PIPE_PENDING;
        Tcl_Release(pipePtr);
    }
    return 1;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetupHandlers --
 *
 *      This procedure is invoked before Tcl_DoOneEvent blocks waiting for an
 *      event.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Adjusts the block time if needed.
 *
 *---------------------------------------------------------------------------
 */
static void
SetupHandlers(ClientData clientData, int flags)
{
    Blt_Chain chain = clientData;
    Blt_ChainLink link;
    int dontBlock;
    Tcl_Time blockTime;

    if (!(flags & TCL_FILE_EVENTS)) {
        return;
    }
    /*
     * Loop through the list of pipe handlers.  Check if any I/O events are
     * currently pending.
     */
    dontBlock = FALSE;
    blockTime.sec = blockTime.usec = 0L;
    for (link = Blt_Chain_FirstLink(chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        PipeHandler *pipePtr;

        pipePtr = Blt_Chain_GetValue(link);
        if (pipePtr->flags & PIPE_DELETED) {
            continue;                   /* Ignore pipes pending to be
                                         * freed. */
        }
        if (pipePtr->flags & TCL_READABLE) {
            int numBytesAvail;

            if (PeekOnPipe(pipePtr, &numBytesAvail)) {
                dontBlock = TRUE;
            }
        }
        if (pipePtr->flags & TCL_WRITABLE) {
            if (WaitForSingleObject(pipePtr->readyEvent, 0) != WAIT_TIMEOUT) {
                dontBlock = TRUE;
            }
        }
    }
    if (dontBlock) {
        Tcl_SetMaxBlockTime(&blockTime);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckHandlers --
 *
 *      This procedure is called by Tcl_DoOneEvent to check the pipe
 *      event source for events.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      May queue an event.
 *
 *---------------------------------------------------------------------------
 */
static void
CheckHandlers(ClientData clientData, int flags)
{
    Blt_Chain chain = clientData;
    Blt_ChainLink link;

    if ((flags & TCL_FILE_EVENTS) == 0) {
        return;
    }
    /* Queue events for any ready pipes that aren't already queued.  */

    for (link = Blt_Chain_FirstLink(chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        PipeHandler *pipePtr;
        int queueEvent;

        pipePtr = Blt_Chain_GetValue(link);
        if (pipePtr->flags & (PIPE_PENDING | PIPE_DELETED)) {
            continue;                  /* If this pipe already is scheduled to
                                        * service an event, wait for it to
                                        * handle it. */
        }
        /* Queue an event if the pipe is signaled for reading or writing.  */
        queueEvent = FALSE;
        if (pipePtr->flags & TCL_READABLE) {
            int numBytesAvail;

            if (PeekOnPipe(pipePtr, &numBytesAvail)) {
                queueEvent = TRUE;
            }
        }
        if (pipePtr->flags & TCL_WRITABLE) {
            if (WaitForSingleObject(pipePtr->readyEvent, 0) != WAIT_TIMEOUT) {
                queueEvent = TRUE;
            }
        }
        if (queueEvent) {
            PipeEvent *eventPtr;

            pipePtr->flags |= PIPE_PENDING;
            eventPtr = Blt_AssertMalloc(sizeof(PipeEvent));
            eventPtr->header.proc = PipeEventProc;
            eventPtr->pipePtr = pipePtr;
            Tcl_QueueEvent((Tcl_Event *) eventPtr, TCL_QUEUE_TAIL);
        }
    }
}

static PipeHandler *
NewPipeHandler(HANDLE hPipe, unsigned int flags, Tcl_FileProc *proc,
               ClientData clientData)

{
    DWORD id;
    PipeHandler *pipePtr;
    LPTHREAD_START_ROUTINE threadProc;

    pipePtr = Blt_AssertCalloc(1, sizeof(PipeHandler));
    pipePtr->hPipe = hPipe;
    pipePtr->flags = flags;
    pipePtr->proc = proc;
    pipePtr->clientData = clientData;
    pipePtr->parentId = GetCurrentThreadId();
    pipePtr->parent = GetCurrentThread();
    pipePtr->hWindow = GetNotifierWindow();
    pipePtr->readyEvent = CreateEvent(
        NULL,                           /* Security attributes. */
        TRUE,                           /* Manual reset event */
        FALSE,                          /* Initially not signaled. */
        NULL);                          /* Event object's name. */
    pipePtr->idleEvent = CreateEvent(
        NULL,                           /* Security attributes. */
        FALSE,                          /* Auto reset event. */
        TRUE,                           /* Initially signaled. */
        NULL);                          /* Event object's name. */

    if (flags & TCL_READABLE) {
        threadProc = (LPTHREAD_START_ROUTINE) PipeReaderThread;
        pipePtr->buffer = Blt_AssertCalloc(1, PIPE_BUFSIZ);
        pipePtr->size = PIPE_BUFSIZ;
    } else {
        threadProc = (LPTHREAD_START_ROUTINE) PipeWriterThread;
    }

    pipePtr->thread = CreateThread(
        NULL,                           /* Security attributes */
        8000,                           /* Initial stack size. */
        threadProc,                     /* Starting address of thread routine */
        (DWORD *)pipePtr,               /* One-word of data passed to
                                         * routine. */
        0,                              /* Creation flags */
        &id);                           /* (out) Will contain Id of new
                                         * thread. */
    SetThreadPriority(pipePtr->thread, THREAD_PRIORITY_HIGHEST);
    return pipePtr;
}

static void
DestroyPipe(DestroyData data)
{
    PipeHandler *pipePtr = (PipeHandler *)data;

    if (pipePtr->buffer != NULL) {
        Blt_Free(pipePtr->buffer);
    }
    Blt_Free(pipePtr);
}

static void
DeletePipeHandler(PipeHandler * pipePtr)
{
    fprintf(stderr, "deleting pipe %p (%d entries)\n", pipePtr->hPipe,
            Blt_Chain_GetLength(pipeChain));
    
    if ((pipePtr->flags & TCL_WRITABLE) &&
        (pipePtr->hPipe != INVALID_HANDLE_VALUE)) {
        /* Wait for the writer thread to finish with the current buffer */
        WaitForSingleObject(pipePtr->idleEvent, INFINITE);
    }
    if (pipePtr->hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(pipePtr->hPipe);
	pipePtr->hPipe = INVALID_HANDLE_VALUE;
    }
    CloseHandle(pipePtr->readyEvent);
    CloseHandle(pipePtr->idleEvent);
    CloseHandle(pipePtr->thread);

    pipePtr->idleEvent = pipePtr->readyEvent = INVALID_HANDLE_VALUE;
    pipePtr->thread = INVALID_HANDLE_VALUE;

    pipePtr->flags |= PIPE_DELETED;     /* Mark the pipe has deleted. */
    Tcl_EventuallyFree(pipePtr, DestroyPipe);
}

/*
 *---------------------------------------------------------------------------
 *
 * PipeInit --
 *
 *      This function initializes the static variables for this file.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Creates a new event source.
 *
 *---------------------------------------------------------------------------
 */
static void
PipeInit(void)
{
    initialized = TRUE;
    InitializeCriticalSection(&pipeCriticalSection);
    if (pipeChain == NULL) {
        pipeChain = Blt_Chain_Create();
    } else {
        Blt_Chain_Init(pipeChain);
    }
    Tcl_CreateEventSource(SetupHandlers, CheckHandlers, pipeChain);
}

static PipeHandler *
GetPipeHandler(HANDLE hPipe)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(pipeChain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        PipeHandler *pipePtr;

        pipePtr = Blt_Chain_GetValue(link);
        if ((pipePtr->hPipe == hPipe) && !(pipePtr->flags & PIPE_DELETED)){
            return pipePtr;
        }
    }
    fprintf(stderr, "Can't find pipe for %p (%d entries)\n", hPipe,
            Blt_Chain_GetLength(pipeChain));
    return NULL;
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * Blt_PipeTeardown --
 *
 *      This function releases any storage allocated for this file.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Creates a new event source.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_PipeTeardown(void)
{
    Blt_ChainLink link;

    if (!initialized) {
        return;                         /* Was never initialized. */
    }
    initialized = FALSE;
    EnterCriticalSection(&pipeCriticalSection);
    for (link = Blt_Chain_FirstLink(pipeChain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        PipeHandler *pipePtr;

        pipePtr = Blt_Chain_GetValue(link);
        if ((pipePtr != NULL) && !(pipePtr->flags & PIPE_DELETED)) {
            DeletePipeHandler(pipePtr);
        }
    }
    DestroyWindow(GetNotifierWindow());
    UnregisterClassA("PipeNotifier", TclWinGetTclInstance());

    Blt_Chain_Reset(pipeChain);
    LeaveCriticalSection(&pipeCriticalSection);
    Tcl_DeleteEventSource(SetupHandlers, CheckHandlers, pipeChain);
    DeleteCriticalSection(&pipeCriticalSection);
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * PipeReaderThread --
 *
 *      This function runs in a separate thread and waits for input to become
 *      available on a pipe.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Signals the main thread when input become available.  May cause the
 *      main thread to wake up by posting a message.
 *
 *---------------------------------------------------------------------------
 */
static DWORD WINAPI
PipeReaderThread(void *clientData)
{
    PipeHandler *pipePtr = clientData;
    DWORD count;
    BOOL result;

    for (;;) {
        if (pipePtr->flags & PIPE_DELETED) {
            break;
        }
        /* Synchronize with the main thread so that we don't try to read
         * from the pipe while it's copying to the buffer.  */
        WaitForSingleObject(pipePtr->idleEvent, INFINITE);
        /* Read from the pipe. The thread will block here until some data
         * is read into its buffer. */
        if (pipePtr->start != pipePtr->end) {
            fprintf(stderr, "pipe %p (start %d != end %d)\n", pipePtr->hPipe,
                    pipePtr->start, pipePtr->end);
        }
        result = ReadFile(
            pipePtr->hPipe,             /* Handle to anonymous pipe. */
            pipePtr->buffer,            /* Data buffer. */
            pipePtr->size,              /* Requested number of bytes (the
                                         * size of the buffer) */
            &count,                     /* (out) Number of bytes actually
                                         * read. */
            NULL);                      /* Overlapping I/O */

        if (!result) {
            fprintf(stderr, "ReadFile failed on pipe %p errno=%d error=%ld\n",
                    pipePtr->hPipe, errno, GetLastError());
        }
        /*
         * Reset counters to indicate that the buffer has been refreshed.
         */
        pipePtr->start = 0;
        pipePtr->end = count;
        if (count == 0) {
            /* We've hit EOF or an error. */
            pipePtr->lastError = GetLastError();
            if ((pipePtr->lastError == ERROR_BROKEN_PIPE) ||
                (pipePtr->lastError == ERROR_HANDLE_EOF)) {
                pipePtr->flags |= PIPE_EOF;
            }
            fprintf(stderr, "ReadFile returned 0 lasterror is %ld\n",
                    GetLastError());
        }
        WakeupNotifier(pipePtr->hWindow);
        SetEvent(pipePtr->readyEvent);
        if (count == 0) {
            ExitThread(0);
        }
    }
    /* NOTREACHED */
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * PipeWriterThread --
 *
 *      This function runs in a separate thread and writes data to the
 *      process' standard input pipe.
 *
 * Results:
 *      Always returns 0.
 *
 * Side effects:
 *      Signals the main thread when an output operation is completed.  May
 *      cause the main thread to wake up by posting a message.
 *
 *---------------------------------------------------------------------------
 */
static DWORD WINAPI
PipeWriterThread(void *clientData)
{
    PipeHandler *pipePtr = clientData;

    for (;;) {
        int bytesLeft;
        char *ptr;

        if (pipePtr->flags & PIPE_DELETED) {
            break;
        }

        /*
         * Synchronize with the main thread so that we don't test the pipe
         * until its done writing.
         */
g        WaitForSingleObject(pipePtr->idleEvent, INFINITE);

        ptr = pipePtr->buffer;
        bytesLeft = pipePtr->end;

        /* Loop until all of the bytes are written or an error occurs.  */

        while (bytesLeft > 0) {
            DWORD count;

            if (!WriteFile(pipePtr->hPipe, ptr, bytesLeft, &count, NULL)) {
                pipePtr->lastError = GetLastError();
                break;
            }
            bytesLeft -= count;
            ptr += count;
        }
        CloseHandle(pipePtr->hPipe);
        /* Tell the main thread that data can be written to the pipe.
         * Remember to wake up the notifier thread.  */
        SetEvent(pipePtr->readyEvent);
        WakeupNotifier(pipePtr->hWindow);
    }
    /* NOTREACHED */
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * TempFileName --
 *
 *      Gets a temporary file name and deals with the fact that the temporary
 *      file path provided by Windows may not actually exist if the "TMP" or
 *      "TEMP" environment variables refer to a non-existent directory.
 *
 * Results:
 *      0 if error, non-zero otherwise.  If non-zero is returned, the name
 *      buffer will be filled with a name that can be used to construct a
 *      temporary file.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

static int
TempFileName(char *name)                /* (out) Buffer to hold name of
                                         * temporary file. */
{
    if ((GetTempPath(MAX_PATH, name) > 0) &&
        (GetTempFileName(name, "TCL", 0, name))) {
        return 1;
    }
    /* Bail out and use the current working directory. */
    return GetTempFileName(".", "TCL", 0, name);
}

/*
 *---------------------------------------------------------------------------
 *
 * OpenRedirectFile --
 *
 *      Open a file for use in a pipeline.
 *
 * Results:
 *      Returns a new handle or NULL on failure.
 *
 * Side effects:
 *      May cause a file to be created on the file system.
 *
 *---------------------------------------------------------------------------
 */
static HANDLE
OpenRedirectFile(const char *path, DWORD accessFlags, DWORD createFlags)
{
    HANDLE hFile;
    DWORD attribFlags;

    attribFlags = 0;
    if (createFlags & (TRUNCATE_EXISTING | OPEN_EXISTING)) {
        attribFlags = GetFileAttributes(path);
        if (attribFlags == 0xFFFFFFFF) {
            attribFlags = 0;
        }
    }
    hFile = CreateFile(path,
        accessFlags,                    /* Access mode flags */
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,                           /* No security */
        createFlags,                    /* Creation attributes */
        attribFlags,                    /* File attribute flags */
        NULL);                          /* Template file */

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD lastError;

        lastError = GetLastError();
        if ((lastError & 0xffffL) == ERROR_OPEN_FAILED) {
            lastError = (createFlags & (TRUNCATE_EXISTING | OPEN_EXISTING)) 
                ? ERROR_FILE_NOT_FOUND : ERROR_FILE_EXISTS;
        }
        TclWinConvertError(lastError);
        return INVALID_HANDLE_VALUE;
    }
    /*
     * Seek to the end of file if we are writing.
     */
    if (accessFlags & GENERIC_WRITE) {
        SetFilePointer(hFile, 0, NULL, FILE_END);
    }
    return hFile;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateTempFile --
 *
 *      This function creates a temporary file initialized with an optional
 *      string, and returns a file handle with the file pointer at the
 *      beginning of the file.
 *
 * Results:
 *      A handle to a file.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static HANDLE
CreateTempFile(const char *data)        /* String to write into temp file, or
                                         *  NULL. */
{
    char fileName[MAX_PATH + 1];
    HANDLE hFile;
    DWORD lastError;

    if (!TempFileName(fileName)) {
        return INVALID_HANDLE_VALUE;
    }
    hFile = CreateFile(
        fileName,                       /* File path */
        GENERIC_READ | GENERIC_WRITE,   /* Access mode */
        0,                              /* No sharing. */
        NULL,                           /* Security attributes */
        CREATE_ALWAYS,                  /* Overwrite any existing file */
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
        NULL);                          /* No template file */

    if (hFile == INVALID_HANDLE_VALUE) {
        goto error;
    }
    if (data != NULL) {
        DWORD result;
        int length;
        const char *p;
        const char *string;

        string = data;
        for (p = string; *p != '\0'; p++) {
            if (*p == '\n') {
                length = p - string;
                if (length > 0) {
                    if (!WriteFile(hFile, string, length, &result, NULL)) {
                        goto error;
                    }
                }
                if (!WriteFile(hFile, "\r\n", 2, &result, NULL)) {
                    goto error;
                }
                string = p + 1;
            }
        }
        length = p - string;
        if (length > 0) {
            if (!WriteFile(hFile, string, length, &result, NULL)) {
                goto error;
            }
        }
        if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == (DWORD) - 1) {
            goto error;
        }
    }
    return hFile;

  error:
    lastError = GetLastError();
    CloseHandle(hFile);
    DeleteFile(fileName);               /* Do I need this? Delete on close? */
    TclWinConvertError(lastError);
    return INVALID_HANDLE_VALUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * HasConsole --
 *
 *      Determines whether the current application is attached to a console.
 *
 * Results:
 *      Returns TRUE if this application has a console, else FALSE.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static BOOL
HasConsole(void)
{
    HANDLE hFile;

    hFile = CreateFileA("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    CloseHandle(hFile);
    return TRUE;
}

static ApplicationType
GetApplicationType(const char *file, char *cmdPrefix)
{
    char *dot;
    HANDLE hFile;
    IMAGE_DOS_HEADER imageDosHeader;
    ULONG signature;
    BOOL result;
    DWORD offset;
    DWORD numBytes;
    ApplicationType type;

    dot = strrchr(file, '.');
    if ((dot != NULL) && (strcasecmp(dot, ".bat") == 0)) {
        return APPL_DOS;
    }
    /* Work a little harder. Open the binary and read the header */
    hFile = CreateFileA(file, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return APPL_NONE;
    }
    type = APPL_NONE;
    result = ReadFile(hFile, &imageDosHeader, sizeof(IMAGE_DOS_HEADER),
        &numBytes, NULL);
    if ((!result) || (numBytes != sizeof(IMAGE_DOS_HEADER))) {
        goto done;
    }
    if (imageDosHeader.e_magic == 0x2123) {     /* #! */
        char *p;
        unsigned int i;

        offset = SetFilePointer(hFile, 2, NULL, FILE_BEGIN);
        if (offset == INVALID_SET_FILE_POINTER) {
            goto done;
        }
        result = ReadFile(hFile, cmdPrefix, MAX_PATH + 1, &numBytes, NULL);
        if ((!result) || (numBytes < 1)) {
            goto done;
        }
        for (p = cmdPrefix, i = 0; i < numBytes; i++, p++) {
            if ((*p == '\n') || (*p == '\r')) {
                break;
            }
        }
        *p = '\0';
        type = APPL_INTERP;
        goto done;
    }
    /*
     * Doesn't have the magic number for relocatable executables.  If filename
     * ends with .com, assume it's a DOS application anyhow.  Note that we
     * didn't make this assumption at first, because some supposed .com files
     * are really 32-bit executables with all the magic numbers and
     * everything.
     */
    if ((dot != NULL) && (strcmp(dot, ".com") == 0)) {
        type = APPL_DOS;
        goto done;
    }
    if (imageDosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
    }
    if (imageDosHeader.e_lfarlc != sizeof(IMAGE_DOS_HEADER)) {
        /* This assumes that all 3.x and Win32 programs have their file
         * relocation table immediately following this header. */
        /*
         * All Windows 3.X and Win32 and some DOS programs have this value set
         * here.  If it doesn't, assume that since it already had the other
         * magic number it was a DOS application.
         */
        type = APPL_DOS;
        goto done;
    }
    offset = SetFilePointer(hFile, imageDosHeader.e_lfanew, NULL, FILE_BEGIN);
    if (offset == INVALID_SET_FILE_POINTER) {
        goto done;
    }
    result = ReadFile(hFile, &signature, sizeof(ULONG), &numBytes, NULL);
    if ((!result) || (numBytes != sizeof(ULONG))) {
        goto done;
    }
    switch (signature) {
    case IMAGE_NT_SIGNATURE:
        type = APPL_WIN32;
        break;
    case IMAGE_OS2_SIGNATURE:
        type = APPL_WIN3X;
        break;
    case IMAGE_VXD_SIGNATURE:
        type = APPL_WIN32;
        break;
    default:
        type = APPL_DOS;
        break;
    }
  done:
    CloseHandle(hFile);
    return type;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetFullPath --
 *
 *      Look for the program as an external program.  First try the name as it
 *      is, then try adding .com, .exe, and .bat, in that order, to the name,
 *      looking for an executable.
 *
 *      Using the raw SearchPath() procedure doesn't do quite what is
 *      necessary.  If the name of the executable already contains a '.'
 *      character, it will not try appending the specified extension when
 *      searching (in other words, SearchPath will not find the program
 *      "a.b.exe" if the arguments specified "a.b" and ".exe").  So, first
 *      look for the file as it is named.  Then manually append extensions,
 *      looking for a match.
 *
 * Results:
 *      Always returns TCL_OK.
 *
 * Side Effects:
 *
 *---------------------------------------------------------------------------
 */
static int
GetFullPath(
    Tcl_Interp *interp,                 /* Interpreter to report errors to */
    const char *program,                /* Name of program. */
    char *fullPath,                     /* (out) Returned full path. */
    char *cmdPrefix,                    /* (out) If program is a script, this
                                         * contains the name of the
                                         * interpreter. */
    ApplicationType *typePtr)
{                                       /* (out) Type of program */
    TCHAR *rest;
    DWORD attr;
    int length;
    char cmd[MAX_PATH + 5];
    const char **p;
    char *ext;

    static const char *dosExts[] =
    {
        "", ".com", ".exe", ".bat", ".cmd", NULL
    };

    *typePtr = APPL_NONE;

    length = strlen(program);
    strcpy(cmd, program);
    cmdPrefix[0] = '\0';
    ext = cmd + length;
    for (p = dosExts; *p != NULL; p++) {
        *ext = '\0';                    /* Reset to original program name. */
        strcpy(ext, *p);                /* Append the DOS extension to the
                                         * program name. */

        if (!SearchPath(
                NULL,                   /* Use standard Windows search
                                         * paths */
                cmd,                    /* Program name */
                NULL,                   /* Extension provided by program
                                         * name. */
                MAX_PATH,               /* Buffer size */
                fullPath,               /* Buffer for absolute path of
                                         * program */
                &rest)) {
            continue;                   /* Can't find program with that
                                         * extension */
        }
        /*
         * Ignore matches on directories or data files.  Return when we
         * identify a known program type.
         */
        attr = GetFileAttributesA(fullPath);
        if ((attr == INVALID_FILE_ATTRIBUTES) ||
            (attr & FILE_ATTRIBUTE_DIRECTORY)) {
            continue;
        }
        *typePtr = GetApplicationType(fullPath, cmdPrefix);
        if (*typePtr != APPL_NONE) {
            break;
        }
    }
    if (*typePtr == APPL_NONE) {
        /*
         * Can't find the program.  Check if it's an internal shell command
         * like "copy" or "dir" and let cmd.exe deal with it.
         */
        static const char *shellCmds[] =
        {
            "copy", "del", "dir", "echo", "edit", "erase", "label",
            "md", "rd", "ren", "start", "time", "type", "ver", "vol", NULL
        };

        for (p = shellCmds; *p != NULL; p++) {
            if (((*p)[0] == program[0]) && (strcmp(*p, program) == 0)) {
                break;
            }
        }
        if (*p == NULL) {
            Tcl_AppendResult(interp, "can't execute \"", program, 
                             "\": no such file or directory", (char *)NULL);
            return TCL_ERROR;
        }
        *typePtr = APPL_DOS;
        strcpy(fullPath, program);
    }
    if ((*typePtr == APPL_DOS) || (*typePtr == APPL_WIN3X)) {

        /* For 16-bit applications, convert the long executable path name to a
         * short one.  Otherwise the application may not be able to correctly
         * parse its own command line.  */
        GetShortPathName(fullPath, fullPath, MAX_PATH);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConcatCmdArgs --
 *
 *      Concatenates command line arguments parsed from TCL into a single
 *      string.  If an argument contain spaces, it is grouped with surrounding
 *      double quotes. Must also escape any quotes we find.
 *
 * Results:
 *      Returns a malloc-ed string containing the concatenated command
 *      line.
 *
 *---------------------------------------------------------------------------
 */
static char *
ConcatCmdArgs(
    Tcl_Interp *interp,
    int argc, 
    char **argv, 
    Tcl_DString *resultPtr)
{
    BOOL needQuote;
    const char *s;
    char *cp;
    char *string;                       /* Will contain the new command
                                         * line */
    int count;
    int i;

    /*
     * Pass 1.  Compute how much space we need for an array to hold the entire
     *          command line.  Then allocate the string.
     */
    count = 0;
    for (i = 0; i < argc; i++) {
        needQuote = FALSE;
        if (*argv[i] == '\0') {
            needQuote = TRUE;           /* Zero length args also need
                                         * quotes. */
        }
        for (s = argv[i]; *s != '\0'; s++) {
            if (*s == '"') {
                const char *bp;

                count++;                /* +1 Backslash needed to escape
                                         * quote */
                for (bp = s - 1; (*bp == '\\') && (bp >= argv[i]); bp--) {
                    count++;    /* +? one for each preceding backslash */
                }
            } else if (isspace(*s)) {
                needQuote = TRUE;
            }
            count++;                    /* +1 Normal character */
        }
        if (needQuote) {
            count += 2;                 /* +2 Pair of quotes */
        }
        count++;                        /* +1 Space separating arguments */
    }

    string = Blt_AssertMalloc(count + 1);
    /*
     * Pass 2.  Copy the arguments, quoting arguments with embedded spaces and
     *          escaping all other quotes in the string.
     */
    cp = string;
    for (i = 0; i < argc; i++) {
        needQuote = FALSE;

        if (*argv[i] == '\0') {
            needQuote = TRUE;
        }
        for (s = argv[i]; *s != '\0'; s++) {
            if (isspace(*s)) {
                needQuote = TRUE;
            }
        }
        if (needQuote) {
            *cp++ = '"';
        }
        for (s = argv[i]; *s; s++) {
            if (*s == '"') {
                const char *bp;

                for (bp = s - 1; (*bp == '\\') && (bp >= argv[i]); bp--) {
                    *cp++ = '\\';
                }
                *cp++ = '\\';
            }
            *cp++ = *s;
        }
        if (needQuote) {
            *cp++ = '"';
        }
        *cp++ = ' ';
    }
    *cp = '\0';
    assert((cp - string) == count);

#if (_TCL_VERSION >= _VERSION(8,1,0)) 
    {
        Tcl_DString ds;
        Tcl_Encoding encoding;

        /* Convert to external encoding */
        encoding = Tcl_GetEncoding(interp, NULL);
        Tcl_UtfToExternalDString(encoding, string, count, &ds);
        Tcl_DStringAppend(resultPtr, Tcl_DStringValue(&ds), -1);
        Tcl_DStringFree(&ds);
    }
#else
    Tcl_DStringAppend(resultPtr, string, count);
#endif 
    Blt_Free(string);
    return Tcl_DStringValue(resultPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * StartProcess --
 *
 *      Create a child process that has the specified files as its standard
 *      input, output, and error.
 *
 *      The complete Windows search path is searched to find the specified
 *      executable.  If an executable by the given name is not found,
 *      automatically tries appending ".com", ".exe", and ".bat" to the
 *      executable name.
 *
 * Results:
 *      The return value is TCL_ERROR and an error message is left in
 *      the interp's result if there was a problem creating the child
 *      process.  Otherwise, the return value is TCL_OK and *pidPtr is
 *      filled with the process id of the child process.
 *
 * Side effects:
 *      A process is created.
 *
 *---------------------------------------------------------------------------
 */
static int
StartProcess(
    Tcl_Interp *interp,                 /* Interpreter to report errors that
                                         * occurred when creating the child
                                         * process.  Error messages from the
                                         * child process itself are sent to
                                         * errorFile. */
    int argc,                           /* # of arguments. */
    char **argv,                        /* Command line arguments. */
    HANDLE hStdin,                      /* File handle to use as input (stdin)
                                         * for the child process. If handle is
                                         * -1, no * standard input. */
    HANDLE hStdout,                     /* File handle to receive output
                                         * (stdout) from the child process.
                                         * If -1, output is discarded. */
    HANDLE hStderr,                     /* File handle to receive errors
                                         * (stderr) from the child process.
                                         * If -1, stderr will be
                                         * discarded. Can be the same handle
                                         * as hStdOut */
    char *const *env,
    HANDLE *hProcessPtr,                /* (out) Handle of child process. */
    DWORD *pidPtr)                      /* (out) Id of child process. */
{
    int result, createFlags;
    ApplicationType applType;
    Tcl_DString ds;                     /* Complete command line */
    char *command;
    BOOL hasConsole;
#ifndef notdef
    DWORD idleResult;
#endif
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES securityAttrs;
    HANDLE hProcess, hPipe;
    char progPath[MAX_PATH];
    char cmdPrefix[MAX_PATH];
    
    *pidPtr = 0;
    *hProcessPtr = INVALID_HANDLE_VALUE;
    GetFullPath(interp, argv[0], progPath, cmdPrefix, &applType);
    if (applType == APPL_NONE) {
        return TCL_ERROR;
    }
    result = TCL_ERROR;

    hProcess = GetCurrentProcess();

    ZeroMemory(&si, sizeof(STARTUPINFOA));
    si.cb = sizeof(STARTUPINFOA);

    /*
     * The flag STARTF_USESTDHANDLES must be set to pass handles to the child
     * process.  Using SetStdHandle and/or dup2 works only when a console mode
     * parent process is spawning an attached console mode child process.
     */

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = si.hStdOutput = si.hStdError = INVALID_HANDLE_VALUE;

    securityAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttrs.lpSecurityDescriptor = NULL;
    securityAttrs.bInheritHandle = TRUE;

    /*
     * Duplicate all the handles to be passed off as stdin, stdout and stderr
     * of the child process. The duplicate handles are set to be inheritable,
     * so the child process can use them.
     */
    if (hStdin == INVALID_HANDLE_VALUE) {
        /*
         * If handle was not set, stdin should return immediate EOF.  Under
         * Windows95, some applications (both 16 and 32 bit!)  can't read from
         * the NUL device; they read from console instead.  When running tk,
         * this is fatal because the child process would hang forever waiting
         * for EOF from the unmapped console window used by the helper
         * application.
         *
         * Fortunately, the helper application detects a closed pipe as an
         * immediate EOF and can pass that information to the child process.
         */
        if (CreatePipe(&si.hStdInput, &hPipe, &securityAttrs, 0)) {
            CloseHandle(hPipe);
	    hPipe = INVALID_HANDLE_VALUE;
        }
    } else {
        DuplicateHandle(hProcess, hStdin, hProcess, &si.hStdInput, 0, TRUE,
            DUPLICATE_SAME_ACCESS);
    }

    if (si.hStdInput == INVALID_HANDLE_VALUE) {
        Tcl_AppendResult(interp, "can't duplicate input handle: ",
            Blt_LastError(), (char *)NULL);
        goto closeHandles;
    }
    if (hStdout == INVALID_HANDLE_VALUE) {
        /*
         * If handle was not set, output should be sent to an infinitely deep
         * sink.  Under Windows 95, some 16 bit applications cannot have
         * stdout redirected to NUL; they send their output to the console
         * instead.  Some applications, like "more" or "dir /p", when
         * outputting multiple pages to the console, also then try and read
         * from the console to go the next page.  When running tk, this is
         * fatal because the child process would hang forever waiting for
         * input from the unmapped console window used by the helper
         * application.
         *
         * Fortunately, the helper application will detect a closed pipe as a
         * sink.
         */
        if ((Blt_GetPlatformId() == VER_PLATFORM_WIN32_WINDOWS)
            && (applType == APPL_DOS)) {
            if (CreatePipe(&hPipe, &si.hStdOutput, &securityAttrs, 0)) {
                CloseHandle(hPipe);
	        hPipe = INVALID_HANDLE_VALUE;
            }
        } else {
            si.hStdOutput = CreateFileA("NUL:", GENERIC_WRITE, 0,
                &securityAttrs, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        }
    } else {
        DuplicateHandle(hProcess, hStdout, hProcess, &si.hStdOutput, 0, TRUE,
            DUPLICATE_SAME_ACCESS);
    }
    if (si.hStdOutput == INVALID_HANDLE_VALUE) {
        Tcl_AppendResult(interp, "can't duplicate output handle: ",
            Blt_LastError(), (char *)NULL);
        goto closeHandles;
    }
    if (hStderr == INVALID_HANDLE_VALUE) {
        /*
         * If handle was not set, errors should be sent to an infinitely deep
         * sink.
         */
        si.hStdError = CreateFileA("NUL:", GENERIC_WRITE, 0,
            &securityAttrs, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    } else {
        DuplicateHandle(hProcess, hStderr, hProcess, &si.hStdError, 0, TRUE,
            DUPLICATE_SAME_ACCESS);
    }
    if (si.hStdError == INVALID_HANDLE_VALUE) {
        Tcl_AppendResult(interp, "can't duplicate error handle: ",
            Blt_LastError(), (char *)NULL);
        goto closeHandles;
    }
    Tcl_DStringInit(&ds);
    createFlags = 0;
    hasConsole = HasConsole();
    if (!hasConsole) {
        createFlags |= DETACHED_PROCESS;
    }
    /*
     * If we do not have a console window, then we must run DOS and WIN32
     * console mode applications as detached processes. This tells the loader
     * that the child application should not inherit the console, and that it
     * should not create a new console window for the child application.  The
     * child application should get its stdio from the redirection handles
     * provided by this application, and run in the background.
     *
     * If we are starting a GUI process, they don't automatically get a
     * console, so it doesn't matter if they are started as foreground or
     * detached processes.  The GUI window will still pop up to the
     * foreground.
     */
    if (applType == APPL_DOS) {
        if (Blt_GetPlatformId() == VER_PLATFORM_WIN32_NT) {
            /*
             * Under NT, 16-bit DOS applications will not run unless they can
             * be attached to a console.  If we are running without a console,
             * run the 16-bit program as an normal process inside of a hidden
             * console application, and then run that hidden console as a
             * detached process.
             */
            si.wShowWindow = SW_HIDE;
            si.dwFlags |= STARTF_USESHOWWINDOW;
            createFlags = CREATE_NEW_CONSOLE;
            Tcl_DStringAppend(&ds, "cmd.exe /c ", -1);
        } else {
            /*
             * Under Windows 95, 16-bit DOS applications do not work well
             * with pipes:
             *
             * 1. EOF on a pipe between a detached 16-bit DOS application
             *    and another application is not seen at the other
             *    end of the pipe, so the listening process blocks forever on
             *    reads.  This inablity to detect EOF happens when either a
             *    16-bit app or the 32-bit app is the listener.
             *
             * 2. If a 16-bit DOS application (detached or not) blocks when
             *    writing to a pipe, it will never wake up again, and it
             *    eventually brings the whole system down around it.
             *
             * The 16-bit application is run as a normal process inside of a
             * hidden helper console app, and this helper may be run as a
             * detached process.  If a stdio handle is a pipe, the helper
             * application accumulates information into temp files and
             * forwards it to or from the DOS application as appropriate.
             * This means that DOS apps must receive EOF from a stdin pipe
             * before they will actually begin, and must finish generating
             * stdout or stderr before the data will be sent to the next stage
             * of the pipe.
             *
             * The helper app should be located in the same directory as the
             * tcl dll.
             */
            if (!hasConsole) {
                si.wShowWindow = SW_HIDE;
                si.dwFlags |= STARTF_USESHOWWINDOW;
                createFlags = CREATE_NEW_CONSOLE;
            }
            Tcl_DStringAppend(&ds, "tclpip" STRINGIFY(TCL_MAJOR_VERSION)
                STRINGIFY(TCL_MINOR_VERSION) ".dll ", -1);
        }
    } else if (applType == APPL_INTERP) {
        Tcl_DStringAppend(&ds, cmdPrefix, -1);
        Tcl_DStringAppend(&ds, " ", -1);
    }
    argv[0] = progPath;

    command = ConcatCmdArgs(interp, argc, argv, &ds);
    result = CreateProcess(
        NULL,                           /* Module name. */
        (TCHAR *)command,               /* Command line */
        NULL,                           /* Process security */
        NULL,                           /* Thread security */
        TRUE,                           /* Inherit handles */
        createFlags,                    /* Creation flags */
        (char **)env,                            /* Environment */
        NULL,                           /* Current working directory */
        &si,                            /* Initialization for process:
                                         * includes standard handles,
                                         * appearance and location of
                                         * window */
        &pi);                           /* (out) Information about newly
                                         * created process */
    Tcl_DStringFree(&ds);
#ifdef notdef
    if (env != NULL) {
        Blt_Free(env);
    }
#endif
    if (!result) {
        Tcl_AppendResult(interp, "can't execute \"", argv[0], "\": ",
            Blt_LastError(), (char *)NULL);
        goto closeHandles;
    }
    if (applType == APPL_DOS) {
        /* Force the OS to give some time to the DOS process. */
        WaitForSingleObject(hProcess, 50);
    }
#ifndef notdef                           
    /* FIXME: I don't think this actually ever worked. WaitForInputIdle
     * usually fails with "Access is denied" (maybe the process handle isn't
     * valid yet?).  When you add a delay, WaitForInputIdle will time out
     * instead. */

    /*
     * PSS ID Number: Q124121
     *
     *    "When an application spawns a process repeatedly, a new thread
     *    instance will be created for each process but the previous instances
     *    may not be cleaned up.  This results in a significant virtual memory
     *    loss each time the process is spawned.  If there is a
     *    WaitForInputIdle() call between CreateProcess() and CloseHandle(),
     *    the problem does not occur."  */
    idleResult = WaitForInputIdle(pi.hProcess, 1000);
    if (idleResult == WAIT_FAILED) {
    }
#endif
    CloseHandle(pi.hThread);

    *hProcessPtr = pi.hProcess;

    /* Add the entry to mapping table. Its purpose is to translate process
     * handles to process ids. Most things we do with the Win32 API take
     * handles, but we still want to present process ids to the user.  */
    *pidPtr = pi.dwProcessId;
    result = TCL_OK;

  closeHandles:
    if (si.hStdInput != INVALID_HANDLE_VALUE) {
        CloseHandle(si.hStdInput);
	si.hStdInput = INVALID_HANDLE_VALUE;
    }
    if (si.hStdOutput != INVALID_HANDLE_VALUE) {
        CloseHandle(si.hStdOutput);
	si.hStdOutput = INVALID_HANDLE_VALUE;
    }
    if (si.hStdError != INVALID_HANDLE_VALUE) {
        CloseHandle(si.hStdError);
	si.hStdError = INVALID_HANDLE_VALUE;
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * FileForRedirect --
 *
 *      This procedure does much of the work of parsing redirection operators.
 *      It handles "@" if specified and allowed, and a file name, and opens
 *      the file if necessary.
 *
 * Results:
 *      The return value is the descriptor number for the file.  If an error
 *      occurs then NULL is returned and an error message is left in
 *      interp->result.  Several arguments are side-effected; see the argument
 *      list below for details.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static HANDLE
FileForRedirect(
    Tcl_Interp *interp,         /* Intepreter to use for error reporting. */
    char *spec,                 /* Points to character just after
                                 * redirection character. */
    BOOL atOK,                  /* Non-zero means that '@' notation can be
                                 * used to specify a channel, zero means that
                                 * it isn't. */
    char *arg,                  /* Pointer to entire argument containing
                                 * spec:  used for error reporting. */
    char *nextArg,              /* Next argument in argc/argv array, if needed
                                 * for file name or channel name.  May be
                                 * NULL. */
    DWORD accessFlags,          /* Flags to use for opening file or to
                                 * specify mode for channel. */
    DWORD createFlags,          /* Flags to use for opening file or to
                                 * specify mode for channel. */
    int *skipPtr,               /* Filled with 1 if redirection target was
                                 * in spec, 2 if it was in nextArg. */
    int *closePtr)              /* Filled with one if the caller should
                                 * close the file when done with it, zero
                                 * otherwise. */
{
    int writing = (accessFlags & GENERIC_WRITE);
    Tcl_Channel chan;
    HANDLE hFile;

    *skipPtr = 1;
    *closePtr = FALSE;
    if ((atOK) && (*spec == '@')) {
        spec++;
        if (*spec == '\0') {
            spec = nextArg;
            if (spec == NULL) {
                goto badLastArg;
            }
            *skipPtr = 2;
        }
        chan = Tcl_GetChannel(interp, spec, NULL);
        if (chan == NULL) {
            return INVALID_HANDLE_VALUE;
        }
        if (Tcl_GetChannelHandle(chan, (writing) ? TCL_WRITABLE : TCL_READABLE,
                (ClientData *)&hFile) != TCL_OK) {
            hFile = INVALID_HANDLE_VALUE;
        }
        if (hFile == INVALID_HANDLE_VALUE) {
            Tcl_AppendResult(interp, "channel \"", Tcl_GetChannelName(chan),
                "\" wasn't opened for ",
                ((writing) ? "writing" : "reading"), (char *)NULL);
            return INVALID_HANDLE_VALUE;
        }
        if (writing) {
            /* Be sure to flush output to the file, so that anything written
             * by the child appears after stuff we've already written. */
            Tcl_Flush(chan);
        }
    } else {
        char *name;
        Tcl_DString ds;

        if (*spec == '\0') {
            spec = nextArg;
            if (spec == NULL) {
                goto badLastArg;
            }
            *skipPtr = 2;
        }
        name = Tcl_TranslateFileName(interp, spec, &ds);
        if (name != NULL) {
            hFile = OpenRedirectFile(name, accessFlags, createFlags);
        } else {
            hFile = INVALID_HANDLE_VALUE;
        }
        Tcl_DStringFree(&ds);

        if (hFile == INVALID_HANDLE_VALUE) {
            Tcl_AppendResult(interp, "can't ", (writing) ? "write" : "read",
                " file \"", spec, "\": ", Tcl_PosixError(interp),
                (char *)NULL);
            return INVALID_HANDLE_VALUE;
        }
        *closePtr = TRUE;
    }
    return hFile;

  badLastArg:
    Tcl_AppendResult(interp, "can't specify \"", arg,
        "\" as last word in command", (char *)NULL);
    return INVALID_HANDLE_VALUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreatePipeline --
 *
 *      Given an argc/argv array, instantiate a pipeline of processes as
 *      described by the argv.
 *
 * Results:
 *      The return value is a count of the number of new processes created, or
 *      -1 if an error occurred while creating the pipeline.  *pidArrayPtr is
 *      filled in with the address of a dynamically allocated array giving the
 *      ids of all of the processes.  It is up to the caller to free this
 *      array when it isn't needed anymore.  If inPipePtr is non-NULL,
 *      *inPipePtr is filled in with the file id for the input pipe for the
 *      pipeline (if any): the caller must eventually close this file.  If
 *      outPipePtr isn't NULL, then *outPipePtr is filled in with the file id
 *      for the output pipe from the pipeline: the caller must close this
 *      file.  If errPipePtr isn't NULL, then *errPipePtr is filled with a
 *      file id that may be used to read error output after the pipeline
 *      completes.
 *
 * Side effects:
 *      Processes and pipes are created.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_CreatePipeline(
    Tcl_Interp *interp,         /* Interpreter to use for error reporting. */
    int objc,                   /* Number of entries in objv. */
    Tcl_Obj *const *objv,       /* Array of strings describing commands in
                                 * pipeline plus I/O redirection with <,
                                 * <<,  >, etc.  Objv[objc] must be NULL. */
    Blt_Pid **pidsPtr,          /* *pidsPtr gets filled in with
                                 * address of array of pids for processes
                                 * in pipeline (first pid is first process
                                 * in pipeline). */
    void *inPipePtr,            /* If non-NULL, input to the pipeline comes
                                 * from a pipe (unless overridden by
                                 * redirection in the command).  The file
                                 * id with which to write to this pipe is
                                 * stored at *inPipePtr.  NULL means command
                                 * specified its own input source. */
    void *outPipePtr,           /* If non-NULL, output to the pipeline goes
                                 * to a pipe, unless overriden by redirection
                                 * in the command.  The file id with which to
                                 * read frome this pipe is stored at
                                 * *outPipePtr.  NULL means command specified
                                 * its own output sink. */
    void *errPipePtr,            /* If non-NULL, all stderr output from the
                                 * pipeline will go to a temporary file
                                 * created here, and a descriptor to read
                                 * the file will be left at *errPipePtr.
                                 * The file will be removed already, so
                                 * closing this descriptor will be the end
                                 * of the file.  If this is NULL, then
                                 * all stderr output goes to our stderr.
                                 * If the pipeline specifies redirection
                                 * then the file will still be created
                                 * but it will never get any data. */
    char *const *env)
{
    Blt_Pid *pids = NULL;       /* Points to malloc-ed array holding all
                                 * the handles of child processes. */
    int numPids;                        /* Actual number of processes that exist
                                 * at *pids right now. */
    int cmdCount;               /* Count of number of distinct commands
                                 * found in objc/argv. */
    char *inputLiteral = NULL;  /* If non-null, then this points to a
                                 * string containing input data (specified
                                 * via <<) to be piped to the first process
                                 * in the pipeline. */
    BOOL atOK, needCmd;
    FileInfo in, out, err;
    HANDLE hPipe;
    Tcl_DString execBuffer;
    char **argv;
    char *p;
    int errorToOutput;
    DWORD pid;
    int skip, lastBar, lastArg, i, j, flags;
    HANDLE *hInPipePtr = inPipePtr;
    HANDLE *hOutPipePtr = outPipePtr;
    HANDLE *hErrPipePtr = errPipePtr;

    InitFileInfo(&in);
    InitFileInfo(&out);
    InitFileInfo(&err);
    
    if (hInPipePtr != NULL) {
        *hInPipePtr = NULL;
    }
    if (hOutPipePtr != NULL) {
        *hOutPipePtr = NULL;
    }
    if (hErrPipePtr != NULL) {
        *hErrPipePtr = NULL;
    }
    Tcl_DStringInit(&execBuffer);
    numPids = 0;

    /*
     * First, scan through all the arguments to figure out the structure of
     * the pipeline.  Process all of the input and output redirection
     * arguments and remove them from the argument list in the pipeline.
     * Count the number of distinct processes (it's the number of "|"
     * arguments plus one) but don't remove the "|" arguments because they'll
     * be used in the second pass to seperate the individual child processes.
     * Cannot start the child processes in this pass because the redirection
     * symbols may appear anywhere in the command line -- e.g., the '<' that
     * specifies the input to the entire pipe may appear at the very end of
     * the argument list.
     */

    /* Convert all the Tcl_Objs to strings. */
    argv = Blt_AssertMalloc((objc + 1) *  sizeof(char *));
    for (i = 0; i < objc; i++) {
        argv[i] = Tcl_GetString(objv[i]);
    }
    argv[i] = NULL;

    lastBar = -1;
    cmdCount = 1;
    needCmd = TRUE;
    for (i = 0; i < objc; i++) {
        errorToOutput = 0;
        skip = 0;
        p = argv[i];
        switch (*p++) {
        case '|':
            if (*p == '&') {
                p++;
            }
            if (*p == '\0') {
                if ((i == (lastBar + 1)) || (i == (objc - 1))) {
                    Tcl_AppendResult(interp, 
                                     "illegal use of | or |& in command",
                                     (char *)NULL);
                    goto error;
                }
            }
            lastBar = i;
            cmdCount++;
            needCmd = TRUE;
            break;

        case '<':
            if (in.redirected) {
                Tcl_AppendResult(interp, "ambiguous input redirect.",
                        (char *)NULL);
                goto error;
            }
            if (in.mustClose) {
                in.mustClose = FALSE;
                CloseHandle(in.hFile);
            }
            if (*p == '<') {
                in.hFile = INVALID_HANDLE_VALUE;
                inputLiteral = p + 1;
                skip = 1;
                if (*inputLiteral == '\0') {
		    inputLiteral = argv[i + 1];
                    if (inputLiteral == NULL) {
                        Tcl_AppendResult(interp, "can't specify \"", argv[i],
                            "\" as last word in command", (char *)NULL);
                        goto error;
                    }
                    skip = 2;
                }
            } else {
                inputLiteral = NULL;
                in.hFile = FileForRedirect(interp, p, TRUE, argv[i], argv[i+1],
                        GENERIC_READ, OPEN_EXISTING, &skip, &in.mustClose);
                if (in.hFile == INVALID_HANDLE_VALUE) {
                    goto error;
                }
            }
            in.redirected = TRUE;
            break;

        case '>':
            atOK = TRUE;
            flags = CREATE_ALWAYS;
            if (*p == '>') {
                p++;
                atOK = FALSE;
                flags = OPEN_ALWAYS;
            }
            if (*p == '&') {
                if (err.redirected) {
                    Tcl_AppendResult(interp, "ambiguous error redirect.",
                                     (char *)NULL);
                    goto error;
                }
                if (err.mustClose) {
                    err.mustClose = FALSE;
                    CloseHandle(err.hFile);
                }
                errorToOutput = TRUE;
                p++;
            }
            if (out.redirected) {
                Tcl_AppendResult(interp, "ambiguous output redirect.",
                                 (char *)NULL);
                goto error;
            }
            /*
	     * Close the old output file, but only if the error file is not
	     * also using it.
	     */
            if (out.mustClose) {
                out.mustClose = FALSE;
                if (err.hFile == out.hFile) {
                    err.mustClose = TRUE;
                } else {
                    CloseHandle(out.hFile);
                }
            }
            out.hFile = FileForRedirect(interp, p, atOK, argv[i], argv[i + 1],
                GENERIC_WRITE, flags, &skip, &out.mustClose);
            if (out.hFile == INVALID_HANDLE_VALUE) {
                goto error;
            }
            if (errorToOutput) {
                if (err.mustClose) {
                    err.mustClose = FALSE;
                    CloseHandle(err.hFile);
                }
                err.hFile = out.hFile;
                err.redirected = TRUE;                
            }
            out.redirected = TRUE;
            break;

        case '2':
            if (*p != '>') {
                break;
            }
            p++;
            atOK = TRUE;
            flags = CREATE_ALWAYS;
            if (*p == '>') {
                p++;
                atOK = FALSE;
                flags = OPEN_ALWAYS;
            }
            if (err.redirected) {
                Tcl_AppendResult(interp, "ambiguous error redirect.",
                                 (char *)NULL);
                goto error;
            }
            if (err.mustClose) {
                err.mustClose = FALSE;
                CloseHandle(err.hFile);
            }
 	    if ((atOK) && (p[0] == '@') && (p[1] == '1') && (p[2] == '\0')) {
		/*
		 * Special case handling of 2>@1 to redirect stderr to the
		 * exec/open output pipe as well. This is meant for the end of
		 * the command string, otherwise use |& between commands.
		 */

		if (i != (objc - 1)) {
		    Tcl_AppendResult(interp, "must specify \"", argv[i],
			    "\" as last word in command", NULL);
		    goto error;
		}
		err.hFile = out.hFile;
		errorToOutput = 2;
		skip = 1;
	    } else {
                err.hFile = FileForRedirect(interp, p, atOK, argv[i],
                    argv[i + 1], GENERIC_WRITE, flags, &skip, &err.mustClose);
		if (err.hFile == INVALID_HANDLE_VALUE) {
		    goto error;
		}
	    }
            err.redirected = TRUE;
            break;
    
        default:
            /* Got a command word, not a redirection */
            needCmd = FALSE;
            break;
        }

        if (skip != 0) {
            for (j = i + skip; j < objc; j++) {
                argv[j - skip] = argv[j];
            }
            objc -= skip;
            i -= 1;
        }
    }

    if (needCmd) {
	/* We had a bar followed only by redirections. */

        Tcl_AppendResult(interp, "missing command for \"", argv[0], "\"",
                         (char *)NULL);
	goto error;
    }


    if (in.hFile == INVALID_HANDLE_VALUE) {
        if (inputLiteral != NULL) {
            /*
             * The input for the first process is immediate data coming from
             * Tcl.  Create a temporary file for it and put the data into the
             * file.
             */
            in.hFile = CreateTempFile(inputLiteral);
            if (in.hFile == INVALID_HANDLE_VALUE) {
                Tcl_AppendResult(interp,
                    "can't create input file for command: ",
                    Tcl_PosixError(interp), (char *)NULL);
                goto error;
            }
            in.mustClose = TRUE;
        } else if (hInPipePtr != NULL) {
            /*
             * The input for the first process in the pipeline is to
             * come from a pipe that can be written from by the caller.
             */

            if (!CreatePipe(&in.hFile, &in.hPipe, NULL, 0)) {
                Tcl_AppendResult(interp,
                    "can't create input pipe for command: ",
                    Tcl_PosixError(interp), (char *)NULL);
                goto error;
            }
            in.mustClose = TRUE;
        } else {
            /*
             * The input for the first process comes from stdin.
             */
            in.hFile = GetStdHandle(STD_INPUT_HANDLE);
        }
    }
    if (out.hFile == INVALID_HANDLE_VALUE) {
        if (hOutPipePtr != NULL) {
            /*
             * Output from the last process in the pipeline is to go to a
             * pipe that can be read by the caller.
             */

            if (!CreatePipe(&out.hPipe, &out.hFile, NULL, 0)) {
                Tcl_AppendResult(interp,
                    "can't create output pipe for command: ",
                    Tcl_PosixError(interp), (char *)NULL);
                goto error;
            }
            out.mustClose = TRUE;
        } else {
            /*
             * The output for the last process goes to stdout.
             */
            out.hFile = GetStdHandle(STD_OUTPUT_HANDLE);
        }
    }
    if (err.hFile == INVALID_HANDLE_VALUE) {
    	if (errorToOutput == 2) {
	    /*
	     * Handle 2>@1 special case at end of cmd line.
	     */
	    err.hFile = out.hFile;
        } else if (hErrPipePtr != NULL) {
            /*
             * Stderr from the last process in the pipeline is to go to a
             * pipe that can be read by the caller.
             */
            if (CreatePipe(&err.hPipe, &err.hFile, NULL, 0) == 0) { 
                Tcl_AppendResult(interp,
                    "can't create error pipe for command: ",
                    Tcl_PosixError(interp), (char *)NULL);
                goto error;
            }
            err.mustClose = TRUE;
        } else {
            /*
             * Errors from the pipeline go to stderr.
             */
            err.hFile = GetStdHandle(STD_ERROR_HANDLE);
        }
    }

    /*
     * Scan through the objc array, creating a process for each
     * group of arguments between the "|" characters.
     */

    Tcl_ReapDetachedProcs();
    pids = Blt_AssertMalloc((unsigned)((cmdCount + 1) * sizeof(Blt_Pid)));
    in.hCurrFile = in.hFile;
    if (objc == 0) {
        Tcl_AppendResult(interp, "invalid null command", (char *)NULL);
        goto error;
    }

    lastArg = 0;                        /* Suppress compiler warning */
    for (i = 0; i < objc; i = lastArg + 1) {
        BOOL joinThisError;
        HANDLE hProcess;

        /* Convert the program name into native form. */
        argv[i] = Tcl_TranslateFileName(interp, argv[i], &execBuffer);
        if (argv[i] == NULL) {
            goto error;
        }
        /* Find the end of the current segment of the pipeline. */
        joinThisError = FALSE;
        for (lastArg = i; lastArg < objc; lastArg++) {
            if (argv[lastArg][0] == '|') {
                if (argv[lastArg][1] == '\0') {
                    break;
                }
                if ((argv[lastArg][1] == '&') && (argv[lastArg][2] == '\0')) {
                    joinThisError = TRUE;
                    break;
                }
            }
        }
        argv[lastArg] = NULL;
        if ((lastArg - i) == 0) {
            Tcl_AppendResult(interp, "invalid null command", (char *)NULL);
            goto error;
        }

        /* If this is the last segment, use the specified output handle.
         * Otherwise create an intermediate pipe.  hPipe will become the input
         * for the next segment of the pipe. */
        if (lastArg == objc) {
            out.hCurrFile = out.hFile;
        } else {
            if (CreatePipe(&hPipe, &out.hCurrFile, NULL, 0) == 0) {
                Tcl_AppendResult(interp, "can't create pipe: ",
                    Tcl_PosixError(interp), (char *)NULL);
                goto error;
            }
        }

        err.hCurrFile = (joinThisError) ? out.hCurrFile : err.hFile;

        if (StartProcess(interp, lastArg - i, argv + i, in.hCurrFile,
                out.hCurrFile, err.hCurrFile, env, &hProcess, &pid)
            != TCL_OK) {
            goto error;
        }
        Tcl_DStringFree(&execBuffer);

        pids[numPids].hProcess = hProcess;
        pids[numPids].pid = pid;
        numPids++;

        /* Close off our copies of file descriptors that were set up for this
         * child, then set up the input for the next child. */

        if ((in.hCurrFile != INVALID_HANDLE_VALUE) &&
            (in.hCurrFile != in.hFile)) {
            CloseHandle(in.hCurrFile);
        }
        in.hCurrFile = hPipe;
        hPipe = INVALID_HANDLE_VALUE;

        if ((out.hCurrFile != INVALID_HANDLE_VALUE) &&
            (out.hCurrFile != out.hFile)) {
            CloseHandle(out.hCurrFile);
        }
        out.hCurrFile = INVALID_HANDLE_VALUE;
    }

    *pidsPtr = pids;

    if (hInPipePtr != NULL) {
        *hInPipePtr = in.hPipe;
    }
    if (hOutPipePtr != NULL) {
        *hOutPipePtr = out.hPipe;
    }
    if (hErrPipePtr != NULL) {
        *hErrPipePtr = err.hPipe;
    }
    /*
     * All done.  Cleanup open files lying around and then return.
     */
  cleanup:
    Tcl_DStringFree(&execBuffer);

    if (in.mustClose) {
        CloseHandle(in.hFile);
    }
    if (out.mustClose) {
        CloseHandle(out.hFile);
    }
    if (err.mustClose) {
        CloseHandle(err.hFile);
    }
    if (argv != NULL) {
        Blt_Free(argv);
    }
    return numPids;

    /* An error occurred.  There could have been extra files open, such as
     * pipes between children.  Clean them all up.  Detach any child processes
     * that have been created. */
  error:
    if (hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(hPipe);
	hPipe = INVALID_HANDLE_VALUE;
    }
    if ((out.hCurrFile != INVALID_HANDLE_VALUE) &&
        (out.hCurrFile != out.hFile)) {
        CloseHandle(out.hCurrFile);
	out.hCurrFile = INVALID_HANDLE_VALUE;
    }
    if ((in.hCurrFile != INVALID_HANDLE_VALUE) && (in.hCurrFile != in.hFile)) {
        CloseHandle(in.hCurrFile);
	in.hCurrFile = INVALID_HANDLE_VALUE;
    }
    if (in.hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(in.hPipe);
	in.hPipe = INVALID_HANDLE_VALUE;
    }
    if (out.hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(out.hPipe);
	out.hPipe = INVALID_HANDLE_VALUE;
    }
    if (err.hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(err.hPipe);
	err.hPipe = INVALID_HANDLE_VALUE;
    }
    if (pids != NULL) {
        Blt_DetachPids(numPids, pids);
        Blt_Free(pids);
    }
    numPids = -1;
    goto cleanup;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreateFileHandler --
 *
 *      Limited emulation Tcl_CreateFileHandler for Win32. Works
 *      with pipes. Don't know if anything else will (such as sockets).
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Registers procedure and data to call back when data
 *      is available on the pipe.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_CreateFileHandler(
    HANDLE hFile,                       /* Handle of file */
    int flags,                          /* TCL_READABLE or TCL_WRITABLE  */
    Tcl_FileProc *proc,
    ClientData clientData)
{
    PipeHandler *pipePtr;

    if (!initialized) {
        PipeInit();
    }
    if ((flags != TCL_READABLE) && (flags != TCL_WRITABLE)) {
        return;                 /* Only one of the flags can be set. */
    }
    assert(hFile != INVALID_HANDLE_VALUE);
    
    pipePtr = NewPipeHandler(hFile, flags, proc, clientData);

    /* Add the handler to the list of managed pipes. */
    EnterCriticalSection(&pipeCriticalSection);
    Blt_Chain_Append(pipeChain, pipePtr);
    LeaveCriticalSection(&pipeCriticalSection);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DeleteFileHandler --
 *
 *      Win32 emulation Tcl_DeleteFileHandler.  Cleans up resources
 *      used.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DeleteFileHandler(HANDLE hPipe)     /* Handle of file */
{
    Blt_ChainLink link;

    if (!initialized) {
        PipeInit();
    }
    EnterCriticalSection(&pipeCriticalSection);

    for (link = Blt_Chain_FirstLink(pipeChain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        PipeHandler *pipePtr;

        pipePtr = Blt_Chain_GetValue(link);
        if ((pipePtr->hPipe == hPipe) && !(pipePtr->flags & PIPE_DELETED)) {
            Blt_Chain_DeleteLink(pipeChain, link);
            DeletePipeHandler(pipePtr);
            break;
        }
    }
    LeaveCriticalSection(&pipeCriticalSection);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_AsyncRead --
 *
 *      Reads input from the pipe into the given buffer.
 *
 * Results:
 *      Returns the number of bytes read.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_AsyncRead(HANDLE hFile, char *buffer, size_t count)
{
    PipeHandler *pipePtr;
    int numBytes, numBytesAvail;

    pipePtr = GetPipeHandler(hFile);
    if ((pipePtr == NULL) || (pipePtr->flags & PIPE_DELETED)) {
        errno = EBADF;
        return -1;
    }
    if (!PeekOnPipe(pipePtr, &numBytesAvail)) {
        return -1;                      /* No data available. */
    }
    /*
     * numBytesAvail is 0       EOF found.
     *                  -1      Error occured.
     *                  1+      # of bytes available.
     */
    if (numBytesAvail == -1) {
        return -1;
    }
    if (numBytesAvail == 0) {
        return 0;
    }
    numBytes = pipePtr->end - pipePtr->start;
    if (numBytes > count) {
        numBytes = count; 
    }
    memcpy(buffer, pipePtr->buffer + pipePtr->start, numBytes);
    pipePtr->start += numBytes;
    if (pipePtr->start >= pipePtr->end) {
        ResetEvent(pipePtr->readyEvent);
        SetEvent(pipePtr->idleEvent);
    }
    return numBytes;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_AsyncWrite --
 *
 *      Writes output to the pipe from the given buffer.
 *
 * Results:
 *      Returns the number of bytes written.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_AsyncWrite(HANDLE hFile, const char *buffer, size_t reqNumBytes)
{
    PipeHandler *pipePtr;

    pipePtr = GetPipeHandler(hFile);
    if ((pipePtr == NULL) || (pipePtr->flags & PIPE_DELETED)) {
        errno = EBADF;
        errno = ESRCH;
        return -1;
    }
    if (WaitForSingleObject(pipePtr->readyEvent, 0) == WAIT_TIMEOUT) {
        /*
         * Writer thread is currently blocked waiting for a write to
         * complete.
         */
        errno = EAGAIN;
        return -1;
    }
    /* Check for a background error on the last write. */
    if (pipePtr->lastError) {
        TclWinConvertError(pipePtr->lastError);
        pipePtr->lastError = 0;
        return -1;
    }
    /* Reallocate the buffer to be large enough to hold the data. */
    if (reqNumBytes > pipePtr->size) {
        char *ptr;

        ptr = Blt_AssertMalloc(reqNumBytes);
        Blt_Free(pipePtr->buffer);
        pipePtr->buffer = ptr;
    }
    memcpy(pipePtr->buffer, buffer, reqNumBytes);
    pipePtr->end = pipePtr->size = reqNumBytes;
    ResetEvent(pipePtr->readyEvent);
    SetEvent(pipePtr->idleEvent);
    return reqNumBytes;
}

void
Blt_DetachPids(int numPids, Blt_Pid *pids)
{
    Tcl_Pid *tclPidArr;
    Tcl_Pid staticStorage[64];
    int i, count;

    if (numPids > 64) {
        tclPidArr = Blt_AssertMalloc(numPids * sizeof(Tcl_Pid));
    } else {
        tclPidArr = staticStorage;
    }
    for (i = count = 0; i < numPids; i++) {
        if (pids[i].hProcess != INVALID_HANDLE_VALUE) {
            tclPidArr[count] = (Tcl_Pid)(uintptr_t)pids[i].pid;
            count++;
        }
    }
    Tcl_DetachPids(count, tclPidArr);
    if (tclPidArr != staticStorage) {
        Blt_Free(tclPidArr);
    }
}

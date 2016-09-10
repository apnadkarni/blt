/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltUnixPipe.c --
 *
 * This module implements the former Tcl_CreatePipeline API. This file
 * contains the generic portion of the command channel driver as well
 * as various utility routines used in managing subprocesses.
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
 * This file was adapted from tclPipe.c and tclUnixPipe.c of the Tcl
 * distribution.
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

#define _GNU_SOURCE 1
#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif /* HAVE_UNISTD_H */
  #include <unistd.h>

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_FCNTL_H
  #include <fcntl.h>
#endif  /* HAVE_FCNTL_H */

#ifdef HAVE_SIGNAL_H
  #include <signal.h>
#endif  /* HAVE_SIGNAL_H */

#include "bltAlloc.h"
#include "bltWait.h"

typedef int Tcl_File;

#ifdef MACOSX

/* The following routines are used to emulate the "execvpe" system call for
 * MacOSX. */

/*
 *---------------------------------------------------------------------------
 *
 * GetCwd --
 *
 *      Fills the dynamic string with the current working directory.
 *
 * Results:
 *      Returns 1 if the current working directory was obtained, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
static int
GetCwd(Tcl_DString *resultPtr)
{
    size_t length;
    char *cwd;
    
    length = BUFSIZ;
    do {
        Tcl_DStringSetLength(resultPtr, length);
        cwd = getcwd(Tcl_DStringValue(resultPtr), length);
        if ((cwd == NULL) && (errno != ERANGE)) {
            return FALSE;
        }
        /* Keep doubling space until current working directory fits into
         * the string. */
        length += length;
    } while (cwd == NULL);
    Tcl_DStringSetLength(resultPtr, strlen(cwd));
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * NormalizePath --
 *
 *      Returns the normalized path of the file. 
 *
 * Results:
 *      Returns 1 is the path was normalized or if the path is a single 
 *      component.  *foundPtr* will indicate if the file was found or
 *      not.  This indicates whether we need to be search for it in the
 *      PATH.
 *
 *---------------------------------------------------------------------------
 */
static int
NormalizePath(const char *path, Tcl_DString *resultPtr, int *foundPtr)
{
    char *copy;
    int last, length;
    char *p;
    
    *foundPtr = FALSE;
    Tcl_DStringSetLength(resultPtr, 0);
    if (strchr(path, '/') == NULL) {  /* Path is just the one component. */
        Tcl_DStringAppend(resultPtr, path, -1);
        return TRUE;                  
    }
    if (path[0] == '/') {               /* Absolute path */
        if (access(path, F_OK) == -1) {
            return FALSE;
        }
        Tcl_DStringAppend(resultPtr, path, -1);
        *foundPtr = TRUE;
        return TRUE;
    }
    /* Relative path. */
    /* Load the current loading directory into the buffer. */
    if (!GetCwd(resultPtr)) {
        return FALSE;
    }
    length = Tcl_DStringLength(resultPtr) - 1;
    last = length - 1;
    copy = (char *)Blt_AssertStrdup(path);
    for (p = strtok(copy, "/"); p != NULL; p = strtok(NULL, "/")) {
        if ((p[0] == '.') && (p[1] == '\0')) {
            continue;                   /* Ignore "." */
        }
        if ((p[0] == '.') && (p[1] == '.') && (p[2] == '\0')) {
            char *slash;
            
            slash = strrchr(Tcl_DStringValue(resultPtr), '/');
            if (slash == NULL) {
                errno = ENOENT;
                goto error;             /* Can't find previous component in
                                         * cwd. */
            }
            if (last <= 0) {
                errno = ENOENT;
                goto error;
            }
            last = (slash - Tcl_DStringValue(resultPtr));
            /* Peel off last component from current working directory.  */
            Tcl_DStringSetLength(resultPtr, last);
            continue;
        }
        Tcl_DStringAppend(resultPtr, "/", 1);
        Tcl_DStringAppend(resultPtr, p, -1);
        last = Tcl_DStringLength(resultPtr);
        if (access(Tcl_DStringValue(resultPtr), F_OK) == -1) {
            Tcl_DStringSetLength(resultPtr, 0);
            Tcl_DStringAppend(resultPtr, path, -1);
            return FALSE;
        }
    }
    Blt_Free(copy);
    *foundPtr = TRUE;
    return TRUE;
 error:
    Blt_Free(copy);
    Tcl_DStringSetLength(resultPtr, 0);
    Tcl_DStringAppend(resultPtr, path, -1);
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * FindProgram --
 *
 *      Finds the named program.  If the program is just the name, then in
 *      the PATH environment variable is searched. We only check if the
 *      program is executable when we are looking for it in the PATH.
 *      We'll let execve tell us whether it's executable for paths with
 *      "/".
 *
 * Results:
 *      Return 1 if a program was found. The full path to the program
 *      if returned in *resultPtr*.
 *
 *---------------------------------------------------------------------------
 */
static int
FindProgram(const char *program, Tcl_DString *resultPtr)
{
    int found;
    
    Tcl_DStringInit(resultPtr);
    if (!NormalizePath(program, resultPtr, &found)) {
        return FALSE;
    }
    if (!found) {
        int progLen;
        char *dirs, *copy;
        char *p;
        
        /* Path was a single name like "ls". Search for program in PATH. */
        dirs = getenv("PATH");
        if (dirs == NULL) {
            /* No PATH variable, so no access for program even if it's in the
             * current working directory. */
            errno = ENOENT;
            Tcl_DStringFree(resultPtr);
            return FALSE;
        }
        program = Tcl_DStringValue(resultPtr);
        progLen = Tcl_DStringLength(resultPtr);
        /* Make a copy of the program name because we're going to store the
         * full path in the dynamic string.  */
        program = Blt_AssertStrdup(program);
        copy = (char *)Blt_AssertStrdup(dirs);
        for (p = strtok(copy, ":"); p != NULL; p = strtok(NULL, ":")) {
            Tcl_DStringSetLength(resultPtr, 0);
            Tcl_DStringAppend(resultPtr, p, -1); 
            Tcl_DStringAppend(resultPtr, "/", 1);
            Tcl_DStringAppend(resultPtr, program, progLen);
            if (access(Tcl_DStringValue(resultPtr), X_OK|F_OK) != -1) {
                 found = TRUE;
                 break;
            }
        }    
        Blt_Free(copy);
        Blt_Free(program);
    }
    return found;
}

/*
 *---------------------------------------------------------------------------
 *
 * execvpe --
 *
 *      Emulates the execvpe linux system call for MacOSX.  Emulates the
 *      "p" part by searching for the program in the PATH. Eventually calls
 *      the "execve" system call.
 *
 * Results:
 *      Doesn't return if successful. Errno will contain the id of
 *      the error.
 *
 *---------------------------------------------------------------------------
 */
static int
execvpe(const char *file, char *const *argv, char *const *env)
{
    const char *program;
    Tcl_DString ds;
    int result;
    
    Tcl_DStringInit(&ds);
    if (FindProgram(file, &ds)) {
        program = Tcl_DStringValue(&ds);
    } else {
        program = file;
    }
    result = execve(program, argv, env);
    Tcl_DStringFree(&ds);
    return result;
}

#endif /* MACOSX */

static int
GetFdFromChannel(Tcl_Channel channel, int direction)
{
    long f;
    ClientData clientData;
            
    if (Tcl_GetChannelHandle(channel, direction, &clientData) != TCL_OK) {
        return -1;
    }
    f = (long)clientData;
    return (int)f;
}

/*
 *---------------------------------------------------------------------------
 *
 * OpenFile --
 *
 *      Open a file for use in a pipeline.
 *
 * Results:
 *      Returns a new TclFile handle or NULL on failure.
 *
 * Side effects:
 *      May cause a file to be created on the file system.
 *
 *---------------------------------------------------------------------------
 */

static int
OpenFile(
    const char *fname,                 /* The name of the file to open. */
    int mode)                          /* In what mode to open the file? */
{
    int fd;

    fd = open(fname, mode, 0666);
    if (fd != -1) {
        fcntl(fd, F_SETFD, FD_CLOEXEC);
        /*
         * If the file is being opened for writing, seek to the end so we can
         * append to any data already in the file.
         */
        if (mode & O_WRONLY) {
            lseek(fd, 0, SEEK_END);
        }
        return fd;
    }
    return -1;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateTempFile --
 *
 *      This function creates a temporary file initialized with an
 *      optional string, and returns a file handle with the file pointer
 *      at the beginning of the file.
 *
 * Results:
 *      A handle to a file.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

static int
CreateTempFile(const char *contents)    /* String to write into temp file,
                                         * or NULL. */
{
    char fileName[L_tmpnam];
    int fd;
    size_t length = (contents == NULL) ? 0 : strlen(contents);

    strcpy(fileName, "tmpXXXXXX");
    if (mkstemp(fileName) < 0) {
        panic("can't create temp file");
    }
    fd = OpenFile(fileName, O_RDWR | O_CREAT | O_TRUNC);
    unlink(fileName);

    if ((fd >= 0) && (length > 0)) {
        for (;;) {
            if (write(fd, contents, length) != -1) {
                break;
            } else if (errno != EINTR) {
                close(fd);
                return -1;
            }
        }
        lseek(fd, 0, SEEK_SET);
    }
    return fd;
}

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
        Tcl_AppendResult(interp, "can't create pipe for command: ",
                    Tcl_PosixError(interp), (char *)NULL);
        return TCL_ERROR;
    }
    fcntl(fd[0], F_SETFD, FD_CLOEXEC);
    fcntl(fd[1], F_SETFD, FD_CLOEXEC);

    *inPtr = fd[0];
    *outPtr = fd[1];
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CloseFile --
 *
 *      Implements a mechanism to close a UNIX file.
 *
 * Results:
 *      Returns 0 on success, or -1 on error, setting errno.
 *
 * Side effects:
 *      The file is closed.
 *
 *---------------------------------------------------------------------------
 */
static int
CloseFile(int fd)                       /* File descriptor to be closed. */
{
    if ((fd == 0) || (fd == 1) || (fd == 2)) {
        return 0;                       /* Don't close stdin, stdout or
                                         * stderr. */
    }
    Tcl_DeleteFileHandler(fd);
    return close(fd);
}

/*
 *---------------------------------------------------------------------------
 *
 * RestoreSignals --
 *
 *      This procedure is invoked in a forked child process just before
 *      exec-ing a new program to restore all signals to their default
 *      settings.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Signal settings get changed.
 *
 *---------------------------------------------------------------------------
 */
static void
RestoreSignals(void)
{
#ifdef SIGABRT
    signal(SIGABRT, SIG_DFL);
#endif
#ifdef SIGALRM
    signal(SIGALRM, SIG_DFL);
#endif
#ifdef SIGFPE
    signal(SIGFPE, SIG_DFL);
#endif
#ifdef SIGHUP
    signal(SIGHUP, SIG_DFL);
#endif
#ifdef SIGILL
    signal(SIGILL, SIG_DFL);
#endif
#ifdef SIGINT
    signal(SIGINT, SIG_DFL);
#endif
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_DFL);
#endif
#ifdef SIGQUIT
    signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGSEGV
    signal(SIGSEGV, SIG_DFL);
#endif
#ifdef SIGTERM
    signal(SIGTERM, SIG_DFL);
#endif
#ifdef SIGUSR1
    signal(SIGUSR1, SIG_DFL);
#endif
#ifdef SIGUSR2
    signal(SIGUSR2, SIG_DFL);
#endif
#ifdef SIGCHLD
    signal(SIGCHLD, SIG_DFL);
#endif
#ifdef SIGCONT
    signal(SIGCONT, SIG_DFL);
#endif
#ifdef SIGTSTP
    signal(SIGTSTP, SIG_DFL);
#endif
#ifdef SIGTTIN
    signal(SIGTTIN, SIG_DFL);
#endif
#ifdef SIGTTOU
    signal(SIGTTOU, SIG_DFL);
#endif
}

/*
 *---------------------------------------------------------------------------
 *
 * SetupStdFile --
 *
 *      Set up stdio file handles for the child process, using the
 *      current standard channels if no other files are specified.
 *      If no standard channel is defined, or if no file is associated
 *      with the channel, then the corresponding standard fd is closed.
 *
 * Results:
 *      Returns 1 on success, or 0 on failure.
 *
 * Side effects:
 *      Replaces stdio fds.
 *
 *---------------------------------------------------------------------------
 */
static int
SetupStdFile(
    int fd,                             /* File descriptor to dup, or -1. */
    int type)                           /* One of TCL_STDIN, TCL_STDOUT,
                                         * TCL_STDERR */
{
    int targetFd = 0;                   /* Initializations here needed only
                                         * to */
    int direction = 0;                  /* Prevent warnings about using
                                         * uninitialized variables. */
    switch (type) {
    case TCL_STDIN:
        targetFd = 0;
        direction = TCL_READABLE;
        break;
    case TCL_STDOUT:
        targetFd = 1;
        direction = TCL_WRITABLE;
        break;
    case TCL_STDERR:
        targetFd = 2;
        direction = TCL_WRITABLE;
        break;
    }
    if (fd < 0) {
        Tcl_Channel channel;

        channel = Tcl_GetStdChannel(type);
        if (channel) {
            fd = GetFdFromChannel(channel, direction);
        }
    }
    if (fd >= 0) {
        if (fd != targetFd) {
            if (dup2(fd, targetFd) == -1) {
                return 0;
            }
            /*
             * Must clear the close-on-exec flag for the target FD, since some
             * systems (e.g. Ultrix) do not clear the CLOEXEC flag on the
             * target FD.
             */

            fcntl(targetFd, F_SETFD, 0);
        } else {
            /*
             * Since we aren't dup'ing the file, we need to explicitly clear
             * the close-on-exec flag.
             */
            fcntl(fd, F_SETFD, 0);
        }
    } else {
        close(targetFd);
    }
    return 1;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateProcess --
 *
 *      Create a child process that has the specified files as its standard
 *      input, output, and error.  The child process runs asynchronously and
 *      runs with the same environment variables as the creating process.
 *
 *      The path is searched to find the specified executable.
 *
 * Results:
 *      The return value is TCL_ERROR and an error message is left in
 *      interp->result if there was a problem creating the child process.
 *      Otherwise, the return value is TCL_OK and *pidPtr is filled with the
 *      process id of the child process.
 *
 * Side effects:
 *      A process is created.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
CreateProcess(
    Tcl_Interp *interp,         /* Interpreter in which to leave errors that
                                 * occurred when creating the child process.
                                 * Error messages from the child process
                                 * itself are sent to stderrFd. */
    int argc,                   /* Number of arguments in following array. */
    char **argv,                /* Array of argument strings.  argv[0]
                                 * contains the name of the executable
                                 * converted to native format (using the
                                 * Tcl_TranslateFileName call).  Additional
                                 * arguments have not been converted. */
    int stdinFd,                /* The file to use as input for the child
                                 * process.  If stdinFd file is -1, input is
                                 * read from the standard input channel. If
                                 * the file isn't readable, the child will
                                 * receive no standard input. */
    int stdoutFd,               /* The file that receives output from the
                                 * child process.  If stdoutFd is -1, output
                                 * is sent to the standard output channel.  If
                                 * the file is not writeable, output from the
                                 * child will be discarded. */
    int stderrFd,               /* The file that receives errors from the
                                 * child process.  If stderrFd file is -1,
                                 * errors will be sent to the standard error
                                 * channel. If the file isn't writeable,
                                 * errors from the child will be discarded.
                                 * stderrFd may be the same as stdoutFd. */
    int *pidPtr,                /* (out) If this procedure is successful,
                                 * pidPtr is filled with the process id of the
                                 * child process. */
    char *const *env)
{
#if (_TCL_VERSION >= _VERSION(8,1,0)) 
    Tcl_DString *dsArr;
    Tcl_Encoding encoding;
#endif
    char errSpace[200];
    int errPipeIn, errPipeOut;
    int i;
    int joinThisError, status, fd;
    long pid;
    size_t count;

    errPipeIn = errPipeOut = -1;
    pid = -1;

#if (_TCL_VERSION >= _VERSION(8,1,0)) 
    dsArr = Blt_AssertMalloc(argc * sizeof(Tcl_DString));
    encoding = Tcl_GetEncoding(interp, NULL);
    for(i = 0; i < argc; i++) {
        argv[i] = Tcl_UtfToExternalDString(encoding, argv[i], 
                strlen(argv[i]), dsArr + i);
    }
#endif
    /*
     * Create a pipe that the child can use to return error information if
     * anything goes wrong.
     */
    if (CreatePipe(interp, &errPipeIn, &errPipeOut) != TCL_OK) {
        goto error;
    }
    joinThisError = (stderrFd == stdoutFd);
    pid = fork();
    if (pid == 0) {
        ssize_t numWritten;
        size_t length;
        fd = errPipeOut;

        /*
         * Set up stdio file handles for the child process.
         */
        if (!SetupStdFile(stdinFd, TCL_STDIN) ||
            !SetupStdFile(stdoutFd, TCL_STDOUT) ||
            (!joinThisError && !SetupStdFile(stderrFd, TCL_STDERR)) ||
            (joinThisError &&
                ((dup2(1, 2) == -1) || (fcntl(2, F_SETFD, 0) != 0)))) {
            Blt_FormatString(errSpace, 200, 
                "%dforked process can't set up input/output: ", errno);
            length = strlen(errSpace);
            numWritten = write(fd, errSpace, length);
            assert(numWritten == length);
            _exit(1);
        }
        /*
         * Close the input side of the error pipe.
         */
        RestoreSignals();
        execvpe(argv[0], argv, env);
        Blt_FormatString(errSpace, 200, "%dcan't execute \"%.150s\": ",
                         errno, argv[0]);
        length = strlen(errSpace);
        numWritten = write(fd, errSpace, (size_t)strlen(errSpace));
        assert(numWritten == length);
        _exit(1);
    }
    if (pid == -1) {
        Tcl_AppendResult(interp, "can't fork child process: ",
            Tcl_PosixError(interp), (char *)NULL);
        goto error;
    }

    /*
     * Read back from the error pipe to see if the child started up OK.  The
     * info in the pipe (if any) consists of a decimal errno value followed by
     * an error message.
     */
    CloseFile(errPipeOut);
    errPipeOut = -1;

    fd = errPipeIn;
    count = read(fd, errSpace, (size_t) (sizeof(errSpace) - 1));
    if (count > 0) {
        char *end;

        errSpace[count] = 0;
        errno = strtol(errSpace, &end, 10);
        Tcl_AppendResult(interp, end, Tcl_PosixError(interp), (char *)NULL);
        goto error;
    }
#if (_TCL_VERSION >= _VERSION(8,1,0)) 
    for(i = 0; i < argc; i++) {
        Tcl_DStringFree(dsArr + i);
    }
    Blt_Free(dsArr);
#endif
    CloseFile(errPipeIn);
    *pidPtr = pid;
    return TCL_OK;

  error:
    if (pid != -1) {
        /*
         * Reap the child process now if an error occurred during its startup.
         */
        Tcl_WaitPid((Tcl_Pid)pid, &status, WNOHANG);
    }
    if (errPipeIn >= 0) {
        CloseFile(errPipeIn);
    }
    if (errPipeOut >= 0) {
        CloseFile(errPipeOut);
    }
#if (_TCL_VERSION >= _VERSION(8,1,0)) 
    for(i = 0; i < argc; i++) {
        Tcl_DStringFree(dsArr + i);
    }
    Blt_Free(dsArr);
#endif
    return TCL_ERROR;
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

static int
FileForRedirect(
    Tcl_Interp *interp,         /* Intepreter to use for error reporting. */
    char *spec,                 /* Points to character just after redirection
                                 * character. */
    char *arg,                  /* Pointer to entire argument containing spec:
                                 * used for error reporting. */
    int atOK,                   /* Non-zero means that '@' notation can be
                                 * used to specify a channel, zero means that
                                 * it isn't. */
    char *nextArg,              /* Next argument in argc/argv array, if needed
                                 * for file name or channel name.  May be
                                 * NULL. */
    int flags,                  /* Flags to use for opening file or to specify
                                 * mode for channel. */
    int *skipPtr,               /* (out) Filled with 1 if redirection target
                                 * was in spec, 2 if it was in nextArg. */
    int *closePtr)              /* (out) Filled with one if the caller should
                                 * close the file when done with it, zero
                                 * otherwise. */
{
    int writing = (flags & O_WRONLY);
    int fd;

    *skipPtr = 1;
    if ((atOK != 0) && (*spec == '@')) {
        int direction;
        Tcl_Channel chan;

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
            return -1;
        }
        direction = (writing) ? TCL_WRITABLE : TCL_READABLE;
        fd = GetFdFromChannel(chan, direction);
        if (fd < 0) {
            Tcl_AppendResult(interp, "channel \"", Tcl_GetChannelName(chan),
                "\" wasn't opened for ",
                ((writing) ? "writing" : "reading"), (char *)NULL);
            return -1;
        }
        if (writing) {
            /*
             * Be sure to flush output to the file, so that anything
             * written by the child appears after stuff we've already
             * written.
             */
            Tcl_Flush(chan);
        }
    } else {
        char *name;
        Tcl_DString nameString;

        if (*spec == '\0') {
            spec = nextArg;
            if (spec == NULL) {
                goto badLastArg;
            }
            *skipPtr = 2;
        }
        name = Tcl_TranslateFileName(interp, spec, &nameString);

        if (name != NULL) {
            fd = OpenFile(name, flags);
        } else {
            fd = -1;
        }
        Tcl_DStringFree(&nameString);
        if (fd < 0) {
            Tcl_AppendResult(interp, "can't ",
                ((writing) ? "write" : "read"), " file \"", spec, "\": ",
                Tcl_PosixError(interp), (char *)NULL);
            return -1;
        }
        *closePtr = TRUE;
    }
    return fd;

  badLastArg:
    Tcl_AppendResult(interp, "can't specify \"", arg,
        "\" as last word in command", (char *)NULL);
    return -1;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreatePipeline --
 *
 *      Given an objc/objv array, instantiate a pipeline of processes as
 *      described by the objv.
 *
 * Results:
 *      The return value is a count of the number of new processes created, or
 *      -1 if an error occurred while creating the pipeline.  *pidsPtr is
 *      filled in with the address of a dynamically allocated array giving the
 *      ids of all of the processes.
 *
 *      It is up to the caller to free this array when it isn't needed
 *      anymore.
 *
 *      If stdinPipePtr isn't NULL, then *stdinPipePtr is filled with the file
 *      id for the input pipe for the pipeline (if any): the caller must
 *      eventually close this file.
 *
 *      If stdoutPipePtr isn't NULL, then *stdoutPipePtr is filled with the
 *      file id for the output pipe from the pipeline: the caller must close
 *      this file.
 *
 *      If stderrPipePtr isn't NULL, then *stderrPipePtr is filled with a file
 *      id that may be used to read error output after the pipeline completes.
 *
 * Side effects:
 *      Processes and pipes are created.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_CreatePipeline(
    Tcl_Interp *interp,                 /* Interpreter to use for error
                                         * reporting. */
    int objc,                           /* Number of entries in objv. */
    Tcl_Obj *const *objv,               /* Array of strings describing
                                         * commands in pipeline plus I/O
                                         * redirection * with <, <<, >, etc.
                                         * Objv[objc] must be * NULL. */
    Blt_Pid **pidsPtr,              /* (out) Word at *pidsPtr gets
                                         * filled in with address of array of
                                         * pids for processes in pipeline
                                         * (first pid is first process in
                                         * pipeline). */
    int *stdinPipePtr,                  /* (out) If non-NULL, input to the
                                         * pipeline comes from a pipe (unless
                                         * overridden by redirection in the
                                         * command).  The file id with which
                                         * to write to this pipe is stored at
                                         * *stdinPipePtr.  NULL means command
                                         * specified its own input source. */
    int *stdoutPipePtr,                 /* (out) If non-NULL, output to the
                                         * pipeline goes to a pipe, unless
                                         * overriden by redirection in the
                                         * command.  The file id with which to
                                         * read frome this pipe is stored at
                                         * *stdoutPipePtr.  NULL means command
                                         * specified its own output sink. */
    int *stderrPipePtr,                 /* (out) If non-NULL, all stderr
                                         * output from the pipeline will go to
                                         * a temporary file created here, and
                                         * a descriptor to read the file will
                                         * be left at *stderrPipePtr.  The
                                         * file will be removed already, so
                                         * closing this descriptor will be the
                                         * end of the file.  If this is NULL,
                                         * then all stderr output goes to our
                                         * stderr. If the pipeline specifies
                                         * redirection then the file will
                                         * still be created but it will never
                                         * get any data. */
    char *const *env)
{
    Blt_Pid *pids = NULL;               /* Points to malloc-ed array holding
                                         * all the pids of child processes. */
    int numPids;                        /* Actual number of processes that
                                         * exist at *pids right now. */
    int cmdCount;                       /* Count of number of distinct
                                         * commands found in objc/objv. */
    char *inputLiteral = NULL;          /* If non-null, then this points to a
                                         * string containing input data
                                         * (specified via <<) to be piped to
                                         * the first process in the
                                         * pipeline. */
    char *p;
    int skip, lastBar, lastArg, i, j, atOK, flags, errorToOutput;
    Tcl_DString execBuffer;
    int pipeIn;
    int isOpen[3];
    int curFd[3];               /* If non-zero, then fd should be closed
                                 * when cleaning up. */
    int fd[3];
    
    char **argv;

    fd[0] = fd[1] = fd[2] = -1;
    isOpen[0] = isOpen[1] = isOpen[2] = FALSE;
    if (stdinPipePtr != NULL) {
        *stdinPipePtr = -1;
    }
    if (stdoutPipePtr != NULL) {
        *stdoutPipePtr = -1;
    }
    if (stderrPipePtr != NULL) {
        *stderrPipePtr = -1;
    }
    Tcl_DStringInit(&execBuffer);

    pipeIn = curFd[0] = curFd[1] = curFd[2] = -1;
    numPids = 0;

    /*
     * First, scan through all the arguments to figure out the structure of
     * the pipeline.  Process all of the input and output redirection
     * arguments and remove them from the argument list in the pipeline.
     * Count the number of distinct processes (it's the number of "|"
     * arguments plus one) but don't remove the "|" arguments because they'll
     * be used in the second pass to seperate the individual child processes.
     *
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
    for (i = 0; i < objc; i++) {
        skip = 0;
        p = argv[i];
        switch (*p++) {
        case '\\':
            p++;
            continue;

        case '|':
            if (*p == '&') {
                p++;
            }
            if (*p == '\0') {
                if ((i == (lastBar + 1)) || (i == (objc - 1))) {
                    Tcl_AppendResult(interp, 
                        "illegal use of | or |& in command", (char *)NULL);
                    goto error;
                }
            }
            lastBar = i;
            cmdCount++;
            break;

        case '<':
            if (isOpen[0] != 0) {
                isOpen[0] = FALSE;
                CloseFile(fd[0]);
            }
            if (*p == '<') {
                fd[0] = -1;
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
                fd[0] = FileForRedirect(interp, p, argv[i], TRUE, argv[i + 1],
                        O_RDONLY, &skip, &isOpen[0]);
                if (fd[0] < 0) {
                    goto error;
                }
            }
            break;

        case '>':
            atOK = TRUE;
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            errorToOutput = FALSE;
            if (*p == '>') {
                p++;
                atOK = FALSE;
                flags = O_WRONLY | O_CREAT;
            }
            if (*p == '&') {
                if (isOpen[2] != 0) {
                    isOpen[2] = FALSE;
                    CloseFile(fd[2]);
                }
                errorToOutput = TRUE;
                p++;
            }
            if (isOpen[1] != 0) {
                isOpen[1] = FALSE;
                CloseFile(fd[1]);
            }
            fd[1] = FileForRedirect(interp, p, argv[i], atOK, argv[i + 1], 
                flags, &skip, &isOpen[1]);
            if (fd[1] < 0) {
                goto error;
            }
            if (errorToOutput) {
                isOpen[2] = FALSE;
                fd[2] = fd[1];
            }
            break;

        case '2':
            if (*p != '>') {
                break;
            }
            p++;
            atOK = TRUE;
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            if (*p == '>') {
                p++;
                atOK = FALSE;
                flags = O_WRONLY | O_CREAT;
            }
            if (isOpen[2] != 0) {
                isOpen[2] = FALSE;
                CloseFile(fd[2]);
            }
            fd[2] = FileForRedirect(interp, p, argv[i], atOK, argv[i + 1], 
                flags, &skip, &isOpen[2]);
            if (fd[2] < 0) {
                goto error;
            }
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

    if (fd[0] == -1) {
        if (inputLiteral != NULL) {
            /*
             * The input for the first process is immediate data coming from
             * Tcl.  Create a temporary file for it and put the data into the
             * file.
             */
            fd[0] = CreateTempFile(inputLiteral);
            if (fd[0] < 0) {
                Tcl_AppendResult(interp,
                    "can't create input file for command: ",
                    Tcl_PosixError(interp), (char *)NULL);
                goto error;
            }
            isOpen[0] = TRUE;
        } else if (stdinPipePtr != NULL) {
            /*
             * The input for the first process in the pipeline is to come from
             * a pipe that can be written from by the caller.
             */
            if (CreatePipe(interp, &fd[0], stdinPipePtr) != TCL_OK) {
                goto error;
            }
            isOpen[0] = TRUE;
        } else {
            /*
             * The input for the first process comes from stdin.
             */
            fd[0] = 0;
        }
    }
    if (fd[1] == -1) {
        if (stdoutPipePtr != NULL) {
            /*
             * Output from the last process in the pipeline is to go to a pipe
             * that can be read by the caller.
             */
            if (CreatePipe(interp, stdoutPipePtr, &fd[1]) != TCL_OK) {
                goto error;
            }
            isOpen[1] = TRUE;
        } else {
            /*
             * The output for the last process goes to stdout.
             */
            fd[1] = 1;
        }
    }
    if (fd[2] == -1) {
        if (stderrPipePtr != NULL) {
            /*
             * Stderr from the last process in the pipeline is to go to a pipe
             * that can be read by the caller.
             */
            if (CreatePipe(interp, stderrPipePtr, &fd[2]) != TCL_OK) {
                goto error;
            }
            isOpen[2] = TRUE;
        } else {
            /*
             * Errors from the pipeline go to stderr.
             */
            fd[2] = 2;
        }
    }
    /*
     * Scan through the objc array, creating a process for each group of
     * arguments between the "|" characters.
     */

    Tcl_ReapDetachedProcs();
    pids = Blt_AssertMalloc(cmdCount * sizeof(int));
    curFd[0] = fd[0];

    lastArg = 0;                        /* Suppress compiler warning */
    for (i = 0; i < objc; i = lastArg + 1) {
        int joinThisError;
        int pid;

        /*
         * Convert the program name into native form.
         */

        argv[i] = Tcl_TranslateFileName(interp, argv[i], &execBuffer);
        if (argv[i] == NULL) {
            goto error;
        }
        /*
         * Find the end of the curent segment of the pipeline.
         */
        joinThisError = 0;
        for (lastArg = i + 1; lastArg < objc; lastArg++) {
            if (argv[lastArg][0] == '|') {
                if (argv[lastArg][1] == '\0') {
                    break;
                }
                if ((argv[lastArg][1] == '&') && (argv[lastArg][2] == '\0')) {
                    joinThisError = 1;
                    break;
                }
            }
        }
        argv[lastArg] = NULL;

        /*
         * If this is the last segment, use the specified fd[1].  Otherwise
         * create an intermediate pipe.  pipeIn will become the curInFile for
         * the next segment of the pipe.
         */
        if (lastArg == objc) {
            curFd[1] = fd[1];
        } else {
            if (CreatePipe(interp, &pipeIn, &curFd[1]) != TCL_OK) {
                goto error;
            }
        }

        if (joinThisError != 0) {
            curFd[2] = curFd[1];
        } else {
            curFd[2] = fd[2];
        }

        if (CreateProcess(interp, lastArg - i, argv + i, curFd[0], curFd[1], 
                curFd[2], &pid, env) != TCL_OK) {
            goto error;
        }
        Tcl_DStringFree(&execBuffer);

        pids[numPids].pid = pid;
        numPids++;

        /*
         * Close off our copies of file descriptors that were set up for this
         * child, then set up the input for the next child.
         */
        if ((curFd[0] >= 0) && (curFd[0] != fd[0])) {
            CloseFile(curFd[0]);
        }
        curFd[0] = pipeIn;
        pipeIn = -1;

        if ((curFd[1] >= 0) && (curFd[1] != fd[1])) {
            CloseFile(curFd[1]);
        }
        curFd[1] = -1;
    }

    *pidsPtr = pids;

    /*
     * All done.  Cleanup open files lying around and then return.
     */

  cleanup:
    Tcl_DStringFree(&execBuffer);

    for (i = 0; i < 3; i++) {
        if (isOpen[i]) {
            CloseFile(fd[i]);
        }
    }
    if (argv != NULL) {
        Blt_Free(argv);
    }
    return numPids;

    /*
     * An error occured.  There could have been extra files open, such as
     * pipes between children.  Clean them all up.  Detach any child processes
     * that have been created.
     */

  error:
    if (pipeIn >= 0) {
        CloseFile(pipeIn);
    }
    if ((curFd[2] >= 0) && (curFd[2] != fd[2])) {
        CloseFile(curFd[2]);
    }
    if ((curFd[1] >= 0) && (curFd[1] != fd[1])) {
        CloseFile(curFd[1]);
    }
    if ((curFd[0] >= 0) && (curFd[0] != fd[0])) {
        CloseFile(curFd[0]);
    }
    if ((stdinPipePtr != NULL) && (*stdinPipePtr >= 0)) {
        CloseFile(*stdinPipePtr);
        *stdinPipePtr = -1;
    }
    if ((stdoutPipePtr != NULL) && (*stdoutPipePtr >= 0)) {
        CloseFile(*stdoutPipePtr);
        *stdoutPipePtr = -1;
    }
    if ((stderrPipePtr != NULL) && (*stderrPipePtr >= 0)) {
        CloseFile(*stderrPipePtr);
        *stderrPipePtr = -1;
    }
    if (pids != NULL) {
        for (i = 0; i < numPids; i++) {
            if (pids[i].pid != -1) {
                Tcl_DetachPids(1, (Tcl_Pid *)(pids + i));
            }
        }
        Blt_Free(pids);
    }
    numPids = -1;
    goto cleanup;
}

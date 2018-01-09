/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltWinMain.c --
 *
 * Main entry point for wish and other Tk-based applications.
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
 * This file was adapted from the Tk library distribution.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
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


#include "config.h"

#ifdef USE_TCL_STUBS
  #define HAVE_TCL_STUBS 1
  #undef USE_TCL_STUBS
#endif

#include <tcl.h>
#ifndef TCL_ONLY
#include <tk.h>
#endif /*TCL_ONLY*/
#include "blt.h"
#include <locale.h>
#include <stdio.h>

#ifdef HAVE_STRING_H
  #include <string.h>
#endif

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif

#ifdef WIN32
  #define STRICT
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #undef STRICT
  #undef WIN32_LEAN_AND_MEAN
  #include <windowsx.h>
#endif /* WIN32 */

#define vsnprintf               _vsnprintf
#define TK_MAX_WARN_LEN 1024

/*
 * Forward declarations for procedures defined later in this file:
 */

#ifdef HAVE_TCL_STUBS
#undef Tcl_InitStubs
#ifdef notdef
extern const char *Tcl_InitStubs(Tcl_Interp *interp, const char *version, 
        int exact);
#endif
#endif

static BOOL consoleRequired = TRUE;

/*
 *---------------------------------------------------------------------------
 *
 * PanicProc --
 *
 *      Display a message and exit.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Exits the program.
 *
 *---------------------------------------------------------------------------
 */
static void
PanicProc(const char *fmt, ...)
{
    va_list args;
    char buf[TK_MAX_WARN_LEN];
    WCHAR msgString[TK_MAX_WARN_LEN + 5];

    va_start(args, fmt);
    vsnprintf(buf, TK_MAX_WARN_LEN, fmt, args);
    va_end(args);
    MultiByteToWideChar(CP_UTF8, 0, buf, -1, msgString, TK_MAX_WARN_LEN);
    memcpy(msgString + TK_MAX_WARN_LEN, L" ...", 5 * sizeof(WCHAR));
    MessageBeep(MB_ICONEXCLAMATION);
    MessageBoxW(NULL, msgString, L"Fatal Error in Bltwish",
	    MB_ICONSTOP | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
#if defined(_MSC_VER) 
    DebugBreak();
#endif /* _MSC_VER || __BORLANDC__ */
    ExitProcess(1);
}


#ifndef TCL_ONLY
#if defined(__CYGWIN__) || defined(__MINGW32__)

/*
 *---------------------------------------------------------------------------
 *
 * setargv --
 *
 *      Parse the Windows command line string into argc/argv.  Done here
 *      because we don't trust the builtin argument parser in crt0.  Windows
 *      applications are responsible for breaking their command line into
 *      arguments.
 *
 *      2N backslashes + quote -> N backslashes + begin quoted string
 *      2N + 1 backslashes + quote -> literal
 *      N backslashes + non-quote -> literal
 *      quote + quote in a quoted string -> single quote
 *      quote + quote not in quoted string -> empty string
 *      quote -> begin quoted string
 *
 * Results:
 *      Fills argcPtr with the number of arguments and argvPtr with the
 *      array of arguments.
 *
 * Side effects:
 *      Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

static void
setargv(
    int *argcPtr,               /* Filled with number of argument strings. */
    char ***argvPtr)
{                               /* Filled with argument strings (malloc'd). */
    char *cmd, *p, *arg, *argSpace;
    char **argv;
    int argc, numArgs, inquote, copy, slashes;

    cmd = GetCommandLine();     /* INTL: BUG */
    /*
     * Precompute an overly pessimistic guess at the number of arguments in
     * the command line by counting non-space spans.
     */
    numArgs = 2;
    for (p = cmd; *p != '\0'; p++) {
        if ((*p == ' ') || (*p == '\t')) {      /* INTL: ISO space. */
            numArgs++;
            while ((*p == ' ') || (*p == '\t')) { /* INTL: ISO space. */
                p++;
            }
            if (*p == '\0') {
                break;
            }
        }
    }
    argSpace = malloc(numArgs * sizeof(char *) + (p - cmd) + 1);
    argv = (char **)argSpace;
    argSpace += numArgs * sizeof(char *);
    numArgs--;

    p = cmd;
    for (argc = 0; argc < numArgs; argc++) {
        argv[argc] = arg = argSpace;
        while ((*p == ' ') || (*p == '\t')) {   /* INTL: ISO space. */
            p++;
        }
        if (*p == '\0') {
            break;
        }
        inquote = 0;
        slashes = 0;
        while (1) {
            copy = 1;
            while (*p == '\\') {
                slashes++;
                p++;
            }
            if (*p == '"') {
                if ((slashes & 1) == 0) {
                    copy = 0;
                    if ((inquote) && (p[1] == '"')) {
                        p++;
                        copy = 1;
                    } else {
                        inquote = !inquote;
                    }
                }
                slashes >>= 1;
            }
            while (slashes) {
                *arg = '\\';
                arg++;
                slashes--;
            }

            if ((*p == '\0') || (!inquote && ((*p == ' ') || 
              (*p == '\t')))) { /* INTL: ISO space. */
                break;
            }
            if (copy != 0) {
                *arg = *p;
                arg++;
            }
            p++;
        }
        *arg = '\0';
        argSpace = arg + 1;
    }
    argv[argc] = NULL;

    *argcPtr = argc;
    *argvPtr = argv;
}
#endif /* CYGWIN || MINGW32 */
#endif /* !TCL_ONLY */

#ifdef TCL_ONLY
extern Tcl_AppInitProc Blt_TclInit;
extern Tcl_AppInitProc Blt_TclPkgsInit;

static int
InitializeForTclOnly(Tcl_Interp *interp) /* Interpreter for application. */
{
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
#ifdef TCLLIBPATH
    /* 
     * It seems that some distributions of TCL don't compile-in a
     * default location of the library.  This causes Tcl_Init to fail
     * if bltwish and bltsh are moved to another directory. The
     * workaround is to set the magic variable "tclDefaultLibrary".
     */
    Tcl_SetVar(interp, "tclDefaultLibrary", TCLLIBPATH, TCL_GLOBAL_ONLY);
#endif /* TCLLIBPATH */

#ifdef USE_BLT_STUBS
    if (Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    };
#else 
    if (Blt_TclInit(interp) != TCL_OK) {
        return TCL_ERROR;
    }
#endif  /*USE_BLT_STUBS*/
#ifdef STATIC_PKGS
    if (Blt_TclPkgsInit(interp) != TCL_OK) {
        return TCL_ERROR;
    }
#endif  /*STATIC_PACKAGES*/
    Tcl_SetVar(interp, "tcl_rcFileName", "~/tclshrc.tcl", TCL_GLOBAL_ONLY);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * main --
 *
 *      This is the main program for the application.
 *
 * Results:
 *      None: Tcl_Main never returns here, so this procedure never returns
 *      either.
 *
 * Side effects:
 *      Whatever the application does.
 *
 *---------------------------------------------------------------------------
 */
int
main(int argc, char **argv)
{
    char *p;
    
    Tcl_SetPanicProc(PanicProc);

    /*
     * Set up the default locale to be standard "C" locale so parsing is
     * performed correctly.
     */

    setlocale(LC_ALL, "C");

    /*
     * Console emulation widget not required as this entry is from the
     * console subsystem, thus stdin,out,err already have end-points.
     */

    consoleRequired = FALSE;

    /*
     * Forward slashes substituted for backslashes.
     */
    for (p = argv[0]; *p != '\0'; p++) {
	if (*p == '\\') {
	    *p = '/';
	}
    }

    Tcl_Main(argc, argv, InitializeForTclOnly);
    return 0;                   /* Needed only to prevent compiler warning. */
}

#else /* TCL_ONLY */


extern Tcl_AppInitProc Blt_TclInit;
extern Tcl_AppInitProc Blt_TkInit;
extern Tcl_AppInitProc Blt_TclPkgsInit;
extern Tcl_AppInitProc Blt_TkPkgsInit;

static int
InitializeForTclTk(Tcl_Interp *interp)  /* Interpreter for application. */
{
    WCHAR msgString[TK_MAX_WARN_LEN + 5];

#ifdef STATIC_PKGS
    if (Tcl_Init(interp) != TCL_OK) {
        goto error;
    }
    if (Tk_Init(interp) != TCL_OK) {
        goto error;
    }
#else
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
        goto error;
    }
    if (Tcl_PkgRequire(interp, "Tk", TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
        goto error;
    }
#endif  /*STATIC_PKGS*/
    if (consoleRequired) {
        if (Tk_CreateConsoleWindow(interp) == TCL_ERROR) {
            goto error;
        }
    }

#ifdef TCLLIBPATH
    /* 
     * It seems that some distributions of TCL don't compile-in a
     * default location of the library.  This causes Tcl_Init to fail
     * if bltwish and bltsh are moved to another directory. The
     * workaround is to set the magic variable "tclDefaultLibrary".
     */
    Tcl_SetVar(interp, "tclDefaultLibrary", TCLLIBPATH, TCL_GLOBAL_ONLY);
#endif /* TCLLIBPATH */
#ifdef USE_BLT_STUBS
    if (Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
        goto error;
    };
    if (Blt_InitTkStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
        goto error;
    };
#else 
    if (Blt_TclInit(interp) != TCL_OK) {
        goto error;
    }
    if (Blt_TkInit(interp)  != TCL_OK) {
        goto error;
    }
#endif  /*USE_BLT_STUBS*/
#ifdef STATIC_PKGS
    if (Blt_TclPkgsInit(interp) != TCL_OK) {
        goto error;
    }
    if (Blt_TkPkgsInit(interp) != TCL_OK) {
        goto error;
    }
#endif  /*STATIC_PKGS*/
    Tcl_SetVar(interp, "tcl_rcFileName", "~/wishrc.tcl", TCL_GLOBAL_ONLY);
    return TCL_OK;
  error:
    MultiByteToWideChar(CP_UTF8, 0, Tcl_GetStringResult(interp), -1,
	    msgString, TK_MAX_WARN_LEN);
    /*
     * Truncate MessageBox string if it is too long to not overflow the screen
     * and cause possible oversized window error.
     */
    memcpy(msgString + TK_MAX_WARN_LEN, L" ...", 5 * sizeof(WCHAR));
    MessageBeep(MB_ICONEXCLAMATION);
    MessageBoxW(NULL, msgString, L"Error in Bltwish",
	    MB_ICONSTOP | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
    ExitProcess(1);

    /*
     * We won't reach this, but we need the return.
     */

    return TCL_ERROR;
}


/*
 *---------------------------------------------------------------------------
 *
 * WinMain --
 *
 *      Main entry point from Windows.
 *
 * Results:
 *      Returns false if initialization fails, otherwise it never returns.
 *
 * Side effects:
 *      Just about anything, since from here we call arbitrary TCL code.
 *
 *---------------------------------------------------------------------------
 */

int APIENTRY
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpszCmdLine,
    int numCmdShow)
{
    char **argv;
    int argc;
    char *p;
    
    Tcl_SetPanicProc(PanicProc);
    /*
     * Create the console channels and install them as the standard channels.
     * All I/O will be discarded until Tk_CreateConsoleWindow is called to
     * attach the console to a text widget.
     */
    consoleRequired = TRUE;

    /*
     * Set up the default locale to be standard "C" locale so parsing is
     * performed correctly.
     */

    setlocale(LC_ALL, "C");

    /*
     * Get our args from the c-runtime. Ignore lpszCmdLine.
     */

#if defined(__CYGWIN__) || defined(__MINGW32__)
    setargv(&argc, &argv);
#else
    argc = __argc;
    argv = __argv;
#endif

    /*
     * Forward slashes substituted for backslashes.
     */
    for (p = argv[0]; *p != '\0'; p++) {
	if (*p == '\\') {
	    *p = '/';
	}
    }

    Tk_Main(argc, argv, InitializeForTclTk);
    return 0;
}

#endif /* TCL_ONLY */

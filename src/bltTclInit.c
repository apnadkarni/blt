/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTclInit.c --
 *
 * This module initials the non-Tk command of the BLT toolkit, registering the
 * commands with the TCL interpreter.
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

#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"

#ifndef BLT_LIBRARY
#  ifdef WIN32
#    define BLT_LIBRARY  "c:/Program Files/Tcl/lib/blt"##BLT_VERSION
#  else
#    define BLT_LIBRARY "unknown"
#  endif
#endif

double bltNaN;

BLT_EXTERN Tcl_AppInitProc Blt_TclInit;
BLT_EXTERN Tcl_AppInitProc Blt_TclSafeInit;

BLT_EXTERN Tcl_AppInitProc Blt_tcl_Init;
BLT_EXTERN Tcl_AppInitProc Blt_tcl_SafeInit;

BLT_EXTERN int Blt_tcl_Unload(Tcl_Interp *interp, int flags);
BLT_EXTERN int Blt_tcl_SafeUnload(Tcl_Interp *interp, int flags);

static Tcl_MathProc MinMathProc, MaxMathProc;
static char libPath[1024] =
{
    BLT_LIBRARY
};

/*
 * Script to set the BLT library path in the variable global "blt_library"
 *
 * Checks the usual locations for a file (bltGraph.pro) from the BLT library.
 * The places searched in order are
 *
 *      $BLT_LIBRARY
 *      $BLT_LIBRARY/blt2.4
 *      $BLT_LIBRARY/..
 *      $BLT_LIBRARY/../blt2.4
 *      $blt_libPath
 *      $blt_libPath/blt2.4
 *      $blt_libPath/..
 *      $blt_libPath/../blt2.4
 *      $tcl_library
 *      $tcl_library/blt2.4
 *      $tcl_library/..
 *      $tcl_library/../blt2.4
 *      $env(TCL_LIBRARY)
 *      $env(TCL_LIBRARY)/blt2.4
 *      $env(TCL_LIBRARY)/..
 *      $env(TCL_LIBRARY)/../blt2.4
 *
 *  The TCL variable "blt_library" is set to the discovered path.  If the file
 *  wasn't found, no error is returned.  The actual usage of $blt_library is
 *  purposely deferred so that it can be set from within a script.
 */

/* FIXME: Change this to a namespace procedure in 3.0 */

static char initScript[] =
{"\n\
global blt_library blt_libPath blt_version tcl_library env\n\
set blt_library {}\n\
set path {}\n\
foreach var { env(BLT_LIBRARY) blt_libPath tcl_library env(TCL_LIBRARY) } { \n\
    if { ![info exists $var] } { \n\
        continue \n\
    } \n\
    set path [set $var] \n\
    if { [file readable [file join $path bltGraph.pro]] } { \n\
        set blt_library $path\n\
        break \n\
    } \n\
    set path [file join $path blt$blt_version ] \n\
    if { [file readable [file join $path bltGraph.pro]] } { \n\
        set blt_library $path\n\
        break \n\
    } \n\
    set path [file dirname [set $var]] \n\
    if { [file readable [file join $path bltGraph.pro]] } { \n\
        set blt_library $path\n\
        break \n\
    } \n\
    set path [file join $path blt$blt_version ] \n\
    if { [file readable [file join $path bltGraph.pro]] } { \n\
        set blt_library $path\n\
        break \n\
    } \n\
} \n\
if { $blt_library != \"\" } { \n\
    global auto_path \n\
    lappend auto_path $blt_library \n\
}\n\
unset var path\n\
\n"
};


static Tcl_AppInitProc *cmdProcs[] =
{
#ifndef NO_BGEXEC
    Blt_Base64CmdInitProc,
#endif
#ifndef NO_BGEXEC
    Blt_BgexecCmdInitProc,
#endif
#ifndef NO_PTYEXEC
    Blt_PtyExecCmdInitProc,
#endif
#ifndef NO_CRC32
    Blt_Crc32CmdInitProc,
#endif
#ifndef NO_CSV
    Blt_CsvCmdInitProc,
#endif
#ifndef NO_DATATABLE
    Blt_TableCmdInitProc,
#endif
#ifndef NO_DDE
    Blt_DdeCmdInitProc,
#endif
#ifndef NO_DEBUG
    Blt_DebugCmdInitProc,
#endif
#ifndef NO_FILECMD
    Blt_FileCmdInitProc,
#endif
#ifndef NO_SPLINE
    Blt_SplineCmdInitProc,
#endif
#ifndef NO_TREE
    Blt_TreeCmdInitProc,
#endif
#ifndef NO_VECTOR
    Blt_VectorCmdInitProc,
#endif
#ifndef NO_WATCH
    Blt_WatchCmdInitProc,
#endif
#ifndef NO_TIMESTAMP
    Blt_TimeStampCmdInitProc,
#endif
#ifndef NO_MESH
    Blt_MeshCmdInitProc,
#endif
#ifndef NO_COMPARE
    Blt_CompareCmdInitProc,
#endif
    (Tcl_AppInitProc *) NULL
};

double 
Blt_NaN(void)
{
    return bltNaN;
}

static double
MakeNaN(void)
{
    union DoubleValue {
        unsigned int words[2];
        double value;
    } result;

#ifdef WORDS_BIGENDIAN
    result.words[0] = 0x7ff80000;
    result.words[1] = 0x00000000;
#else
    result.words[0] = 0x00000000;
    result.words[1] = 0x7ff80000;
#endif
    return result.value;
}

/* ARGSUSED */
static int
MinMathProc(
    ClientData clientData,      /* Not used. */
    Tcl_Interp *interp,         /* Not used. */
    Tcl_Value *argsPtr,
    Tcl_Value *resultPtr)
{
    Tcl_Value *op1Ptr, *op2Ptr;

    op1Ptr = argsPtr, op2Ptr = argsPtr + 1;
    if ((op1Ptr->type == TCL_INT) && (op2Ptr->type == TCL_INT)) {
        resultPtr->intValue = MIN(op1Ptr->intValue, op2Ptr->intValue);
        resultPtr->type = TCL_INT;
    } else {
        double a, b;

        a = (op1Ptr->type == TCL_INT) 
            ? (double)op1Ptr->intValue : op1Ptr->doubleValue;
        b = (op2Ptr->type == TCL_INT)
            ? (double)op2Ptr->intValue : op2Ptr->doubleValue;
        resultPtr->doubleValue = MIN(a, b);
        resultPtr->type = TCL_DOUBLE;
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
MaxMathProc(
    ClientData clientData,      /* Not used. */
    Tcl_Interp *interp,         /* Not used. */
    Tcl_Value *argsPtr,
    Tcl_Value *resultPtr)
{
    Tcl_Value *op1Ptr, *op2Ptr;

    op1Ptr = argsPtr, op2Ptr = argsPtr + 1;
    if ((op1Ptr->type == TCL_INT) && (op2Ptr->type == TCL_INT)) {
        resultPtr->intValue = MAX(op1Ptr->intValue, op2Ptr->intValue);
        resultPtr->type = TCL_INT;
    } else {
        double a, b;

        a = (op1Ptr->type == TCL_INT)
            ? (double)op1Ptr->intValue : op1Ptr->doubleValue;
        b = (op2Ptr->type == TCL_INT)
            ? (double)op2Ptr->intValue : op2Ptr->doubleValue;
        resultPtr->doubleValue = MAX(a, b);
        resultPtr->type = TCL_DOUBLE;
    }
    return TCL_OK;
}

static int
SetLibraryPath(Tcl_Interp *interp)
{
    Tcl_DString ds;
    const char *value;

    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, libPath, -1);
#ifdef WIN32
    {
        HKEY key;
        DWORD result;
#  ifndef BLT_REGISTRY_KEY
#    define BLT_REGISTRY_KEY "Software\\BLT\\" BLT_VERSION "\\" TCL_VERSION
#  endif
        result = RegOpenKeyEx(
              HKEY_LOCAL_MACHINE, /* Parent key. */
              BLT_REGISTRY_KEY, /* Path to sub-key. */
              0,                /* Reserved. */
              KEY_READ,         /* Security access mask. */
              &key);            /* Resulting key.*/

        if (result == ERROR_SUCCESS) {
            DWORD size;

            /* Query once to get the size of the string needed */
            result = RegQueryValueEx(key, "BLT_LIBRARY", NULL, NULL, NULL, 
                     &size);
            if (result == ERROR_SUCCESS) {
                Tcl_DStringSetLength(&ds, size);
                /* And again to collect the string. */
                RegQueryValueEx(key, "BLT_LIBRARY", NULL, NULL,
                                (LPBYTE)Tcl_DStringValue(&ds), &size);
                RegCloseKey(key);
            }
        }
    }
#endif /* WIN32 */
    value = Tcl_SetVar(interp, "blt_libPath", Tcl_DStringValue(&ds),
        TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    Tcl_DStringFree(&ds);
    if (value == NULL) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*LINTLIBRARY*/
int
Blt_TclInit(Tcl_Interp *interp) /* Interpreter to add extra commands */
{
    Tcl_AppInitProc **p;
    Tcl_Namespace *nsPtr;
    Tcl_ValueType args[2];
    const char *mesg;
#ifdef USE_BLT_STUBS
    extern BltTclProcs bltTclProcs;
#endif  /* USE_Blt_STUBS */
    int result;

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
        return TCL_ERROR;
    };
#endif  /* USE_TCL_STUBS */
    Blt_AllocInit(NULL, NULL, NULL);

    /*
     * Check that the versions of TCL that have been loaded are the same ones
     * that BLT was compiled against.
     */
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION_COMPILED, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    }

    /* Set the "blt_version", "blt_patchLevel", and "blt_libPath" Tcl
     * variables. We'll use them in the following script. */

    mesg = Tcl_SetVar(interp, "blt_version", BLT_VERSION, TCL_GLOBAL_ONLY);
    if (mesg == NULL) {
        return TCL_ERROR;
    }
    mesg = Tcl_SetVar(interp, "blt_patchLevel", BLT_PATCH_LEVEL, 
                        TCL_GLOBAL_ONLY);
    if (mesg == NULL) {
        return TCL_ERROR;
    }
    if (SetLibraryPath(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_Eval(interp, initScript) != TCL_OK) {
        return TCL_ERROR;
    }

    nsPtr = Tcl_FindNamespace(interp, "::blt", (Tcl_Namespace *)NULL, 0);
    if (nsPtr == NULL) {
        nsPtr = Tcl_CreateNamespace(interp, "::blt", NULL, NULL);
        if (nsPtr == NULL) {
            return TCL_ERROR;
        }
    }
    /* Initialize the BLT commands that only require Tcl. */
    for (p = cmdProcs; *p != NULL; p++) {
        if ((**p) (interp) != TCL_OK) {
            Tcl_DeleteNamespace(nsPtr);
            return TCL_ERROR;
        }
    }
    args[0] = args[1] = TCL_EITHER;
    Tcl_CreateMathFunc(interp, "min", 2, args, MinMathProc, (ClientData)0);
    Tcl_CreateMathFunc(interp, "max", 2, args, MaxMathProc, (ClientData)0);
    Blt_RegisterArrayObj();
    bltNaN = MakeNaN();
#ifdef USE_BLT_STUBS
    result = Tcl_PkgProvideEx(interp, "blt_tcl", BLT_VERSION, &bltTclProcs);
    Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT);
#else 
    result = Tcl_PkgProvide(interp, "blt_tcl", BLT_VERSION);
#endif  /* USE_BLT_STUBS */
    return result;
}

/*LINTLIBRARY*/
int
Blt_TclSafeInit(Tcl_Interp *interp) /* Interpreter to add extra commands */
{
    return Blt_TclInit(interp);
}

int
Blt_tcl_Init(Tcl_Interp *interp)  
{
    return Blt_TclInit(interp);
}

int
Blt_tcl_SafeInit(Tcl_Interp *interp) /* Interpreter to add extra commands */
{
    return Blt_TclInit(interp);
}

int
Blt_tcl_Unload(Tcl_Interp *interp, int flags) 
{
    return TCL_OK;
}

int
Blt_tcl_SafeUnload(Tcl_Interp *interp, int flags) 
{
    return TCL_OK;
}

#ifdef USE_DLL
#  include "bltWinDll.c"
#endif

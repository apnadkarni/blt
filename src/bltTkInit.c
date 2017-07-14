/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTkInit.c --
 *
 * This module initials the Tk-related commands of BLT toolkit, registering
 * the commands with the TCL interpreter.
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

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

BLT_EXTERN Tcl_AppInitProc Blt_TkInit;
BLT_EXTERN Tcl_AppInitProc Blt_TkSafeInit;
BLT_EXTERN Tcl_AppInitProc Blt_TclInit;
BLT_EXTERN Tcl_AppInitProc Blt_Init;
BLT_EXTERN Tcl_AppInitProc Blt_tk_Init;
BLT_EXTERN Tcl_AppInitProc Blt_tk_SafeInit;

BLT_EXTERN int Blt_tk_Unload(Tcl_Interp *interp, int flags);
BLT_EXTERN int Blt_tk_SafeUnload(Tcl_Interp *interp, int flags);

static Tcl_AppInitProc *cmdProcs[] =
{
    Blt_AfmCmdInitProc,
    Blt_BackgroundCmdInitProc,
#ifndef NO_BELL
    Blt_BeepCmdInitProc,
#endif
#ifndef NO_BITMAP
    Blt_BitmapCmdInitProc,
#endif
#ifndef NO_BUSY
    Blt_BusyCmdInitProc,
#endif
    Blt_ComboButtonInitProc,
    Blt_ComboEditorInitProc,
    Blt_ComboEntryInitProc,
    Blt_ComboFrameInitProc,
    Blt_ComboMenuInitProc,
    Blt_ComboTreeInitProc,
#ifndef NO_CONTAINER
    Blt_ContainerCmdInitProc,
#endif
#ifndef NO_CUTBUFFER
    Blt_CutbufferCmdInitProc,
#endif
#ifndef NO_DND
    Blt_DndCmdInitProc,
#endif
#ifndef NO_DRAGDROP
    Blt_DragDropCmdInitProc,
#endif
#ifndef NO_DRAWERSET
    Blt_DrawersetCmdInitProc,
#endif
    Blt_GrabCmdInitProc,
#ifndef NO_GRAPH
    Blt_GraphCmdInitProc,
#endif
#ifndef NO_HTEXT
    Blt_HtextCmdInitProc,
#endif
    Blt_ListViewInitProc,
    Blt_PaletteCmdInitProc,
#ifndef NO_PANESET
    Blt_PanesetCmdInitProc,
#endif
#ifndef NO_FILMSTRIP
    Blt_FilmstripCmdInitProc,
#endif
#ifndef NO_PICTURE
    Blt_PictureCmdInitProc,
#endif
#ifndef NO_PRINTER
    Blt_PrinterCmdInitProc,
#endif
#ifndef NO_SCROLLSET
    Blt_ScrollsetCmdInitProc,
#endif
    Blt_SendEventCmdInitProc,
#ifndef NO_TABLEMGR
    Blt_TableMgrCmdInitProc,
#endif
#ifndef NO_TABSET
    Blt_TabsetCmdInitProc,
#endif
#ifndef NO_TKFRAME
    Blt_FrameCmdInitProc,
#endif
#ifndef NO_TKBUTTON
    Blt_ButtonCmdInitProc,
#endif
#ifndef NO_TKSCROLLBAR
    Blt_ScrollbarCmdInitProc,
#endif
#ifndef NO_WINOP
    Blt_WinopCmdInitProc,
#endif
#ifndef NO_TREEVIEW
    Blt_TreeViewCmdInitProc,
#endif
#ifndef NO_TABLEVIEW
    Blt_TableViewCmdInitProc,
#endif
#if (BLT_MAJOR_VERSION > 3)
#ifndef NO_MOUNTAIN
    Blt_MountainCmdInitProc,
#endif
#endif
#ifndef NO_TED
    Blt_TedCmdInitProc,
#endif
#ifndef NO_ARCBALL
    Blt_ArcBallCmdInitProc,
#endif
    Blt_PaintBrushCmdInitProc,
#ifndef NO_KIOSK
    Blt_KioskCmdInitProc,
#endif
    (Tcl_AppInitProc *) NULL
};

/*LINTLIBRARY*/
int
Blt_TkInit(Tcl_Interp *interp)         /* Interpreter to add extra commands */
{
    Tcl_Namespace *nsPtr;
    Tcl_AppInitProc **p;
#ifdef USE_BLT_STUBS
    extern BltTkProcs bltTkProcs;
#endif
    int result;

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
        return TCL_ERROR;
    };
#endif  /*USE_TCL_STUBS*/
#if (_TCL_VERSION >= _VERSION(8,1,0)) 
#ifdef USE_TK_STUBS
    if (Tk_InitStubs(interp, TK_VERSION_COMPILED, PKG_ANY) == NULL) {
        return TCL_ERROR;
    };
#endif  /*USE_TKSTUBS*/
    if (Tcl_PkgPresent(interp, "Tk", TK_VERSION_COMPILED, PKG_ANY) == NULL) {
        Tcl_AppendResult(interp, "Tk package must be loaded", (char *)NULL);
        return TCL_ERROR;
    } 
#else
    if (Tcl_PkgRequire(interp, "Tk", TK_VERSION_COMPILED, PKG_ANY) == NULL) {
        Tcl_ResetResult(interp);
        return TCL_OK;
    } 
#endif  /* TCL >= 8.1 */

#ifdef USE_BLT_STUBS
    if (Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    }
#endif  /*USE_BLT_STUBS*/
    if (Tcl_PkgRequire(interp, "blt_tcl", BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    }
    nsPtr = Tcl_CreateNamespace(interp, "::blt::tk", NULL, NULL);
    if (nsPtr == NULL) {
        return TCL_ERROR;
    }
    nsPtr = Tcl_FindNamespace(interp, "::blt", NULL, TCL_LEAVE_ERR_MSG);
    if (nsPtr == NULL) {
        return TCL_ERROR;
    }
    Blt_RegisterPictureImageType(interp);
    Blt_RegisterCanvasEpsItem();
    Blt_RegisterCanvasLabelItem();
    Blt_InitXRandrConfig(interp);
    Blt_CollectExtInfo(interp);

    /* Initialize the BLT commands that only use Tk. */
    for (p = cmdProcs; *p != NULL; p++) {
        if ((**p) (interp) != TCL_OK) {
            Tcl_DeleteNamespace(nsPtr);
            return TCL_ERROR;
        }
    }
#ifdef USE_BLT_STUBS
    result = Tcl_PkgProvideEx(interp, "blt_tk", BLT_VERSION, &bltTkProcs);
    Blt_InitTkStubs(interp, BLT_VERSION, PKG_EXACT);
#else 
    result = Tcl_PkgProvide(interp, "blt_tk", BLT_VERSION);
#endif
    return result;
}

/*LINTLIBRARY*/
int
Blt_TkSafeInit(Tcl_Interp *interp) /* Interpreter to add extra commands */
{
    return Blt_TkInit(interp);
}

int
Blt_tk_Init(Tcl_Interp *interp) {
    return Blt_TkInit(interp);
}

int
Blt_tk_SafeInit(Tcl_Interp *interp) {
    return Blt_TkInit(interp);
}

int
Blt_tk_Unload(Tcl_Interp *interp, int flags) 
{
    return TCL_OK;
}

int
Blt_tk_SafeUnload(Tcl_Interp *interp, int flags) 
{
    return TCL_OK;
}


#ifdef USE_DLL
  #include "bltWinDll.c"
#endif


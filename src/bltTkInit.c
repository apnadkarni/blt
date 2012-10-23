
/*
 * bltTkInit.c --
 *
 * This module initials the Tk-related commands of BLT toolkit, registering
 * the commands with the TCL interpreter.
 *
 *	Copyright 1991-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
    Blt_BgPatternCmdInitProc,
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
    Blt_ComboEntryInitProc,
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
    Blt_GrabCmdInitProc,
#ifndef NO_GRAPH
    Blt_GraphCmdInitProc,
#endif
#ifndef NO_HTEXT
    Blt_HtextCmdInitProc,
#endif
    Blt_ListViewInitProc,
    Blt_MeshCmdInitProc,
    Blt_PaletteCmdInitProc,
#ifndef NO_PANESET
    Blt_PanesetCmdInitProc,
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
    Blt_PaintbrushCmdInitProc,
#ifndef NO_KIOSK
    Blt_KioskCmdInitProc,
#endif
    (Tcl_AppInitProc *) NULL
};

/*LINTLIBRARY*/
int
Blt_TkInit(Tcl_Interp *interp)	       /* Interpreter to add extra commands */
{
    Tcl_Namespace *nsPtr;
    Tcl_AppInitProc **p;
    extern BltTkProcs bltTkProcs;
    int result;

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
	return TCL_ERROR;
    };
#endif	/*USE_TCL_STUBS*/
#if (_TCL_VERSION >= _VERSION(8,1,0)) 
#ifdef USE_TK_STUBS
    if (Tk_InitStubs(interp, TK_VERSION_COMPILED, PKG_ANY) == NULL) {
	return TCL_ERROR;
    };
#endif	/*USE_TKSTUBS*/
    if (Tcl_PkgPresent(interp, "Tk", TK_VERSION_COMPILED, PKG_ANY) == NULL) {
	Tcl_AppendResult(interp, "Tk package must be loaded", (char *)NULL);
	return TCL_ERROR;
    } 
#else
    if (Tcl_PkgRequire(interp, "Tk", TK_VERSION_COMPILED, PKG_ANY) == NULL) {
	Tcl_ResetResult(interp);
	return TCL_OK;
    } 
#endif	/* TCL >= 8.1 */

#ifdef USE_BLT_STUBS
    if (Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    }
#endif	/*USE_BLT_STUBS*/
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
    Blt_RegisterEpsCanvasItem();
    Blt_InitXRandrConfig(interp);

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
#  include "bltWinDll.c"
#endif


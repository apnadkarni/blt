/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTkInit.h --
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

#ifndef _BLT_TK_INIT_H
#define _BLT_TK_INIT_H

#ifndef NO_KIOSK
BLT_EXTERN Tcl_AppInitProc Blt_KioskCmdInitProc;
#endif
#ifndef NO_PALETTE
BLT_EXTERN Tcl_AppInitProc Blt_PaletteCmdInitProc;
#endif
#ifndef NO_BEEP
BLT_EXTERN Tcl_AppInitProc Blt_BeepCmdInitProc;
#endif
#ifndef NO_BITMAP
BLT_EXTERN Tcl_AppInitProc Blt_BitmapCmdInitProc;
#endif
#ifndef NO_BUSY
BLT_EXTERN Tcl_AppInitProc Blt_BusyCmdInitProc;
#endif
#ifndef NO_CONTAINER
BLT_EXTERN Tcl_AppInitProc Blt_ContainerCmdInitProc;
#endif
#ifndef NO_CUTBUFFER
BLT_EXTERN Tcl_AppInitProc Blt_CutbufferCmdInitProc;
#endif
#ifndef NO_DRAGDROP
BLT_EXTERN Tcl_AppInitProc Blt_DragDropCmdInitProc;
#endif
#ifndef NO_DRAWERSET
BLT_EXTERN Tcl_AppInitProc Blt_DrawersetCmdInitProc;
#endif
#ifndef NO_DND
BLT_EXTERN Tcl_AppInitProc Blt_DndCmdInitProc;
#endif
#ifndef NO_GRAPH
BLT_EXTERN Tcl_AppInitProc Blt_GraphCmdInitProc;
#endif
#ifndef NO_HTEXT
BLT_EXTERN Tcl_AppInitProc Blt_HtextCmdInitProc;
#endif
#ifdef WIN32
#  ifndef NO_PRINTER
BLT_EXTERN Tcl_AppInitProc Blt_PrinterCmdInitProc;
#  endif
#endif
BLT_EXTERN Tcl_AppInitProc Blt_AfmCmdInitProc;
#ifndef NO_PICTURE
BLT_EXTERN Tcl_AppInitProc Blt_PictureCmdInitProc;
#endif
#ifndef NO_TABLEMGR
BLT_EXTERN Tcl_AppInitProc Blt_TableMgrCmdInitProc;
#endif
#ifndef NO_WINOP
BLT_EXTERN Tcl_AppInitProc Blt_WinopCmdInitProc;
#endif
#ifndef NO_TABSET
BLT_EXTERN Tcl_AppInitProc Blt_TabsetCmdInitProc;
#endif
#ifndef NO_TABLE
BLT_EXTERN Tcl_AppInitProc Blt_TableCmdInitProc;
#endif
#ifndef NO_TREEVIEW
BLT_EXTERN Tcl_AppInitProc Blt_TreeViewCmdInitProc;
#endif
#ifndef NO_TABLEVIEW
BLT_EXTERN Tcl_AppInitProc Blt_TableViewCmdInitProc;
#endif
#ifndef NO_TKFRAME
BLT_EXTERN Tcl_AppInitProc Blt_FrameCmdInitProc;
#endif
#ifndef NO_TKBUTTON
BLT_EXTERN Tcl_AppInitProc Blt_ButtonCmdInitProc;
#endif
#ifndef NO_SCROLLSET
BLT_EXTERN Tcl_AppInitProc Blt_ScrollsetCmdInitProc;
#endif
#ifndef NO_PANESET
BLT_EXTERN Tcl_AppInitProc Blt_PanesetCmdInitProc;
#endif
#ifndef NO_TKSCROLLBAR
BLT_EXTERN Tcl_AppInitProc Blt_ScrollbarCmdInitProc;
#endif
BLT_EXTERN Tcl_AppInitProc Blt_BackgroundCmdInitProc;

#ifndef NO_TED
BLT_EXTERN Tcl_AppInitProc Blt_TedCmdInitProc;
#endif

BLT_EXTERN Tcl_AppInitProc Blt_GrabCmdInitProc;
BLT_EXTERN Tcl_AppInitProc Blt_ComboButtonInitProc;
BLT_EXTERN Tcl_AppInitProc Blt_ComboEditorInitProc;
BLT_EXTERN Tcl_AppInitProc Blt_ComboEntryInitProc;
BLT_EXTERN Tcl_AppInitProc Blt_ComboMenuInitProc;
BLT_EXTERN Tcl_AppInitProc Blt_ComboTreeInitProc;
BLT_EXTERN Tcl_AppInitProc Blt_ListViewInitProc;
BLT_EXTERN Tcl_AppInitProc Blt_PaintBrushCmdInitProc;

BLT_EXTERN Tcl_AppInitProc Blt_SendEventCmdInitProc;

#ifndef NO_DDE
BLT_EXTERN Tcl_AppInitProc Blt_DdeCmdInitProc;
#endif

#endif /*_BLT_TK_INIT_H*/

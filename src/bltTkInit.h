
/*
 * bltTkInit.h --
 *
 *	Copyright 1993-2004 George A Howlett.
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
BLT_EXTERN Tcl_AppInitProc Blt_DateScanCmdInitProc;

#ifndef NO_TED
BLT_EXTERN Tcl_AppInitProc Blt_TedCmdInitProc;
#endif

BLT_EXTERN Tcl_AppInitProc Blt_GrabCmdInitProc;
BLT_EXTERN Tcl_AppInitProc Blt_ComboButtonInitProc;
BLT_EXTERN Tcl_AppInitProc Blt_ComboEntryInitProc;
BLT_EXTERN Tcl_AppInitProc Blt_ComboMenuInitProc;
BLT_EXTERN Tcl_AppInitProc Blt_ComboTreeInitProc;
BLT_EXTERN Tcl_AppInitProc Blt_ListViewInitProc;
BLT_EXTERN Tcl_AppInitProc Blt_PaintbrushCmdInitProc;

BLT_EXTERN Tcl_AppInitProc Blt_SendEventCmdInitProc;

#ifndef NO_DDE
BLT_EXTERN Tcl_AppInitProc Blt_DdeCmdInitProc;
#endif

#endif /*_BLT_TK_INIT_H*/

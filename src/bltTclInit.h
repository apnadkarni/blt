/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltTclInit.h --
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

#ifndef _BLT_TCL_INIT_H
#define _BLT_TCL_INIT_H

#ifdef WIN32
#  define	NO_PTYEXEC	1
#  define	NO_CUTBUFFER	1
#  define	NO_DND		1  
#  define	NO_KIOSK	1  
#else
#  define	NO_DDE		1
#  define	NO_PRINTER	1
#endif /* WIN32 */

#ifndef HAVE_LIBLIBSSH2
#define NO_SFTP 1
#endif

#ifndef NO_SFTP
extern Tcl_AppInitProc Blt_sftp_Init;
#endif
#ifndef NO_MESH
extern Tcl_AppInitProc Blt_MeshCmdInitProc;
#endif
#ifndef NO_ARCBALL
extern Tcl_AppInitProc Blt_ArcBallCmdInitProc;
#endif
#ifndef NO_BASE64
extern Tcl_AppInitProc Blt_Base64CmdInitProc;
#endif
#ifndef NO_BGEXEC
extern Tcl_AppInitProc Blt_BgexecCmdInitProc;
#endif
#ifndef NO_PTYEXEC
extern Tcl_AppInitProc Blt_PtyExecCmdInitProc;
#endif
#ifndef NO_CRC32
extern Tcl_AppInitProc Blt_Crc32CmdInitProc;
#endif
#ifndef NO_CSV
extern Tcl_AppInitProc Blt_CsvCmdInitProc;
#endif
#ifndef NO_DEBUG
extern Tcl_AppInitProc Blt_DebugCmdInitProc;
#endif
#ifndef NO_FILECMD
extern Tcl_AppInitProc Blt_FileCmdInitProc;
#endif
#ifdef WIN32
#  ifndef NO_PRINTER
extern Tcl_AppInitProc Blt_PrinterCmdInitProc;
#  endif
#endif	/*WIN32*/

#ifndef NO_VECTOR
extern Tcl_AppInitProc Blt_VectorCmdInitProc;
#endif
#ifndef NO_WATCH
extern Tcl_AppInitProc Blt_WatchCmdInitProc;
#endif

#ifndef NO_SPLINE
extern Tcl_AppInitProc Blt_SplineCmdInitProc;
#endif

#ifndef NO_TREE
extern Tcl_AppInitProc Blt_TreeCmdInitProc;
#endif

#ifndef NO_DATE
extern Tcl_AppInitProc Blt_DateCmdInitProc;
#endif

#ifndef NO_COMPARE
extern Tcl_AppInitProc Blt_CompareCmdInitProc;
#endif

#endif /*_BLT_TCL_INIT_H*/

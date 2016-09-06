/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTclInit.h --
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

#ifndef _BLT_TCL_INIT_H
#define _BLT_TCL_INIT_H

#ifdef WIN32
#  define       NO_PTYEXEC      1
#  define       NO_CUTBUFFER    1
#  define       NO_DND          1  
#  define       NO_KIOSK        1  
#else
#  define       NO_DDE          1
#  define       NO_PRINTER      1
#endif /* WIN32 */

#  define       NO_KIOSK        1  
#  define       NO_TED          1  

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
#ifndef NO_CHECKSUM
extern Tcl_AppInitProc Blt_ChecksumCmdInitProc;
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
#endif  /*WIN32*/

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

#ifndef NO_TIMESTAMP
extern Tcl_AppInitProc Blt_TimeStampCmdInitProc;
#endif

#ifndef NO_COMPARE
extern Tcl_AppInitProc Blt_CompareCmdInitProc;
#endif

#endif /*_BLT_TCL_INIT_H*/
